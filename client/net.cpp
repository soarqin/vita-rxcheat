#include "net.h"

#include "ikcp.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#if NTDDI_VERSION < NTDDI_VISTA
int inet_pton(int af, const char *src, void *dst) {
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN + 1];

    ZeroMemory(&ss, sizeof(ss));
    /* stupid non-const API */
    strncpy(src_copy, src, INET6_ADDRSTRLEN + 1);
    src_copy[INET6_ADDRSTRLEN] = 0;

    if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
        switch (af) {
            case AF_INET:
                *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                return 1;
            case AF_INET6:
                *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                return 1;
        }
    }
    return 0;
}
#endif
#else
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define closesocket close
#endif

void UdpClient::init() {
#ifdef _WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
}

void UdpClient::finish() {
#ifdef _WIN32
    WSACleanup();
#endif
}

UdpClient::UdpClient() {
    _init();
}

UdpClient::~UdpClient() {
    closesocket(fd_);
}

bool UdpClient::connect(const std::string &addr, uint16_t port) {
    disconnect();
    recvBuf_.clear();
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    if (inet_pton(AF_INET, addr.c_str(), &sa.sin_addr) < 0)
        return false;
    sa.sin_port = htons(port);
    if (::connect(fd_, (const sockaddr*)&sa, sizeof(sa)) < 0)
        return false;
    char res[256] = {0};
    int r = _sendAndRecv("C", 1, res, 255);
    if (r < 14 || res[0] != 'C') return false;
    conv_ = *(uint32_t*)&res[1];
    titleid_.assign(res + 5, res + 14);
    title_ = res + 14;
    kcp_ = ikcp_create(conv_, this);
    ikcp_nodelay(kcp_, 0, 50, 0, 0);
    ikcp_setmtu(kcp_, 1440);
    kcp_->output = [](const char *buf, int len, ikcpcb *kcp, void *user) {
        fprintf(stdout, "Sending K packets: %d\n", len);
        UdpClient *uc = (UdpClient*)user;
        std::string tosend;
        tosend.resize(len + 4);
        tosend[0] = 'K';
        memcpy(&tosend[4], buf, len);
        uc->packets_.push_back(tosend);
        return 0;
    };
#ifdef _WIN32
    u_long n = 1;
    ioctlsocket(fd_, FIONBIO, &n);
#else
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        return true;
    }
    if (!(flags & O_NONBLOCK)) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
#endif
    return true;
}

void UdpClient::disconnect() {
    if (kcp_ == NULL) return;
    title_.clear();
    titleid_.clear();
    _send("D", 1);
    ikcp_release(kcp_);
    kcp_ = NULL;
    recvBuf_.clear();
    closesocket(fd_);
    _init();
}

void UdpClient::process() {
    if (kcp_ != NULL) {
        ikcp_update(kcp_, GetTickCount());
        int n;
        char buf[3072];
        while ((n = ikcp_recv(kcp_, buf, 3072)) > 0) {
            recvBuf_.insert(recvBuf_.end(), buf, buf + n);
            while (recvBuf_.length() >= 8) {
                size_t len = *(uint32_t*)&recvBuf_[4];
                if (len + 8 > recvBuf_.length()) break;
                onRecv_(*(int*)&recvBuf_[0], recvBuf_.c_str() + 8, len);
                recvBuf_.erase(recvBuf_.begin(), recvBuf_.begin() + len + 8);
            }
        }
    }
    fd_set rfds = {1, {fd_}}, efds = {1, {fd_}};
    struct timeval tv = {0, 0};
    int n;
    if (packets_.empty()) {
        n = select(1, &rfds, NULL, &efds, &tv);
        if (n == 0) return;
    } else {
        fd_set wfds = {1, {fd_}};
        n = select(1, &rfds, &wfds, &efds, &tv);
        if (n == 0) return;
        if (wfds.fd_count > 0) {
            while (!packets_.empty()) {
                std::string &s = packets_.front();
                if (_send(s.c_str(), s.length()) <= 0) break;
                packets_.pop_front();
            }
        }
    }
    if (rfds.fd_count > 0) {
        int r;
        char buf[2048];
        while ((r = _recv(buf, 2048)) > 0) {
            switch (buf[0]) {
            case 'K':
                ikcp_input(kcp_, buf + 4, r - 4);
                break;
            }
        }
    }
    if (efds.fd_count > 0) {
        // handle exception
    }
}

int UdpClient::send(const char *buf, int len) {
    return ikcp_send(kcp_, buf, len);
}

void UdpClient::_init() {
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0)
        throw std::exception("unable to create socket");
    int n = 1;
    setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, (const char*)&n, sizeof(n));
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = 0;
    if (bind(fd_, (const sockaddr*)&sa, sizeof(sa)) < 0)
        throw std::exception("unable to bind address");
}

int UdpClient::_send(const char *buf, int len) {
    int n = ::send(fd_, buf, len, 0);
    if (n < 0) {
        int err = WSAGetLastError();
        if (err == WSAEINTR || err == WSAEWOULDBLOCK || err == WSATRY_AGAIN) {
            return 0;
        }
        return -1;
    }
    return n;
}

int UdpClient::_recv(char *buf, int len) {
    int n = ::recv(fd_, buf, len, 0);
    if (n < 0) {
        int err = WSAGetLastError();
        if (err == WSAEINTR || err == WSAEWOULDBLOCK || err == WSATRY_AGAIN) {
            return 0;
        }
        return -1;
    }
    return n;
}

int UdpClient::_sendAndRecv(const char *buf, int len, char *res, int reslen) {
    if (::send(fd_, buf, len, 0) < 0) return -1;
    return ::recv(fd_, res, reslen, 0);
}
