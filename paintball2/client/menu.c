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
menu_mouse_t	m_mouse; // jitmouse
int				scale;


qboolean widget_is_selectable(menu_widget_t *widget)
{
	return widget->cvar || widget->command;
}

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
   Returns the length of strings ignoring formatting (underlines, colors, etc)
*/
int strlen_noformat(const unsigned char *s)
{
	int count = 0;
	if(!s)
		return 0;

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
	int xcenteradj, ycenteradj;
	char *text = NULL;
	image_t *pic = NULL;

	scale = cl_hudscale->value;

	// we want the menu to be drawn in the center of the screen
	// if it's too small to fill it.
	xcenteradj = (viddef.width - 320*scale)/2;
	ycenteradj = (viddef.height - 240*scale)/2;
	
	widget->picCorner.x = widget->textCorner.x = widget->x * scale + xcenteradj;
	widget->picCorner.y = widget->textCorner.y = widget->y * scale + ycenteradj;

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

		if(widget->type == WIDGET_TYPE_SLIDER)
		{
			widget->picSize.x = SLIDER_TOTAL_WIDTH;
			widget->picSize.y = SLIDER_TOTAL_HEIGHT;
		}
		else if(widget->type == WIDGET_TYPE_CHECKBOX)
		{
			widget->picSize.x = CHECKBOX_WIDTH;
			widget->picSize.y = CHECKBOX_HEIGHT;
		}
		else if(widget->type == WIDGET_TYPE_FIELD)
		{
			int width;

			width = widget->field_width - 2;
			
			if(width < 1)
				width = 1;

			widget->picSize.x = FIELD_LWIDTH + TEXT_WIDTH*width + FIELD_RWIDTH;
			widget->picSize.y = FIELD_HEIGHT;
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
			switch(widget->type)
			{
			case WIDGET_TYPE_SLIDER:
				widget->textCorner.x -= (8*scale + SLIDER_TOTAL_WIDTH);
				break;
			case WIDGET_TYPE_CHECKBOX:
				widget->textCorner.x -= (8*scale + CHECKBOX_WIDTH);
				break;
			default:
				break;
			}
			break;
		case WIDGET_HALIGN_LEFT:
		default:
			switch(widget->type)
			{
			case WIDGET_TYPE_SLIDER:
				widget->textCorner.x += (8*scale + SLIDER_TOTAL_WIDTH);
				break;
			case WIDGET_TYPE_CHECKBOX:
				widget->textCorner.x += (8*scale + CHECKBOX_WIDTH);
				break;
			default:
				break;
			}
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

		if (pic || widget->type == WIDGET_TYPE_CHECKBOX)
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

		if(widget->type == WIDGET_TYPE_SLIDER || widget->type == WIDGET_TYPE_FIELD)
		{
			widget->mouseBoundaries.left = widget->picCorner.x;
			widget->mouseBoundaries.right = widget->picCorner.x + widget->picSize.x;
			widget->mouseBoundaries.top = widget->picCorner.y;
			widget->mouseBoundaries.bottom = widget->picCorner.y + widget->picSize.y;
		}
	}
}


menu_widget_t* M_FindNextEntryUnderCursor(menu_screen_t* menu, MENU_ACTION action) 
{
	menu_widget_t* current = menu->widget;
	menu_widget_t* selected = NULL;

	while (current) 
	{
		M_UpdateDrawingInformation(current);

		if (widget_is_selectable(current) &&
			m_mouse.x > current->mouseBoundaries.left &&
			m_mouse.x < current->mouseBoundaries.right &&
			m_mouse.y < current->mouseBoundaries.bottom &&
			m_mouse.y > current->mouseBoundaries.top)
		{
			selected = current;
		}
		else // deselect everything else.
		{
			// clear selections
			current->slider_hover = SLIDER_SELECTED_NONE;
			current->slider_selected = SLIDER_SELECTED_NONE;
			current->selected = false;
			current->hover = false;
		}
		current = current->next;
	}
	return selected;
}

