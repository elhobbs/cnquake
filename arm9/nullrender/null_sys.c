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
// sys_null.h -- null system driver to aid porting efforts
#include "quakedef.h"
#include "errno.h"
#include "dsrumble.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
//#include <gl\glaux.h>

static double		pfreq;
static double		curtime = 0.0;
static double		lastcurtime = 0.0;
static int			lowshift;

#endif

#ifdef NDS
#include <fat.h>
//#include "IPCFifo.h"
#include "cyg-profile.h"
#include "fat\partition.h"
//#include "nds/arm9/dldi.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>


u16		*ds_display_top; 
u16		*ds_display_bottom;
int		ds_display_bottom_height;
u16		*ds_display_menu; 

int ds_bg_sub = 0;
int ds_bg_main = 0;
int ds_bg_text = 0;

#ifdef USE_WIFI
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#define KEYS_CUR (( ((~REG_KEYINPUT)&0x3ff) | (((~IPC->buttons)&3)<<10) | (((~IPC->buttons)<<6) & (KEY_TOUCH|KEY_LID) ))^KEY_LID)
//const DISC_INTERFACE* dldi_disc;

void Sys_Wait(float t);
void waitforit(void)
{

	if(developer.value)
	{
	Con_Printf("\nwait for it...");
	while((keysCurrent() & KEY_A) == 0);
	Con_Printf("done.\n");
	Sys_Wait(0.2f);
	}
}
#else
void waitforit(void)
{
}
#endif

qboolean			isDedicated;
//extern const GBFS_FILE  data_gbfs;

#ifdef _WIN322
byte        scantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,0  ,    0  , K_HOME, 
	K_UPARROW,K_PGUP,'-',K_LEFTARROW,'5',K_RIGHTARROW,'+',K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
					}; 

byte        shiftscantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '!',    '@',    '#',    '$',    '%',    '^', 
	'&',    '*',    '(',    ')',    '_',    '+',    K_BACKSPACE, 9, // 0 
	'Q',    'W',    'E',    'R',    'T',    'Y',    'U',    'I', 
	'O',    'P',    '{',    '}',    13 ,    K_CTRL,'A',  'S',      // 1 
	'D',    'F',    'G',    'H',    'J',    'K',    'L',    ':', 
	'"' ,    '~',    K_SHIFT,'|',  'Z',    'X',    'C',    'V',      // 2 
	'B',    'N',    'M',    '<',    '>',    '?',    K_SHIFT,'*', 
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,0  ,    0  , K_HOME, 
	K_UPARROW,K_PGUP,'_',K_LEFTARROW,'%',K_RIGHTARROW,'+',K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
					}; 
