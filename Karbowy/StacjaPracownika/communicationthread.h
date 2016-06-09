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

    void login(const ClientConfig& config);
    void retrieveTasks();
    void sendLogs();
    void logout();

signals:
    void loggedIn(int employeeId);
    void loggedOut();
    void tasksRetrieved();
    void logsSent();
    void error(QString errorMsg);
private:
    MainLoop _mainLoop;
    TaskQueue _queue;
    ClientConfig _config;
    AsyncClient _client;
    std::thread _thread;
    int _userId;

    void run();
    void loginOnCommThread(ClientConfig config);
    void onConnectSuccess();
    void retrieveTasksOnCommThread();
    void onTasksRetrieved(std::vector<std::unique_ptr<ClientTask> >&& tasks);
    void sendLogsOnCommThread();
    void enqueueIfNotBusy(const std::function<void()>& task, const char* busyMsg);
};

#endif // COMMUNICATIONTHREAD_H
