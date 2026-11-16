#include <Mantids30/Network/Sockets/socket_tcp.h>
#include <Mantids30/Network/Sockets/acceptor_multithreaded.h>
#include <Mantids30/Network/Sockets/acceptor_poolthreaded.h>
#include <Mantids30/Network/Sockets/socket_tls.h>
#include <Mantids30/Network/Sockets/socket_udp.h>
#include <Mantids30/Network/Sockets/socket_unix.h>
#include <Mantids30/Network/Sockets/streams_bufferedreader.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <csignal>
#include <pthread.h>
#include <sys/resource.h>
#include <filesystem>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <sys/un.h>
#include <unistd.h>

using namespace Mantids30::Network::Sockets;
using namespace std::chrono_literals;

// Interpose only a reserved test name. Real resolver allocations are retained so
// freeaddrinfo() and the production connection loop exercise their normal paths.
extern "C" int getaddrinfo(const char *node, const char *service, const addrinfo *hints, addrinfo **result)
{
    using Resolver = int (*)(const char *, const char *, const addrinfo *, addrinfo **);
    static const auto resolve = reinterpret_cast<Resolver>(dlsym(RTLD_NEXT, "getaddrinfo"));
    if (node && std::strcmp(node, "mantids-fallback.invalid") == 0)
    {
        addrinfo firstHints = *hints;
        firstHints.ai_family = AF_INET6;
        firstHints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
        addrinfo *first = nullptr;
        int error = resolve("::1", service, &firstHints, &first);
        if (error != 0)
        {
            return error;
        }
        addrinfo secondHints = firstHints;
        secondHints.ai_family = AF_INET;
        error = resolve("127.0.0.1", service, &secondHints, result);
        if (error != 0)
        {
            freeaddrinfo(first);
            return error;
        }
        first->ai_next = *result;
        *result = first;
        return 0;
    }
    return resolve(node, service, hints, result);
}

static void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

