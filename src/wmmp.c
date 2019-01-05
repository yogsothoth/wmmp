/*
 *    WMmp - Music Player Daemon client dockapp for Window Maker
 *    Copyright (c) 2017 Nicolas Herry <nicolasherry@gmail.com>
 *    Copyright (C) 2002  Thomas Nemeth <tnemeth@free.fr>
 *
 *    Based on work by Seiichi SATO <ssato@sh.rim.or.jp>
 *    Copyright (C) 2001,2002  Seiichi SATO <ssato@sh.rim.or.jp>
 *    and on work by Mark Staggs <me@markstaggs.net>
 *    Copyright (C) 2002  Mark Staggs <me@markstaggs.net>

 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.

 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include <ctype.h>
#include "dockapp.h"
#include "backlight_on.xpm"
#include "backlight_off.xpm"
#include "parts.xpm"
#include "mpd_func.h"
#include "mouse_regions.h"

#ifdef linux
#include <sys/stat.h>
#endif

#define FREE(data) {if (data) free (data); data = NULL;}

#define SIZE	    58
#define MAXSTRLEN   512
#define WINDOWED_BG ". c #AEAAAE"
#define MAX_HISTORY 16
#define CPUNUM_NONE -1
#define SEPARATOR " ** "

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "6600"
#define PACKAGE "WMmp"
#define VERSION "0.12.5"

typedef struct MpdInfos {
	int shuffling;
	int playing;
	int paused;
	int repeat;
	int running;
	int volume;
	int time;
	int track;
	char *title;
} MpdInfos;

typedef enum { LIGHTOFF, LIGHTON } light;

Pixmap pixmap;
Pixmap backdrop_on;
Pixmap backdrop_off;
Pixmap parts;
Pixmap mask;
int but_stat = -1;
int left_down = 0;
int triangle_loc = 0;
float title_pos = 0;
int motion = 0;
static char	*display_name     = "";
static char	*light_color      = NULL;	/* back-light color */
static unsigned update_interval   = 1;
static light    backlight         = LIGHTOFF;
static MpdInfos cur_mpd_infos;


#ifdef linux
# ifndef ACPI_32_BIT_SUPPORT
#  define ACPI_32_BIT_SUPPORT      0x0002
# endif
#endif


/* prototypes */
static void update();
static void switch_light();
static void parse_arguments(int argc, char **argv);
static void print_help(char *prog);
static void init_buttons();
static void button_down(int w,int x, int y);
static void unhighlight_button(int x, int y, int b);
static void track_motion(int x, int y);
static void update_slider(int x);
static void highlight_button(int z);
static void update_triangle();
static void update_track(int trk);
static void update_time(int tme);
static void update_title(char *name);

int count;


