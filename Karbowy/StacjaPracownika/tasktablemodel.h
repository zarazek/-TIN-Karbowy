#ifndef TASKTABLEMODEL_H
#define TASKTABLEMODEL_H

#include <QAbstractTableModel>
#include <memory>
#include "task.h"

class ClientTask;

class TaskTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TaskTableModel(QObject* parent);

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex& index, int role) const override;

public slots:
    void setEmployeeId(int employeeId);
    void refresh();

private:
    enum ColumnIndex
    {
        ColumnIndex_ID,
        ColumnIndex_TITLE,
        ColumnIndex_DESCRIPTION,
        ColumnIndex_TIME_SPENT,
        COLUMN_COUNT
    };

    static const int INVALID_EMPLOYEE_ID = -1;

    int _employeeId;
    std::vector<std::unique_ptr<ClientTask> > _tasks;
};

#endif // TASKTABLEMODEL_H
