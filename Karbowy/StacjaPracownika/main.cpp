#include "communicationthread.h"
#include "predefinedqueries.h"
#include "mainwindow.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include <QApplication>

namespace po = boost::program_options;
namespace uuid = boost::uuids;

int main(int argc, char *argv[])
{
    uuid::uuid serverUuid;
    std::string serverAddressStr;
    uint16_t port;
    po::options_description desc("Opcje");
    desc.add_options()
        ("server-uuid,u", po::value<uuid::uuid>(&serverUuid)->required(), "UUID stacji szefa")
        ("server-address,a", po::value<std::string>(&serverAddressStr)->required(), "Adres stacji szefa")
        ("ipv4,4", "Używaj IPv4")
        ("ipv6,6", "Używaj IPv6")
        ("server-port,p", po::value<uint16_t>(&port)->default_value(10001), "Port stacji szefa");
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (po::error& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return 1;
    }
    
    if ((vm.count("ipv4") && vm.count("ipv6")) ||
        (! vm.count("ipv4") && ! vm.count("ipv6")))
    {
        std::cout << " Wybież jedną wersję: IPv4 lub IPv6" << std::endl;
        return 1;
    }

    try {
        initializeDatabase();
        auto& retrieveUuid = retrieveUuidQ();
        retrieveUuid.execute();
        std::string myUuid;
        if (! retrieveUuid.next(myUuid))
        {
            myUuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
            auto& insertUuid = insertUuidC();
            insertUuid.execute(myUuid);
        }
        std::cout << "UUID = " << myUuid << std::endl;

        ClientConfig config;
        config._myUuid = myUuid;
        config._serverUuid = boost::lexical_cast<std::string>(serverUuid);
        config._serverAddress = serverAddressStr;
        config._serverPort = port;
        config._userId = "wwisniew";
        config._password = "pass4";
        config._useIpv6 = vm.count("ipv6");
        CommunicationThread thr;
        thr.start();
        thr.setClientConfig(config);
        thr.retrieveTasks();
        sleep(10);
        thr.sendLogs();
        sleep(10);
        thr.stop();
        return 0;
    }
    catch (std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return 1;
    }

//    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
//    return a.exec();
}
