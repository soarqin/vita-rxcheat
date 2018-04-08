#ifndef __DEBUG_H_
#define __DEBUG_H_

enum {
    NONE = 0,
    INFO = 1,
    ERROR = 2,
    TRACE = 3,
    DEBUG = 4,
};

#ifdef RCSVR_DEBUG
#ifdef RCSVR_DEBUG_LINENO
void debug_printf(int level, const char *srcfile, int lineno, const char* format, ...);
#else
void debug_printf(int level, const char* format, ...);
#endif
void debug_set_loglevel(int level);
void debug_init(int level);
#else
inline void debug_printf(int level, const char* format, ...) {}
inline void debug_set_loglevel(int level) {}
inline void debug_init(int level) {}
#endif

#ifdef RCSVR_DEBUG_LINENO
#define log_none(...) debug_printf(NONE, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) debug_printf(INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) debug_printf(ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_trace(...) debug_printf(TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) debug_printf(DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define log_none(...) debug_printf(NONE, __VA_ARGS__)
#define log_info(...) debug_printf(INFO, __VA_ARGS__)
#define log_error(...) debug_printf(ERROR, __VA_ARGS__)
#define log_trace(...) debug_printf(TRACE, __VA_ARGS__)
#define log_debug(...) debug_printf(DEBUG, __VA_ARGS__)
#endif

#endif
