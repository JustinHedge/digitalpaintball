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

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "menu.h"

#define MAX_MENU_SCREENS 32

int				m_menudepth	= 0;
menu_screen_t	*root_menu	= NULL;
menu_screen_t	*m_menu_screens[MAX_MENU_SCREENS];

void M_ForceMenuOff (void)
{
//	m_drawfunc = 0;
//	m_keyfunc = 0;
	m_menudepth = 0;

	cls.key_dest = key_game;
	Key_ClearStates ();
	Cvar_Set ("paused", "0");
}

void M_Menu_Main_f (void)
{
	Cbuf_AddText("menu main\n");
}

/*
   strlen_noformat
   Returns the length of strings ignoring formatting (underline, etc)
*/
int strlen_noformat(const char *s)
{
	int count = 0;

	while(*s)
	{
		if(*(s+1) && (*s == CHAR_UNDERLINE || *s == CHAR_ITALICS))
		{
			// don't count character
		}
		else if(*(s+1) && *s == CHAR_COLOR)
		{
			s++; // skip two characters.
		}
		else
			count++;

		s++;
	}

	return count;
}
// ++ ARTHUR [9/04/03]
void M_UpdateDrawingInformation (menu_widget_t *widget)
{
	int scale;
	char *text = NULL;
	image_t *pic = NULL;

	scale = cl_hudscale->value;
	widget->picCorner.x = widget->textCorner.x = widget->x * scale;
	widget->picCorner.y = widget->textCorner.y = widget->y * scale;

	if(widget->enabled)
	{
		if(widget->text)
			text = widget->text;

		if(widget->pic)
			pic = widget->pic;

		//
		// Update text/pic for hovering/selection
		//
		if(widget->hover || widget->selected)
		{
			if(widget->hovertext)
				text = widget->hovertext;
			if(widget->hoverpic)
				pic = widget->hoverpic;
			else if(widget->pic)
				pic = widget->pic;
		}

		//
		// Calculate picture size
		//
		if (pic) 
		{
			widget->picSize.x = (widget->picwidth) ? widget->picwidth : pic->width;
			widget->picSize.y = (widget->picheight) ? widget->picheight : pic->height;
			if (pic == widget->hoverpic)
			{
				if (widget->hoverpicwidth)
					widget->picSize.x = widget->hoverpicwidth;

				if (widget->hoverpicheight)
					widget->picSize.y = widget->hoverpicheight;			
			}
			widget->picSize.x *= scale;
			widget->picSize.y *= scale;
		}

		switch(widget->halign)
		{
		case WIDGET_HALIGN_CENTER:
			widget->picCorner.x -= widget->picSize.x/2;
			widget->textCorner.x -= (strlen_noformat(text)*8*scale)/2;
			break;
		case WIDGET_HALIGN_RIGHT:
			widget->picCorner.x -= widget->picSize.x;
			widget->textCorner.x -= (strlen_noformat(text)*8*scale);
			break;
		default:
			break;
		}

		switch(widget->valign)
		{
		case WIDGET_VALIGN_MIDDLE:
			widget->picCorner.y -= (widget->picSize.y/2);
			widget->textCorner.y -= (4*scale);
			break;
		case WIDGET_VALIGN_BOTTOM:
			widget->picCorner.y -= widget->picSize.y;
			widget->textCorner.y -= (8*scale);
			break;
		default:
			break;
		}

		widget->mouseBoundaries.bottom = -0x0FFFFFFF;
		widget->mouseBoundaries.left = 0x0FFFFFFF;
		widget->mouseBoundaries.right = -0x0FFFFFFF;
		widget->mouseBoundaries.top = 0x0FFFFFFF;
		if (pic) 
		{
			widget->mouseBoundaries.left = widget->picCorner.x;
			widget->mouseBoundaries.right = widget->picCorner.x + widget->picSize.x;
			widget->mouseBoundaries.top = widget->picCorner.y;
			widget->mouseBoundaries.bottom = widget->picCorner.y + widget->picSize.y;
		}

		if (text)
		{
			widget->textSize.x = (strlen_noformat(text)*8*scale);
			widget->textSize.y = (8*scale);

			if (widget->mouseBoundaries.left > widget->textCorner.x)
				widget->mouseBoundaries.left = widget->textCorner.x;
			if (widget->mouseBoundaries.right < widget->textCorner.x + widget->textSize.x)
				widget->mouseBoundaries.right = widget->textCorner.x + widget->textSize.x;
			if (widget->mouseBoundaries.top > widget->textCorner.y)
				widget->mouseBoundaries.top = widget->textCorner.y;
			if (widget->mouseBoundaries.bottom < widget->textCorner.y + widget->textSize.y)
				widget->mouseBoundaries.bottom = widget->textCorner.y + widget->textSize.y;
			
		}
	}
}