// figure out what portion of the slider to highlight
// (buttons, knob, or bar)
void M_HilightSlider(menu_widget_t *widget, qboolean selected)
{
	widget->slider_hover = SLIDER_SELECTED_NONE;
	widget->slider_selected = SLIDER_SELECTED_NONE;

	if(widget->type != WIDGET_TYPE_SLIDER)
		return;

	// left button:
	if (m_mouse.x < widget->mouseBoundaries.left + SLIDER_BUTTON_WIDTH)
	{
		if(selected)
			widget->slider_selected = SLIDER_SELECTED_LEFTARROW;
		else
			widget->slider_hover = SLIDER_SELECTED_LEFTARROW;
	}
	// tray area and knob:
	else if (m_mouse.x < widget->mouseBoundaries.left + SLIDER_BUTTON_WIDTH + SLIDER_TRAY_WIDTH)
	{
		if(selected)
			widget->slider_selected = SLIDER_SELECTED_TRAY;
		else
			widget->slider_hover = SLIDER_SELECTED_TRAY;
	}
	// right button:
	else
	{
		if(selected)
			widget->slider_selected = SLIDER_SELECTED_RIGHTARROW;
		else
			widget->slider_hover = SLIDER_SELECTED_RIGHTARROW;
	}
}

// update slider depending on where the user clicked
void M_UpdateSlider(menu_widget_t *widget)
{
	float value;
	if(widget->cvar)
		value = Cvar_Get(widget->cvar, "0", CVAR_ARCHIVE)->value;
	else
		return;

	if(!widget->slider_inc)
		widget->slider_inc = (widget->slider_max - widget->slider_min)/10.0;

	switch(widget->slider_selected)
	{
	case SLIDER_SELECTED_LEFTARROW:
		MENU_SOUND_SLIDER;
		value -= widget->slider_inc;
		break;
	case SLIDER_SELECTED_RIGHTARROW:
		MENU_SOUND_SLIDER;
		value += widget->slider_inc;
		break;
	case SLIDER_SELECTED_TRAY:
		value = (float)(m_mouse.x - (widget->mouseBoundaries.left + SLIDER_BUTTON_WIDTH + SLIDER_KNOB_WIDTH/2))/((float)(SLIDER_TRAY_WIDTH-SLIDER_KNOB_WIDTH));
		value = value * (widget->slider_max - widget->slider_min) + widget->slider_min;
		break;
	case SLIDER_SELECTED_KNOB:

		break;
	default:
		break;
	}

	// we can have the min > max for things like vid_gamma, where it makes
	// more sense for the user to have it backwards
	if(widget->slider_min > widget->slider_max)
	{
		if(value > widget->slider_min)
			value = widget->slider_min;
		else if(value < widget->slider_max)
			value = widget->slider_max;
	}
	else
	{
		if(value > widget->slider_max)
			value = widget->slider_max;
		if(value < widget->slider_min)
			value = widget->slider_min;
	}

	Cvar_SetValue(widget->cvar, value);
	// todo: may not want to set value immediately (video mode change) -- need temp variables.
}

void toggle_checkbox(menu_widget_t *widget)
{
	if(widget->cvar)
	{
		if(Cvar_Get(widget->cvar, "0", CVAR_ARCHIVE)->value)
			Cvar_SetValue(widget->cvar, 0.0f);
		else
			Cvar_SetValue(widget->cvar, 1.0f);	
	}
}

void M_HilightNextWidget(menu_screen_t *menu)
{
	menu_widget_t *widget;

	widget = menu->widget;

	// find the hilighted widget
	while(widget && !widget->selected && !widget->hover)
		widget = widget->next;

	// nothing hilighted, start with the first one on the list
	// that's selectable
	if(!widget)
	{
		widget = menu->widget;
		while(widget && !widget_is_selectable(widget))
			widget = widget->next;

		if(widget)
		{
			widget->hover = true;
			MENU_SOUND_SELECT;
		}
	}
	else
	{
		widget->hover = false;
		widget->selected = false;

		widget = widget->next;
		if(!widget)
			widget = menu->widget;
		while(!widget_is_selectable(widget))
		{
			widget = widget->next;
			if(!widget)
				widget = menu->widget;
		}

		widget->hover = true;
		MENU_SOUND_SELECT;
	}
}

