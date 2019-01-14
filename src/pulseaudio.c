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
extern void  *dsp_fd;

short int *buf = 0;
int bufsize = 0;
int bufpos = 0;

void *open_dsp () {
	static int opened = 0;

	/* with PA we only open the device once and then leave it
	   opened */
	if (opened) {
		return dsp_fd;
	}

	/* The Sample format to use */
	static pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = 8000,
		.channels = 1
	};
	ss.rate = samplerate;
	pa_simple *s = NULL;
	int error;

	if (!(s = pa_simple_new(NULL, "qrq", PA_STREAM_PLAYBACK, NULL, 
				"playback", &ss, NULL, NULL, &error))) {
	        fprintf(stderr, "pa_simple_new() failed: %s\n", 
				pa_strerror(error));
	}

	opened = 1;
	return s;
}

/* actually just puts samples into the buffer that is played at the end 
(close_audio) */
void write_audio (void *bla, int *in, int size) {
	int sample_count = size/sizeof(int);
	int i = 0;
	if(bufsize <= bufpos + sample_count) {
		buf = realloc(buf, (bufpos + sample_count) * sizeof(short int));
		bufsize = bufpos + sample_count;
	}
	for (i=0; i < sample_count; i++) {
		buf[bufpos] = (short int) in[i];
		bufpos++;
	}	
}

void close_audio (void *s) {
	int e;
	pa_simple_write(s, buf, bufpos*sizeof(short int), &e);
	pa_simple_drain(s, &e);
	bufpos = 0;
}