static bool ipv6Available()
{
    int descriptor = socket(AF_INET6, SOCK_STREAM, 0);
    if (descriptor < 0)
    {
        const int error = errno;
        std::cout << "IPv6 probe: " << error << " " << std::strerror(error) << '\n';
        require(error == EAFNOSUPPORT || error == EPROTONOSUPPORT || error == EOPNOTSUPP || error == EPERM || error == EACCES, "IPv6 socket creation failed unexpectedly");
        return false;
    }
    sockaddr_in6 address{};
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_loopback;
    bool available = bind(descriptor, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0;
    const int error = errno;
    close(descriptor);
    require(available || error == EADDRNOTAVAIL || error == EAFNOSUPPORT, "IPv6 bind failed unexpectedly");
    return available;
}

static int option(Socket &socket, int level, int name)
{
    int value = -1;
    socklen_t length = sizeof(value);
    require(socket.getSocketOption(level, name, &value, &length) == 0, "getsockopt failed");
    return value;
}

static void timeouts(Socket &socket)
{
    require(socket.setReadTimeout(2), "read timeout setup failed");
    require(socket.setWriteTimeout(2), "write timeout setup failed");
}

static void lifecycle()
{
    Socket_TCP listener;
    require(!listener.isActive(), "new socket is active");
    require(listener.listenOn(0, "127.0.0.1"), "listen failed");
    Socket_TCP collision;
    require(!collision.listenOn(listener.getLocalPort(), "127.0.0.1"), "duplicate bind succeeded");
    require(!collision.isActive(), "failed bind retained a descriptor");
    int unrelated = open("/dev/null", O_RDONLY);
    require(unrelated >= 0, "open failed");
    collision.closeSocket();
    require(fcntl(unrelated, F_GETFD) >= 0, "failed socket closed an unrelated descriptor");
    close(unrelated);
    auto pair = Socket_Stream::GetSocketPair();
    require(pair.first && pair.second, "socketpair failed");
    require(pair.first->setReadTimeout(7) && pair.first->setWriteTimeout(9), "descriptor timeout fixture failed");
    int descriptor = pair.first->adquireSocketFD();
    require(!pair.first->isActive(), "descriptor transfer did not relinquish ownership");
    Socket_Stream moved;
    moved.setSocketFD(descriptor);
    timeval adoptedTimeout{};
    socklen_t timeoutLength = sizeof(adoptedTimeout);
    require(moved.getSocketOption(SOL_SOCKET, SO_RCVTIMEO, &adoptedTimeout, &timeoutLength) == 0 && adoptedTimeout.tv_sec == 7, "adopting descriptor overwrote read timeout");
    timeoutLength = sizeof(adoptedTimeout);
    require(moved.getSocketOption(SOL_SOCKET, SO_SNDTIMEO, &adoptedTimeout, &timeoutLength) == 0 && adoptedTimeout.tv_sec == 9, "adopting descriptor overwrote write timeout");
    require(moved.writeFull("x"), "transferred descriptor cannot write");
    char byte;
    require(pair.second->readFull(&byte, 1) && byte == 'x', "transferred descriptor data mismatch");
    require(moved.closeSocket() == 0 && moved.closeSocket() == 0, "close is not idempotent");
}

static void tcp(bool ipv6)
{
    const char *host = ipv6 ? "::1" : "127.0.0.1";
    Socket_TCP listener;
    listener.setUseIPv6(ipv6);
    require(listener.listenOn(0, host), "listen failed");
    timeouts(listener);
    const auto local = listener.getLocalAddressAndPort();
    require(local.port != 0 && local.address == host && listener.getLocalPort() == local.port, "listener metadata mismatch");
    Socket_TCP client;
    client.setUseIPv6(ipv6);
    require(client.connectFrom(host, host, local.port, 2), "connectFrom failed");
    auto peer = listener.acceptConnection();
    require(peer != nullptr && peer->isConnected(), "accept failed");
    require(client.getRemotePairStr() == host && peer->getRemotePairStr() == host, "remote IP mismatch");
    require(peer->getUseIPv6() == ipv6, "accepted socket family mismatch");
    require(peer->getRemotePort() == client.getLocalPort(), "remote port mismatch");
    require(peer->getLocalAddressAndPort().address == host, "accepted local IP mismatch");
    timeval timeout{};
    socklen_t length = sizeof(timeout);
    require(peer->getSocketOption(SOL_SOCKET, SO_RCVTIMEO, &timeout, &length) == 0 && timeout.tv_sec == 2, "accepted timeout not inherited");
    require(client.writeFull("hello"), "write failed");
    char data[5];
    require(peer->readFull(data, sizeof(data)) && std::memcmp(data, "hello", 5) == 0, "payload mismatch");
    require(client.shutdownSocket(SHUT_WR) == 0, "half close failed");
    require(peer->partialRead(data, 1) == 0, "half close did not deliver EOF");
    require(peer->writeFull("r") && client.readFull(data, 1) && data[0] == 'r', "half close prevented response");
}

static void tcpOptions()
{
    Socket_TCP listener;
    listener.setTcpUseKeepAlive(true);
    listener.setTcpKeepIdle(123);
    listener.setTcpKeepCnt(7);
    listener.setTcpKeepIntvl(11);
    listener.setTcpNoDelayOption(false);
    require(listener.listenOn(0, "127.0.0.1"), "listen failed");
    require(option(listener, IPPROTO_TCP, TCP_KEEPIDLE) == 123, "listener idle value truncated");
    require(option(listener, IPPROTO_TCP, TCP_KEEPCNT) == 7, "listener probe count truncated");
    require(option(listener, IPPROTO_TCP, TCP_KEEPINTVL) == 11, "listener probe interval truncated");
    Socket_TCP client;
    client.setTcpUseKeepAlive(true);
    client.setTcpKeepIdle(99);
    client.setTcpKeepCnt(6);
    client.setTcpKeepIntvl(13);
    require(client.connectTo("127.0.0.1", listener.getLocalPort(), 2), "connect failed");
    auto peer = listener.acceptConnection();
    require(peer != nullptr, "accept failed");
    require(option(*peer, SOL_SOCKET, SO_KEEPALIVE) == 1, "accepted keepalive disabled");
    require(option(*peer, IPPROTO_TCP, TCP_NODELAY) == 0, "accepted nodelay not inherited");
    require(option(client, IPPROTO_TCP, TCP_NODELAY) == 1, "client nodelay disabled");
    timeval timeout{};
    socklen_t timeoutLength = sizeof(timeout);
    require(client.getSocketOption(SOL_SOCKET, SO_RCVTIMEO, &timeout, &timeoutLength) == 0 && timeout.tv_sec == 0 && timeout.tv_usec == 0, "default TCP client read timeout changed");
    require(option(client, IPPROTO_TCP, TCP_KEEPIDLE) == 99, "client idle value truncated");
    require(option(client, IPPROTO_TCP, TCP_KEEPCNT) == 6, "client count truncated");
    require(option(client, IPPROTO_TCP, TCP_KEEPINTVL) == 13, "client interval truncated");
}

static void tcpFailures()
{
    Socket_TCP client;
    require(!client.connectTo(nullptr, 1, 1), "null host accepted");
    require(!client.connectTo("", 1, 1), "empty host accepted");
    require(!client.listenOn(0, "invalid address"), "invalid bind accepted");
    require(!client.isActive(), "invalid bind leaked descriptor");
    Socket_TCP reserved;
    require(reserved.listenOn(0, "127.0.0.1"), "listen failed");
    const auto port = reserved.getLocalPort();
    reserved.closeSocket();
    require(!client.connectTo("127.0.0.1", port, 1), "refused connection succeeded");
    require(!client.isActive() && !client.getLastError().empty(), "failed connection retained descriptor or lost error");
    const auto diagnostic = client.getLastError();
    require(diagnostic.find("127.0.0.1:" + std::to_string(port)) != std::string::npos, "connection error lost destination context");
    require(diagnostic.find("error #" + std::to_string(ECONNREFUSED)) != std::string::npos, "connection error lost numeric error code");
    require(diagnostic.find("connect()") != std::string::npos, "connection error lost operation context");
}

static void tcpReuse()
{
    Socket_TCP listener;
    require(listener.listenOn(0, "127.0.0.1"), "listen failed");
    Socket_TCP client;
    for (int i = 0; i < 8; ++i)
    {
        require(client.connectTo("127.0.0.1", listener.getLocalPort(), 2), "reconnect failed");
        auto peer = listener.acceptConnection();
        require(peer != nullptr, "accept failed");
        timeouts(*peer);
        require(client.writeFull("x"), "reused socket write failed");
        char byte;
        require(peer->readFull(&byte, 1), "reused socket read failed");
        require(client.shutdownSocket() == 0, "shutdown state not reset on reconnect");
        client.closeSocket();
    }
    // Re-listening on the same object must release its previous descriptor.
    const int countBefore = static_cast<int>(std::distance(std::filesystem::directory_iterator("/proc/self/fd"), std::filesystem::directory_iterator{}));
    for (int i = 0; i < 12; ++i)
    {
        require(listener.listenOn(0, "127.0.0.1"), "relisten failed");
    }
    const int countAfter = static_cast<int>(std::distance(std::filesystem::directory_iterator("/proc/self/fd"), std::filesystem::directory_iterator{}));
    require(countAfter == countBefore, "relisten leaked descriptors");
}

static void dnsFallback()
{
    Socket_TCP listener;
    require(listener.listenOn(0, "127.0.0.1"), "listen failed");
    Socket_TCP client;
    client.setUseIPv6(true);
    require(client.connectTo("mantids-fallback.invalid", listener.getLocalPort(), 2), "IPv6 to IPv4 DNS fallback failed");
    require(!client.getUseIPv6() && client.getRemotePairStr() == "127.0.0.1", "fallback metadata has wrong family");
}

static void streamTransfer()
{
    auto pair = Socket_Stream::GetSocketPair();
    require(pair.first && pair.second, "socketpair failed");
    timeouts(*pair.first);
    timeouts(*pair.second);
    pair.first->setChunkSize(13);
    pair.second->setChunkSize(17);
    std::vector<char> data(512 * 1024);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<char>(i % 251);
    }
    auto writer = std::async(std::launch::async, [&] { return pair.first->writeFull(data.data(), data.size()); });
    std::vector<char> received(data.size());
    size_t count = 0;
    const bool read = pair.second->readFull(received.data(), received.size(), &count);
    require(writer.get() && read && count == data.size() && received == data, "fragmented transfer corrupted data");
    require(pair.first->writeFull(nullptr, 0), "empty write failed");
    char emptyRead = 0;
    require(pair.second->readFull(&emptyRead, 0, &count) && count == 0, "empty read failed");
    require(!pair.second->readFull(nullptr, 0, &count), "empty read bypassed null-buffer validation");
    pair.first->setChunkSize(0);
    require(pair.first->getChunkSize() > 0, "zero chunk size retained");
    require(!pair.first->writeFull(nullptr, 1) && !pair.second->readFull(nullptr, 1), "null buffer accepted");
}

