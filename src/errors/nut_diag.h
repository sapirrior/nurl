#ifndef NUT_DIAG_H
#define NUT_DIAG_H

/**
 * Emits a standardized error message to stderr in Unix style.
 * nut: error: <msg>
 */
void nut_diag_err(const char *fmt, ...);

/**
 * Emits a standardized hint message to stderr.
 *       hint: <msg>
 */
void nut_diag_hint(const char *fmt, ...);

/**
 * Emits a standardized warning message to stderr.
 * nut: warning: <msg>
 */
void nut_diag_warn(const char *fmt, ...);

/**
 * Safely emits an out-of-memory error using low-level I/O.
 */
void nut_diag_oom(void);

#endif /* NUT_DIAG_H */
