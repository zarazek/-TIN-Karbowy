#include "server.h"
#include "clientconnection.h"
#include "taskstablemodel.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <signal.h>
#include <iostream>

Server::Server(std::string&& uuid, uint16_t port) :
    _uuid(std::forward<std::string>(uuid)),
    _ipv4Listener(port),
    _ipv6Listener(port),
    _run(false),
    _tasksModel(nullptr) { }

void Server::setTasksTableModel(TasksTableModel& model)
{
    _tasksModel = &model;
}

void Server::start()
{
    _run = true;
    _ipv4Thread = std::thread(&Server::runListener, this, &_ipv4Listener, "Ipv4Listener");
    _ipv6Thread = std::thread(&Server::runListener, this, &_ipv6Listener, "Ipv6Listener");
    _reaperThread = std::thread(&Server::runReaper, this);
}

void Server::stop()
{
    _run = false;
    pthread_kill(_ipv4Thread.native_handle(), SIGUSR1);
    pthread_kill(_ipv6Thread.native_handle(), SIGUSR1);
    _ipv4Thread.join();
    _ipv6Thread.join();
    {
        std::lock_guard<std::mutex> guard(_clientsMutex);
        for (const auto& client : _clients)
        {
            client->stop();
        }
    }
    _reaperCondition.notify_one();
    _reaperThread.join();
    assert(_clients.empty());
    assert(_clientsToRemove.empty());
}

void Server::removeClient(const std::shared_ptr<ClientConnection>& client)
{
    std::lock_guard<std::mutex> guard(_clientsToRemoveMutex);
    _clientsToRemove.push_back(client);
    _reaperCondition.notify_one();
}

void Server::runListener(Listener* listener, const char* className)
{
    try
    {
        while (_run)
        {
            auto stream = listener->awaitConnection();
            if (_run)
            {
                auto client = std::make_shared<ClientConnection>(*this, std::move(stream));
                QObject::connect(client.get(), &ClientConnection::tasksStatusChanged,
                                 _tasksModel, &TasksTableModel::refresh,
                                 Qt::QueuedConnection);
                client->start();
                std::lock_guard<std::mutex> guard(_clientsMutex);
                auto res = _clients.insert(client);
                assert(res.second);
            }
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << className << " exception: " << ex.what() << std::endl;
    }
}

std::shared_ptr<ClientConnection> Server::getClientToRemove()
{
    std::unique_lock<std::mutex> lock(_clientsToRemoveMutex);
    _reaperCondition.wait(lock, [this]() { return !_run || ! _clientsToRemove.empty(); });
    std::shared_ptr<ClientConnection> client;
    if (! _clientsToRemove.empty())
    {
        client = _clientsToRemove.front();
        _clientsToRemove.pop_front();
    }
    return client;
}

void Server::runReaper()
{
    while (true)
    {
        auto client = getClientToRemove();
        if (client)
        {
           client->waitToFinish();
        }
        std::lock_guard<std::mutex> guard(_clientsMutex);
        size_t numOfRemovedElements = _clients.erase(client);
        assert(numOfRemovedElements == (client ? 1 : 0));
        if (! _run && _clients.empty())
        {
            break;
        }
    }
}
