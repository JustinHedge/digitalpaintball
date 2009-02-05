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

// draw.c

#include "gl_local.h"

image_t		*draw_chars = NULL;
byte		*char_colors = NULL; // jittext

extern	qboolean	scrap_dirty;
extern cvar_t *cl_hudscale; //jithudscale
extern cvar_t *cl_crosshairscale; // viciouz - crosshair scale
void Scrap_Upload (void);
extern cvar_t	*gl_textshadow; // jittext

// vertex arrays
float	tex_array[MAX_ARRAY][2];
float	vert_array[MAX_ARRAY][3];
float	col_array[MAX_ARRAY][4];



/*
===============
Draw_InitLocal
===============
*/

void Draw_InitLocal (void)
{
	int width, height;
	void LoadTGA (char *name, byte **pic, int *width, int *height);

	//draw_chars = GL_FindImage("pics/conchars.pcx", it_pic);
	if (ri.Cvar_Get("gl_overbright", "1", CVAR_ARCHIVE)->value && gl_state.texture_combine)
		draw_chars = GL_FindImage("pics/conchars1ovb.tga", it_pic); // dark conchars (brightness doubled)
	else
		draw_chars = GL_FindImage("pics/conchars1.tga", it_pic); // jitconsole

	GL_Bind(draw_chars->texnum);
	//qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	LoadTGA("pics/char_colors.tga", &char_colors, &width, &height); // jittext

	if (!char_colors || (width*height != 256))
		Sys_Error("Invalid or missing char_colors.tga.");
}


void Draw_InitLocalOld (void) // jitest
{
	int width, height;
	void LoadTGA (char *name, byte **pic, int *width, int *height);

	// load console characters (don't bilerp characters)
	draw_chars = GL_FindImage("pics/conchars.pcx", it_pic);
	GL_Bind(draw_chars->texnum);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	LoadTGA("pics/char_colors.tga", &char_colors, &width, &height); // jittext
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num) // jitodo -- try to remove all calls to this, use draw_string
{
	int				row, col;
	float			frow, fcol, size;
	int textscale;

	textscale = (int)cl_hudscale->value; // jithudscale
	num &= 255;
	
	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num >> 4;
	col = num & 15;
	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625f;
	GLSTATE_DISABLE_ALPHATEST // jitconsole
	GLSTATE_ENABLE_BLEND // jitconsole
	GL_Bind(draw_chars->texnum);

#ifdef BEEFQUAKERENDER // jit3dfx
	VA_SetElem2(tex_array[0],fcol, frow);
	VA_SetElem2(vert_array[0],x, y);
	VA_SetElem2(tex_array[1],fcol + size, frow);
	VA_SetElem2(vert_array[1],x+8*textscale, y);
	VA_SetElem2(tex_array[2],fcol + size, frow + size);
	VA_SetElem2(vert_array[2],x+8*textscale, y+8*textscale);
	VA_SetElem2(tex_array[3],fcol, frow + size);
	VA_SetElem2(vert_array[3],x, y+8*textscale);
	qglDrawArrays (GL_QUADS, 0, 4);
#else
	qglBegin(GL_QUADS);
	qglTexCoord2f(fcol, frow);
	qglVertex2f(x, y);
	qglTexCoord2f(fcol + size, frow);
	qglVertex2f(x + 8 * textscale, y); // jithudscale...
	qglTexCoord2f(fcol + size, frow + size);
	qglVertex2f(x + 8 * textscale, y+8*textscale);
	qglTexCoord2f(fcol, frow + size);
	qglVertex2f(x, y + 8 * textscale);
	qglEnd();
#endif
}

//#define CHAR_UNDERLINE_NUM 158
#define CHAR_UNDERLINE_NUM 2

