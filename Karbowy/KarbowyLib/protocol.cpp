#include "protocol.h"
#include "sockets.h"
#include "eventdispatcher.h"
#include "concat.h"
#include "protocolerror.h"
#include "parse.h"
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <crypto++/filters.h>
#include <crypto++/hex.h>
#include <crypto++/osrng.h>

#include <iostream>

enum ChallengeType
{
    ChallengeType_SERVER,
    ChallengeType_CLIENT,
    ChallengeType_LOGIN
};

static const char* typeToString(ChallengeType type)
{
    switch (type)
    {
    case ChallengeType_SERVER:
        return "SERVER";
    case ChallengeType_CLIENT:
        return "CLIENT";
    case ChallengeType_LOGIN:
        return "LOGIN";
    default:
        assert(false);
        return nullptr;
    }
}



static std::string generateChallenge()
{
    CryptoPP::AutoSeededRandomPool prng;
    CryptoPP::SecByteBlock seed(16);
    prng.GenerateBlock(seed, seed.size());
    std::string challenge;
    CryptoPP::ArraySource(seed, seed.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(challenge)));
    return challenge;
}

static std::string SHA(const std::string& message)
{
    CryptoPP::SHA256 hash;
    std::string digest;
    CryptoPP::StringSource s(message, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
    return digest;
}

static boost::optional<std::string> extractSuffix(const std::string& str, const std::string& prefix)
{
    if (boost::istarts_with(str, prefix))
    {
        return boost::optional<std::string>(str.substr(prefix.length()));
    }
    else
    {
        return boost::none;
    }
}

static boost::optional<std::string> extractSuffix(const std::string& str, const char* prefix)
{
    if (boost::istarts_with(str, prefix))
    {
        return boost::optional<std::string>(str.substr(strlen(prefix)));
    }
    else
    {
        return boost::none;
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------------

static std::string sendChallenge(ChallengeType type, TcpStream& conn)
{
    std::string challenge = generateChallenge();
    conn.writeLine(concatln(typeToString(type), " CHALLENGE ", challenge));
    return challenge;
}

std::string sendServerChallenge(TcpStream& conn)
{
    return sendChallenge(ChallengeType_SERVER, conn);
}

std::string sendClientChallenge(TcpStream& conn)
{
    return sendChallenge(ChallengeType_CLIENT, conn);
}

std::string sendLoginChallenge(TcpStream& conn)
{
    return sendChallenge(ChallengeType_LOGIN, conn);
}

static std::string receiveChallenge(ChallengeType type, TcpStream& conn)
{
    std::string line = conn.readLine();
    std::string prefix = concat(typeToString(type), " CHALLENGE ");
    auto challenge = extractSuffix(line, prefix);
    if (challenge)
    {
        return *challenge;
    }
    else
    {
        throw ProtocolError(concat("Invalid ", typeToString(type), " challenge"), line);
    }
}

std::string receiveServerChallenge(TcpStream &conn)
{
    return receiveChallenge(ChallengeType_SERVER, conn);
}

std::string receiveClientChallenge(TcpStream &conn)
{
    return receiveChallenge(ChallengeType_CLIENT, conn);
}

std::string receiveLoginChallenge(TcpStream &conn)
{
    return receiveChallenge(ChallengeType_LOGIN, conn);
}

static void sendChallengeResponse(ChallengeType type, TcpStream& conn, const std::string& secret, const std::string& challenge)
{
    conn.writeLine(concatln(typeToString(type), " RESPONSE ", SHA(secret + challenge)));
}

void sendServerChallengeResponse(TcpStream& conn, const std::string& secret, const std::string& challenge)
{
    sendChallengeResponse(ChallengeType_SERVER, conn, secret, challenge);
}

void sendClientChallengeResponse(TcpStream &conn, const std::string &secret, const std::string &challenge)
{
    sendChallengeResponse(ChallengeType_CLIENT, conn, secret, challenge);
}

void sendLoginChallengeResponse(TcpStream &conn, const std::string &secret, const std::string &challenge)
{
    sendChallengeResponse(ChallengeType_LOGIN, conn, secret, challenge);
}

static std::string receiveChallengeResponse(ChallengeType type, TcpStream& conn)
{
    std::string line = conn.readLine();
    std::string prefix = concat(typeToString(type), " RESPONSE ");
    auto response = extractSuffix(line, prefix);
    if (! response)
    {
        throw ProtocolError(concat("Invalid ", typeToString(type), " challenge response"), line);
    }
    return *response;
}

std::string receiveServerChallengeResponse(TcpStream& conn)
{
    return receiveChallengeResponse(ChallengeType_SERVER, conn);
}

std::string receiveClientChallengeResponse(TcpStream& conn)
{
    return receiveChallengeResponse(ChallengeType_CLIENT, conn);
}

std::string receiveLoginChallengeResponse(TcpStream& conn)
{
    return receiveChallengeResponse(ChallengeType_LOGIN, conn);
}

bool verifyChallengeResponse(const std::string& secret, const std::string& challenge, const std::string& response)
{
    return SHA(secret + challenge) == response;
}

static void sendChallengeAck(ChallengeType type, TcpStream& conn, bool ok)
{
    conn.writeLine(concatln(typeToString(type), " RESPONSE ", ok ? "OK" : "NOK"));
}

void sendServerChallengeAck(TcpStream &conn, bool ok)
{
    sendChallengeAck(ChallengeType_SERVER, conn, ok);
}

void sendClientChallengeAck(TcpStream &conn, bool ok)
{
    sendChallengeAck(ChallengeType_CLIENT, conn, ok);
}

void sendLoginChallengeAck(TcpStream& conn, bool ok)
{
    sendChallengeAck(ChallengeType_LOGIN, conn, ok);
}

static bool receiveChallengeAck(ChallengeType type, TcpStream& conn)
{
    std::string line = conn.readLine();
    std::string prefix = concat(typeToString(type), " RESPONSE ");
    std::string okLine = concat(prefix, "OK");
    std::string nokLine = concat(prefix, "NOK");
    if (boost::iequals(line, okLine))
    {
        return true;
    }
    else if (boost::iequals(line, nokLine))
    {
        return false;
    }
    else
    {
        throw ProtocolError(concat("Invalid ", typeToString(type), " challenge response"), line);
    }
}

bool receiveServerChallengeAck(TcpStream &conn)
{
    return receiveChallengeAck(ChallengeType_SERVER, conn);
}

bool receiveClientChallengeAck(TcpStream& conn)
{
    return receiveChallengeAck(ChallengeType_CLIENT, conn);
}

bool receiveLoginChallengeAck(TcpStream &conn)
{
    return receiveChallengeAck(ChallengeType_LOGIN, conn);
}

static const char* clientUuidCmdPrefix = "CLIENT UUID ";

void sendClientUuid(TcpStream& conn, const std::string& uuid)
{
    conn.writeLine(concatln(clientUuidCmdPrefix, uuid));
}

std::string receiveClientUuid(TcpStream& conn)
{
    std::string line = conn.readLine();
    auto maybeUuidStr = extractSuffix(line, clientUuidCmdPrefix);
    if (maybeUuidStr)
    {
        try {
            boost::lexical_cast<boost::uuids::uuid>(*maybeUuidStr);
        }
        catch (boost::bad_lexical_cast&)
        {
            throw ProtocolError("Invalid client uuid", *maybeUuidStr);
        }
        return *maybeUuidStr;
    }
    else
    {
        throw ProtocolError("Invalid client uuid cmd", line);
    }
}

static const char* loginCmdPrefix = "LOGIN ";

void sendLoginRequest(TcpStream& conn, const std::string& userId)
{
    conn.writeLine(concatln(loginCmdPrefix, userId));
}

std::string receiveLoginRequest(TcpStream& conn)
{
    std::string line = conn.readLine();
    auto maybeUserId = extractSuffix(line, loginCmdPrefix);
    if (! maybeUserId)
    {
        throw ProtocolError("Invalid login request", line);
    }
    return *maybeUserId;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

using namespace std::placeholders;

Task::Task(int id, const std::string &title, const std::string &description, int secondsSpent) :
    _id(id),
    _title(title),
    _secondsSpent(secondsSpent)
{
    boost::split(_description, description, [](char c){ return c == '\n'; });
}

AsyncClient::AsyncClient(MainLoop& mainLoop,
                         const ClientConfig& config,
                         const ErrorCallback& onError,
                         const ConnectCallback& onConnect) :
    _mainLoop(mainLoop),
    _config(config),
    _onErrorHook(onError),
    _onConnectHook(onConnect),
    _connected(false),
    _busy(false) { }

void AsyncClient::retrieveTasks(const RetrieveTasksCallback& onTasksRetrieved)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    assert(! _busy);

    _busy = true;
    _onTasksRetrievedHook = onTasksRetrieved;
    if (_connected)
    {
        issueRetrieveTasksRequest();
    }
    else
    {
        startConnection(std::bind(&AsyncClient::issueRetrieveTasksRequest, this));
    }
}

void AsyncClient::sendLogs(const RetrieveLogsCallback& retrieveLogs, const LogsSentCallback& onLogsSent)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    assert(! _busy);

    _busy = true;
    _retrieveLogs = retrieveLogs;
    _onLogsSentHook = onLogsSent;
    if (_connected)
    {
        issueSendLogsRequest();
    }
    else
    {
        startConnection(std::bind(&AsyncClient::issueSendLogsRequest, this));
    }
}

void AsyncClient::startConnection(const std::function<void()>& onConnect)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    assert(! _connected);

    _onConnect = onConnect;
    _conn = std::make_unique<AsyncSocket>(std::bind(&AsyncClient::handleError, this, _1));
    AsyncSocket::ConnectHandler afterConnect = std::bind(&AsyncClient::afterConnect, this);
    bool successSoFar = true;
    try
    {
        if (_config._useIpv6)
        {
            Ipv6Address addr = Ipv6Address::resolve(_config._serverAddress, _config._serverPort);
            successSoFar = _conn->asyncConnect(addr, afterConnect);
        }
        else
        {
            Ipv4Address addr = Ipv4Address::resolve(_config._serverAddress, _config._serverPort);
            successSoFar = _conn->asyncConnect(addr, afterConnect);
        }
    }
    catch (std::exception &ex)
    {
        successSoFar = false;
        handleError(ex.what());
    }
    if (successSoFar)
    {
        _mainLoop.addObject(*_conn);
    }
}

void AsyncClient::afterConnect()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _serverChallenge = generateChallenge();
    _conn->asyncWrite(concatln("SERVER CHALLENGE ", _serverChallenge),
                     std::bind(&AsyncClient::afterSendServerChallenge, this));
}

