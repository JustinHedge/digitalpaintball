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

#include "gl_local.h"
#include "gl_cin.h"
#include "jpeglib.h"

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;
int			base_textureid;		// gltextures[i] = base_textureid+i

static byte			 intensitytable[256];
static unsigned char gammatable[256];

cvar_t		*intensity;

unsigned	d_8to24table[256];
float		d_8to24tablef[256][3];

qboolean GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky, qboolean sharp);
qboolean GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean sharp);


int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;	// default to trilinear filtering - MrG
int		gl_filter_max = GL_LINEAR;

void GL_EnableMultitexture( qboolean enable )
{
	if ( !qglSelectTextureSGIS && !qglActiveTextureARB )
		return;

	if ( enable )
	{
		GL_SelectTexture( GL_TEXTURE1 );
		qglEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	else
	{
		GL_SelectTexture( GL_TEXTURE1 );
		qglDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	GL_SelectTexture( GL_TEXTURE0 );
	GL_TexEnv( GL_REPLACE );
}

void GL_SelectTexture( GLenum texture )
{
	int tmu;

	if ( !qglSelectTextureSGIS && !qglActiveTextureARB )
		return;

	if ( texture == GL_TEXTURE0 )
	{
		tmu = 0;
	}
	else
	{
		tmu = 1;
	}

	if ( tmu == gl_state.currenttmu )
	{
		return;
	}

	gl_state.currenttmu = tmu;

	if ( qglSelectTextureSGIS )
	{
		qglSelectTextureSGIS( texture );
	}
	else if ( qglActiveTextureARB )
	{
		qglActiveTextureARB( texture );
		qglClientActiveTextureARB( texture );
	}
}

// <!-- jitbright
extern cvar_t	*gl_overbright;

void GL_TexEnv(GLenum mode) 
{
	static int lastmodes[2] = { -1, -1 };
	if (mode != lastmodes[gl_state.currenttmu])
	{
		if(GL_COMBINE_EXT == mode) // a bit of a hack...
		{
			if(gl_state.texture_combine && gl_overbright->value)
			{
				//qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
				qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
				qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
			}
			else // failed to combine, default to modulate.
				mode = GL_MODULATE;
		}

		//qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode); // jit - should be i, right?
		lastmodes[gl_state.currenttmu] = mode;		
	}
}
// jit -->

void GL_Bind (int texnum)
{
	extern	image_t	*draw_chars;

	if (gl_nobind->value && draw_chars)		// performance evaluation option
		texnum = draw_chars->texnum;
	if ( gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;
	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	qglBindTexture (GL_TEXTURE_2D, texnum);
}

void GL_MBind( GLenum target, int texnum )
{
	GL_SelectTexture( target );
	if ( target == GL_TEXTURE0 )
	{
		if ( gl_state.currenttextures[0] == texnum )
			return;
	}
	else
	{
		if ( gl_state.currenttextures[1] == texnum )
			return;
	}
	GL_Bind( texnum );
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct
{
	char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !Q_stricmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky )
		{
			GL_Bind (glt->texnum);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !Q_stricmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !Q_stricmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
GL_ImageList_f
===============
*/
void	GL_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;
	const char *palstrings[2] =
	{
		"RGB",
		"PAL"
	};

	ri.Con_Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_skin:
			ri.Con_Printf (PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf (PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf (PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf (PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf (PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf (PRINT_ALL,  " %3i %3i %s: %s\n",
			image->upload_width, image->upload_height, palstrings[image->paletted], image->name);
	}
	ri.Con_Printf (PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		1
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT];
qboolean	scrap_dirty;

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
//	Sys_Error ("Scrap_AllocBlock: full");
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	scrap_uploads++;
	GL_Bind(TEXNUM_SCRAPS);
	GL_Upload8 (scrap_texels[0], BLOCK_WIDTH, BLOCK_HEIGHT, false, false, true); // (sharp) false);
	scrap_dirty = false;
}

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480)
	{
		ri.Con_Printf (PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}

	out = malloc ( (pcx->ymax+1) * (pcx->xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = malloc(768);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		free (*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile (pcx);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
LoadTGA
=============
*/
void LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	// jit - Knightmare- check for bad data
	if (!targa_header.width || !targa_header.height) {
		ri.Con_Printf (PRINT_ALL, "Bad tga file %s\n", name);
		ri.FS_FreeFile (buffer);
		return;
	}
	if (targa_header.image_type != 2 
		&& targa_header.image_type != 10) {
		ri.Con_Printf (PRINT_ALL, "LoadTGA: %s has wrong image format; only type 2 and 10 targa RGB images supported.\n", name);
		ri.FS_FreeFile (buffer);
		return;
	}
	if (targa_header.colormap_type !=0 
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24)) {
		ri.Con_Printf (PRINT_ALL, "LoadTGA: %s has wrong image format; only 32 or 24 bit images supported (no colormaps).\n", name);
		ri.FS_FreeFile (buffer);
		return;
	} 

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment
	
	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:
							
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
					}
	
					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}

	ri.FS_FreeFile (buffer);
}


/*
=================================================================

JPEG LOADING

By Robert 'Heffo' Heffernan

=================================================================
*/

void jpg_null(j_decompress_ptr cinfo)
{
}

unsigned char jpg_fill_input_buffer(j_decompress_ptr cinfo)
{
    ri.Con_Printf(PRINT_ALL, "Premature end of JPEG data\n");
    return 1;
}

void jpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
        
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;

    if (cinfo->src->bytes_in_buffer < 0) 
		ri.Con_Printf(PRINT_ALL, "Premature end of JPEG data\n");
}

void jpeg_mem_src(j_decompress_ptr cinfo, byte *mem, int len)
{
    cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    cinfo->src->init_source = jpg_null;
    cinfo->src->fill_input_buffer = jpg_fill_input_buffer;
    cinfo->src->skip_input_data = jpg_skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart;
    cinfo->src->term_source = jpg_null;
    cinfo->src->bytes_in_buffer = len;
    cinfo->src->next_input_byte = mem;
}

/*
==============
LoadJPG
==============
*/
void LoadJPG (char *filename, byte **pic, int *width, int *height)
{
	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rawdata, *rgbadata, *scanline, *p, *q;
	int								rawsize, i;

	// Load JPEG file into memory
	rawsize = ri.FS_LoadFile(filename, (void **)&rawdata);
	if(!rawdata)
		return;	

	// Knightmare- check for bad data
	if (      rawdata[6] != 'J'
		||    rawdata[7] != 'F'
		||    rawdata[8] != 'I'
		||    rawdata[9] != 'F') {
		ri.Con_Printf (PRINT_ALL, "Bad jpg file %s\n", filename);
		ri.FS_FreeFile(rawdata);
		return;
	}

	// Initialise libJpeg Object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// Feed JPEG memory into the libJpeg Object
	jpeg_mem_src(&cinfo, rawdata, rawsize);

	// Process JPEG header
	jpeg_read_header(&cinfo, true);

	// Start Decompression
	jpeg_start_decompress(&cinfo);

	// Check Colour Components
	if(cinfo.output_components != 3)
	{
		ri.Con_Printf(PRINT_ALL, "Invalid JPEG colour components\n");
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}

	// Allocate Memory for decompressed image
	rgbadata = malloc(cinfo.output_width * cinfo.output_height * 4);
	if(!rgbadata)
	{
		ri.Con_Printf(PRINT_ALL, "Insufficient RAM for JPEG buffer\n");
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}

	// Pass sizes to output
	*width = cinfo.output_width; *height = cinfo.output_height;

	// Allocate Scanline buffer
	scanline = malloc(cinfo.output_width * 3);
	if(!scanline)
	{
		ri.Con_Printf(PRINT_ALL, "Insufficient RAM for JPEG scanline buffer\n");
		free(rgbadata);
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}

	// Read Scanlines, and expand from RGB to RGBA
	q = rgbadata;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		p = scanline;
		jpeg_read_scanlines(&cinfo, &scanline, 1);

		for(i=0; i<cinfo.output_width; i++)
		{
			q[0] = p[0];
			q[1] = p[1];
			q[2] = p[2];
			q[3] = 255;

			p+=3; q+=4;
		}
	}

	// Free the scanline buffer
	free(scanline);

	// Finish Decompression
	jpeg_finish_decompress(&cinfo);

	// Destroy JPEG object
	jpeg_destroy_decompress(&cinfo);

	// Return the 'rgbadata'
	*pic = rgbadata;
}


/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int skinwidth, int skinheight )
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

//=======================================================


/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for (i=0 ; i<outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
/* jitgamma GL_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}
*/

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

/*
===============
GL_Upload32

Returns has_alpha
===============
*/
void GL_BuildPalettedTexture( unsigned char *paletted_texture, unsigned char *scaled, int scaled_width, int scaled_height )
{
	int i;

	for ( i = 0; i < scaled_width * scaled_height; i++ )
	{
		unsigned int r, g, b, c;

		r = ( scaled[0] >> 3 ) & 31;
		g = ( scaled[1] >> 2 ) & 63;
		b = ( scaled[2] >> 3 ) & 31;

		c = r | ( g << 5 ) | ( b << 11 );

		paletted_texture[i] = gl_state.d_16to8table[c];

		scaled += 4;
	}
}

int		upload_width, upload_height;
qboolean uploaded_paletted;

int nearest_power_of_2(int size)
{
	int i = 2;

	while (1) {
		i <<= 1;
		if (size == i)
			return i;
		if (size > i && size < (i <<1)) {
			if (size >= ((i+(i<<1))/2))
				return i<<1;
			else
				return i;
		}
	};
}

extern cvar_t *gl_anisotropy; // jitanisotropy
extern cvar_t *gl_texture_saturation; // jitsaturation
void desaturate_texture(unsigned *udata, int width, int height) // jitsaturation
{
	int i,size;
	float r,g,b,v,s;
	unsigned char *data;

	data = (unsigned char*)udata;

	s = gl_texture_saturation->value;

	size = width*height*4;

	for(i=0; i<size; i+=4)
	{
		r = data[i];
		g = data[i+1];
		b = data[i+2];
		v = r * 0.30 + g * 0.59 + b * 0.11;

		data[i]   = (1-s)*v + s*r;
		data[i+1] = (1-s)*v + s*g;
		data[i+2] = (1-s)*v + s*b;
	}
}

// ===[
// jit3dfx -- taken from Mesa source code since some 3dfx drivers don't support glu.

/*
 * Compute ceiling of integer quotient of A divided by B:
 */
#define CEILING( A, B )  ( (A) % (B) == 0 ? (A)/(B) : (A)/(B)+1 )

/* To work around optimizer bug in MSVC4.1 */
#if defined(__WIN32__) && !defined(OPENSTEP)
void
dummy(GLuint j, GLuint k)
{
}
#else
#define dummy(J, K)
#endif

/*
 * Find the value nearest to n which is also a power of two.
 */
static GLint round2(GLint n)
{
   GLint m;

   for (m = 1; m < n; m *= 2);

   /* m>=n */
   if (m - n <= n - m / 2) {
      return m;
   }
   else {
      return m / 2;
   }
}

/*
 * Given an pixel format and datatype, return the number of bytes to
 * store one pixel.
 */
static GLint
bytes_per_pixel(GLenum format, GLenum type)
{
   GLint n, m;

   switch (format) {
   case GL_COLOR_INDEX:
   case GL_STENCIL_INDEX:
   case GL_DEPTH_COMPONENT:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
      n = 1;
      break;
   case GL_LUMINANCE_ALPHA:
      n = 2;
      break;
   case GL_RGB:
   case GL_BGR:
      n = 3;
      break;
   case GL_RGBA:
   case GL_BGRA:
#ifdef GL_EXT_abgr
   case GL_ABGR_EXT:
#endif
      n = 4;
      break;
   default:
      n = 0;
   }

   switch (type) {
   case GL_UNSIGNED_BYTE:
      m = sizeof(GLubyte);
      break;
   case GL_BYTE:
      m = sizeof(GLbyte);
      break;
   case GL_BITMAP:
      m = 1;
      break;
   case GL_UNSIGNED_SHORT:
      m = sizeof(GLushort);
      break;
   case GL_SHORT:
      m = sizeof(GLshort);
      break;
   case GL_UNSIGNED_INT:
      m = sizeof(GLuint);
      break;
   case GL_INT:
      m = sizeof(GLint);
      break;
   case GL_FLOAT:
      m = sizeof(GLfloat);
      break;
   default:
      m = 0;
   }

   return n * m;
}

GLint Mesa_gluScaleImage(GLenum format,
	      GLsizei widthin, GLsizei heightin,
	      GLenum typein, const void *datain,
	      GLsizei widthout, GLsizei heightout,
	      GLenum typeout, void *dataout)
{
   GLint components, i, j, k;
   GLfloat *tempin, *tempout;
   GLfloat sx, sy;
   GLint unpackrowlength, unpackalignment, unpackskiprows, unpackskippixels;
   GLint packrowlength, packalignment, packskiprows, packskippixels;
   GLint sizein, sizeout;
   GLint rowstride, rowlen;


   /* Determine number of components per pixel */
   switch (format) {
   case GL_COLOR_INDEX:
   case GL_STENCIL_INDEX:
   case GL_DEPTH_COMPONENT:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
      components = 1;
      break;
   case GL_LUMINANCE_ALPHA:
      components = 2;
      break;
   case GL_RGB:
   case GL_BGR:
      components = 3;
      break;
   case GL_RGBA:
   case GL_BGRA:
#ifdef GL_EXT_abgr
   case GL_ABGR_EXT:
#endif
      components = 4;
      break;
   default:
      return GLU_INVALID_ENUM;
   }

   /* Determine bytes per input datum */
   switch (typein) {
   case GL_UNSIGNED_BYTE:
      sizein = sizeof(GLubyte);
      break;
   case GL_BYTE:
      sizein = sizeof(GLbyte);
      break;
   case GL_UNSIGNED_SHORT:
      sizein = sizeof(GLushort);
      break;
   case GL_SHORT:
      sizein = sizeof(GLshort);
      break;
   case GL_UNSIGNED_INT:
      sizein = sizeof(GLuint);
      break;
   case GL_INT:
      sizein = sizeof(GLint);
      break;
   case GL_FLOAT:
      sizein = sizeof(GLfloat);
      break;
   case GL_BITMAP:
      /* not implemented yet */
   default:
      return GL_INVALID_ENUM;
   }

   /* Determine bytes per output datum */
   switch (typeout) {
   case GL_UNSIGNED_BYTE:
      sizeout = sizeof(GLubyte);
      break;
   case GL_BYTE:
      sizeout = sizeof(GLbyte);
      break;
   case GL_UNSIGNED_SHORT:
      sizeout = sizeof(GLushort);
      break;
   case GL_SHORT:
      sizeout = sizeof(GLshort);
      break;
   case GL_UNSIGNED_INT:
      sizeout = sizeof(GLuint);
      break;
   case GL_INT:
      sizeout = sizeof(GLint);
      break;
   case GL_FLOAT:
      sizeout = sizeof(GLfloat);
      break;
   case GL_BITMAP:
      /* not implemented yet */
   default:
      return GL_INVALID_ENUM;
   }

   /* Get glPixelStore state */
   qglGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpackrowlength);
   qglGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackalignment);
   qglGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpackskiprows);
   qglGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpackskippixels);
   qglGetIntegerv(GL_PACK_ROW_LENGTH, &packrowlength);
   qglGetIntegerv(GL_PACK_ALIGNMENT, &packalignment);
   qglGetIntegerv(GL_PACK_SKIP_ROWS, &packskiprows);
   qglGetIntegerv(GL_PACK_SKIP_PIXELS, &packskippixels);

   /* Allocate storage for intermediate images */
   tempin = (GLfloat *) malloc(widthin * heightin
			       * components * sizeof(GLfloat));
   if (!tempin) {
      return GLU_OUT_OF_MEMORY;
   }
   tempout = (GLfloat *) malloc(widthout * heightout
				* components * sizeof(GLfloat));
   if (!tempout) {
      free(tempin);
      return GLU_OUT_OF_MEMORY;
   }


   /*
    * Unpack the pixel data and convert to floating point
    */

   if (unpackrowlength > 0) {
      rowlen = unpackrowlength;
   }
   else {
      rowlen = widthin;
   }
   if (sizein >= unpackalignment) {
      rowstride = components * rowlen;
   }
   else {
      rowstride = unpackalignment / sizein
	 * CEILING(components * rowlen * sizein, unpackalignment);
   }

   switch (typein) {
   case GL_UNSIGNED_BYTE:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLubyte *ubptr = (GLubyte *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * ubptr++;
	 }
      }
      break;
   case GL_BYTE:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLbyte *bptr = (GLbyte *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * bptr++;
	 }
      }
      break;
   case GL_UNSIGNED_SHORT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLushort *usptr = (GLushort *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * usptr++;
	 }
      }
      break;
   case GL_SHORT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLshort *sptr = (GLshort *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * sptr++;
	 }
      }
      break;
   case GL_UNSIGNED_INT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLuint *uiptr = (GLuint *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * uiptr++;
	 }
      }
      break;
   case GL_INT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLint *iptr = (GLint *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * iptr++;
	 }
      }
      break;
   case GL_FLOAT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLfloat *fptr = (GLfloat *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = *fptr++;
	 }
      }
      break;
   default:
      return GLU_INVALID_ENUM;
   }


   /*
    * Scale the image!
    */

   if (widthout > 1)
      sx = (GLfloat) (widthin - 1) / (GLfloat) (widthout - 1);
   else
      sx = (GLfloat) (widthin - 1);
   if (heightout > 1)
      sy = (GLfloat) (heightin - 1) / (GLfloat) (heightout - 1);
   else
      sy = (GLfloat) (heightin - 1);

