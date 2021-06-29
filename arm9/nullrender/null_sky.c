#include "quakedef.h"
#include "ds_textures.h"

extern model_t *r_currentmodel;
extern texture_t *r_sky_texture;

void R_InitSky (texture_t *mt)
{
	ds_load_bsp_sky(r_currentmodel,mt);
}
