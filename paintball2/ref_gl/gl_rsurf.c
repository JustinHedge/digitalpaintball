/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// GL_RSURF.C: surface-related refresh code
#include <assert.h>

#include "gl_local.h"

static vec3_t	modelorg;		// relative to viewpoint

msurface_t	*r_alpha_surfaces;
msurface_t	*r_caustic_surfaces; // jitcaustics

#define DYNAMIC_LIGHT_WIDTH  128
#define DYNAMIC_LIGHT_HEIGHT 128

#define LIGHTMAP_BYTES 4

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	MAX_LIGHTMAPS	128

int		c_visible_lightmaps;
int		c_visible_textures;

#define GL_LIGHTMAP_FORMAT GL_RGBA

typedef struct
{
	int internal_format;
	int	current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;


static void		LM_InitBlock( void );
static void		LM_UploadBlock( qboolean dynamic );
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);

extern void R_SetCacheState( msurface_t *surf );
extern void R_BuildLightMap (msurface_t *surf, byte *dest, int stride);

extern qboolean fogenabled; // jitfog
extern qboolean alphasurf; // jitrscript

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *R_TextureAnimation (mtexinfo_t *tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}

/*
================
DrawGLPoly
================
*/

_inline void DrawGLPoly (glpoly_t *p)
{
	int		i;
	float	*v;

	qglBegin(GL_POLYGON);
	v = p->verts[0];

	for (i=0; i<p->numverts; i++,v+=VERTEXSIZE)
	{
		qglTexCoord2f(v[3], v[4]);
		qglVertex3fv(v);
	}

	qglEnd();
}

//============
//PGM
/*
================
DrawGLFlowingPoly -- version of DrawGLPoly that handles scrolling texture
================
*/
void DrawGLFlowingPoly (msurface_t *fa)
{
	int		i;
	float	*v;
	glpoly_t *p;
	float	scroll;

	p = fa->polys;

	scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
	if(scroll == 0.0)
		scroll = -64.0;

#ifdef BEEFQUAKERENDER // jit3dfx
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		VA_SetElem2(tex_array[i],(v[3]+scroll),v[4]);
		VA_SetElem3(vert_array[i],v[0],v[1],v[2]);
	}
	// if (qglLockArraysEXT != 0) qglLockArraysEXT(0,p->numverts);
	qglDrawArrays (GL_POLYGON, 0, p->numverts);
	// if (qglUnlockArraysEXT != 0) qglUnlockArraysEXT();
#else
		qglBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		qglTexCoord2f ((v[3] + scroll), v[4]);
		qglVertex3fv (v);
	}
	qglEnd ();
#endif
}
//PGM
//============

/*
** R_DrawTriangleOutlines
*/
void R_DrawTriangleOutlines(msurface_t *surf) // jit/GuyP, redone
{
	int        i;
	glpoly_t *p;
	float	distcolor; // jit
    if (!gl_showtris->value)
        return;

    // Guy: *\/\/\/ gl_showtris fix begin \/\/\/*
    qglDisable(GL_DEPTH_TEST);
	qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    if (!surf)    // Guy: Called from non-multitexture mode; need to loop through surfaces defined by non-mtex functions
    {
        int j;

        qglDisable(GL_TEXTURE_2D);
        
        for (i = 0; i < MAX_LIGHTMAPS; i++)
        {
            for (surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain)
            {
                for (p = surf->polys; p; p = p->chain)
                {
                    for (j = 2; j < p->numverts; j++)
                    {
                        qglBegin(GL_LINE_STRIP);
                            //qglColor4f(1, 1, 1, 1);
							//distcolor=(p->verts[0][0] + 4096.0f)/8192.0f;
							//distcolor=fabs(p->verts[0][0])/4096.0f; // jitest
							//qglColor4f(distcolor,1,distcolor,1);
                            qglVertex3fv(p->verts[0]);
                            qglVertex3fv(p->verts[j - 1]);
                            qglVertex3fv(p->verts[j]);
                            qglVertex3fv(p->verts[0]);
                        qglEnd();
                    }
                }
            }
        }

        qglEnable(GL_TEXTURE_2D);
    }
    
    else    // Guy: Called from multitexture mode; surface to be rendered in wireframe already passed in
    {
        float    tex_state0,
                 tex_state1;


        GL_SelectTexture(GL_TEXTURE0);
        qglGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &tex_state0);

        GL_SelectTexture(GL_TEXTURE1);
        qglGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &tex_state1);

        GL_EnableMultitexture(false);
        qglDisable(GL_TEXTURE_2D);

        for (p = surf->polys; p; p = p->chain)
        {
            for (i = 2; i < p->numverts; i++)
            {
				//distcolor = p->verts[
				
                qglBegin(GL_LINE_STRIP);
                    //qglColor4f(1, 1, 1, 1);
					//qglColor4f(0,1,0,1);
                    qglVertex3fv(p->verts[0]);
                    qglVertex3fv(p->verts[i - 1]);
                    qglVertex3fv(p->verts[i]);
                    qglVertex3fv(p->verts[0]);
                qglEnd();
            }
        }

        qglEnable(GL_TEXTURE_2D);
        GL_EnableMultitexture(true);
        
        GL_SelectTexture(GL_TEXTURE0);
        GL_TexEnv(tex_state0);

        GL_SelectTexture(GL_TEXTURE1);
        GL_TexEnv(tex_state1);
    }

	qglEnable(GL_DEPTH_TEST);
	// Guy: */\/\/\ gl_showtris fix end /\/\/\*
}

