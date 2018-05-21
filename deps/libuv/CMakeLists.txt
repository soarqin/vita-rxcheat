PROJECT(uv)

include_directories(include src)

set(UV_COMMON_SRCS
    include/uv.h
    include/uv-errno.h
    include/uv-threadpool.h
    include/uv-version.h
    src/heap-inl.h
    src/queue.h
    src/fs-poll.c
    src/inet.c
    src/threadpool.c
    src/uv-common.c
    src/uv-common.h
    src/uv-data-getter-setters.c
    src/version.c
    )


set(UV_WIN32_SRCS
    include/uv-win.h
    include/tree.h
    src/win/async.c
    src/win/atomicops-inl.h
    src/win/core.c
    src/win/detect-wakeup.c
    src/win/dl.c
    src/win/error.c
    src/win/fs.c
    src/win/fs-event.c
    src/win/getaddrinfo.c
    src/win/getnameinfo.c
    src/win/handle.c
    src/win/handle-inl.h
    src/win/internal.h
    src/win/loop-watcher.c
    src/win/pipe.c
    src/win/poll.c
    src/win/process-stdio.c
    src/win/process.c
    src/win/req.c
    src/win/req-inl.h
    src/win/signal.c
    src/win/snprintf.c
    src/win/stream.c
    src/win/stream-inl.h
    src/win/tcp.c
    src/win/thread.c
    src/win/timer.c
    src/win/tty.c
    src/win/udp.c
    src/win/util.c
    src/win/winapi.c
    src/win/winapi.h
    src/win/winsock.c
    src/win/winsock.h
    )

set(UV_POSIX_SRCS
    include/uv-unix.h
    src/unix/async.c
    src/unix/atomic-ops.h
    src/unix/core.c
    src/unix/dl.c
    src/unix/fs.c
    src/unix/getaddrinfo.c
    src/unix/getnameinfo.c
    src/unix/internal.h
    src/unix/loop.c
    src/unix/loop-watcher.c
    src/unix/pipe.c
    src/unix/poll.c
    src/unix/process.c
    src/unix/proctitle.c
    src/unix/signal.c
    src/unix/spinlock.h
    src/unix/stream.c
    src/unix/tcp.c
    src/unix/thread.c
    src/unix/timer.c
    src/unix/tty.c
    src/unix/udp.c
    )

# set UV_SRCS and UV_LIBS

if(WIN32)
    include_directories(src/win)
    add_definitions(-D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -D_GNU_SOURCE)
    if(MSVC)
        add_definitions(-wd4244 -wd4267 -wd4996)
    endif()
    set(UV_SRCS ${UV_WIN32_SRCS} ${UV_COMMON_SRCS})
    set(UV_LIBS advapi32 iphlpapi psapi shell32 user32 userenv ws2_32)
else()
    # POSIX common
    add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE)
    include_directories(src/unix)

    if(APPLE)
        add_definitions(-D_DARWIN_USE_64_BIT_INODE=1 -D_DARWIN_UNLIMITED_SELECT=1)
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/bsd-ifaddrs.c
            src/unix/darwin.c
            src/unix/darwin-proctitle.c
            src/unix/fsevents.c
            src/unix/kqueue.c
            src/unix/proctitle.c
            )
        FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices)
        MARK_AS_ADVANCED(APP_SERVICES_LIBRARY)
        set(UV_LIBS m ${APP_SERVICES_LIBRARY})
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        add_definitions(-D_POSIX_C_SOURCE=200112 -D_GNU_SOURCE)
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/linux-core.c
            src/unix/linux-inotify.c
            src/unix/linux-syscalls.c
            src/unix/linux-syscalls.h
            src/unix/procfs-exepath.c
            src/unix/proctitle.c
            src/unix/sysinfo-loadavg.c
            src/unix/sysinfo-memory.c
            )
        set(UV_LIBS rt m dl)
    elseif(${CMAKE_SYSTEM_NAME} Matches "FreeBSD|DragonFly")
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/bsd-ifaddrs.c
            src/unix/freebsd.c
            src/unix/kqueue.c
            src/unix/posix-hrtime.c
            )
        set(UV_LIBS m pthread kvm)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/bsd-ifaddrs.c
            src/unix/openbsd.c
            src/unix/kqueue.c
            src/unix/posix-hrtime.c
            )
        set(UV_LIBS m pthread kvm)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD")
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/bsd-ifaddrs.c
            src/unix/netbsd.c
            src/unix/kqueue.c
            src/unix/posix-hrtime.c
            )
        set(UV_LIBS m pthread kvm)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "CYGWIN")
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/bsd-ifaddrs.c
            src/unix/cygwin.c
            src/unix/no-fsevents.c
            src/unix/no-proctitle.c
            src/unix/posix-hrtime.c
            src/unix/posix-poll.c
            src/unix/procfs-exepath.c
            src/unix/sysinfo-loadavg.c
            src/unix/sysinfo-memory.c
            )
        set(UV_LIBS rt m dl)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "ANDROID")
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            include/android-ifaddrs.h
            include/pthread-fixes.h
            include/pthread-barrier.h
            src/unix/android-ifaddrs.c
            src/unix/procfs-exepath.c
            src/unix/pthread-fixes.c
            src/unix/sysinfo-loadavg.c
            src/unix/sysinfo-memory.c
            )
        set(UV_LIBS rt m dl)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS|Solaris")
        add_definitions(-D__EXTENSIONS__ -D_XOPEN_SOURCE=500)
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/no-proctitle.c
            src/unix/sunos.c
            )
        set(UV_LIBS kstat nsl sendfile socket)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "AIX")
        add_definitions(-D_ALL_SOURCE -D_XOPEN_SOURCE=500 -D_LINUX_SOURCE_COMPAT -D_THREAD_SAFE -DHAVE_SYS_AHAFS_EVPRODS_H)
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/aix.c
            src/unix/aix-common.c
            )
        set(UV_LIBS perfstat)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "ZOS")
        add_definitions(-D_UNIX03_THREADS -D_UNIX03_SOURCE -D_UNIX03_WITHDRAWN -D_OPEN_SYS_IF_EXT -D_OPEN_SYS_SOCK_IPV6 -D_OPEN_MSGQ_EXT -D_XOPEN_SOURCE_EXTENDED -D_ALL_SOURCE -D_LARGE_TIME_API -D_OPEN_SYS_FILE_EXT -D_AE_BIMODAL -DPATH_MAX=255)
        set(UV_SRCS
            ${UV_POSIX_SRCS}
            ${UV_COMMON_SRCS}
            src/unix/pthread-fixes.c
            src/unix/no-fsevents.c
            src/unix/os390.c
            src/unix/os390-syscalls.c
            src/unix/os390-syscalls.h
            )
    else()
        message(FATAL_ERROR
            "We don't support this system for now: ${CMAKE_SYSTEM_NAME}")
    endif()
endif()

add_library(uv-static ${UV_SRCS})
target_link_libraries(uv-static ${UV_LIBS})
