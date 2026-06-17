#include "nurl_cli.h"
#include "errors/nurl_error.h"
#include "errors/nurl_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <strings.h>
#include <ctype.h>

void nurl_cli_init_args(CommonArgs *args) {
    memset(args, 0, sizeof(CommonArgs));
    args->timeout = 30; // Default timeout 30s
    args->connect_timeout = 10;
    args->ping_count = 1;
    args->ping_interval = 1000;
    args->upload_name = strdup("file");
}

static bool is_subcommand(const char *arg) {
    const char *commands[] = {
        "get", "post", "put", "delete", "head", "patch", "options",
        "download", "upload", "inspect", "ping", "resolve"
    };
    size_t count = sizeof(commands) / sizeof(commands[0]);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(arg, commands[i]) == 0) return true;
    }
    return false;
}

static bool looks_like_url(const char *arg) {
    if (strstr(arg, "://") != NULL) return true;
    if (strchr(arg, '.') != NULL) return true;
    if (strchr(arg, '/') != NULL) return true;
    if (strcmp(arg, "localhost") == 0) return true;
    return false;
}

static int edit_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1), len2 = strlen(s2);
    int matrix[32][32];
    if (len1 > 31) len1 = 31;
    if (len2 > 31) len2 = 31;
    for (int i = 0; i <= len1; i++) matrix[i][0] = i;
    for (int j = 0; j <= len2; j++) matrix[0][j] = j;
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            int a = matrix[i - 1][j] + 1;
            int b = matrix[i][j - 1] + 1;
            int c = matrix[i - 1][j - 1] + cost;
            int min = a < b ? a : b;
            matrix[i][j] = min < c ? min : c;
        }
    }
    return matrix[len1][len2];
}

static const char *suggest_command(const char *arg) {
    const char *commands[] = {
        "get", "post", "put", "delete", "head", "patch", "options",
        "download", "upload", "inspect", "ping", "resolve"
    };
    const char *best_match = NULL;
    int min_dist = 999;
    for (size_t i = 0; i < 12; i++) {
        int dist = edit_distance(arg, commands[i]);
        if (dist < min_dist) { min_dist = dist; best_match = commands[i]; }
    }
    return (min_dist <= 3) ? best_match : NULL;
}

static char *nurl_normalize_url(const char *raw) {
    if (!raw) return NULL;
    if (strstr(raw, "://")) return strdup(raw);
    size_t len = strlen(raw);
    char *normalized = malloc(len + 9);
    if (!normalized) return NULL;
    snprintf(normalized, len + 9, "https://%s", raw);
    return normalized;
}

static void nurl_cli_infer_method(const CommonArgs *args, char **command) {
    if (args->method) {
        free(*command);
        *command = strdup(args->method);
        return;
    }
    if (args->upload_file && strlen(args->upload_file) > 0) {
        free(*command); *command = strdup("upload");
    } else if ((args->data && strlen(args->data) > 0) || args->json) {
        free(*command); *command = strdup("post");
    }
}

static int set_arg_str(char **dest, const char *val, const char *name) {
    if (*dest) free(*dest);
    if (!(*dest = strdup(val))) {
        nurl_diag_err("out of memory while processing --%s.", name);
        return -1;
    }
    return 0;
}

static int append_arg_str(char ***array, size_t *count, const char *val, const char *name) {
    char **temp = realloc(*array, sizeof(char *) * (*count + 1));
    if (!temp || !(temp[*count] = strdup(val))) {
        nurl_diag_err("out of memory while processing --%s.", name);
        if (temp) *array = temp;
        return -1;
    }
    *array = temp;
    (*count)++;
    return 0;
}