menu_widget_t* M_FindNextEntryUnderCursor(menu_screen_t* menu, MENU_ACTION action) 
{
	POINT current_pos;
	int	mouse_x, mouse_y;
	menu_widget_t* current = menu->widget;
	menu_widget_t* selected = NULL;
	qboolean selectNext = false;

	if (!q_get_cursor_pos(&current_pos.x,&current_pos.y))
		return NULL;

	mouse_x = current_pos.x;
	mouse_y = current_pos.y;

	while (current) 
	{
		M_UpdateDrawingInformation(current);
		if (action == M_ACTION_SELECT)
			current->selected = false;
		if (action == M_ACTION_SELECT || action == M_ACTION_HILIGHT)
			current->hover = false;

		if ((current->cvar || current->command) &&
			mouse_x > current->mouseBoundaries.left &&
			mouse_x < current->mouseBoundaries.right &&
			mouse_y < current->mouseBoundaries.bottom &&
			mouse_y > current->mouseBoundaries.top)
		{
			current->hover = true;
			
			if (selectNext == true)
			{
				selected = current;
				selectNext = false;
			}

			//
			// If our mouse is over a currently selected item, and we click, we want to cycle
			// to the next lower item the mouse is over, so turn on "select next".
			//
			if (current->selected)
			{
				selectNext = true;
			}

			//
			// Only select this one if we havent found a valid selection already.
			//
			if (selected == NULL)
				selected = current;
		}
		current = current->next;
	}
	return selected;
}



qboolean M_MouseAction(menu_screen_t* menu, MENU_ACTION action)
{
	menu_widget_t* newSelection=NULL;

	if (!menu)
		return false;

	// If we're actually over something
	if (NULL != (newSelection = M_FindNextEntryUnderCursor(menu, action))) 
	{
		switch (action)
		{
		case M_ACTION_SELECT:
			newSelection->selected = true;
			break;
		case M_ACTION_HILIGHT:
			newSelection->hover = true;
			break;
		case M_ACTION_NONE:
		default:
			break;
		}
		
	}		

	return true;
}

void M_Keydown (int key)
{
/*	const char *s;

	if (m_keyfunc)
		if ((s = m_keyfunc(key)) != 0 )
			S_StartLocalSound(s);
*/
	switch (key) 
	{
		case K_MOUSE1:
			M_MouseAction(m_menu_screens[m_menudepth-1], M_ACTION_SELECT);
			key = K_ENTER;
		case K_MOUSEMOVE:
			M_MouseAction(m_menu_screens[m_menudepth-1], M_ACTION_HILIGHT);
			break;
	};
			
//*/
}
// -- ARTHUR


void M_PopMenu (void)
{
/*	S_StartLocalSound( menu_out_sound );
	if (m_menudepth < 1)
		Com_Error (ERR_FATAL, "M_PopMenu: depth < 1");
	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;

	if (!m_menudepth)
		M_ForceMenuOff ();
*/
}

void M_AddToServerList (netadr_t adr, char *info)
{
/*	int		i;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;
	while ( *info == ' ' )
		info++;

	// ignore if duplicated
	for (i=0 ; i<m_num_servers ; i++)
		if (!strcmp(info, local_server_names[i]))
			return;

	local_server_netadr[m_num_servers] = adr;
	strncpy (local_server_names[m_num_servers], info, sizeof(local_server_names[0])-1);
	m_num_servers++;*/
}

menu_widget_t* M_GetNewBlankMenuWidget()
{
	menu_widget_t *widget;

	widget = Z_Malloc(sizeof(menu_widget_t)); // todo: should prolly free this at some point...
	memset(widget, 0, sizeof(menu_widget_t));

	widget->enabled = true;

	return widget;
}

