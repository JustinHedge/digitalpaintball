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
#ifdef WIN32
#include "../win32/glw_win.h"
#else
#include "../linux/glw_linux.h"
#include <ctype.h>
#include <dlfcn.h>
char *strlwr(char*); // defined in q_shlinux.c
#endif

#ifdef WIN32
int   (WINAPI *qwglChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR *);
int   (WINAPI *qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
int   (WINAPI *qwglGetPixelFormat)(HDC);
BOOL  (WINAPI *qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
BOOL  (WINAPI *qwglSwapBuffers)(HDC);
BOOL  (WINAPI *qwglCopyContext)(HGLRC, HGLRC, UINT);
HGLRC (WINAPI *qwglCreateContext)(HDC);
HGLRC (WINAPI *qwglCreateLayerContext)(HDC, int);
BOOL  (WINAPI *qwglDeleteContext)(HGLRC);
HGLRC (WINAPI *qwglGetCurrentContext)(VOID);
HDC   (WINAPI *qwglGetCurrentDC)(VOID);
PROC  (WINAPI *qwglGetProcAddress)(LPCSTR);
BOOL  (WINAPI *qwglMakeCurrent)(HDC, HGLRC);
BOOL  (WINAPI *qwglShareLists)(HGLRC, HGLRC);
BOOL  (WINAPI *qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);
BOOL  (WINAPI *qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);
BOOL  (WINAPI *qwglDescribeLayerPlane)(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR);
int   (WINAPI *qwglSetLayerPaletteEntries)(HDC, int, int, int, CONST COLORREF *);
int   (WINAPI *qwglGetLayerPaletteEntries)(HDC, int, int, int, COLORREF *);
BOOL  (WINAPI *qwglRealizeLayerPalette)(HDC, int, BOOL);
BOOL  (WINAPI *qwglSwapLayerBuffers)(HDC, UINT);
BOOL  (WINAPI *qwglSwapIntervalEXT)(int interval);
BOOL  (WINAPI *qwglGetDeviceGammaRampEXT)(unsigned char *, unsigned char *, unsigned char *);
BOOL  (WINAPI *qwglSetDeviceGammaRampEXT)(const unsigned char *, const unsigned char *, const unsigned char *);
#endif

void ( APIENTRY * qglAccum )(GLenum op, GLfloat value);
void ( APIENTRY * qglAlphaFunc )(GLenum func, GLclampf ref);
GLboolean ( APIENTRY * qglAreTexturesResident )(GLsizei n, const GLuint *textures, GLboolean *residences);
void ( APIENTRY * qglArrayElement )(GLint i);
void ( APIENTRY * qglBegin )(GLenum mode);
void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY * qglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglCallList )(GLuint list);
void ( APIENTRY * qglCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglClearDepth )(GLclampd depth);
void ( APIENTRY * qglClearIndex )(GLfloat c);
void ( APIENTRY * qglClearStencil )(GLint s);
void ( APIENTRY * qglClipPlane )(GLenum plane, const GLdouble *equation);
void ( APIENTRY * qglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
void ( APIENTRY * qglColor3bv )(const GLbyte *v);
void ( APIENTRY * qglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
void ( APIENTRY * qglColor3dv )(const GLdouble *v);
void ( APIENTRY * qglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
void ( APIENTRY * qglColor3fv )(const GLfloat *v);
void ( APIENTRY * qglColor3i )(GLint red, GLint green, GLint blue);
void ( APIENTRY * qglColor3iv )(const GLint *v);
void ( APIENTRY * qglColor3s )(GLshort red, GLshort green, GLshort blue);
void ( APIENTRY * qglColor3sv )(const GLshort *v);
void ( APIENTRY * qglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
void ( APIENTRY * qglColor3ubv )(const GLubyte *v);
void ( APIENTRY * qglColor3ui )(GLuint red, GLuint green, GLuint blue);
void ( APIENTRY * qglColor3uiv )(const GLuint *v);
void ( APIENTRY * qglColor3us )(GLushort red, GLushort green, GLushort blue);
void ( APIENTRY * qglColor3usv )(const GLushort *v);
void ( APIENTRY * qglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void ( APIENTRY * qglColor4bv )(const GLbyte *v);
void ( APIENTRY * qglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void ( APIENTRY * qglColor4dv )(const GLdouble *v);
void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY * qglColor4fv )(const GLfloat *v);
void ( APIENTRY * qglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
void ( APIENTRY * qglColor4iv )(const GLint *v);
void ( APIENTRY * qglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
void ( APIENTRY * qglColor4sv )(const GLshort *v);
void ( APIENTRY * qglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void ( APIENTRY * qglColor4ubv )(const GLubyte *v);
void ( APIENTRY * qglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
void ( APIENTRY * qglColor4uiv )(const GLuint *v);
void ( APIENTRY * qglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
void ( APIENTRY * qglColor4usv )(const GLushort *v);
void ( APIENTRY * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ( APIENTRY * qglColorMaterial )(GLenum face, GLenum mode);
void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void ( APIENTRY * qglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void ( APIENTRY * qglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void ( APIENTRY * qglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void ( APIENTRY * qglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglDeleteLists )(GLuint list, GLsizei range);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY * qglDisable )(GLenum cap);
void ( APIENTRY * qglDisableClientState )(GLenum array);
void ( APIENTRY * qglDrawArrays )(GLenum mode, GLint first, GLsizei count);
void ( APIENTRY * qglDrawBuffer )(GLenum mode);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglEdgeFlag )(GLboolean flag);
void ( APIENTRY * qglEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglEdgeFlagv )(const GLboolean *flag);
void ( APIENTRY * qglEnable )(GLenum cap);
void ( APIENTRY * qglEnableClientState )(GLenum array);
void ( APIENTRY * qglEnd )(void);
void ( APIENTRY * qglEndList )(void);
void ( APIENTRY * qglEvalCoord1d )(GLdouble u);
void ( APIENTRY * qglEvalCoord1dv )(const GLdouble *u);
void ( APIENTRY * qglEvalCoord1f )(GLfloat u);
void ( APIENTRY * qglEvalCoord1fv )(const GLfloat *u);
void ( APIENTRY * qglEvalCoord2d )(GLdouble u, GLdouble v);
void ( APIENTRY * qglEvalCoord2dv )(const GLdouble *u);
void ( APIENTRY * qglEvalCoord2f )(GLfloat u, GLfloat v);
void ( APIENTRY * qglEvalCoord2fv )(const GLfloat *u);
void ( APIENTRY * qglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
void ( APIENTRY * qglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void ( APIENTRY * qglEvalPoint1 )(GLint i);
void ( APIENTRY * qglEvalPoint2 )(GLint i, GLint j);
void ( APIENTRY * qglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
void ( APIENTRY * qglFinish )(void);
void ( APIENTRY * qglFlush )(void);
void ( APIENTRY * qglFogf )(GLenum pname, GLfloat param);
void ( APIENTRY * qglFogfv )(GLenum pname, const GLfloat *params);
void ( APIENTRY * qglFogi )(GLenum pname, GLint param);
void ( APIENTRY * qglFogiv )(GLenum pname, const GLint *params);
void ( APIENTRY * qglFrontFace )(GLenum mode);
void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint ( APIENTRY * qglGenLists )(GLsizei range);
void ( APIENTRY * qglGenTextures )(GLsizei n, GLuint *textures);
void ( APIENTRY * qglGetBooleanv )(GLenum pname, GLboolean *params);
void ( APIENTRY * qglGetClipPlane )(GLenum plane, GLdouble *equation);
void ( APIENTRY * qglGetDoublev )(GLenum pname, GLdouble *params);
GLenum ( APIENTRY * qglGetError )(void);
void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetIntegerv )(GLenum pname, GLint *params);
void ( APIENTRY * qglGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetLightiv )(GLenum light, GLenum pname, GLint *params);
void ( APIENTRY * qglGetMapdv )(GLenum target, GLenum query, GLdouble *v);
void ( APIENTRY * qglGetMapfv )(GLenum target, GLenum query, GLfloat *v);
void ( APIENTRY * qglGetMapiv )(GLenum target, GLenum query, GLint *v);
void ( APIENTRY * qglGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
void ( APIENTRY * qglGetPixelMapfv )(GLenum map, GLfloat *values);
void ( APIENTRY * qglGetPixelMapuiv )(GLenum map, GLuint *values);
void ( APIENTRY * qglGetPixelMapusv )(GLenum map, GLushort *values);
void ( APIENTRY * qglGetPointerv )(GLenum pname, GLvoid* *params);
void ( APIENTRY * qglGetPolygonStipple )(GLubyte *mask);
const GLubyte * ( APIENTRY * qglGetString )(GLenum name);
void ( APIENTRY * qglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
void ( APIENTRY * qglGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
void ( APIENTRY * qglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
void ( APIENTRY * qglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
void ( APIENTRY * qglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
void ( APIENTRY * qglHint )(GLenum target, GLenum mode);
void ( APIENTRY * qglIndexMask )(GLuint mask);
void ( APIENTRY * qglIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglIndexd )(GLdouble c);
void ( APIENTRY * qglIndexdv )(const GLdouble *c);
void ( APIENTRY * qglIndexf )(GLfloat c);
void ( APIENTRY * qglIndexfv )(const GLfloat *c);
void ( APIENTRY * qglIndexi )(GLint c);
void ( APIENTRY * qglIndexiv )(const GLint *c);
void ( APIENTRY * qglIndexs )(GLshort c);
void ( APIENTRY * qglIndexsv )(const GLshort *c);
void ( APIENTRY * qglIndexub )(GLubyte c);
void ( APIENTRY * qglIndexubv )(const GLubyte *c);
void ( APIENTRY * qglInitNames )(void);
void ( APIENTRY * qglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean ( APIENTRY * qglIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * qglIsList )(GLuint list);
GLboolean ( APIENTRY * qglIsTexture )(GLuint texture);
void ( APIENTRY * qglLightModelf )(GLenum pname, GLfloat param);
void ( APIENTRY * qglLightModelfv )(GLenum pname, const GLfloat *params);
void ( APIENTRY * qglLightModeli )(GLenum pname, GLint param);
void ( APIENTRY * qglLightModeliv )(GLenum pname, const GLint *params);
void ( APIENTRY * qglLightf )(GLenum light, GLenum pname, GLfloat param);
void ( APIENTRY * qglLightfv )(GLenum light, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglLighti )(GLenum light, GLenum pname, GLint param);
void ( APIENTRY * qglLightiv )(GLenum light, GLenum pname, const GLint *params);
void ( APIENTRY * qglLineStipple )(GLint factor, GLushort pattern);
void ( APIENTRY * qglLineWidth )(GLfloat width);
void ( APIENTRY * qglListBase )(GLuint base);
void ( APIENTRY * qglLoadIdentity )(void);
void ( APIENTRY * qglLoadMatrixd )(const GLdouble *m);
void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY * qglLoadName )(GLuint name);
void ( APIENTRY * qglLogicOp )(GLenum opcode);
void ( APIENTRY * qglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void ( APIENTRY * qglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void ( APIENTRY * qglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void ( APIENTRY * qglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void ( APIENTRY * qglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
void ( APIENTRY * qglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
void ( APIENTRY * qglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void ( APIENTRY * qglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void ( APIENTRY * qglMaterialf )(GLenum face, GLenum pname, GLfloat param);
void ( APIENTRY * qglMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglMateriali )(GLenum face, GLenum pname, GLint param);
void ( APIENTRY * qglMaterialiv )(GLenum face, GLenum pname, const GLint *params);
void ( APIENTRY * qglMatrixMode )(GLenum mode);
void ( APIENTRY * qglMultMatrixd )(const GLdouble *m);
void ( APIENTRY * qglMultMatrixf )(const GLfloat *m);
void ( APIENTRY * qglNewList )(GLuint list, GLenum mode);
void ( APIENTRY * qglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
void ( APIENTRY * qglNormal3bv )(const GLbyte *v);
void ( APIENTRY * qglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
void ( APIENTRY * qglNormal3dv )(const GLdouble *v);
void ( APIENTRY * qglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
void ( APIENTRY * qglNormal3fv )(const GLfloat *v);
void ( APIENTRY * qglNormal3i )(GLint nx, GLint ny, GLint nz);
void ( APIENTRY * qglNormal3iv )(const GLint *v);
void ( APIENTRY * qglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
void ( APIENTRY * qglNormal3sv )(const GLshort *v);
void ( APIENTRY * qglNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglPassThrough )(GLfloat token);
void ( APIENTRY * qglPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
void ( APIENTRY * qglPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
void ( APIENTRY * qglPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
void ( APIENTRY * qglPixelStoref )(GLenum pname, GLfloat param);
void ( APIENTRY * qglPixelStorei )(GLenum pname, GLint param);
void ( APIENTRY * qglPixelTransferf )(GLenum pname, GLfloat param);
void ( APIENTRY * qglPixelTransferi )(GLenum pname, GLint param);
void ( APIENTRY * qglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
void ( APIENTRY * qglPointSize )(GLfloat size);
void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY * qglPolygonOffset )(GLfloat factor, GLfloat units);
void ( APIENTRY * qglPolygonStipple )(const GLubyte *mask);
void ( APIENTRY * qglPopAttrib )(void);
void ( APIENTRY * qglPopClientAttrib )(void);
void ( APIENTRY * qglPopMatrix )(void);
void ( APIENTRY * qglPopName )(void);
void ( APIENTRY * qglPrioritizeTextures )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
void ( APIENTRY * qglPushAttrib )(GLbitfield mask);
void ( APIENTRY * qglPushClientAttrib )(GLbitfield mask);
void ( APIENTRY * qglPushMatrix )(void);
void ( APIENTRY * qglPushName )(GLuint name);
void ( APIENTRY * qglRasterPos2d )(GLdouble x, GLdouble y);
void ( APIENTRY * qglRasterPos2dv )(const GLdouble *v);
void ( APIENTRY * qglRasterPos2f )(GLfloat x, GLfloat y);
void ( APIENTRY * qglRasterPos2fv )(const GLfloat *v);
void ( APIENTRY * qglRasterPos2i )(GLint x, GLint y);
void ( APIENTRY * qglRasterPos2iv )(const GLint *v);
void ( APIENTRY * qglRasterPos2s )(GLshort x, GLshort y);
void ( APIENTRY * qglRasterPos2sv )(const GLshort *v);
void ( APIENTRY * qglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglRasterPos3dv )(const GLdouble *v);
void ( APIENTRY * qglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglRasterPos3fv )(const GLfloat *v);
void ( APIENTRY * qglRasterPos3i )(GLint x, GLint y, GLint z);
void ( APIENTRY * qglRasterPos3iv )(const GLint *v);
void ( APIENTRY * qglRasterPos3s )(GLshort x, GLshort y, GLshort z);
void ( APIENTRY * qglRasterPos3sv )(const GLshort *v);
void ( APIENTRY * qglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( APIENTRY * qglRasterPos4dv )(const GLdouble *v);
void ( APIENTRY * qglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY * qglRasterPos4fv )(const GLfloat *v);
void ( APIENTRY * qglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
void ( APIENTRY * qglRasterPos4iv )(const GLint *v);
void ( APIENTRY * qglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( APIENTRY * qglRasterPos4sv )(const GLshort *v);
void ( APIENTRY * qglReadBuffer )(GLenum mode);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void ( APIENTRY * qglRectdv )(const GLdouble *v1, const GLdouble *v2);
void ( APIENTRY * qglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void ( APIENTRY * qglRectfv )(const GLfloat *v1, const GLfloat *v2);
void ( APIENTRY * qglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
void ( APIENTRY * qglRectiv )(const GLint *v1, const GLint *v2);
void ( APIENTRY * qglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void ( APIENTRY * qglRectsv )(const GLshort *v1, const GLshort *v2);
GLint ( APIENTRY * qglRenderMode )(GLenum mode);
void ( APIENTRY * qglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglScaled )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglSelectBuffer )(GLsizei size, GLuint *buffer);
void ( APIENTRY * qglShadeModel )(GLenum mode);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilMask )(GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglTexCoord1d )(GLdouble s);
void ( APIENTRY * qglTexCoord1dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord1f )(GLfloat s);
void ( APIENTRY * qglTexCoord1fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord1i )(GLint s);
void ( APIENTRY * qglTexCoord1iv )(const GLint *v);
void ( APIENTRY * qglTexCoord1s )(GLshort s);
void ( APIENTRY * qglTexCoord1sv )(const GLshort *v);
void ( APIENTRY * qglTexCoord2d )(GLdouble s, GLdouble t);
void ( APIENTRY * qglTexCoord2dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord2f )(GLfloat s, GLfloat t);
void ( APIENTRY * qglTexCoord2fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord2i )(GLint s, GLint t);
void ( APIENTRY * qglTexCoord2iv )(const GLint *v);
void ( APIENTRY * qglTexCoord2s )(GLshort s, GLshort t);
void ( APIENTRY * qglTexCoord2sv )(const GLshort *v);
void ( APIENTRY * qglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
void ( APIENTRY * qglTexCoord3dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
void ( APIENTRY * qglTexCoord3fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord3i )(GLint s, GLint t, GLint r);
void ( APIENTRY * qglTexCoord3iv )(const GLint *v);
void ( APIENTRY * qglTexCoord3s )(GLshort s, GLshort t, GLshort r);
void ( APIENTRY * qglTexCoord3sv )(const GLshort *v);
void ( APIENTRY * qglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void ( APIENTRY * qglTexCoord4dv )(const GLdouble *v);
void ( APIENTRY * qglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void ( APIENTRY * qglTexCoord4fv )(const GLfloat *v);
void ( APIENTRY * qglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
void ( APIENTRY * qglTexCoord4iv )(const GLint *v);
void ( APIENTRY * qglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
void ( APIENTRY * qglTexCoord4sv )(const GLshort *v);
void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglTexEnviv )(GLenum target, GLenum pname, const GLint *params);
void ( APIENTRY * qglTexGend )(GLenum coord, GLenum pname, GLdouble param);
void ( APIENTRY * qglTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
void ( APIENTRY * qglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglTexGeni )(GLenum coord, GLenum pname, GLint param);
void ( APIENTRY * qglTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
void ( APIENTRY * qglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
void ( APIENTRY * qglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTranslated )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglVertex2d )(GLdouble x, GLdouble y);
void ( APIENTRY * qglVertex2dv )(const GLdouble *v);
void ( APIENTRY * qglVertex2f )(GLfloat x, GLfloat y);
void ( APIENTRY * qglVertex2fv )(const GLfloat *v);
void ( APIENTRY * qglVertex2i )(GLint x, GLint y);
void ( APIENTRY * qglVertex2iv )(const GLint *v);
void ( APIENTRY * qglVertex2s )(GLshort x, GLshort y);
void ( APIENTRY * qglVertex2sv )(const GLshort *v);
void ( APIENTRY * qglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY * qglVertex3dv )(const GLdouble *v);
void ( APIENTRY * qglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglVertex3fv )(const GLfloat *v);
void ( APIENTRY * qglVertex3i )(GLint x, GLint y, GLint z);
void ( APIENTRY * qglVertex3iv )(const GLint *v);
void ( APIENTRY * qglVertex3s )(GLshort x, GLshort y, GLshort z);
void ( APIENTRY * qglVertex3sv )(const GLshort *v);
void ( APIENTRY * qglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( APIENTRY * qglVertex4dv )(const GLdouble *v);
void ( APIENTRY * qglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY * qglVertex4fv )(const GLfloat *v);
void ( APIENTRY * qglVertex4i )(GLint x, GLint y, GLint z, GLint w);
void ( APIENTRY * qglVertex4iv )(const GLint *v);
void ( APIENTRY * qglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( APIENTRY * qglVertex4sv )(const GLshort *v);
void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

void ( APIENTRY * qglLockArraysEXT )( int, int );
void ( APIENTRY * qglUnlockArraysEXT )( void );

void ( APIENTRY * qglPointParameterfEXT )( GLenum param, GLfloat value );
void ( APIENTRY * qglPointParameterfvEXT )( GLenum param, const GLfloat *value );
void ( APIENTRY * qglColorTableEXT )( GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid * );
void ( APIENTRY * qglSelectTextureSGIS )( GLenum );
void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum, GLfloat, GLfloat );
void ( APIENTRY * qglActiveTextureARB )( GLenum );
void ( APIENTRY * qglClientActiveTextureARB )( GLenum );
void ( APIENTRY * qglMultiTexCoord3fARB )( GLenum, GLfloat, GLfloat, GLfloat );
void ( APIENTRY * qglMultiTexCoord3fvARB )( GLenum, GLfloat * );

static void ( APIENTRY * dllAccum )(GLenum op, GLfloat value);
static void ( APIENTRY * dllAlphaFunc )(GLenum func, GLclampf ref);
GLboolean ( APIENTRY * dllAreTexturesResident )(GLsizei n, const GLuint *textures, GLboolean *residences);
static void ( APIENTRY * dllArrayElement )(GLint i);
static void ( APIENTRY * dllBegin )(GLenum mode);
static void ( APIENTRY * dllBindTexture )(GLenum target, GLuint texture);
static void ( APIENTRY * dllBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
static void ( APIENTRY * dllBlendFunc )(GLenum sfactor, GLenum dfactor);
static void ( APIENTRY * dllCallList )(GLuint list);
static void ( APIENTRY * dllCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
static void ( APIENTRY * dllClear )(GLbitfield mask);
static void ( APIENTRY * dllClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void ( APIENTRY * dllClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static void ( APIENTRY * dllClearDepth )(GLclampd depth);
static void ( APIENTRY * dllClearIndex )(GLfloat c);
static void ( APIENTRY * dllClearStencil )(GLint s);
static void ( APIENTRY * dllClipPlane )(GLenum plane, const GLdouble *equation);
static void ( APIENTRY * dllColor3b )(GLbyte red, GLbyte green, GLbyte blue);
static void ( APIENTRY * dllColor3bv )(const GLbyte *v);
static void ( APIENTRY * dllColor3d )(GLdouble red, GLdouble green, GLdouble blue);
static void ( APIENTRY * dllColor3dv )(const GLdouble *v);
static void ( APIENTRY * dllColor3f )(GLfloat red, GLfloat green, GLfloat blue);
static void ( APIENTRY * dllColor3fv )(const GLfloat *v);
static void ( APIENTRY * dllColor3i )(GLint red, GLint green, GLint blue);
static void ( APIENTRY * dllColor3iv )(const GLint *v);
static void ( APIENTRY * dllColor3s )(GLshort red, GLshort green, GLshort blue);
static void ( APIENTRY * dllColor3sv )(const GLshort *v);
static void ( APIENTRY * dllColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
static void ( APIENTRY * dllColor3ubv )(const GLubyte *v);
static void ( APIENTRY * dllColor3ui )(GLuint red, GLuint green, GLuint blue);
static void ( APIENTRY * dllColor3uiv )(const GLuint *v);
static void ( APIENTRY * dllColor3us )(GLushort red, GLushort green, GLushort blue);
static void ( APIENTRY * dllColor3usv )(const GLushort *v);
static void ( APIENTRY * dllColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
static void ( APIENTRY * dllColor4bv )(const GLbyte *v);
static void ( APIENTRY * dllColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
static void ( APIENTRY * dllColor4dv )(const GLdouble *v);
static void ( APIENTRY * dllColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void ( APIENTRY * dllColor4fv )(const GLfloat *v);
static void ( APIENTRY * dllColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
static void ( APIENTRY * dllColor4iv )(const GLint *v);
static void ( APIENTRY * dllColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
static void ( APIENTRY * dllColor4sv )(const GLshort *v);
static void ( APIENTRY * dllColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
static void ( APIENTRY * dllColor4ubv )(const GLubyte *v);
static void ( APIENTRY * dllColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
static void ( APIENTRY * dllColor4uiv )(const GLuint *v);
static void ( APIENTRY * dllColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
static void ( APIENTRY * dllColor4usv )(const GLushort *v);
static void ( APIENTRY * dllColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static void ( APIENTRY * dllColorMaterial )(GLenum face, GLenum mode);
static void ( APIENTRY * dllColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
static void ( APIENTRY * dllCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
static void ( APIENTRY * dllCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static void ( APIENTRY * dllCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
static void ( APIENTRY * dllCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllCullFace )(GLenum mode);
static void ( APIENTRY * dllDeleteLists )(GLuint list, GLsizei range);
static void ( APIENTRY * dllDeleteTextures )(GLsizei n, const GLuint *textures);
static void ( APIENTRY * dllDepthFunc )(GLenum func);
static void ( APIENTRY * dllDepthMask )(GLboolean flag);
static void ( APIENTRY * dllDepthRange )(GLclampd zNear, GLclampd zFar);
static void ( APIENTRY * dllDisable )(GLenum cap);
static void ( APIENTRY * dllDisableClientState )(GLenum array);
static void ( APIENTRY * dllDrawArrays )(GLenum mode, GLint first, GLsizei count);
static void ( APIENTRY * dllDrawBuffer )(GLenum mode);
static void ( APIENTRY * dllDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static void ( APIENTRY * dllDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllEdgeFlag )(GLboolean flag);
static void ( APIENTRY * dllEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllEdgeFlagv )(const GLboolean *flag);
static void ( APIENTRY * dllEnable )(GLenum cap);
static void ( APIENTRY * dllEnableClientState )(GLenum array);
static void ( APIENTRY * dllEnd )(void);
static void ( APIENTRY * dllEndList )(void);
static void ( APIENTRY * dllEvalCoord1d )(GLdouble u);
static void ( APIENTRY * dllEvalCoord1dv )(const GLdouble *u);
static void ( APIENTRY * dllEvalCoord1f )(GLfloat u);
static void ( APIENTRY * dllEvalCoord1fv )(const GLfloat *u);
static void ( APIENTRY * dllEvalCoord2d )(GLdouble u, GLdouble v);
static void ( APIENTRY * dllEvalCoord2dv )(const GLdouble *u);
static void ( APIENTRY * dllEvalCoord2f )(GLfloat u, GLfloat v);
static void ( APIENTRY * dllEvalCoord2fv )(const GLfloat *u);
static void ( APIENTRY * dllEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
static void ( APIENTRY * dllEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
static void ( APIENTRY * dllEvalPoint1 )(GLint i);
static void ( APIENTRY * dllEvalPoint2 )(GLint i, GLint j);
static void ( APIENTRY * dllFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
static void ( APIENTRY * dllFinish )(void);
static void ( APIENTRY * dllFlush )(void);
static void ( APIENTRY * dllFogf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllFogfv )(GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllFogi )(GLenum pname, GLint param);
static void ( APIENTRY * dllFogiv )(GLenum pname, const GLint *params);
static void ( APIENTRY * dllFrontFace )(GLenum mode);
static void ( APIENTRY * dllFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint ( APIENTRY * dllGenLists )(GLsizei range);
static void ( APIENTRY * dllGenTextures )(GLsizei n, GLuint *textures);
static void ( APIENTRY * dllGetBooleanv )(GLenum pname, GLboolean *params);
static void ( APIENTRY * dllGetClipPlane )(GLenum plane, GLdouble *equation);
static void ( APIENTRY * dllGetDoublev )(GLenum pname, GLdouble *params);
GLenum ( APIENTRY * dllGetError )(void);
static void ( APIENTRY * dllGetFloatv )(GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetIntegerv )(GLenum pname, GLint *params);
static void ( APIENTRY * dllGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetLightiv )(GLenum light, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetMapdv )(GLenum target, GLenum query, GLdouble *v);
static void ( APIENTRY * dllGetMapfv )(GLenum target, GLenum query, GLfloat *v);
static void ( APIENTRY * dllGetMapiv )(GLenum target, GLenum query, GLint *v);
static void ( APIENTRY * dllGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetPixelMapfv )(GLenum map, GLfloat *values);
static void ( APIENTRY * dllGetPixelMapuiv )(GLenum map, GLuint *values);
static void ( APIENTRY * dllGetPixelMapusv )(GLenum map, GLushort *values);
static void ( APIENTRY * dllGetPointerv )(GLenum pname, GLvoid* *params);
static void ( APIENTRY * dllGetPolygonStipple )(GLubyte *mask);
const GLubyte * ( APIENTRY * dllGetString )(GLenum name);
static void ( APIENTRY * dllGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
static void ( APIENTRY * dllGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
static void ( APIENTRY * dllGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
static void ( APIENTRY * dllHint )(GLenum target, GLenum mode);
static void ( APIENTRY * dllIndexMask )(GLuint mask);
static void ( APIENTRY * dllIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllIndexd )(GLdouble c);
static void ( APIENTRY * dllIndexdv )(const GLdouble *c);
static void ( APIENTRY * dllIndexf )(GLfloat c);
static void ( APIENTRY * dllIndexfv )(const GLfloat *c);
static void ( APIENTRY * dllIndexi )(GLint c);
static void ( APIENTRY * dllIndexiv )(const GLint *c);
static void ( APIENTRY * dllIndexs )(GLshort c);
static void ( APIENTRY * dllIndexsv )(const GLshort *c);
static void ( APIENTRY * dllIndexub )(GLubyte c);
static void ( APIENTRY * dllIndexubv )(const GLubyte *c);
static void ( APIENTRY * dllInitNames )(void);
static void ( APIENTRY * dllInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean ( APIENTRY * dllIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * dllIsList )(GLuint list);
GLboolean ( APIENTRY * dllIsTexture )(GLuint texture);
static void ( APIENTRY * dllLightModelf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllLightModelfv )(GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllLightModeli )(GLenum pname, GLint param);
static void ( APIENTRY * dllLightModeliv )(GLenum pname, const GLint *params);
static void ( APIENTRY * dllLightf )(GLenum light, GLenum pname, GLfloat param);
static void ( APIENTRY * dllLightfv )(GLenum light, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllLighti )(GLenum light, GLenum pname, GLint param);
static void ( APIENTRY * dllLightiv )(GLenum light, GLenum pname, const GLint *params);
static void ( APIENTRY * dllLineStipple )(GLint factor, GLushort pattern);
static void ( APIENTRY * dllLineWidth )(GLfloat width);
static void ( APIENTRY * dllListBase )(GLuint base);
static void ( APIENTRY * dllLoadIdentity )(void);
static void ( APIENTRY * dllLoadMatrixd )(const GLdouble *m);
static void ( APIENTRY * dllLoadMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllLoadName )(GLuint name);
static void ( APIENTRY * dllLogicOp )(GLenum opcode);
static void ( APIENTRY * dllMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
static void ( APIENTRY * dllMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
static void ( APIENTRY * dllMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
static void ( APIENTRY * dllMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
static void ( APIENTRY * dllMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
static void ( APIENTRY * dllMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
static void ( APIENTRY * dllMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
static void ( APIENTRY * dllMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
static void ( APIENTRY * dllMaterialf )(GLenum face, GLenum pname, GLfloat param);
static void ( APIENTRY * dllMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllMateriali )(GLenum face, GLenum pname, GLint param);
static void ( APIENTRY * dllMaterialiv )(GLenum face, GLenum pname, const GLint *params);
static void ( APIENTRY * dllMatrixMode )(GLenum mode);
static void ( APIENTRY * dllMultMatrixd )(const GLdouble *m);
static void ( APIENTRY * dllMultMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllNewList )(GLuint list, GLenum mode);
static void ( APIENTRY * dllNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
static void ( APIENTRY * dllNormal3bv )(const GLbyte *v);
static void ( APIENTRY * dllNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
static void ( APIENTRY * dllNormal3dv )(const GLdouble *v);
static void ( APIENTRY * dllNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
static void ( APIENTRY * dllNormal3fv )(const GLfloat *v);
static void ( APIENTRY * dllNormal3i )(GLint nx, GLint ny, GLint nz);
static void ( APIENTRY * dllNormal3iv )(const GLint *v);
static void ( APIENTRY * dllNormal3s )(GLshort nx, GLshort ny, GLshort nz);
static void ( APIENTRY * dllNormal3sv )(const GLshort *v);
static void ( APIENTRY * dllNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static void ( APIENTRY * dllPassThrough )(GLfloat token);
static void ( APIENTRY * dllPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
static void ( APIENTRY * dllPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
static void ( APIENTRY * dllPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
static void ( APIENTRY * dllPixelStoref )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllPixelStorei )(GLenum pname, GLint param);
static void ( APIENTRY * dllPixelTransferf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllPixelTransferi )(GLenum pname, GLint param);
static void ( APIENTRY * dllPixelZoom )(GLfloat xfactor, GLfloat yfactor);
static void ( APIENTRY * dllPointSize )(GLfloat size);
static void ( APIENTRY * dllPolygonMode )(GLenum face, GLenum mode);
static void ( APIENTRY * dllPolygonOffset )(GLfloat factor, GLfloat units);
static void ( APIENTRY * dllPolygonStipple )(const GLubyte *mask);
static void ( APIENTRY * dllPopAttrib )(void);
static void ( APIENTRY * dllPopClientAttrib )(void);
static void ( APIENTRY * dllPopMatrix )(void);
static void ( APIENTRY * dllPopName )(void);
static void ( APIENTRY * dllPrioritizeTextures )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
static void ( APIENTRY * dllPushAttrib )(GLbitfield mask);
static void ( APIENTRY * dllPushClientAttrib )(GLbitfield mask);
static void ( APIENTRY * dllPushMatrix )(void);
static void ( APIENTRY * dllPushName )(GLuint name);
static void ( APIENTRY * dllRasterPos2d )(GLdouble x, GLdouble y);
static void ( APIENTRY * dllRasterPos2dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos2f )(GLfloat x, GLfloat y);
static void ( APIENTRY * dllRasterPos2fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos2i )(GLint x, GLint y);
static void ( APIENTRY * dllRasterPos2iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos2s )(GLshort x, GLshort y);
static void ( APIENTRY * dllRasterPos2sv )(const GLshort *v);
static void ( APIENTRY * dllRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllRasterPos3dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllRasterPos3fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos3i )(GLint x, GLint y, GLint z);
static void ( APIENTRY * dllRasterPos3iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos3s )(GLshort x, GLshort y, GLshort z);
static void ( APIENTRY * dllRasterPos3sv )(const GLshort *v);
static void ( APIENTRY * dllRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void ( APIENTRY * dllRasterPos4dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void ( APIENTRY * dllRasterPos4fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
static void ( APIENTRY * dllRasterPos4iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
static void ( APIENTRY * dllRasterPos4sv )(const GLshort *v);
static void ( APIENTRY * dllReadBuffer )(GLenum mode);
static void ( APIENTRY * dllReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static void ( APIENTRY * dllRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
static void ( APIENTRY * dllRectdv )(const GLdouble *v1, const GLdouble *v2);
static void ( APIENTRY * dllRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
static void ( APIENTRY * dllRectfv )(const GLfloat *v1, const GLfloat *v2);
static void ( APIENTRY * dllRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
static void ( APIENTRY * dllRectiv )(const GLint *v1, const GLint *v2);
static void ( APIENTRY * dllRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
static void ( APIENTRY * dllRectsv )(const GLshort *v1, const GLshort *v2);
GLint ( APIENTRY * dllRenderMode )(GLenum mode);
static void ( APIENTRY * dllRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScaled )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllScalef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllSelectBuffer )(GLsizei size, GLuint *buffer);
static void ( APIENTRY * dllShadeModel )(GLenum mode);
static void ( APIENTRY * dllStencilFunc )(GLenum func, GLint ref, GLuint mask);
static void ( APIENTRY * dllStencilMask )(GLuint mask);
static void ( APIENTRY * dllStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
static void ( APIENTRY * dllTexCoord1d )(GLdouble s);
static void ( APIENTRY * dllTexCoord1dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord1f )(GLfloat s);
static void ( APIENTRY * dllTexCoord1fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord1i )(GLint s);
static void ( APIENTRY * dllTexCoord1iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord1s )(GLshort s);
static void ( APIENTRY * dllTexCoord1sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord2d )(GLdouble s, GLdouble t);
static void ( APIENTRY * dllTexCoord2dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord2f )(GLfloat s, GLfloat t);
static void ( APIENTRY * dllTexCoord2fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord2i )(GLint s, GLint t);
static void ( APIENTRY * dllTexCoord2iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord2s )(GLshort s, GLshort t);
static void ( APIENTRY * dllTexCoord2sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
static void ( APIENTRY * dllTexCoord3dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
static void ( APIENTRY * dllTexCoord3fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord3i )(GLint s, GLint t, GLint r);
static void ( APIENTRY * dllTexCoord3iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord3s )(GLshort s, GLshort t, GLshort r);
static void ( APIENTRY * dllTexCoord3sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
static void ( APIENTRY * dllTexCoord4dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
static void ( APIENTRY * dllTexCoord4fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
static void ( APIENTRY * dllTexCoord4iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
static void ( APIENTRY * dllTexCoord4sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllTexEnvf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexEnvi )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexEnviv )(GLenum target, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexGend )(GLenum coord, GLenum pname, GLdouble param);
static void ( APIENTRY * dllTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
static void ( APIENTRY * dllTexGenf )(GLenum coord, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexGeni )(GLenum coord, GLenum pname, GLint param);
static void ( APIENTRY * dllTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexParameterf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexParameteri )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTranslated )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllTranslatef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertex2d )(GLdouble x, GLdouble y);
static void ( APIENTRY * dllVertex2dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex2f )(GLfloat x, GLfloat y);
static void ( APIENTRY * dllVertex2fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex2i )(GLint x, GLint y);
static void ( APIENTRY * dllVertex2iv )(const GLint *v);
static void ( APIENTRY * dllVertex2s )(GLshort x, GLshort y);
static void ( APIENTRY * dllVertex2sv )(const GLshort *v);
static void ( APIENTRY * dllVertex3d )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllVertex3dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex3f )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertex3fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex3i )(GLint x, GLint y, GLint z);
static void ( APIENTRY * dllVertex3iv )(const GLint *v);
static void ( APIENTRY * dllVertex3s )(GLshort x, GLshort y, GLshort z);
static void ( APIENTRY * dllVertex3sv )(const GLshort *v);
static void ( APIENTRY * dllVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void ( APIENTRY * dllVertex4dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void ( APIENTRY * dllVertex4fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex4i )(GLint x, GLint y, GLint z, GLint w);
static void ( APIENTRY * dllVertex4iv )(const GLint *v);
static void ( APIENTRY * dllVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
static void ( APIENTRY * dllVertex4sv )(const GLshort *v);
static void ( APIENTRY * dllVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

static void APIENTRY logAccum(GLenum op, GLfloat value)
{
	fprintf( glw_state.log_fp, "glAccum\n" );
	dllAccum( op, value );
}

static void APIENTRY logAlphaFunc(GLenum func, GLclampf ref)
{
	fprintf( glw_state.log_fp, "glAlphaFunc( 0x%x, %f )\n", func, ref );
	dllAlphaFunc( func, ref );
}

static GLboolean APIENTRY logAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
	fprintf( glw_state.log_fp, "glAreTexturesResident\n" );
	return dllAreTexturesResident( n, textures, residences );
}

static void APIENTRY logArrayElement(GLint i)
{
	fprintf( glw_state.log_fp, "glArrayElement\n" );
	dllArrayElement( i );
}

static void APIENTRY logBegin(GLenum mode)
{
	fprintf( glw_state.log_fp, "glBegin( 0x%x )\n", mode );
	dllBegin( mode );
}

static void APIENTRY logBindTexture(GLenum target, GLuint texture)
{
	fprintf( glw_state.log_fp, "glBindTexture( 0x%x, %u )\n", target, texture );
	dllBindTexture( target, texture );
}

static void APIENTRY logBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	fprintf( glw_state.log_fp, "glBitmap\n" );
	dllBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
}

static void APIENTRY logBlendFunc(GLenum sfactor, GLenum dfactor)
{
	fprintf( glw_state.log_fp, "glBlendFunc( 0x%x, 0x%x )\n", sfactor, dfactor );
	dllBlendFunc( sfactor, dfactor );
}

static void APIENTRY logCallList(GLuint list)
{
	fprintf( glw_state.log_fp, "glCallList( %u )\n", list );
	dllCallList( list );
}

static void APIENTRY logCallLists(GLsizei n, GLenum type, const void *lists)
{
	fprintf( glw_state.log_fp, "glCallLists\n" );
	dllCallLists( n, type, lists );
}

static void APIENTRY logClear(GLbitfield mask)
{
	fprintf( glw_state.log_fp, "glClear\n" );
	dllClear( mask );
}

static void APIENTRY logClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	fprintf( glw_state.log_fp, "glClearAccum\n" );
	dllClearAccum( red, green, blue, alpha );
}

static void APIENTRY logClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	fprintf( glw_state.log_fp, "glClearColor\n" );
	dllClearColor( red, green, blue, alpha );
}

static void APIENTRY logClearDepth(GLclampd depth)
{
	fprintf( glw_state.log_fp, "glClearDepth\n" );
	dllClearDepth( depth );
}

static void APIENTRY logClearIndex(GLfloat c)
{
	fprintf( glw_state.log_fp, "glClearIndex\n" );
	dllClearIndex( c );
}

static void APIENTRY logClearStencil(GLint s)
{
	fprintf( glw_state.log_fp, "glClearStencil\n" );
	dllClearStencil( s );
}

static void APIENTRY logClipPlane(GLenum plane, const GLdouble *equation)
{
	fprintf( glw_state.log_fp, "glClipPlane\n" );
	dllClipPlane( plane, equation );
}

static void APIENTRY logColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
	fprintf( glw_state.log_fp, "glColor3b\n" );
	dllColor3b( red, green, blue );
}

static void APIENTRY logColor3bv(const GLbyte *v)
{
	fprintf( glw_state.log_fp, "glColor3bv\n" );
	dllColor3bv( v );
}

static void APIENTRY logColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
	fprintf( glw_state.log_fp, "glColor3d\n" );
	dllColor3d( red, green, blue );
}

static void APIENTRY logColor3dv(const GLdouble *v)
{
	fprintf( glw_state.log_fp, "glColor3dv\n" );
	dllColor3dv( v );
}

static void APIENTRY logColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	fprintf( glw_state.log_fp, "glColor3f\n" );
	dllColor3f( red, green, blue );
}

static void APIENTRY logColor3fv(const GLfloat *v)
{
	fprintf( glw_state.log_fp, "glColor3fv\n" );
	dllColor3fv( v );
}

static void APIENTRY logColor3i(GLint red, GLint green, GLint blue)
{
	fprintf( glw_state.log_fp, "glColor3i\n" );
	dllColor3i( red, green, blue );
}

static void APIENTRY logColor3iv(const GLint *v)
{
	fprintf( glw_state.log_fp, "glColor3iv\n" );
	dllColor3iv( v );
}

static void APIENTRY logColor3s(GLshort red, GLshort green, GLshort blue)
{
	fprintf( glw_state.log_fp, "glColor3s\n" );
	dllColor3s( red, green, blue );
}

static void APIENTRY logColor3sv(const GLshort *v)
{
	fprintf( glw_state.log_fp, "glColor3sv\n" );
	dllColor3sv( v );
}

static void APIENTRY logColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
	fprintf( glw_state.log_fp, "glColor3ub\n" );
	dllColor3ub( red, green, blue );
}

static void APIENTRY logColor3ubv(const GLubyte *v)
{
	fprintf( glw_state.log_fp, "glColor3ubv\n" );
	dllColor3ubv( v );
}

#define SIG( x ) fprintf( glw_state.log_fp, x "\n" )

static void APIENTRY logColor3ui(GLuint red, GLuint green, GLuint blue)
{
	SIG( "glColor3ui" );
	dllColor3ui( red, green, blue );
}

static void APIENTRY logColor3uiv(const GLuint *v)
{
	SIG( "glColor3uiv" );
	dllColor3uiv( v );
}

static void APIENTRY logColor3us(GLushort red, GLushort green, GLushort blue)
{
	SIG( "glColor3us" );
	dllColor3us( red, green, blue );
}

static void APIENTRY logColor3usv(const GLushort *v)
{
	SIG( "glColor3usv" );
	dllColor3usv( v );
}

static void APIENTRY logColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
	SIG( "glColor4b" );
	dllColor4b( red, green, blue, alpha );
}

static void APIENTRY logColor4bv(const GLbyte *v)
{
	SIG( "glColor4bv" );
	dllColor4bv( v );
}

static void APIENTRY logColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
	SIG( "glColor4d" );
	dllColor4d( red, green, blue, alpha );
}
static void APIENTRY logColor4dv(const GLdouble *v)
{
	SIG( "glColor4dv" );
	dllColor4dv( v );
}
static void APIENTRY logColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	fprintf( glw_state.log_fp, "glColor4f( %f,%f,%f,%f )\n", red, green, blue, alpha );
	dllColor4f( red, green, blue, alpha );
}
static void APIENTRY logColor4fv(const GLfloat *v)
{
	fprintf( glw_state.log_fp, "glColor4fv( %f,%f,%f,%f )\n", v[0], v[1], v[2], v[3] );
	dllColor4fv( v );
}
static void APIENTRY logColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
	SIG( "glColor4i" );
	dllColor4i( red, green, blue, alpha );
}
static void APIENTRY logColor4iv(const GLint *v)
{
	SIG( "glColor4iv" );
	dllColor4iv( v );
}
static void APIENTRY logColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
	SIG( "glColor4s" );
	dllColor4s( red, green, blue, alpha );
}
static void APIENTRY logColor4sv(const GLshort *v)
{
	SIG( "glColor4sv" );
	dllColor4sv( v );
}
static void APIENTRY logColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	SIG( "glColor4b" );
	dllColor4b( red, green, blue, alpha );
}
static void APIENTRY logColor4ubv(const GLubyte *v)
{
	SIG( "glColor4ubv" );
	dllColor4ubv( v );
}
static void APIENTRY logColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
	SIG( "glColor4ui" );
	dllColor4ui( red, green, blue, alpha );
}
static void APIENTRY logColor4uiv(const GLuint *v)
{
	SIG( "glColor4uiv" );
	dllColor4uiv( v );
}
static void APIENTRY logColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
	SIG( "glColor4us" );
	dllColor4us( red, green, blue, alpha );
}
static void APIENTRY logColor4usv(const GLushort *v)
{
	SIG( "glColor4usv" );
	dllColor4usv( v );
}
static void APIENTRY logColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	SIG( "glColorMask" );
	dllColorMask( red, green, blue, alpha );
}
static void APIENTRY logColorMaterial(GLenum face, GLenum mode)
{
	SIG( "glColorMaterial" );
	dllColorMaterial( face, mode );
}

static void APIENTRY logColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	SIG( "glColorPointer" );
	dllColorPointer( size, type, stride, pointer );
}

static void APIENTRY logCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
	SIG( "glCopyPixels" );
	dllCopyPixels( x, y, width, height, type );
}

static void APIENTRY logCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
	SIG( "glCopyTexImage1D" );
	dllCopyTexImage1D( target, level, internalFormat, x, y, width, border );
}

static void APIENTRY logCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	SIG( "glCopyTexImage2D" );
	dllCopyTexImage2D( target, level, internalFormat, x, y, width, height, border );
}

static void APIENTRY logCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	SIG( "glCopyTexSubImage1D" );
	dllCopyTexSubImage1D( target, level, xoffset, x, y, width );
}

static void APIENTRY logCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	SIG( "glCopyTexSubImage2D" );
	dllCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

static void APIENTRY logCullFace(GLenum mode)
{
	SIG( "glCullFace" );
	dllCullFace( mode );
}

static void APIENTRY logDeleteLists(GLuint list, GLsizei range)
{
	SIG( "glDeleteLists" );
	dllDeleteLists( list, range );
}

static void APIENTRY logDeleteTextures(GLsizei n, const GLuint *textures)
{
	SIG( "glDeleteTextures" );
	dllDeleteTextures( n, textures );
}

static void APIENTRY logDepthFunc(GLenum func)
{
	SIG( "glDepthFunc" );
	dllDepthFunc( func );
}

static void APIENTRY logDepthMask(GLboolean flag)
{
	SIG( "glDepthMask" );
	dllDepthMask( flag );
}

static void APIENTRY logDepthRange(GLclampd zNear, GLclampd zFar)
{
	SIG( "glDepthRange" );
	dllDepthRange( zNear, zFar );
}

static void APIENTRY logDisable(GLenum cap)
{
	fprintf( glw_state.log_fp, "glDisable( 0x%x )\n", cap );
	dllDisable( cap );
}

static void APIENTRY logDisableClientState(GLenum array)
{
	SIG( "glDisableClientState" );
	dllDisableClientState( array );
}

static void APIENTRY logDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	SIG( "glDrawArrays" );
	dllDrawArrays( mode, first, count );
}

static void APIENTRY logDrawBuffer(GLenum mode)
{
	SIG( "glDrawBuffer" );
	dllDrawBuffer( mode );
}

static void APIENTRY logDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	SIG( "glDrawElements" );
	dllDrawElements( mode, count, type, indices );
}

static void APIENTRY logDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
	SIG( "glDrawPixels" );
	dllDrawPixels( width, height, format, type, pixels );
}

static void APIENTRY logEdgeFlag(GLboolean flag)
{
	SIG( "glEdgeFlag" );
	dllEdgeFlag( flag );
}

static void APIENTRY logEdgeFlagPointer(GLsizei stride, const void *pointer)
{
	SIG( "glEdgeFlagPointer" );
	dllEdgeFlagPointer( stride, pointer );
}

static void APIENTRY logEdgeFlagv(const GLboolean *flag)
{
	SIG( "glEdgeFlagv" );
	dllEdgeFlagv( flag );
}

static void APIENTRY logEnable(GLenum cap)
{
	fprintf( glw_state.log_fp, "glEnable( 0x%x )\n", cap );
	dllEnable( cap );
}

static void APIENTRY logEnableClientState(GLenum array)
{
	SIG( "glEnableClientState" );
	dllEnableClientState( array );
}

static void APIENTRY logEnd(void)
{
	SIG( "glEnd" );
	dllEnd();
}

static void APIENTRY logEndList(void)
{
	SIG( "glEndList" );
	dllEndList();
}

static void APIENTRY logEvalCoord1d(GLdouble u)
{
	SIG( "glEvalCoord1d" );
	dllEvalCoord1d( u );
}

static void APIENTRY logEvalCoord1dv(const GLdouble *u)
{
	SIG( "glEvalCoord1dv" );
	dllEvalCoord1dv( u );
}

static void APIENTRY logEvalCoord1f(GLfloat u)
{
	SIG( "glEvalCoord1f" );
	dllEvalCoord1f( u );
}

static void APIENTRY logEvalCoord1fv(const GLfloat *u)
{
	SIG( "glEvalCoord1fv" );
	dllEvalCoord1fv( u );
}
static void APIENTRY logEvalCoord2d(GLdouble u, GLdouble v)
{
	SIG( "glEvalCoord2d" );
	dllEvalCoord2d( u, v );
}
static void APIENTRY logEvalCoord2dv(const GLdouble *u)
{
	SIG( "glEvalCoord2dv" );
	dllEvalCoord2dv( u );
}
static void APIENTRY logEvalCoord2f(GLfloat u, GLfloat v)
{
	SIG( "glEvalCoord2f" );
	dllEvalCoord2f( u, v );
}
static void APIENTRY logEvalCoord2fv(const GLfloat *u)
{
	SIG( "glEvalCoord2fv" );
	dllEvalCoord2fv( u );
}

static void APIENTRY logEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
	SIG( "glEvalMesh1" );
	dllEvalMesh1( mode, i1, i2 );
}
static void APIENTRY logEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
	SIG( "glEvalMesh2" );
	dllEvalMesh2( mode, i1, i2, j1, j2 );
}
static void APIENTRY logEvalPoint1(GLint i)
{
	SIG( "glEvalPoint1" );
	dllEvalPoint1( i );
}
static void APIENTRY logEvalPoint2(GLint i, GLint j)
{
	SIG( "glEvalPoint2" );
	dllEvalPoint2( i, j );
}

static void APIENTRY logFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
	SIG( "glFeedbackBuffer" );
	dllFeedbackBuffer( size, type, buffer );
}

static void APIENTRY logFinish(void)
{
	SIG( "glFinish" );
	dllFinish();
}

static void APIENTRY logFlush(void)
{
	SIG( "glFlush" );
	dllFlush();
}

static void APIENTRY logFogf(GLenum pname, GLfloat param)
{
	SIG( "glFogf" );
	dllFogf( pname, param );
}

static void APIENTRY logFogfv(GLenum pname, const GLfloat *params)
{
	SIG( "glFogfv" );
	dllFogfv( pname, params );
}

static void APIENTRY logFogi(GLenum pname, GLint param)
{
	SIG( "glFogi" );
	dllFogi( pname, param );
}

static void APIENTRY logFogiv(GLenum pname, const GLint *params)
{
	SIG( "glFogiv" );
	dllFogiv( pname, params );
}

static void APIENTRY logFrontFace(GLenum mode)
{
	SIG( "glFrontFace" );
	dllFrontFace( mode );
}

static void APIENTRY logFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	SIG( "glFrustum" );
	dllFrustum( left, right, bottom, top, zNear, zFar );
}

static GLuint APIENTRY logGenLists(GLsizei range)
{
	SIG( "glGenLists" );
	return dllGenLists( range );
}

static void APIENTRY logGenTextures(GLsizei n, GLuint *textures)
{
	SIG( "glGenTextures" );
	dllGenTextures( n, textures );
}

static void APIENTRY logGetBooleanv(GLenum pname, GLboolean *params)
{
	SIG( "glGetBooleanv" );
	dllGetBooleanv( pname, params );
}

static void APIENTRY logGetClipPlane(GLenum plane, GLdouble *equation)
{
	SIG( "glGetClipPlane" );
	dllGetClipPlane( plane, equation );
}

static void APIENTRY logGetDoublev(GLenum pname, GLdouble *params)
{
	SIG( "glGetDoublev" );
	dllGetDoublev( pname, params );
}

static GLenum APIENTRY logGetError(void)
{
	SIG( "glGetError" );
	return dllGetError();
}

static void APIENTRY logGetFloatv(GLenum pname, GLfloat *params)
{
	SIG( "glGetFloatv" );
	dllGetFloatv( pname, params );
}

static void APIENTRY logGetIntegerv(GLenum pname, GLint *params)
{
	SIG( "glGetIntegerv" );
	dllGetIntegerv( pname, params );
}

static void APIENTRY logGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
	SIG( "glGetLightfv" );
	dllGetLightfv( light, pname, params );
}

static void APIENTRY logGetLightiv(GLenum light, GLenum pname, GLint *params)
{
	SIG( "glGetLightiv" );
	dllGetLightiv( light, pname, params );
}

static void APIENTRY logGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
	SIG( "glGetMapdv" );
	dllGetMapdv( target, query, v );
}

static void APIENTRY logGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
	SIG( "glGetMapfv" );
	dllGetMapfv( target, query, v );
}

static void APIENTRY logGetMapiv(GLenum target, GLenum query, GLint *v)
{
	SIG( "glGetMapiv" );
	dllGetMapiv( target, query, v );
}

static void APIENTRY logGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
	SIG( "glGetMaterialfv" );
	dllGetMaterialfv( face, pname, params );
}

static void APIENTRY logGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
	SIG( "glGetMaterialiv" );
	dllGetMaterialiv( face, pname, params );
}

static void APIENTRY logGetPixelMapfv(GLenum map, GLfloat *values)
{
	SIG( "glGetPixelMapfv" );
	dllGetPixelMapfv( map, values );
}

static void APIENTRY logGetPixelMapuiv(GLenum map, GLuint *values)
{
	SIG( "glGetPixelMapuiv" );
	dllGetPixelMapuiv( map, values );
}

static void APIENTRY logGetPixelMapusv(GLenum map, GLushort *values)
{
	SIG( "glGetPixelMapusv" );
	dllGetPixelMapusv( map, values );
}

static void APIENTRY logGetPointerv(GLenum pname, GLvoid* *params)
{
	SIG( "glGetPointerv" );
	dllGetPointerv( pname, params );
}

static void APIENTRY logGetPolygonStipple(GLubyte *mask)
{
	SIG( "glGetPolygonStipple" );
	dllGetPolygonStipple( mask );
}

static const GLubyte * APIENTRY logGetString(GLenum name)
{
	SIG( "glGetString" );
	return dllGetString( name );
}

static void APIENTRY logGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
	SIG( "glGetTexEnvfv" );
	dllGetTexEnvfv( target, pname, params );
}

static void APIENTRY logGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
	SIG( "glGetTexEnviv" );
	dllGetTexEnviv( target, pname, params );
}

static void APIENTRY logGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
	SIG( "glGetTexGendv" );
	dllGetTexGendv( coord, pname, params );
}

static void APIENTRY logGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
	SIG( "glGetTexGenfv" );
	dllGetTexGenfv( coord, pname, params );
}

static void APIENTRY logGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
	SIG( "glGetTexGeniv" );
	dllGetTexGeniv( coord, pname, params );
}

static void APIENTRY logGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels)
{
	SIG( "glGetTexImage" );
	dllGetTexImage( target, level, format, type, pixels );
}
static void APIENTRY logGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params )
{
	SIG( "glGetTexLevelParameterfv" );
	dllGetTexLevelParameterfv( target, level, pname, params );
}

static void APIENTRY logGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
	SIG( "glGetTexLevelParameteriv" );
	dllGetTexLevelParameteriv( target, level, pname, params );
}

static void APIENTRY logGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	SIG( "glGetTexParameterfv" );
	dllGetTexParameterfv( target, pname, params );
}

static void APIENTRY logGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
	SIG( "glGetTexParameteriv" );
	dllGetTexParameteriv( target, pname, params );
}

static void APIENTRY logHint(GLenum target, GLenum mode)
{
	fprintf( glw_state.log_fp, "glHint( 0x%x, 0x%x )\n", target, mode );
	dllHint( target, mode );
}

static void APIENTRY logIndexMask(GLuint mask)
{
	SIG( "glIndexMask" );
	dllIndexMask( mask );
}

static void APIENTRY logIndexPointer(GLenum type, GLsizei stride, const void *pointer)
{
	SIG( "glIndexPointer" );
	dllIndexPointer( type, stride, pointer );
}

static void APIENTRY logIndexd(GLdouble c)
{
	SIG( "glIndexd" );
	dllIndexd( c );
}

static void APIENTRY logIndexdv(const GLdouble *c)
{
	SIG( "glIndexdv" );
	dllIndexdv( c );
}

static void APIENTRY logIndexf(GLfloat c)
{
	SIG( "glIndexf" );
	dllIndexf( c );
}

static void APIENTRY logIndexfv(const GLfloat *c)
{
	SIG( "glIndexfv" );
	dllIndexfv( c );
}

static void APIENTRY logIndexi(GLint c)
{
	SIG( "glIndexi" );
	dllIndexi( c );
}

static void APIENTRY logIndexiv(const GLint *c)
{
	SIG( "glIndexiv" );
	dllIndexiv( c );
}

static void APIENTRY logIndexs(GLshort c)
{
	SIG( "glIndexs" );
	dllIndexs( c );
}

static void APIENTRY logIndexsv(const GLshort *c)
{
	SIG( "glIndexsv" );
	dllIndexsv( c );
}

static void APIENTRY logIndexub(GLubyte c)
{
	SIG( "glIndexub" );
	dllIndexub( c );
}

static void APIENTRY logIndexubv(const GLubyte *c)
{
	SIG( "glIndexubv" );
	dllIndexubv( c );
}

static void APIENTRY logInitNames(void)
{
	SIG( "glInitNames" );
	dllInitNames();
}

static void APIENTRY logInterleavedArrays(GLenum format, GLsizei stride, const void *pointer)
{
	SIG( "glInterleavedArrays" );
	dllInterleavedArrays( format, stride, pointer );
}

static GLboolean APIENTRY logIsEnabled(GLenum cap)
{
	SIG( "glIsEnabled" );
	return dllIsEnabled( cap );
}
static GLboolean APIENTRY logIsList(GLuint list)
{
	SIG( "glIsList" );
	return dllIsList( list );
}
static GLboolean APIENTRY logIsTexture(GLuint texture)
{
	SIG( "glIsTexture" );
	return dllIsTexture( texture );
}

static void APIENTRY logLightModelf(GLenum pname, GLfloat param)
{
	SIG( "glLightModelf" );
	dllLightModelf( pname, param );
}

static void APIENTRY logLightModelfv(GLenum pname, const GLfloat *params)
{
	SIG( "glLightModelfv" );
	dllLightModelfv( pname, params );
}

static void APIENTRY logLightModeli(GLenum pname, GLint param)
{
	SIG( "glLightModeli" );
	dllLightModeli( pname, param );

}

static void APIENTRY logLightModeliv(GLenum pname, const GLint *params)
{
	SIG( "glLightModeliv" );
	dllLightModeliv( pname, params );
}

static void APIENTRY logLightf(GLenum light, GLenum pname, GLfloat param)
{
	SIG( "glLightf" );
	dllLightf( light, pname, param );
}

static void APIENTRY logLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	SIG( "glLightfv" );
	dllLightfv( light, pname, params );
}

static void APIENTRY logLighti(GLenum light, GLenum pname, GLint param)
{
	SIG( "glLighti" );
	dllLighti( light, pname, param );
}

static void APIENTRY logLightiv(GLenum light, GLenum pname, const GLint *params)
{
	SIG( "glLightiv" );
	dllLightiv( light, pname, params );
}

static void APIENTRY logLineStipple(GLint factor, GLushort pattern)
{
	SIG( "glLineStipple" );
	dllLineStipple( factor, pattern );
}

static void APIENTRY logLineWidth(GLfloat width)
{
	SIG( "glLineWidth" );
	dllLineWidth( width );
}

static void APIENTRY logListBase(GLuint base)
{
	SIG( "glListBase" );
	dllListBase( base );
}

static void APIENTRY logLoadIdentity(void)
{
	SIG( "glLoadIdentity" );
	dllLoadIdentity();
}

static void APIENTRY logLoadMatrixd(const GLdouble *m)
{
	SIG( "glLoadMatrixd" );
	dllLoadMatrixd( m );
}

static void APIENTRY logLoadMatrixf(const GLfloat *m)
{
	SIG( "glLoadMatrixf" );
	dllLoadMatrixf( m );
}

static void APIENTRY logLoadName(GLuint name)
{
	SIG( "glLoadName" );
	dllLoadName( name );
}

static void APIENTRY logLogicOp(GLenum opcode)
{
	SIG( "glLogicOp" );
	dllLogicOp( opcode );
}

static void APIENTRY logMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
	SIG( "glMap1d" );
	dllMap1d( target, u1, u2, stride, order, points );
}

static void APIENTRY logMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
	SIG( "glMap1f" );
	dllMap1f( target, u1, u2, stride, order, points );
}

static void APIENTRY logMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
	SIG( "glMap2d" );
	dllMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void APIENTRY logMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
	SIG( "glMap2f" );
	dllMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void APIENTRY logMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
	SIG( "glMapGrid1d" );
	dllMapGrid1d( un, u1, u2 );
}

static void APIENTRY logMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
	SIG( "glMapGrid1f" );
	dllMapGrid1f( un, u1, u2 );
}

static void APIENTRY logMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
	SIG( "glMapGrid2d" );
	dllMapGrid2d( un, u1, u2, vn, v1, v2 );
}
static void APIENTRY logMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
	SIG( "glMapGrid2f" );
	dllMapGrid2f( un, u1, u2, vn, v1, v2 );
}
static void APIENTRY logMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	SIG( "glMaterialf" );
	dllMaterialf( face, pname, param );
}
static void APIENTRY logMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	SIG( "glMaterialfv" );
	dllMaterialfv( face, pname, params );
}

static void APIENTRY logMateriali(GLenum face, GLenum pname, GLint param)
{
	SIG( "glMateriali" );
	dllMateriali( face, pname, param );
}

static void APIENTRY logMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
	SIG( "glMaterialiv" );
	dllMaterialiv( face, pname, params );
}

static void APIENTRY logMatrixMode(GLenum mode)
{
	SIG( "glMatrixMode" );
	dllMatrixMode( mode );
}

static void APIENTRY logMultMatrixd(const GLdouble *m)
{
	SIG( "glMultMatrixd" );
	dllMultMatrixd( m );
}

static void APIENTRY logMultMatrixf(const GLfloat *m)
{
	SIG( "glMultMatrixf" );
	dllMultMatrixf( m );
}

static void APIENTRY logNewList(GLuint list, GLenum mode)
{
	SIG( "glNewList" );
	dllNewList( list, mode );
}

static void APIENTRY logNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
	SIG ("glNormal3b" );
	dllNormal3b( nx, ny, nz );
}

static void APIENTRY logNormal3bv(const GLbyte *v)
{
	SIG( "glNormal3bv" );
	dllNormal3bv( v );
}

static void APIENTRY logNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
	SIG( "glNormal3d" );
	dllNormal3d( nx, ny, nz );
}

static void APIENTRY logNormal3dv(const GLdouble *v)
{
	SIG( "glNormal3dv" );
	dllNormal3dv( v );
}

static void APIENTRY logNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	SIG( "glNormal3f" );
	dllNormal3f( nx, ny, nz );
}

static void APIENTRY logNormal3fv(const GLfloat *v)
{
	SIG( "glNormal3fv" );
	dllNormal3fv( v );
}
static void APIENTRY logNormal3i(GLint nx, GLint ny, GLint nz)
{
	SIG( "glNormal3i" );
	dllNormal3i( nx, ny, nz );
}
static void APIENTRY logNormal3iv(const GLint *v)
{
	SIG( "glNormal3iv" );
	dllNormal3iv( v );
}
static void APIENTRY logNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
	SIG( "glNormal3s" );
	dllNormal3s( nx, ny, nz );
}
static void APIENTRY logNormal3sv(const GLshort *v)
{
	SIG( "glNormal3sv" );
	dllNormal3sv( v );
}
static void APIENTRY logNormalPointer(GLenum type, GLsizei stride, const void *pointer)
{
	SIG( "glNormalPointer" );
	dllNormalPointer( type, stride, pointer );
}
static void APIENTRY logOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	SIG( "glOrtho" );
	dllOrtho( left, right, bottom, top, zNear, zFar );
}

static void APIENTRY logPassThrough(GLfloat token)
{
	SIG( "glPassThrough" );
	dllPassThrough( token );
}

static void APIENTRY logPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values)
{
	SIG( "glPixelMapfv" );
	dllPixelMapfv( map, mapsize, values );
}

static void APIENTRY logPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values)
{
	SIG( "glPixelMapuiv" );
	dllPixelMapuiv( map, mapsize, values );
}

static void APIENTRY logPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values)
{
	SIG( "glPixelMapusv" );
	dllPixelMapusv( map, mapsize, values );
}
static void APIENTRY logPixelStoref(GLenum pname, GLfloat param)
{
	SIG( "glPixelStoref" );
	dllPixelStoref( pname, param );
}
static void APIENTRY logPixelStorei(GLenum pname, GLint param)
{
	SIG( "glPixelStorei" );
	dllPixelStorei( pname, param );
}
static void APIENTRY logPixelTransferf(GLenum pname, GLfloat param)
{
	SIG( "glPixelTransferf" );
	dllPixelTransferf( pname, param );
}

static void APIENTRY logPixelTransferi(GLenum pname, GLint param)
{
	SIG( "glPixelTransferi" );
	dllPixelTransferi( pname, param );
}

static void APIENTRY logPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
	SIG( "glPixelZoom" );
	dllPixelZoom( xfactor, yfactor );
}

static void APIENTRY logPointSize(GLfloat size)
{
	SIG( "glPointSize" );
	dllPointSize( size );
}

static void APIENTRY logPolygonMode(GLenum face, GLenum mode)
{
	fprintf( glw_state.log_fp, "glPolygonMode( 0x%x, 0x%x )\n", face, mode );
	dllPolygonMode( face, mode );
}

static void APIENTRY logPolygonOffset(GLfloat factor, GLfloat units)
{
	SIG( "glPolygonOffset" );
	dllPolygonOffset( factor, units );
}
static void APIENTRY logPolygonStipple(const GLubyte *mask )
{
	SIG( "glPolygonStipple" );
	dllPolygonStipple( mask );
}
static void APIENTRY logPopAttrib(void)
{
	SIG( "glPopAttrib" );
	dllPopAttrib();
}

static void APIENTRY logPopClientAttrib(void)
{
	SIG( "glPopClientAttrib" );
	dllPopClientAttrib();
}

static void APIENTRY logPopMatrix(void)
{
	SIG( "glPopMatrix" );
	dllPopMatrix();
}

static void APIENTRY logPopName(void)
{
	SIG( "glPopName" );
	dllPopName();
}

static void APIENTRY logPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
	SIG( "glPrioritizeTextures" );
	dllPrioritizeTextures( n, textures, priorities );
}

static void APIENTRY logPushAttrib(GLbitfield mask)
{
	SIG( "glPushAttrib" );
	dllPushAttrib( mask );
}

static void APIENTRY logPushClientAttrib(GLbitfield mask)
{
	SIG( "glPushClientAttrib" );
	dllPushClientAttrib( mask );
}

static void APIENTRY logPushMatrix(void)
{
	SIG( "glPushMatrix" );
	dllPushMatrix();
}

static void APIENTRY logPushName(GLuint name)
{
	SIG( "glPushName" );
	dllPushName( name );
}

static void APIENTRY logRasterPos2d(GLdouble x, GLdouble y)
{
	SIG ("glRasterPot2d" );
	dllRasterPos2d( x, y );
}

static void APIENTRY logRasterPos2dv(const GLdouble *v)
{
	SIG( "glRasterPos2dv" );
	dllRasterPos2dv( v );
}

static void APIENTRY logRasterPos2f(GLfloat x, GLfloat y)
{
	SIG( "glRasterPos2f" );
	dllRasterPos2f( x, y );
}
static void APIENTRY logRasterPos2fv(const GLfloat *v)
{
	SIG( "glRasterPos2dv" );
	dllRasterPos2fv( v );
}
static void APIENTRY logRasterPos2i(GLint x, GLint y)
{
	SIG( "glRasterPos2if" );
	dllRasterPos2i( x, y );
}
static void APIENTRY logRasterPos2iv(const GLint *v)
{
	SIG( "glRasterPos2iv" );
	dllRasterPos2iv( v );
}
static void APIENTRY logRasterPos2s(GLshort x, GLshort y)
{
	SIG( "glRasterPos2s" );
	dllRasterPos2s( x, y );
}
static void APIENTRY logRasterPos2sv(const GLshort *v)
{
	SIG( "glRasterPos2sv" );
	dllRasterPos2sv( v );
}
static void APIENTRY logRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
	SIG( "glRasterPos3d" );
	dllRasterPos3d( x, y, z );
}
static void APIENTRY logRasterPos3dv(const GLdouble *v)
{
	SIG( "glRasterPos3dv" );
	dllRasterPos3dv( v );
}
static void APIENTRY logRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
	SIG( "glRasterPos3f" );
	dllRasterPos3f( x, y, z );
}
static void APIENTRY logRasterPos3fv(const GLfloat *v)
{
	SIG( "glRasterPos3fv" );
	dllRasterPos3fv( v );
}
static void APIENTRY logRasterPos3i(GLint x, GLint y, GLint z)
{
	SIG( "glRasterPos3i" );
	dllRasterPos3i( x, y, z );
}
static void APIENTRY logRasterPos3iv(const GLint *v)
{
	SIG( "glRasterPos3iv" );
	dllRasterPos3iv( v );
}
static void APIENTRY logRasterPos3s(GLshort x, GLshort y, GLshort z)
{
	SIG( "glRasterPos3s" );
	dllRasterPos3s( x, y, z );
}
static void APIENTRY logRasterPos3sv(const GLshort *v)
{
	SIG( "glRasterPos3sv" );
	dllRasterPos3sv( v );
}
static void APIENTRY logRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	SIG( "glRasterPos4d" );
	dllRasterPos4d( x, y, z, w );
}
static void APIENTRY logRasterPos4dv(const GLdouble *v)
{
	SIG( "glRasterPos4dv" );
	dllRasterPos4dv( v );
}
static void APIENTRY logRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	SIG( "glRasterPos4f" );
	dllRasterPos4f( x, y, z, w );
}
static void APIENTRY logRasterPos4fv(const GLfloat *v)
{
	SIG( "glRasterPos4fv" );
	dllRasterPos4fv( v );
}
static void APIENTRY logRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
	SIG( "glRasterPos4i" );
	dllRasterPos4i( x, y, z, w );
}
static void APIENTRY logRasterPos4iv(const GLint *v)
{
	SIG( "glRasterPos4iv" );
	dllRasterPos4iv( v );
}
static void APIENTRY logRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	SIG( "glRasterPos4s" );
	dllRasterPos4s( x, y, z, w );
}
static void APIENTRY logRasterPos4sv(const GLshort *v)
{
	SIG( "glRasterPos4sv" );
	dllRasterPos4sv( v );
}
static void APIENTRY logReadBuffer(GLenum mode)
{
	SIG( "glReadBuffer" );
	dllReadBuffer( mode );
}
static void APIENTRY logReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)
{
	SIG( "glReadPixels" );
	dllReadPixels( x, y, width, height, format, type, pixels );
}

static void APIENTRY logRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
	SIG( "glRectd" );
	dllRectd( x1, y1, x2, y2 );
}

static void APIENTRY logRectdv(const GLdouble *v1, const GLdouble *v2)
{
	SIG( "glRectdv" );
	dllRectdv( v1, v2 );
}

static void APIENTRY logRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	SIG( "glRectf" );
	dllRectf( x1, y1, x2, y2 );
}

static void APIENTRY logRectfv(const GLfloat *v1, const GLfloat *v2)
{
	SIG( "glRectfv" );
	dllRectfv( v1, v2 );
}
static void APIENTRY logRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
	SIG( "glRecti" );
	dllRecti( x1, y1, x2, y2 );
}
static void APIENTRY logRectiv(const GLint *v1, const GLint *v2)
{
	SIG( "glRectiv" );
	dllRectiv( v1, v2 );
}
static void APIENTRY logRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
	SIG( "glRects" );
	dllRects( x1, y1, x2, y2 );
}
static void APIENTRY logRectsv(const GLshort *v1, const GLshort *v2)
{
	SIG( "glRectsv" );
	dllRectsv( v1, v2 );
}
static GLint APIENTRY logRenderMode(GLenum mode)
{
	SIG( "glRenderMode" );
	return dllRenderMode( mode );
}
static void APIENTRY logRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	SIG( "glRotated" );
	dllRotated( angle, x, y, z );
}

static void APIENTRY logRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	SIG( "glRotatef" );
	dllRotatef( angle, x, y, z );
}

static void APIENTRY logScaled(GLdouble x, GLdouble y, GLdouble z)
{
	SIG( "glScaled" );
	dllScaled( x, y, z );
}

static void APIENTRY logScalef(GLfloat x, GLfloat y, GLfloat z)
{
	SIG( "glScalef" );
	dllScalef( x, y, z );
}

static void APIENTRY logScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	SIG( "glScissor" );
	dllScissor( x, y, width, height );
}

static void APIENTRY logSelectBuffer(GLsizei size, GLuint *buffer)
{
	SIG( "glSelectBuffer" );
	dllSelectBuffer( size, buffer );
}

static void APIENTRY logShadeModel(GLenum mode)
{
	SIG( "glShadeModel" );
	dllShadeModel( mode );
}

static void APIENTRY logStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	SIG( "glStencilFunc" );
	dllStencilFunc( func, ref, mask );
}

static void APIENTRY logStencilMask(GLuint mask)
{
	SIG( "glStencilMask" );
	dllStencilMask( mask );
}

static void APIENTRY logStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	SIG( "glStencilOp" );
	dllStencilOp( fail, zfail, zpass );
}

static void APIENTRY logTexCoord1d(GLdouble s)
{
	SIG( "glTexCoord1d" );
	dllTexCoord1d( s );
}

static void APIENTRY logTexCoord1dv(const GLdouble *v)
{
	SIG( "glTexCoord1dv" );
	dllTexCoord1dv( v );
}

static void APIENTRY logTexCoord1f(GLfloat s)
{
	SIG( "glTexCoord1f" );
	dllTexCoord1f( s );
}
static void APIENTRY logTexCoord1fv(const GLfloat *v)
{
	SIG( "glTexCoord1fv" );
	dllTexCoord1fv( v );
}
static void APIENTRY logTexCoord1i(GLint s)
{
	SIG( "glTexCoord1i" );
	dllTexCoord1i( s );
}
static void APIENTRY logTexCoord1iv(const GLint *v)
{
	SIG( "glTexCoord1iv" );
	dllTexCoord1iv( v );
}
static void APIENTRY logTexCoord1s(GLshort s)
{
	SIG( "glTexCoord1s" );
	dllTexCoord1s( s );
}
static void APIENTRY logTexCoord1sv(const GLshort *v)
{
	SIG( "glTexCoord1sv" );
	dllTexCoord1sv( v );
}
static void APIENTRY logTexCoord2d(GLdouble s, GLdouble t)
{
	SIG( "glTexCoord2d" );
	dllTexCoord2d( s, t );
}

static void APIENTRY logTexCoord2dv(const GLdouble *v)
{
	SIG( "glTexCoord2dv" );
	dllTexCoord2dv( v );
}
static void APIENTRY logTexCoord2f(GLfloat s, GLfloat t)
{
	SIG( "glTexCoord2f" );
	dllTexCoord2f( s, t );
}
static void APIENTRY logTexCoord2fv(const GLfloat *v)
{
	SIG( "glTexCoord2fv" );
	dllTexCoord2fv( v );
}
static void APIENTRY logTexCoord2i(GLint s, GLint t)
{
	SIG( "glTexCoord2i" );
	dllTexCoord2i( s, t );
}
static void APIENTRY logTexCoord2iv(const GLint *v)
{
	SIG( "glTexCoord2iv" );
	dllTexCoord2iv( v );
}
static void APIENTRY logTexCoord2s(GLshort s, GLshort t)
{
	SIG( "glTexCoord2s" );
	dllTexCoord2s( s, t );
}
static void APIENTRY logTexCoord2sv(const GLshort *v)
{
	SIG( "glTexCoord2sv" );
	dllTexCoord2sv( v );
}
static void APIENTRY logTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
	SIG( "glTexCoord3d" );
	dllTexCoord3d( s, t, r );
}
static void APIENTRY logTexCoord3dv(const GLdouble *v)
{
	SIG( "glTexCoord3dv" );
	dllTexCoord3dv( v );
}
static void APIENTRY logTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
	SIG( "glTexCoord3f" );
	dllTexCoord3f( s, t, r );
}
static void APIENTRY logTexCoord3fv(const GLfloat *v)
{
	SIG( "glTexCoord3fv" );
	dllTexCoord3fv( v );
}
static void APIENTRY logTexCoord3i(GLint s, GLint t, GLint r)
{
	SIG( "glTexCoord3i" );
	dllTexCoord3i( s, t, r );
}
static void APIENTRY logTexCoord3iv(const GLint *v)
{
	SIG( "glTexCoord3iv" );
	dllTexCoord3iv( v );
}
static void APIENTRY logTexCoord3s(GLshort s, GLshort t, GLshort r)
{
	SIG( "glTexCoord3s" );
	dllTexCoord3s( s, t, r );
}
static void APIENTRY logTexCoord3sv(const GLshort *v)
{
	SIG( "glTexCoord3sv" );
	dllTexCoord3sv( v );
}
static void APIENTRY logTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
	SIG( "glTexCoord4d" );
	dllTexCoord4d( s, t, r, q );
}
static void APIENTRY logTexCoord4dv(const GLdouble *v)
{
	SIG( "glTexCoord4dv" );
	dllTexCoord4dv( v );
}
static void APIENTRY logTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	SIG( "glTexCoord4f" );
	dllTexCoord4f( s, t, r, q );
}
static void APIENTRY logTexCoord4fv(const GLfloat *v)
{
	SIG( "glTexCoord4fv" );
	dllTexCoord4fv( v );
}
static void APIENTRY logTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
	SIG( "glTexCoord4i" );
	dllTexCoord4i( s, t, r, q );
}
static void APIENTRY logTexCoord4iv(const GLint *v)
{
	SIG( "glTexCoord4iv" );
	dllTexCoord4iv( v );
}
static void APIENTRY logTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
	SIG( "glTexCoord4s" );
	dllTexCoord4s( s, t, r, q );
}
static void APIENTRY logTexCoord4sv(const GLshort *v)
{
	SIG( "glTexCoord4sv" );
	dllTexCoord4sv( v );
}
static void APIENTRY logTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	SIG( "glTexCoordPointer" );
	dllTexCoordPointer( size, type, stride, pointer );
}

static void APIENTRY logTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	fprintf( glw_state.log_fp, "glTexEnvf( 0x%x, 0x%x, %f )\n", target, pname, param );
	dllTexEnvf( target, pname, param );
}

static void APIENTRY logTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	SIG( "glTexEnvfv" );
	dllTexEnvfv( target, pname, params );
}

static void APIENTRY logTexEnvi(GLenum target, GLenum pname, GLint param)
{
	fprintf( glw_state.log_fp, "glTexEnvi( 0x%x, 0x%x, 0x%x )\n", target, pname, param );
	dllTexEnvi( target, pname, param );
}
static void APIENTRY logTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
	SIG( "glTexEnviv" );
	dllTexEnviv( target, pname, params );
}

static void APIENTRY logTexGend(GLenum coord, GLenum pname, GLdouble param)
{
	SIG( "glTexGend" );
	dllTexGend( coord, pname, param );
}

static void APIENTRY logTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
	SIG( "glTexGendv" );
	dllTexGendv( coord, pname, params );
}

static void APIENTRY logTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
	SIG( "glTexGenf" );
	dllTexGenf( coord, pname, param );
}
static void APIENTRY logTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
	SIG( "glTexGenfv" );
	dllTexGenfv( coord, pname, params );
}
static void APIENTRY logTexGeni(GLenum coord, GLenum pname, GLint param)
{
	SIG( "glTexGeni" );
	dllTexGeni( coord, pname, param );
}
static void APIENTRY logTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
	SIG( "glTexGeniv" );
	dllTexGeniv( coord, pname, params );
}
static void APIENTRY logTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels)
{
	SIG( "glTexImage1D" );
	dllTexImage1D( target, level, internalformat, width, border, format, type, pixels );
}
static void APIENTRY logTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
	SIG( "glTexImage2D" );
	dllTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

static void APIENTRY logTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	fprintf( glw_state.log_fp, "glTexParameterf( 0x%x, 0x%x, %f )\n", target, pname, param );
	dllTexParameterf( target, pname, param );
}

static void APIENTRY logTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	SIG( "glTexParameterfv" );
	dllTexParameterfv( target, pname, params );
}
static void APIENTRY logTexParameteri(GLenum target, GLenum pname, GLint param)
{
	fprintf( glw_state.log_fp, "glTexParameteri( 0x%x, 0x%x, 0x%x )\n", target, pname, param );
	dllTexParameteri( target, pname, param );
}
static void APIENTRY logTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
	SIG( "glTexParameteriv" );
	dllTexParameteriv( target, pname, params );
}
static void APIENTRY logTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels)
{
	SIG( "glTexSubImage1D" );
	dllTexSubImage1D( target, level, xoffset, width, format, type, pixels );
}
static void APIENTRY logTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
	SIG( "glTexSubImage2D" );
	dllTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}
static void APIENTRY logTranslated(GLdouble x, GLdouble y, GLdouble z)
{
	SIG( "glTranslated" );
	dllTranslated( x, y, z );
}

static void APIENTRY logTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	SIG( "glTranslatef" );
	dllTranslatef( x, y, z );
}

static void APIENTRY logVertex2d(GLdouble x, GLdouble y)
{
	SIG( "glVertex2d" );
	dllVertex2d( x, y );
}

static void APIENTRY logVertex2dv(const GLdouble *v)
{
	SIG( "glVertex2dv" );
	dllVertex2dv( v );
}
static void APIENTRY logVertex2f(GLfloat x, GLfloat y)
{
	SIG( "glVertex2f" );
	dllVertex2f( x, y );
}
static void APIENTRY logVertex2fv(const GLfloat *v)
{
	SIG( "glVertex2fv" );
	dllVertex2fv( v );
}
static void APIENTRY logVertex2i(GLint x, GLint y)
{
	SIG( "glVertex2i" );
	dllVertex2i( x, y );
}
static void APIENTRY logVertex2iv(const GLint *v)
{
	SIG( "glVertex2iv" );
	dllVertex2iv( v );
}
static void APIENTRY logVertex2s(GLshort x, GLshort y)
{
	SIG( "glVertex2s" );
	dllVertex2s( x, y );
}
static void APIENTRY logVertex2sv(const GLshort *v)
{
	SIG( "glVertex2sv" );
	dllVertex2sv( v );
}
static void APIENTRY logVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
	SIG( "glVertex3d" );
	dllVertex3d( x, y, z );
}
static void APIENTRY logVertex3dv(const GLdouble *v)
{
	SIG( "glVertex3dv" );
	dllVertex3dv( v );
}
static void APIENTRY logVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	SIG( "glVertex3f" );
	dllVertex3f( x, y, z );
}
static void APIENTRY logVertex3fv(const GLfloat *v)
{
	SIG( "glVertex3fv" );
	dllVertex3fv( v );
}
static void APIENTRY logVertex3i(GLint x, GLint y, GLint z)
{
	SIG( "glVertex3i" );
	dllVertex3i( x, y, z );
}
static void APIENTRY logVertex3iv(const GLint *v)
{
	SIG( "glVertex3iv" );
	dllVertex3iv( v );
}
static void APIENTRY logVertex3s(GLshort x, GLshort y, GLshort z)
{
	SIG( "glVertex3s" );
	dllVertex3s( x, y, z );
}
static void APIENTRY logVertex3sv(const GLshort *v)
{
	SIG( "glVertex3sv" );
	dllVertex3sv( v );
}
static void APIENTRY logVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	SIG( "glVertex4d" );
	dllVertex4d( x, y, z, w );
}
static void APIENTRY logVertex4dv(const GLdouble *v)
{
	SIG( "glVertex4dv" );
	dllVertex4dv( v );
}
static void APIENTRY logVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	SIG( "glVertex4f" );
	dllVertex4f( x, y, z, w );
}
static void APIENTRY logVertex4fv(const GLfloat *v)
{
	SIG( "glVertex4fv" );
	dllVertex4fv( v );
}
static void APIENTRY logVertex4i(GLint x, GLint y, GLint z, GLint w)
{
	SIG( "glVertex4i" );
	dllVertex4i( x, y, z, w );
}
static void APIENTRY logVertex4iv(const GLint *v)
{
	SIG( "glVertex4iv" );
	dllVertex4iv( v );
}
static void APIENTRY logVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	SIG( "glVertex4s" );
	dllVertex4s( x, y, z, w );
}
static void APIENTRY logVertex4sv(const GLshort *v)
{
	SIG( "glVertex4sv" );
	dllVertex4sv( v );
}
static void APIENTRY logVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	SIG( "glVertexPointer" );
	dllVertexPointer( size, type, stride, pointer );
}
static void APIENTRY logViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	SIG( "glViewport" );
	dllViewport( x, y, width, height );
}

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

