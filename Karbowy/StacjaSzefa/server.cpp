#include "server.h"
#include "clientconnection.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

const std::string& Server::uuidStr() const
{
    std::lock_guard<std::mutex> guard(_uuidStrMutex);
    if (_uuidStr.empty())
    {
        _uuidStr = boost::lexical_cast<std::string>(_uuid);
    }
    return _uuidStr;
}

void Server::runIpv4Listener()
{
    try
    {
        while (_run)
        {
            TcpStream stream = _ipv4Listener.awaitConnection();
            auto client = std::make_shared<ClientConnection>(*this, std::move(stream));
            _clients.insert(client);
            client->start();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Ipv4Listener exception: " << ex.what() << std::endl;
    }
}

void Server::runIpv6Listener()
{
    try
    {
        while (_run)
        {
            TcpStream stream = _ipv6Listener.awaitConnection();
            auto client = std::make_shared<ClientConnection>(*this, std::move(stream));
            _clients.insert(client);
            client->start();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Ipv6Listener exception: " << ex.what() << std::endl;
    }
}