int main(int argc, char **argv) {
    XEvent   event;
    XpmColorSymbol colors[2] = { {"Back0", NULL, 0}, {"Back1", NULL, 0} };
    int      ncolor = 0;
    struct   sigaction sa;
    char * test;
    char * port;

    sa.sa_handler = SIG_IGN;
#ifdef SA_NOCLDWAIT
    sa.sa_flags = SA_NOCLDWAIT;
#else
    sa.sa_flags = 0;
#endif
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    /* Parse CommandLine */
    parse_arguments(argc, argv);

    if(!(mpd_host = getenv("MPD_HOST"))) {
	    mpd_host = DEFAULT_HOST;
    }
    mpd_host = strdup(mpd_host);

    if(!(port = getenv("MPD_PORT"))) {
	    port = DEFAULT_PORT;
    }

    mpd_port = strtol(port,&test,10);
    if(*test!='\0' || mpd_port<0) {
	    fprintf(stderr,"MPD_PORT \"%s\" is not a postiive integer\n",port);
	    exit(-1);
    }
    
    /* parse password and host */
    {
            char * ret = strstr(mpd_host,"@");
            int len = ret-mpd_host;

            if(ret) {
	           *ret = '\0';
                   if(len) mpd_password = strdup(mpd_host);
                   mpd_host = ret+1;
            }
    }

    /* Initialize Application */
    dockapp_open_window(display_name, PACKAGE, SIZE, SIZE, argc, argv);
    dockapp_set_eventmask();

    if (light_color) {
        colors[0].pixel = dockapp_getcolor(light_color);
        colors[1].pixel = dockapp_blendedcolor(light_color, -24, -24, -24, 1.0);
        ncolor = 2;
    }

    /* change raw xpm data to pixmap */
    if (dockapp_iswindowed)
        backlight_on_xpm[1] = backlight_off_xpm[1] = WINDOWED_BG;

    if (!dockapp_xpm2pixmap(backlight_on_xpm, &backdrop_on, &mask, colors, ncolor)) {
        fprintf(stderr, "Error initializing back-lit background image.\n");
        exit(1);
    }
    if (!dockapp_xpm2pixmap(backlight_off_xpm, &backdrop_off, NULL, NULL, 0)) {
        fprintf(stderr, "Error initializing background image.\n");
        exit(1);
    }
    if (!dockapp_xpm2pixmap(parts_xpm, &parts, NULL, colors, ncolor)) {
        fprintf(stderr, "Error initializing parts image.\n");
        exit(1);
    }

    /* shape window */
    if (!dockapp_iswindowed) dockapp_setshape(mask, 0, 0);
    if (mask) XFreePixmap(display, mask);

    /* pixmap : draw area */
    pixmap = dockapp_XCreatePixmap(SIZE, SIZE);

    /* Initialize pixmap */
    if (backlight == LIGHTON) 
        dockapp_copyarea(backdrop_on, pixmap, 0, 0, SIZE, SIZE, 0, 0);
    else
        dockapp_copyarea(backdrop_off, pixmap, 0, 0, SIZE, SIZE, 0, 0);

    dockapp_set_background(pixmap);
    dockapp_show();
    init_buttons();

    update();

    /* Main loop */
    while (1) {
        if(dockapp_nextevent_or_timeout(&event, update_interval*200)) {
            /* Next Event */
            switch (event.type) {
                case ButtonPress:
				button_down(event.xbutton.button,event.xbutton.x, event.xbutton.y);
				break;
		case ButtonRelease:
				left_down = 0;
			        unhighlight_button(event.xbutton.x, event.xbutton.y, 0);
			       break;
		case MotionNotify:
			       track_motion(event.xmotion.x, event.xmotion.y);
			       break;
                default: break;
		    }
            }
	else {
		if((!motion) && (!left_down)) {
			update();
		}
		else motion = 0;
	}
	}
    return 0;
}

static void track_motion(int x, int y) {

	if(cur_mpd_infos.running) {
	if(left_down) {
		motion = 1;
		if ((x > 3) && (x < 55) && (y > 41) && (y < 45)) {
			fprintf(stderr,"%d %d\n",x,y);
			MpdSetVolume((x-4)*2);
			update_slider(x-4);
		}
	}
    }
}

static void unhighlight_button(int x, int y, int b) {
	MpdInfos *mpd = &cur_mpd_infos;

	int z;
	int q;
	
	q = 25;
	

	if( backlight == LIGHTON) 
		q = 0;

        mpd->running = MpdStatus();			       

	/*if(left_down && !motion) 
		left_down = 0;
		*/
			

	if (!b)
	    z = CheckMouseRegion(x, y);
	else z = b;

	if(cur_mpd_infos.running) {

		switch (z) {

		 	case 0:
				if (cur_mpd_infos.repeat != 1)
					dockapp_copyarea(parts, pixmap, 135, 81, 10, 6, 44, 5);
				break;
			case 1:
				if (cur_mpd_infos.shuffling != 1)
				dockapp_copyarea(parts, pixmap, 148, 81, 10, 6, 44, 12);
				break;
			case 2:
				dockapp_copyarea(parts, pixmap, 105, q, 13, 8, 4, 32);
				break;
			case 3:
				dockapp_copyarea(parts, pixmap, 118, q, 13, 8, 17, 32);
				break;
			case 4:
				dockapp_copyarea(parts, pixmap, 130, q, 13, 8, 29, 32);
				break;
			case 5:
				dockapp_copyarea(parts, pixmap, 142, q, 13, 8, 41, 32);
				break;
			case 6:
				dockapp_copyarea(parts, pixmap, 105, q+13, 9, 9, 4, 45);
				break;
			case 7:
				if (!cur_mpd_infos.playing) {
					triangle_loc = 3;
				dockapp_copyarea(parts, pixmap, 114, q+13, 21, 9, 13, 45);
				}
				break;
			case 8:
				if (!cur_mpd_infos.paused)
				dockapp_copyarea(parts, pixmap, 135, q+13, 10, 9, 34, 45);
				break;
			case 9:
				dockapp_copyarea(parts, pixmap, 145, q+13, 10, 9, 44, 45);
				break;
		}
	}
    dockapp_copy2window(pixmap);
}

