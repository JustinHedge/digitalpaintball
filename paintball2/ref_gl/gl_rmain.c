/*
Copyright(C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_main.c
#include "gl_local.h"
#include "gl_refl.h" // jitwater

#include "glide.h" // jit3dfx
#ifndef WIN32
#include <ctype.h>
char *strlwr(char*); // defined in q_shlinux.c
#endif

//****************************************************************************
// nVidia extensions
//****************************************************************************
PFNGLCOMBINERPARAMETERFVNVPROC					qglCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERFNVPROC					qglCombinerParameterfNV;
PFNGLCOMBINERPARAMETERIVNVPROC					qglCombinerParameterivNV;
PFNGLCOMBINERPARAMETERINVPROC					qglCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC						qglCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC						qglCombinerOutputNV;
PFNGLFINALCOMBINERINPUTNVPROC					qglFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC			qglGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC			qglGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC			qglGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC			qglGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC		qglGetFinalCombinerInputParameterfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC		qglGetFinalCombinerInputParameterivNV;
//****************************************************************************
// === jitwater -- fragment program extensions
PFNGLGENPROGRAMSARBPROC             qglGenProgramsARB            = NULL;
PFNGLDELETEPROGRAMSARBPROC          qglDeleteProgramsARB         = NULL;
PFNGLBINDPROGRAMARBPROC             qglBindProgramARB            = NULL;
PFNGLPROGRAMSTRINGARBPROC           qglProgramStringARB          = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC   qglProgramEnvParameter4fARB  = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB = NULL;
// jitwater ===
//****************************************************************************

cvar_t	*gl_debug; // jit
cvar_t	*gl_sgis_generate_mipmap;
cvar_t	*gl_arb_fragment_program; // jit

void R_Clear (void);

viddef_t	vid;

refimport_t	ri;

int QGL_TEXTURE0, QGL_TEXTURE1, QGL_TEXTURE2;

model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

glconfig_t gl_config;
glstate_t  gl_state;

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles
image_t		*r_whitetexture;	// jitfog
image_t		*r_caustictexture = NULL;	// jitcaustics

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

float		v_blend[4];			// final blending color

// <!-- jitfog
void	Draw_String(int x, int y, const char *str);
vec3_t fogcolor = { 0.408, 0.447, 0.584 };
float fogdensity = 0.008;
float fogdistance = 512;
qboolean fogenabled = false; // doesn't work right with overbright :(
// jit -->

void GL_Strings_f(void);

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*gl_nosubimage;
cvar_t	*gl_allow_software;

cvar_t	*gl_vertex_arrays;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;
cvar_t	*gl_ext_texture_compression; // Heffo - ARB Texture Compression

cvar_t	*gl_screenshot_jpeg;			// Heffo - JPEG Screenshots
cvar_t	*gl_screenshot_jpeg_quality;	// Heffo - JPEG Screenshots

cvar_t	*gl_stainmaps;				// stainmaps
cvar_t	*gl_motionblur;				// motionblur

cvar_t	*gl_log;
cvar_t	*gl_bitdepth;
cvar_t	*gl_drawbuffer;
cvar_t  *gl_driver;
cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t  *gl_monolightmap;
cvar_t	*gl_anisotropy; // jitanisotropy
cvar_t	*gl_texture_saturation; // jitsaturation
cvar_t	*gl_highres_textures; // jithighres
cvar_t	*gl_lightmap_saturation; // jitsaturation
cvar_t	*gl_overbright; // jitbright
cvar_t	*gl_textshadow; // jittext
cvar_t	*gl_brightness; // jit
cvar_t	*gl_autobrightness; // jit
cvar_t	*gl_showbbox; // jit / Guy
cvar_t	*gl_hash_textures; // jithash
cvar_t	*gl_free_unused_textures; // jitfreeunused
//cvar_t	*gl_modulate;
cvar_t	*gl_lightmapgamma; // jitgamma
cvar_t	*cl_hudscale; // jithudscale
cvar_t	*gl_nobind;
cvar_t	*gl_round_down;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_ztrick;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t	*gl_playermip;
cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;

cvar_t	*gl_3dlabs_broken;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_lighten; // jitgamma
cvar_t	*vid_ref;

cvar_t	*cl_animdump;
cvar_t	*vid_gamma_hw;

cvar_t	*r_caustics; // jitcaustics
cvar_t	*r_reflectivewater; // jitwater
cvar_t	*r_reflectivewater_debug; // jitwater
cvar_t	*r_reflectivewater_max; // jitwater

cvar_t	*r_oldmodels; // jit

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
#if 0
qboolean R_CullBox(const vec3_t mins, const vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if (BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}
#else // jitopt: taken from darkplaces (doesn't seem to make much of a difference)
qboolean R_CullBox(const vec3_t mins, const vec3_t maxs)
{
	int i;
//	mplane_t *p;
	cplane_t *p;
	for (i = 0; i < 4; i++)
	{
		p = frustum + i;
		switch(p->signbits)
		{
		default:
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		}
	}
	return false;
}
#endif

void R_RotateForEntity (entity_t *e)
{
	register float scalebleh;
    
	qglTranslatef(e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef(e->angles[1],  0, 0, 1);
    qglRotatef(-e->angles[0],  0, 1, 0);
    qglRotatef(-e->angles[2],  1, 0, 0);

	//jit:
	scalebleh = e->scale;

	if (scalebleh)
		qglScalef(scalebleh, scalebleh, scalebleh);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/


/*
=================
R_DrawSpriteModel -- jitodo -- sort these!@

=================
*/
void new_R_DrawSpriteModel(entity_t *ent)
{
	float alpha;
	float scale;
	
	alpha = ent->alpha;
	scale = ent->scale;

	//qglScalef(scale,scale,scale);
	qglColor4f(1,1,1,alpha);

	//GLSTATE_ENABLE_BLEND

	GL_Bind(currentmodel->skins[ent->frame]->texnum);
	GL_TexEnv(GL_MODULATE);

	qglPushMatrix();
		qglLoadIdentity();
		//qglScalef(/*frame->width*/64*ent->scale,/*frame->height*/64*ent->scale,1);
		qglTranslatef(ent->origin[0],ent->origin[1],ent->origin[2]);
		qglBegin(GL_QUADS);
			qglTexCoord2f(0, 1);
			qglVertex3f(-64, 0,64);
			qglTexCoord2f(0, 0);
			qglVertex3f(-64, 0,-64);
			qglTexCoord2f(1, 0);
			qglVertex3f(64, 0,-64);
			qglTexCoord2f(1, 1);
			qglVertex3f(64, 0,64);
		qglEnd();
		
	qglPopMatrix();
	GL_TexEnv(GL_REPLACE);
	GLSTATE_DISABLE_BLEND
	qglColor4f(1, 1, 1, 1);
}

void R_DrawSpriteModel(entity_t *e)
{
	vec3_t		point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t	*psprite;

	psprite =(dsprite_t *)currentmodel->extradata;
	e->frame %= psprite->numframes;
	frame = &psprite->frames[e->frame];
	up = vup;
	right = vright;
	GLSTATE_ENABLE_BLEND;
	qglColor4f(1, 1, 1, e->alpha);

    GL_Bind(currentmodel->skins[e->frame]->texnum);

	GL_TexEnv(GL_MODULATE);

	qglBegin(GL_QUADS);

		qglTexCoord2f(0, 1);
		VectorMA(e->origin, -e->scale*frame->origin_y, up, point);
		VectorMA(point, -e->scale*frame->origin_x, right, point);
		qglVertex3fv(point);

		qglTexCoord2f(0, 0);
		VectorMA(e->origin, e->scale*frame->height - frame->origin_y, up, point);
		VectorMA(point, -e->scale*frame->origin_x, right, point);
		qglVertex3fv(point);

		qglTexCoord2f(1, 0);
		VectorMA(e->origin, e->scale*frame->height - frame->origin_y, up, point);
		VectorMA(point, e->scale*frame->width - frame->origin_x, right, point);
		qglVertex3fv(point);

		qglTexCoord2f(1, 1);
		VectorMA(e->origin, -e->scale*frame->origin_y, up, point);
		VectorMA(point, e->scale*frame->width - frame->origin_x, right, point);
		qglVertex3fv(point);
	
	qglEnd();
}

