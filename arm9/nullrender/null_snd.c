/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// snd_null.c -- include this instead of all the other snd_* files to have
// no sound code whatsoever

#include "quakedef.h"
#include "hash.h"

extern mleaf_t		*r_viewleaf, *r_oldviewleaf;
static qboolean	snd_ambient = 1;

channel_t   channels[MAX_CHANNELS];
int			total_channels;

int			soundtime;		// sample PAIRS
int   		paintedtime; 	// sample PAIRS

hashtable_t *sfx_known_hash;

sfx_t		*ambient_sfx[NUM_AMBIENTS];

cvar_t bgmvolume = {"bgmvolume", "1", true};
cvar_t volume = {"volume", "1.0", true};

cvar_t nosound = {"nosound", "0"};
cvar_t precache = {"precache", "1"};
cvar_t loadas8bit = {"loadas8bit", "1"};
cvar_t _snd_mixahead = {"_snd_mixahead", "0.1", true};
cvar_t ambient_level = {"ambient_level", "0.3"};
cvar_t ambient_fade = {"ambient_fade", "100"};

// pointer should go away
volatile dma_t  *shm = 0;
volatile dma_t sn;

qboolean		snd_initialized = false;

vec3_t		listener_origin;
vec3_t		listener_forward;
vec3_t		listener_right;
vec3_t		listener_up;
float		sound_nominal_clip_dist=1000.0;
 
float sound_start;
long long ds_sound_start;
long long ds_time();

int SND_SamplePos() {
#ifdef NDS
	static long long v;

	v = (ds_time() - ds_sound_start);
	
	//Con_Printf("time: %d\n",(int)v);

	return (int)v;
#else
	float tm = Sys_FloatTime()-sound_start;
	int pos = tm * shm->speed;
	return pos;
#endif
}

void GetSoundtime(void)
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;
	
	fullsamples = shm->samples / shm->channels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
#ifdef __sun__
	soundtime = SNDDMA_GetSamples();
#else

#if 1
	soundtime = SND_SamplePos();
#else

	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if (paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds (true);
		}
	}
	oldsamplepos = samplepos;

	soundtime = buffers*fullsamples + samplepos/shm->channels;
#endif
	//printf("soundtime: %d\n",soundtime);
#endif
}

void S_Update_(void)
{
	unsigned        endtime;
	int				samps;
	
	if (!snd_initialized)// || (snd_blocked > 0))
		return;

// Updates DMA time
	GetSoundtime();

// check to make sure that we haven't overshot
	if (paintedtime < soundtime)
	{
		//Con_Printf ("S_Update_ : overflow\n");
		paintedtime = soundtime;
	}

// mix ahead of current position
	endtime = soundtime + _snd_mixahead.value * shm->speed;
	samps = shm->samples >> (shm->channels-1);
	if (endtime - soundtime > samps)
		endtime = soundtime + samps;


	S_PaintChannels (endtime);

	//SNDDMA_Submit ();
}

