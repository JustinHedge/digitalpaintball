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

// gl_script.c - scripted texture rendering - MrG

#include "gl_local.h"
#include <io.h>

void CIN_FreeCin (int texnum);

extern float	r_turbsin[];

#define		TURBSCALE (256.0 / (2 * M_PI))

#define		TOK_DELIMINATORS "\r\n\t "

float		rs_realtime = 0;
rscript_t	*rs_rootscript = NULL;

void RS_ListScripts (void)
{
	rscript_t	*rs;
	int			i;

	for (i = 0, rs = rs_rootscript; rs; rs = rs->next, i++) {
		ri.Con_Printf (PRINT_ALL, ">> %s\n", rs->name);
	}
}

int RS_Animate (rs_stage_t *stage)
{
	anim_stage_t	*anim = stage->last_anim;
	float			time = rs_realtime * 1000 - 
		(stage->last_anim_time + stage->anim_delay);

	while (stage->last_anim_time < rs_realtime) {
		anim = anim->next;
		if (!anim)
			anim = stage->anim_stage;
		stage->last_anim_time += stage->anim_delay;
	}

	stage->last_anim = anim;

	return anim->texture->texnum;
}

void RS_ResetScript (rscript_t *rs)
{
	rs_stage_t		*stage = rs->stage, *tmp_stage;
	anim_stage_t	*anim, *tmp_anim;
	
	if(rs->img_ptr) // jitrscript
		rs->img_ptr->rscript = NULL;

	rs->name[0] = 0;
	
	while (stage != NULL)
	{
		if (stage->anim_count)
		{
			anim = stage->anim_stage;
			while (anim != NULL)
			{
				tmp_anim = anim;

				if (anim->texture)
					if (anim->texture->is_cin)
						CIN_FreeCin(anim->texture->texnum);

				anim = anim->next;
				free(tmp_anim);
			}
		}

		tmp_stage = stage;
		stage = stage->next;
		free(tmp_stage);
	}

	/*rs->mirror = false;
	rs->stage = NULL;
	rs->dontflush = false;
	rs->subdivide = 0;
	rs->warpdist = 0.0f;
	rs->warpsmooth = 0.0f;
	rs->ready = false;*/
	memset(rs, 0, sizeof(rscript_t)); // jitrscript -- make sure we clear everything out.
}

#if 0
void RS_ClearStage (rs_stage_t *stage)
{
#if 1 // jitrscript -- rewritten
	anim_stage_t	*anim, *tmp_anim;
	anim = stage->anim_stage;

	while (anim != NULL) {
		tmp_anim = anim;
		anim = anim->next;
		free (tmp_anim);
	}

	memset(stage, 0, sizeof(rs_stage_t)); // clear EVERYTHING

	stage->lightmap = true;

#else // MrG's old code:
	anim_stage_t	*anim = stage->anim_stage, *tmp_anim;

	stage->alphashift.max = stage->alphashift.min 
		= stage->alphashift.speed = 0;
	stage->anim_delay = 0;

	while (anim != NULL) {
		tmp_anim = anim;
		anim = anim->next;
		free (tmp_anim);
	}

	stage->anim_stage = NULL;
		
	stage->blendfunc.blend = false;
	stage->blendfunc.dest = stage->blendfunc.source = 0;

	stage->last_anim = 0;
	stage->last_anim_time = 0;
	stage->anim_count = 0;

	stage->rot_speed = 0;
	stage->scroll.speedX = 0;
	stage->scroll.speedY = 0;
	stage->scroll.typeX = 0;
	stage->scroll.typeY = 0;
	stage->texture = NULL;
	stage->envmap = false;
	stage->lightmap = true;
	stage->alphamask = false;
#endif
}
#endif

rscript_t *RS_NewScript (char *name)
{
	rscript_t	*rs;

	if (!rs_rootscript) {
		rs_rootscript = (rscript_t *)malloc(sizeof(rscript_t));
		rs = rs_rootscript;
	} else {
		rs = rs_rootscript;

		while (rs->next != NULL)
			rs = rs->next;

		rs->next = (rscript_t *)malloc(sizeof(rscript_t));
		rs = rs->next;
	}

	strncpy (rs->name, name, sizeof(rs->name));

	rs->stage = NULL;
	rs->next = NULL;
	rs->dontflush = false;
	rs->subdivide = 0;
	rs->warpdist = 0.0f;
	rs->warpsmooth = 0.0f;
	rs->ready = false;
	rs->mirror = false;

	return rs;
}

