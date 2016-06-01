#include "mainwindow.h"
#include "predefinedqueries.h"
#include "server.h"
#include <QApplication>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <signal.h>
#include <iostream>

static void handler(int) { }

int main(int argc, char *argv[]) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;
    if (sigaction(SIGUSR1, &action, nullptr) < 0) {
      perror("sigaction");
      return 1;
    }

    QApplication a(argc, argv);

    try {
        initializeDatabase();
        auto& retrieveUuid = retrieveUuidQ();
        retrieveUuid.execute();
        std::string strUuid;
        if (! retrieveUuid.next(strUuid))
        {
            strUuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
            auto& insertUuid = insertUuidC();
            insertUuid.execute(strUuid);
        }
        std::cout << "UUID = " << strUuid << std::endl;
        Server server(std::move(strUuid), 10001);
        server.start();

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "KarbowyDb");
        db.setDatabaseName("StacjaSzefa.db");
        if (! db.open()) {
            QMessageBox::critical(0, "Błąd otwarcia bazy danych",
                                  db.lastError().text() + "\n\n" +
                                  "Kliknij OK aby wyjść.",
                                  QMessageBox::Ok);
            return 1;
        }
        db.close();

        MainWindow w;
        w.show();
        int res =  a.exec();
        server.stop();
        shutdownDatabase();
        return res;
    } catch (std::exception &ex)
    {
        QMessageBox::critical(0, "Wyjątek",
                              QString(ex.what()) + "\n\n" +
                              "Kliknij OK aby wyjść.",
                              QMessageBox::Ok);
        return 1;
    }
}
