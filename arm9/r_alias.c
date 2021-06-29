#include "quakedef.h"
#include "r_alias.h"


finalvert_t			*r_pfinalverts;
auxvert_t			*r_pauxverts;
float				aliastransform[3][4];
static vec3_t		alias_forward, alias_right, alias_up;
vec3_t				r_entorigin,r_modelorg;
float				r_aliasuvscale = 1.0;

affinetridesc_t		r_affinetridesc;
byte				*d_pcolormap;
mtriangle_t			*r_tri2;

byte				*r_skintable[MAX_LBM_HEIGHT];
byte				*r_skinstart;

finalvert_t			finalverts[MAXALIASVERTS + ((CACHE_SIZE - 1) / sizeof(finalvert_t)) + 1];
auxvert_t			auxverts[MAXALIASVERTS];
int					r_alias_top,r_alias_right,r_alias_bottom,r_alias_left;
int					r_alias_width, r_alias_height;

/*
================
D_PolysetUpdateTables
================
*/
void D_PolysetUpdateTables (void)
{
	int		i;
	byte	*s;
	
	if (r_affinetridesc.skinwidth != r_skinwidth ||
		r_affinetridesc.pskin != r_skinstart)
	{
		r_skinwidth = r_affinetridesc.skinwidth;
		r_skinstart = (byte*)r_affinetridesc.pskin;
		s = r_skinstart;
		for (i=0 ; i<MAX_LBM_HEIGHT ; i++, s+=r_skinwidth)
			r_skintable[i] = s;
	}
}


