#include "protocol.h"
#include "sockets.h"
#include "formatedexception.h"
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <crypto++/filters.h>
#include <crypto++/hex.h>
#include <crypto++/osrng.h>

#include <sstream>

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

static void streamTo(std::ostream&) { }

template <typename... Args>
static void streamTo(std::ostream& stream, const char* str, const Args&... args);
template <typename... Args>
static void streamTo(std::ostream& stream, const std::string& str, const Args&... args);

template <typename... Args>
static void streamTo(std::ostream& stream, const char* str, const Args&... args)
{
    stream << str;
    streamTo(stream, args...);
}

template <typename... Args>
static void streamTo(std::ostream& stream, const std::string& str, const Args&... args)
{
    stream << str;
    streamTo(stream, args...);
}

template <typename... Args>
static std::string concat(const Args&... args)
{
    std::ostringstream stream;
    streamTo(stream, args...);
    return stream.str();
}

template <typename... Args>
static std::string concatln(const Args&... args)
{
    std::ostringstream stream;
    streamTo(stream, args...);
    stream << std::endl;
    return stream.str();
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

class ProtocolError : public FormatedException
{
public:
    ProtocolError(const std::string& errorMsg, const std::string& line);
private:
    std::string _errorMsg;
    std::string _line;

    virtual void formatWhatMsg(std::ostream& stream) const;
};

ProtocolError::ProtocolError(const std::string& errorMsg, const std::string& line) :
    _errorMsg(errorMsg),
    _line(line) { }

void ProtocolError::formatWhatMsg(std::ostream& stream) const
{
    stream << _errorMsg << ": '" << _line << '\'';
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
