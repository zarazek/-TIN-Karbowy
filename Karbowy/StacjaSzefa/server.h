#ifndef SERVER_H
#define SERVER_H

#include "sockets.h"
#include <boost/uuid/uuid.hpp>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>

class ClientConnection;

class Server
{
public:
    Server(uint16_t port);

    const boost::uuids::uuid& uuid() const
    {
        return _uuid;
    }

    const std::string& uuidStr() const;

    void start();
    void stop();
private:
    boost::uuids::uuid _uuid;
    mutable std::mutex _uuidStrMutex;
    mutable std::string _uuidStr;
    Ipv4Listener _ipv4Listener;
    Ipv6Listener _ipv6Listener;
    std::thread _ipv4Thread;
    std::thread _ipv6Thread;
    std::set<std::shared_ptr<ClientConnection> > _clients;
    std::atomic<bool> _run;

    void runIpv4Listener();
    void runIpv6Listener();
};

#endif // SERVER_H
