#ifndef EMPLOYEETABLEMODEL_H
#define EMPLOYEETABLEMODEL_H

#include <QSqlQueryModel>
#include <QSqlQuery>

class EmployeeTableModel : public QSqlQueryModel
{
    Q_OBJECT;
public:
    EmployeeTableModel(QObject* parent = nullptr);

//    int columnCount(const QModelIndex& index = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override;
//    void sort(int column, Qt::SortOrder order) override;

    void addRow();
//    void showActive(bool v);
//    void showInactive(bool v);
private:
    enum
    {
        LOGIN_COLUMN = 0,
        PASSWORD_COLUMN = 1,
        NAME_COLUMN = 2,
        ENABLED_COLUMN = 3
    };
    int _editableLogin;
//    int _sortingColumn;
//    Qt::SortOrder _sortingOrder;
//    bool _showActive;
//    bool _showInactive;

    bool setLogin(const QString& oldLogin, const QString& newLogin);
    bool setPassword(const QString& login, const QString& password);
    bool setName(const QString& login, const QString& name);
    bool setActive(const QString& login, bool active);
    void refresh();
};

#endif // EMPLOYEETABLEMODEL_H
