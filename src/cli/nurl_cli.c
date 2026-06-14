#include "nurl_cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <strings.h>

void nurl_cli_init_args(CommonArgs *args) {
    memset(args, 0, sizeof(CommonArgs));
    args->timeout = 30; // Default timeout 30s
    args->connect_timeout = 10;
    args->ping_count = 1;
    args->ping_interval = 1000;
}

static bool is_subcommand(const char *arg) {
    const char *commands[] = {
        "get", "post", "put", "delete", "head", "patch", "options",
        "download", "upload", "inspect", "ping", "resolve"
    };
    size_t count = sizeof(commands) / sizeof(commands[0]);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(arg, commands[i]) == 0) {
            return true;
        }
    }
    return false;
}

int nurl_cli_parse(int argc, char **argv, CommonArgs *args, char **command, char **url) {
    nurl_cli_init_args(args);

    static struct option long_options[] = {
        {"user",            required_argument, NULL, 'u'},
        {"bearer",          required_argument, NULL, 1},
        {"token",           required_argument, NULL, 2},
        {"no-auth",         no_argument,       NULL, 3},
        {"data",            required_argument, NULL, 'd'},
        {"json",            no_argument,       NULL, 'j'},
        {"no-verify",       no_argument,       NULL, 'k'},
        {"cacert",          required_argument, NULL, 4},
        {"timeout",         required_argument, NULL, 't'},
        {"location",        no_argument,       NULL, 'L'},
        {"header",          required_argument, NULL, 'H'},
        {"output",          required_argument, NULL, 'o'},
        {"include",         no_argument,       NULL, 'i'},
        {"verbose",         no_argument,       NULL, 'v'},
        {"silent",          no_argument,       NULL, 's'},
        {"raw",             no_argument,       NULL, 5},
        {"count",           required_argument, NULL, 6},
        {"interval",        required_argument, NULL, 7},
        {"help",            no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    opterr = 0; // Disable default getopt error printing

    while ((opt = getopt_long(argc, argv, "u:d:jt:LH:o:ivshk", long_options, NULL)) != -1) {
        switch (opt) {
            case 'u':
                if (args->user) free(args->user);
                args->user = strdup(optarg);
                if (!args->user) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                break;
            case 1:
                if (args->bearer) free(args->bearer);
                args->bearer = strdup(optarg);
                if (!args->bearer) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                break;
            case 2:
                if (args->token) free(args->token);
                args->token = strdup(optarg);
                if (!args->token) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                break;
            case 3:
                args->no_auth = true;
                break;
            case 'd':
                if (args->data) free(args->data);
                args->data = strdup(optarg);
                if (!args->data) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                break;
            case 'j':
                args->json = true;
                break;
            case 'k':
                args->no_verify = true;
                break;
            case 4:
                if (args->cacert) free(args->cacert);
                args->cacert = strdup(optarg);
                if (!args->cacert) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                break;
            case 't':
                args->timeout = strtoul(optarg, NULL, 10);
                break;
            case 'L':
                args->location = true;
                break;
            case 'H': {
                char **temp = realloc(args->header, sizeof(char *) * (args->header_count + 1));
                if (!temp) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                args->header = temp;
                args->header[args->header_count] = strdup(optarg);
                if (!args->header[args->header_count]) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                args->header_count++;
                break;
            }
            case 'o':
                if (args->output) free(args->output);
                args->output = strdup(optarg);
                if (!args->output) {
                    fprintf(stderr, "Error: Out of memory.\n");
                    return -1;
                }
                break;
            case 'i':
                args->include = true;
                break;
            case 'v':
                args->verbose = true;
                break;
            case 's':
                args->silent = true;
                break;
            case 5:
                args->raw = true;
                break;
            case 6:
                args->ping_count = (unsigned int)strtoul(optarg, NULL, 10);
                break;
            case 7:
                args->ping_interval = strtoul(optarg, NULL, 10);
                break;
            case 'h':
                return -1; // Help requested
            default:
                fprintf(stderr, "Unknown option: %s\n", argv[optind - 1]);
                return -1;
        }
    }

    int remaining = argc - optind;
    if (remaining <= 0) {
        fprintf(stderr, "Error: Missing target URL.\n");
        return -1;
    }

    if (remaining >= 2) {
        const char *first = argv[optind];
        if (is_subcommand(first)) {
            *command = strdup(first);
            *url = strdup(argv[optind + 1]);
        } else {
            *command = strdup("get");
            *url = strdup(first);
        }
    } else {
        // Only 1 argument remaining
        const char *first = argv[optind];
        if (is_subcommand(first)) {
            // A subcommand without a URL
            fprintf(stderr, "Error: Subcommand '%s' requires a target URL.\n", first);
            return -1;
        } else {
            *command = strdup("get");
            *url = strdup(first);
        }
    }

    return 0;
}

void nurl_cli_free_args(CommonArgs *args) {
    if (args) {
        free(args->user);
        free(args->bearer);
        free(args->token);
        free(args->data);
        free(args->cacert);
        free(args->output);
        if (args->header) {
            for (size_t i = 0; i < args->header_count; i++) {
                free(args->header[i]);
            }
            free(args->header);
        }
        memset(args, 0, sizeof(CommonArgs));
    }
}
