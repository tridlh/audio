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
#include <alsa/asoundlib.h>

#define FILENAMESZ  	40
#define WAVHEADSZ		44
#define FORMATTAG_PCM	0x0001

#define DEFAULT_WIDTH   16
#define DEFAULT_CHNUM   2
#define DEFAULT_SRATE   44100
#define DEFAULT_LENTH   10
#define DEFAULT_FREQ    300
#define DEFAULT_NAMEI   "in.wav"
#define DEFAULT_NAMEO   "out.wav"

//alsa record and play device, review with aplay -l, cat /proc/asound/cards and cat /proc/asound/devices
#define ALSA_CAPTDEV    "hw:0,0"
#define ALSA_PLAYDEV    "hw:0,0"
#define ALSA_BUFSZ      1024*16

#define RANGE_8         0x7F
#define RANGE_16        0x7FFF
#define RANGE_24        0x7FFFFF
#define RANGE_32        0x7FFFFFFF

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
	Uint8  riffid[4];
	Uint32 riffsz;
	Uint8  waveid[4];
	Uint8  fmtid[4];
	Uint32 fmtsz;
	Uint16 ftag;
	Uint16 ch;
	Uint32 sps;
	Uint32 bps;
	Uint16 ba;
	Uint16 wid;
	Uint8  dataid[4];
	Uint32 datasz;
	Uint32 pad; 
}	s_waveinfo;

typedef struct {
    char playdev[FILENAMESZ];
    char captdev[FILENAMESZ];
    snd_pcm_stream_t stream;
    snd_pcm_access_t access;
    snd_pcm_format_t format;
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params;
    int dir;                        //play/record
    int nb;                         //nonblock
}   s_alsainfo;

typedef struct {
	int ibn;		//is big endian
	int wid;		//width
	int sr;			//sample rate
	int ch;			//channel
	int len;		//length in seconds
	int freq;		//frequency
	int op;			//1. create pcm 2. play wav 3. create record 4. codec amr
	int ns;			//number of sample
	int sz;			//data size in bytes
	int fsize;      //file size in bytes
	char fnamei[FILENAMESZ];
	char fnameo[FILENAMESZ];
    s_alsainfo a;
	s_waveinfo w;
    char *file;
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

