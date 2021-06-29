#include "quakedef.h"
#include "ds_textures.h"

#ifdef WIN32
#include <windows.h>
#include <gl\gl.h>
#endif

#define TEXTURE_RESIDENT 1
#define BIG_TEXTURES 1


#ifdef BIG_TEXTURES

#define NUM_TEXTURE_BLOCKS2 32
#define TEXTURE_BLOCK_SIZE2 (128*64)

#define NUM_TEXTURE_BLOCKS3 6
#define TEXTURE_BLOCK_SIZE3 (256*128)

#define NUM_TEXTURE_BLOCKS 64
#define TEXTURE_BLOCK_SIZE (32*32)
#define NUM_TEXTURE_MASK (NUM_TEXTURE_BLOCKS-1)

#else

#define NUM_TEXTURE_BLOCKS2 48
#define TEXTURE_BLOCK_SIZE2 (128*64)

#define NUM_TEXTURE_BLOCKS 128
#define TEXTURE_BLOCK_SIZE (32*32)
#define NUM_TEXTURE_MASK (NUM_TEXTURE_BLOCKS-1)

#endif

extern int		r_framecount;
extern int		enable_texture_cache;
byte			*temp_texture_cache;

byte *DS_TEXTURE_BASE;
byte *DS_TEXTURE_BASE2;
#ifdef BIG_TEXTURES
byte *DS_TEXTURE_BASE3;
#endif
#define DS_TEXTURE_MEM_SIZE (256*256*2*4)

typedef struct {
	char *name;
	int visframe;
	int status;
	int texnum;
} ds_texture_t;

ds_texture_t ds_textures[NUM_TEXTURE_BLOCKS];
ds_texture_t ds_textures2[NUM_TEXTURE_BLOCKS2];
#ifdef BIG_TEXTURES
ds_texture_t ds_textures3[NUM_TEXTURE_BLOCKS3];
#endif

#ifdef WIN32
int texnums[NUM_TEXTURE_BLOCKS];
int texnums2[NUM_TEXTURE_BLOCKS2];
int texnums3[NUM_TEXTURE_BLOCKS3];
int texnum_sky_top,texnum_sky_bottom;
float ds_texture_width = 256.0f;
float ds_texture_height = 256.0f;
#endif

int r_sky_top = 0, r_sky_bottom = 0;

#ifdef NDS
void memcpy32(void *dst, const void *src, uint wdcount) ITCM_CODE;
#endif

int foo()
{
	return 0;
}

void texstats_f (void)
{
	int i;
	int used = 0;
	int used2 = 0;
	int used3 = 0;
	int in = 0;
	int in2 = 0;
	int in3 = 0;

	for(i=0;i<NUM_TEXTURE_BLOCKS;i++)
	{
		if(r_framecount - ds_textures[i].visframe < 2)
		{
			used++;
		}
		else if(ds_textures[i].name)
		{
			in++;
		}
	}

	for(i=0;i<NUM_TEXTURE_BLOCKS2;i++)
	{
		if(r_framecount - ds_textures2[i].visframe < 2)
		{
			used2++;
		}
		else if(ds_textures[i].name)
		{
			in2++;
		}
	}
#ifdef BIG_TEXTURES
	for(i=0;i<NUM_TEXTURE_BLOCKS3;i++)
	{
		if(r_framecount - ds_textures3[i].visframe < 2)
		{
			used3++;
		}
		else if(ds_textures3[i].name)
		{
			in3++;
		}
	}
#endif

	Con_Printf("v: %3d  i: %3d a: %3d\n",used,in,NUM_TEXTURE_BLOCKS);
	Con_Printf("v: %3d  i: %3d a: %3d\n",used2,in2,NUM_TEXTURE_BLOCKS2);
#ifdef BIG_TEXTURES
	Con_Printf("v: %3d  i: %3d a: %3d\n",used3,in3,NUM_TEXTURE_BLOCKS3);
#endif
}
void Tex_Init()
{
	Cmd_AddCommand ("texstats", texstats_f);
#ifdef WIN32
	glGenTextures(1,&texnum_sky_top);
	glGenTextures(1,&texnum_sky_bottom);
#endif
}

