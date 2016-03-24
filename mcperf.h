#ifndef MCPERF_H
#define MCPERF_H

#include "cmdline.h"

#define USE_CACHED_TIME 0
#define MINIMUM_KEY_LENGTH 2
#define MAXIMUM_CONNECTIONS 512

#define MAX_SAMPLES 100000

#define LOADER_CHUNK 1024

extern char random_char[];
extern gengetopt_args_info args;

#endif // MCPERF_H