rs_stage_t *RS_NewStage (rscript_t *rs)
{
	rs_stage_t	*stage;

	if (rs->stage == NULL) {
		rs->stage = (rs_stage_t *)malloc(sizeof(rs_stage_t));
		stage = rs->stage;
	} else {
		stage = rs->stage;
		while (stage->next != NULL)
			stage = stage->next;
		stage->next = (rs_stage_t *)malloc(sizeof(rs_stage_t));
		stage = stage->next;
	}

	///*jit stage->anim_stage = NULL;
	//stage->next = NULL;
	//stage->last_anim = NULL;

	//RS_ClearStage (stage); 
	//*/

	// jitrscript:
	memset(stage, 0, sizeof(rs_stage_t)); // clear EVERYTHING
	stage->lightmap = true;
	//strncpy (stage->name, "pics/noimage.tga", sizeof(stage->name));
	strncpy (stage->name, "pics/noimage", sizeof(stage->name));

	return stage;
}

void RS_FreeAllScripts (void)
{
	rscript_t	*rs = rs_rootscript, *tmp_rs;

	while (rs != NULL)
	{
		tmp_rs = rs->next;
		RS_ResetScript(rs);
		free(rs);
		rs = tmp_rs;
	}

	rs_rootscript = NULL; // jitrscript
}

void RS_FreeScript(rscript_t *rs)
{
	rscript_t	*tmp_rs;

	if (!rs)
		return;

	if (rs_rootscript == rs)
	{
		rs_rootscript = rs_rootscript->next;
		RS_ResetScript(rs);
		free(rs);
		return;
	}

	tmp_rs = rs_rootscript;

	while (tmp_rs->next != rs)
		tmp_rs = tmp_rs->next;

	tmp_rs->next = rs->next;

	RS_ResetScript(rs);
	free(rs);
}

void RS_FreeUnmarked (void)
{
	rscript_t	*rs = rs_rootscript, *tmp_rs;

	while (rs != NULL) {
		tmp_rs = rs->next;

		//if (!rs->dontflush)
		if (!rs->dontflush && !rs->img_ptr) // jitrscript
			RS_FreeScript(rs);

		rs = tmp_rs;
	}
}

rscript_t *RS_FindScript(char *name)
{
	rscript_t	*rs = rs_rootscript;

	while (rs != NULL)
	{
		if (!Q_strcasecmp(rs->name, name))
			return rs;

		rs = rs->next;
	}

	return NULL;
}

void RS_ReadyScript (rscript_t *rs)
{
	rs_stage_t		*stage;
	anim_stage_t	*anim;
	char			mode;

	if (rs->ready)
		return;

	mode = (rs->dontflush) ? it_pic : it_wall;
	stage = rs->stage;

	while (stage != NULL) {
		anim = stage->anim_stage;

		while (anim != NULL) {
			anim->texture = GL_FindImage (anim->name, mode);
			if (!anim->texture)
				anim->texture = r_notexture;

			anim = anim->next;
		}

		if (stage->name[0]) {
			stage->texture = GL_FindImage (stage->name, mode);

			if (!stage->texture)
				stage->texture = r_notexture;
		}

		stage = stage->next;
	}

	rs->ready = true;
}

void RS_UpdateRegistration (void)
{
	rscript_t		*rs = rs_rootscript;
	rs_stage_t		*stage;
	anim_stage_t	*anim;
	int				mode;

	while (rs != NULL) {
		mode = (rs->dontflush) ? it_pic : it_wall;
		stage = rs->stage;

		while (stage != NULL) {
			anim = stage->anim_stage;

			while (anim != NULL) {
				anim->texture = GL_FindImage(anim->name,mode);
				if (!anim->texture)
					anim->texture = r_notexture;
				anim = anim->next;
			}

			if (stage->texture) {
				stage->texture = GL_FindImage(stage->name,mode);

				if (!stage->texture)
					stage->texture = r_notexture;
			}

			stage = stage->next;
		}

		rs = rs->next;
	}
}