void M_HilightPreviousWidget(menu_screen_t *menu)
{
	menu_widget_t *widget, *oldwidget, *prevwidget;

	widget = menu->widget;

	// find the hilighted widget
	while(widget && !widget->selected && !widget->hover)
		widget = widget->next;

	// nothing hilighted, start with the first one on the list
	// that's selectable
	if(!widget)
	{
		widget = menu->widget;
		while(widget && !widget_is_selectable(widget))
			widget = widget->next;

		if(widget)
		{
			MENU_SOUND_SELECT;
			widget->hover = true;
		}
	}
	// otherwise find the previous selectable widget
	else
	{
		widget->hover = false;
		widget->selected = false;
		oldwidget = widget;

		widget = widget->next;
		if(!widget)
			widget = menu->widget;
		while(widget != oldwidget)
		{
			if(widget_is_selectable(widget))
				prevwidget = widget;

			widget = widget->next;
			if(!widget)
				widget = menu->widget;
		}

		prevwidget->hover = true;
		MENU_SOUND_SELECT;
	}
}

void field_adjustCursor(menu_widget_t *widget)
{
	int pos, string_len=0;
	
	pos = widget->field_cursorpos;

	// jitodo -- compensate for color formatting
	if(widget->cvar)
		string_len = strlen(Cvar_Get(widget->cvar, "", CVAR_ARCHIVE)->string);

	if(pos < 0)
		pos = 0;

	if(pos < widget->field_start)
		widget->field_start = pos;

	if(pos > string_len)
		pos = string_len;

	if(pos >= widget->field_width + widget->field_start)
		widget->field_start = pos - widget->field_width + 1;

	widget->field_cursorpos = pos;
}


void M_AdjustWidget(menu_screen_t *menu, int direction, qboolean keydown)
{
	menu_widget_t *widget;

	// find an active widget, if there is one:
	for(widget=menu->widget; widget && !widget->hover && !widget->selected; widget = widget->next);

	if(widget)
	{
		switch(widget->type)
		{
		case WIDGET_TYPE_SLIDER:
			if(keydown)
			{
				if(direction < 0)
					widget->slider_selected = SLIDER_SELECTED_LEFTARROW;
				else
					widget->slider_selected = SLIDER_SELECTED_RIGHTARROW;
			}
			else
				widget->slider_selected = SLIDER_SELECTED_NONE;
			M_UpdateSlider(widget);
			break;
		case WIDGET_TYPE_FIELD:
			if(keydown)
			{
				widget->field_cursorpos += direction;
				field_adjustCursor(widget);
			}
			break;
		default:
			break;
		}
	}
}

void field_activate(menu_widget_t *widget)
{
	widget->field_cursorpos = (m_mouse.x - widget->picCorner.x)/(TEXT_WIDTH) + widget->field_start;
	field_adjustCursor(widget);
}

void widget_execute(menu_widget_t *widget)
{
	if(widget->selected)
	{
		//MENU_SOUND_OPEN;
		if(widget->command)
		{
			Cbuf_AddText(widget->command);
			Cbuf_AddText("\n");
		}
		switch(widget->type)
		{
		case WIDGET_TYPE_SLIDER:
			M_UpdateSlider(widget);
			widget->selected = false;
			break;
		case WIDGET_TYPE_CHECKBOX:
			toggle_checkbox(widget);
			widget->selected = false;
			break;
		case WIDGET_TYPE_FIELD:
			widget->selected = true;
			field_activate(widget);
			break;
		default:
			break;
		}

		widget->hover = true;
	}
}


void M_KBAction(menu_screen_t *menu, MENU_ACTION action)
{
	menu_widget_t *widget;

	widget = menu->widget;

	while(widget && !widget->hover && !widget->selected)
		widget = widget->next;

	if(widget)
	{
		switch(action)
		{
		case M_ACTION_SELECT:
			widget->selected = true;
			break;
		case M_ACTION_EXECUTE:
			widget_execute(widget);
			break;
		default:
			break;
		}
	}
}

qboolean M_MouseAction(menu_screen_t* menu, MENU_ACTION action)
{
	menu_widget_t* newSelection=NULL;
	m_mouse.cursorpic = i_cursor;

	if (!menu)
		return false;

	// If we're actually over something
	if (NULL != (newSelection = M_FindNextEntryUnderCursor(menu, action))) 
	{
		switch (action)
		{
		case M_ACTION_HILIGHT:
			switch (newSelection->type)
			{
			case WIDGET_TYPE_SLIDER:
				M_HilightSlider(newSelection, newSelection->selected);
				if(newSelection->slider_selected == SLIDER_SELECTED_TRAY)
					M_UpdateSlider(newSelection);
				break;
			case WIDGET_TYPE_FIELD:
				m_mouse.cursorpic = i_cursor_text;
				break;
			default:
				break;
			}
			
			newSelection->hover = true;
			break;
		case M_ACTION_SELECT:
			switch(newSelection->type)
			{
			case WIDGET_TYPE_SLIDER:
				M_HilightSlider(newSelection, true);
				if(newSelection->slider_selected == SLIDER_SELECTED_TRAY)
					M_UpdateSlider(newSelection);
				break;
			case WIDGET_TYPE_FIELD:
				m_mouse.cursorpic = i_cursor_text;
				break;
			default:
				break;
			}
			newSelection->selected = true;
			break;
		case M_ACTION_EXECUTE:
			widget_execute(newSelection);
			break;
		case M_ACTION_NONE:
		default:
			break;
		}
		
	}		

	return true;
}


