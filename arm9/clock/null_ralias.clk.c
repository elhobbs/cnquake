#include "quakedef.h"
#include "null_ralias.h"
#include "ds_textures.h"

#ifdef WIN32
#include <windows.h>
#include <gl\gl.h>
#define RGB15(r,g,b)  ((r)|((g)<<5)|((b)<<10))
#endif

aliashdr_t		*r_aliashdr;
mdl_t 			*r_mdl;
trivertx_t		*r_pverts;
mstvert_t		*r_pstverts;
int				r_numverts;
mtriangle_t		*r_ptri;
int				r_numtri;
int				r_skinwidth,r_skinheight;
int				r_frame;
int				r_frameofs;


float		r_ziscale;

#define NUMVERTEXNORMALS	162
extern int	ds_normals[NUMVERTEXNORMALS];

#if 0
// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];
#endif

extern entity_t *r_currententity;
extern model_t *r_currentmodel;
void R_RotateForEntity (entity_t *e);
qboolean R_CullBox (short* mins, short* maxs);
void R_AliasPreparePoints (void);

void glScale3f32(int x, int y, int z)
{
//---------------------------------------------------------------------------------
#ifdef NDS
  MATRIX_SCALE = (x);
  MATRIX_SCALE = (y);
  MATRIX_SCALE = (z);
#else
	glScalef(x/4096.0f,y/4096.0f,z/4096.0f);
#endif
}
static char alias_skin_name[128];
int ds_load_alias_texture(model_t *mod,maliasskindesc_t	*pskindesc);

short twidth;

void R_SetAliasSkin(aliashdr_t *paliashdr) {
	maliasskindesc_t	*pskindesc;
	maliasskingroup_t	*paliasskingroup;
	float				*pskinintervals, fullskininterval;
	float				skintargettime, skintime;
	int					numskins;
	int					i;
	int					texnum;
	//register char * stack_ptr asm ("sp"); 
	
	//iprintf("sp: %x\n",stack_ptr);

	if(r_currententity->skinnum >= r_mdl->numskins)
	{
		//Sys_Error("error");
		r_currententity->skinnum = 0;

	}

	pskindesc = ((maliasskindesc_t *)BYTE_OFFSET(paliashdr,paliashdr->skindesc)) + r_currententity->skinnum;
	if (pskindesc->type == ALIAS_SKIN_GROUP)
	{
		paliasskingroup = (maliasskingroup_t *)BYTE_OFFSET(paliashdr,pskindesc->skin);
		pskinintervals = (float *)BYTE_OFFSET(paliashdr,paliasskingroup->intervals);
		numskins = paliasskingroup->numskins;
		fullskininterval = pskinintervals[numskins-1];
	
		skintime = cl.time + r_currententity->syncbase;
	
	// when loading in Mod_LoadAliasSkinGroup, we guaranteed all interval
	// values are positive, so we don't have to worry about division by 0
		skintargettime = skintime -
				((int)(skintime / fullskininterval)) * fullskininterval;
	
		for (i=0 ; i<(numskins-1) ; i++)
		{
			if (pskinintervals[i] > skintargettime)
				break;
		}
	
		pskindesc = &paliasskingroup->skindescs[i];
	}
	if(pskindesc->ds.name == 0)
	{
		sprintf(alias_skin_name,"%s_%d",r_currentmodel->name,pskindesc->ds.file_offset);
		pskindesc->ds.name = ED_NewString(alias_skin_name);
		if(!r_currentmodel->cache.data)
		{
			Sys_Error("R_SetAliasSkin: unloaded model\n");
		}
	}
#if 0
	r_affinetridesc.pskindesc = pskindesc;
	r_affinetridesc.pskin = (void *)((byte *)paliashdr + pskindesc->skin);
	r_affinetridesc.skinwidth = r_mdl->skinwidth;
	r_affinetridesc.seamfixupX16 =  (r_mdl->skinwidth >> 1);// << 16;
	r_affinetridesc.skinheight = r_mdl->skinheight;
#endif

	twidth = pskindesc->ds.width;//>>1;
	twidth <<= 3;//4;
	if(!r_currentmodel->cache.data)
	{
		//since the creating a new skin name can push the model out of the cache
		//we are going to let the model flash white for a frame rather than crashing
		texnum = 0;
		//Sys_Error("ds_load_alias_texture: unloaded model\n");
	} else {
		texnum = ds_load_alias_texture(r_currentmodel,pskindesc);
	}
#if 0
		if(!r_currentmodel->cache.data)
		{
			Sys_Error("ds_load_alias_texture: unloaded model\n");
		}
#endif
#ifdef NDS
	GFX_TEX_FORMAT = texnum;
#endif
	//DS_BindTexture( pskindesc->ndstexnum);
}

