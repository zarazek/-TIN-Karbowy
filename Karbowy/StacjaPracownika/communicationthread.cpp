#include "communicationthread.h"
#include "protocol.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

CommunicationThread::CommunicationThread(const boost::uuids::uuid& myUuid,
                                         const AddressVariant& serverAddress,
                                         const boost::uuids::uuid& serverUuid,
                                         const std::string& userId,
                                         const std::string& password) :
    _myUuid(boost::lexical_cast<std::string>(myUuid)),
    _serverAddress(serverAddress),
    _serverUuid(boost::lexical_cast<std::string>(serverUuid)),
    _userId(userId),
    _password(password),
    _run(false) { }

void CommunicationThread::start()
{
    _run = true;
    _thread = std::thread(&CommunicationThread::run, this);
}

void CommunicationThread::stop()
{
    _run = false;
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

TcpStream CommunicationThread::initializeConnection()
{
    TcpStream conn = connect(_serverAddress);
    std::string serverChallenge = sendServerChallenge(conn);
    std::string serverResponse = receiveServerChallengeResponse(conn);
    bool serverOk = verifyChallengeResponse(_serverUuid, serverChallenge, serverResponse);
    sendServerChallengeAck(conn, serverOk);
    if (! serverOk)
    {
        throw std::runtime_error("Invalid server challenge response");
    }
    std::string clientChallenge = receiveClientChallenge(conn);
    sendClientChallengeResponse(conn, _serverUuid, clientChallenge);
    bool clientOk = receiveClientChallengeAck(conn);
    if (! clientOk)
    {
        throw std::runtime_error("Client challenge response rejected");
    }
    sendClientUuid(conn, _myUuid);
    sendLoginRequest(conn, _userId);
    std::string loginChallenge = receiveLoginChallenge(conn);
    sendLoginChallengeResponse(conn, _password, loginChallenge);
    bool loginOk = receiveLoginChallengeAck(conn);
    if (! loginOk)
    {
        throw std::runtime_error("Login challenge response rejected");
    }
    return conn;
}

void CommunicationThread::run()
{
    while (_run)
    {
        int milis = 10;
        const int maxMilis = 4000;
        try {
            TcpStream conn = initializeConnection();
            while (_run)
            { }
        } catch (std::exception& ex) {
            std::cerr << ex.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(milis));
            milis = std::min(2*milis, maxMilis);
        }
    }
}
