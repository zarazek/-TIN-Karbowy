#include "clientconnection.h"
#include "protocol.h"
#include "parse.h"
#include "concat.h"
#include "protocolerror.h"
#include "server.h"
#include "predefinedqueries.h"
#include "employee.h"
#include <boost/algorithm/string.hpp>
#include <signal.h>

#include <iostream>

ClientConnection::ClientConnection(Server& server, TcpStream&& stream) :
    _server(server),
    _stream(std::move(stream)),
    _run(false) { }

void ClientConnection::start()
{
    _run = true;
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
    if (! receiveServerChallengeAck(_stream))
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
    bool loginOk = verifyChallengeResponse(employee->_password, loginChallenge, loginResponse);
    sendLoginChallengeAck(_stream, loginOk);
    if (! loginOk)
    {
        return false;
    }

    auto& insertClientUuid = insertClientUuidC();
    insertClientUuid.execute(clientUuid);
    auto& findClientIdByUuid = findClientIdByUuidQ();
    findClientIdByUuid.execute(clientUuid);
    if (! findClientIdByUuid.next(_clientId))
    {
        throw std::runtime_error("Client id not found after insert");
    }
    if (findClientIdByUuid.next(_clientId))
    {
        throw std::runtime_error("Many client ids found after insert");
    }
    _userId = userId;
    return true;
}

void ClientConnection::handleCommand(const std::string& line)
{
    if (boost::iequals(line, "RETRIEVE TASKS"))
    {
        auto& query = findTasksForLoginQ();
        query.execute(_userId);
        std::unique_ptr<Task> task;
        while (query.next(task))
        {
            _stream.writeLine(concatln("TASK ", task->_id, " TITLE ", quoteString(task->_title), " SPENT ", task->_secondsSpent));
            for (const auto& line : task->_description)
            {
                _stream.writeLine(concatln(line));
            }
            _stream.writeLine("\n");
        }
        _stream.writeLine("END TASKS\n");
    }
    else if (boost::iequals(line, "LOG UPLOAD"))
    {
        auto& lastEntryTimeQ = findLastEntryTimeQ();
        lastEntryTimeQ.execute(_clientId);
        boost::optional<Timestamp> lastEntryTime;
        if (! lastEntryTimeQ.next(lastEntryTime))
        {
            throw std::runtime_error("No results from findLastEntryTime query");
        }
        if (lastEntryTimeQ.next(lastEntryTime))
        {
            throw std::runtime_error("Too many results from lastEntryTime query");
        }
        if (lastEntryTime)
        {
            _stream.writeLine(concatln("LAST ENTRY AT ", formatTimestamp(*lastEntryTime)));
        }
        else
        {
            _stream.writeLine("NO ENTRYS\n");
        }
        int taskId;
        bool loop = true;
        while (loop)
        {
            auto line = _stream.readLine();
            LogEntry entry;
            if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId), " LOGIN"))
            {
                entry._type = LogEntryType_LOGIN;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId), " LOGOUT"))
            {
                entry._type = LogEntryType_LOGOUT;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId),
                           " TASK ", IntToken(taskId), " START"))
            {
                entry._type = LogEntryType_TASK_START;
                entry._taskId = taskId;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId),
                           " TASK ", IntToken(taskId), " PAUSE"))
            {
                entry._type = LogEntryType_TASK_PAUSE;
                entry._taskId = taskId;
            }
            else if (parse(line, TimestampToken(entry._timestamp), " ", BareStringToken(entry._userId),
                           " TASK ", IntToken(taskId), " FINISH"))
            {
                entry._type = LogEntryType_TASK_FINISH;
                entry._taskId = taskId;
            }
            else if (boost::iequals(line, "END LOG"))
            {
                loop = false;
            }
            else
            {
                throw ProtocolError("Invalid log entry", line);
            }

            if (loop)
            {
                insertLogEntry(entry);
            }
        }
    }
    else
    {
        throw ProtocolError("Invalid command", line);
    }
}

void ClientConnection::insertLogEntry(const LogEntry& entry)
{
    auto& cmd = insertLogEntryC();
    cmd.execute(entry._type, _clientId, entry._userId, entry._timestamp, entry._taskId);
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
