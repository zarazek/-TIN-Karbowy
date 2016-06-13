#include "logprocessor.h"
#include "predefinedqueries.h"
#include "employee.h"
#include <iostream>

std::ostream& operator<<(std::ostream& stream, LogEntryType type)
{
    switch (type)
    {
    case LogEntryType_LOGIN:
        return stream << "LOGIN";
    case LogEntryType_LOGOUT:
        return stream << "LOGOUT";
    case LogEntryType_TASK_START:
        return stream << "TASK START";
    case LogEntryType_TASK_PAUSE:
        return stream << "TASK_PAUSE";
    case LogEntryType_TASK_FINISH:
        return stream << "TASK FINISH";
    default:
        return stream << static_cast<int>(type);
    }
}

LogProcessor::LogProcessor(const std::string &employeeId) :
    _employeeId(employeeId),
    _employeeIsValid(false) { }


void LogProcessor::checkEmployeeId()
{
    auto& query = findEmployeeByLoginQ();
    query.execute(_employeeId);
    std::unique_ptr<Employee> employee;
    if (query.next(employee))
    {
        _employeeIsValid = true;
    }
    else
    {
        std::cerr << "Employee '" << _employeeId << "' doesn't exist" << std::endl;
    }
}

void LogProcessor::process(ServerLogEntry&& entry)
{
    if (! preliminaryValidate(entry))
    {
        _processed.push_back(entry._id);
        return;
    }

    switch (entry._entry._type)
    {
    case LogEntryType_LOGIN:
        if (_loginEntry)
        {
            invalidEntryMsg(entry) << "login before logout (previous login "
                                   << formatTimestamp(_loginEntry->_entry._timestamp)
                                   << " at " << _loginEntry->_clientId << ')' << std::endl;
            _processed.push_back(_loginEntry->_id);
        }
        _loginEntry = std::move(entry);
        break;
    case LogEntryType_LOGOUT:
        if (! _loginEntry)
        {
            invalidEntryMsg(entry) << "logout before login" << std::endl;
        }
        else
        {
            if (_loginEntry->_clientId != entry._clientId)
            {
                          invalidEntryMsg(entry) << "login at different station: "
                                                 << formatTimestamp(_loginEntry->_entry._timestamp)
                                                 << " at " << _loginEntry->_clientId << std::endl;
            }
            _processed.push_back(_loginEntry->_id);
            _loginEntry = boost::none;
        }
        _processed.push_back(entry._id);
        break;
    case LogEntryType_TASK_START:
    {
        int taskId = *entry._entry._taskId;
        auto workStartEntry = _workStartEntrys.find(taskId);
        if (workStartEntry != _workStartEntrys.end())
        {
            ServerLogEntry& prevEntry = workStartEntry->second;
            invalidEntryMsg(entry) << "work start before work stop (previous start "
                                   << formatTimestamp(prevEntry._entry._timestamp)
                                   << " at " << prevEntry._clientId << ')' << std::endl;
            _processed.push_back(prevEntry._id);
            prevEntry = std::move(entry);
        }
        else
        {
            auto res = _workStartEntrys.insert(std::make_pair(taskId, std::move(entry)));
            assert(res.second);
        }
        break;
    }
    case LogEntryType_TASK_PAUSE:
    {
        int taskId = *entry._entry._taskId;
        auto workStartEntry = _workStartEntrys.find(taskId);
        if (workStartEntry != _workStartEntrys.end())
        {
            const ServerLogEntry& prevEntry = workStartEntry->second;
            if (prevEntry._clientId != entry._clientId)
            {
                invalidEntryMsg(entry) << "work start at different station: "
                                       << formatTimestamp(prevEntry._entry._timestamp)
                                       << " at " << prevEntry._clientId << std::endl;
            }
            else
            {
                auto assignment = _assignments.find(taskId);
                assignment->second._timeSpent += entry._entry._timestamp - prevEntry._entry._timestamp;
            }
            _processed.push_back(prevEntry._id);
            _processed.push_back(entry._id);
            _workStartEntrys.erase(workStartEntry);
        }
        else
        {
            invalidEntryMsg(entry) << "work pause before work start" << std::endl;
            _processed.push_back(entry._id);
        }
        break;
    }
    case LogEntryType_TASK_FINISH:
    {
        int taskId = *entry._entry._taskId;
        auto workStartEntry = _workStartEntrys.find(taskId);
        if (workStartEntry != _workStartEntrys.end())
        {
            const ServerLogEntry& prevEntry = workStartEntry->second;
            if (prevEntry._clientId != entry._clientId)
            {
                invalidEntryMsg(entry) << "work start at different station (start "
                                       << formatTimestamp(prevEntry._entry._timestamp)
                                       << " at " << prevEntry._clientId << std::endl;
            }
            else
            {
                auto assignment = _assignments.find(taskId);
                assignment->second._timeSpent += entry._entry._timestamp - prevEntry._entry._timestamp;
                assignment->second._finished = true;
            }
            _processed.push_back(prevEntry._id);
            _processed.push_back(entry._id);
            _workStartEntrys.erase(workStartEntry);
        }
        else
        {
            invalidEntryMsg(entry) << "work finish before work start" << std::endl;
            _processed.push_back(entry._id);
        }
        break;
    }
    }
}

