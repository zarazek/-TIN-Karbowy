#include "clientconnection.h"
#include "protocol.h"
#include "parse.h"
#include "concat.h"
#include "protocolerror.h"
#include "server.h"
#include "predefinedqueries.h"
#include "employee.h"
#include "task.h"
#include "serverlogentry.h"
#include <boost/algorithm/string.hpp>
#include <signal.h>

#include <iostream>

ClientConnection::ClientConnection(Server& server, TcpStream&& stream) :
    _server(server),
    _stream(std::move(stream)),
    _run(false) { }

void ClientConnection::start()
{
    _run = true;
    _thread = std::thread(&ClientConnection::run, this);
}

void ClientConnection::stop()
{
    _run = false;
    pthread_kill(_thread.native_handle(), SIGUSR1);
}

void ClientConnection::waitToFinish()
{
    _thread.join();
}

std::unique_ptr<Employee> ClientConnection::verifyUserId(const std::string& userId)
{
    auto& query = findEmployeeByLoginQ();
    query.execute(userId);
    std::unique_ptr<Employee> employee;
    if (! query.next(employee))
    {
        std::cerr << "Can't find employee id '" << userId << '\'' << std::endl;
        return std::unique_ptr<Employee>();
    }
    if (query.next(employee))
    {
        while (query.next(employee)) { }
        std::cerr << "Found more than one employee with id '" << userId << '\'' << std::endl;
        return std::unique_ptr<Employee>();
    }
    if (! employee->_active)
    {
        std::cerr << "Employee '" << userId << "' is not active" << std::endl;
        return std::unique_ptr<Employee>();
    }
    return employee;
}

bool ClientConnection::initializeConnection()
{
    std::string serverChallenge = receiveServerChallenge(_stream);
    sendServerChallengeResponse(_stream, _server.uuid(), serverChallenge);
    if (! receiveServerChallengeAck(_stream))
    {
        std::cerr << "Server challenge response rejected" << std::endl;
        return false;
    }
    std::string clientChallenge = sendClientChallenge(_stream);
    std::string clientResponse = receiveClientChallengeResponse(_stream);
    bool clientOk = verifyChallengeResponse(_server.uuid(), clientChallenge, clientResponse);
    sendClientChallengeAck(_stream, clientOk);
    if (! clientOk)
    {
        return false;
    }
    std::string clientUuid = receiveClientUuid(_stream);
    std::string userId = receiveLoginRequest(_stream);
    std::unique_ptr<Employee> employee = verifyUserId(userId);
    if (! employee)
    {
        return false;
    }
    std::string loginChallenge = sendLoginChallenge(_stream);
    std::string loginResponse = receiveLoginChallengeResponse(_stream);
    bool loginOk = verifyChallengeResponse(employee->_password, loginChallenge, loginResponse);
    sendLoginChallengeAck(_stream, loginOk);
    if (! loginOk)
    {
        return false;
    }

    auto& insertClientUuid = insertClientUuidC();
    insertClientUuid.execute(clientUuid);
    auto& findClientIdByUuid = findClientIdByUuidQ();
    findClientIdByUuid.execute(clientUuid);
    if (! findClientIdByUuid.next(_clientId))
    {
        throw std::runtime_error("Client id not found after insert");
    }
    if (findClientIdByUuid.next(_clientId))
    {
        throw std::runtime_error("Many client ids found after insert");
    }
    _userId = userId;
    return true;
}