static void streamErrors()
{
    auto pair = Socket_Stream::GetSocketPair();
    timeouts(*pair.first);
    require(pair.second->setReadTimeout(1), "read timeout setup failed");
    require(pair.first->writeFull("abc"), "write failed");
    std::array<char, 5> data{};
    size_t count = 999;
    const auto begin = std::chrono::steady_clock::now();
    require(!pair.second->readFull(data.data(), data.size(), &count), "incomplete read succeeded");
    require(count == 3, "timeout discarded partial byte count");
    require(std::chrono::steady_clock::now() - begin < 3s, "read timeout ignored");
    pair.first->shutdownSocket(SHUT_WR);
    require(pair.second->partialRead(data.data(), data.size()) == 0, "EOF not delivered");
    pair.second->closeSocket();
    require(pair.first->partialWrite("x", 1) < 0, "closed peer write succeeded");
    require(!pair.second->readFull(data.data(), 1), "closed socket read succeeded");
    auto blocked = Socket_Stream::GetSocketPair();
    require(blocked.first->setWriteTimeout(1), "write timeout setup failed");
    int smallBuffer = 4096;
    require(blocked.first->setSocketOption(SOL_SOCKET, SO_SNDBUF, &smallBuffer, sizeof(smallBuffer)) == 0, "send buffer setup failed");
    std::vector<char> large(1024 * 1024);
    const auto writeBegin = std::chrono::steady_clock::now();
    require(!blocked.first->writeFull(large.data(), large.size()), "write to stalled peer did not time out");
    require(std::chrono::steady_clock::now() - writeBegin < 4s, "write timeout ignored");
}

static void interruptedIO()
{
    struct sigaction action{};
    action.sa_handler = [](int) {};
    sigemptyset(&action.sa_mask);
    struct sigaction previous{};
    require(sigaction(SIGUSR1, &action, &previous) == 0, "signal setup failed");
    auto pair = Socket_Stream::GetSocketPair();
    timeouts(*pair.second);
    const auto readingThread = pthread_self();
    auto sender = std::async(std::launch::async, [&]
    {
        std::this_thread::sleep_for(30ms);
        pthread_kill(readingThread, SIGUSR1);
        std::this_thread::sleep_for(30ms);
        return pair.first->writeFull("x");
    });
    char byte;
    const bool read = pair.second->readFull(&byte, 1);
    require(sender.get() && read && byte == 'x', "signal interrupted a full read");
    require(sigaction(SIGUSR1, &previous, nullptr) == 0, "signal restore failed");
}

static void shutdownWakeup()
{
    Socket_TCP listener;
    require(listener.listenOn(0, "127.0.0.1"), "listen failed");
    std::promise<void> started;
    auto ready = started.get_future();
    auto acceptor = std::async(std::launch::async, [&]
    {
        started.set_value();
        return listener.acceptConnection();
    });
    ready.wait();
    listener.shutdownSocket();
    require(acceptor.wait_for(1s) == std::future_status::ready && !acceptor.get(), "shutdown did not wake accept");
    auto pair = Socket_Stream::GetSocketPair();
    auto reader = std::async(std::launch::async, [&]
    {
        char byte;
        return pair.first->partialRead(&byte, 1);
    });
    pair.first->shutdownSocket();
    require(reader.wait_for(1s) == std::future_status::ready && reader.get() == 0, "shutdown did not wake read");
}

