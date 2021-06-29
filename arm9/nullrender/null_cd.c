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
#include "quakedef.h"
#ifdef NDS
#include "mp3_shared.h"
#endif

static int mp3_playing;
static float mp3_volume;
static qboolean	mp3_enabled = false;

void CDAudio_Play(byte track, qboolean looping)
{
	char name[256];
	FILE *file;

	CDAudio_Stop();
	
	sprintf(name,"music/%02d.mp3",track);
	//Con_Printf("MP3: music/%02d.mp3",track);
	if(COM_FOpenFile(name,&file) == -1) {
		return;
	}
	Con_Printf("Playing track %d\n",(int)track);
#ifdef NDS
	mp3_play_file(file,looping);
#endif
}


void CDAudio_Stop(void)
{
#ifdef NDS
	mp3_stop();
#endif
}


void CDAudio_Pause(void)
{
#ifdef NDS
	mp3_pause();
#endif
}


void CDAudio_Resume(void)
{
#ifdef NDS
	mp3_pause();
#endif
}


void CDAudio_Update(void)
{
#ifdef NDS
	mp3_fill_buffer();
#endif

	if(mp3_volume != bgmvolume.value) {
		int volume = bgmvolume.value * 127;
#ifdef NDS
		mp3_set_volume(volume);
#endif
		mp3_volume = bgmvolume.value;
	}
}

static void CD_f (void)
{
	char	*command;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		mp3_enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (mp3_playing)
			CDAudio_Stop();
		mp3_enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), true);
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	/*if (Q_strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", maxTrack);
		if (playing)
			Con_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (wasPlaying)
			Con_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		Con_Printf("Volume is %f\n", cdvolume);
		return;
	}*/
}


int CDAudio_Init(void)
{
	mp3_playing = 0;
	mp3_volume = 0;
	if (cls.state == ca_dedicated)
		return -1;

	if (COM_CheckParm("-nocdaudio"))
		return -1;

#ifdef NDS

	if(mp3_init()) {
		return 1;
	}
#endif
	mp3_enabled = true;
	Cmd_AddCommand ("cd", CD_f);
	return 0;
}


void CDAudio_Shutdown(void)
{
}

