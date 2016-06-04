#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include "protocol.h"
#include <thread>

class CommunicationThread
{
public:
    CommunicationThread();
    ~CommunicationThread();
    void start();
    void stop();

    void setClientConfig(ClientConfig config);
    void retrieveTasks();
private:
    MainLoop _mainLoop;
    TaskQueue _queue;
    ClientConfig _config;
    AsyncClient _client;
    std::thread _thread;

    void run();
    void retrieveTasksOnCommThread();
};

#endif // COMMUNICATIONTHREAD_H
