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
int newwav(s_audinfo *i);
int captwav(s_audinfo *i);
int playwav(s_audinfo *i);
int parsewav(s_audinfo *i);
int alsa_prepare(s_audinfo *i);
int clear_struct(s_audinfo *i);
int initnewf(s_audinfo *i);
int initplay(s_audinfo *i);
int initcapt(s_audinfo *i);

int encode(s_audinfo *i);
int decode(s_audinfo *i);
int audinfo(s_audinfo *i);

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
			ret = newwav(&inf);
			break;
		case 2:
			ret = playwav(&inf);
			break;
		case 3:
			ret = captwav(&inf);
			break;
		default:
            Loge("not supported operation %d", inf.op);
			break;
	}
	if (ret < 0) goto finish;
    
finish:
    Log("Finishing...");
    uninit(&inf);
   
    return 0;
}

int parsewav(s_audinfo *i) {
    int ret = 0;
    int fsize = 0;
    FILE *fp = NULL;
    
    if ((fp = fopen(i->fnamei, "rb")) == NULL){
        fprintf (stderr, "Input file '%s' does not exist !!\n", i->fnamei);
        ret = -1;
        goto end;
    } else {
        fseek(fp, 0L, SEEK_END);
        i->fsize = ftell(fp);
        rewind(fp);
        Log("Open inputfile '%s' [%d bytes] succeed.", i->fnamei, i->fsize);
    }
        
    if ((i->file = calloc(sizeof(char),i->fsize)) == NULL) {ret = -1; goto end;}
    
    ret = fread(i->file, sizeof(char), i->fsize, fp);
    Log("read: %x, filesize: %x", ret, i->fsize);
    if (ret != i->fsize) {
        Loge("ferror %x feof %x", ferror(fp), feof(fp));
        ret = -1;
        goto end;
    } else {
        ret = 0;
    }

    if ((i->head = calloc(sizeof(char), WAVHEADSZ)) == NULL) {ret = -1; goto end;}
    memcpy(i->head, i->file, WAVHEADSZ);

    int idx = 0;
    strncpy(i->w.riffid, i->head + idx, 4);
    idx += 4;
    if (0 != strncmp(i->w.riffid, "RIFF", 4)) {
        Loge("riffid not match: '%c' '%c' '%c' '%c'", i->w.riffid[0], i->w.riffid[1], i->w.riffid[2], i->w.riffid[3]);
        goto end;
    }
    
    i->w.riffsz = str2int32(i->head + idx, i->ibn);
    idx += 4;

    strncpy(i->w.waveid, i->head + idx, 4);
    idx += 4;
    if (0 != strncmp(i->w.waveid, "WAVE", 4)) {
        Loge("waveid not match");
        goto end;
    }

    strncpy(i->w.fmtid, i->head + idx, 4);
    idx += 4;
    if (0 != strncmp(i->w.fmtid, "fmt ", 4)) {
        Loge("fmtid not match");
        goto end;
    }

    i->w.fmtsz  = str2int32(i->head + idx, i->ibn);
    idx += 4;

    i->w.ftag   = str2int16(i->head + idx, i->ibn);
    idx += 2;

    i->w.ch     = str2int16(i->head + idx, i->ibn);
    idx += 2;
    i->ch = i->w.ch;

    i->w.sps    = str2int32(i->head + idx, i->ibn);
    idx += 4;
    i->sr = i->w.sps;

    i->w.bps    = str2int32(i->head + idx, i->ibn);
    idx += 4;

    i->w.ba     = str2int16(i->head + idx, i->ibn);
    idx += 2;

    i->w.wid    = str2int16(i->head + idx, i->ibn);
    idx += 2;
    i->wid = i->w.wid;

    strncpy(i->w.dataid, i->head + idx, 4);
    idx += 4;
    if (0 != strncmp(i->w.dataid, "data", 4)) {
        Loge("dataid not match");
        goto end;
    }

    i->w.datasz = str2int32(i->head + idx, i->ibn);
    idx += 4;

    i->sz = i->w.datasz;
    i->ns = i->sz / i->ch * 8 / i->wid;
    i->len = i->sz / i->ch / i->sr * 8 / i->wid;
    if (i->sz != (i->w.riffsz + 8 - WAVHEADSZ)) 
        i->w.pad = i->w.riffsz + 8 - WAVHEADSZ - i->sz;

    if ((i->data = calloc(sizeof(char),i->sz)) == NULL) {ret = -1; goto end;}
    memcpy(i->data, i->file + idx, i->sz);
    
end:
    fclose(fp);
    audinfo(i);
    return ret;
}


