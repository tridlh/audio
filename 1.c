/* 1.c 
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

#define DEBUG_LOG   1

#include "a.h"

/*									      */
/*																			  */
/* 
function support:
	1. create a single frequent pcm with dedicated freq, sample rate, 
channel, length, endian and width, and capsulate as .wav.
	2. play the wav through alsa or tiny alsa interface.
	3. record a pcm with dedicated sample rate, channel, length, endian and 
width, and capsulate as .wav. 
	4. encode the .wav to amr and decode with 3rd party lib. 
	5. encode the .wav to mp3 and decode with 3rd party lib. 
	6. encode the .wav to aac and decode with 3rd party lib. 
	5. encapsulate as mp4, 3gp format
*/

void init(s_audinfo *i);
void uninit(s_audinfo *i);
int argproc(s_audinfo *i, int argc, char *argv[]);
int newsingle(s_audinfo *i);
int newrecord(s_audinfo *i);
int play(s_audinfo *i);
int encode(s_audinfo *i);
int decode(s_audinfo *i);
int ininfo(s_audinfo *i);
int outinfo(s_audinfo *i);

Uint16 str2int16(Uint8 *buf, int endian);
Uint32 str2int32(Uint8 *buf, int endian);
void int2str16(Uint8* buf, int val, int endian);
void int2str32(Uint8* buf, int val, int endian);

int s2i(Uint8 *s, int cnt);


int main(int argc, char *argv[])
{
    int ret = 0;
    s_audinfo inf;

    init(&inf);
    
    ret = argproc(&inf, argc, argv);
    if (ret < 0) goto finish;

	switch (inf.op) {
		case 1:
			ret = newsingle(&inf);
			break;
		case 2:
			ret = play(&inf);
			break;
		case 3:
			ret = newrecord(&inf);
			break;
		default:
			break;
	}
	if (ret < 0) goto finish;
    
finish:
    Log("Finishing...");
    uninit(&inf);
   
    return 0;
}

int newsingle(s_audinfo *i) {
	int ret = 0;
	int width = i->wid / 8;
	i->ns = i->sr * i->len;
	i->sz = i->ns * i->ch * width; 
	i->wh.pad = i->sz & 0x1;
	
	if ((i->head = calloc(sizeof(char), WAVHEADSZ)) == NULL) {ret = -1; goto end;}

	int idx = 0;
	strncpy(i->head + idx, "RIFF", 4);
	idx += 4;

	int2str32(i->head + idx, WAVHEADSZ - 8 + i->sz + i->wh.pad, i->ibn);
	idx += 4;

	strncpy(i->head + idx, "WAVE", 4);
	idx += 4;

	strncpy(i->head + idx, "fmt ", 4);
	idx += 4;

	int2str32(i->head + idx, 16, i->ibn);
	idx += 4;

	int2str16(i->head + idx, FORMATTAG_PCM, i->ibn);
	idx += 2;

	int2str16(i->head + idx, i->ch, i->ibn);
	idx += 2;

	int2str32(i->head + idx, i->sr, i->ibn);
	idx += 4;

	int2str32(i->head + idx, i->sr * i->ch * width, i->ibn);
	idx += 4;

	int2str16(i->head + idx, i->ch * width, i->ibn);
	idx += 2;

	int2str16(i->head + idx, i->wid, i->ibn);
	idx += 2;

	strncpy(i->head + idx, "data", 4);
	idx += 4;

	int2str32(i->head + idx, i->ns * i->ch * width, i->ibn);
	idx += 4;

	if ((i->data = calloc(sizeof(char), i->sz)) == NULL) {ret = -1; goto end;}
	
	int val = 0;
	int cnt = 0;
	double t = 0.0;
	double dt = (double)1.0 / i->sr;
	while (cnt < i->ns) {
		val = (Uint16) (0x7fff * sin(2 * PI * t * i->freq));
		t += dt;
		switch (i->ch) {
			case 1:
				int2str16(i->data + cnt * width, val, i->ibn);
				cnt++;
				break;
			case 2:
				int2str16(i->data + cnt * width * i->ch, val, i->ibn);
				int2str16(i->data + (cnt * width + 1) * i->ch, val, i->ibn);
				cnt++;
				break;
			default:
				Loge("Not supported channel number %d", i->ch);
				goto end;
		}
	}
	
	/* write to wav*/	
    FILE *fpout = NULL;
    if ((fpout = fopen(i->fnameo, "wb+")) == NULL){
        fprintf (stderr, "Open file '%s' fail !!\n", i->fnameo);
        ret = -1;
        goto end;
    } else {
        Log("Open file '%s' succeed.", i->fnameo);
    }

	fwrite(i->head, sizeof(char), WAVHEADSZ, fpout);
	fwrite(i->data, sizeof(char), i->sz, fpout);
	if (i->wh.pad)
		fwrite(&(i->wh.pad), sizeof(char), 1, fpout);

	fclose(fpout);
end:
	Log("done");
	return ret;
}

