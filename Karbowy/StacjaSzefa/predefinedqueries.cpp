#include "predefinedqueries.h"
#include "employee.h"
#include "task.h"
#include "serverlogentry.h"
#include "parse.h"
#include <vector>

static Database *db = nullptr;
static std::vector<QueryBase*> queries;

static const char* createEmployeesTable =
"CREATE TABLE IF NOT EXISTS Employees (\n"
"  login     VARCHAR(8) PRIMARY KEY\n,"
"  password  VARCHAR(15) NOT NULL,\n"
"  name      VARCHAR(100) NOT NULL,\n"
"  active    BOOL NOT NULL DEFAULT 1)\n";

static const char* createTasksTable =
"CREATE TABLE IF NOT EXISTS Tasks (\n"
"  id          INTEGER PRIMARY KEY AUTOINCREMENT\n,"
"  title       VARCHAR(100) NOT NULL UNIQUE,\n"
"  description VARCHAR(1000) NOT NULL,\n"
"  status      INTEGER NOT NULL DEFAULT 0)";

static const char* createEmployeesTasksTable =
"CREATE TABLE IF NOT EXISTS EmployeesTasks (\n"
"  employee          REFERENCES Employees(login),\n"
"  task              REFERENCES Tasks(id),\n"
"  assignment_active BOOL NOT NULL DEFAULT 1,\n"
"  finished          BOOL NOT NULL DEFAULT 0,\n"
"  time_spent        INTEGER NOT NULL DEFAULT 0,\n"
"  PRIMARY KEY (employee, task))\n";

static const char* createClientsTable =
"CREATE TABLE IF NOT EXISTS Clients (\n"
"  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
"  uuid VARCHAR(100) UNIQUE)\n";

static const char* createLogsTable =
"CREATE TABLE IF NOT EXISTS Logs (\n"
"  id        INTEGER PRIMARY KEY AUTOINCREMENT,\n"
"  type      INTEGER NOT NULL,\n"
"  client    REFERENCES Clients(id),\n"
"  employee  REFERENCES Employees(login),\n"
"  timestamp DATETIME NOT NULL,\n"
"  task      INTEGER,\n"
"  processed BOOL NOT NULL DEFAULT 0)\n";

static const char* createUuidTable =
"CREATE TABLE IF NOT EXISTS Uuid (\n"
"  uuid   VARCHAR(100) PRIMARY KEY)\n";

static const char* populateEmployeesTable =
"INSERT OR IGNORE INTO Employees(login, password, name) VALUES\n"
"  ('ybarodzi', 'pass1', 'Yauheni Barodzich'   ),\n"
"  ('mlukashe', 'pass2', 'Mikhail Lukashevich' ),\n"
"  ('tlukashe', 'pass3', 'Tatsiana Lukashevich'),\n"
"  ('wwisniew', 'pass4', 'Wojciech Wiśniewski' )\n";

static const char* populateTasksTable =
"INSERT OR IGNORE INTO Tasks(title, description, status) VALUES\n"
"  ('Pompowanie przedniego koła', 'Zadanie polega na napompowaniu przedniego koła roweru.\nSzybciutko!', 0),\n"
"  ('Pompowanie tylnego koła', 'Zadanie polega na napompowaniu tylnego koła roweru.\nPrędziutko!', 1),\n"
"  ('Smarowanie łańcucha', 'Zadanie polega na nasmarowaniu łańcucha rowerowego.\nMigiem!', 2)";

static const char* populateEmployeesTasksTable =
"INSERT OR IGNORE INTO EmployeesTasks(employee, task)\n"
"          SELECT E.login, T.id\n"
"          FROM Employees AS E JOIN Tasks AS T\n"
"          WHERE E.name = 'Wojciech Wiśniewski' AND T.title = 'Pompowanie przedniego koła'\n"
"UNION ALL SELECT E.login, T.id\n"
"          FROM Employees AS E JOIN Tasks as T\n"
"          WHERE E.name = 'Mikhail Lukashevich' AND T.title = 'Pompowanie tylnego koła'\n"
"UNION ALL SELECT E.login, T.id\n"
"          FROM Employees AS E JOIN Tasks as T\n"
"          WHERE E.name = 'Tatsiana Lukashevich' AND T.title = 'Smarowanie łańcucha'\n"
"UNION ALL SELECT E.login as employee, T.id AS task\n"
"          FROM Employees AS E JOIN Tasks as T\n"
"          WHERE E.name = 'Yauheni Barodzich' AND T.title = 'Smarowanie łańcucha'\n";

