#include "net.h"

#include "ikcp.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <Windows.h>

#undef EINTR
#undef EAGAIN
#undef EWOULDBLOCK

#define EINTR WSAEINTR
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EAGAIN WSATRY_AGAIN
#define socklen_t int

#undef errno
#define errno WSAGetLastError()

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
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define closesocket close
#endif
#include <cstring>
#include <stdexcept>

#ifndef _WIN32
#include <ctime>
uint32_t GetTickCount() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

static std::vector<struct in_addr> broadcastAddr_;

void UdpClient::init() {
#ifdef _WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    struct in_addr IPAddr;

    pIPAddrTable = (MIB_IPADDRTABLE *)malloc(sizeof(MIB_IPADDRTABLE));
    if (pIPAddrTable) {
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            free(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE *)malloc(dwSize);
            if (pIPAddrTable == NULL)
                return;
        }
    } else return;
    if ((dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0)) != NO_ERROR) {
        return;
    }

    for (int i = 0; i < (int)pIPAddrTable->dwNumEntries; i++) {
        IPAddr.s_addr = (u_long)(pIPAddrTable->table[i].dwAddr | ~pIPAddrTable->table[i].dwMask);
        printf("\tBroadCast[%d]:      \t%s\n", i, inet_ntoa(IPAddr));
        broadcastAddr_.push_back(IPAddr);
        printf("\n");
    }

    free(pIPAddrTable);
#endif
}

void UdpClient::finish() {
    broadcastAddr_.clear();
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

void UdpClient::autoconnect(uint16_t port) {
    disconnect();
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    for (auto &p: broadcastAddr_) {
        sa.sin_addr = p;
        ::sendto(fd_, "B", 1, 0, (const struct sockaddr*)&sa, sizeof(struct sockaddr_in));
    }
}

bool UdpClient::connect(const std::string &addr, uint16_t port) {
    disconnect();
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    if (inet_pton(AF_INET, addr.c_str(), &sa.sin_addr) < 0)
        return false;
    sa.sin_port = htons(port);
    return _startConnect(&sa);
}

void UdpClient::disconnect() {
    title_.clear();
    titleid_.clear();
    if (kcp_ != NULL) {
        _send("D", 1);
        ikcp_release(kcp_);
        kcp_ = NULL;
        conv_ = 0;
        recvBuf_.clear();
    }
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
    fd_set rfds, efds;
    FD_ZERO(&rfds); FD_ZERO(&efds); FD_SET(fd_, &rfds); FD_SET(fd_, &efds);
    struct timeval tv = {0, 0};
    int n;
    if (packets_.empty()) {
        n = select(1, &rfds, NULL, &efds, &tv);
        if (n == 0) return;
    } else {
        fd_set wfds;
        FD_ZERO(&wfds); FD_SET(fd_, &wfds);
        n = select(1, &rfds, &wfds, &efds, &tv);
        if (n == 0) return;
        if (FD_ISSET(fd_, &wfds)) {
            while (!packets_.empty()) {
                std::string &s = packets_.front();
                if (_send(s.c_str(), s.length()) <= 0) break;
                packets_.pop_front();
            }
        }
    }
    if (FD_ISSET(fd_, &rfds)) {
        int r;
        char buf[2048];
        struct sockaddr_in addr;
        while ((r = _recv(buf, 2048, &addr)) > 0) {
            if (buf[0] == 'D') {
                disconnect();
                break;
            }
            switch (buf[0]) {
                case 'B':
                    _startConnect(&addr);
                    break;
                case 'C':
                    if (r < 14) break;
                    conv_ = *(uint32_t*)&buf[1];
                    titleid_.assign(buf + 5, buf + 14);
                    title_ = buf + 14;
                    kcp_ = ikcp_create(conv_, this);
                    ikcp_nodelay(kcp_, 0, 100, 0, 0);
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
                    connecting_ = false;
                    if (onConnected_) onConnected_(inet_ntoa(addr.sin_addr));
                    break;
                case 'K':
                    ikcp_input(kcp_, buf + 4, r - 4);
                    break;
            }
        }
    }
    if (FD_ISSET(fd_, &efds)) {
        // handle exception
    }
}

int UdpClient::send(const char *buf, int len) {
    return ikcp_send(kcp_, buf, len);
}

void UdpClient::_init() {
    connecting_ = false;
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0)
        throw std::runtime_error("unable to create socket");
    int n = 1;
    setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, (const char*)&n, sizeof(n));
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = 0;
    if (bind(fd_, (const sockaddr*)&sa, sizeof(sa)) < 0)
        throw std::runtime_error("unable to bind address");
}

bool UdpClient::_startConnect(void *addr) {
    recvBuf_.clear();
    if (::connect(fd_, (const struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0) {
        return false;
    }
#ifdef _WIN32
    u_long n = 1;
    ioctlsocket(fd_, FIONBIO, &n);
#else
    int flags;
    if ((flags = fcntl(fd_, F_GETFL, NULL)) < 0) {
        return true;
    }
    if (!(flags & O_NONBLOCK)) {
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
    }
#endif
    char res[256] = {0};
    packets_.push_back("C");
    connecting_ = true;
    return true;
}

int UdpClient::_send(const char *buf, int len) {
    int n = ::send(fd_, buf, len, 0);
    if (n < 0) {
        int err = errno;
        if (err == EINTR || err == EWOULDBLOCK || err == EAGAIN) {
            return 0;
        }
        return -1;
    }
    return n;
}

int UdpClient::_recv(char *buf, int len, void *addr) {
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int n = ::recvfrom(fd_, buf, len, 0, (struct sockaddr *)addr, &addrlen);
    if (n < 0) {
        int err = errno;
        if (err == EINTR || err == EWOULDBLOCK || err == EAGAIN) {
            return 0;
        }
        return -1;
    }
    return n;
}