int newrecord(s_audinfo *i) {

}
int play(s_audinfo *i) {

}
/*
int encode(s_audinfo *i);
int decode(s_audinfo *i);
int ininfo(s_audinfo *i);
int outinfo(s_audinfo *i);
*/

void init(s_audinfo *i) {
	memset(i, 0, sizeof(s_audinfo));
	Log("%d bytes cleared", sizeof(s_audinfo));
	i->wid = 16;
	i->sr = 44100;
	i->ch = 2;
	i->len = 10;
	i->freq = 300;
	strcpy(i->fnamei, "in.wav");
	strcpy(i->fnameo, "out.wav");
}

void uninit(s_audinfo *i) {
	if (i->head != NULL) {
		free(i->head);
		i->head == NULL;
	}
	if (i->data != NULL) {
		free(i->data);
		i->data = NULL;
	}
}


int argproc(s_audinfo *i, int argc, char *argv[]) {
    int ret = 0;
    int c;
    int digit_optind = 0;
    
    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"add", 1, 0, 0},
            {"append", 0, 0, 0},
            {"delete", 1, 0, 0},
            {"verbose", 0, 0, 0},
            {"create", 1, 0, 'c'},
            {"file", 1, 0, 0},
            {0, 0, 0, 0}
        };
    
        c = getopt_long (argc, argv, "abc:d:012n:r:p:",
                 long_options, &option_index);
        if (c == -1)
            break;
    
        switch (c) {
        case 0:
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;
    
        case '0':
        case '1':
        case '2':
            if (digit_optind != 0 && digit_optind != this_option_optind)
                printf ("digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            printf ("option %c\n", c);
            break;
    
        case 'a':
            printf ("option a\n");
            break;
    
        case 'b':
            printf ("option b\n");
            break;
    
        case 'c':
            printf ("option c with value '%s'\n", optarg);
            break;
    
        case 'd':
            printf ("option d with value '%s'\n", optarg);
            break;
    
        case '?':
            break;

        case 'n':
            printf ("option %c with value '%s' len %d\n", c, optarg, strlen(optarg));
            if (strlen(optarg) <  FILENAMESZ) {
                memset(i->fnameo, 0, FILENAMESZ);
                strncpy(i->fnameo, optarg, strlen(optarg));
            } else {
                Loge("file name length over limit: %d", FILENAMESZ);
            }
			i->op = 1;
            break;

		case 'p':
			printf ("option %c with value '%s' len %d\n", c, optarg, strlen(optarg));
			if (strlen(optarg) <  FILENAMESZ) {
                memset(i->fnamei, 0, FILENAMESZ);
                strncpy(i->fnamei, optarg, strlen(optarg));
            } else {
                Loge("file name length over limit: %d", FILENAMESZ);
            }
			i->op = 2;
			break;
			
        case 'r':
            printf ("option %c with value '%s' len %d\n", c, optarg, strlen(optarg));
            if (strlen(optarg) <  FILENAMESZ) {
                memset(i->fnameo, 0, FILENAMESZ);
                strncpy(i->fnameo, optarg, strlen(optarg));
            } else {
                Loge("file name length over limit: %d", FILENAMESZ);
            }
			i->op = 3;
            break;
    
        default:
            printf ("?? getopt returned character code 0%o ??\n", c);
        }
    }
    
    if (optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        printf ("\n");
    }

    return ret;
}

Uint16 str2int16(Uint8* buf, int endian){
    if (endian)
        return (((Uint16)buf[0] << 8) + buf[1]);
    else
        return (((Uint16)buf[1] << 8) + buf[0]);
}

Uint32 str2int32(Uint8* buf, int endian){
    if (endian)
        return (((((((Uint32)buf[0] << 8) + (Uint32)buf[1]) << 8) + (Uint32)buf[2]) << 8) + (Uint32)buf[3]);
    else
        return (((((((Uint32)buf[3] << 8) + (Uint32)buf[2]) << 8) + (Uint32)buf[1]) << 8) + (Uint32)buf[0]);
}

void int2str16(Uint8* buf, int val, int endian){
    if (endian) {
		buf[1] = val & 0xFF;
		buf[0] = (val >> 8) & 0xFF;
 	} else {
		buf[0] = val & 0xFF;
		buf[1] = (val >> 8) & 0xFF;
	}
}

void int2str32(Uint8* buf, int val, int endian){
    if (endian) {
		buf[3] = val & 0xFF;
		buf[2] = (val >> 8)  & 0xFF;
		buf[1] = (val >> 16) & 0xFF;
		buf[0] = (val >> 24) & 0xFF;
 	} else {
		buf[0] = val & 0xFF;
		buf[1] = (val >> 8)  & 0xFF;
		buf[2] = (val >> 16) & 0xFF;
		buf[3] = (val >> 24) & 0xFF;
	}
}


