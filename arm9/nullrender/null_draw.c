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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

#define BLEND_CR		REG_BLDCNT
#define BLEND_AB		REG_BLDALPHA

extern int ds_bg_sub;
extern int ds_bg_main;
extern int ds_bg_text;

typedef struct {
	union {
		byte b[2];
		unsigned short s;
	};
}pix16_t;

typedef struct {
	vrect_t	rect;
	int		width;
	int		height;
	byte	*ptexbytes;
	int		rowbytes;
} rectdesc_t;

static rectdesc_t	r_rectdesc;

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	cache_user_t	cache;
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;


qpic_t	*Draw_PicFromWad (char *name)
{
	return (qpic_t	*)W_GetLumpName (name);
}

/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	
	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			break;

	if (i == menu_numcachepics)
	{
		if (menu_numcachepics == MAX_CACHED_PICS)
			Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
		menu_numcachepics++;
		strcpy (pic->name, path);
	}

	//Con_Printf("Cache_Check 1 %s\n",path);
	dat = (qpic_t	*)Cache_Check (&pic->cache);

	if (dat)
		return dat;

//
// load the pic from disk
//
	COM_LoadCacheFile (path, &pic->cache);
	
	dat = (qpic_t *)pic->cache.data;
	if (!dat)
	{
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	}

	SwapPic (dat);

	return dat;
}

void DS_Load_Char (int num,unsigned short *dest)
{
	byte			*source;
	pix16_t			temp;
	int				drawline;	
	int				row, col, i;


		
	num &= 255;
	
	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for(i=0;i<4;i++)
		{
			temp.b[0] = source[i*2 + 0];
			temp.b[1] = source[i*2 + 1];
			dest[i] = temp.s;
		}

		source += 128;
		dest += 4;
	}
}

void DS_Load_Chars(unsigned short *dest,int start,int end)
{
	int i;

	for(i=start;i<end;i++)
	{
		DS_Load_Char(i,dest);
		dest += (64/2);
	}
}
int show_stdout = 0;
void toggle_stdout_f(void)
{
#ifdef NDS
	if(show_stdout)
	{
		videoBgDisable(1);
		REG_BG1CNT |= BG_PRIORITY_3;
		show_stdout = 0;
	}
	else
	{
		videoBgEnable(1);
		REG_BG1CNT &= (~BG_PRIORITY_3);
		show_stdout = 1;
	}
#endif
}

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{

	draw_chars = (byte	*)W_GetLumpName ("conchars");
	draw_disc = (qpic_t	*)W_GetLumpName ("disc");
	draw_backtile = (qpic_t	*)W_GetLumpName ("backtile");

	r_rectdesc.width = draw_backtile->width;
	r_rectdesc.height = draw_backtile->height;
	r_rectdesc.ptexbytes = draw_backtile->data;
	r_rectdesc.rowbytes = draw_backtile->width;

	vid.conrowbytes = CON_SCREEN_WIDTH;
	vid.conheight = CON_SCREEN_HEIGHT;
	vid.conwidth = CON_SCREEN_WIDTH;
	vid.rowbytes = CON_SCREEN_WIDTH;
	//vid.conbuffer = vid.buffer = (pixel_t *)Hunk_AllocName(320*200,"vidbuf");
	vid.conbuffer = vid.buffer = (pixel_t *)Hunk_AllocName(SCREEN_WIDTH*SCREEN_HEIGHT+4,"vidbuf");
	*((int *)(vid.buffer+SCREEN_WIDTH*SCREEN_HEIGHT)) = 0xDEADBEEF;
	//vid.aliasbuffer = (pixel_t *)Hunk_AllocName(256*192,"aliasbuf");

#ifdef NDS
	//hide stdout
	bgSetPriority(0,1);

	bgInit(1,BgType_Text8bpp,BgSize_T_256x256,14,0);
	bgSetPriority(1,3);
	videoBgDisable(1);
	
	ds_bg_text = bgInit(2,BgType_Text8bpp,BgSize_T_256x256,15,0);
	bgSetPriority(2,0);

	DS_Load_Chars((u16*)CHAR_BASE_BLOCK(0),0,256);

	int i;
	for(i=0;i<32*32;i++)
	{
		((u16*)SCREEN_BASE_BLOCK(15))[i] = (u16)' ';
	}
#endif

	show_overlay(true,false);
	Cmd_AddCommand ("stdout",toggle_stdout_f);
}

