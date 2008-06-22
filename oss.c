#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <ncurses.h>
#include <stdlib.h>
#include <fcntl.h>

extern long samplerate;

int open_dsp (char * device) {
	int tmp;
	int fd;
	
	if ((fd = open(device, O_WRONLY, 0)) == -1) {
		endwin();
		perror(device);
		exit(EXIT_FAILURE);
	}

	tmp = AFMT_S16_NE; 
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &tmp)==-1) {
		endwin();
		perror("SNDCTL_DSP_SETFMT");
		exit(EXIT_FAILURE);
	}

	if (tmp != AFMT_S16_NE) {
		endwin();
		fprintf(stderr, "Cannot switch to AFMT_S16_NE\n");
		exit(EXIT_FAILURE);
	}
  
	tmp = 2;	/* 2 channels, stereo */
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &tmp)==-1) {
		endwin();
		perror("SNDCTL_DSP_CHANNELS");
		exit(EXIT_FAILURE);
	}

	if (tmp != 2) {
		endwin();
		fprintf(stderr, "No stereo mode possible :(.\n");
		exit(EXIT_FAILURE);
	}

	if (ioctl(fd, SNDCTL_DSP_SPEED, &samplerate)==-1) {
		endwin();
		perror("SNDCTL_DSP_SPEED");
		exit(EXIT_FAILURE);
	}
return fd;
}