int RS_BlendID (char *blend)
{
	if (!blend[0])
		return 0;
	if (!Q_strcasecmp(blend, "GL_ZERO"))
		return GL_ZERO;
	if (!Q_strcasecmp(blend, "GL_ONE"))
		return GL_ONE;
	if (!Q_strcasecmp(blend, "GL_DST_COLOR"))
		return GL_DST_COLOR;
	if (!Q_strcasecmp(blend, "GL_ONE_MINUS_DST_COLOR"))
		return GL_ONE_MINUS_DST_COLOR;
	if (!Q_strcasecmp(blend, "GL_SRC_ALPHA"))
		return GL_SRC_ALPHA;
	if (!Q_strcasecmp(blend, "GL_ONE_MINUS_SRC_ALPHA"))
		return GL_ONE_MINUS_SRC_ALPHA;
	if (!Q_strcasecmp(blend, "GL_DST_ALPHA"))
		return GL_DST_ALPHA;
	if (!Q_strcasecmp(blend, "GL_ONE_MINUS_DST_ALPHA"))
		return GL_ONE_MINUS_DST_ALPHA;
	if (!Q_strcasecmp(blend, "GL_SRC_ALPHA_SATURATE"))
		return GL_SRC_ALPHA_SATURATE;
	if (!Q_strcasecmp(blend, "GL_SRC_COLOR"))
		return GL_SRC_COLOR;
	if (!Q_strcasecmp(blend, "GL_ONE_MINUS_SRC_COLOR"))
		return GL_ONE_MINUS_SRC_COLOR;

	return 0;
}

int RS_FuncName (char *text)
{
	if (!Q_strcasecmp(text, "static"))			// static
		return RSCRIPT_STATIC; // jitrscript (defines)
	else if (!Q_strcasecmp(text, "sine"))		// sine wave
		return RSCRIPT_SINE;
	else if (!Q_strcasecmp(text, "cosine"))	// cosine wave
		return RSCRIPT_COSINE;
	else if (!Q_strcasecmp(text, "sinabs"))		// sine wave, only positive - jitrscript
		return RSCRIPT_SINABS;
	else if (!Q_strcasecmp(text, "cosabs"))		// cosine wave, only positive - jitrscript
		return RSCRIPT_COSABS;

	return 0;
}

/*
scriptname
{
	subdivide <size>
	vertexwarp <speed> <distance> <smoothness>
	safe
	{
		map <texturename>
		scroll <xtype> <xspeed> <ytype> <yspeed>
		blendfunc <source> <dest>
		alphashift <speed> <min> <max>
		anim <delay> <tex1> <tex2> <tex3> ... end
		envmap
		nolightmap
		alphamask
	}
}
*/

void rs_stage_map (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);

	strncpy (stage->name, *token, sizeof(stage->name));
}

void rs_stage_scroll (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.typeX = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.speedX = atof(*token);
	
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.typeY = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.speedY = atof(*token);
}

void rs_stage_blendfunc (rs_stage_t *stage, char **token)
{
	stage->blendfunc.blend = true;

	*token = strtok (NULL, TOK_DELIMINATORS);

	if (!Q_strcasecmp(*token, "add"))
	{
		stage->blendfunc.source = GL_ONE;
		stage->blendfunc.dest = GL_ONE;
	}
	else if (!Q_strcasecmp(*token, "blend"))
	{
		stage->blendfunc.source = GL_SRC_ALPHA;
		stage->blendfunc.dest = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_strcasecmp(*token, "filter"))
	{
		stage->blendfunc.source = GL_ZERO;
		stage->blendfunc.dest = GL_SRC_COLOR;
	}
	else
	{
		stage->blendfunc.source = RS_BlendID (*token);

		*token = strtok (NULL, TOK_DELIMINATORS);
		stage->blendfunc.dest = RS_BlendID (*token);
	}
}

void rs_stage_alphashift (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.speed = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.min = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.max = (float)atof(*token);
}