void R_SetAliasFrame(aliashdr_t *paliashdr) {
	mdl_t 			*pmdl;
	maliasgroup_t	*paliasgroup;
	float			*pintervals, fullinterval, targettime, time;
	int				frame;
	int				numframes;
	int				i;
	
	//register char * stack_ptr asm ("sp"); 
	
	//iprintf("sp: %x\n",stack_ptr);
	
	pmdl = (mdl_t *)BYTE_OFFSET(paliashdr,paliashdr->model);
	frame = r_currententity->frame;
	if ((frame >= pmdl->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}
	
	if (paliashdr->frames[frame].type == ALIAS_SINGLE) {
		r_frameofs = paliashdr->frames[frame].frame;
		r_pverts = (trivertx_t *)BYTE_OFFSET(paliashdr,r_frameofs);
		r_frame = frame;
		//iprintf("%d\n",frame);
	} else {
		paliasgroup = (maliasgroup_t *)BYTE_OFFSET(paliashdr,paliashdr->frames[frame].frame);
		pintervals = (float *)BYTE_OFFSET(paliashdr,paliasgroup->intervals);
		numframes = paliasgroup->numframes;
		fullinterval = pintervals[numframes-1];
		//return;
	
		time = cl.time + r_currententity->syncbase;
		//c_alias_polys += paliashdr->numtris;
	
	//
	// when loading in Mod_LoadAliasGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
	//
		targettime = time - ((int)(time / fullinterval)) * fullinterval;
		targettime = time - ((int)(time / fullinterval)) * fullinterval;
		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}
		r_frame = i;
		r_frameofs = paliasgroup->frames[i].frame;
		r_pverts = (trivertx_t *)BYTE_OFFSET(paliashdr,r_frameofs);
	}
	
	//r_pstverts = (mstvert_t *)&pmdl[1];//BYTE_OFFSET(paliashdr,paliashdr->stverts);
	r_pstverts = (mstvert_t *)BYTE_OFFSET(paliashdr,paliashdr->stverts);
	if(((byte *)&pmdl[1]) != BYTE_OFFSET(paliashdr,paliashdr->stverts))
	{
		Con_Printf("here: %x %x\n",((byte *)&pmdl[1]), BYTE_OFFSET(paliashdr,paliashdr->stverts));
		//while(1);
	}
	r_numverts = pmdl->numverts;
	
	r_ptri = (mtriangle_t *)BYTE_OFFSET(paliashdr,paliashdr->triangles);
	r_numtri = pmdl->numtris;
	r_skinwidth = pmdl->skinwidth;
	r_skinheight = pmdl->skinheight;
	//Con_Printf("%s %d %d %d\n",fcurrententity->model->name,f_frame,f_frameofs,paliashdr->frames[frame].type);
	//Con_Printf(" %x %x %x\n",paliashdr,f_frameofs,paliashdr->stverts);
	//Con_Printf("  %d %d %d\n",f_numtri,f_numverts,f_skinwidth);
	//Con_Printf("  %d %d\n",f_frame,f_frameofs);
	//Con_Printf("  %d\n",paliashdr->frames[frame].type);
}

int R_LightPoint (vec3_t p);

#ifdef NDS
typedef long long big_int;
#define NDS_INLINE static inline
#else
typedef __int64 big_int;
#define NDS_INLINE static __forceinline
#endif

NDS_INLINE big_int mul64(big_int a,big_int b)
{
	return (a*b);
}

int ambientlight,shadelight;
void ds_lightpoint(vec3_t p)
{
int lnum;
big_int len,len2;
float add;
	vec3_t dist;

	//
	// get lighting information
	//
	if (!strcmp (r_currentmodel->name, "progs/flame2.mdl")
		|| !strcmp (r_currentmodel->name, "progs/flame.mdl") )
	{
		DS_COLOR(RGB15(31,31,31));
		ambientlight = shadelight = 255;
#ifdef NDS
		glMaterialf(GL_AMBIENT, RGB15(24,24,24));
		glMaterialf(GL_DIFFUSE, RGB15(24,24,24));
#endif
		return;
	}

	ambientlight = shadelight = R_LightPoint (p);

	// allways give the gun some light
	if (r_currententity == &cl.viewent && ambientlight < 32)
		ambientlight = shadelight = 32;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			dist[0] = (int)p[0] - cl_dlights[lnum].iorigin[0];
			dist[1] = (int)p[1] - cl_dlights[lnum].iorigin[1];
			dist[2] = (int)p[2] - cl_dlights[lnum].iorigin[2];

			len = mul64(dist[0],dist[0]) + mul64(dist[1],dist[1]) + mul64(dist[2],dist[2]);
			len2 = mul64(cl_dlights[lnum].iradius,cl_dlights[lnum].iradius);

			if(len > len2)
				continue;

#ifdef NDS
			len = sqrt32(len);
#else
			len = sqrt (len);
#endif
			add = cl_dlights[lnum].iradius - len;

			//VectorSubtract (p, cl_dlights[lnum].origin,dist);
			//add = cl_dlights[lnum].radius - Length(dist);

			if (add > 0) {
				ambientlight += add;
				//ZOID models should be affected by dlights as well
				shadelight += add;
			}
		}
	}
	if(shadelight > 255)
		shadelight = 255;
	else if(shadelight < 32)
		shadelight = 32;