// === jitwater - FBO extensions
PFNGLISRENDERBUFFEREXTPROC                               qglIsRenderbufferEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC                             qglBindRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC                          qglDeleteRenderbuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC                             qglGenRenderbuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC                          qglRenderbufferStorageEXT = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC                   qglGetRenderbufferParameterivEXT = NULL;
PFNGLISFRAMEBUFFEREXTPROC                                qglIsFramebufferEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC                              qglBindFramebufferEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC                           qglDeleteFramebuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC                              qglGenFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC                       qglCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC                         qglFramebufferTexture1DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC                         qglFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC                         qglFramebufferTexture3DEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC                      qglFramebufferRenderbufferEXT = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC          qglGetFramebufferAttachmentParameterivEXT = NULL;
PFNGLGENERATEMIPMAPEXTPROC                               qglGenerateMipmapEXT = NULL;
// jitwater ===
//****************************************************************************

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
#ifdef WIN32
	if (glw_state.hinstOpenGL)
	{
		FreeLibrary(glw_state.hinstOpenGL);
		glw_state.hinstOpenGL = NULL;
	}

	glw_state.hinstOpenGL = NULL;
#else
	if (glw_state.OpenGLLib)
	{
		dlclose(glw_state.OpenGLLib);
		glw_state.OpenGLLib = NULL;
	}

	glw_state.OpenGLLib = NULL;