void Draw_CharacterCenter (int x, int y, int num)
{
	byte			*dest;
	unsigned short	*pusdest;
	int				drawline;	
	int				row, col;
		
	num &= 255;
	
	row = y/8;
	col = x/8;

	if(col < 0 || col > 31 || row < 0 || row > 23)
		return;
#ifdef NDS
	pusdest = ((u16*)SCREEN_BASE_BLOCK(15));
	pusdest[row*32+col] = num;
#endif
}


/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character2 (int x, int y, int num,char *vbuf)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;	
	int				row, col;


	if(vid.buffer == 0)
		return;
		
	num &= 255;
	
	if (y <= -8)
		return;			// totally off screen

#ifdef PARANOID
	if (y > CON_SCREEN_HEIGHT - 8 || x < 0 || x > vid.width - 8)
		Sys_Error ("Con_DrawCharacter: (%i, %i)", x, y);
	if (num < 0 || num > 255)
		Sys_Error ("Con_DrawCharacter: char %i", num);
#endif

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 128*y;
		y = 0;
	}
	else
		drawline = 8;


		dest = (byte *)vbuf + y*vid.conrowbytes + x;
	
		while (drawline--)
		{
			if (source[0])
				dest[0] = source[0];
			if (source[1])
				dest[1] = source[1];
			if (source[2])
				dest[2] = source[2];
			if (source[3])
				dest[3] = source[3];
			if (source[4])
				dest[4] = source[4];
			if (source[5])
				dest[5] = source[5];
			if (source[6])
				dest[6] = source[6];
			if (source[7])
				dest[7] = source[7];
			source += 128;
			dest += vid.conrowbytes;
		}
}
void Draw_Character (int x, int y, int num)
{
	Draw_Character2(x,y,num,(char*)vid.buffer);
}

