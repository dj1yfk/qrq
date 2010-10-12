/* 
qrq - High speed morse trainer, similar to the DOS classic "Rufz"
Copyright (C) 2006-2010  Fabian Kurz

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

*/ 


#include <pthread.h>			/* CW output will be in a separate thread */
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>				/* basename */
#include <ctype.h>
#include <time.h> 
#include <limits.h> 			/* PATH_MAX */
#include <dirent.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>			/* mkdir */
#include <sys/types.h>
#include <errno.h>


#define PI M_PI

#define SILENCE 0		/* Waveforms for the tone generator */
#define SINE 1
#define SAWTOOTH 2
#define SQUARE 3

#ifndef DESTDIR
#	define DESTDIR "/usr"
#endif

#ifndef VERSION
#  define VERSION "0.0.0"
#endif

#ifndef OPENAL
#include "oss.h"
#define write_audio(x, y, z) write(x, y, z)
#define close_audio(x) close(x)
typedef int AUDIO_HANDLE;
#else
#include "OpenAlImp.h"
typedef void *AUDIO_HANDLE;
#endif

/* callsign array will be dynamically allocated */
static char **calls = NULL;

const static char *codetable[] = {
".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",".---",
"-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",
".--","-..-","-.--","--..","-----",".----","..---","...--","....-",".....",
"-....", "--...","---..","----."};

/* List of available callbase files. Probably no need to do dynamic memory allocation for that list.... */

static char cblist[100][PATH_MAX];

static char mycall[15]="DJ1YFK";		/* mycall. will be read from qrqrc */
static char dspdevice[PATH_MAX]="/dev/dsp";	/* will also be read from qrqrc */
static int score = 0;					/* qrq score */
static int callnr;						/* nr of actual call in attempt */
static int initialspeed=200;			/* initial speed. to be read from file*/
static int mincharspeed=0;				/* min. char. speed, below: farnsworth*/
static int speed=200;					/* current speed in cpm */
static int maxspeed=0;
static int freq=800;					/* current cw sidetone freq */
static int errornr=0;					/* number of errors in attempt */
static int p=0;							/* position of cursor, relative to x */
static int status=1;					/* 1= attempt, 2=config */
static int mode=1;						/* 0 = overwrite, 1 = insert */
static int j=0;							/* counter etc. */
static int constanttone=0;              /* if 1 don't change the pitch */
static int ctonefreq=800;               /* if constanttone=1 use this freq */
static int f6=0;						/* f6 = 1: allow unlimited repeats */
static int fixspeed=0;					/* keep speed fixed, regardless of err*/
static int unlimitedattempt=0;			/* attempt with all calls  of the DB */
static int attemptvalid=1;				/* 1 = not using any "cheats" */
static unsigned long int nrofcalls=0;	

long samplerate=44100;
static long long_i;
static int waveform = SINE;				/* waveform: (0 = none) */
static char wavename[10]="Sine    ";	/* Name of the waveform */
static int edge=2;						/* rise/fall time in milliseconds */
static int ed;							/* risetime, normalized to samplerate */

static short buffer[88200];

static AUDIO_HANDLE dsp_fd;

static int display_toplist();
static int calc_score (char * realcall, char * input, int speed, char * output);
static int update_score();
static int show_error (char * realcall, char * wrongcall); 
static int clear_display();
static int add_to_toplist(char * mycall, int score, int maxspeed);
static int read_config();
static int save_config();
static int tonegen(int freq, int length, int waveform);
static void *morse(void * arg); 
static int readline(WINDOW *win, int y, int x, char *line, int i); 
static void thread_fail (int j);
static int check_toplist ();
static int find_files ();
static int statistics ();
static int read_callbase ();
static void find_callbases();
static void select_callbase ();
static void help ();

pthread_t cwthread;				/* thread for CW output, to enable
								   keyboard reading at the same time */
pthread_attr_t cwattr;

char rcfilename[PATH_MAX]="";			/* filename and path to qrqrc */
char tlfilename[PATH_MAX]="";			/* filename and path to toplist */
char cbfilename[PATH_MAX]="";			/* filename and path to callbase */

char destdir[PATH_MAX]="";


/* create windows */
WINDOW *top_w;					/* actual score					*/
WINDOW *mid_w;					/* callsign history/mistakes	*/
WINDOW *bot_w;					/* user input line				*/
WINDOW *right_w;				/* highscore list/settings		*/

