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
#include "../client/client.h"
#include "../client/qmenu.h"

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *scr_viewsize;

static cvar_t *gl_mode;
static cvar_t *gl_driver;
static cvar_t *gl_picmip;
static cvar_t *gl_finish;
cvar_t *vid_gamma_hw;

extern void M_ForceMenuOff( void );
void M_Banner( char *name ); // jit

/*
====================================================================

MENU INTERACTION

====================================================================
*/
static menuframework_s	s_opengl_menu;
static menuframework_s *s_current_menu;
static int				s_current_menu_index;

static menulist_s		s_mode_list;
static menuslider_s		s_tq_slider;
static menuslider_s		s_screensize_slider;
static menuslider_s		s_brightness_slider;
static menulist_s  		s_fs_box;
static menulist_s  		s_texcompress_box;
static menulist_s  		s_overbright_box;
static menulist_s		s_hardware_gamma;
static menulist_s  		s_finish_box;
static menuaction_s		s_apply_action;
static menuaction_s		s_defaults_action;

static void DriverCallback( void *unused )
{
	s_current_menu = &s_opengl_menu;
	s_current_menu_index = 1;
}

static void ScreenSizeCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "viewsize", slider->curvalue * 10 );
}

static void BrightnessCallback( void *s )
{
	float gamma;
	gamma = ( 0.8 - ( s_brightness_slider.curvalue/10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
}

static void ResetDefaults( void *unused )
{
	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
	float gamma;


	/*
	** invert sense so greater = brighter, and scale to a range of 0.5 to 1.3
	*/
	gamma = ( 0.8 - ( s_brightness_slider.curvalue/10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
	Cvar_SetValue( "vid_gamma_hw", s_hardware_gamma.curvalue);
	Cvar_SetValue( "gl_ext_texture_compression", s_texcompress_box.curvalue);
	Cvar_SetValue( "gl_overbright", s_overbright_box.curvalue);
	Cvar_SetValue( "gl_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValue( "vid_fullscreen", s_fs_box.curvalue );
	//Cvar_SetValue( "gl_finish", s_finish_box.curvalue );
	Cvar_SetValue( "gl_swapinterval", s_finish_box.curvalue ); // jitvsync
	Cvar_SetValue( "gl_mode", s_mode_list.curvalue );

	Cvar_Set( "vid_ref", "pbgl" ); // jitodo, enable software support again

	/*
	** update appropriate stuff if we're running OpenGL and gamma
	** has been modified
	*/
		if ( vid_gamma->modified )
			vid_ref->modified = true;

		if ( gl_driver->modified )
			vid_ref->modified = true;

		if (vid_gamma_hw->modified)
			vid_ref->modified = true;

	M_ForceMenuOff();
}

static void CancelChanges( void *unused )
{
	extern void M_PopMenu( void );

	M_PopMenu();
}

/*
** VID_MenuInit
*/
void VID_MenuInit( void )
{
	static const char *resolutions[] = 
	{
		"[320 240  ]",
		"[400 300  ]",
		"[512 384  ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 960 ]",
		"[1600 1200]",
		"[2048 1536]",
		0
	};
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	
	vid_gamma_hw = Cvar_Get("vid_gamma_hw","0",CVAR_ARCHIVE);
	
	if ( !gl_driver )
		gl_driver = Cvar_Get( "gl_driver", "opengl32", CVAR_ARCHIVE );
	if ( !gl_picmip )
		gl_picmip = Cvar_Get( "gl_picmip", "0", 0 );
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "3", CVAR_ARCHIVE );
	if ( !gl_finish )
		gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );
	
	s_mode_list.curvalue = gl_mode->value;
	
	if ( !scr_viewsize )
		scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);
	
	s_screensize_slider.curvalue = scr_viewsize->value/10;
	
	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;
	
	s_mode_list.generic.type = MTYPE_SPINCONTROL;
	s_mode_list.generic.name = "video mode";
	s_mode_list.generic.x    = 0;
	s_mode_list.generic.y    = 10;
	s_mode_list.itemnames    = resolutions;
	
	s_screensize_slider.generic.type     = MTYPE_SLIDER;
	s_screensize_slider.generic.x        = 0;
	s_screensize_slider.generic.y        = 20;
	s_screensize_slider.generic.name     = "screen size";
	s_screensize_slider.minvalue         = 3;
	s_screensize_slider.maxvalue         = 12;
	s_screensize_slider.generic.callback = ScreenSizeCallback;
	
	s_fs_box.generic.type               = MTYPE_SPINCONTROL;
	s_fs_box.generic.x                  = 0;
	s_fs_box.generic.y                  = 30;
	s_fs_box.generic.name               = "fullscreen";
	s_fs_box.itemnames                  = yesno_names;
	s_fs_box.curvalue                   = vid_fullscreen->value;
	
	s_tq_slider.generic.type            = MTYPE_SLIDER;
	s_tq_slider.generic.x               = 0;
	s_tq_slider.generic.y               = 50;
	s_tq_slider.generic.name            = "texture quality";
	s_tq_slider.minvalue                = 0;
	s_tq_slider.maxvalue                = 3;
	s_tq_slider.curvalue                = 3-gl_picmip->value;
	
	s_texcompress_box.generic.type		= MTYPE_SPINCONTROL;
	s_texcompress_box.generic.x			= 0;
	s_texcompress_box.generic.y			= 60;
	s_texcompress_box.generic.name		= "texture compression";
	s_texcompress_box.itemnames			= yesno_names;
	s_texcompress_box.curvalue			= Cvar_VariableValue("gl_ext_texture_compression");

	s_overbright_box.generic.type		= MTYPE_SPINCONTROL;
	s_overbright_box.generic.x			= 0;
	s_overbright_box.generic.y			= 70;
	s_overbright_box.generic.name		= "overbright";
	s_overbright_box.itemnames			= yesno_names;
	s_overbright_box.curvalue			= Cvar_VariableValue("gl_overbright");
	
	s_hardware_gamma.generic.type		= MTYPE_SPINCONTROL;
	s_hardware_gamma.generic.x			= 0;
	s_hardware_gamma.generic.y			= 80;
	s_hardware_gamma.generic.name		= "hardware gamma (may crash)";
	s_hardware_gamma.itemnames			= yesno_names;
	s_hardware_gamma.curvalue           = vid_gamma_hw->value;
	
	s_brightness_slider.generic.type     = MTYPE_SLIDER;
	s_brightness_slider.generic.x        = 0;
	s_brightness_slider.generic.y        = 90;
	s_brightness_slider.generic.name     = "brightness";
	s_brightness_slider.generic.callback = BrightnessCallback;
	s_brightness_slider.minvalue         = 5;
	s_brightness_slider.maxvalue         = 13;
	s_brightness_slider.curvalue         = ( 1.3 - vid_gamma->value + 0.5 ) * 10;

	s_finish_box.generic.type            = MTYPE_SPINCONTROL;
	s_finish_box.generic.x               = 0;
	s_finish_box.generic.y               = 100;
	s_finish_box.generic.name            = "vertical sync"; // jitvsync
	s_finish_box.curvalue                = Cvar_Get("gl_swapinterval", "1", CVAR_ARCHIVE)->value; // jitvsync
	s_finish_box.itemnames               = yesno_names;

	s_defaults_action.generic.type       = MTYPE_ACTION;
	s_defaults_action.generic.name       = "reset to defaults";
	s_defaults_action.generic.x          = 0;
	s_defaults_action.generic.y          = 120;
	s_defaults_action.generic.callback   = ResetDefaults;
	
	s_apply_action.generic.type          = MTYPE_ACTION;
	s_apply_action.generic.name          = "Apply Changes";
	s_apply_action.generic.x             = 0;
	s_apply_action.generic.y             = 130;
	s_apply_action.generic.callback      = ApplyChanges;

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_screensize_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fs_box);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_tq_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_texcompress_box);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_overbright_box);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_hardware_gamma);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider); // jit, moved
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_finish_box);
	
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_defaults_action);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_apply_action);
	
	Menu_Center( &s_opengl_menu );
	s_opengl_menu.x -= 8;
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	//int w, h;

	s_current_menu = &s_opengl_menu;

	/*
	** draw the banner
	*/
	/*re.DrawGetPicSize( &w, &h, "m_banner_video" );
	re.DrawPic( (int)(viddef.width *0.5 - w *0.5), (int)(viddef.height *0.5 - 110), "m_banner_video" );*/

	M_Banner( "m_banner_video" ); // jit

	/*
	** move cursor to a reasonable starting position
	*/
	Menu_AdjustCursor( s_current_menu, 1 );

	/*
	** draw the menu
	*/
	Menu_Draw( s_current_menu );
}

/*
================
VID_MenuKey
================
*/
const char *VID_MenuKey( int key )
{
	menuframework_s *m = s_current_menu;
	static const char *sound = "misc/menu1.wav";

	switch ( key )
	{
	case K_ESCAPE:
		CancelChanges(NULL);
		return NULL;
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor( m, -1 );
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem( m, -1 );
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem( m, 1 );
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if ( !Menu_SelectItem( m ) )
			ApplyChanges( NULL );
		break;
	}

	return sound;
}


