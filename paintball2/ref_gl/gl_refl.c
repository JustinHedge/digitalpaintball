// gl_refl.c
// by Matt Ownby - updates by jitspoe

// adds reflective water to the Quake2 engine

#include "gl_local.h"
#include "gl_refl.h"


// width and height of the texture we are gonna use to capture our reflection
unsigned int REFL_TEXW;
unsigned int REFL_TEXH;		

unsigned int g_reflTexW;		// dynamic size of reflective texture
unsigned int g_reflTexH;

int		g_num_refl		= 0;	// how many reflections we need to generate
int		g_active_refl	= 0;	// which reflection is being rendered at the moment

float	*g_refl_X;
float	*g_refl_Y;
float	*g_refl_Z;				// the Z (vertical) value of each reflection
float	*g_waterDistance;		// the rough distance from player to water .. we want to render the closest water surface.
image_t	**g_refl_images;
int		maxReflections;			// maximum number of reflections
unsigned int g_water_program_id; // jitwater
image_t *distort_tex = NULL; // jitwater
image_t *water_normal_tex = NULL; // jitwater

// whether we are actively rendering a reflection of the world
// (instead of the world itself)
qboolean g_drawing_refl = false;
qboolean g_refl_enabled = true;	// whether reflections should be drawn at all
//#define USE_FBO
#ifdef USE_FBO
GLuint g_refl_framebuffer = 0; // jitwater
GLuint g_refl_renderbuffer = 0; // jitwater
#endif

float	g_last_known_fov = 90.0f;	// jit - default to 90.

void MYgluPerspective (GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar); // jit
void GL_MipMap (byte *in, int width, int height); // jit
void R_Clear (void); // jitwater


/*
================
R_init_refl

sets everything up 
================
*/

image_t *Draw_FindPic(const char *name);

void R_init_refl (int maxNoReflections)
{
	//===========================
	int				power;
	int				maxSize;
	unsigned char	*buf = NULL;
	int				i = 0;
	int len; // jitwater
	char *fragment_program_text; // jitwater
#ifdef _DEBUG
	int err;
#endif
	//===========================

	if (maxNoReflections < 1) // jit
		maxNoReflections = 1;

	R_setupArrays(maxNoReflections);	// setup number of reflections
	assert((err = qglGetError()) == GL_NO_ERROR);

	//okay we want to set REFL_TEXH etc to be less than the resolution 
	//otherwise white boarders are left .. we dont want that.
	//if waves function is turned off we can set reflection size to resolution.
	//however if it is turned on in game white marks round the sides will be left again
	//so maybe its best to leave this alone.

	for (power = 2; power < vid.height; power*=2)
	{	
		REFL_TEXW = power;	
		REFL_TEXH = power;  
	}

	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);		//get max supported texture size

	if (REFL_TEXW > maxSize)
	{
		for(power = 2; power < maxSize; power*=2)
		{	
			REFL_TEXW = power;	
			REFL_TEXH = power;  
		}
	}

	g_reflTexW = REFL_TEXW;
	g_reflTexH = REFL_TEXH;
	
	// if screen dimensions are smaller than texture size, we have to use screen dimensions instead (doh!)
	g_reflTexW = (vid.width < REFL_TEXW) ? vid.width : REFL_TEXW;	//keeping these in for now ..
	g_reflTexH = (vid.height < REFL_TEXH) ? vid.height : REFL_TEXH;

	for (i = 0; i < maxReflections; i++)
	{
		g_refl_images[i] = GL_CreateBlankImage("_reflection", g_reflTexW, g_reflTexH, it_pic);

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "Reflection texture %d texnum = %d\n", i, g_refl_images[i]->texnum);
	}

	if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "Initialising reflective textures\n");
		ri.Con_Printf(PRINT_ALL, "...reflective texture size set at %d\n",g_reflTexH);
		ri.Con_Printf(PRINT_ALL, "...maximum reflective textures %d\n", maxReflections);
	}

	// === jitwater - fragment program initializiation
	if (gl_state.fragment_program)
	{
		qglGenProgramsARB(1, &g_water_program_id);
		qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, g_water_program_id);
		qglProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, 1.0f, 0.1f, 0.6f, 0.5f); // jitest
		len = ri.FS_LoadFileZ("scripts/water1.arbf", (void *)&fragment_program_text);

		if (len > 0)
		{
			qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, len, fragment_program_text);
			ri.FS_FreeFile(fragment_program_text);
		}
		else
		{
			ri.Con_Printf(PRINT_ALL, "Unable to find scripts/water1.arbf\n");
		}
		
		// Make sure the program loaded correctly
		{
			int err = 0;

			err = qglGetError();

			if (err != GL_NO_ERROR)
				ri.Con_Printf(PRINT_ALL, "OpenGL error with ARB fragment program: 0x%x\n", err);

#ifdef WIN32 // since linux is retarded and kills the whole program
			assert(err == GL_NO_ERROR);
#endif
		}

		distort_tex = Draw_FindPic("/textures/sfx/water/distort1.tga");
		water_normal_tex = Draw_FindPic("/textures/sfx/water/normal1.tga");
	}