int newwav(s_audinfo *i) {
	int ret = 0;

    initnewf(i);
    
	int width = i->wid / 8;
	i->ns = i->sr * i->len;
	i->sz = i->ns * i->ch * width; 
	i->w.pad = i->sz & 0x1;
	
	if ((i->head = calloc(sizeof(char), WAVHEADSZ)) == NULL) {ret = -1; goto end;}
    if ((i->data = calloc(sizeof(char), i->sz)) == NULL) {ret = -1; goto end;}

	int idx = 0;
	strncpy(i->head + idx, "RIFF", 4);
    strncpy(i->w.riffid, i->head + idx, 4);
	idx += 4;

	int2str32(i->head + idx, WAVHEADSZ - 8 + i->sz + i->w.pad, i->ibn);
    i->w.riffsz = str2int32(i->head + idx, i->ibn);
	idx += 4;

	strncpy(i->head + idx, "WAVE", 4);
    strncpy(i->w.waveid, i->head + idx, 4);
	idx += 4;

	strncpy(i->head + idx, "fmt ", 4);
    strncpy(i->w.fmtid, i->head + idx, 4);
	idx += 4;

	int2str32(i->head + idx, 16, i->ibn);
    i->w.fmtsz  = str2int32(i->head + idx, i->ibn);
	idx += 4;

	int2str16(i->head + idx, FORMATTAG_PCM, i->ibn);
    i->w.ftag   = str2int16(i->head + idx, i->ibn);
	idx += 2;

	int2str16(i->head + idx, i->ch, i->ibn);
    i->w.ch     = str2int16(i->head + idx, i->ibn);
	idx += 2;

	int2str32(i->head + idx, i->sr, i->ibn);
    i->w.sps    = str2int32(i->head + idx, i->ibn);
	idx += 4;

	int2str32(i->head + idx, i->sr * i->ch * width, i->ibn);
    i->w.bps    = str2int32(i->head + idx, i->ibn);
	idx += 4;

	int2str16(i->head + idx, i->ch * width, i->ibn);
    i->w.ba     = str2int16(i->head + idx, i->ibn);
	idx += 2;

	int2str16(i->head + idx, i->wid, i->ibn);
    i->w.wid    = str2int16(i->head + idx, i->ibn);
	idx += 2;

	strncpy(i->head + idx, "data", 4);
    strncpy(i->w.dataid, i->head + idx, 4);
	idx += 4;

	int2str32(i->head + idx, i->ns * i->ch * width, i->ibn);
    i->w.datasz = str2int32(i->head + idx, i->ibn);
	idx += 4;
	
	int val = 0;
	int cnt = 0;
	double t = 0.0;
	double dt = (double)1.0 / i->sr;
	while (cnt < i->ns) {
		val = (Uint16) (RANGE_16 * sin(2 * PI * t * i->freq));
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
	if (i->w.pad)
		fwrite(&(i->w.pad), sizeof(char), 1, fpout);

	fclose(fpout);
end:
    audinfo(i);
	Log("done");
	return ret;
}

int captwav(s_audinfo *i) {
    int ret = 0;
    initcapt(i);
    return ret;
}

int playwav(s_audinfo *i) {
    int ret = 0;

    initplay(i);

    /* read pcm data to buffer*/
    if ((ret = parsewav(i)) != 0) {
        fprintf (stderr, "parse wav error(%s)\n", 
             snd_strerror (ret));
        goto end;
    }

    /* play pcm through alsa api*/
    int idx = 0;
    snd_pcm_t *playback_handle = i->a.playback_handle;
       
    if ((ret = snd_pcm_open (&playback_handle, i->a.playdev, i->a.stream, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n", 
             i->a.playdev,
             snd_strerror (ret));
        goto end;
    }

    snd_pcm_hw_params_t *hw_params = i->a.hw_params;
          
    if ((ret = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (ret));
        goto end;
    }
             
    if ((ret = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (ret));
        goto end;
    }
    
    if ((ret = snd_pcm_hw_params_set_access (playback_handle, hw_params, i->a.access)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (ret));
        goto end;
    }
   
    if ((ret = snd_pcm_hw_params_set_format (playback_handle, hw_params, i->a.format)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (ret));
        goto end;
    }
     
    if ((ret = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &(i->sr), &(i->a.dir))) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (ret));
        goto end;
    }
   
    if ((ret = snd_pcm_hw_params_set_channels (playback_handle, hw_params, i->ch)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (ret));
        goto end;
    }
     
    if ((ret = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (ret));
        goto end;
    }
     
    snd_pcm_hw_params_free (hw_params);   

    if ((ret = snd_pcm_nonblock (playback_handle, i->a.nb)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (ret));
        goto end;
    }

    if ((ret = snd_pcm_prepare (playback_handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (ret));
        goto end;
    }

    idx = 0;
    while (idx < (i->sz - ALSA_BUFSZ)) {
        Log("idx %x %d sz %x buf %x", idx, idx, i->sz, ALSA_BUFSZ);
        if ((ret = snd_pcm_writei (playback_handle, i->data + idx, ALSA_BUFSZ)) != ALSA_BUFSZ) {
            fprintf (stderr, "write to audio interface failed (%s)\n",
                 snd_strerror (ret));
            goto end;
        }
        idx += ALSA_BUFSZ;
    }
    Log("idx %d sz %d gap %d", idx, i->sz, i->sz - idx);
    if ((ret = snd_pcm_writei (playback_handle, i->data + idx, i->sz - idx)) != (i->sz - idx)) {
        fprintf (stderr, "write to audio interface failed (%s)\n",
             snd_strerror (ret));
        goto end;
    }
    
    snd_pcm_close (playback_handle);

end:
    return ret;
}

int alsa_prepare(s_audinfo *i) {
    int ret = 0;
    snd_pcm_t *playback_handle = i->a.playback_handle;
    snd_pcm_hw_params_t *hw_params = i->a.hw_params;
          
    if ((ret = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (ret));
        goto end;
    }
             
    if ((ret = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (ret));
        goto end;
    }
    
    if ((ret = snd_pcm_hw_params_set_access (playback_handle, hw_params, i->a.access)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (ret));
        goto end;
    }
   
    if ((ret = snd_pcm_hw_params_set_format (playback_handle, hw_params, i->a.format)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (ret));
        goto end;
    }
     
    if ((ret = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &(i->sr), &(i->a.dir))) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (ret));
        goto end;
    }
   
    if ((ret = snd_pcm_hw_params_set_channels (playback_handle, hw_params, i->ch)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (ret));
        goto end;
    }
     
    if ((ret = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (ret));
        goto end;
    }
     
    snd_pcm_hw_params_free (hw_params);
    
