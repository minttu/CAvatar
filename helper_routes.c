#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <evhtp.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "helper_routes.h"
#include "util.h"

/**
 * Returns 404.
 */
void route_404(evhtp_request_t *req) {
    evbuffer_add_printf(req->buffer_out, " _  _    ___  _  _\n"
                                         "| || |  / _ \\| || |\n"
                                         "| || |_| | | | || |_ \n"
                                         "|__   _| | | |__   _|\n"
                                         "   | | | |_| |  | |  \n"
                                         "   |_|  \\___/   |_|\n");
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "text/plain", 0, 0));
    evhtp_send_reply(req, 404);
}

/**
 * Returns 400.
 */
void route_400(evhtp_request_t *req) {
    evbuffer_add_printf(req->buffer_out, " _  _    ___   ___\n"
                                         "| || |  / _ \\ / _ \\ \n"
                                         "| || |_| | | | | | |\n"
                                         "|__   _| | | | | | |\n"
                                         "   | | | |_| | |_| |\n"
                                         "   |_|  \\___/ \\___/ \n");
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "text/plain", 0, 0));
    evhtp_send_reply(req, 400);
}

/**
 * Serves a static file.
 */
void serve_static(evhtp_request_t *req, char *file, char *type) {
    int fd = -1;
    struct stat st;
    if (stat(file, &st)<0) {
        route_404(req);
        return;
    }
    if ((fd = open(file, O_RDONLY)) < 0) {
        route_404(req);
        return;
    }
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", type, 0, 1));
    evbuffer_add_file(req->buffer_out, fd, 0, st.st_size);
    evhtp_send_reply(req, 200);
}