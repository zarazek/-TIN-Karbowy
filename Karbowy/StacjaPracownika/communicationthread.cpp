#include "communicationthread.h"
#include "protocol.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace std::placeholders;

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
        restartConnection();
    };
    AsyncSocket::ErrorHandler errorHandler = [this](const char* errorMsg, int err)
    {
        std::cerr << "Socket error: " << errorMsg << ": " << strerror(err) << " (errno " << err << ')' << std::endl;
        restartConnection();
    };
    _conn = std::make_unique<AsyncSocket>(eofHandler, errorHandler);
    _mainLoop.addObject(*_conn);
    AsyncSocket::ConnectHandler connectHandler = std::bind(&CommunicationThread::afterConnect, this);
    ConnectVisitor visitor(*_conn, connectHandler);
    boost::apply_visitor(visitor, _serverAddress);
}

void CommunicationThread::afterConnect()
{
    asyncSendServerChallenge(*_conn,
                             std::bind(&CommunicationThread::afterSendServerChallenge, this, _1));
}

void CommunicationThread::afterSendServerChallenge(const std::string& serverChallenge)
{
    _serverChallenge = serverChallenge;
    asyncReceiveServerChallenge(*_conn,
                                std::bind(&CommunicationThread::afterReceiveServerChallengeResponse, this, _1));
}

void CommunicationThread::afterReceiveServerChallengeResponse(const std::string& serverResponse)
{
    bool serverOk = verifyChallengeResponse(_serverUuid, _serverChallenge, serverResponse);
    if (! serverOk)
    {
        std::cout << "Invalid server challenge response" << std::endl;
    }
    asyncSendServerChallengeAck(*_conn, serverOk,
                               serverOk ? std::bind(&CommunicationThread::afterSendServerChallengeAck, this) :
                                          std::bind(&CommunicationThread::restartConnection, this));
}

void CommunicationThread::afterSendServerChallengeAck()
{
    asyncReceiveClientChallenge(*_conn,
                                std::bind(&CommunicationThread::afterReceiveClientChallenge, this, _1));
}

void CommunicationThread::afterReceiveClientChallenge(const std::string& clientChallenge)
{
    asyncSendClientChallengeResponse(*_conn, _serverUuid, clientChallenge,
                                     std::bind(&CommunicationThread::afterSendClientChallengeResponse, this));
}

void CommunicationThread::afterSendClientChallengeResponse()
{
    asyncReceiveClientChallengeAck(*_conn,
                                   std::bind(&CommunicationThread::afterReceiveClientChallengeAck, this, _1));
}

void CommunicationThread::afterReceiveClientChallengeAck(bool clientOk)
{
    if (clientOk)
    {
        asyncSendClientUuid(*_conn, _myUuid,
                            std::bind(&CommunicationThread::afterSendClientUuid, this));
    }
    else
    {
        std::cerr << "Client challenge response rejected" << std::endl;
        restartConnection();
    }
}

void CommunicationThread::afterSendClientUuid()
{
    asyncSendLoginRequest(*_conn, _userId,
                          std::bind(&CommunicationThread::afterSendLoginRequest, this));
}

void CommunicationThread::afterSendLoginRequest()
{
    asyncReceiveLoginChallenge(*_conn,
                               std::bind(&CommunicationThread::afterReceiveLoginChallenge, this, _1));
}

void CommunicationThread::afterReceiveLoginChallenge(const std::string& loginChallenge)
{
    asyncSendLoginChallengeResponse(*_conn, _password, loginChallenge,
                                    std::bind(&CommunicationThread::afterSendLoginChallengeResponse, this));
}

void CommunicationThread::afterSendLoginChallengeResponse()
{
    asyncReceiveLoginChallengeAck(*_conn,
                                  std::bind(&CommunicationThread::afterReceiveLoginChallengeAck, this, _1));
}

void CommunicationThread::afterReceiveLoginChallengeAck(bool loginOk)
{
    if (loginOk)
    {
        executeTask();
    }
    else
    {
        std::cerr << "Login rejected" << std::endl;
        restartConnection();
    }
}

void CommunicationThread::restartConnection()
{
    // TODO: expotential backoff
    startConnection();
}

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
