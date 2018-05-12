#ifdef RCSVR_DEBUG

#include "debug.h"
#include "../version.h"

#include <vitasdkkern.h>

#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <libk/string.h>

static int debug_fd = -1;
static int loglevel = 0;

const char *level_names[] = {" NONE", " INFO", "ERROR", "TRACE", "DEBUG"};

#ifdef RCSVR_DEBUG_LINENO
void debug_printf(int level, const char *srcfile, int lineno, const char* format, ...) {
#else
void debug_printf(int level, const char* format, ...) {
#endif
    if (debug_fd < 0 || level > loglevel) return;
    enum {
        buffer_size = 1460,
    };
    char buffer[buffer_size];
    char *msgbuf = buffer;
    int len;
    size_t left = buffer_size;
    va_list args;

    switch(level) {
        case INFO:
        case ERROR:
        case TRACE:
        case DEBUG:
#ifdef RCSVR_DEBUG_LINENO
            len = snprintf(buffer, buffer_size, "[VITA][%s] (%s:%d) ", level_names[level], srcfile, lineno);
#else
            len = snprintf(buffer, buffer_size, "[VITA][%s] ", level_names[level]);
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
    vsnprintf(msgbuf, left, format, args);
    buffer[buffer_size - 1] = 0;
    va_end(args);
    ksceIoWrite(debug_fd, buffer, strlen(buffer));
}

void debug_set_loglevel(int level) {
    loglevel = level;
}

void debug_init(int level) {
    if (debug_fd >= 0) return;
    debug_set_loglevel(level);
    debug_fd = ksceIoOpen("ux0:temp/rcsvr_k.log", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);

    log_none(PLUGIN_NAME " v" VERSION_STR " kernel plugin loaded\n");
}

#endif
