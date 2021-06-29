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

//
// these are the key numbers that should be passed to Key_Event
//
#define K_CAPS			1
#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32


// normal keys should be passed as lowercased ascii

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131

#define	K_ALT			132
#define	K_CTRL			133
#define	K_SHIFT			134
#define	K_F1			135
#define	K_F2			136
#define	K_F3			137
#define	K_F4			138
#define	K_F5			139
#define	K_F6			140
#define	K_F7			141
#define	K_F8			142
#define	K_F9			143
#define	K_F10			144
#define	K_F11			145
#define	K_F12			146
#define	K_INS			147
#define	K_DEL			148
#define	K_PGDN			149
#define	K_PGUP			150
#define	K_HOME			151
#define	K_END			152

//
// mouse buttons generate virtual keys
//
#define	K_MOUSE1		200
#define	K_MOUSE2		201
#define	K_MOUSE3		202

// JACK: Intellimouse(c) Mouse Wheel Support

#define K_MWHEELUP		239
#define K_MWHEELDOWN	240

#define K_NDS_START		241
#define K_NDS_SELECT	242
#define K_NDS_A			243
#define K_NDS_B			244
#define K_NDS_X			245
#define K_NDS_Y			246
#define K_NDS_L			247
#define K_NDS_R			248
#define K_NDS_LEFT		249
#define K_NDS_RIGHT		250
#define K_NDS_UP		251
#define K_NDS_DOWN		252
#define K_NDS_F1		253
#define K_NDS_TAP		254

#define K_PAUSE			255

typedef enum {key_game, key_console, key_message, key_menu,key_max=0xffffffff} keydest_t;

extern keydest_t	key_dest;
extern char *keybindings[256];
extern	int		key_repeats[256];
extern	int		key_count;			// incremented every key event
extern	int		key_lastpress;

void Key_Event (int key, qboolean down);
void Key_Init (void);
void Key_WriteBindings (FILE *f);
void Key_SetBinding (int keynum, char *binding);
void Key_ClearStates (void);

