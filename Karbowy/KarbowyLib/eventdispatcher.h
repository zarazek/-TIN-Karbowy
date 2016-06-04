#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#include "sockets.h"
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <functional>
#include <mutex>
#include <atomic>
#include <sys/epoll.h>

class WaitableObject;

class MainLoop
{
public:
   MainLoop();
   void addObject(WaitableObject& object);
   void removeObject(WaitableObject& object);
   void removeAllObjects();
   void start();
   void exit();
   void run();
private:
   std::set <WaitableObject*> _objects;
   std::map<int, WaitableObject*> _descriptorsToObjects;
   std::atomic<bool> _run;
};

class WaitableObject
{
public:
    enum WaitFor
    {
        WaitFor_READ = EPOLLIN,
        WaitFor_WRITE = EPOLLOUT,
    };

    WaitableObject();
    virtual ~WaitableObject();

protected:
    MainLoop* _loop;
    int _whatToWaitFor;

private:
    // interface for MainLoop:
    virtual int descriptor() const = 0;
    void handleReadyToRead();
    void handleReadyToWrite();

    virtual void onReadyToRead() = 0;
    virtual void onReadyToWrite() = 0;

    friend class MainLoop;
};

class AsyncSocket : public WaitableObject
{
public:
    typedef std::function<void(const std::string&)> ErrorHandler;
    typedef std::function<void()> ConnectHandler;
    typedef std::function<void(const std::string&)> ReadHandler;
    typedef std::function<void()> WriteHandler;

    AsyncSocket(const ErrorHandler& onError);
    void asyncConnect(const Ipv4Address& address, const ConnectHandler& handler);
    void asyncConnect(const Ipv6Address& address, const ConnectHandler& handler);
    void asyncReadLine(const ReadHandler& handler);
    void asyncWrite(const std::string& line, const WriteHandler &handler);
private:
    enum State
    {
        State_BEFORE_CONNECTION,
        State_CONNECTING,
        State_CONNECTED,
        State_AFTER_CONNECTION
    };

    State _state;
    Descriptor _fd;
    LineBuffer _inputBuffer;
    std::vector<char> _outputBuffer;
    ErrorHandler _errorHandler;
    ConnectHandler _connectHandler;
    ReadHandler _readHandler;
    WriteHandler _writeHandler;

    int descriptor() const override;
    void onReadyToRead() override;
    void onReadyToWrite() override;

    bool detectError();

    enum WriteResult
    {
        WriteResult_COMPLETE,
        WriteResult_INCOMPLETE,
        WriteResult_ERROR
    };

    WriteResult writeWhilePossible();
    void handleError(const std::string& errorMsg, int errorCode);
    void handleEof();
};

class TaskQueue : public WaitableObject
{
public:
    TaskQueue();
    void addTask(const std::function<void()>& task);
private:
    Descriptor _readFd;
    Descriptor _writeFd;
    std::mutex _tasksMutex;
    std::deque<std::function<void()> > _tasks;

    int descriptor() const override;
    void onReadyToRead() override;
    void onReadyToWrite() override;
//    void onEof() override;
//    void onError() override;

    std::function<void()> getTask();
};


#endif // EVENTDISPATCHER_H
