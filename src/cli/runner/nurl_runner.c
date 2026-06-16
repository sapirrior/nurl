#include "nurl_runner.h"
#include "commands.h"
#include "nurl_request.h"
#include "errors/nurl_diag.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

typedef int (*nurl_cmd_handler_t)(const char *url, const CommonArgs *common);

typedef struct {
    const char *name;
    nurl_cmd_handler_t handler;
} NurlCommand;

static const NurlCommand commands[] = {
    {"GET",      nurl_cmd_get},
    {"POST",     nurl_cmd_post},
    {"PUT",      nurl_cmd_put},
    {"DELETE",   nurl_cmd_delete},
    {"PATCH",    nurl_cmd_patch},
    {"HEAD",     nurl_cmd_head},
    {"OPTIONS",  nurl_cmd_options},
    {"RESOLVE",  nurl_cmd_resolve},
    {"PING",     nurl_cmd_ping},
    {"DOWNLOAD", nurl_cmd_download},
    {"UPLOAD",   nurl_cmd_upload},
    {"INSPECT",  nurl_cmd_inspect},
    {NULL,       NULL}
};

int nurl_runner_execute(const char *command, const char *url, const CommonArgs *common) {
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcasecmp(command, commands[i].name) == 0) {
            return commands[i].handler(url, common);
        }
    }

    nurl_diag_err("command '%s' is not supported by nurl.", command);
    nurl_diag_hint("supported commands include: get, post, put, delete, head, patch, options, download, upload, inspect, ping, resolve.");
    return NURL_ERR_BAD_ARGS;
}
