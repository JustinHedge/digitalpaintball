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
// r_misc.c

#include "gl_local.h"
#ifdef WIN32
#include "jpeg/jpeglib.h" //Heffo - JPEG Screenshots
#else
#include <jpeglib.h>
#endif

/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

byte	missing_texture[16][16] =
{
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

void R_InitNoTexture (void) /// jit, renamed
{
	int		x,y,i;
	byte	notex[16][16][4];
	byte	data[8][8][4];
	byte	white[8][8][4]; // jitfog

	// particle texture
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}

	r_particletexture = GL_LoadPic("***particle***", (byte *)data, 8, 8, it_sprite, 32);

	for (x = 0; x < 16; x++)
	{
		for (y = 0; y < 16; y++)
		{
			// jit: tweaked to look like quake3's missing textures
			notex[y][x][0] = (1+missing_texture[x][y]*2)*85;
			notex[y][x][1] = (1+missing_texture[x][y]*2)*85;
			notex[y][x][2] = (1+missing_texture[x][y]*2)*85;
			notex[y][x][3] = 255;
		}
	}

	r_notexture = GL_LoadPic("***r_notexture***", (byte *)notex, 16, 16, it_wall, 32);

	// jitfog (solid white texture)
	for (x = 0; x < 8; x++)
		for (y = 0; y < 8; y++)
			for (i = 0; i < 4; i++)
				white[x][y][i] = 255;

	r_whitetexture = GL_LoadPic("***r_whitetexture***", (byte*)white, 8, 8, it_sky, 32);

	if (r_caustics->value > 1.0f)
		r_caustictexture = GL_FindImage("textures/sfx/caustics/caustics1_02.tga", it_wall); // jitcaustics
	else if (r_caustics->value)
		r_caustictexture = GL_FindImage("textures/sfx/caustics/caustics1_01.tga", it_wall); // jitcaustics
}


/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


// jitgamma -- apply gamma to screenshots
void apply_gamma(byte *rgbdata, int w, int h)
{
	register int i,j,k;
	extern unsigned short gamma_ramp[3][256];

	if (!vid_gamma_hw->value || !gl_state.gammaramp ||
		!ri.Cvar_Get("gl_screenshot_applygamma", "0", CVAR_ARCHIVE)->value)
		return; // don't apply gamma if it's not turned on!
	else
	{
		k = w*h;

		for(i=0; i<k; i++)
			for(j=0; j<3; j++)
				rgbdata[i*3+j] = ((gamma_ramp[j][rgbdata[i*3+j]]) >> 8);
	}
}

/* 
================== 
GL_ScreenShot_JPG
By Robert 'Heffo' Heffernan
================== 
*/
void GL_ScreenShot_JPG (void)
{
	struct jpeg_compress_struct		cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rgbdata;
	JSAMPROW						s[1];
	FILE							*file;
	char							picname[80], checkname[MAX_OSPATH];
	int								i, offset;

	// Create the scrnshots directory if it doesn't exist
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir(checkname);

	// Find a file name to save it to 
	strcpy(picname,"sshot000.jpg"); // jit

	for (i = 0; i <= 999; i++) 
	{ 
		picname[5] = i/100 + '0'; 
		picname[6] = (i%100)/10 + '0'; 
		picname[7] = i%10 + '0';
		Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		file = fopen(checkname, "rb");

		if (!file)
			break;	// file doesn't exist

		fclose (file);
	}

	if (i == 1000) 
	{
		ri.Con_Printf (PRINT_ALL, "SCR_JPGScreenShot_f: Too many files\n"); 
		return;
 	}

	// Open the file for Binary Output
	file = fopen(checkname, "wb");

	if (!file)
	{
		ri.Con_Printf (PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n"); 
		return;
 	}

	// Allocate room for a copy of the framebuffer
	rgbdata = malloc(vid.width * vid.height * 3);

	if (!rgbdata)
	{
		fclose(file);
		return;
	}

	// Read the framebuffer into our storage
	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);
	apply_gamma(rgbdata, vid.width, vid.height); // jitgamma -- apply video gammaramp to screenshot

	// Initialise the JPEG compression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	// Setup JPEG Parameters
	cinfo.image_width = vid.width;
	cinfo.image_height = vid.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	if ((gl_screenshot_jpeg_quality->value >= 101) || (gl_screenshot_jpeg_quality->value <= 0))
		ri.Cvar_Set("gl_screenshot_jpeg_quality", "85");
	jpeg_set_quality(&cinfo, gl_screenshot_jpeg_quality->value, TRUE);

	// Start Compression
	jpeg_start_compress(&cinfo, true);

	// Feed Scanline data
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while(cinfo.next_scanline < cinfo.image_height)
	{
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	// Finish Compression
	jpeg_finish_compress(&cinfo);

	// Destroy JPEG object
	jpeg_destroy_compress(&cinfo);

	// Close File
	fclose(file);

	// Free Temp Framebuffer
	free(rgbdata);

	// Done!
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
}

/* 
================== 
GL_ScreenShot_f
================== 
*/  
void GL_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;

	// Heffo - JPEG Screenshots
	if (gl_screenshot_jpeg->value)
	{
		GL_ScreenShot_JPG();
		return;
	}

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"sshot000.tga"); // jitsshot

	for (i=0 ; i<=99 ; i++) 
	{ 
//		picname[5] = i/10 + '0'; 
//		picname[6] = i%10 + '0'; 
		picname[5] = i/100 + '0'; // jitsshot
		picname[6] = (i%100)/10 + '0';  // jitsshot
		picname[7] = i%10 + '0'; // jitsshot
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==100) 
	{
		ri.Con_Printf (PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n"); 
		return;
 	}


	buffer = malloc(vid.width*vid.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	qglReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 
	apply_gamma(buffer+18, vid.width, vid.height); // jitgamma -- apply video gammaramp to screenshot

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
} 

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	ri.Con_Printf (PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string );
	ri.Con_Printf (PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string );
	ri.Con_Printf (PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string );
	ri.Con_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
extern qboolean fogenabled;
extern vec3_t fogcolor;
void GL_SetDefaultState (void)
{
	//qglClearColor (1,0, 0.5 , 0.5); jitclearcolor

	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666f);
	gl_state.alpha_test=true;

	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);
	gl_state.blend = false;

	qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel(GL_FLAT);

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv(GL_REPLACE);

	gl_state.texgen = false;
	qglDisable(GL_TEXTURE_GEN_S);
	qglDisable(GL_TEXTURE_GEN_T);

	if (qglPointParameterfEXT)
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		qglEnable(GL_POINT_SMOOTH);
		qglPointParameterfEXT(GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value);
		qglPointParameterfEXT(GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value);
		qglPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, attenuations);
	}
}

void GL_UpdateSwapInterval (void)
{
	if (gl_swapinterval->modified)
	{
		gl_swapinterval->modified = false;

		if (!gl_state.stereo_enabled)
		{
#ifdef _WIN32
			if (qwglSwapIntervalEXT)
			{
				qwglSwapIntervalEXT(!gl_swapinterval->value); // jitest -- ugly hack
				qwglSwapIntervalEXT(gl_swapinterval->value);
			}
#endif
		}
	}
}

