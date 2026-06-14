#include "nurl_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <stdint.h>

int nurl_utils_parse_url(const char *url, char **scheme, char **host, int *port, char **path) {
    if (!url || strlen(url) == 0) {
        return -1;
    }

    const char *start = url;
    const char *scheme_end = strstr(url, "://");
    char *parsed_scheme = NULL;

    if (scheme_end) {
        size_t scheme_len = scheme_end - url;
        parsed_scheme = malloc(scheme_len + 1);
        if (!parsed_scheme) return -1;
        strncpy(parsed_scheme, url, scheme_len);
        parsed_scheme[scheme_len] = '\0';
        
        // Convert to lowercase
        for (size_t i = 0; i < scheme_len; i++) {
            parsed_scheme[i] = tolower((unsigned char)parsed_scheme[i]);
        }
        start = scheme_end + 3;
    } else {
        parsed_scheme = strdup("https");
        if (!parsed_scheme) return -1;
    }

    // Find start of the path component (first '/')
    const char *path_start = strchr(start, '/');
    size_t host_port_len = path_start ? (size_t)(path_start - start) : strlen(start);

    char *host_port = malloc(host_port_len + 1);
    if (!host_port) {
        free(parsed_scheme);
        return -1;
    }
    strncpy(host_port, start, host_port_len);
    host_port[host_port_len] = '\0';

    // Parse port if specified
    char *colon = strchr(host_port, ':');
    char *parsed_host = NULL;
    int parsed_port = -1;

    if (colon) {
        *colon = '\0';
        parsed_host = strdup(host_port);
        parsed_port = atoi(colon + 1);
    } else {
        parsed_host = strdup(host_port);
        if (strcmp(parsed_scheme, "http") == 0) {
            parsed_port = 80;
        } else {
            parsed_port = 443;
        }
    }
    free(host_port);

    if (!parsed_host) {
        free(parsed_scheme);
        return -1;
    }

    char *parsed_path = NULL;
    if (path_start) {
        parsed_path = strdup(path_start);
    } else {
        parsed_path = strdup("/");
    }

    if (!parsed_path) {
        free(parsed_scheme);
        free(parsed_host);
        return -1;
    }

    *scheme = parsed_scheme;
    *host = parsed_host;
    *port = parsed_port;
    *path = parsed_path;

    return 0;
}

const char *nurl_utils_redact_header(const char *key, const char *value) {
    if (!key) return value;
    if (strcasecmp(key, "authorization") == 0 || strcasecmp(key, "cookie") == 0) {
        return "[hidden]";
    }
    return value;
}

char *nurl_utils_trim(char *str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == '\0') {
        return str;
    }
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    return str;
}

char *nurl_utils_base64_encode(const unsigned char *src, size_t len) {
    static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out_len = 4 * ((len + 2) / 3);
    char *out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        uint32_t octet_a = i < len ? src[i++] : 0;
        uint32_t octet_b = i < len ? src[i++] : 0;
        uint32_t octet_c = i < len ? src[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        out[j++] = b64_table[(triple >> 18) & 0x3F];
        out[j++] = b64_table[(triple >> 12) & 0x3F];
        out[j++] = (i > len + 1) ? '=' : b64_table[(triple >> 6) & 0x3F];
        out[j++] = (i > len) ? '=' : b64_table[triple & 0x3F];
    }
    out[out_len] = '\0';
    return out;
}