void AsyncClient::afterSendServerChallenge()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncReadLine(std::bind(&AsyncClient::afterReceiveServerChallengeResponse, this, _1));
}

void AsyncClient::afterReceiveServerChallengeResponse(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    auto response = extractSuffix(line, "SERVER RESPONSE ");
    if (! response)
    {
        handleProtocolError("Invalid server challenge response", line);
        return;
    }
    bool serverOk = verifyChallengeResponse(_config._serverUuid, _serverChallenge, *response);
    _conn->asyncWrite(concatln("SERVER RESPONSE ", serverOk ? "OK" : "NOK"),
                     serverOk ? AsyncSocket::WriteHandler(std::bind(&AsyncClient::afterSendServerChallengeAck, this)) :
                                AsyncSocket::WriteHandler(std::bind(&AsyncClient::handleError, this, "Invalid server challenge response")));
}

void AsyncClient::afterSendServerChallengeAck()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncReadLine(std::bind(&AsyncClient::afterReceiveClientChallenge, this, _1));
}

void AsyncClient::afterReceiveClientChallenge(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    auto challenge = extractSuffix(line, "CLIENT CHALLENGE ");
    if (! challenge)
    {
        handleProtocolError("Invalid client challenge", line);
        return;
    }
    _conn->asyncWrite(concatln("CLIENT RESPONSE ", SHA(_config._serverUuid + *challenge)),
                     std::bind(&AsyncClient::afterSendClientChallengeResponse, this));
}

