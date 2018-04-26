#ifdef RCSVR_DEBUG

#include "debug.h"

#include <vitasdk.h>
#include "kio.h"

#ifndef RCSVR_DEBUG_IP
#define RCSVR_DEBUG_IP "192.168.11.125"
#endif
#ifndef RCSVR_DEBUG_PORT
#define RCSVR_DEBUG_PORT 18194
#endif

static int debug_fd = -1;
static int is_file = 0;
static int loglevel = 0;

const char *level_names[] = {" NONE", " INFO", "ERROR", "TRACE", "DEBUG"};

#ifdef RCSVR_DEBUG_LINENO
void debug_printf(int level, const char *srcfile, int lineno, const char* format, ...) {
#else
void debug_printf(int level, const char* format, ...) {
#endif
    if (debug_fd < 0) return;
    enum {
        buffer_size = 1460,
    };
    char buffer[buffer_size];
    char *msgbuf = buffer;
    int len;
    size_t left = buffer_size;
    va_list args;

    if (level > loglevel) return;

    switch(level) {
        case INFO:
        case ERROR:
        case TRACE:
        case DEBUG:
#ifdef RCSVR_DEBUG_LINENO
            len = sceClibSnprintf(buffer, buffer_size, "[VITA][%s] (%s:%d) ", level_names[level], srcfile, lineno);
#else
            len = sceClibSnprintf(buffer, buffer_size, "[VITA][%s] ", level_names[level]);
#endif
            if (len > 0) {
                msgbuf += len;
                left -= len;
                break;
            }
        default:
            buffer[0] = 0;
            break;
    }
    va_start(args, format);
    sceClibVsnprintf(msgbuf, left, format, args);
    buffer[buffer_size - 1] = 0;
    va_end(args);
    if (is_file)
        kIoWrite(debug_fd, buffer, strlen(buffer), NULL);
    else
        sceNetSend(debug_fd, buffer, strlen(buffer), 0);
}

void debug_set_loglevel(int level) {
    loglevel = level;
}

void debug_init(int level) {
    if (level > 0) {
        struct SceNetSockaddrIn sockaddr;
        debug_set_loglevel(level);
        debug_fd = sceNetSocket("debug_socket", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, SCE_NET_IPPROTO_UDP);
        memset(&sockaddr, 0, sizeof sockaddr);

        sockaddr.sin_family = SCE_NET_AF_INET;
        sockaddr.sin_port = sceNetHtons(RCSVR_DEBUG_PORT);
        sceNetInetPton(SCE_NET_AF_INET, RCSVR_DEBUG_IP, &sockaddr.sin_addr);

        /*Connect socket to server*/
        sceNetConnect(debug_fd, (struct SceNetSockaddr *)&sockaddr, sizeof sockaddr);

        is_file = 0;
    } else {
        debug_set_loglevel(-level);
        kIoOpen("ux0:/temp/rcsvr.log", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, &debug_fd);
        is_file = 1;
    }

    /*Show log on pc/mac side*/
    log_none("Initialized debug log\n");
}

#endif
