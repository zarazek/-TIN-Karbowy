#include "communicationthread.h"
#include "predefinedqueries.h"
#include "mainwindow.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
//#include <boost/program_options.hpp>
#include <iostream>

#include <QApplication>
#include <QMessageBox>

//namespace po = boost::program_options;
namespace uuid = boost::uuids;

int main(int argc, char *argv[])
{
//    uuid::uuid serverUuid;
//    std::string serverAddressStr;
//    uint16_t port;
//    po::options_description desc("Opcje");
//    desc.add_options()
//        ("server-uuid,u", po::value<uuid::uuid>(&serverUuid)->required(), "UUID stacji szefa")
//        ("server-address,a", po::value<std::string>(&serverAddressStr)->required(), "Adres stacji szefa")
//        ("ipv4,4", "Używaj IPv4")
//        ("ipv6,6", "Używaj IPv6")
//        ("server-port,p", po::value<uint16_t>(&port)->default_value(10001), "Port stacji szefa");
//    po::variables_map vm;
//    try {
//        po::store(po::parse_command_line(argc, argv, desc), vm);
//        po::notify(vm);
//    }
//    catch (po::error& ex)
//    {
//        std::cerr << "Error: " << ex.what() << std::endl << std::endl;
//        std::cerr << desc << std::endl;
//        return 1;
//    }
    
//    if ((vm.count("ipv4") && vm.count("ipv6")) ||
//        (! vm.count("ipv4") && ! vm.count("ipv6")))
//    {
//        std::cout << " Wybież jedną wersję: IPv4 lub IPv6" << std::endl;
//        return 1;
//    }

    std::string myUuid;
    int retValue;
    try {
        initializeDatabase();
        auto& retrieveUuid = retrieveUuidQ();
        retrieveUuid.execute();
        if (! retrieveUuid.next(myUuid))
        {
            myUuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
            auto& insertUuid = insertUuidC();
            insertUuid.execute(myUuid);
        }
        std::cout << "UUID = " << myUuid << std::endl;

        CommunicationThread thr;
        thr.start();

        QApplication a(argc, argv);
        MainWindow w(std::move(myUuid), thr);
        w.show();
        retValue = a.exec();

        thr.stop();
    }
    catch (std::exception& ex)
    {
        QMessageBox::critical(0, "Wyjątek",
                              QString(ex.what()) + "\n\n" +
                              "Kliknij OK aby wyjść.",
                              QMessageBox::Ok);
        retValue = 1;
    }
    shutdownDatabase();
    return retValue;
}
