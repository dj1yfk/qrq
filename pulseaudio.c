/* 
Copyright (C) 2011  Fabian Kurz, DJ1YFK
 
$Id$

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA  02110-1301, USA.

PulseAudio specific functions and includes.

*/

#include <ncurses.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>

extern long samplerate;

short int buf[441000];		/* 10 secs buffer */
int bufpos = 0;

void *open_dsp () {

	/* The Sample format to use */
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = 44100,
		.channels = 2
	};
	pa_simple *s = NULL;
	int ret = 1;
	int error;

	if (!(s = pa_simple_new(NULL, "qrq", PA_STREAM_PLAYBACK, NULL, 
				"playback", &ss, NULL, NULL, &error))) {
	        fprintf(stderr, "pa_simple_new() failed: %s\n", 
				pa_strerror(error));
	}

	return s;
}

/* actually just puts samples into the buffer that is played at the end 
(close_audio) */
void write_audio (void *bla, short int *in, size_t size) {
	int i = 0;
	for (i=0; i < size; i++) {
		buf[bufpos] = in[i];
		bufpos++;
	}	
}

void close_audio (void *s) {
	int e;
	pa_simple_write(s, buf, 2*bufpos, &e);
	pa_simple_free(s);
	bufpos = 0;
}


