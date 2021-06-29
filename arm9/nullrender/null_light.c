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
// r_light.c

#include "quakedef.h"
#include "r_local.h"

int	r_dlightframecount;

extern int		d_lightstylevalue[256];	// 8.8 fraction of base light value


/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight (void)
{
	int			i,j,k;
	
//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)(cl.time*10);
	for (j=0 ; j<MAX_LIGHTSTYLES ; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		k = k*22;
		d_lightstylevalue[j] = k;
	}	
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/
#ifdef NDS
typedef long long big_int;
#define NDS_INLINE static inline
NDS_INLINE big_int idiv64(big_int a,big_int b)
{
	REG_DIVCNT = DIV_64_64;
	
	while(REG_DIVCNT & DIV_BUSY);
	
	REG_DIV_NUMER = a<<16;
	REG_DIV_DENOM = b;
	
	while(REG_DIVCNT & DIV_BUSY);
	
	return (REG_DIV_RESULT);
}

#else
typedef __int64 big_int;
#define NDS_INLINE static __forceinline
NDS_INLINE big_int idiv64(big_int a,big_int b)
{
	return (a<<16)/b;
}

#endif

NDS_INLINE big_int imul64(big_int a,big_int b)
{
	return (a*b)>>16;
}

NDS_INLINE big_int mul64(big_int a,big_int b)
{
	return (a*b);
}

#define IIDotProduct(x,y) (mul64(x[0],y[0])+mul64(x[1],y[1])+mul64(x[2],y[2]))
#define IDotProduct(x,y) (imul64(x[0],y[0])+imul64(x[1],y[1])+imul64(x[2],y[2]))

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	mplane_t	*splitplane;
	//float		dist;
	int idist;
	msurface_t	*surf;
	int			i;
extern bmodel_t	*r_currentbmodel;
	//bmodel_t *model;
	do {
		
		if (node == 0 || node->contents < 0)
			return;

		splitplane = node->plane;
		//dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;
		if (splitplane->type < 3)
		{
			idist = light->iorigin[splitplane->type] - (splitplane->idist>>16);
		}
		else
		{
			idist = (IIDotProduct (splitplane->inormal, light->iorigin) - splitplane->idist)>>16;
		}
		
		if (idist > light->iradius)
		{
			//R_MarkLights (light, bit, node->children[0]);
			//return;
			node = node->children[0];
			continue;
		}
		if (idist < -light->iradius)
		{
			//R_MarkLights (light, bit, node->children[1]);
			//return;
			node = node->children[1];
			continue;
		}
			
	// mark the polygons
		//model = (bmodel_t *)cl.worldmodel->cache.data;
		surf = r_currentbmodel->surfaces + node->firstsurface;
		for (i=0 ; i<node->numsurfaces ; i++, surf++)
		{
			if (surf->dlightframe != r_dlightframecount)
			{
				surf->dlightbits = 0;
				surf->dlightframe = r_dlightframecount;
			}
			surf->dlightbits |= bit;
		}

		R_MarkLights (light, bit, node->children[0]);
		//R_MarkLights (light, bit, node->children[1]);
		node = node->children[1];
	}while(1);

}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;
extern bmodel_t	*r_currentbmodel;
	//bmodel_t *model;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	l = cl_dlights;

	//model = (bmodel_t *)cl.worldmodel->cache.data;
	r_currentbmodel = cl.worldmodel->bmodel;
	for (i=0 ; i<MAX_DLIGHTS ; i++, l++)
	{
		if (l->die < cl.time || !l->iradius)
			continue;
		R_MarkLights ( l, 1<<i, r_currentbmodel->nodes );
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/
#define	ISURFACE_CLIP_EPSILON	(2048L)
#define HULLCHECKSTATE_EMPTY 0
#define HULLCHECKSTATE_SOLID 1
#define HULLCHECKSTATE_DONE 2

//int  RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end)
int RecursiveLightPoint_f (mnode_t	*node, int p1f, int p2f, int* p1, int* p2)
{
	mnode_t	*split;
	mplane_t	*plane;
	int		t[2];
	int		frac, frac2;
	int		mid[3], mid2[3];
	int			side, ret, onplane;
	int		midf, midf2, adjf;
	mtexinfo_t	*tex;
	byte		*lightmap;
	unsigned	scale;
	int			i,maps;
	bmodel_t	*model;
	int *u,*v;
	int x,y,xs,ys,ss,tt;
	msurface_t	*surf;
	int pt[3];
	int			r, ds, dt;


	split = 0;
	//find the split node
	do {
		//found a leaf
		if(node->contents)
		{
			//do something???
			//putParticleLine(p1,p2,12*16);
			if (node->contents != CONTENTS_SOLID)
			{
				return -1;		// empty
			}
			else
			{
				return -2;		//solid
			}
		}

		//
		// find the point distances
		//
		//node = hull->clipnodes + num;
		plane = node->plane;

		if (plane->type < 3)
		{
			t[0] = p1[plane->type] - plane->idist;
			t[1] = p2[plane->type] - plane->idist;
		}
		else
		{
			t[0] = IDotProduct (plane->inormal, p1) - plane->idist;
			t[1] = IDotProduct (plane->inormal, p2) - plane->idist;
		}
		//both in front
		if (t[0] >= 0 && t[1] >= 0)
		{
			node = node->children[0];
			continue;
		}

		//both behind
		if (t[0] < 0 && t[1] < 0)
		{
			node = node->children[1];
			continue;
		}
		
		split = node;
		//we have a split
		break;
	} while(1);

	// put the crosspoint SURFACE_CLIP_EPSILON pixels on the near side
	onplane = 0;
	if ( t[0] < t[1] ) {
		//idist = 1.0/(t[0]-t[1]);
		side = 1;
		//frac = frac2 = t[0]*idist;
		frac = frac2 = idiv64(t[0],t[0]-t[1]);
		adjf = idiv64(ISURFACE_CLIP_EPSILON,(t[1]-t[0]));
	} else if (t[0] > t[1]) {
		//idist = 1.0/(t[0]-t[1]);
		side = 0;
		//frac = frac2 = t[0]*idist;
		frac = frac2 = idiv64(t[0],t[0]-t[1]);
		adjf = idiv64(ISURFACE_CLIP_EPSILON,(t[0]-t[1]));
	} else {
		side = 0;
		frac = 1<<16;
		frac2 = 0;
		//adjf = adj[0] = adj[1] = adj[2] = 0.0f;
		adjf = 0;
		//this is a point
		//do something special???
	}

	frac -= adjf;

	// move up to the node
	if ( frac < 0 ) {
		frac = 0;
	}
	if ( frac > (1<<16) ) {
		frac = (1<<16);
	}
		
	midf = p1f + imul64((p2f - p1f),frac);

	mid[0] = p1[0] + imul64((frac),(p2[0] - p1[0]));
	mid[1] = p1[1] + imul64((frac),(p2[1] - p1[1]));
	mid[2] = p1[2] + imul64((frac),(p2[2] - p1[2]));

	ret = RecursiveLightPoint_f( node->children[side], p1f, midf, p1, mid );
	if(ret != -1)
		return ret;


	frac2 += adjf;

	// go past the node
	if ( frac2 < 0 ) {
		frac2 = 0;
	}
	if ( frac2 > (1<<16) ) {
		frac2 = (1<<16);
	}
		
	midf2 = p1f + imul64((p2f - p1f),frac2);

	mid2[0] = p1[0] + imul64((frac2),(p2[0] - p1[0]));
	mid2[1] = p1[1] + imul64((frac2),(p2[1] - p1[1]));
	mid2[2] = p1[2] + imul64((frac2),(p2[2] - p1[2]));


	ret = RecursiveLightPoint_f( node->children[side^1], midf, p2f, mid, p2 );
	//if(ret != 1)
	if(ret != -2)
		return ret;
	/*	
	if (!side)
	{
		VectorCopy (plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
		trace->ifraction = midf;
		IVectorCopy (mid, trace->iendpos);
	}
	else
	{
		VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
		trace->ifraction = midf;
		IVectorCopy (mid, trace->iendpos);
	}*/

	pt[0] = mid[0]>>14;
	pt[1] = mid[1]>>14;
	pt[2] = mid[2]>>14;

	model = cl.worldmodel->bmodel;
	surf = model->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;
		ss = surf->texturemins[0];
		tt = surf->texturemins[1];
		ss <<= 4;
		tt <<= 4;
		xs = (tex->texture->width<<16)/tex->texture->ds.width;
		ys = (tex->texture->height<<16)/tex->texture->ds.height;
		u = tex->ivecs[0];
		v = tex->ivecs[1];
		
		x = CALC_COORD(pt,u);
		y = CALC_COORD(pt,v);
		x = (x-ss)>>4;
		y = (y-tt)>>4;
		x = (x * xs)>>16;//20;
		y = (y * ys)>>16;//20;

/*		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

*/		
		if(x < 0 || y < 0)
		{
			continue;
		}
		if ( x > surf->extents[0] || y > surf->extents[1] )
		{
			continue;
		}
		if (!surf->samples)
			return 0;

		ds = x >> 4;
		dt = y >> 4;

		lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{

			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;

			for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
					maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]];
				r += *lightmap * scale;
				lightmap += ((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}
			
			r >>= 8;
		}
		
		return r;
	}

	return -3;
}
int RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end)
{
	int			r;
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	bmodel_t	*model;
	int *u,*v;
	int x,y,xs,ys,ss,tt;
	int pt[3];
	if (node->contents < 0)
		return -1;		// didn't hit anything
	
// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something
		
	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing
		
	pt[0] = mid[0]*(1<<2);
	pt[1] = mid[1]*(1<<2);
	pt[2] = mid[2]*(1<<2);
// check for impact on this node

	model = cl.worldmodel->bmodel;
	surf = model->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;
		ss = surf->texturemins[0];
		tt = surf->texturemins[1];
		ss <<= 4;
		tt <<= 4;
		xs = (tex->texture->width<<16)/tex->texture->ds.width;
		ys = (tex->texture->height<<16)/tex->texture->ds.height;
		u = tex->ivecs[0];
		v = tex->ivecs[1];
		
		x = CALC_COORD(pt,u);
		y = CALC_COORD(pt,v);
		x = (x-ss)>>4;
		y = (y-tt)>>4;
		x = (x * xs)>>16;//20;
		y = (y * ys)>>16;//20;

/*		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

*/		
		if(x < 0 || y < 0)
		{
			continue;
		}
		if ( x > surf->extents[0] || y > surf->extents[1] )
		{
			continue;
		}
		if (!surf->samples)
			return 0;

		ds = x >> 4;
		dt = y >> 4;

		lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{

			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;

			for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
					maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]];
				r += *lightmap * scale;
				lightmap += ((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}
			
			r >>= 8;
		}
		
		return r;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

int R_LightPoint (vec3_t p)
{
	vec3_t		end;
	int end2[3],p2[3];
	int			i,r;
	bmodel_t	*model = cl.worldmodel->bmodel;
	
	if (!model->lightdata)
		return 255;
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	//r = RecursiveLightPoint (model->nodes, p, end);

	for(i=0;i<3;i++) {
		p2[i] = p[i]*(1<<16);
		end2[i] = end[i]*(1<<16);
	}

	r = RecursiveLightPoint_f (model->nodes, 0,(1<<16), p2, end2);
	
	if (r < 0)
		r = 0;

	if (r < r_refdef.ambientlight)
		r = r_refdef.ambientlight;

	return r;
}