#ifdef USE_FBO
	// FBO initialization
	qglGenFramebuffersEXT(1, &g_refl_framebuffer);
	assert((err = qglGetError()) == GL_NO_ERROR);
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_refl_framebuffer);
	assert((err = qglGetError()) == GL_NO_ERROR);

	qglGenRenderbuffersEXT(1, &g_refl_renderbuffer);
	qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, g_refl_renderbuffer);
	qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, g_reflTexW, g_reflTexH);
	qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, g_refl_renderbuffer);
	assert((err = qglGetError()) == GL_NO_ERROR);

	if (gl_debug->value)
	{
		int wtf = g_refl_images[0]->texnum;
		ri.Con_Printf(PRINT_ALL, "Reflective texture bound = %d\n", wtf);
	}

	GL_Bind(g_refl_images[0]->texnum);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, g_refl_images[0]->texnum, 0);
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	assert(status == GL_FRAMEBUFFER_COMPLETE_EXT);

	// Unbind framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif // USE_FBO
	// jitwater ===
}

void R_shutdown_refl (void) // jitodo - call this.
{
	if (gl_state.fragment_program)
		qglDeleteProgramsARB(1, &g_water_program_id);
}

/*
================
R_setupArrays

creates the actual arrays
to hold the reflections in
================
*/
void R_setupArrays (int maxNoReflections)
{
	R_clear_refl();

	free(g_refl_X);
	free(g_refl_Y);
	free(g_refl_Z);
	free(g_refl_images); // TODO: Make sure there isn't a memory leak here.  Do we need to free the images, too?
	free(g_waterDistance);

	g_refl_X		= (float *)	malloc ( sizeof(float) * maxNoReflections );
	g_refl_Y		= (float *)	malloc ( sizeof(float) * maxNoReflections );
	g_refl_Z		= (float *)	malloc ( sizeof(float) * maxNoReflections );
	g_waterDistance	= (float *)	malloc ( sizeof(float) * maxNoReflections );
	g_refl_images	= (image_t **)malloc(sizeof(image_t *) * maxNoReflections);
	
	memset(g_refl_X			, 0, sizeof(float));
	memset(g_refl_Y			, 0, sizeof(float));
	memset(g_refl_Z			, 0, sizeof(float));
	memset(g_waterDistance	, 0, sizeof(float));

	maxReflections = maxNoReflections;
}


/*
================
R_clear_refl

clears the relfection array
================
*/
void R_clear_refl (void)
{
	g_num_refl = 0;
}


/*
================
R_add_refl

creates an array of reflections
================
*/
void R_add_refl (float x, float y, float z)
{
	float distance;
	int i = 0;
	vec3_t v, v2;

	if (!maxReflections)
		return;		//safety check.

	if (r_reflectivewater_max->value != maxReflections)
		R_init_refl(r_reflectivewater_max->value);

	// make sure this isn't a duplicate entry
	// (I expect a lot of duplicates, which is why I put this check first)
	for (; i < g_num_refl; i++)
	{
		// if this is a duplicate entry then we don't want to add anything
		if (fabs(g_refl_Z[i] - z) < 8.0f)
			return;
	}

	VectorSet(v, x, y, z);
	VectorSubtract(v, r_newrefdef.vieworg, v2);
	distance = VectorLength(v2);

	// make sure we have room to add
	if (g_num_refl < maxReflections)
	{
		g_refl_X[g_num_refl]			= x;
		g_refl_Y[g_num_refl]			= y;
		g_refl_Z[g_num_refl]			= z;
		g_waterDistance[g_num_refl]		= distance;
		g_num_refl++;
	}
	else
	{
		// we want to use the closest surface
		// not just any random surface
		// good for when 1 reflection enabled.
		for (i = 0; i < g_num_refl; i++)
		{
			if (distance < g_waterDistance[i])
			{
				g_refl_X[i]			= x;
				g_refl_Y[i]			= y;
				g_refl_Z[i]			= z;
				g_waterDistance[i]	= distance;
				return;	//lets go
			}
		}
	}
}