static void highDescriptors()
{
    rlimit limit{};
    require(getrlimit(RLIMIT_NOFILE, &limit) == 0, "descriptor limit query failed");
    if (limit.rlim_cur <= FD_SETSIZE + 32)
    {
        std::cout << "SKIP: descriptor limit is too low\n";
        std::exit(77);
    }
    Socket_TCP listener;
    require(listener.listenOn(0, "127.0.0.1"), "listen failed");
    std::vector<int> descriptors;
    int descriptor = -1;
    do
    {
        descriptor = open("/dev/null", O_RDONLY);
        require(descriptor >= 0, "descriptor allocation failed");
        descriptors.push_back(descriptor);
    } while (descriptor <= FD_SETSIZE + 8);
    Socket_TCP client;
    const bool connected = client.connectTo("127.0.0.1", listener.getLocalPort(), 2);
    for (int held : descriptors)
    {
        close(held);
    }
    require(connected, "connect failed above FD_SETSIZE");
    auto peer = listener.acceptConnection();
    require(peer != nullptr, "high-descriptor accept failed");
}

static void streamBlocks()
{
    auto pair = Socket_Stream::GetSocketPair();
    timeouts(*pair.second);
    const std::string binary("a\0bc", 4);
    require(pair.first->writeStringEx<uint16_t>(binary), "binary string write failed");
    bool ok = false;
    require(pair.second->readStringEx<uint16_t>(&ok, 16) == binary && ok, "binary string round trip failed");
    require(pair.first->writeStringEx<uint8_t>(""), "empty string write failed");
    require(pair.second->readStringEx<uint8_t>(&ok, 1).empty() && ok, "empty string rejected");
    require(pair.first->writeStringEx<uint16_t>("oversized"), "oversize fixture write failed");
    pair.second->readStringEx<uint16_t>(&ok, 2);
    require(!ok, "oversized string accepted");
    auto truncated = Socket_Stream::GetSocketPair();
    require(truncated.first->writeU<uint16_t>(5) && truncated.first->writeFull("ab"), "truncated fixture failed");
    truncated.first->shutdownSocket(SHUT_WR);
    truncated.second->readStringEx<uint16_t>(&ok, 10);
    require(!ok, "truncated block accepted");
    auto overflow = Socket_Stream::GetSocketPair();
    require(overflow.first->writeU<uint64_t>(std::numeric_limits<uint64_t>::max()), "overflow fixture failed");
    uint64_t maximum = std::numeric_limits<uint64_t>::max();
    std::unique_ptr<char[]> rejected(overflow.second->readBlockWAllocEx(&maximum));
    require(!rejected && maximum == 0, "terminator allocation overflow accepted");
}

static void streamIntegers()
{
    auto pair = Socket_Stream::GetSocketPair();
    timeouts(*pair.second);
    require(pair.first->writeU<uint8_t>(0xAB) && pair.first->writeU<uint16_t>(0x1234) && pair.first->writeU<uint32_t>(0x12345678) && pair.first->writeU<uint64_t>(0x123456789ABCDEF0ULL), "integer write failed");
    bool ok = false;
    require(pair.second->readU<uint8_t>(&ok) == 0xAB && ok, "u8 mismatch");
    require(pair.second->readU<uint16_t>(&ok) == 0x1234 && ok, "u16 mismatch");
    require(pair.second->readU<uint32_t>(&ok) == 0x12345678 && ok, "u32 mismatch");
    require(pair.second->readU<uint64_t>(&ok) == 0x123456789ABCDEF0ULL && ok, "u64 mismatch");
    pair.first->shutdownSocket(SHUT_WR);
    pair.second->readU<uint64_t>(&ok);
    require(!ok, "truncated integer accepted");
}

static void streamDelimiters()
{
    for (unsigned int boundary : {65534u, 65535u, 65536u, 65537u})
    {
        auto pair = Socket_Stream::GetSocketPair();
        timeouts(*pair.first);
        timeouts(*pair.second);
        std::string message(boundary, 'x');
        message += "\r\nEND";
        auto writer = std::async(std::launch::async, [&] { return pair.first->writeFull(message.data(), message.size()); });
        unsigned int length = 131072;
        std::unique_ptr<char[]> result(pair.second->readBlock32WAllocAndDelim(&length, "\r\nEND", 5));
        require(writer.get() && result && length == message.size() && std::memcmp(result.get(), message.data(), length) == 0, "delimiter crossing 64K boundary failed");
    }
    auto pair = Socket_Stream::GetSocketPair();
    unsigned int length = 65536;
    require(pair.second->readBlock32WAllocAndDelim(&length, "", 0) == nullptr, "empty delimiter accepted");
    require(pair.second->readBlock32WAllocAndDelim(nullptr, "x", 1) == nullptr, "null length accepted");
    require(pair.second->readBlock32WAllocAndDelim(&length, nullptr, 1) == nullptr, "null delimiter accepted");
}

static void bufferedReader()
{
    auto pair = Socket_Stream::GetSocketPair();
    timeouts(*pair.second);
    NetStreams::BufferedReader reader(pair.second, 16);
    require(pair.first->writeFull("hello\nworld\n"), "write failed");
    std::array<char, 3> small{'a', 'b', 'c'};
    size_t length = 2;
    require(reader.bufferedReadUntil(small.data(), &length, '\n') == NetStreams::BufferedReader::ReadResult::READ_FULL, "small destination accepted");
    require(length == 6 && small[0] == 'a' && small[2] == 'c', "small destination overwritten");
    std::string line;
    require(reader.readLineLF(line) == NetStreams::BufferedReader::ReadResult::READ_OK && line == "hello", "failed read consumed data");
    require(reader.readLineLF(line) == NetStreams::BufferedReader::ReadResult::READ_OK && line == "world", "buffer displacement failed");
    require(pair.first->writeFull("0123456789abcdef"), "write failed");
    require(reader.readLineLF(line) == NetStreams::BufferedReader::ReadResult::READ_FULL, "delimiter-free full buffer not detected");
}