void rs_stage_anim (rs_stage_t *stage, char **token)
{
	anim_stage_t	*anim = (anim_stage_t *)malloc(sizeof(anim_stage_t));

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->anim_delay = (float)atof(*token);

	stage->anim_stage = anim;
	stage->last_anim = anim;

	*token = strtok(NULL, TOK_DELIMINATORS);
	
	while (Q_strcasecmp(*token, "end"))
	{
		stage->anim_count++;

		strncpy (anim->name, *token, sizeof(anim->name));

		anim->texture = NULL;

		*token = strtok(NULL, TOK_DELIMINATORS);

		if (!Q_strcasecmp(*token, "end"))
		{
			anim->next = NULL;
			break;
		}

		anim->next = (anim_stage_t *)malloc(sizeof(anim_stage_t));
		anim = anim->next;
	}
}

void rs_stage_envmap (rs_stage_t *stage, char **token)
{
	stage->envmap = true;
}

void rs_stage_nolightmap (rs_stage_t *stage, char **token)
{
	stage->lightmap = false;
}

void rs_stage_alphamask (rs_stage_t *stage, char **token)
{
	stage->alphamask = true;
}

void rs_stage_rotate (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->rot_speed = (float)atof(*token);
}

// scaleadd adds to the normal scale offering more dynamic options to scaling.
void rs_stage_scaleadd (rs_stage_t *stage, char **token) // jitrscript
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scaleadd.typeX = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scaleadd.scaleX = atof(*token);
	
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scaleadd.typeY = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scaleadd.scaleY = atof(*token);
}

void rs_stage_scale (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.typeX = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.scaleX = atof(*token);
	
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.typeY = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.scaleY = atof(*token);
}

void rs_stage_offset (rs_stage_t *stage, char **token) // jitrscript
{
	*token = strtok(NULL, TOK_DELIMINATORS);
	stage->offset.offsetX = atof(*token);
	
	*token = strtok(NULL, TOK_DELIMINATORS);
	stage->offset.offsetY = atof(*token);
}

static rs_stagekey_t rs_stagekeys[] = 
{
	{	"map",			&rs_stage_map			},
	{	"scroll",		&rs_stage_scroll		},
	{	"blendfunc",	&rs_stage_blendfunc		},
	{	"alphashift",	&rs_stage_alphashift	},
	{	"anim",			&rs_stage_anim			},
	{	"envmap",		&rs_stage_envmap		},
	{	"nolightmap",	&rs_stage_nolightmap	},
	{	"alphamask",	&rs_stage_alphamask		},
	{	"rotate",		&rs_stage_rotate		},
	{	"scale",		&rs_stage_scale			},
	{	"scaleadd",		&rs_stage_scaleadd		}, // jitrscript
	{	"offset",		&rs_stage_offset		}, // jitrscript

	{	NULL,			NULL					}
};

static int num_stagekeys = sizeof (rs_stagekeys) / sizeof(rs_stagekeys[0]) - 1;

// =====================================================

void rs_script_safe (rscript_t *rs, char **token)
{
	rs->dontflush = true;
}

void rs_script_subdivide (rscript_t *rs, char **token)
{
	int divsize, p2divsize;

	*token = strtok (NULL, TOK_DELIMINATORS);
	divsize = atoi (*token);
 
	// cap max & min subdivide sizes
	if (divsize > 128)
		divsize = 128;
	else if (divsize <= 8)
		divsize = 8;

	// find the next smallest valid ^2 size, if not already one
	for (p2divsize = 2; p2divsize <= divsize ; p2divsize <<= 1 );

	p2divsize >>= 1;

	rs->subdivide = (char)p2divsize;
}

void rs_script_vertexwarp (rscript_t *rs, char **token)
{
	*token = strtok(NULL, TOK_DELIMINATORS);
	rs->warpspeed = atof (*token);
	*token = strtok(NULL, TOK_DELIMINATORS);
	rs->warpdist = atof (*token);
	*token = strtok(NULL, TOK_DELIMINATORS);
	rs->warpsmooth = atof (*token);

	if (rs->warpsmooth < 0.001f)
		rs->warpsmooth = 0.001f;
	else if (rs->warpsmooth > 1.0f)
		rs->warpsmooth = 1.0f;
}

void rs_script_mirror (rscript_t *rs, char **token)
{
	// TODO
	rs->mirror = true;
}

