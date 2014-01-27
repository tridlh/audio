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

e.g. 
	gcc 1.c -lm;./a.out -n 1.wav
