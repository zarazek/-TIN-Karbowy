#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include "sockets.h"
#include <thread>
#include <atomic>

class Server;
class Employee;
class LogEntry;

class ClientConnection : public std::enable_shared_from_this<ClientConnection>
{
public:
    ClientConnection(Server& server, TcpStream&& stream);
    void start();
    void stop();
    void waitToFinish();

private:
    Server &_server;
    TcpStream _stream;
    std::atomic<bool> _run;
    std::thread _thread;
    int _clientId;
    std::string _userId;

    void run();
    bool initializeConnection();
    static std::unique_ptr<Employee> verifyUserId(const std::string& userId);


    void handleCommand(const std::string& line);
    void insertLogEntry(const LogEntry& entry);
    void processLogs();
};

#endif // CLIENTCONNECTION_H
