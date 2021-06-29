#include "quakedef.h"
#include "hash.h"
#ifdef NDS
//#include "IPCFifo.h"
#endif
/*
typedef struct
{
    byte  *data;
    int size;
    int  format;
    int  rate;
    int  volume;
    int  pan;
    int  loop;
	int  fixed_volume;
	int  fixed_attenuation;
	int  origin[3];
} fifo_sound_t;

*/
int ds_startSound9(fifo_sound_t *fsnd)
{
	//return 0;
#ifdef NDS
	soundPlaySample(fsnd->data,
		fsnd->format==1?SoundFormat_8Bit:SoundFormat_16Bit,
		fsnd->size,
		fsnd->rate,
		fsnd->volume,
		fsnd->pan,
		fsnd->loop,
		0);
	//IPCFifoSendMultiAsync(FIFO_SUBSYSTEM_SOUND,1,(const u32 *)fsnd,sizeof(*fsnd)/sizeof(int));
#endif
	return 0;
}

int ds_update_position(vec3_t origin,vec3_t vright)
{
	int data[6];

	data[0] = (int)origin[0];
	data[1] = (int)origin[1];
	data[2] = (int)origin[2];

	data[3] = (int)(vright[0]*(1<<16));
	data[4] = (int)(vright[1]*(1<<16));
	data[5] = (int)(vright[2]*(1<<16));

#ifdef NDS
	//IPCFifoSendMultiAsync(FIFO_SUBSYSTEM_SOUND,2,(const u32 *)&data,sizeof(data)/sizeof(int));
#endif
	return 0;
}