void M_PushMenuScreen(menu_screen_t *menu)
{
	MENU_SOUND_OPEN;
	if(m_menudepth < MAX_MENU_SCREENS)
	{
		m_menu_screens[m_menudepth] = menu;
		m_menudepth++;
		cls.key_dest = key_menu;
	}
}

void M_PopMenu (void)
{
	MENU_SOUND_CLOSE;
	if (m_menudepth < 1)
		//Com_Error (ERR_FATAL, "M_PopMenu: depth < 1");
		m_menudepth = 1;
	m_menudepth--;
/*
	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;
*/
	if (!m_menudepth)
		M_ForceMenuOff ();
}

// find the first widget that is selected or hilighted and return it
// otherwise return null.
menu_widget_t *M_GetActiveWidget(menu_screen_t *menu)
{
	menu_widget_t *widget;

	if(!menu)
		menu = m_menu_screens[m_menudepth-1];

	for(widget=menu->widget; widget && !widget->hover && !widget->selected; widget = widget->next);

	return widget;
}


void M_InsertField (int key)
{
	char s[256];
	menu_widget_t *widget;
	int cursorpos, maxlength;//, start;

	widget = M_GetActiveWidget(NULL);

	if (!widget || widget->type != WIDGET_TYPE_FIELD || !widget->cvar)
		return;

	strcpy(s, Cvar_Get(widget->cvar, "", CVAR_ARCHIVE)->string);
	cursorpos = widget->field_cursorpos;
	maxlength = widget->field_width;
	//start = widget->field_start;

	key = KeyPadKey(key);


	if ((toupper(key) == 'V' && keydown[K_CTRL]) ||
		 (((key == K_INS) || (key == K_KP_INS)) && keydown[K_SHIFT]))
	{
		char *cbd;
		
		if ((cbd = Sys_GetClipboardData()) != 0)
		{
			int i;

			strtok(cbd, "\n\r\b");

			i = strlen(cbd);
			if (i + strlen(s) >= sizeof(s)-1)
				i= sizeof(s) - 1 - strlen(s);

			if (i > 0)
			{
				cbd[i] = 0;
				strcat(s, cbd);
				cursorpos += i;
			}
			free(cbd);
		}
	}
	else if ((key == K_BACKSPACE))
	{
		if (cursorpos > 0)
		{
			// skip to the end of color sequence

			strcpy (s + cursorpos - 1, s + cursorpos);
			cursorpos--;
		}
	}
	else if (key == K_DEL)
	{
		if (cursorpos < strlen(s))
			strcpy(s + cursorpos, s + cursorpos + 1);
	}
	else if (key == K_INS)
	{
		key_insert = !key_insert;
	}
	else if (key == K_HOME)
	{
		cursorpos = 0;
	}
	else if (key == K_END)
	{
		cursorpos = strlen(s);
	}
	else if(key > 32 && key < 127)
	{
		// color codes
		if(keydown[K_CTRL]) // jitconsole / jittext
		{
			if(toupper(key) == 'K')
				key = CHAR_COLOR;
			else if(toupper(key) == 'U')
				key = CHAR_UNDERLINE;
			else if(toupper(key) == 'I')
				key = CHAR_ITALICS;
		}

		// normal text
		if (cursorpos < sizeof(s)-1)
		{
			int i;

			// check insert mode
			if (key_insert)
			{
				// can't do strcpy to move string to right
				i = strlen(s);

				if (i == 254) 
					i--;

				for (; i >= cursorpos; i--)
					s[i + 1] = s[i];
			}

			// only null terminate if at the end
			i = s[cursorpos];
			s[cursorpos] = key;
			cursorpos++;
			if (!i)
				s[cursorpos] = 0;	
		}
	}

	// put updated values in widget's cvar
	widget->field_cursorpos = cursorpos;
	Cvar_Set(widget->cvar, s);
	field_adjustCursor(widget);
}