static int txm_genTexObject(unsigned char *texData, int w, int h,
								int format, qboolean repeat, qboolean mipmap)
{
	unsigned int texNum;

	qglGenTextures(1, &texNum);
	qglGenTextures(1, &texNum);

	if (gl_debug->value)
		ri.Con_Printf(PRINT_ALL, "Reflection texnum = %d\n", texNum);

	repeat = false;
	mipmap = false;

	if (texData)
	{
		qglBindTexture(GL_TEXTURE_2D, texNum);

		// Set the tiling mode
		if (repeat)
		{
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		else
		{
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
		}

		// Set the filtering
		if (mipmap)
		{
			int scaled_width = w, scaled_height = h, miplevel = 0; // jit

			qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

#if 1 // jit -  q2 compatible code
			qglTexImage2D(GL_TEXTURE_2D, 0, format, scaled_width, scaled_height,
				0, GL_RGBA, GL_UNSIGNED_BYTE, texData);

			while (scaled_width > 1 || scaled_height > 1)
			{
				GL_MipMap((byte*)texData, scaled_width, scaled_height);
				scaled_width >>= 1;
				scaled_height >>= 1;

				if (scaled_width < 1)
					scaled_width = 1;

				if (scaled_height < 1)
					scaled_height = 1;

				miplevel++;
				qglTexImage2D(GL_TEXTURE_2D, miplevel, format, scaled_width, scaled_height,
					0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
			}
#else
			gluBuild2DMipmaps(GL_TEXTURE_2D, format, w, h, format, 
				GL_UNSIGNED_BYTE, texData);
#endif
		}
		else
		{
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, 
				GL_UNSIGNED_BYTE, texData);
		}
	}

	return texNum;
}

#if 0
// based off of R_RecursiveWorldNode,
// this locates all reflective surfaces and their associated height
// old method - delete this if you want
void R_RecursiveFindRefl (mnode_t *node)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	// MPO : if this function returns true, it means that the polygon is not visible
	// in the frustum, therefore drawing it would be a waste of resources
	if (R_CullBox(node->minmaxs, node->minmaxs+3))
		return;

	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
			if (!(r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7))))
				return;		// not visible

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
		dot = r_newrefdef.vieworg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = r_newrefdef.vieworg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = r_newrefdef.vieworg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (r_newrefdef.vieworg, plane->normal) - plane->dist;
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
	R_RecursiveFindRefl (node->children[side]);

	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		// MPO : from this point onward, we should be dealing with visible surfaces
		// start MPO
		
		// if this is a reflective surface ...
		if (surf->flags & SURF_DRAWTURB)
		{
			// and if it is flat on the Z plane ...
			if (plane->type == PLANE_Z)
				R_add_refl(surf->polys->verts[0][0], surf->polys->verts[0][1], surf->polys->verts[0][2]);
		}
		// stop MPO
	}

	// recurse down the back side
	R_RecursiveFindRefl(node->children[!side]);
}
#endif

/*
================
R_DrawDebugReflTexture

draws debug texture in game
so you can see whats going on
================
*/
void R_DrawDebugReflTexture (void)
{
	qglBindTexture(GL_TEXTURE_2D, g_refl_images[0]->texnum);	// do the first texture
	qglBegin(GL_QUADS);
	qglTexCoord2f(1, 1); qglVertex3f(0, 0, 0);
	qglTexCoord2f(0, 1); qglVertex3f(200, 0, 0);
	qglTexCoord2f(0, 0); qglVertex3f(200, 200, 0);
	qglTexCoord2f(1, 0); qglVertex3f(0, 200, 0);
	qglEnd();
}

