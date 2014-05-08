#include <png.h>
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

#include "gen.h"
#include "config.h"

/**
 * Gets a color from a hex.
 */
void make_color(const char *hex, color *col) {
    unsigned char *digest = (unsigned char*)unhex(hex);
    long longcol = crc(digest, 16);
    col->r = (unsigned int)(longcol & 0x000000ffUL);
    col->g = (unsigned int)(longcol & 0x0000ff00UL) >> 8;
    col->b = (unsigned int)(longcol & 0x00ff0000UL) >> 16;
    free(digest);
}

/**
 * Gets a color from a hex.
 */
void make_color_hex(const char *hex, char *color) {
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

void set_single(png_byte *ptr, int on, color *C) {
    if(on) {
        ptr[0] = C->r;
        ptr[1] = C->g;
        ptr[2] = C->b;
    } else {
        ptr[0] = bgcol.r;
        ptr[1] = bgcol.g;
        ptr[2] = bgcol.b;
    }
}

void set_area(png_byte *ptr, int on, color *C, int scale) {
    for(int y = 0; y < scale; y++) {
        for(int x = 0; x < scale; x++) {
            set_single(&ptr[(x+y*8*scale)*3], on, C);
        }
    }
}

/**
 * Makes a image from hex with the size of side and dumps it onto evb.
 */
void make_image(const char *hex, struct evbuffer *evb, int side) {
    int scale = side/8;
    int data[8][8];
    int m = 0;
    char *imgname = malloc(sizeof(char) * 64);
    int fd = -1;
    struct stat st;
    
    sprintf(imgname, "%s/%s_%d.png", imgfolder, hex, scale*8);
    
    if(cache == 1 && stat(imgname, &st) >= 0 && (fd = open(imgname, O_RDONLY)) >= 0) {
        evbuffer_add_file(evb, fd, 0, st.st_size);
        fprintf(stdout, "Hit cache [ %s ]\n", imgname);
        free(imgname);
        return;
    }
    
    make_pattern(hex, data);
    color *col = malloc(sizeof(color));
    make_color(hex, col);

    png_image image;
    png_bytep buffer;

    memset(&image, 0, (sizeof image));
    image.version = PNG_IMAGE_VERSION;
    image.format = PNG_FORMAT_RGB;
    image.opaque = NULL;
    image.width = 8*scale;
    image.height = 8*scale;
    image.flags = 0;
    image.colormap_entries = 0;

    buffer = malloc(PNG_IMAGE_SIZE(image));

    for(int i = 0; i < 8; i++) {
        m = i*8*scale*scale;
        for(int j = 0; j < 4; j++) {
            set_area(&(buffer[(m+(j*2)*scale)*3]), data[i][(j*2)], col, scale);
            set_area(&(buffer[(m+((j*2)+1)*scale)*3]), data[i][(j*2)+1], col, scale);
        }
    }
    
    png_image_write_to_file(&image, imgname, 0, buffer, 0, NULL);
    
    stat(imgname, &st);
    fd = open(imgname, O_RDONLY);
    
    evbuffer_add_file(evb, fd, 0, st.st_size);
    if(cache == 0) {
        remove(imgname);
        fprintf(stdout, "Served [ %s ]\n", imgname);
    } else {
        fprintf(stdout, "Cached [ %s ]\n", imgname);
    }
    free(imgname);
    free(buffer);
}