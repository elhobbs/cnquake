#include "quakedef.h"

#ifdef NDS
#define MULV(_a,_b) ((int)((((long long)_a)*((long long)_b))>>16))
#define DIVV(_a,_b) ((int)((((long long)_a)<<16)/((long long)_b)))
#else
#define MULV(_a,_b) ((int)((((__int64)_a)*((__int64)_b))>>16))
#define DIVV(_a,_b) ((int)((((__int64)_a)<<16)/((__int64)_b)))
#endif

#ifdef NDS
typedef long long big_int;
#define NDS_INLINE inline
#else
typedef __int64 big_int;
#define NDS_INLINE static __forceinline
#endif

NDS_INLINE big_int mul64(big_int a,big_int b)
{
	return (a*b)>>16;
}

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)
#define	SURFACE_CLIP_EPSILON	(0.03125f)

extern particle_t	*active_particles, *free_particles;
int SV_HullPointContents (hull_t *hull, int num, vec3_t p);

int part_draw = 0;
void putParticleLine (vec3_t start, vec3_t end, int c)
{
#if 0
	vec3_t		vec,pt;
	float		len;
	int			j;
	particle_t	*p;
	int			dec = 8;
	float dp,cc;

	if(part_draw == 0)
		return;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	if(len == 0.0f)
		return;
	dp = 16.0/len;
	cc=c;

	VectorCopy(start,pt);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		
		VectorCopy (vec3_origin, p->vel);
		VectorCopy (pt,p->org);
		p->die = cl.time + 7;
		p->type = pt_static;
		p->color = (-c)&15;
		

		VectorAdd (pt, vec, pt);
		cc+=dp;
		c = cc;
	}
#endif
}

int SV_HullCheck_r (hull_t *hull , int num, float p1f, float p2f, float* p1, float* p2, trace_t *trace)
{
	mclipnode_t	*node,*split;
	mplane_t	*plane;
	float		t[2], idist;
	float		frac, frac2;
	int			i;
	vec3_t		mid, mid2;
	int			side, ret, onplane;
	float		midf, midf2, adjf;


	split = 0;
	//find the split node
	do {
		//found a leaf
		if(num < 0)
		{
			//do something???
			//putParticleLine(p1,p2,12*16);
			if (num != CONTENTS_SOLID)
			{
				//putParticleLine(p1,p2,14*16);
				trace->allsolid = false;
				if (num == CONTENTS_EMPTY)
					trace->inopen = true;
				else
					trace->inwater = true;
				return 0;		// empty
			}
			else
			{
				if(trace->inopen == false)
					trace->startsolid = true;
				return 1;
			}
		}

		//
		// find the point distances
		//
		node = hull->clipnodes + num;
		plane = node->plane;

		if (plane->type < 3)
		{
			t[0] = p1[plane->type] - plane->dist;
			t[1] = p2[plane->type] - plane->dist;
		}
		else
		{
			t[0] = DotProduct (plane->normal, p1) - plane->dist;
			t[1] = DotProduct (plane->normal, p2) - plane->dist;
		}
		//both in front
		if (t[0] >= 0 && t[1] >= 0)
		{
			num = node->children[0];
			continue;
		}

		//both behind
		if (t[0] < 0 && t[1] < 0)
		{
			num = node->children[1];
			continue;
		}
		
		split = node;
		//we have a split
		break;
	} while(1);

	// put the crosspoint SURFACE_CLIP_EPSILON pixels on the near side
	onplane = 0;
	if ( t[0] < t[1] ) {
		idist = 1.0/(t[0]-t[1]);
		side = 1;
		frac = frac2 = t[0]*idist;
		adjf = SURFACE_CLIP_EPSILON/(t[1]-t[0]);
	} else if (t[0] > t[1]) {
		idist = 1.0/(t[0]-t[1]);
		side = 0;
		frac = frac2 = t[0]*idist;
		adjf = SURFACE_CLIP_EPSILON/(t[0]-t[1]);
	} else {
		side = 0;
		frac = 1;
		frac2 = 0;
		//adjf = adj[0] = adj[1] = adj[2] = 0.0f;
		adjf = 0.0f;
		//this is a point
		//do something special???
	}

	frac -= adjf;

	// move up to the node
	if ( frac < 0 ) {
		frac = 0;
	}
	if ( frac > 1 ) {
		frac = 1;
	}
		
	midf = p1f + (p2f - p1f)*frac;

	mid[0] = p1[0] + (frac)*(p2[0] - p1[0]);
	mid[1] = p1[1] + (frac)*(p2[1] - p1[1]);
	mid[2] = p1[2] + (frac)*(p2[2] - p1[2]);

	ret = SV_HullCheck_r( hull, node->children[side], p1f, midf, p1, mid, trace );
	switch(ret)
	{
	case 0:
		break;
	case 1:
		if (!side)
		{
			VectorCopy (plane->normal, trace->plane.normal);
			trace->plane.dist = plane->dist;
		}
		else
		{
			VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
			trace->plane.dist = -plane->dist;
		}
		trace->fraction = midf;
		VectorCopy (mid, trace->endpos);
#if 0
		if(SV_HullPointContents(hull,hull->firstclipnode,mid) == CONTENTS_SOLID)
		{
 			Con_Printf("check in solid 1\n");;
		}
#endif
		return 2;
	case 2:
		return 2;
	default:
		Sys_Error("bad");
		break;
	}

	frac2 += adjf;

	// go past the node
	if ( frac2 < 0 ) {
		frac2 = 0;
	}
	if ( frac2 > 1 ) {
		frac2 = 1;
	}
		
	midf2 = p1f + (p2f - p1f)*frac2;

	mid2[0] = p1[0] + (frac2)*(p2[0] - p1[0]);
	mid2[1] = p1[1] + (frac2)*(p2[1] - p1[1]);
	mid2[2] = p1[2] + (frac2)*(p2[2] - p1[2]);

	//if(SV_HullPointContents(hull,num,mid2) == CONTENTS_SOLID)
	if(SV_HullPointContents(hull,node->children[side^1],mid) == CONTENTS_SOLID)
	{
		if (!side)
		{
			VectorCopy (plane->normal, trace->plane.normal);
			trace->plane.dist = plane->dist;
		}
		else
		{
			VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
			trace->plane.dist = -plane->dist;
		}
		trace->fraction = midf;
		VectorCopy (mid, trace->endpos);
#if 0
		if(SV_HullPointContents(hull,hull->firstclipnode,mid) == CONTENTS_SOLID)
		{
 			Con_Printf("check in solid 2\n");;
		}
#endif
		return 2;
	}

	ret = SV_HullCheck_r( hull, node->children[side^1], midf, p2f, mid, p2 , trace);
	switch(ret)
	{
	case 0:
		break;
	case 1:
		if (!side)
		{
			VectorCopy (plane->normal, trace->plane.normal);
			trace->plane.dist = plane->dist;
		}
		else
		{
			VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
			trace->plane.dist = -plane->dist;
		}
		trace->fraction = midf;
		VectorCopy (mid, trace->endpos);
#if 0
		if(SV_HullPointContents(hull,hull->firstclipnode,mid) == CONTENTS_SOLID)
		{
 			Con_Printf("check in solid 3\n");;
		}
#endif
		return 2;
	case 2:
		return 2;
	default:
		Sys_Error("bad");
		break;
	}

	return 0;
}

