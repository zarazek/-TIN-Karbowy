#include "communicationthread.h"
#include "protocol.h"
#include "parse.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

static void onError(const std::string& errorMsg)
{
    std::cerr << errorMsg << std::endl;
}

CommunicationThread::CommunicationThread() :
    _client(_mainLoop, _config, onError)
{
    _mainLoop.addObject(_queue);
}

CommunicationThread::~CommunicationThread()
{
    _mainLoop.removeAllObjects();
}

void CommunicationThread::start()
{
    _mainLoop.start();
    _thread = std::thread(&CommunicationThread::run, this);
}

void CommunicationThread::stop()
{
    _queue.addTask([this] { _mainLoop.exit(); });
    _thread.join();
}

void CommunicationThread::setClientConfig(ClientConfig config)
{
    _queue.addTask([this, config] { _config = config; });
}

void CommunicationThread::retrieveTasks()
{
    _queue.addTask(std::bind(&CommunicationThread::retrieveTasksOnCommThread, this));
}

void CommunicationThread::run()
{
    try {
        _mainLoop.run();
    }
    catch (std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}



static void onTasksRetrieved(std::vector<std::unique_ptr<Task> >&& tasks)
{
    for (const auto& task : tasks)
    {
        std::cout << "TASK " << task->_id << " TITLE " << quoteString(task->_title) << " SPENT " << task->_secondsSpent << std::endl;
        for (const auto& line : task->_description)
        {
            std::cout << line << std::endl;
        }
        std::cout << std::endl;
    }
    std::cout << "END TASKS" << std::endl;
}

void CommunicationThread::retrieveTasksOnCommThread()
{
    _client.retrieveTasks(onTasksRetrieved);
}
