#ifndef NUT_CLI_H
#define NUT_CLI_H

#include "nut.h"

/**
 * Initializes CommonArgs with default values.
 */
void nut_cli_init_args(CommonArgs *args);

/**
 * Parses the CLI arguments into CommonArgs, routing command name and target URL.
 * Returns 0 on success, non-zero on parsing error or help request.
 */
int nut_cli_parse(int argc, char **argv, CommonArgs *args, char **url);

/**
 * Frees all dynamically allocated strings/arrays within CommonArgs.
 */
void nut_cli_free_args(CommonArgs *args);

#endif /* NUT_CLI_H */