void S_Init (void)
{
	Cvar_RegisterVariable(&bgmvolume);
	Cvar_RegisterVariable(&volume);
	Cvar_RegisterVariable(&nosound);
	Cvar_RegisterVariable(&precache);
	Cvar_RegisterVariable(&loadas8bit);
	Cvar_RegisterVariable(&_snd_mixahead);
	Cvar_RegisterVariable(&ambient_level);
	Cvar_RegisterVariable(&ambient_fade);

	if (host_parms.memsize < 0x800000)
	{
		Cvar_Set ("loadas8bit", "1");
		Con_Printf ("loading all sounds as 8bit\n");
	}

	if(nosound.value)
		return;

	SND_InitScaletable();

	shm = &sn;

	shm->buffer = (unsigned char *)Hunk_AllocName(4*1024,"snd_dma");
	
	memset(shm->buffer,0,4*1024);
	shm->buffer[4] = shm->buffer[5] = 0x7f;	// force a pop for debugging
	
	shm->channels = 1;
	shm->samplebits = 8;
	shm->speed = 11025;
	shm->samples = 4096/2;

	sound_start = Sys_FloatTime();
#ifdef NDS
	//timerStart(2,ClockDivider_1,timerFreqToTicks_1(11025),0);
#if 0
	TIMER_DATA(2) = 0;
	TIMER_CR(2) = TIMER_DIV_256 | TIMER_ENABLE;
#else
	/*TIMER_DATA(2) = 0x10000 - (0x1000000 / shm->speed) * 2;
	TIMER_CR(2) = TIMER_ENABLE | TIMER_DIV_1;
	TIMER_DATA(3) = 0;
	TIMER_CR(3) = TIMER_ENABLE | TIMER_CASCADE | TIMER_DIV_1;*/

#endif
	soundPlaySample(shm->buffer,
		SoundFormat_8Bit,
		4*1024/2,
		11025,
		127,
		64,
		true,
		0);
	ds_sound_start = ds_time();
	//IPCFifoSendMultiAsync(FIFO_SUBSYSTEM_SOUND,1,(const u32 *)fsnd,sizeof(*fsnd)/sizeof(int));
#endif

	snd_initialized = true;
	
}

void S_AmbientOff (void)
{
	snd_ambient = false;
}


void S_AmbientOn (void)
{
	snd_ambient = true;
}


void S_Shutdown (void)
{
}

sfx_t *S_FindName (char *name)
{
	char *ED_NewString (char *string);
	sfx_t	*sfx;

	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");

	sfx = (sfx_t*)Hash_Get(sfx_known_hash,name);
	if(sfx != 0)
		return sfx;
	else
	{
		char *str = ED_NewString(name);
		bucket_t *bucket = (bucket_t*)Hunk_AllocName(sizeof(bucket_t) + sizeof(model_t) + 1,"sfx_known_hash");
		sfx = (sfx_t *)(bucket+1);
		sfx->name = str;
		Hash_Add(sfx_known_hash, sfx->name, sfx, bucket);
		return sfx;
	}
	return 0;
}

void S_TouchSound (char *name)
{
	sfx_t	*sfx;

	if (nosound.value)
		return;

	sfx = S_FindName (name);
	if(sfx)
		Cache_Check (&sfx->cache);
}

void S_ClearBuffer (void)
{
	if (!snd_initialized || !shm || !shm->buffer)
		return;
	Q_memset(shm->buffer, 0x00, shm->samples * shm->samplebits/8);
}

#define SOUND_NOMINAL_CLIP_DISTANCE_MULT 0.001f

int spatialise (int entnum, float fvol, float attenuation, vec3_t origin, int *ds_pan, int *ds_vol)
{
	float master_v = fvol * 255.0f;
		int diff;
	
	int leftvol = (int)master_v;
	int rightvol = (int)master_v;
	
//	printf("att %.2f, vol %.2f\n", attenuation, fvol);
//	printf("or %.2f %.2f %.2f\n", origin[0], origin[1], origin[2]);
	
	if (entnum != cl.viewentity)
	{
		float dist_mult,dist,dot,rscale,lscale,scale;
		vec3_t source_vec;
		dist_mult = attenuation * SOUND_NOMINAL_CLIP_DISTANCE_MULT;
		
		VectorSubtract(origin, listener_origin, source_vec);
		
		dist = VectorNormalize(source_vec) * dist_mult;
		dot = DotProduct(listener_right, source_vec);
		
//		printf("dist %.2f dot %.2f\n", dist, dot);
		
		rscale = 1.0f + dot;
		lscale = 1.0f - dot;
		
		scale = (1.0f - dist) * rscale;
		rightvol = (int)(master_v * scale);
		
		if (rightvol < 0)
			rightvol = 0;
		
		scale = (1.0f - dist) * lscale;
		leftvol = (int)(master_v * scale);
		
		if (leftvol < 0)
			leftvol = 0;
			
//		printf("l %d, r %d\n", leftvol, rightvol);
			
		if ((leftvol == 0) && (rightvol == 0))
			return 0;
	}
	
	diff = (leftvol - rightvol) >> 3;
	*ds_pan = 64 - diff;
	
	*ds_vol = (leftvol + rightvol) >> 2;
	
	return 1;
}