int nurl_cli_parse(int argc, char **argv, CommonArgs *args, char **command, char **url) {
    nurl_cli_init_args(args);
    static struct option long_options[] = {
        {"user", 1, 0, 'u'}, {"bearer", 1, 0, 1}, {"token", 1, 0, 2}, {"no-auth", 0, 0, 3},
        {"data", 1, 0, 'd'}, {"json", 0, 0, 'j'}, {"no-verify", 0, 0, 'k'}, {"cacert", 1, 0, 4},
        {"timeout", 1, 0, 't'}, {"location", 0, 0, 'L'}, {"header", 1, 0, 'H'}, {"output", 1, 0, 'o'},
        {"include", 0, 0, 'i'}, {"verbose", 0, 0, 'v'}, {"silent", 0, 0, 's'}, {"raw", 0, 0, 5},
        {"count", 1, 0, 6}, {"interval", 1, 0, 7}, {"resume", 0, 0, 8}, {"progress", 0, 0, 9},
        {"mime", 1, 0, 10}, {"name", 1, 0, 11}, {"field", 1, 0, 12}, {"cookie", 1, 0, 'b'},
        {"cookie-jar", 1, 0, 'c'}, {"session", 1, 0, 13}, {"write-out", 1, 0, 'w'}, {"version", 0, 0, 'V'},
        {"help", 0, 0, 'h'}, {"cert", 1, 0, 14}, {"key", 1, 0, 15}, {"proxy", 1, 0, 'x'},
        {"proxy-user", 1, 0, 16}, {"no-proxy", 1, 0, 17}, {"user-agent", 1, 0, 'A'}, {"compressed", 0, 0, 18},
        {"retry", 1, 0, 19}, {"retry-delay", 1, 0, 20}, {"referer", 1, 0, 'e'}, {"fail", 0, 0, 'f'},
        {"tls1.2", 0, 0, 21}, {"tls1.3", 0, 0, 22}, {"request", 1, 0, 'X'}, {"upload", 1, 0, 23},
        {"connect-timeout", 1, 0, 24}, {0, 0, 0, 0}
    };

    int opt; opterr = 0;
    while ((opt = getopt_long(argc, argv, "u:d:jt:LH:o:ivshkb:c:w:Vx:A:e:fX:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'u': if (set_arg_str(&args->user, optarg, "user")) return -1; break;
            case 1:   if (set_arg_str(&args->bearer, optarg, "bearer")) return -1; break;
            case 2:   if (set_arg_str(&args->token, optarg, "token")) return -1; break;
            case 3:   args->no_auth = true; break;
            case 'd': if (set_arg_str(&args->data, optarg, "data")) return -1; args->data_len = strlen(optarg); break;
            case 'j': args->json = true; break;
            case 'k': args->no_verify = true; break;
            case 4:   if (set_arg_str(&args->cacert, optarg, "cacert")) return -1; break;
            case 't': args->timeout = strtoul(optarg, NULL, 10); break;
            case 'L': args->location = true; break;
            case 'H': if (!strchr(optarg, ':')) { nurl_err(NURL_ERR_ARG, "invalid header '%s'", optarg); return -1; }
                      if (append_arg_str(&args->header, &args->header_count, optarg, "header")) return -1; break;
            case 'o': if (set_arg_str(&args->output, optarg, "output")) return -1; break;
            case 'i': args->include = true; break;
            case 'v': args->verbose = true; break;
            case 's': args->silent = true; break;
            case 5:   args->raw = true; break;
            case 6:   args->ping_count = (unsigned int)strtoul(optarg, NULL, 10); break;
            case 7:   args->ping_interval = strtoul(optarg, NULL, 10); break;
            case 8:   args->resume = true; break;
            case 9:   args->progress = true; break;
            case 10:  if (set_arg_str(&args->upload_mime, optarg, "mime")) return -1; break;
            case 11:  if (set_arg_str(&args->upload_name, optarg, "name")) return -1; break;
            case 12:  if (append_arg_str(&args->upload_fields, &args->upload_fields_count, optarg, "field")) return -1; break;
            case 'b': if (set_arg_str(&args->cookie, optarg, "cookie")) return -1; break;
            case 'c': if (set_arg_str(&args->cookie_jar, optarg, "cookie-jar")) return -1; break;
            case 13:  if (set_arg_str(&args->session, optarg, "session")) return -1; break;
            case 'w': if (set_arg_str(&args->write_out, optarg, "write-out")) return -1; break;
            case 14:  if (set_arg_str(&args->cert, optarg, "cert")) return -1; break;
            case 15:  if (set_arg_str(&args->key, optarg, "key")) return -1; break;
            case 'x': if (set_arg_str(&args->proxy, optarg, "proxy")) return -1; break;
            case 16:  if (set_arg_str(&args->proxy_user, optarg, "proxy-user")) return -1; break;
            case 17:  if (set_arg_str(&args->no_proxy, optarg, "no-proxy")) return -1; break;
            case 'A': if (set_arg_str(&args->user_agent, optarg, "user-agent")) return -1; break;
            case 18:  args->compressed = true; break;
            case 19:  args->retry = (unsigned int)strtoul(optarg, NULL, 10); break;
            case 20:  args->retry_delay = strtoul(optarg, NULL, 10); break;
            case 'e': if (set_arg_str(&args->referer, optarg, "referer")) return -1; break;
            case 'f': args->fail = true; break;
            case 21:  args->tls12 = true; break;
            case 22:  args->tls13 = true; break;
            case 'X': if (set_arg_str(&args->method, optarg, "request")) return -1; break;
            case 23:  if (set_arg_str(&args->upload_file, optarg, "upload")) return -1; break;
            case 24:  args->connect_timeout = strtoul(optarg, NULL, 10); break;
            case 'V': printf("nurl %s\n", NURL_VERSION); exit(0);
            case 'h': return -1;
            default:  nurl_diag_err("option '%s' unrecognized.", argv[optind - 1]);
                      nurl_diag_hint("run 'nurl --help' for usage."); return -1;
        }
    }

    if (args->tls12 && args->tls13) { nurl_err(NURL_ERR_ARG, "conflicting TLS versions"); return -1; }
    int rem = argc - optind;
    if (rem <= 0) { nurl_err(NURL_ERR_ARG, "no URL specified!"); return -1; }

    const char *first = argv[optind];
    if (!is_subcommand(first) && !looks_like_url(first)) {
        nurl_err(NURL_ERR_ARG, "unknown command '%s'", first);
        const char *sug = suggest_command(first);
        if (sug) nurl_hint("did you mean: %s", sug);
        return -1;
    }

    if (is_subcommand(first)) {
        *command = strdup(first);
        if (strcasecmp(first, "upload") == 0) {
            if (rem >= 3) { *url = strdup(argv[optind+1]); if (set_arg_str(&args->upload_file, argv[optind+2], "upload")) return -1; }
            else if (rem == 2 && args->upload_file) *url = strdup(argv[optind+1]);
            else { nurl_diag_err("upload requires URL and file."); return -1; }
        } else {
            if (rem >= 2) *url = strdup(argv[optind+1]);
            else { nurl_diag_err("command '%s' requires a URL.", first); return -1; }
        }
    } else {
        *command = strdup("get"); *url = strdup(first); nurl_cli_infer_method(args, command);
    }

    if (*url) { char *norm = nurl_normalize_url(*url); free(*url); *url = norm; }
    return 0;
}

void nurl_cli_free_args(CommonArgs *args) {
    if (!args) return;
    free(args->method); free(args->user); free(args->bearer); free(args->token);
    free(args->data); free(args->cacert); free(args->output); free(args->upload_file);
    free(args->upload_name); free(args->upload_mime); free(args->cookie);
    free(args->cookie_jar); free(args->session); free(args->write_out);
    free(args->cert); free(args->key); free(args->proxy); free(args->proxy_user);
    free(args->no_proxy); free(args->user_agent); free(args->referer);
    if (args->upload_fields) {
        for (size_t i = 0; i < args->upload_fields_count; i++) free(args->upload_fields[i]);
        free(args->upload_fields);
    }
    if (args->header) {
        for (size_t i = 0; i < args->header_count; i++) free(args->header[i]);
        free(args->header);
    }
    memset(args, 0, sizeof(CommonArgs));
}