#endif

	qglAccum                     = NULL;
	qglAlphaFunc                 = NULL;
	qglAreTexturesResident       = NULL;
	qglArrayElement              = NULL;
	qglBegin                     = NULL;
	qglBindTexture               = NULL;
	qglBitmap                    = NULL;
	qglBlendFunc                 = NULL;
	qglCallList                  = NULL;
	qglCallLists                 = NULL;
	qglClear                     = NULL;
	qglClearAccum                = NULL;
	qglClearColor                = NULL;
	qglClearDepth                = NULL;
	qglClearIndex                = NULL;
	qglClearStencil              = NULL;
	qglClipPlane                 = NULL;
	qglColor3b                   = NULL;
	qglColor3bv                  = NULL;
	qglColor3d                   = NULL;
	qglColor3dv                  = NULL;
	qglColor3f                   = NULL;
	qglColor3fv                  = NULL;
	qglColor3i                   = NULL;
	qglColor3iv                  = NULL;
	qglColor3s                   = NULL;
	qglColor3sv                  = NULL;
	qglColor3ub                  = NULL;
	qglColor3ubv                 = NULL;
	qglColor3ui                  = NULL;
	qglColor3uiv                 = NULL;
	qglColor3us                  = NULL;
	qglColor3usv                 = NULL;
	qglColor4b                   = NULL;
	qglColor4bv                  = NULL;
	qglColor4d                   = NULL;
	qglColor4dv                  = NULL;
	qglColor4f                   = NULL;
	qglColor4fv                  = NULL;
	qglColor4i                   = NULL;
	qglColor4iv                  = NULL;
	qglColor4s                   = NULL;
	qglColor4sv                  = NULL;
	qglColor4ub                  = NULL;
	qglColor4ubv                 = NULL;
	qglColor4ui                  = NULL;
	qglColor4uiv                 = NULL;
	qglColor4us                  = NULL;
	qglColor4usv                 = NULL;
	qglColorMask                 = NULL;
	qglColorMaterial             = NULL;
	qglColorPointer              = NULL;
	qglCopyPixels                = NULL;
	qglCopyTexImage1D            = NULL;
	qglCopyTexImage2D            = NULL;
	qglCopyTexSubImage1D         = NULL;
	qglCopyTexSubImage2D         = NULL;
	qglCullFace                  = NULL;
	qglDeleteLists               = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDepthRange                = NULL;
	qglDisable                   = NULL;
	qglDisableClientState        = NULL;
	qglDrawArrays                = NULL;
	qglDrawBuffer                = NULL;
	qglDrawElements              = NULL;
	qglDrawPixels                = NULL;
	qglEdgeFlag                  = NULL;
	qglEdgeFlagPointer           = NULL;
	qglEdgeFlagv                 = NULL;
	qglEnable                    = NULL;
	qglEnableClientState         = NULL;
	qglEnd                       = NULL;
	qglEndList                   = NULL;
	qglEvalCoord1d               = NULL;
	qglEvalCoord1dv              = NULL;
	qglEvalCoord1f               = NULL;
	qglEvalCoord1fv              = NULL;
	qglEvalCoord2d               = NULL;
	qglEvalCoord2dv              = NULL;
	qglEvalCoord2f               = NULL;
	qglEvalCoord2fv              = NULL;
	qglEvalMesh1                 = NULL;
	qglEvalMesh2                 = NULL;
	qglEvalPoint1                = NULL;
	qglEvalPoint2                = NULL;
	qglFeedbackBuffer            = NULL;
	qglFinish                    = NULL;
	qglFlush                     = NULL;
	qglFogf                      = NULL;
	qglFogfv                     = NULL;
	qglFogi                      = NULL;
	qglFogiv                     = NULL;
	qglFrontFace                 = NULL;
	qglFrustum                   = NULL;
	qglGenLists                  = NULL;
	qglGenTextures               = NULL;
	qglGetBooleanv               = NULL;
	qglGetClipPlane              = NULL;
	qglGetDoublev                = NULL;
	qglGetError                  = NULL;
	qglGetFloatv                 = NULL;
	qglGetIntegerv               = NULL;
	qglGetLightfv                = NULL;
	qglGetLightiv                = NULL;
	qglGetMapdv                  = NULL;
	qglGetMapfv                  = NULL;
	qglGetMapiv                  = NULL;
	qglGetMaterialfv             = NULL;
	qglGetMaterialiv             = NULL;
	qglGetPixelMapfv             = NULL;
	qglGetPixelMapuiv            = NULL;
	qglGetPixelMapusv            = NULL;
	qglGetPointerv               = NULL;
	qglGetPolygonStipple         = NULL;
	qglGetString                 = NULL;
	qglGetTexEnvfv               = NULL;
	qglGetTexEnviv               = NULL;
	qglGetTexGendv               = NULL;
	qglGetTexGenfv               = NULL;
	qglGetTexGeniv               = NULL;
	qglGetTexImage               = NULL;
	qglGetTexLevelParameterfv    = NULL;
	qglGetTexLevelParameteriv    = NULL;
	qglGetTexParameterfv         = NULL;
	qglGetTexParameteriv         = NULL;
	qglHint                      = NULL;
	qglIndexMask                 = NULL;
	qglIndexPointer              = NULL;
	qglIndexd                    = NULL;
	qglIndexdv                   = NULL;
	qglIndexf                    = NULL;
	qglIndexfv                   = NULL;
	qglIndexi                    = NULL;
	qglIndexiv                   = NULL;
	qglIndexs                    = NULL;
	qglIndexsv                   = NULL;
	qglIndexub                   = NULL;
	qglIndexubv                  = NULL;
	qglInitNames                 = NULL;
	qglInterleavedArrays         = NULL;
	qglIsEnabled                 = NULL;
	qglIsList                    = NULL;
	qglIsTexture                 = NULL;
	qglLightModelf               = NULL;
	qglLightModelfv              = NULL;
	qglLightModeli               = NULL;
	qglLightModeliv              = NULL;
	qglLightf                    = NULL;
	qglLightfv                   = NULL;
	qglLighti                    = NULL;
	qglLightiv                   = NULL;
	qglLineStipple               = NULL;
	qglLineWidth                 = NULL;
	qglListBase                  = NULL;
	qglLoadIdentity              = NULL;
	qglLoadMatrixd               = NULL;
	qglLoadMatrixf               = NULL;
	qglLoadName                  = NULL;
	qglLogicOp                   = NULL;
	qglMap1d                     = NULL;
	qglMap1f                     = NULL;
	qglMap2d                     = NULL;
	qglMap2f                     = NULL;
	qglMapGrid1d                 = NULL;
	qglMapGrid1f                 = NULL;
	qglMapGrid2d                 = NULL;
	qglMapGrid2f                 = NULL;
	qglMaterialf                 = NULL;
	qglMaterialfv                = NULL;
	qglMateriali                 = NULL;
	qglMaterialiv                = NULL;
	qglMatrixMode                = NULL;
	qglMultMatrixd               = NULL;
	qglMultMatrixf               = NULL;
	qglNewList                   = NULL;
	qglNormal3b                  = NULL;
	qglNormal3bv                 = NULL;
	qglNormal3d                  = NULL;
	qglNormal3dv                 = NULL;
	qglNormal3f                  = NULL;
	qglNormal3fv                 = NULL;
	qglNormal3i                  = NULL;
	qglNormal3iv                 = NULL;
	qglNormal3s                  = NULL;
	qglNormal3sv                 = NULL;
	qglNormalPointer             = NULL;
	qglOrtho                     = NULL;
	qglPassThrough               = NULL;
	qglPixelMapfv                = NULL;
	qglPixelMapuiv               = NULL;
	qglPixelMapusv               = NULL;
	qglPixelStoref               = NULL;
	qglPixelStorei               = NULL;
	qglPixelTransferf            = NULL;
	qglPixelTransferi            = NULL;
	qglPixelZoom                 = NULL;
	qglPointSize                 = NULL;
	qglPolygonMode               = NULL;
	qglPolygonOffset             = NULL;
	qglPolygonStipple            = NULL;
	qglPopAttrib                 = NULL;
	qglPopClientAttrib           = NULL;
	qglPopMatrix                 = NULL;
	qglPopName                   = NULL;
	qglPrioritizeTextures        = NULL;
	qglPushAttrib                = NULL;
	qglPushClientAttrib          = NULL;
	qglPushMatrix                = NULL;
	qglPushName                  = NULL;
	qglRasterPos2d               = NULL;
	qglRasterPos2dv              = NULL;
	qglRasterPos2f               = NULL;
	qglRasterPos2fv              = NULL;
	qglRasterPos2i               = NULL;
	qglRasterPos2iv              = NULL;
	qglRasterPos2s               = NULL;
	qglRasterPos2sv              = NULL;
	qglRasterPos3d               = NULL;
	qglRasterPos3dv              = NULL;
	qglRasterPos3f               = NULL;
	qglRasterPos3fv              = NULL;
	qglRasterPos3i               = NULL;
	qglRasterPos3iv              = NULL;
	qglRasterPos3s               = NULL;
	qglRasterPos3sv              = NULL;
	qglRasterPos4d               = NULL;
	qglRasterPos4dv              = NULL;
	qglRasterPos4f               = NULL;
	qglRasterPos4fv              = NULL;
	qglRasterPos4i               = NULL;
	qglRasterPos4iv              = NULL;
	qglRasterPos4s               = NULL;
	qglRasterPos4sv              = NULL;
	qglReadBuffer                = NULL;
	qglReadPixels                = NULL;
	qglRectd                     = NULL;
	qglRectdv                    = NULL;
	qglRectf                     = NULL;
	qglRectfv                    = NULL;
	qglRecti                     = NULL;
	qglRectiv                    = NULL;
	qglRects                     = NULL;
	qglRectsv                    = NULL;
	qglRenderMode                = NULL;
	qglRotated                   = NULL;
	qglRotatef                   = NULL;
	qglScaled                    = NULL;
	qglScalef                    = NULL;
	qglScissor                   = NULL;
	qglSelectBuffer              = NULL;
	qglShadeModel                = NULL;
	qglStencilFunc               = NULL;
	qglStencilMask               = NULL;
	qglStencilOp                 = NULL;
	qglTexCoord1d                = NULL;
	qglTexCoord1dv               = NULL;
	qglTexCoord1f                = NULL;
	qglTexCoord1fv               = NULL;
	qglTexCoord1i                = NULL;
	qglTexCoord1iv               = NULL;
	qglTexCoord1s                = NULL;
	qglTexCoord1sv               = NULL;
	qglTexCoord2d                = NULL;
	qglTexCoord2dv               = NULL;
	qglTexCoord2f                = NULL;
	qglTexCoord2fv               = NULL;
	qglTexCoord2i                = NULL;
	qglTexCoord2iv               = NULL;
	qglTexCoord2s                = NULL;
	qglTexCoord2sv               = NULL;
	qglTexCoord3d                = NULL;
	qglTexCoord3dv               = NULL;
	qglTexCoord3f                = NULL;
	qglTexCoord3fv               = NULL;
	qglTexCoord3i                = NULL;
	qglTexCoord3iv               = NULL;
	qglTexCoord3s                = NULL;
	qglTexCoord3sv               = NULL;
	qglTexCoord4d                = NULL;
	qglTexCoord4dv               = NULL;
	qglTexCoord4f                = NULL;
	qglTexCoord4fv               = NULL;
	qglTexCoord4i                = NULL;
	qglTexCoord4iv               = NULL;
	qglTexCoord4s                = NULL;
	qglTexCoord4sv               = NULL;
	qglTexCoordPointer           = NULL;
	qglTexEnvf                   = NULL;
	qglTexEnvfv                  = NULL;
	qglTexEnvi                   = NULL;
	qglTexEnviv                  = NULL;
	qglTexGend                   = NULL;
	qglTexGendv                  = NULL;
	qglTexGenf                   = NULL;
	qglTexGenfv                  = NULL;
	qglTexGeni                   = NULL;
	qglTexGeniv                  = NULL;
	qglTexImage1D                = NULL;
	qglTexImage2D                = NULL;
	qglTexParameterf             = NULL;
	qglTexParameterfv            = NULL;
	qglTexParameteri             = NULL;
	qglTexParameteriv            = NULL;
	qglTexSubImage1D             = NULL;
	qglTexSubImage2D             = NULL;
	qglTranslated                = NULL;
	qglTranslatef                = NULL;
	qglVertex2d                  = NULL;
	qglVertex2dv                 = NULL;
	qglVertex2f                  = NULL;
	qglVertex2fv                 = NULL;
	qglVertex2i                  = NULL;
	qglVertex2iv                 = NULL;
	qglVertex2s                  = NULL;
	qglVertex2sv                 = NULL;
	qglVertex3d                  = NULL;
	qglVertex3dv                 = NULL;
	qglVertex3f                  = NULL;
	qglVertex3fv                 = NULL;
	qglVertex3i                  = NULL;
	qglVertex3iv                 = NULL;
	qglVertex3s                  = NULL;
	qglVertex3sv                 = NULL;
	qglVertex4d                  = NULL;
	qglVertex4dv                 = NULL;
	qglVertex4f                  = NULL;
	qglVertex4fv                 = NULL;
	qglVertex4i                  = NULL;
	qglVertex4iv                 = NULL;
	qglVertex4s                  = NULL;
	qglVertex4sv                 = NULL;
	qglVertexPointer             = NULL;
	qglViewport                  = NULL;

