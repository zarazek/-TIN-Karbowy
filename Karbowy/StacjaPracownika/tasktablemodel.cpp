#include "tasktablemodel.h"
#include "predefinedqueries.h"
#include <QMessageBox>
#include <chrono>

TaskTableModel::TaskTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    _employeeId(INVALID_EMPLOYEE_ID) { }

int TaskTableModel::rowCount(const QModelIndex&) const
{
    return _tasks.size();
}

int TaskTableModel::columnCount(const QModelIndex&) const
{
    return COLUMN_COUNT;
}

static QString formatTime(int timeInSeconds)
{
    std::chrono::seconds t(timeInSeconds);
    int hours = std::chrono::duration_cast<std::chrono::hours>(t).count();
    t -= std::chrono::hours(hours);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(t).count();
    t -= std::chrono::minutes(minutes);
    int seconds = t.count();
    QString res;
    if (hours > 0)
    {
        res = QString("%02d:02d:02d").arg(hours).arg(minutes).arg(seconds);
    }
    else if (minutes > 0)
    {
        res = QString("%02d:02d").arg(minutes).arg(seconds);
    }
    else
    {
        res = QString("%02d").arg(seconds);
    }
    return res;
}

static QString join(const std::vector<std::string>& lst)
{
    QString res;
    if (! lst.empty())
    {
        res += lst.cbegin()->c_str();
        std::for_each(lst.cbegin() + 1, lst.cend(), [&res](const auto& str){ res += '\n'; res += str.c_str(); });
    }
    return res;
}


QVariant TaskTableModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < _tasks.size())
    {
        const auto& task = *_tasks[index.row()];
        switch (role)
        {
        case Qt::DisplayRole:
            switch (index.column())
            {
            case ColumnIndex_ID:
                return QVariant(task._id);
            case ColumnIndex_TITLE:
                return QVariant(QString(task._title.c_str()));
            case ColumnIndex_DESCRIPTION:
                return QVariant(join(task._description));
            case ColumnIndex_TIME_SPENT:
                return QVariant(formatTime(task._secondsSpent));
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
    try
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
    catch (std::exception& ex)
    {
        QMessageBox::critical(0, "Wyjątek",
                              QString(ex.what()) + "\n\n" +
                              "Kliknij OK aby wyjść.",
                              QMessageBox::Ok);
    }
}
