#ifndef TASKTABLEMODEL_H
#define TASKTABLEMODEL_H

#include <QAbstractTableModel>
#include <memory>
#include "task.h"
#include "logentry.h"

class ClientTask;

class TaskTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum ColumnIndex
    {
        ColumnIndex_ID,
        ColumnIndex_TITLE,
        ColumnIndex_DESCRIPTION,
        ColumnIndex_TIME_SPENT,
        COLUMN_COUNT
    };

    TaskTableModel(QObject* parent);

    int id(size_t rowidx) const;
    QString title(size_t rowIdx) const;
    QString description(size_t rowIdx) const;
    QString timeSpent(size_t rowIdx) const;
    bool active(size_t rowIdx) const;

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void setEmployeeId(int employeeId);
    void startWork(size_t idx);
    void workCheckpoint(size_t idx);
    void pauseWork(size_t idx);
    void finishWork(size_t idx);
    void refresh();

signals:
    void taskActivated(size_t rowIdx);
    void taskDeactivated(size_t rowIdx);

private:
    static const int INVALID_EMPLOYEE_ID = -1;

    int _employeeId;
    std::vector<std::unique_ptr<ClientTask> > _tasks;

    void updateDuration(ClientTask& task, const Timestamp& newCheckpoint, bool finished = false);
    void deactivateTask(ClientTask& task);
    void addLogEntry(LogEntryType type, const ClientTask& task);
};

#endif // TASKTABLEMODEL_H
