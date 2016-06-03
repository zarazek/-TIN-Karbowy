#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <functional>

class TcpStream;
class AsyncSocket;

std::string sendServerChallenge(TcpStream& conn);
std::string sendClientChallenge(TcpStream& conn);
std::string sendLoginChallenge(TcpStream &conn);

void asyncSendServerChallenge(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);
void asyncSendClientChallenge(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);
void asyncSendLoginChallenge(AsyncSocket &conn, const std::function<void(const std::string&)>& fn);

std::string receiveServerChallenge(TcpStream& conn);
std::string receiveClientChallenge(TcpStream& conn);
std::string receiveLoginChallenge(TcpStream& conn);

void asyncReceiveServerChallenge(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);
void asyncReceiveClientChallenge(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);
void asyncReceiveLoginChallenge(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);


void sendServerChallengeResponse(TcpStream &conn, const std::string& secret, const std::string& challenge);
void sendClientChallengeResponse(TcpStream &conn, const std::string& secret, const std::string& challenge);
void sendLoginChallengeResponse(TcpStream &conn, const std::string& secret, const std::string& challenge);

void asyncSendServerChallengeResponse(AsyncSocket &conn, const std::string& secret, const std::string& challenge, const std::function<void()>& fn);
void asyncSendClientChallengeResponse(AsyncSocket &conn, const std::string& secret, const std::string& challenge, const std::function<void()>& fn);
void asyncSendLoginChallengeResponse(AsyncSocket &conn, const std::string& secret, const std::string& challenge, const std::function<void()>& fn);

std::string receiveServerChallengeResponse(TcpStream& conn);
std::string receiveClientChallengeResponse(TcpStream& conn);
std::string receiveLoginChallengeResponse(TcpStream& conn);

void asyncReceiveServerChallengeResponse(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);
void asyncReceiveClientChallengeResponse(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);
void asyncReceiveLoginChallengeResponse(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);

bool verifyChallengeResponse(const std::string& secret, const std::string& challenge, const std::string& response);

void sendServerChallengeAck(TcpStream& conn, bool ok);
void sendClientChallengeAck(TcpStream& conn, bool ok);
void sendLoginChallengeAck(TcpStream& conn, bool ok);

void asyncSendServerChallengeAck(AsyncSocket& conn, bool ok, const std::function<void()>& fn);
void asyncSendClientChallengeAck(AsyncSocket& conn, bool ok, const std::function<void()>& fn);
void asyncSendLoginChallengeAck(AsyncSocket& conn, bool ok, const std::function<void()>& fn);

bool receiveServerChallengeAck(TcpStream& conn);
bool receiveClientChallengeAck(TcpStream& conn);
bool receiveLoginChallengeAck(TcpStream& conn);

void asyncReceiveServerChallengeAck(AsyncSocket& conn, const std::function<void(bool)>& fn);
void asyncReceiveClientChallengeAck(AsyncSocket& conn, const std::function<void(bool)>& fn);
void asyncReceiveLoginChallengeAck(AsyncSocket& conn, const std::function<void(bool)>& fn);

void sendClientUuid(TcpStream& conn, const std::string& uuid);

void asyncSendClientUuid(AsyncSocket& conn, const std::string& uuid, const std::function<void()>& fn);

std::string receiveClientUuid(TcpStream& conn);

void asyncReceiveClientUuid(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);

void sendLoginRequest(TcpStream& conn, const std::string& userId);

void asyncSendLoginRequest(AsyncSocket& conn, const std::string& userId, const std::function<void()>& fn);

std::string receiveLoginRequest(TcpStream& conn);

void asyncReceiveLoginRequest(AsyncSocket& conn, const std::function<void(const std::string&)>& fn);

#endif // PROTOCOL_H
