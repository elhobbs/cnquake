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
// r_misc.c

#include "quakedef.h"
cvar_t	r_ambient = {"r_ambient", "0"};
cvar_t	ds_crosshair_color = {"ds_crosshair_color", "0",true};
cvar_t	ds_draw = {"ds_draw", "1"};
cvar_t	ds_drawsky = {"ds_drawsky", "1"};
cvar_t	ds_drawturb = {"ds_drawturb", "1"};
cvar_t	ds_particles = {"ds_particles", "1"};

#ifdef NDS
#include "cyg-profile.h"
#endif

extern	int d_lightstylevalue[256]; // 8.8 frac of base light value

entity_t	r_worldentity;
extern mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern cvar_t	r_drawentities;
extern cvar_t	r_draw;
/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;


	start = Sys_FloatTime ();
	for (i=0 ; i<128 ; i++)
	{
		r_refdef.viewangles[1] = i/128.0*360.0;
		R_RenderView ();
	}

	stop = Sys_FloatTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);

}



/*
==================
R_InitTextures
==================
*/
void	R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = (texture_t*)Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");
	
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->ds.width = r_notexture_mip->ds.height = 16;
	r_notexture_mip->ds.name = "notexture";
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;
	
	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}	
	r_particle_mip = (texture_t*)Hunk_AllocName (sizeof(texture_t), "particle");
	
	r_particle_mip->width = r_particle_mip->height = 8;
	r_particle_mip->ds.width = r_particle_mip->ds.height = 8;
	r_particle_mip->ds.name = "particle";
	
	r_crosshair_mip = (texture_t*)Hunk_AllocName (sizeof(texture_t), "crosshair");
	
	r_crosshair_mip->width = r_notexture_mip->height = 8;
	r_crosshair_mip->ds.width = r_notexture_mip->ds.height = 8;
	r_crosshair_mip->ds.name = "crosshair";
}

/*
===============
R_Init
===============
*/
void profile_on()
{
#ifdef NDS2
cygprofile_enable();
#endif
}

void profile_off()
{
#ifdef NDS2
cygprofile_disable();
cygprofile_end();
#endif
}

void R_InitParticles (void);
void R_Init (void)
{	
	extern byte *hunk_base;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("profon", profile_on);	
	Cmd_AddCommand ("profoff", profile_off);	
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_draw);
	Cvar_RegisterVariable (&r_ambient);
	Cvar_RegisterVariable (&ds_crosshair_color);
	Cvar_RegisterVariable (&ds_draw);
	Cvar_RegisterVariable (&ds_drawsky);
	Cvar_RegisterVariable (&ds_drawturb);
	Cvar_RegisterVariable (&ds_particles);
	//Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	

	/*Cvar_RegisterVariable (&r_norefresh);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_shadows);
	Cvar_RegisterVariable (&r_wateralpha);
	Cvar_RegisterVariable (&r_dynamic);
	Cvar_RegisterVariable (&r_novis);
	Cvar_RegisterVariable (&r_speeds);*/

	R_InitParticles ();

}

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
}


/*
===============
R_NewMap
===============
*/
void R_ClearParticles (void);
void R_NewMap (void)
{
	int		i;
	bmodel_t *model = cl.worldmodel->bmodel;
	
	for (i=0 ; i<256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	memset (&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<model->numleafs ; i++)
		model->leafs[i].efrags = NULL;
		 	
	r_viewleaf = NULL;
	R_ClearParticles ();

	//GL_BuildLightmaps ();

	// identify sky texture
#if 0
	skytexturenum = -1;
	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		if (!cl.worldmodel->textures[i])
			continue;
		if (!Q_strncmp(cl.worldmodel->textures[i]->name,"sky",3) )
			skytexturenum = i;
 		cl.worldmodel->textures[i]->texturechain = NULL;
	}
#endif
}

void D_FlushCaches (void)
{
}


