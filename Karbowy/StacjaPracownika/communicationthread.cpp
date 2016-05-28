#include "communicationthread.h"
#include <iostream>

CommunicationThread::CommunicationThread(const boost::uuids::uuid& myUuid,
                                         const AddressVariant& serverAddress,
                                         const boost::uuids::uuid& serverUuid) :
    _myUuid(myUuid),
    _serverAddress(serverAddress),
    _serverUuid(serverUuid),
    _running(true),
    _thread(&CommunicationThread::run, this) { }

void CommunicationThread::stop()
{
    _running = false;
    _thread.join();
}

class ConnectVisitor : public boost::static_visitor<TcpStream>
{
public:
    TcpStream operator()(const Ipv4Address& address)
    {
        return TcpStream::connect(address);
    }

    TcpStream operator()(const Ipv6Address& address)
    {
        return TcpStream::connect(address);
    }
};

TcpStream connect(const AddressVariant& serverAddress)
{
    ConnectVisitor visitor;
    return boost::apply_visitor(visitor, serverAddress);
}

void CommunicationThread::run()
{
    try {
        TcpStream conn = connect(_serverAddress);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}