/*
void R_DrawTriangleOutlines (void)
{
	int			i, j;
	glpoly_t	*p;

	qglDisable (GL_TEXTURE_2D);
	qglDisable (GL_DEPTH_TEST);
	qglColor4f (1,1,1,1);

	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		msurface_t *surf;

		for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
		{
			p = surf->polys;
			for ( ; p ; p=p->chain)
			{
				for (j=2 ; j<p->numverts ; j++ )
				{
					qglBegin (GL_LINE_STRIP);
					qglVertex3fv (p->verts[0]);
					qglVertex3fv (p->verts[j-1]);
					qglVertex3fv (p->verts[j]);
					qglVertex3fv (p->verts[0]);
					qglEnd ();
				}
			}
		}
	}

	qglEnable (GL_DEPTH_TEST);
	qglEnable (GL_TEXTURE_2D);
}
*/

/*
** DrawGLPolyChain
*/
#ifdef BEEFQUAKERENDER // jit3dfx
void DrawGLPolyChain( glpoly_t *p, float soffset, float toffset )
{
	if (soffset == 0 && toffset == 0)
	{
		for ( ; p != 0; p = p->chain)
		{
			float *v;
			int j;

			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				VA_SetElem2(tex_array[j],v[5],v[6]);
				VA_SetElem3(vert_array[j],v[0],v[1],v[2]);
			}

			// if (qglLockArraysEXT) qglLockArraysEXT(0,p->numverts);
			qglDrawArrays (GL_POLYGON, 0, p->numverts);
			// if (qglUnlockArraysEXT) qglUnlockArraysEXT();
		}
	}
	else
	{
		for ( ; p != 0; p = p->chain)
		{
			float *v;
			int j;

			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				VA_SetElem2(tex_array[j],(v[5]-soffset),(v[6]-toffset));
				VA_SetElem3(vert_array[j],v[0],v[1],v[2]);
			}
			// if (qglLockArraysEXT) qglLockArraysEXT(0,p->numverts);
			qglDrawArrays (GL_POLYGON, 0, p->numverts);
			// if (qglUnlockArraysEXT)	qglUnlockArraysEXT();
		}
	}
}
#else // jit3dfx:
void DrawGLPolyChain( glpoly_t *p, float soffset, float toffset )
{
	if ( soffset == 0 && toffset == 0 )
	{
		for ( ; p != 0; p = p->chain )
		{
			float *v;
			int j;

			qglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				qglTexCoord2f (v[5], v[6] );
				qglVertex3fv (v);
			}
			qglEnd ();
		}
	}
	else
	{
		for ( ; p != 0; p = p->chain )
		{
			float *v;
			int j;

			qglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				qglTexCoord2f (v[5] - soffset, v[6] - toffset );
				qglVertex3fv (v);
			}
			qglEnd ();
		}
	}
}
#endif

