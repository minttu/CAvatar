#ifndef CAVATAR_GEN_H
#define CAVATAR_GEN_H

#include "util.h"

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color;

static const color bgcol = {248, 248, 248};

void make_color(const char *hex, color *col);
void make_color_hex(const char *hex, char *color);
void make_pattern(const char *hex, int data[8][8]);
void make_image(const char *hex, struct evbuffer *evb, int side);

#endif