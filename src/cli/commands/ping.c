#include "commands.h"
#include "nurl_net.h"
#include "nurl_tls.h"
#include "nurl_utils.h"
#include "nurl_http.h"
#include "engine/net/nurl_stream.h"
#include "errors/nurl_error_handler.h"
#include "errors/nurl_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static NurlStream *connect_and_handshake(const char *host, int port, const CommonArgs *common) {
    nurl_err_t err = NURL_OK;
    int fd = nurl_net_connect_proxy_ex(host, port, common->proxy, common->proxy_user, common->no_proxy, common->connect_timeout, &err);
    if (fd < 0) return NULL;

    nurl_tls_t *t = nurl_tls_create(!common->no_verify, common->cacert, common->cert, common->key, common->tls12, common->tls13);
    if (!t) { nurl_net_close(fd); return NULL; }

    if (nurl_tls_handshake(t, fd, host) != 0) { nurl_tls_free(t); nurl_net_close(fd); return NULL; }
    return nurl_stream_new(fd, t);
}

int nurl_cmd_ping(const char *url, const CommonArgs *common) {
    char *scheme = NULL, *host = NULL, *path = NULL;
    int port = 0;
    if (nurl_utils_parse_url(url, &scheme, &host, &port, &path) != 0) return NURL_ERR_INVALID_URL;

    unsigned int count = common->ping_count > 0 ? common->ping_count : 1;
    unsigned long interval = common->ping_interval > 0 ? common->ping_interval : 1000;

    NurlStream *stream = connect_and_handshake(host, port, common);
    if (!stream) {
        nurl_diag_err("initial TLS connection failed for '%s'.", host);
        free(scheme); free(host); free(path);
        return NURL_ERR_TLS;
    }

    if (common->verbose && !common->silent) fprintf(stderr, "* Connected to %s:%d (TLS warm)\n", host, port);

    unsigned long *latencies = malloc(sizeof(unsigned long) * count);
    if (!latencies) { 
        int fd = stream->fd; nurl_tls_t *tls = stream->tls;
        nurl_stream_free(stream); nurl_tls_free(tls); nurl_net_close(fd);
        free(scheme); free(host); free(path); return NURL_ERR_OOM; 
    }

    for (unsigned int i = 0; i < count; i++) {
        struct timeval start, end;
        gettimeofday(&start, NULL);

        nurl_http_response_t *res = NULL;
        nurl_err_t err = nurl_http_request(stream, "HEAD", path, host, NULL, NULL, 0, NULL, 0, NULL, false, true, 0, NULL, NULL, &res);

        if (err != NURL_OK) {
            fprintf(stderr, "ping: request %u failed (error %d)\n", i + 1, err);
            latencies[i] = 0;
        } else {
            gettimeofday(&end, NULL);
            unsigned long diff = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
            latencies[i] = diff;
            if (!common->silent) printf("ping %s: seq=%u status=%d time=%lu ms\n", host, i + 1, res->status_code, diff);
            nurl_http_response_free(res);
        }

        if (i < count - 1) usleep(interval * 1000);
    }

    unsigned long min = 999999, max = 0, avg = 0, sum = 0, success = 0;
    for (unsigned int i = 0; i < count; i++) {
        if (latencies[i] > 0) {
            if (latencies[i] < min) min = latencies[i];
            if (latencies[i] > max) max = latencies[i];
            sum += latencies[i];
            success++;
        }
    }
    if (success > 0) {
        avg = sum / success;
        if (!common->silent) printf("\n--- %s ping statistics ---\n%u requests, %u success, %u%% packet loss\nround-trip min/avg/max = %lu/%lu/%lu ms\n", host, count, (unsigned int)success, (unsigned int)((count - success) * 100 / count), min, avg, max);
    } else if (!common->silent) printf("\n--- %s ping statistics ---\n%u requests, 0 success, 100%% packet loss\n", host, count);

    free(latencies);
    int fd = stream->fd; nurl_tls_t *tls = stream->tls;
    nurl_stream_free(stream); nurl_tls_free(tls); nurl_net_close(fd);
    free(scheme); free(host); free(path);
    return 0;
}