menu_widget_t* M_GetNewMenuWidget(int type, const char *text, const char *cvar, 
								  const char *cmd, int x, int y, qboolean enabled)
{
	menu_widget_t *widget;
	int len;

	widget = M_GetNewBlankMenuWidget();

	widget->type = type;
	if(text)
	{
		len = sizeof(char) * (strlen(text) + 1);
		widget->text = Z_Malloc(len);
		memcpy(widget->text, text, len);
	}
	if(cvar)
	{
		len = sizeof(char) * (strlen(cvar) + 1);
		widget->cvar = Z_Malloc(len);
		memcpy(widget->cvar, cvar, len);
	}
	if(cmd)
	{
		len = sizeof(char) * (strlen(cmd) + 1);
		widget->command = Z_Malloc(len);
		memcpy(widget->command, cmd, len);
	}
	widget->x = x;
	widget->y = y;
	widget->enabled = enabled;

	return widget;
}

menu_screen_t* M_GetNewMenuScreen(const char *menu_name)
{
	menu_screen_t* menu;
	int len;

	len = strlen(menu_name) + 1;
	
	menu = Z_Malloc(sizeof(menu_screen_t));
	memset(menu, 0, sizeof(menu_screen_t));
	menu->name = Z_Malloc(sizeof(char)*len);
	memcpy(menu->name, menu_name, sizeof(char)*len);

	menu->next = root_menu;
	root_menu = menu;

	return menu;
}

menu_screen_t* M_ErrorMenu(const char *text)
{
	menu_screen_t *menu;
	
	menu = M_GetNewMenuScreen(text);
	menu->widget = M_GetNewMenuWidget(WIDGET_TYPE_TEXT, text,
			NULL, NULL, 60, 120, true);

	return menu;
}

int M_WidgetGetType(const char *s)
{
	if(!strcmp(s, "text"))
		return WIDGET_TYPE_TEXT;
	if(!strcmp(s, "button"))
		return WIDGET_TYPE_BUTTON;
	if(!strcmp(s, "slider"))
		return WIDGET_TYPE_SLIDER;
	if(!strcmp(s, "checkbox"))
		return WIDGET_TYPE_CHECKBOX;
	if(!strcmp(s, "dropdown"))
		return WIDGET_TYPE_DROPDOWN;

	return WIDGET_TYPE_UNKNOWN;
}

int M_WidgetGetAlign(const char *s)
{
	if(!strcmp(s, "left"))
		return WIDGET_HALIGN_LEFT;
	if(!strcmp(s, "center"))
		return WIDGET_HALIGN_CENTER;
	if(!strcmp(s, "right"))
		return WIDGET_HALIGN_RIGHT;

	if(!strcmp(s, "top"))
		return WIDGET_VALIGN_TOP;
	if(!strcmp(s, "middle"))
		return WIDGET_VALIGN_MIDDLE;
	if(!strcmp(s, "bottom"))
		return WIDGET_VALIGN_BOTTOM;

	return WIDGET_HALIGN_LEFT; // default top/left
}


char *text_copy(const char *in)
{
	char *out;

	out = Z_Malloc(strlen(in)+1);
	strcpy(out, in);

	return out;
}