void Init_DS_Textures()
{
#ifdef WIN32
	DS_TEXTURE_BASE = (byte*)malloc(DS_TEXTURE_MEM_SIZE);
	DS_TEXTURE_BASE2 = (byte*)malloc(DS_TEXTURE_MEM_SIZE);
	
#ifdef BIG_TEXTURES
	DS_TEXTURE_BASE3 = (byte*)malloc(DS_TEXTURE_MEM_SIZE);
#endif

#else

#ifdef BIG_TEXTURES
	DS_TEXTURE_BASE2 = (byte*)VRAM_A;
	DS_TEXTURE_BASE3 = (byte *)VRAM_C;
	DS_TEXTURE_BASE = (byte*)VRAM_D;
	DS_TEXTURE_BASE += (NUM_TEXTURE_BLOCKS*TEXTURE_BLOCK_SIZE);
#else
	DS_TEXTURE_BASE2 = (byte*)VRAM_A;
	DS_TEXTURE_BASE = (byte*)VRAM_D;
#endif

#endif
	memset(ds_textures,0,sizeof(ds_textures));
	memset(ds_textures2,0,sizeof(ds_textures2));
#ifdef BIG_TEXTURES
	memset(ds_textures3,0,sizeof(ds_textures3));
#endif

#ifdef WIN32
	memset(texnums,0,sizeof(texnums));
	memset(texnums2,0,sizeof(texnums2));
#ifdef BIG_TEXTURES
	memset(texnums3,0,sizeof(texnums3));
#endif
#endif
	if(enable_texture_cache) {
		temp_texture_cache = (byte *)malloc(256*256);
	}
}

void ds_lock_block(int block)
{
#ifdef NDS
	switch(block>>7)
	{
	case 0:
		vramSetBankD(VRAM_D_TEXTURE);
		break;
	default:
		Sys_Error("ds_lock_block: %d",block);
		break;
	}
#endif
}

void ds_unlock_block(int block)
{
#ifdef NDS
	switch(block>>7)
	{
	case 0:
		vramSetBankD(VRAM_D_LCD);
		break;
	default:
		Sys_Error("ds_lock_block: %d",block);
		break;
	}
#endif
}

void ds_lock_block2(int block)
{
#ifdef NDS
	switch(block>>4)
	{
	case 0:
		vramSetBankA(VRAM_A_TEXTURE);
		break;
	case 1:
		vramSetBankB(VRAM_B_TEXTURE);
		break;
	case 2:
		vramSetBankC(VRAM_C_TEXTURE);
		break;
	}
#endif
}

void ds_unlock_block2(int block)
{
#ifdef NDS
	switch(block>>4)
	{
	case 0:
		vramSetBankA(VRAM_A_LCD);
		break;
	case 1:
		vramSetBankB(VRAM_B_LCD);
		break;
	case 2:
		vramSetBankC(VRAM_C_LCD);
		break;
	}
#endif
}

void ds_lock_block3(int block)
{
#ifdef NDS
	switch(block>>2)
	{
	case 0:
		vramSetBankC(VRAM_C_TEXTURE);
		break;
	case 1:
		vramSetBankD(VRAM_D_TEXTURE);
		break;
	}
#endif
}

void ds_unlock_block3(int block)
{
#ifdef NDS
	switch(block>>2)
	{
	case 0:
		vramSetBankC(VRAM_C_LCD);
		break;
	case 1:
		vramSetBankD(VRAM_D_LCD);
		break;
	}
#endif
}

int ds_adjust_size(int x)
{
	//if(x >= 64)
	//{
	//	return 64;
	//}
	if(x > 16)
	{
		return 32;
	}
	if(x > 8)
	{
		return 16;
	}
	return 8;
}

int ds_adjust_size2(int x)
{
	if(x > 64)
	{
		return 128;
	}
	if(x > 32)
	{
		return 64;
	}
	if(x > 16)
	{
		return 32;
	}
	if(x > 8)
	{
		return 16;
	}
	return 8;
}

int ds_adjust_size3(int x)
{
	if(x > 128)
	{
		return 256;
	}
	if(x > 64)
	{
		return 128;
	}
	if(x > 32)
	{
		return 64;
	}
	if(x > 16)
	{
		return 32;
	}
	if(x > 8)
	{
		return 16;
	}
	return 8;
}

int last_free_block = -1;

int find_free_block(char *name)
{
	int i;
	
	//names are in a hash list so they do not need strcmp
	for(i=0;i<NUM_TEXTURE_BLOCKS;i++)
	{
		if(ds_textures[i].name == name)
			return i;
	}
	
	//do not start at beggining - might leave textures in longer
	//look for ones that have not been used in a couple frames
	for(i=0;i<NUM_TEXTURE_BLOCKS;i++)
	{
		last_free_block++;
		
		if(r_framecount - ds_textures[last_free_block&NUM_TEXTURE_MASK].visframe > 1)
		{
			return last_free_block&NUM_TEXTURE_MASK;
		}
	}
	return -1;
}