end:
    return ret;
}

int audinfo(s_audinfo *i) {
    int ret = 0;
    Log("is big endian:         %8d", i->ibn);
    Log("sample width in bits:  %8d", i->wid);
    Log("sample rate per second:%8d", i->sr);
    Log("channel number:        %8d", i->ch);
    Log("clip length in seconds:%8d", i->len);
    Log("single frequency in HZ:%8d", i->freq);
    Log("operation:             %8d", i->op);
    Log("number of sample:      %8d", i->ns);
    Log("clip size in Bytes:    %8d", i->sz);
    Log("file size in Bytes:    %8d", i->fsize);
    Log("alsa stream type:      %8d", i->a.stream);
    Log("alsa access type:      %8d", i->a.access);
    Log("alsa format type:      %8d", i->a.format);
    Log("alsa direction:        %8d", i->a.dir);
    Log("alsa non-block:        %8d", i->a.nb);
    Log("wav riffid:     '%c' '%c' '%c' '%c'", i->w.riffid[0],i->w.riffid[1],i->w.riffid[2],i->w.riffid[3]);
    Log("wav riff size:         %8d", i->w.riffsz);
    Log("wav waveid:     '%c' '%c' '%c' '%c'", i->w.waveid[0],i->w.waveid[1],i->w.waveid[2],i->w.waveid[3]);
    Log("wav fmt id:     '%c' '%c' '%c' '%c'", i->w.fmtid[0], i->w.fmtid[1], i->w.fmtid[2], i->w.fmtid[3]);
    Log("wav fmt size in Bytes: %8d", i->w.fmtsz);
    Log("wav ftag:            0x%8x", i->w.ftag);
    Log("wav channel number:    %8d", i->w.ch);
    Log("wav sample per seconds:%8d", i->w.sps);
    Log("wav bytes per seconds: %8d", i->w.bps);
    Log("wav bytes align:       %8d", i->w.ba);
    Log("wav sample width:      %8d", i->w.wid);
    Log("wav dataid:     '%c' '%c' '%c' '%c'", i->w.dataid[0],i->w.dataid[1],i->w.dataid[2],i->w.dataid[3]);
    Log("wav data size in Bytes:%8d", i->w.datasz);
    Log("wav pad sign:          %8d", i->w.pad);
    return ret;
}

