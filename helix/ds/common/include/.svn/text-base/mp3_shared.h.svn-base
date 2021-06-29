#ifndef __mp3_shared_h__
#define __mp3_shared_h__


#define MP3_DEBUG	1

typedef vu32 DSTIME;

typedef enum {
	MP3_IDLE=0,
	MP3_STARTING=1,
	MP3_PLAYING=2,
	MP3_PAUSING=3,
	MP3_RESUMING=4,
	MP3_PAUSED=5,
	MP3_STOPPING=6,
	MP3_ERROR=0xffffffff
} mp3_player_state;

typedef struct {
	vu32	flag;
	vs32	rate;
	vs32	filesize;
	vs32	loop;
	u8		*buffer;
	u16		*audio;
	FILE	*file;
	
	//the rest is just used for watching the state from the arm9
	vu8		*readPtr;
	vu32	bytesleft;
	
	DSTIME	paintedtime;
	DSTIME	soundtime;
	DSTIME	timer;
	
	vs32	decoded;
	vs32	painted;
	vs32	overflow;
	vs32	debug;
	vu32	lost;
	
} mp3_player;

enum {
	MP3_MSG_START=0,
	MP3_MSG_STOP=1,
	MP3_MSG_PAUSE=2,
	MP3_MSG_RESUME=3,
	MP3_MSG_AT_END=4,
	MP3_MSG_READ=5,
	MP3_MSG_VOLUME,
	MP3_MSG_ERROR=0xffffffff
};

typedef struct {
	u32	type;
	union {
		volatile mp3_player	*player;
		u32		count;
		u32		volume;
	};
} mp3_msg;


#define OUTBUF_SIZE	(MAX_NCHAN * MAX_NGRAN * MAX_NSAMP)
#define MP3_FILE_BUFFER_SIZE (8 * 1024)

#define MP3_AUDIO_BUFFER_SAMPS (8 * 1024)
#define MP3_AUDIO_BUFFER_SIZE (MP3_AUDIO_BUFFER_SAMPS*2)

#ifdef ARM9

int mp3_init();
int mp3_play_file(FILE *file,int loop);
int mp3_play(char *filename,int loop);
void mp3_fill_buffer();
void mp3_print();
int mp3_pause();
int mp3_resume();
int mp3_stop();
int mp3_set_volume(int volume);

#endif

#ifdef ARM7

void mp3_init();
void mp3_process();
void AS_StereoDesinterleave(s16 *input, s16 *outputL, s16 *outputR, u32 samples);

#endif

#endif