int main (int argc, char *argv[]) {

  /* if built as osx bundle set DESTDIR to Resources dir of bundle */
#ifdef OSX_BUNDLE
  char* p_slash = strrchr(argv[0], '/');
  strncpy(destdir, argv[0], p_slash - argv[0]);
  strcat(destdir, "/../Resources");
#else
  strcpy(destdir, DESTDIR);
#endif

	char tmp[80]="";
	char input[15]="";
	int i=0,j=0;						/* counter etc. */
	int f6pressed=0;

	if (argc > 1) {
		help();
	}
	
	(void) initscr();
	cbreak();
	noecho();
	curs_set(FALSE);
	keypad(stdscr, TRUE);
	scrollok(stdscr, FALSE);

	printw("qrq v%s - Copyright (C) 2006-2010 Fabian Kurz, DJ1YFK\n", VERSION);
	printw("This is free software, and you are welcome to redistribute it\n");
	printw("under certain conditions (see COPYING).\n");

	refresh();

	/* search for 'toplist', 'qrqrc' and callbase.qcb and put their locations
	 * into tlfilename, rcfilename, cbfilename */
	find_files();

	/* check if the toplist is in the suitable format. as of 0.0.7, each line
	 * is 31 characters long, with the added time stamp */
	check_toplist();


	/* buffer for audio */
	for (long_i=0;long_i<88200;long_i++) {
		buffer[long_i]=0;
	}
	
	/* random seed from time */
	srand( (unsigned) time(NULL) ); 

	/* Initialize cwthread. We have to wait for the cwthread to finish before
	 * the next cw output can be made, this will be done with pthread_join */
	pthread_attr_init(&cwattr);
	pthread_attr_setdetachstate(&cwattr, PTHREAD_CREATE_JOINABLE);

	/****** Reading configuration file ******/
	printw("\nReading configuration file qrqrc \n");
	read_config();

	attemptvalid = 1;
	if (f6 || fixspeed || unlimitedattempt) {
		attemptvalid = 0;	
	}

	/****** Reading callsign database ******/
	printw("\nReading callsign database... ");
	nrofcalls = read_callbase();

	printw("done. %d calls read.\n\n", nrofcalls);
	printw("Press any key to continue...");

	refresh();
	getch();

	top_w = newwin(4, 60, 0, 0);
	mid_w = newwin(17, 60, 4, 0);
	bot_w = newwin(3, 60, 21, 0);
	right_w = newwin(24, 20, 0, 60);

	/* no need to join here, this is the first possible time CW is sent */
	pthread_create(&cwthread, NULL, & morse, (void *) "QRQ");

/* very outter loop */
while (1) {	

/* status 1 = running an attempt of 50 calls */	
while (status == 1) {
	box(top_w,0,0);
	box(mid_w,0,0);
	box(bot_w,0,0);
	box(right_w,0,0);
	wattron(top_w,A_BOLD);
	mvwaddstr(top_w,1,1, "QRQ v");
	mvwaddstr(top_w,1,6, VERSION);
	wattroff(top_w, A_BOLD);
	mvwaddstr(top_w,1,11, " by Fabian Kurz, DJ1YFK");
	mvwaddstr(top_w,2,1, "Homepage and Toplist: http://fkurz.net/ham/qrq.html"
					"     ");

	clear_display();
	wattron(mid_w,A_BOLD);
	mvwaddstr(mid_w,1,1, "Usage:");
	mvwaddstr(mid_w,10,2, "F6                          F10       ");
	wattroff(mid_w, A_BOLD);
	mvwaddstr(mid_w,2,2, "After entering your callsign, 50 random callsigns");
	mvwaddstr(mid_w,3,2, "from a database will be sent. After each callsign,");
	mvwaddstr(mid_w,4,2, "enter what you have heard. If you copied correctly,");
	mvwaddstr(mid_w,5,2, "full points are credited and the speed increases by");
	mvwaddstr(mid_w,6,2, "2 WpM -- otherwise the speed decreases and only a ");
	mvwaddstr(mid_w,7,2, "fraction of the points, depending on the number of");
	mvwaddstr(mid_w,8,2, "errors is credited.");
	mvwaddstr(mid_w,10,2, "F6 repeats a callsign once, F10 quits.");
	mvwaddstr(mid_w,12,2, "Settings can be changed with F5 (or in qrqrc).");
	mvwaddstr(mid_w,14,2, "Score statistics (requires gnuplot) with F7.");

	wattron(right_w,A_BOLD);
	mvwaddstr(right_w,1, 6, "Toplist");
	wattroff(right_w,A_BOLD);

	display_toplist();

	p=0;						/* cursor to start position */
	wattron(bot_w,A_BOLD);
	mvwaddstr(bot_w, 1, 1, "Please enter your callsign:                      ");
	wattroff(bot_w,A_BOLD);
	
	wrefresh(top_w);
	wrefresh(mid_w);
	wrefresh(bot_w);
	wrefresh(right_w); 

	/* reset */
	maxspeed = errornr = score = 0;
	speed = initialspeed;
	
	/* prompt for own callsign */
	i = readline(bot_w, 1, 30, mycall, 0);

	/* F5 -> Configure sound */
	if (i == 5) {
		status = 2;
		break;
	} 
	/* F6 -> play test CW */
	else if (i == 6) {
		freq = constanttone ? ctonefreq : 800;
		pthread_join(cwthread, NULL);
		j = pthread_create(&cwthread, NULL, &morse, (void *) "VVVTEST");	
		thread_fail(j);
		break;
	}
	else if (i == 7) {
		statistics();
		break;
	}

	if (strlen(mycall) == 0) {
		strcpy(mycall, "NOCALL");
	}
	else if (strlen(mycall) > 7) {		/* cut excessively long calls */
		mycall[7] = '\0';
	}
	
	clear_display();
	wrefresh(mid_w);
	
	/* update toplist (highlight may change) */
	display_toplist();

	mvwprintw(top_w,1,1,"                                      ");
	mvwprintw(top_w,2,1,"                                               ");
	mvwprintw(top_w,1,1,"Callsign:");
	wattron(top_w,A_BOLD);
	mvwprintw(top_w,1,11, "%s", mycall);
	wattroff(top_w,A_BOLD);
	update_score();
	wrefresh(top_w);


	/* Reread callbase */
	nrofcalls = read_callbase();

	/****** send 50 or unlimited calls, ask for input, score ******/
	
	for (callnr=1; callnr < (unlimitedattempt ? nrofcalls : 51); callnr++) {
		/* Make sure to wait for the cwthread of the previous callsign, if
		 * necessary. */
		pthread_join(cwthread, NULL);
		
		/* select an unused callsign from the calls-array */
		do {
			i = (int) ((float) nrofcalls*rand()/(RAND_MAX+1.0));
		} while (calls[i] == NULL);

		/* only relevant for callbases with less than 50 calls */
		if (nrofcalls == callnr) { 		/* Only one call left!" */
				callnr =  51; 			/* Get out after next one */
		}



		/* output frequency handling a) random b) fixed */
		if ( constanttone == 0 ) {
				/* random freq, fraction of samplerate */
				freq = (int) (samplerate/(50+(40.0*rand()/(RAND_MAX+1.0))));
		}
		else { /* fixed frequency */
				freq = ctonefreq;
		}

		mvwprintw(bot_w,1,1,"                                      ");
		mvwprintw(bot_w, 1, 1, "%3d/%s", callnr, unlimitedattempt ? "-" : "50");	
		wrefresh(bot_w);	
		tmp[0]='\0';

		/* starting the morse output in a separate process to make keyboard
		 * input and echoing at the same time possible */
	
		j = pthread_create(&cwthread, NULL, morse, calls[i]);	
		thread_fail(j);		
		
		f6pressed=0;
		while (readline(bot_w, 1, 8, input,1) > 4) {/* F5 or F6 was pressed */
			if (f6pressed && (f6 == 0)) {
				continue;
			}
			f6pressed=1;
			/* wait for old cwthread to finish, then send call again */
			pthread_join(cwthread, NULL);
			j = pthread_create(&cwthread, NULL, morse, calls[i]);	
			thread_fail(j);		
		}
		tmp[0]='\0';	
		score += calc_score(calls[i], input, speed, tmp);
		update_score();
		if (strcmp(tmp, "*")) {			/* made an error */
				show_error(calls[i], tmp);
		}
		input[0]='\0';
		calls[i] = NULL;
	}

	/* attempt is over, send AR */
	pthread_join(cwthread, NULL);
	j = pthread_create(&cwthread, NULL, &morse, (void *) "+");	

	add_to_toplist(mycall, score, maxspeed);
	
	wattron(bot_w,A_BOLD);
	mvwprintw(bot_w,1,1, "Attempt finished. Press any key to continue!");
	wattroff(bot_w,A_BOLD);
	wrefresh(bot_w);
	getch();
	mvwprintw(bot_w,1,1, "                                            ");
	
} /* while (status == 1) */

/* status == 2. Change parameters */
while (status == 2) {
	clear_display();

	switch (waveform) {
		case SINE:
			strcpy(wavename, "Sine    ");
			break;
		case SAWTOOTH:
			strcpy(wavename, "Sawtooth");
			break;
		case SQUARE:
			strcpy(wavename, "Square  ");
			break;
	}

	mvwaddstr(bot_w,1,1, "                                                         ");
	curs_set(0);
	wattron(mid_w,A_BOLD);
	mvwaddstr(mid_w,1,1, "Configuration:          Value                Change");
	mvwprintw(mid_w,14,2, "      F6                    F10            ");
	mvwprintw(mid_w,15,2, "      F2");
	wattroff(mid_w, A_BOLD);
	mvwprintw(mid_w,2,2, "Initial Speed:         %3d CpM / %3d WpM" 
					"    up/down", initialspeed, initialspeed/5);
	mvwprintw(mid_w,3,2, "Min. character Speed:  %3d CpM / %3d WpM" 
					"    left/right", mincharspeed, mincharspeed/5);
	mvwprintw(mid_w,4,2, "CW rise/falltime (ms): %d %s" 
					"        +/-", edge, (edge < 0) ? "(adaptive)" : "           ");
	mvwprintw(mid_w,5,2, "Callsign:              %-14s" 
					"       c", mycall);
	mvwprintw(mid_w,6,2, "CW pitch (0 = random): %-4d"
					"                 k/l or 0", (constanttone)?ctonefreq : 0);
	mvwprintw(mid_w,7,2, "CW waveform:           %-8s"
					"             w", wavename);
	mvwprintw(mid_w,8,2, "Allow unlimited F6*:   %-3s"
					"                  f", (f6 ? "yes" : "no"));
	mvwprintw(mid_w,9,2, "Fixed CW speed*:       %-3s"
					"                  s", (fixspeed ? "yes" : "no"));
	mvwprintw(mid_w,10,2, "Unlimited attempt*:    %-3s"
					"                  u", (unlimitedattempt ? "yes" : "no"));
	mvwprintw(mid_w,11,2, "Callsign database:     %-15s"
					"      d (%d)", basename(cbfilename),nrofcalls);
	mvwprintw(mid_w,12,2, "DSP device:            %-15s"
					"      e", dspdevice);
	mvwprintw(mid_w,14,2, "Press");
	mvwprintw(mid_w,14,11, "to play sample CW,");
	mvwprintw(mid_w,14,34, "to go back.");
	mvwprintw(mid_w,15,2, "Press");
	mvwprintw(mid_w,15,11, "to save config permanently.");
	mvwprintw(bot_w,1,11, "* Makes scores ineligible for toplist");
	wrefresh(mid_w);
	wrefresh(bot_w);
	
	j = getch();

	switch ((int) j) {
		case '+':							/* rise/falltime */
			if (edge < 9) {
				edge++;
			}
			break;
		case '-':
			if (edge >= 0) {
				edge--;
			}
			break;
		case 'w':							/* change waveform */
			waveform = ((waveform + 1) % 3)+1;	/* toggle 1-2-3 */
			break;
		case 'k':							/* constanttone */
			if (ctonefreq >= 160) {
				ctonefreq -= 10;
			}
			else {
					constanttone = 0;
			}
			break;
		case 'l':
			if (constanttone == 0) {
				constanttone = 1;
			}
			else if (ctonefreq < 1600) {
				ctonefreq += 10;
			}
			break;
		case '0':
			if (constanttone == 1) {
				constanttone = 0;
			}
			else {
				constanttone = 1;
			}
			break;
		case 'f':
				f6 = (f6 ? 0 : 1);
			break;
		case 's':
				fixspeed = (fixspeed ? 0 : 1);
			break;
		case 'u':
				unlimitedattempt = (unlimitedattempt ? 0 : 1);
			break;
		case KEY_UP: 
			initialspeed += 10;
			break;
		case KEY_DOWN:
			if (initialspeed > 10) {
				initialspeed -= 10;
			}
			break;
		case KEY_RIGHT:
			mincharspeed += 10;
			break;
		case KEY_LEFT:
			if (mincharspeed > 10) {
				mincharspeed -= 10;
			}
			break;
		case 'c':
			readline(mid_w, 5, 25, mycall, 1);
			if (strlen(mycall) == 0) {
				strcpy(mycall, "NOCALL");
			}
			else if (strlen(mycall) > 7) {	/* cut excessively long calls */
				mycall[7] = '\0';
			}
			p=0;							/* cursor position */
			break;
		case 'e':
			readline(mid_w, 12, 25, dspdevice, 0);
			if (strlen(dspdevice) == 0) {
				strcpy(dspdevice, "/dev/dsp");
			}
			p=0;							/* cursor position */
			break;
		case 'd':							/* go to database browser */
				status = 3;
				curs_set(1);
			break;
		case KEY_F(2):
			save_config();	
			mvwprintw(mid_w,15,39, "Config saved!");
			wrefresh(mid_w);
			sleep(1);	
			break;
		case KEY_F(6):
			freq = constanttone ? ctonefreq : 800;
			pthread_join(cwthread, NULL);
			j = pthread_create(&cwthread, NULL, &morse, (void *) "TESTING");	
			thread_fail(j);
			break;
		case KEY_F(10):
		case KEY_F(3):
			status = 1;
			curs_set(1);
	}

	speed = initialspeed;

	attemptvalid = 1;
	if (f6 || fixspeed || unlimitedattempt) {
		attemptvalid = 0;	
	}
}

while (status == 3) {

	clear_display();

	wattron(mid_w,A_BOLD);
	mvwaddstr(mid_w,1,1, "Change Callsign Database");
	wattroff(mid_w,A_BOLD);
	mvwaddstr(mid_w,3,1, ".qcb files found (in "DESTDIR"/share/qrq/ and ~/.qrq/):");

	/* populate cblist */	
	find_callbases();
	/* selection dialog */
	select_callbase();
	
	wrefresh(mid_w);
	status = 2;	/* back to config menu */
}












} /* very outter loop */

	getch();
	endwin();
	delwin(top_w);
	delwin(bot_w);
	delwin(mid_w);
	delwin(right_w);
	getch();
	return 0;
}


