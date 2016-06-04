#include "predefinedqueries.h"
#include "employee.h"
#include "protocol.h"
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

Query<std::unique_ptr<Task>, std::string>&
findTasksForLoginQ()
{
    static const char *txt = "SELECT T.id, T.title, T.description, ET.time_spent\n"
                             "FROM EmployeesTasks AS ET\n"
                             "JOIN Employees AS E ON ET.employee = E.login\n"
                             "JOIN Tasks AS t ON ET.task = T.id\n"
                             "WHERE E.login = ? AND T.status = 0 AND ET.assignment_active AND NOT ET.finished\n";
    static Query<std::unique_ptr<Task>, std::string> *query = nullptr;

    if (! query)
    {
        query = new Query<std::unique_ptr<Task>, std::string>(*db, txt,
                                                              std::make_unique<Task, int&&, std::string&&, std::string&&, int&&>);
        queries.push_back(query);
    }
    return *query;
}