static rs_scriptkey_t rs_scriptkeys[] = 
{
	{	"safe",			&rs_script_safe			},
	{	"subdivide",	&rs_script_subdivide	},
	{	"vertexwarp",	&rs_script_vertexwarp	},
	{	"mirror",		&rs_script_mirror		},

	{	NULL,			NULL					}
};

static int num_scriptkeys = sizeof (rs_scriptkeys) / sizeof(rs_scriptkeys[0]) - 1;

// =====================================================

void RS_LoadScript(char *script)
{
	qboolean		inscript = false, instage = false;
	char			ignored = 0;
	char			*token, *fbuf, *buf;
	rscript_t		*rs = NULL;
	rs_stage_t		*stage;
	unsigned char	tcmod = 0;
	unsigned int	len, i;

	len = ri.FS_LoadFile (script, (void **)&fbuf);

	if (!fbuf || len < 16) 
	{
		// jit -- removed -- don't want to confuse the newbies :)
//		ri.Con_Printf(PRINT_ALL,"Could not load script %s\n",script);
		return;
	}

	buf = (char *)malloc(len+1);
	memcpy (buf, fbuf, len);
	buf[len] = 0;

	ri.FS_FreeFile (fbuf);

	token = strtok (buf, TOK_DELIMINATORS);

	while (token != NULL) 
	{
		if (Q_streq(token, "/*") || Q_streq(token, "["))
		{
			ignored++;
		}
		//else if (!Q_strcasecmp(token, "*/") || !Q_strcasecmp(token, "]"))
		//{
		//	ignored--;
		//	token = strtok (NULL, TOK_DELIMINATORS); // jitrscript (don't make rscripts named "*/")
		//}

		if (!inscript && !ignored) 
		{
			if (!Q_strcasecmp(token, "{"))
			{
				inscript = true;
			}
			else
			{
				rs = RS_FindScript(token);

				if (rs)
				{
					image_t *image;

					// jitrscript: if we have a texture pointing to this, update the pointer!
					image = rs->img_ptr;

					RS_FreeScript(rs);
					rs = RS_NewScript(token);
					
					if(image) // jit
					{
						rs->img_ptr = image;
						image->rscript = rs;
					}
				}
				else
				{
					rs = RS_NewScript(token);
				}
			}
		}
		else if (inscript && !ignored)
		{
			if (!Q_strcasecmp(token, "}"))
			{
				if (instage)
				{
					instage = false;
				}
				else
				{
					inscript = false;
				}
			}
			else if (!Q_strcasecmp(token, "{"))
			{
				if (!instage)
				{
					instage = true;
					stage = RS_NewStage(rs);
				}
			}
			else
			{
				if (instage && !ignored)
				{
					for (i = 0; i < num_stagekeys; i++)
					{
						if (!Q_strcasecmp(rs_stagekeys[i].stage, token))
						{
							rs_stagekeys[i].func (stage, &token);
							break;
						}
					}
				}
				else
				{
					for (i = 0; i < num_scriptkeys; i++)
					{
						if (!Q_strcasecmp(rs_scriptkeys[i].script, token))
						{
							rs_scriptkeys[i].func (rs, &token);
							break;
						}
					}
				}
			}			
		}

		// jitrscript, moved: don't make rscripts named "*/"
		if (Q_streq(token, "*/") || Q_streq(token, "]"))
		{
			ignored--;
		}

		token = strtok (NULL, TOK_DELIMINATORS);
	}

	free(buf);
}

void RS_ScanPathForScripts (char *dir)
{
	char			script[MAX_OSPATH];
	char			dirstring[1024], *c;
	int				handle;
	struct			_finddata_t fileinfo;

	Com_sprintf (dirstring, sizeof(dirstring), "%s/scripts/*.txt", dir);
	handle = _findfirst (dirstring, &fileinfo);

	if (handle != -1) {
		do {
			if (fileinfo.name[0] == '.')
				continue;

			c = COM_SkipPath(fileinfo.name);
			Com_sprintf(script, MAX_OSPATH, "scripts/%s", c);
			RS_LoadScript (script);
		} while (_findnext( handle, &fileinfo ) != -1);

		_findclose (handle);
	}
}

