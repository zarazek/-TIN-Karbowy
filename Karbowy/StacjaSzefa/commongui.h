#ifndef COMMONGUI_H
#define COMMONGUI_H

enum EmployeeColumns
{
    EMPLOYEE_COLUMN_LOGIN = 0,
    EMPLOYEE_COLUMN_PASSWORD = 1,
    EMPLOYEE_COLUMN_NAME = 2,
    EMPLOYEE_COLUMN_ACTIVE = 3
};

enum TasksColumns
{
    TASK_COLUMN_ID = 0,
    TASK_COLUMN_TITLE = 1,
    TASK_COLUMN_DESC = 2,
    TASK_COLUMN_STATUS = 3,
    TASK_COLUMN_ALL_FINISHED = 4,
    TASK_COLUMN_SOME_FINISHED = 5,
    TASK_COLUMN_TOTAL_TIME = 6
};

class QSqlError;
class QSqlQuery;

void showSqlError(const QSqlError& err);
bool execQuery(QSqlQuery& query);

#endif // COMMONGUI_H

