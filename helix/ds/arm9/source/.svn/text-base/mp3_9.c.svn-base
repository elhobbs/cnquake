#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <nds/arm9/sound.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include "mp3dec.h"
#include "mp3_shared.h"

void Con_Printf (char *fmt, ...);

volatile mp3_player	*mp3;
u8		*mp3_buffer;
u16		*mp3_audio;

int filled = 0;
int flag = 0;

void dump_buffer() {
	FILE *f = fopen("test.txt","wb");
	fwrite((void *)mp3->audio,1,MP3_AUDIO_BUFFER_SIZE,f);
	fclose(f);
}


void mp3_print() {
	float ds_time;

	if(mp3 == 0) {
		return;
	}
	
	ds_time = (float)mp3->soundtime/(float)(mp3->rate);

	Con_Printf("filled: %d %d %d %x %d\n",filled,mp3->bytesleft,mp3->rate,mp3->file,mp3->filesize);
	Con_Printf("time: %d %d\n",(int)mp3->soundtime,(int)(mp3->paintedtime-mp3->soundtime));
	Con_Printf("painted: %d %d\n",(int)mp3->paintedtime,(int)mp3->painted);
	//Con_Printf("state: %d\n",(int)mp3->state);
	Con_Printf("buffer: %x %x %d\n",(unsigned)mp3->readPtr,(unsigned)mp3->buffer,mp3->bytesleft);
	Con_Printf("decode: %d %d %d %d\n",mp3->decoded,mp3->painted,mp3->overflow,mp3->debug);
	Con_Printf("time: %f\n",ds_time);
	Con_Printf("lost: %d\n",mp3->lost);
	Con_Printf("timer: %d\n",mp3->timer);
	if(mp3->flag == 3) {
		dump_buffer();
		do {
			swiWaitForVBlank();
		} while(1);
	}
}


void mp3_fill_buffer() {
	int n;
	//iprintf("in\n");
	//mp3_print();
	if(mp3 && mp3->flag) {
		n = fread((void *)(mp3->buffer + MP3_FILE_BUFFER_SIZE), 1, MP3_FILE_BUFFER_SIZE, mp3->file);
		filled += n;
		if(mp3->loop && n < MP3_FILE_BUFFER_SIZE) {
			fseek (mp3->file, 0, SEEK_SET);
			n = fread((void *)(mp3->buffer + MP3_FILE_BUFFER_SIZE + n), 1, MP3_FILE_BUFFER_SIZE-n, mp3->file);
			filled += n;
		}
		mp3->flag = 0;
	}
	//iprintf("out\n");
}

void *uncached_malloc(size_t count) {
	void *p = malloc(count);
	return ((p == 0) ? 0 : memUncached(p));
}

int ds_filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int mp3_play_file(FILE *file,int loop){
	
	mp3_msg msg;
	int ret = 0;
		
	if(mp3 == 0) {
		return 1;
	}
	
	memset((void *)mp3,0,sizeof(*mp3));
	mp3->buffer = mp3_buffer;
	mp3->audio = mp3_audio;
	
	mp3->file = file;
	mp3->filesize = ds_filelength(mp3->file);
	mp3->loop = loop;
	
	
	msg.type = MP3_MSG_START;
	msg.player = mp3;
	
	filled += fread((void *)(mp3->buffer), 1, MP3_FILE_BUFFER_SIZE*2, mp3->file);

	fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);

	while(!fifoCheckValue32(FIFO_USER_01));

	ret = (int)fifoGetValue32(FIFO_USER_01);
	
	//Con_Printf("mp3_play_file: %d %d\n",mp3->rate,ret);
	
	return ret;
}

int mp3_play(char *filename,int loop){
	
	FILE *file = fopen(filename,"rb");
	
	return mp3_play_file(file,loop);
}

int mp3_pause() {
	mp3_msg msg;

	if(mp3 == 0) {
		return 1;
	}
	
	msg.type = MP3_MSG_PAUSE;
	
	fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
	while(!fifoCheckValue32(FIFO_USER_01));

	return (int)fifoGetValue32(FIFO_USER_01);
}
int mp3_stop() {
	mp3_msg msg;
	int ret;

	if(mp3 == 0) {
		return 1;
	}
	
	msg.type = MP3_MSG_STOP;
	
	fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
	while(!fifoCheckValue32(FIFO_USER_01));

	ret = (int)fifoGetValue32(FIFO_USER_01);
	
	if(mp3->file) {
		fclose(mp3->file);
		mp3->file = 0;
	}
	
	return ret;
}
int mp3_resume() {
	mp3_msg msg;
	
	if(mp3 == 0) {
		return 1;
	}
	
	msg.type = MP3_MSG_RESUME;
	
	fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
	while(!fifoCheckValue32(FIFO_USER_01));

	return (int)fifoGetValue32(FIFO_USER_01);
}
int mp3_set_volume(int volume) {
	mp3_msg msg;
		
	msg.type = MP3_MSG_VOLUME;
	msg.volume = volume;
	
	fifoSendDatamsg(FIFO_USER_01, sizeof(msg), (u8*)&msg);
	while(!fifoCheckValue32(FIFO_USER_01));

	return (int)fifoGetValue32(FIFO_USER_01);
}

int mp3_init() {
	mp3 = (mp3_player *)uncached_malloc(sizeof(mp3_player));
	mp3_buffer = (u8 *)uncached_malloc(MP3_FILE_BUFFER_SIZE*2);
	mp3_audio = (u16 *)malloc(MP3_AUDIO_BUFFER_SIZE);
	if(mp3 == 0 || mp3_buffer == 0 || mp3_audio == 0) {
		mp3 = 0;
		mp3_buffer = 0;
		mp3_audio = 0;
		Con_Printf("mp3 failed to allocate buffers\n");
		while(1);
		return 1;
	}
	
	memset((void *)mp3,0,sizeof(*mp3));
	memset((void *)mp3_buffer,0,MP3_FILE_BUFFER_SIZE*2);
	memset((void *)mp3_audio,0,MP3_AUDIO_BUFFER_SIZE);
	
	return 0;
}