void D_PolysetRecursiveTriangle (int *lp1, int *lp2, int *lp3)
{
	int		*temp;
	int		d;
	int		new_p[6];
	int		z;
	short	*zbuf;

	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;

	d = lp3[0] - lp2[0];
	if (d < -1 || d > 1)
		goto split2;
	d = lp3[1] - lp2[1];
	if (d < -1 || d > 1)
		goto split2;

	d = lp1[0] - lp3[0];
	if (d < -1 || d > 1)
		goto split3;
	d = lp1[1] - lp3[1];
	if (d < -1 || d > 1)
	{
split3:
		temp = lp1;
		lp1 = lp3;
		lp3 = lp2;
		lp2 = temp;

		goto split;
	}

	return;			// entire tri is filled

split2:
	temp = lp1;
	lp1 = lp2;
	lp2 = lp3;
	lp3 = temp;

split:
// split this edge
	new_p[0] = (lp1[0] + lp2[0]) >> 1;
	new_p[1] = (lp1[1] + lp2[1]) >> 1;
	new_p[2] = (lp1[2] + lp2[2]) >> 1;
	new_p[3] = (lp1[3] + lp2[3]) >> 1;
	new_p[5] = (lp1[5] + lp2[5]) >> 1;

// draw the point if splitting a leading edge
	if (lp2[1] > lp1[1])
		goto nodraw;
	if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
		goto nodraw;

#if 0
	z = new_p[5]>>16;
	zbuf = zspantable[new_p[1]] + new_p[0];
	if (z >= *zbuf)
	{
		int		pix;
		
		*zbuf = z;
		pix = d_pcolormap[skintable[new_p[3]>>16][new_p[2]>>16]];
		d_viewbuffer[d_scantable[new_p[1]] + new_p[0]] = pix;
	}
#else
	{
		int		pix;
		
	//GFX_COLOR = d_8to16table[(((int)(&r_ptri[i]))>>3)&0xff];
		//pix = (((int)(r_tri2))>>3)&0xff;//d_pcolormap[skintable[new_p[3]>>16][new_p[2]>>16]];
		pix = d_pcolormap[r_skintable[new_p[3]>>16][new_p[2]>>16]];
		vid.aliasbuffer[new_p[1]*256 + new_p[0]] = pix;
	}

#endif

nodraw:
// recursively continue
	D_PolysetRecursiveTriangle (lp3, lp1, new_p);
	D_PolysetRecursiveTriangle (lp3, new_p, lp2);
}
#ifdef WIN32
void glTexCoord2t16(short s,short t)
{
}
void glVertex3v16(short x,short y,short z)
{
}
#endif
/*
================
D_DrawSubdiv
================
*/
void D_DrawNDS (void)
{
	auxvert_t		*pav, *i0,*i1,*i2;
	mtriangle_t		*ptri;
	finalvert_t		*pfv, *index0, *index1, *index2;
	int				i, x, y, z;
	int				lnumtriangles;
	int				s, t;
			int		s0, s1, s2;
			int		t0, t1, t2;

	pav = r_pauxverts;
	pfv = r_affinetridesc.pfinalverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

#ifdef NDS
	glBegin(GL_TRIANGLES);
#endif
	for (i=0 ; i<lnumtriangles ; i++)
	{
		r_tri2 = &ptri[i];
		index0 = pfv + ptri[i].vertindex[0];
		index1 = pfv + ptri[i].vertindex[1];
		index2 = pfv + ptri[i].vertindex[2];

		if (((index0->v[1]-index1->v[1]) *
			 (index0->v[0]-index2->v[0]) -
			 (index0->v[0]-index1->v[0]) * 
			 (index0->v[1]-index2->v[1])) >= 0)
		{
			continue;
		}

		i0 = pav + ptri[i].vertindex[0];
		i1 = pav + ptri[i].vertindex[1];
		i2 = pav + ptri[i].vertindex[2];
#if 1
		if (ptri[i].facesfront)
		{
			s = index0->v[2];
			t = index0->v[3];
			x = i0->fv[0]*(1<<2);
			y = i0->fv[1]*(1<<2);
			z = i0->fv[2]*(1<<2);
			glTexCoord2t16(s,t);
			glVertex3v16(x,y,z);

			s = index1->v[2];
			t = index1->v[3];
			x = i1->fv[0]*(1<<2);
			y = i1->fv[1]*(1<<2);
			z = i1->fv[2]*(1<<2);
			glTexCoord2t16(s,t);
			glVertex3v16(x,y,z);

			s = index2->v[2];
			t = index2->v[3];
			x = i2->fv[0]*(1<<2);
			y = i2->fv[1]*(1<<2);
			z = i2->fv[2]*(1<<2);
			glTexCoord2t16(s,t);
			glVertex3v16(x,y,z);

		}
		else
		{

			s0 = index0->v[2];
			s1 = index1->v[2];
			s2 = index2->v[2];

			if (index0->flags & ALIAS_ONSEAM)
				index0->v[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				index1->v[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				index2->v[2] += r_affinetridesc.seamfixupX16;

			s = index0->v[2];
			t = index0->v[3];
			x = i0->fv[0]*(1<<2);
			y = i0->fv[1]*(1<<2);
			z = i0->fv[2]*(1<<2);
			glTexCoord2t16(s,t);
			glVertex3v16(x,y,z);

			s = index1->v[2];
			t = index1->v[3];
			x = i1->fv[0]*(1<<2);
			y = i1->fv[1]*(1<<2);
			z = i1->fv[2]*(1<<2);
			glTexCoord2t16(s,t);
			glVertex3v16(x,y,z);

			s = index2->v[2];
			t = index2->v[3];
			x = i2->fv[0]*(1<<2);
			y = i2->fv[1]*(1<<2);
			z = i2->fv[2]*(1<<2);
			glTexCoord2t16(s,t);
			glVertex3v16(x,y,z);

			index0->v[2] = s0;
			index1->v[2] = s1;
			index2->v[2] = s2;
		}
#endif
	}
#ifdef NDS
	glEnd();
#endif
}

/*
================
D_DrawSubdiv
================
*/
void D_DrawSubdiv (void)
{
	mtriangle_t		*ptri;
	finalvert_t		*pfv, *index0, *index1, *index2;
	int				i;
	int				lnumtriangles;

	pfv = r_affinetridesc.pfinalverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

	for (i=0 ; i<lnumtriangles ; i++)
	{
		r_tri2 = &ptri[i];
		index0 = pfv + ptri[i].vertindex[0];
		index1 = pfv + ptri[i].vertindex[1];
		index2 = pfv + ptri[i].vertindex[2];

		if (((index0->v[1]-index1->v[1]) *
			 (index0->v[0]-index2->v[0]) -
			 (index0->v[0]-index1->v[0]) * 
			 (index0->v[1]-index2->v[1])) >= 0)
		{
			continue;
		}

		d_pcolormap = &r_currententity->colormap[index0->v[4] & 0xFF00];

		if (ptri[i].facesfront)
		{
			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);
		}
		else
		{
			int		s0, s1, s2;

			s0 = index0->v[2];
			s1 = index1->v[2];
			s2 = index2->v[2];

			if (index0->flags & ALIAS_ONSEAM)
				index0->v[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				index1->v[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				index2->v[2] += r_affinetridesc.seamfixupX16;

			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);

			index0->v[2] = s0;
			index1->v[2] = s1;
			index2->v[2] = s2;
		}
	}
}

/*
================
D_PolysetDraw
================
*/
void D_PolysetDraw (void)
{
/*	spanpackage_t	spans[DPS_MAXSPANS + 1 +
			((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
						// one extra because of cache line pretouching

	a_spans = (spanpackage_t *)
			(((long)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_affinetridesc.drawtype)
	{*/
		//D_DrawSubdiv ();
		D_DrawNDS();
	/*}
	else
	{
		D_DrawNonSubdiv ();
	}*/
}
/*
================
R_AliasSetUpTransform
================
*/
void R_AliasSetUpTransform (int trivial_accept)
{
	int				i;
	float			rotationmatrix[3][4], t2matrix[3][4];
	static float	tmatrix[3][4];
	static float	viewmatrix[3][4];
	vec3_t			angles;

// TODO: should really be stored with the entity instead of being reconstructed
// TODO: should use a look-up table
// TODO: could cache lazily, stored in the entity

	angles[ROLL] = r_currententity->angles[ROLL];
	angles[PITCH] = -r_currententity->angles[PITCH];
	angles[YAW] = r_currententity->angles[YAW];
	AngleVectors (angles, alias_forward, alias_right, alias_up);

	tmatrix[0][0] = r_mdl->scale[0];
	tmatrix[1][1] = r_mdl->scale[1];
	tmatrix[2][2] = r_mdl->scale[2];

	tmatrix[0][3] = r_mdl->scale_origin[0];
	tmatrix[1][3] = r_mdl->scale_origin[1];
	tmatrix[2][3] = r_mdl->scale_origin[2];

// TODO: can do this with simple matrix rearrangement

	for (i=0 ; i<3 ; i++)
	{
		t2matrix[i][0] = alias_forward[i];
		t2matrix[i][1] = -alias_right[i];
		t2matrix[i][2] = alias_up[i];
	}

	t2matrix[0][3] = -r_modelorg[0];
	t2matrix[1][3] = -r_modelorg[1];
	t2matrix[2][3] = -r_modelorg[2];

// FIXME: can do more efficiently than full concatenation
	R_ConcatTransforms (t2matrix, tmatrix, rotationmatrix);

// TODO: should be global, set when vright, etc., set
	VectorCopy (r_vright, viewmatrix[0]);
	VectorCopy (r_vup, viewmatrix[1]);
	VectorInverse (viewmatrix[1]);
	VectorCopy (r_vpn, viewmatrix[2]);

//	viewmatrix[0][3] = 0;
//	viewmatrix[1][3] = 0;
//	viewmatrix[2][3] = 0;

	R_ConcatTransforms (viewmatrix, rotationmatrix, aliastransform);

// do the scaling up of x and y to screen coordinates as part of the transform
// for the unclipped case (it would mess up clipping in the clipped case).
// Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
// correspondingly so the projected x and y come out right
// FIXME: make this work for clipped case too?
	if (trivial_accept)
	{
		for (i=0 ; i<4 ; i++)
		{
			aliastransform[0][i] *= aliasxscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[1][i] *= aliasyscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[2][i] *= 1.0 / ((float)0x8000 * 0x10000);

		}
	}
}

/*
================
R_AliasProjectFinalVert
================
*/
void R_AliasProjectFinalVert (finalvert_t *fv, auxvert_t *av)
{
	float	zi;

// project points
	zi = 1.0 / av->fv[2];

	fv->v[5] = zi * r_ziscale;

	fv->v[0] = (av->fv[0] * aliasxscale * zi) + aliasxcenter;
	fv->v[1] = (av->fv[1] * aliasyscale * zi) + aliasycenter;
}

/*
================
R_AliasTransformFinalVert
================
*/
void R_AliasTransformFinalVert (finalvert_t *fv, auxvert_t *av,
	trivertx_t *pverts, mstvert_t *pstverts)
{
	int		temp;
	float	lightcos, *plightnormal;

	av->fv[0] = DotProduct(pverts->v, aliastransform[0]) +
			aliastransform[0][3];
	av->fv[1] = DotProduct(pverts->v, aliastransform[1]) +
			aliastransform[1][3];
	av->fv[2] = DotProduct(pverts->v, aliastransform[2]) +
			aliastransform[2][3];

	fv->v[2] = ((int)pstverts->s);//<<16;
	fv->v[3] = ((int)pstverts->t);//<<16;

	fv->flags = pstverts->onseam;

	/*
// lighting
	plightnormal = r_avertexnormals[pverts->lightnormalindex];
	lightcos = DotProduct (plightnormal, r_plightvec);
	temp = r_ambientlight;

	if (lightcos < 0)
	{
		temp += (int)(r_shadelight * lightcos);

	// clamp; because we limited the minimum ambient and shading light, we
	// don't have to clamp low light, just bright
		if (temp < 0)
			temp = 0;
	}

	fv->v[4] = temp;
	*/
}

/*
================
R_AliasPreparePoints

General clipped case
================
*/
void R_AliasPreparePoints (void)
{
	int			i;
	mstvert_t	*pstverts;
	finalvert_t	*fv;
	auxvert_t	*av;
	mtriangle_t	*ptri;
	trivertx_t	*pverts;
	finalvert_t	*pfv[3];


	VectorCopy (r_currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, r_modelorg);

	R_AliasSetUpTransform (r_currententity->trivial_accept);
	D_PolysetUpdateTables();

// cache align
	r_pfinalverts = (finalvert_t *)
			(((long)&finalverts[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	r_pauxverts = &auxverts[0];

	//pstverts = (stvert_t *)((byte *)paliashdr + paliashdr->stverts);
	//r_anumverts = pmdl->numverts;
	pstverts = r_pstverts;
	pverts = r_pverts;

	fv = r_pfinalverts;
	av = r_pauxverts;

	r_alias_top = 192;
	r_alias_left = 256;
	r_alias_right = 0;
	r_alias_bottom = 0;

	for (i=0 ; i<r_numverts ; i++, fv++, av++, pverts++, pstverts++)
	{
		R_AliasTransformFinalVert (fv, av, pverts, pstverts);
		if (av->fv[2] < ALIAS_Z_CLIP_PLANE)
			fv->flags |= ALIAS_Z_CLIP;
		else
		{
			 R_AliasProjectFinalVert (fv, av);

			if (fv->v[0] < r_refdef.aliasvrect.x)
			{
				fv->flags |= ALIAS_LEFT_CLIP;
				r_alias_left = r_refdef.aliasvrect.x;
			}
			else
			{
				if(fv->v[0] < r_alias_left)
					r_alias_left = fv->v[0];
			}

			if (fv->v[1] < r_refdef.aliasvrect.y)
			{
				fv->flags |= ALIAS_TOP_CLIP;
				r_alias_top = r_refdef.aliasvrect.y;
			}
			else
			{
				if(fv->v[1] < r_alias_top)
					r_alias_top = fv->v[1];
			}

			if (fv->v[0] > r_refdef.aliasvrectright)
			{
				fv->flags |= ALIAS_RIGHT_CLIP;
				r_alias_right = r_refdef.aliasvrectright;
			}
			else
			{
				if(fv->v[0] > r_alias_right)
					r_alias_right = fv->v[0];
			}

			if (fv->v[1] > r_refdef.aliasvrectbottom)
			{
				fv->flags |= ALIAS_BOTTOM_CLIP;	
				r_alias_bottom = r_refdef.aliasvrectbottom;
			}
			else
			{
				if(fv->v[1] > r_alias_bottom)
					r_alias_bottom = fv->v[1];
			}
		}
	}

	r_alias_width = r_alias_right - r_alias_left;
	r_alias_height = r_alias_bottom - r_alias_top;

	//if(r_alias_width > 64 || r_alias_height > 64)
	//{
	//	return;
	//}
#if 1
//
// clip and draw all triangles
//
	r_affinetridesc.numtriangles = 1;

	ptri = (mtriangle_t *)((byte *)r_aliashdr + r_aliashdr->triangles);
	for (i=0 ; i<r_mdl->numtris ; i++, ptri++)
	{
		pfv[0] = &r_pfinalverts[ptri->vertindex[0]];
		pfv[1] = &r_pfinalverts[ptri->vertindex[1]];
		pfv[2] = &r_pfinalverts[ptri->vertindex[2]];

		if ( pfv[0]->flags & pfv[1]->flags & pfv[2]->flags & (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) )
			continue;		// completely clipped
		
		if ( ! ( (pfv[0]->flags | pfv[1]->flags | pfv[2]->flags) &
			(ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) ) )
		{	// totally unclipped
			r_affinetridesc.pfinalverts = r_pfinalverts;
			r_affinetridesc.ptriangles = ptri;
			D_PolysetDraw ();
		}
		else		
		{	// partially clipped
			R_AliasClipTriangle (ptri);
		}
	}
#endif
}
