#include "communicationthread.h"
#include "mainwindow.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <QApplication>

namespace po = boost::program_options;
namespace uuid = boost::uuids;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::string serverUuid;
    std::string serverAddressStr;
    uint16_t port;
    po::options_description desc("Opcje");
    desc.add_options()
        ("server-uuid", po::value<std::string>(&serverUuid), "UUID stacji szefa")
        ("server-address", po::value<std::string>(&serverAddressStr), "Adres stacji szefa")
        ("ipv4", "Używaj IPv4")
        ("ipv6", "Używaj IPv6")
        ("server-port", po::value<uint16_t>(&port)->default_value(10001), "Port stacji szefa");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
//    std::string serverUuid;
//    if (vm.cout("server-uuid") == 0)
//    {
        
//    }
    
    
    std::string myUuid = boost::lexical_cast<std::string>(uuid::random_generator()());
    
    
    
    AddressVariant serverAddress(Ipv4Address::resolve("localhost", 10001));
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
