#include "sockets.h"
#include "formatedexception.h"

#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>
#include <assert.h>

class SystemError : public FormatedException
{
public:
    SystemError(std::string&& errorMsg);
private:
    std::string _errorMsg;
    int _errno;
    std::string _errorStr;

    void formatWhatMsg(std::ostream& stream) const override;
};

SystemError::SystemError(std::string&& errorMsg) :
    _errorMsg(std::forward<std::string>(errorMsg)),
    _errno(errno),
    _errorStr(strerror(_errno)) { }

void SystemError::formatWhatMsg(std::ostream& stream) const
{
    stream << _errorMsg << ": " << _errorStr << " (errno " << _errno << ')';
}

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

DescriptorHolder::DescriptorHolder() :
    _fd(INVALID_DESCRIPTOR) { }

DescriptorHolder::DescriptorHolder(int fd) :
    _fd(fd) { }

DescriptorHolder::DescriptorHolder(DescriptorHolder&& other) :
    _fd(other._fd)
{
    other._fd = INVALID_DESCRIPTOR;
}

DescriptorHolder::~DescriptorHolder()
{
    if (_fd >= 0)
    {
        ::close(_fd);
    }
}

TcpStream::TcpStream(int fd) :
    DescriptorHolder(fd) { }

TcpStream::TcpStream(TcpStream&& other) :
    DescriptorHolder(std::forward<DescriptorHolder>(other)) { }

TcpStream TcpStream::connect(const Ipv4Address& address)
{
    TcpStream stream(socket(AF_INET, SOCK_STREAM, 0));
    if (stream._fd < 0)
    {
        throw SystemError("IPv4 socket error");
    }
    if (::connect(stream._fd, address.address(), address.length()) < 0)
    {
        throw SystemError("IPv4 connect error");
    }
    return stream;
}

TcpStream TcpStream::connect(const Ipv6Address& address)
{
    TcpStream stream(socket(AF_INET6, SOCK_STREAM, 0));
    if (stream._fd < 0)
    {
        throw SystemError("IPv6 socket error");
    }
    if (::connect(stream._fd, address.address(), address.length()) < 0)
    {
        throw SystemError("IPv6 connect error");
    }
    return stream;
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
        ssize_t readBytes = recv(_fd, chunk, chunkSize, 0);
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

Ipv4Listener::Ipv4Listener(uint16_t port)
{
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if(_fd < 0)
    {
        throw SystemError("IPv4 socket error");
    }
    Ipv4Address address = Ipv4Address::any(port);
    if(bind(_fd, address.address(), address.length()) < 0)
    {
        SystemError err("IPv4 bind error");
        close(_fd);
        throw err;
    }
    if(listen(_fd, 20) < 0)
    {
        SystemError err("IPv4 listen error");
        close(_fd);
        throw err;
    }
}

TcpStream Ipv4Listener::awaitConnection()
{
    int fd = accept(_fd, nullptr, nullptr);
    if (fd < 0)
    {
        throw SystemError("IPv4 accept error");
    }
    return TcpStream(fd);
}

Ipv6Listener::Ipv6Listener(uint16_t port)
{
    _fd = socket(AF_INET6, SOCK_STREAM, 0);
    if(_fd < 0)
    {
        throw SystemError("IPv6 socket error");
    }
    int on = 1;
    if (setsockopt(_fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0)
    {
        SystemError err("IPv6 setsockopt error");
        close(_fd);
        throw err;
    }
    Ipv6Address address = Ipv6Address::any(port);
    if(bind(_fd, address.address(), address.length()) < 0)
    {
        SystemError err("IPv6 bind error");
        close(_fd);
        throw err;
    }
    if(listen(_fd, 20) < 0)
    {
        SystemError err("IPv6 listen error");
        close(_fd);
        throw err;
    }
}

TcpStream Ipv6Listener::awaitConnection()
{
    int fd = accept(_fd, nullptr, nullptr);
    if (fd < 0)
    {
        throw SystemError("IPv6 accept error");
    }
    return TcpStream(fd);
}