static void udp(bool ipv6)
{
    const char *host = ipv6 ? "::1" : "127.0.0.1";
    Socket_UDP listener;
    listener.setUseIPv6(ipv6);
    require(listener.listenOn(0, host), "UDP listen failed");
    timeouts(listener);
    Socket_UDP sender;
    sender.setUseIPv6(ipv6);
    require(sender.connectFrom(host, host, listener.getLocalPort(), 2), "UDP setup failed");
    for (size_t length : {size_t(0), size_t(1), size_t(4096), size_t(65000)})
    {
        std::vector<unsigned char> payload(length, 0xA5);
        require(sender.writeBlock(payload.data(), payload.size()), "datagram write failed");
        auto block = listener.readBlock();
        require(block && block->dataLength == static_cast<int>(length), "datagram length mismatch");
        require(length == 0 || std::memcmp(block->data, payload.data(), length) == 0, "datagram data mismatch");
        require(reinterpret_cast<const sockaddr *>(&block->socketAddress)->sa_family == (ipv6 ? AF_INET6 : AF_INET), "datagram address family mismatch");
        if (ipv6)
        {
            auto *address = reinterpret_cast<const sockaddr_in6 *>(&block->socketAddress);
            require(IN6_IS_ADDR_LOOPBACK(&address->sin6_addr) && ntohs(address->sin6_port) == sender.getLocalPort(), "UDP IPv6 source truncated");
        }
    }
}

static void udpErrors()
{
    Socket_UDP socket;
    require(!socket.isConnected() && !socket.writeBlock("x", 1), "inactive UDP socket usable");
    require(socket.listenOn(0, "127.0.0.1"), "UDP listen failed");
    require(socket.setReadTimeout(1), "UDP timeout setup failed");
    const auto begin = std::chrono::steady_clock::now();
    auto block = socket.readBlock();
    require(block && block->dataLength == -1 && !block->data, "UDP timeout not reported");
    require(std::chrono::steady_clock::now() - begin < 3s, "UDP listener ignored timeout");
    require(!socket.connectTo(nullptr, 1, 1), "null UDP host accepted");
    require(!socket.isActive(), "failed UDP setup retained descriptor");
}

static void datagramOwnership()
{
    Socket_DatagramBase::Block block;
    block.copy("abc", 3);
    require(block.dataLength == 3, "copy lost length");
    auto copy = block;
    block.data[0] = 'z';
    require(copy.data != block.data && copy.data[0] == 'a', "datagram copy aliases storage");
    Socket_DatagramBase::Block assigned;
    assigned = copy;
    auto moved = std::move(copy);
    require(!copy.data && moved.dataLength == 3 && assigned.data[0] == 'a', "datagram move/copy assignment failed");
    assigned = std::move(moved);
    require(!moved.data && assigned.dataLength == 3, "move assignment failed");
    block.free();
    block.free();
    require(!block.data && block.dataLength == -1, "free did not clear block");
    block.copy("", 0);
    require(block.dataLength == 0 && !block.data, "empty datagram state incorrect");
}

static void unixSockets()
{
    const std::string path = "/tmp/mantids30-socket-" + std::to_string(getpid());
    struct Cleanup
    {
        std::string path;
        ~Cleanup() { unlink(path.c_str()); }
    } cleanup{path};
    Socket_UNIX listener;
    require(listener.listenOn(path.c_str()), "UNIX listen failed");
    timeouts(listener);
    Socket_UNIX client;
    require(client.connectTo(path.c_str(), 0, 2), "UNIX connect failed");
    auto peer = listener.acceptConnection();
    require(peer && client.writeFull("unix"), "UNIX accept/write failed");
    char data[4];
    require(peer->readFull(data, sizeof(data)) && std::memcmp(data, "unix", 4) == 0, "UNIX data mismatch");
    require(!client.connectTo((path + ".missing").c_str(), 0, 1) && !client.isActive(), "failed UNIX connect retained descriptor");
    require(!client.connectTo(std::string(300, 'x').c_str(), 0, 1), "oversize UNIX path accepted");
    require(!client.connectTo(nullptr, 0, 1), "null UNIX path accepted");
}

static void unixBacklog()
{
    const std::string path = "/tmp/mantids30-backlog-" + std::to_string(getpid());
    struct Cleanup
    {
        std::string path;
        ~Cleanup() { unlink(path.c_str()); }
    } cleanup{path};
    int file = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    require(file >= 0, "fixture file creation failed");
    close(file);
    Socket_UNIX listener;
    require(!listener.listenOn(path.c_str()), "UNIX listener replaced a regular file");
    require(std::filesystem::is_regular_file(path), "UNIX listener deleted a regular file");
    unlink(path.c_str());
    require(listener.listenOn(path.c_str(), 0, 1), "UNIX listen failed");
    Socket_UNIX first;
    Socket_UNIX second;
    require(first.connectTo(path.c_str(), 0, 1) && second.connectTo(path.c_str(), 0, 1), "UNIX backlog setup failed");
    Socket_UNIX blocked;
    const auto begin = std::chrono::steady_clock::now();
    require(!blocked.connectTo(path.c_str(), 0, 1), "full UNIX backlog accepted connection");
    require(!blocked.isActive() && std::chrono::steady_clock::now() - begin < 3s, "UNIX connect timeout ignored");
}

