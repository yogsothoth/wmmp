#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpd_func.h"

char * mpd_host;
int mpd_port;
char * mpd_password = NULL;

mpd_Connection * connection = NULL;
mpd_Status * status = NULL;

int MpdIsErrored() {
	if(!connection->error) return 0;
	if(connection->error==MPD_ERROR_UNKHOST || 
			connection->error==MPD_ERROR_SYSTEM ||
			connection->error==MPD_ERROR_NOTMPD) {
		fprintf(stderr,"%s\n",connection->errorStr);
		exit(-1);
	}
	else if(connection->error==MPD_ERROR_TIMEOUT ||
			connection->error==MPD_ERROR_SENDING ||
			connection->error==MPD_ERROR_CONNCLOSED ||
			connection->error==MPD_ERROR_NORESPONSE ||
			connection->error==MPD_ERROR_CONNPORT) {
		/*fprintf(stderr,"%s\n",connection->errorStr);
		fprintf(stderr,"resetting connection\n");*/
		mpd_closeConnection(connection);
		connection = NULL;
	}
	else {
		fprintf(stderr,"%s\n",connection->errorStr);
		connection->error = 0;
	}
	return 1;
}

void MpdPlay() {
	if(status && status->state==MPD_STATUS_STATE_PAUSE) {
		MpdPause();
		return;
	}

	mpd_sendPlayCommand(connection,MPD_PLAY_AT_BEGINNING);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
}

void MpdStop() {
	mpd_sendStopCommand(connection);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
}

void MpdPause() {
	mpd_sendPauseCommand(connection);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
}

void MpdEject() {
}

void MpdPrev() {
	mpd_sendPrevCommand(connection);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
}

void MpdNext() {
	mpd_sendNextCommand(connection);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
}

void MpdFastr() {
	if(!status || status->state==MPD_STATUS_STATE_STOP) return;
	mpd_sendSeekCommand(connection,status->song,status->elapsedTime-10);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
	MpdStatus();
}

void MpdFastf() {
	if(!status || status->state==MPD_STATUS_STATE_STOP) return;
	mpd_sendSeekCommand(connection,status->song,status->elapsedTime+10);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
	MpdStatus();
}

int MpdGetVolume() {
	if(status && status->volume!=MPD_STATUS_NO_VOLUME) {
		return status->volume;
	}
	return 0;
}

void MpdToggleRandom() {
	mpd_sendRandomCommand(connection,!status->random);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
}

void MpdToggleRepeat() {
	mpd_sendRepeatCommand(connection,!status->repeat);
	if(MpdIsErrored()) return;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return;
}

int MpdStatus() {
	if(!connection) {
		connection = mpd_newConnection(mpd_host,mpd_port,10);
		if(MpdIsErrored()) return 0;
		if(mpd_password && strlen(mpd_password)) {
			mpd_sendPasswordCommand(connection,mpd_password);
			if(MpdIsErrored()) return 0;
			mpd_finishCommand(connection);
			if(MpdIsErrored()) return 0;
		}
	}
	if(status) mpd_freeStatus(status);
	status = mpd_getStatus(connection);
	if(MpdIsErrored()) return 0;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return 0;
	if(!status) return 0;

	return 1;
}

int MpdPauseStatus() {
	if(status) return (status->state == MPD_STATUS_STATE_PAUSE);
	return 0;
}

int MpdRandomStatus() {
	if(status) return status->random;
	return 0;
}

int MpdRepeatStatus() {
	if(status) return status->repeat;
	return 0;
}

int MpdPlayStatus() {
	if(status) return (status->state == MPD_STATUS_STATE_PLAY);
	return 0;
}

int MpdGetTrack() {
	if(status) return status->song;
	return 0;
}

char *MpdGetTitle() {
	static char * ret = NULL;
	static unsigned long lastVersion = 0;
	static int lastSong = -1;
	mpd_InfoEntity * entity;


	if(!status || (status->state!=MPD_STATUS_STATE_PLAY && 
			status->state!=MPD_STATUS_STATE_PAUSE)) {
		if(ret) free(ret);
		lastVersion = 0;
		lastSong = -1;
		ret = strdup("");
		return ret;
	}

	if(ret && lastVersion == status->playlist && 
			lastSong == status->song) {
		return ret;
	}

	lastVersion = status->playlist;
	lastSong = status->song;

	if(ret) free(ret);

	mpd_sendPlaylistInfoCommand(connection,status->song);
	if(MpdIsErrored()) {
		ret = strdup("");
		return ret;
	}
	if((entity = mpd_getNextInfoEntity(connection))) {
		if(entity->info.song->artist && entity->info.song->title) {
			ret = malloc(strlen(entity->info.song->artist) +
					strlen(entity->info.song->title) + 4);
			sprintf(ret,"%s - %s",entity->info.song->artist,
					entity->info.song->title);
		}
		else ret = strdup(entity->info.song->file);
		mpd_freeInfoEntity(entity);
	}
	if(MpdIsErrored()) return ret;
	mpd_finishCommand(connection);
	if(MpdIsErrored()) return ret;
	return ret;
}

int MpdGetTime() {
	if(status) return status->elapsedTime;
	return 0;
}

void MpdSetVolume(int v) {
	int change;

	if(!status || status->volume == MPD_STATUS_NO_VOLUME) return;

	change = v-status->volume;
	fprintf(stdout,"Target volume value %i\n",v);
	//	mpd_sendVolumeCommand(connection,change);
	//	mpd_sendSetvolCommand(connection,change);
	mpd_sendSetvolCommand(connection,v);
	if(connection->error) {
		fprintf(stderr,"%s\n",connection->errorStr);
	}
	mpd_finishCommand(connection);
	if(connection->error) {
		fprintf(stderr,"%s\n",connection->errorStr);
	}
	MpdStatus();
}
