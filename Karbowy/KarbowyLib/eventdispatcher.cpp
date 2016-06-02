#include "eventdispatcher.h"
#include "systemerror.h"
#include <string.h>
#include <unistd.h>
#include <assert.h>

MainLoop::MainLoop() :
    _fd(epoll_create1(0)),
    _run(true)
{
    if (_fd < 0)
    {
        throw SystemError("epoll_create1 error");
    }
}

void MainLoop::addObject(WaitableObject& obj)
{
    assert(obj._loop == nullptr);

    int fd = obj.descriptor();
    assert(fd >= 0);
    int whatToWaitFor = obj._whatToWaitFor;
    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = whatToWaitFor | EPOLLONESHOT;
    ev.data.fd = fd;
    ev.data.ptr = &obj;
    if (epoll_ctl(_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        throw SystemError("epoll_clt add error");
    }
    obj._loop = this;
}

void MainLoop::removeObject(WaitableObject& obj)
{
    assert(obj._loop == this);

    if (epoll_ctl(_fd, EPOLL_CTL_DEL, obj.descriptor(), nullptr) < 0)
    {
        throw SystemError("epoll_clt del error");
    }
    obj._loop = nullptr;
}

void MainLoop::run()
{
    while (_run)
    {
        epoll_event ev;
        int numOfEvents = epoll_wait(_fd, &ev, 1, std::numeric_limits<int>::max());
        if (numOfEvents < 0)
        {
            throw SystemError("epoll_wait error");
        }
        assert(numOfEvents <= 1);
        if (numOfEvents)
        {
            WaitableObject* obj = static_cast<WaitableObject*>(ev.data.ptr);
            if (ev.events & EPOLLIN)
            {
                obj->handleReadyToRead();
            }
            if (ev.events & EPOLLOUT)
            {
                obj->handleReadyToWrite();
            }
            if (ev.events & EPOLLHUP)
            {
                obj->handleEof();
            }
            if (ev.events & EPOLLERR)
            {
                obj->handleError();
            }
            if (obj->_loop == this)
            {
                ev.events = obj->_whatToWaitFor | EPOLLONESHOT;
                epoll_ctl(_fd, EPOLL_CTL_MOD, ev.data.fd, &ev);
            }
        }
    }
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
    if (_loop)
    {
        _loop->removeObject(*this);
    }
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

void WaitableObject::handleEof()
{
    assert(_loop);

    onEof();
}

void WaitableObject::handleError()
{
    assert(_loop);

    onError();
}

AsyncSocket::AsyncSocket(const EofHandler& eofHandler, const ErrorHandler& errorHandler) :
    _state(State_BEFORE_CONNECTION),
    _eofHandler(eofHandler),
    _errorHandler(errorHandler) { }

void AsyncSocket::asyncConnect(const Ipv4Address &address, const ConnectHandler &handler)
{
    assert(_loop);
    assert(_state == State_BEFORE_CONNECTION);

    _fd = Descriptor(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0));
    if (_fd < 0)
    {
        _errorHandler(*this, "IPv4 socket error", errno);
    }
    int err = connect(_fd, address.address(), address.length());
    if (err == 0)
    {
        _state = State_CONNECTED;
        handler(*this);
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
            _errorHandler(*this, "IPv4 connect error", errno);
        }
    }
}

void AsyncSocket::asyncConnect(const Ipv6Address &address, const ConnectHandler &handler)
{
    assert(_loop);
    assert(_state == State_BEFORE_CONNECTION);

    _fd = Descriptor(socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0));
    if (_fd < 0)
    {
        _errorHandler(*this, "IPv6 socket error", errno);
    }
    int err = connect(_fd, address.address(), address.length());
    if (err == 0)
    {
        _state = State_CONNECTED;
        handler(*this);
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
            _errorHandler(*this, "IPv6 connect error", errno);
        }
    }
}

void AsyncSocket::asyncReadLine(const ReadHandler& handler)
{
    assert(_state == State_CONNECTED || _state == State_AFTER_CONNECTION);

    if (_inputBuffer.hasFullLine())
    {
        handler(*this, _inputBuffer.getFirstLine());
    }
    else if (_inputBuffer.isEof())
    {
        _eofHandler(*this);
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
        handler(*this);
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
            _errorHandler(*this, "read error", errno);
            return;
        }
    }
    while (bytesRead >= 0);
    if (_inputBuffer.hasFullLine())
    {
        _readHandler(*this, _inputBuffer.getFirstLine());
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
            _connectHandler(*this);
        }
        break;
    case State_CONNECTED:
    {
        if (writeWhilePossible())
        {
            _writeHandler(*this);
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

void AsyncSocket::onEof()
{
    _eofHandler(*this);
}

void AsyncSocket::onError()
{
    detectError();
}

bool AsyncSocket::detectError()
{
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, &error, &len)  < 0)
    {
        _errorHandler(*this, "getsockopt error", errno);
        return true;
    }
    if (error == 0)
    {
        return false;
    }
    else
    {
        _errorHandler(*this, "socket error", error);
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
            _errorHandler(*this, "write error", errno);
            return false;
        }
    }

    return _outputBuffer.empty();
}