/* reads a callsign etc. in *win at y/x and writes it to *line */

static int readline(WINDOW *win, int y, int x, char *line, int capitals) {
	int c;						/* character we read */
	int i=0;

	if (strlen(line) == 0) {p=0;}	/* cursor to start if no call in buffer */
	
	if (mode == 1) { 
		mvwaddstr(win,1,55,"INS");
	}
	else {
		mvwaddstr(win,1,55,"OVR");
	}

	mvwaddstr(win,y,x,line);
	wmove(win,y,x+p);
	wrefresh(win);
	curs_set(TRUE);
	
	while ((c = getch()) != '\n') {

		if (((c > 64 && c < 91) || (c > 96 && c < 123) || (c > 47 && c < 58)
					 || c == '/') && strlen(line) < 14) {
	
			line[strlen(line)+1]='\0';
			if (capitals) {
				c = toupper(c);
			}
			if (mode == 1) {						/* insert */
				for(i=strlen(line);i > p; i--) {	/* move all chars by one */
					line[i] = line[i-1];
				}
			} 
			line[p]=c;						/* insert into gap */
			p++;
		}
		else if ((c == KEY_BACKSPACE || c == 127 || c == 9)
						&& p != 0) {					/* BACKSPACE */
			for (i=p-1;i < strlen(line); i++) {
				line[i] =  line[i+1];
			}
			p--;
		}
		else if (c == KEY_DC && strlen(line) != 0) {		/* DELETE */ 
			p++;
			for (i=p-1;i < strlen(line); i++) {
				line[i] =  line[i+1];
			}
			p--;
		}
		else if (c == KEY_LEFT && p != 0) {
			p--;	
		}
		else if (c == KEY_RIGHT && p < strlen(line)) {
			p++;
		}
		else if (c == KEY_HOME) {
			p = 0;
		}
		else if (c == KEY_END) {
			p = strlen(line);
		}
		else if (c == KEY_IC) {						/* INS/OVR */
			if (mode == 1) { 
				mode = 0; 
				mvwaddstr(win,1,55,"OVR");
			}
			else {
				mode = 1;
				mvwaddstr(win,1,55,"INS");
			}
		}
		else if (c == KEY_PPAGE && callnr && !attemptvalid) {
			speed += 5;
			update_score();
			wrefresh(top_w);
		}
		else if (c == KEY_NPAGE && callnr && !attemptvalid) {
			if (speed > 20) speed -= 5;
			update_score();
			wrefresh(top_w);
		}
		else if (c == KEY_F(5)) {
			return 5;
		}
		else if (c == KEY_F(6)) {
			return 6;
		}
		else if (c == KEY_F(7)) {
			return 7;
		}
		else if (c == KEY_F(10)) {				/* quit */
			endwin();
			printf("Thanks for using 'qrq'!\nYou can submit your"
					" highscore to http://fkurz.net/ham/qrqtop.php\n");
			/* make sure that no more output is running, then send 73 & quit */
			pthread_join(cwthread, NULL);
			speed = 200; freq = 800;
			j = pthread_create(&cwthread, NULL, &morse, (void *) "73");	
			thread_fail(j);
			/* make sure the cw thread doesn't die with the main thread */
			pthread_exit(NULL);
			/* Exit the whole main thread */
			exit(0);
		}
		
		mvwaddstr(win,y,x,"                ");
		mvwaddstr(win,y,x,line);
		wmove(win,y,x+p);
		wrefresh(win);
	}
	curs_set(FALSE);
	return 0;
}

