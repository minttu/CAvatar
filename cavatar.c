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

#include "config.h"
#include "util.h"

void make_image(const char *str, struct evbuffer *evb, int side) {
    int rside = side/8;
    unsigned char *resp = NULL;
    size_t len;
    MagickWand *mw = NULL;

    mw = NewMagickWand();
    PixelWand **pw = NewPixelWands(2);
    PixelSetColor(pw[0], "#f8f8f8");

    unsigned char *digest = (unsigned char*)unhex(str);
    long longcol = crc(digest, 16);
    char *color = malloc(7*sizeof(char));
    color[0] = 0;
    sprintf(color, "#%02x%02x%02x",
            (unsigned int)(longcol & 0x000000ffUL),
            (unsigned int)(longcol & 0x0000ff00UL) >> 8,
            (unsigned int)(longcol & 0x00ff0000UL) >> 16);
    PixelSetColor(pw[1], color);
    MagickNewImage(mw, rside*8, rside*8, pw[0]);
    DrawingWand *dw = NewDrawingWand();
    DrawSetFillColor(dw, pw[1]);
    int tot = 0;
    for(int i = 0; i < 8; i++) {
        int data[8];
        for(int j = 0; j < 4; j++) {
            data[j] = data[7-j] = str[tot]&1;
            tot++;
        }
        for(int j = 0; j < 8; j++) {
            if (data[j]) {
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
    free(digest);
    free(resp);
}

void route_index(evhtp_request_t *req, void *arg) {
    evhtp_uri_t *uri = req->uri;
    evhtp_query_t *query = uri->query;
    const char *parq = evhtp_kv_find(query, "q");
    const char *pars = evhtp_kv_find(query, "s");
    if (parq) {
        char *m = malloc(sizeof(char)*64);
        char *md = md5((const char *)makelower(parq, strlen(parq)),
                       strlen(parq));
        if (pars) {
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
        int fd = -1;
        struct stat st;
        if (stat("index.html", &st)<0) {
            goto error;
        }
        if ((fd = open("index.html", O_RDONLY)) < 0) {
            goto error;
        }
        evbuffer_add_file(req->buffer_out, fd, 0, st.st_size);

        evhtp_headers_add_header(req->headers_out,
            evhtp_header_new("Content-Type", "text/html", 0, 0));
        evhtp_send_reply(req, 200);
    }
    return;
error:
    evhtp_send_reply(req, 400);
}

void route_favicon(evhtp_request_t *req, void *arg) {
    int fd = -1;
    struct stat st;
    if (stat("favicon.ico", &st)<0) {
        goto error;
    }
    if ((fd = open("favicon.ico", O_RDONLY)) < 0) {
        goto error;
    }
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "image/x-icon", 0, 0));
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Cache-Control", "max-age=90000, public", 0, 0));
    evbuffer_add_file(req->buffer_out, fd, 0, st.st_size);
    evhtp_send_reply(req, 200);
    return;
error:
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "text/plain", 0, 0));
    evhtp_send_reply(req, 400);
}

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
                goto error;
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
    return;
error:
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "text/plain", 0, 0));
    evhtp_send_reply(req, 400);
}

void route_generic(evhtp_request_t *req, void *arg) {
    evbuffer_add_printf(req->buffer_out, "<!DOCTYPE HTML><html><head>"
        "<meta charset=\"utf-8\"><title>404</title></head>"
        "<body><p>404 not found</p></body></html>");
    evhtp_headers_add_header(req->headers_out,
        evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, 404);
}

static evhtp_res print_path(evhtp_request_t *req, evhtp_path_t *path, void *arg) {
    puts(path->full);
    return EVHTP_RES_OK;
}

static evhtp_res handlers(evhtp_connection_t *conn, void *arg) {
    evhtp_set_hook(&conn->hooks, evhtp_hook_on_path, print_path, NULL);
    return EVHTP_RES_OK;
}

int main() {
    MagickWandGenesis();

    evbase_t *evbase = event_base_new();
    evhtp_t *htp = evhtp_new(evbase, NULL);

    evhtp_set_cb(htp, "/", route_index, NULL);
    evhtp_set_cb(htp, "/favicon.ico", route_favicon, NULL);
    evhtp_set_regex_cb(htp, "/(.{32})", route_image, NULL);
    evhtp_set_regex_cb(htp, "/(.{32})/", route_image, NULL);
    evhtp_set_regex_cb(htp, "/(.{32})/([0-9]{1,3})", route_image, NULL);
    evhtp_set_gencb(htp, route_generic, NULL);

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
