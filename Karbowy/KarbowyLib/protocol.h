#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "sockets.h"
#include "eventdispatcher.h"
#include "logentry.h"
#include <memory>
#include <functional>
#include <string>

std::string sendServerChallenge(TcpStream& conn);
std::string sendClientChallenge(TcpStream& conn);
std::string sendLoginChallenge(TcpStream &conn);

std::string receiveServerChallenge(TcpStream& conn);
std::string receiveClientChallenge(TcpStream& conn);
std::string receiveLoginChallenge(TcpStream& conn);

void sendServerChallengeResponse(TcpStream &conn, const std::string& secret, const std::string& challenge);
void sendClientChallengeResponse(TcpStream &conn, const std::string& secret, const std::string& challenge);
void sendLoginChallengeResponse(TcpStream &conn, const std::string& secret, const std::string& challenge);

std::string receiveServerChallengeResponse(TcpStream& conn);
std::string receiveClientChallengeResponse(TcpStream& conn);
std::string receiveLoginChallengeResponse(TcpStream& conn);

bool verifyChallengeResponse(const std::string& secret, const std::string& challenge, const std::string& response);

void sendServerChallengeAck(TcpStream& conn, bool ok);
void sendClientChallengeAck(TcpStream& conn, bool ok);
void sendLoginChallengeAck(TcpStream& conn, bool ok);

bool receiveServerChallengeAck(TcpStream& conn);
bool receiveClientChallengeAck(TcpStream& conn);
bool receiveLoginChallengeAck(TcpStream& conn);

void sendClientUuid(TcpStream& conn, const std::string& uuid);
std::string receiveClientUuid(TcpStream& conn);


void sendLoginRequest(TcpStream& conn, const std::string& userId);
std::string receiveLoginRequest(TcpStream& conn);

struct ClientConfig
{
    std::string _myUuid;
    std::string _serverUuid;
    std::string _serverAddress;
    uint16_t _serverPort;
    std::string _userId;
    std::string _password;
    bool _useIpv6;
};

struct Task
{
    int _id;
    std::string _title;
    int _secondsSpent;
    std::vector<std::string> _description;

    Task() = default;
    Task(int id, const std::string& title, const std::string& description, int secondsSpent);
};

class AsyncClient
{
public:
    typedef std::function<void(const std::string&)> ErrorCallback;
    typedef std::function<void()> ConnectCallback;
    typedef std::vector<std::unique_ptr<Task> > TasksList;
    typedef std::function<void(TasksList&&)> RetrieveTasksCallback;
    typedef std::deque<LogEntry> LogEntryList;
    typedef std::function<LogEntryList(const boost::optional<Timestamp>&)> RetrieveLogsCallback;
    typedef std::function<void()> LogsSentCallback;

    AsyncClient(MainLoop &mainLoop,
                const ClientConfig& config,
                const ErrorCallback& onError,
                const ConnectCallback& onConnect);

    void retrieveTasks(const RetrieveTasksCallback& onTasksRetrieved);
    void sendLogs(const RetrieveLogsCallback &receiveLogs, const LogsSentCallback& onLogsSent);

    bool busy() const { return _busy; }
private:
    MainLoop& _mainLoop;
    const ClientConfig& _config;
    ErrorCallback _onErrorHook;
    ConnectCallback _onConnectHook;
    std::unique_ptr<AsyncSocket> _conn;
    bool _connected;
    bool _busy;

    std::function<void()> _onConnect;
    std::string _serverChallenge;

    RetrieveTasksCallback _onTasksRetrievedHook;
    std::unique_ptr<Task> _currentTask;
    TasksList _tasks;

    RetrieveLogsCallback _retrieveLogs;
    LogsSentCallback _onLogsSentHook;
    LogEntryList _entrys;

    void startConnection(const std::function<void()>& onConnect);
    void afterConnect();
    void afterSendServerChallenge();
    void afterReceiveServerChallengeResponse(const std::string& line);
    void afterSendServerChallengeAck();
    void afterReceiveClientChallenge(const std::string& line);
    void afterSendClientChallengeResponse();
    void afterReceiveClientChallengeAck(const std::string& line);
    void afterSendClientUuid();
    void afterSendLoginRequest();
    void afterReceiveLoginChallenge(const std::string& line);
    void afterSendLoginChallengeResponse();
    void afterReceiveLoginChallengeAck(const std::string& line);

    void issueRetrieveTasksRequest();
    void startReceivingTasks();
    void receiveTaskHeader(const std::string& line);
    void receiveTaskDescription(const std::string& line);

    void issueSendLogsRequest();
    void readLastTimestamp();
    void startSendingLogs(const std::string& line);
    void sendLogEntry();
    void sendNextLogEntry();
    void finishSendingLogs();

    void handleProtocolError(const std::string& errorMsg, const std::string& line);
    void handleError(const std::string& errorMsg);
};

#endif // PROTOCOL_H