_inline void RS_RotateST (float *os, float *ot, float degrees, msurface_t *fa)
{
	float cost = cos(degrees), sint = sin(degrees);
	float is = *os, it = *ot;

	*os = cost * (is - fa->c_s) + sint * (fa->c_t - it) + fa->c_s;
	*ot = cost * (it - fa->c_t) + sint * (is - fa->c_s) + fa->c_t;
}

_inline void RS_SetEnvmap (vec3_t v, float *os, float *ot)
{
	vec3_t vert;
	
	vert[0] = v[0]*r_world_matrix[0]+v[1]*r_world_matrix[4]+v[2]*r_world_matrix[8] +r_world_matrix[12];
	vert[1] = v[0]*r_world_matrix[1]+v[1]*r_world_matrix[5]+v[2]*r_world_matrix[9] +r_world_matrix[13];
	vert[2] = v[0]*r_world_matrix[2]+v[1]*r_world_matrix[6]+v[2]*r_world_matrix[10]+r_world_matrix[14];

	VectorNormalize (vert);
	
	*os = vert[0];
	*ot = vert[1];
}

void RS_SetTexcoords (rs_stage_t *stage, float *os, float *ot, msurface_t *fa)
{
	float	txm = 0, tym = 0;

	// scale
	if (stage->scale.scaleX)
	{
		switch (stage->scale.typeX)
		{
		case RSCRIPT_STATIC:	// static
			*os *= stage->scale.scaleX;
			break;
		case RSCRIPT_SINE:	// sine
			*os *= stage->scale.scaleX*sin(rs_realtime*0.05);
			break;
		case RSCRIPT_COSINE:	// cosine
			*os *= stage->scale.scaleX*cos(rs_realtime*0.05);
			break;
		}
	}

	if (stage->scale.scaleY)
	{
		switch (stage->scale.typeY)
		{
		case RSCRIPT_STATIC:	// static
			*ot *= stage->scale.scaleY;
			break;
		case RSCRIPT_SINE:	// sine
			*ot *= stage->scale.scaleY*sin(rs_realtime*0.05);
			break;
		case RSCRIPT_COSINE:	// cosine
			*ot *= stage->scale.scaleY*cos(rs_realtime*0.05);
			break;
		}
	}

	// rotate
	if (stage->rot_speed)
		RS_RotateST (os, ot, -stage->rot_speed * rs_realtime * 0.0087266388888888888888888888888889, fa);
}

_inline void RS_RotateST2 (float *os, float *ot, float degrees)
{
	float cost = cos(degrees), sint = sin(degrees);
	float is = *os, it = *ot;

	*os = cost * (is - 0.5) + sint * (0.5 - it) + 0.5;
	*ot = cost * (it - 0.5) + sint * (is - 0.5) + 0.5;
}