/*#define POINT_SAMPLE*/
#ifdef POINT_SAMPLE
   for (i = 0; i < heightout; i++) {
      GLint ii = i * sy;
      for (j = 0; j < widthout; j++) {
	 GLint jj = j * sx;

	 GLfloat *src = tempin + (ii * widthin + jj) * components;
	 GLfloat *dst = tempout + (i * widthout + j) * components;

	 for (k = 0; k < components; k++) {
	    *dst++ = *src++;
	 }
      }
   }
#else
   if (sx < 1.0 && sy < 1.0) {
      /* magnify both width and height:  use weighted sample of 4 pixels */
      GLint i0, i1, j0, j1;
      GLfloat alpha, beta;
      GLfloat *src00, *src01, *src10, *src11;
      GLfloat s1, s2;
      GLfloat *dst;

      for (i = 0; i < heightout; i++) {
	 i0 = i * sy;
	 i1 = i0 + 1;
	 if (i1 >= heightin)
	    i1 = heightin - 1;
/*	 i1 = (i+1) * sy - EPSILON;*/
	 alpha = i * sy - i0;
	 for (j = 0; j < widthout; j++) {
	    j0 = j * sx;
	    j1 = j0 + 1;
	    if (j1 >= widthin)
	       j1 = widthin - 1;
/*	    j1 = (j+1) * sx - EPSILON; */
	    beta = j * sx - j0;

	    /* compute weighted average of pixels in rect (i0,j0)-(i1,j1) */
	    src00 = tempin + (i0 * widthin + j0) * components;
	    src01 = tempin + (i0 * widthin + j1) * components;
	    src10 = tempin + (i1 * widthin + j0) * components;
	    src11 = tempin + (i1 * widthin + j1) * components;

	    dst = tempout + (i * widthout + j) * components;

	    for (k = 0; k < components; k++) {
	       s1 = *src00++ * (1.0 - beta) + *src01++ * beta;
	       s2 = *src10++ * (1.0 - beta) + *src11++ * beta;
	       *dst++ = s1 * (1.0 - alpha) + s2 * alpha;
	    }
	 }
      }
   }
   else {
      /* shrink width and/or height:  use an unweighted box filter */
      GLint i0, i1;
      GLint j0, j1;
      GLint ii, jj;
      GLfloat sum, *dst;

      for (i = 0; i < heightout; i++) {
	 i0 = i * sy;
	 i1 = i0 + 1;
	 if (i1 >= heightin)
	    i1 = heightin - 1;
/*	 i1 = (i+1) * sy - EPSILON; */
	 for (j = 0; j < widthout; j++) {
	    j0 = j * sx;
	    j1 = j0 + 1;
	    if (j1 >= widthin)
	       j1 = widthin - 1;
/*	    j1 = (j+1) * sx - EPSILON; */

	    dst = tempout + (i * widthout + j) * components;

	    /* compute average of pixels in the rectangle (i0,j0)-(i1,j1) */
	    for (k = 0; k < components; k++) {
	       sum = 0.0;
	       for (ii = i0; ii <= i1; ii++) {
		  for (jj = j0; jj <= j1; jj++) {
		     sum += *(tempin + (ii * widthin + jj) * components + k);
		  }
	       }
	       sum /= (j1 - j0 + 1) * (i1 - i0 + 1);
	       *dst++ = sum;
	    }
	 }
      }
   }