/*
** R_BlendLightMaps
**
** This routine takes all the given light mapped surfaces in the world and
** blends them into the framebuffer.
*/
extern cvar_t	*gl_overbright;
void R_BlendLightmaps (void)
{
	int			i;
	msurface_t	*surf, *newdrawsurf = 0;

	// don't bother if we're set to fullbright
	if (r_fullbright->value)
		return;
	if (!r_worldmodel->lightdata)
		return;

	if(fogenabled)
	{
		vec3_t v;
		VectorSet(v, 0.0f, 0.0f, 0.0f);
		qglFogfv(GL_FOG_COLOR, v); // jitodo
		qglEnable(GL_FOG);
	}

	// don't bother writing Z
	qglDepthMask( 0 );

	/*
	** set the appropriate blending mode unless we're only looking at the
	** lightmaps.
	*/

	if (!gl_lightmap->value)
	{
		GLSTATE_ENABLE_BLEND

		if (gl_monolightmap->string[0] != '0')
		{
			switch (toupper(gl_monolightmap->string[0]))
			{
			case 'I':
			case 'L':				
				if(gl_overbright->value)
					qglBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);// jitbright
				else
					qglBlendFunc(GL_ZERO, GL_SRC_COLOR);
				break;
			case 'A':
			default:
				qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			}
		}
		else
		{
			if(gl_overbright->value)
				qglBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);// jitbright
			else
				qglBlendFunc(GL_ZERO, GL_SRC_COLOR );
		}
	}

	if (currentmodel == r_worldmodel)
		c_visible_lightmaps = 0;

	/*
	** render static lightmaps first
	*/
	for (i = 1; i < MAX_LIGHTMAPS; i++)
	{
		if ( gl_lms.lightmap_surfaces[i] )
		{
			if (currentmodel == r_worldmodel)
				c_visible_lightmaps++;
			GL_Bind( gl_state.lightmap_textures + i);

			for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
			{
				if ( surf->polys )
					DrawGLPolyChain( surf->polys, 0, 0 );
			}
		}
	}

	/*
	** render dynamic lightmaps
	*/
	if ( gl_dynamic->value )
	{
		LM_InitBlock();

		GL_Bind( gl_state.lightmap_textures+0 );

		if (currentmodel == r_worldmodel)
			c_visible_lightmaps++;

		newdrawsurf = gl_lms.lightmap_surfaces[0];

		for ( surf = gl_lms.lightmap_surfaces[0]; surf != 0; surf = surf->lightmapchain )
		{
			int		smax, tmax;
			byte	*base;

			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			if ( LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
			{
				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
			else
			{
				msurface_t *drawsurf;

				// upload what we have so far
				LM_UploadBlock( true );

				// draw all surfaces that use this lightmap
				for ( drawsurf = newdrawsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain )
				{
					if ( drawsurf->polys )
						DrawGLPolyChain( drawsurf->polys, 
							              ( drawsurf->light_s - drawsurf->dlight_s ) * 0.0078125, // ( 1.0 / 128.0 ), 
										( drawsurf->light_t - drawsurf->dlight_t ) * 0.0078125); // ( 1.0 / 128.0 ) );
				}

				newdrawsurf = drawsurf;

				// clear the block
				LM_InitBlock();

				// try uploading the block now
				if ( !LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
				{
					ri.Sys_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed (dynamic)\n", smax, tmax );
				}

				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
		}

		/*
		** draw remainder of dynamic lightmaps that haven't been uploaded yet
		*/
		if ( newdrawsurf )
			LM_UploadBlock( true );

		for ( surf = newdrawsurf; surf != 0; surf = surf->lightmapchain )
		{
			if ( surf->polys )
				DrawGLPolyChain( surf->polys, ( surf->light_s - surf->dlight_s ) * 0.0078125 /*( 1.0 / 128.0 )*/ , ( surf->light_t - surf->dlight_t ) * 0.0078125); // ( 1.0 / 128.0 ) );
		}
	}

	/*
	** restore state
	*/
	GLSTATE_DISABLE_BLEND
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask( 1 );
}

void R_AddFog (void) // jitfog -- for when multitexture is disabled.
{
	int			i;
	image_t		*image;
	msurface_t	*s;
	extern vec3_t fogcolor;

	// don't bother writing Z
	qglDepthMask(0);

	qglFogfv(GL_FOG_COLOR, fogcolor);
	//qglEnable(GL_FOG);

	GL_TexEnv(GL_REPLACE); // jitest
	qglDisable(GL_TEXTURE_2D);
	GLSTATE_ENABLE_BLEND;

	qglColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	qglBlendFunc(GL_ONE, GL_ONE);


	for (i=0,image=gltextures; i<numgltextures; i++,image++) // jitodo, render fog on brush ents
	{
		if (!image->registration_sequence)
			continue;
		s = image->texturechain;
		if (!s)
			continue;

		for (; s; s=s->texturechain)
			DrawGLPoly(s->polys);

		image->texturechain = NULL;
	}

	/*
	** restore state
	*/
	qglEnable(GL_TEXTURE_2D);
	//qglDisable(GL_FOG);
	GLSTATE_DISABLE_BLEND
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask(1);
}


void DrawLightmaps (void) // jitfog -- lightmaps need to be drawn before textures
{
	int			i;
	msurface_t	*surf, *newdrawsurf = 0;

	if (currentmodel == r_worldmodel)
		c_visible_lightmaps = 0;

	/*
	** render static lightmaps first
	*/
	for(i = 1; i < MAX_LIGHTMAPS; i++)
	{
		if(gl_lms.lightmap_surfaces[i])
		{
			if(currentmodel == r_worldmodel)
				c_visible_lightmaps++;

			GL_Bind(gl_state.lightmap_textures + i);

			for(surf=gl_lms.lightmap_surfaces[i]; surf!=0; surf=surf->lightmapchain)
				if(surf->polys)
					DrawGLPolyChain(surf->polys, 0, 0);
		}
	}

	/*
	** render dynamic lightmaps
	*/
	if (gl_dynamic->value)
	{
		LM_InitBlock();

		GL_Bind(gl_state.lightmap_textures + 0);

		if (currentmodel == r_worldmodel)
			c_visible_lightmaps++;

		newdrawsurf = gl_lms.lightmap_surfaces[0];

		for(surf=gl_lms.lightmap_surfaces[0]; surf; surf=surf->lightmapchain)
		{
			int		smax, tmax;
			byte	*base;

			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			if ( LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
			{
				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
			else
			{
				msurface_t *drawsurf;

				// upload what we have so far
				LM_UploadBlock( true );

				// draw all surfaces that use this lightmap
				for ( drawsurf = newdrawsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain )
				{
					if ( drawsurf->polys )
						DrawGLPolyChain( drawsurf->polys, 
							              ( drawsurf->light_s - drawsurf->dlight_s ) * 0.0078125, // ( 1.0 / 128.0 ), 
										( drawsurf->light_t - drawsurf->dlight_t ) * 0.0078125); // ( 1.0 / 128.0 ) );
				}

				newdrawsurf = drawsurf;

				// clear the block
				LM_InitBlock();

				// try uploading the block now
				if ( !LM_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
				{
					ri.Sys_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed (dynamic)\n", smax, tmax );
				}

				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
		}

		/*
		** draw remainder of dynamic lightmaps that haven't been uploaded yet
		*/
		if ( newdrawsurf )
			LM_UploadBlock( true );

		for ( surf = newdrawsurf; surf != 0; surf = surf->lightmapchain )
		{
			if ( surf->polys )
				DrawGLPolyChain( surf->polys, ( surf->light_s - surf->dlight_s ) * 0.0078125 /*( 1.0 / 128.0 )*/ , ( surf->light_t - surf->dlight_t ) * 0.0078125); // ( 1.0 / 128.0 ) );
		}
	}
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa)
{
	int			maps;
	image_t		*image;
	qboolean is_dynamic = false;

	c_brush_polys++;

	image = R_TextureAnimation (fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{	
		GL_Bind( image->texnum );

		// warp texture, no lightmaps
		qglColor4f(1.0f, 1.0f, 1.0f, 1.0f); // jit

		EmitWaterPolys (fa);
		return;
	}

//======
//PGM
	if(fa->texinfo->flags & SURF_FLOWING) 
	{
		GL_Bind(image->texnum);
		GL_TexEnv(GL_REPLACE);
		DrawGLFlowingPoly(fa);
	} 
	else 
	{
		if (!fa->texinfo->script) 
		{
			GL_Bind(image->texnum);
			GL_TexEnv(GL_REPLACE);
			DrawGLPoly(fa->polys);
		} 
		else // jitodo -- put if(fa->texinfo->script) at top, everything else in else, so scripts work on water/trans/flowing.  hmm, maybe not flowing...
		{
			GL_TexEnv(GL_REPLACE);
			RS_DrawPolyNoLightMap(fa);
		}
	}
//PGM
//======

	/*
	** check for lightmap modification
	*/
	for (maps=0; maps<MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (r_newrefdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((fa->dlightframe == r_framecount))
	{
dynamic:
		if (gl_dynamic->value)
		{
			if (!(fa->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		if ((fa->styles[maps] >= 32 || fa->styles[maps] == 0) && (fa->dlightframe != r_framecount))
		{
			unsigned	temp[34*34];
			int			smax, tmax;

			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;

			R_BuildLightMap(fa, (void *)temp, smax*4);
			R_SetCacheState(fa);

			GL_Bind(gl_state.lightmap_textures + fa->lightmaptexturenum);

			qglTexSubImage2D( GL_TEXTURE_2D, 0,
							  fa->light_s, fa->light_t, 
							  smax, tmax, 
							  GL_LIGHTMAP_FORMAT, 
							  GL_UNSIGNED_BYTE, temp );

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = gl_lms.lightmap_surfaces[0];
			gl_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}

void R_DrawCaustics (void) // jitcaustics
{
	msurface_t	*s;
	

	if(!r_caustics->value || !r_caustictexture || !r_caustictexture->rscript)
	{
		r_caustic_surfaces = NULL; // prevent infinite loop
		return;
	}

	// don't bother writing Z
	qglDepthMask(0);

	GLSTATE_ENABLE_BLEND
	GL_TexEnv(GL_MODULATE);

	qglEnable(GL_POLYGON_OFFSET_FILL); 
	qglPolygonOffset(-3, -2); 

	alphasurf = true;

	for (s=r_caustic_surfaces; s; s=s->causticchain)
	{
//		GL_Bind(r_caustictexture->texnum);
//		DrawGLPoly(s->polys);
		RS_DrawSurface(s, false, r_caustictexture->rscript);
	}

	alphasurf = false;

	qglDisable(GL_POLYGON_OFFSET_FILL); 

	GL_TexEnv(GL_REPLACE);
	qglColor4f (1,1,1,1);

	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GLSTATE_DISABLE_BLEND

	qglDepthMask(1); // re-enable z writing

	r_caustic_surfaces = NULL;
}

/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces (void)
{
	msurface_t	*s;
	float		intens;

	alphasurf = true; // jitrscript
	//
	// go back to the world matrix
	//
    qglLoadMatrixf (r_world_matrix);

	GLSTATE_ENABLE_BLEND
	GL_TexEnv(GL_MODULATE);

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	intens = 1.0f; // jit -- was gl_state.inverse_intensity;

	for (s=r_alpha_surfaces ; s ; s=s->texturechain)
	{
		GL_Bind(s->texinfo->image->texnum);
		c_brush_polys++;

		if(s->texinfo->script && !(s->flags & SURF_DRAWTURB)) // jitrscript
		{
			GL_TexEnv(GL_REPLACE);
			RS_DrawPolyNoLightMap(s); // jitrscript
			GLSTATE_ENABLE_BLEND
			GL_TexEnv(GL_MODULATE);
		}
		else
		{
			if (s->texinfo->flags & SURF_TRANS33)
			{
				if (s->texinfo->flags & SURF_TRANS66) // jittrans -- trans33+trans66 only uses texture transparency.
					qglColor4f (intens,intens,intens, 1.0f);
				else
					qglColor4f (intens,intens,intens, 0.33f);
			}
			else if (s->texinfo->flags & SURF_TRANS66)
				qglColor4f (intens,intens,intens,0.66);
			else
				qglColor4f (intens,intens,intens,1);

			if (s->flags & SURF_DRAWTURB)
				EmitWaterPolys (s);
			else if(s->texinfo->flags & SURF_FLOWING)			// PGM	9/16/98
				DrawGLFlowingPoly (s);							// PGM
			else
				DrawGLPoly (s->polys);
		}
	}

	GL_TexEnv( GL_REPLACE );
	qglColor4f (1,1,1,1);
	GLSTATE_DISABLE_BLEND

	r_alpha_surfaces = NULL;

	alphasurf = false; // jitrscript
}


/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	image_t		*image;

	c_visible_textures = 0;

//	GL_TexEnv( GL_REPLACE );

	if (!qglSelectTextureSGIS && !qglActiveTextureARB)
	{
		for (i=0,image=gltextures; i<numgltextures; i++,image++)
		{
			if (!image->registration_sequence)
				continue;

			s = image->texturechain;

			if (!s)
				continue;

			c_visible_textures++;

			for ( ; s; s=s->texturechain)
				R_RenderBrushPoly(s);

			if(!fogenabled) // jitfog
				image->texturechain = NULL;
		}
	}
	else
	{
		for (i=0, image=gltextures; i<numgltextures; i++,image++)
		{
			if (!image->registration_sequence)
				continue;

			if (!image->texturechain)
				continue;

			c_visible_textures++;

			for (s=image->texturechain; s; s=s->texturechain)
			{
				if (!(s->flags & SURF_DRAWTURB))
					R_RenderBrushPoly(s);
			}
		}

		GL_EnableMultitexture(false);

		for (i=0, image=gltextures; i<numgltextures; i++,image++)
		{
			if (!image->registration_sequence)
				continue;

			s = image->texturechain;

			if (!s)
				continue;

			for (; s; s=s->texturechain)
			{
				if (s->flags & SURF_DRAWTURB)
					R_RenderBrushPoly(s);
			}

			image->texturechain = NULL;
		}
	}

	GL_TexEnv(GL_REPLACE);
}

static void GL_RenderLightmappedPoly (msurface_t *surf)
{
	int		i, nv = surf->polys->numverts;
	int		map;
	float	*v;
	image_t *image = R_TextureAnimation(surf->texinfo);
	qboolean is_dynamic = false;
	unsigned lmtex = surf->lightmaptexturenum;
	glpoly_t *p;

	for (map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++)
	{
		if (r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((surf->dlightframe == r_framecount))
	{
dynamic:
		if (gl_dynamic->value)
		{
			if (!(surf->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		unsigned	temp[128*128];
		int			smax, tmax;

		if ((surf->styles[map] >= 32 || surf->styles[map] == 0) && (surf->dlightframe != r_framecount))
		{
			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			R_BuildLightMap(surf, (void*)temp, smax*4);
			R_SetCacheState(surf);

			GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + surf->lightmaptexturenum);

			lmtex = surf->lightmaptexturenum;

			qglTexSubImage2D(GL_TEXTURE_2D, 0,
							 surf->light_s, surf->light_t, 
							 smax, tmax, 
							 GL_LIGHTMAP_FORMAT, 
							 GL_UNSIGNED_BYTE, temp);

		}
		else
		{
			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;

			R_BuildLightMap(surf, (void*)temp, smax*4);

			GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + 0);

			lmtex = 0;

			qglTexSubImage2D(GL_TEXTURE_2D, 0,
							 surf->light_s, surf->light_t, 
							 smax, tmax, 
							 GL_LIGHTMAP_FORMAT, 
							 GL_UNSIGNED_BYTE, temp);

		}

		c_brush_polys++;

		GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + lmtex);

//==========
//PGM
		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;
			GL_MBind(GL_TEXTURE0, image->texnum);
	
			scroll = -64 * ((r_newrefdef.time*0.025 /* / 40.0*/) - (int)(r_newrefdef.time*0.025 /* / 40.0 */));
			if(scroll == 0.0)
				scroll = -64.0;

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				qglBegin(GL_POLYGON);
				for (i=0; i< nv; i++, v+= VERTEXSIZE)
				{
					qglMTexCoord2fSGIS(GL_TEXTURE0, (v[3]+scroll), v[4]);
					qglMTexCoord2fSGIS(GL_TEXTURE1, v[5], v[6]);
					qglVertex3fv(v);
				}
				qglEnd();
			}
		}
		else
		{
			if (surf->texinfo->script)
			{
				RS_DrawPoly(surf);
			}
			else
			{
				GL_MBind(GL_TEXTURE0, image->texnum);

				for (p = surf->polys; p; p = p->chain)
				{
					v = p->verts[0];
					qglBegin(GL_POLYGON);
					for (i=0; i<nv; i++, v+=VERTEXSIZE)
					{
						qglMTexCoord2fSGIS(GL_TEXTURE0, v[3], v[4]);
						qglMTexCoord2fSGIS(GL_TEXTURE1, v[5], v[6]);
						qglVertex3fv(v);
					}
					qglEnd();
				}
			}
		}
//PGM
//==========
	}
	else
	{
		c_brush_polys++;
		GL_MBind(GL_TEXTURE1, gl_state.lightmap_textures + lmtex);
//==========
//PGM
		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;

			GL_MBind(GL_TEXTURE0, image->texnum);
		
			scroll = -64 * ((r_newrefdef.time*0.025  /* / 40.0 */) - (int)(r_newrefdef.time*0.025 /* / 40.0 */));

			if(scroll == 0.0)
				scroll = -64.0;

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				qglBegin(GL_POLYGON);
				for (i=0; i<nv; i++, v+=VERTEXSIZE)
				{
					qglMTexCoord2fSGIS(GL_TEXTURE0, (v[3]+scroll), v[4]);
					qglMTexCoord2fSGIS(GL_TEXTURE1, v[5], v[6]);
					qglVertex3fv (v);
				}
				qglEnd();
			}
		}
		else
		{
//PGM
//==========
			if (surf->texinfo->script)
			{
				RS_DrawPoly(surf);
			}
			else
			{
				GL_MBind(GL_TEXTURE0, image->texnum);
#ifdef COLORNODES
				GL_TexEnv(GL_COMBINE_EXT); // jitest
#endif

				for (p = surf->polys; p; p = p->chain)
				{
					v = p->verts[0];
					qglBegin(GL_POLYGON);
					for (i=0 ; i<nv; i++, v+=VERTEXSIZE)
					{
						qglMTexCoord2fSGIS(GL_TEXTURE0, v[3], v[4]);
						qglMTexCoord2fSGIS(GL_TEXTURE1, v[5], v[6]);
						qglVertex3fv(v);
					}
					qglEnd();
				}
			}
//==========
//PGM
		}
//PGM
//==========
	}
}

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel (void)
{
	int			i, k;
	cplane_t	*pplane;
	float		dot;
	msurface_t	*psurf;
	dlight_t	*lt;

	// calculate dynamic lighting for bmodel
	if ( !gl_flashblend->value )
	{
		lt = r_newrefdef.dlights;
		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
		{
			R_MarkLights (lt, 1<<k, currentmodel->nodes + currentmodel->firstnode);
		}
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		GLSTATE_ENABLE_BLEND
		qglColor4f (1,1,1,0.25);
		GL_TexEnv( GL_MODULATE );
	}

	//
	// draw texture
	//
	for (i=0 ; i<currentmodel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->flags & SURF_UNDERWATER) // jitcaustics
			{
				psurf->causticchain = r_caustic_surfaces;
				r_caustic_surfaces = psurf;
			}

			if (psurf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66) )
			{	// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if (qglMTexCoord2fSGIS && !(psurf->flags & SURF_DRAWTURB))
			{
				GL_RenderLightmappedPoly(psurf);
			}
			else if (qglMTexCoord2fSGIS) // jitfog
			{
				GL_EnableMultitexture(false);
				R_RenderBrushPoly(psurf);
				GL_EnableMultitexture(true);
			}
			else // jitfog
			{
				// create chain for fog to render in
				// multipass mode.
				// this is a pretty ugly hack.
				if(fogenabled)
				{
					psurf->texturechain = gltextures->texturechain;
					gltextures->texturechain = psurf;
					qglDisable(GL_FOG);
				}
				R_RenderBrushPoly(psurf);
			}
		}
	}

	if (!(currententity->flags & RF_TRANSLUCENT))
	{
		if (!qglMTexCoord2fSGIS)
		{
			R_BlendLightmaps(); // jitodo, fog -- test doors
			if(fogenabled) // jitfog
				R_AddFog(); // jitfog
		}
	}
	else
	{
		GLSTATE_DISABLE_BLEND
		qglColor4f (1,1,1,1);
		GL_TexEnv( GL_REPLACE );
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;
	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, currentmodel->mins, mins);
		VectorAdd (e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

	qglColor3f (1,1,1);
	memset (gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	VectorSubtract (r_newrefdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

    qglPushMatrix ();
e->angles[0] = -e->angles[0];	// stupid quake bug
e->angles[2] = -e->angles[2];	// stupid quake bug
	R_RotateForEntity (e);
e->angles[0] = -e->angles[0];	// stupid quake bug
e->angles[2] = -e->angles[2];	// stupid quake bug

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE0);
	GL_TexEnv( GL_REPLACE );
	GL_SelectTexture( GL_TEXTURE1);
//	GL_TexEnv( GL_MODULATE );
	GL_TexEnv( GL_COMBINE_EXT ); // jitbright

	R_DrawInlineBModel ();
	GL_EnableMultitexture( false );

	qglPopMatrix ();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;
	image_t		*image;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (R_CullBox(node->minmaxs, node->minmaxs+3))
		return;
	
// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (!(r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7))))
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct(modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);

	// draw stuff
	for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{	// just adds to visible sky bounds
			R_AddSkySurface (surf);
		}
		else if (surf->texinfo->flags & SURF_NODRAW)
		{
			continue;
		}
		else
		{
			if (surf->flags & SURF_UNDERWATER) // jitcaustics
			{
				surf->causticchain = r_caustic_surfaces;
				r_caustic_surfaces = surf;
			}

			if (surf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66))
			{	// add to the translucent chain
				surf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = surf;
			}
			else
			{
				// == jitest
#ifdef COLORNODES
				//static char r=0,g=128,b=200;
				char r,g,b;
				union {
					mnode_t *ptr;
					unsigned char bytes[4];
				} nodeptr;
				nodeptr.ptr = node;
				//qglColor3ub(r++, g+=200, b+=30); // jitest
				//qglColor3ubv(nodeptr.bytes); // jitest
				r=nodeptr.bytes[0]<<2;
				g=nodeptr.bytes[1]+nodeptr.bytes[0];
				b=nodeptr.bytes[2]+nodeptr.bytes[3];
				qglColor3ub(r,g,b);
#endif
				// jitest == *note, be sure to remove gl_combine_ext
				if (qglMTexCoord2fSGIS && !(surf->flags & SURF_DRAWTURB))
				{
					GL_RenderLightmappedPoly(surf);
				}
				else
				{
					// the polygon is visible, so add it to the texture
					// sorted chain
					// FIXME: this is a hack for animation
					image = R_TextureAnimation(surf->texinfo);
					surf->texturechain = image->texturechain;
					image->texturechain = surf;
				}
			}

			if (gl_showtris->value && qglMTexCoord2fSGIS) // jit / GuyP
				R_DrawTriangleOutlines(surf);    // Guy: gl_showtris fix
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode(node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	currentmodel = r_worldmodel;

	VectorCopy(r_newrefdef.vieworg, modelorg);

	// auto cycle the world frame for texture animation
	memset(&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time*2);
	currententity = &ent;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	qglColor3f(1,1,1);
	memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));
	R_ClearSkyBox();

	if (qglMTexCoord2fSGIS)
	{
		GL_EnableMultitexture(true);
		GL_SelectTexture(GL_TEXTURE0);
		GL_TexEnv(GL_REPLACE);
		GL_SelectTexture(GL_TEXTURE1);

		if (gl_lightmap->value)
			GL_TexEnv(GL_REPLACE);
		else 
			GL_TexEnv(GL_COMBINE_EXT); // jitbright

		R_RecursiveWorldNode(r_worldmodel->nodes);
		DrawTextureChains();
	}
	else // no multitexture
	{
		R_RecursiveWorldNode(r_worldmodel->nodes);
		
		if(fogenabled)
			qglDisable(GL_FOG);
		
		//DrawLightmaps(); // jitfog / jitodo
		DrawTextureChains(); // jitodo

		R_BlendLightmaps(); // jitodo, remove jitfog
		if(fogenabled) // jitfog
			R_AddFog(); // jitfog
	}

	R_DrawSkyBox();

	if (gl_showtris->value && !qglMTexCoord2fSGIS)
		R_DrawTriangleOutlines(NULL);
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	byte	fatvis[MAX_MAP_LEAFS/8];
	mnode_t	*node;
	int		i, c;
	mleaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (gl_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy (fatvis, vis, (size_t)((r_worldmodel->numleafs+7.0)*0.125)); // jit, kill warning
		vis = Mod_ClusterPVS (r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs+31)*0.03125;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}
	
	for (i=0,leaf=r_worldmodel->leafs ; i<r_worldmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}

#if 0
	for (i=0 ; i<r_worldmodel->vis->numclusters ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&r_worldmodel->leafs[i];	// FIXME: cluster
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
#endif
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

_inline static void LM_InitBlock( void )
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

static void LM_UploadBlock( qboolean dynamic )
{
	int texture;
	int height = 0;

	if ( dynamic )
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	GL_Bind( gl_state.lightmap_textures + texture );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ( dynamic )
	{
		int i;

		for ( i = 0; i < BLOCK_WIDTH; i++ )
		{
			if ( gl_lms.allocated[i] > height )
				height = gl_lms.allocated[i];
		}

		qglTexSubImage2D( GL_TEXTURE_2D, 
						  0,
						  0, 0,
						  BLOCK_WIDTH, height,
						  GL_LIGHTMAP_FORMAT,
						  GL_UNSIGNED_BYTE,
						  gl_lms.lightmap_buffer );
	}
	else
	{
		qglTexImage2D( GL_TEXTURE_2D, 
					   0, 
					   gl_lms.internal_format,
					   BLOCK_WIDTH, BLOCK_HEIGHT, 
					   0, 
					   GL_LIGHTMAP_FORMAT, 
					   GL_UNSIGNED_BYTE, 
					   gl_lms.lightmap_buffer );
		if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
			ri.Sys_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
	}
}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for (i=0 ; i<BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

/*
================
GL_BuildPolygonFromSurface
================
*/
void GL_BuildPolygonFromSurface(msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	int			vertpage;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;
	vec3_t		total;

// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	VectorClear (total);
	//
	// draw texture
	//
	poly = Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorAdd (total, vec, total);
		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;

	VectorScale (total, 1.0f/(float)lnumverts, total);

	fa->c_s = (DotProduct (total, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3])
				/ fa->texinfo->image->width;
	fa->c_t = (DotProduct (total, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3])
				/ fa->texinfo->image->height;
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock( false );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			ri.Sys_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}


/*
==================
GL_BeginBuildingLightmaps

==================
*/
void GL_BeginBuildingLightmaps (model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;
	unsigned		dummy[128*128];

	memset( gl_lms.allocated, 0, sizeof(gl_lms.allocated) );

	r_framecount = 1;		// no dlightcache

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE1);

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
	{
		gl_state.lightmap_textures	= TEXNUM_LIGHTMAPS;
//		gl_state.lightmap_textures	= gl_state.texture_extension_number;
//		gl_state.texture_extension_number = gl_state.lightmap_textures + MAX_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** if mono lightmaps are enabled and we want to use alpha
	** blending (a,1-a) then we're likely running on a 3DLabs
	** Permedia2.  In a perfect world we'd use a GL_ALPHA lightmap
	** in order to conserve space and maximize bandwidth, however 
	** this isn't a perfect world.
	**
	** So we have to use alpha lightmaps, but stored in GL_RGBA format,
	** which means we only get 1/16th the color resolution we should when
	** using alpha lightmaps.  If we find another board that supports
	** only alpha lightmaps but that can at least support the GL_ALPHA
	** format then we should change this code to use real alpha maps.
	*/
	if ( toupper( gl_monolightmap->string[0] ) == 'A' )
	{
		gl_lms.internal_format = gl_tex_alpha_format;
	}
	/*
	** try to do hacked colored lighting with a blended texture
	*/
	else if ( toupper( gl_monolightmap->string[0] ) == 'C' )
	{
		gl_lms.internal_format = gl_tex_alpha_format;
	}
	else if ( toupper( gl_monolightmap->string[0] ) == 'I' )
	{
		gl_lms.internal_format = GL_INTENSITY8;
	}
	else if ( toupper( gl_monolightmap->string[0] ) == 'L' ) 
	{
		gl_lms.internal_format = GL_LUMINANCE8;
	}
	else
	{
		gl_lms.internal_format = gl_tex_solid_format;
	}

	/*
	** initialize the dynamic lightmap texture
	*/
	GL_Bind( gl_state.lightmap_textures + 0 );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D( GL_TEXTURE_2D, 
				   0, 
				   gl_lms.internal_format,
				   BLOCK_WIDTH, BLOCK_HEIGHT, 
				   0, 
				   GL_LIGHTMAP_FORMAT, 
				   GL_UNSIGNED_BYTE, 
				   dummy );
}

/*
=======================
GL_EndBuildingLightmaps
=======================
*/
void GL_EndBuildingLightmaps (void)
{
	LM_UploadBlock( false );
	GL_EnableMultitexture( false );
}