void LogProcessor::finish()
{
    auto& setProcessed = setLogEntryToProcessedC();
    for (int entryId : _processed)
    {
        setProcessed.execute(entryId);
    }
    auto& updateAssignment = updateEmployeeTaskStatusC();
    for (const auto& assignment : _assignments)
    {
        updateAssignment.execute(assignment.second._finished, assignment.second._timeSpent, _employeeId, assignment.first);
    }
}

bool LogProcessor::preliminaryValidate(const ServerLogEntry &entry)
{
    if (entry._entry._userId != _employeeId)
    {
        invalidEntryMsg(entry) << "invalid employee (" << entry._entry._userId
                               << " instead of " << _employeeId << ')' << std::endl;
        return false;
    }
    if (! _employeeIsValid)
    {
        return false;
    }
    if (_previousTimestamp)
    {
        if (*_previousTimestamp > entry._entry._timestamp)
        {
            invalidEntryMsg(entry) << "timestamp not in order (previous timestamp "
                                   << formatTimestamp(*_previousTimestamp) << ')' << std::endl;
            return false;
        }
    }
    _previousTimestamp = entry._entry._timestamp;
    switch (entry._entry._type)
    {
    case LogEntryType_LOGIN:
    case LogEntryType_LOGOUT:
        if (entry._entry._taskId)
        {
            invalidEntryMsg(entry) << " unexpected task id" << std::endl;
            return false;
        }
        break;
    case LogEntryType_TASK_START:
    case LogEntryType_TASK_PAUSE:
    case LogEntryType_TASK_FINISH:
        if (! checkTaskId(entry))
        {
            return false;
        }
        break;
    default:
        invalidEntryMsg(entry) << " invalid type: " << static_cast<int>(entry._entry._type) << std::endl;
        return false;
    }
    return true;
}

bool LogProcessor::checkTaskId(const ServerLogEntry& entry)
{
    if (! entry._entry._taskId)
    {
        invalidEntryMsg(entry) << " task id missing" << std::endl;
        return false;
    }
    const AssignmentStatus* assignment = getAssignment(entry);
    if (! assignment)
    {
        return false;
    }
    if (assignment->_finished)
    {
        invalidEntryMsg(entry) << "employee already finished this task, but processing anyway" << std::endl;
    }
    return true;
}

const AssignmentStatus* LogProcessor::getAssignment(const ServerLogEntry& entry)
{
    int taskId = *entry._entry._taskId;
    auto found = _assignments.find(taskId);
    if (found != _assignments.end())
    {
        return &found->second;
    }
    else
    {
        auto& query = findTaskStatusQ();
        query.execute(_employeeId, taskId);
        TaskStatus status;
        if (! query.next(status))
        {
            invalidEntryMsg(entry) << "invalid task id " << taskId << std::endl;
            return nullptr;
        }
        if (! status._assignment)
        {
            invalidEntryMsg(entry) << "employee was never assigned to this task" << std::endl;
            return nullptr;
        }
        auto res = _assignments.insert(std::make_pair(taskId, *status._assignment));
        assert(res.second);
        return &res.first->second;
    }
}

std::ostream& LogProcessor::invalidEntryMsg(const ServerLogEntry &entry)
{
    std::cerr << "Invalid log entry: "
              << formatTimestamp(entry._entry._timestamp)
              << ' ' << entry._entry._userId
              << " at " << entry._clientId
              << ": " << entry._entry._type;
    if (entry._entry._taskId)
    {
        std::cerr << ' ' << *entry._entry._taskId;
    }
    return std::cerr << ": ";
}