/* Read toplist and diplay first 10 entries */
static int display_toplist () {
	FILE * fh;
	int i=0;
	char tmp[35]="";
	if ((fh = fopen(tlfilename, "a+")) == NULL) {
		endwin();
		fprintf(stderr, "Couldn't read or create file '%s'!", tlfilename);
		exit(EXIT_FAILURE);
	}
	rewind(fh);				/* a+ -> end of file, we want the beginning */
	(void) fgets(tmp, 34, fh);		/* first line not used */
	while ((feof(fh) == 0) && i < 20) {
		i++;
		if (fgets(tmp, 34, fh) != NULL) {
			tmp[17]='\0';
			if (strstr(tmp, mycall)) {		/* highlight own call */
				wattron(right_w, A_BOLD);
			}
			mvwaddstr(right_w,i+2, 2, tmp);
			wattroff(right_w, A_BOLD);
		}
	}
	fclose(fh);
	wrefresh(right_w);
	return 0;
}

/* calculate score depending on number of errors and speed.
 * writes the correct call and entered call with highlighted errors to *output
 * and returns the score for this call
 *
 * in training modes (unlimited attempts, f6, fixed speed), no points.
 * */
static int calc_score (char * realcall, char * input, int spd, char * output) {
	int i,x,m=0;

	x = strlen(realcall);

	if (strcmp(input, realcall) == 0) {		 /* exact match! */
		output[0]='*';						/* * == OK, no mistake */
		output[1]='\0';	
		if (speed > maxspeed) {maxspeed = speed;}
		if (!fixspeed) speed += 10;
		if (attemptvalid) {
			return 2*x*spd;						/* score */
		}
		else {
			return 0;
		}
	}
	else {									/* assemble error string */
		errornr += 1;
		if (strlen(input) >= x) {x =  strlen(input);}
		for (i=0;i < x;i++) {
			if (realcall[i] != input[i]) {
				m++;								/* mistake! */
				output[i] = tolower(input[i]);		/* print as lower case */
			}
			else {
				output[i] = input[i];
			}
		}
		output[i]='\0';
		if ((speed > 29) && !fixspeed) {speed -= 10;}

		/* score when 1-3 mistakes was made */
		if ((m < 4) && attemptvalid) {
			return (int) (2*x*spd)/(5*m);
		}
		else {return 0;};
	}
}

/* print score, current speed and max speed to window */
static int update_score() {
	mvwaddstr(top_w,1,20, "Score:                         ");
	mvwaddstr(top_w,2,20, "Speed:     CpM/    WpM, Max:    /  ");
	if (attemptvalid) {
		mvwprintw(top_w, 1, 27, "%6d", score);	
	}
	else {
		mvwprintw(top_w, 1, 27, "[training mode]", score);	
	}
	mvwprintw(top_w, 2, 27, "%3d", speed);	
	mvwprintw(top_w, 2, 35, "%3d", speed/5);	
	mvwprintw(top_w, 2, 49, "%3d", maxspeed);	
	mvwprintw(top_w, 2, 54, "%3d", maxspeed/5);	
	wrefresh(top_w);
	return 0;
}

/* display the correct callsign and what the user entered, with mistakes
 * highlighted. */
static int show_error (char * realcall, char * wrongcall) {
	int x=2;
	int y = errornr;
	int i;

	/* Screen is full of errors. Remove them and start at the beginning */
	if (errornr == 31) {	
		for (i=1;i<16;i++) {
			mvwaddstr(mid_w,i,2,"                                        "
							 "          ");
		}
		errornr = y = 1;
	}

	/* Move to second column after 15 errors */	
	if (errornr > 15) {
		x=30; y = (errornr % 16)+1;
	}

	mvwprintw(mid_w,y,x, "%-13s %-13s", realcall, wrongcall);
	wrefresh(mid_w);		
	return 0;
}

/* clear error display */
static int clear_display() {
	int i;
	for (i=1;i<16;i++) {
		mvwprintw(mid_w,i,1,"                                 "
										"                        ");
	}
	return 0;
}

/* write entry into toplist at the right place 
 * going down from the top of the list until the score in the current line is
 * lower than the score made. then */