/*
================
Draw_StringCenter
================
*/
void Draw_StringCenter (int x, int y, char *str)
{
	while (*str)
	{
		Draw_CharacterCenter (x, y, *str);
		str++;
		x += 8;
	}
}
/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
	byte			*dest;
	byte			*source;
	int				drawline;	
	extern byte		*draw_chars;
	int				row, col;

	if (!vid.direct)
		return;		// don't have direct FB access, so no debugchars...

	drawline = 8;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	dest = vid.direct + 312;

	while (drawline--)
	{
		dest[0] = source[0];
		dest[1] = source[1];
		dest[2] = source[2];
		dest[3] = source[3];
		dest[4] = source[4];
		dest[5] = source[5];
		dest[6] = source[6];
		dest[7] = source[7];
		source += 128;
		dest += 320;
	}
}
void Draw_PicScale (int x, int y, qpic_t *pic)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u, w, h;
	int		nw,nh,dx,dy,ddx,ddy,sw,sh;

	if(vid.buffer == 0)
		return;

	w = pic->width;
	h = pic->height;
	nw = (pic->width*256)/320;
	nh = (pic->height*192)/200;

	sw = sh = 0;

	if(x < 0)
	{
		x = 0;
		sw = -x;
	}
	if(y < 0)
	{
		y = 0;
		sh = -y;
	}
		
	if(x + nw > CON_SCREEN_WIDTH) {
		nw = CON_SCREEN_WIDTH - x;
	}
	if(y + nh > CON_SCREEN_HEIGHT) {
		nh = CON_SCREEN_HEIGHT - y;
	}
	if(nw < 0 || nh < 0)
		return;

	source = pic->data;

	dest = vid.buffer + y * vid.conrowbytes + x;

	dx = (320<<16)/256;
	dy = (200<<16)/192;

	for (v=0,ddy=(sh<<16) ; v<nh ; v++,ddy+=dy)
	{
		source = pic->data + (w*(ddy>>16));
		for (u=0,ddx=(sw<<16); u<nw ; u++,ddx+=dx)
			dest[u] = source[ddx>>16];

		dest += vid.conrowbytes;
	}
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	byte			*dest, *source;
	unsigned short	*pusdest;
	int				v, u, w, h;
	
	if(vid.buffer == 0)
		return;

	//if ((x < 0) ||
		//(x + pic->width > vid.width) ||
	//	(y < 0) //||
		//(y + pic->height > vid.height)
	//	)
	//{
		//Sys_Error ("Draw_Pic: bad coordinates");
		//return;
	//}
	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;
	w = pic->width;
	if(x + w > CON_SCREEN_WIDTH) {
		w = CON_SCREEN_WIDTH - x;
	}
	h = pic->height;
	if(y + h > CON_SCREEN_HEIGHT) {
		h = CON_SCREEN_HEIGHT - y;
	}
	if(w < 0 || h < 0)
		return;

	source = pic->data;

		dest = vid.buffer + y * vid.conrowbytes + x;

		for (v=0 ; v<h ; v++)
		{
			Q_memcpy (dest, source, w);
			dest += vid.conrowbytes;
			source += pic->width;
		}
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPicScale (int x, int y, qpic_t *pic)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u, w, h;
	int		nw,nh,dx,dy,ddx,ddy,sw,sh;

	if(vid.buffer == 0)
		return;

	w = pic->width;
	h = pic->height;
	nw = (pic->width*256)/320;
	nh = (pic->height*192)/200;

	sw = sh = 0;

	if(x < 0)
	{
		x = 0;
		sw = -x;
	}
	if(y < 0)
	{
		y = 0;
		sh = -y;
	}
		
	if(x + nw > CON_SCREEN_WIDTH) {
		nw = CON_SCREEN_WIDTH - x;
	}
	if(y + nh > CON_SCREEN_HEIGHT) {
		nh = CON_SCREEN_HEIGHT - y;
	}
	if(nw < 0 || nh < 0)
		return;

	source = pic->data;

	dest = vid.buffer + y * vid.conrowbytes + x;

	dx = (320<<16)/256;
	dy = (200<<16)/192;

	for (v=0,ddy=(sh<<16) ; v<nh ; v++,ddy+=dy)
	{
		source = pic->data + (w*(ddy>>16));
		for (u=0,ddx=(sw<<16); u<nw ; u++,ddx+=dx)
			if ( (tbyte=source[ddx>>16]) != TRANSPARENT_COLOR)
				dest[u] = tbyte;

		dest += vid.conrowbytes;
	}
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u, w, h;

	if(vid.buffer == 0)
		return;
	//if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
	//	 (unsigned)(y + pic->height) > vid.height)
	//if (x < 0 || y < 0)
	//{
		//Sys_Error ("Draw_TransPic: bad coordinates");
		//return;
	//}
	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;
		
	w = pic->width;
	if(x + w > CON_SCREEN_WIDTH) {
		w = CON_SCREEN_WIDTH - x;
	}
	h = pic->height;
	if(y + h > CON_SCREEN_HEIGHT) {
		h = CON_SCREEN_HEIGHT - y;
	}
	if(w < 0 || h < 0)
		return;

	source = pic->data;

		dest = vid.buffer + y * vid.conrowbytes + x;

		if (1 || pic->width & 7)
		{	// general
			for (v=0 ; v<h ; v++)
			{
				for (u=0 ; u<w ; u++)
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;
	
				dest += vid.conrowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;
					if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
						dest[u+1] = tbyte;
					if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
						dest[u+2] = tbyte;
					if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
						dest[u+3] = tbyte;
					if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
						dest[u+4] = tbyte;
					if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
						dest[u+5] = tbyte;
					if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
						dest[u+6] = tbyte;
					if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
						dest[u+7] = tbyte;
				}
				dest += vid.conrowbytes;
				source += pic->width;
			}
		}
}


void Draw_TransPicTranslateScale (int x, int y, qpic_t *pic, byte *translation)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u, w, h;
	int		nw,nh,dx,dy,ddx,ddy,sw,sh;

	if(vid.buffer == 0)
		return;

	w = pic->width;
	h = pic->height;
	nw = (pic->width*256)/320;
	nh = (pic->height*192)/200;

	sw = sh = 0;

	if(x < 0)
	{
		x = 0;
		sw = -x;
	}
	if(y < 0)
	{
		y = 0;
		sh = -y;
	}
		
	if(x + nw > CON_SCREEN_WIDTH) {
		nw = CON_SCREEN_WIDTH - x;
	}
	if(y + nh > CON_SCREEN_HEIGHT) {
		nh = CON_SCREEN_HEIGHT - y;
	}
	if(nw < 0 || nh < 0)
		return;

	source = pic->data;

	dest = vid.buffer + y * vid.conrowbytes + x;

	dx = (320<<16)/256;
	dy = (200<<16)/192;

	for (v=0,ddy=(sh<<16) ; v<nh ; v++,ddy+=dy)
	{
		source = pic->data + (w*(ddy>>16));
		for (u=0,ddx=(sw<<16); u<nw ; u++,ddx+=dx)
			if ( (tbyte=source[ddx>>16]) != TRANSPARENT_COLOR)
				dest[u] = translation[tbyte];

		dest += vid.conrowbytes;
	}
}

