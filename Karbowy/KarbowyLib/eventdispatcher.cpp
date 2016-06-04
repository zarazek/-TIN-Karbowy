#include "eventdispatcher.h"
#include "systemerror.h"
#include "concat.h"
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

MainLoop::MainLoop() :
    _run(false) { }

void MainLoop::addObject(WaitableObject& obj)
{
    assert(obj._loop == nullptr);

    auto res1 = _objects.insert(&obj);
    assert(res1.second);
    auto res2 = _descriptorsToObjects.insert(std::make_pair(obj.descriptor(), &obj));
    assert(res2.second);
    obj._loop = this;
}

void MainLoop::removeObject(WaitableObject& obj)
{
    assert(obj._loop == this);

    size_t numOfRemovedObjects = _objects.erase(&obj);
    assert(numOfRemovedObjects == 1);
    numOfRemovedObjects = _descriptorsToObjects.erase(obj.descriptor());
    assert(numOfRemovedObjects == 1);
    obj._loop = nullptr;
}

void MainLoop::removeAllObjects()
{
    for (auto obj : _objects)
    {
        removeObject(*obj);
    }
}

void MainLoop::run()
{
    while (_run)
    {
        fd_set readDescriptors, writeDescriptors;
        int maxFd = -1;
        FD_ZERO(&readDescriptors);
        FD_ZERO(&writeDescriptors);
        for (auto obj : _objects)
        {
            int whatToWaitFor = obj->_whatToWaitFor;
            int fd = obj->descriptor();
            if (whatToWaitFor & WaitableObject::WaitFor_READ)
            {
                FD_SET(fd, &readDescriptors);
            }
            if (whatToWaitFor & WaitableObject::WaitFor_WRITE)
            {
                FD_SET(fd, &writeDescriptors);
            }
            if (whatToWaitFor)
            {
                if (fd > maxFd)
                {
                    maxFd = fd;
                }
            }
        }
        if (select(maxFd + 1, &readDescriptors, &writeDescriptors, nullptr, nullptr) < 0)
        {
            throw SystemError("select error");
        }
        for (int i = 0; i <= maxFd; ++i)
        {
            if (FD_ISSET(i, &readDescriptors))
            {
                auto it = _descriptorsToObjects.find(i);
                if (it != _descriptorsToObjects.end())
                {
                    it->second->handleReadyToRead();
                }
            }
        }
        for (int i = 0; i <= maxFd; ++i)
        {
            if (FD_ISSET(i, &writeDescriptors))
            {
                auto it = _descriptorsToObjects.find(i);
                if (it != _descriptorsToObjects.end())
                {
                    it->second->handleReadyToWrite();
                }
            }
        }
    }
}

void MainLoop::start()
{
    _run = true;
}

void MainLoop::exit()
{
    _run = false;
}

WaitableObject::WaitableObject() :
    _loop(nullptr),
    _whatToWaitFor(0) { }

WaitableObject::~WaitableObject()
{
    assert(! _loop);
}

void WaitableObject::handleReadyToRead()
{
    assert(_loop);
    assert(_whatToWaitFor & WaitFor_READ);

    _whatToWaitFor &= ~WaitFor_READ;
    onReadyToRead();
}

void WaitableObject::handleReadyToWrite()
{
    assert(_loop);
    assert(_whatToWaitFor & WaitFor_WRITE);

    _whatToWaitFor &= ~WaitFor_WRITE;
    onReadyToWrite();
}

AsyncSocket::AsyncSocket(const ErrorHandler& errorHandler) :
    _state(State_BEFORE_CONNECTION),
    _errorHandler(errorHandler) { }

void AsyncSocket::asyncConnect(const Ipv4Address &address, const ConnectHandler &handler)
{
    assert(_state == State_BEFORE_CONNECTION);

    _fd = Descriptor(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0));
    if (_fd < 0)
    {
        handleError("IPv4 socket error", errno);
    }
    int err = connect(_fd, address.address(), address.length());
    if (err == 0)
    {
        _state = State_CONNECTED;
        handler();
    }
    else if (err < 0)
    {
        if (errno == EINPROGRESS)
        {
            _state = State_CONNECTING;
            _connectHandler = handler;
            _whatToWaitFor |= WaitFor_WRITE;
        }
        else
        {
            handleError("IPv4 connect error", errno);
        }
    }
}

void AsyncSocket::asyncConnect(const Ipv6Address &address, const ConnectHandler &handler)
{
    assert(_state == State_BEFORE_CONNECTION);

    _fd = Descriptor(socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0));
    if (_fd < 0)
    {
        handleError("IPv6 socket error", errno);
    }
    int err = connect(_fd, address.address(), address.length());
    if (err == 0)
    {
        _state = State_CONNECTED;
        handler();
    }
    else if (err < 0)
    {
        if (errno == EINPROGRESS)
        {
            _state = State_CONNECTING;
            _connectHandler = handler;
            _whatToWaitFor |= WaitFor_WRITE;
        }
        else
        {
            handleError("IPv6 connect error", errno);
        }
    }
}