void RS_SetTexcoords2D (rs_stage_t *stage, float *os, float *ot)
{
	float	txm = 0, tym = 0;

	*os += stage->offset.offsetX; // jitrscript
	*ot += stage->offset.offsetY; // jitrscript

	// scale
	if (stage->scale.scaleX)
	{
		switch (stage->scale.typeX)
		{
		case RSCRIPT_STATIC:	// static
			*os *= stage->scale.scaleX;
			break;
		case RSCRIPT_SINE:	// sine
			*os *= stage->scale.scaleX*sin(rs_realtime*0.05);
			break;
		case RSCRIPT_COSINE:	// cosine
			*os *= stage->scale.scaleX*cos(rs_realtime*0.05);
			break;
		case RSCRIPT_SINABS: // jitrscript
			*os *= stage->scale.scaleX*(sin(rs_realtime*0.05)+1.0);
			break;
		case RSCRIPT_COSABS: // jitrscript
			*os *= stage->scale.scaleX*(cos(rs_realtime*0.05)+1.0);
			break;
		}
	}

	if (stage->scale.scaleY)
	{
		switch (stage->scale.typeY)
		{
		case RSCRIPT_STATIC:	// static
			*ot *= stage->scale.scaleY;
			break;
		case RSCRIPT_SINE:	// sine
			*ot *= stage->scale.scaleY*sin(rs_realtime*0.05);
			break;
		case RSCRIPT_COSINE:	// cosine
			*ot *= stage->scale.scaleY*cos(rs_realtime*0.05);
			break;
		case RSCRIPT_SINABS: // jitrscript
			*ot *= stage->scale.scaleY*(sin(rs_realtime*0.05)+1.0);
			break;
		case RSCRIPT_COSABS: // jitrscript
			*ot *= stage->scale.scaleY*(cos(rs_realtime*0.05)+1.0);
			break;
		}
	}

	// scaleadd -- jitrscript
	if (stage->scaleadd.scaleX) // jitrscript
	{
		switch (stage->scaleadd.typeX) 
		{
		case 0:	// static
			if(*os > 0)
				*os += stage->scaleadd.scaleX;
			else
				*os -= stage->scaleadd.scaleX;
			break;
		case 1:	// sine (probably won't get used, but just for completeness)
			if(*os > 0)
				*os += stage->scaleadd.scaleX*sin(rs_realtime*0.05);
			else
				*os -= stage->scaleadd.scaleX*sin(rs_realtime*0.05);
			break;
		case 2:	// cosine
			if(*os > 0)
				*os += stage->scaleadd.scaleX*cos(rs_realtime*0.05);
			else
				*os -= stage->scaleadd.scaleX*cos(rs_realtime*0.05);
			break;
		}
	}

	if (stage->scaleadd.scaleY) // jitrscript
	{
		switch (stage->scaleadd.typeY)
		{
		case 0:	// static
			if(*ot > 0)
				*ot += stage->scaleadd.scaleY;
			else
				*ot -= stage->scaleadd.scaleY;
			break;
		case 1:	// sine
			if(*ot > 0)
				*ot += stage->scaleadd.scaleY*sin(rs_realtime*0.05);
			else
				*ot -= stage->scaleadd.scaleY*sin(rs_realtime*0.05);
			break;
		case 2:	// cosine
			if(*ot > 0)
				*ot += stage->scaleadd.scaleY*cos(rs_realtime*0.05);
			else
				*ot -= stage->scaleadd.scaleY*cos(rs_realtime*0.05);
			break;
		}
	}

	// rotate
	if (stage->rot_speed)
		RS_RotateST2 (os, ot, -stage->rot_speed * rs_realtime * 0.0087266388888888888888888888888889);
}


qboolean alphasurf = false; // jitrscript