void M_Keyup (int key)
{
	switch (key) 
	{
	case K_ENTER:
	case K_KP_ENTER:
		M_KBAction(m_menu_screens[m_menudepth-1], M_ACTION_EXECUTE);
		break;
	case K_MOUSE1:
		M_MouseAction(m_menu_screens[m_menudepth-1], M_ACTION_EXECUTE);
		//key = K_ENTER;
		break;
//	case K_MOUSEMOVE:
//		M_MouseAction(m_menu_screens[m_menudepth-1], M_ACTION_HILIGHT);
//		break;
	case K_RIGHTARROW:
		M_AdjustWidget(m_menu_screens[m_menudepth-1], 1, false);
		break;
	case K_LEFTARROW:
		M_AdjustWidget(m_menu_screens[m_menudepth-1], -1, false);
		break;
	default:
		break;
	};
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
	case K_ESCAPE:
		M_PopMenu();
		break;
	case K_ENTER:
	case K_KP_ENTER:
		M_KBAction(m_menu_screens[m_menudepth-1], M_ACTION_SELECT);
		break;
	case K_MOUSE1:
		M_MouseAction(m_menu_screens[m_menudepth-1], M_ACTION_SELECT);
		key = K_ENTER;
		break;
	case K_MOUSEMOVE:
		M_MouseAction(m_menu_screens[m_menudepth-1], M_ACTION_HILIGHT);
		break;
	case K_DOWNARROW:
		M_HilightNextWidget(m_menu_screens[m_menudepth-1]);
		break;
	case K_UPARROW:
		M_HilightPreviousWidget(m_menu_screens[m_menudepth-1]);
		break;
	case K_RIGHTARROW:
		M_AdjustWidget(m_menu_screens[m_menudepth-1], 1, true);
		break;
	case K_LEFTARROW:
		M_AdjustWidget(m_menu_screens[m_menudepth-1], -1, true);
		break;
	default:
		// insert letter into field, if one is active
		M_InsertField(key);
		break;
	};
}
// -- ARTHUR




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

