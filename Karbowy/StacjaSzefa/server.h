#ifndef SERVER_H
#define SERVER_H

#include "sockets.h"
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ClientConnection;
class TasksTableModel;

class Server
{
public:
    Server(std::string&& uuid, uint16_t port);

    const std::string& uuid() const
    {
        return _uuid;
    }

    void setTasksTableModel(TasksTableModel& model);
    void start();
    void stop();
    void removeClient(const std::shared_ptr<ClientConnection>& client);
private:
    std::string _uuid;
    Ipv4Listener _ipv4Listener;
    Ipv6Listener _ipv6Listener;
    std::thread _ipv4Thread;
    std::thread _ipv6Thread;
    std::thread _reaperThread;
    std::mutex _clientsToRemoveMutex;
    std::condition_variable _reaperCondition;
    std::deque<std::shared_ptr<ClientConnection> > _clientsToRemove;
    std::mutex _clientsMutex;
    std::set<std::shared_ptr<ClientConnection> > _clients;
    std::atomic<bool> _run;
    TasksTableModel* _tasksModel;

    void runListener(Listener* listener, const char* className);
    std::shared_ptr<ClientConnection> getClientToRemove();
    void runReaper();
};

#endif // SERVER_H
