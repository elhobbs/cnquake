#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <dswifi7.h>
#include <malloc.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include "mp3dec.h"
#include "mp3_shared.h"

void enableSound();
int getFreeChannel(void);


MP3FrameInfo mp3FrameInfo;
HMP3Decoder hMP3Decoder = 0;

volatile mp3_player			*mp3;
int					mp3_loop;
int					mp3_channel;
int					mp3_bytesleft;
int					mp3_volume;
u8					*mp3_readPtr;

volatile mp3_player_state	mp3_state;

DSTIME				ds_sound_start;
DSTIME				soundtime;
DSTIME   			paintedtime;

vs32 mp3_debug = 0;
u16 *outbuf = 0;//[OUTBUF_SIZE/2];

DSTIME ds_time()
{
	u16 time1 = TIMER1_DATA;
	u32 time2 = TIMER2_DATA;
#if 0
	static u32 last;
	static DSTIME t;
	
	if(time2 < last) {
		t += (1LL<<32);
	}
	last = time2;
	return (t + (time2 << 16) + time1);
#else
	return (time2 << 16) + time1;
#endif
}

void ds_set_timer(int rate) {
	if(rate == 0) {
		TIMER_CR(0) = 0;
		TIMER_CR(1) = 0;
		TIMER_CR(2) = 0;
	} else {
		TIMER_DATA(0) = 0x10000 - (0x1000000 / rate) * 2;
		TIMER_CR(0) = TIMER_ENABLE | TIMER_DIV_1;
		TIMER_DATA(1) = 0;
		TIMER_CR(1) = TIMER_ENABLE | TIMER_CASCADE | TIMER_DIV_1;
		TIMER_DATA(2) = 0;
		TIMER_CR(2) = TIMER_ENABLE | TIMER_CASCADE | TIMER_DIV_1;
	}
}

DSTIME ds_sample_pos() {
	DSTIME v;

	v = (ds_time() - ds_sound_start);
	
	return v;
}

void S_TransferPaintBuffer(int count)
{
	int 	out_idx;
	int 	out_mask;
	s16 	*p;
	int 	step;
	short	*out = (short *) mp3->audio;

	
	p = (s16 *)outbuf;
	out_mask = MP3_AUDIO_BUFFER_SAMPS - 1; 
	out_idx = paintedtime & out_mask;
	step = 3 - 1;
	if(out_idx + count > MP3_AUDIO_BUFFER_SAMPS)
	{
		while (count > 0)
		{
			out[out_idx] = *p;
			p += step;
			out_idx = (out_idx + 1) & out_mask;
			count -= step;
		}
	}
	else
	{
		out += out_idx;
		AS_StereoDesinterleave(p,out,out,count/2);
	}
}

int mp3_frame() {
	int offset,err;
	
	// if mp3 is set to loop indefinitely, don't bother with how many data is left
	if(mp3_loop && mp3_bytesleft < 2*MAINBUF_SIZE)
		mp3_bytesleft += MP3_FILE_BUFFER_SIZE;


	/* find start of next MP3 frame - assume EOF if no sync found */
	offset = MP3FindSyncWord((u8 *)mp3_readPtr, mp3_bytesleft);
	if (offset < 0) {
		mp3_debug = 4;
		return 0;
	}
	mp3_readPtr += offset;
	mp3_bytesleft -= offset;

	err = MP3Decode(hMP3Decoder, (u8 **)&mp3_readPtr, (s32 *)&mp3_bytesleft, (s16*)outbuf, 0);
	if (err) {
		/* error occurred */
		switch (err) {
		case ERR_MP3_INDATA_UNDERFLOW:
			//outOfData = 1;
			break;
		case ERR_MP3_MAINDATA_UNDERFLOW:
			/* do nothing - next call to decode will provide more mainData */
			return 1;
		case ERR_MP3_FREE_BITRATE_SYNC:
		default:
			//outOfData = 1;
			break;
		}
		mp3_debug = err;
		return 0;
	}
	/* no error */
	MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
	S_TransferPaintBuffer(mp3FrameInfo.outputSamps);
	paintedtime += (mp3FrameInfo.outputSamps>>1);
	mp3->decoded++;
	
	return 1;
}