void ClientConnection::handleCommand(const std::string& line)
{
    if (boost::iequals(line, "RETRIEVE TASKS"))
    {
        auto& query = findTasksForLoginQ();
        query.execute(_userId);
        std::unique_ptr<ClientTask> task;
        while (query.next(task))
        {
            _stream.writeLine(concatln("TASK ", task->_id, " TITLE ", quoteString(task->_title), " SPENT ", toSeconds(task->_timeSpent)));
            for (const auto& line : task->_description)
            {
                _stream.writeLine(concatln(line));
            }
            _stream.writeLine("\n");
        }
        _stream.writeLine("END TASKS\n");
    }
    else if (boost::iequals(line, "LOG UPLOAD"))
    {
        auto& lastEntryTimeQ = findLastLogEntryTimeForClientQ();
        lastEntryTimeQ.execute(_clientId);
        boost::optional<Timestamp> lastEntryTime;
        bool res = lastEntryTimeQ.next(lastEntryTime);
        assert(res);
        assert(! lastEntryTimeQ.next(lastEntryTime));
        if (lastEntryTime)
        {
            _stream.writeLine(concatln("LAST ENTRY AT ", formatTimestamp(*lastEntryTime)));
        }
        else
        {
            _stream.writeLine("NO ENTRYS\n");
        }
        int taskId;
        bool loop = true;
        while (loop)
        {
            auto line = _stream.readLine();
            LogEntry entry;
            if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId), " LOGIN"))
            {
                entry._type = LogEntryType_LOGIN;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId), " LOGOUT"))
            {
                entry._type = LogEntryType_LOGOUT;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId),
                           " TASK ", IntToken(taskId), " START"))
            {
                entry._type = LogEntryType_TASK_START;
                entry._taskId = taskId;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId),
                           " TASK ", IntToken(taskId), " PAUSE"))
            {
                entry._type = LogEntryType_TASK_PAUSE;
                entry._taskId = taskId;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId),
                           " TASK ", IntToken(taskId), " FINISH"))
            {
                entry._type = LogEntryType_TASK_FINISH;
                entry._taskId = taskId;
            }
            else if (boost::iequals(line, "END LOG"))
            {
                loop = false;
            }
            else
            {
                throw ProtocolError("Invalid log entry", line);
            }

            if (loop)
            {
                insertLogEntry(entry);
            }
        }
        processLogs();
    }
    else
    {
        throw ProtocolError("Invalid command", line);
    }
}

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

void ClientConnection::insertLogEntry(const LogEntry& entry)
{
    std::cout << __PRETTY_FUNCTION__
              << ' ' << formatTimestamp(entry._timestamp)
              << ' ' << entry._userId
              << ' ' << entry._type;
    if (entry._taskId)
    {
        std::cout << ' ' << *entry._taskId;
    }
    std::cout << std::endl;


    auto& cmd = insertLogEntryC();
    cmd.execute(entry._type, _clientId, entry._userId, entry._timestamp, entry._taskId);
}

void ClientConnection::run()
{
    try
    {
        if (initializeConnection())
        {
            while (_run)
            {
                handleCommand(_stream.readLine());
            }
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Client exception: " << ex.what() << std::endl;
    }
    _server.removeClient(shared_from_this());
}


class LogProcessor
{
public:
    LogProcessor(const std::string& employeeId);
    void process(ServerLogEntry&& entry);
    void finish();
private:
    const std::string& _employeeId;
    boost::optional<Timestamp> _previousTimestamp;
    boost::optional<ServerLogEntry> _loginEntry;
    std::map<int, AssignmentStatus> _assignments;
    std::map<int, ServerLogEntry> _workStartEntrys;
    std::vector<int> _processed;

    bool preliminaryValidate(const ServerLogEntry& entry);
    bool checkTaskId(const ServerLogEntry& entry);
    const AssignmentStatus* getAssignment(const ServerLogEntry& entry);
    static std::ostream& invalidEntryMsg(const ServerLogEntry& entry);
};

LogProcessor::LogProcessor(const std::string &employeeId) :
    _employeeId(employeeId) { }

void LogProcessor::process(ServerLogEntry&& entry)
{
    std::cout << __PRETTY_FUNCTION__
              << ' ' << entry._id
              << ' ' << formatTimestamp(entry._entry._timestamp)
              << ' ' << entry._clientId
              << ' ' << entry._entry._userId
              << ' ' << entry._entry._type;
    if (entry._entry._taskId)
    {
        std::cout << ' ' << *entry._entry._taskId;
    }
    std::cout << std::endl;

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


void ClientConnection::processLogs()
{
    auto& query = findUnprocessedLogEntriesForEmployeeQ();
    query.execute(_userId);
    std::vector<ServerLogEntry> entries;
    ServerLogEntry entry;
    while (query.next(entry))
    {
        entries.push_back(std::move(entry));
    }
    LogProcessor processor(_userId);
    for (auto& e : entries)
    {
        processor.process(std::move(e));
    }
    processor.finish();
}
