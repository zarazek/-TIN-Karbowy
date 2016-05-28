#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include "sockets.h"
#include <boost/uuid/uuid.hpp>
#include <boost/optional.hpp>
#include <thread>

class Server;
class Employee;

class ClientConnection
{
public:
    ClientConnection(Server& server, TcpStream&& stream);
    void start();
    void stop();

private:
    Server &_server;
    TcpStream _stream;
    std::thread _thread;
    boost::uuids::uuid _clientUuid;
    std::string _userId;

    void run();
    bool initializeConnection();
    std::string receiveServerChallenge();
    void sendServerChallengeResponse(const std::string& serverChallenge);
    bool receiveServerChallengeAck();
    std::string sendClientChallenge();
    std::string receiveClientChallengeResponse();
    bool verifyClientChallengeResponse(const std::string& challenge, const std::string& response);
    void sendClientChallengeAck(bool isOk);
    boost::uuids::uuid receiveClientUuid();
    std::string receiveLoginRequest();
    static std::unique_ptr<Employee> verifyUserId(const std::string& userId);
    std::string sendLoginChallenge();
    std::string receiveLoginChallengeResponse();
    static bool verifyLoginChallengeResponse(const std::string& password, const std::string& challenge, const std::string& response);


    bool handleCommand(const std::string& line);
    void onShutdown();
};

#endif // CLIENTCONNECTION_H