void mp3_frames(DSTIME endtime)
{

	mp3_debug = 2;
	
	while (paintedtime < endtime)
	{
			
		mp3_debug = 3;
		mp3_frame();

		// check if we moved onto the 2nd file data buffer, if so move it to the 1st one and request a refill
		if(mp3_readPtr > (mp3->buffer +  MP3_FILE_BUFFER_SIZE  + (MP3_FILE_BUFFER_SIZE/2))) {
			//memcpy((void *)mp3->buffer, (void *)(mp3->buffer + MP3_FILE_BUFFER_SIZE), MP3_FILE_BUFFER_SIZE);
			mp3_readPtr = mp3_readPtr - MP3_FILE_BUFFER_SIZE;
			memcpy((void *)mp3_readPtr, (void *)(mp3_readPtr + MP3_FILE_BUFFER_SIZE), MP3_FILE_BUFFER_SIZE - (mp3_readPtr-mp3->buffer));
			mp3->flag = 1;
			mp3_debug = 1;
			//fifoSendValue32(FIFO_USER_02, 0);
		}
	}
}




int mp3_playing() {

	DSTIME endtime;
	//int samps;
	
	soundtime = ds_sample_pos();

// check to make sure that we haven't overshot
	if (paintedtime < soundtime)
	{
		mp3->lost += (soundtime - paintedtime);
		//Con_Printf ("S_Update_ : overflow\n");
		paintedtime = soundtime;
		mp3->overflow++;
	}
	
	//mp3->paintedtime = paintedtime;

	// mix ahead of current position
	endtime = soundtime + (mp3FrameInfo.samprate/16);//(mp3->rate * 1);
		
	mp3_frames(endtime);
	
	mp3->debug = mp3_debug;
	mp3->bytesleft = mp3_bytesleft;
	mp3->paintedtime = paintedtime;
	mp3->soundtime = soundtime;
	mp3->readPtr = mp3_readPtr;

	return 0;
}

void mp3_pause() {
	if(mp3 == 0 || mp3_channel == -1) {
		mp3_state = MP3_IDLE;
		return;
	}
	ds_set_timer(0);
	SCHANNEL_CR(mp3_channel) = 0;
	mp3_channel = -1;
	mp3_state = MP3_PAUSED;
}	

int mp3_resume() {

	if(mp3 == 0 || mp3_channel != -1) {
		mp3->debug = 42;
		mp3_state = MP3_IDLE;
		return 1;
	}
	
	paintedtime = 0;
	memset((void *)mp3->audio,0,MP3_AUDIO_BUFFER_SIZE);
	mp3_frames(MP3_AUDIO_BUFFER_SAMPS/2);

	MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
	mp3->rate = mp3FrameInfo.samprate;
	
	mp3_channel = getFreeChannel();
	//mp3->audio[4] = mp3->audio[5] = 0x7fff;	// force a pop for debugging
	SCHANNEL_SOURCE(mp3_channel) = (u32)mp3->audio;
	SCHANNEL_REPEAT_POINT(mp3_channel) = 0;
	SCHANNEL_LENGTH(mp3_channel) = (MP3_AUDIO_BUFFER_SIZE)>>2;
	SCHANNEL_TIMER(mp3_channel) = 0x10000 - (0x1000000 / mp3->rate);
	SCHANNEL_CR(mp3_channel) = SCHANNEL_ENABLE | SOUND_VOL(mp3_volume) | SOUND_PAN(64) | (SOUND_FORMAT_16BIT) | (SOUND_REPEAT);
	
	ds_set_timer(mp3->rate);
	ds_sound_start = ds_time();
	
	mp3_state = MP3_PLAYING;
	return 0;
}

