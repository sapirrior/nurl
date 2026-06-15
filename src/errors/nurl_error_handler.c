#include "nurl_error_handler.h"
#include "nurl_diag.h"
#include <stdio.h>
#include <string.h>

void nurl_handle_request_error(nurl_err_t err, const NurlRequest *req, const char *target_url) {
    if (err == NURL_OK) return;

    switch (err) {
        case NURL_ERR_INVALID_URL:
            nurl_diag_block("Error", "Malformed URL '%s' provided.", target_url);
            nurl_diag_block("Hint", "Ensure the URL uses a supported scheme like 'http://' or 'https://' and has a valid hostname.");
            break;

        case NURL_ERR_NETWORK:
            nurl_diag_block("Error", "Network connection reset or interrupted during the request to '%s'.", target_url);
            if (req && req->out && req->out != stdout) {
                nurl_diag_block("Suggestion", "Since you are downloading a file to disk, you can attempt to pick up where you left off by adding the --resume flag.");
            } else {
                nurl_diag_block("Hint", "Check your internet connection or verify if the server is reachable.");
            }
            break;

        case NURL_ERR_TLS:
            nurl_diag_block("Error", "TLS certificate verification failed for '%s'.", target_url);
            nurl_diag_block("Hint", "If you trust this host and want to bypass verification for local testing, use the -k or --no-verify flag.");
            break;

        case NURL_ERR_OOM:
            nurl_diag_block("Error", "nurl ran out of memory while processing the request to '%s'.", target_url);
            nurl_diag_block("Suggestion", "Try closing other high-memory applications or check your system's resource limits.");
            break;

        case NURL_ERR_IO:
            nurl_diag_block("Error", "Local I/O error occurred while reading or writing data for '%s'.", target_url);
            nurl_diag_block("Hint", "Ensure you have proper read/write permissions for the target file paths.");
            break;

        case NURL_ERR_HTTP_4XX:
            nurl_diag_block("Error", "The server returned a 4xx Client Error for the request to '%s'.", target_url);
            nurl_diag_block("Info", "This usually indicates a problem with the request parameters, authentication, or the resource path.");
            break;

        case NURL_ERR_HTTP_5XX:
            nurl_diag_block("Error", "The server returned a 5xx Server Error for the request to '%s'.", target_url);
            nurl_diag_block("Hint", "The remote server is currently experiencing issues. You might want to try again later.");
            break;

        default:
            nurl_diag_block("Error", "An unexpected error (code %d) occurred while requesting '%s'.", err, target_url);
            break;
    }
}
