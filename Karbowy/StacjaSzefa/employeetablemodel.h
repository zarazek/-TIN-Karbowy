#ifndef EMPLOYEETABLEMODEL_H
#define EMPLOYEETABLEMODEL_H

#include <QSqlQueryModel>
#include <QSqlQuery>

class EmployeeTableModel : public QSqlQueryModel
{
    Q_OBJECT;
public:
    EmployeeTableModel(QObject* parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override;

    void addRow();
private:
    int _editableLogin;

    bool setLogin(const QString& oldLogin, const QString& newLogin);
    bool setPassword(const QString& login, const QString& password);
    bool setName(const QString& login, const QString& name);
    bool setActive(const QString& login, bool active);
    void refresh();
};

#endif // EMPLOYEETABLEMODEL_H
