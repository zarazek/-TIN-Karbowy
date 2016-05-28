#include "clientconnection.h"
#include "formatedexception.h"
#include "server.h"
#include "predefinedqueries.h"
#include "employee.h"
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <crypto++/sha.h>
#include <crypto++/filters.h>
#include <crypto++/hex.h>
#include <crypto++/osrng.h>
#include <iostream>

class ClientError : public FormatedException
{
public:
    ClientError(const std::string& errorMsg, const std::string& line);
private:
    std::string _errorMsg;
    std::string _line;

    virtual void formatWhatMsg(std::ostream& stream) const;
};

ClientError::ClientError(const std::string& errorMsg, const std::string& line) :
    _errorMsg(errorMsg),
    _line(line) { }

void ClientError::formatWhatMsg(std::ostream& stream) const
{
    stream << _errorMsg << ": '" << _line << '\'';
}

ClientConnection::ClientConnection(Server& server, TcpStream&& stream) :
    _server(server),
    _stream(std::move(stream)) { }

void ClientConnection::start()
{
    _thread = std::thread(&ClientConnection::run, this);
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

std::string ClientConnection::receiveServerChallenge()
{
    static const char *serverChallengeCmd = "SERVER CHALLENGE ";

    std::string line = _stream.readLine();
    auto challenge = extractSuffix(line, serverChallengeCmd);
    if (challenge)
    {
        return *challenge;
    }
    else
    {
        throw ClientError("Invalid server challenge", line);
    }
}

static std::string SHA(const std::string& message)
{
    CryptoPP::SHA256 hash;
    std::string digest;
    CryptoPP::StringSource s(message, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
    return digest;
}

void ClientConnection::sendServerChallengeResponse(const std::string& serverChallenge)
{
    std::ostringstream stream;
    stream << "SERVER RESPONSE " << SHA(_server.uuidStr() + serverChallenge);
    _stream.writeLine(stream.str());
}

bool ClientConnection::receiveServerChallengeAck()
{
    static const char* serverChallengeResponseOk = "SERVER RESPONSE OK";
    static const char* serverChallengeResponseNok = "SERVER RESPONSE NOK";

    std::string line = _stream.readLine();
    if (boost::iequals(line, serverChallengeResponseOk))
    {
        return true;
    }
    else if (boost::iequals(line, serverChallengeResponseNok))
    {
        std::cerr << "Server challege response not accepted\n";
        return false;
    }
    else
    {
        throw ClientError("Invalid server challenge ack", line);
    }
}

static std::string generateChallenge()
{
    CryptoPP::AutoSeededRandomPool prng;
    CryptoPP::SecByteBlock seed(16);
    prng.GenerateBlock(seed, seed.size());
    std::string challenge;
    CryptoPP::ArraySource(seed, seed.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(challenge)));
    std::cout << "challenge = " << challenge << std::endl;
    return challenge;
}

std::string ClientConnection::sendClientChallenge()
{
    std::string clientChallenge = generateChallenge();
    std::ostringstream stream;
    stream << "CLIENT CHALLENGE " << clientChallenge;
    _stream.writeLine(stream.str());
    return clientChallenge;
}

std::string ClientConnection::receiveClientChallengeResponse()
{
    static const char* clientChallengeResponse = "CLIENT RESPONSE ";

    std::string line = _stream.readLine();
    auto response = extractSuffix(line, clientChallengeResponse);
    if (! response)
    {
        throw ClientError("Invalid client challenge response", line);
    }
    return *response;
}

bool ClientConnection::verifyClientChallengeResponse(const std::string& challenge, const std::string& response)
{
    return SHA(_server.uuidStr() + challenge) == response;
}

void ClientConnection::sendClientChallengeAck(bool isOk)
{
    static const char* ack = "CLIENT RESPONSE NOK";
    static const char* nack = "CLIENT RESPONSE OK";

    _stream.writeLine(isOk ? ack : nack);
}

boost::uuids::uuid ClientConnection::receiveClientUuid()
{
    static const char* uuidCmd = "CLIENT UUID ";

    std::string line = _stream.readLine();
    auto maybeUuidStr = extractSuffix(line, uuidCmd);
    if (maybeUuidStr)
    {
        try
        {
            return boost::lexical_cast<boost::uuids::uuid>(*maybeUuidStr);
        }
        catch (boost::bad_lexical_cast&)
        {
            throw ClientError("Invalid client uuid", *maybeUuidStr);
        }
    }
    else
    {
        throw ClientError("Invalid client uuid cmd", line);
    }
}

std::string ClientConnection::receiveLoginRequest()
{
    static const char* loginCmd = "LOGIN ";

    std::string line = _stream.readLine();
    auto maybeUserId = extractSuffix(line, loginCmd);
    if (! maybeUserId)
    {
        throw ClientError("Invalid login request", line);
    }
    return *maybeUserId;
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

std::string ClientConnection::sendLoginChallenge()
{
    std::string loginChallenge = generateChallenge();
    std::ostringstream stream;
    stream << "LOGIN CHALLENGE " << loginChallenge;
    _stream.writeLine(stream.str());
    return loginChallenge;
}

std::string ClientConnection::receiveLoginChallengeResponse()
{
    std::string line = _stream.readLine();
    auto maybeResponse = extractSuffix(line, "LOGIN RESPONSE ");
    if (! maybeResponse)
    {
        throw ClientError("Invalid login response", line);
    }
    return *maybeResponse;
}

bool ClientConnection::verifyLoginChallengeResponse(const std::string& password, const std::string& challenge, const std::string& response)
{
    return response == SHA(password + challenge);
}

bool ClientConnection::initializeConnection()
{
    std::string serverChallenge = receiveServerChallenge();
    sendServerChallengeResponse(serverChallenge);
    if (! receiveServerChallengeAck())
    {
        return false;
    }
    std::string clientChallenge = sendClientChallenge();
    std::string clientResponse = receiveClientChallengeResponse();
    bool clientOk = verifyClientChallengeResponse(clientChallenge, clientResponse);
    sendClientChallengeAck(clientOk);
    if (! clientOk)
    {
        return false;
    }
    boost::uuids::uuid clientUuid = receiveClientUuid();
    std::string userId = receiveLoginRequest();
    std::unique_ptr<Employee> employee = verifyUserId(userId);
    if (! employee)
    {
        return false;
    }
    std::string loginChallenge = sendLoginChallenge();
    std::string loginResponse = receiveLoginChallengeResponse();
    if (! verifyLoginChallengeResponse(employee->_password, loginChallenge, loginResponse))
    {
        return false;
    }
    _clientUuid = clientUuid;
    _userId = userId;
    return true;
}

bool ClientConnection::handleCommand(const std::string& line)
{
    // TODO
    return true;
}

void ClientConnection::onShutdown()
{
    // TODO
}

void ClientConnection::run()
{
    try
    {
        if (initializeConnection())
        {
            bool run = true;
            while (run)
            {
                run = handleCommand(_stream.readLine());
            }
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Client exception: " << ex.what() << std::endl;
    }
    onShutdown();
}
