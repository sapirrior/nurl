#include "nurl_request.h"
#include "nurl_net.h"
#include "nurl_tls.h"
#include "nurl_utils.h"
#include "nurl_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>

static bool has_header(char **headers, size_t count, const char *key) {
    size_t key_len = strlen(key);
    for (size_t i = 0; i < count; i++) {
        if (strncasecmp(headers[i], key, key_len) == 0 && headers[i][key_len] == ':') {
            return true;
        }
    }
    return false;
}

static char *resolve_redirect(const char *current_url, const char *location) {
    if (strstr(location, "://")) {
        return strdup(location);
    }

    char *scheme = NULL;
    char *host = NULL;
    char *path = NULL;
    int port = 0;

    if (nurl_utils_parse_url(current_url, &scheme, &host, &port, &path) != 0) {
        return strdup(location);
    }

    char *resolved = NULL;
    int ret = -1;
    if (location[0] == '/') {
        if (port == 80 || port == 443) {
            ret = asprintf(&resolved, "%s://%s%s", scheme, host, location);
        } else {
            ret = asprintf(&resolved, "%s://%s:%d%s", scheme, host, port, location);
        }
    } else {
        char *last_slash = strrchr(path, '/');
        if (last_slash) {
            *last_slash = '\0';
        }
        if (port == 80 || port == 443) {
            ret = asprintf(&resolved, "%s://%s%s/%s", scheme, host, path, location);
        } else {
            ret = asprintf(&resolved, "%s://%s:%d%s/%s", scheme, host, port, path, location);
        }
    }

    if (ret < 0) {
        resolved = NULL;
    }

    free(scheme);
    free(host);
    free(path);

    return resolved;
}

static bool append_hdr_str(char **buf, size_t *len, size_t *cap, const char *fmt, const char *val) {
    size_t needed = strlen(fmt) + (val ? strlen(val) : 0) + 16;
    if (*len + needed >= *cap) {
        *cap = (*cap + needed) * 2;
        char *temp = realloc(*buf, *cap);
        if (!temp) {
            return false;
        }
        *buf = temp;
    }
    int written = snprintf(*buf + *len, *cap - *len, fmt, val);
    if (written < 0) {
        return false;
    }
    *len += written;
    return true;
}