void AsyncSocket::asyncReadLine(const ReadHandler& handler)
{
    assert(_state == State_CONNECTED || _state == State_AFTER_CONNECTION);

    if (_inputBuffer.hasFullLine())
    {
        handler(_inputBuffer.getFirstLine());
    }
    else if (_inputBuffer.isEof())
    {
        handleEof();
    }
    else
    {
        _readHandler = handler;
        _whatToWaitFor |= WaitFor_READ;
    }
}

void AsyncSocket::asyncWrite(const std::string& str, const WriteHandler& handler)
{
    _outputBuffer.insert(_outputBuffer.end(), str.begin(), str.end());
    if (writeWhilePossible())
    {
        handler();
    }
    else
    {
        _writeHandler = handler;
        _whatToWaitFor |= WaitFor_WRITE;
    }
}

int AsyncSocket::descriptor() const
{
    return _fd;
}

void AsyncSocket::onReadyToRead()
{
    static const size_t chunkSize = 1024;
    ssize_t bytesRead;
    do {
        char chunk[chunkSize];
        bytesRead = read(_fd, chunk, chunkSize);
        if (bytesRead >= 0)
        {
            _inputBuffer.addData(chunk, bytesRead);
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            handleError("read error", errno);
            return;
        }
    }
    while (bytesRead >= 0);
    if (_inputBuffer.hasFullLine())
    {
        _readHandler(_inputBuffer.getFirstLine());
    }
    else
    {
        _whatToWaitFor |= WaitFor_READ;
    }
}

void AsyncSocket::onReadyToWrite()
{
    switch (_state)
    {
    case State_CONNECTING:
        if (! detectError())
        {
            _state = State_CONNECTED;
            _connectHandler();
        }
        break;
    case State_CONNECTED:
    {
        if (writeWhilePossible())
        {
            _writeHandler();
        }
        else
        {
            _whatToWaitFor |= WaitFor_WRITE;
        }
        break;
    }
    default:
        assert(false);
    }
}

bool AsyncSocket::detectError()
{
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, &error, &len)  < 0)
    {
        handleError("getsockopt error", errno);
        return true;
    }
    if (error == 0)
    {
        return false;
    }
    else
    {
        handleError("socket error", error);
        return true;
    }
}

bool AsyncSocket::writeWhilePossible()
{
    ssize_t bytesWritten = 0;
    while (! _outputBuffer.empty() && bytesWritten >= 0)
    {
        bytesWritten = write(_fd, &_outputBuffer.front(), _outputBuffer.size());
        if (bytesWritten >= 0)
        {
            _outputBuffer.erase(_outputBuffer.begin(), _outputBuffer.begin() + bytesWritten);
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            handleError("write error", errno);
            return false;
        }
    }

    return _outputBuffer.empty();
}

void AsyncSocket::handleError(const std::string& errorMsg, int errorCode)
{
    _errorHandler(concat(errorMsg, ": ", strerror(errorCode), " (errno ", errorCode, ')'));
}

void AsyncSocket::handleEof()
{
    _errorHandler("EOF");
}


TaskQueue::TaskQueue()
{
    int fds[2];
    if (pipe2(fds, 0) < 0)
    {
        throw SystemError("pipe error");
    }
    _readFd = Descriptor(fds[0]);
    _writeFd = Descriptor(fds[1]);
    if (fcntl(_readFd, F_SETFL, O_NONBLOCK) < 0)
    {
        throw SystemError("fcntl error");
    }
    _whatToWaitFor |= WaitFor_READ;
}

void TaskQueue::addTask(const std::function<void()>& task)
{
    {
        std::lock_guard<std::mutex> guard(_tasksMutex);
        _tasks.push_back(task);
    }
    char dummy = 0;
    if (write(_writeFd, &dummy, sizeof(dummy)) < 0)
    {
        throw SystemError("pipe write error");
    }
}

int TaskQueue::descriptor() const
{
    return _readFd;
}

void TaskQueue::onReadyToRead()
{
    char dummy;
    if (read(_readFd, &dummy, sizeof(dummy)) < 0)
    {
        throw SystemError("pipe read error");
    }
    while (auto task = getTask())
    {
        task();
    }
    _whatToWaitFor |= WaitFor_READ;
}

std::function<void()> TaskQueue::getTask()
{
    std::lock_guard<std::mutex> guard(_tasksMutex);
    if (_tasks.empty())
    {
        return std::function<void()>();
    }
    else
    {
        auto task = _tasks.front();
        _tasks.pop_front();
        return task;
    }
}

void TaskQueue::onReadyToWrite()
{
    assert(false);
}

//void TaskQueue::onEof()
//{
//    throw std::runtime_error("pipe EOF");
//}

//void TaskQueue::onError()
//{
//    throw std::runtime_error("pipe error");
//}