static void acceptors(bool pooled)
{
    auto listener = std::make_shared<Socket_TCP>();
    require(listener->listenOn(0, "127.0.0.1"), "listen failed");
    timeouts(*listener);
    auto onConnect = [](void *, const std::shared_ptr<Socket_Stream> &peer)
    {
        char byte;
        if (peer->readFull(&byte, 1))
        {
            peer->writeFull(&byte, 1);
        }
    };
    auto exercise = [&]
    {
        std::vector<std::future<bool>> clients;
        for (int i = 0; i < 12; ++i)
        {
            clients.emplace_back(std::async(std::launch::async, [&]
            {
                Socket_TCP client;
                client.overrideReadTimeout(2);
                client.overrideWriteTimeout(2);
                char byte;
                return client.connectTo("127.0.0.1", listener->getLocalPort(), 2) &&
                       client.writeFull("x") && client.readFull(&byte, 1) && byte == 'x';
            }));
        }
        bool success = true;
        for (auto &client : clients)
        {
            success = client.get() && success;
        }
        return success;
    };
    if (pooled)
    {
        auto acceptor = std::make_shared<Acceptors::PoolThreaded>();
        acceptor->parameters.threadsCount = 3;
        acceptor->parameters.taskQueues = 2;
        acceptor->callbacks.onClientConnected = onConnect;
        acceptor->addAcceptorSocket(listener);
        acceptor->startInBackground();
        const bool success = exercise();
        acceptor->stop();
        const auto deadline = std::chrono::steady_clock::now() + 3s;
        while (acceptor->isRunning() && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::yield();
        }
        require(success && !acceptor->isRunning(), "pool acceptor failed to process clients or stop");
        require(acceptor->getAcceptorSocketCount() == 0, "pool acceptor retained listener list after stopping");
    }
    else
    {
        auto acceptor = std::make_shared<Acceptors::MultiThreaded>();
        acceptor->parameters.setMaxConnectionsPerIP(32);
        acceptor->callbacks.onClientConnected = onConnect;
        acceptor->addAcceptorSocket(listener);
        acceptor->startInBackground();
        const bool success = exercise();
        acceptor->stop();
        require(success, "multithreaded acceptor failed to process clients");
    }
}

static void acceptorLimits()
{
    auto listener = std::make_shared<Socket_TCP>();
    require(listener->listenOn(0, "127.0.0.1"), "listen failed");
    timeouts(*listener);
    auto acceptor = std::make_shared<Acceptors::MultiThreaded>();
    acceptor->parameters.setMaxConnectionsPerIP(1);
    struct Context
    {
        Acceptors::MultiThreaded *acceptor;
        std::promise<void> connected;
        std::promise<bool> limited;
    } context{acceptor.get(), {}, {}};
    auto connected = context.connected.get_future();
    auto limited = context.limited.get_future();
    acceptor->callbacks.setAllContexts(&context);
    acceptor->callbacks.onClientConnected = [](void *opaque, const std::shared_ptr<Socket_Stream> &peer)
    {
        auto *context = static_cast<Context *>(opaque);
        context->connected.set_value();
        char byte;
        peer->partialRead(&byte, 1);
    };
    acceptor->callbacks.onClientConnectionLimitPerIPReached = [](void *opaque, const std::shared_ptr<Socket_Stream> &)
    {
        auto *context = static_cast<Context *>(opaque);
        // Public configuration access must be safe from inside callbacks.
        context->limited.set_value(context->acceptor->parameters.getMaxConnectionsPerIP() == 1);
    };
    acceptor->addAcceptorSocket(listener);
    acceptor->startInBackground();
    Socket_TCP first;
    Socket_TCP second;
    require(first.connectTo("127.0.0.1", listener->getLocalPort(), 2), "first connect failed");
    require(connected.wait_for(2s) == std::future_status::ready, "client callback did not run");
    require(second.connectTo("127.0.0.1", listener->getLocalPort(), 2), "second connect failed");
    const bool ready = limited.wait_for(2s) == std::future_status::ready;
    first.shutdownSocket();
    require(ready && limited.get(), "IP limit callback deadlocked or was not invoked");
    acceptor->stop();
}

static void immediateStop()
{
    // Include destruction without start and stop racing with initial startup.
    auto unstarted = std::make_shared<Acceptors::PoolThreaded>();
    for (int i = 0; i < 20; ++i)
    {
        auto listener = std::make_shared<Socket_TCP>();
        require(listener->listenOn(0, "127.0.0.1"), "listen failed");
        auto acceptor = std::make_shared<Acceptors::PoolThreaded>();
        acceptor->parameters.threadsCount = 2;
        acceptor->parameters.taskQueues = 1;
        acceptor->addAcceptorSocket(listener);
        acceptor->startInBackground();
        acceptor->stop();
        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (acceptor->isRunning() && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::yield();
        }
        require(!acceptor->isRunning(), "immediate stop was lost during startup");
    }
}

static void debugOutput()
{
    {
        auto pair = Socket_Stream::GetSocketPair();
        pair.first->setDebugOptions(Socket::PRINT_CLOSE);
        pair.first->setDebugOutput("");
    }
    require(fcntl(STDERR_FILENO, F_GETFD) >= 0, "socket destructor closed stderr");
    auto pair = Socket_Stream::GetSocketPair();
    pair.first->setDebugOptions(Socket::PRINT_ERRORS | Socket::PRINT_CLOSE);
    pair.first->closeSocket();
    require(pair.first->partialWrite("x", 1) == -1, "inactive write accepted");
}

