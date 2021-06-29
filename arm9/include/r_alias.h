#ifndef __R_ALIAS_H__
#define __R_ALIAS_H__

#define	MAXALIASVERTS	1024
#define ALIAS_Z_CLIP_PLANE	5
// flags in finalvert_t.flags
#define ALIAS_LEFT_CLIP				0x0001
#define ALIAS_TOP_CLIP				0x0002
#define ALIAS_RIGHT_CLIP			0x0004
#define ALIAS_BOTTOM_CLIP			0x0008
#define ALIAS_Z_CLIP				0x0010

#define ALIAS_XY_CLIP_MASK			0x000F

typedef struct {
	float	fv[3];		// viewspace x, y
} auxvert_t;

extern aliashdr_t		*r_aliashdr;
extern mdl_t 			*r_mdl;
extern trivertx_t		*r_pverts;
extern mstvert_t		*r_pstverts;
extern int				r_numverts;
extern mtriangle_t		*r_ptri;
extern int				r_numtri;
extern int				r_skinwidth,r_skinheight;
extern int				r_frame;
extern int				r_frameofs;

extern entity_t *r_currententity;
extern model_t *r_currentmodel;

extern float		r_ziscale;
extern float		aliasxscale, aliasyscale, aliasxcenter, aliasycenter;
extern int		r_skinwidth;

extern finalvert_t			*r_pfinalverts;
extern auxvert_t			*r_pauxverts;

void R_AliasClipTriangle (mtriangle_t *ptri);

#endif