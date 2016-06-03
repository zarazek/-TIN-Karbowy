#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include "eventdispatcher.h"

#include <thread>
#include <atomic>
#include <boost/uuid/uuid.hpp>
#include <boost/variant.hpp>

typedef boost::variant<Ipv4Address, Ipv6Address> AddressVariant;

class CommunicationThread
{
public:
    CommunicationThread(const boost::uuids::uuid& myUuid,
                        const AddressVariant& serverAddress,
                        const boost::uuids::uuid& serverUuid,
                        const std::string& userId,
                        const std::string& password);
    void start();
    void connect();
    void stop();

private:
    std::string _myUuid;
    AddressVariant _serverAddress;
    std::string _serverUuid;
    std::string _userId;
    std::string _password;
    MainLoop _mainLoop;
    TaskQueue _queue;
    std::unique_ptr<AsyncSocket> _conn;
    std::thread _thread;

    std::string _serverChallenge;

    void run();

    void startConnection();
    void restartConnection();
    void afterConnect();
    void afterSendServerChallenge(const std::string& serverChallenge);
    void afterReceiveServerChallengeResponse(const std::string& serverResponse);
    void afterSendServerChallengeAck();
    void afterReceiveClientChallenge(const std::string& clientChallenge);
    void afterSendClientChallengeResponse();
    void afterReceiveClientChallengeAck(bool clientOk);
    void afterSendClientUuid();
    void afterSendLoginRequest();
    void afterReceiveLoginChallenge(const std::string& loginChallenge);
    void afterSendLoginChallengeResponse();
    void afterReceiveLoginChallengeAck(bool loginOk);

    void executeTask();
};

#endif // COMMUNICATIONTHREAD_H