static void tls(const std::string &mode, const char *certificate, const char *key)
{
    Socket_TLS listener;
    require(listener.tlsKeys.loadPublicKeyFromPEMFile(certificate), "load certificate failed");
    require(listener.tlsKeys.loadPrivateKeyFromPEMFile(key), "load private key failed");
    require(listener.listenOn(0, "127.0.0.1"), "TLS listen failed");
    timeouts(listener);
    if (mode == "client_timeout")
    {
        auto acceptor = std::async(std::launch::async, [&] { return listener.acceptConnection(); });
        Socket_TLS client;
        client.overrideReadTimeout(1);
        client.overrideWriteTimeout(1);
        require(client.tlsKeys.loadCAFromPEMFile(certificate), "load trust failed");
        const auto begin = std::chrono::steady_clock::now();
        require(!client.connectTo("localhost", listener.getLocalPort(), 2), "silent server completed TLS handshake");
        require(acceptor.get() != nullptr && !client.isActive() && std::chrono::steady_clock::now() - begin < 3s, "TLS client handshake ignored timeout");
        return;
    }
    if (mode == "server_timeout")
    {
        Socket_TCP raw;
        require(raw.connectTo("127.0.0.1", listener.getLocalPort(), 2), "raw connect failed");
        auto accepted = listener.acceptConnection();
        const auto begin = std::chrono::steady_clock::now();
        require(accepted && !accepted->postAcceptSubInitialization(), "silent TLS peer accepted");
        require(std::chrono::steady_clock::now() - begin < 4s, "TLS server handshake ignored timeout");
        return;
    }
    std::promise<void> release;
    auto released = release.get_future();
    auto server = std::async(std::launch::async, [&]
    {
        auto accepted = listener.acceptConnection();
        if (!accepted || !accepted->postAcceptSubInitialization())
        {
            return mode == "untrusted" || mode == "hostname" || mode == "default_trust";
        }
        if (mode == "exchange")
        {
            char data[5];
            if (!accepted->readFull(data, sizeof(data)) || std::memcmp(data, "hello", 5) != 0 || !accepted->writeFull("world"))
            {
                return false;
            }
            accepted->shutdownSocket(SHUT_WR);
            char byte;
            // Keep the transport alive until the client sends close_notify.
            // Otherwise its clean-shutdown assertion races with server teardown.
            if (accepted->partialRead(&byte, 1) != 0)
            {
                return false;
            }
        }
        else if (mode == "truncated")
        {
            accepted->closeSocket(); // Deliberately omit close_notify.
        }
        else
        {
            released.wait(); // Keep the peer silent during client shutdown.
        }
        return true;
    });
    bool connected = false;
    bool success = true;
    auto closeBegin = std::chrono::steady_clock::now();
    {
        Socket_TLS client;
        client.overrideReadTimeout(2);
        client.overrideWriteTimeout(2);
        if (mode == "insecure")
        {
            client.setCertValidation(Socket_TLS::X509ValidationOption::NOVALIDATE);
        }
        else if (mode == "default_trust")
        {
            // Keep the default VALIDATE setting, with no trust source configured.
        }
        else if (mode != "untrusted")
        {
            require(client.tlsKeys.loadCAFromPEMFile(certificate), "load trust failed");
        }
        else
        {
            client.tlsKeys.setUseSystemCertificates(true);
        }
        client.tlsKeys.setValidateServerHostname(true);
        connected = client.connectTo(mode == "hostname" ? "127.0.0.1" : "localhost", listener.getLocalPort(), 2);
        if (mode == "hostname" || mode == "untrusted" || mode == "default_trust")
        {
            success = !connected && !client.isActive();
        }
        else if (!connected)
        {
            success = false;
        }
        else if (mode == "exchange")
        {
            char data[5];
            ERR_put_error(ERR_LIB_USER, 0, 123, __FILE__, __LINE__);
            require(client.partialRead(nullptr, 0) == 0 && client.partialWrite(nullptr, 0) == 0, "empty TLS operation failed");
            require(client.writeFull("hello"), "TLS exchange write failed");
            require(!client.getTLSErrorsAndClear().empty(), "pending OpenSSL diagnostic was discarded");
            require(client.readFull(data, sizeof(data)) && std::memcmp(data, "world", 5) == 0, "TLS exchange reply mismatch");
            require(client.partialRead(data, 1) == 0, "TLS exchange did not receive close_notify");
            require(client.shutdownSocket() == 0, "TLS exchange clean shutdown failed");
        }
        else if (mode == "truncated")
        {
            char byte;
            success = client.partialRead(&byte, 1) == -1;
        }
        else
        {
            client.setReadTimeout(0);
            const auto begin = std::chrono::steady_clock::now();
            if (mode != "destructor")
            {
                const int shutdownStatus = client.shutdownSocket();
                success = shutdownStatus == -2 && client.getLastError().find("SSL_shutdown() incomplete") != std::string::npos;
            }
            success = success && std::chrono::steady_clock::now() - begin < 500ms;
            closeBegin = std::chrono::steady_clock::now();
        }
    }
    success = success && (mode != "destructor" || std::chrono::steady_clock::now() - closeBegin < 500ms);
    release.set_value();
    require(server.get() && success, "TLS scenario failed");
}