/*
int encode(s_audinfo *i);
int decode(s_audinfo *i);
*/
int clear_struct(s_audinfo *i) {
    int ret = 0;
	memset(i, 0, sizeof(s_audinfo));	
    return ret;
}

int initnewf(s_audinfo *i) {
    int ret = 0;
	if (0 == i->wid) {i->wid = DEFAULT_WIDTH;}
	if (0 == i->sr)  {i->sr = DEFAULT_SRATE;}
	if (0 == i->ch)  {i->ch = DEFAULT_CHNUM;}
	if (0 == i->len) {i->len = DEFAULT_LENTH;}
	if (0 == i->freq){i->freq = DEFAULT_FREQ;}
    return ret;
}

int initplay(s_audinfo *i) {
    int ret = 0;
    i->a.stream = SND_PCM_STREAM_PLAYBACK;
    i->a.access = SND_PCM_ACCESS_RW_INTERLEAVED;
    i->a.format = SND_PCM_FORMAT_S16_LE;  
    i->a.dir = 0;
    i->a.nb = 0;
    return ret;
}

int initcapt(s_audinfo *i) {
    int ret = 0;
    i->a.stream = SND_PCM_STREAM_PLAYBACK;
    i->a.access = SND_PCM_ACCESS_RW_INTERLEAVED;
    i->a.format = SND_PCM_FORMAT_S16_LE;
    i->a.dir = 1;
    return ret;
}

void init(s_audinfo *i) {
    clear_struct(i);
    Log("%ld bytes cleared", sizeof(s_audinfo));
    strcpy(i->fnamei, DEFAULT_NAMEI);
    strcpy(i->fnameo, DEFAULT_NAMEO);
    strcpy(i->a.playdev, ALSA_PLAYDEV);
    strcpy(i->a.captdev, ALSA_CAPTDEV);
}

void uninit(s_audinfo *i) {
    if (i->file != NULL) {
		free(i->file);
		i->file == NULL;
	}
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
            printf ("option %c with value '%s' len %ld\n", c, optarg, strlen(optarg));
            if (strlen(optarg) <  FILENAMESZ) {
                memset(i->fnameo, 0, FILENAMESZ);
                strncpy(i->fnameo, optarg, strlen(optarg));
            } else {
                Loge("file name length over limit: %d", FILENAMESZ);
            }
			i->op = 1;
            break;

		case 'p':
			printf ("option %c with value '%s' len %ld\n", c, optarg, strlen(optarg));
			if (strlen(optarg) <  FILENAMESZ) {
                memset(i->fnamei, 0, FILENAMESZ);
                strncpy(i->fnamei, optarg, strlen(optarg));
            } else {
                Loge("file name length over limit: %d", FILENAMESZ);
            }
			i->op = 2;
			break;
			
        case 'r':
            printf ("option %c with value '%s' len %ld\n", c, optarg, strlen(optarg));
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


