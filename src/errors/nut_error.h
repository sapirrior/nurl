#ifndef NUT_ERROR_H
#define NUT_ERROR_H

typedef enum {
    NUT_OK            = 0,
    NUT_ERR_OOM       = 1,   /* malloc/realloc returned NULL */
    NUT_ERR_NETWORK   = 2,   /* TCP connect/recv/send failed */
    NUT_ERR_URL       = 4,   /* Malformed or unsupported URL */
    NUT_ERR_TLS       = 5,   /* TLS handshake or cert error */
    NUT_ERR_IO        = 6,   /* File read/write failed */
    NUT_ERR_RESOLVE   = 7,   /* Hostname resolution failed */
    NUT_ERR_CONNECT   = 8,   /* TCP connection failed */
    NUT_ERR_PROXY     = 9,   /* Proxy connection/handshake failed */
    NUT_ERR_TLS_HANDSHAKE = 10, /* Specific TLS handshake failure */
    NUT_ERR_TIMEOUT   = 28,  /* curl exit 28: operation timed out */
    NUT_ERR_HTTP_4XX  = 22,  /* curl exit 22: HTTP 4xx response with -f/--fail */
    NUT_ERR_HTTP_5XX  = 23,  /* mapped to 22 on exit */
    NUT_ERR_ARG       = 3,   /* curl exit 3: Bad CLI argument */
    NUT_ERR_GENERIC   = 99,
} nut_err_t;
#endif /* NUT_ERROR_H */
