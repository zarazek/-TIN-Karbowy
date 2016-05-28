#include "mainwindow.h"
#include "predefinedqueries.h"
#include <QApplication>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    try {
        initializeDatabase();
        auto& retrieveUuid = retrieveUuidQ();
        retrieveUuid.execute();
        std::string strUuid;
        if (! retrieveUuid.next(strUuid))
        {
            boost::uuids::uuid uuid = boost::uuids::random_generator()();
            strUuid = boost::lexical_cast<std::string>(uuid);
            auto& insertUuid = insertUuidC();
            insertUuid.execute(strUuid);
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
    db.close();

    MainWindow w;
    w.show();
    return a.exec();
}