int last_free_block2 = -1;
int find_free_block2(char *name)
{
	int i;
	
	//names are in a hash list so they do not need strcmp
	for(i=0;i<NUM_TEXTURE_BLOCKS2;i++)
	{
		if(ds_textures2[i].name == name)
			return i;
	}
	
	//do not start at beggining - might leave textures in longer
	//look for ones that have not been used in a couple frames
	for(i=0;i<NUM_TEXTURE_BLOCKS2;i++)
	{
		last_free_block2++;
		if(last_free_block2 == NUM_TEXTURE_BLOCKS2)
		{
			last_free_block2 = 0;
		}
		
		if(r_framecount - ds_textures2[last_free_block2].visframe > 1)
		{
			return last_free_block2;
		}
	}
	return -1;
}

#ifdef BIG_TEXTURES
int last_free_block3 = -1;
int find_free_block3(char *name)
{
	int i;
	
	//names are in a hash list so they do not need strcmp
	for(i=0;i<NUM_TEXTURE_BLOCKS3;i++)
	{
		if(ds_textures3[i].name == name)
			return i;
	}
	
	//do not start at beggining - might leave textures in longer
	//look for ones that have not been used in a couple frames
	for(i=0;i<NUM_TEXTURE_BLOCKS3;i++)
	{
		last_free_block3++;
		if(last_free_block3 == NUM_TEXTURE_BLOCKS3)
		{
			last_free_block3 = 0;
		}
		
		if(r_framecount - ds_textures3[last_free_block3].visframe > 1)
		{
			return last_free_block3;
		}
	}
	return -1;
}
#endif
int ds_is_texture_resident(dstex_t *tx)
{
	int block = tx->block&NUM_TEXTURE_MASK;
	if(ds_textures[block].name == tx->name)
		return block;
	return -1;
}

int ds_is_texture_resident2(dstex_t *tx)
{
	int block = tx->block%NUM_TEXTURE_BLOCKS2;
	if(ds_textures2[block].name == tx->name)
		return block;
	return -1;
}

#ifdef BIG_TEXTURES
int ds_is_texture_resident3(dstex_t *tx)
{
	int block = tx->block%NUM_TEXTURE_BLOCKS3;
	if(ds_textures3[block].name == tx->name)
		return block;
	return -1;
}
#endif

byte* ds_scale_texture(dstex_t *tx,int inwidth,int inheight,byte *in,byte *outt)
{
	int width,height,i,j;
	//int inwidth, inheight;
	unsigned	frac, fracstep;
	byte *inrow,*out;
	//int has255 = 0;

	width = tx->width;
	height = tx->height;

	if(inwidth == width && inheight == height)
	{
		memcpy(outt,in,inwidth*inheight);
		return outt;
	}

	out = outt;
	fracstep = (inwidth<<16)/width;
	for (i=0 ; i<height ; i++, out += width)
	{
		inrow = (in + inwidth*(i*inheight/height));
		frac = fracstep >> 1;
		for (j=0 ; j<width ; j++)
		{
			out[j] = inrow[frac>>16];
			/*
			if(out[j] == 0xff)
			{
				has255 = GL_TEXTURE_COLOR0_TRANSPARENT;
				out[j] = 0;
			} else if(out[j] == 0)
			{
				out[j] = 0xff;
			}*/
			frac += fracstep;
		}
	}
	return outt;
}
#ifdef NDS
static inline void DSdmaCopyWords(uint8 channel, const void* src, void* dest, uint32 size) {
	DMA_SRC(channel) = (uint32)src;
	DMA_DEST(channel) = (uint32)dest;
	//if(REG_VCOUNT > 191)
	{
		DMA_CR(channel) = DMA_COPY_WORDS | (size>>2);
	}
	//else
	//{
	//	DMA_CR(channel) = DMA_COPY_WORDS | DMA_START_HBL | (size>>2);
	//}
	while(DMA_CR(channel) & DMA_BUSY);
}
#endif

byte *ds_block_address(int block)
{
	return DS_TEXTURE_BASE + (block * TEXTURE_BLOCK_SIZE);
}

byte* ds_load_block(int block,byte *texture,int size)
{
	byte *addr = ds_block_address(block);

	ds_unlock_block(block);

#ifdef WIN32
	memcpy(addr,texture,size);
#else
	//DC_FlushAll();
	////DC_FlushRange((uint32*)texture, size);
	//dmaCopyWords(2,(uint32*)texture, (uint32*)addr , size);
	memcpy32(addr,texture,size/4);
#endif
	
	ds_lock_block(block);
	return addr;
}

byte *ds_block_address2(int block)
{
	return DS_TEXTURE_BASE2 + (block * TEXTURE_BLOCK_SIZE2);
}

byte* ds_load_block2(int block,byte *texture,int size)
{
	byte *addr = ds_block_address2(block);

	ds_unlock_block2(block);

#ifdef WIN32
	memcpy(addr,texture,size);
#else
	//DC_FlushAll();
	////DC_FlushRange((uint32*)texture, size);
	//dmaCopyWords(2,(uint32*)texture, (uint32*)addr , size);
	memcpy32(addr,texture,size/4);
#endif
	
	ds_lock_block2(block);
	return addr;
}