void AsyncClient::afterSendClientChallengeResponse()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncReadLine(std::bind(&AsyncClient::afterReceiveClientChallengeAck, this, _1));
}

void AsyncClient::afterReceiveClientChallengeAck(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    static const char* okLine = "CLIENT RESPONSE OK";
    static const char* nokLine = "CLIENT RESPONSE NOK";

    bool clientOk;
    if (boost::iequals(line, okLine))
    {
        clientOk = true;
    }
    else if (boost::iequals(line, nokLine))
    {
        clientOk = false;
    }
    else
    {
        handleProtocolError("Invalid client challenge ack", line);
        return;
    }

    if (clientOk)
    {
        _conn->asyncWrite(concatln("CLIENT UUID ", _config._myUuid),
                         std::bind(&AsyncClient::afterSendClientUuid, this));
    }
    else
    {
        handleError("Client challenge response rejected");
    }
}

void AsyncClient::afterSendClientUuid()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncWrite(concatln("LOGIN ", _config._userId),
                     std::bind(&AsyncClient::afterSendLoginRequest, this));
}

void AsyncClient::afterSendLoginRequest()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncReadLine(std::bind(&AsyncClient::afterReceiveLoginChallenge, this, _1));
}

void AsyncClient::afterReceiveLoginChallenge(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    auto challenge = extractSuffix(line, "LOGIN CHALLENGE ");
    if (! challenge)
    {
        handleProtocolError("Invalid login challenge", line);
        return;
    }
    _conn->asyncWrite(concatln("LOGIN RESPONSE ", SHA(_config._password + *challenge)),
                     std::bind(&AsyncClient::afterSendLoginChallengeResponse, this));
}

void AsyncClient::afterSendLoginChallengeResponse()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncReadLine(std::bind(&AsyncClient::afterReceiveLoginChallengeAck, this, _1));
}