//float redtext[] = { 1.0f, .8f, .5f };
//float whitetext[] = { 1.0f, 1.0f, 1.0f };
void Draw_StringAlpha (int x, int y, const char *str, float alpha) // jit
{
	int				px,py,row, col,num;
	float			frow, fcol, size;
	const char		*s = str; // jit, shush little warning
	int				textscale; // jithudscale
	int				coloredtext = 0;
	qboolean		nextiscolor = false;
	qboolean		shadowpass = false;
	qboolean		passagain = false;
	qboolean		italicized = false;
	qboolean		underlined = false;

	if (!*s)
		return;

	textscale = (int)cl_hudscale->value;

	if (gl_state.currenttextures[gl_state.currenttmu] != draw_chars->texnum)
		GL_Bind(draw_chars->texnum);

	GLSTATE_DISABLE_ALPHATEST // jitconsole
	GLSTATE_ENABLE_BLEND // jitconsole
	GL_TexEnv(GL_COMBINE_EXT); // jittext (brighter text now)
	size = 0.0625;

	if (gl_textshadow->value)
	{
		shadowpass = true;
		px = x + textscale;
		py = y + textscale;
	}
	else
	{
		px = x;
		py = y;
	}

	qglBegin(GL_QUADS);

	do
	{
		if (shadowpass)
			qglColor4f(0.0f, 0.0f, 0.0f, alpha);

		while (*s)
		{
			num = *s;
			num &= 255;

			// ===[
			// jittext
			if (!nextiscolor)
			{
				switch (num)
				{
				case CHAR_COLOR:

					if (!(*(s+1))) // end of string
					{
						nextiscolor = false;
					}
					else
					{
						nextiscolor = true;
						coloredtext = 1;
						s++;
						continue;
					}

					break;
				case CHAR_UNDERLINE:
					s++;
					underlined = !underlined;

					//if (*s != '\0') // only draw if at end of string
						continue;
					//else // so the string null-terminates!
					//	s--;

					break;
				case CHAR_ITALICS:
					s++;
					italicized = !italicized;

					//if (*s != '\0') // only draw if at end of string
						continue;
					//else
					//	s--;

					break;
				case CHAR_ENDFORMAT:
					s++;
					italicized = false;
					underlined = false;

					if (!shadowpass)
						qglColor4f(1.0f, 1.0f, 1.0f, alpha);

					//if (*s != '\0') // draw character if at end of string.
						continue;
					//else
					//	s--;

					break;
				default:
					break;
				}
			}
			else // (nextiscolor)
			{
				// look up color in char_colors.tga:
				if (!shadowpass)
				{
					register int num4 = num * 4;

					qglColor4f(*(char_colors + num4) / 255.0f, 
						*(char_colors + num4 + 1) / 255.0f,
						*(char_colors + num4 + 2) / 255.0f, alpha);
				}

				nextiscolor = false;
				s++;
				continue;
			}
			// ]==

			if (py <= -8 * textscale)
			{	// totally off screen
				s++;
				px += 8 * textscale; //jithudscale
				continue;
			}

			if (underlined) // jitconsole
			{
				row = CHAR_UNDERLINE_NUM >> 4;
				col = CHAR_UNDERLINE_NUM & 15;
				frow = row * 0.0625;
				fcol = col * 0.0625;
				qglTexCoord2f(fcol, frow);
				qglVertex2f(px, py + 4 * textscale);
				qglTexCoord2f(fcol + size, frow);
				qglVertex2f(px + 8 * textscale, py + 4 * textscale); // jithudscale...
				qglTexCoord2f(fcol + size, frow + size);
				qglVertex2f(px + 8 * textscale, py + 12 * textscale);
				qglTexCoord2f(fcol, frow + size);
				qglVertex2f(px, py + 12 * textscale);
			}

			if ((num & 127) == 32)		// space
			{
				s++;
				px += 8 * textscale; //jithudscale
				continue;
			}

			row = num>>4;
			col = num&15;
			frow = row*0.0625;
			fcol = col*0.0625;

			if (italicized)
			{
				qglTexCoord2f (fcol, frow);
				qglVertex2f (px+2*textscale, py);
				qglTexCoord2f (fcol + size, frow);
				qglVertex2f (px+10*textscale, py); // jithudscale...
				qglTexCoord2f (fcol + size, frow + size);
				qglVertex2f (px+6*textscale, py+8*textscale);
				qglTexCoord2f (fcol, frow + size);
				qglVertex2f (px-2*textscale, py+8*textscale);
			}
			else
			{
				qglTexCoord2f (fcol, frow);
				qglVertex2f (px, py);
				qglTexCoord2f (fcol + size, frow);
				qglVertex2f (px+8*textscale, py); // jithudscale...
				qglTexCoord2f (fcol + size, frow + size);
				qglVertex2f (px+8*textscale, py+8*textscale);
				qglTexCoord2f (fcol, frow + size);
				qglVertex2f (px, py+8*textscale);
			}

			s++;
			px += 8 * textscale; //jithudscale
		}

		if (shadowpass)
		{
			nextiscolor = false;
			italicized = false;
			underlined = false;
			shadowpass = false;
			passagain = true;
			qglColor4f(1.0f, 1.0f, 1.0f, alpha);
			s = str;
			px = x;
			py = y;
		}
		else
		{
			passagain = false;
		}
	} while (passagain);

	qglEnd();

	if (coloredtext)
	{
		//qglColor3fv(whitetext);
		qglColor4f(1.0f, 1.0f, 1.0f, alpha);
	}

	GL_TexEnv(GL_MODULATE); // jittext
}


