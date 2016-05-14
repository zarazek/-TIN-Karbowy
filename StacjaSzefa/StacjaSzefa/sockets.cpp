#include "sockets.h"

#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>
#include <assert.h>

static void resolve(sa_family_t srcFamily, const std::string& name, sa_family_t* dstFamily, void* address)
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
        throw std::runtime_error("resolve error");
    }
    assert(result);
    assert(result->h_length > 0);
    assert(result->h_addr_list[0]);

    *dstFamily = result->h_addrtype;
    memcpy(address, result->h_addr_list[0], result->h_length);
}

Ipv4Address Ipv4Address::resolve(const std::string& name, uint16_t port)
{
    Ipv4Address address;
    memset(&address._address, 0, sizeof(address._address));
    ::resolve(AF_INET, name, &address._address.sin_family, &address._address.sin_addr);
    address._address.sin_port = htons(port);
    return address;
}

Ipv6Address Ipv6Address::resolve(const std::string& name, uint16_t port)
{
    Ipv6Address address;
    memset(&address._address, 0, sizeof(address._address));
    ::resolve(AF_INET6, name, &address._address.sin6_family, &address._address.sin6_addr);
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