#define FSOUND_NOMINAL_CLIP_DISTANCE_MULT 66

static unsigned intsqrt(unsigned int val) {
	unsigned int temp, g=0, b = 0x8000, bshft = 15;
	do {
		if (val >= (temp = (((g << 1) + b)<<bshft--))) {
		   g += b;
		   val -= temp;
		}
	} while (b >>= 1);
	return g;
}

int norm (int *v)
{
	int	length;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = intsqrt (length);		// FIXME

	if (length)
	{
		v[0] = (v[0]*(1<<16))/length;
		v[1] = (v[1]*(1<<16))/length;
		v[2] = (v[2]*(1<<16))/length;
	}
		
	return length;

}

int ispatialise (fifo_sound_t *snd, int *ds_pan, int *ds_vol)
{
	int master_v = (snd->fixed_volume * 255)>>16;
	int diff;
	
	int leftvol = (int)master_v;
	int rightvol = (int)master_v;

	int listener_right1[3];

	listener_right1[0] = listener_right[0] *(1<<16);
	listener_right1[1] = listener_right[1] *(1<<16);
	listener_right1[2] = listener_right[2] *(1<<16);
	
//	printf("att %.2f, vol %.2f\n", attenuation, fvol);
//	printf("or %.2f %.2f %.2f\n", origin[0], origin[1], origin[2]);
	
	{
		int dist_mult,dist,dot,rscale,lscale,scale;
		int source_vec[3];
		long long dd;
		dist_mult = (snd->fixed_attenuation * FSOUND_NOMINAL_CLIP_DISTANCE_MULT)>>16;
		
		//VectorSubtract(origin, listener_origin, source_vec);
		source_vec[0] = snd->origin[0] - listener_origin[0];
		source_vec[1] = snd->origin[1] - listener_origin[1];
		source_vec[2] = snd->origin[2] - listener_origin[2];
		
		//dist = VectorNormalize(source_vec) * dist_mult;
		dist = norm(source_vec) * dist_mult;
		
		//dot = DotProduct(listener_right, source_vec);
		dd = (((long long)listener_right1[0]*source_vec[0])>>16) + (((long long)listener_right1[1]*source_vec[1])>>16) + (((long long)listener_right1[2]*source_vec[2])>>16);
		dot = dd>>16;
//		printf("dist %.2f dot %.2f\n", dist, dot);
		
		rscale = (1<<16) + dot;
		lscale = (1<<16) - dot;
		
		dd = ((1<<16) - dist) * (long long)rscale;
		scale = dd>>16;
		rightvol = (master_v * scale)>>16;
		
		if (rightvol < 0)
			rightvol = 0;
		
		dd = ((1<<16) - dist) * (long long)lscale;
		scale = dd>>16;
		leftvol = (master_v * scale)>>16;
		
		if (leftvol < 0)
			leftvol = 0;
			
//		printf("l %d, r %d\n", leftvol, rightvol);
			
		if ((leftvol == 0) && (rightvol == 0))
			return 0;
	}
	
	diff = (leftvol - rightvol) >> 3;
	*ds_pan = 64 - diff;
	
	*ds_vol = (leftvol + rightvol) >> 2;
	
	return 1;
}