#endif


   /*
    * Return output image
    */

   if (packrowlength > 0) {
      rowlen = packrowlength;
   }
   else {
      rowlen = widthout;
   }
   if (sizeout >= packalignment) {
      rowstride = components * rowlen;
   }
   else {
      rowstride = packalignment / sizeout
	 * CEILING(components * rowlen * sizeout, packalignment);
   }

   switch (typeout) {
   case GL_UNSIGNED_BYTE:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLubyte *ubptr = (GLubyte *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *ubptr++ = (GLubyte) tempout[k++];
	 }
      }
      break;
   case GL_BYTE:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLbyte *bptr = (GLbyte *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *bptr++ = (GLbyte) tempout[k++];
	 }
      }
      break;
   case GL_UNSIGNED_SHORT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLushort *usptr = (GLushort *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *usptr++ = (GLushort) tempout[k++];
	 }
      }
      break;
   case GL_SHORT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLshort *sptr = (GLshort *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *sptr++ = (GLshort) tempout[k++];
	 }
      }
      break;
   case GL_UNSIGNED_INT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLuint *uiptr = (GLuint *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *uiptr++ = (GLuint) tempout[k++];
	 }
      }
      break;
   case GL_INT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLint *iptr = (GLint *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *iptr++ = (GLint) tempout[k++];
	 }
      }
      break;
   case GL_FLOAT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLfloat *fptr = (GLfloat *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *fptr++ = tempout[k++];
	 }
      }
      break;
   default:
      return GLU_INVALID_ENUM;
   }


   /* free temporary image storage */
   free(tempin);
   free(tempout);

   return 0;
}