int nurl_request_generic(const char *method, const char *url, const CommonArgs *common) {
    char *current_url = strdup(url);
    int redirects_followed = 0;
    const int max_redirects = common->max_redirects > 0 ? common->max_redirects : 5;

    nurl_http_response_t *res = NULL;

    while (true) {
        char *scheme = NULL;
        char *host = NULL;
        char *path = NULL;
        int port = 0;

        if (nurl_utils_parse_url(current_url, &scheme, &host, &port, &path) != 0) {
            fprintf(stderr, "Error: Invalid URL format: %s\n", current_url);
            free(current_url);
            return NURL_ERR_INVALID_URL;
        }

        bool use_tls = (strcmp(scheme, "https") == 0);
        if (!use_tls) {
            fprintf(stderr, "Error: nurl currently only supports HTTPS (TLS) requests.\n");
            free(scheme); free(host); free(path); free(current_url);
            return NURL_ERR_TLS;
        }

        int sock_fd = nurl_net_connect(host, port);
        if (sock_fd < 0) {
            fprintf(stderr, "Error: Could not connect to host %s:%d\n", host, port);
            free(scheme); free(host); free(path); free(current_url);
            return NURL_ERR_NETWORK;
        }

        if (common->timeout > 0) {
            nurl_net_set_timeout(sock_fd, common->timeout);
        }

        nurl_tls_t *tls = nurl_tls_create(!common->no_verify, common->cacert);
        if (!tls) {
            fprintf(stderr, "Error: Failed to initialize TLS context.\n");
            nurl_net_close(sock_fd);
            free(scheme); free(host); free(path); free(current_url);
            return NURL_ERR_TLS;
        }

        if (nurl_tls_handshake(tls, sock_fd, host) != 0) {
            fprintf(stderr, "Error: TLS verification failed.\n");
            nurl_tls_free(tls);
            nurl_net_close(sock_fd);
            free(scheme); free(host); free(path); free(current_url);
            return NURL_ERR_TLS;
        }

        size_t extra_hdr_capacity = 1024;
        char *extra_hdr = malloc(extra_hdr_capacity);
        if (!extra_hdr) {
            fprintf(stderr, "Error: Out of memory.\n");
            nurl_tls_free(tls);
            nurl_net_close(sock_fd);
            free(scheme); free(host); free(path); free(current_url);
            return NURL_ERR_GENERIC;
        }
        extra_hdr[0] = '\0';
        size_t extra_hdr_len = 0;
        bool oom = false;

        for (size_t i = 0; i < common->header_count; i++) {
            if (!append_hdr_str(&extra_hdr, &extra_hdr_len, &extra_hdr_capacity, "%s\r\n", common->header[i])) {
                oom = true;
                break;
            }
        }

        if (!oom && !common->no_auth) {
            if (common->bearer || common->token) {
                const char *tok = common->bearer ? common->bearer : common->token;
                if (!has_header(common->header, common->header_count, "Authorization")) {
                    if (!append_hdr_str(&extra_hdr, &extra_hdr_len, &extra_hdr_capacity, "Authorization: Bearer %s\r\n", tok)) {
                        oom = true;
                    }
                }
            } else if (common->user) {
                if (!has_header(common->header, common->header_count, "Authorization")) {
                    char *b64 = nurl_utils_base64_encode((const unsigned char *)common->user, strlen(common->user));
                    if (b64) {
                        if (!append_hdr_str(&extra_hdr, &extra_hdr_len, &extra_hdr_capacity, "Authorization: Basic %s\r\n", b64)) {
                            oom = true;
                        }
                        free(b64);
                    } else {
                        oom = true;
                    }
                }
            }
        }

        if (!oom && common->json && !has_header(common->header, common->header_count, "Content-Type")) {
            if (!append_hdr_str(&extra_hdr, &extra_hdr_len, &extra_hdr_capacity, "%s", "Content-Type: application/json\r\n")) {
                oom = true;
            }
        }

        if (oom) {
            fprintf(stderr, "Error: Out of memory preparing headers.\n");
            free(extra_hdr);
            nurl_tls_free(tls);
            nurl_net_close(sock_fd);
            free(scheme); free(host); free(path); free(current_url);
            return NURL_ERR_GENERIC;
        }

        if (common->verbose && !common->silent) {
            fprintf(stderr, "* Connected to %s port %d\n", host, port);
            fprintf(stderr, "* TLS handshake complete\n\n");
            fprintf(stderr, "> %s %s HTTP/1.1\n", method, path);
            fprintf(stderr, "> Host: %s\n", host);
            fprintf(stderr, "> User-Agent: nurl/0.1.1\n");
            fprintf(stderr, "> Connection: close\n");

            char *hdr_copy = strdup(extra_hdr);
            char *line = strtok(hdr_copy, "\r\n");
            while (line) {
                char *colon = strchr(line, ':');
                if (colon) {
                    *colon = '\0';
                    char *key = line;
                    char *val = colon + 1;
                    while (*val && isspace((unsigned char)*val)) val++;
                    const char *redacted = nurl_utils_redact_header(key, val);
                    fprintf(stderr, "> %s: %s\n", key, redacted);
                }
                line = strtok(NULL, "\r\n");
            }
            free(hdr_copy);
            fprintf(stderr, "\n");
        }

        const unsigned char *body_data = (const unsigned char *)common->data;
        size_t body_len = common->data ? strlen(common->data) : 0;

        res = nurl_http_request(tls, method, path, host, extra_hdr, body_data, body_len);
        free(extra_hdr);

        if (!res) {
            fprintf(stderr, "Error: HTTP request failed or timed out.\n");
            nurl_tls_free(tls);
            nurl_net_close(sock_fd);
            free(scheme); free(host); free(path); free(current_url);
            return NURL_ERR_NETWORK;
        }

        if (common->verbose && !common->silent) {
            fprintf(stderr, "< HTTP/1.1 %d %s\n", res->status_code, res->status_text);
            for (size_t i = 0; i < res->header_count; i++) {
                fprintf(stderr, "< %s\n", res->headers[i]);
            }
            fprintf(stderr, "\n");
        }

        if (common->location && res->status_code >= 300 && res->status_code < 400) {
            char *redir_url = NULL;
            for (size_t i = 0; i < res->header_count; i++) {
                if (strncasecmp(res->headers[i], "Location:", 9) == 0) {
                    char *val = res->headers[i] + 9;
                    while (*val && isspace((unsigned char)*val)) val++;
                    redir_url = resolve_redirect(current_url, val);
                    break;
                }
            }

            if (redir_url) {
                nurl_http_response_free(res);
                nurl_tls_free(tls);
                nurl_net_close(sock_fd);
                free(scheme); free(host); free(path);

                free(current_url);
                current_url = redir_url;
                redirects_followed++;

                if (redirects_followed >= max_redirects) {
                    fprintf(stderr, "Error: Maximum redirect limit exceeded (%d).\n", max_redirects);
                    free(current_url);
                    return NURL_ERR_GENERIC;
                }
                continue;
            }
        }

        nurl_tls_free(tls);
        nurl_net_close(sock_fd);
        free(scheme); free(host); free(path);
        break;
    }

    free(current_url);

    if (!common->silent) {
        if (common->include && !common->verbose) {
            printf("HTTP/1.1 %d %s\n", res->status_code, res->status_text);
            for (size_t i = 0; i < res->header_count; i++) {
                printf("%s\n", res->headers[i]);
            }
            printf("\n");
        }

        if (common->output) {
            FILE *f = fopen(common->output, "wb");
            if (!f) {
                fprintf(stderr, "Error: Could not open file for writing: %s\n", common->output);
                nurl_http_response_free(res);
                return NURL_ERR_WRITE;
            }
            if (res->body_len > 0) {
                fwrite(res->body, 1, res->body_len, f);
            }
            fclose(f);
            if (!common->silent) {
                fprintf(stderr, "+ Saved response to %s (%zu bytes)\n", common->output, res->body_len);
            }
        } else {
            if (res->body_len > 0) {
                printf("%s\n", res->body);
            }
        }
    }

    int ret_code = NURL_OK;
    if (res->status_code >= 500) {
        ret_code = NURL_ERR_STATUS_5XX;
    } else if (res->status_code >= 400) {
        ret_code = NURL_ERR_STATUS_4XX;
    }

    nurl_http_response_free(res);
    return ret_code;
}
