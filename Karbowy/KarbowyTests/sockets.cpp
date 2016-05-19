#include <gtest/gtest.h>
#include "sockets.h"

TEST(SocketTest, ResolveLocalhostIpv4Address)
{
    Ipv4Address addr = Ipv4Address::resolve("127.0.0.1", 80);
    TcpStream stream = TcpStream::connect(addr);
}

TEST(SocketTest, ResolveGooleIpv4Address)
{
    EXPECT_NO_FATAL_FAILURE(Ipv4Address::resolve("google.pl", 80));
}

TEST(SocketTest, ResolveIpv6Name)
{
    EXPECT_NO_FATAL_FAILURE(Ipv6Address::resolve("google.pl", 80));
}