void old_R_DrawSpriteModel(entity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite =(dsprite_t *)currentmodel->extradata;

/*#if 0
	if (e->frame < 0 || e->frame >= psprite->numframes)
	{
		ri.Con_Printf(PRINT_ALL, "no such sprite frame %i\n", e->frame);
		e->frame = 0;
	}
#endif*/
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

	// jitodo -- oriented sprites for splats
/*#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
	vec3_t		v_forward, v_right, v_up;

	AngleVectors(currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif*/
	{	// normal sprite
		up = vup;
		right = vright;
	}

	if (e->flags & RF_TRANSLUCENT)
		alpha = e->alpha;

/*	if (alpha != 1.0F) - jit, smoke has alpha built in */
		GLSTATE_ENABLE_BLEND

	qglColor4f(1, 1, 1, alpha);

    GL_Bind(currentmodel->skins[e->frame]->texnum);

	GL_TexEnv(GL_MODULATE);

	/*if(alpha == 1.0)-- jit, smoke sprites have built in alpha
		GLSTATE_ENABLE_ALPHATEST
	else*/
	GLSTATE_DISABLE_ALPHATEST

	qglBegin(GL_QUADS);

	qglTexCoord2f(0, 1);
	VectorMA(e->origin, -frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(0, 0);
	VectorMA(e->origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(1, 0);
	VectorMA(e->origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(1, 1);
	VectorMA(e->origin, -frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	qglVertex3fv(point);
	
	qglEnd();

	//GLSTATE_DISABLE_ALPHATEST
	GL_TexEnv(GL_REPLACE);

	if (alpha != 1.0F) {
		GLSTATE_DISABLE_BLEND
	}

	qglColor4f(1, 1, 1, 1);
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel(void)
{
	vec3_t	shadelight;
	int		i;

	if (currententity->flags & RF_FULLBRIGHT)
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint(currententity->origin, shadelight);

    qglPushMatrix();
	R_RotateForEntity(currententity);

	qglDisable(GL_TEXTURE_2D);
	qglColor3fv(shadelight);

	qglBegin(GL_TRIANGLE_FAN);
	qglVertex3f(0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f(16*cos(i*M_PI*0.5f), 16*sin(i*M_PI*0.5f), 0);
	qglEnd();

	qglBegin(GL_TRIANGLE_FAN);
	qglVertex3f(0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f(16*cos(i*M_PI*0.5f), 16*sin(i*M_PI*0.5f), 0);
	qglEnd();

	qglColor3f(1,1,1);
	qglPopMatrix();
	qglEnable(GL_TEXTURE_2D);
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// not solid

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			currentmodel = currententity->model;
			
			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}

			switch(currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_skeletal: // jitskm
				R_DrawSkeletalModel(currententity);
				break;
			case mod_brush:
				R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	qglDepthMask(0);		// no z writes

	for (i=0; i<r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}

			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_skeletal: // jitskm
				R_DrawSkeletalModel(currententity);
				break;
			case mod_brush:
				R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				//R_DrawSpriteModel(currententity); jit, draw sprites later
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	qglDepthMask(1);		// back to writing
}

void R_DrawSpritesOnList(void) // jit, draw sprites after water
{
	int i;

	qglDepthMask(0);		// no z writes

	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];
		currentmodel = currententity->model;

		if (currentmodel && mod_sprite == currentmodel->type) // jitodo -- why is currentmodel sometimes null???
			R_DrawSpriteModel(currententity);
	}

	qglDepthMask(1);		// back to writing
}

/*
** GL_DrawParticles
**
*/
void GL_DrawParticles(int num_particles, const particle_t particles[], const unsigned colortable[768])
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

    GL_Bind(r_particletexture->texnum);
	qglDepthMask(GL_FALSE);		// no z buffering
	GLSTATE_ENABLE_BLEND
	GL_TexEnv(GL_MODULATE);
	qglBegin(GL_TRIANGLES);

	VectorScale(vup, 1.5, up);
	VectorScale(vright, 1.5, right);

	for (p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		scale =(p->origin[0] - r_origin[0]) * vpn[0] + 
			   (p->origin[1] - r_origin[1]) * vpn[1] +
			   (p->origin[2] - r_origin[2]) * vpn[2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;

		*(int *)color = colortable[p->color];
		color[3] = p->alpha*255;

		qglColor4ubv(color);

		qglTexCoord2f(0.0625, 0.0625);
		qglVertex3fv(p->origin);

		qglTexCoord2f(1.0625, 0.0625);
		qglVertex3f(p->origin[0] + up[0]*scale, 
			         p->origin[1] + up[1]*scale, 
					 p->origin[2] + up[2]*scale);

		qglTexCoord2f(0.0625, 1.0625);
		qglVertex3f(p->origin[0] + right[0]*scale, 
			         p->origin[1] + right[1]*scale, 
					 p->origin[2] + right[2]*scale);
	}

	qglEnd();
	GLSTATE_DISABLE_BLEND
	qglColor4f(1,1,1,1);
	qglDepthMask(1);		// back to normal Z buffering
	GL_TexEnv(GL_REPLACE);
}
/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles(void)
{
	if (gl_ext_pointparameters->value && qglPointParameterfEXT)
	{
		int i;
		unsigned char color[4];
		const particle_t *p;

		qglDepthMask(GL_FALSE);
		GLSTATE_ENABLE_BLEND
		qglDisable(GL_TEXTURE_2D);
		qglPointSize(gl_particle_size->value);
		qglBegin(GL_POINTS);

		for (i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++)
		{
			*(int *)color = d_8to24table[p->color];
			color[3] = p->alpha*255;
			qglColor4ubv(color);
			qglVertex3fv(p->origin);
		}

		qglEnd();
		GLSTATE_DISABLE_BLEND
		qglColor4f(1.0F, 1.0F, 1.0F, 1.0F);
		qglDepthMask(GL_TRUE);
		qglEnable(GL_TEXTURE_2D);
	}
	else
	{
		GL_DrawParticles(r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table);
	}
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	// === 
	// jit
	static float autobright=0;
	vec3_t shadelight;
	float shadeavg;
	float b;

	if (gl_brightness->value || v_blend[3])
	{
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_ENABLE_BLEND
		qglDisable(GL_DEPTH_TEST);
		qglDisable(GL_TEXTURE_2D);
		qglLoadIdentity();

		// FIXME: get rid of these
		qglRotatef(-90,  1, 0, 0);	    // put Z going up
		qglRotatef(90,  0, 0, 1);	    // put Z going up
	}

	if (gl_brightness->value)
	{
		if (gl_autobrightness->value > 1.0 || gl_autobrightness->value < 0.0)
			ri.Cvar_SetValue("gl_autobrightness", 1);

		R_LightPoint(r_newrefdef.vieworg, shadelight);
		shadeavg = (shadelight[0]+shadelight[1]+shadelight[2]);
		shadeavg /= 4.5;
		autobright=0.985*autobright + 0.015*(0.5-sqrt(shadeavg)/2.0);
		qglBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
		b = gl_brightness->value *(1.0-gl_autobrightness->value +
			gl_autobrightness->value*autobright)/2.0+0.5;
		qglColor4f(b,b,b,1);

		qglBegin(GL_QUADS);
			qglVertex3f(10, 100, 100);
			qglVertex3f(10, -100, 100);
			qglVertex3f(10, -100, -100);
			qglVertex3f(10, 100, -100);
		qglEnd();

		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
// jit
// ===

#ifdef SHADOW_VOLUMES
	qglColor4f(0,0,0,0.3);
	qglStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFFL);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	qglEnable(GL_STENCIL_TEST);

	qglBegin(GL_QUADS);
	qglVertex3f(10, 100, 100);
	qglVertex3f(10, -100, 100);
	qglVertex3f(10, -100, -100);
	qglVertex3f(10, 100, -100);
	qglEnd();

	qglDisable(GL_STENCIL_TEST);
#endif

	if (v_blend[3])
	{
		qglColor4fv(v_blend);
		qglBegin(GL_QUADS);
		qglVertex3f(10, 100, 100);
		qglVertex3f(10, -100, 100);
		qglVertex3f(10, -100, -100);
		qglVertex3f(10, 100, -100);
		qglEnd();
	}

	GLSTATE_DISABLE_BLEND
	qglEnable(GL_TEXTURE_2D);
	GLSTATE_ENABLE_ALPHATEST

	qglColor4f(1,1,1,1);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum(void)
{
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x * 0.5));
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector(frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x * 0.5);
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector(frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y * 0.5);
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - r_newrefdef.fov_y * 0.5));

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane(&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame(void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy(r_newrefdef.vieworg, r_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	// === jitwater - MPO's code to draw reflective water
	if (g_drawing_refl)
	{
		vec3_t tmp;

		r_origin[2] = (2 * g_refl_Z[g_active_refl]) - r_origin[2]; // flip

		VectorCopy(r_newrefdef.viewangles, tmp);
		tmp[0] *= -1.0f;
		AngleVectors(tmp, vpn, vright, vup);

		if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		{
			vec3_t temp;
			
			leaf = Mod_PointInLeaf(r_origin, r_worldmodel);
			temp[0] = g_refl_X[g_active_refl];
			temp[1] = g_refl_Y[g_active_refl];

			//if (r_newrefdef.rdflags & RDF_UNDERWATER) todo
			if (r_newrefdef.vieworg[2] < g_refl_Z[g_active_refl])
				temp[2] = g_refl_Z[g_active_refl] - 1;
			else
				temp[2] = g_refl_Z[g_active_refl] + 1;

			leaf = Mod_PointInLeaf(temp, r_worldmodel);

			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_viewcluster))
				r_viewcluster2 = leaf->cluster;
		}

		return;
	}
	// jitwater ===

// current viewcluster
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf(r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy(r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel);
			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy(r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel);
			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	//c_brush_polys = 0;
	//c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		qglEnable(GL_SCISSOR_TEST);
		qglClearColor(0.3, 0.3, 0.3, 1);
		qglScissor(r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//qglClearColor(1, 0, 0.5, 0.5); jitclearcolor
		qglDisable(GL_SCISSOR_TEST);
	}
}


void MYgluPerspective (GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar)
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan(fovy * M_PI /*/ 360.0*/ * 0.0027777777777777777777777777777778f);
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   xmin += -(2 * gl_state.camera_separation) / zNear;
   xmax += -(2 * gl_state.camera_separation) / zNear;

   qglFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
//	float	yfov;
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height -(r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	// === jitwater
	if (!g_drawing_refl)
		qglViewport(x, y2, w, h);
	else
		qglViewport(0, 0, g_reflTexW, g_reflTexH); // width/height of texture, not screen
	// jitwater ===
	
	//
	// set up projection matrix
	//
    screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity();

	if (fogenabled && fogdistance) // jitfog
		MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4, fogdistance+128);
	else
		MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4, 15000/*4096*/);  //jit

	qglCullFace(GL_FRONT); // todo
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();
    qglRotatef(-90,  1, 0, 0);	    // put Z going up
    qglRotatef(90,  0, 0, 1);	    // put Z going up

	// === jitwater
	if (!g_drawing_refl)
	{
		qglRotatef(-r_newrefdef.viewangles[2],  1, 0, 0);
		qglRotatef(-r_newrefdef.viewangles[0],  0, 1, 0);
		qglRotatef(-r_newrefdef.viewangles[1],  0, 0, 1);
		qglTranslatef(-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);
	}
	else
	{
		R_DoReflTransform();
		qglTranslatef(0, 0, -0); // what the hell does this do?! (todo, remove?)
	}
	// jitwater ===

	if (gl_state.camera_separation != 0 && gl_state.stereo_enabled)
		qglTranslatef(gl_state.camera_separation, 0, 0);

	qglGetFloatv(GL_MODELVIEW_MATRIX, r_world_matrix);

	// set drawing parms
	if (gl_cull->value) // jitwater -- culling disabled for reflection (todo - cull front instead of back?)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_ALPHATEST
	qglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
extern qboolean have_stencil;

void R_Clear (void)
{
	if (fogenabled)
		qglClearColor(fogcolor[0], fogcolor[1], fogcolor[2], 0.5); // jitfog

	if (gl_ztrick->value && !r_reflectivewater->value) // jitwater -- ztrick makes the screen flicker
	{
		static int trickframe;
		int clearbits = 0;

		if (gl_clear->value || fogenabled) // jitfog
			clearbits = GL_COLOR_BUFFER_BIT;

		if (have_stencil && gl_shadows->value == 2) // Stencil shadows - MrG
		{
			qglClearStencil(0);
			clearbits |= GL_STENCIL_BUFFER_BIT;
		}

		qglClear(clearbits);
		trickframe++;

		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			qglDepthFunc(GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			qglDepthFunc(GL_GEQUAL);
		}
	}
	else
	{
		int clearbits = GL_DEPTH_BUFFER_BIT;

		if (gl_clear->value || fogenabled) // jitfog
			clearbits |= GL_COLOR_BUFFER_BIT;
	
		if (have_stencil && gl_shadows->value == 2) // Stencil shadows - MrG
		{
			qglClearStencil(0);
			clearbits |= GL_STENCIL_BUFFER_BIT;
		}

		qglClear(clearbits);
		gldepthmin = 0;
		gldepthmax = 1;
		qglDepthFunc(GL_LEQUAL);
	}

	qglDepthRange(gldepthmin, gldepthmax);
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/

void R_ApplyStains(void);

void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");

	//if (!g_drawing_refl) // jitrspeeds r_speeds->value)
	//{
	//	c_brush_polys = 0;
	//	c_alias_polys = 0;
	//}

	// === jitfog -- enable fog rendering
	if (fogenabled)
	{
		if (fogdistance)
		{
			qglFogi(GL_FOG_MODE, GL_LINEAR);
			qglFogf(GL_FOG_END, fogdistance);
		}
		else
		{
			qglFogi(GL_FOG_MODE, GL_EXP);
			qglFogf(GL_FOG_DENSITY, fogdensity);
		}

		qglFogfv(GL_FOG_COLOR, fogcolor);  
		qglFogf(GL_FOG_START, 0.0f);		
		qglEnable(GL_FOG);
		//qglHint(GL_FOG_HINT, GL_NICEST);
	}
	// jit ===

	R_PushDlights();

	if (gl_finish->value)
		qglFinish();

	if (r_newrefdef.num_newstains > 0 && gl_stainmaps->value)
		R_ApplyStains();

	R_SetupFrame();

	R_SetFrustum();

	R_SetupGL();

	// === jitwater
	// MPO - if we are doing a reflection, we want to do a clip plane now,
	// after  we've set up our projection/modelview matrices
	if (g_drawing_refl)
	{
		double clipPlane[] = { 0.0, 0.0, 0.0, 0.0 }; // this must be double, glClipPlane requires double

		//if (r_newrefdef.rdflags & RDF_UNDERWATER)
		if (r_newrefdef.vieworg[2] < g_refl_Z[g_active_refl])
		{
			clipPlane[2] = -1.0;
			clipPlane[3] = g_refl_Z[g_active_refl];
		}
		else
		{
			clipPlane[2] = 1.0;
			clipPlane[3] = -g_refl_Z[g_active_refl];
		}

		// we need clipping so we don't reflect objects behind the water
		qglEnable(GL_CLIP_PLANE0);
		qglClipPlane(GL_CLIP_PLANE0, clipPlane);
	}
	// jitwater ===

	R_MarkLeaves();	// done here so we know if we're in water

	R_DrawWorld();

	R_DrawCaustics(); // jitcaustics

	R_DrawEntitiesOnList();

	R_RenderDlights();

	R_DrawParticles();

	R_DrawAlphaSurfaces();

	// todo R_DrawParticles();			// MPO dukey particles have to be drawn twice .. otherwise you dont get reflection of them.

	R_DrawSpritesOnList(); // draw smoke after water so water doesn't cover it!

	if (fogenabled)
		qglDisable(GL_FOG);

	if (g_drawing_refl) // jitwater
		qglDisable(GL_CLIP_PLANE0);
	else
		R_PolyBlend(); // jit, replaced R_Flash();
}

unsigned int blurtex = 0;

void R_SetGL2D (void)
{
	int hudscale;

	hudscale = cl_hudscale->value; // jithudscale

	// set 2D virtual screen size
	qglViewport(0,0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity();
	qglOrtho (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_CULL_FACE);

	if (gl_state.tex_rectangle && gl_motionblur->value)
	{
		if (blurtex)
		{
			register float height = (float)vid.height;
			register float width = (float)vid.width;

			GL_TexEnv(GL_MODULATE);
			qglDisable(GL_TEXTURE_2D);
			qglEnable(gl_state.tex_rectangle); // jitblur
			GLSTATE_ENABLE_BLEND
			GLSTATE_DISABLE_ALPHATEST
			qglBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

			if (gl_motionblur->value >= 1.0f)
				qglColor4f(1.0f, 1.0f, 1.0f, 0.45f);
			else
				qglColor4f(1.0f, 1.0f, 1.0f, gl_motionblur->value);

			qglBegin(GL_QUADS);
			qglTexCoord2f(0.0f, height);
			qglVertex2f(0.0f, 0.0f);
			qglTexCoord2f(width, height);
			qglVertex2f(width, 0.0f);
			qglTexCoord2f(width, 0.0f);
			qglVertex2f(width, height);
			qglTexCoord2f(0.0f, 0.0f);
			qglVertex2f(0.0f, height);
			qglEnd();

			qglDisable(gl_state.tex_rectangle); // jitblur
			qglEnable(GL_TEXTURE_2D);
		}

		if (!blurtex)
			qglGenTextures(1, &blurtex);

		qglBindTexture(gl_state.tex_rectangle, blurtex); // jitblur
		qglCopyTexImage2D(gl_state.tex_rectangle, 0, GL_RGB, 0, 0, vid.width, vid.height, 0); // jitblur
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	GLSTATE_DISABLE_BLEND
	GLSTATE_ENABLE_ALPHATEST
	qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// === jit
	if (r_speeds->value)
	{
		char s[256]; 

		Com_sprintf(s, sizeof(s), "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys, 
			c_alias_polys, 
			c_visible_textures, 
			c_visible_lightmaps);
		Draw_String(0, r_newrefdef.height - 32*hudscale, s);
	}
	// jit ===
}

static void GL_DrawColoredStereoLinePair (float r, float g, float b, float y)
{
	qglColor3f(r, g, b);
	qglVertex2f(0, y);
	qglVertex2f(vid.width, y);
	qglColor3f(0, 0, 0);
	qglVertex2f(0, y + 1);
	qglVertex2f(vid.width, y + 1);
}

static void GL_DrawStereoPattern (void)
{
	int i;

	if (!(gl_config.renderer & GL_RENDERER_INTERGRAPH))
		return;

	if (!gl_state.stereo_enabled)
		return;

	R_SetGL2D();

	qglDrawBuffer(GL_BACK_LEFT);

	for (i = 0; i < 20; i++)
	{
		qglBegin(GL_LINES);
			GL_DrawColoredStereoLinePair(1, 0, 0, 0);
			GL_DrawColoredStereoLinePair(1, 0, 0, 2);
			GL_DrawColoredStereoLinePair(1, 0, 0, 4);
			GL_DrawColoredStereoLinePair(1, 0, 0, 6);
			GL_DrawColoredStereoLinePair(0, 1, 0, 8);
			GL_DrawColoredStereoLinePair(1, 1, 0, 10);
			GL_DrawColoredStereoLinePair(1, 1, 0, 12);
			GL_DrawColoredStereoLinePair(0, 1, 0, 14);
		qglEnd();
		
		GLimp_EndFrame();
	}
}


/*
====================
R_SetLightLevel
====================
*/
void R_SetLightLevel(void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint(r_newrefdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
#if 1
	// === jitwater
	g_refl_enabled = false;

	if (r_reflectivewater->value)
		R_UpdateReflTex(fd);
	// jitwater ===

	R_RenderView(fd);
	R_SetLightLevel();
	R_SetGL2D();
	c_brush_polys = 0; // jitrspeeds - relocated
	c_alias_polys = 0;

	// === jitwater
	if (r_reflectivewater_debug->value && g_refl_enabled)
		R_DrawDebugReflTexture();

	if (!g_refl_enabled)
		R_clear_refl();
#else
	//start MPO
	if (gl_reflection->value)
	{
		R_clear_refl();								//clear our reflections found in last frame
		R_RecursiveFindRefl(r_worldmodel->nodes);	//find reflections for this frame
		R_UpdateReflTex(fd);						//render reflections to textures
	}
	else
	{
		R_clear_refl();
	}
	// end MPO

  	R_RenderView(fd);
	R_SetLightLevel();
	R_SetGL2D();

	// start MPO
	// if debugging is enabled and reflections are enabled.. draw it
	if ((gl_reflection_debug->value) && (g_refl_enabled))
		R_DrawDebugReflTexture();

	if (!g_refl_enabled)
		R_clear_refl();
	// end MPO
#endif
}


void R_Register(void)
{
	cl_animdump = ri.Cvar_Get("cl_animdump", "0", 0); // frame dump - MrG
	r_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);
	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	gl_nosubimage = ri.Cvar_Get("gl_nosubimage", "0", 0);
	gl_allow_software = ri.Cvar_Get("gl_allow_software", "0", CVAR_ARCHIVE); // jits - archive this now
	gl_particle_min_size = ri.Cvar_Get("gl_particle_min_size", "2", CVAR_ARCHIVE);
	gl_particle_max_size = ri.Cvar_Get("gl_particle_max_size", "40", CVAR_ARCHIVE);
	gl_particle_size = ri.Cvar_Get("gl_particle_size", "40", CVAR_ARCHIVE);
	gl_particle_att_a = ri.Cvar_Get("gl_particle_att_a", "0.01", CVAR_ARCHIVE);
	gl_particle_att_b = ri.Cvar_Get("gl_particle_att_b", "0.0", CVAR_ARCHIVE);
	gl_particle_att_c = ri.Cvar_Get("gl_particle_att_c", "0.01", CVAR_ARCHIVE);
	gl_texture_saturation = ri.Cvar_Get("gl_texture_saturation", "1", CVAR_ARCHIVE); // jitsaturation
	gl_highres_textures = ri.Cvar_Get("gl_highres_textures", "1", CVAR_ARCHIVE); // jithighres
	gl_lightmap_saturation = ri.Cvar_Get("gl_lightmap_saturation", "1", CVAR_ARCHIVE); // jitsaturation / jitlight
	gl_anisotropy = ri.Cvar_Get("gl_anisotropy", "8", CVAR_ARCHIVE); // jitanisotropy
	gl_overbright = ri.Cvar_Get("gl_overbright", "1", CVAR_ARCHIVE); // jitbright
	gl_brightness = ri.Cvar_Get("gl_brightness", "0", CVAR_ARCHIVE); // jit
	gl_autobrightness = ri.Cvar_Get("gl_autobrightness", ".8", CVAR_ARCHIVE); // jit
	gl_showbbox = ri.Cvar_Get("gl_showbbox", "0", 0);  // jit / Guy
//	gl_modulate = ri.Cvar_Get("gl_modulate", "1.6", CVAR_ARCHIVE); // jit, default to 1.6
	gl_lightmapgamma = ri.Cvar_Get("gl_lightmapgamma", ".6", CVAR_ARCHIVE); // jitgamma
	gl_textshadow = ri.Cvar_Get("gl_textshadow", "1", CVAR_ARCHIVE); // jittext
	gl_hash_textures = ri.Cvar_Get("gl_hash_textures", "1", CVAR_ARCHIVE); // jithash
	gl_free_unused_textures = ri.Cvar_Get("gl_free_unused_textures", "1", CVAR_ARCHIVE); // jitfreeunused
	cl_hudscale = ri.Cvar_Get("cl_hudscale", "2", CVAR_ARCHIVE); // jithudscale
	gl_log = ri.Cvar_Get("gl_log", "0", 0);
	gl_bitdepth = ri.Cvar_Get("gl_bitdepth", "0", 0);
	gl_mode = ri.Cvar_Get("gl_mode", "3", CVAR_ARCHIVE);
	gl_lightmap = ri.Cvar_Get("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get("gl_shadows", "0", CVAR_ARCHIVE);
	gl_dynamic = ri.Cvar_Get("gl_dynamic", "1", 0);
	gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);
	gl_round_down = ri.Cvar_Get("gl_round_down", "0", 0); // jit, was 1
	gl_picmip = ri.Cvar_Get("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get("gl_skymip", "0", 0);
	gl_showtris = ri.Cvar_Get("gl_showtris", "0", 0);
	gl_ztrick = ri.Cvar_Get("gl_ztrick", "0", 0);
	gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = ri.Cvar_Get("gl_clear", "0", 0);
	gl_cull = ri.Cvar_Get("gl_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get("gl_polyblend", "1", 0);
	gl_flashblend = ri.Cvar_Get("gl_flashblend", "0", 0);
	gl_playermip = ri.Cvar_Get("gl_playermip", "0", 0);
	gl_monolightmap = ri.Cvar_Get("gl_monolightmap", "0", 0);
#ifdef WIN32
	gl_driver = ri.Cvar_Get("gl_driver", "opengl32", CVAR_ARCHIVE);
#else
	gl_driver = ri.Cvar_Get("gl_driver", "libGL.so", CVAR_ARCHIVE);
#endif
	gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE); // jit
	gl_texturealphamode = ri.Cvar_Get("gl_texturealphamode", "default", CVAR_ARCHIVE);
	gl_texturesolidmode = ri.Cvar_Get("gl_texturesolidmode", "default", CVAR_ARCHIVE);
	gl_lockpvs = ri.Cvar_Get("gl_lockpvs", "0", 0);
	gl_vertex_arrays = ri.Cvar_Get("gl_vertex_arrays", "0", CVAR_ARCHIVE);
	gl_ext_multitexture = ri.Cvar_Get("gl_ext_multitexture", "1", CVAR_ARCHIVE);
	gl_ext_pointparameters = ri.Cvar_Get("gl_ext_pointparameters", "1", CVAR_ARCHIVE);
	gl_ext_compiled_vertex_array = ri.Cvar_Get("gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE);
	gl_ext_texture_compression = ri.Cvar_Get("gl_ext_texture_compression", "0", CVAR_ARCHIVE); // Heffo - ARB Texture Compression
	gl_screenshot_jpeg = ri.Cvar_Get("gl_screenshot_jpeg", "1", CVAR_ARCHIVE);					// Heffo - JPEG Screenshots
	gl_screenshot_jpeg_quality = ri.Cvar_Get("gl_screenshot_jpeg_quality", "85", CVAR_ARCHIVE);	// Heffo - JPEG Screenshots
	gl_stainmaps = ri.Cvar_Get("gl_stainmaps", "1", CVAR_ARCHIVE);	// stainmaps
	gl_motionblur = ri.Cvar_Get("gl_motionblur", "0", CVAR_ARCHIVE);	// motionblur
	vid_gamma_hw = ri.Cvar_Get("vid_gamma_hw", "0", CVAR_ARCHIVE);		// hardware gamma
	gl_drawbuffer = ri.Cvar_Get("gl_drawbuffer", "GL_BACK", 0);
	gl_swapinterval = ri.Cvar_Get("gl_swapinterval", "1", CVAR_ARCHIVE);
	gl_saturatelighting = ri.Cvar_Get("gl_saturatelighting", "0", 0);
	gl_3dlabs_broken = ri.Cvar_Get("gl_3dlabs_broken", "1", CVAR_ARCHIVE);
	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);
	vid_lighten = ri.Cvar_Get("vid_lighten", "0", CVAR_ARCHIVE); // jitgamma
	vid_ref = ri.Cvar_Get("vid_ref", "pbgl", CVAR_ARCHIVE);
	r_caustics = ri.Cvar_Get("r_caustics", "2", CVAR_ARCHIVE); // jitcaustics
	r_reflectivewater = ri.Cvar_Get("r_reflectivewater", "1", CVAR_ARCHIVE); // jitwater
	r_reflectivewater_debug = ri.Cvar_Get("r_reflectivewater_debug", "0", 0); // jitwater
	r_reflectivewater_max = ri.Cvar_Get("r_reflectivewater_max", "2", CVAR_ARCHIVE); // jitwater
	r_oldmodels = ri.Cvar_Get("r_oldmodels", "0", CVAR_ARCHIVE); // jit
	ri.Cmd_AddCommand("imagelist", GL_ImageList_f);
	ri.Cmd_AddCommand("screenshot", GL_ScreenShot_f);
	ri.Cmd_AddCommand("modellist", Mod_Modellist_f);
	ri.Cmd_AddCommand("gl_strings", GL_Strings_f);
}

/*
==================
R_SetMode
==================
*/
qboolean R_SetMode (void)
{
	rserr_t err;
	qboolean fullscreen;

	if (vid_fullscreen->modified && !gl_config.allow_cds)
	{
		ri.Con_Printf(PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n");
		ri.Cvar_SetValue("vid_fullscreen", !vid_fullscreen->value);
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	gl_mode->modified = false;

	if ((err = GLimp_SetMode(&vid.width, &vid.height, gl_mode->value, fullscreen)) == rserr_ok)
	{
		gl_state.prev_mode = gl_mode->value;
	}
	else
	{
		if (err == rserr_invalid_fullscreen)
		{
			ri.Cvar_SetValue("vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n");

			if ((err = GLimp_SetMode(&vid.width, &vid.height, gl_mode->value, false)) == rserr_ok)
				return true;
		}
		else if (err == rserr_invalid_mode)
		{
			ri.Cvar_SetValue("gl_mode", gl_state.prev_mode);
			gl_mode->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode(&vid.width, &vid.height, gl_state.prev_mode, false)) != rserr_ok)
		{
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}
	return true;
}

// jit3dfx -- check if a 3dfx board is present.
// http://talika.eii.us.es/~titan/oglfaq/#SUBJECT16
// GlideReports3dfxBoardPresent(void)
#ifdef WIN32
BOOL(__stdcall* GLIDEFUN_grSstQueryBoards)(GrHwConfiguration* hwconfig);
#endif

qboolean UsingGlideDriver () // jit3dfx
{
#ifdef WIN32
	char GlideDriver[] = "glide2x.dll";

	// load glide
	HMODULE hmGlide = LoadLibrary(GlideDriver);

	if (hmGlide)
	{
		// get query function
		GLIDEFUN_grSstQueryBoards =(BOOL(__stdcall*)(GrHwConfiguration* hwconfig)) 
			GetProcAddress(hmGlide, "_grSstQueryBoards@4");

		if (GLIDEFUN_grSstQueryBoards)
		{
			GrHwConfiguration GlideHwConfig;

			GLIDEFUN_grSstQueryBoards(&GlideHwConfig);

			if (GlideHwConfig.num_sst > 0)
			{
				FreeLibrary(hmGlide);
				return true;
			}
			else
			{
				FreeLibrary(hmGlide);
				return false;
			}
		}
		else
		{
			FreeLibrary(hmGlide);
			return false;
		}
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

/*
===============
R_Init
===============
*/
qboolean R_Init (void *hinstance, void *hWnd)
{	
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int err;
	int j;
	extern float r_turbsin[256];
	char *path = NULL; // jitrscript

	gl_debug = ri.Cvar_Get("gl_debug", "0", CVAR_ARCHIVE); // jit
	gl_sgis_generate_mipmap = ri.Cvar_Get("gl_sgis_generate_mipmap", "0", CVAR_ARCHIVE); // jit
	gl_arb_fragment_program = ri.Cvar_Get("gl_arb_fragment_program", "1", CVAR_ARCHIVE); // jit

	for (j = 0; j < 256; j++)
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Con_Printf(PRINT_ALL, "ref_gl version: "REF_VERSION"\n");

	Draw_GetPalette();

	R_Register();

	// initialize our QGL dynamic bindings
	if (!QGL_Init(gl_driver->string))
	{
		ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - could not load \"%s\"\n", gl_driver->string);

		// if glide2x detects a voodoo present, switch modes to
		// 3dfxgl, fullscreen, 640x480.
		if (UsingGlideDriver() && !Q_streq(gl_driver->string, "3dfxgl")) // jit3dfx
		{
			ri.Cvar_Set("gl_driver", "3dfxgl");
			ri.Cvar_Set("vid_fullscreen", "1");
			ri.Cvar_Set("gl_mode", "3");
			ri.Cvar_Set("vid_gamma_hw", "0");
			ri.Cvar_Set("vid_gamma", "1");
		}
		// otherwise, if the driver was set improperly, change it back to
		// opengl32(bastard might have finally upgraded), and put it in
		// windowed 640x480, shutting off hardware gamma to be safe.
#ifdef WIN32
#define GL_DRIVER_LIB "opengl32"
#else
#define GL_DRIVER_LIB "libGL.so"
#endif
		else if (!Q_streq (gl_driver->string, GL_DRIVER_LIB))
		{
			ri.Cvar_Set("gl_driver", GL_DRIVER_LIB);
			ri.Cvar_Set("vid_fullscreen", "0");
			ri.Cvar_Set("gl_mode", "3");
			ri.Cvar_Set("vid_gamma_hw", "0");
			ri.Cvar_Set("vid_gamma", "1");
		}

		QGL_Shutdown();
        
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if (!GLimp_Init(hinstance, hWnd))
	{
		QGL_Shutdown();
		return -1;
	}

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if (!R_SetMode())
	{
		QGL_Shutdown();
        ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n");
		// ===
		// jit -- error out if we can't load this, rather than crash!
		Sys_Error("Error during initialization.\nMake sure you have an OpenGL capable video card and that the latest drivers are installed.");
		// ===
		return -1;
	}

//jitmenu	ri.Vid_MenuInit();

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = qglGetString(GL_VENDOR);
	gl_config.renderer_string = qglGetString(GL_RENDERER);
	gl_config.version_string = qglGetString(GL_VERSION);
	gl_config.extensions_string = qglGetString(GL_EXTENSIONS);

	if (gl_debug->value) // jit
	{
		ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
		ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
		ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string);
		ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string);
	}

	strcpy(renderer_buffer, gl_config.renderer_string);
	strlwr(renderer_buffer);

	strcpy(vendor_buffer, gl_config.vendor_string);
	strlwr(vendor_buffer);

	if (strstr(renderer_buffer, "voodoo"))
	{
		if (!strstr(renderer_buffer, "rush"))
			gl_config.renderer = GL_RENDERER_VOODOO;
		else
			gl_config.renderer = GL_RENDERER_VOODOO_RUSH;
	}
	else if (strstr (vendor_buffer, "sgi"))
		gl_config.renderer = GL_RENDERER_SGI;
	else if (strstr (renderer_buffer, "permedia"))
		gl_config.renderer = GL_RENDERER_PERMEDIA2;
	else if (strstr (renderer_buffer, "glint"))
		gl_config.renderer = GL_RENDERER_GLINT_MX;
	else if (strstr (renderer_buffer, "glzicd"))
		gl_config.renderer = GL_RENDERER_REALIZM;
	else if (strstr (renderer_buffer, "gdi"))
		gl_config.renderer = GL_RENDERER_MCD;
	else if (strstr (renderer_buffer, "pcx2"))
		gl_config.renderer = GL_RENDERER_PCX2;
	else if (strstr (renderer_buffer, "verite"))
		gl_config.renderer = GL_RENDERER_RENDITION;
	else
		gl_config.renderer = GL_RENDERER_OTHER;

	if (toupper(gl_monolightmap->string[1]) != 'F')
	{
		if (gl_config.renderer == GL_RENDERER_PERMEDIA2)
		{
			ri.Cvar_Set("gl_monolightmap", "A");
			ri.Con_Printf(PRINT_ALL, "...using gl_monolightmap 'a'\n");
		}
		else if (gl_config.renderer & GL_RENDERER_POWERVR) 
		{
			ri.Cvar_Set("gl_monolightmap", "0");
		}
		else
		{
			ri.Cvar_Set("gl_monolightmap", "0");
		}
	}

	// power vr can't have anything stay in the framebuffer, so
	// the screen needs to redraw the tiled background every frame
	if (gl_config.renderer & GL_RENDERER_POWERVR) 
	{
		ri.Cvar_Set("scr_drawall", "1");
	}
	else
	{
		ri.Cvar_Set("scr_drawall", "0");
	}

#ifdef __linux__
	ri.Cvar_SetValue("gl_finish", 1);
#endif

	// MCD has buffering issues
	if (gl_config.renderer == GL_RENDERER_MCD)
	{
		ri.Cvar_SetValue("gl_finish", 1);
	}

	if (gl_config.renderer & GL_RENDERER_3DLABS)
	{
		if (gl_3dlabs_broken->value)
			gl_config.allow_cds = false;
		else
			gl_config.allow_cds = true;
	}
	else
	{
		gl_config.allow_cds = true;
	}

	if (gl_debug->value) // jit
	{
		if (gl_config.allow_cds)
			ri.Con_Printf(PRINT_ALL, "...allowing CDS\n");
		else
			ri.Con_Printf(PRINT_ALL, "...disabling CDS\n");
	}

	/*
	** grab extensions
	*/
	if (strstr(gl_config.extensions_string, "GL_EXT_compiled_vertex_array") || 
		 strstr(gl_config.extensions_string, "GL_SGI_compiled_vertex_array"))
	{
		if (gl_debug->value)// jit
			ri.Con_Printf(PRINT_ALL, "...enabling GL_EXT_compiled_vertex_array\n");

		qglLockArraysEXT =(void*)qwglGetProcAddress("glLockArraysEXT");
		qglUnlockArraysEXT =(void*)qwglGetProcAddress("glUnlockArraysEXT");
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n");
	}

#ifdef _WIN32
	if (strstr(gl_config.extensions_string, "WGL_EXT_swap_control"))
	{
		qwglSwapIntervalEXT =(BOOL(WINAPI*)(int))qwglGetProcAddress("wglSwapIntervalEXT");

		if (gl_debug->value) // jit
			ri.Con_Printf(PRINT_ALL, "...enabling WGL_EXT_swap_control\n");
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...WGL_EXT_swap_control not found\n");
	}
#endif

	if (strstr(gl_config.extensions_string, "GL_EXT_point_parameters"))
	{
		//if(gl_ext_pointparameters->value)
		if (0) // Workaround for ATI driver bug.
		{
			qglPointParameterfEXT = (void(APIENTRY*)(GLenum, GLfloat))qwglGetProcAddress("glPointParameterfEXT");
			qglPointParameterfvEXT = (void(APIENTRY*)(GLenum, const GLfloat*))qwglGetProcAddress("glPointParameterfvEXT");

			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_EXT_point_parameters\n");
		}
		else if (gl_debug->value) // jit
		{
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_EXT_point_parameters\n");
		}
	}
	else if (gl_debug->value) // jit
	{
		ri.Con_Printf(PRINT_ALL, "...GL_EXT_point_parameters not found\n");
	}

	if (strstr(gl_config.extensions_string, "GL_ARB_multitexture"))
	{
		if (gl_ext_multitexture->value)
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_ARB_multitexture\n");

			qglMultiTexCoord2fARB = (void*)qwglGetProcAddress("glMultiTexCoord2fARB");
			qglMultiTexCoord3fvARB = (void*)qwglGetProcAddress("glMultiTexCoord3fvARB");
			qglMultiTexCoord3fARB = (void*)qwglGetProcAddress("glMultiTexCoord3fARB");
			qglActiveTextureARB = (void*)qwglGetProcAddress("glActiveTextureARB");
			qglClientActiveTextureARB = (void*)qwglGetProcAddress("glClientActiveTextureARB");
			QGL_TEXTURE0 = GL_TEXTURE0_ARB;
			QGL_TEXTURE1 = GL_TEXTURE1_ARB;
			QGL_TEXTURE2 = GL_TEXTURE2_ARB;
		}
		else if (gl_debug->value)
		{
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_ARB_multitexture\n");
		}
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...GL_ARB_multitexture not found\n");
	}

	if (strstr(gl_config.extensions_string, "GL_SGIS_multitexture"))
	{
		if (qglActiveTextureARB)
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...GL_SGIS_multitexture deprecated in favor of ARB_multitexture\n");
		}
		else if (gl_ext_multitexture->value)
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_SGIS_multitexture\n");

			qglMultiTexCoord2fARB = (void*)qwglGetProcAddress("glMTexCoord2fSGIS");
			qglSelectTextureSGIS = (void*)qwglGetProcAddress("glSelectTextureSGIS");
			QGL_TEXTURE0 = GL_TEXTURE0_SGIS;
			QGL_TEXTURE1 = GL_TEXTURE1_SGIS;
		}
		else if (gl_debug->value) // jit
		{
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_SGIS_multitexture\n");
		}
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...GL_SGIS_multitexture not found\n");
	}

	// ===
	// jitspoe - these texture shaders screw up geforce4 cards... killing it.

	// Texture Shader support - MrG
/*	if (strstr(gl_config.extensions_string, "GL_NV_texture_shader"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_NV_texture_shader\n");
		gl_state.texshaders=true;
	} else {
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_NV_texture_shader not found\n");
		gl_state.texshaders=false;
	}
*/
	gl_state.texshaders = false;
	// ===

	if (strstr(gl_config.extensions_string, "GL_SGIS_generate_mipmap"))
	{
		if (gl_sgis_generate_mipmap->value)
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_SGIS_generate_mipmap\n");

			gl_state.sgis_mipmap = true;
		}
		else
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...GL_SGIS_generate_mipmap found but disabled\n");

			gl_state.sgis_mipmap = false;
		}
	}
	else
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_SGIS_generate_mipmap not found\n");

		gl_state.sgis_mipmap = false;
	}

	if (strstr(gl_config.extensions_string, "GL_NV_texture_rectangle"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_NV_texture_rectangle\n");

		gl_state.tex_rectangle = GL_TEXTURE_RECTANGLE_NV; // jitblur
	} 
	else if (strstr (gl_config.extensions_string, "GL_EXT_texture_rectangle"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_EXT_texture_rectangle\n");

		gl_state.tex_rectangle = GL_TEXTURE_RECTANGLE_NV; // jitodo(jitblur)
	} 
	else
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_EXT_texture_rectangle not found\n");

		gl_state.tex_rectangle = 0; // jitblur
	}

	// Heffo - ARB Texture Compression
	if (strstr(gl_config.extensions_string, "GL_ARB_texture_compression"))
	{
		if (!gl_ext_texture_compression->value)
		{
			if (gl_debug->value) // jit
				ri.Con_Printf(PRINT_ALL, "...ignoring GL_ARB_texture_compression\n");

			gl_state.texture_compression = false;
		}
		else
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_ARB_texture_compression\n");

			gl_state.texture_compression = true;
		}
	}
	else
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_ARB_texture_compression not found\n");

		gl_state.texture_compression = false;
		ri.Cvar_Set("gl_ext_texture_compression", "0");
	}

	// ===
	// jitanisotropy
	gl_state.max_anisotropy = 0;

	if (strstr(gl_config.extensions_string, "texture_filter_anisotropic"))
	{
		qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_state.max_anisotropy);
	}

	if (gl_debug->value)
		ri.Con_Printf(PRINT_ALL, "Max anisotropy level: %g\n", gl_state.max_anisotropy);

	// === jitbright
	if (strstr(gl_config.extensions_string, "texture_env_combine")) 
	{
		gl_state.texture_combine = true;

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_ARB_texture_env_combine\n");
	}
	else
	{
		gl_state.texture_combine = false;
	}
	// jitbright ===

	// === jitwater
	if (!gl_arb_fragment_program->value)
	{
		gl_state.fragment_program = false;

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_ARB_fragment_program disabled\n");
	}
	else if (strstr(gl_config.extensions_string, "GL_ARB_fragment_program"))
	{
		gl_state.fragment_program = true;

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_ARB_fragment_program\n");

		qglGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)qwglGetProcAddress("glGenProgramsARB");
		qglDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)qwglGetProcAddress("glDeleteProgramsARB");
		qglBindProgramARB = (PFNGLBINDPROGRAMARBPROC)qwglGetProcAddress("glBindProgramARB");
		qglProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)qwglGetProcAddress("glProgramStringARB");
		qglProgramEnvParameter4fARB =
			(PFNGLPROGRAMENVPARAMETER4FARBPROC)qwglGetProcAddress("glProgramEnvParameter4fARB");
		qglProgramLocalParameter4fARB =
			(PFNGLPROGRAMLOCALPARAMETER4FARBPROC)qwglGetProcAddress("glProgramLocalParameter4fARB");

		if (!(qglGenProgramsARB && qglDeleteProgramsARB && qglBindProgramARB &&
			qglProgramStringARB && qglProgramEnvParameter4fARB && qglProgramLocalParameter4fARB))
		{
			gl_state.fragment_program = false;
			
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "... Failed!  Fragment programs disabled\n");
		}
	}
	else
	{
		gl_state.fragment_program = false;

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_ARB_fragment_program not found\n");
	}
	// jitwater ===

	if (strstr(gl_config.extensions_string, "GL_NV_register_combiners"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_NV_register_combiners\n");

		qglCombinerParameterfvNV=(PFNGLCOMBINERPARAMETERFVNVPROC) qwglGetProcAddress("glCombinerParameterfvNV");
		qglCombinerParameterfNV=(PFNGLCOMBINERPARAMETERFNVPROC) qwglGetProcAddress("glCombinerParameterfNV");
		qglCombinerParameterivNV=(PFNGLCOMBINERPARAMETERIVNVPROC) qwglGetProcAddress("glCombinerParameterivNV");
		qglCombinerParameteriNV=(PFNGLCOMBINERPARAMETERINVPROC) qwglGetProcAddress("glCombinerParameteriNV");
		qglCombinerInputNV=(PFNGLCOMBINERINPUTNVPROC) qwglGetProcAddress("glCombinerInputNV");
		qglCombinerOutputNV=(PFNGLCOMBINEROUTPUTNVPROC) qwglGetProcAddress("glCombinerOutputNV");
		qglFinalCombinerInputNV=(PFNGLFINALCOMBINERINPUTNVPROC) qwglGetProcAddress("glFinalCombinerInputNV");
		qglGetCombinerInputParameterfvNV=(PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC) qwglGetProcAddress("glGetCombinerInputParameterfvNV");
		qglGetCombinerInputParameterivNV=(PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC) qwglGetProcAddress("glGetCombinerInputParameterivNV");
		qglGetCombinerOutputParameterfvNV=(PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC) qwglGetProcAddress("glGetCombinerOutputParameterfvNV");
		qglGetCombinerOutputParameterivNV=(PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC) qwglGetProcAddress("glGetCombinerOutputParameterivNV");
		qglGetFinalCombinerInputParameterfvNV=(PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC) qwglGetProcAddress("glGetFinalCombinerInputParameterfvNV");
		qglGetFinalCombinerInputParameterivNV=(PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC) qwglGetProcAddress("glGetFinalCombinerInputParameterivNV");

		gl_state.reg_combiners=true;
	} else {
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_NV_register_combiners\n");
		gl_state.reg_combiners=false;
	}

	GL_SetDefaultState();

	/*
	** draw our stereo patterns
	*/
//#if 0 // commented out until H3D pays us the money they owe us
#if 1 // jit
	GL_DrawStereoPattern();
#endif

	// ===
	// jitanisotropy(make sure nobody goes out of bounds)
	if (gl_anisotropy->value < 0)
		ri.Cvar_Set("gl_anisotropy", "0");
	else if (gl_anisotropy->value > gl_state.max_anisotropy)
		ri.Cvar_SetValue("gl_anisotropy", gl_state.max_anisotropy);
	// ===

	if (gl_texture_saturation->value > 1 || gl_texture_saturation->value < 0)
		ri.Cvar_Set("gl_texture_saturation", "1");  // jitsaturation

	while((path = ri.FS_NextPath(path)) != NULL) // jitrscript -- get all game dirs
		RS_ScanPathForScripts(path);

	init_image_hash_tables();
	GL_InitImages();
	Mod_Init();
	R_InitNoTexture(); // jit, renamed
	Draw_InitLocal();

	err = qglGetError();

	if (err != GL_NO_ERROR)
		ri.Con_Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);

	R_init_refl(r_reflectivewater_max->value); // jitwater / MPO

	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown(void)
{	
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	Mod_FreeAll();

	GL_ShutdownImages();
	RS_FreeAllScripts(); // jitrscript

	if (char_colors)
		free(char_colors); // jittext

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();

	/*
	** shutdown our QGL subsystem
	*/
	QGL_Shutdown();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void UpdateGammaRamp ();

void R_BeginFrame (float camera_separation)
{

	gl_state.camera_separation = camera_separation;

	// change modes if necessary
	if (gl_mode->modified || vid_fullscreen->modified)
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = ri.Cvar_Get("vid_ref", "gl", 0);
		ref->modified = true;
	}

	if (gl_log->modified)
	{
		GLimp_EnableLogging(gl_log->value);
		gl_log->modified = false;
	}

	if (gl_log->value)
		GLimp_LogNewFrame();

	if (vid_gamma->modified)
	{
		vid_gamma->modified = false;

		if (vid_gamma_hw->value && gl_state.gammaramp) 
		{
			UpdateGammaRamp();
		} 
		else if (gl_config.renderer & GL_RENDERER_VOODOO) 
		{
			char envbuffer[1024];
			float g;

			g = 2.0f * (0.8f -(vid_gamma->value - 0.5f)) + 1.0F;
			Com_sprintf(envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g);
			putenv(envbuffer);
			Com_sprintf(envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g);
			putenv(envbuffer);
		}
	}

	if (vid_lighten->modified) // jitgamma
	{
		vid_lighten->modified = false;

		if (vid_gamma_hw->value && gl_state.gammaramp) 
		{
			UpdateGammaRamp();
		} 
	}

	GLimp_BeginFrame(camera_separation);

	// go into 2D mode
	qglViewport(0,0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity();
	qglOrtho(0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_CULL_FACE);
	GLSTATE_DISABLE_BLEND
	GLSTATE_ENABLE_ALPHATEST
	qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// draw buffer stuff
	if (gl_drawbuffer->modified)
	{
		gl_drawbuffer->modified = false;

		if (gl_state.camera_separation == 0 || !gl_state.stereo_enabled)
		{
			if (Q_strcasecmp(gl_drawbuffer->string, "GL_FRONT") == 0)
				qglDrawBuffer(GL_FRONT);
			else
				qglDrawBuffer(GL_BACK);
		}
	}

	// texturemode stuff
	if (gl_texturemode->modified)
	{
		GL_TextureMode(gl_texturemode->string);
		gl_texturemode->modified = false;
	}

	if (gl_texturealphamode->modified)
	{
		GL_TextureAlphaMode(gl_texturealphamode->string);
		gl_texturealphamode->modified = false;
	}

	if (gl_texturesolidmode->modified)
	{
		GL_TextureSolidMode(gl_texturesolidmode->string);
		gl_texturesolidmode->modified = false;
	}

	// swapinterval stuff (vsync)
	GL_UpdateSwapInterval();

	// clear screen if desired
	R_Clear();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette(const unsigned char *palette)
{
	int		i;

	byte *rp =(byte *) r_rawpalette;

	if (palette)
	{
		for (i = 0; i < 256; i++)
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for (i = 0; i < 256; i++)
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] =(d_8to24table[i] >> 8) & 0xff;
			rp[i*4+2] =(d_8to24table[i] >> 16) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
	qglClearColor(0,0,0,0);
	qglClear(GL_COLOR_BUFFER_BIT);
	//qglClearColor(1,0, 0.5 , 0.5); jitclearcolor	
}

/*
** R_DrawBeam
*/
void R_DrawBeam(entity_t *e)
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0)
		return;

	PerpendicularVector(perpvec, normalized_direction);
	VectorScale(perpvec, e->frame * 0.5, perpvec);

	for (i = 0; i < 6; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec,(360.0/NUM_BEAM_SEGS)*i);
		VectorAdd(start_points[i], origin, start_points[i]);
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	qglDisable(GL_TEXTURE_2D);
	GLSTATE_ENABLE_BLEND
	qglDepthMask(GL_FALSE);

	r =(d_8to24table[e->skinnum & 0xFF]) & 0xFF;
	g =(d_8to24table[e->skinnum & 0xFF] >> 8) & 0xFF;
	b =(d_8to24table[e->skinnum & 0xFF] >> 16) & 0xFF;

	r /= 255.0F;
	g /= 255.0F;
	b /= 255.0F;

	qglColor4f(r, g, b, e->alpha);

	qglBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i < NUM_BEAM_SEGS; i++)
	{
		qglVertex3fv(start_points[i]);
		qglVertex3fv(end_points[i]);
		qglVertex3fv(start_points[(i+1)%NUM_BEAM_SEGS]);
		qglVertex3fv(end_points[(i+1)%NUM_BEAM_SEGS]);
	}

	qglEnd();
	qglEnable(GL_TEXTURE_2D);
	GLSTATE_DISABLE_BLEND
	qglDepthMask(GL_TRUE);
}