#ifdef WIN32
	qwglCopyContext              = NULL;
	qwglCreateContext            = NULL;
	qwglCreateLayerContext       = NULL;
	qwglDeleteContext            = NULL;
	qwglDescribeLayerPlane       = NULL;
	qwglGetCurrentContext        = NULL;
	qwglGetCurrentDC             = NULL;
	qwglGetLayerPaletteEntries   = NULL;
	qwglGetProcAddress           = NULL;
	qwglMakeCurrent              = NULL;
	qwglRealizeLayerPalette      = NULL;
	qwglSetLayerPaletteEntries   = NULL;
	qwglShareLists               = NULL;
	qwglSwapLayerBuffers         = NULL;
	qwglUseFontBitmaps           = NULL;
	qwglUseFontOutlines          = NULL;

	qwglChoosePixelFormat        = NULL;
	qwglDescribePixelFormat      = NULL;
	qwglGetPixelFormat           = NULL;
	qwglSetPixelFormat           = NULL;
	qwglSwapBuffers              = NULL;

	qwglSwapIntervalEXT          = NULL;

	qwglGetDeviceGammaRampEXT    = NULL;
	qwglSetDeviceGammaRampEXT    = NULL;
#endif
}

#ifdef WIN32

#pragma warning(disable: 4113 4133 4047)
#define GPA(a) GetProcAddress(glw_state.hinstOpenGL, a)

