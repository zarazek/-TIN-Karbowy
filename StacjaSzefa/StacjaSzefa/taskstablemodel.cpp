#include "taskstablemodel.h"
#include "commongui.h"
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <assert.h>

static const char* selectQ =
"SELECT t.id, t.title, t.description, t.status, MIN(et.finished), MAX(et.finished), SUM(et.time_spent)\n"
"FROM Tasks AS t JOIN EmployeesTasks AS et ON t.id = et.task\n"
"WHERE et.assignment_active = 1\n"
"GROUP BY t.id\n";

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

    return v;
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