// Regression for the thread-pool acceptor reusing a worker thread whose
// OpenSSL error queue was left dirty by a previous connection: the next
// socket must not inherit stale TLS errors, and the expected incomplete
// shutdown (-2) must not be recorded in the error list.
static void tlsPoolReuse(const char *certificate, const char *key)
{
    struct PoolContext
    {
        std::atomic<bool> firstConnected{false};
        std::atomic<bool> initFailure{false};
    };
    auto context = std::make_shared<PoolContext>();
    auto onConnected = [](void *data, const std::shared_ptr<Socket_Stream> &sock)
    {
        auto *ctx = static_cast<PoolContext *>(data);
        if (!ctx->firstConnected.exchange(true))
        {
            // First client: complete the handshake, then drop the transport
            // without close_notify so the worker's SSL_shutdown() takes the
            // incomplete (-2) path.
            sock->shutdownSocket();
        }
    };
    auto onInitFailure = [](void *data, const std::shared_ptr<Socket_Stream> &sock)
    {
        auto *ctx = static_cast<PoolContext *>(data);
        auto tls = std::dynamic_pointer_cast<Socket_TLS>(sock);
        if (tls && !tls->getTLSErrorsAndClear().empty())
        {
            ctx->initFailure.store(true);
        }
    };

    auto listener = std::make_shared<Socket_TLS>();
    listener->tlsKeys.setSecurityLevel(-1);
    require(listener->tlsKeys.loadPublicKeyFromPEMFile(certificate), "TLS pool load certificate failed");
    require(listener->tlsKeys.loadPrivateKeyFromPEMFile(key), "TLS pool load private key failed");
    listener->setCertValidation(Socket_TLS::X509ValidationOption::NOVALIDATE);
    require(listener->listenOn(0, "127.0.0.1"), "TLS pool listen failed");
    timeouts(*listener);

    auto acceptor = std::make_shared<Acceptors::PoolThreaded>();
    acceptor->parameters.threadsCount = 1; // Force worker-thread reuse.
    acceptor->parameters.taskQueues = 1;
    acceptor->callbacks.onClientConnected = onConnected;
    acceptor->callbacks.onProtocolInitializationFailure = onInitFailure;
    acceptor->callbacks.contextOnConnect = context.get();
    acceptor->callbacks.contextOnInitFail = context.get();
    acceptor->addAcceptorSocket(listener);
    acceptor->startInBackground();

    Socket_TLS client;
    client.overrideReadTimeout(2);
    client.overrideWriteTimeout(2);
    client.setCertValidation(Socket_TLS::X509ValidationOption::NOVALIDATE);
    require(client.connectTo("127.0.0.1", listener->getLocalPort(), 2), "TLS pool first connect failed");
    require(context->firstConnected.load(), "TLS pool first handshake not served");

    // The first connection's incomplete shutdown must not pollute the error
    // list even though its status is -2.
    require(client.shutdownSocket() >= -2, "TLS pool first shutdown status lost");
    require(client.getTLSErrorsAndClear().empty(), "incomplete TLS shutdown recorded an error");

    // Second client: may land on the same worker thread. Its initialization
    // must succeed with a clean error list (no stale queue entries).
    require(!context->initFailure.load(), "TLS pool init failure reported stale errors");

    Socket_TLS client2;
    client2.overrideReadTimeout(2);
    client2.overrideWriteTimeout(2);
    client2.setCertValidation(Socket_TLS::X509ValidationOption::NOVALIDATE);
    require(client2.connectTo("127.0.0.1", listener->getLocalPort(), 2), "TLS pool second connect failed");
    require(client2.getTLSErrorsAndClear().empty(), "TLS pool second connect inherited stale TLS errors");
    require(!context->initFailure.load(), "TLS pool init failure after worker reuse");
    client2.shutdownSocket();

    acceptor->stop();
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (acceptor->isRunning() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    require(!context->initFailure.load(), "TLS pool worker reuse leaked TLS errors");
}

int main(int argc, char **argv)
{
    try
    {
        require(argc >= 2, "test case argument required");
        const std::string name = argv[1];
        if ((name == "tcp_ipv6" || name == "udp_ipv6") && !ipv6Available())
        {
            std::cout << "SKIP: IPv6 loopback is unavailable\n";
            return 77;
        }
        if (name == "lifecycle") { lifecycle(); }
        else if (name == "tcp_ipv4" || name == "tcp_ipv6") { tcp(name == "tcp_ipv6"); }
        else if (name == "tcp_options") { tcpOptions(); }
        else if (name == "tcp_failures") { tcpFailures(); }
        else if (name == "tcp_reuse") { tcpReuse(); }
        else if (name == "tcp_dns_fallback") { dnsFallback(); }
        else if (name == "stream_transfer") { streamTransfer(); }
        else if (name == "stream_errors") { streamErrors(); }
        else if (name == "interrupted_io") { interruptedIO(); }
        else if (name == "shutdown_wakeup") { shutdownWakeup(); }
        else if (name == "high_descriptors") { highDescriptors(); }
        else if (name == "stream_blocks") { streamBlocks(); }
        else if (name == "stream_integers") { streamIntegers(); }
        else if (name == "stream_delimiters") { streamDelimiters(); }
        else if (name == "buffered_reader") { bufferedReader(); }
        else if (name == "udp_ipv4" || name == "udp_ipv6") { udp(name == "udp_ipv6"); }
        else if (name == "udp_errors") { udpErrors(); }
        else if (name == "datagram_ownership") { datagramOwnership(); }
        else if (name == "unix_sockets") { unixSockets(); }
        else if (name == "unix_backlog") { unixBacklog(); }
        else if (name == "acceptor_pool") { acceptors(true); }
        else if (name == "acceptor_threads") { acceptors(false); }
        else if (name == "acceptor_limits") { acceptorLimits(); }
        else if (name == "immediate_stop") { immediateStop(); }
        else if (name == "debug_output") { debugOutput(); }
        else if (name == "tls_pool_reuse" && argc == 4) { tlsPoolReuse(argv[2], argv[3]); }
        else if (name == "tls" && argc == 5) { tls(argv[2], argv[3], argv[4]); }
        else { throw std::runtime_error("Unknown test case"); }
        std::cout << "PASS: " << name << '\n';
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