#ifdef BIG_TEXTURES
byte *ds_block_address3(int block)
{
	return DS_TEXTURE_BASE3 + (block * TEXTURE_BLOCK_SIZE3);
}

byte* ds_load_block3(int block,byte *texture,int size)
{
	byte *addr = ds_block_address3(block);

	ds_unlock_block3(block);

#ifdef WIN32
	memcpy(addr,texture,size);
#else
	//DC_FlushAll();
	////DC_FlushRange((uint32*)texture, size);
	//dmaCopyWords(3,(uint32*)texture, (uint32*)addr , size);
	memcpy32(addr,texture,size/4);
#endif
	
	ds_lock_block3(block);
	return addr;
}

#endif

int ds_tex_size(int x)
{
	switch(x)
	{
	case 512:
		return 6;
	case 256:
		return 5;
	case 128:
		return 4;
	case 64:
		return 3;
	case 32:
		return 2;
	case 16:
		return 1;
	case 8:
		return 0;
	}
	return 0;
}

int ds_tex_parameters(	int sizeX, int sizeY,
						const byte* addr,
						int mode,
						unsigned int param)
{
//---------------------------------------------------------------------------------
	return param | (sizeX << 20) | (sizeY << 23) | (((unsigned int)addr >> 3) & 0xFFFF) | (mode << 26);
}

