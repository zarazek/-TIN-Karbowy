#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include "protocol.h"
#include <QObject>
#include <thread>

class CommunicationThread : public QObject
{
    Q_OBJECT

public:
    CommunicationThread();
    ~CommunicationThread();
    void start();
    void stop();

    void setClientConfig(ClientConfig config);
    void retrieveTasks();
    void sendLogs();
    bool busy() const { return _client.busy(); }

signals:
    void loginSuccessfull(int employeeId);
    void tasksChanged();
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
    void onTasksRetrieved(std::vector<std::unique_ptr<ClientTask> >&& tasks);
    void sendLogsOnCommThread();
};

#endif // COMMUNICATIONTHREAD_H