#if 1
	// clamp lighting so it doesn't overbright as much
	if (ambientlight > 64)
		ambientlight = 64;
	//if (ambientlight + shadelight > 255)
	//	shadelight = 255 - ambientlight;
#endif
/*
// ZOID: never allow players to go totally black
	i = r_currententity - cl_entities;
	if (i >= 1 && i<=cl.maxclients)
		if (ambientlight < 8)
			ambientlight = shadelight = 8;
*/
	glColor3b(shadelight,shadelight,shadelight);
#ifdef NDS
	glMaterialf(GL_AMBIENT, RGB8(ambientlight,ambientlight,ambientlight));
	glMaterialf(GL_DIFFUSE, RGB8(shadelight,shadelight,shadelight));
	glMaterialf(GL_SPECULAR, RGB8(shadelight,shadelight,shadelight));
#endif

}

	extern int r_alias_tri;
void R_DrawAliasModel ()
{
	int			i;
	//aliashdr_t	*paliashdr;
	//mdl_t 		*pmdl;
	trivertx_t	*index0,*index1,*index2;
	mstvert_t	*st0,*st1,*st2;

	short mins[3],maxs[3];
	short ax0,ay0,az0;
	short ax1,ay1,az1;
	short ax2,ay2,az2;
	
	short as0,at0;
	short as1,at1;
	short as2,at2;

#if 0
	trivertx_t vert[2000];
	mtriangle_t tri[600];
#endif
	
	//fvec_t		skin_d2 = (inttofvec(f_skinwidth) + 0x8000) >> 1;
	
	//fvec_t iw,ih;
	
#ifdef NDS
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT | POLY_ID(6) | POLY_FORMAT_LIGHT0 | (1 << 13));
#endif	

	for(i=0;i<3;i++) {
		mins[i] = (short)(r_currententity->origin[i] + r_currentmodel->mins[i]);
		maxs[i] = (short)(r_currententity->origin[i] + r_currentmodel->maxs[i]);
	}

	if (r_currententity == &cl.viewent)
	{
		ds_lightpoint(r_refdef.vieworg);
	}
	else if (R_CullBox (mins, maxs))
	{
		return;
	}
	else
	{
		ds_lightpoint(r_currententity->origin);
	}

#if 0
	if (r_currententity != &cl.viewent)
		r_ziscale = (float)0x8000 * (float)0x10000;
	else
		r_ziscale = (float)0x8000 * (float)0x10000 * 3.0;
#endif
	 i = 0;
	//
	// locate the proper data
	//
	r_aliashdr = (aliashdr_t *)Mod_Extradata (r_currentmodel);
	
	//if(paliashdr->frames[r_currententity->frame].type ==0)
	//	return;

	r_mdl = (mdl_t *)BYTE_OFFSET(r_aliashdr,r_aliashdr->model);
		
#define fdst16(n)		  ((int)((n) * (1 << 4)))
	//twidth = fdst16(pmdl->skinwidth)>>1;
	
	
	R_SetAliasSkin(r_aliashdr);

	//since setting the skin may require loading the skin
	//this may cause memory to be allocated which may cause the model
	//to dump out of the cache

	//we could just bail and draw it next frame or reload it

	if(!r_currentmodel->cache.data)
	{
		r_aliashdr = (aliashdr_t *)Mod_Extradata (r_currentmodel);
		r_mdl = (mdl_t *)BYTE_OFFSET(r_aliashdr,r_aliashdr->model);
	}
	
	R_SetAliasFrame(r_aliashdr);
#if 0
	if(pmdl->numtris <= 600)
	{
		memcpy(tri,r_ptri,sizeof(mtriangle_t)*pmdl->numtris);
		r_ptri = tri;
	}
	if(pmdl->numverts <= 2000)
	{
		memcpy(vert,r_pverts,sizeof(trivertx_t)*pmdl->numverts);
		r_pverts = vert;
	}
#endif

	//
	// draw all the triangles
	//
#if 0
#ifdef NDS
	glPushMatrix ();
	glLoadIdentity();
#endif
	R_AliasPreparePoints();
#ifdef NDS
	glPopMatrix (1);
	return;
#endif
#endif

#if 1
	glPushMatrix ();

		R_RotateForEntity (r_currententity);

