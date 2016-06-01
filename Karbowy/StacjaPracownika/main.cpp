#include "communicationthread.h"
#include "mainwindow.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::string myUuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
    AddressVariant serverAddress(Ipv4Address::resolve("localhost", 10001));
    std::string serverUuid = argv[1];
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