int ds_loadTexture(dstex_t *ds,int w,int h,byte *buf,int trans)
{
	int block;
	byte *addr;

	block = find_free_block(ds->name);
	if(block == -1)
		return 0;

	addr = ds_load_block(block,buf,w*h);
	ds->block = block;
#ifdef WIN32
	if(texnums[block] == 0) {
		glGenTextures(1,&texnums[block]);
	}
	glBindTexture(GL_TEXTURE_2D,texnums[block]);
	ds_textures[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,0,0);
	{
		unsigned int dest32[256*256],v,r,g,b;
		int i;
		for(i=0;i<(w*h);i++) {
			r = host_basepal[addr[i]*3 + 0];
			g = host_basepal[addr[i]*3 + 1];
			b = host_basepal[addr[i]*3 + 2];
			if(trans) {
				v = ((addr[i] == 0 ? 0 : 255)<<24) + (r<<0) + (g<<8) + (b<<16);
			} else {
				v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			}
			dest32[i] = v;
		}
		glTexImage2D (GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
#else
	ds_textures[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),
		addr,GL_RGB256,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|
		(trans  ? GL_TEXTURE_COLOR0_TRANSPARENT : 0));
#endif
	ds_textures[block].name = ds->name;
	ds_textures[block].visframe = r_framecount;
	return ds_textures[block].texnum;
}

int ds_loadTexture2(dstex_t *ds,int w,int h,byte *buf)
{
	int block;
	byte *addr;

	block = find_free_block2(ds->name);
	if(block == -1)
		return 0;
	addr = ds_load_block2(block,buf,w*h);
	ds->block = block;
#ifdef WIN32
	if(texnums2[block] == 0) {
		glGenTextures(1,&texnums2[block]);
	}
	glBindTexture(GL_TEXTURE_2D,texnums2[block]);
	ds_textures2[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,0,0);
	{
		unsigned int dest32[256*256],v,r,g,b;
		int i;
		for(i=0;i<(w*h);i++) {
			r = host_basepal[addr[i]*3 + 0];
			g = host_basepal[addr[i]*3 + 1];
			b = host_basepal[addr[i]*3 + 2];
			v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			dest32[i] = v;
		}
		glTexImage2D (GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
#else
	ds_textures2[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,GL_RGB256,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
#endif
	ds_textures2[block].name = ds->name;
	ds_textures2[block].visframe = r_framecount;
	return ds_textures2[block].texnum;
}

#ifdef BIG_TEXTURES

int ds_loadTexture3(dstex_t *ds,int w,int h,byte *buf,int trans)
{
	int block;
	byte *addr;

	block = find_free_block3(ds->name);
	if(block == -1)
		return 0;
	addr = ds_load_block3(block,buf,w*h);
	ds->block = block;
#ifdef WIN32
	if(texnums3[block] == 0) {
		glGenTextures(1,&texnums3[block]);
	}
	glBindTexture(GL_TEXTURE_2D,texnums3[block]);
	ds_textures3[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,0,0);
	{
		unsigned int dest32[256*256],v,r,g,b;
		int i;
		for(i=0;i<(w*h);i++) {
			r = host_basepal[addr[i]*3 + 0];
			g = host_basepal[addr[i]*3 + 1];
			b = host_basepal[addr[i]*3 + 2];
			v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			dest32[i] = v;
		}
		glTexImage2D (GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
#else
	ds_textures3[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,GL_RGB256,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|
		(trans  ? GL_TEXTURE_COLOR0_TRANSPARENT : 0));
#endif
	ds_textures3[block].name = ds->name;
	ds_textures3[block].visframe = r_framecount;
	return ds_textures3[block].texnum;
}

#endif

byte	dottexture[8][8] =
{
	{0x0,0xf,0xf,0xf,0,0,0,0},
	{0xf,0xf,0xf,0xf,0xf,0,0,0},
	{0xf,0xf,0xf,0xf,0xf,0,0,0},
	{0xf,0xf,0xf,0xf,0xf,0,0,0},
	{0x0,0xf,0xf,0xf,0,0,0,0},
	{0x0,0x0,0x0,0x0,0,0,0,0},
	{0x0,0x0,0x0,0x0,0,0,0,0},
	{0x0,0x0,0x0,0x0,0,0,0,0},
};

int ds_load_particle_texture(dstex_t *ds)
{
	int handle, length, size, block, w, h;
	byte *buf,*addr,*dst;

#ifdef WIN32
	ds_texture_width = ds->width*16.0f;
	ds_texture_height = ds->height*16.0f;
#endif

	block = ds_is_texture_resident(ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums[block]);
#endif
		ds_textures[block].visframe = r_framecount;
		return ds_textures[block].texnum;
	}
	Con_DPrintf("%s %d %d\n",ds->name,ds->width,ds->height);
	return ds_loadTexture(ds,8,8,&dottexture[0][0],1);
}
#if 1
byte	crosstexture[8][8] =
{
	{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0xf,0xf,0xf,0xf,0xf,0xf,0x0},
	{0x0,0xf,0xf,0xf,0xf,0xf,0xf,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
};
#else
byte	crosstexture[8][8] =
{
	{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0xf,0xf,0xf,0xf,0xf,0xf,0x0},
	{0x0,0xf,0xf,0xf,0xf,0xf,0xf,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0xf,0xf,0x0,0x0,0x0},
	{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
};
#endif
int ds_load_crosshair_texture(dstex_t *ds)
{
	int handle, length, size, block, w, h;
	byte *buf,*addr,*dst;


	block = ds_is_texture_resident(ds);
	if(block != -1)
	{
		ds_textures[block].visframe = r_framecount;
		return ds_textures[block].texnum;
	}
	Con_DPrintf("%s %d %d\n",ds->name,ds->width,ds->height);
	return ds_loadTexture(ds,8,8,&crosstexture[0][0],1);
}

int ds_loadTextureSky(dstex_t *ds,int w,int h,byte *buf)
{
	int block;
	byte *addr;

	//return 0;
	block = find_free_block2(ds->name);
	if(block == -1)
		return 0;
	if(0)
	{
		int ww,hh;
		byte *top = buf;
		for(hh=0;hh<64;hh++)
		{
			for(ww=0;ww<64;ww++)
			{
				top[(hh*64)+ww] = (ww/2) | ((hh/8)<<5);
			}
		}
	}
	addr = ds_load_block2(block,buf,w*h);
	ds->block = block;
#ifdef WIN32
	r_sky_top = ds_tex_parameters(ds_tex_size(w>>1),ds_tex_size(h),addr,0,0);
	r_sky_bottom = ds_tex_parameters(ds_tex_size(w>>1),ds_tex_size(h),addr+((w>>1)*h),0,0);
	ds_textures2[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,0,0);
	{
		unsigned int dest32[256*256],v,r,g,b;
		int i;
		for(i=0;i<(w*h);i++) {
			r = host_basepal[addr[i]*3 + 0];
			g = host_basepal[addr[i]*3 + 1];
			b = host_basepal[addr[i]*3 + 2];
			v = ((addr[i] == 0 ? 0 : 255)<<24) + (r<<0) + (g<<8) + (b<<16);
			//if(i/64 == 0 || (i&63) == 0)
			//	v = 0xFFFFFFFF;
			dest32[i] = v;
		}
		glBindTexture(GL_TEXTURE_2D,texnum_sky_top);
		glTexImage2D (GL_TEXTURE_2D, 0, 4, w>>1, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D,texnum_sky_bottom);
		glTexImage2D (GL_TEXTURE_2D, 0, 4, w>>1, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32+((w>>1)*h));
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
#else
	ds_textures2[block].texnum = ds_tex_parameters(ds_tex_size(w),ds_tex_size(h),addr,
		GL_RGB256,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
	if(0)
	{
		r_sky_top = ds_tex_parameters(ds_tex_size(w>>1),ds_tex_size(h),addr,
			GL_RGB32_A3,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
		r_sky_bottom = ds_tex_parameters(ds_tex_size(w>>1),ds_tex_size(h),addr+((w>>1)*h),
			GL_RGB256,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
	}
	else
	{
		r_sky_top = ds_tex_parameters(ds_tex_size(w>>1),ds_tex_size(h),addr,
			GL_RGB256,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|GL_TEXTURE_COLOR0_TRANSPARENT);
		r_sky_bottom = ds_tex_parameters(ds_tex_size(w>>1),ds_tex_size(h),addr+((w>>1)*h),
			GL_RGB256,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T);
	}
	//r_sky_top = r_sky_bottom = ds_textures2[block].texnum;
#endif
	ds_textures2[block].name = ds->name;
	ds_textures2[block].visframe = r_framecount;
	return ds_textures2[block].texnum;
}

byte * COM_LoadTempFilePartExtra(int handle,int offset,int length,int extra);
int ds_load_bsp_texture2(model_t *mod,texture_t *texture);

int ds_load_bsp_texture(model_t *mod,texture_t *texture)
{
	int handle, length, size, size_extra, block, w, h;
	byte *buf,*addr,*dst;

#ifdef WIN32
	ds_texture_width = texture->ds.width*16.0f;
	ds_texture_height = texture->ds.height*16.0f;
#endif
	//return 0;
	if(texture->ds.width > 32 || texture->ds.height > 32)
	{
		return ds_load_bsp_texture2(mod,texture);
	}

	block = ds_is_texture_resident(&texture->ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums[block]);
#endif
		ds_textures[block].visframe = r_framecount;
		return ds_textures[block].texnum;
	}
	Con_DPrintf("%s %d %d\n",texture->ds.name,texture->ds.width,texture->ds.height);

	size = texture->width * texture->height;
	if(texture == r_notexture_mip)
	{
		dst = (byte *)r_notexture_mip + r_notexture_mip->offsets[0];
		w = r_notexture_mip->width;
		h = r_notexture_mip->height;
	}
	else
	{
		cache_user_t *cache = (cache_user_t *)(enable_texture_cache == 0 ? 0 : (texture+1));
		w = texture->ds.width;
		h = texture->ds.height;
		if(cache && cache->data) {
			dst = (byte *)cache->data;
		} else {
			if(mod->name[0] == '*')
			{
				length = COM_OpenFile(cl.worldmodel->name,&handle);
			}
			else
			{
				length = COM_OpenFile(mod->name,&handle);
			}
			if(length == -1)
			{
				//Sys_Error ("Mod_NumForName: %s not found", mod->name);
				return 0;
			}
			size_extra = (enable_texture_cache == 0 ? texture->ds.width*texture->ds.height : 0);
			if(texture->width < 64 || texture->height < 64)
			{
				buf = COM_LoadTempFilePartExtra(handle,texture->ds.file_offset,size,size_extra);
				COM_CloseFile(handle);
				if(buf == 0)
					return 0;
				dst = (enable_texture_cache == 0 ? (buf + size) : temp_texture_cache);

				dst = ds_scale_texture(&texture->ds,texture->width,texture->height,buf,dst);
			}
			else
			{
				buf = COM_LoadTempFilePartExtra(handle,texture->ds.file_offset+size,size>>2,size_extra);
				COM_CloseFile(handle);
				if(buf == 0)
					return 0;
				dst = (enable_texture_cache == 0 ? (buf + (size>>2)) : temp_texture_cache);
				//dst = buf + (size>>2);

				dst = ds_scale_texture(&texture->ds,texture->width>>1,texture->height>>1,buf,dst);
			}
			if(enable_texture_cache) {
				byte *data = (byte *)Cache_Alloc(cache,w*h,"texcache");
				memcpy(data,dst,w*h);
			}
		}
	}

	return ds_loadTexture(&texture->ds,w,h,dst,0);
}

int ds_load_bsp_texture2(model_t *mod,texture_t *texture)
{
	int handle, length, size, size_extra, block, w, h;
	byte *buf,*addr,*dst;
	cache_user_t *cache;

	//return 0;
	block = ds_is_texture_resident2(&texture->ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums2[block]);
#endif
		ds_textures2[block].visframe = r_framecount;
		return ds_textures2[block].texnum;
	}
	cache = (cache_user_t *)(enable_texture_cache == 0 ? 0 : (texture+1));
	Con_DPrintf("%s %d %d\n",texture->ds.name,texture->ds.width,texture->ds.height);

	w = texture->ds.width;
	h = texture->ds.height;
	size = texture->width * texture->height;
	if(cache && cache->data) {
		dst = (byte *)cache->data;
	} else {
		if(mod->name[0] == '*')
		{
			length = COM_OpenFile(cl.worldmodel->name,&handle);
		}
		else
		{
			length = COM_OpenFile(mod->name,&handle);
		}
		if(length == -1)
		{
			//Sys_Error ("Mod_NumForName: %s not found", mod->name);
			return 0;
		}
		size_extra = (enable_texture_cache == 0 ? w*h : 0);
		buf = COM_LoadTempFilePartExtra(handle,texture->ds.file_offset,size,size_extra);
		COM_CloseFile(handle);

		if(buf == 0)
			return 0;
		dst = (enable_texture_cache == 0 ? (buf + size) : temp_texture_cache);

		dst = ds_scale_texture(&texture->ds,texture->width,texture->height,buf,dst);
		if(enable_texture_cache) {
			byte *data = (byte *)Cache_Alloc(cache,w*h,"texcache");
			memcpy(data,dst,w*h);
		}
	}

	return ds_loadTexture2(&texture->ds,w,h,dst);
}

int ds_fixup_sky(byte *buf,int width,int height,byte*scrap) 
{
	int i , w;
	byte *dest,*dest2;

	w = width>>1;

	dest = scrap;
	dest2 = scrap + (w*height);
	for(i=0;i<height;i++)
	{
		memcpy(&dest[i*w],&buf[i*width],w);
		memcpy(&dest2[i*w],&buf[i*width + w],w);
	}
	return 0;
}
int ds_load_bsp_sky(model_t *mod,texture_t *texture)
{
	//return 0;
	int handle, length, size, block, w, h, size2;
	byte *buf,*addr,*dst,*scrap;

#ifdef WIN32
	ds_texture_width = texture->ds.width*8.0f;
	ds_texture_height = texture->ds.height*16.0f;
#endif
	block = ds_is_texture_resident2(&texture->ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums2[block]);
#endif
		ds_textures2[block].visframe = r_framecount;
		return ds_textures2[block].texnum;
	}
	Con_DPrintf("%s %d %d\n",texture->ds.name,texture->ds.width,texture->ds.height);

	w = texture->ds.width = 128;
	h = texture->ds.height = 64;
	size = texture->width * texture->height;
	if(mod->name[0] == '*')
	{
		length = COM_OpenFile(cl.worldmodel->name,&handle);
	}
	else
	{
		length = COM_OpenFile(mod->name,&handle);
	}
	if(length == -1)
	{
		//Sys_Error ("Mod_NumForName: %s not found", mod->name);
		return 0;
	}
	size2 = texture->ds.width*texture->ds.height;
	buf = COM_LoadTempFilePartExtra(handle,texture->ds.file_offset,size,size2<<1);
	COM_CloseFile(handle);

	if(buf == 0)
		return 0;
	dst = buf + size;
	scrap = dst + size2;

	dst = ds_scale_texture(&texture->ds,texture->width,texture->height,buf,dst);
	ds_fixup_sky(dst,texture->ds.width,texture->ds.height,scrap);

	return ds_loadTextureSky(&texture->ds,w,h,scrap);
}

int ds_load_alias_texture3(model_t *mod,maliasskindesc_t	*pskindesc);
int ds_load_alias_texture(model_t *mod,maliasskindesc_t	*pskindesc)
{
	int handle, length, size, block, w, h;
	byte *buf,*addr,*dst;
	
#ifdef WIN32
	ds_texture_width = pskindesc->ds.width*16.0f;
	ds_texture_height = pskindesc->ds.height*16.0f;
#endif
	//return 0;
	if(pskindesc->ds.width > 128 || pskindesc->ds.height > 128)
	{
		return ds_load_alias_texture3(mod,pskindesc);
	}

	block = ds_is_texture_resident2(&pskindesc->ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums2[block]);
#endif
		ds_textures2[block].visframe = r_framecount;
		return ds_textures2[block].texnum;
	}
	Con_DPrintf("%s %d %d\n",mod->name,pskindesc->ds.width,pskindesc->ds.height);

	w = pskindesc->ds.width; 
	h = pskindesc->ds.height;
	size = pskindesc->width * pskindesc->height;
	
	if(mod->name[0] == '*')
	{
		length = COM_OpenFile(cl.worldmodel->name,&handle);
	}
	else
	{
		length = COM_OpenFile(mod->name,&handle);
	}
	if(length == -1)
	{
		//Sys_Error ("Mod_NumForName: %s not found", mod->name);
		return 0;
	}
	buf = COM_LoadTempFilePartExtra(handle,pskindesc->ds.file_offset,size,pskindesc->ds.width*pskindesc->ds.height);
	COM_CloseFile(handle);

	if(buf == 0)
		return 0;
	dst = buf + size;

	dst = ds_scale_texture(&pskindesc->ds,pskindesc->width,pskindesc->height,buf,dst);

	return ds_loadTexture2(&pskindesc->ds,w,h,dst);
}

#ifdef BIG_TEXTURES

int ds_load_alias_texture3(model_t *mod,maliasskindesc_t	*pskindesc)
{
	int handle, length, size, block, w, h;
	byte *buf,*addr,*dst,c;

	//return 0;
	block = ds_is_texture_resident3(&pskindesc->ds);
	if(block != -1)
	{
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums3[block]);
#endif
		ds_textures3[block].visframe = r_framecount;
		return ds_textures3[block].texnum;
	}
	Con_DPrintf("%s %d %d\n",mod->name,pskindesc->ds.width,pskindesc->ds.height);

	w = pskindesc->ds.width; 
	h = pskindesc->ds.height;
	size = pskindesc->width * pskindesc->height;
	if(size > (0x00000001 << 20))
	{
		return 0;
	}
	if(mod->name[0] == '*')
	{
		length = COM_OpenFile(cl.worldmodel->name,&handle);
	}
	else
	{
		length = COM_OpenFile(mod->name,&handle);
	}
	if(length == -1)
	{
		//Sys_Error ("Mod_NumForName: %s not found", mod->name);
		return 0;
	}
	buf = COM_LoadTempFilePartExtra(handle,pskindesc->ds.file_offset,size,pskindesc->ds.width*pskindesc->ds.height);
	COM_CloseFile(handle);

	if(buf == 0)
		return 0;
	dst = buf + size;
	c = *buf;

	dst = ds_scale_texture(&pskindesc->ds,pskindesc->width,pskindesc->height,buf,dst);
#ifdef NDS
	if(strncasecmp(mod->name,"progs/v_shot",12) == 0)
#else
	if(strnicmp(mod->name,"progs/v_shot",12) == 0)
#endif
	{
		int i;
		buf = dst;
		for(i=0;i<w*h;i++)
		{
			if(*buf == 0)
				*buf = 1;
			else if(*buf == c)
				*buf = 0;
			buf++;
		}
		return ds_loadTexture3(&pskindesc->ds,w,h,dst,1);
	}

	return ds_loadTexture3(&pskindesc->ds,w,h,dst,0);
}

#endif

void waitforit(void);

void ds_precache_bsp_textures(model_t *mod)
{
	int i, n;
	texture_t **tex;

	n = mod->bmodel->numtextures;
	tex = mod->bmodel->textures;

	Con_DPrintf("Precaching textures %d...\n",n);

	for(i=0;i<n;i++)
	{
		if (tex[i] && tex[i]->ds.name && Q_strncmp(tex[i]->ds.name,"sky",3))
		{
			Con_DPrintf("%d ",i);
			Con_DPrintf("%s\n",tex[i]->ds.name);
			ds_load_bsp_texture(mod,tex[i]);
		}
	}
	Con_DPrintf("\n");
}

int ds_load_sprite_texture(model_t *mod,mspriteframe_t *pspriteframe)
{
	int handle, length, size, block, w, h, i;
	byte *buf,*addr,*dst;
	
	//return 0;

#ifdef WIN32
	ds_texture_width = pspriteframe->ds.width*16.0f;
	ds_texture_height = pspriteframe->ds.height*16.0f;
#endif

	block = ds_is_texture_resident(&pspriteframe->ds);
	if(block != -1)
	{
		ds_textures[block].visframe = r_framecount;
#ifdef WIN32
		glBindTexture(GL_TEXTURE_2D,texnums[block]);
#endif
		return ds_textures[block].texnum;
	}
	Con_DPrintf("%s %d %d\n",pspriteframe->ds.name,pspriteframe->ds.width,pspriteframe->ds.height);

	size = pspriteframe->width * pspriteframe->height;
	{
		w = pspriteframe->ds.width;
		h = pspriteframe->ds.height;
		length = COM_OpenFile(mod->name,&handle);
		if(length == -1)
		{
			//Sys_Error ("Mod_NumForName: %s not found", mod->name);
			return 0;
		}
		buf = COM_LoadTempFilePartExtra(handle,pspriteframe->ds.file_offset,size,pspriteframe->ds.width*pspriteframe->ds.height);
		COM_CloseFile(handle);
		if(buf == 0)
			return 0;
		dst = buf + size;

		dst = ds_scale_texture(&pspriteframe->ds,pspriteframe->width,pspriteframe->height,buf,dst);
		
		buf = dst;
		for(i=0;i<w*h;i++)
		{
			if(*buf == 0)
				*buf = 255;
			else if(*buf == 255)
				*buf = 0;
			buf++;
		}
	}

	return ds_loadTexture(&pspriteframe->ds,w,h,dst,1);
}