int iispatialise (int *origin,int dist_mult,int vol, int *lvol, int *rvol)
{
	int master_v = vol;
	int diff;
	
	int leftvol = (int)master_v;
	int rightvol = (int)master_v;

	int listener_right1[3];

	listener_right1[0] = listener_right[0] *(1<<16);
	listener_right1[1] = listener_right[1] *(1<<16);
	listener_right1[2] = listener_right[2] *(1<<16);
	
//	printf("att %.2f, vol %.2f\n", attenuation, fvol);
//	printf("or %.2f %.2f %.2f\n", origin[0], origin[1], origin[2]);
	
	{
		int /*dist_mult,*/dist,dot,rscale,lscale,scale;
		int source_vec[3];
		long long dd;
		//dist_mult = (atten * FSOUND_NOMINAL_CLIP_DISTANCE_MULT)>>16;
		
		//VectorSubtract(origin, listener_origin, source_vec);
		source_vec[0] = origin[0] - listener_origin[0];
		source_vec[1] = origin[1] - listener_origin[1];
		source_vec[2] = origin[2] - listener_origin[2];
		
		//dist = VectorNormalize(source_vec) * dist_mult;
		dist = norm(source_vec) * dist_mult;
		
		//dot = DotProduct(listener_right, source_vec);
		dd = (((long long)listener_right1[0]*source_vec[0])>>16) + (((long long)listener_right1[1]*source_vec[1])>>16) + (((long long)listener_right1[2]*source_vec[2])>>16);
		dot = dd>>16;
//		printf("dist %.2f dot %.2f\n", dist, dot);
		
		rscale = (1<<16) + dot;
		lscale = (1<<16) - dot;
		
		dd = ((1<<16) - dist) * (long long)rscale;
		scale = dd>>16;
		rightvol = (master_v * scale)>>16;
		
		if (rightvol < 0)
			rightvol = 0;
		
		dd = ((1<<16) - dist) * (long long)lscale;
		scale = dd>>16;
		leftvol = (master_v * scale)>>16;
		
		if (leftvol < 0)
			leftvol = 0;

		*lvol = leftvol;
		*rvol = rightvol;
			
//		printf("l %d, r %d\n", leftvol, rightvol);
			
		if ((leftvol == 0) && (rightvol == 0))
			return 0;
	}
	
	diff = (leftvol - rightvol) >> 3;
	//*ds_pan = 64 - diff;
	
	//*ds_vol = (leftvol + rightvol) >> 1;
	
	return 1;
}

/*
=================
SND_PickChannel
=================
*/
channel_t *SND_PickChannel(int entnum, int entchannel)
{
    int ch_idx;
    int first_to_die;
    int life_left;

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx=NUM_AMBIENTS ; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS ; ch_idx++)
    {
		if (entchannel != 0		// channel 0 never overrides
		&& channels[ch_idx].entnum == entnum
		&& (channels[ch_idx].entchannel == entchannel || entchannel == -1) )
		{	// allways override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (channels[ch_idx].entnum == cl.viewentity && entnum != cl.viewentity && channels[ch_idx].sfx)
			continue;

		if (channels[ch_idx].end - paintedtime < life_left)
		{
			life_left = channels[ch_idx].end - paintedtime;
			first_to_die = ch_idx;
		}
   }

	if (first_to_die == -1)
		return NULL;

	if (channels[first_to_die].sfx)
		channels[first_to_die].sfx = NULL;

    return &channels[first_to_die];    
}       

