#ifndef SOCKETS_H
#define SOCKETS_H

#include <sys/socket.h>
#include <netinet/ip.h>

#include "linebuffer.h"

class Ipv4Address
{
public:
    static Ipv4Address resolve(const std::string& name, uint16_t port);
    static Ipv4Address any(uint16_t port);

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
    static Ipv6Address any(uint16_t port);

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

class DescriptorHolder
{
public:
    virtual ~DescriptorHolder();
protected:
    static const int INVALID_DESCRIPTOR = -1;

    int _fd;

    DescriptorHolder();
    explicit DescriptorHolder(int fd);
    DescriptorHolder(const DescriptorHolder&) = delete;
    DescriptorHolder(DescriptorHolder&&);
};

class TcpStream : public DescriptorHolder
{
public:
    static TcpStream connect(const Ipv4Address& address);
    static TcpStream connect(const Ipv6Address& address);

    TcpStream() = delete;
    TcpStream(const TcpStream&) = delete;
    TcpStream(TcpStream&& other);

    std::string readLine();
    void writeLine(std::string);
private:
    LineBuffer _buffer;

    TcpStream(int fd);

    friend class Ipv4Listener;
    friend class Ipv6Listener;
};

class Listener : public DescriptorHolder
{
public:
    virtual TcpStream awaitConnection() = 0;
};

class Ipv4Listener : public Listener
{
public:
    Ipv4Listener(uint16_t port);

    TcpStream awaitConnection() override;
};

class Ipv6Listener : public Listener
{
public:
    Ipv6Listener(uint16_t port);

    TcpStream awaitConnection() override;
};

#endif
