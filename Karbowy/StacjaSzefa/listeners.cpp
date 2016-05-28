#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>


//ClientConnection::ClientConnection(TcpStream&& stream) :
//    _stream(std::move(stream)),
//    _state(State_UNAUTHORIZED) { }


//void ClientConnection::start()
//{
//    _thread = std::thread(&ClientConnection::run, this);
//}



//static boost::optional<std::string> parseServerChallenge(const std::string& str)
//{
//    static const char *serverChallengeCmd = "SERVER CHALLENGE ";
//    if (boost::istarts_with(str, serverChallengeCmd))
//    {
//        return boost::optional<std::string>(str.substr(strlen(serverChallengeCmd)));
//    }
//    else
//    {
//        return boost::none;
//    }
//}

//static std::string SHA(std::string message)
//{
//    CryptoPP::SHA256 hash;
//    std::string digest;
//    CryptoPP::StringSource s(message, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
//    return digest;
//}

//void ClientConnection::handleConnection()
//{
//    std::string line = _stream.readLine();
//    boost::optional<std::string> serverChallenge = parseServerChallenge(line);
//    if (! serverChallenge)
//    {
//        std::cerr << "Invalid server challenge: '" << line << "'" << std::endl;
//        return;
//    }
//    _stream << "SERVER RESPONSE " << SHA()
//}

//void ClientConnection::run()
//{
//    try
//    {
//        handleConnection();
//    }
//    catch (std::exception& ex)
//    {
//        std::cerr << "Client exception: " << ex.what() << std::endl;
//    }
//    onShutdown();
//}
