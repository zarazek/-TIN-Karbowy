#include "mainwindow.h"
#include "database.h"
#include "employee.h"
#include "sockets.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <thread>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <crypto++/sha.h>
#include <crypto++/filters.h>
#include <crypto++/hex.h>
#include <QDebug>
#include <sstream>

enum command {SC, SR, CR, CG};

const char* createEmployeesTable =
"CREATE TABLE IF NOT EXISTS Employees (\n"
"  login     VARCHAR(8) PRIMARY KEY\n,"
"  password  VARCHAR(15) NOT NULL,\n"
"  name      VARCHAR(100) NOT NULL,\n"
"  active    BOOL NOT NULL DEFAULT 1)";

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
"  assignment_active BOOL NOT NULL DEFAULT 1,\n"
"  finished          BOOL NOT NULL DEFAULT 0,\n"
"  time_spent        INTEGER NOT NULL DEFAULT 0,"
"  PRIMARY KEY (employee, task))\n";

const char* createUuidTable =
"CREATE TABLE IF NOT EXISTS Uuid (\n"
"  uuid   VARCHAR(100) PRIMARY KEY)";

const char* populateEmployeesTable =
"INSERT OR IGNORE INTO Employees(login, password, name) VALUES\n"
"  ('ybarodzi', 'pass1', 'Yauheni Barodzich'   ),\n"
"  ('mlukashe', 'pass2', 'Mikhail Lukashevich' ),\n"
"  ('tlukashe', 'pass3', 'Tatsiana Lukashevich'),\n"
"  ('wwisniew', 'pass4', 'Wojciech Wiśniewski' )\n";

const char* populateTasksTable =
"INSERT OR IGNORE INTO Tasks(title, description, status) VALUES\n"
"  ('Pompowanie przedniego koła', 'Zadanie polega na napompowaniu przedniego koła roweru.\nSzybciutko!', 0),\n"
"  ('Pompowanie tylnego koła', 'Zadanie polega na napompowaniu tylnego koła roweru.\nPrędziutko!', 1),\n"
"  ('Smarowanie łańcucha', 'Zadanie polega na nasmarowaniu łańcucha rowerowego.\nMigiem!', 2)";

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
    createUuidTable,
    //populateEmployeesTable,
    //populateTasksTable,
    //populateEmployeesTasksTable
};

const char* insertUuid =
"INSERT INTO Uuid(uuid) VALUES (?)";

const char *retrieveAllEmployeesQ =
"SELECT login, password, name, active\n"
"FROM Employees\n";

const char *retrieveEmployeesByLoginQ =
"SELECT login, password, name, active\n"
"FROM Employees\n"
"WHERE login = ?\n";

const char *retrieveUuidQ =
"SELECT uuid\n"
"FROM Uuid";

RowRetriever<std::shared_ptr<Employee>,
             string, string, string, bool>
employeeRetriever(std::make_shared<Employee, const string&, const string&, const string&, bool>);

std::string returnArg(const std::string& arg)
{
    return arg;
}

RowRetriever<std::string,
             string>
stringRetriever(returnArg);

static void printEmpl(const Employee& empl)
{
    qDebug() << "login = " << empl._login.c_str() << " password = " << empl._password.c_str()
             << " name = " << empl._name.c_str() << " active = " << empl._active;
}

static std::string parse(command cmd, std::string line)
{
    switch(cmd)
    {
    case SC:
        if(line.substr(0,17) != "SERWER CHALLENGE ")
        {
            throw std::runtime_error("SERVER CHALLENGE error");
        }
        else
        {
            return line.substr(18,line.length()-17);
        }
        break;
    case SR:
        if (line != "SERWER RESPONSE OK")
        {
            throw std::runtime_error("SERVER RESPONSE error");
        }
        else
        {
            return line;
        }
        break;
    case CR:
        if (line.substr(0,16) != "CLIENT RESPONSE ")
        {
            throw std::runtime_error("CLIENT RESPONSE error");
        }
        else
        {
            return line.substr(17,line.length()-16);
        }
        break;
    case CG:
        if (line.substr(0,12) != "CLIENT GUID")
        {
            throw std::runtime_error("CLIENT GUID error");
        }
        else
        {
            return line.substr(13,line.length()-12);
        }
        break;
    }

}

static std::string SHA(std::string message)
{
    CryptoPP::SHA256 hash;
    std::string digest;
    CryptoPP::StringSource s(message, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
    return digest;
}

static void handleConnection(TcpStream connection, std::string strUuid)
{
    std::string salt = parse(SC, connection.readLine());
    std::string response = SHA(strUuid + salt);
    connection.writeLine("SERVER RESPONSE " + response);
    parse(SR, connection.readLine());


}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    std::string strUuid;

    try {
        Database db("StacjaSzefa.db");
        for (const char* txt : queries)
        {
            Command<> query(db, txt);
            query.execute();
        }

        Query<std::string> retrieveUuid(
                    db, retrieveUuidQ, stringRetriever);
        retrieveUuid.execute();


        if (!retrieveUuid.next(strUuid))
        {
            boost::uuids::uuid uuid = boost::uuids::random_generator()();
            std::stringstream ss;
            ss << uuid;
            strUuid = ss.str();

            Command<std::string> query(db, insertUuid);
            query.execute(strUuid);
        }

//        Query<std::shared_ptr<Employee> > retrieveAllEmployees(
//          db, retrieveAllEmployeesQ, employeeRetriever);
//        retrieveAllEmployees.execute();
//        std::shared_ptr<Employee> empl;
//        while (retrieveAllEmployees.next(empl))
//        {
//            printEmpl(*empl);
//        }
//        Query<std::shared_ptr<Employee>, string> retrieveSpecificEmployees(
//            db, retrieveEmployeesByLoginQ, employeeRetriever);
//        retrieveSpecificEmployees.execute("wwisniew");
//        while (retrieveSpecificEmployees.next(empl))
//        {
//            printEmpl(*empl);
//        }
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

    //TODO
    //wątki, nasłuchujące IPv4 oraz IPv6
    Ipv4Listener listener(21455);
    while(true)
    {
        TcpStream stream = listener.awaitConnection();
        std::thread client(handleConnection, std::move(stream), strUuid);
    }

    MainWindow w;
    w.show();
    return a.exec();
}