//===================================================================

struct model_s	*R_RegisterModel (const char *name);
void	R_RegisterSkin (const char *name, struct model_s *model, struct image_s **skins); // jitskm
struct image_s	*Draw_FindPic (const char *name);
void	R_BeginRegistration (const char *map);
void	R_SetSky (const char *name, float rotate, vec3_t axis);
void	R_EndRegistration (void);
void	R_RenderFrame (refdef_t *fd);
void	Draw_Pic (int x, int y, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_TileClear2 (int x, int y, int w, int h, image_t *image);
void	Draw_StretchPic2 (int x, int y, int w, int h, image_t *gl);
void	Draw_Pic2 (int x, int y, image_t *gl);
void	Draw_String (int x, int y, const char *str); // jit, shush little warning
void	Draw_StringAlpha (int x, int y, const char *str, float alhpa); // jit
int		Draw_GetStates (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI
@@@@@@@@@@@@@@@@@@@@@
*/

refexport_t GetRefAPI (refimport_t rimp)
{
	refexport_t	re;

	ri = rimp;
	re.api_version = API_VERSION;
	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;
	re.RenderFrame = R_RenderFrame;
	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;
	re.DrawStretchRaw = Draw_StretchRaw;
	re.DrawFindPic = Draw_FindPic;
	re.DrawPic2 = Draw_Pic2;
	re.DrawStretchPic2 = Draw_StretchPic2;
	re.DrawTileClear2 = Draw_TileClear2;
	re.DrawString = Draw_String;
	re.DrawStringAlpha = Draw_StringAlpha;
	re.DrawGetStates = Draw_GetStates;
	re.Init = R_Init;
	re.Shutdown = R_Shutdown;
	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;
	re.AppActivate = GLimp_AppActivate;
	Swap_Init();

	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error(char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	_vsnprintf(text, sizeof(text), error, argptr); // jitsecurity -- prevent buffer overruns
	va_end(argptr);
	NULLTERMINATE(text); // jitsecurity -- make sure string is null terminated.

	ri.Sys_Error(ERR_FATAL, "%s", text);
}

void Com_Printf(char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	_vsnprintf(text, sizeof(text), fmt, argptr); // jitsecurity -- prevent buffer overruns
	va_end(argptr);
	NULLTERMINATE(text); // jitsecurity -- make sure string is null terminated.

	ri.Con_Printf(PRINT_ALL, "%s", text);
}

#endif
