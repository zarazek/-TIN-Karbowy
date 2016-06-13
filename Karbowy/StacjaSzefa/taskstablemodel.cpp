#include "taskstablemodel.h"
#include "commongui.h"
#include "predefinedqueries.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QDebug>
#include <assert.h>

static const char* selectQ =
"SELECT t.id, t.title, t.description, t.status, MIN(et.finished), MAX(et.finished), SUM(et.time_spent)\n"
"FROM Tasks AS t LEFT JOIN EmployeesTasks AS et ON t.id = et.task\n"
"WHERE et.assignment_active = 1 OR et.assignment_active IS NULL\n"
"GROUP BY t.id\n";
static const char* addTaskQ = "INSERT INTO Tasks(title, description) VALUES(?, 'Opis zadania')";
static const char* setTitleQ = "UPDATE Tasks SET title = ? WHERE id = ?";
static const char* setDescriptionQ = "UPDATE Tasks SET description = ? WHERE id = ?";
static const char* setStatusQ = "UPDATE Tasks SET status = ? WHERE id = ?";

TasksTableModel::TasksTableModel(QObject* parent) :
    QSqlQueryModel(parent)
{
    refresh();
}

Qt::ItemFlags TasksTableModel::flags(const QModelIndex& index) const
{
    if (! index.isValid())
    {
        return 0;
    }

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (index.column())
    {
    case TASK_COLUMN_TITLE:
        flags |= Qt::ItemIsEditable;
        switch (record(index.row()).field(TASK_COLUMN_STATUS).value().toInt())
        {
        case TASK_STATE_ACTIVE:
        case TASK_STATE_CANCELED:
            flags |= Qt::ItemIsUserCheckable;
            break;
        case TASK_STATE_FINISHED:
            break;
        default:
            assert(false);
        }
        break;
    case TASK_COLUMN_DESC:
        flags |= Qt::ItemIsEditable;
        break;
    }
    return flags;
}

static QString formatTime(Duration duration)
{
    int hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    duration -= std::chrono::hours(hours);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    duration -= std::chrono::minutes(minutes);
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

QVariant TasksTableModel::data(const QModelIndex& item, int role) const
{
    QVariant v = QSqlQueryModel::data(item, role);
    if (item.column() == TASK_COLUMN_TITLE && role == Qt::CheckStateRole)
    {
        switch (record(item.row()).field(TASK_COLUMN_STATUS).value().toInt())
        {
        case TASK_STATE_ACTIVE:
            v = Qt::Checked;
            break;
        case TASK_STATE_CANCELED:
            v = Qt::Unchecked;
            break;
        default:
            v = QVariant();
        }
    }
    else if (item.column() == TASK_COLUMN_STATUS && role == Qt::DisplayRole)
    {
        QSqlRecord rec = record(item.row());
        switch (rec.field(TASK_COLUMN_STATUS).value().toInt())
        {
        case TASK_STATE_ACTIVE:
            if (rec.field(TASK_COLUMN_ALL_FINISHED).value().toBool())
            {
                v = "zakończone";
            }
            else if (rec.field(TASK_COLUMN_SOME_FINISHED).value().toBool())
            {
                v = "częściowo zakończone";
            }
            else if (rec.field(TASK_COLUMN_TOTAL_TIME).value().toInt() > 0)
            {
                v = "rozpoczęte";
            }
            else
            {
                v = "nierozpoczęte";
            }
            break;
        case TASK_STATE_FINISHED:
            v = "zakończone";
            break;
        case TASK_STATE_CANCELED:
            v = "anulowane";
            break;
        default:
            assert(false);
        }
    }
    else if (item.column() == TASK_COLUMN_TOTAL_TIME && role == Qt::DisplayRole)
    {
        int numOfSeconds;
        bool ok = false;
        if (v.isNull())
        {
            numOfSeconds = 0;
            ok = true;
        }
        else
        {
            numOfSeconds = v.toInt(&ok);
        }
        assert(ok);
        v = formatTime(std::chrono::seconds(numOfSeconds));
    }

    return v;
}

static void logInvalidEdit(int role, const QModelIndex& index, const QVariant data)
{
    qDebug() << "invalid set data: role = " << role
             << " row = "<< index.row()
             << " column = " << index.column()
             << " data = " << data;
}

bool TasksTableModel::setData(const QModelIndex& index, const QVariant& data, int role)
{
    int key = record(index.row()).field(TASK_COLUMN_ID).value().toInt();

    bool ok = false;
    switch (role)
    {
    case Qt::CheckStateRole:
        switch (index.column())
        {
        case TASK_COLUMN_TITLE:
            ok = setActive(key, data.toBool());
            break;
        default:
            logInvalidEdit(role, index, data);
        }
        break;
    case Qt::EditRole:
        switch (index.column())
        {
        case TASK_COLUMN_TITLE:
            ok = setTitle(key, data.toString());
            break;
        case TASK_COLUMN_DESC:
            ok = setDescription(key, data.toString());
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

void TasksTableModel::addRow()
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    int cnt = rowCount();
    q.prepare(addTaskQ);
    q.addBindValue("Zadanie nr " + QString::number(cnt));
    if (execQuery(q))
    {
        refresh();
    }
}

bool TasksTableModel::setActive(int key, bool active)
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    q.prepare(setStatusQ);
    q.addBindValue(active ? TASK_STATE_ACTIVE : TASK_STATE_CANCELED);
    q.addBindValue(key);
    return execQuery(q);
}

bool TasksTableModel::setTitle(int key, const QString& title)
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    q.prepare(setTitleQ);
    q.addBindValue(title);
    q.addBindValue(key);
    return execQuery(q);
}

bool TasksTableModel::setDescription(int key, const QString& desc)
{
    QSqlQuery q(QSqlDatabase::database("KarbowyDb"));
    q.prepare(setDescriptionQ);
    q.addBindValue(desc);
    q.addBindValue(key);
    return execQuery(q);
}

void TasksTableModel::refresh()
{
    clear();
    setQuery(selectQ, QSqlDatabase::database("KarbowyDb"));
    showSqlError(lastError());
    setHeaderData(TASK_COLUMN_TITLE, Qt::Horizontal, "Tytuł");
    setHeaderData(TASK_COLUMN_DESC, Qt::Horizontal, "Opis");
    setHeaderData(TASK_COLUMN_STATUS, Qt::Horizontal, "Status");
    setHeaderData(TASK_COLUMN_TOTAL_TIME, Qt::Horizontal, "Czas sumaryczny");
}