static int add_to_toplist(char * mycall, int score, int maxspeed) {
	FILE *fh;	
	char tmp[35]="";
	char line[35]="";
	char insertline[35]="DJ1YFK     36666 333 1111111111";		/* example */
						/* call       pts   max timestamp */
	int i=0;
	int pos = 0;		/* position where first score < our score appears */
	int timestamp = 0;

	/* For the training modes */
	if (score == 0) {
		return 0;
	}

	timestamp = (int) time(NULL);
	
	/* assemble scoreline to insert */
	sprintf(insertline, "%-10s%6d %3d %10d",mycall, score, maxspeed, timestamp);
	
	if ((fh = fopen(tlfilename, "r+")) == NULL) {
		endwin();
		perror("Unable to open toplist file 'toplist'!\n");
		exit(EXIT_FAILURE);
	}
	while ((feof(fh) == 0) && (fgets(line, 35, fh) != NULL)) {
		pos++;
		for (i=10;i<16;i++) {			/* extract the score to tmp*/
			tmp[i-10] = line[i];
		}
		tmp[i] = '\0';
		i = atoi(tmp);				/* i = score of current line */
		if (i < score){ 			/* insert score and shift lines below */
			score = i;				/* (ugly) */
			fseek(fh, -32L, SEEK_CUR);
			fputs(insertline, fh);
			strcpy(insertline, line);	/* actual line -> print one later */
		}
	}
	fputs(insertline, fh);
	if (score != i) {					/* last place. add newline! */
		fputs("\n",fh);
	}
	fclose(fh);	
	return 0;
}


/* Read config file 
 *
 * TODO contains too much copypasta. write proper function to parse a key=value
 *
 * */

static int read_config () {
	FILE *fh;
	char tmp[80]="";
	int i=0;
	int k=0;
	int line=0;

	if ((fh = fopen(rcfilename, "r")) == NULL) {
		endwin();
		fprintf(stderr, "Unable to open config file %s!\n", rcfilename);
		exit(EXIT_FAILURE);
	}
	while ((feof(fh) == 0) && (fgets(tmp, 80, fh) != NULL)) {
		i=0;
		line++;
		tmp[strlen(tmp)-1]='\0';
		/* find callsign, speed etc. 
		 * only allow if the lines are beginning at zero, so stuff can be
		 * commented out easily; return value if strstr must point to tmp*/
		if(tmp == strstr(tmp,"callsign=")) {
			while (isalnum(tmp[i] = toupper(tmp[9+i]))) {
				i++;
			}
			tmp[i]='\0';
			if (strlen(tmp) < 8) {				/* empty call allowed */
				strcpy(mycall,tmp);
				printw("  line  %2d: callsign: >%s<\n", line, mycall);
			}
			else {
				printw("  line  %2d: callsign: >%s< too long. "
								"Using default >%s<.\n", line, tmp, mycall);
			}
		}
		else if (tmp == strstr(tmp,"initialspeed=")) {
			while (isdigit(tmp[i] = tmp[13+i])) {
				i++;
			}
			tmp[i]='\0';
			i = atoi(tmp);
			if (i > 9) {
				initialspeed = speed = i;
				printw("  line  %2d: initial speed: %d\n", line, initialspeed);
			}
			else {
				printw("  line  %2d: initial speed: %d invalid (range: 10..oo)."
								" Using default %d.\n",line,  i, initialspeed);
			}
		}
		else if (tmp == strstr(tmp,"mincharspeed=")) {
			while (isdigit(tmp[i] = tmp[13+i])) {
				i++;
			}
			tmp[i]='\0';
			if ((i = atoi(tmp)) > 0) {
				mincharspeed = i;
				printw("  line  %2d: min.char.speed: %d\n", line, mincharspeed);
			} /* else ignore */
		}
		else if (tmp == strstr(tmp,"dspdevice=")) {
			while (isgraph(tmp[i] = tmp[10+i])) {
				i++;
			}
			tmp[i]='\0';
			if (strlen(tmp) > 1) {
				strcpy(dspdevice,tmp);
				printw("  line  %2d: dspdevice: >%s<\n", line, dspdevice);
			}
			else {
				printw("  line  %2d: dspdevice: >%s< invalid. "
								"Using default >%s<.\n", line, tmp, dspdevice);
			}
		}
		else if (tmp == strstr(tmp, "risetime=")) {
			while (isdigit(tmp[i] = tmp[9+i]) || ((tmp[i] = tmp[9+i])) == '-') {
				i++;	
			}
			tmp[i]='\0';
			edge = atoi(tmp);
			printw("  line  %2d: risetime: %d\n", line, edge);
		}
		else if (tmp == strstr(tmp, "waveform=")) {
			if (isdigit(tmp[i] = tmp[9+i])) {	/* read 1 char only */
				tmp[++i]='\0';
				waveform = atoi(tmp);
			}
			if ((waveform <= 3) && (waveform > 0)) {
				printw("  line  %2d: waveform: %d\n", line, waveform);
			}
			else {
				printw("  line  %2d: waveform: %d invalid. Using default.\n",
						 line, waveform);
				waveform = SINE;
			}
		}
		else if (tmp == strstr(tmp, "constanttone=")) {
			while (isdigit(tmp[i] = tmp[13+i])) {
				i++;    
			}
			tmp[i]='\0';
			k = 0; 
			k = atoi(tmp); 							/* constanttone */
			if ( (k*k) > 1) {
				printw("  line  %2d: constanttone: %s invalid. "
							"Using default %d.\n", line, tmp, constanttone);
			}
			else {
				constanttone = k ;
				printw("  line  %2d: constanttone: %d\n", line, constanttone);
			}
        }
        else if (tmp == strstr(tmp, "ctonefreq=")) {
			while (isdigit(tmp[i] = tmp[10+i])) {
            	i++;    
			}
			tmp[i]='\0';
			k = 0; 
			k = atoi(tmp);							/* ctonefreq */
			if ( (k > 1600) || (k < 100) ) {
				printw("  line  %2d: ctonefreq: %s invalid. "
					"Using default %d.\n", line, tmp, ctonefreq);
			}
			else {
				ctonefreq = k ;
				printw("  line  %2d: ctonefreq: %d\n", line, ctonefreq);
			}
		}
		else if (tmp == strstr(tmp, "f6=")) {
			f6=0;
			if (tmp[3] == '1') {
				f6 = 1;
			}
			printw("  line  %2d: unlimited f6: %s\n", line, (f6 ? "yes":"no"));
        }
		else if (tmp == strstr(tmp, "fixspeed=")) {
			fixspeed=0;
			if (tmp[9] == '1') {
				fixspeed = 1;
			}
			printw("  line  %2d: fixed speed:  %s\n", line, (fixspeed ? "yes":"no"));
        }
		else if (tmp == strstr(tmp, "unlimitedattempt=")) {
			unlimitedattempt=0;
			if (tmp[17] == '1') {
				unlimitedattempt= 1;
			}
			printw("  line  %2d: unlim. att.:  %s\n", line, (unlimitedattempt ? "yes":"no"));
        }
		else if (tmp == strstr(tmp,"callbase=")) {
			fprintf(stderr, "%d\n", i);
			while (isgraph(tmp[i] = tmp[9+i])) {
				i++;
			}
			tmp[i]='\0';
			if (strlen(tmp) > 1) {
				strcpy(cbfilename,tmp);
				printw("  line  %2d: callbase:  >%s<\n", line, cbfilename);
			}
			else {
				printw("  line  %2d: callbase:  >%s< invalid. "
								"Using default >%s<.\n", line, tmp, cbfilename);
			}
		}
	}

	printw("Finished reading qrqrc.\n");
	return 0;
}


