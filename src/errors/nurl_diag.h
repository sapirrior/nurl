#ifndef NURL_DIAG_H
#define NURL_DIAG_H

/**
 * Emits a standardized diagnostic block to stderr.
 * block_type: The tag to display (e.g., "Error", "Hint", "Suggestion").
 * fmt: The formatted message.
 */
void nurl_diag_block(const char *block_type, const char *fmt, ...);

#endif /* NURL_DIAG_H */