void highlight_button(int z) {

		switch (z) {
			case 0: 
				dockapp_copyarea(parts, pixmap, 109, 81, 10, 6, 44, 5);
				break;
			case 1:
				dockapp_copyarea(parts, pixmap, 122, 81, 10, 6, 44, 12);
				break;
			case 2:
				dockapp_copyarea(parts, pixmap, 0, 49, 13, 8, 4, 32);
				break;
			case 3:
				dockapp_copyarea(parts, pixmap, 13, 49, 13, 8, 17, 32);
				break;
			case 4:
				dockapp_copyarea(parts, pixmap, 25, 49, 13, 8, 29, 32);
				break;
			case 5:
				dockapp_copyarea(parts, pixmap, 37, 49, 13, 8, 41, 32);
				break;
			case 6:
				dockapp_copyarea(parts, pixmap, 0, 62, 9, 9, 4, 45);
				break;
			case 7:
					dockapp_copyarea(parts, pixmap, 9, 62, 21, 9, 13, 45);
				break;
			case 8:
					dockapp_copyarea(parts, pixmap, 30, 62, 10, 9, 34, 45);
					unhighlight_button(0,0,7);
				break;
			case 9:
				dockapp_copyarea(parts, pixmap, 40, 62, 10, 9, 44, 45);
				unhighlight_button(0,0,7);
				unhighlight_button(0,0,8);
				break;
		}
    dockapp_copy2window(pixmap);
        }

static void button_down(int w,int x, int y) {

	MpdInfos *mpd = &cur_mpd_infos;
	int z;

	if(w == 1) left_down = 1;
	else if (w == 3) switch_light();
        
	mpd->running = MpdStatus();			       

	z = CheckMouseRegion(x, y);

	if(cur_mpd_infos.running) {

            	if (w == 4) { /* up */
                	int tmp = mpd->volume + 5;
			if(tmp>100) tmp = 100;
                	MpdSetVolume(tmp);
                	update_slider(tmp / 2);
            	} else if (w == 5) { /* down */
                	int tmp = mpd->volume - 5;
			if(tmp<0) tmp = 0;
                	MpdSetVolume(tmp);
                	update_slider(tmp / 2);
            	} else {
			switch (z) {
				case 0: MpdToggleRepeat();
					if(!cur_mpd_infos.repeat) {
						highlight_button(0);
						mpd->repeat = 1;
					}
					else {
						mpd->repeat = 0;
						unhighlight_button(x,y,0);
					}
	
					break;
			
				case 1:
					MpdToggleRandom();
					if(!cur_mpd_infos.shuffling) {
						highlight_button(1);
						mpd->shuffling = 1;
					}
					else {
						mpd->shuffling = 0;
						unhighlight_button(0,0,1);
					}
					break;
				case 2:
					highlight_button(2);
					MpdPrev();
					break;
				case 3:
					highlight_button(3);
					MpdFastr();
					break;
				case 4:
					highlight_button(4);
					MpdFastf();
					break;
				case 5:
					highlight_button(5);
					MpdNext();
					break;
				case 6:
					highlight_button(6);
					MpdEject();
					break;
				case 7:
					if (!cur_mpd_infos.playing) {
						MpdPlay();
						highlight_button(7);
						mpd->playing = 1;
					}
					break;
				case 8:
					if (!cur_mpd_infos.paused) {
						highlight_button(8);
						mpd->paused = 1;
						if(cur_mpd_infos.playing) {
							mpd->playing = 0;
							unhighlight_button(0,0,7);
							mpd->playing = 1;
						}
						MpdPause();
					}
					else {
						mpd->paused = 0;
						if(cur_mpd_infos.playing)
							highlight_button(7);
						MpdPause();
					}
					break;
				case 9:
					highlight_button(9);
					mpd->playing = 0;
					mpd->paused = 0;
					unhighlight_button(0,0,7);
					unhighlight_button(0,0,8);
					MpdStop();
					break;
			}
		}
	}
	else 
    		dockapp_copy2window(pixmap);
}

static void init_buttons() {

	MpdInfos *mpd = &cur_mpd_infos;

	AddMouseRegion(0,44,5,54,10);
	AddMouseRegion(1,44,12,54,17);
	AddMouseRegion(2,4,33,16,40);
	AddMouseRegion(3,17,33,28,40);
	AddMouseRegion(4,30,33,40,40);
	AddMouseRegion(5,42,33,53,40);
	AddMouseRegion(6,4,46,12,54);
	AddMouseRegion(7,13,46,33,54);
	AddMouseRegion(8,35,46,44,54);
	AddMouseRegion(9,45,46,53,54);

	mpd->shuffling = 0;
	mpd->playing = 0;
	mpd->paused = 0;
	mpd->repeat = 0;
	mpd->volume = MpdGetVolume();
	mpd->running = MpdStatus();
	mpd->title = MpdGetTitle();
	if(cur_mpd_infos.running)
		update_slider(cur_mpd_infos.volume/2);
	else
		update_slider(25);
}