static void *morse(void *arg) { 
	char * text = arg;
	int i,j;
	int c, fulldotlen, dotlen, dashlen, charspeed, farnsworth, fwdotlen;
	const char *code;

	/* opening the DSP device */
	dsp_fd = open_dsp(dspdevice);

	/* Some silence; otherwise the call starts right after pressing enter */
	tonegen(0, 11025, SILENCE);

	/* Farnsworth? */
	if (speed < mincharspeed) {
			charspeed = mincharspeed;
			farnsworth = 1;
			fwdotlen = (int) (samplerate * 6/speed);
	}
	else {
		charspeed = speed;
		farnsworth = 0;
	}

	/* speed is in LpM now, so we have to calculate the dot-length in
	 * milliseconds using the well-known formula  dotlength= 60/(wpm*50) 
	 * and then to samples */

	dotlen = (int) (samplerate * 6/charspeed);
	fulldotlen = dotlen;
	dashlen = 3*dotlen;
	
	/* rise == risetime in milliseconds, we need nr. of samples (ed) 
	 * these rise and fall-times are symmetrically added in front
	 * of and after the dots and dashes, reaching half of the amplitude exactly
	 * where the element is supposed to start/end; making the element spaces
	 * shorter. If they exceed half of the element space, the dots/dashes
	 * itself are shorted	to the point where it just reaches the maximum
	 * amplitude in the middle. (case B below) Beyond this point, the fall/rise
	 * times are shortened (case A below) */

	ed = (int) (samplerate * (edge/1000.0));

//	fprintf(stderr, "dotlen %d, dashlen %d, ed %d\n",dotlen, dashlen, ed);
	
	if (ed > dotlen) {	/* case A */
//		fprintf(stderr, "CASE A: Shorten Edges\n");
		ed = dotlen;
		dotlen = 1;
		dashlen -= ed;
	}
	else if (2*ed > dotlen) {	/* case B */
//		fprintf(stderr, "CASE A: Shorten Dot\n");
		dashlen = (dashlen + dotlen) - 2*ed;
		dotlen = 2*dotlen - 2*ed;
	}


	for (i = 0; i < strlen(text); i++) {
		c = text[i];
		if (isalpha(c)) {
			code = codetable[c-65];
		}
		else if (isdigit(c)) {
			code = codetable[c-22];
		}
		else if (c == '/') { 
			code = "-..-.";
		}
		else if (c == '+') {
			code = ".-.-.";
		}
		else {						/* not supposed to happen! */
			code = "..--..";
		}
		
		/* code is now available as string with - and . */

		for (j = 0; j < strlen(code) ; j++) {
			c = code[j];
			if (c == '.') {
				tonegen(freq, dotlen + ed, waveform);
				tonegen(0, fulldotlen - ed, SILENCE);
			}
			else {
				tonegen(freq, dashlen + ed, waveform);
				tonegen(0, fulldotlen - ed, SILENCE);
			}
		}
		if (farnsworth) {
			tonegen(0, 3*fwdotlen - fulldotlen, SILENCE);
		}
		else {
			tonegen(0, 2*fulldotlen, SILENCE);
		}
	}

	write_audio(dsp_fd, buffer, 88200);
	close_audio(dsp_fd);

	return NULL;
}

/* tonegen generates a sinus tone of frequency 'freq' and length 'len' (samples)
 * based on 'samplerate', 'rise' (risetime), 'fall' (falltime) */

static int tonegen (int freq, int len, int waveform) {
	int x=0;
	int out;
	double val=0;
	/* convert len from milliseconds to samples, determine rise/fall time */
	/* len = (int) (samplerate * (len/1000.0)); */

	for (x=0; x < len-1; x++) {
		switch (waveform) {
			case SINE:
				val = sin(2*PI*freq*x/samplerate);
				break;
			case SAWTOOTH:
				val=((1.0*freq*x/samplerate)-floor(1.0*freq*x/samplerate))-0.5;
				break;
			case SQUARE:
				val = ceil(sin(2*PI*freq*x/samplerate))-0.5;
				break;
			case SILENCE:
				val = 0;
		}


		if (x < ed) { val *= pow(sin(PI*x/(2.0*ed)),2); }	/* rising edge */

		if (x > (len-ed)) {								/* falling edge */
				val *= pow(sin(2*PI*(x-(len-ed)+ed)/(4*ed)),2); 
		}
		
		//fprintf(stderr, "%f\n", val);
		out = (int) (val * 32500.0);
		out = out + (out<<16);				/* add second channel */
		write_audio(dsp_fd, &out, sizeof(out));
	}
	return 0;
}

/* TODO: Remove copypasta, write small functions to do it */

static int save_config () {
	FILE *fh;
	char tmp[80]="";
	int i;
	
	if ((fh = fopen(rcfilename, "r+")) == NULL) {
		endwin();
		fprintf(stderr, "Unable to open config file '%s'!\n", rcfilename);
		exit(EXIT_FAILURE);
	}

	while ((feof(fh) == 0) && (fgets(tmp, 80, fh) != NULL)) {
		tmp[strlen(tmp)-1]='\0';
		i = strlen(tmp);
		if (strstr(tmp,"initialspeed=")) {
			fseek(fh, -(i+1), SEEK_CUR);	/* go to beginning of the line */
			snprintf(tmp, i+1, "initialspeed=%d ", initialspeed);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"constanttone=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "constanttone=%d ", constanttone);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"mincharspeed=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "mincharspeed=%d ", mincharspeed);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"ctonefreq=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "ctonefreq=%d ", ctonefreq);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp, "risetime=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "risetime=%d ", edge);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"callsign=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "callsign=%-7s ", mycall);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"dspdevice=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "dspdevice=%s ", dspdevice);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"waveform=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "waveform=%d ", waveform);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"f6=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "f6=%d ", f6);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"fixspeed=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "fixspeed=%d ", fixspeed);
			fputs(tmp, fh);	
		}
		else if (strstr(tmp,"unlimitedattempt=")) {
			fseek(fh, -(i+1), SEEK_CUR);
			snprintf(tmp, i+1, "unlimitedattempt=%d ", unlimitedattempt);
			fputs(tmp, fh);	
		}
	}
	return 0;
}
		
