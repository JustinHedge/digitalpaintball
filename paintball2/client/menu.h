/*
Copyright (C) 2003-2004 Nathan Wulf (jitspoe)

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

#ifdef WIN32
///* ACT
// For socket stuff
#include "winsock.h"

// From net_wins.c
int NET_TCPSocket (int port);	
int NET_TCPConnect(SOCKET sockfd, char *net_remote_address, int port);
char *NET_ErrorString (void);

#endif

extern cvar_t *serverlist_source; // jitserverlist

#define TEXT_WIDTH_UNSCALED		8
#define TEXT_HEIGHT_UNSCALED	8

#define TEXT_WIDTH		(TEXT_WIDTH_UNSCALED*scale)
#define TEXT_HEIGHT		(TEXT_HEIGHT_UNSCALED*scale)

#define SLIDER_BUTTON_WIDTH_UNSCALED	8
#define SLIDER_BUTTON_HEIGHT_UNSCALED	8
#define SLIDER_TRAY_WIDTH_UNSCALED		32
#define SLIDER_TRAY_HEIGHT_UNSCALED		8
#define SLIDER_KNOB_WIDTH_UNSCALED		8
#define SLIDER_KNOB_HEIGHT_UNSCALED		8
#define SLIDER_TOTAL_WIDTH_UNSCALED		((SLIDER_BUTTON_WIDTH_UNSCALED*2)+SLIDER_TRAY_WIDTH_UNSCALED)
#define SLIDER_TOTAL_HEIGHT_UNSCALED	8

#define SLIDER_BUTTON_WIDTH		(SLIDER_BUTTON_WIDTH_UNSCALED	*scale)
#define SLIDER_BUTTON_HEIGHT	(SLIDER_BUTTON_HEIGHT_UNSCALED	*scale)
#define SLIDER_TRAY_WIDTH		(SLIDER_TRAY_WIDTH_UNSCALED		*scale)
#define SLIDER_TRAY_HEIGHT		(SLIDER_TRAY_HEIGHT_UNSCALED	*scale)
#define SLIDER_KNOB_WIDTH		(SLIDER_KNOB_WIDTH_UNSCALED		*scale)
#define SLIDER_KNOB_HEIGHT		(SLIDER_KNOB_HEIGHT_UNSCALED	*scale)
#define SLIDER_TOTAL_WIDTH		(SLIDER_TOTAL_WIDTH_UNSCALED	*scale)
#define SLIDER_TOTAL_HEIGHT		(SLIDER_TOTAL_HEIGHT_UNSCALED	*scale)

#define CHECKBOX_WIDTH_UNSCALED			8
#define CHECKBOX_HEIGHT_UNSCALED		8

#define CHECKBOX_WIDTH			(CHECKBOX_WIDTH_UNSCALED	*scale)
#define CHECKBOX_HEIGHT			(CHECKBOX_HEIGHT_UNSCALED	*scale)

#define CURSOR_HEIGHT	(16*scale)
#define CURSOR_WIDTH	(16*scale)

#define FIELD_HEIGHT	(12*scale)
#define FIELD_LWIDTH	(10*scale)
#define FIELD_RWIDTH	(10*scale)

#define SELECT_HSPACING_UNSCALED	2
#define SELECT_VSPACING_UNSCALED	2

#define SELECT_HSPACING		(SELECT_HSPACING_UNSCALED*scale)
#define SELECT_VSPACING		(SELECT_VSPACING_UNSCALED*scale)

#define SELECT_BACKGROUND_SIZE		4

#define SCROLL_ARROW_WIDTH_UNSCALED	8
#define SCROLL_ARROW_HEIGHT_UNSCALED 8



// widget flags
#define WIDGET_FLAG_NONE		0
#define WIDGET_FLAG_USEMAP		2
#define WIDGET_FLAG_VSCROLLBAR	4
#define WIDGET_FLAG_HSCROLLBAR	8
#define WIDGET_FLAG_DROPPEDDOWN	16
#define WIDGET_FLAG_NOAPPLY		32 // don't apply changes immediately (jitodo)
#define WIDGET_FLAG_FLOAT		64
#define WIDGET_FLAG_INT			128
#define WIDGET_FLAG_FILELIST	256
#define WIDGET_FLAG_SERVERLIST	512
#define WIDGET_FLAG_NOBG		1024 // no background for select widget
#define WIDGET_FLAG_BIND		2048 // key binding
//#define WIDGET_FLAG_ACTIVEBIND	4096 // next button pressed goes to the bind of this widget

typedef enum {
	WIDGET_TYPE_UNKNOWN	= 0,
	WIDGET_TYPE_BUTTON,
	WIDGET_TYPE_SLIDER,
	WIDGET_TYPE_CHECKBOX,
	WIDGET_TYPE_SELECT,
	WIDGET_TYPE_TEXT,
	WIDGET_TYPE_PIC,
	WIDGET_TYPE_FIELD
} WIDGET_TYPE;

// Horizontal alignment of widget
typedef enum {
	WIDGET_HALIGN_LEFT		= 0,
	WIDGET_HALIGN_CENTER	= 1,
	WIDGET_HALIGN_RIGHT		= 2
} WIDGET_HALIGN;

// Vertical alignment of widget
typedef enum {
	WIDGET_VALIGN_TOP		= 0,
	WIDGET_VALIGN_MIDDLE	= 1,
	WIDGET_VALIGN_BOTTOM	= 2
} WIDGET_VALIGN;

typedef enum {
	SLIDER_SELECTED_NONE = 0,
	SLIDER_SELECTED_TRAY,
	SLIDER_SELECTED_LEFTARROW,
	SLIDER_SELECTED_RIGHTARROW,
	SLIDER_SELECTED_KNOB
} SLIDER_SELECTED;

//#ifndef _WINDEF_
typedef struct POINT_S {
	int x;
	int y;
} point_t;

typedef struct RECT_S {
	int bottom;
	int right;
	int left;
	int top;
} rect_t;
//#endif

typedef struct MENU_MOUSE_S {
	int x;
	int y;
	image_t *cursorpic;
} menu_mouse_t;

typedef enum {
	M_ACTION_NONE = 0,
	M_ACTION_HILIGHT,
	M_ACTION_SELECT,
	M_ACTION_EXECUTE,
	M_ACTION_DOUBLECLICK,
	M_ACTION_SCROLLUP,
	M_ACTION_SCROLLDOWN
} MENU_ACTION;

typedef struct SELECT_MAP_LIST_S {
	char *cvar_string;
	char *string;
	struct SELECT_MAP_LIST_S *next;
} select_map_list_t;


typedef struct MENU_WIDGET_S {
	WIDGET_TYPE type;
	int flags;			// for things like numbersonly
	qboolean modified;	// drawing information changed
	char *command;		// command executed when widget activated
	char *doubleclick;	// command exceuted on double click.
	char *cvar;			// cvar widget reads and/or modifies
	char *cvar_default;	// set cvar to this value if unset
	int x;				// position from 0 (left) to 320 (right)
	int y;				// position from 0 (top) to 240 (bottom)
	WIDGET_HALIGN halign;	// horizontal alignment relative to x, y coords
	WIDGET_VALIGN valign;	// vertical alignment relative to x, y coords
	char *text;			// text displayed by widget
	char *hovertext;	// text when mouse over widget
	char *selectedtext;	// text when mouse clicked on widget
	image_t *pic;		// image displayed by widget
	
	image_t *hoverpic;	// image displayed when mouse over widget
	int hoverpicwidth;
	int hoverpicheight;
	image_t *selectedpic;// image displayed when mouse clicked on widget
	qboolean enabled;	// for greying out widgets
	qboolean hover;		// mouse is over widget
	qboolean selected;	// widget has 'focus'
	SLIDER_SELECTED slider_hover; // which part of the slider is the mouse over
	SLIDER_SELECTED slider_selected; // which part of the slider is the mouse clicked on?

	float	slider_min;
	float	slider_max;
	float	slider_inc;

	union {
		int	field_cursorpos;
		int select_pos;
		float slider_pos;
	};
	union {
		int	field_width;
		int	select_width;
		int picwidth;		// width to scale image to
	};
	union {
		int	field_start;
		int select_vstart; // start point for vertical scroll
	};
	union {
		int		select_rows;
		int picheight;		// height to scale image to
	};
	int		select_totalitems;
	int		select_hstart; // start point for horizontal scroll.
	char	**select_list;
	char	**select_map;

	// Drawing Information
	point_t widgetCorner;
	point_t widgetSize;
	point_t textCorner;
	point_t textSize;
	rect_t mouseBoundaries;

	// callback function
	void (*callback)(struct MENU_WIDGET_S *widget);
	void (*callback_doubleclick)(struct MENU_WIDGET_S *widget);

	struct MENU_WIDGET_S *parent;
	struct MENU_WIDGET_S *subwidget; // child
	struct MENU_WIDGET_S *next;
} menu_widget_t;

typedef struct MENU_SCREEN_S {
	char *name;
	menu_widget_t *widget;
	menu_widget_t *selected_widget;
	menu_widget_t *hover_widget;
	struct MENU_SCREEN_S *next;
} menu_screen_t;

/*typedef struct SERVERLIST_NODE_S {
	netadr_t adr;
	char *info;
	// jitodo
	// ...
	struct SERVERLIST_NODE_S *next;
} serverlist_node_t;*/

typedef struct M_SERVERLIST_SERVER_S {
	netadr_t adr;
	char *servername;
	char *mapname;
	int players;
	int maxplayers;
	int ping;
	int ping_request_time; // time (ms) at which ping was requested.
} m_serverlist_server_t;

typedef struct M_SERVERLIST_S {
	int numservers;
	int actualsize;
	char **ips;
	char **info;
	m_serverlist_server_t *server;
	//serverlist_node_t *list;
} m_serverlist_t;

extern cvar_t *cl_hudscale;

#define MENU_SOUND_OPEN S_StartLocalSound("misc/menu1.wav");
#define MENU_SOUND_SELECT S_StartLocalSound("misc/menu2.wav");
#define MENU_SOUND_CLOSE S_StartLocalSound("misc/menu3.wav");
#define MENU_SOUND_SLIDER S_StartLocalSound("misc/menu4.wav");

#endif

// ]===

