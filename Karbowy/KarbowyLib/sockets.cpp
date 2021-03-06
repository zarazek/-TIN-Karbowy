#include "sockets.h"
#include "systemerror.h"

#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>
#include <assert.h>


static void resolve(sa_family_t srcFamily, const std::string& name, void* address, int addressLength)
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
    address._address.sin6_addr = in6addr_any;
    address._address.sin6_port = htons(port);
    return address;
}

Descriptor::Descriptor() :
    _fd(INVALID_DESCRIPTOR) { }

Descriptor::Descriptor(int fd) :
    _fd(fd) { }

Descriptor::Descriptor(Descriptor&& other) :
    _fd(other._fd)
{
    other._fd = INVALID_DESCRIPTOR;
}

Descriptor& Descriptor::operator=(Descriptor&& other)
{
    std::swap(_fd, other._fd);
    return *this;
}

Descriptor::~Descriptor()
{
    close();
}

Descriptor::operator int() const
{
    return _fd;
}

void Descriptor::close()
{
    if (_fd >= 0)
    {
        ::close(_fd);
    }
}

TcpStream::TcpStream(Descriptor&& fd) :
    _fd(std::forward<Descriptor>(fd)) { }


TcpStream TcpStream::connect(const Ipv4Address& address)
{
    Descriptor fd(socket(AF_INET, SOCK_STREAM, 0));
    if (fd < 0)
    {
        throw SystemError("IPv4 socket error");
    }
    if (::connect(fd, address.address(), address.length()) < 0)
    {
        throw SystemError("IPv4 connect error");
    }
    return TcpStream(std::move(fd));
}

TcpStream TcpStream::connect(const Ipv6Address& address)
{
    Descriptor fd(socket(AF_INET6, SOCK_STREAM, 0));
    if (fd < 0)
    {
        throw SystemError("IPv6 socket error");
    }
    if (::connect(fd, address.address(), address.length()) < 0)
    {
        throw SystemError("IPv6 connect error");
    }
    return TcpStream(std::move(fd));
}

std::string TcpStream::readLine()
{

    while (! _buffer.hasFullLine())
    {
        if (_buffer.isEof())
        {
            throw std::runtime_error("EOF");
        }

        static const size_t chunkSize = 1024;

        char chunk[chunkSize];
        ssize_t readBytes = read(_fd, chunk, chunkSize);
        if (readBytes < 0)
        {
            throw SystemError("read error");
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

void TcpStream::writeLine(const std::string line)
{
    ssize_t writeBytes = 0;
    unsigned length = line.length();
    const char* charLine = line.c_str();
    while (writeBytes != line.length())
    {
        ssize_t written = write(_fd, charLine + writeBytes, length - writeBytes);
        if (written < 0)
        {
            throw SystemError("write error");
        }
        else
        {
            writeBytes += written;
        }
    }
}

Ipv4Listener::Ipv4Listener(uint16_t port) :
    _fd(socket(AF_INET, SOCK_STREAM, 0))
{
    if(_fd < 0)
    {
        throw SystemError("IPv4 socket error");
    }
    Ipv4Address address = Ipv4Address::any(port);
    if(bind(_fd, address.address(), address.length()) < 0)
    {
        throw SystemError("IPv4 bind error");
    }
    if(listen(_fd, 20) < 0)
    {
        throw SystemError("IPv4 listen error");
    }
}

TcpStream Ipv4Listener::awaitConnection()
{
    Descriptor fd(accept(_fd, nullptr, nullptr));
    if (fd < 0)
    {
        throw SystemError("IPv4 accept error");
    }
    return TcpStream(std::move(fd));
}

Ipv6Listener::Ipv6Listener(uint16_t port) :
    _fd(socket(AF_INET6, SOCK_STREAM, 0))
{
    if(_fd < 0)
    {
        throw SystemError("IPv6 socket error");
    }
    int on = 1;
    if (setsockopt(_fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0)
    {
        throw SystemError("IPv6 setsockopt error");
    }
    Ipv6Address address = Ipv6Address::any(port);
    if(bind(_fd, address.address(), address.length()) < 0)
    {
        throw SystemError("IPv6 bind error");
    }
    if(listen(_fd, 20) < 0)
    {
        throw SystemError("IPv6 listen error");
    }
}

TcpStream Ipv6Listener::awaitConnection()
{
    Descriptor fd(accept(_fd, nullptr, nullptr));
    if (fd < 0)
    {
        throw SystemError("IPv6 accept error");
    }
    return TcpStream(std::move(fd));
}

