/** Copyright (c) 2026 pzjw0066
 * 该库是对Winsock2的一个简易C++封装版本，使用RAII，让资源管理更便捷。
 * 由于本库使用了Header Only的风格，你需要在引入头文件前定义WINSOCK2_CPP_IMPLEMENTATION宏，以插入该库的实现。
 * 注意：WINSOCK2_CPP_IMPLEMENTATION宏在项目中只能定义一次！
 */

#ifndef WINSOCK2_CPP_HPP
#define WINSOCK2_CPP_HPP

#include <stdexcept>
#include <string>
#include <cstdint>
#include <ws2tcpip.h>
#include <winsock.h>

namespace winsock2 {
    // Socket每个阶段可能发生的异常：
    struct WSAStartupFailedError : public std::runtime_error {
        WSAStartupFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct InvalidSocketError : public std::runtime_error {
        InvalidSocketError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct GetaddrinfoFailedError : public std::runtime_error {
        GetaddrinfoFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketCreatingFailedError : public std::runtime_error {
        SocketCreatingFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketConnectingFailedError : public std::runtime_error {
        SocketConnectingFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketSendingFailedError : public std::runtime_error {
        SocketSendingFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketRecvFailedError : public std::runtime_error {
        SocketRecvFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketBindingFailedError : public std::runtime_error {
        SocketBindingFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketListeningFailedError : public std::runtime_error {
        SocketListeningFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketAcceptingFailedError : public std::runtime_error {
        SocketAcceptingFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct SocketShutdownFailedError : public std::runtime_error {
        SocketShutdownFailedError(const std::string& msg) : std::runtime_error(msg) {}
    };

    class WSAData {
        WSADATA wsaData;
        WSAData();
    public:
        WSAData(const WSAData&) = delete;
        WSAData(WSAData&&) = delete;
        ~WSAData() {
            WSACleanup();
        }
        WSAData& operator=(const WSAData&) = delete;
        WSAData& operator=(const WSAData&&) = delete;

        // 初始化Winsock2，该函数需要在所有的网络操作前调用。
        static void Initialize() {
            static WSAData wsaData;
        }
    };

    class Socket {
        addrinfo hints_;
        SOCKET socket_ = INVALID_SOCKET;

        // 为了支持Accept函数返回新的Socket。
        Socket(const SOCKET& socket) {
            socket_ = socket;
        }
    public:
        // 创建服务器端Socket时，ai_flags要传入AI_PASSIVE。
        Socket(int ai_family, int ai_socktype, int ai_protocol, int ai_flags = 0) :
            hints_{.ai_flags = ai_flags, .ai_family = ai_family, .ai_socktype = ai_socktype, .ai_protocol = ai_protocol} {}

        Socket(const Socket&) = delete;
        Socket(Socket&& other) {
            this->socket_ = other.socket_;
            other.socket_ = INVALID_SOCKET;
        }
        ~Socket() {
            if (socket_ != INVALID_SOCKET)
                closesocket(socket_);
        }
        Socket& operator=(const Socket&) = delete;
        Socket& operator=(Socket&& other) {
            this->socket_ = other.socket_;
            other.socket_ = INVALID_SOCKET;
            return *this;
        }

        // 除了Send、Recv、Shutdown以外，其他操作均支持流畅接口模式。

        Socket& Connect(const std::string& addr, std::uint16_t port);

        int Send(const std::string& data);

        /** is_blocked表示阻塞模式：
         * 当它为true时，表示阻塞，函数永远返回true，结果保存在output中；
         * 当它为false时，表示非阻塞，函数如果有结果才会返回true，结果同样保存在output中，否则返回false（此时output不是结果，因为没有发生读写）。
         */
        bool Recv(bool is_blocked, std::string& output); 

        Socket& Bind(std::uint16_t port);
        Socket& Listen(int max_count);
        Socket Accept();
        void Shutdown();
    };
}

#ifdef WINSOCK2_CPP_IMPLEMENTATION

#include <format>
#include <sstream>

namespace winsock2 {
    WSAData::WSAData() {
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult)
            throw WSAStartupFailedError(std::format("WSAStartup Failed with error: {}", iResult));
    }

    Socket& Socket::Connect(const std::string& addr, std::uint16_t port) {
        addrinfo* result;
        
        int iResult = getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &hints_, &result);
        if (iResult) {
            throw GetaddrinfoFailedError(std::format("Getaddrinfo failed with error: {}", iResult));
        }

        addrinfo* ptr = result;
        while (ptr) {
            socket_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (socket_ == INVALID_SOCKET)
                throw SocketCreatingFailedError(std::format("Socket creating failed with error: {}", WSAGetLastError()));
            
            iResult = connect(socket_, ptr->ai_addr, ptr->ai_addrlen);
            if (iResult != SOCKET_ERROR) 
                break;

            closesocket(socket_);
            socket_ = INVALID_SOCKET;
            ptr = ptr->ai_next;
        }

        freeaddrinfo(result);

        if (socket_ == INVALID_SOCKET)
            throw SocketConnectingFailedError("Unable to connect to server.");

        return *this;
    }

    int Socket::Send(const std::string& data) {
        if (socket_ == INVALID_SOCKET)
            throw InvalidSocketError("Socket is invalied.");

        int iResult = send(socket_, data.c_str(), data.size(), 0);
        if (iResult == SOCKET_ERROR)
            throw SocketSendingFailedError(std::format("Socket sending failed with error: {}", iResult));
        return iResult;
    }

    bool Socket::Recv(bool is_blocked, std::string& output) {
        if (socket_ == INVALID_SOCKET)
            throw InvalidSocketError("Socket is invalied.");

        u_long mode = !is_blocked;
        ioctlsocket(socket_, FIONBIO, &mode);

        std::ostringstream writer;
        char buffer[512]{};
        int iResult;

        iResult = recv(socket_, buffer, sizeof(buffer), 0);
        
        if (iResult == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
                throw SocketRecvFailedError(std::format("Socket recv failed with error: {}", WSAGetLastError()));
            return false;
        }
        
        writer << buffer;

        output = std::move(writer.str());
        return true;
    }

    Socket& Socket::Bind(std::uint16_t port) {
        addrinfo* result;

        int iResult = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints_, &result);
        if (iResult)
            throw GetaddrinfoFailedError(std::format("Getaddrinfo failed with error: {}", iResult));
        
        socket_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (socket_ == INVALID_SOCKET)
            throw SocketCreatingFailedError(std::format("Socket creating failed with error: {}", WSAGetLastError()));

        iResult = bind(socket_, result->ai_addr, result->ai_addrlen);
        if (iResult)
            throw SocketBindingFailedError(std::format("Socket binding failed with error: {}", iResult));
        
        freeaddrinfo(result);

        return *this;
    }

    Socket& Socket::Listen(int max_count) {
        if (socket_ == INVALID_SOCKET)
            throw InvalidSocketError("Socket is invalied.");

        int iResult = listen(socket_, max_count);
        if (iResult == SOCKET_ERROR)
            throw SocketListeningFailedError(std::format("Socket listening failed with error: {}", WSAGetLastError()));

        return *this;
    }

    Socket Socket::Accept() {
        if (socket_ == INVALID_SOCKET)
            throw InvalidSocketError("Socket is invalied.");

        SOCKET client_socket = accept(socket_, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET)
            throw SocketAcceptingFailedError(std::format("Socket accepting failed with error: {}", WSAGetLastError()));
        
        return Socket(client_socket);
    }

    void Socket::Shutdown() {
        int iResult = shutdown(socket_, SD_SEND);
        if (iResult == SOCKET_ERROR)
            throw SocketShutdownFailedError(std::format("Socket shutdown failed with error: {}", WSAGetLastError()));
    }
}

#endif

#endif
