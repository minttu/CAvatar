#ifndef CAVATAR_UTIL_H
#define CAVATAR_UTIL_H

long crc(unsigned char *octets, size_t len);
char *md5(const char *str, size_t len);
char *unhex(const char *hex);

#endif
