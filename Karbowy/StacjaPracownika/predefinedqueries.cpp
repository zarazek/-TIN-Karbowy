#include "predefinedqueries.h"
#include <vector>

static Database *db = nullptr;
static std::vector<QueryBase*> queries;

static const char* createEmployeesTable =
"CREATE TABLE IF NOT EXISTS Employees (\n"
"  id        INTEGER PRIMARY KEY AUTOINCREMENT,\n"
"  login     TEXT UNIQUE NOT NULL)\n";

static const char* createTasksTable =
"CREATE TABLE IF NOT EXISTS Tasks (\n"
"  id          INTEGER PRIMARY KEY\n,"
"  title       TEXT NOT NULL,\n"
"  description TEXT NOT NULL)\n";

static const char* createEmployeesTasksTable =
"CREATE TABLE IF NOT EXISTS EmployeesTasks (\n"
"  employee          REFERENCES Employees(id),\n"
"  task              REFERENCES Tasks(id),\n"
"  time_spent        INTEGER NOT NULL,\n"
"  finished          BOOL NOT NULL DEFAULT FALSE,\n"
"  PRIMARY KEY (employee, task))\n";

static const char* createLogsTable =
"CREATE TABLE IF NOT EXISTS Logs (\n"
"  type INTEGER NOT NULL,\n"
"  employee REFERENCES Employees(login),\n"
"  timestamp DATETIME NOT NULL,\n"
"  task INTEGER)\n";

static const char* createUuidTable =
"CREATE TABLE IF NOT EXISTS Uuid (\n"
"  uuid TEXT PRIMARY KEY)\n";

static const char *commands[] = {
    createEmployeesTable,
    createTasksTable,
    createEmployeesTasksTable,
    createLogsTable,
    createUuidTable,
};

void initializeDatabase()
{
    static const char *dbFileName = "StacjaPracownika.db";

    db = new Database(dbFileName);
    for (const char* txt : commands)
    {
        Command<> cmd(*db, txt);
        cmd.execute();
    }
}

void shutdownDatabase()
{
    for (QueryBase* query : queries)
    {
        delete query;
    }
    queries.clear();
    delete db;
    db = nullptr;
}

Query<std::string>&
retrieveUuidQ()
{
    static const char* txt = "SELECT uuid FROM Uuid";
    static Query<std::string> *query = nullptr;

    if (! query)
    {
        query = new Query<std::string>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<std::string>&
insertUuidC()
{
    static const char* txt = "INSERT INTO Uuid(uuid) VALUES (?)";
    static Command<std::string> *command = nullptr;

    if (! command)
    {
        command = new Command<std::string>(*db, txt);
        queries.push_back(command);
    }
    return *command;
}

Query<int, std::string>&
findUserIdByLoginQ()
{
    static const char* txt = "SELECT id FROM Employees WHERE login = ?";
    static Query<int, std::string> *query = nullptr;

    if (! query)
    {
        query = new Query<int, std::string>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<std::string>&
insertUserC()
{
    static const char* txt = "INSERT INTO Employees(login) VALUES (?)";
    static Command<std::string> *command = nullptr;

    if (! command)
    {
        command = new Command<std::string>(*db, txt);
        queries.push_back(command);
    }
    return *command;
}

Command<int>&
deleteTaskAssociationsForUserC()
{
    static const char* txt = "DELETE FROM EmployeesTasks WHERE employee = ?";
    static Command<int>* query = nullptr;

    if (! query)
    {
        query = new Command<int>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<int, std::string, std::string>&
insertTaskC()
{
    static const char* txt = "INSERT OR REPLACE INTO Tasks (id, title, description)\n"
                             "VALUES (?, ?, ?)\n";
    static Command<int, std::string, std::string>* query = nullptr;

    if (! query)
    {
        query = new Command<int, std::string, std::string>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<int, int, int>&
insertTaskAssociationC()
{
    static const char* txt = "INSERT INTO EmployeesTasks(employee, task, time_spent)\n"
                             "VALUES (?, ?, ?)\n";
    static Command<int, int, int>* query = nullptr;

    if (! query)
    {
        query = new Command<int, int, int>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}