#endif
/*
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
#if 1

typedef struct {
	FILE* fptr;
	int readonly;
	int start;
	int length;
#ifdef NDS
	PARTITION* partition;
	u32 startSector;
	u32 currentPosition;
#endif
}qfile_t;
qfile_t sys_handles[MAX_HANDLES];
#else
FILE	*sys_handles[MAX_HANDLES];
#endif
int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (sys_handles[i].fptr == 0)
			return i;
	Sys_Error ("out of handles");
	return -1;
}
#ifdef NDS
size_t freadj ( const void * ptr, size_t size, size_t count, FILE * stream ) 
{ 
   size_t TotalBytes = size * count; 
   int offset = 0;
   byte *p = (byte *)ptr;
    
   //Write 256 byte blocks 
   while ((TotalBytes - offset) >= 256) 
   { 
      fread(p + offset, 256, 1, stream); 
      offset += 256; 
   } 
    
   //Write 16 byte blocks 
   while ((TotalBytes - offset) >= 16) 
   { 
      fread(p + offset, 16, 1, stream); 
      offset += 16; 
   } 
    
   //Write 1 byte blocks 
   while ((TotalBytes - offset) >= 1) 
   { 
      fread(p + offset, 1, 1, stream); 
      offset += 1; 
   } 

   return offset; 
} 
size_t fwritej ( const void * ptr, size_t size, size_t count, FILE * stream ) 
{ 
   size_t TotalBytes = size * count; 
   int offset = 0; 
   byte *bptr = (byte *)ptr;
    
   //Write 256 byte blocks 
   while ((TotalBytes - offset) >= 256) 
   { 
      fwrite(bptr + offset, 256, 1, stream); 
      offset += 256; 
   } 
    
   //Write 16 byte blocks 
   while ((TotalBytes - offset) >= 16) 
   { 
      fwrite(bptr + offset, 16, 1, stream); 
      offset += 16; 
   } 
    
   //Write 1 byte blocks 
   while ((TotalBytes - offset) >= 1) 
   { 
      fwrite(bptr + offset, 1, 1, stream); 
      offset += 1; 
   } 

   return offset; 
} 
#endif
/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int		pos;
	int		end;
	//int		t;

//	t = VID_ForceUnlockedAndReturnState ();

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

//	VID_ForceLockState (t);

	return end;
}

#ifdef NDS
#define CLUSTER_FIRST	0x00000002
static inline sec_t _FAT_fat_clusterToSector (PARTITION* partition, uint32_t cluster) {
	return (cluster >= CLUSTER_FIRST) ? 
		((cluster - CLUSTER_FIRST) * (sec_t)partition->sectorsPerCluster) + partition->dataStart : 
		partition->rootDirStart;
}
#endif
int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int		hf, retval;
#ifdef NDS
struct stat st; 
u32 cluster; 
#endif
	//char filename[256];
	//strncpy(filename, path, 256);
	//int		t;

//	t = VID_ForceUnlockedAndReturnState ();

	hf = findhandle ();
	
#ifdef NDS
	f = fopen(path, "r");
#else
	f = fopen(path, "rb");
#endif

	//Con_Printf("o: %d %x\n",hf,f);

	if (!f)
	{
		int e = errno;
		Con_Printf("Sys_FileOpenRead: %s failed!!\n",path);
		Con_Printf("%s %d\n",strerror(e),e);
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[hf].fptr = f;
		*hndl = hf;
		retval = filelength(f);
		sys_handles[hf].length = retval;
		sys_handles[hf].start = 0;
		sys_handles[hf].readonly = 1;
#ifdef NDS2
		sys_handles[hf].partition = _FAT_partition_getPartitionFromPath(path);
		stat (path, &st); 
		cluster = st.st_ino;
		sys_handles[hf].startSector = _FAT_fat_clusterToSector(sys_handles[hf].partition,cluster);
		sys_handles[hf].currentPosition = 0;

#endif
	}

//	VID_ForceLockState (t);

	return retval;
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int		i;
	//int		t;

//	t = VID_ForceUnlockedAndReturnState ();
	
	i = findhandle ();

	if(*path == '.' && *(path+1) == '\\') {
		f = fopen(path+2, "w");
		
	} else {
		f = fopen(path, "w");
	}
	if (!f){
		int e = errno;
		//iprintf("Sys_FileOpenWrite: failed\n%s\n",path);
		Sys_Error ("Error opening %s: %s %d", path,strerror(e),e);
	}
	sys_handles[i].fptr = f;
	sys_handles[i].start = 0;
	sys_handles[i].length = 0;
	sys_handles[i].readonly = 0;
	
	Con_Printf("fopen: %d %x\n",i,f);
	waitforit();
	
//	VID_ForceLockState (t);

	return i;
}

int Sys_FilePosition(int handle) {
	if(sys_handles[handle].readonly)
		return ftell(sys_handles[handle].fptr) - sys_handles[handle].start;
	return ftell(sys_handles[handle].fptr);
}

void Sys_SubFile(int handle,int start,int length)
{
	if(sys_handles[handle].readonly)
	{
		sys_handles[handle].length = length;
		sys_handles[handle].start = start;
#ifdef NDS
		sys_handles[handle].currentPosition = sys_handles[handle].start;
#endif
	}
}

void Sys_FileClose (int handle)
{
	//int		t;

//	t = VID_ForceUnlockedAndReturnState ();
	fflush(sys_handles[handle].fptr);
	fclose (sys_handles[handle].fptr);
	sys_handles[handle].fptr = 0;
	sys_handles[handle].length = 0;
	sys_handles[handle].readonly = 0;
	sys_handles[handle].start = 0;
#ifdef NDS
		sys_handles[handle].currentPosition = 0;
#endif
//	VID_ForceLockState (t);
}

void Sys_FileSeek (int handle, int position)
{
	//int		t;

//	t = VID_ForceUnlockedAndReturnState ();
	if(sys_handles[handle].readonly)
	{
#ifdef NDS2
		sys_handles[handle].currentPosition = position+sys_handles[handle].start;
#else
		fseek (sys_handles[handle].fptr, position+sys_handles[handle].start, SEEK_SET);
#endif
	}
	else
	{
		fseek (sys_handles[handle].fptr, position, SEEK_SET);
#ifdef NDS
		sys_handles[handle].currentPosition = position;
#endif
	}
//	VID_ForceLockState (t);
}


int nds_readFile(int handle, void *dest_r, int count)
{
int i;
#ifdef NDS
	const DISC_INTERFACE* disc = sys_handles[handle].partition->disc;
	u32 current = sys_handles[handle].currentPosition;
	u32 preBytes = current % BYTES_PER_READ;
	u32 sector = sys_handles[handle].startSector + (current/BYTES_PER_READ);
	int cb_read = 0, to_read = count,numFull,cb;
	byte *dest = (byte*)dest_r;
	byte buf[512];

	if(preBytes) {
		_FAT_disc_readSectors(disc,sector,1,(void *)buf);
		cb = BYTES_PER_READ-preBytes;
		if(cb > to_read)
		{
			cb = to_read;
		}
		memcpy(dest,buf+preBytes,cb);
		cb_read += cb;
		to_read -= cb;
		dest += cb;
		sector++;
	}
	numFull = to_read/BYTES_PER_READ;
#if 0
		_FAT_disc_readSectors(disc,sector,numFull,(void *)dest);
		cb_read += (BYTES_PER_READ*numFull);
		to_read -= (BYTES_PER_READ*numFull);
		dest += (BYTES_PER_READ*numFull);
		//numFull--;
		sector+=numFull;
#else
	while(numFull)
	{
		_FAT_disc_readSectors(disc,sector,1,(void *)dest);
		cb_read += BYTES_PER_READ;
		to_read -= BYTES_PER_READ;
		dest += BYTES_PER_READ;
		numFull--;
		sector++;
	}
#endif
	if(to_read)
	{
		_FAT_disc_readSectors(disc,sector,1,(void *)buf);
		memcpy(dest,buf,to_read);
		cb_read += to_read;
		to_read -= to_read;
		dest += preBytes;
	}
	sys_handles[handle].currentPosition += cb_read;
	return cb_read;
#else
	int x = fread(dest_r, 1, count, sys_handles[handle].fptr);
	return x;
#endif
}

int Sys_FileRead (int handle, void *dest, int count)
{
	//int		t;
	int		x;
	int tIME;

	//	t = VID_ForceUnlockedAndReturnState ();
#ifdef NDS2
	x = nds_readFile(handle,dest,count);
	//x = freadj (dest, 1, count, sys_handles[handle].fptr);
#else
	x = fread (dest, 1, count, sys_handles[handle].fptr);
#endif
//	VID_ForceLockState (t);
	return x;
}

int Sys_FileWrite (int handle, void *data, int count)
{
	//int		t;
	int		x;

//	t = VID_ForceUnlockedAndReturnState ();
#ifdef NDS2
	x = fwritej (data, 1, count, sys_handles[handle].fptr);
#else
	x = fwrite (data, 1, count, sys_handles[handle].fptr);
	if (x != count)
	{
		int e = errno;
		Con_Printf("fwrite failed!!\n");
		Con_Printf("%s %d %x\n",strerror(e),e,sys_handles[handle].fptr);
	}
#endif
//	VID_ForceLockState (t);
	return x;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	int		retval;
	//int		t;

//	t = VID_ForceUnlockedAndReturnState ();
	
	f = fopen(path, "r");

	if (f)
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}
	
//	VID_ForceLockState (t);
	return retval;
}

void Sys_mkdir (char *path)
{
	//_mkdir (path);
}

void show_overlay(qboolean show,qboolean showkeys)
{
#ifdef NDS
	if(show)
	{
		//bgSetPriority(3,1);
		//bgSetPriority(0,2);
	}
	else
	{
		//bgSetPriority(3,2);
		//bgSetPriority(0,1);
		memset(((u16*)BG_BMP_RAM(2)), 0,vid.width*vid.height);
		memset((char *)BG_BMP_RAM_SUB(0),0,vid.height*vid.width);
		if(vid.buffer)
			memset(vid.buffer,0,vid.width*vid.height);
	}
	bgSetPriority(3,1);
	bgSetPriority(0,2);
	int i;
	for(i=0;i<32*32;i++)
	{
		((u16*)SCREEN_BASE_BLOCK(15))[i] = (u16)' ';
	}
#endif
	if(showkeys)
	{
		Cvar_Set("keyboard","1");
	}
	else
	{
		Cvar_Set("keyboard","0");
	}
}

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}

int fifoError(char *buffer, ...) // expected to be defined externally.
{
	va_list         argptr;
	va_start (argptr,buffer);

	Sys_Error (buffer,argptr);

	va_end (argptr);
	return 0;
}

#ifdef WIN32
#define iprintf printf
#endif

void Sys_Error (char *error, ...)
{
	va_list         argptr;

	iprintf ("Sys_Error: ");   
	va_start (argptr,error);
#ifdef NDS
	viprintf (error,argptr);
#else
	vprintf (error,argptr);
#endif
	va_end (argptr);
	iprintf ("\n");

#ifdef NDS
	REG_BG0CNT |= BG_PRIORITY_3;
	REG_BG2CNT |= BG_PRIORITY_3;
	REG_BG3CNT |= BG_PRIORITY_3;
	REG_BG1CNT &= (~BG_PRIORITY_3);
#endif
	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list         argptr;
	
	va_start (argptr,fmt);
#ifdef NDS
	viprintf (fmt,argptr);
#else
	vprintf (fmt,argptr);
#endif
	va_end (argptr);
}

#ifdef __cplusplus
extern "C" {
#endif

void sgIP_dbgprint(char *fmt, ...)
{
	va_list         argptr;
	
	va_start (argptr,fmt);
	Con_Printf (fmt,argptr);
	va_end (argptr);
}
#ifdef __cplusplus
};
#endif

void Sys_Quit (void)
{
	exit (0);
}

#ifdef _WIN32
void Sys_SetFPCW (void)
{
}

void Sys_PushFPCW_SetHigh (void)
{
}

void Sys_PopFPCW (void)
{
}

void MaskExceptions (void)
{
}
double Sys_FloatTime (void)
{
	static int			sametimecount;
	static unsigned int	oldtime;
	static int			first = 1;
	LARGE_INTEGER		PerformanceCount;
	unsigned int		temp, t2;
	double				time;

	Sys_PushFPCW_SetHigh ();

	QueryPerformanceCounter (&PerformanceCount);

	temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
		   ((unsigned int)PerformanceCount.HighPart << (32 - lowshift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
	// check for turnover or backward time
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp;	// so we can't get stuck
		}
		else
		{
			t2 = temp - oldtime;

			time = (double)t2 * pfreq;
			oldtime = temp;

			curtime += time;

			if (curtime == lastcurtime)
			{
				sametimecount++;

				if (sametimecount > 100000)
				{
					curtime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			lastcurtime = curtime;
		}
	}

	Sys_PopFPCW ();

    return curtime;
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime (void)
{
	int		j;

	Sys_FloatTime ();

	j = COM_CheckParm("-starttime");

	if (j)
	{
		curtime = (double) (Q_atof(com_argv[j+1]));
	}
	else
	{
		curtime = 0.0;
	}

	lastcurtime = curtime;
}
int Sys_IntTime (void)
{
	return Sys_FloatTime()*32728.5;
}
#else

long long ds_time()
{
	static u16 last;
	static long long t;
	u16 time1 = TIMER1_DATA;
	u16 time = TIMER2_DATA;
	if(time < last) {
		t += (1<<32);
	}
	last = time;
	return (t + (time << 16) + time1);
}
int Sys_IntTime (void)
{
	return ds_time();
}
double Sys_FloatTime (void)
{
	return ds_time()/11025.0;
}
#endif

char *Sys_ConsoleInput(void)
{
	static char	text[256];
	static int	len = 0;
	char		ch;

	if (!isDedicated)
		return NULL;

#ifdef WIN32
	if (! kbhit())
		return NULL;

	ch = getche();

	switch (ch)
	{
		case '\r':
			putch('\n');
			if (len)
			{
				text[len] = 0;
				len = 0;
				return text;
			}
			break;

		case '\b':
			putch(' ');
			if (len)
			{
				len--;
				putch('\b');
			}
			break;

		default:
			text[len] = ch;
			len = (len + 1) & 0xff;
			break;
	}
#endif
	return NULL;
}

void Sys_Sleep (void)
{
}

void Sys_Wait(float t)
{
	float end = Sys_FloatTime () + t;
		
	while(1) {
		if(Sys_FloatTime () > end)
			break;
	}

}

#ifdef WIN32

void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
	// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}
}
#else
void IN_keyboard (void);
void Sys_SendKeyEvents (void)
{
	scanKeys();
	IN_keyboard();
}
#endif


void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

#if 0
void DO_BUFFER(u16* dest,u16* src)
{
	int i;
	u16 *fb1,*fb2,c16;
	fb1 = (u16*)dest;
	fb2 = (u16*)src;
	
	for(i=0;i<0xC000/*256*192*/;i++) {
		c16 = (*fb2++);
		*fb1++ = c16;
	}
}
#endif