static void update_triangle() {

	int y;
	int x;

	y = 56;
	x = 52;

	if(backlight == LIGHTON) y = 63;

	dockapp_copyarea(parts,pixmap,(x + (triangle_loc * 8)),y,6,6,34,20);
	triangle_loc++;
	dockapp_copy2window(pixmap);

	if ( triangle_loc > 2) triangle_loc = 0;
}

static void update_slider(int x) {

	int y;
	MpdInfos *mpd = &cur_mpd_infos;

	y = 57;
        mpd->volume = x*2;

	if(backlight == LIGHTON) y = 67;
	if (x == 50) x = 49;
	dockapp_copyarea(parts,pixmap,105,62,x,4,5,41);
	/*fprintf(stderr,"X = %d\n",x);*/
	dockapp_copyarea(parts,pixmap,105,y,(49 - x),4,x+5,41);
	dockapp_copy2window(pixmap);
}

static void update_track(int trk) {
	
	char posstr[16];
	char *p = posstr;
	int i = 1;
	int y;

	y = 81;

	if(backlight == LIGHTON)
		y = 98;

	trk++;

	if (trk > 99) trk = 0;
	sprintf(posstr, "%02d",trk);

	for (;i<3;i++)
	{
		dockapp_copyarea(parts,pixmap,(*p - '0')*6 ,y,6,7,(i*6)+38,20);
		p++;

	}
}

static void update_time(int tme) {

	char timestr[16];
	char *p = timestr;
	int i = 0;
	int y;
	int x;

	y = 40;
	x = 0;

	if(backlight == LIGHTON)
		x = 50;

	if (tme < 6000)
	{
		sprintf(timestr,"%02d%02d",tme / 60, tme % 60);
	} else {
		if (tme < 360000)
		{
			sprintf(timestr, "%02d%02d", tme / 3600, tme % 3600 / 60);
		} else {
			sprintf(timestr, "%02d%02d", 0 , 0);
		}
	}

	for (;i < 4; i++)
	{
		dockapp_copyarea(parts,pixmap,(*p - '0')*5 + x ,y, 5, 9,i<2 ?(i*6)+5:(i*6)+7,18);
		p++;
	}
}

static void update_title(char *name)
{
	int i,len,max,pos;
	char *title = NULL;
	char c = ' ';

	int y;

	y = 0;

	if (backlight == LIGHTON)
		y = 17;

	if (name == NULL)
		return;

	pos = title_pos;

	len = strlen(name);

	if (len < 6)
	{
		max = len;
		title = strdup(name);
	} else {
		max = 6;
		title = malloc(strlen(name)+strlen(SEPARATOR)+1);
		sprintf(title,"%s%s", name,SEPARATOR);
		len = strlen(title);
	}

	if ( pos >= len)
	{
		title_pos = 0;
		pos = 0;
	}

	for (i=0; i<max; i++)
	{
		{
			int tmp = i+pos;

			if(tmp < len)
				c = toupper(title[tmp]);
			else
				c = toupper(title[(tmp - len)%6]);
		}

		if (isalpha(c))
			dockapp_copyarea(parts,pixmap,(c - 'A')*6 ,72 + y, 6, 8,(i*6)+7,7);
		else if (isdigit(c))
			dockapp_copyarea(parts,pixmap,(c - '0')*6 ,81 + y, 6, 8,(i*6)+7,7);
		else if (isspace(c))
			dockapp_copyarea(parts,pixmap,60 ,81 + y, 6, 8,(i*6)+7,7);
		else {
			switch(c)
			{
				case '-':
					dockapp_copyarea(parts,pixmap,66 ,81 + y, 6, 8,(i*6)+7,7);
					break;
				case '.':
					dockapp_copyarea(parts,pixmap,72 ,81 + y, 6, 8,(i*6)+7,7);
					break;
				case 0x27:
					dockapp_copyarea(parts,pixmap,79 ,81 + y, 6, 8,(i*6)+7,7);
					break;
				case '(':
					dockapp_copyarea(parts,pixmap,84 ,81 + y, 6, 8,(i*6)+7,7);
					break;
				case ')':
					dockapp_copyarea(parts,pixmap,90 ,81 + y, 6, 8,(i*6)+7,7);
					break;
				case '*':
					dockapp_copyarea(parts,pixmap,96 ,81 + y, 6, 8,(i*6)+7,7);
					break;
				case '/':
					dockapp_copyarea(parts,pixmap,103 ,81 + y, 6, 8,(i*6)+7,7);
					break;
				default:
					dockapp_copyarea(parts,pixmap,60 ,81 + y, 6, 8,(i*6)+7,7);
					break;
			}
		}
	}
	for (;i<6; i++)
	{
		dockapp_copyarea(parts,pixmap,60 ,81 + y, 6, 8,(i*6)+7,7);
	}
	if (len > 6)
		title_pos = title_pos + 0.5;
	free(title);
}

