#include "employeetablemodel.h"
#include "commongui.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>

static const char* selectQ = "SELECT login, password, name, active FROM Employees";
static const char* setLoginQ = "UPDATE Employees SET login = ? WHERE login = ?";
static const char* setPasswordQ = "UPDATE Employees SET password = ? WHERE login = ?";
static const char* setNameQ = "UPDATE Employees SET name = ? WHERE login = ?";
static const char* setActiveQ = "UPDATE Employees SET active = ? WHERE login = ?";
static const char* addEmployeeQ = "INSERT INTO Employees(login, password, name) VALUES(?, 'hasło', ?)";

EmployeeTableModel::EmployeeTableModel(QObject* parent) :
    QSqlQueryModel(parent),
    _editableLogin(-1)
{
    refresh();
}

QVariant EmployeeTableModel::data(const QModelIndex& item, int role) const
{
    QVariant v = QSqlQueryModel::data(item, role);
    if (item.column() == EMPLOYEE_COLUMN_LOGIN && role == Qt::CheckStateRole)
    {
        bool value = record(item.row()).field(EMPLOYEE_COLUMN_ACTIVE).value().toBool();
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
    case EMPLOYEE_COLUMN_LOGIN:
        flags |= Qt::ItemIsUserCheckable;
        if (_editableLogin == index.row())
        {
            flags |= Qt::ItemIsEditable;
        }
        break;
    case EMPLOYEE_COLUMN_PASSWORD:
        flags |= Qt::ItemIsEditable;
        break;
    case EMPLOYEE_COLUMN_NAME:
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
    QString key = record(index.row()).field(EMPLOYEE_COLUMN_LOGIN).value().toString();

    bool ok = false;
    switch (role)
    {
    case Qt::CheckStateRole:
        switch (index.column())
        {
        case EMPLOYEE_COLUMN_LOGIN:
            ok = setActive(key, data.toBool());
            break;
        default:
            logInvalidEdit(role, index, data);
        }
        break;
    case Qt::EditRole:
        switch (index.column())
        {
        case EMPLOYEE_COLUMN_LOGIN:
            ok = setLogin(key, data.toString());
            if (ok)
            {
                _editableLogin = -1;
            }
            break;
        case EMPLOYEE_COLUMN_PASSWORD:
            ok = setPassword(key, data.toString());
            break;
        case EMPLOYEE_COLUMN_NAME:
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
    setQuery(selectQ, QSqlDatabase::database("KarbowyDb"));
    setHeaderData(EMPLOYEE_COLUMN_LOGIN, Qt::Horizontal, "Login");
    setHeaderData(EMPLOYEE_COLUMN_PASSWORD, Qt::Horizontal, "Hasło");
    setHeaderData(EMPLOYEE_COLUMN_NAME, Qt::Horizontal, "Imię i nazwisko");
}