void RS_DrawSurface (msurface_t *surf, qboolean lightmap, rscript_t *rs) // jitrscript
{
	glpoly_t	*p;
	float		*v;
	int			i, nv = surf->polys->numverts;
	vec3_t		wv;
	//rscript_t	*rs = (rscript_t *)surf->texinfo->script;
	rs_stage_t	*stage = rs->stage;
	float		os, ot, alpha;
	float		scale, time = rs_realtime * rs->warpspeed, txm, tym;
	qboolean	firststage = true; // jitrscript
	
	if(!rs)
		rs = (rscript_t *)surf->texinfo->script; // jitrscript

	do
	{
		if (stage->anim_count)
			GL_MBind (GL_TEXTURE0, RS_Animate(stage));
		else
			GL_MBind (GL_TEXTURE0, stage->texture->texnum);

		// sane defaults
		alpha = 1.0f;

		if (stage->blendfunc.blend && (alphasurf || !firststage))
		{
			qglBlendFunc(stage->blendfunc.source, stage->blendfunc.dest);
			GLSTATE_ENABLE_BLEND
		}
		else
		{
			GLSTATE_DISABLE_BLEND
		}

		if ((stage->alphashift.min || stage->alphashift.speed) &&
			(alphasurf || !firststage))
		{
			if (!stage->alphashift.speed && stage->alphashift.min > 0)
			{
				alpha = stage->alphashift.min;
			}
			else if (stage->alphashift.speed)
			{
				alpha = sin (rs_realtime * stage->alphashift.speed);
				alpha = (alpha + 1)*0.5f;

				if (alpha > stage->alphashift.max) 
					alpha = stage->alphashift.max;
				if (alpha < stage->alphashift.min) 
					alpha = stage->alphashift.min;
			}
		}
		else
			alpha = 1.0f;

		if (stage->scroll.speedX)
		{
			switch (stage->scroll.typeX)
			{
			case 0:	// static
				txm = rs_realtime*stage->scroll.speedX;
				break;
			case 1:	// sine
				txm = sin (rs_realtime*stage->scroll.speedX);
				break;
			case 2:	// cosine
				txm = cos (rs_realtime*stage->scroll.speedX);
				break;
			}
		}
		else
			txm=0;
	
		if (stage->scroll.speedY)
		{
			switch (stage->scroll.typeY)
			{
			case 0:	// static
				tym = rs_realtime*stage->scroll.speedY;
				break;
			case 1:	// sine
				tym = sin (rs_realtime*stage->scroll.speedY);
				break;
			case 2:	// cosine
				tym = cos (rs_realtime*stage->scroll.speedY);
				break;
			}
		}
		else
			tym=0;

		qglColor4f (1, 1, 1, alpha);

		if (stage->envmap)
			GL_TexEnv( GL_MODULATE );

		if (stage->alphamask && (alphasurf || !firststage))
		{
			GLSTATE_ENABLE_ALPHATEST
		}
		else
		{
			GLSTATE_DISABLE_ALPHATEST
		}

		if (rs->subdivide)
		{
			glpoly_t *bp;
			int i;

			for (bp = surf->polys; bp; bp = bp->next)
			{
				p = bp;

				if (stage->envmap)
				{
//					qglTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
//					qglTexGenf(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
//					GLSTATE_ENABLE_TEXGEN
				}
				qglBegin(GL_TRIANGLE_FAN);
				for (i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE)
				{
					if (stage->envmap)
					{
						RS_SetEnvmap (v, &os, &ot);
					}
					else
					{
						os = v[3];
						ot = v[4];

						RS_SetTexcoords (stage, &os, &ot, surf);
					}

					if (lightmap) {
						qglMTexCoord2fSGIS(GL_TEXTURE0, os+txm, ot+tym);
						qglMTexCoord2fSGIS(GL_TEXTURE1, v[5], v[6]);
					} else {
						qglTexCoord2f (os+txm, ot+tym); // jitrscript (added txm/tym)
					}

					if (!rs->warpsmooth)
						qglVertex3fv (v);
					else {
						scale = rs->warpdist * sin(v[0]*rs->warpsmooth+time)*sin(v[1]*rs->warpsmooth+time)*sin(v[2]*rs->warpsmooth+time);
						VectorMA (v, scale, surf->plane->normal, wv);
						qglVertex3fv (wv);
					}
				}

				qglEnd();
			}
		} else {
			for (p = surf->polys; p; p = p->chain)
			{
//				if (stage->envmap)
//				{
//					qglTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
//					qglTexGenf(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
//					GLSTATE_ENABLE_TEXGEN
//				}

				qglBegin (GL_TRIANGLE_FAN);

				for (i = 0, v = p->verts[0]; i < nv; i++, v += VERTEXSIZE)
				{
					if (stage->envmap)
					{
						RS_SetEnvmap (v, &os, &ot);
					}
					else
					{
						os = v[3];
						ot = v[4];

						RS_SetTexcoords (stage, &os, &ot, surf);
					}

					if (lightmap) {
						qglMTexCoord2fSGIS(GL_TEXTURE0, os+txm, ot+tym);
						qglMTexCoord2fSGIS(GL_TEXTURE1, v[5], v[6]);
					} else {
						qglTexCoord2f (os+txm, ot+tym); // jitrscript (added txm/tym)
					}

					if (!rs->warpsmooth)
						qglVertex3fv (v);
					else {
						scale = rs->warpdist*sin(v[0]*rs->warpsmooth+time)*sin(v[1]*rs->warpsmooth+time)*sin(v[2]*rs->warpsmooth+time);
						VectorMA (v, scale, surf->plane->normal, wv);
						qglVertex3fv (wv);
					}
				}

				qglEnd ();
			}
		}

		if (stage->envmap)
			GL_TexEnv(GL_REPLACE);

		qglColor4f(1, 1, 1, 1);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		GLSTATE_DISABLE_BLEND
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_DISABLE_TEXGEN
		firststage = false; // jitrscript
	} while (stage = stage->next);
}