GLint Mesa_gluBuild2DMipmaps(GLenum target, GLint components,
		  GLsizei width, GLsizei height, GLenum format,
		  GLenum type, const void *data) // jit3dfx
{
   GLint w, h, maxsize;
   void *image, *newimage;
   GLint neww, newh, level, bpp;
   int error;
   GLboolean done;
   GLint retval = 0;
   GLint unpackrowlength, unpackalignment, unpackskiprows, unpackskippixels;
   GLint packrowlength, packalignment, packskiprows, packskippixels;

   if (width < 1 || height < 1)
      return GLU_INVALID_VALUE;

   qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);

   w = round2(width);
   if (w > maxsize) {
      w = maxsize;
   }
   h = round2(height);
   if (h > maxsize) {
      h = maxsize;
   }

   bpp = bytes_per_pixel(format, type);
   if (bpp == 0) {
      /* probably a bad format or type enum */
      return GLU_INVALID_ENUM;
   }

   /* Get current glPixelStore values */
   qglGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpackrowlength);
   qglGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackalignment);
   qglGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpackskiprows);
   qglGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpackskippixels);
   qglGetIntegerv(GL_PACK_ROW_LENGTH, &packrowlength);
   qglGetIntegerv(GL_PACK_ALIGNMENT, &packalignment);
   qglGetIntegerv(GL_PACK_SKIP_ROWS, &packskiprows);
   qglGetIntegerv(GL_PACK_SKIP_PIXELS, &packskippixels);

   /* set pixel packing */
   qglPixelStorei(GL_PACK_ROW_LENGTH, 0);
   qglPixelStorei(GL_PACK_ALIGNMENT, 1);
   qglPixelStorei(GL_PACK_SKIP_ROWS, 0);
   qglPixelStorei(GL_PACK_SKIP_PIXELS, 0);

   done = GL_FALSE;

   if (w != width || h != height) {
      /* must rescale image to get "top" mipmap texture image */
      image = malloc((w + 4) * h * bpp);
      if (!image) {
	 return GLU_OUT_OF_MEMORY;
      }
      error = Mesa_gluScaleImage(format, width, height, type, data,
			    w, h, type, image);
      if (error) {
	 retval = error;
	 done = GL_TRUE;
      }
   }
   else {
      image = (void *) data;
   }

   level = 0;
   while (!done) {
      if (image != data) {
	 /* set pixel unpacking */
	 qglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	 qglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	 qglPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	 qglPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
      }

      qglTexImage2D(target, level, components, w, h, 0, format, type, image);

      if (w == 1 && h == 1)
	 break;

      neww = (w < 2) ? 1 : w / 2;
      newh = (h < 2) ? 1 : h / 2;
      newimage = malloc((neww + 4) * newh * bpp);
      if (!newimage) {
	 return GLU_OUT_OF_MEMORY;
      }

      error = Mesa_gluScaleImage(format, w, h, type, image,
			    neww, newh, type, newimage);
      if (error) {
	 retval = error;
	 done = GL_TRUE;
      }

      if (image != data) {
	 free(image);
      }
      image = newimage;

      w = neww;
      h = newh;
      level++;
   }

   if (image != data) {
      free(image);
   }

   /* Restore original glPixelStore state */
   qglPixelStorei(GL_UNPACK_ROW_LENGTH, unpackrowlength);
   qglPixelStorei(GL_UNPACK_ALIGNMENT, unpackalignment);
   qglPixelStorei(GL_UNPACK_SKIP_ROWS, unpackskiprows);
   qglPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackskippixels);
   qglPixelStorei(GL_PACK_ROW_LENGTH, packrowlength);
   qglPixelStorei(GL_PACK_ALIGNMENT, packalignment);
   qglPixelStorei(GL_PACK_SKIP_ROWS, packskiprows);
   qglPixelStorei(GL_PACK_SKIP_PIXELS, packskippixels);

   return retval;
}
// ]===

