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

#include "client.h"

image_t		*i_conback;
//image_t		*i_inventory;
image_t		*i_net;
image_t		*i_pause;
image_t		*i_loading;
image_t		*i_backtile;
// jitmenu:
image_t		*i_slider1l;
image_t		*i_slider1r;
image_t		*i_slider1lh;
image_t		*i_slider1rh;
image_t		*i_slider1ls;
image_t		*i_slider1rs;
image_t		*i_slider1t;
image_t		*i_slider1th;
image_t		*i_slider1b;
image_t		*i_slider1bh;
image_t		*i_slider1bs;
image_t		*i_checkbox1u;
image_t		*i_checkbox1uh;
image_t		*i_checkbox1us;
image_t		*i_checkbox1c;
image_t		*i_checkbox1ch;
image_t		*i_checkbox1cs;
image_t		*i_field1l;
image_t		*i_field1lh;
image_t		*i_field1ls;
image_t		*i_field1m;
image_t		*i_field1mh;
image_t		*i_field1ms;
image_t		*i_field1r;
image_t		*i_field1rh;
image_t		*i_field1rs;
image_t		*i_cursor;
image_t		*i_cursor_text;

void CL_InitImages()
{
	i_conback = re.DrawFindPic("conback");
	//i_inventory = re.DrawFindPic("inventory");
	i_net = re.DrawFindPic("net");
	i_pause = re.DrawFindPic("pause");
	i_loading = re.DrawFindPic("loading");
	i_backtile = re.DrawFindPic("backtile");

	// jitmenu:
	i_slider1l = re.DrawFindPic("slider1l");
	i_slider1lh = re.DrawFindPic("slider1lh");
	i_slider1ls = re.DrawFindPic("slider1ls");
	i_slider1r = re.DrawFindPic("slider1r");
	i_slider1rh = re.DrawFindPic("slider1rh");
	i_slider1rs = re.DrawFindPic("slider1rs");
	i_slider1t = re.DrawFindPic("slider1t");
	i_slider1th = re.DrawFindPic("slider1th");
	i_slider1b = re.DrawFindPic("slider1b");
	i_slider1bh = re.DrawFindPic("slider1bh");
	i_slider1bs = re.DrawFindPic("slider1bs");
	i_checkbox1u = re.DrawFindPic("checkbox1u");
	i_checkbox1uh = re.DrawFindPic("checkbox1uh");
	i_checkbox1us = re.DrawFindPic("checkbox1us");
	i_checkbox1c = re.DrawFindPic("checkbox1c");
	i_checkbox1ch = re.DrawFindPic("checkbox1ch");
	i_checkbox1cs = re.DrawFindPic("checkbox1cs");
	i_field1l  = re.DrawFindPic("field1l");
	i_field1lh = re.DrawFindPic("field1lh");
	i_field1ls = re.DrawFindPic("field1ls");
	i_field1m  = re.DrawFindPic("field1m");
	i_field1mh = re.DrawFindPic("field1mh");
	i_field1ms = re.DrawFindPic("field1ms");
	i_field1r  = re.DrawFindPic("field1r");
	i_field1rh = re.DrawFindPic("field1rh");
	i_field1rs = re.DrawFindPic("field1rs");

	i_cursor = re.DrawFindPic("cursor");
	i_cursor_text = re.DrawFindPic("cursor_text");
}
