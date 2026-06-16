#ifndef NURL_H
#define NURL_H

#ifndef NURL_VERSION
#define NURL_VERSION "0.2.2"
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    // Auth
    char *user;
    char *bearer;
    char *token;
    bool no_auth;

    // Body
    char *data;
    size_t data_len;
    bool json;

    // TLS
    bool no_verify;
    char *cacert;
    char *cert;
    char *key;
    bool tls12;
    bool tls13;

    // Cookies
    char *cookie;
    char *cookie_jar;
    char *session;

    // Request Control
    char *method;
    unsigned long timeout;
    unsigned long connect_timeout;
    bool location;
    unsigned int max_redirects;
    char *user_agent;
    char *referer;
    char **header;
    size_t header_count;
    bool compressed;
    char *proxy;
    char *proxy_user;
    char *no_proxy;
    unsigned int retry;
    unsigned long retry_delay;

    // Output
    char *output;
    bool include;
    bool verbose;
    bool silent;
    char *write_out;
    bool fail;
    bool raw;

    // Ping specific
    unsigned int ping_count;
    unsigned long ping_interval;

    // Download & Upload specific
    bool resume;
    bool progress;
    char *upload_file;
    char *upload_name;
    char *upload_mime;
    char **upload_fields;
    size_t upload_fields_count;
} CommonArgs;

#include "errors/nurl_error.h"

#endif /* NURL_H */