#if 1 // jit3dfx
qboolean GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean sharp)
{
	int			samples;
	unsigned 	*scaled;
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	int comp;

	uploaded_paletted = false;

	// scan the texture for any non-255 alpha
	c = width*height;
	scan = ((byte *)data) + 3;
	samples = gl_solid_format;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if(gl_texture_saturation->value < 1)
		desaturate_texture(data, width, height); // jitsaturation

	//Heffo - ARB Texture Compression
	if (samples == gl_solid_format)
		comp = (gl_state.texture_compression) ? GL_COMPRESSED_RGB_ARB : gl_tex_solid_format;
	else if (samples == gl_alpha_format)
		comp = (gl_state.texture_compression) ? GL_COMPRESSED_RGBA_ARB : gl_tex_alpha_format;

	// find sizes to scale to
	{
		int max_size;

		qglGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_size);
		scaled_width = nearest_power_of_2(width);
		scaled_height = nearest_power_of_2(height);

		if (scaled_width > max_size)
			scaled_width = max_size;
		if (scaled_height > max_size)
			scaled_height = max_size;
	}

	if (scaled_width != width || scaled_height != height) {
		scaled=malloc((scaled_width * scaled_height) * 4);
		GL_ResampleTexture(data,width,height,scaled,scaled_width,scaled_height);
	} else {
		scaled_width=width;
		scaled_height=height;
		scaled=data;
	}
	// ===
	// jithudscale -- clean up hud images