void Draw_String (int x, int y, const char *str)
{
	Draw_StringAlpha(x, y, str, 1.0f); // jit
}

int Draw_GetStates (void)
{
	int states = 0;

	qglPushAttrib(GL_ENABLE_BIT);
	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_FOG);
	qglBegin(GL_TRIANGLE_STRIP);
	qglEnd();
	states |= qglIsEnabled(GL_FOG) ? 1 : 0;
	states <<= 1;
	states |= qglIsEnabled(GL_BLEND) ? 1 : 0;
	states <<= 1;
	states |= qglIsEnabled(GL_ALPHA_TEST) ? 1 : 0;
	states <<= 1;
	states |= qglIsEnabled(GL_DEPTH_TEST) ? 1 : 0;
	states <<= 1;
	qglPopAttrib();

	return states;
}

/*
=============
Draw_FindPic
=============
*/
image_t	*Draw_FindPic (const char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = GL_FindImage(fullname, it_pic);
	}
	else
	{
		gl = GL_FindImage(name + 1, it_pic);
	}

	if (!gl) // jit -- remove "can't find pic" spam
		return r_notexture;
	else
		return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, char *pic)
{
	image_t *gl;

	if (!(gl = Draw_FindPic(pic)))
	{
		*w = *h = -1;
		return;
	}

	*w = gl->width;
	*h = gl->height;
}

