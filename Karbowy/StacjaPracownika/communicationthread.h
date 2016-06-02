#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include "sockets.h"

#include <thread>
#include <atomic>
#include <boost/uuid/uuid.hpp>
#include <boost/variant.hpp>

typedef boost::variant<Ipv4Address, Ipv6Address> AddressVariant;

class CommunicationThread
{
public:
    CommunicationThread(const boost::uuids::uuid& myUuid,
                        const AddressVariant& serverAddress,
                        const boost::uuids::uuid& serverUuid,
                        const std::string& userId,
                        const std::string& password);
    void start();
    void stop();

private:
    std::string _myUuid;
    AddressVariant _serverAddress;
    std::string _serverUuid;
    std::string _userId;
    std::string _password;

    std::atomic<bool> _run;
    std::thread _thread;

    TcpStream initializeConnection();
    void run();
};

#endif // COMMUNICATIONTHREAD_H
