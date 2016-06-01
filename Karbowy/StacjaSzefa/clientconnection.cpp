#include "clientconnection.h"
#include "protocol.h"
#include "server.h"
#include "predefinedqueries.h"
#include "employee.h"
#include <signal.h>

#include <iostream>

ClientConnection::ClientConnection(Server& server, TcpStream&& stream) :
    _server(server),
    _stream(std::move(stream)) { }

void ClientConnection::start()
{
    _thread = std::thread(&ClientConnection::run, this);
}

void ClientConnection::stop()
{
    _run = false;
    pthread_kill(_thread.native_handle(), SIGUSR1);
}

void ClientConnection::waitToFinish()
{
    _thread.join();
}

std::unique_ptr<Employee> ClientConnection::verifyUserId(const std::string& userId)
{
    auto& query = findEmployeeByLoginQ();
    query.execute(userId);
    std::unique_ptr<Employee> employee;
    if (! query.next(employee))
    {
        std::cerr << "Can't find employee id '" << userId << '\'' << std::endl;
        return std::unique_ptr<Employee>();
    }
    if (query.next(employee))
    {
        while (query.next(employee)) { }
        std::cerr << "Found more than one employee with id '" << userId << '\'' << std::endl;
        return std::unique_ptr<Employee>();
    }
    if (! employee->_active)
    {
        std::cerr << "Employee '" << userId << "' is not active" << std::endl;
        return std::unique_ptr<Employee>();
    }
    return employee;
}

bool ClientConnection::initializeConnection()
{
    std::string serverChallenge = receiveServerChallenge(_stream);
    sendServerChallengeResponse(_stream, _server.uuid(), serverChallenge);
    if (receiveServerChallengeAck(_stream))
    {
        std::cerr << "Server challenge response rejected" << std::endl;
        return false;
    }
    std::string clientChallenge = sendClientChallenge(_stream);
    std::string clientResponse = receiveClientChallengeResponse(_stream);
    bool clientOk = verifyChallengeResponse(_server.uuid(), clientChallenge, clientResponse);
    sendClientChallengeAck(_stream, clientOk);
    if (! clientOk)
    {
        return false;
    }
    std::string clientUuid = receiveClientUuid(_stream);
    std::string userId = receiveLoginRequest(_stream);
    std::unique_ptr<Employee> employee = verifyUserId(userId);
    if (! employee)
    {
        return false;
    }
    std::string loginChallenge = sendLoginChallenge(_stream);
    std::string loginResponse = receiveLoginChallengeResponse(_stream);
    if (! verifyChallengeResponse(employee->_password, loginChallenge, loginResponse))
    {
        return false;
    }
    _clientUuid = clientUuid;
    _userId = userId;
    return true;
}

void ClientConnection::handleCommand(const std::string& line)
{
    // TODO
}

void ClientConnection::run()
{
    try
    {
        if (initializeConnection())
        {
            while (_run)
            {
                handleCommand(_stream.readLine());
            }
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Client exception: " << ex.what() << std::endl;
    }
    _server.removeClient(shared_from_this());
}
