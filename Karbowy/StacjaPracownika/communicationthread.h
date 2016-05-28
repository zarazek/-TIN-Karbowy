#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include "sockets.h"

#include <thread>
#include <atomic>
#include <boost/variant.hpp>
#include <boost/uuid/uuid.hpp>

typedef boost::variant<Ipv4Address, Ipv6Address> AddressVariant;

class CommunicationThread
{
public:
    CommunicationThread(const boost::uuids::uuid& myUuid,
                        const AddressVariant& serverAddress,
                        const boost::uuids::uuid& serverUuid);
    void stop();

private:
    boost::uuids::uuid _myUuid;
    AddressVariant _serverAddress;
    boost::uuids::uuid _serverUuid;

    std::atomic<bool> _running;
    std::thread _thread;

    void run();
};

#endif // COMMUNICATIONTHREAD_H