#else

#define GPA(a) dlsym(glw_state.OpenGLLib, a)

void *qwglGetProcAddress (char *symbol)
{
	if (glw_state.OpenGLLib)
		return GPA(symbol);

	return NULL;
}

#endif


/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
** 
*/
qboolean QGL_Init (const char *dllname)
{
	R_Init3dfxGamma(); // jit3dfx

#ifdef WIN32

	if ((glw_state.hinstOpenGL = LoadLibrary(dllname)) == 0)
	{
		char *buf = NULL;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &buf, 0, NULL);
		ri.Con_Printf(PRINT_ALL, "%s\n", buf);
		return false;
	}

#else // !WIN32

	if (glw_state.OpenGLLib)
		QGL_Shutdown();

	if ((glw_state.OpenGLLib = dlopen(dllname, RTLD_LAZY)) == 0)
	{
		char	fn[MAX_OSPATH];
		char	*path;

		// try basedir next
		path = ri.Cvar_Get("basedir", ".", CVAR_NOSET)->string;
		snprintf(fn, MAX_OSPATH, "%s/%s", path, dllname);

		if ((glw_state.OpenGLLib = dlopen(fn, RTLD_LAZY)) == 0)
		{
			Com_Printf("Failed to load \"%s\", trying \"%s\"\n", dllname, GL_DRIVER_LIB2);

			if ((glw_state.OpenGLLib = dlopen(GL_DRIVER_LIB2, RTLD_LAZY)) == 0) // jitlinux
			{
				Com_Printf("Failed to load \"%s\", trying \"%s\"\n", GL_DRIVER_LIB2, GL_DRIVER_LIB3);

				if ((glw_state.OpenGLLib = dlopen(GL_DRIVER_LIB3, RTLD_LAZY)) == 0) // jitlinux
				{
					ri.Con_Printf(PRINT_ALL, "%s\n", dlerror());
					return false;
				}
				else
				{
					Com_Printf("Using %s for OpenGL...\n", GL_DRIVER_LIB3);
				}
			}
			else
			{
				Com_Printf("Using %s for OpenGL...\n", GL_DRIVER_LIB2);
			}
		}
		else
		{
			Com_Printf("Using %s for OpenGL...\n", fn);
		}
	}
	else
	{
		Com_Printf("Using %s for OpenGL...\n", dllname);
	}

#ifdef USE_GLU
	if ((dlopen("libGLU.so", RTLD_LAZY)) == 0)
	{
		ri.Con_Printf(PRINT_ALL, "%s\n", dlerror());
		return false;
	}
	else
	{
		Com_Printf("Opened GLU sucessfully OpenGLU...\n", dllname);
	}
#endif

#endif // !WIN32

	gl_config.allow_cds = true;

	qglAccum                     = dllAccum = GPA( "glAccum" );
	qglAlphaFunc                 = dllAlphaFunc = GPA( "glAlphaFunc" );
	qglAreTexturesResident       = dllAreTexturesResident = GPA( "glAreTexturesResident" );
	qglArrayElement              = dllArrayElement = GPA( "glArrayElement" );
	qglBegin                     = dllBegin = GPA( "glBegin" );
	qglBindTexture               = dllBindTexture = GPA( "glBindTexture" );
	qglBitmap                    = dllBitmap = GPA( "glBitmap" );
	qglBlendFunc                 = dllBlendFunc = GPA( "glBlendFunc" );
	qglCallList                  = dllCallList = GPA( "glCallList" );
	qglCallLists                 = dllCallLists = GPA( "glCallLists" );
	qglClear                     = dllClear = GPA( "glClear" );
	qglClearAccum                = dllClearAccum = GPA( "glClearAccum" );
	qglClearColor                = dllClearColor = GPA( "glClearColor" );
	qglClearDepth                = dllClearDepth = GPA( "glClearDepth" );
	qglClearIndex                = dllClearIndex = GPA( "glClearIndex" );
	qglClearStencil              = dllClearStencil = GPA( "glClearStencil" );
	qglClipPlane                 = dllClipPlane = GPA( "glClipPlane" );
	qglColor3b                   = dllColor3b = GPA( "glColor3b" );
	qglColor3bv                  = dllColor3bv = GPA( "glColor3bv" );
	qglColor3d                   = dllColor3d = GPA( "glColor3d" );
	qglColor3dv                  = dllColor3dv = GPA( "glColor3dv" );
	qglColor3f                   = dllColor3f = GPA( "glColor3f" );
	qglColor3fv                  = dllColor3fv = GPA( "glColor3fv" );
	qglColor3i                   = dllColor3i = GPA( "glColor3i" );
	qglColor3iv                  = dllColor3iv = GPA( "glColor3iv" );
	qglColor3s                   = dllColor3s = GPA( "glColor3s" );
	qglColor3sv                  = dllColor3sv = GPA( "glColor3sv" );
	qglColor3ub                  = dllColor3ub = GPA( "glColor3ub" );
	qglColor3ubv                 = dllColor3ubv = GPA( "glColor3ubv" );
	qglColor3ui                  = dllColor3ui = GPA( "glColor3ui" );
	qglColor3uiv                 = dllColor3uiv = GPA( "glColor3uiv" );
	qglColor3us                  = dllColor3us = GPA( "glColor3us" );
	qglColor3usv                 = dllColor3usv = GPA( "glColor3usv" );
	qglColor4b                   = dllColor4b = GPA( "glColor4b" );
	qglColor4bv                  = dllColor4bv = GPA( "glColor4bv" );
	qglColor4d                   = dllColor4d = GPA( "glColor4d" );
	qglColor4dv                  = dllColor4dv = GPA( "glColor4dv" );
	qglColor4f                   = dllColor4f = GPA( "glColor4f" );
	qglColor4fv                  = dllColor4fv = GPA( "glColor4fv" );
	qglColor4i                   = dllColor4i = GPA( "glColor4i" );
	qglColor4iv                  = dllColor4iv = GPA( "glColor4iv" );
	qglColor4s                   = dllColor4s = GPA( "glColor4s" );
	qglColor4sv                  = dllColor4sv = GPA( "glColor4sv" );
	qglColor4ub                  = dllColor4ub = GPA( "glColor4ub" );
	qglColor4ubv                 = dllColor4ubv = GPA( "glColor4ubv" );
	qglColor4ui                  = dllColor4ui = GPA( "glColor4ui" );
	qglColor4uiv                 = dllColor4uiv = GPA( "glColor4uiv" );
	qglColor4us                  = dllColor4us = GPA( "glColor4us" );
	qglColor4usv                 = dllColor4usv = GPA( "glColor4usv" );
	qglColorMask                 = dllColorMask = GPA( "glColorMask" );
	qglColorMaterial             = dllColorMaterial = GPA( "glColorMaterial" );
	qglColorPointer              = dllColorPointer = GPA( "glColorPointer" );
	qglCopyPixels                = dllCopyPixels = GPA( "glCopyPixels" );
	qglCopyTexImage1D            = dllCopyTexImage1D = GPA( "glCopyTexImage1D" );
	qglCopyTexImage2D            = dllCopyTexImage2D = GPA( "glCopyTexImage2D" );
	qglCopyTexSubImage1D         = dllCopyTexSubImage1D = GPA( "glCopyTexSubImage1D" );
	qglCopyTexSubImage2D         = dllCopyTexSubImage2D = GPA( "glCopyTexSubImage2D" );
	qglCullFace                  = dllCullFace = GPA( "glCullFace" );
	qglDeleteLists               = dllDeleteLists = GPA( "glDeleteLists" );
	qglDeleteTextures            = dllDeleteTextures = GPA( "glDeleteTextures" );
	qglDepthFunc                 = dllDepthFunc = GPA( "glDepthFunc" );
	qglDepthMask                 = dllDepthMask = GPA( "glDepthMask" );
	qglDepthRange                = dllDepthRange = GPA( "glDepthRange" );
	qglDisable                   = dllDisable = GPA( "glDisable" );
	qglDisableClientState        = dllDisableClientState = GPA( "glDisableClientState" );
	qglDrawArrays                = dllDrawArrays = GPA( "glDrawArrays" );
	qglDrawBuffer                = dllDrawBuffer = GPA( "glDrawBuffer" );
	qglDrawElements              = dllDrawElements = GPA( "glDrawElements" );
	qglDrawPixels                = dllDrawPixels = GPA( "glDrawPixels" );
	qglEdgeFlag                  = dllEdgeFlag = GPA( "glEdgeFlag" );
	qglEdgeFlagPointer           = dllEdgeFlagPointer = GPA( "glEdgeFlagPointer" );
	qglEdgeFlagv                 = dllEdgeFlagv = GPA( "glEdgeFlagv" );
	qglEnable                    = 	dllEnable                    = GPA( "glEnable" );
	qglEnableClientState         = 	dllEnableClientState         = GPA( "glEnableClientState" );
	qglEnd                       = 	dllEnd                       = GPA( "glEnd" );
	qglEndList                   = 	dllEndList                   = GPA( "glEndList" );
	qglEvalCoord1d				 = 	dllEvalCoord1d				 = GPA( "glEvalCoord1d" );
	qglEvalCoord1dv              = 	dllEvalCoord1dv              = GPA( "glEvalCoord1dv" );
	qglEvalCoord1f               = 	dllEvalCoord1f               = GPA( "glEvalCoord1f" );
	qglEvalCoord1fv              = 	dllEvalCoord1fv              = GPA( "glEvalCoord1fv" );
	qglEvalCoord2d               = 	dllEvalCoord2d               = GPA( "glEvalCoord2d" );
	qglEvalCoord2dv              = 	dllEvalCoord2dv              = GPA( "glEvalCoord2dv" );
	qglEvalCoord2f               = 	dllEvalCoord2f               = GPA( "glEvalCoord2f" );
	qglEvalCoord2fv              = 	dllEvalCoord2fv              = GPA( "glEvalCoord2fv" );
	qglEvalMesh1                 = 	dllEvalMesh1                 = GPA( "glEvalMesh1" );
	qglEvalMesh2                 = 	dllEvalMesh2                 = GPA( "glEvalMesh2" );
	qglEvalPoint1                = 	dllEvalPoint1                = GPA( "glEvalPoint1" );
	qglEvalPoint2                = 	dllEvalPoint2                = GPA( "glEvalPoint2" );
	qglFeedbackBuffer            = 	dllFeedbackBuffer            = GPA( "glFeedbackBuffer" );
	qglFinish                    = 	dllFinish                    = GPA( "glFinish" );
	qglFlush                     = 	dllFlush                     = GPA( "glFlush" );
	qglFogf                      = 	dllFogf                      = GPA( "glFogf" );
	qglFogfv                     = 	dllFogfv                     = GPA( "glFogfv" );
	qglFogi                      = 	dllFogi                      = GPA( "glFogi" );
	qglFogiv                     = 	dllFogiv                     = GPA( "glFogiv" );
	qglFrontFace                 = 	dllFrontFace                 = GPA( "glFrontFace" );
	qglFrustum                   = 	dllFrustum                   = GPA( "glFrustum" );
	qglGenLists                  = 	dllGenLists                  = GPA( "glGenLists" );
	qglGenTextures               = 	dllGenTextures               = GPA( "glGenTextures" );
	qglGetBooleanv               = 	dllGetBooleanv               = GPA( "glGetBooleanv" );
	qglGetClipPlane              = 	dllGetClipPlane              = GPA( "glGetClipPlane" );
	qglGetDoublev                = 	dllGetDoublev                = GPA( "glGetDoublev" );
	qglGetError                  = 	dllGetError                  = GPA( "glGetError" );
	qglGetFloatv                 = 	dllGetFloatv                 = GPA( "glGetFloatv" );
	qglGetIntegerv               = 	dllGetIntegerv               = GPA( "glGetIntegerv" );
	qglGetLightfv                = 	dllGetLightfv                = GPA( "glGetLightfv" );
	qglGetLightiv                = 	dllGetLightiv                = GPA( "glGetLightiv" );
	qglGetMapdv                  = 	dllGetMapdv                  = GPA( "glGetMapdv" );
	qglGetMapfv                  = 	dllGetMapfv                  = GPA( "glGetMapfv" );
	qglGetMapiv                  = 	dllGetMapiv                  = GPA( "glGetMapiv" );
	qglGetMaterialfv             = 	dllGetMaterialfv             = GPA( "glGetMaterialfv" );
	qglGetMaterialiv             = 	dllGetMaterialiv             = GPA( "glGetMaterialiv" );
	qglGetPixelMapfv             = 	dllGetPixelMapfv             = GPA( "glGetPixelMapfv" );
	qglGetPixelMapuiv            = 	dllGetPixelMapuiv            = GPA( "glGetPixelMapuiv" );
	qglGetPixelMapusv            = 	dllGetPixelMapusv            = GPA( "glGetPixelMapusv" );
	qglGetPointerv               = 	dllGetPointerv               = GPA( "glGetPointerv" );
	qglGetPolygonStipple         = 	dllGetPolygonStipple         = GPA( "glGetPolygonStipple" );
	qglGetString                 = 	dllGetString                 = GPA( "glGetString" );
	qglGetTexEnvfv               = 	dllGetTexEnvfv               = GPA( "glGetTexEnvfv" );
	qglGetTexEnviv               = 	dllGetTexEnviv               = GPA( "glGetTexEnviv" );
	qglGetTexGendv               = 	dllGetTexGendv               = GPA( "glGetTexGendv" );
	qglGetTexGenfv               = 	dllGetTexGenfv               = GPA( "glGetTexGenfv" );
	qglGetTexGeniv               = 	dllGetTexGeniv               = GPA( "glGetTexGeniv" );
	qglGetTexImage               = 	dllGetTexImage               = GPA( "glGetTexImage" );
	qglGetTexLevelParameterfv    = 	dllGetTexLevelParameterfv    = GPA( "glGetLevelParameterfv" );
	qglGetTexLevelParameteriv    = 	dllGetTexLevelParameteriv    = GPA( "glGetLevelParameteriv" );
	qglGetTexParameterfv         = 	dllGetTexParameterfv         = GPA( "glGetTexParameterfv" );
	qglGetTexParameteriv         = 	dllGetTexParameteriv         = GPA( "glGetTexParameteriv" );
	qglHint                      = 	dllHint                      = GPA( "glHint" );
	qglIndexMask                 = 	dllIndexMask                 = GPA( "glIndexMask" );
	qglIndexPointer              = 	dllIndexPointer              = GPA( "glIndexPointer" );
	qglIndexd                    = 	dllIndexd                    = GPA( "glIndexd" );
	qglIndexdv                   = 	dllIndexdv                   = GPA( "glIndexdv" );
	qglIndexf                    = 	dllIndexf                    = GPA( "glIndexf" );
	qglIndexfv                   = 	dllIndexfv                   = GPA( "glIndexfv" );
	qglIndexi                    = 	dllIndexi                    = GPA( "glIndexi" );
	qglIndexiv                   = 	dllIndexiv                   = GPA( "glIndexiv" );
	qglIndexs                    = 	dllIndexs                    = GPA( "glIndexs" );
	qglIndexsv                   = 	dllIndexsv                   = GPA( "glIndexsv" );
	qglIndexub                   = 	dllIndexub                   = GPA( "glIndexub" );
	qglIndexubv                  = 	dllIndexubv                  = GPA( "glIndexubv" );
	qglInitNames                 = 	dllInitNames                 = GPA( "glInitNames" );
	qglInterleavedArrays         = 	dllInterleavedArrays         = GPA( "glInterleavedArrays" );
	qglIsEnabled                 = 	dllIsEnabled                 = GPA( "glIsEnabled" );
	qglIsList                    = 	dllIsList                    = GPA( "glIsList" );
	qglIsTexture                 = 	dllIsTexture                 = GPA( "glIsTexture" );
	qglLightModelf               = 	dllLightModelf               = GPA( "glLightModelf" );
	qglLightModelfv              = 	dllLightModelfv              = GPA( "glLightModelfv" );
	qglLightModeli               = 	dllLightModeli               = GPA( "glLightModeli" );
	qglLightModeliv              = 	dllLightModeliv              = GPA( "glLightModeliv" );
	qglLightf                    = 	dllLightf                    = GPA( "glLightf" );
	qglLightfv                   = 	dllLightfv                   = GPA( "glLightfv" );
	qglLighti                    = 	dllLighti                    = GPA( "glLighti" );
	qglLightiv                   = 	dllLightiv                   = GPA( "glLightiv" );
	qglLineStipple               = 	dllLineStipple               = GPA( "glLineStipple" );
	qglLineWidth                 = 	dllLineWidth                 = GPA( "glLineWidth" );
	qglListBase                  = 	dllListBase                  = GPA( "glListBase" );
	qglLoadIdentity              = 	dllLoadIdentity              = GPA( "glLoadIdentity" );
	qglLoadMatrixd               = 	dllLoadMatrixd               = GPA( "glLoadMatrixd" );
	qglLoadMatrixf               = 	dllLoadMatrixf               = GPA( "glLoadMatrixf" );
	qglLoadName                  = 	dllLoadName                  = GPA( "glLoadName" );
	qglLogicOp                   = 	dllLogicOp                   = GPA( "glLogicOp" );
	qglMap1d                     = 	dllMap1d                     = GPA( "glMap1d" );
	qglMap1f                     = 	dllMap1f                     = GPA( "glMap1f" );
	qglMap2d                     = 	dllMap2d                     = GPA( "glMap2d" );
	qglMap2f                     = 	dllMap2f                     = GPA( "glMap2f" );
	qglMapGrid1d                 = 	dllMapGrid1d                 = GPA( "glMapGrid1d" );
	qglMapGrid1f                 = 	dllMapGrid1f                 = GPA( "glMapGrid1f" );
	qglMapGrid2d                 = 	dllMapGrid2d                 = GPA( "glMapGrid2d" );
	qglMapGrid2f                 = 	dllMapGrid2f                 = GPA( "glMapGrid2f" );
	qglMaterialf                 = 	dllMaterialf                 = GPA( "glMaterialf" );
	qglMaterialfv                = 	dllMaterialfv                = GPA( "glMaterialfv" );
	qglMateriali                 = 	dllMateriali                 = GPA( "glMateriali" );
	qglMaterialiv                = 	dllMaterialiv                = GPA( "glMaterialiv" );
	qglMatrixMode                = 	dllMatrixMode                = GPA( "glMatrixMode" );
	qglMultMatrixd               = 	dllMultMatrixd               = GPA( "glMultMatrixd" );
	qglMultMatrixf               = 	dllMultMatrixf               = GPA( "glMultMatrixf" );
	qglNewList                   = 	dllNewList                   = GPA( "glNewList" );
	qglNormal3b                  = 	dllNormal3b                  = GPA( "glNormal3b" );
	qglNormal3bv                 = 	dllNormal3bv                 = GPA( "glNormal3bv" );
	qglNormal3d                  = 	dllNormal3d                  = GPA( "glNormal3d" );
	qglNormal3dv                 = 	dllNormal3dv                 = GPA( "glNormal3dv" );
	qglNormal3f                  = 	dllNormal3f                  = GPA( "glNormal3f" );
	qglNormal3fv                 = 	dllNormal3fv                 = GPA( "glNormal3fv" );
	qglNormal3i                  = 	dllNormal3i                  = GPA( "glNormal3i" );
	qglNormal3iv                 = 	dllNormal3iv                 = GPA( "glNormal3iv" );
	qglNormal3s                  = 	dllNormal3s                  = GPA( "glNormal3s" );
	qglNormal3sv                 = 	dllNormal3sv                 = GPA( "glNormal3sv" );
	qglNormalPointer             = 	dllNormalPointer             = GPA( "glNormalPointer" );
	qglOrtho                     = 	dllOrtho                     = GPA( "glOrtho" );
	qglPassThrough               = 	dllPassThrough               = GPA( "glPassThrough" );
	qglPixelMapfv                = 	dllPixelMapfv                = GPA( "glPixelMapfv" );
	qglPixelMapuiv               = 	dllPixelMapuiv               = GPA( "glPixelMapuiv" );
	qglPixelMapusv               = 	dllPixelMapusv               = GPA( "glPixelMapusv" );
	qglPixelStoref               = 	dllPixelStoref               = GPA( "glPixelStoref" );
	qglPixelStorei               = 	dllPixelStorei               = GPA( "glPixelStorei" );
	qglPixelTransferf            = 	dllPixelTransferf            = GPA( "glPixelTransferf" );
	qglPixelTransferi            = 	dllPixelTransferi            = GPA( "glPixelTransferi" );
	qglPixelZoom                 = 	dllPixelZoom                 = GPA( "glPixelZoom" );
	qglPointSize                 = 	dllPointSize                 = GPA( "glPointSize" );
	qglPolygonMode               = 	dllPolygonMode               = GPA( "glPolygonMode" );
	qglPolygonOffset             = 	dllPolygonOffset             = GPA( "glPolygonOffset" );
	qglPolygonStipple            = 	dllPolygonStipple            = GPA( "glPolygonStipple" );
	qglPopAttrib                 = 	dllPopAttrib                 = GPA( "glPopAttrib" );
	qglPopClientAttrib           = 	dllPopClientAttrib           = GPA( "glPopClientAttrib" );
	qglPopMatrix                 = 	dllPopMatrix                 = GPA( "glPopMatrix" );
	qglPopName                   = 	dllPopName                   = GPA( "glPopName" );
	qglPrioritizeTextures        = 	dllPrioritizeTextures        = GPA( "glPrioritizeTextures" );
	qglPushAttrib                = 	dllPushAttrib                = GPA( "glPushAttrib" );
	qglPushClientAttrib          = 	dllPushClientAttrib          = GPA( "glPushClientAttrib" );
	qglPushMatrix                = 	dllPushMatrix                = GPA( "glPushMatrix" );
	qglPushName                  = 	dllPushName                  = GPA( "glPushName" );
	qglRasterPos2d               = 	dllRasterPos2d               = GPA( "glRasterPos2d" );
	qglRasterPos2dv              = 	dllRasterPos2dv              = GPA( "glRasterPos2dv" );
	qglRasterPos2f               = 	dllRasterPos2f               = GPA( "glRasterPos2f" );
	qglRasterPos2fv              = 	dllRasterPos2fv              = GPA( "glRasterPos2fv" );
	qglRasterPos2i               = 	dllRasterPos2i               = GPA( "glRasterPos2i" );
	qglRasterPos2iv              = 	dllRasterPos2iv              = GPA( "glRasterPos2iv" );
	qglRasterPos2s               = 	dllRasterPos2s               = GPA( "glRasterPos2s" );
	qglRasterPos2sv              = 	dllRasterPos2sv              = GPA( "glRasterPos2sv" );
	qglRasterPos3d               = 	dllRasterPos3d               = GPA( "glRasterPos3d" );
	qglRasterPos3dv              = 	dllRasterPos3dv              = GPA( "glRasterPos3dv" );
	qglRasterPos3f               = 	dllRasterPos3f               = GPA( "glRasterPos3f" );
	qglRasterPos3fv              = 	dllRasterPos3fv              = GPA( "glRasterPos3fv" );
	qglRasterPos3i               = 	dllRasterPos3i               = GPA( "glRasterPos3i" );
	qglRasterPos3iv              = 	dllRasterPos3iv              = GPA( "glRasterPos3iv" );
	qglRasterPos3s               = 	dllRasterPos3s               = GPA( "glRasterPos3s" );
	qglRasterPos3sv              = 	dllRasterPos3sv              = GPA( "glRasterPos3sv" );
	qglRasterPos4d               = 	dllRasterPos4d               = GPA( "glRasterPos4d" );
	qglRasterPos4dv              = 	dllRasterPos4dv              = GPA( "glRasterPos4dv" );
	qglRasterPos4f               = 	dllRasterPos4f               = GPA( "glRasterPos4f" );
	qglRasterPos4fv              = 	dllRasterPos4fv              = GPA( "glRasterPos4fv" );
	qglRasterPos4i               = 	dllRasterPos4i               = GPA( "glRasterPos4i" );
	qglRasterPos4iv              = 	dllRasterPos4iv              = GPA( "glRasterPos4iv" );
	qglRasterPos4s               = 	dllRasterPos4s               = GPA( "glRasterPos4s" );
	qglRasterPos4sv              = 	dllRasterPos4sv              = GPA( "glRasterPos4sv" );
	qglReadBuffer                = 	dllReadBuffer                = GPA( "glReadBuffer" );
	qglReadPixels                = 	dllReadPixels                = GPA( "glReadPixels" );
	qglRectd                     = 	dllRectd                     = GPA( "glRectd" );
	qglRectdv                    = 	dllRectdv                    = GPA( "glRectdv" );
	qglRectf                     = 	dllRectf                     = GPA( "glRectf" );
	qglRectfv                    = 	dllRectfv                    = GPA( "glRectfv" );
	qglRecti                     = 	dllRecti                     = GPA( "glRecti" );
	qglRectiv                    = 	dllRectiv                    = GPA( "glRectiv" );
	qglRects                     = 	dllRects                     = GPA( "glRects" );
	qglRectsv                    = 	dllRectsv                    = GPA( "glRectsv" );
	qglRenderMode                = 	dllRenderMode                = GPA( "glRenderMode" );
	qglRotated                   = 	dllRotated                   = GPA( "glRotated" );
	qglRotatef                   = 	dllRotatef                   = GPA( "glRotatef" );
	qglScaled                    = 	dllScaled                    = GPA( "glScaled" );
	qglScalef                    = 	dllScalef                    = GPA( "glScalef" );
	qglScissor                   = 	dllScissor                   = GPA( "glScissor" );
	qglSelectBuffer              = 	dllSelectBuffer              = GPA( "glSelectBuffer" );
	qglShadeModel                = 	dllShadeModel                = GPA( "glShadeModel" );
	qglStencilFunc               = 	dllStencilFunc               = GPA( "glStencilFunc" );
	qglStencilMask               = 	dllStencilMask               = GPA( "glStencilMask" );
	qglStencilOp                 = 	dllStencilOp                 = GPA( "glStencilOp" );
	qglTexCoord1d                = 	dllTexCoord1d                = GPA( "glTexCoord1d" );
	qglTexCoord1dv               = 	dllTexCoord1dv               = GPA( "glTexCoord1dv" );
	qglTexCoord1f                = 	dllTexCoord1f                = GPA( "glTexCoord1f" );
	qglTexCoord1fv               = 	dllTexCoord1fv               = GPA( "glTexCoord1fv" );
	qglTexCoord1i                = 	dllTexCoord1i                = GPA( "glTexCoord1i" );
	qglTexCoord1iv               = 	dllTexCoord1iv               = GPA( "glTexCoord1iv" );
	qglTexCoord1s                = 	dllTexCoord1s                = GPA( "glTexCoord1s" );
	qglTexCoord1sv               = 	dllTexCoord1sv               = GPA( "glTexCoord1sv" );
	qglTexCoord2d                = 	dllTexCoord2d                = GPA( "glTexCoord2d" );
	qglTexCoord2dv               = 	dllTexCoord2dv               = GPA( "glTexCoord2dv" );
	qglTexCoord2f                = 	dllTexCoord2f                = GPA( "glTexCoord2f" );
	qglTexCoord2fv               = 	dllTexCoord2fv               = GPA( "glTexCoord2fv" );
	qglTexCoord2i                = 	dllTexCoord2i                = GPA( "glTexCoord2i" );
	qglTexCoord2iv               = 	dllTexCoord2iv               = GPA( "glTexCoord2iv" );
	qglTexCoord2s                = 	dllTexCoord2s                = GPA( "glTexCoord2s" );
	qglTexCoord2sv               = 	dllTexCoord2sv               = GPA( "glTexCoord2sv" );
	qglTexCoord3d                = 	dllTexCoord3d                = GPA( "glTexCoord3d" );
	qglTexCoord3dv               = 	dllTexCoord3dv               = GPA( "glTexCoord3dv" );
	qglTexCoord3f                = 	dllTexCoord3f                = GPA( "glTexCoord3f" );
	qglTexCoord3fv               = 	dllTexCoord3fv               = GPA( "glTexCoord3fv" );
	qglTexCoord3i                = 	dllTexCoord3i                = GPA( "glTexCoord3i" );
	qglTexCoord3iv               = 	dllTexCoord3iv               = GPA( "glTexCoord3iv" );
	qglTexCoord3s                = 	dllTexCoord3s                = GPA( "glTexCoord3s" );
	qglTexCoord3sv               = 	dllTexCoord3sv               = GPA( "glTexCoord3sv" );
	qglTexCoord4d                = 	dllTexCoord4d                = GPA( "glTexCoord4d" );
	qglTexCoord4dv               = 	dllTexCoord4dv               = GPA( "glTexCoord4dv" );
	qglTexCoord4f                = 	dllTexCoord4f                = GPA( "glTexCoord4f" );
	qglTexCoord4fv               = 	dllTexCoord4fv               = GPA( "glTexCoord4fv" );
	qglTexCoord4i                = 	dllTexCoord4i                = GPA( "glTexCoord4i" );
	qglTexCoord4iv               = 	dllTexCoord4iv               = GPA( "glTexCoord4iv" );
	qglTexCoord4s                = 	dllTexCoord4s                = GPA( "glTexCoord4s" );
	qglTexCoord4sv               = 	dllTexCoord4sv               = GPA( "glTexCoord4sv" );
	qglTexCoordPointer           = 	dllTexCoordPointer           = GPA( "glTexCoordPointer" );
	qglTexEnvf                   = 	dllTexEnvf                   = GPA( "glTexEnvf" );
	qglTexEnvfv                  = 	dllTexEnvfv                  = GPA( "glTexEnvfv" );
	qglTexEnvi                   = 	dllTexEnvi                   = GPA( "glTexEnvi" );
	qglTexEnviv                  = 	dllTexEnviv                  = GPA( "glTexEnviv" );
	qglTexGend                   = 	dllTexGend                   = GPA( "glTexGend" );
	qglTexGendv                  = 	dllTexGendv                  = GPA( "glTexGendv" );
	qglTexGenf                   = 	dllTexGenf                   = GPA( "glTexGenf" );
	qglTexGenfv                  = 	dllTexGenfv                  = GPA( "glTexGenfv" );
	qglTexGeni                   = 	dllTexGeni                   = GPA( "glTexGeni" );
	qglTexGeniv                  = 	dllTexGeniv                  = GPA( "glTexGeniv" );
	qglTexImage1D                = 	dllTexImage1D                = GPA( "glTexImage1D" );
	qglTexImage2D                = 	dllTexImage2D                = GPA( "glTexImage2D" );
	qglTexParameterf             = 	dllTexParameterf             = GPA( "glTexParameterf" );
	qglTexParameterfv            = 	dllTexParameterfv            = GPA( "glTexParameterfv" );
	qglTexParameteri             = 	dllTexParameteri             = GPA( "glTexParameteri" );
	qglTexParameteriv            = 	dllTexParameteriv            = GPA( "glTexParameteriv" );
	qglTexSubImage1D             = 	dllTexSubImage1D             = GPA( "glTexSubImage1D" );
	qglTexSubImage2D             = 	dllTexSubImage2D             = GPA( "glTexSubImage2D" );
	qglTranslated                = 	dllTranslated                = GPA( "glTranslated" );
	qglTranslatef                = 	dllTranslatef                = GPA( "glTranslatef" );
	qglVertex2d                  = 	dllVertex2d                  = GPA( "glVertex2d" );
	qglVertex2dv                 = 	dllVertex2dv                 = GPA( "glVertex2dv" );
	qglVertex2f                  = 	dllVertex2f                  = GPA( "glVertex2f" );
	qglVertex2fv                 = 	dllVertex2fv                 = GPA( "glVertex2fv" );
	qglVertex2i                  = 	dllVertex2i                  = GPA( "glVertex2i" );
	qglVertex2iv                 = 	dllVertex2iv                 = GPA( "glVertex2iv" );
	qglVertex2s                  = 	dllVertex2s                  = GPA( "glVertex2s" );
	qglVertex2sv                 = 	dllVertex2sv                 = GPA( "glVertex2sv" );
	qglVertex3d                  = 	dllVertex3d                  = GPA( "glVertex3d" );
	qglVertex3dv                 = 	dllVertex3dv                 = GPA( "glVertex3dv" );
	qglVertex3f                  = 	dllVertex3f                  = GPA( "glVertex3f" );
	qglVertex3fv                 = 	dllVertex3fv                 = GPA( "glVertex3fv" );
	qglVertex3i                  = 	dllVertex3i                  = GPA( "glVertex3i" );
	qglVertex3iv                 = 	dllVertex3iv                 = GPA( "glVertex3iv" );
	qglVertex3s                  = 	dllVertex3s                  = GPA( "glVertex3s" );
	qglVertex3sv                 = 	dllVertex3sv                 = GPA( "glVertex3sv" );
	qglVertex4d                  = 	dllVertex4d                  = GPA( "glVertex4d" );
	qglVertex4dv                 = 	dllVertex4dv                 = GPA( "glVertex4dv" );
	qglVertex4f                  = 	dllVertex4f                  = GPA( "glVertex4f" );
	qglVertex4fv                 = 	dllVertex4fv                 = GPA( "glVertex4fv" );
	qglVertex4i                  = 	dllVertex4i                  = GPA( "glVertex4i" );
	qglVertex4iv                 = 	dllVertex4iv                 = GPA( "glVertex4iv" );
	qglVertex4s                  = 	dllVertex4s                  = GPA( "glVertex4s" );
	qglVertex4sv                 = 	dllVertex4sv                 = GPA( "glVertex4sv" );
	qglVertexPointer             = 	dllVertexPointer             = GPA( "glVertexPointer" );
	qglViewport                  = 	dllViewport                  = GPA( "glViewport" );

