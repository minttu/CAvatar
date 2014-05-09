#ifndef CAVATAR_CONFIG_H
#define CAVATAR_CONFIG_H

#include "gen.h"

// What port to run on
const static int port = 3000;
// How many threads
const static int threads = 6;
// Where to store images
const static char *imgfolder = "img";
// Should we cache 1=yes 0=no
const static int cache = 1;
// Default background color
static const color bgcol = {248, 248, 248};
// Minimum image size
const static int minsize = 8;
// Default image size
const static int defsize = 128;
// Maximum image size
const static int maxsize = 512;

#endif