/*
=============
Draw_StretchPic
=============
*/
void RS_SetTexcoords2D (rs_stage_t *stage, float *os, float *ot); // jit
void Draw_StretchPic2 (int x, int y, int w, int h, image_t *gl)
{
	rscript_t *rs;
	float	txm,tym, alpha,s,t;
	rs_stage_t *stage;

	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "NULL pic in Draw_StretchPic\n");
		return;
	}

	if (scrap_dirty)
		Scrap_Upload ();

	if (((gl_config.renderer == GL_RENDERER_MCD) ||
		(gl_config.renderer & GL_RENDERER_RENDITION)) && !gl->has_alpha)
	{
		GLSTATE_DISABLE_ALPHATEST
	}

	/*rs = RS_FindScript(gl->name); // jitrscript
	if (!rs)
		rs = RS_FindScript(gl->bare_name);*/
	rs = gl->rscript; // jitrscript


	if (!rs) 
	{
		//GLSTATE_ENABLE_ALPHATEST //jitodo / jitmenu - reenable this after rscripts for menu stuff are made.
		GLSTATE_DISABLE_ALPHATEST // jitodo (see above)
		GLSTATE_ENABLE_BLEND // jitodo (see above)
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_Bind(gl->texnum);

		qglBegin(GL_QUADS);

		qglTexCoord2f(gl->sl, gl->tl);
		qglVertex2f(x, y);
		qglTexCoord2f(gl->sh, gl->tl);
		qglVertex2f(x+w, y);
		qglTexCoord2f(gl->sh, gl->th);
		qglVertex2f(x+w, y+h);
		qglTexCoord2f(gl->sl, gl->th);
		qglVertex2f(x, y+h);

		qglEnd();
	}
	else
	{
		image_t *stage_pic; // jitrscript
		image_t *RS_Animate_image (rs_stage_t *stage); // jitrscript

		if (!rs->ready) // jit
			RS_ReadyScript(rs);

		stage=rs->stage;

		while (stage)
		{
			if (stage->anim_count)
			//	GL_Bind(RS_Animate(stage));
				stage_pic = RS_Animate_image(stage); // jitrscript
			else
			//	GL_Bind(stage->texture->texnum);
				stage_pic = stage->texture; // jitrscript

			GL_Bind(stage_pic->texnum);

			if (stage->scroll.speedX)
			{
				switch(stage->scroll.typeX)
				{
					case 0:	// static
						txm=rs_realtime*stage->scroll.speedX;
						break;
					case 1:	// sine
						txm=sin(rs_realtime*stage->scroll.speedX);
						break;
					case 2:	// cosine
						txm=cos(rs_realtime*stage->scroll.speedX);
						break;
				}
			}
			else
			{
				txm=0;
			}

			if (stage->scroll.speedY)
			{
				switch(stage->scroll.typeY)
				{
					case 0:	// static
						tym=rs_realtime*stage->scroll.speedY;
						break;
					case 1:	// sine
						tym=sin(rs_realtime*stage->scroll.speedY);
						break;
					case 2:	// cosine
						tym=cos(rs_realtime*stage->scroll.speedY);
						break;
				}
			}
			else
			{
				tym=0;
			}

			if (stage->blendfunc.blend)
			{
				qglBlendFunc(stage->blendfunc.source,stage->blendfunc.dest);
				GLSTATE_ENABLE_BLEND
			}
			else
			{
				GLSTATE_DISABLE_BLEND
			}

			if (stage->alphashift.min || stage->alphashift.speed)
			{
				if (!stage->alphashift.speed && stage->alphashift.min > 0)
				{
					alpha=stage->alphashift.min;
				}
				else if (stage->alphashift.speed)
				{
					alpha = sin(rs_realtime * stage->alphashift.speed);
					// jit alpha=(alpha+1)*0.5f;
					alpha = 0.5f * (alpha + 1) * 
						(stage->alphashift.max - stage->alphashift.min) +
						stage->alphashift.min; // jitrscript
					// jit if (alpha > stage->alphashift.max) alpha=stage->alphashift.max;
					// jit if (alpha < stage->alphashift.min) alpha=stage->alphashift.min;
				}
			}
			else
			{
				alpha=1.0f;
			}

			qglColor4f(1,1,1,alpha);
			GL_TexEnv(GL_MODULATE); // jitrscript

//			if (stage->envmap)
			if (stage->tcGen == TC_GEN_ENVIRONMENT)
			{
				qglTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
				qglTexGenf(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
				GLSTATE_ENABLE_TEXGEN
			}

			if (stage->alphamask)
			{
				GLSTATE_ENABLE_ALPHATEST
			}
			else
			{
				GLSTATE_DISABLE_ALPHATEST
			}

			qglBegin(GL_QUADS);

			s = stage_pic->sl;//0.0f;//gl->sl; //
			t = stage_pic->tl;//0.0f;//gl->tl; //
			RS_SetTexcoords2D(stage, &s, &t);
			qglTexCoord2f(s+txm, t+tym);
			qglVertex2f(x, y);

			s = stage_pic->sh;//1.0f;//gl->sh; //
			t = stage_pic->tl;//0.0f;//gl->tl;//
			RS_SetTexcoords2D(stage, &s, &t);
			qglTexCoord2f(s+txm, t+tym);
			qglVertex2f(x+w, y);

			s = stage_pic->sh;//1.0f;//gl->sh;//
			t = stage_pic->th;//1.0f;//gl->th;//
			RS_SetTexcoords2D(stage, &s, &t);
			qglTexCoord2f(s+txm, t+tym);
			qglVertex2f(x+w, y+h);

			s = stage_pic->sl;//0.0f;//gl->sl;//
			t = stage_pic->th;//1.0f;//gl->th;//
			RS_SetTexcoords2D(stage, &s, &t);
			qglTexCoord2f(s+txm, t+tym);
			qglVertex2f(x, y+h);

			qglEnd();

			//jit qglColor4f(1,1,1,1);
			//jit qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GLSTATE_DISABLE_TEXGEN

			stage=stage->next;
		}
		qglColor4f(1,1,1,1);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GLSTATE_ENABLE_ALPHATEST
		GLSTATE_DISABLE_BLEND
	}

	/*jit if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha) {
		GLSTATE_ENABLE_ALPHATEST
	}*/
}


void Draw_StretchPic (int x, int y, int w, int h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
	Draw_StretchPic2(x,y,w,h,gl);
}

/*
=============
Draw_Pic
=============
*/

void Draw_Pic2 (int x, int y, image_t *gl)
{
// jit - no sense in doing all the exact same stuff twice -- just call stretchpic.
	int picscale;

	picscale = (int)cl_hudscale->value; // jithudscale

	if (strstr(gl->name, "ch")) // fix this hackyness, it will probably cause problems
		picscale = (int)cl_crosshairscale->value; // viciouz - crosshair scale

	Draw_StretchPic2 (x, y, gl->width*picscale, gl->height*picscale, gl);
}

