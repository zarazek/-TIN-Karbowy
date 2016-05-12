#ifndef TASKSTABLEMODEL_H
#define TASKSTABLEMODEL_H

#include <QSqlQueryModel>

class TasksTableModel : public QSqlQueryModel
{
    Q_OBJECT;
public:
    TasksTableModel(QObject* parent = nullptr);
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
//    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override;
private:
    enum
    {
        TASK_STATE_ACTIVE = 0,
        TASK_STATE_FINISHED = 1,
        TASK_STATE_CANCELED = 2
    };

    void refresh();
};

#endif // TASKSTABLEMODEL_H