#if 0
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap) ? gl_filter_min : gl_filter_max);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
#else
	if(mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		//qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if(gl_anisotropy->value) // jitanisotropy
		{
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 
			                 gl_anisotropy->value);
		}
		else
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		if(sharp)
		{
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
#endif
	// ===
	if (mipmap) {
//		if (!gl_state.gammaramp)
// jitgamma			GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap);
		if (gl_state.sgis_mipmap) 
		{
			qglTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, true);
			qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, 
				scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		} 
		else
		{
			/*if(glw_state.minidriver) */ 
				Mesa_gluBuild2DMipmaps (GL_TEXTURE_2D, comp, scaled_width,
					scaled_height, GL_RGBA, GL_UNSIGNED_BYTE, scaled); // jit3dfx
			/*else
				gluBuild2DMipmaps (GL_TEXTURE_2D, comp, scaled_width,
					scaled_height, GL_RGBA, GL_UNSIGNED_BYTE, scaled);*/
		}
	} 
	else 
	{ 
//		if (!gl_state.gammaramp)
// jitgamma			GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap );
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, 
			scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}
	if (scaled_width != width || scaled_height != height)
		free(scaled);

	upload_width=scaled_width; upload_height = scaled_height;



	return (samples == gl_alpha_format || samples == GL_COMPRESSED_RGBA_ARB);
}


#else ////////////////// jit3dfx testing


qboolean GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean sharp)
{
	int			samples;
	unsigned	scaled[256*256];
	unsigned char paletted_texture[256*256];
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	int comp;

	uploaded_paletted = false;

	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	if (gl_round_down->value && scaled_width > width && mipmap)
		scaled_width >>= 1;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
	if (gl_round_down->value && scaled_height > height && mipmap)
		scaled_height >>= 1;

	// let people sample down the world textures for speed
	if (mipmap)
	{
		scaled_width >>= (int)gl_picmip->value;
		scaled_height >>= (int)gl_picmip->value;
	}

	// don't ever bother with >256 textures
	if (scaled_width > 256)
		scaled_width = 256;
	if (scaled_height > 256)
		scaled_height = 256;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled)/4)
		ri.Sys_Error (ERR_DROP, "GL_Upload32: too big");

	// scan the texture for any non-255 alpha
	c = width*height;
	scan = ((byte *)data) + 3;
	samples = gl_solid_format;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if (samples == gl_solid_format)
	    comp = gl_tex_solid_format;
	else if (samples == gl_alpha_format)
	    comp = gl_tex_alpha_format;
	else {
	    ri.Con_Printf (PRINT_ALL,
			   "Unknown number of texture components %i\n",
			   samples);
	    comp = samples;
	}

#if 0
	if (mipmap)
		gluBuild2DMipmaps (GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else if (scaled_width == width && scaled_height == height)
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else
	{
		gluScaleImage (GL_RGBA, width, height, GL_UNSIGNED_BYTE, trans,
			scaled_width, scaled_height, GL_UNSIGNED_BYTE, scaled);
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}
#else

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			/*if ( qglColorTableEXT && gl_ext_palettedtexture->value && samples == gl_solid_format )
			{
				uploaded_paletted = true;
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) data, scaled_width, scaled_height );
				qglTexImage2D( GL_TEXTURE_2D,
							  0,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
			}
			else*/
			{
				qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}
			goto done;
		}
		memcpy (scaled, data, width*height*4);
	}
	else
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);

