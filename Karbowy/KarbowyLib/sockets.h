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

class Descriptor
{
public:
    Descriptor();
    explicit Descriptor(int fd);
    Descriptor(const Descriptor& other) = delete;
    Descriptor(Descriptor&& other);
    Descriptor& operator=(const Descriptor& other) = delete;
    Descriptor& operator=(Descriptor&& other);
    ~Descriptor();

    operator int() const;

protected:
    static const int INVALID_DESCRIPTOR = -1;

    int _fd;

    void close();
};

class TcpStream : public Descriptor
{
public:
    static TcpStream connect(const Ipv4Address& address);
    static TcpStream connect(const Ipv6Address& address);

    TcpStream() = delete;
    TcpStream(const TcpStream&) = delete;
    TcpStream(TcpStream&& other);
    TcpStream& operator=(const TcpStream& other) = delete;
    TcpStream& operator=(TcpStream&& other);

    std::string readLine();
    void writeLine(std::string);
private:
    Descriptor _fd;
    LineBuffer _buffer;

    TcpStream(Descriptor&& fd);

    friend class Ipv4Listener;
    friend class Ipv6Listener;
};

class Listener
{
public:
    virtual ~Listener() { }
    virtual TcpStream awaitConnection() = 0;
};

class Ipv4Listener : public Listener
{
public:
    Ipv4Listener(uint16_t port);

    TcpStream awaitConnection() override;
private:
    Descriptor _fd;
};

class Ipv6Listener : public Listener
{
public:
    Ipv6Listener(uint16_t port);

    TcpStream awaitConnection() override;
private:
    Descriptor _fd;
};

#endif
