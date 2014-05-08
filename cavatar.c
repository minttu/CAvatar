#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <evhtp.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "helper_routes.h"
#include "config.h"
#include "util.h"
#include "gen.h"

/**
 * Index route, serves a static file and handles a redirect.
 */
void route_index(evhtp_request_t *req, void *arg) {
    evhtp_uri_t *uri = req->uri;
    evhtp_query_t *query = uri->query;
    const char *parq = evhtp_kv_find(query, "q");
    const char *pars = evhtp_kv_find(query, "s");
    const char *parm = evhtp_kv_find(query, "m");
    if (parq) {
        char *m = malloc(sizeof(char)*64);
        char *tmp = makelower(parq, strlen(parq));
        char *md = md5((const char *)tmp, strlen(parq));
        free(tmp);
        if (parm) {
            sprintf(m, "/meta/%s", md);
        } else if (pars) {
            sprintf(m, "/%s/%d", md, atoi(pars));
        } else {
            sprintf(m, "/%s", md);
        }
        free(md);
        evhtp_headers_add_header(req->headers_out,
            evhtp_header_new("Location", m, 0, 1));
        free(m);
        evhtp_send_reply(req, 301);
    } else {
        serve_static(req, "static/index.html", "text/html");
    }
}

/**
 * Serves the favicon file.
 */
void route_favicon(evhtp_request_t *req, void *arg) {
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Cache-Control", "max-age=90000, public", 0, 0));
    serve_static(req, "static/favicon.ico", "image/x-icon");
}

/**
 * Serves meta information in JSON about a hash.
 */
void route_meta(evhtp_request_t *req, void *arg) {
    evhtp_uri_t *uri = req->uri;
    char *res = uri->path->full;
    char *hash = "";
    res = strtok((char *)res, "/");
    while (res != NULL) {
        if (strlen(res) == 32) {
            hash = res;
            break;
        }
        res = strtok(NULL, "/");
    }
    
    char *color = malloc(sizeof(char)*7);
    make_color_hex(hash, color);

    int i[8][8];
    make_pattern(hash, i);

    char *pattern = malloc(sizeof(char)*146);
    sprintf(pattern, "[[%d,%d,%d,%d,%d,%d,%d,%d],"
                      "[%d,%d,%d,%d,%d,%d,%d,%d],"
                      "[%d,%d,%d,%d,%d,%d,%d,%d],"
                      "[%d,%d,%d,%d,%d,%d,%d,%d],"
                      "[%d,%d,%d,%d,%d,%d,%d,%d],"
                      "[%d,%d,%d,%d,%d,%d,%d,%d],"
                      "[%d,%d,%d,%d,%d,%d,%d,%d],"
                      "[%d,%d,%d,%d,%d,%d,%d,%d]]", 
        i[0][0],i[0][1],i[0][2],i[0][3],i[0][4],i[0][5],i[0][6],i[0][7],
        i[1][0],i[1][1],i[1][2],i[1][3],i[1][4],i[1][5],i[1][6],i[1][7],
        i[2][0],i[2][1],i[2][2],i[2][3],i[2][4],i[2][5],i[2][6],i[2][7],
        i[3][0],i[3][1],i[3][2],i[3][3],i[3][4],i[3][5],i[3][6],i[3][7],
        i[4][0],i[4][1],i[4][2],i[4][3],i[4][4],i[4][5],i[4][6],i[4][7],
        i[5][0],i[5][1],i[5][2],i[5][3],i[5][4],i[5][5],i[5][6],i[5][7],
        i[6][0],i[6][1],i[6][2],i[6][3],i[6][4],i[6][5],i[6][6],i[6][7],
        i[7][0],i[7][1],i[7][2],i[7][3],i[7][4],i[7][5],i[7][6],i[7][7]
    );
    pattern[145] = 0;

    evbuffer_add_printf(req->buffer_out, "{\"hash\":\"%s\",\"modified\":false,\"color\":\"%s\",\"pattern\":%s}", hash, color, pattern);
    free(color);
    free(pattern);
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "application/json", 0, 0));
    evhtp_send_reply(req, 200);
}

/**
 * Serves a PNG file of the hash.
 */
void route_image(evhtp_request_t *req, void *arg) {
    // redisReply *reply;

    evhtp_uri_t *uri = req->uri;

    int side = 128;
    char *hash = "";

    char *res = uri->path->full;
    int ind = 0;
    res = strtok((char *)res, "/");
    while (res != NULL) {
        if (ind == 0) {
            if (strlen(res) == 32) {
                hash = res;
            } else if(strcmp(res, "avatar") == 0) {
                ind--;
            } else {
                route_400(req);
                return;
            }
        }
        if (ind == 1) {
            side = atoi(res);
            if (side == 0) {
                side = 128;
            } else if (side < 8) {
                side = 8;
            } else if (side > 512) {
                side = 512;
            }
        }
        res = strtok( NULL, "/" );
        ind++;
    }

    make_image(hash, req->buffer_out, side);
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "image/png", 0, 0));
    evhtp_send_reply(req, 200);
}

/**
 * A catch all route for serving some 404.
 */
void route_generic(evhtp_request_t *req, void *arg) {
    route_404(req);
}

int main() {
    evbase_t *evbase = event_base_new();
    evhtp_t *htp = evhtp_new(evbase, NULL);
    //MagickWandGenesis();
    
    // A place for our images
    mkdir(imgfolder, 0700);

    // A few static routes
    evhtp_set_cb(htp, "/", route_index, NULL);
    evhtp_set_cb(htp, "/favicon.ico", route_favicon, NULL);

    // Metas
    evhtp_set_regex_cb(htp, "[\\/](meta)[\\/]([0-9a-fA-F]{32})", route_meta, NULL);

    // Images
    evhtp_set_regex_cb(htp, "[\\/](avatar)[\\/]([0-9a-fA-F]{32})", route_image, NULL);
    evhtp_set_regex_cb(htp, "[\\/]([0-9a-fA-F]{32})", route_image, NULL);

    // 404 routes
    evhtp_set_regex_cb(htp, "[\\/](.{1,})", route_generic, NULL);
    evhtp_set_gencb(htp, route_generic, NULL);

    // libevhtp has a cool wrapper for pthreads
    evhtp_use_threads(htp, NULL, threads, NULL);

    // aaaand bind a socket..
    if(evhtp_bind_socket(htp, "0.0.0.0", port, 1024) < 0) {
        fprintf(stderr, "Could not bind to socket %d: %s\n", port, strerror(errno));
        exit(-1);
    }
    fprintf(stdout, "Binded to socket %d\n", port);

    // and listen!
    event_base_loop(evbase, 0);

    // free stuff after event loop
    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);
    //MagickWandTerminus();

    return 0;
}
