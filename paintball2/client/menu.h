/*
Copyright (C) 2003 Nathan Wulf

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

// ===[
// jitmenu

#ifndef _MENU_H_
#define _MENU_H_

#include "client.h"
#include "qmenu.h"

typedef enum {
	WIDGET_TYPE_UNKNOWN		= 0,
	WIDGET_TYPE_BUTTON		= 1,
	WIDGET_TYPE_SLIDER		= 2,
	WIDGET_TYPE_CHECKBOX	= 3,
	WIDGET_TYPE_DROPDOWN	= 4,
	WIDGET_TYPE_TEXT		= 5
} WIDGET_TYPE;

typedef enum {
	WIDGET_HALIGN_LEFT		= 0,
	WIDGET_HALIGN_CENTER	= 1,
	WIDGET_HALIGN_RIGHT		= 2,
} WIDGET_HALIGN;

typedef enum {
	WIDGET_VALIGN_TOP		= 0,
	WIDGET_VALIGN_MIDDLE	= 1,
	WIDGET_VALIGN_BOTTOM	= 2,
} WIDGET_VALIGN;

#ifndef _WINDEF_
typedef struct _POINT {
	int x;
	int y;
} POINT;

typedef struct _RECT {
	int bottom;
	int right;
	int left;
	int top;
} RECT;
#endif

typedef enum {
	M_ACTION_NONE = 0,
	M_ACTION_SELECT = 1,
	M_ACTION_HILIGHT = 2
} MENU_ACTION;

typedef struct MENU_WIDGET_S {
	WIDGET_TYPE type;
	char *command;		// command executed when widget activated
	char *cvar;			// cvar widget reads and/or modifies
	int x;				// position from 0 (left) to 320 (right)
	int y;				// position from 0 (top) to 240 (bottom)
	WIDGET_HALIGN halign;	// horizontal alignment relative to x, y coords
	WIDGET_VALIGN valign;	// vertical alignment relative to x, y coords
	char *text;			// text displayed by widget
	char *hovertext;	// text when mouse over widget
// todo: should probably revert back to text names because vid_restart breaks pics...
	image_t *pic;		// image displayed by widget
 	int picwidth;		// width to scale image to
	int picheight;		// height to scale image to
	image_t *hoverpic;	// image displayed when mouse over widget
	int hoverpicwidth;
	int hoverpicheight;
	qboolean enabled;	// for greying out widgets
	qboolean hover;		// mouse is over widget
	qboolean selected;	// widget has 'focus'
// Drawing Information
	POINT picCorner;
	POINT picSize;
	POINT textCorner;
	POINT textSize;
	RECT mouseBoundaries;
	struct MENU_WIDGET_S *next;
} menu_widget_t;

typedef struct MENU_SCREEN_S {
	char *name;
	menu_widget_t *widget;
	struct MENU_SCREEN_S *next;
} menu_screen_t;

extern cvar_t *cl_hudscale;

#endif

// ]===