static const char *commands[] = {
    createEmployeesTable,
    createTasksTable,
    createEmployeesTasksTable,
    createClientsTable,
    createLogsTable,
    createUuidTable,
    populateEmployeesTable,
    populateTasksTable,
    populateEmployeesTasksTable
};

void initializeDatabase()
{
    static const char *dbFileName = "StacjaSzefa.db";

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

Query<std::unique_ptr<Employee>, std::string>&
findEmployeeByLoginQ()
{
    static const char* txt = "SELECT login, password, name, active FROM Employees WHERE login = ?";
    static Query<std::unique_ptr<Employee>, std::string> *query = nullptr;

    if (! query)
    {
        query = new Query<std::unique_ptr<Employee>, std::string>(*db, txt,
                                                                   std::make_unique<Employee, std::string&&, std::string&&, std::string&&, bool&&>);
        queries.push_back(query);
    }
    return *query;
}

Query<std::unique_ptr<ClientTask>, std::string>&
findTasksForLoginQ()
{
    static const char *txt = "SELECT T.id, T.title, T.description, ET.time_spent\n"
                             "FROM EmployeesTasks AS ET\n"
                             "JOIN Employees AS E ON ET.employee = E.login\n"
                             "JOIN Tasks AS t ON ET.task = T.id\n"
                             "WHERE E.login = ? AND T.status = 0 AND ET.assignment_active = 1 AND ET.finished = 0\n";
    static Query<std::unique_ptr<ClientTask>, std::string> *query = nullptr;

    if (! query)
    {
        query = new Query<std::unique_ptr<ClientTask>, std::string>(*db, txt,
                                                                    std::make_unique<ClientTask, int&&, std::string&&, std::string&&, Duration&&>);
        queries.push_back(query);
    }
    return *query;
}

static boost::optional<Timestamp> parseTimestamp(const boost::optional<std::string>& str)
{
    if (str)
    {
        Timestamp timestamp;
        if (! parse(*str, TimestampToken(timestamp)))
        {
            throw std::runtime_error("Invalid timestamp");
        }
        return boost::optional<Timestamp>(timestamp);
    }
    else
    {
        return boost::none;
    }
}

Command<std::string>&
insertClientUuidC()
{
    static const char* txt = "INSERT OR IGNORE INTO Clients(uuid) VALUES(?)\n";
    static Command<std::string>* query = nullptr;

    if (! query)
    {
        query = new Command<std::string>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Query<int, std::string>&
findClientIdByUuidQ()
{
    static const char* txt = "SELECT id FROM Clients WHERE uuid = ?\n";
    static Query<int, std::string>* query = nullptr;

    if (! query)
    {
        query = new Query<int, std::string>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<int, int, std::string, Timestamp, boost::optional<int> >&
insertLogEntryC()
{
    static const char *txt = "INSERT INTO Logs(type, client, employee, timestamp, task)\n"
                             "VALUES (?, ?, ?, ?, ?)\n";
    static Command<int, int, std::string, Timestamp, boost::optional<int> >* query = nullptr;

    if (! query)
    {
        query = new Command<int, int, std::string, Timestamp, boost::optional<int> >(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Query<boost::optional<Timestamp>, int>&
findLastLogEntryTimeForClientQ()
{
    static const char *txt = "SELECT MAX(timestamp) FROM Logs WHERE client = ?\n";
    static Query<boost::optional<Timestamp>, int> *query = nullptr;

    if (! query)
    {
        query = new Query<boost::optional<Timestamp>, int>(*db, txt,
                                                           parseTimestamp);
        queries.push_back(query);
    }
    return *query;
}

static ServerLogEntry makeServerEmployee(int id,
                                         int type,
                                         int clientId,
                                         std::string&& employeeId,
                                         const Timestamp& timestamp,
                                         boost::optional<int>&& taskId)
{
    return ServerLogEntry
    {
        ._id = id,
        ._clientId = clientId,
        ._entry = LogEntry
        {
            ._type = static_cast<LogEntryType>(type),
            ._timestamp = timestamp,
            ._userId = std::forward<std::string>(employeeId),
            ._taskId = std::forward<boost::optional<int> >(taskId),
        },
    };
}

Query<ServerLogEntry, std::string>&
findUnprocessedLogEntriesForEmployeeQ()
{
    static const char *txt = "SELECT id, type, client, employee, timestamp, task\n"
                             "FROM Logs\n"
                             "WHERE employee = ? AND NOT processed\n"
                             "ORDER BY timestamp\n";
    static Query<ServerLogEntry, std::string>* query = nullptr;

    if (! query)
    {
        query = new Query<ServerLogEntry, std::string>(*db, txt, makeServerEmployee);
        queries.push_back(query);
    }
    return *query;
}

static TaskStatus makeTaskStatus(int id,
                                 const boost::optional<bool>& finished,
                                 const boost::optional<Duration>& timeSpent)
{
    TaskStatus status;
    status._id = id;
    if (finished && timeSpent)
    {
        AssignmentStatus assignment;
        assignment._finished = *finished;
        assignment._timeSpent =  *timeSpent;
        status._assignment = std::move(assignment);
    }
    else if (! finished && ! timeSpent)
    {
        status._assignment = boost::none;
    }
    else
    {
        assert(false);
    }
    return status;
}

Query<TaskStatus, std::string, int>& findTaskStatusQ()
{
    static const char *txt = "SELECT T.id, ET.finished, ET.time_spent\n"
                             "FROM Tasks AS T\n"
                             "LEFT JOIN EmployeesTasks AS ET ON T.id = ET.task\n"
                             "WHERE ET.employee = ? AND T.id = ?\n";
    static Query<TaskStatus, std::string, int>* query = nullptr;

    if (! query)
    {
        query = new Query<TaskStatus, std::string, int>(*db, txt, makeTaskStatus);
        queries.push_back(query);
    }
    return *query;
}

Command<int>& setLogEntryToProcessedC()
{
    static const char *txt = "UPDATE Logs SET processed = 1 WHERE id = ?\n";
    static Command<int>* query = nullptr;

    if (! query)
    {
        query = new Command<int>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<bool, Duration, std::string, int>& updateEmployeeTaskStatusC()
{
    static const char *txt = "UPDATE EmployeesTasks\n"
                             "SET finished = ?, time_spent = ?\n"
                             "WHERE employee = ? AND  task = ?\n";
    static Command<bool, Duration, std::string, int>* query = nullptr;

    if (! query)
    {
        query = new Command<bool, Duration, std::string, int>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Query<std::string>&
findAllActiveEmployeesQ()
{
    static const char *txt = "SELECT login FROM Employees WHERE active\n";
    static Query<std::string>* query = nullptr;

    if (! query)
    {
        query = new Query<std::string>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

static TaskAssignedEmployee makeTaskAssignedEmployee(std::string&& employeeId, bool assignementActive)
{
    return TaskAssignedEmployee { std::forward<std::string>(employeeId), assignementActive };
}

Query<TaskAssignedEmployee, int>&
findAllEmployeesAssignedToTaskQ()
{
    static const char *txt = "SELECT E.login, ET.assignment_active\n"
                             "FROM Employees AS E\n"
                             "JOIN EmployeesTasks AS ET ON E.login = ET.employee\n"
                             "WHERE ET.task = ? AND E.active\n";
    static Query<TaskAssignedEmployee, int>* query = nullptr;

    if (! query)
    {
        query = new Query<TaskAssignedEmployee, int>(*db, txt, makeTaskAssignedEmployee);
        queries.push_back(query);
    }
    return *query;
}

Command<bool, std::string, int>&
changeEmployeeTaskAssignmentStatusC()
{
    static const char *txt = "UPDATE EmployeesTasks SET assignment_active = ? WHERE employee = ? AND task = ?\n";
    static Command<bool, std::string, int>* query = nullptr;

    if (! query)
    {
        query = new Command<bool, std::string, int>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}

Command<std::string, int>&
addEmployeeToTaskC()
{
    static const char *txt = "INSERT INTO EmployeesTasks(employee, task) VALUES(?, ?)\n";
    static Command<std::string, int>* query = nullptr;

    if (! query)
    {
        query = new Command<std::string, int>(*db, txt);
        queries.push_back(query);
    }
    return *query;
}
