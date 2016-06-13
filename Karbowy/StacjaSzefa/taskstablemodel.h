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
    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override;
    void addRow();
    void refresh();
private:
    bool setActive(int key, bool active);
    bool setTitle(int key, const QString& title);
    bool setDescription(int key, const QString& desc);
};

#endif // TASKSTABLEMODEL_H
