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
    void sendLogs();
private:
    MainLoop _mainLoop;
    TaskQueue _queue;
    ClientConfig _config;
    AsyncClient _client;
    std::thread _thread;
    int _userId;

    void run();
    void onConnectSuccess();
    void retrieveTasksOnCommThread();
    void onTasksRetrieved(std::vector<std::unique_ptr<Task> >&& tasks);
    void sendLogsOnCommThread();
};

#endif // COMMUNICATIONTHREAD_H