int mp3_starting() {
	int cb = OUTBUF_SIZE*sizeof(u16);
	if(hMP3Decoder == 0) {
		if ( (hMP3Decoder = MP3InitDecoder()) == 0 ) {
			mp3_state = MP3_IDLE;
			fifoSendValue32(FIFO_USER_01, 1);
			return 0;
		}
	}
	
	if(outbuf == 0) {
		do {
			outbuf = (u16 *)malloc(cb);
			if(outbuf) {
				break;
			}
			cb -= 128;
		} while(cb > 0);
	}
		
	mp3_bytesleft = mp3->filesize;
	mp3_readPtr = mp3->buffer;
	mp3_loop = mp3->loop;
	
	mp3_resume();

	fifoSendValue32(FIFO_USER_01, cb);
	return 1;
}

void mp3_resuming() {
	if(mp3 == 0) {
		mp3_state = MP3_IDLE;
		fifoSendValue32(FIFO_USER_01, 0);
		return;
	}
	
	mp3_resume();
	fifoSendValue32(FIFO_USER_01, 0);
}

void mp3_pausing() {
	if(mp3 == 0) {
		mp3_state = MP3_IDLE;
		fifoSendValue32(FIFO_USER_01, 0);
		return;
	}
	mp3_pause();
	fifoSendValue32(FIFO_USER_01, 0);
}

void mp3_stopping() {
	if(mp3 == 0) {
		mp3_state = MP3_IDLE;
		fifoSendValue32(FIFO_USER_01, 0);
		return;
	}
	ds_set_timer(0);
	SCHANNEL_CR(mp3_channel) = 0;
	mp3_channel = -1;
	MP3FreeDecoder(hMP3Decoder);
	hMP3Decoder = 0;
	fifoSendValue32(FIFO_USER_01, 0);
	mp3_state = MP3_IDLE;
}
void mp3_process() {
	switch(mp3_state) {
	case MP3_STARTING:
		mp3_starting();
		break;
	case MP3_PLAYING:
		mp3_playing();
		break;
	case MP3_PAUSING:
		mp3_pausing();
		break;
	case MP3_RESUMING:
		mp3_resuming();
		break;
	case MP3_STOPPING:
		mp3_stopping();
		break;
	case MP3_IDLE:
	case MP3_PAUSED:
	case MP3_ERROR:
		break;
	}
}

void mp3_set_volume(int volume) {
	
	if(volume < 0)
		volume = 0;
	if(volume > 127)
		volume = 127;
		
	mp3_volume = volume;
	
	if(mp3_channel == -1) {
		fifoSendValue32(FIFO_USER_01, 0);
		return;
	}
	SCHANNEL_CR(mp3_channel) &= ~0xFF;
	SCHANNEL_CR(mp3_channel) |= volume;
	fifoSendValue32(FIFO_USER_01, 0);
}

//---------------------------------------------------------------------------------
void mp3_DataHandler(int bytes, void *user_data) {
//---------------------------------------------------------------------------------
	mp3_msg msg;

	fifoGetDatamsg(FIFO_USER_01, bytes, (u8*)&msg);

	switch(msg.type) {
	case MP3_MSG_START:
		mp3 = msg.player;
		mp3_state = MP3_STARTING;
		break;
	case MP3_MSG_PAUSE:
		mp3_state = MP3_PAUSING;
		break;
	case MP3_MSG_RESUME:
		mp3_state = MP3_RESUMING;
		break;
	case MP3_MSG_STOP:
		mp3_state = MP3_STOPPING;
		break;
	case MP3_MSG_AT_END:
		break;
	case MP3_MSG_VOLUME:
		mp3_set_volume(msg.volume);
		break;
	}
	
}

void mp3_init() {
	mp3_state = MP3_IDLE;
	hMP3Decoder = 0;
	mp3_channel = -1;
	mp3 = 0;
	mp3_bytesleft = 0;
	mp3_volume = 127;
	enableSound();
	fifoSetDatamsgHandler(FIFO_USER_01, mp3_DataHandler, 0);
}