void S_StaticSound (sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
	channel_t	*ss;
	sfxcache_t	*sc;
	fifo_sound_t fs;
	int ds_pan, ds_vol;
	int ds_pan1, ds_vol1;

	if (!snd_initialized)
		return;

	if (!sfx)
		return;

	if (nosound.value)
		return;

	if (total_channels == MAX_CHANNELS)
	{
		Con_Printf ("total_channels == MAX_CHANNELS\n");
		return;
	}

	ss = &channels[total_channels];
	total_channels++;

	sc = S_LoadSound (sfx);
	if (!sc)
	{
		//target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}
	if (sc->loopstart == -1)
	{
		Con_Printf ("Sound %s not looped\n", sfx->name);
		return;
	}
	ss->sfx = sfx;
	//VectorCopy (origin, ss->origin);
	//ss->master_vol = fvol*255;
	//ss->rightvol = ss->leftvol = fvol*255;
	//ss->dist_mult = (attenuation/64) / sound_nominal_clip_dist;
    ss->end = paintedtime + sc->length;	
	ss->origin[0] = (int)origin[0];
	ss->origin[1] = (int)origin[1];
	ss->origin[2] = (int)origin[2];
	ss->dist_mult = ((int)(attenuation*(1<<(16-6))) * FSOUND_NOMINAL_CLIP_DISTANCE_MULT)>>16;
	ss->master_vol = (int)fvol;
	/*
	if (!spatialise(-1, fvol * volume.value, attenuation, origin, &ds_pan, &ds_vol))
		return;

	fs.data = sc->data;
	fs.format = 1;
	fs.loop = 0;
	fs.pan = ds_pan;
	fs.rate = sc->speed;
	fs.size = sc->length;
	fs.volume = ds_vol;
	fs.fixed_attenuation = (int)(attenuation*(1<<16));
	fs.fixed_volume = (int)(fvol*(1<<16));
	fs.origin[0] = (int)origin[0];
	fs.origin[1] = (int)origin[1];
	fs.origin[2] = (int)origin[2];

	ispatialise(&fs, &ds_pan1, &ds_vol1);

	ds_startSound9(&fs);*/
}

void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol,  float attenuation)
{
	channel_t *target_chan, *check;
	sfxcache_t	*sc;
	fifo_sound_t fs;
	int ds_pan, ds_vol;
	int ds_pan1, ds_vol1;

	if (!snd_initialized)
		return;

	if (!sfx)
		return;

	if (nosound.value)
		return;

// pick a channel to play on
	target_chan = SND_PickChannel(entnum, entchannel);
	if (!target_chan)
		return;

	fs.format = 1;
	fs.loop = 0;
	fs.fixed_attenuation = (int)(attenuation*(1<<16));
	fs.fixed_volume = (int)(fvol*(1<<16));
	fs.origin[0] = (int)origin[0];
	fs.origin[1] = (int)origin[1];
	fs.origin[2] = (int)origin[2];

	if(!ispatialise(&fs, &ds_pan, &ds_vol))
		return;
	//if (!spatialise(entnum, fvol * volume.value, attenuation, origin, &ds_pan, &ds_vol))
	//	return;

	sc = S_LoadSound (sfx);
	if (!sc)
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}
	target_chan->sfx = sfx;
	target_chan->pos = 0.0;
    target_chan->end = paintedtime + sc->length;	
	//target_chan->rightvol = target_chan->leftvol = fvol*255;
	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;
	target_chan->origin[0] = (int)origin[0];
	target_chan->origin[1] = (int)origin[1];
	target_chan->origin[2] = (int)origin[2];
	target_chan->dist_mult = ((int)(attenuation*(1<<16)) * FSOUND_NOMINAL_CLIP_DISTANCE_MULT)>>16;
	target_chan->master_vol = (int)(fvol*255);

	fs.pan = ds_pan;
	fs.volume = ds_vol;
	fs.rate = sc->speed;
	fs.size = sc->length;
	fs.data = sc->data;

	//ds_startSound9(&fs);
}

void S_StopSound (int entnum, int entchannel)
{
}

sfx_t *S_PrecacheSound (char *name)
{
	sfx_t	*sfx;

	if (nosound.value)
		return NULL;

	sfx = S_FindName (name);
	
// cache it in
	if (precache.value)
		S_LoadSound (sfx);
	
	return sfx;
}

void S_ClearPrecache (void)
{
}