static void thread_fail (int j) {
	if (j) {
		endwin();
		perror("Error: Unable to create cwthread!\n");
		exit(EXIT_FAILURE);
	}
}

/* Add timestamps to toplist file if not there yet */
static int check_toplist () {
	char line[80]="";
	char tmp[80]="";
	FILE *fh;
	FILE *fh2;

	if ((fh = fopen(tlfilename, "r+")) == NULL) {
		endwin();
		perror("Unable to open toplist file 'toplist'!\n");
		exit(EXIT_FAILURE);
	}

	fgets(tmp, 35, fh);
	
	rewind(fh);
	
	if (strlen(tmp) == 21) {
			printw("Toplist file in old format. Converting...");
			strcpy(tmp, "cp -f ");
			strcat(tmp, tlfilename);
			strcat(tmp, " /tmp/qrq-toplist");
			if (system(tmp)) {
					printw("Failed to copy to /tmp/qrq-toplist\n");
					getch();
					endwin();
					exit(EXIT_FAILURE);
			}

			fh2 = fopen("/tmp/qrq-toplist", "r+"); 		/* should work ... */

			while ((feof(fh2) == 0) && (fgets(line, 35, fh2) != NULL)) {
					line[20]=' ';
					strcpy(tmp, line);
					strcat(tmp, "1181234567\n");
					fputs(tmp, fh);
			}
			
			printw(" done!\n");
	
			fclose(fh2);

	}

	fclose(fh);

	return 0;
}



/* See where our files are. We need 'callbase.qcb', 'qrqrc' and 'toplist'.
 * The can be: 
 * 1) In the current directory -> use them
 * 2) In ~/.qrq/  -> use toplist and qrqrc from there and callbase from
 *    DESTDIR/share/qrq/
 * 3) in DESTDIR/share/qrq/ -> create ~/.qrq/ and copy qrqrc and toplist
 *    there.
 * 4) Nowhere --> Exit.*/
static int find_files () {

	FILE *fh;
	const char *homedir = NULL;
	char tmp_rcfilename[1024] = "";
	char tmp_tlfilename[1024] = "";
	char tmp_cbfilename[1024] = "";

	printw("\nChecking for necessary files (qrqrc, toplist, callbase)...\n");
	
	if (((fh = fopen("qrqrc", "r")) == NULL) ||
		((fh = fopen("toplist", "r")) == NULL) ||
		((fh = fopen("callbase.qcb", "r")) == NULL)) {
		
		homedir = getenv("HOME");
		
		printw("... not found in current directory. Checking "
						"%s/.qrq/...\n", homedir);
		refresh();
		strcat(rcfilename, homedir);
		strcat(rcfilename, "/.qrq/qrqrc");
	
		/* check if there is ~/.qrq/qrqrc. If it's there, it's safe to assume
		 * that toplist also exists at the same place and callbase exists in
		 * DESTDIR/share/qrq/. */

		if ((fh = fopen(rcfilename, "r")) == NULL ) {
			printw("... not found in %s/.qrq/. Checking %s/share/qrq..."
							"\n", homedir, destdir);
			/* check for the files in DESTDIR/share/qrq/. if exists, copy 
			 * qrqrc and toplist to ~/.qrq/  */

			strcpy(tmp_rcfilename, destdir);
			strcat(tmp_rcfilename, "/share/qrq/qrqrc");
			strcpy(tmp_tlfilename, destdir);
			strcat(tmp_tlfilename, "/share/qrq/toplist");
			strcpy(tmp_cbfilename, destdir);
			strcat(tmp_cbfilename, "/share/qrq/callbase.qcb");

			if (((fh = fopen(tmp_rcfilename, "r")) == NULL) ||
				((fh = fopen(tmp_tlfilename, "r")) == NULL) ||
				 ((fh = fopen(tmp_cbfilename, "r")) == NULL)) {
				printw("Sorry: Couldn't find 'qrqrc', 'toplist' and"
			   			" 'callbase.qcb' anywhere. Exit.\n");
				getch();
				endwin();
				exit(EXIT_FAILURE);
			}
			else {			/* finally found it in DESTDIR/share/qrq/ ! */
				/* abusing rcfilename here for something else temporarily */
				printw("Found files in %s/share/qrq/."
						"\nCreating directory %s/.qrq/ and copy qrqrc and"
						" toplist there.\n", destdir, homedir);
				strcpy(rcfilename, homedir);
				strcat(rcfilename, "/.qrq/");
				j = mkdir(rcfilename,  0777);
				if (j && (errno != EEXIST)) {
					printw("Failed to create %s! Exit.\n", rcfilename);
					getch();
					endwin();
					exit(EXIT_FAILURE);
				}
				/* OK, now we created the directory, we can read in
				 * DESTDIR/local/, so I assume copying files won't cause any
				 * problem, with system()... */

				strcpy(rcfilename, "install -m 644 ");
				strcat(rcfilename, tmp_tlfilename);
				strcat(rcfilename, " ");
				strcat(rcfilename, homedir);
				strcat(rcfilename, "/.qrq/ 2> /dev/null");
				if (system(rcfilename)) {
					printw("Failed to copy toplist file: %s\n", rcfilename);
					getch();
					endwin();
					exit(EXIT_FAILURE);
				}
				strcpy(rcfilename, "install -m 644 ");
				strcat(rcfilename, tmp_rcfilename);
				strcat(rcfilename, " ");
				strcat(rcfilename, homedir);
				strcat(rcfilename, "/.qrq/ 2> /dev/null");
				if (system(rcfilename)) {
					printw("Failed to copy qrqrc file: %s\n", rcfilename);
					getch();
					endwin();
					exit(EXIT_FAILURE);
				}
				printw("Files copied. You might want to edit "
						"qrqrc according to your needs.\n", homedir);
				strcpy(rcfilename, homedir);
				strcat(rcfilename, "/.qrq/qrqrc");
				strcpy(tlfilename, homedir);
				strcat(tlfilename, "/.qrq/toplist");
				strcpy(cbfilename, tmp_cbfilename);
			} /* found in DESTDIR/share/qrq/ */
		}
		else {
			printw("... found files in %s/.qrq/.\n", homedir);
			strcat(tlfilename, homedir);
			strcat(tlfilename, "/.qrq/toplist");
			strcpy(cbfilename, destdir);
			strcat(cbfilename, "/share/qrq/callbase.qcb");
		}
	}
	else {
		printw("... found in current directory.\n");
		strcpy(rcfilename, "qrqrc");
		strcpy(tlfilename, "toplist");
		strcpy(cbfilename, "callbase.qcb");
	}
	refresh();
	fclose(fh);

	return 0;
}