void M_ErrorMenu(menu_screen_t* menu, const char *text)
{
	char err[16];
	sprintf(err, "%c%c%cERROR:", CHAR_UNDERLINE, CHAR_COLOR, 'A');
	menu->widget = M_GetNewMenuWidget(WIDGET_TYPE_TEXT, err,
			NULL, NULL, 60, 116, true);
	menu->widget->next = M_GetNewMenuWidget(WIDGET_TYPE_TEXT, text,
			NULL, NULL, 60, 124, true);
	menu->widget->next->next = M_GetNewMenuWidget(WIDGET_TYPE_TEXT, "Back",
			NULL, "menu pop", 8, 224, true);
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
	if(!strcmp(s, "editbox") || !strcmp(s, "field"))
		return WIDGET_TYPE_FIELD;

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

void menu_from_file(menu_screen_t *menu)
{
	char menu_filename[MAX_QPATH];
	char *buf;
	int file_len;

	scale = cl_hudscale->value;

	sprintf(menu_filename, "menus/%s.txt", menu->name);
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
						widget->type = M_WidgetGetType(COM_Parse(&buf));
					else if(strcmp(token, "text") == 0)
						widget->text = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "hovertext") == 0)
						widget->hovertext = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "selectedtext") == 0)
						widget->selectedtext = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "cvar") == 0)
						widget->cvar = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "command") == 0 || strcmp(token, "cmd") == 0)
						widget->command = text_copy(COM_Parse(&buf));
					else if(strcmp(token, "pic") == 0)
					{
						widget->pic = re.DrawFindPic(COM_Parse(&buf));
						// default to double resolution, since it's too blocky otherwise.
						if(!widget->picwidth)
							widget->picwidth = widget->pic->width / 2.0f;
						if(!widget->picheight)
							widget->picheight = widget->pic->height / 2.0f;
					}
					else if(strcmp(token, "picwidth") == 0)
						widget->picwidth = atoi(COM_Parse(&buf));
					else if(strcmp(token, "picheight") == 0)
						widget->picheight = atoi(COM_Parse(&buf));
					else if(strcmp(token, "hoverpic") == 0)
					{
						widget->hoverpic = re.DrawFindPic(COM_Parse(&buf));
						// default to double resolution, since it's too blocky otherwise.
						if(!widget->hoverpicwidth)
							widget->hoverpicwidth = widget->hoverpic->width / 2.0f;
						if(!widget->hoverpicheight)
							widget->hoverpicheight = widget->hoverpic->height / 2.0f;
					}
					else if(strcmp(token, "hoverpicwidth") == 0)
						widget->hoverpicwidth = atoi(COM_Parse(&buf));
					else if(strcmp(token, "hoverpicheight") == 0)
						widget->hoverpicheight = atoi(COM_Parse(&buf));
					else if(strcmp(token, "selectedpic") == 0)
						widget->selectedpic = re.DrawFindPic(COM_Parse(&buf));
					else if(strcmp(token, "xabs") == 0 || strcmp(token, "xleft") == 0)
						x = widget->x = atoi(COM_Parse(&buf));
					else if(strcmp(token, "yabs") == 0 || strcmp(token, "ytop") == 0)
						y = widget->y = atoi(COM_Parse(&buf));
					else if(strstr(token, "xcent"))
						x = widget->x = 160 + atoi(COM_Parse(&buf));
					else if(strstr(token, "ycent"))
						y = widget->y = 120 + atoi(COM_Parse(&buf));
					else if(strcmp(token, "xright") == 0)
						x = widget->x = 320 + atoi(COM_Parse(&buf));
					else if(strstr(token, "ybot"))
						y = widget->y = 240 + atoi(COM_Parse(&buf));
					else if(strstr(token, "xrel"))
						x = widget->x += atoi(COM_Parse(&buf));
					else if(strstr(token, "yrel"))
						y = widget->y += atoi(COM_Parse(&buf));
					else if(strcmp(token, "halign") == 0)
						widget->halign = M_WidgetGetAlign(COM_Parse(&buf));
					else if(strcmp(token, "valign") == 0)
						widget->valign = M_WidgetGetAlign(COM_Parse(&buf));
					// slider cvar min, max, and increment
					else if(strstr(token, "min"))
						widget->slider_min = atof(COM_Parse(&buf));
					else if(strstr(token, "max"))
						widget->slider_max = atof(COM_Parse(&buf));
					else if(strstr(token, "inc"))
						widget->slider_inc = atof(COM_Parse(&buf));
					// editbox/field options
					else if(strstr(token, "width"))
						widget->field_width = atoi(COM_Parse(&buf));
					
					token = COM_Parse(&buf);
				}
			}
			else
				M_ErrorMenu(menu, "Invalid menu version.");
		}
		else
			M_ErrorMenu(menu, "Invalid menu file.");

		Z_Free(buf2);
	}
	else
	{
		char notfoundtext[MAX_QPATH+10];

		sprintf(notfoundtext, "%s not found.", menu_filename);
		M_ErrorMenu(menu, notfoundtext);
	}
}