void Draw_Pic (int x, int y, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
	Draw_Pic2(x,y,gl);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear2 (int x, int y, int w, int h, image_t *image)
{
	if (!image)
	{
		ri.Con_Printf (PRINT_ALL, "NULL pic in Draw_TileClear\n");
		return;
	}

	if (((gl_config.renderer == GL_RENDERER_MCD) ||
		(gl_config.renderer & GL_RENDERER_RENDITION)) && !image->has_alpha)
	{
		GLSTATE_DISABLE_ALPHATEST
	}

	GL_Bind(image->texnum);

	qglBegin(GL_QUADS);
	qglTexCoord2f(x/256.0f, y/128.0f); // jit -- new pic dimensions
	qglVertex2f(x, y);
	qglTexCoord2f((x+w)/256.0f, y/128.0f);
	qglVertex2f(x+w, y);
	qglTexCoord2f((x+w)/256.0f, (y+h)/128.0f);
	qglVertex2f(x+w, y+h);
	qglTexCoord2f(x/256.0f, (y+h)/128.0f);
	qglVertex2f(x, y+h);
	qglEnd();

	if (((gl_config.renderer == GL_RENDERER_MCD) ||
		(gl_config.renderer & GL_RENDERER_RENDITION)) && !image->has_alpha)
	{
		GLSTATE_ENABLE_ALPHATEST
	}
}

void Draw_TileClear (int x, int y, int w, int h, char *pic)
{
	image_t	*image;

	image = Draw_FindPic(pic);

	if (!image)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	Draw_TileClear2(x,y,w,h,image);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/

void Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		unsigned	c;
		byte		v[4];
	} color;

	if ((unsigned)c > 255)
		ri.Sys_Error(ERR_FATAL, "Draw_Fill: bad color");

	qglDisable(GL_TEXTURE_2D);
	color.c = d_8to24table[c];
	qglColor3f(color.v[0]/255.0,
		color.v[1]/255.0,
		color.v[2]/255.0);
	qglBegin(GL_QUADS);
	qglVertex2f(x,y);
	qglVertex2f(x+w, y);
	qglVertex2f(x+w, y+h);
	qglVertex2f(x, y+h);
	qglEnd();
	qglColor3f(1,1,1);
	qglEnable(GL_TEXTURE_2D);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_ENABLE_BLEND
	qglDisable (GL_TEXTURE_2D);
	qglColor4f (0, 0, 0, 0.5);

	VA_SetElem2(vert_array[0],0,0);
	VA_SetElem2(vert_array[1],vid.width, 0);
	VA_SetElem2(vert_array[2],vid.width, vid.height);
	VA_SetElem2(vert_array[3],0, vid.height);
	qglDrawArrays (GL_QUADS, 0, 4);

	qglColor4f (1,1,1,1);
	qglEnable (GL_TEXTURE_2D);
	GLSTATE_DISABLE_BLEND
	GLSTATE_ENABLE_ALPHATEST
}


//====================================================================


/*
=============
Draw_StretchRaw
=============
*/
extern unsigned	r_rawpalette[256];

void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{
	unsigned	image32[256*256];
	unsigned char image8[256*256];
	int			i, j, trows;
	byte		*source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	GL_Bind(0);

	if (rows <= 256)
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows * 0.00390625; // /256.0;
		trows = 256;
	}

	t = rows * hscale * 0.00390625;// / 256;

	if (!qglColorTableEXT)
	{
		unsigned *dest;

		for (i = 0; i < trows; i++)
		{
			row = (int)(i * hscale);

			if (row > rows)
				break;

			source = data + cols*row;
			dest = &image32[i*256];
			fracstep = cols * 0x10000 / 256;
			frac = fracstep >> 1;

			for (j = 0; j < 256; j++)
			{
				dest[j] = r_rawpalette[source[frac>>16]];
				frac += fracstep;
			}
		}

		qglTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format, 256,
			256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	}
	else
	{
		unsigned char *dest;

		for (i = 0; i < trows; i++)
		{
			row = (int)(i * hscale);

			if (row > rows)
				break;

			source = data + cols * row;
			dest = &image8[i * 256];
			fracstep = cols * 0x10000 / 256;
			frac = fracstep >> 1;

			for (j = 0; j < 256; j++)
			{
				dest[j] = source[frac >> 16];
				frac += fracstep;
			}
		}

		qglTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, 256,
			256, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, image8);
	}

	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ((gl_config.renderer == GL_RENDERER_MCD) ||
		(gl_config.renderer & GL_RENDERER_RENDITION))
	{
		GLSTATE_DISABLE_ALPHATEST
	}

	qglBegin(GL_QUADS);
	qglTexCoord2f(0, 0);
	qglVertex2f(x, y);
	qglTexCoord2f(1, 0);
	qglVertex2f(x+w, y);
	qglTexCoord2f(1, t);
	qglVertex2f(x+w, y+h);
	qglTexCoord2f(0, t);
	qglVertex2f(x, y+h);
	qglEnd();

	if ((gl_config.renderer == GL_RENDERER_MCD) ||
		(gl_config.renderer & GL_RENDERER_RENDITION))
	{
		GLSTATE_ENABLE_ALPHATEST
	}
}