static int statistics () {
		char line[80]="";

		int time = 0;
		int score = 0;
		int count= 0;

		FILE *fh;
		FILE *fh2;
		
		if ((fh = fopen(tlfilename, "r")) == NULL) {
				fprintf(stderr, "Unable to open toplist.");
				exit(0);
		}
		
		if ((fh2 = fopen("/tmp/qrq-plot", "w+")) == NULL) {
				fprintf(stderr, "Unable to open /tmp/qrq-plot.");
				exit(0);
		}

		fprintf(fh2, "set yrange [0:]\nset xlabel \"Date/Time\"\n"
					"set title \"QRQ scores for %s. Press 'q' to "
					"close this window.\"\n"
					"set ylabel \"Score\"\nset xdata time\nset "
					" timefmt \"%%s\"\n "
					"plot \"-\" using 1:2 title \"\"\n", mycall);

		while ((feof(fh) == 0) && (fgets(line, 80, fh) != NULL)) {
				if ((strstr(line, mycall) != NULL)) {
					count++;
					sscanf(line, "%*s %d %*d %d", &score, &time);
					fprintf(fh2, "%d %d\n", time, score);
				}
		}

		if (!count) {
			fprintf(fh2, "0 0\n");
		}
		
		fprintf(fh2, "end\npause 10000");

		fclose(fh);
		fclose(fh2);

		system("gnuplot /tmp/qrq-plot 2> /dev/null &");
	return 0;
}


int read_callbase () {
	FILE *fh;
	int c,i;
	int maxlen=0;
	char tmp[80] = "";
	int nr=0;

	if ((fh = fopen(cbfilename, "r")) == NULL) {
		endwin();
		fprintf(stderr, "Error: Couldn't read callsign database ('%s')!\n",
						cbfilename);
		exit(EXIT_FAILURE);
	}

	/* count the lines/calls and lengths */
	i=0;
	while ((c = getc(fh)) != EOF) {
		i++;
		if (c == '\n') {
			nr++;
			maxlen = (i > maxlen) ? i : maxlen;
			i = 0;
		}
	}
	maxlen++;

	if (!nr) {
		endwin();
		printf("\nError: Callsign database empty, no calls read. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	/* allocate memory for calls array, free if needed */

	free(calls);

	if ((calls = (char **) malloc( (size_t) sizeof(char *)*nr )) == NULL) {
		fprintf(stderr, "Error: Couldn't allocate %d bytes!\n", 
						(int) sizeof(char)*nr);
		exit(EXIT_FAILURE);
	}
	
	/* Allocate each element of the array with size maxlen */
	for (c=0; c < nr; c++) {
		if ((calls[c] = (char *) malloc (maxlen * sizeof(char))) == NULL) {
			fprintf(stderr, "Error: Couldn't allocate %d bytes!\n", maxlen);
			exit(EXIT_FAILURE);
		}
	}

	rewind(fh);
	
	nr=0;
	while (fgets(tmp,maxlen,fh) != NULL) {
		for (i = 0; i < strlen(tmp); i++) {
				tmp[i] = toupper(tmp[i]);
		}
		tmp[i-1]='\0';				/* remove newline */
		if (tmp[i-2] == '\r') {		/* also for DOS files */
			tmp[i-2] = '\0';
		}
		strcpy(calls[nr],tmp);
		nr++;
		if (nr == c) 			/* may happen if call file corrupted */
				break;
	}
	fclose(fh);


	return nr;

}

void find_callbases () {
	DIR *dir;
	struct dirent *dp;
	char tmp[PATH_MAX];
	char path[3][PATH_MAX];
	int i=0,j=0,k=0;

	strcpy(path[0], getenv("PWD"));
	strcat(path[0], "/");
	strcpy(path[1], getenv("HOME"));
	strcat(path[1], "/.qrq/");
	strcpy(path[2], DESTDIR"/share/qrq/");

	for (i=0; i < 100; i++) {
		strcpy(cblist[i], "");
	}

	/* foreach paths...  */
	for (k = 0; k < 3; k++) {

		if (!(dir = opendir(path[k]))) {
			continue;
		}
	
		while ((dp = readdir(dir))) {
			strcpy(tmp, dp->d_name);
			i = strlen(tmp);
			/* find *.qcb files ...  */
			if (i>4 && tmp[i-1] == 'b' && tmp[i-2] == 'c' && tmp[i-3] == 'q') {
				strcpy(cblist[j], path[k]);
				strcat(cblist[j], tmp);
				j++;
			}
		}
	} /* for paths */
}



void select_callbase () {
	int i = 0, j = 0, k = 0;
	int c = 0;		/* cursor position   */
	int p = 0;		/* page a 10 entries */


	curs_set(FALSE);

	/* count files */
	while (strcmp(cblist[i], "")) i++;

	if (!i) {
		mvwprintw(mid_w,10,4, "No qcb-files found!");
		wrefresh(mid_w);
		sleep(1);
		return;
	}

	/* loop for key unput */
	while (1) {

	/* cls */
	for (j = 5; j < 16; j++) {
			mvwprintw(mid_w,j,2, "                                         ");
	}

	/* display 10 files, highlight cursor position */
	for (j = p*10; j < (p+1)*10; j++) {
		if (j <= i) {
			mvwprintw(mid_w,5+(j - p*10 ),2, "  %s       ", cblist[j]);
		}
		if (c == j) {						/* cursor */
			mvwprintw(mid_w,5+(j - p*10),2, ">");
		}
	}
	
	wrefresh(mid_w);

	k = getch();

	switch ((int) k) {
		case KEY_UP:
			c = (c > 0) ? (c-1) : c;
			if (!((c+1) % 10)) {	/* scroll down */
				p = (p > 0) ? (p-1) : p;
			}
			break;
		case KEY_DOWN:
			c = (c < (i-1)) ? (c+1) : c;
			if (!(c % 10)) {	/* scroll down */
				p++;
			}
			break;
		case '\n':
			strcpy(cbfilename, cblist[c]);
			nrofcalls = read_callbase();
			return;	
			break;
	}

	wrefresh(mid_w);

	} /* while 1 */

	curs_set(TRUE);

}



void help () {
		printf("qrq v%s  (c) 2006-2010 Fabian Kurz, DJ1YFK. "
					"http://fkurz.net/ham/qrq.html\n", VERSION);
		printf("High speed morse telegraphy trainer, similar to"
					" RUFZ.\n\n");
		printf("This is free software, and you are welcome to" 
						" redistribute it\n");
		printf("under certain conditions (see COPYING).\n\n");
		printf("Start 'qrq' without any command line arguments for normal"
					" operation.\n");
		exit(0);
}


/* vim: noai:ts=4:sw=4 
*/
