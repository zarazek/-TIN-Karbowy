#include "sockets.h"

#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>
#include <assert.h>

static void resolve(sa_family_t srcFamily, const std::string& name, void* address, size_t addressLength)
{
    hostent hostentBuffer;
    char stringBuffer[512];
    hostent* result = nullptr;
    int errorCode = 0;
    if (gethostbyname2_r(name.c_str(),
                         srcFamily,
                         &hostentBuffer,
                         stringBuffer,
                         sizeof(stringBuffer),
                         &result,
                         &errorCode) != 0)
    {
        throw std::runtime_error(hstrerror(errorCode));
    }
    assert(result);
    assert(result->h_addrtype == srcFamily);
    assert(result->h_length == addressLength);
    assert(result->h_addr_list);
    assert(result->h_addr_list[0]);

    memcpy(address, result->h_addr_list[0], result->h_length);
}

Ipv4Address Ipv4Address::resolve(const std::string& name, uint16_t port)
{
    Ipv4Address address;
    memset(&address._address, 0, sizeof(address._address));
    address._address.sin_family = AF_INET;
    ::resolve(AF_INET, name, &address._address.sin_addr, sizeof(address._address.sin_addr));
    address._address.sin_port = htons(port);
    return address;
}

Ipv4Address Ipv4Address::any(uint16_t port)
{
    Ipv4Address address;
    memset(&address._address, 0, sizeof(address._address));
    address._address.sin_family = AF_INET;
    address._address.sin_port = htons(port);
    return address;
}

Ipv6Address Ipv6Address::resolve(const std::string& name, uint16_t port)
{
    Ipv6Address address;
    memset(&address._address, 0, sizeof(address._address));
    address._address.sin6_family = AF_INET6;
    ::resolve(AF_INET6, name, &address._address.sin6_addr, sizeof(address._address.sin6_addr));
    address._address.sin6_port = htons(port);
    return address;
}

Ipv6Address Ipv6Address::any(uint16_t port)
{
    Ipv6Address address;
    memset(&address._address, 0, sizeof(address._address));
    address._address.sin6_family = AF_INET6;
    address._address.sin6_port = htons(port);
    return address;
}

TcpStream TcpStream::connect(const Ipv4Address& address)
{
    TcpStream stream(socket(AF_INET, SOCK_STREAM, 0));
    if (stream._fd < 0)
    {
        throw std::runtime_error("ipv4 socket error");
    }
    if (::connect(stream._fd, address.address(), address.length()) < 0)
    {
        throw std::runtime_error("ipv4 connect error");
    }
    return stream;
}

TcpStream TcpStream::connect(const Ipv6Address& address)
{
    TcpStream stream(socket(AF_INET6, SOCK_STREAM, 0));
    if (stream._fd < 0)
    {
        throw std::runtime_error("ipv6 socket error");
    }
    if (::connect(stream._fd, address.address(), address.length()) < 0)
    {
        throw std::runtime_error("ipv6 connect error");
    }
    return stream;
}

TcpStream::~TcpStream()
{
    if (_fd >= 0)
    {
        close(_fd);
    }
}


std::string TcpStream::readLine()
{
    while (! _buffer.hasFullLine())
    {
        static const size_t chunkSize = 1024;

        char chunk[chunkSize];
        ssize_t readBytes = read(_fd, chunk, chunkSize);
        if (readBytes < 0)
        {
            throw std::runtime_error("read error");
        }
        else if (readBytes == 0)
        {
            _buffer.setEof();
        }
        else
        {
            _buffer.addData(chunk, readBytes);
        }
    }

    return _buffer.getFirstLine();
}

void TcpStream::writeLine(std::string)
{
    //TO DO

}

Ipv4Listener::Ipv4Listener(uint16_t port)
{
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if(_fd < 0)
    {
        throw std::runtime_error("ipv4 socket error");
    }
    Ipv4Address address = Ipv4Address::any(port);
    if(bind(_fd, address.address(), address.length()) < 0)
    {
        close(_fd);
        throw std::runtime_error("bind ipv4 error");
    }
    if(listen(_fd, 20) < 0)
    {
        close(_fd);
        throw std::runtime_error("listen ipv4 error");
    }
}

Ipv4Listener::~Ipv4Listener()
{
    if (_fd >=0)
    {
        close(_fd);
    }
}

TcpStream Ipv4Listener::awaitConnection()
{
    int fd = accept(_fd, nullptr, nullptr);
    if (fd < 0)
    {
        throw std::runtime_error("accept ipv4 error");
    }
    return TcpStream(fd);
}

Ipv6Listener::Ipv6Listener(uint16_t port)
{
    _fd = socket(AF_INET6, SOCK_STREAM, 0);
    if(_fd < 0)
    {
        throw std::runtime_error("ipv6 socket error");
    }
    Ipv6Address address = Ipv6Address::any(port);
    if(bind(_fd, address.address(), address.length()) < 0)
    {
        close(_fd);
        throw std::runtime_error("bind ipv6 error");
    }
    if(listen(_fd, 20) < 0)
    {
        close(_fd);
        throw std::runtime_error("listen ipv6 error");
    }
}

Ipv6Listener::~Ipv6Listener()
{
    if (_fd >=0)
    {
        close(_fd);
    }
}

TcpStream Ipv6Listener::awaitConnection()
{
    int fd = accept(_fd, nullptr, nullptr);
    if (fd < 0)
    {
        throw std::runtime_error("accept ipv6 error");
    }
    return TcpStream(fd);
}