#ifdef WIN32
	qwglCopyContext              = GPA( "wglCopyContext" );
	qwglCreateContext            = GPA( "wglCreateContext" );
	qwglCreateLayerContext       = GPA( "wglCreateLayerContext" );
	qwglDeleteContext            = GPA( "wglDeleteContext" );
	qwglDescribeLayerPlane       = GPA( "wglDescribeLayerPlane" );
	qwglGetCurrentContext        = GPA( "wglGetCurrentContext" );
	qwglGetCurrentDC             = GPA( "wglGetCurrentDC" );
	qwglGetLayerPaletteEntries   = GPA( "wglGetLayerPaletteEntries" );
	qwglGetProcAddress           = GPA( "wglGetProcAddress" );
	qwglMakeCurrent              = GPA( "wglMakeCurrent" );
	qwglRealizeLayerPalette      = GPA( "wglRealizeLayerPalette" );
	qwglSetLayerPaletteEntries   = GPA( "wglSetLayerPaletteEntries" );
	qwglShareLists               = GPA( "wglShareLists" );
	qwglSwapLayerBuffers         = GPA( "wglSwapLayerBuffers" );
	qwglUseFontBitmaps           = GPA( "wglUseFontBitmapsA" );
	qwglUseFontOutlines          = GPA( "wglUseFontOutlinesA" );

	qwglChoosePixelFormat        = GPA( "wglChoosePixelFormat" );
	qwglDescribePixelFormat      = GPA( "wglDescribePixelFormat" );
	qwglGetPixelFormat           = GPA( "wglGetPixelFormat" );
	qwglSetPixelFormat           = GPA( "wglSetPixelFormat" );
	qwglSwapBuffers              = GPA( "wglSwapBuffers" );

	qwglSwapIntervalEXT = 0;
#endif

	qglPointParameterfEXT = 0;
	qglPointParameterfvEXT = 0;
	qglColorTableEXT = 0;
	qglSelectTextureSGIS = 0;
	qglMultiTexCoord2fARB = 0;
	qglMultiTexCoord3fARB = 0;
	qglMultiTexCoord3fvARB = 0;

	return true;
}

