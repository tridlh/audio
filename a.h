/* a.h
 **
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef _A_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <getopt.h>
#include <sys/time.h>

#define FILENAMESZ  	40
#define WAVHEADSZ		44
#define FORMATTAG_PCM	0x0001

#define PI              3.1416


typedef unsigned char   Uint8;
typedef unsigned short  Uint16;
typedef unsigned int    Uint32;

typedef Uint8			uint8_t;

typedef struct {
    struct timeval start;
    struct timeval end;
    struct timezone tz;
}   s_clock;

typedef struct {
	Uint32 riffid;
	Uint32 riffsz;
	Uint32 waveid;
	Uint32 fmtid;
	Uint32 fmtsz;
	Uint16 ftag;
	Uint16 ch;
	Uint32 sps;
	Uint32 bps;
	Uint16 ba;
	Uint16 wid;
	Uint32 dataid;
	Uint32 datasz;
	Uint32 pad; 
}	s_wavhead;

typedef struct {
	int ibn;		//is big endian
	int wid;		//width
	int sr;			//sample rate
	int ch;			//channel
	int len;		//length
	int freq;		//frequency
	int op;			//1. create pcm 2. play wav 3. create record 4. codec amr
	int ns;			//number of sample
	int sz;			//size in bytes
	char fnamei[FILENAMESZ];
	char fnameo[FILENAMESZ];
	s_wavhead wh;
	char *head;
	char *data;
	int *enc;
	int *dec;
}	s_audinfo;


#if DEBUG_LOG
    #define Log(...) \
        do { \
            printf("%s %-10s [%d]:",__FILE__, __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } while(0)
    #define Loge(...) \
        do { \
            printf("%s %-10s [%d]:",__FILE__, __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } while(0)
#else
    #define Log(...)
    #define Loge(...)\
        do { \
            printf("%s %-10s [%d]:",__FILE__, __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } while(0)
#endif

extern char *optarg;
extern int optind, opterr, optopt;

#define _GNU_SOURCE
#include <getopt.h>

int getopt_long(int argc, char * const argv[],
           const char *optstring,
           const struct option *longopts, int *longindex);



#endif

