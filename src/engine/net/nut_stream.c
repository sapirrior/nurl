#include "nut_stream.h"
#include "utils/nut_utils.h"
#include "errors/nut_error.h"
#include "compat/nut_compat.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#define socket_read(fd, buf, len) recv(fd, (char *)(buf), (int)(len), 0)
#define socket_write(fd, buf, len) send(fd, (const char *)(buf), (int)(len), 0)
#else
#define socket_read(fd, buf, len) read(fd, buf, len)
#define socket_write(fd, buf, len) write(fd, buf, len)
#endif

NutStream *nut_stream_new(int fd, nut_tls_t *tls) {
    NutStream *s = calloc(1, sizeof(NutStream));
    if (!s) return NULL;
    s->fd = fd;
    s->tls = tls;
    return s;
}

void nut_stream_set_limit_rate(NutStream *s, unsigned long rate) {
    if (s) s->limit_rate = rate;
}

void nut_stream_free(NutStream *s) {
    free(s);
}

static void throttle(NutStream *s, size_t bytes) {
    if (!s || s->limit_rate == 0 || bytes == 0) return;

    double now = nut_utils_get_time_sec();
    if (s->last_throttle_time == 0) {
        s->last_throttle_time = now;
        s->bytes_this_sec = 0;
    }

    s->bytes_this_sec += bytes;

    if (now - s->last_throttle_time >= 1.0) {
        s->last_throttle_time = now;
        s->bytes_this_sec = 0;
    }

    if (s->bytes_this_sec >= s->limit_rate) {
        double elapsed = now - s->last_throttle_time;
        if (elapsed < 1.0) {
            nut_sleep_ms((unsigned long)((1.0 - elapsed) * 1000));
        }
        s->last_throttle_time = nut_utils_get_time_sec();
        s->bytes_this_sec = 0;
    }
}

static int fill_buffer(NutStream *s) {
    if (s->read_buf.pos < s->read_buf.len) {
        return (int)(s->read_buf.len - s->read_buf.pos);
    }

    size_t to_read = NUT_STREAM_BUFFER_SIZE;
    if (s->limit_rate > 0) {
        size_t remaining_this_sec = s->limit_rate > s->bytes_this_sec ? s->limit_rate - s->bytes_this_sec : 1;
        if (to_read > remaining_this_sec) to_read = remaining_this_sec;
    }

    int n;
    if (s->tls) {
        n = nut_tls_read(s->tls, s->read_buf.data, (int)to_read);
    } else {
        n = socket_read(s->fd, s->read_buf.data, to_read);
    }

    if (n > 0) {
        s->read_buf.pos = 0;
        s->read_buf.len = (size_t)n;
        throttle(s, (size_t)n);
    } else if (n < 0) {
        s->read_buf.pos = 0;
        s->read_buf.len = 0;
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAETIMEDOUT) return -NUT_ERR_TIMEOUT;
#else
        if (errno == ETIMEDOUT || errno == EAGAIN || errno == EWOULDBLOCK) return -NUT_ERR_TIMEOUT;
#endif
        return -NUT_ERR_NETWORK;
    } else {
        s->read_buf.pos = 0;
        s->read_buf.len = 0;
    }
    return n;
}

int nut_stream_read(NutStream *s, void *buf, size_t len) {
    if (len == 0) return 0;

    // If buffer is empty, try to fill it
    if (s->read_buf.pos >= s->read_buf.len) {
        int n = fill_buffer(s);
        if (n <= 0) return n;
    }

    size_t avail = s->read_buf.len - s->read_buf.pos;
    size_t to_copy = len < avail ? len : avail;
    memcpy(buf, s->read_buf.data + s->read_buf.pos, to_copy);
    s->read_buf.pos += to_copy;

    return (int)to_copy;
}

int nut_stream_read_exact(NutStream *s, void *buf, size_t len) {
    size_t total_read = 0;
    unsigned char *p = (unsigned char *)buf;

    while (total_read < len) {
        int n = nut_stream_read(s, p + total_read, len - total_read);
        if (n <= 0) return n;
        total_read += n;
    }
    return (int)total_read;
}

int nut_stream_read_line(NutStream *s, char *buf, size_t max_len) {
    if (max_len <= 1) return 0;

    size_t total_read = 0;
    while (total_read < max_len - 1) {
        if (s->read_buf.pos >= s->read_buf.len) {
            int n = fill_buffer(s);
            if (n <= 0) {
                if (total_read > 0) break; // Return what we have
                return n;
            }
        }

        unsigned char c = s->read_buf.data[s->read_buf.pos++];
        buf[total_read++] = (char)c;
        if (c == '\n') break;
    }
    buf[total_read] = '\0';
    return (int)total_read;
}

int nut_stream_write(NutStream *s, const void *buf, size_t len) {
    size_t to_write = len;
    if (s->limit_rate > 0) {
        size_t remaining_this_sec = s->limit_rate > s->bytes_this_sec ? s->limit_rate - s->bytes_this_sec : 1;
        if (to_write > remaining_this_sec) to_write = remaining_this_sec;
    }

    int n;
    if (s->tls) {
        n = nut_tls_write(s->tls, buf, (int)to_write);
    } else {
        n = socket_write(s->fd, buf, to_write);
    }

    if (n > 0) {
        throttle(s, (size_t)n);
    } else if (n < 0) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAETIMEDOUT) return -NUT_ERR_TIMEOUT;
#else
        if (errno == ETIMEDOUT || errno == EAGAIN || errno == EWOULDBLOCK) return -NUT_ERR_TIMEOUT;
#endif
        return -NUT_ERR_NETWORK;
    }
    return n;
}

bool nut_stream_has_buffered(const NutStream *s) {
    return s->read_buf.pos < s->read_buf.len;
}
