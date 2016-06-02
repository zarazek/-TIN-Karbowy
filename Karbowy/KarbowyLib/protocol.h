#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <functional>

class TcpStream;
class AsyncSocket;

std::string sendServerChallenge(TcpStream& conn);
std::string sendClientChallenge(TcpStream& conn);
std::string sendLoginChallenge(TcpStream &conn);

typedef std::function<void(const std::string&)> SendChallengeContinuation;

void asyncSendServerChallenge(AsyncSocket& conn, const SendChallengeContinuation& fn);
void asyncSendClientChallenge(AsyncSocket& conn, const SendChallengeContinuation& fn);
void asyncSendLoginChallenge(AsyncSocket &conn, const SendChallengeContinuation& fn);

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

#endif // PROTOCOL_H
