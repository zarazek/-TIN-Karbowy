#include "mainwindow.h"
#include "database.h"
#include "employee.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

#include <QDebug>

const char* createEmployeesTable =
"CREATE TABLE IF NOT EXISTS Employees (\n"
"  login     VARCHAR(8) PRIMARY KEY\n,"
"  password  VARCHAR(15) NOT NULL,\n"
"  name      VARCHAR(100) NOT NULL,\n"
"  active    BOOL NOT NULL DEFAULT TRUE)";

const char* createTasksTable =
"CREATE TABLE IF NOT EXISTS Tasks (\n"
"  id          INTEGER PRIMARY KEY AUTOINCREMENT\n,"
"  title       VARCHAR(100) NOT NULL UNIQUE,"
"  description VARCHAR(1000) NOT NULL,"
"  status      INTEGER NOT NULL DEFAULT 0)";

const char* createEmployeesTasksTable =
"CREATE TABLE IF NOT EXISTS EmployeesTasks (\n"
"  employee          REFERENCES Employees(login),\n"
"  task              REFERENCES Tasks(id),\n"
"  assignment_acitve BOOL NOT NULL DEFAULT TRUE,\n"
"  finished          BOOL NOT NULL DEFAULT FALSE,\n"
"  time_spent        INTEGER NOT NULL DEFAULT 0,"
"  PRIMARY KEY (employee, task))\n";

const char* populateEmployeesTable =
"INSERT OR IGNORE INTO Employees(login, password, name) VALUES\n"
"  ('ybarodzi', 'pass1', 'Yauheni Barodzich'   ),\n"
"  ('mlukashe', 'pass2', 'Mikhail Lukashevich' ),\n"
"  ('tlukashe', 'pass3', 'Tatsiana Lukashevich'),\n"
"  ('wwisniew', 'pass4', 'Wojciech Wiśniewski' )\n";

const char* populateTasksTable =
"INSERT OR IGNORE INTO Tasks(title, description) VALUES\n"
"  ('Pompowanie przedniego koła', 'Zadanie polega na napompowaniu przedniego koła roweru.\nSzybciutko!'),\n"
"  ('Pompowanie tylnego koła', 'Zadanie polega na napompowaniu tylnego koła roweru.\nPrędziutko!'),\n"
"  ('Smarowanie łańcucha', 'Zadanie polega na nasmarowaniu łańcucha rowerowego.\nMigiem!')";

const char* populateEmployeesTasksTable =
"INSERT OR IGNORE INTO EmployeesTasks(employee, task)\n"
"          SELECT E.login, T.id\n"
"          FROM Employees AS E JOIN Tasks AS T\n"
"          WHERE E.name = 'Yauheni Barodzich' AND T.title = 'Pompowanie przedniego koła'\n"
"UNION ALL SELECT E.login, T.id\n"
"          FROM Employees AS E JOIN Tasks as T\n"
"          WHERE E.name = 'Mikhail Lukashevich' AND T.title = 'Pompowanie tylnego koła'\n"
"UNION ALL SELECT E.login, T.id\n"
"          FROM Employees AS E JOIN Tasks as T\n"
"          WHERE E.name = 'Tatsiana Lukashevich' AND T.title = 'Smarowanie łańcucha'\n"
"UNION ALL SELECT E.login as employee, T.id AS task\n"
"          FROM Employees AS E JOIN Tasks as T\n"
"          WHERE E.name = 'Wojciech Wiśniewski' AND T.title = 'Smarowanie łańcucha'\n";

const char *queries[] = {
    createEmployeesTable,
    createTasksTable,
    createEmployeesTasksTable,
    populateEmployeesTable,
    populateTasksTable,
    populateEmployeesTasksTable
};

const char *retrieveAllEmployeesQ =
"SELECT login, password, name, active\n"
"FROM Employees\n";

const char *retrieveEmployeesByLoginQ =
"SELECT login, password, name, active\n"
"FROM Employees\n"
"WHERE login = ?\n";

RowRetriever<std::shared_ptr<Employee>,
             string, string, string, bool>
employeeRetriever(std::make_shared<Employee, const string&, const string&, const string&, bool>);

static void printEmpl(const Employee& empl)
{
    qDebug() << "login = " << empl._login.c_str() << " password = " << empl._password.c_str()
             << " name = " << empl._name.c_str() << " active = " << empl._active;
}


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    try {
        Database db("StacjaSzefa.db");
        for (const char* txt : queries)
        {
            Command<> query(db, txt);
            query.execute();
        }

        Query<std::shared_ptr<Employee> > retrieveAllEmployees(
          db, retrieveAllEmployeesQ, employeeRetriever);
        retrieveAllEmployees.execute();
        std::shared_ptr<Employee> empl;
        while (retrieveAllEmployees.next(empl))
        {
            printEmpl(*empl);
        }
        Query<std::shared_ptr<Employee>, string> retrieveSpecificEmployees(
            db, retrieveEmployeesByLoginQ, employeeRetriever);
        retrieveSpecificEmployees.execute("wwisniew");
        while (retrieveSpecificEmployees.next(empl))
        {
            printEmpl(*empl);
        }
    } catch (std::exception &ex)
    {
        QMessageBox::critical(0, "Wyjątek",
                              QString(ex.what()) + "\n\n" +
                              "Kliknij OK aby wyjść.",
                              QMessageBox::Ok);
        return 1;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "KarbowyDb");
    db.setDatabaseName("StacjaSzefa.db");
    if (! db.open()) {
        QMessageBox::critical(0, "Błąd otwarcia bazy danych",
                              db.lastError().text() + "\n\n" +
                              "Kliknij OK aby wyjść.",
                              QMessageBox::Ok);
        return 1;
    }
//    QSqlQuery query(db);
//    for (const char* txt : queries) {
//        if (! query.exec(txt)) {
//            QMessageBox::critical(0, "Błąd inicjacji bazy danych",
//                                  query.lastError().text() + "\n\n" +
//                                  "Kliknij OK aby wyjść.",
//                                  QMessageBox::Ok);
//            return 1;
//        }
//    }
    db.close();

    MainWindow w;
    w.show();
    return a.exec();
}