void AsyncClient::afterReceiveLoginChallengeAck(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    static const char* okLine = "LOGIN RESPONSE OK";
    static const char* nokLine = "LOGIN RESPONSE NOK";

    bool loginOk;
    if (boost::iequals(line, okLine))
    {
        loginOk = true;
    }
    else if (boost::iequals(line, nokLine))
    {
        loginOk = false;
    }
    else
    {
        handleProtocolError("Invalid login challenge ack", line);
        return;
    }

    if (loginOk)
    {
        _connected = true;
        if (_onConnectHook)
        {
            _onConnectHook();
        }
        _onConnect();
    }
    else
    {
        handleError("Login challenge response rejected");
    }
}

void AsyncClient::issueRetrieveTasksRequest()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncWrite("RETRIEVE TASKS\n", std::bind(&AsyncClient::startReceivingTasks, this));
}

void AsyncClient::startReceivingTasks()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncReadLine(std::bind(&AsyncClient::receiveTaskHeader, this, _1));
}

void AsyncClient::receiveTaskHeader(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    if (_currentTask)
    {
        _tasks.emplace_back(std::move(_currentTask));
    }
    if (boost::iequals(line, "END TASKS"))
    {
        _busy = false;
        if (_onTasksRetrievedHook)
        {
            _onTasksRetrievedHook(std::move(_tasks));
        }
    }
    else
    {
        _currentTask = std::make_unique<Task>();
        if (parse(line,
                  "TASK ", IntToken(_currentTask->_id),
                  " TITLE ", QuotedStringToken(_currentTask->_title),
                  " SPENT ", IntToken(_currentTask->_secondsSpent)))
        {
            _conn->asyncReadLine(std::bind(&AsyncClient::receiveTaskDescription, this, _1));
        }
        else
        {
            handleProtocolError("Invalid task header", line);
        }
    }
}

void AsyncClient::receiveTaskDescription(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    if (line.empty())
    {
        _conn->asyncReadLine(std::bind(&AsyncClient::receiveTaskHeader, this, _1));
    }
    else
    {
        _currentTask->_description.push_back(line);
        _conn->asyncReadLine(std::bind(&AsyncClient::receiveTaskDescription, this, _1));
    }
}

void AsyncClient::issueSendLogsRequest()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncWrite("LOG UPLOAD\n", std::bind(&AsyncClient::readLastTimestamp, this));
}

void AsyncClient::readLastTimestamp()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _conn->asyncReadLine(std::bind(&AsyncClient::startSendingLogs, this, _1));
}

void AsyncClient::startSendingLogs(const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    Timestamp lastTimestamp;
    if (parse(line, "LAST ENTRY AT ", TimestampToken(lastTimestamp)))
    {
        _entrys = _retrieveLogs(lastTimestamp);
    }
    else if (boost::iequals(line, "NO ENTRYS"))
    {
        _entrys = _retrieveLogs(boost::none);
    }
    else
    {
        handleProtocolError("Invalid last entry line", line);
        return;
    }
    sendLogEntry();
}

void AsyncClient::sendLogEntry()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    if (_entrys.empty())
    {
        _conn->asyncWrite("END LOG\n", std::bind(&AsyncClient::finishSendingLogs, this));
    }
    else
    {
        const LogEntry &entry = _entrys.front();
        std::string line = concat(formatTimestamp(entry._timestamp), ' ', entry._userId);
        switch (entry._type)
        {
        case LogEntryType_LOGIN:
            line = concatln(line, " LOGIN");
            break;
        case LogEntryType_LOGOUT:
            line = concatln(line, " LOGOUT");
            break;
        case LogEntryType_TASK_START:
            line = concatln(line, " TASK ", *entry._taskId, " START");
            break;
        case LogEntryType_TASK_PAUSE:
            line = concatln(line, " TASK ", *entry._taskId, " PAUSE");
            break;
        case LogEntryType_TASK_FINISH:
            line = concatln(line, " TASK ", *entry._taskId, " FINISH");
            break;
        default:
            assert(false);
        }
        _conn->asyncWrite(line, std::bind(&AsyncClient::sendNextLogEntry, this));
    }
}

void AsyncClient::sendNextLogEntry()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    assert(! _entrys.empty());
    _entrys.pop_front();
    sendLogEntry();
}

void AsyncClient::finishSendingLogs()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _busy = false;
    if (_onLogsSentHook)
    {
        _onLogsSentHook();
    }
}


void AsyncClient::handleProtocolError(const std::string& errorMsg, const std::string& line)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    handleError(concat("Protocol error: ", errorMsg, ": '", line, '\''));
}

void AsyncClient::handleError(const std::string& errorMsg)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    _connected = false;
    _busy = false;
    if (_conn)
    {
        _conn->detach();
        _conn.reset();
    }
    if (_onErrorHook)
    {
        _onErrorHook(errorMsg);
    }
}