/*
===================
S_UpdateAmbientSounds
===================
*/
void S_UpdateAmbientSounds (void)
{
	mleaf_t		*l;
	float		vol;
	int			ambient_channel;
	channel_t	*chan;

	if (!snd_ambient)
		return;

// calc ambient sound levels
	if (!cl.worldmodel)
		return;

	l = r_viewleaf;//Mod_PointInLeaf (listener_origin, cl.worldmodel);
	if (!l || !ambient_level.value)
	{
		for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
			channels[ambient_channel].sfx = NULL;
		return;
	}

	for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
	{
		chan = &channels[ambient_channel];	
		chan->sfx = ambient_sfx[ambient_channel];
	
		vol = ambient_level.value * l->ambient_sound_level[ambient_channel];
		if (vol < 8)
			vol = 0;

	// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol += host_frametime * ambient_fade.value;
			if (chan->master_vol > vol)
				chan->master_vol = vol;
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= host_frametime * ambient_fade.value;
			if (chan->master_vol < vol)
				chan->master_vol = vol;
		}
		
		chan->leftvol = chan->rightvol = chan->master_vol;
	}
}


void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up)
{	
	int			i, j;
	int			total;
	channel_t	*ch;
	channel_t	*combine;
	int leftvol,rightvol;
	//int ds_pan,ds_vol;

	if (nosound.value)
		return;
	if (!snd_initialized)
		return;
	
	VectorCopy(origin, listener_origin);
	VectorCopy(v_forward, listener_forward);
	VectorCopy(v_right, listener_right);
	VectorCopy(v_up, listener_up);

// update general area ambient sound sources
	S_UpdateAmbientSounds ();

	combine = NULL;

// update spatialization for static and dynamic sounds	
	ch = channels+NUM_AMBIENTS;
	for (i=NUM_AMBIENTS ; i<total_channels; i++, ch++)
	{
		if (!ch->sfx)
			continue;
	// anything coming from the view entity will allways be full volume
		if (ch->entnum == cl.viewentity)
		{
			ch->leftvol = ch->master_vol;
			ch->rightvol = ch->master_vol;
		}
		else if(!iispatialise(ch->origin,ch->dist_mult,ch->master_vol,&ch->leftvol,&ch->rightvol))         // respatialize channel
		{
			continue;
		}
		//else
		//{
		//	ch->leftvol = ds_vol;
		//	ch->rightvol = ds_vol;
		//}

		if (!ch->leftvol && !ch->rightvol)
			continue;

	// try to combine static sounds with a previous channel of the same
	// sound effect so we don't mix five torches every frame
	
		if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS)
		{
		// see if it can just use the last one
			if (combine && combine->sfx == ch->sfx)
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
				continue;
			}
		// search for one
			combine = channels+MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
			for (j=MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS ; j<i; j++, combine++)
				if (combine->sfx == ch->sfx)
					break;
					
			if (j == total_channels)
			{
				combine = NULL;
			}
			else
			{
				if (combine != ch)
				{
					combine->leftvol += ch->leftvol;
					combine->rightvol += ch->rightvol;
					ch->leftvol = ch->rightvol = 0;
				}
				continue;
			}
		}
		
		
	}

	S_Update_();
	ds_update_position(origin,v_right);
}

void S_StopAllSounds (qboolean clear)
{
	int i;
	total_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;	// no statics

	for (i=0 ; i<MAX_CHANNELS ; i++)
		if (channels[i].sfx)
			channels[i].sfx = NULL;
	for (i=0 ; i<NUM_AMBIENTS ; i++)
		ambient_sfx[i] = 0;
	S_ClearBuffer();
	CDAudio_Stop();
}

void S_BeginPrecaching (void)
{
}

void S_EndPrecaching (void)
{
}

void IN_Accumulate (void);
void S_ExtraUpdate (void)
{
	IN_Accumulate();
	S_Update_();
	CDAudio_Update();
}

void S_LocalSound (char *s)
{
	sfx_t	*sfx;

	if (nosound.value)
		return;
	if (!snd_initialized)
		return;
		
	sfx = S_PrecacheSound (s);
	if (!sfx)
	{
		Con_Printf ("S_LocalSound: can't cache %s\n", s);
		return;
	}
	S_StartSound (cl.viewentity, -1, sfx, vec3_origin, 1, 1);
}

