#ifndef MPD_FUNC_H
#define MPD_FUNC_H

#include "libmpdclient.h"

extern char * mpd_host;
extern char * mpd_password;
extern int mpd_port;

void MpdPlay();
void MpdPause();
void MpdStop();
void MpdEject();
void MpdPrev();
void MpdNext();
void MpdFastr();
void MpdFastf();
int MpdGetVolume();
int MpdStatus();
int MpdGetTime();
int MpdGetTrack();
char * MpdGetTitle();
int MpdPlayStatus();
int MpdPauseStatus();
int MpdRepeatStatus();
int MpdRandomStatus();
void MpdSetVolume(int v);
void MpdToggleRepeat();
void MpdToggleRandom();

#endif
