#include <gtest/gtest.h>
#include "sockets.h"
#include <thread>
#include <string>

//TEST(SocketTest, ResolveLocalhostIpv4Address)
//{
//    Ipv4Address addr = Ipv4Address::resolve("127.0.0.1", 80);
//    TcpStream stream = TcpStream::connect(addr);
//}

TEST(SocketTest, ResolveGooleIpv4Address)
{
    EXPECT_NO_FATAL_FAILURE(Ipv4Address::resolve("localhost", 80));
}

TEST(SocketTest, ResolveIpv6Name)
{
    EXPECT_NO_FATAL_FAILURE(Ipv6Address::resolve("google.pl", 80));
}

std::string error;

void client_function()
{
    try
    {
        Ipv4Address addr = Ipv4Address::resolve("localhost", 21456);
        TcpStream stream = TcpStream::connect(addr);
    }
    catch(std::exception &exc)
    {
        error = exc.what();
    }
}

TEST(SocketTest, ConnectingByIpv4)
{
    Ipv4Listener listener(21456);
    std::thread client(client_function);
    TcpStream stream = listener.awaitConnection();
    client.join();
    EXPECT_EQ(error,"");
}