/*
================
R_UpdateReflTex

this method renders the reflection
into the right texture (slow)
we have to draw everything a 2nd time
================
*/
void R_UpdateReflTex (refdef_t *fd)
{
	if (!g_num_refl)
		return;	// nothing to do here

	g_drawing_refl = true;	// begin drawing reflection
	g_last_known_fov = fd->fov_y;
	
	// go through each reflection and render it
	for (g_active_refl = 0; g_active_refl < g_num_refl; g_active_refl++)
	{
		//qglClearColor(0, 0, 0, 1);								//clear screen
		//qglClear(/*GL_COLOR_BUFFER_BIT |*/ GL_DEPTH_BUFFER_BIT); jitwater
#ifdef USE_FBO
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_refl_framebuffer); // todo - array of framebuffers
#endif
		qglPushAttrib(GL_VIEWPORT_BIT);
		qglViewport(0, 0, g_reflTexW, g_reflTexH);
		//qglBindTexture(GL_TEXTURE_2D, g_refl_images[g_active_refl]->texnum); // not necessary, but can't get the stupid texture rendered to
		R_RenderView(fd);	// draw the scene here!
#ifdef USE_FBO
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#else
		qglBindTexture(GL_TEXTURE_2D, g_refl_images[g_active_refl]->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0,
			0,//(REFL_TEXW - g_reflTexW) >> 1,
			0,//(REFL_TEXH - g_reflTexH) >> 1,
			0, 0, g_reflTexW, g_reflTexH);
		//qglViewport(0, 0, vid.width, vid.height);
#endif // !USE_FBO
		qglPopAttrib();

		R_Clear(); // jitwater
	}

	g_drawing_refl = false;	// done drawing refl
	// jitwater qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//clear stuff now cause we want to render scene
}															


// sets modelview to reflection instead of normal view
void R_DoReflTransform()
{
	//qglRotatef (180, 1, 0, 0);	// flip upside down (X-axis is forward)
    qglRotatef(r_newrefdef.viewangles[2],  1, 0, 0);
    qglRotatef(r_newrefdef.viewangles[0],  0, 1, 0);	// up/down rotation (reversed)
    qglRotatef(-r_newrefdef.viewangles[1], 0, 0, 1);	// left/right rotation
    qglTranslatef(-r_newrefdef.vieworg[0],
    	-r_newrefdef.vieworg[1],
    	-((2*g_refl_Z[g_active_refl]) - r_newrefdef.vieworg[2]));
}

///////////

void print_matrix(int which_matrix, const char *desc)
{
	GLfloat m[16];	// receives our matrix
	qglGetFloatv(which_matrix, m);	// snag the matrix
	
	printf("[%s]\n", desc);
	printf("%0.3f %0.3f %0.3f %0.3f\n", m[0], m[4], m[8],  m[12]);
	printf("%0.3f %0.3f %0.3f %0.3f\n", m[1], m[5], m[9],  m[13]);
	printf("%0.3f %0.3f %0.3f %0.3f\n", m[2], m[6], m[10], m[14]);
	printf("%0.3f %0.3f %0.3f %0.3f\n", m[3], m[7], m[11], m[15]);
}


// alters texture matrix to handle our reflection
void R_LoadReflMatrix (void)
{
	float aspect = (float)r_newrefdef.width/r_newrefdef.height;

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();

	qglTranslatef(0.5f, 0.5f, 0.0f); // Center texture

	qglScalef(0.5f * (float)g_reflTexW / REFL_TEXW,
			  0.5f * (float)g_reflTexH / REFL_TEXH,
			  1.0f);								/* Scale and bias */

	MYgluPerspective(g_last_known_fov, aspect, 4, 4096);

	qglRotatef(-90.0f, 1.0f, 0.0f, 0.0f);	    // put Z going up
	qglRotatef(90.0f,  0.0f, 0.0f, 1.0f);	    // put Z going up

	// do transform
	R_DoReflTransform();
	qglTranslatef(0.0f, 0.0f, 0.0f);
	qglMatrixMode(GL_MODELVIEW);
}

/*
 * Load identity into texture matrix
 */
void R_ClearReflMatrix()
{
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

// the frustum function from the Mesa3D Library
// Apparently the regular glFrustum function can be broken in certain instances?
void mesa_frustum(GLdouble left, GLdouble right,
        GLdouble bottom, GLdouble top, 
        GLdouble nearval, GLdouble farval)
{
   GLdouble x, y, a, b, c, d;
   GLdouble m[16];

   x = (2.0 * nearval) / (right - left);
   y = (2.0 * nearval) / (top - bottom);
   a = (right + left) / (right - left);
   b = (top + bottom) / (top - bottom);
   c = -(farval + nearval) / ( farval - nearval);
   d = -(2.0 * farval * nearval) / (farval - nearval);

#define M(row,col)  m[col*4+row]
   M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
   M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
   M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
   M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M

   qglMultMatrixd(m);
}