void GLimp_EnableLogging( qboolean enable )
{
	if ( enable )
	{
		if ( !glw_state.log_fp )
		{
			struct tm *newtime;
			time_t aclock;
			char buffer[1024];

			time( &aclock );
			newtime = localtime( &aclock );

			asctime( newtime );

			Com_sprintf( buffer, sizeof(buffer), "%s/gl.log", ri.FS_Gamedir() ); 
			glw_state.log_fp = fopen( buffer, "wt" );

			fprintf( glw_state.log_fp, "%s\n", asctime( newtime ) );
		}

		qglAccum                     = logAccum;
		qglAlphaFunc                 = logAlphaFunc;
		qglAreTexturesResident       = logAreTexturesResident;
		qglArrayElement              = logArrayElement;
		qglBegin                     = logBegin;
		qglBindTexture               = logBindTexture;
		qglBitmap                    = logBitmap;
		qglBlendFunc                 = logBlendFunc;
		qglCallList                  = logCallList;
		qglCallLists                 = logCallLists;
		qglClear                     = logClear;
		qglClearAccum                = logClearAccum;
		qglClearColor                = logClearColor;
		qglClearDepth                = logClearDepth;
		qglClearIndex                = logClearIndex;
		qglClearStencil              = logClearStencil;
		qglClipPlane                 = logClipPlane;
		qglColor3b                   = logColor3b;
		qglColor3bv                  = logColor3bv;
		qglColor3d                   = logColor3d;
		qglColor3dv                  = logColor3dv;
		qglColor3f                   = logColor3f;
		qglColor3fv                  = logColor3fv;
		qglColor3i                   = logColor3i;
		qglColor3iv                  = logColor3iv;
		qglColor3s                   = logColor3s;
		qglColor3sv                  = logColor3sv;
		qglColor3ub                  = logColor3ub;
		qglColor3ubv                 = logColor3ubv;
		qglColor3ui                  = logColor3ui;
		qglColor3uiv                 = logColor3uiv;
		qglColor3us                  = logColor3us;
		qglColor3usv                 = logColor3usv;
		qglColor4b                   = logColor4b;
		qglColor4bv                  = logColor4bv;
		qglColor4d                   = logColor4d;
		qglColor4dv                  = logColor4dv;
		qglColor4f                   = logColor4f;
		qglColor4fv                  = logColor4fv;
		qglColor4i                   = logColor4i;
		qglColor4iv                  = logColor4iv;
		qglColor4s                   = logColor4s;
		qglColor4sv                  = logColor4sv;
		qglColor4ub                  = logColor4ub;
		qglColor4ubv                 = logColor4ubv;
		qglColor4ui                  = logColor4ui;
		qglColor4uiv                 = logColor4uiv;
		qglColor4us                  = logColor4us;
		qglColor4usv                 = logColor4usv;
		qglColorMask                 = logColorMask;
		qglColorMaterial             = logColorMaterial;
		qglColorPointer              = logColorPointer;
		qglCopyPixels                = logCopyPixels;
		qglCopyTexImage1D            = logCopyTexImage1D;
		qglCopyTexImage2D            = logCopyTexImage2D;
		qglCopyTexSubImage1D         = logCopyTexSubImage1D;
		qglCopyTexSubImage2D         = logCopyTexSubImage2D;
		qglCullFace                  = logCullFace;
		qglDeleteLists               = logDeleteLists ;
		qglDeleteTextures            = logDeleteTextures ;
		qglDepthFunc                 = logDepthFunc ;
		qglDepthMask                 = logDepthMask ;
		qglDepthRange                = logDepthRange ;
		qglDisable                   = logDisable ;
		qglDisableClientState        = logDisableClientState ;
		qglDrawArrays                = logDrawArrays ;
		qglDrawBuffer                = logDrawBuffer ;
		qglDrawElements              = logDrawElements ;
		qglDrawPixels                = logDrawPixels ;
		qglEdgeFlag                  = logEdgeFlag ;
		qglEdgeFlagPointer           = logEdgeFlagPointer ;
		qglEdgeFlagv                 = logEdgeFlagv ;
		qglEnable                    = 	logEnable                    ;
		qglEnableClientState         = 	logEnableClientState         ;
		qglEnd                       = 	logEnd                       ;
		qglEndList                   = 	logEndList                   ;
		qglEvalCoord1d				 = 	logEvalCoord1d				 ;
		qglEvalCoord1dv              = 	logEvalCoord1dv              ;
		qglEvalCoord1f               = 	logEvalCoord1f               ;
		qglEvalCoord1fv              = 	logEvalCoord1fv              ;
		qglEvalCoord2d               = 	logEvalCoord2d               ;
		qglEvalCoord2dv              = 	logEvalCoord2dv              ;
		qglEvalCoord2f               = 	logEvalCoord2f               ;
		qglEvalCoord2fv              = 	logEvalCoord2fv              ;
		qglEvalMesh1                 = 	logEvalMesh1                 ;
		qglEvalMesh2                 = 	logEvalMesh2                 ;
		qglEvalPoint1                = 	logEvalPoint1                ;
		qglEvalPoint2                = 	logEvalPoint2                ;
		qglFeedbackBuffer            = 	logFeedbackBuffer            ;
		qglFinish                    = 	logFinish                    ;
		qglFlush                     = 	logFlush                     ;
		qglFogf                      = 	logFogf                      ;
		qglFogfv                     = 	logFogfv                     ;
		qglFogi                      = 	logFogi                      ;
		qglFogiv                     = 	logFogiv                     ;
		qglFrontFace                 = 	logFrontFace                 ;
		qglFrustum                   = 	logFrustum                   ;
		qglGenLists                  = 	logGenLists                  ;
		qglGenTextures               = 	logGenTextures               ;
		qglGetBooleanv               = 	logGetBooleanv               ;
		qglGetClipPlane              = 	logGetClipPlane              ;
		qglGetDoublev                = 	logGetDoublev                ;
		qglGetError                  = 	logGetError                  ;
		qglGetFloatv                 = 	logGetFloatv                 ;
		qglGetIntegerv               = 	logGetIntegerv               ;
		qglGetLightfv                = 	logGetLightfv                ;
		qglGetLightiv                = 	logGetLightiv                ;
		qglGetMapdv                  = 	logGetMapdv                  ;
		qglGetMapfv                  = 	logGetMapfv                  ;
		qglGetMapiv                  = 	logGetMapiv                  ;
		qglGetMaterialfv             = 	logGetMaterialfv             ;
		qglGetMaterialiv             = 	logGetMaterialiv             ;
		qglGetPixelMapfv             = 	logGetPixelMapfv             ;
		qglGetPixelMapuiv            = 	logGetPixelMapuiv            ;
		qglGetPixelMapusv            = 	logGetPixelMapusv            ;
		qglGetPointerv               = 	logGetPointerv               ;
		qglGetPolygonStipple         = 	logGetPolygonStipple         ;
		qglGetString                 = 	logGetString                 ;
		qglGetTexEnvfv               = 	logGetTexEnvfv               ;
		qglGetTexEnviv               = 	logGetTexEnviv               ;
		qglGetTexGendv               = 	logGetTexGendv               ;
		qglGetTexGenfv               = 	logGetTexGenfv               ;
		qglGetTexGeniv               = 	logGetTexGeniv               ;
		qglGetTexImage               = 	logGetTexImage               ;
		qglGetTexLevelParameterfv    = 	logGetTexLevelParameterfv    ;
		qglGetTexLevelParameteriv    = 	logGetTexLevelParameteriv    ;
		qglGetTexParameterfv         = 	logGetTexParameterfv         ;
		qglGetTexParameteriv         = 	logGetTexParameteriv         ;
		qglHint                      = 	logHint                      ;
		qglIndexMask                 = 	logIndexMask                 ;
		qglIndexPointer              = 	logIndexPointer              ;
		qglIndexd                    = 	logIndexd                    ;
		qglIndexdv                   = 	logIndexdv                   ;
		qglIndexf                    = 	logIndexf                    ;
		qglIndexfv                   = 	logIndexfv                   ;
		qglIndexi                    = 	logIndexi                    ;
		qglIndexiv                   = 	logIndexiv                   ;
		qglIndexs                    = 	logIndexs                    ;
		qglIndexsv                   = 	logIndexsv                   ;
		qglIndexub                   = 	logIndexub                   ;
		qglIndexubv                  = 	logIndexubv                  ;
		qglInitNames                 = 	logInitNames                 ;
		qglInterleavedArrays         = 	logInterleavedArrays         ;
		qglIsEnabled                 = 	logIsEnabled                 ;
		qglIsList                    = 	logIsList                    ;
		qglIsTexture                 = 	logIsTexture                 ;
		qglLightModelf               = 	logLightModelf               ;
		qglLightModelfv              = 	logLightModelfv              ;
		qglLightModeli               = 	logLightModeli               ;
		qglLightModeliv              = 	logLightModeliv              ;
		qglLightf                    = 	logLightf                    ;
		qglLightfv                   = 	logLightfv                   ;
		qglLighti                    = 	logLighti                    ;
		qglLightiv                   = 	logLightiv                   ;
		qglLineStipple               = 	logLineStipple               ;
		qglLineWidth                 = 	logLineWidth                 ;
		qglListBase                  = 	logListBase                  ;
		qglLoadIdentity              = 	logLoadIdentity              ;
		qglLoadMatrixd               = 	logLoadMatrixd               ;
		qglLoadMatrixf               = 	logLoadMatrixf               ;
		qglLoadName                  = 	logLoadName                  ;
		qglLogicOp                   = 	logLogicOp                   ;
		qglMap1d                     = 	logMap1d                     ;
		qglMap1f                     = 	logMap1f                     ;
		qglMap2d                     = 	logMap2d                     ;
		qglMap2f                     = 	logMap2f                     ;
		qglMapGrid1d                 = 	logMapGrid1d                 ;
		qglMapGrid1f                 = 	logMapGrid1f                 ;
		qglMapGrid2d                 = 	logMapGrid2d                 ;
		qglMapGrid2f                 = 	logMapGrid2f                 ;
		qglMaterialf                 = 	logMaterialf                 ;
		qglMaterialfv                = 	logMaterialfv                ;
		qglMateriali                 = 	logMateriali                 ;
		qglMaterialiv                = 	logMaterialiv                ;
		qglMatrixMode                = 	logMatrixMode                ;
		qglMultMatrixd               = 	logMultMatrixd               ;
		qglMultMatrixf               = 	logMultMatrixf               ;
		qglNewList                   = 	logNewList                   ;
		qglNormal3b                  = 	logNormal3b                  ;
		qglNormal3bv                 = 	logNormal3bv                 ;
		qglNormal3d                  = 	logNormal3d                  ;
		qglNormal3dv                 = 	logNormal3dv                 ;
		qglNormal3f                  = 	logNormal3f                  ;
		qglNormal3fv                 = 	logNormal3fv                 ;
		qglNormal3i                  = 	logNormal3i                  ;
		qglNormal3iv                 = 	logNormal3iv                 ;
		qglNormal3s                  = 	logNormal3s                  ;
		qglNormal3sv                 = 	logNormal3sv                 ;
		qglNormalPointer             = 	logNormalPointer             ;
		qglOrtho                     = 	logOrtho                     ;
		qglPassThrough               = 	logPassThrough               ;
		qglPixelMapfv                = 	logPixelMapfv                ;
		qglPixelMapuiv               = 	logPixelMapuiv               ;
		qglPixelMapusv               = 	logPixelMapusv               ;
		qglPixelStoref               = 	logPixelStoref               ;
		qglPixelStorei               = 	logPixelStorei               ;
		qglPixelTransferf            = 	logPixelTransferf            ;
		qglPixelTransferi            = 	logPixelTransferi            ;
		qglPixelZoom                 = 	logPixelZoom                 ;
		qglPointSize                 = 	logPointSize                 ;
		qglPolygonMode               = 	logPolygonMode               ;
		qglPolygonOffset             = 	logPolygonOffset             ;
		qglPolygonStipple            = 	logPolygonStipple            ;
		qglPopAttrib                 = 	logPopAttrib                 ;
		qglPopClientAttrib           = 	logPopClientAttrib           ;
		qglPopMatrix                 = 	logPopMatrix                 ;
		qglPopName                   = 	logPopName                   ;
		qglPrioritizeTextures        = 	logPrioritizeTextures        ;
		qglPushAttrib                = 	logPushAttrib                ;
		qglPushClientAttrib          = 	logPushClientAttrib          ;
		qglPushMatrix                = 	logPushMatrix                ;
		qglPushName                  = 	logPushName                  ;
		qglRasterPos2d               = 	logRasterPos2d               ;
		qglRasterPos2dv              = 	logRasterPos2dv              ;
		qglRasterPos2f               = 	logRasterPos2f               ;
		qglRasterPos2fv              = 	logRasterPos2fv              ;
		qglRasterPos2i               = 	logRasterPos2i               ;
		qglRasterPos2iv              = 	logRasterPos2iv              ;
		qglRasterPos2s               = 	logRasterPos2s               ;
		qglRasterPos2sv              = 	logRasterPos2sv              ;
		qglRasterPos3d               = 	logRasterPos3d               ;
		qglRasterPos3dv              = 	logRasterPos3dv              ;
		qglRasterPos3f               = 	logRasterPos3f               ;
		qglRasterPos3fv              = 	logRasterPos3fv              ;
		qglRasterPos3i               = 	logRasterPos3i               ;
		qglRasterPos3iv              = 	logRasterPos3iv              ;
		qglRasterPos3s               = 	logRasterPos3s               ;
		qglRasterPos3sv              = 	logRasterPos3sv              ;
		qglRasterPos4d               = 	logRasterPos4d               ;
		qglRasterPos4dv              = 	logRasterPos4dv              ;
		qglRasterPos4f               = 	logRasterPos4f               ;
		qglRasterPos4fv              = 	logRasterPos4fv              ;
		qglRasterPos4i               = 	logRasterPos4i               ;
		qglRasterPos4iv              = 	logRasterPos4iv              ;
		qglRasterPos4s               = 	logRasterPos4s               ;
		qglRasterPos4sv              = 	logRasterPos4sv              ;
		qglReadBuffer                = 	logReadBuffer                ;
		qglReadPixels                = 	logReadPixels                ;
		qglRectd                     = 	logRectd                     ;
		qglRectdv                    = 	logRectdv                    ;
		qglRectf                     = 	logRectf                     ;
		qglRectfv                    = 	logRectfv                    ;
		qglRecti                     = 	logRecti                     ;
		qglRectiv                    = 	logRectiv                    ;
		qglRects                     = 	logRects                     ;
		qglRectsv                    = 	logRectsv                    ;
		qglRenderMode                = 	logRenderMode                ;
		qglRotated                   = 	logRotated                   ;
		qglRotatef                   = 	logRotatef                   ;
		qglScaled                    = 	logScaled                    ;
		qglScalef                    = 	logScalef                    ;
		qglScissor                   = 	logScissor                   ;
		qglSelectBuffer              = 	logSelectBuffer              ;
		qglShadeModel                = 	logShadeModel                ;
		qglStencilFunc               = 	logStencilFunc               ;
		qglStencilMask               = 	logStencilMask               ;
		qglStencilOp                 = 	logStencilOp                 ;
		qglTexCoord1d                = 	logTexCoord1d                ;
		qglTexCoord1dv               = 	logTexCoord1dv               ;
		qglTexCoord1f                = 	logTexCoord1f                ;
		qglTexCoord1fv               = 	logTexCoord1fv               ;
		qglTexCoord1i                = 	logTexCoord1i                ;
		qglTexCoord1iv               = 	logTexCoord1iv               ;
		qglTexCoord1s                = 	logTexCoord1s                ;
		qglTexCoord1sv               = 	logTexCoord1sv               ;
		qglTexCoord2d                = 	logTexCoord2d                ;
		qglTexCoord2dv               = 	logTexCoord2dv               ;
		qglTexCoord2f                = 	logTexCoord2f                ;
		qglTexCoord2fv               = 	logTexCoord2fv               ;
		qglTexCoord2i                = 	logTexCoord2i                ;
		qglTexCoord2iv               = 	logTexCoord2iv               ;
		qglTexCoord2s                = 	logTexCoord2s                ;
		qglTexCoord2sv               = 	logTexCoord2sv               ;
		qglTexCoord3d                = 	logTexCoord3d                ;
		qglTexCoord3dv               = 	logTexCoord3dv               ;
		qglTexCoord3f                = 	logTexCoord3f                ;
		qglTexCoord3fv               = 	logTexCoord3fv               ;
		qglTexCoord3i                = 	logTexCoord3i                ;
		qglTexCoord3iv               = 	logTexCoord3iv               ;
		qglTexCoord3s                = 	logTexCoord3s                ;
		qglTexCoord3sv               = 	logTexCoord3sv               ;
		qglTexCoord4d                = 	logTexCoord4d                ;
		qglTexCoord4dv               = 	logTexCoord4dv               ;
		qglTexCoord4f                = 	logTexCoord4f                ;
		qglTexCoord4fv               = 	logTexCoord4fv               ;
		qglTexCoord4i                = 	logTexCoord4i                ;
		qglTexCoord4iv               = 	logTexCoord4iv               ;
		qglTexCoord4s                = 	logTexCoord4s                ;
		qglTexCoord4sv               = 	logTexCoord4sv               ;
		qglTexCoordPointer           = 	logTexCoordPointer           ;
		qglTexEnvf                   = 	logTexEnvf                   ;
		qglTexEnvfv                  = 	logTexEnvfv                  ;
		qglTexEnvi                   = 	logTexEnvi                   ;
		qglTexEnviv                  = 	logTexEnviv                  ;
		qglTexGend                   = 	logTexGend                   ;
		qglTexGendv                  = 	logTexGendv                  ;
		qglTexGenf                   = 	logTexGenf                   ;
		qglTexGenfv                  = 	logTexGenfv                  ;
		qglTexGeni                   = 	logTexGeni                   ;
		qglTexGeniv                  = 	logTexGeniv                  ;
		qglTexImage1D                = 	logTexImage1D                ;
		qglTexImage2D                = 	logTexImage2D                ;
		qglTexParameterf             = 	logTexParameterf             ;
		qglTexParameterfv            = 	logTexParameterfv            ;
		qglTexParameteri             = 	logTexParameteri             ;
		qglTexParameteriv            = 	logTexParameteriv            ;
		qglTexSubImage1D             = 	logTexSubImage1D             ;
		qglTexSubImage2D             = 	logTexSubImage2D             ;
		qglTranslated                = 	logTranslated                ;
		qglTranslatef                = 	logTranslatef                ;
		qglVertex2d                  = 	logVertex2d                  ;
		qglVertex2dv                 = 	logVertex2dv                 ;
		qglVertex2f                  = 	logVertex2f                  ;
		qglVertex2fv                 = 	logVertex2fv                 ;
		qglVertex2i                  = 	logVertex2i                  ;
		qglVertex2iv                 = 	logVertex2iv                 ;
		qglVertex2s                  = 	logVertex2s                  ;
		qglVertex2sv                 = 	logVertex2sv                 ;
		qglVertex3d                  = 	logVertex3d                  ;
		qglVertex3dv                 = 	logVertex3dv                 ;
		qglVertex3f                  = 	logVertex3f                  ;
		qglVertex3fv                 = 	logVertex3fv                 ;
		qglVertex3i                  = 	logVertex3i                  ;
		qglVertex3iv                 = 	logVertex3iv                 ;
		qglVertex3s                  = 	logVertex3s                  ;
		qglVertex3sv                 = 	logVertex3sv                 ;
		qglVertex4d                  = 	logVertex4d                  ;
		qglVertex4dv                 = 	logVertex4dv                 ;
		qglVertex4f                  = 	logVertex4f                  ;
		qglVertex4fv                 = 	logVertex4fv                 ;
		qglVertex4i                  = 	logVertex4i                  ;
		qglVertex4iv                 = 	logVertex4iv                 ;
		qglVertex4s                  = 	logVertex4s                  ;
		qglVertex4sv                 = 	logVertex4sv                 ;
		qglVertexPointer             = 	logVertexPointer             ;
		qglViewport                  = 	logViewport                  ;
	}
	else
	{
		qglAccum                     = dllAccum;
		qglAlphaFunc                 = dllAlphaFunc;
		qglAreTexturesResident       = dllAreTexturesResident;
		qglArrayElement              = dllArrayElement;
		qglBegin                     = dllBegin;
		qglBindTexture               = dllBindTexture;
		qglBitmap                    = dllBitmap;
		qglBlendFunc                 = dllBlendFunc;
		qglCallList                  = dllCallList;
		qglCallLists                 = dllCallLists;
		qglClear                     = dllClear;
		qglClearAccum                = dllClearAccum;
		qglClearColor                = dllClearColor;
		qglClearDepth                = dllClearDepth;
		qglClearIndex                = dllClearIndex;
		qglClearStencil              = dllClearStencil;
		qglClipPlane                 = dllClipPlane;
		qglColor3b                   = dllColor3b;
		qglColor3bv                  = dllColor3bv;
		qglColor3d                   = dllColor3d;
		qglColor3dv                  = dllColor3dv;
		qglColor3f                   = dllColor3f;
		qglColor3fv                  = dllColor3fv;
		qglColor3i                   = dllColor3i;
		qglColor3iv                  = dllColor3iv;
		qglColor3s                   = dllColor3s;
		qglColor3sv                  = dllColor3sv;
		qglColor3ub                  = dllColor3ub;
		qglColor3ubv                 = dllColor3ubv;
		qglColor3ui                  = dllColor3ui;
		qglColor3uiv                 = dllColor3uiv;
		qglColor3us                  = dllColor3us;
		qglColor3usv                 = dllColor3usv;
		qglColor4b                   = dllColor4b;
		qglColor4bv                  = dllColor4bv;
		qglColor4d                   = dllColor4d;
		qglColor4dv                  = dllColor4dv;
		qglColor4f                   = dllColor4f;
		qglColor4fv                  = dllColor4fv;
		qglColor4i                   = dllColor4i;
		qglColor4iv                  = dllColor4iv;
		qglColor4s                   = dllColor4s;
		qglColor4sv                  = dllColor4sv;
		qglColor4ub                  = dllColor4ub;
		qglColor4ubv                 = dllColor4ubv;
		qglColor4ui                  = dllColor4ui;
		qglColor4uiv                 = dllColor4uiv;
		qglColor4us                  = dllColor4us;
		qglColor4usv                 = dllColor4usv;
		qglColorMask                 = dllColorMask;
		qglColorMaterial             = dllColorMaterial;
		qglColorPointer              = dllColorPointer;
		qglCopyPixels                = dllCopyPixels;
		qglCopyTexImage1D            = dllCopyTexImage1D;
		qglCopyTexImage2D            = dllCopyTexImage2D;
		qglCopyTexSubImage1D         = dllCopyTexSubImage1D;
		qglCopyTexSubImage2D         = dllCopyTexSubImage2D;
		qglCullFace                  = dllCullFace;
		qglDeleteLists               = dllDeleteLists ;
		qglDeleteTextures            = dllDeleteTextures ;
		qglDepthFunc                 = dllDepthFunc ;
		qglDepthMask                 = dllDepthMask ;
		qglDepthRange                = dllDepthRange ;
		qglDisable                   = dllDisable ;
		qglDisableClientState        = dllDisableClientState ;
		qglDrawArrays                = dllDrawArrays ;
		qglDrawBuffer                = dllDrawBuffer ;
		qglDrawElements              = dllDrawElements ;
		qglDrawPixels                = dllDrawPixels ;
		qglEdgeFlag                  = dllEdgeFlag ;
		qglEdgeFlagPointer           = dllEdgeFlagPointer ;
		qglEdgeFlagv                 = dllEdgeFlagv ;
		qglEnable                    = 	dllEnable                    ;
		qglEnableClientState         = 	dllEnableClientState         ;
		qglEnd                       = 	dllEnd                       ;
		qglEndList                   = 	dllEndList                   ;
		qglEvalCoord1d				 = 	dllEvalCoord1d				 ;
		qglEvalCoord1dv              = 	dllEvalCoord1dv              ;
		qglEvalCoord1f               = 	dllEvalCoord1f               ;
		qglEvalCoord1fv              = 	dllEvalCoord1fv              ;
		qglEvalCoord2d               = 	dllEvalCoord2d               ;
		qglEvalCoord2dv              = 	dllEvalCoord2dv              ;
		qglEvalCoord2f               = 	dllEvalCoord2f               ;
		qglEvalCoord2fv              = 	dllEvalCoord2fv              ;
		qglEvalMesh1                 = 	dllEvalMesh1                 ;
		qglEvalMesh2                 = 	dllEvalMesh2                 ;
		qglEvalPoint1                = 	dllEvalPoint1                ;
		qglEvalPoint2                = 	dllEvalPoint2                ;
		qglFeedbackBuffer            = 	dllFeedbackBuffer            ;
		qglFinish                    = 	dllFinish                    ;
		qglFlush                     = 	dllFlush                     ;
		qglFogf                      = 	dllFogf                      ;
		qglFogfv                     = 	dllFogfv                     ;
		qglFogi                      = 	dllFogi                      ;
		qglFogiv                     = 	dllFogiv                     ;
		qglFrontFace                 = 	dllFrontFace                 ;
		qglFrustum                   = 	dllFrustum                   ;
		qglGenLists                  = 	dllGenLists                  ;
		qglGenTextures               = 	dllGenTextures               ;
		qglGetBooleanv               = 	dllGetBooleanv               ;
		qglGetClipPlane              = 	dllGetClipPlane              ;
		qglGetDoublev                = 	dllGetDoublev                ;
		qglGetError                  = 	dllGetError                  ;
		qglGetFloatv                 = 	dllGetFloatv                 ;
		qglGetIntegerv               = 	dllGetIntegerv               ;
		qglGetLightfv                = 	dllGetLightfv                ;
		qglGetLightiv                = 	dllGetLightiv                ;
		qglGetMapdv                  = 	dllGetMapdv                  ;
		qglGetMapfv                  = 	dllGetMapfv                  ;
		qglGetMapiv                  = 	dllGetMapiv                  ;
		qglGetMaterialfv             = 	dllGetMaterialfv             ;
		qglGetMaterialiv             = 	dllGetMaterialiv             ;
		qglGetPixelMapfv             = 	dllGetPixelMapfv             ;
		qglGetPixelMapuiv            = 	dllGetPixelMapuiv            ;
		qglGetPixelMapusv            = 	dllGetPixelMapusv            ;
		qglGetPointerv               = 	dllGetPointerv               ;
		qglGetPolygonStipple         = 	dllGetPolygonStipple         ;
		qglGetString                 = 	dllGetString                 ;
		qglGetTexEnvfv               = 	dllGetTexEnvfv               ;
		qglGetTexEnviv               = 	dllGetTexEnviv               ;
		qglGetTexGendv               = 	dllGetTexGendv               ;
		qglGetTexGenfv               = 	dllGetTexGenfv               ;
		qglGetTexGeniv               = 	dllGetTexGeniv               ;
		qglGetTexImage               = 	dllGetTexImage               ;
		qglGetTexLevelParameterfv    = 	dllGetTexLevelParameterfv    ;
		qglGetTexLevelParameteriv    = 	dllGetTexLevelParameteriv    ;
		qglGetTexParameterfv         = 	dllGetTexParameterfv         ;
		qglGetTexParameteriv         = 	dllGetTexParameteriv         ;
		qglHint                      = 	dllHint                      ;
		qglIndexMask                 = 	dllIndexMask                 ;
		qglIndexPointer              = 	dllIndexPointer              ;
		qglIndexd                    = 	dllIndexd                    ;
		qglIndexdv                   = 	dllIndexdv                   ;
		qglIndexf                    = 	dllIndexf                    ;
		qglIndexfv                   = 	dllIndexfv                   ;
		qglIndexi                    = 	dllIndexi                    ;
		qglIndexiv                   = 	dllIndexiv                   ;
		qglIndexs                    = 	dllIndexs                    ;
		qglIndexsv                   = 	dllIndexsv                   ;
		qglIndexub                   = 	dllIndexub                   ;
		qglIndexubv                  = 	dllIndexubv                  ;
		qglInitNames                 = 	dllInitNames                 ;
		qglInterleavedArrays         = 	dllInterleavedArrays         ;
		qglIsEnabled                 = 	dllIsEnabled                 ;
		qglIsList                    = 	dllIsList                    ;
		qglIsTexture                 = 	dllIsTexture                 ;
		qglLightModelf               = 	dllLightModelf               ;
		qglLightModelfv              = 	dllLightModelfv              ;
		qglLightModeli               = 	dllLightModeli               ;
		qglLightModeliv              = 	dllLightModeliv              ;
		qglLightf                    = 	dllLightf                    ;
		qglLightfv                   = 	dllLightfv                   ;
		qglLighti                    = 	dllLighti                    ;
		qglLightiv                   = 	dllLightiv                   ;
		qglLineStipple               = 	dllLineStipple               ;
		qglLineWidth                 = 	dllLineWidth                 ;
		qglListBase                  = 	dllListBase                  ;
		qglLoadIdentity              = 	dllLoadIdentity              ;
		qglLoadMatrixd               = 	dllLoadMatrixd               ;
		qglLoadMatrixf               = 	dllLoadMatrixf               ;
		qglLoadName                  = 	dllLoadName                  ;
		qglLogicOp                   = 	dllLogicOp                   ;
		qglMap1d                     = 	dllMap1d                     ;
		qglMap1f                     = 	dllMap1f                     ;
		qglMap2d                     = 	dllMap2d                     ;
		qglMap2f                     = 	dllMap2f                     ;
		qglMapGrid1d                 = 	dllMapGrid1d                 ;
		qglMapGrid1f                 = 	dllMapGrid1f                 ;
		qglMapGrid2d                 = 	dllMapGrid2d                 ;
		qglMapGrid2f                 = 	dllMapGrid2f                 ;
		qglMaterialf                 = 	dllMaterialf                 ;
		qglMaterialfv                = 	dllMaterialfv                ;
		qglMateriali                 = 	dllMateriali                 ;
		qglMaterialiv                = 	dllMaterialiv                ;
		qglMatrixMode                = 	dllMatrixMode                ;
		qglMultMatrixd               = 	dllMultMatrixd               ;
		qglMultMatrixf               = 	dllMultMatrixf               ;
		qglNewList                   = 	dllNewList                   ;
		qglNormal3b                  = 	dllNormal3b                  ;
		qglNormal3bv                 = 	dllNormal3bv                 ;
		qglNormal3d                  = 	dllNormal3d                  ;
		qglNormal3dv                 = 	dllNormal3dv                 ;
		qglNormal3f                  = 	dllNormal3f                  ;
		qglNormal3fv                 = 	dllNormal3fv                 ;
		qglNormal3i                  = 	dllNormal3i                  ;
		qglNormal3iv                 = 	dllNormal3iv                 ;
		qglNormal3s                  = 	dllNormal3s                  ;
		qglNormal3sv                 = 	dllNormal3sv                 ;
		qglNormalPointer             = 	dllNormalPointer             ;
		qglOrtho                     = 	dllOrtho                     ;
		qglPassThrough               = 	dllPassThrough               ;
		qglPixelMapfv                = 	dllPixelMapfv                ;
		qglPixelMapuiv               = 	dllPixelMapuiv               ;
		qglPixelMapusv               = 	dllPixelMapusv               ;
		qglPixelStoref               = 	dllPixelStoref               ;
		qglPixelStorei               = 	dllPixelStorei               ;
		qglPixelTransferf            = 	dllPixelTransferf            ;
		qglPixelTransferi            = 	dllPixelTransferi            ;
		qglPixelZoom                 = 	dllPixelZoom                 ;
		qglPointSize                 = 	dllPointSize                 ;
		qglPolygonMode               = 	dllPolygonMode               ;
		qglPolygonOffset             = 	dllPolygonOffset             ;
		qglPolygonStipple            = 	dllPolygonStipple            ;
		qglPopAttrib                 = 	dllPopAttrib                 ;
		qglPopClientAttrib           = 	dllPopClientAttrib           ;
		qglPopMatrix                 = 	dllPopMatrix                 ;
		qglPopName                   = 	dllPopName                   ;
		qglPrioritizeTextures        = 	dllPrioritizeTextures        ;
		qglPushAttrib                = 	dllPushAttrib                ;
		qglPushClientAttrib          = 	dllPushClientAttrib          ;
		qglPushMatrix                = 	dllPushMatrix                ;
		qglPushName                  = 	dllPushName                  ;
		qglRasterPos2d               = 	dllRasterPos2d               ;
		qglRasterPos2dv              = 	dllRasterPos2dv              ;
		qglRasterPos2f               = 	dllRasterPos2f               ;
		qglRasterPos2fv              = 	dllRasterPos2fv              ;
		qglRasterPos2i               = 	dllRasterPos2i               ;
		qglRasterPos2iv              = 	dllRasterPos2iv              ;
		qglRasterPos2s               = 	dllRasterPos2s               ;
		qglRasterPos2sv              = 	dllRasterPos2sv              ;
		qglRasterPos3d               = 	dllRasterPos3d               ;
		qglRasterPos3dv              = 	dllRasterPos3dv              ;
		qglRasterPos3f               = 	dllRasterPos3f               ;
		qglRasterPos3fv              = 	dllRasterPos3fv              ;
		qglRasterPos3i               = 	dllRasterPos3i               ;
		qglRasterPos3iv              = 	dllRasterPos3iv              ;
		qglRasterPos3s               = 	dllRasterPos3s               ;
		qglRasterPos3sv              = 	dllRasterPos3sv              ;
		qglRasterPos4d               = 	dllRasterPos4d               ;
		qglRasterPos4dv              = 	dllRasterPos4dv              ;
		qglRasterPos4f               = 	dllRasterPos4f               ;
		qglRasterPos4fv              = 	dllRasterPos4fv              ;
		qglRasterPos4i               = 	dllRasterPos4i               ;
		qglRasterPos4iv              = 	dllRasterPos4iv              ;
		qglRasterPos4s               = 	dllRasterPos4s               ;
		qglRasterPos4sv              = 	dllRasterPos4sv              ;
		qglReadBuffer                = 	dllReadBuffer                ;
		qglReadPixels                = 	dllReadPixels                ;
		qglRectd                     = 	dllRectd                     ;
		qglRectdv                    = 	dllRectdv                    ;
		qglRectf                     = 	dllRectf                     ;
		qglRectfv                    = 	dllRectfv                    ;
		qglRecti                     = 	dllRecti                     ;
		qglRectiv                    = 	dllRectiv                    ;
		qglRects                     = 	dllRects                     ;
		qglRectsv                    = 	dllRectsv                    ;
		qglRenderMode                = 	dllRenderMode                ;
		qglRotated                   = 	dllRotated                   ;
		qglRotatef                   = 	dllRotatef                   ;
		qglScaled                    = 	dllScaled                    ;
		qglScalef                    = 	dllScalef                    ;
		qglScissor                   = 	dllScissor                   ;
		qglSelectBuffer              = 	dllSelectBuffer              ;
		qglShadeModel                = 	dllShadeModel                ;
		qglStencilFunc               = 	dllStencilFunc               ;
		qglStencilMask               = 	dllStencilMask               ;
		qglStencilOp                 = 	dllStencilOp                 ;
		qglTexCoord1d                = 	dllTexCoord1d                ;
		qglTexCoord1dv               = 	dllTexCoord1dv               ;
		qglTexCoord1f                = 	dllTexCoord1f                ;
		qglTexCoord1fv               = 	dllTexCoord1fv               ;
		qglTexCoord1i                = 	dllTexCoord1i                ;
		qglTexCoord1iv               = 	dllTexCoord1iv               ;
		qglTexCoord1s                = 	dllTexCoord1s                ;
		qglTexCoord1sv               = 	dllTexCoord1sv               ;
		qglTexCoord2d                = 	dllTexCoord2d                ;
		qglTexCoord2dv               = 	dllTexCoord2dv               ;
		qglTexCoord2f                = 	dllTexCoord2f                ;
		qglTexCoord2fv               = 	dllTexCoord2fv               ;
		qglTexCoord2i                = 	dllTexCoord2i                ;
		qglTexCoord2iv               = 	dllTexCoord2iv               ;
		qglTexCoord2s                = 	dllTexCoord2s                ;
		qglTexCoord2sv               = 	dllTexCoord2sv               ;
		qglTexCoord3d                = 	dllTexCoord3d                ;
		qglTexCoord3dv               = 	dllTexCoord3dv               ;
		qglTexCoord3f                = 	dllTexCoord3f                ;
		qglTexCoord3fv               = 	dllTexCoord3fv               ;
		qglTexCoord3i                = 	dllTexCoord3i                ;
		qglTexCoord3iv               = 	dllTexCoord3iv               ;
		qglTexCoord3s                = 	dllTexCoord3s                ;
		qglTexCoord3sv               = 	dllTexCoord3sv               ;
		qglTexCoord4d                = 	dllTexCoord4d                ;
		qglTexCoord4dv               = 	dllTexCoord4dv               ;
		qglTexCoord4f                = 	dllTexCoord4f                ;
		qglTexCoord4fv               = 	dllTexCoord4fv               ;
		qglTexCoord4i                = 	dllTexCoord4i                ;
		qglTexCoord4iv               = 	dllTexCoord4iv               ;
		qglTexCoord4s                = 	dllTexCoord4s                ;
		qglTexCoord4sv               = 	dllTexCoord4sv               ;
		qglTexCoordPointer           = 	dllTexCoordPointer           ;
		qglTexEnvf                   = 	dllTexEnvf                   ;
		qglTexEnvfv                  = 	dllTexEnvfv                  ;
		qglTexEnvi                   = 	dllTexEnvi                   ;
		qglTexEnviv                  = 	dllTexEnviv                  ;
		qglTexGend                   = 	dllTexGend                   ;
		qglTexGendv                  = 	dllTexGendv                  ;
		qglTexGenf                   = 	dllTexGenf                   ;
		qglTexGenfv                  = 	dllTexGenfv                  ;
		qglTexGeni                   = 	dllTexGeni                   ;
		qglTexGeniv                  = 	dllTexGeniv                  ;
		qglTexImage1D                = 	dllTexImage1D                ;
		qglTexImage2D                = 	dllTexImage2D                ;
		qglTexParameterf             = 	dllTexParameterf             ;
		qglTexParameterfv            = 	dllTexParameterfv            ;
		qglTexParameteri             = 	dllTexParameteri             ;
		qglTexParameteriv            = 	dllTexParameteriv            ;
		qglTexSubImage1D             = 	dllTexSubImage1D             ;
		qglTexSubImage2D             = 	dllTexSubImage2D             ;
		qglTranslated                = 	dllTranslated                ;
		qglTranslatef                = 	dllTranslatef                ;
		qglVertex2d                  = 	dllVertex2d                  ;
		qglVertex2dv                 = 	dllVertex2dv                 ;
		qglVertex2f                  = 	dllVertex2f                  ;
		qglVertex2fv                 = 	dllVertex2fv                 ;
		qglVertex2i                  = 	dllVertex2i                  ;
		qglVertex2iv                 = 	dllVertex2iv                 ;
		qglVertex2s                  = 	dllVertex2s                  ;
		qglVertex2sv                 = 	dllVertex2sv                 ;
		qglVertex3d                  = 	dllVertex3d                  ;
		qglVertex3dv                 = 	dllVertex3dv                 ;
		qglVertex3f                  = 	dllVertex3f                  ;
		qglVertex3fv                 = 	dllVertex3fv                 ;
		qglVertex3i                  = 	dllVertex3i                  ;
		qglVertex3iv                 = 	dllVertex3iv                 ;
		qglVertex3s                  = 	dllVertex3s                  ;
		qglVertex3sv                 = 	dllVertex3sv                 ;
		qglVertex4d                  = 	dllVertex4d                  ;
		qglVertex4dv                 = 	dllVertex4dv                 ;
		qglVertex4f                  = 	dllVertex4f                  ;
		qglVertex4fv                 = 	dllVertex4fv                 ;
		qglVertex4i                  = 	dllVertex4i                  ;
		qglVertex4iv                 = 	dllVertex4iv                 ;
		qglVertex4s                  = 	dllVertex4s                  ;
		qglVertex4sv                 = 	dllVertex4sv                 ;
		qglVertexPointer             = 	dllVertexPointer             ;
		qglViewport                  = 	dllViewport                  ;
	}
}


