#include "predefinedqueries.h"
#include "task.h"
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

Command<int, int, Duration>&
insertTaskAssociationC()
{
    static const char* txt = "INSERT INTO EmployeesTasks(employee, task, time_spent)\n"
                             "VALUES (?, ?, ?)\n";
    static Command<int, int, Duration>* query = nullptr;

    if (! query)
    {
        query = new Command<int, int, Duration>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Query<std::unique_ptr<ClientTask>, int>&
findActiveTasksForEmployeeQ()
{
    static const char* txt = "SELECT T.id, T.title, T.description, ET.time_spent\n"
                             "FROM EmployeesTasks AS ET\n"
                             "JOIN Employees AS E ON ET.employee = E.id\n"
                             "JOIN Tasks AS T ON ET.task = T.id\n"
                             "WHERE E.id = ? AND NOT ET.finished\n";
    static Query<std::unique_ptr<ClientTask>, int>* query = nullptr;

    if (! query)
    {
        query = new Query<std::unique_ptr<ClientTask>, int>(*db, txt, std::make_unique<ClientTask, int&&, std::string&&, std::string&&, Duration&&>);
        queries.push_back(query);
    }
    return *query;
}

Command<LogEntryType, int, Timestamp, boost::optional<int> >&
insertLogEntryC()
{
    static const char *txt = "INSERT INTO Logs(type, employee, timestamp, task)\n"
                             "VALUES (?, ?, ?, ?)\n";
    static Command<LogEntryType, int, Timestamp, boost::optional<int> >* query = nullptr;

    if (! query)
    {
        query = new Command<LogEntryType, int, Timestamp, boost::optional<int> >(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<Duration, bool, int, int>&
updateTimeSpentOnTaskC()
{
    static const char *txt = "UPDATE EmployeesTasks\n"
                             "SET time_spent = ?, finished = ?\n"
                             "WHERE employee = ? AND task = ?\n";
    static Command<Duration, bool, int , int>* query = nullptr;

    if (! query)
    {
        query = new Command<Duration, bool, int, int>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}