/* called by timer */
static void update() {
    MpdInfos *mpd = &cur_mpd_infos;

    mpd->running = MpdStatus();
    mpd->playing = MpdPlayStatus();
    mpd->paused = MpdPauseStatus();
    mpd->shuffling = MpdRandomStatus();
    mpd->repeat = MpdRepeatStatus();
    mpd->track = MpdGetTrack();
    mpd->time = MpdGetTime();
    mpd->title = MpdGetTitle();

    if(cur_mpd_infos.running) {
	    if(cur_mpd_infos.shuffling) 
		    highlight_button(1);
	    else
		    unhighlight_button(0,0,1);
	    if(cur_mpd_infos.repeat)
		    highlight_button(0);
	    else
		    unhighlight_button(48,8,0);
    	if(!cur_mpd_infos.paused) {
    		unhighlight_button(0,0,8);
    		if(!cur_mpd_infos.playing) {
    			triangle_loc = 3;
    			unhighlight_button(0,0,7);
    		}
    		else 
    			highlight_button(7);
    	}
   	else {
   		mpd->playing = 0;
   		unhighlight_button(0,0,7);
   		highlight_button(8);
   	}
  	update_slider(MpdGetVolume()/2);
        update_triangle();
	update_title(cur_mpd_infos.title);
	update_track(cur_mpd_infos.track);
	update_time(cur_mpd_infos.time);
    }
    else {
	    if (backlight == LIGHTON)
		 dockapp_copyarea(backdrop_on, pixmap, 0, 0, 58, 58, 0, 0);
	        else
		 dockapp_copyarea(backdrop_off, pixmap, 0, 0, 58, 58, 0, 0);
    	    triangle_loc = 3;
	    mpd->playing = 0;
	    update_slider(25);
            update_triangle();
     }
    
}


/* called when mouse button pressed */
static void switch_light() {
    switch (backlight) {
        case LIGHTOFF:
            backlight = LIGHTON;
            dockapp_copyarea(backdrop_on, pixmap, 0, 0, 58, 58, 0, 0);
            break;
        case LIGHTON:
            backlight = LIGHTOFF;
            dockapp_copyarea(backdrop_off, pixmap, 0, 0, 58, 58, 0, 0);
            break;
    }

    /* show */
    dockapp_copy2window(pixmap);
}

static void parse_arguments(int argc, char **argv) {
    int i;
    int integer;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            print_help(argv[0]), exit(0);
        } else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
            printf("%s version %s\n", PACKAGE, VERSION), exit(0);
        } else if (!strcmp(argv[i], "--display") || !strcmp(argv[i], "-d")) {
            display_name = argv[i + 1];
            i++;
        } else if (!strcmp(argv[i], "--backlight") || !strcmp(argv[i], "-bl")) {
            backlight = LIGHTON;
        } else if (!strcmp(argv[i], "--light-color") || !strcmp(argv[i], "-lc")) {
            light_color = argv[i + 1];
            i++;
        } else if (!strcmp(argv[i], "--interval") || !strcmp(argv[i], "-i")) {
            if (argc == i + 1)
                fprintf(stderr, "%s: error parsing argument for option %s\n",
                        argv[0], argv[i]), exit(1);
            if (sscanf(argv[i + 1], "%i", &integer) != 1)
                fprintf(stderr, "%s: error parsing argument for option %s\n",
                        argv[0], argv[i]), exit(1);
            if (integer < 1)
                fprintf(stderr, "%s: argument %s must be >=1\n",
                        argv[0], argv[i]), exit(1);
            update_interval = integer;
            i++;
        } else {
            fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0], argv[i]);
            print_help(argv[0]), exit(1);
        }
    }
}


static void print_help(char *prog)
{
    printf("Usage : %s [OPTIONS]\n"
           "%s - Window Maker MPD dockapp\n"
           "  -d,  --display <string>        display to use\n"
           "  -bl, --backlight               turn on back-light\n"
           "  -lc, --light-color <string>    back-light color(rgb:6E/C6/3B is default)\n"
           "  -i,  --interval <number>       number of secs between updates (1 is default)\n"
           "  -h,  --help                    show this help text and exit\n"
           "  -v,  --version                 show program version and exit\n",
           prog, prog);
    /* OPTIONS SUPP :
     *  ? -f, --file    : configuration file
     */
}
