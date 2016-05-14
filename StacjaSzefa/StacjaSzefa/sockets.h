#ifndef SOCKETS_H
#define SOCKETS_H

#include <sys/socket.h>
#include <netinet/ip.h>

#include "linebuffer.h"

class Ipv4Address
{
public:
    static Ipv4Address resolve(const std::string& name, uint16_t port);

    const sockaddr* address() const
    {
        return reinterpret_cast<const sockaddr*>(&_address);
    }

    socklen_t length() const
    {
        return sizeof(_address);
    }

private:
    sockaddr_in _address;

    Ipv4Address() { }
};

class Ipv6Address
{
public:
    static Ipv6Address resolve(const std::string& name, uint16_t port);

    const sockaddr* address() const
    {
        return reinterpret_cast<const sockaddr*>(&_address);
    }

    socklen_t length() const
    {
        return sizeof(_address);
    }

private:
    sockaddr_in6 _address;

    Ipv6Address() { }
};

class TcpStream
{
public:
    static TcpStream connect(const Ipv4Address& address);
    static TcpStream connect(const Ipv6Address& address);

    TcpStream(const TcpStream&) = delete;
    TcpStream(TcpStream&& other) :
        _fd(other._fd)
    {
        other._fd = INVALID_DESCRIPTOR;
    }

    ~TcpStream();

    std::string readLine();
private:
    static const int INVALID_DESCRIPTOR = -1;

    int _fd;
    LineBuffer _buffer;

    TcpStream(int fd) :
        _fd(fd) { }
};

class Ipv4Listener
{
public:
    Ipv4Listener(uint16_t port);
    ~Ipv4Listener();

    TcpStream* awaitConnection();
private:
    int _description;
};

class Ipv6Listener
{
public:
    Ipv6Listener(uint16_t port);

    TcpStream* awaitConnection();
private:
    int _description;
};

#endif