/*
=============
Draw_TransPicTranslate
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u;

	if(vid.buffer == 0)
		return;
	if (x < 0 || (x + pic->width) > CON_SCREEN_WIDTH || y < 0 ||
		 (y + pic->height) > CON_SCREEN_HEIGHT)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	source = pic->data;

		dest = vid.buffer + y * vid.conrowbytes + x;

		if (pic->width & 7)
		{	// general
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u++)
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = translation[tbyte];

				dest += vid.conrowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = translation[tbyte];
					if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
						dest[u+1] = translation[tbyte];
					if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
						dest[u+2] = translation[tbyte];
					if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
						dest[u+3] = translation[tbyte];
					if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
						dest[u+4] = translation[tbyte];
					if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
						dest[u+5] = translation[tbyte];
					if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
						dest[u+6] = translation[tbyte];
					if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
						dest[u+7] = translation[tbyte];
				}
				dest += vid.conrowbytes;
				source += pic->width;
			}
		}
}


void Draw_CharToConback (int num, byte *dest)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x])
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}

}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	int				x, y, v;
	byte			*src, *dest;
	unsigned short	*pusdest;
	int				f, fstep;
	qpic_t			*conback;
#ifdef _WIN32
	char			ver[100];
#endif
	if(vid.buffer == 0)
		return;
	conback = Draw_CachePic ("gfx/conback.lmp");
	//Con_Printf("lines: %d\n",lines);

// hack the version number directly into the pic
#ifdef _WIN32
	sprintf (ver, "(WinQuake) %4.2f", (float)VERSION);
	dest = conback->data + 320*186 + 320 - 11 - 8*strlen(ver);
#elif defined(X11)
	sprintf (ver, "(X11 Quake %2.2f) %4.2f", (float)X11_VERSION, (float)VERSION);
	dest = conback->data + 320*186 + 320 - 11 - 8*strlen(ver);
#elif defined(__linux__)
	sprintf (ver, "(Linux Quake %2.2f) %4.2f", (float)LINUX_VERSION, (float)VERSION);
	dest = conback->data + 320*186 + 320 - 11 - 8*strlen(ver);
#else
	//dest = conback->data + 320 - 43 + 320*186;
	//sprintf (ver, "%4.2f", VERSION);
#endif

	//for (x=0 ; x<strlen(ver) ; x++)
	//	Draw_CharToConback (ver[x], dest+(x<<3));
	
// draw the pic
	dest = vid.conbuffer;

	for (y=0 ; y<lines ; y++, dest += vid.conrowbytes)
	{
		v = (vid.conheight - lines + y)*200/vid.conheight;
		src = conback->data + v*320;
		if (vid.conwidth == 320)
			memcpy (dest, src, vid.conwidth);
		else
		{
			f = 0;
			fstep = 320*0x10000/vid.conwidth;
			for (x=0 ; x<vid.conwidth ; x+=4)
			{
				dest[x] = src[f>>16];
				f += fstep;
				dest[x+1] = src[f>>16];
				f += fstep;
				dest[x+2] = src[f>>16];
				f += fstep;
				dest[x+3] = src[f>>16];
				f += fstep;
			}
		}
	}
}


/*
==============
R_DrawRect8
==============
*/
void R_DrawRect8 (vrect_t *prect, int rowbytes, byte *psrc,
	int transparent)
{
	byte	t;
	int		i, j, srcdelta, destdelta;
	byte	*pdest;

	if(vid.buffer == 0)
		return;
	pdest = vid.buffer + (prect->y * vid.conrowbytes) + prect->x;

	srcdelta = rowbytes - prect->width;
	destdelta = vid.conrowbytes - prect->width;

	if (transparent)
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				t = *psrc;
				if (t != TRANSPARENT_COLOR)
				{
					*pdest = t;
				}

				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
	}
	else
	{
		for (i=0 ; i<prect->height ; i++)
		{
			memcpy (pdest, psrc, prect->width);
			psrc += rowbytes;
			pdest += vid.conrowbytes;
		}
	}
}