menu_screen_t* M_LoadMenuScreen(const char *menu_name)
{
	menu_screen_t *menu;
	char menu_filename[MAX_QPATH];
	char *buf;
	int file_len;

	sprintf(menu_filename, "menus/%s.txt", menu_name);
	file_len = FS_LoadFile (menu_filename, (void **)&buf);
	
	if(file_len != -1)
	{
		char *token;
		char *buf2;

		// put null terminator at end:
		buf2 = Z_Malloc(file_len+1);
		memcpy(buf2, buf, file_len);
		buf2[file_len] = '\0';
		FS_FreeFile(buf);
		buf = buf2;

		// check for header: "pb2menu 1"
		token = COM_Parse(&buf); // "pb2menu"
		if(strcmp(token, "pb2menu") == 0)
		{
			token = COM_Parse(&buf); // "1"
			if(atoi(token) == 1)
			{
// jitodo, probably should have different types of menus, like dialog boxes, etc.
				menu_widget_t *widget=NULL;
				int x = 0, y = 0;
				//qboolean enabled = true;


				menu = M_GetNewMenuScreen(menu_name);
				token = COM_Parse(&buf); 

				while(*token)
				{
					// new widget:
					if(strcmp(token, "widget") == 0)
					{
						if(!widget)
						{
							widget = menu->widget = M_GetNewBlankMenuWidget();
						}
						else
						{
							widget = widget->next = M_GetNewBlankMenuWidget();
						}
						widget->y = y;
						widget->x = x;
						//y += 8;
					}
					// widget properties:
					else if(strcmp(token, "type") == 0)
					{
						widget->type = M_WidgetGetType(COM_Parse(&buf));
						if(widget->type == WIDGET_TYPE_SLIDER)
						{
							widget->picwidth = 96;
							widget->picheight = 16;
						}
					}
					else if(strcmp(token, "text") == 0)
						widget->text = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "hovertext") == 0)
						widget->hovertext = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "cvar") == 0)
						widget->cvar = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "command") == 0)
						widget->command = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "pic") == 0)
						widget->pic = re.DrawFindPic(COM_Parse(&buf));
					else if(strcmp(token, "picwidth") == 0)
						widget->picwidth = atoi(COM_Parse(&buf));
					else if(strcmp(token, "picheight") == 0)
						widget->picheight = atoi(COM_Parse(&buf));
					else if(strcmp(token, "hoverpic") == 0)
						widget->hoverpic = re.DrawFindPic(COM_Parse(&buf));
					else if(strcmp(token, "hoverpicwidth") == 0)
						widget->hoverpicwidth = atoi(COM_Parse(&buf));
					else if(strcmp(token, "hoverpicheight") == 0)
						widget->hoverpicheight = atoi(COM_Parse(&buf));
					else if(strcmp(token, "xabs") == 0)
						x = widget->x = atoi(COM_Parse(&buf));
					else if(strcmp(token, "yabs") == 0)
						y = widget->y = atoi(COM_Parse(&buf));
					else if(strcmp(token, "xrel") == 0)
						x = widget->x += atoi(COM_Parse(&buf));
					else if(strcmp(token, "yrel") == 0)
						y = widget->y += atoi(COM_Parse(&buf));
					else if(strcmp(token, "halign") == 0)
						widget->halign = M_WidgetGetAlign(COM_Parse(&buf));
					else if(strcmp(token, "valign") == 0)
						widget->valign = M_WidgetGetAlign(COM_Parse(&buf));
					else if(strstr(token, "min"))
						widget->slider_min = atof(COM_Parse(&buf));
					else if(strstr(token, "max"))
						widget->slider_max = atof(COM_Parse(&buf));
					
					token = COM_Parse(&buf);
				}
			}
			else
				menu = M_ErrorMenu("Invalid menu version.");
		}
		else
			menu = M_ErrorMenu("Invalid menu file.");

		Z_Free(buf2);
	}
	else
	{
		char notfoundtext[MAX_QPATH+10];

		sprintf(notfoundtext, "%s not found.", menu_filename);
		return M_ErrorMenu(notfoundtext);
	}

	return menu;
}

menu_screen_t* M_FindMenuScreen(const char *menu_name)
{
	menu_screen_t *menu;

	menu = root_menu;

	// look through "cached" menus
	while(menu)
	{
		if(!strcmp(menu_name, menu->name))
			return menu;

		menu = menu->next;
	}

	// not found, load from file:
	return M_LoadMenuScreen(menu_name);
}

void M_PushMenuScreen(menu_screen_t *menu) // jitodo
{
	if(m_menudepth < MAX_MENU_SCREENS)
	{
		m_menu_screens[m_menudepth] = menu;
		m_menudepth++;
		cls.key_dest = key_menu;
	}
}

void M_Menu_f (void)
{
	menu_screen_t *menu;
	Com_Printf("Menu: %s (%d)\n", Cmd_Argv(1), m_menudepth);
	menu = M_FindMenuScreen(Cmd_Argv(1));

	M_PushMenuScreen(menu);
}

void M_Init (void)
{

	Cmd_AddCommand("menu", M_Menu_f);
}

// jitodo: reload menus on vid_restart
// M_Shutdown()