void GLimp_LogNewFrame (void)
{
	fprintf(glw_state.log_fp, "*** R_BeginFrame ***\n");
}

#pragma warning(default : 4113 4133 4047)



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

// === jitfog
void Draw_String (int x, int y, const char *str);
vec3_t fogcolor = { 0.408, 0.447, 0.584 };
float fogdensity = 0.008;
float fogdistance = 512;
qboolean fogenabled = false; // doesn't work right with overbright :(
// jit ===

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
cvar_t  *cl_crosshairscale;
cvar_t	*gl_round_down;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_skyedge; // jitsky
cvar_t	*gl_showtris;
cvar_t	*gl_ztrick;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_flashblend;
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
				R_DrawSpriteModel(currententity);
				break;
			default:
				assert(0);
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
				assert(0);
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
void GL_DrawParticles (int num_particles, const particle_t particles[], const unsigned colortable[768])
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
	if (gl_ext_pointparameters->value && qglPointParameterfEXT )
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
	static float autobright = 0.0f;
	vec3_t shadelight;
	float shadeavg;
	float b;
	static int lasttime = 0;
	float timediff;
	int time;

	time = Sys_Milliseconds();
	timediff = min(0.9f, (time - lasttime) / 1000.0f);
	lasttime = time;

	if (gl_brightness->value || v_blend[3])
	{
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_ENABLE_BLEND
		qglDisable(GL_DEPTH_TEST);
		qglDisable(GL_TEXTURE_2D);
		qglLoadIdentity();

		// FIXME: get rid of these
		qglRotatef(-90, 1, 0, 0);	    // put Z going up
		qglRotatef(90,  0, 0, 1);	    // put Z going up
	}

	if (gl_brightness->value)
	{
		if (gl_autobrightness->value > 1.0f || gl_autobrightness->value < 0.0f)
			ri.Cvar_SetValue("gl_autobrightness", 1.0f);

		R_LightPoint(r_newrefdef.vieworg, shadelight);
		shadeavg = max(0.0f, min(1.0f, (shadelight[0] + shadelight[1] + shadelight[2]) / 1.5f - 0.2f));
		autobright = (1.0f - timediff) * autobright + timediff * (1.0f - sqrt(shadeavg));
		qglBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
		b = gl_brightness->value * (1.0f - gl_autobrightness->value +
			gl_autobrightness->value * autobright) / 2.0f + 0.5f; // value needs to be between 0.5 and 1.0
		qglColor4f(b, b, b, 1);
		//ri.Con_Printf(PRINT_ALL, "%g  %g  %g\n", shadeavg, autobright, timediff);

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

	for (j = 0; j < 3; j++)
		if (out->normal[j] < 0)
			bits |= 1 << j;

	return bits;
}


void R_SetFrustum (void)
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

	for (i = 0; i < 4; i++)
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
		MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4 * 74 / max(r_newrefdef.fov_y, 40), fogdistance + 128); // jitfov
	else
		MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4 * 74 / max(r_newrefdef.fov_y, 40), 15000/*4096*/);  //jitfov

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

		Com_sprintf(s, sizeof(s), "%c%c%4i wpoly %c%c%4i epoly %c9%i tex %i lmaps\n",
			CHAR_COLOR, c_brush_polys > 1024 ? c_brush_polys > 2048 ? c_brush_polys > 4096 ? 'A' : 'C' : 'E' : '9',
			c_brush_polys,
			CHAR_COLOR, c_alias_polys > 2048 ? c_alias_polys > 4096 ? c_alias_polys > 8192 ? 'A' : 'C' : 'E' : '9',
			c_alias_polys,
			CHAR_COLOR,
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
#ifdef QUAKE2
	gl_brightness = ri.Cvar_Get("gl_brightness", "1", CVAR_ARCHIVE); // jit
#else
	gl_brightness = ri.Cvar_Get("gl_brightness", "0", CVAR_ARCHIVE); // jit
#endif
	gl_autobrightness = ri.Cvar_Get("gl_autobrightness", "1", CVAR_ARCHIVE); // jit
	gl_showbbox = ri.Cvar_Get("gl_showbbox", "0", 0);  // jit / Guy
//	gl_modulate = ri.Cvar_Get("gl_modulate", "1.6", CVAR_ARCHIVE); // jit, default to 1.6
	gl_lightmapgamma = ri.Cvar_Get("gl_lightmapgamma", ".6", CVAR_ARCHIVE); // jitgamma
	gl_textshadow = ri.Cvar_Get("gl_textshadow", "1", CVAR_ARCHIVE); // jittext
	gl_hash_textures = ri.Cvar_Get("gl_hash_textures", "1", CVAR_ARCHIVE); // jithash
	gl_free_unused_textures = ri.Cvar_Get("gl_free_unused_textures", "1", CVAR_ARCHIVE); // jitfreeunused
	cl_hudscale = ri.Cvar_Get("cl_hudscale", "2", CVAR_ARCHIVE); // jithudscale
	cl_crosshairscale = ri.Cvar_Get("cl_crosshairscale", "2", CVAR_ARCHIVE); // viciouz - crosshair scale
	gl_log = ri.Cvar_Get("gl_log", "0", 0);
	gl_bitdepth = ri.Cvar_Get("gl_bitdepth", "0", 0);
	gl_mode = ri.Cvar_Get("gl_mode", "3", CVAR_ARCHIVE);
	gl_lightmap = ri.Cvar_Get("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get("gl_shadows", "0", CVAR_ARCHIVE);
	gl_dynamic = ri.Cvar_Get("gl_dynamic", "1", 0);
	gl_round_down = ri.Cvar_Get("gl_round_down", "0", 0); // jit, was 1
	gl_picmip = ri.Cvar_Get("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get("gl_skymip", "0", 0);
	gl_skyedge = ri.Cvar_Get("gl_skyedge", "0", 0); // jitsky
	gl_showtris = ri.Cvar_Get("gl_showtris", "0", 0);
	gl_ztrick = ri.Cvar_Get("gl_ztrick", "0", 0);
	gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = ri.Cvar_Get("gl_clear", "0", 0);
	gl_cull = ri.Cvar_Get("gl_cull", "1", 0);
	gl_flashblend = ri.Cvar_Get("gl_flashblend", "0", 0);
	gl_monolightmap = ri.Cvar_Get("gl_monolightmap", "0", 0);
	gl_driver = ri.Cvar_Get("gl_driver", GL_DRIVER_LIB, CVAR_ARCHIVE);
	gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE); // jit
	gl_texturealphamode = ri.Cvar_Get("gl_texturealphamode", "default", CVAR_ARCHIVE);
	gl_texturesolidmode = ri.Cvar_Get("gl_texturesolidmode", "default", CVAR_ARCHIVE);
	gl_lockpvs = ri.Cvar_Get("gl_lockpvs", "0", 0);
	gl_vertex_arrays = ri.Cvar_Get("gl_vertex_arrays", "0", CVAR_ARCHIVE);
	gl_ext_multitexture = ri.Cvar_Get("gl_ext_multitexture", "1", CVAR_ARCHIVE);
	gl_ext_pointparameters = ri.Cvar_Get("gl_ext_pointparameters", "0", CVAR_ARCHIVE); // disabled this as it doesn't play nice with some drivers
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
		ri.Con_Printf(PRINT_ALL, "R_SetMode() - CDS not allowed with this driver.\n");
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
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - Fullscreen unavailable in this mode.\n");

			if ((err = GLimp_SetMode(&vid.width, &vid.height, gl_mode->value, false)) == rserr_ok)
				return true;
		}
		else if (err == rserr_invalid_mode)
		{
			ri.Cvar_SetValue("gl_mode", gl_state.prev_mode);
			gl_mode->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - Invalid mode.\n");
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode(&vid.width, &vid.height, gl_state.prev_mode, false)) != rserr_ok)
		{
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - Could not revert to safe mode.\n");
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

void R_Init3dfxGamma (void)
{
	char envbuffer[1024];
	float g;

	// update 3Dfx gamma irrespective of underlying DLL
	g = 2.0f * (0.8f - (vid_gamma->value - 0.5f)) + 1.0F;
	Com_sprintf(envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g);
	putenv(envbuffer);
	Com_sprintf(envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g);
	putenv(envbuffer);
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
		r_turbsin[j] *= 0.5;

	ri.Con_Printf(PRINT_ALL, "ref_gl version: "REF_VERSION"\n");
	Draw_GetPalette();
	R_Register();

	// initialize our QGL dynamic bindings
	if (!QGL_Init(gl_driver->string))
	{
		ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - Could not load \"%s\".\n", gl_driver->string);

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
		else if (!Q_streq(gl_driver->string, GL_DRIVER_LIB))
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
        ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - Could not R_SetMode().\n");
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
	gl_config.version = atof(gl_config.version_string);
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
	else if (strstr (vendor_buffer, "ati "))
	{
		gl_config.renderer = GL_RENDERER_ATI;
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...ATI card workarounds will be used.\n");
	}
	else
		gl_config.renderer = GL_RENDERER_OTHER;

	if (toupper(gl_monolightmap->string[1]) != 'F')
	{
		if (gl_config.renderer == GL_RENDERER_PERMEDIA2)
		{
			ri.Cvar_Set("gl_monolightmap", "A");
			ri.Con_Printf(PRINT_ALL, "...using gl_monolightmap 'a'.\n");
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

#ifdef __unix__
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
			ri.Con_Printf(PRINT_ALL, "...allowing CDS.\n");
		else
			ri.Con_Printf(PRINT_ALL, "...disabling CDS.\n");
	}

	/*
	** grab extensions
	*/
	if (strstr(gl_config.extensions_string, "GL_EXT_compiled_vertex_array") ||
		 strstr(gl_config.extensions_string, "GL_SGI_compiled_vertex_array"))
	{
		if (gl_debug->value)// jit
			ri.Con_Printf(PRINT_ALL, "...enabling GL_EXT_compiled_vertex_array.\n");

		qglLockArraysEXT =(void*)qwglGetProcAddress("glLockArraysEXT");
		qglUnlockArraysEXT =(void*)qwglGetProcAddress("glUnlockArraysEXT");
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...GL_EXT_compiled_vertex_array not found.\n");
	}

#ifdef _WIN32
	if (strstr(gl_config.extensions_string, "WGL_EXT_swap_control"))
	{
		qwglSwapIntervalEXT =(BOOL(WINAPI*)(int))qwglGetProcAddress("wglSwapIntervalEXT");

		if (gl_debug->value) // jit
			ri.Con_Printf(PRINT_ALL, "...enabling WGL_EXT_swap_control.\n");
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...WGL_EXT_swap_control not found.\n");
	}
#endif

	if (strstr(gl_config.extensions_string, "GL_EXT_point_parameters"))
	{
		if (gl_ext_pointparameters->value && (gl_config.renderer != GL_RENDERER_ATI)) // Workaround for ATI driver bug.
		{
			qglPointParameterfEXT = (void(APIENTRY*)(GLenum, GLfloat))qwglGetProcAddress("glPointParameterfEXT");
			qglPointParameterfvEXT = (void(APIENTRY*)(GLenum, const GLfloat*))qwglGetProcAddress("glPointParameterfvEXT");

			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_EXT_point_parameters.\n");
		}
		else if (gl_debug->value) // jit
		{
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_EXT_point_parameters.\n");
		}
	}
	else if (gl_debug->value) // jit
	{
		ri.Con_Printf(PRINT_ALL, "...GL_EXT_point_parameters not found.\n");
	}

	if (strstr(gl_config.extensions_string, "GL_ARB_multitexture"))
	{
		if (gl_ext_multitexture->value)
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_ARB_multitexture.\n");

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
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_ARB_multitexture.\n");
		}
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...GL_ARB_multitexture not found.\n");
	}

	if (strstr(gl_config.extensions_string, "GL_SGIS_multitexture"))
	{
		if (qglActiveTextureARB)
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...GL_SGIS_multitexture deprecated in favor of ARB_multitexture.\n");
		}
		else if (gl_ext_multitexture->value)
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_SGIS_multitexture.\n");

			qglMultiTexCoord2fARB = (void*)qwglGetProcAddress("glMTexCoord2fSGIS");
			qglSelectTextureSGIS = (void*)qwglGetProcAddress("glSelectTextureSGIS");
			QGL_TEXTURE0 = GL_TEXTURE0_SGIS;
			QGL_TEXTURE1 = GL_TEXTURE1_SGIS;
		}
		else if (gl_debug->value) // jit
		{
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_SGIS_multitexture.\n");
		}
	}
	else if (gl_debug->value)
	{
		ri.Con_Printf(PRINT_ALL, "...GL_SGIS_multitexture not found.\n");
	}

	// ===
	// jitspoe - these texture shaders screw up geforce4 cards... killing it.

	// Texture Shader support - MrG
/*	if (strstr(gl_config.extensions_string, "GL_NV_texture_shader"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_NV_texture_shader.\n");
		gl_state.texshaders=true;
	} else {
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_NV_texture_shader not found.\n");
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
				ri.Con_Printf(PRINT_ALL, "...using GL_SGIS_generate_mipmap.\n");

			gl_state.sgis_mipmap = true;
		}
		else
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...GL_SGIS_generate_mipmap found but disabled.\n");

			gl_state.sgis_mipmap = false;
		}
	}
	else
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_SGIS_generate_mipmap not found.\n");

		gl_state.sgis_mipmap = false;
	}

	if (strstr(gl_config.extensions_string, "GL_NV_texture_rectangle"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_NV_texture_rectangle.\n");

		gl_state.tex_rectangle = GL_TEXTURE_RECTANGLE_NV; // jitblur
	}
	else if (strstr (gl_config.extensions_string, "GL_EXT_texture_rectangle"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_EXT_texture_rectangle.\n");

		gl_state.tex_rectangle = GL_TEXTURE_RECTANGLE_NV; // jitodo(jitblur)
	}
	else
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_EXT_texture_rectangle not found.\n");

		gl_state.tex_rectangle = 0; // jitblur
	}

	// Heffo - ARB Texture Compression
	if (strstr(gl_config.extensions_string, "GL_ARB_texture_compression"))
	{
		if (!gl_ext_texture_compression->value)
		{
			if (gl_debug->value) // jit
				ri.Con_Printf(PRINT_ALL, "...ignoring GL_ARB_texture_compression.\n");

			gl_state.texture_compression = false;
		}
		else
		{
			if (gl_debug->value)
				ri.Con_Printf(PRINT_ALL, "...using GL_ARB_texture_compression.\n");

			gl_state.texture_compression = true;
		}
	}
	else
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_ARB_texture_compression not found.\n");

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
			ri.Con_Printf(PRINT_ALL, "...using GL_ARB_texture_env_combine.\n");
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
			ri.Con_Printf(PRINT_ALL, "...GL_ARB_fragment_program disabled.\n");
	}
	else if (strstr(gl_config.extensions_string, "GL_ARB_fragment_program"))
	{
		gl_state.fragment_program = true;

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_ARB_fragment_program.\n");

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
				ri.Con_Printf(PRINT_ALL, "... Failed! Fragment programs disabled.\n");
		}
	}
	else
	{
		gl_state.fragment_program = false;

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_ARB_fragment_program not found.\n");
	}
	// jitwater ===

	// === jitwater - FBO extensions
	if (strstr(gl_config.extensions_string, "GL_EXT_framebuffer_object"))
	{
		qglGenFramebuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)qwglGetProcAddress("glGenFramebuffersEXT");
		qglBindFramebufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)qwglGetProcAddress("glBindFramebufferEXT");
		qglGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)qwglGetProcAddress("glGenRenderbuffersEXT");
		qglBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)qwglGetProcAddress("glBindRenderbufferEXT");
		qglRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)qwglGetProcAddress("glRenderbufferStorageEXT");
		qglFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)qwglGetProcAddress("glFramebufferRenderbufferEXT");
		qglDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)qwglGetProcAddress("glDeleteFramebuffersEXT");
		qglDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)qwglGetProcAddress("glDeleteRenderbuffersEXT");
	}
	else
	{
		gl_state.fbo = false;

		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...GL_EXT_framebuffer_object not found.\n");
	}

	if (strstr(gl_config.extensions_string, "GL_NV_register_combiners"))
	{
		if (gl_debug->value)
			ri.Con_Printf(PRINT_ALL, "...using GL_NV_register_combiners.\n");

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
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_NV_register_combiners.\n");
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
void R_Shutdown (void)
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	Mod_FreeAll();

	GL_ShutdownImages();
	RS_FreeAllScripts(); // jitrscript

	if (char_colors)
	{
		free(char_colors); // jittext
		char_colors = NULL;
	}

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

	if (VectorNormalizeRetLen(normalized_direction) == 0)
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


int Draw_GetIntVarByID (int id)
{
	switch (id)
	{
	case VARID_GL_INSTANCE:
#ifdef WIN32
		return (int)glw_state.hinstOpenGL;
#else
		return (int)glw_state.OpenGLLib;
#endif
	default:
		assert(0);
		return 0;
	}

	return 0;
}

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
	re.DrawGetIntVarByID = Draw_GetIntVarByID;
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
	int			len;

	va_start(argptr, error);
	len = _vsnprintf(text, sizeof(text), error, argptr); // jitsecurity -- prevent buffer overruns
	va_end(argptr);
	NULLTERMINATE(text); // jitsecurity -- make sure string is null terminated.
	assert(len < sizeof(text));
	ri.Sys_Error(ERR_FATAL, "%s", text);
}

void Com_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	int			len;

	va_start(argptr, fmt);
	len = _vsnprintf(text, sizeof(text), fmt, argptr); // jitsecurity -- prevent buffer overruns
	va_end(argptr);
	NULLTERMINATE(text); // jitsecurity -- make sure string is null terminated.
	assert(len < sizeof(text));
	ri.Con_Printf(PRINT_ALL, "%s", text);
}

#endif
