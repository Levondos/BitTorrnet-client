#include "tcp_connect.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>


TcpConnect::TcpConnect(std::string ip, int16_t port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout):
    ip_{ip},
    port_{port},
    connectTimeout_{connectTimeout},
    readTimeout_{readTimeout},
    sock_{-1}
{
    struct in_addr addr;
    if (inet_pton(AF_INET, ip_.c_str(), &addr) != 1) {
        throw std::invalid_argument("Invalid IP address format: " + ip_);
    }
    ipv4_ = addr.s_addr;
}

TcpConnect::~TcpConnect() {
    CloseConnection();
}

void TcpConnect::EstablishConnection() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        throw std::runtime_error("socket: " + std::string(strerror(errno)));
    }

    {
        int flags = fcntl(sock_, F_GETFL, 0);
        fcntl(sock_, F_SETFL, flags | O_NONBLOCK);
    }

    {
        sockaddr_in addr_peer;
        addr_peer.sin_family = AF_INET;
        addr_peer.sin_port = htons(port_);
        addr_peer.sin_addr.s_addr = ipv4_;
        if (connect(sock_, reinterpret_cast<sockaddr*>(&addr_peer), sizeof(addr_peer)) < 0 && errno != EINPROGRESS) {
            throw std::runtime_error("Start connect: " + std::string(strerror(errno)));
        }
    }

    {
        fd_set fd_write;
        FD_ZERO(&fd_write);
        FD_SET(sock_, &fd_write);
        timeval con_timeout;
        con_timeout.tv_sec = connectTimeout_.count()/1000;
        con_timeout.tv_usec = (connectTimeout_.count()%1000)*1000;
        int res_connection = select(sock_+1, nullptr, &fd_write, nullptr, &con_timeout);
        if (res_connection < 0) {
            CloseConnection();
            throw std::runtime_error("select: " + std::string(strerror(errno)));
        }
        else if (res_connection == 0){
            CloseConnection();
            throw std::runtime_error("Connection timed out");
        }
    }

    {
        int err_end_connect;
        socklen_t len = sizeof(err_end_connect);
        getsockopt(sock_, SOL_SOCKET, SO_ERROR, &err_end_connect, &len);
        if (err_end_connect != 0) {
            CloseConnection();
            throw std::runtime_error("End connect: " + std::string(strerror(err_end_connect)));
        }
    }

    {
        int flags = fcntl(sock_, F_GETFL, 0);
        flags &= ~O_NONBLOCK;
        fcntl(sock_, F_SETFL, flags);
    }

    {
        timeval read_timeout{};
        read_timeout.tv_sec = readTimeout_.count()/1000;
        read_timeout.tv_usec = (readTimeout_.count()%1000)*1000;
        setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    }
}

void TcpConnect::SendData(const std::string& data) const {
    size_t total = 0;
    while (total < data.size()) {
        ssize_t sent_bytes = send(sock_, data.c_str()+total, data.size()-total, 0);
        if (sent_bytes < 0) {
            throw std::runtime_error("send: " + std::string(strerror(errno)));
        }
        total += sent_bytes;
    }
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    if (bufferSize == 0) {
        std::string len_buf;
        len_buf.resize(4);
        size_t total = 0;
        while (total < len_buf.size()) {
            ssize_t read_bytes = recv(sock_, &len_buf[total], len_buf.size()-total, 0);
            if (read_bytes <= 0) {
                throw std::runtime_error("recv: " + std::string(strerror(errno)));
            }
            total += read_bytes;
        }
        bufferSize = BytesToInt(len_buf);
    }

    std::string buf;
    buf.resize(bufferSize);
    size_t total = 0;
    while (total < bufferSize) {
        ssize_t read_bytes = recv(sock_, &buf[total], bufferSize - total, 0);
        if (read_bytes <= 0) {
            throw std::runtime_error("recv: " + std::string(strerror(errno)));
        }
        total += read_bytes;
    }
    return buf;
}


void TcpConnect::CloseConnection() {
    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
    }
}

const std::string& TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}