qboolean SV_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
#if 1
	return SV_HullCheck_r (hull ,  num,  p1f,  p2f, p1, p2, trace);
#else
	mclipnode_t	*node;
	mplane_t	*plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

// check for empty
	if (num < 0)
	{
		if (num != CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
		}
		else
			trace->startsolid = true;
		return true;		// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
		Sys_Error ("SV_RecursiveHullCheck: bad node number");

//
// find the point distances
//
	node = hull->clipnodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
	}
	
#if 1
	if (t1 >= 0 && t2 >= 0)
		return SV_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if (t1 < 0 && t2 < 0)
		return SV_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#else
	if ( (t1 >= DIST_EPSILON && t2 >= DIST_EPSILON) || (t2 > t1 && t1 >= 0) )
		return SV_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if ( (t1 <= -DIST_EPSILON && t2 <= -DIST_EPSILON) || (t2 < t1 && t1 <= 0) )
		return SV_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#endif

// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0)
		frac = (t1 + DIST_EPSILON)/(t1-t2);
	else
		frac = (t1 - DIST_EPSILON)/(t1-t2);
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
		
	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	side = (t1 < 0);

// move up to the node
	if (!SV_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace) )
		return false;

#ifdef PARANOID
	if (SV_HullPointContents (sv_hullmodel, mid, node->children[side])
	== CONTENTS_SOLID)
	{
		Con_Printf ("mid PointInHullSolid\n");
		return false;
	}
#endif
	
	if (SV_HullPointContents (hull, node->children[side^1], mid)
	!= CONTENTS_SOLID)
// go past the node
		return SV_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);
	
	if (trace->allsolid)
		return false;		// never got out of the solid area
		
//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy (plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (SV_HullPointContents (hull, hull->firstclipnode, mid)
	== CONTENTS_SOLID)
	{ // shouldn't really happen, but does occasionally
		frac -= 0.1f;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy (mid, trace->endpos);
			Con_DPrintf ("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f)*frac;
		for (i=0 ; i<3 ; i++)
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
	}

	trace->fraction = midf;
	VectorCopy (mid, trace->endpos);

	return false;
#endif
}



#ifdef NDS2
int SV_HullPointContentsf (hull_t *hull, int num, int* pp);
/*
==================
SV_HullPointContents

==================
*/
int SV_HullPointContents (hull_t *hull, int num, vec3_t p)
{
	int			pp[3];

	pp[0] = floattofp16(p[0]);
	pp[1] = floattofp16(p[1]);
	pp[2] = floattofp16(p[2]);

	return SV_HullPointContentsf (hull, num, pp);
}

int SV_HullPointContentsf (hull_t *hull, int num, int* pp)
{
	int		d;
	mclipnode_t	*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			Sys_Error ("SV_HullPointContents: bad node number");
	
		node = hull->clipnodes + num;
		plane = node->plane;
		
		if (plane->type < 3)
		{
			d = pp[plane->type] - plane->idist;
			//dd = p[plane->type] - plane->dist;
		}
		else
		{
			d = MULV(plane->inormal[0],pp[0]) + MULV(plane->inormal[1],pp[1]) + MULV(plane->inormal[2],pp[2]) - plane->idist;
			//dd = DotProduct (plane->normal, p) - plane->dist;
		}
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}
	
	return num;
}
#else
int SV_HullPointContents (hull_t *hull, int num, vec3_t p)
{
	float		d;
	mclipnode_t	*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			Sys_Error ("SV_HullPointContents: bad node number");
	
		node = hull->clipnodes + num;
		//plane = hull->planes + node->planenum;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}
	
	return num;
}
#endif
