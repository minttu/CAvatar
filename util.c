#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"

#define CRC24_INIT 0xb704ceL
#define CRC24_POLY 0x1864cfbL

/**
 * crc24 as defined in http://www.ietf.org/rfc/rfc2440.txt 6.1
 */
long crc(unsigned char *octets, size_t len) {
    long crc = CRC24_INIT;
    int i;

    while (len--) {
        crc ^= (*octets++) << 16;
        for (i = 0; i < 8; i++) {
            crc <<= 1;
            if (crc & 0x1000000)
                crc ^= CRC24_POLY;
        }
    }
    return crc & 0xffffffL;
}

/**
 * Returns a md5 hash of str.
 */
char *md5(const char *str, size_t len) {
    MD5_CTX ctx;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&ctx);
    MD5_Update(&ctx, str, len);
    MD5_Final(digest, &ctx);

    for(int i = 0; i < 16; i++) {
        snprintf(&(out[i*2]), 3, "%02x", (unsigned int)digest[i]);
    }
    return out;
}

/**
 * Returns a character from hex.
 */
char hex_to_char(unsigned char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    }
    return -1;
}

/**
 * From base16 to base 32
 */
char *unhex(const char *hex) {
    char *buffer = malloc(17*sizeof(char));
    buffer[16] = 0;
    for(int i = 0; i < 16; i++) {
        buffer[i] = hex_to_char(hex[i*2]) | (hex_to_char(hex[i*2+1])<<4);
    }
    return buffer;
}

/**
 * Returns str made lowercase
 */
char *makelower(const char *str, size_t t) {
    char *out = malloc(sizeof(char) * (t + 1));
    for (int i = 0; i < t; i++) {
        out[i] = tolower(str[i]);
    }
    out[t] = 0;
    return out;
}