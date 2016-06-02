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
    _password(password)
{
    _mainLoop.addObject(_queue);
}

void CommunicationThread::start()
{
    _mainLoop.start();
    _thread = std::thread(&CommunicationThread::run, this);
}

void CommunicationThread::stop()
{
    _mainLoop.exit();
    _thread.join();
}

void CommunicationThread::connect()
{
    _queue.addTask(std::bind(&CommunicationThread::startConnection, this));
}

class ConnectVisitor : public boost::static_visitor<void>
{
public:
    ConnectVisitor(AsyncSocket& conn, const AsyncSocket::ConnectHandler& handler) :
        _conn(conn),
        _handler(handler) { }

    void operator()(const Ipv4Address& address)
    {
        _conn.asyncConnect(address, _handler);
    }

    void operator()(const Ipv6Address& address)
    {
        _conn.asyncConnect(address, _handler);
    }
private:
    AsyncSocket& _conn;
    const AsyncSocket::ConnectHandler& _handler;
};

void CommunicationThread::startConnection()
{
    AsyncSocket::EofHandler eofHandler = [this]()
    {
        std::cerr << "Socket error: EOF" << std::endl;
        _conn.reset();
        startConnection();
    };
    AsyncSocket::ErrorHandler errorHandler = [this](const char* errorMsg, int err)
    {
        std::cerr << "Socket error: " << errorMsg << ": " << strerror(err) << " (errno " << err << ')' << std::endl;
        _conn.reset();
        startConnection();
    };
    _conn = std::make_unique<AsyncSocket>(eofHandler, errorHandler);
    AsyncSocket::ConnectHandler connectHandler = std::bind(&CommunicationThread::sendServerChallenge, this);
    ConnectVisitor visitor(*_conn, connectHandler);
    boost::apply_visitor(visitor, _serverAddress);
}

void CommunicationThread::sendServerChallenge()
{

}

//TcpStream CommunicationThread::initializeConnection()
//{
//    TcpStream conn = connect(_serverAddress);
//    std::string serverChallenge = sendServerChallenge(conn);
//    std::string serverResponse = receiveServerChallengeResponse(conn);
//    bool serverOk = verifyChallengeResponse(_serverUuid, serverChallenge, serverResponse);
//    sendServerChallengeAck(conn, serverOk);
//    if (! serverOk)
//    {
//        throw std::runtime_error("Invalid server challenge response");
//    }
//    std::string clientChallenge = receiveClientChallenge(conn);
//    sendClientChallengeResponse(conn, _serverUuid, clientChallenge);
//    bool clientOk = receiveClientChallengeAck(conn);
//    if (! clientOk)
//    {
//        throw std::runtime_error("Client challenge response rejected");
//    }
//    sendClientUuid(conn, _myUuid);
//    sendLoginRequest(conn, _userId);
//    std::string loginChallenge = receiveLoginChallenge(conn);
//    sendLoginChallengeResponse(conn, _password, loginChallenge);
//    bool loginOk = receiveLoginChallengeAck(conn);
//    if (! loginOk)
//    {
//        throw std::runtime_error("Login challenge response rejected");
//    }
//    return conn;
//}

void CommunicationThread::run()
{
    try {
        _mainLoop.run();
    }
    catch (std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}
