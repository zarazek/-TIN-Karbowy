#include "communicationthread.h"
#include "protocol.h"
#include "parse.h"
#include "task.h"
#include "predefinedqueries.h"
#include <functional>
#include <iostream>

using namespace std::placeholders;

static void onError(const std::string& errorMsg)
{
    std::cerr << errorMsg << std::endl;
}

CommunicationThread::CommunicationThread() :
    _client(_mainLoop, _config, onError, std::bind(&CommunicationThread::onConnectSuccess, this))
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

void CommunicationThread::login(const ClientConfig& config)
{
    enqueueIfNotBusy(std::bind(&CommunicationThread::loginOnCommThread, this, config),
                     "Can't log in");
}

void CommunicationThread::retrieveTasks()
{
    enqueueIfNotBusy(std::bind(&CommunicationThread::retrieveTasksOnCommThread, this),
                     "Can't retrieve tasks");
}

void CommunicationThread::sendLogs()
{
    enqueueIfNotBusy(std::bind(&CommunicationThread::sendLogsOnCommThread, this),
                     "Can't send logs");
}

void CommunicationThread::logout()
{
    _queue.addTask([this] { _client.disconnect(); });
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

void CommunicationThread::loginOnCommThread(ClientConfig config)
{
    _config = config;
    _client.connect(std::bind(&CommunicationThread::onConnectSuccess, this));
}

void CommunicationThread::onConnectSuccess()
{
    auto& findUserId = findUserIdByLoginQ();
    findUserId.execute(_config._userId);
    if (! findUserId.next(_userId))
    {
        auto& insertUser = insertUserC();
        insertUser.execute(_config._userId);
        findUserId.execute(_config._userId);
        bool res = findUserId.next(_userId);
        assert(res);
    }
    emit loggedIn(_userId);
}

void CommunicationThread::retrieveTasksOnCommThread()
{
    _client.retrieveTasks(std::bind(&CommunicationThread::onTasksRetrieved, this, _1));
}

void CommunicationThread::onTasksRetrieved(std::vector<std::unique_ptr<ClientTask> >&& tasks)
{
    for (const auto& task : tasks)
    {
        std::cout << "TASK " << task->_id << " TITLE " << quoteString(task->_title) << " SPENT " << toSeconds(task->_timeSpent) << std::endl;
        for (const auto& line : task->_description)
        {
            std::cout << line << std::endl;
        }
        std::cout << std::endl;
    }
    std::cout << "END TASKS" << std::endl;

    auto& deleteTaskAssociations = deleteTaskAssociationsForUserC();
    deleteTaskAssociations.execute(_userId);
    auto& insertTask = insertTaskC();
    auto& insertAssociation = insertTaskAssociationC();
    for (const auto& task : tasks)
    {
        insertTask.execute(task->_id, task->_title, boost::join(task->_description, "\n"));
        insertAssociation.execute(_userId, task->_id, task->_timeSpent);
    }
    emit tasksRetrieved();
}

static AsyncClient::LogEntryList retrieveLogs(const boost::optional<Timestamp>&)
{
    AsyncClient::LogEntryList res;
    LogEntry entry;
    entry._userId = "wwisniew";
    entry._timestamp = Clock::now() - std::chrono::hours(3);
    entry._type = LogEntryType_LOGIN;
    entry._taskId = boost::none;
    res.push_back(entry);
    entry._timestamp += std::chrono::minutes(1);
    entry._type = LogEntryType_TASK_START;
    entry._taskId = 1;
    res.push_back(entry);
    entry._timestamp += std::chrono::minutes(30);
    entry._type = LogEntryType_TASK_PAUSE;
    res.push_back(entry);
    entry._timestamp += std::chrono::minutes(5);
    entry._type = LogEntryType_TASK_START;
    res.push_back(entry);
    entry._timestamp += std::chrono::minutes(90);
    entry._type = LogEntryType_TASK_PAUSE;
    res.push_back(entry);
    entry._timestamp += std::chrono::seconds(2);
    entry._type = LogEntryType_LOGOUT;
    entry._taskId = boost::none;
    res.push_back(entry);
    return res;
}

static void onLogsSent()
{
    std::cout << "LOGS SENT\n";
}

void CommunicationThread::sendLogsOnCommThread()
{
    _client.sendLogs(retrieveLogs, onLogsSent);
}

void CommunicationThread::enqueueIfNotBusy(const std::function<void ()> &task, const char *busyMsg)
{
    if (_client.busy())
    {
        emit error(QString("%1: communication thread busy").arg(busyMsg));
    }
    else
    {
        _queue.addTask(task);
    }
}
