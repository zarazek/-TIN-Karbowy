#include "tasktablemodel.h"
#include "predefinedqueries.h"
#include "stringandtimeutils.h"
#include <QMessageBox>
#include <iostream>

TaskTableModel::TaskTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    _employeeId(INVALID_EMPLOYEE_ID) { }

int TaskTableModel::id(size_t rowIdx) const
{
    return _tasks.at(rowIdx)->_id;
}

QString TaskTableModel::title(size_t rowIdx) const
{
    return QString(_tasks.at(rowIdx)->_title.c_str());
}

QString TaskTableModel::description(size_t rowIdx) const
{
    return join(_tasks.at(rowIdx)->_description);
}

QString TaskTableModel::timeSpent(size_t rowIdx) const
{
    return formatTime(_tasks.at(rowIdx)->_timeSpent);
}

bool TaskTableModel::active(size_t rowIdx) const
{
    return _tasks.at(rowIdx)->_workingNow;
}

int TaskTableModel::rowCount(const QModelIndex&) const
{
    return _tasks.size();
}

int TaskTableModel::columnCount(const QModelIndex&) const
{
    return COLUMN_COUNT;
}

QVariant TaskTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        switch (orientation)
        {
        case Qt::Horizontal:
            switch (section)
            {
            case ColumnIndex_ID:
                return QVariant("ID");
            case ColumnIndex_TITLE:
                return QVariant("Tytuł");
            case ColumnIndex_DESCRIPTION:
                return QVariant("Opis");
            case ColumnIndex_TIME_SPENT:
                return QVariant("Spędzony czas");
            }
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant TaskTableModel::data(const QModelIndex& index, int role) const
{
    size_t rowIdx = index.row();
    if (rowIdx < _tasks.size())
    {
        switch (role)
        {
        case Qt::DisplayRole:
            switch (index.column())
            {
            case ColumnIndex_ID:
                return QVariant(id(rowIdx));
            case ColumnIndex_TITLE:
                return QVariant(title(rowIdx));
            case ColumnIndex_DESCRIPTION:
                return QVariant(description(rowIdx));
            case ColumnIndex_TIME_SPENT:
                return QVariant(timeSpent(rowIdx));
            }
        }
    }

    return QVariant();
}

void TaskTableModel::setEmployeeId(int employeeId)
{
    if (employeeId != _employeeId)
    {
        _employeeId = employeeId;
        refresh();
    }
}

void TaskTableModel::refresh()
{
    if (_employeeId != INVALID_EMPLOYEE_ID)
    {
        Query<std::unique_ptr<ClientTask>, int>& query = findActiveTasksForEmployeeQ();
        query.execute(_employeeId);
        std::vector<std::unique_ptr<ClientTask> > newTasks;
        std::unique_ptr<ClientTask> task;
        while (query.next(task))
        {
            newTasks.emplace_back(std::move(task));
        }
        beginResetModel();
        _tasks = std::move(newTasks);
        endResetModel();
    }
}

void TaskTableModel::startWork(size_t idx)
{
    Timestamp newCheckpoint = Clock::now();
    ClientTask& task = *_tasks.at(idx);
    assert(! task._workingNow);
    task._lastCheckpoint = newCheckpoint;
    addLogEntry(LogEntryType_TASK_START, task);
    task._workingNow = true;
    emit taskActivated(idx);
}

void TaskTableModel::workCheckpoint(size_t rowIdx)
{
    Timestamp newCheckpoint = Clock::now();
    ClientTask& task = *_tasks.at(rowIdx);
    updateDuration(task, newCheckpoint);
    QModelIndex idx = index(rowIdx, ColumnIndex_TIME_SPENT);
    emit dataChanged(idx, idx);
}

void TaskTableModel::pauseWork(size_t rowIdx)
{
    Timestamp newCheckpoint = Clock::now();
    ClientTask& task = *_tasks.at(rowIdx);
    updateDuration(task, newCheckpoint);
    addLogEntry(LogEntryType_TASK_PAUSE, task);
    task._workingNow = false;
    QModelIndex idx = index(rowIdx, ColumnIndex_TIME_SPENT);
    emit dataChanged(idx, idx);
    emit taskDeactivated(rowIdx);
}

void TaskTableModel::finishWork(size_t idx)
{
    Timestamp newCheckpoint = Clock::now();
    ClientTask& task = *_tasks.at(idx);
    if (task._workingNow)
    {
        updateDuration(task, newCheckpoint, true);
        addLogEntry(LogEntryType_TASK_FINISH, task);
        task._workingNow = false;
        QModelIndex idxx = index(idx, ColumnIndex_TIME_SPENT);
        emit dataChanged(idxx, idxx);
        emit taskDeactivated(idx);
    }
    else
    {
        task._lastCheckpoint = newCheckpoint;
        addLogEntry(LogEntryType_TASK_START, task);
        addLogEntry(LogEntryType_TASK_FINISH, task);
    }
    beginRemoveRows(QModelIndex(), static_cast<int>(idx), static_cast<int>(idx));
    _tasks.erase(_tasks.begin() + idx);
    endRemoveRows();
}

void TaskTableModel::updateDuration(ClientTask& task, const Timestamp& newCheckpoint, bool finished)
{
    assert(task._workingNow);
    task._timeSpent += newCheckpoint - task._lastCheckpoint;
    auto& updateTimeSpentCmd = updateTimeSpentOnTaskC();
    updateTimeSpentCmd.execute(task._timeSpent, finished, _employeeId, task._id);
    task._lastCheckpoint = newCheckpoint;
}

void TaskTableModel::addLogEntry(LogEntryType type, const ClientTask& task)
{
    auto& addLogEntryCmd = insertLogEntryC();
    addLogEntryCmd.execute(type, _employeeId, task._lastCheckpoint, boost::optional<int>(task._id));
}