#define fvectodsv16(n)        ((n) >> 14)
#ifdef WIN32
#define floattof32(n)        ((int)((n) * (1 << 12))) /*!< \brief convert float to f32 */
#endif
		glTranslate3f32(floattodsv16(r_mdl->scale_origin[0]),
			floattodsv16(r_mdl->scale_origin[1]),
			floattodsv16(r_mdl->scale_origin[2]));
		glScale3f32(floattof32(r_mdl->scale[0])<<2,
			floattof32(r_mdl->scale[1])<<2,
			floattof32(r_mdl->scale[2])<<2);
//r_pverts = (trivertx_t*)(((u32)r_pverts) | 0x0400000);
//r_pstverts = (mstvert_t*)(((u32)r_pstverts) | 0x0400000);
//r_ptri = (mtriangle_t*)(((u32)r_ptri) | 0x0400000);
	//GFX_POLY_FORMAT |= POLY_FORMAT_LIGHT0;
	glBegin(GL_TRIANGLES);

	for(i=0;i<r_numtri;i++) {
		r_alias_tri++;
		
		//c = r_ptri[i].
	
		st0 = &r_pstverts[r_ptri[i].vertindex[0]];
		index0 = &r_pverts[r_ptri[i].vertindex[0]];
		
		st1 = &r_pstverts[r_ptri[i].vertindex[1]];
		index1 = &r_pverts[r_ptri[i].vertindex[1]];
		
		st2 = &r_pstverts[r_ptri[i].vertindex[2]];
		index2 = &r_pverts[r_ptri[i].vertindex[2]];
		
		
		as0 = (st0->s);
		at0 = (st0->t);
		ax0 = index0->v[0];
		ay0 = index0->v[1];
		az0 = index0->v[2];
		
		as1 = (st1->s);
		at1 = (st1->t);
		ax1 = index1->v[0];
		ay1 = index1->v[1];
		az1 = index1->v[2];
		
		as2 = (st2->s);
		at2 = (st2->t);
		ax2 = index2->v[0];
		ay2 = index2->v[1];
		az2 = index2->v[2];
		
		if(!(r_ptri[i].facesfront)) {
			if(st0->onseam)
				as0 += twidth;
			if(st1->onseam)
				as1 += twidth;
			if(st2->onseam)
				as2 += twidth;
		}
				
		//GFX_COLOR = d_8to16table[(((int)(&r_ptri[i]))>>3)&0xff];
		DS_NORMAL(ds_normals[index0->lightnormalindex]);
		DS_TEXCOORD2T16((as0),(at0));
		DS_VERTEX3V16(ax0,ay0,az0);
		
		DS_NORMAL(ds_normals[index1->lightnormalindex]);
		DS_TEXCOORD2T16((as1),(at1));
		DS_VERTEX3V16(ax1,ay1,az1);
		
		DS_NORMAL(ds_normals[index2->lightnormalindex]);
		DS_TEXCOORD2T16((as2),(at2));
		DS_VERTEX3V16(ax2,ay2,az2);
	}
#ifdef NDS
	glPopMatrix (1);
#else
	glEnd();
	glPopMatrix ();
#endif

#endif
}
extern cvar_t	tempv1;
extern cvar_t	tempv2;
extern cvar_t	tempv3;
void 	R_DrawCrosshair();
extern cvar_t	crosshair;


void R_DrawViewModel (void)
{
	//float		ambient[4], diffuse[4];
	//int			ambientlight, shadelight;

	//if (!r_drawviewmodel.value)
	//	return;

	if (chase_active.value)
		return;

	/*if (envmap)
		return;*/

	if (!r_drawentities.value)
		return;

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	r_currententity = &cl.viewent;
	r_currentmodel = r_currententity->model;
	if (!r_currententity->model)
		return;

	/*j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - Length(dist);
		if (add > 0)
			ambientlight += add;
	}

	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;

	// hack the depth range to prevent view model from poking into walls
	glDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));
	*/
#if 0
	//glClearDepth(0x2666);
	MYgluPerspective (r_refdef.fov_y,  vid.aspect,  0.005,  40.0*0.3);
	glMatrixMode(GL_MODELVIEW);
#endif
	//glPushMatrix();
	//glLoadIdentity();
#ifdef NDS
	glRestoreMatrix(3);
#endif
	//glScalef(1,1,-1);
	glRotateX(tempv1.value);
	glRotateY(tempv2.value);
	glRotateZ(tempv3.value);
	if (crosshair.value)
	{
		R_DrawCrosshair();
	}
	R_DrawAliasModel ();
#ifdef NDS
	glRestoreMatrix(4);
#endif
	//glPopMatrix (1);
	//glDepthRange (gldepthmin, gldepthmax);
}

