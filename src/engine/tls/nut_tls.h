#ifndef NUT_TLS_H
#define NUT_TLS_H

#include <stdbool.h>

typedef struct nut_tls nut_tls_t;

/**
 * Initializes OpenSSL and returns a new nut_tls_t context.
 * verify_certs: true to enforce certificate verification, false to ignore (--no-verify).
 * cacert_path: Path to custom root CA certificates, or NULL for system defaults.
 * cert_path: Path to client certificate PEM file, or NULL.
 * key_path: Path to client private key file, or NULL.
 */
nut_tls_t *nut_tls_create(bool verify_certs, const char *cacert_path, const char *cert_path, const char *key_path, bool force_tls12, bool force_tls13);

/**
 * Performs a TLS handshake over the existing TCP socket.
 * hostname is used for SNI and certificate verification.
 * Returns 0 on success, negative value on error.
 */
int nut_tls_handshake(nut_tls_t *tls, int socket_fd, const char *hostname);

/**
 * Reads decrypted data from the secure channel.
 * Returns bytes read, or <= 0 on error.
 */
int nut_tls_read(nut_tls_t *tls, void *buf, int len);

/**
 * Writes raw data to the secure channel (it will be encrypted).
 * Returns bytes written, or <= 0 on error.
 */
int nut_tls_write(nut_tls_t *tls, const void *buf, int len);

/**
 * Shuts down the TLS session.
 */
void nut_tls_close(nut_tls_t *tls);

/**
 * Frees all TLS context and OpenSSL resources.
 */
void nut_tls_free(nut_tls_t *tls);

/**
 * Returns the ALPN negotiated protocol, e.g. "h2" or "http/1.1", or NULL.
 */
const char *nut_tls_get_negotiated_proto(nut_tls_t *tls);

/**
 * Returns the last detailed TLS error message, or NULL.
 */
const char *nut_tls_last_error(nut_tls_t *tls);

#endif /* NUT_TLS_H */