//	GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap );

/*	if ( qglColorTableEXT && gl_ext_palettedtexture->value && ( samples == gl_solid_format ) )
	{
		uploaded_paletted = true;
		GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
		qglTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_COLOR_INDEX8_EXT,
					  scaled_width,
					  scaled_height,
					  0,
					  GL_COLOR_INDEX,
					  GL_UNSIGNED_BYTE,
					  paletted_texture );
	}
	else*/
	{
		qglTexImage2D( GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled );
	}

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
/*			if ( qglColorTableEXT && gl_ext_palettedtexture->value && samples == gl_solid_format )
			{
				uploaded_paletted = true;
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
				qglTexImage2D( GL_TEXTURE_2D,
							  miplevel,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
			}
			else*/
			{
				qglTexImage2D (GL_TEXTURE_2D, miplevel, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
			}
		}
	}
done: ;
#endif


	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	return (samples == gl_alpha_format);
}

#endif

/*
===============
GL_Upload8

Returns has_alpha
===============
*/

qboolean GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky, qboolean sharp )
{
	unsigned	trans[512*256];
	int			i, s;
	int			p;

	s = width*height;

	if (s > sizeof(trans)/4)
		ri.Sys_Error (ERR_DROP, "GL_Upload8: too large");

		for (i=0 ; i<s ; i++)
		{
			p = data[i];
			trans[i] = d_8to24table[p];

			if (p == 255)
			{	// transparent, so scan around for another color
				// to avoid alpha fringes
				// FIXME: do a full flood fill so mips work...
				if (i > width && data[i-width] != 255)
					p = data[i-width];
				else if (i < s-width && data[i+width] != 255)
					p = data[i+width];
				else if (i > 0 && data[i-1] != 255)
					p = data[i-1];
				else if (i < s-1 && data[i+1] != 255)
					p = data[i+1];
				else
					p = 0;
				// copy rgb components
				((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
				((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
				((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
			}
		}

		return GL_Upload32 (trans, width, height, mipmap, sharp);
}


/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
	image_t		*image;
	int			i;
	qboolean sharp = false; // jit, for images we don't want filtered.

	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			ri.Sys_Error (ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy (image->name, name);
	image->registration_sequence = registration_sequence;
	
	image->width = width;
	image->height = height;
	image->type = type;

	// ===
	// jit -- paintball2 texture fix
	if( !Q_strcasecmp(name,"textures/pball/b_flag1.jpg") ||
		!Q_strcasecmp(name,"textures/pball/y_flag1.jpg") || 
		!Q_strcasecmp(name,"textures/pball/p_flag1.jpg") || 
		!Q_strcasecmp(name,"textures/pball/r_flag1.jpg"))
	{
		image->width = 96;
		image->height = 96;
	}
	else if(!Q_strcasecmp(name,"textures/pball/ksplat2.jpg"))
	{
		image->width = 288;
		image->height = 128;
	}
	else if(strstr(name, "pics/gamma") || strstr(name, "pics/menu_char_colors") ||
		(image->type == it_pic && bits == 8))
	{
		sharp = true; // no bilinear filtering
	}
	// jit
	// ===



	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

	// load little pics into the scrap
	if (image->type == it_pic && bits == 8
		&& image->width < 64 && image->height < 64 && sharp)// && !sharp)
	{
		int		x, y;
		int		i, j, k;
		int		texnum;

		texnum = Scrap_AllocBlock (image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
		scrap_dirty = true;

		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (x+0.01)/(float)BLOCK_WIDTH;
		image->sh = (x+image->width-0.01)/(float)BLOCK_WIDTH;
		image->tl = (y+0.01)/(float)BLOCK_WIDTH;
		image->th = (y+image->height-0.01)/(float)BLOCK_WIDTH;
	}
	else
	{
nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);

		if (bits == 8)
			image->has_alpha = GL_Upload8 (pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_sky, sharp);
		else
			image->has_alpha = GL_Upload32 ((unsigned *)pic, width, height, (image->type != it_pic && image->type != it_sky), sharp);
		image->upload_width = upload_width;		// after power of 2 and scales
		image->upload_height = upload_height;
		image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	return image;
}


/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal (char *name)
{
	miptex_t	*mt;
	int			width, height, ofs;
	image_t		*image;

	ri.FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		ri.Con_Printf (PRINT_ALL, "GL_FindImage: can't load %s\n", name);
		return r_notexture;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = GL_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8);

	ri.FS_FreeFile ((void *)mt);

	return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
char override=0;

image_t	*GL_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
	int		i, len;
	byte	*pic, *palette;
	int		width, height;
	extern cvar_t *gl_highres_textures;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = strlen(name);
	if (len<5)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	// look for it
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			image->alreadyloaded = true;
			return image;
		}
	}

	//
	// load the pic from disk
	//

	pic = NULL;
	palette = NULL;

	if (strcmp(name+len-4, ".tga") && strcmp(name+len-4, ".cin") && !override)
	{	// Targa override crap
		char s[128];

		override=1;
		strcpy(s,name);
		s[len-3]='t'; s[len-2]='g'; s[len-1]='a';
		image = GL_FindImage(s,type);
		if (image) {
			override=0;
			return image;
		}
	}
	if (strcmp(name+len-4, ".jpg") && strcmp(name+len-4, ".cin") && !override)
	{	// Jpeg override crap
		char s[MAX_QPATH];
		char s_high[MAX_QPATH];
		char *finger;

		override=1;
		strcpy(s,name);
		s[len-3]='j'; s[len-2]='p'; s[len-1]='g';
		// ==[
		// jithighres
		if(gl_highres_textures->value)
		{
			// here we add "hr4/" to the directory, like:
			// textures/pball/whatever.jpg becomes:
			// textures/pball/hr4/whatever.jpg
			strcpy(s_high, s);
			finger = strstr(s_high, ".jpg") + 4;
			while(finger>s_high && *finger != '/')
			{
				*(finger+4) = *finger;
				finger--;
			}
			
			*(finger+1) = 'h'; *(finger+2) = 'r';
			*(finger+3) = '4'; *(finger+4) = '/';
			image = GL_FindImage(s_high,type);
			if (image)
			{
				override=0;
				// scale the texture appropriately (/4x each way)
				if(!image->alreadyloaded)
				{
					image->width /= 4;
					image->height /= 4;
				}
				
				return image;
			}
		}
		// ]==

		// didn't find high res version (or highres_textures is off)
		image = GL_FindImage(s,type);
		if (image)
		{
			override=0;
			return image;
		}
	}
	
	override=0;
	if (!strcmp(name+len-4, ".pcx"))
	{
		LoadPCX (name, &pic, &palette, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
		image = GL_LoadPic (name, pic, width, height, type, 8);
		image->alreadyloaded = false; // jithighres
	}
	else if (!strcmp(name+len-4, ".wal"))
	{
		image = GL_LoadWal (name);
		image->alreadyloaded = false; // jithighres
	}
	else if (!strcmp(name+len-4, ".tga"))
	{
		LoadTGA (name, &pic, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
		image = GL_LoadPic (name, pic, width, height, type, 32);
		image->alreadyloaded = false; // jithighres
	}
	else if (!strcmp(name+len-4, ".cin")) // Heffo
	{										// WHY .cin files? because we can!
		cinematics_t *newcin;

		newcin = CIN_OpenCin(name);
		if(!newcin)
			return NULL;

		pic = malloc(256*256*4);
		memset(pic, 192, (256*256*4));

		image = GL_LoadPic (name, pic, 256, 256, type, 32);

		newcin->texnum = image->texnum;
		image->is_cin = true;
		image->alreadyloaded = false; // jithighres
	}
	else if (!strcmp(name+len-4, ".jpg")) // Heffo - JPEG support
	{
		LoadJPG(name, &pic, &width, &height);
		if(!pic)
			return NULL;

		image = GL_LoadPic(name, pic, width, height, type, 32);
		image->alreadyloaded = false; // jithighres
	} else
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name);

	if (pic)
		free(pic);
	if (palette)
		free(palette);

	return image;
}

/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return GL_FindImage (name, it_skin);
}


/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics

		//Heffo - Free Cinematic
		if(image->is_cin)
			CIN_FreeCin(image->texnum);

		// free it
		qglDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}


/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette (void)
{
	int		i;
	int		r, g, b;
	unsigned	v;
	byte	*pic, *pal;
	int		width, height;

	// get the palette

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		ri.Sys_Error (ERR_FATAL, "Couldn't load pics/colormap.pcx\nPlease make sure the game is installed properly."); // jit


	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];
		
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);

		d_8to24tablef[i][0] = r*0.003921568627450980392156862745098f;
		d_8to24tablef[i][1] = g*0.003921568627450980392156862745098f;
		d_8to24tablef[i][2] = b*0.003921568627450980392156862745098f;
	}

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	free (pic);
	free (pal);

	return 0;
}