#define MAX_CMD_LINE (512)
char		*argv[MAX_NUM_ARGVS];
char		cmdLine[MAX_CMD_LINE+1];
static char	*empty_string = "";

void read_cquake_ini(quakeparms_t    *parms)
{
	FILE *f;
	int cb;
	char *lpCmdLine;

	lpCmdLine = &cmdLine[0];
	memset(lpCmdLine,0,sizeof(cmdLine));
#ifdef USE_WIFI
	f = fopen("cquake_mp.ini","r");
#else
	f = fopen("cquake.ini","r");
#endif
	if(f == 0) {
#ifdef USE_WIFI
		f = fopen("/cquake_mp.ini","r");
#else
		f = fopen("/cquake.ini","r");
#endif
	}
	if(f)
	{
		cb = fread(&cmdLine[0],1,MAX_CMD_LINE,f);
		fclose(f);
	}
	

	parms->argc = 1;
	argv[0] = empty_string;

	while (*lpCmdLine && (parms->argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[parms->argc] = lpCmdLine;
			parms->argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

	parms->argv = argv;

	COM_InitArgv (parms->argc, parms->argv);

	parms->argc = com_argc;
	parms->argv = com_argv;
}
static void ds_net_draw() {
#ifdef NDS
	// Clear the screen
	iprintf ("\x1b[2J");

	iprintf("Choose a mode\nA to select");
	// Move to 2nd row
	iprintf ("\x1b[2;0H");
	// Print line of dashes
	iprintf ("--------------------------------");

	// Set row
	iprintf ("\x1b[3;0H [   Single Player   ]");
	iprintf ("\x1b[4;0H [   Access Point    ]");
	iprintf ("\x1b[5;0H [ Connect to DSNIFI ]");
	iprintf ("\x1b[6;0H [ Create new DSNIFI ]");
#endif
}

#ifdef NDS
void dsnifi_connect_to_ap();
void dsnifi_start_host();
#endif

static void ds_net_choose() {
#ifdef NDS
	int pressed = 0, pos = 0;

	ds_net_draw();

	while (true) {
		int i;
		// Clear old cursors
		for (i = 0; i < 4; i++) {
			iprintf ("\x1b[%d;0H ", i+3);
		}
		// Show cursor
		iprintf ("\x1b[%d;0H*", pos + 3);
		
		// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
		do {
			scanKeys();
			pressed = keysDown();
			swiWaitForVBlank();
		} while (!pressed);
	
		if (pressed & KEY_UP)
		{
			pos -= 1;
			if (pos < 0)
			{
				pos = 4 - 1;		// Wrap around to bottom of list
			}
		}
		if (pressed & KEY_DOWN)
		{
			pos += 1;
			if (pos >= 4)
			{
				pos = 0;		// Wrap around to top of list
			}
		}
		
		
		if (pressed & KEY_A) {
			int dhcp = 0;

			// Clear the screen
			iprintf ("\x1b[2J");
			if(pos != 0) {
				wifi_init();
			}

			switch (pos) {
			case 1:
				dhcp = 1;
			case 2:
				printf("searching...\n");
				dsnifi_connect_to_ap();
				break;
			case 3:
				printf("starting host...\n");
				dsnifi_start_host();
				break;
			}
			wifi_fnet_init(dhcp);
			return;
		}	
	}
#endif
}

#define DIR_LIST_COUNT 20
void ds_choose_draw(char *dirlist[],int total,int pos)
{
	int i;
	int start = (pos/DIR_LIST_COUNT)*DIR_LIST_COUNT;
	int end = start + DIR_LIST_COUNT;

	if(end > total)
		end = total;
#ifdef NDS
	// Clear the screen
	iprintf ("\x1b[2J");

	iprintf("Choose a mod game directory\nA to select or B to cancel");
	// Move to 2nd row
	iprintf ("\x1b[2;0H");
	// Print line of dashes
	iprintf ("--------------------------------");
	for(i = start;i < end;i++)
	{
		// Set row
		iprintf ("\x1b[%d;0H", i-start + 3);
		iprintf (" [%s]", dirlist[i]);
	}
	if(end < total)
	{
		// Set row
		iprintf ("\x1b[%d;0H", i-start + 3);
		iprintf (" more...", dirlist[i]);
	}
#endif
}

void ds_choose_game(char *base)
{
	char *buf;
	char *dirlist[DIR_LIST_COUNT*3];
	int start,dircount = 0;
	int pos = 0;
	int len,pressed,ret;
	int i = COM_CheckParm ("-listgame");

	if(i == 0)
		return;

	buf = (char *)Hunk_TempAlloc(4096);
#ifdef NDS
	{
		struct stat st;
		char filename[256];
		DIR *dir;
		struct dirent *pent;
		
		if(base == 0 || *base == 0)
		{
			dir = opendir ("/"); 
		}
		else
		{
			dir = opendir (base);
			chdir(base);
		}


		if (dir == NULL) {
			iprintf ("Unable to open the directory.\n");
		} else {
			while ((pent=readdir(dir))!=NULL) {
	    		ret = stat(pent->d_name,&st);
				if(S_ISDIR(st.st_mode))
				{
					if(strcasecmp(pent->d_name,GAMENAME) == 0 || 
						strcmp(pent->d_name,"..") == 0 ||
						strcmp(pent->d_name,".") == 0)
						continue;
					len = strlen(pent->d_name) + 1;
					if(pos + len >= 4096)
						break;

					dirlist[dircount++] = &buf[pos];
					strcpy(&buf[pos],pent->d_name);
					pos += len + 1;
										
				}
				if(dircount >= DIR_LIST_COUNT*3)
					break;
			}
		}	
		
		closedir (dir);	
	}
	chdir("/");

	start = pressed = pos = 0;

	ds_choose_draw(dirlist,dircount,pos);

	while (true) {
		int i;
		// Clear old cursors
		for (i = 0; i < DIR_LIST_COUNT; i++) {
			iprintf ("\x1b[%d;0H ", i+3);
		}
		// Show cursor
		iprintf ("\x1b[%d;0H*", (pos%DIR_LIST_COUNT) + 3);
		
		// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
		do {
			scanKeys();
			pressed = keysDown();
			swiWaitForVBlank();
		} while (!pressed);
	
		if (pressed & KEY_UP)
		{
			pos -= 1;
			if (pos < 0)
			{
				pos = dircount - 1;		// Wrap around to bottom of list
				ds_choose_draw(dirlist,dircount,pos);
			}
			else if(pos % DIR_LIST_COUNT == (DIR_LIST_COUNT-1))
			{
				ds_choose_draw(dirlist,dircount,pos);
			}
		}
		if (pressed & KEY_DOWN)
		{
			pos += 1;
			if (pos >= dircount)
			{
				pos = 0;		// Wrap around to top of list
				ds_choose_draw(dirlist,dircount,pos);
			}
			else if(pos % DIR_LIST_COUNT == 0)
			{
				ds_choose_draw(dirlist,dircount,pos);
			}
		}
		
		
		if (pressed & KEY_A) {
extern qboolean        com_modified;   // set true if using non-id files
void COM_AddGameDirectory (char *dir);

			// Clear the screen
			iprintf ("\x1b[2J");
			//iprintf("adding game dir:\n\n%s/%s\n",base,dirlist[pos]);
			com_modified = true;
			COM_AddGameDirectory (va("%s/%s", base, dirlist[pos]));
			break;
		}
		
		if (pressed & KEY_B) {
			// Clear the screen
			iprintf ("\x1b[2J");
			iprintf("canceled gamedir selection\n");
			break;
		}
	}
#endif
	//while(1);
}

#ifdef ARM9
extern bool __dsimode;
#endif

int enable_texture_cache = 1;

void quake_main (int argc, char **argv)
{
	float rotateX = 0.0;
	float rotateY = 0.0;
	double newtime,oldtime,time;

	static quakeparms_t    parms;
	static int i = 0;
#ifdef NDS
	unsigned amt = MINIMUM_MEMORY;//4*1024*1024;
#else
	unsigned amt = 14*1024*1024;//MINIMUM_MEMORY;
#endif
	int frame = 0;

#ifdef WIN32
	parms.basedir = ".";
	COM_InitArgv (argc, argv);
#else
	parms.basedir = "";
	read_cquake_ini(&parms);
#endif

	if (COM_CheckParm ("-testmem"))
	{
		parms.memsize = 4*1024*1024;
		while(parms.memsize > 0) {
			parms.membase = malloc (parms.memsize);
			if(parms.membase)
				break;
			parms.memsize -= 1024;
		}
		iprintf("default: %d\n",MINIMUM_MEMORY);
		iprintf("alloced: %d\n",parms.memsize);
		iprintf("try setting -heapsize to:\n");
		iprintf("%d\n",(parms.memsize/1024)-16);
		Sys_Error("testmem finished\ndon't forget to remove -testmem from cquake.ini\n");
	}
#ifdef NDS
	if(__dsimode) {
		parms.memsize = 13.5f*1024*1024;
	} else {
		parms.memsize = MINIMUM_MEMORY;
		enable_texture_cache = 0;
	}
#endif

#ifdef WIN32
	//parms.memsize = MINIMUM_MEMORY;
	//enable_texture_cache = 0;
	parms.memsize = 14*1024*1024;//MINIMUM_MEMORY;
#endif

	if (COM_CheckParm ("-heapsize2"))
	{
		int t = COM_CheckParm("-heapsize2") + 1;

		if (t < com_argc)
			parms.memsize = Q_atoi (com_argv[t]) * 1024;
	}
	parms.membase = malloc (parms.memsize);
#if 0
	while(amt > 0) {
		parms.memsize = amt;
		parms.membase = malloc (amt);
		if(parms.membase)
			break;
		amt -= 1024;
	}
#endif
	if(parms.membase == 0) {
		Sys_Error("failed to allocate membase\n");
	}
	iprintf ("\n\nmem: %p %d\n",parms.membase,parms.memsize);

	//COM_InitArgv (argc, argv);
	if (COM_CheckParm ("-rumble"))
	{
		int t = COM_CheckParm("-rumble") + 1;

		if (t < com_argc)
		{
			if(strcmp("3in1",com_argv[t]) == 0)
			{
				ds_rumble_3in1_init();
			}
		}
	}

	parms.argc = com_argc;
	parms.argv = com_argv;

	Host_Init (&parms);
	VID_SetPalette (host_basepal);

	oldtime = Sys_FloatTime();
	while (1)
	{
		//swiWaitForVBlank();
		
		//Host_Frame (0.1);
		//Con_Printf("@");
		newtime = Sys_FloatTime ();
		time = newtime - oldtime;

		Host_Frame (time);
		oldtime = newtime;
		//Con_Printf ("%f fps\n", 1.0/time);
		//Con_Printf("%f %f %f\n",newtime,oldtime,time);
		
		//Con_Printf("%d %d\n",ds_rumble_count,rumble_freq);
		frame++;
	}
}

#ifdef NDS

#ifdef __cplusplus
extern "C" {
#endif

void systemErrorExit(int rc) { 
   printf("exit with code %d\n",rc); 

   while(1) { 
      swiWaitForVBlank(); 
      if (keysCurrent() & KEY_A) break; 
   } 
    
} 

#ifdef __cplusplus
};
#endif

#if 0
u32 getExceptionAddress( u32 opcodeAddress, u32 thumbState);
static const char *registerNames[] =
	{	"r0","r1","r2","r3","r4","r5","r6","r7",
		"r8 ","r9 ","r10","r11","r12","sp ","lr ","pc " };

static void MYdefaultHandler() {
//---------------------------------------------------------------------------------

	iprintf("\n\x1b[5CGuru Meditation Error!\n\n");
	u32	currentMode = getCPSR() & 0x1f;
	u32 thumbState = ((*(u32*)0x027FFD90) & 0x20);

	u32 codeAddress, exceptionAddress = 0;

	int offset = 8;

	if ( currentMode == 0x17 ) {
		iprintf ("\x1b[10Cdata abort!\n\n");
		codeAddress = exceptionRegisters[15] - offset;
		exceptionAddress = getExceptionAddress( codeAddress, thumbState);
	} else {
		if (thumbState)
			offset = 2;
		else
			offset = 4;
		iprintf("\x1b[5Cundefined instruction!\n\n");
		codeAddress = exceptionRegisters[15] - offset;
		exceptionAddress = codeAddress;
	}


	iprintf("  pc: %08X addr: %08X\n\n",codeAddress,exceptionAddress);

	int i;
	for ( i=0; i < 8; i++ ) {
		iprintf(	"  %s: %08X   %s: %08X\n",
					registerNames[i], exceptionRegisters[i],
					registerNames[i+8],exceptionRegisters[i+8]);
	}

	//while(1);

}
#endif
extern volatile int in_sleep_mode;

void IPC_Power(u32 command, const u32 *data, u32 wordCount)
{
	switch(command)
	{
	case 1:
		in_sleep_mode = 0;
		break;
	}
}

void IPCReceiveUser1(u32 command, const u32 *data, u32 wordCount)
{
	int i;
	Con_Printf("\n\nsound:");
	for(i=0;i<wordCount;i++)
	{
		Con_Printf(" %x",*(data+i));
	}
	Con_Printf("\n\n");
}

#ifdef USE_WIFI
#if 0
void Wifi_timer_50ms(void)
{
   Wifi_Timer(50);
}

// notification function to send fifo message to arm7
void Wifi_arm9_synctoarm7()
{ // send fifo message
   //REG_IPC_FIFO_TX=0x87654321;
	IPCFifoSendWordAsync(FIFO_SUBSYSTEM_WIFI,2,(u32)0x2004);
}

void IPC_Wifi(u32 command, const u32 *data, u32 wordCount)
{
	switch(command)
	{
	case 0: //arm7 init
		//Con_Printf("arm7 init\n");
		break;
	case 1: //wifi init
		//Con_Printf("wifi init\n");
		break;
	case 2: //sync
		//Con_Printf("wifi sync\n");
		break;
	}
}
#endif
#endif

//void irqVBlank(void) {	
//---------------------------------------------------------------------------------
//	scanKeys();
//}



extern const u8 default_font_bin[];

void VID_loadPal();

#ifdef USE_DSNIFI

	int wifi_init();

#endif


void Sys_Init()
{
	bool ret;
	int x, y;
	// Turn on everything
	//powerON(POWER_ALL);

	//put 3D on top
	lcdMainOnTop();
	
    vramSetMainBanks(VRAM_A_TEXTURE, VRAM_B_TEXTURE, VRAM_C_TEXTURE, VRAM_D_TEXTURE);
	vramSetBankE(VRAM_E_MAIN_BG);
	vramSetBankF(VRAM_F_TEX_PALETTE);
	vramSetBankG((VRAM_G_TYPE)(1 | VRAM_OFFSET(2)));
	vramSetBankH(VRAM_H_SUB_BG); 
	vramSetBankI(VRAM_I_SUB_BG_0x06208000); 

	// Subscreen as a console
	videoSetModeSub(MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG2_ACTIVE/* | DISPLAY_BG3_ACTIVE*/);
	
	ds_bg_sub = bgInitSub(2,BgType_Bmp8,BgSize_B8_256x256,0,0);


	videoSetMode(MODE_3_3D | DISPLAY_BG1_ACTIVE | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
	bgSetPriority(0,3);
	
	
	//setup stdout
	consoleInit(0,1, BgType_Text4bpp, BgSize_T_256x256, 14,0, true,true);
	BG_PALETTE[0]=0;	
	BG_PALETTE[255]=0xffff;

	//setup text layer for fps, center print, and Con_Printf temp overlay
	ds_bg_text = bgInit(2,BgType_Text4bpp,BgSize_T_256x256,15,0);
	bgSetPriority(ds_bg_text,0);

	//setup top bitmap for console
	ds_bg_main = bgInit(3,BgType_Bmp8,BgSize_B8_256x256,2,0);
	bgSetPriority(ds_bg_main,1);
	

	/*for (y=0; y < 192; y++)
	{
		for (x=0; x < 128; x++)
		{
			u8 first_pixel 	= y;
			u8 second_pixel = y + 1;
			u16 two_pixels = first_pixel | (second_pixel << 8);
			
			//((u16*)BG_BMP_RAM(2))[y * 128 + x] = two_pixels;
			((u16*)bgGetGfxPtr(ds_bg_main))[y * 128 + x] = two_pixels;
		}
	}*/
	
	ds_display_bottom_height = 192;
	ds_display_menu = ds_display_bottom = (u16*)bgGetGfxPtr(ds_bg_main);
	ds_display_top = (u16*)bgGetGfxPtr(ds_bg_sub);


	irqSet(IRQ_VBLANK, VID_loadPal);

#ifdef USE_WIFI
Wifi_InitDefault(true);
#endif

#ifdef USE_DSNIFI

	ds_net_choose();

#endif

	lcdMainOnBottom();
	
	glInit();
	//glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_2D);

#if 0
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(1);
#endif

	// Specify the Clear Color and Depth 
	// setup the rear plane
	glClearColor(0,0,0,31); // BG must be opaque for AA to work
	glClearPolyID(63); // BG must have a unique polygon ID for AA to work
	//glClearDepth(0x7FFF);

	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT | POLY_ID(0) | (1<<13));
	glCutoffDepth(0x7FFF);

	Con_Printf("Initialing disk...");
	ret = fatInitDefault();
	Con_Printf("%s\n",ret ? "done\n" : "failed!\n");
	if(ret == 0)
	{
		do {
			swiWaitForVBlank();
		}while(1);
	}
	
	glColor3f(1,1,1);

	glViewport(0,0,255,191);

	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(73.74, 256.0 / 192.0, 0.005, 40.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	//setup timer for Sys_FloatTime;
#if 0
	TIMER0_CR = TIMER_ENABLE|TIMER_DIV_1024;
	TIMER1_CR = TIMER_ENABLE|TIMER_CASCADE;
#else
	TIMER_DATA(0) = 0x10000 - (0x1000000 / 11025) * 2;
	TIMER_CR(0) = TIMER_ENABLE | TIMER_DIV_1;
	TIMER_DATA(1) = 0;
	TIMER_CR(1) = TIMER_ENABLE | TIMER_CASCADE | TIMER_DIV_1;
	TIMER_DATA(2) = 0;
	TIMER_CR(2) = TIMER_ENABLE | TIMER_CASCADE | TIMER_DIV_1;

#endif
	
	
	soundEnable();

	defaultExceptionHandler();
	//cygprofile_begin();
}

#ifdef __cplusplus
extern "C" { 
   void __gxx_personality_v0() 
   { 
       
   } 
   //void __aeabi_atexit() 
   //{ 
   //    
   //} 
}
#endif

#endif

#ifdef WIN32

int initDisplay();

void Sys_Init()
{
	LARGE_INTEGER	PerformanceFreq;
	unsigned int	lowpart, highpart;
	int type;

    /*auxInitPosition(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    type = AUX_RGB | AUX_DEPTH16 | AUX_DOUBLE;

    auxInitDisplayMode(type);
    if (auxInitWindow(L"CQuake") == GL_FALSE) {
		auxQuit();
    }*/

	MaskExceptions ();
	Sys_SetFPCW ();

	if (!QueryPerformanceFrequency (&PerformanceFreq))
		Sys_Error ("No hardware timer available");

// get 32 out of the 64 time bits such that we have around
// 1 microsecond resolution
	lowpart = (unsigned int)PerformanceFreq.LowPart;
	highpart = (unsigned int)PerformanceFreq.HighPart;
	lowshift = 0;

	while (highpart || (lowpart > 2000000.0))
	{
		lowshift++;
		lowpart >>= 1;
		lowpart |= (highpart & 1) << 31;
		highpart >>= 1;
	}

	pfreq = 1.0 / (double)lowpart;

	Sys_InitFloatTime ();
	initDisplay();
	wifi_fnet_init(0);
}
#endif


int main(int argc,char **argv)
{
	Sys_Init();

	memset(sys_handles,0,sizeof(sys_handles));
	quake_main(argc,argv);

	return 0;
}