menu_screen_t* M_LoadMenuScreen(const char *menu_name)
{
	menu_screen_t *menu;

	menu = M_GetNewMenuScreen(menu_name);
	menu_from_file(menu);

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


void M_Menu_f (void)
{
	char *menuname;
	menuname = Cmd_Argv(1);

	if(strcmp(menuname, "pop") == 0)
	{
		M_PopMenu();
	}
	else
	{
		menu_screen_t *menu;
		//Com_Printf("Menu: %s (%d)\n", Cmd_Argv(1), m_menudepth);
		menu = M_FindMenuScreen(Cmd_Argv(1));

		M_PushMenuScreen(menu);
	}
}

void M_Init (void)
{
	memset(&m_mouse, 0, sizeof(m_mouse));
	//strcpy(m_mouse.cursorpic, "cursor");
	m_mouse.cursorpic = i_cursor;

	Cmd_AddCommand("menu", M_Menu_f);
}

void free_widget(menu_widget_t *widget)
{
	if(!widget)
		return;

	if(widget->command)
		Z_Free(widget->command);
	if(widget->cvar)
		Z_Free(widget->cvar);
	if(widget->hovertext)
		Z_Free(widget->hovertext);
	if(widget->text)
		Z_Free(widget->text);
	Z_Free(widget);
}

void refresh_menu_screen(menu_screen_t *menu)
{
	menu_widget_t *widgetnext;
	menu_widget_t *widget;

	if(!menu)
		return;

	widget = menu->widget;
	
	while(widget)
	{
		widgetnext = widget->next;
		free_widget(widget);
		widget = widgetnext;
	}

	menu_from_file(menu); // reload data from file
}

void M_RefreshMenu(void)
{
	menu_screen_t *menu;

	menu = root_menu;
	while(menu)
	{
		refresh_menu_screen(menu);
		menu = menu->next;
	}
}

void M_DrawSlider(int x, int y, float pos, SLIDER_SELECTED slider_hover, SLIDER_SELECTED slider_selected)
{
	int xorig;

	xorig = x;

	// left arrow:
	if(slider_selected == SLIDER_SELECTED_LEFTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1ls);
	else if(slider_hover == SLIDER_SELECTED_LEFTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1lh);
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
	if(slider_selected == SLIDER_SELECTED_RIGHTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1rs);
	else if(slider_hover == SLIDER_SELECTED_RIGHTARROW)
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1rh);
	else
		re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_slider1r);

	// knob:
	x = xorig + SLIDER_BUTTON_WIDTH + pos*(SLIDER_TRAY_WIDTH-SLIDER_KNOB_WIDTH);
	if(slider_selected == SLIDER_SELECTED_TRAY ||
		slider_hover == SLIDER_SELECTED_KNOB ||	slider_selected == SLIDER_SELECTED_KNOB)
		re.DrawStretchPic2(x, y, SLIDER_KNOB_WIDTH, SLIDER_KNOB_HEIGHT, i_slider1bs);
	else if(slider_hover == SLIDER_SELECTED_TRAY || slider_hover == SLIDER_SELECTED_KNOB)
		re.DrawStretchPic2(x, y, SLIDER_KNOB_WIDTH, SLIDER_KNOB_HEIGHT, i_slider1bh);
	else
		re.DrawStretchPic2(x, y, SLIDER_KNOB_WIDTH, SLIDER_KNOB_HEIGHT, i_slider1b);
}

float M_SliderGetPos(menu_widget_t *widget)
{
	float sliderdiff;
	float retval = 0.0f;

	// if they forgot to set the slider max
	if(widget->slider_max == widget->slider_min)
		widget->slider_max++;

	sliderdiff = widget->slider_max - widget->slider_min;

	if(widget->cvar)
		retval = Cvar_Get(widget->cvar, "0", CVAR_ARCHIVE)->value;
	else
		retval = 0;

	if(sliderdiff)
		retval = (retval - widget->slider_min) / sliderdiff;

	if (retval > 1.0f)
		retval = 1.0f;
	if (retval < 0.0f)
		retval = 0.0f;

	return retval;
}

void M_DrawCheckbox(int x, int y, qboolean checked, qboolean hover, qboolean selected)
{
	if(checked)
	{
		if(selected)
			re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_checkbox1us);
		else if(hover)
			re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_checkbox1ch);
		else
			re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_checkbox1c);
	}
	else
	{
		if(selected)
			re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_checkbox1cs);
		else if(hover)
			re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_checkbox1uh);
		else
			re.DrawStretchPic2(x, y, SLIDER_BUTTON_WIDTH, SLIDER_BUTTON_HEIGHT, i_checkbox1u);
	}
}




