function support:
	1. create a single frequent pcm with dedicated freq, sample rate, 
channel, length, endian and width, and capsulate as .wav.
		freq:		300Hz, will support any freq
		width:		16bit, will support 8bit
		rate:		44100Hz, adjustable
		channel:	2, adjustable (1 or 2) 
		length:		2second, adjustable
	2. play the wav through alsa or tiny alsa interface.
		alsa:		done
		tinyalsa:	will support
	3. record a pcm with dedicated sample rate, channel, length, endian and 
width, and capsulate as .wav. 
		alsa:		done
		tinyalsa:	will support
	4. encode the .wav to amr and decode with 3rd party lib. 
		will support
	5. encode the .wav to mp3 and decode with 3rd party lib. 
		will support
	6. encode the .wav to aac and decode with 3rd party lib. 
		will support
	5. encapsulate as mp4, 3gp format
		will support

e.g. 
	gcc 1.c -lm -lasound -ldl -lpthread -I../include
	./a.out -n 1.wav --channel 2 --rate 44100 --length 2
	./a.out -p 1.wav
	./a.out -r 2.wav
