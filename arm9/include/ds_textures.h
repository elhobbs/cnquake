#ifndef __DS_TEXTURES_H__
#define __DS_TEXTURES_H__

#ifdef NDS
#define DS_NORMAL(__n) GFX_NORMAL = (__n)
#define DS_COLOR(__c) GFX_COLOR = (__c)
#define DS_TEXCOORD2T16(x, y) GFX_TEX_COORD = TEXTURE_PACK(x, y)
#define DS_VERTEX3V16(x, y, z) GFX_VERTEX16 = (y << 16) | (x & 0xFFFF); GFX_VERTEX16 = ((unsigned int)(unsigned short)z)
#endif

#ifdef WIN32
void DS_COLOR(unsigned short c);
#endif

/*
banks A-D are broken into 64x64 chunks
which makes 128 blocks

*/
int foo();
void Init_DS_Textures();
int ds_load_bsp_texture(model_t *mod,texture_t *texture);

int ds_load_bsp_sky(model_t *mod,texture_t *texture);
extern int r_sky_top, r_sky_bottom;
extern texture_t *r_sky_texture;

#endif