//void M_DrawField(int x, int y, int width, char *cvar_string, qboolean hover, qboolean selected)
void M_DrawField(menu_widget_t *widget)
{
	//M_DrawField(widget->picCorner.x, widget->picCorner.y,
			//	widget->field_width, cvar_val, widget->hover, widget->selected);
	int width, x, y;
	char *cvar_string;
	char temp;
	int nullpos;

	if(widget->cvar)
		cvar_string = Cvar_Get(widget->cvar, "", CVAR_ARCHIVE)->string;
	else
		cvar_string = "";

	width = widget->field_width;
	x = widget->picCorner.x;
	y = widget->picCorner.y;

	if(width > 2)
		width -= 2;
	else
		width = 1;

	if(widget->selected)
	{
		re.DrawStretchPic2(x, y, FIELD_LWIDTH, FIELD_HEIGHT, i_field1ls);
		re.DrawStretchPic2(x+FIELD_LWIDTH, y, TEXT_WIDTH*width, FIELD_HEIGHT, i_field1ms);
		re.DrawStretchPic2(x+FIELD_LWIDTH+TEXT_WIDTH*width, y, FIELD_LWIDTH, FIELD_HEIGHT, i_field1rs);
	}
	else if(widget->hover)
	{
		re.DrawStretchPic2(x, y, FIELD_LWIDTH, FIELD_HEIGHT, i_field1lh);
		re.DrawStretchPic2(x+FIELD_LWIDTH, y, TEXT_WIDTH*width, FIELD_HEIGHT, i_field1mh);
		re.DrawStretchPic2(x+FIELD_LWIDTH+TEXT_WIDTH*width, y, FIELD_LWIDTH, FIELD_HEIGHT, i_field1rh);
	}
	else
	{
		re.DrawStretchPic2(x, y, FIELD_LWIDTH, FIELD_HEIGHT, i_field1l);
		re.DrawStretchPic2(x+FIELD_LWIDTH, y, TEXT_WIDTH*width, FIELD_HEIGHT, i_field1m);
		re.DrawStretchPic2(x+FIELD_LWIDTH+TEXT_WIDTH*width, y, FIELD_LWIDTH, FIELD_HEIGHT, i_field1r);
	}


	// draw only the portion of the string that fits within the field:
	nullpos = widget->field_start + widget->field_width;
	temp = cvar_string[nullpos];
	cvar_string[nullpos] = 0;
	re.DrawString(x+(FIELD_LWIDTH-TEXT_WIDTH), y+(FIELD_HEIGHT-TEXT_HEIGHT)/2, cvar_string+widget->field_start);
	cvar_string[nullpos] = temp;

	if(widget->selected || widget->hover)
	{
		Con_DrawCursor(x + (FIELD_LWIDTH-TEXT_WIDTH) +
			(widget->field_cursorpos - widget->field_start)*TEXT_WIDTH, y+(FIELD_HEIGHT-TEXT_HEIGHT)/2);
	}
}

void M_DrawWidget (menu_widget_t *widget)
{
	char *text = NULL;
	char *cvar_val = "";
	image_t *pic = NULL;
	qboolean checkbox_checked;

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
		if(widget->selected)
		{
			if(widget->selectedtext)
				text = widget->selectedtext;
			else if(widget->text)
				text = va("%c%c%s", CHAR_COLOR, 'E', widget->text);

			if(widget->selectedpic)
				pic = widget->selectedpic;
			else if(widget->hoverpic)
				pic = widget->hoverpic;
			else if(widget->pic)
				pic = widget->pic;
		}
		else if(widget->hover)
		{
			if(widget->hovertext)
				text = widget->hovertext;
			else if(widget->text)
				text = va("%c%c%c%s", CHAR_UNDERLINE, CHAR_COLOR, '?', widget->text);
			
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
		case WIDGET_TYPE_CHECKBOX:
			if(widget->cvar && Cvar_Get(widget->cvar, "0", CVAR_ARCHIVE)->value)
				checkbox_checked = true;
			else
				checkbox_checked = false;
			M_DrawCheckbox(widget->picCorner.x, widget->picCorner.y, 
				checkbox_checked, widget->hover, widget->selected);
			break;
		case WIDGET_TYPE_FIELD:
			//if(widget->cvar)
			//	cvar_val = Cvar_Get(widget->cvar, "0", CVAR_ARCHIVE)->string;
			M_DrawField(widget);
			//M_DrawField(widget->picCorner.x, widget->picCorner.y,
			//	widget->field_width, cvar_val, widget->hover, widget->selected);
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

void M_DrawCursor(void)
{
	re.DrawStretchPic2(m_mouse.x-CURSOR_WIDTH/2, m_mouse.y-CURSOR_HEIGHT/2,
		CURSOR_WIDTH, CURSOR_HEIGHT, m_mouse.cursorpic);
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
		M_DrawCursor();
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

void M_MouseMove(int mx, int my)
{
	m_mouse.x += mx;
	m_mouse.y += my;

	if(m_mouse.x < 0)
		m_mouse.x = 0;
	if(m_mouse.y < 0)
		m_mouse.y = 0;

	if(m_mouse.x > viddef.width)
		m_mouse.x = viddef.width;
	if(m_mouse.y > viddef.height)
		m_mouse.y = viddef.height;
}

qboolean M_MenuActive()
{
	return (m_menudepth != 0);
}
// ]===
