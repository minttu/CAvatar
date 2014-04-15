#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <evhtp.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <wand/magick_wand.h>

#include "helper_routes.h"
#include "config.h"
#include "util.h"

/**
 * Gets a color from a hex.
 */
void make_color(const char *hex, char *color) {
    unsigned char *digest = (unsigned char*)unhex(hex);
    long longcol = crc(digest, 16);
    color[0] = 0;
    sprintf(color, "#%02x%02x%02x",
            (unsigned int)(longcol & 0x000000ffUL),
            (unsigned int)(longcol & 0x0000ff00UL) >> 8,
            (unsigned int)(longcol & 0x00ff0000UL) >> 16);
    free(digest);
}

/**
 * Gets a pattern from a hex.
 */
void make_pattern(const char *hex, int data[8][8]) {
    int tot = 0;
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 4; j++) {
            data[i][j] = data[i][7-j] = hex[tot]&1;
            tot++;
        }
    }
}

/**
 * Makes a image from hex with the size of side and dumps it onto evb.
 */
void make_image(const char *hex, struct evbuffer *evb, int side) {
    int rside = side/8;
    unsigned char *resp = NULL;
    size_t len;
    MagickWand *mw = NULL;

    mw = NewMagickWand();
    PixelWand **pw = NewPixelWands(2);
    PixelSetColor(pw[0], "#f8f8f8");

    char *color = malloc(7*sizeof(char));
    make_color(hex, color);
   
    PixelSetColor(pw[1], color);
    MagickNewImage(mw, rside*8, rside*8, pw[0]);
    DrawingWand *dw = NewDrawingWand();
    DrawSetFillColor(dw, pw[1]);
    
    int data[8][8];
    make_pattern(hex, data);

    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if (data[i][j]) {
                DrawRectangle(dw, rside*j, rside*i,
                              rside*j+rside, rside*i+rside);
            }
        }
    }
    MagickDrawImage(mw, dw);

    MagickSetImageFormat(mw, "PNG");
    resp = MagickGetImageBlob(mw, &len);
    for (int i = 0; i < len; i++) {
        evbuffer_add_printf(evb, "%c", resp[i]);
    }

    if (mw) {
        DestroyMagickWand(mw);
        DestroyDrawingWand(dw);
        DestroyPixelWands(pw, 2);
    }
    free(color);
    free(resp);
}

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
        serve_static(req, "../index.html", "text/html");
    }
}

/**
 * Serves the favicon file.
 */
void route_favicon(evhtp_request_t *req, void *arg) {
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Cache-Control", "max-age=90000, public", 0, 0));
    serve_static(req, "../favicon.ico", "image/x-icon");
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
    make_color(hash, color);

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

/**
 * Prints the route every time a request is made.
 */
static evhtp_res print_path(evhtp_request_t *req, evhtp_path_t *path, void *arg) {
    puts(path->full);
    return EVHTP_RES_OK;
}

/**
 * Registers a few handlers.
 */
static evhtp_res handlers(evhtp_connection_t *conn, void *arg) {
    evhtp_set_hook(&conn->hooks, evhtp_hook_on_path, print_path, NULL);
    return EVHTP_RES_OK;
}

int main() {
    MagickWandGenesis();

    evbase_t *evbase = event_base_new();
    evhtp_t *htp = evhtp_new(evbase, NULL);

    // A few static routes
    evhtp_set_cb(htp, "/", route_index, NULL);
    evhtp_set_cb(htp, "/favicon.ico", route_favicon, NULL);

    // Metas
    evhtp_set_regex_cb(htp, "[\\/](meta)[\\/]([0-9a-fA-F]{32})", route_meta, NULL);

    // Images
    evhtp_set_regex_cb(htp, "[\\/]([0-9a-fA-F]{32})", route_image, NULL);

    // 404 routes
    evhtp_set_regex_cb(htp, "[\\/](.{1,})", route_generic, NULL);
    evhtp_set_gencb(htp, route_generic, NULL);

    // For printing out paths
    evhtp_set_post_accept_cb(htp, handlers, NULL);

    evhtp_use_threads(htp, NULL, THREADS, NULL);
    evhtp_bind_socket(htp, "0.0.0.0", PORT, 1024);
    event_base_loop(evbase, 0);
    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);

    MagickWandTerminus();
    return 0;
}
