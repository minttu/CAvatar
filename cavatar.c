#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <hiredis/hiredis.h>

#include <wand/magick_wand.h>

#include "util.h"

redisContext *redis;

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
            data[j] = data[7-j] = (str[tot]) >> 6;
            tot++;
        }
        for(int j = 0; j < 8; j++) {
            if(data[j]) {
                DrawRectangle(dw, rside*j,       rside*i, 
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
    
    if(mw) {
        DestroyMagickWand(mw);
        DestroyDrawingWand(dw);
        DestroyPixelWands(pw, 2);
    }
    free(color);
    free(digest);
    free(resp);
}

void route_index(struct evhttp_request *req, void *arg) {
    struct evbuffer *evb = evbuffer_new();
    const char *uri = evhttp_request_get_uri(req);
    struct evkeyvalq *query = malloc(sizeof(struct evkeyvalq *));
    evhttp_parse_query(uri, query);
    const char *parq = evhttp_find_header(query, "q");
    const char *pars = evhttp_find_header(query, "s");
    if(parq) {
        uri += 4;
        char *m = malloc(sizeof(char)*64);
        char *md = md5(uri, strlen(uri));
        if(pars) {
            sprintf(m, "/%s/%d", md, atoi(pars));
        } else {
            sprintf(m, "/%s", md);
        }
        evhttp_add_header(evhttp_request_get_output_headers(req),
                          "Location", m);
        free(m);
        free(md);
        evhttp_add_header(evhttp_request_get_output_headers(req),
                          "Content-Type", "text/html");
        evhttp_send_reply(req, 301, "Moved Permanently", evb);
    } else {
        struct evbuffer *evb = evbuffer_new();
        int fd = -1;
        struct stat st;
        if (stat("index.html", &st)<0) {
            goto error;
        }
        if ((fd = open("index.html", O_RDONLY)) < 0) {
            goto error;
        }
        evbuffer_add_file(evb, fd, 0, st.st_size);

        evhttp_add_header(evhttp_request_get_output_headers(req), 
                          "Content-Type", "text/html");
        evhttp_send_reply(req, 200, "OK", evb);
    }
    goto done;
error:
    evhttp_send_error(req, 400, "Terrible request. Please try again.");
done:
    if(evb) {
        evbuffer_free(evb);
    }
}

void route_favicon(struct evhttp_request *req, void *arg) {
    struct evbuffer *evb = evbuffer_new();
    int fd = -1;
    struct stat st;
    if (stat("favicon.ico", &st)<0) {
        goto error;
    }
    if ((fd = open("favicon.ico", O_RDONLY)) < 0) {
        goto error;
    }
    evhttp_add_header(evhttp_request_get_output_headers(req),
                      "Content-Type", "image/x-icon");
    evhttp_add_header(evhttp_request_get_output_headers(req),
                      "Cache-Control", "max-age=90000, public");
    evbuffer_add_file(evb, fd, 0, st.st_size);
    evhttp_send_reply(req, 200, "OK", evb);

    goto done;
error:
    evhttp_add_header(evhttp_request_get_output_headers(req),
                      "Content-Type", "text/plain");
    evhttp_send_error(req, 400, "Terrible request. Please try again.");
done:
    if(evb) {
        evbuffer_free(evb);
    }
}

void route_image(struct evhttp_request *req, void *arg) {
    struct evbuffer *evb = NULL;
    // redisReply *reply;

    const char *uri = evhttp_request_get_uri(req);

    int side = 128;
    char *hash = "";

    char *res = NULL;
    int ind = 0;
    res = strtok((char *)uri, "/");
    while (res != NULL) {
        if(ind == 0) {
            if(strlen(res) == 32) {
                hash = res;
            } else {
                goto error;
            }
        }
        if(ind == 1) {
            side = atoi(res);
            if(side == 0) {
                side = 128;
            } else if(side < 8) {
                side = 8;
            } else if(side > 512) {
                side = 512;
            }
        }
        res = strtok( NULL, "/" );
        ind++;
    }

    if(strlen(uri) < 32) {
        goto error;
    }
    if(strlen(uri) > 32) {
        if(strstr(uri, "/")) {
            
        }
    }

    //reply = redisCommand(redis, "GET %s", uri);
    //if(reply->str) {
    //    evbuffer_add_printf(evb, "%s: %s", uri, reply->str);
    //}else{
    //}

    evb = evbuffer_new();
    make_image(hash, evb, side);
    evhttp_add_header(evhttp_request_get_output_headers(req),
                      "Content-Type", "image/png");
    evhttp_send_reply(req, 200, "OK", evb);
    goto done;
error:
    evhttp_add_header(evhttp_request_get_output_headers(req),
                      "Content-Type", "text/plain");
    evhttp_send_error(req, 400, "Terrible request. Please try again.");
done:
    if(evb) {
        evbuffer_free(evb);
    }
}

void router(struct evhttp_request *req, void *arg) {
    const char *uri = evhttp_request_get_uri(req);
    struct evhttp_uri *decoded = evhttp_uri_parse(uri);
    const char *path;

    if(!decoded) {
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }

    path = evhttp_uri_get_path(decoded);
    if (!path) {
        path = "/";
    }

    if(strcmp(path, "/") == 0) {
        route_index(req, arg);
    }else if(strcmp(uri, "/favicon.ico") == 0) { 
        route_favicon(req, arg);
    }else {
        route_image(req, arg);
    }

    if(decoded) {
        evhttp_uri_free(decoded);
    }

}

void* dispatch(void *arg) {
    event_base_dispatch((struct event_base*)arg);
    return NULL;
}

int get_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        return -1;
    }
    int one = 1;
    int r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
    if(r<0) {
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    r = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if(r<0) {
        return -1;
    }
    r = listen(sock, 10240);
    if(r<0) {
        return -1;
    }

    int flags;
    if((flags = fcntl(sock, F_GETFL, 0)) < 0
        || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    return sock;
}

int main() {
    redis = redisConnect("127.0.0.1", 6379);
    MagickWandGenesis();
    int sock = get_socket(3000);
    if(sock < 0) {
        return -1;
    }
    int nthreads = 12;


    pthread_t threads[nthreads];
    for (int i = 0; i < nthreads; i++) {
        struct event_base *base = event_base_new();
        struct evhttp *http = evhttp_new(base);
        //evhttp_set_cb(http, "/", route_index, NULL);
        evhttp_set_gencb(http, router, NULL);
        evhttp_accept_socket(http, sock);
        pthread_create(&threads[i], NULL, dispatch, base);
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }
    MagickWandTerminus();
    redisFree(redis);
    return 0;
}