/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	int				x1,x2,y1,y2;
	byte			*psrc;

	x1 = x;
	y1 = y;
	x2 = x+w;
	y2 = y+h;
	if(x1 < 0)
		x1 = 0;
	if(x2 >= vid.width)
		x2 = vid.width-1;
	if(y1 < 0)
		y1 = 0;
	if(y2 >= vid.height)
		y2 = vid.height-1;

	if(vid.buffer == 0)
		return;
	if(key_dest == key_game && !cl.intermission)
	{
		while(y1 < y2)
		{
			memset(vid.buffer + x1 + (y1*vid.width),0,x2-x1);
			y1++;
		}
		//memset(vid.aliasbuffer,0,256*192);
	}
	else
	{
		memset(vid.buffer,0,SCREEN_WIDTH*SCREEN_HEIGHT);//320*200);
	}
	return;

}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	byte			*dest;
	unsigned short	*pusdest;
	unsigned		uc;
	int				u, v;

	if(vid.buffer == 0)
		return;
		dest = vid.buffer + y*vid.conrowbytes + x;
		for (v=0 ; v<h ; v++, dest += vid.conrowbytes)
			for (u=0 ; u<w ; u++)
				dest[u] = c;
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	int			x,y;
	byte		*pbuf;

	//if(vid.buffer == 0)
		return;
	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();

	for (y=0 ; y<CON_SCREEN_HEIGHT ; y++)
	{
		int	t;

		pbuf = (byte *)(vid.buffer + vid.conrowbytes*y);
		t = (y & 1) << 1;

		for (x=0 ; x<CON_SCREEN_WIDTH ; x++)
		{
			if ((x & 3) != t)
				pbuf[x] = 0;
		}
	}

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
#ifdef NDS
	*( ((u16*)SCREEN_BASE_BLOCK(15)) + 2) = 0xf00b;
#endif
	//D_BeginDirectRect (vid.width - 24, 0, draw_disc->data, 24, 24);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
#ifdef NDS
	*( ((u16*)SCREEN_BASE_BLOCK(15)) + 2) = 0xf020;
#endif
	//D_EndDirectRect (vid.width - 24, 0, 24, 24);
}

extern qboolean	scr_drawloading;
extern cvar_t		ds_hud_alpha;

void Draw_UpdateVRAM()
{
	unsigned short *dest16;
	pix16_t p16;
	byte *src8;
	int xstep,ystep,x,y,xx,yy;

	if(vid.buffer == 0)
		return;

// draw the pic
	if(key_dest == key_game && 
		!cl.intermission && 
		cls.state == ca_connected && 
		!scr_drawloading &&
		!scr_disabled_for_loading)
	{
		src8 = 0;
#ifdef NDS
extern u16 *ds_display_bottom;
extern int ds_display_bottom_height;
int x,y;
BLEND_CR = BLEND_SRC_BG3|BLEND_DST_BG0|BLEND_DST_BG1|BLEND_ALPHA;
BLEND_AB = ((int)ds_hud_alpha.value)|(31<<8);
	if(sb_lines)
	{
		dest16 = ((u16*)BG_BMP_RAM(2)) + ((vid.height - sb_lines)*(vid.conwidth>>1));
		src8 = vid.buffer + ((vid.conheight - sb_lines)*vid.conwidth);
		dmaCopyWords(0, (uint32*)src8,(uint32*)dest16, vid.conwidth*sb_lines);
	}
#endif
		return;
	}

#ifdef NDS
extern u16 *ds_display_menu;
	dest16 = ds_display_menu;
BLEND_CR = 0;
BLEND_AB = 0;
#else
	dest16 = (unsigned short *)vid.conbuffer;
#endif

#ifdef NDS
	dmaCopyWords(0, (uint32*)vid.buffer,(uint32*)dest16, SCREEN_WIDTH*SCREEN_HEIGHT);
#else
	ystep = (200<<16)/SCREEN_HEIGHT;
	xstep = (320<<16)/SCREEN_WIDTH;

	yy = 0;
	for (y=0 ; y<SCREEN_HEIGHT; y++)
	{
		src8 = vid.buffer + ((yy>>16)*CON_SCREEN_WIDTH);
		xx = 0;
		for (x=0 ; x<SCREEN_WIDTH ; x+=4)
		{
			p16.b[0] = src8[xx>>16];
			xx += xstep;
			p16.b[1] = src8[xx>>16];
			xx += xstep;
			*dest16++ = p16.s;

			p16.b[0] = src8[xx>>16];
			xx += xstep;
			p16.b[1] = src8[xx>>16];
			xx += xstep;
			*dest16++ = p16.s;
		}
		yy += ystep;
	}
#endif
}
