#include "commongui.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

void showSqlError(const QSqlError& err)
{
    if (err.isValid())
    {
        QMessageBox::critical(0, "Błąd bazy danych",
                              err.text(),
                              QMessageBox::Ok);
    }
}

bool execQuery(QSqlQuery& q)
{
    if (! q.exec())
    {
        showSqlError(q.lastError());
        return false;
    }
    return true;
}


