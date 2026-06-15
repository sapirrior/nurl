#include "nurl_async.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #include <winsock2.h>
  #define poll_fn WSAPoll
#else
  #include <poll.h>
  #include <unistd.h>
  #define poll_fn poll
#endif

#define MAX_WATCHERS 1024

typedef struct {
    int fd;
    NurlEventCallback cb;
    void *ctx;
} NurlEventWatcher;

struct NurlEventLoop {
#if defined(_WIN32)
    SOCKET fds_raw[MAX_WATCHERS]; // WSAPoll expects SOCKET on Windows
    WSAPOLLFD fds[MAX_WATCHERS];
#else
    struct pollfd fds[MAX_WATCHERS];
#endif
    NurlEventWatcher watchers[MAX_WATCHERS];
    int watcher_count;
};

NurlEventLoop *nurl_event_loop_create(void) {
    NurlEventLoop *loop = calloc(1, sizeof(NurlEventLoop));
    if (!loop) return NULL;

    for (int i = 0; i < MAX_WATCHERS; i++) {
#if defined(_WIN32)
        loop->fds[i].fd = INVALID_SOCKET;
#else
        loop->fds[i].fd = -1;
#endif
        loop->watchers[i].fd = -1;
    }
    return loop;
}

void nurl_event_loop_destroy(NurlEventLoop *loop) {
    free(loop);
}

int nurl_event_loop_add(NurlEventLoop *loop, int fd, uint32_t events, NurlEventCallback cb, void *ctx) {
    if (!loop || fd < 0) return -1;

    // Find a free slot
    int slot = -1;
    for (int i = 0; i < MAX_WATCHERS; i++) {
#if defined(_WIN32)
        if (loop->fds[i].fd == INVALID_SOCKET) {
#else
        if (loop->fds[i].fd < 0) {
#endif
            slot = i;
            break;
        }
    }

    if (slot < 0) return -1; // Pool full

    loop->fds[slot].fd = fd;
    loop->fds[slot].events = (short)events;
    loop->fds[slot].revents = 0;

    loop->watchers[slot].fd = fd;
    loop->watchers[slot].cb = cb;
    loop->watchers[slot].ctx = ctx;
    loop->watcher_count++;
    return 0;
}

int nurl_event_loop_mod(NurlEventLoop *loop, int fd, uint32_t events, NurlEventCallback cb, void *ctx) {
    if (!loop || fd < 0) return -1;

    for (int i = 0; i < MAX_WATCHERS; i++) {
        if (loop->watchers[i].fd == fd) {
            loop->fds[i].events = (short)events;
            loop->watchers[i].cb = cb;
            loop->watchers[i].ctx = ctx;
            return 0;
        }
    }
    return -1;
}

int nurl_event_loop_del(NurlEventLoop *loop, int fd) {
    if (!loop || fd < 0) return -1;

    for (int i = 0; i < MAX_WATCHERS; i++) {
        if (loop->watchers[i].fd == fd) {
#if defined(_WIN32)
            loop->fds[i].fd = INVALID_SOCKET;
#else
            loop->fds[i].fd = -1;
#endif
            loop->fds[i].events = 0;
            loop->fds[i].revents = 0;

            loop->watchers[i].fd = -1;
            loop->watchers[i].cb = NULL;
            loop->watchers[i].ctx = NULL;
            loop->watcher_count--;
            return 0;
        }
    }
    return -1;
}

int nurl_event_loop_run(NurlEventLoop *loop, int timeout_ms) {
    if (!loop) return -1;
    if (loop->watcher_count == 0) return 0;

    int ret = poll_fn(loop->fds, MAX_WATCHERS, timeout_ms);
    if (ret < 0) {
        return -1;
    }

    if (ret > 0) {
        for (int i = 0; i < MAX_WATCHERS; i++) {
#if defined(_WIN32)
            if (loop->fds[i].fd != INVALID_SOCKET && loop->fds[i].revents != 0) {
#else
            if (loop->fds[i].fd >= 0 && loop->fds[i].revents != 0) {
#endif
                NurlEventCallback cb = loop->watchers[i].cb;
                void *ctx = loop->watchers[i].ctx;
                if (cb) {
                    cb(loop, loop->watchers[i].fd, loop->fds[i].revents, ctx);
                }
            }
        }
    }
    return loop->watcher_count;
}
