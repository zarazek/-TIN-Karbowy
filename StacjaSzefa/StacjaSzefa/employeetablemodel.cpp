#include "employeetablemodel.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>

static const char* selectQ = "SELECT login, password, name, active FROM Employees";
static const char* setLoginQ = "UPDATE Employees SET login = ? WHERE login = ?";
static const char* setPasswordQ = "UPDATE Employees SET password = ? WHERE login = ?";
static const char* setNameQ = "UPDATE Employees SET name = ? WHERE login = ?";
static const char* setActiveQ = "UPDATE Employees SET active = ? WHERE login = ?";
static const char* addEmployeeQ = "INSERT INTO Employees(login, password, name) VALUES(?, 'hasło', ?)";

static bool execQuery(QSqlQuery& q)
{
    if (! q.exec())
    {
        QMessageBox::critical(0, "Błąd bazy danych",
                              q.lastError().text(),
                              QMessageBox::Ok);
        return false;
    }
    return true;
}

EmployeeTableModel::EmployeeTableModel(QObject* parent) :
    QSqlQueryModel(parent),
    _editableLogin(-1)
//    _sortingColumn(LOGIN_COLUMN),
//    _sortingOrder(Qt::AscendingOrder),
//    _showActive(true),
//    _showInactive(true)
{

    refresh();
}

//int EmployeeTableModel::columnCount(const QModelIndex&) const
//{
//    return 3;
//}

QVariant EmployeeTableModel::data(const QModelIndex& item, int role) const
{
    QVariant v = QSqlQueryModel::data(item, role);
    if (item.column() == LOGIN_COLUMN && role == Qt::CheckStateRole)
    {
        bool value = record(item.row()).field(ENABLED_COLUMN).value().toBool();
        v = value ? Qt::Checked : Qt::Unchecked;
    }
    return v;
}

Qt::ItemFlags EmployeeTableModel::flags(const QModelIndex& index) const
{
    if (! index.isValid())
    {
        return 0;
    }

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (index.column())
    {
    case LOGIN_COLUMN:
        flags |= Qt::ItemIsUserCheckable;
        if (_editableLogin == index.row())
        {
            flags |= Qt::ItemIsEditable;
        }
        break;
    case PASSWORD_COLUMN:
        flags |= Qt::ItemIsEditable;
        break;
    case NAME_COLUMN:
        flags |=  Qt::ItemIsEditable;
        break;
    }
    return flags;
}

static void logInvalidEdit(int role, const QModelIndex& index, const QVariant data)
{
    qDebug() << "invalid set data: role = " << role
             << " row = "<< index.row()
             << " column = " << index.column()
             << " data = " << data;
}

bool EmployeeTableModel::setData(const QModelIndex& index, const QVariant& data, int role)
{
    QString key = record(index.row()).field(LOGIN_COLUMN).value().toString();

    bool ok = false;
    switch (role)
    {
    case Qt::CheckStateRole:
        switch (index.column())
        {
        case LOGIN_COLUMN:
            ok = setActive(key, data.toBool());
            break;
        default:
            logInvalidEdit(role, index, data);
        }
        break;
    case Qt::EditRole:
        switch (index.column())
        {
        case LOGIN_COLUMN:
            ok = setLogin(key, data.toString());
            if (ok)
            {
                _editableLogin = -1;
            }
            break;
        case PASSWORD_COLUMN:
            ok = setPassword(key, data.toString());
            break;
        case NAME_COLUMN:
            ok = setName(key, data.toString());
            break;
        default:
            logInvalidEdit(role, index, data);
        }
        break;
    }

    if (ok)
    {
        refresh();
    }

    return ok;
}

//void EmployeeTableModel::sort(int column, Qt::SortOrder order)
//{
//    _sortingColumn = column;
//    _sortingOrder = order;
//    refresh();
//}

void EmployeeTableModel::addRow()
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    int cnt = rowCount();
    q.prepare(addEmployeeQ);
    q.addBindValue("p" + QString::number(cnt));
    q.addBindValue("Pracownik nr " + QString::number(cnt));
    if (execQuery(q))
    {
        _editableLogin = cnt;
        refresh();
    }
}

//void EmployeeTableModel::showActive(bool v)
//{
//    if (v != _showActive)
//    {
//        _showActive = v;
//        refresh();
//    }
//}

//void EmployeeTableModel::showInactive(bool v)
//{
//    if (v != _showInactive)
//    {
//        _showInactive = v;
//        refresh();
//    }
//}

bool EmployeeTableModel::setLogin(const QString& oldLogin, const QString& newLogin)
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    q.prepare(setLoginQ);
    q.addBindValue(newLogin);
    q.addBindValue(oldLogin);
    return execQuery(q);
}

bool EmployeeTableModel::setPassword(const QString& login, const QString& password)
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    q.prepare(setPasswordQ);
    q.addBindValue(password);
    q.addBindValue(login);
    return execQuery(q);
}

bool EmployeeTableModel::setName(const QString& login, const QString& name)
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    q.prepare(setNameQ);
    q.addBindValue(name);
    q.addBindValue(login);
    return execQuery(q);
}

bool EmployeeTableModel::setActive(const QString& login, bool active)
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    q.prepare(setActiveQ);
    q.addBindValue(active);
    q.addBindValue(login);
    return execQuery(q);
}

void EmployeeTableModel::refresh()
{
    clear();
    QString txt(selectQ);
//    if (_showActive && ! _showInactive)
//    {
//        txt += " WHERE active IS TRUE";
//    }
//    else if (! _showActive && _showInactive)
//    {
//        txt += " WHERE active IS FALSE";
//    }
//    else if (! _showActive && !_showInactive)
//    {
//        txt += " WHERE 1 = 0";
//    }

//    switch (_sortingColumn)
//    {
//    case LOGIN_COLUMN:
//        txt += " ORDER BY login";
//        break;
//    case PASSWORD_COLUMN:
//        txt += " ORDER BY password";
//        break;
//    case NAME_COLUMN:
//        txt += " ORDER BY name";
//        break;
//    }
//    switch (_sortingOrder)
//    {
//    case Qt::AscendingOrder:
//        txt += " ASC";
//        break;
//    case Qt::DescendingOrder:
//        txt += " DESC";
//    }

    setQuery(txt, QSqlDatabase::database("KarbowyDb"));
    setHeaderData(LOGIN_COLUMN, Qt::Horizontal, "Login");
    setHeaderData(PASSWORD_COLUMN, Qt::Horizontal, "Hasło");
    setHeaderData(NAME_COLUMN, Qt::Horizontal, "Imię i nazwisko");
}