void M_DrawSlider(int x, int y, float pos, SLIDER_SELECTED slider_hover, SLIDER_SELECTED slider_selected)
{
	int scale;
	int xorig;

	xorig = x;

	scale = cl_hudscale->value;

	// left arrow:
	if(slider_hover == SLIDER_SELECTED_LEFTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1lh);
	else if(slider_selected == SLIDER_SELECTED_LEFTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1ls);
	else
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1l);
	x += SLIDER_BUTTON_WIDTH;

	// tray:
	if(slider_hover == SLIDER_SELECTED_TRAY)
		re.DrawStretchPic2(x, y, SLIDER_TRAY_WIDTH, SLIDER_TRAY_HEIGHT, i_slider1th);
	else
		re.DrawStretchPic2(x, y, SLIDER_TRAY_WIDTH, SLIDER_TRAY_HEIGHT, i_slider1t);
	x += SLIDER_TRAY_WIDTH;

	// right arrow
	if(slider_hover == SLIDER_SELECTED_RIGHTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1rh);
	else if(slider_selected == SLIDER_SELECTED_RIGHTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1rs);
	else
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1r);

	// knob:
	x = xorig + SLIDER_BUTTON_WIDTH + pos*(SLIDER_TRAY_WIDTH-SLIDER_KNOB_WIDTH);
	if(slider_hover == SLIDER_SELECTED_KNOB)
		re.DrawStretchPic2(x, y, SLIDER_KNOB_WIDTH, SLIDER_KNOB_HEIGHT, i_slider1bh);
	else if(slider_selected == SLIDER_SELECTED_KNOB)
		re.DrawStretchPic2(x, y, SLIDER_KNOB_WIDTH, SLIDER_KNOB_HEIGHT, i_slider1bs);
	else
		re.DrawStretchPic2(x, y, SLIDER_KNOB_WIDTH, SLIDER_KNOB_HEIGHT, i_slider1b);
}

float M_SliderGetPos(menu_widget_t *widget)
{
	float sliderdiff;
	float retval = 0.0f;

	sliderdiff = widget->slider_max - widget->slider_min;

	retval = Cvar_Get(widget->cvar, "0", CVAR_ARCHIVE)->value;

	if(sliderdiff)
		retval = (retval - widget->slider_min) / sliderdiff;

	if (retval > 1.0f)
		retval = 1.0f;
	if (retval < 0.0f)
		retval = 0.0f;

	return retval;
}

void M_DrawWidget (menu_widget_t *widget)
{
	char *text = NULL;
	image_t *pic = NULL;

	M_UpdateDrawingInformation(widget);

	if(widget->enabled)
	{
		if(widget->pic)
			pic = widget->pic;

		if(widget->text)
			text = widget->text;

		//
		// Update text/pic for hovering/selection
		//
		if(widget->hover || widget->selected)
		{
			if(widget->hovertext)
				text = widget->hovertext;
			else if(widget->text)
			{
				if (widget->selected)
					text = va("%c%c%c%s", CHAR_UNDERLINE, CHAR_COLOR, 'A', widget->text);
				else 
					text = va("%c%c%c%s", CHAR_UNDERLINE, CHAR_COLOR, 'D', widget->text);
			}

			if(widget->hoverpic)
				pic = widget->hoverpic;
			else if(widget->pic)
				pic = widget->pic;
		}

		switch(widget->type) 
		{
		case WIDGET_TYPE_SLIDER:
			M_DrawSlider(widget->picCorner.x, widget->picCorner.y, M_SliderGetPos(widget),
				widget->slider_hover, widget->slider_selected);
			break;
		default:
			break;
		}

		if(pic)
			re.DrawStretchPic2(widget->picCorner.x,
								widget->picCorner.y,
								widget->picSize.x,
								widget->picSize.y,
								pic);
		if(text)
			re.DrawString(widget->textCorner.x, widget->textCorner.y, text);
	}
}

void M_Draw (void)
{
	if(cls.key_dest == key_menu && m_menudepth)
	{
		menu_screen_t *menu;
		menu_widget_t *widget;
		
		menu = m_menu_screens[m_menudepth-1];
		widget = menu->widget;

		while(widget)
		{
			M_DrawWidget(widget);
			widget = widget->next;
		}
	}
/*
	if (cls.key_dest != key_menu)
		return;

	// repaint everything next frame
	SCR_DirtyScreen ();

	// dim everything behind it down
	if (cl.cinematictime > 0)
		re.DrawFill (0,0,viddef.width, viddef.height, 0);
	else
		re.DrawFadeScreen ();

	m_drawfunc ();

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		S_StartLocalSound( menu_in_sound );
		m_entersound = false;
	}*/
}

// ]===