/*
===============
GL_InitImages
===============
*/
void	GL_InitImages (void)
{
	/* jit, no longer needed
	int		i, j;
	float	g = vid_gamma->value;

	registration_sequence = 1;

	// init intensity conversions
	intensity = ri.Cvar_Get ("intensity", "2", 0);

	if ( intensity->value <= 1 )
		ri.Cvar_Set( "intensity", "1" );

	gl_state.inverse_intensity = 1 / intensity->value;
*/
	Draw_GetPalette ();

	if ( qglColorTableEXT )
	{
		ri.FS_LoadFile( "pics/16to8.dat", &gl_state.d_16to8table );
		if ( !gl_state.d_16to8table )
			ri.Sys_Error( ERR_FATAL, "Couldn't load pics/16to8.dat"); // jit, just bugged me
	}
/*
	if ( gl_config.renderer & ( GL_RENDERER_VOODOO | GL_RENDERER_VOODOO2 ) )
	{
		g = 1.0F;
	}

	for ( i = 0; i < 256; i++ )
	{
		if ( g == 1 )
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;

			inf = 255 * pow ( (i+0.5)*0.0039138943248532289628180039138943 , g ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i=0 ; i<256 ; i++)
	{
		j = i*intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}*/
}

/*
===============
GL_ShutdownImages
===============
*/
void	GL_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot

		//Heffo - Free Cinematic
		if(image->is_cin)
			CIN_FreeCin(image->texnum);

		// free it
		qglDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

