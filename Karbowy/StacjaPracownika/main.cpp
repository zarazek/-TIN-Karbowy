#include "communicationthread.h"
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
    QApplication a(argc, argv);

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
    
    if (vm.count("ipv4") && vm.count("ipv6") ||
            ! vm.count("ipv4") && ! vm.count("ipv6"))
    {
        std::cout << " Wybież jedną wersję: IPv4 lub IP" << std::endl;
        return 1;
    }
    AddressVariant serverAddress = Ipv4Address::any(port);
    if (vm.count("ipv4"))
    {
        serverAddress = Ipv4Address::resolve(serverAddressStr, port);
    }
    else
    {
        serverAddress = Ipv6Address::resolve(serverAddressStr, port);
    }    
    uuid::uuid myUuid = uuid::random_generator()();
    std::string userId = "wwisniew";
    std::string password = "pass4";
    CommunicationThread thr(myUuid, serverAddress, serverUuid, userId, password);
    thr.start();
    MainWindow w;
    w.show();
    int res = a.exec();
    thr.stop();
    return res;
}
