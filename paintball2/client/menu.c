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

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "menu.h"

#define MAX_MENU_SCREENS 32
#define INITIAL_SERVERLIST_SIZE 32

int				m_menudepth	= 0;
menu_screen_t	*root_menu	= NULL;
menu_screen_t	*m_menu_screens[MAX_MENU_SCREENS];
menu_mouse_t	m_mouse; // jitmouse
int				scale;
static float	oldscale = 0;
m_serverlist_t	m_serverlist;
int				m_serverPingSartTime;
menu_widget_t	*m_active_bind_widget = NULL;
char			*m_active_bind_command = NULL;

static qboolean widget_is_selectable(menu_widget_t *widget)
{
	return (widget->enabled && (widget->cvar || widget->command || widget->callback));
}

void M_ForceMenuOff (void)
{
//	m_drawfunc = 0;
//	m_keyfunc = 0;
	m_menudepth = 0;

	cls.key_dest = key_game;
	Key_ClearStates ();
	Cvar_Set ("paused", "0");

	if(oldscale && (oldscale != cl_hudscale->value))
		Cvar_SetValue("cl_hudscale", oldscale);
}

void M_Menu_Main_f (void)
{
	// jitodo -- different menu in-game
	Cbuf_AddText("menu main\n");

	
	/*
	// because I'm too lazy to start a new project to do this:
	// funname editor
	{
		int i;
		FILE *file;
		file = fopen("out.txt", "w");
		for(i=' '; i<255; i++)
		{
			fprintf(file, "widget xrel %d ", (i%16)?TEXT_WIDTH_UNSCALED:-15*TEXT_WIDTH_UNSCALED);
			if(!(i%16))
				fprintf(file, "yrel %d ", TEXT_HEIGHT_UNSCALED); 
			fprintf(file, "text \"%c\" command \"cvar_cat name %c\"\n", i, i);
		}

		for(i=' '; i<255; i++)
		{
			fprintf(file, "widget xrel %d ", (i%16)?TEXT_WIDTH_UNSCALED:-15*TEXT_WIDTH_UNSCALED);
			if(!(i%16))
				fprintf(file, "yrel %d ", TEXT_HEIGHT_UNSCALED); 
			fprintf(file, "pic blank picwidth 8 picheight 8 command \"cvar_cat name %c%c\"\n", CHAR_COLOR, i);
		}
	}*/
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

static void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j=0; j<256; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if(strcmp (b, command) == 0)
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

static void *free_string_array(char *array[], int size)
{
	int i;

	if(array)
	{
		for(i=0; i<size; i++)
			Z_Free(array[i]);

		Z_Free(array);
	}
	return NULL;
}

static void FreeFileList( char **list, int n )
{
	int i;

	for ( i = 0; i < n; i++ )
	{
		if ( list[i] )
		{
			free( list[i] );
			list[i] = 0;
		}
	}
	free( list );
}

// free the widget passed as well as all of its child widgets
// and widgets following it in the list, return NULL.
static menu_widget_t *free_widgets(menu_widget_t *widget)
{
	if(!widget)
		return NULL;

	// recursively free widgets in list
	if(widget->next)
		widget->next = free_widgets(widget->next);
	if(widget->subwidget)
		widget->subwidget = free_widgets(widget->subwidget);

	if(widget->command)
		Z_Free(widget->command);
	if(widget->cvar)
		Z_Free(widget->cvar);
	if(widget->cvar_default)
		Z_Free(widget->cvar_default);
	if(widget->hovertext)
		Z_Free(widget->hovertext);
	if(widget->text)
		Z_Free(widget->text);
	if(widget->selectedtext)
		Z_Free(widget->selectedtext);
	if(!(widget->flags & WIDGET_FLAG_SERVERLIST)) // don't free the serverlist!
	{
		if(widget->select_map)
			free_string_array(widget->select_map, widget->select_totalitems);
		if(widget->select_list)
		{
			if(widget->flags & WIDGET_FLAG_FILELIST)
				FreeFileList(widget->select_list, widget->select_totalitems+1);
			else
				free_string_array(widget->select_list, widget->select_totalitems);
		}
	}
	Z_Free(widget);

	return NULL;
}

static menu_widget_t* M_GetNewBlankMenuWidget()
{
	menu_widget_t *widget;

	widget = Z_Malloc(sizeof(menu_widget_t));
	memset(widget, 0, sizeof(menu_widget_t));

	widget->enabled = true;
	widget->modified = true;

	return widget;
}

static menu_widget_t* M_GetNewMenuWidget(int type, const char *text, const char *cvar, 
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

	widget->modified = true;

	return widget;
}

static void callback_select_item(menu_widget_t *widget)
{
	if(widget->flags & WIDGET_FLAG_BIND)
	{
		m_active_bind_widget = widget;
		m_active_bind_command = widget->parent->select_map[widget->select_pos];
//		widget->parent->modified = true;
	}
	else
	{
		// update the position of the selected item in the list
		if(widget->parent->select_pos != widget->select_pos)
		{
			widget->parent->modified = true;

			widget->parent->select_pos = widget->select_pos;

			// update the widget's cvar
			if (!(widget->parent->flags & WIDGET_FLAG_NOAPPLY) && widget->parent->cvar)
			{
				if(widget->parent->flags & WIDGET_FLAG_USEMAP)
					Cvar_Set(widget->parent->cvar, widget->parent->select_map[widget->select_pos]);
				else
					Cvar_Set(widget->parent->cvar, widget->parent->select_list[widget->select_pos]);
			}
		}
	}
}

static void callback_select_scrollup(menu_widget_t *widget)
{
	if(widget->parent->select_vstart > 0)
	{
		widget->parent->select_vstart --;
		widget->parent->modified = true;
	}
}

static void callback_select_scrolldown(menu_widget_t *widget)
{
	if(widget->parent->select_totalitems - widget->parent->select_vstart - widget->parent->select_rows > 0)
	{
		widget->parent->select_vstart ++;
		widget->parent->modified = true;
	}
}

static menu_widget_t *create_background(int x, int y, int w, int h, menu_widget_t *next)
{
	menu_widget_t *start = next;
	menu_widget_t *new_widget;

	// top left corner:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = SELECT_BACKGROUND_SIZE;
	new_widget->picheight = SELECT_BACKGROUND_SIZE;
	new_widget->pic = re.DrawFindPic("select1btl");
	new_widget->x = x;
	new_widget->y = y;
	new_widget->next = start;
	start = new_widget;

	// top edge
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = w - SELECT_BACKGROUND_SIZE*2;
	new_widget->picheight = SELECT_BACKGROUND_SIZE;
	new_widget->pic = re.DrawFindPic("select1bt");
	new_widget->x = x + SELECT_BACKGROUND_SIZE;
	new_widget->y = y;
	new_widget->next = start;
	start = new_widget;

	// top right corner:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = SELECT_BACKGROUND_SIZE;
	new_widget->picheight = SELECT_BACKGROUND_SIZE;
	new_widget->pic = re.DrawFindPic("select1btr");
	new_widget->x = x + w - SELECT_BACKGROUND_SIZE;
	new_widget->y = y;
	new_widget->next = start;
	start = new_widget;

	// left edge:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = SELECT_BACKGROUND_SIZE;
	new_widget->picheight = h - SELECT_BACKGROUND_SIZE*2;
	new_widget->pic = re.DrawFindPic("select1bl");
	new_widget->x = x;
	new_widget->y = y + SELECT_BACKGROUND_SIZE;
	new_widget->next = start;
	start = new_widget;

	// right edge:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = SELECT_BACKGROUND_SIZE;
	new_widget->picheight = h - SELECT_BACKGROUND_SIZE*2;
	new_widget->pic = re.DrawFindPic("select1br");
	new_widget->x = x + w - SELECT_BACKGROUND_SIZE;
	new_widget->y = y + SELECT_BACKGROUND_SIZE;
	new_widget->next = start;
	start = new_widget;

	// middle:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = w - SELECT_BACKGROUND_SIZE*2;
	new_widget->picheight = h - SELECT_BACKGROUND_SIZE*2;
	new_widget->pic = re.DrawFindPic("select1bm");
	new_widget->x = x + SELECT_BACKGROUND_SIZE;
	new_widget->y = y + SELECT_BACKGROUND_SIZE;
	new_widget->next = start;
	start = new_widget;

	// bottom edge:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = w - SELECT_BACKGROUND_SIZE*2;
	new_widget->picheight = SELECT_BACKGROUND_SIZE;
	new_widget->pic = re.DrawFindPic("select1bb");
	new_widget->x = x + SELECT_BACKGROUND_SIZE;
	new_widget->y = y + h - SELECT_BACKGROUND_SIZE;
	new_widget->next = start;
	start = new_widget;

	// bottom left corner:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = SELECT_BACKGROUND_SIZE;
	new_widget->picheight = SELECT_BACKGROUND_SIZE;
	new_widget->pic = re.DrawFindPic("select1bbl");
	new_widget->x = x;
	new_widget->y = y + h - SELECT_BACKGROUND_SIZE;
	new_widget->next = start;
	start = new_widget;

	// bottom right corner:
	new_widget = M_GetNewBlankMenuWidget();
	new_widget->picwidth = SELECT_BACKGROUND_SIZE;
	new_widget->picheight = SELECT_BACKGROUND_SIZE;
	new_widget->pic = re.DrawFindPic("select1bbr");
	new_widget->x = x + w - SELECT_BACKGROUND_SIZE;
	new_widget->y = y + h - SELECT_BACKGROUND_SIZE;
	new_widget->next = start;
	start = new_widget;

	return start;
}

static void select_widget_center_pos(menu_widget_t *widget)
{
	if(widget->select_pos > 0)
	{
		// put the selection in the middle of the widget (for long lists)
		widget->select_vstart = widget->select_pos - (widget->select_rows / 2);

		// don't go past end
		if(widget->select_totalitems - widget->select_vstart < widget->select_rows)
			widget->select_vstart = widget->select_totalitems - widget->select_rows;

		// don't go past start
		if(widget->select_vstart < 0)
			widget->select_vstart = 0;
	}
}

static void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}

static char *string_for_bind(char *bind)
{
	int keys[2];
	static char str[64];

	M_FindKeysForCommand(bind, keys);

	if(keys[0] == -1)
		sprintf(str, "%cNot Bound", CHAR_ITALICS);
	else
	{
		strcpy(str, Key_KeynumToString(keys[0]));

		if(keys[1] != -1)
		{
			strcat(str, " or ");
			strcat(str, Key_KeynumToString(keys[1]));
		}
	}

	return str;
}

static void update_select_subwidgets(menu_widget_t *widget)
{
	int i;
	char *nullpos;
	char *s;
	menu_widget_t *new_widget;
	char temp;
	int width, x, y;
	char *widget_text;
//	int keys[2];

	if(widget->flags & WIDGET_FLAG_SERVERLIST)
	{
		widget->select_map = m_serverlist.ips;
		widget->select_list = m_serverlist.info;
		widget->select_totalitems = m_serverlist.numservers;
		widget->flags |= WIDGET_FLAG_USEMAP;
	}

	// find which position should be selected:
	if(widget->cvar)
	{
		s = Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->string;

		widget->select_pos = -1; // nothing selected;

		for(i = 0; i < widget->select_totalitems; i++)
		{
			if(widget->flags & WIDGET_FLAG_USEMAP)
			{
				if(!strcmp(s, widget->select_map[i]))
				{
					widget->select_pos = i;
					break;
				}
			}
			else
			{
				if(!strcmp(s, widget->select_list[i]))
				{
					widget->select_pos = i;
					break;
				}
			}
		}
	}

	if(widget->subwidget)
		widget->subwidget = free_widgets(widget->subwidget);

	x = (widget->widgetCorner.x - (viddef.width - 320*scale)/2)/scale; // jitodo -- adjust for y, too
	y = (widget->widgetCorner.y - (viddef.height - 240*scale)/2)/scale;	

	width = widget->select_width;
	if(width < 3)
		width = 3;

	// create the vertical scroll buttons:
	if(widget->select_totalitems > widget->select_rows)
	{
		// Up arrow
		new_widget = M_GetNewMenuWidget(WIDGET_TYPE_PIC, NULL, NULL, NULL,
			x + width*TEXT_WIDTH_UNSCALED - 
			SCROLL_ARROW_WIDTH_UNSCALED + SELECT_HSPACING_UNSCALED*2,
			y, true);

		new_widget->callback = callback_select_scrollup;
		new_widget->picwidth = SCROLL_ARROW_WIDTH_UNSCALED;
		new_widget->picheight = SCROLL_ARROW_HEIGHT_UNSCALED;
		new_widget->pic = re.DrawFindPic("select1u");
		new_widget->hoverpic = re.DrawFindPic("select1uh");
		new_widget->selectedpic = re.DrawFindPic("select1us");
		new_widget->parent = widget;
		new_widget->next = widget->subwidget;
		widget->subwidget = new_widget;

		// Down arrow
		new_widget = M_GetNewMenuWidget(WIDGET_TYPE_PIC, NULL, NULL, NULL,
			x + width*TEXT_WIDTH_UNSCALED - 
			SCROLL_ARROW_WIDTH_UNSCALED + SELECT_HSPACING_UNSCALED*2,
			y + widget->select_rows*
			(TEXT_HEIGHT_UNSCALED+SELECT_VSPACING_UNSCALED) + SELECT_VSPACING_UNSCALED - 
			SCROLL_ARROW_HEIGHT_UNSCALED, true);

		new_widget->callback = callback_select_scrolldown;
		new_widget->picwidth = SCROLL_ARROW_WIDTH_UNSCALED;
		new_widget->picheight = SCROLL_ARROW_HEIGHT_UNSCALED;
		new_widget->pic = re.DrawFindPic("select1d");
		new_widget->hoverpic = re.DrawFindPic("select1dh");
		new_widget->selectedpic = re.DrawFindPic("select1ds");
		new_widget->parent = widget;
		new_widget->next = widget->subwidget;
		widget->subwidget = new_widget;

		width --;

		// jitodo -- scrollbar
	}

	// create a widget for each item visible in the select widget
	for(i = widget->select_vstart; i < widget->select_totalitems
		&& i < widget->select_rows + widget->select_vstart;	i++)
	{
		if(widget->flags & WIDGET_FLAG_BIND)
		{
			// add bind info to end:
			widget_text = va("%s%s", widget->select_list[i],
					string_for_bind(widget->select_map[i]));
		}
		else
		{
			widget_text = widget->select_list[i];
		}

		// jitodo -- should make sure strlen(widget_text) > select_hstart, otherwise bad mem!
		// (once horizontal scrolling added)
		
		// truncate the text if too wide to fit:
		if(strlen_noformat(widget->select_list[i] + widget->select_hstart) > width)
		{
			nullpos = widget->select_list[i] + widget->select_hstart + width;
			temp = *nullpos;
			*nullpos = '\0';

			new_widget = M_GetNewMenuWidget(WIDGET_TYPE_PIC, 
				widget_text + widget->select_hstart, NULL, NULL, 
				x+SELECT_HSPACING_UNSCALED, 
				y + (i - widget->select_vstart) * 
				(TEXT_HEIGHT_UNSCALED + SELECT_VSPACING_UNSCALED) +
				SELECT_VSPACING_UNSCALED, true);

			*nullpos = temp;
		}
		else
		{
			new_widget = M_GetNewMenuWidget(WIDGET_TYPE_PIC, 
				widget_text + widget->select_hstart, NULL, NULL, 
				x+SELECT_HSPACING_UNSCALED, 
				y + (i - widget->select_vstart) * 
				(TEXT_HEIGHT_UNSCALED + SELECT_VSPACING_UNSCALED) +
				SELECT_VSPACING_UNSCALED, true);
		}

		// set images for background
		if(i == widget->select_pos)
			new_widget->pic = re.DrawFindPic("select1bs");
		else
			new_widget->pic = NULL; //re.DrawFindPic("blank");
		new_widget->hoverpic = re.DrawFindPic("select1bh");
		new_widget->picheight = TEXT_HEIGHT_UNSCALED + SELECT_VSPACING_UNSCALED;
		new_widget->valign = WIDGET_VALIGN_MIDDLE;
		new_widget->y += SELECT_VSPACING_UNSCALED*2;
		new_widget->picwidth = width * TEXT_WIDTH_UNSCALED;

		new_widget->select_pos = i;

		new_widget->parent = widget;
		new_widget->callback = callback_select_item;

		new_widget->flags = widget->flags; // inherit flags from parent

		new_widget->next = widget->subwidget;
		widget->subwidget = new_widget;
	}

	// create the background:
	if(!(widget->flags & WIDGET_FLAG_NOBG))
	{
		widget->subwidget = create_background(x, y, 
			widget->select_width * TEXT_WIDTH_UNSCALED + SELECT_HSPACING_UNSCALED*2, 
			widget->select_rows * (TEXT_HEIGHT_UNSCALED+SELECT_VSPACING_UNSCALED) + 
			SELECT_VSPACING_UNSCALED, widget->subwidget);
	}
}

// ++ ARTHUR [9/04/03]
static void M_UpdateDrawingInformation (menu_widget_t *widget)
{
	int xcenteradj, ycenteradj;
	char *text = NULL;
	image_t *pic = NULL;

	// only update if the widget or the hudscale has changed
	if(!(widget->modified || cl_hudscale->modified))
		return;

	widget->modified = false;

	scale = cl_hudscale->value;

	// we want the menu to be drawn in the center of the screen
	// if it's too small to fill it.
	xcenteradj = (viddef.width - 320*scale)/2;
	ycenteradj = (viddef.height - 240*scale)/2;
	
	widget->widgetCorner.x = widget->textCorner.x = widget->x * scale + xcenteradj;
	widget->widgetCorner.y = widget->textCorner.y = widget->y * scale + ycenteradj;

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
		if (pic || (widget->picwidth && widget->picheight))
		{
			widget->widgetSize.x = (widget->picwidth) ? widget->picwidth : pic->width;
			widget->widgetSize.y = (widget->picheight) ? widget->picheight : pic->height;
			if (pic == widget->hoverpic)
			{
				if (widget->hoverpicwidth)
					widget->widgetSize.x = widget->hoverpicwidth;

				if (widget->hoverpicheight)
					widget->widgetSize.y = widget->hoverpicheight;			
			}
			widget->widgetSize.x *= scale;
			widget->widgetSize.y *= scale;
		}

		switch(widget->type)
		{
		case WIDGET_TYPE_SLIDER:
			widget->widgetSize.x = SLIDER_TOTAL_WIDTH;
			widget->widgetSize.y = SLIDER_TOTAL_HEIGHT;
			break;
		case WIDGET_TYPE_CHECKBOX:
			widget->widgetSize.x = CHECKBOX_WIDTH;
			widget->widgetSize.y = CHECKBOX_HEIGHT;
			break;
		case WIDGET_TYPE_FIELD:
			{
			int width;

			width = widget->field_width - 2;
			
			if(width < 1)
				width = 1;

			widget->widgetSize.x = FIELD_LWIDTH + TEXT_WIDTH*width + FIELD_RWIDTH;
			widget->widgetSize.y = FIELD_HEIGHT;
			if(widget->valign != WIDGET_VALIGN_MIDDLE) // center text by field vertically
				widget->textCorner.y += (FIELD_HEIGHT-TEXT_HEIGHT)/2;
			}
			break;
		case WIDGET_TYPE_SELECT:
			if(widget->select_rows < 2) // jitodo -- allow for dropdowns later
				widget->select_rows = 2;
			if(widget->select_width < 2)
				widget->select_width = 2;
			widget->widgetSize.x = widget->select_width * TEXT_WIDTH + SELECT_HSPACING*2;
			widget->widgetSize.y = widget->select_rows * 
				(TEXT_HEIGHT+SELECT_VSPACING) + SELECT_VSPACING;
			break;
		}

		

		switch(widget->halign)
		{
		case WIDGET_HALIGN_CENTER:
			widget->widgetCorner.x -= widget->widgetSize.x/2;
			widget->textCorner.x -= (strlen_noformat(text)*8*scale)/2;
			break;
		case WIDGET_HALIGN_RIGHT:
			widget->widgetCorner.x -= widget->widgetSize.x;
			widget->textCorner.x -= (strlen_noformat(text)*8*scale);
			switch(widget->type)
			{
			case WIDGET_TYPE_SLIDER:
			case WIDGET_TYPE_CHECKBOX:
			case WIDGET_TYPE_FIELD:
			case WIDGET_TYPE_SELECT:
				widget->textCorner.x -= (8*scale + widget->widgetSize.x);
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
			case WIDGET_TYPE_CHECKBOX:
			case WIDGET_TYPE_FIELD:
			case WIDGET_TYPE_SELECT:
				widget->textCorner.x += (8*scale + widget->widgetSize.x);
				break;
			default:
				break;
			}
			break;
		}

		switch(widget->valign)
		{
		case WIDGET_VALIGN_MIDDLE:
			widget->widgetCorner.y -= (widget->widgetSize.y/2);
			widget->textCorner.y -= TEXT_HEIGHT/2.0f;
			break;
		case WIDGET_VALIGN_BOTTOM:
			widget->widgetCorner.y -= widget->widgetSize.y;
			widget->textCorner.y -= TEXT_HEIGHT;
			break;
		default:
			break;
		}

		widget->mouseBoundaries.bottom = -0x0FFFFFFF;
		widget->mouseBoundaries.left = 0x0FFFFFFF;
		widget->mouseBoundaries.right = -0x0FFFFFFF;
		widget->mouseBoundaries.top = 0x0FFFFFFF;

		if (pic || widget->type == WIDGET_TYPE_CHECKBOX || (widget->picwidth && widget->picheight))
		{
			widget->mouseBoundaries.left = widget->widgetCorner.x;
			widget->mouseBoundaries.right = widget->widgetCorner.x + widget->widgetSize.x;
			widget->mouseBoundaries.top = widget->widgetCorner.y;
			widget->mouseBoundaries.bottom = widget->widgetCorner.y + widget->widgetSize.y;
		}

		if (text)
		{
			widget->textSize.x = (strlen_noformat(text)*TEXT_HEIGHT);
			widget->textSize.y = TEXT_HEIGHT;

			switch(widget->type)
			{
			case WIDGET_TYPE_CHECKBOX:
				widget->textCorner.y = widget->widgetCorner.y + (CHECKBOX_HEIGHT - TEXT_HEIGHT)/2;
				break;
			case WIDGET_TYPE_SLIDER:
				widget->textCorner.y = widget->widgetCorner.y + (SLIDER_TOTAL_HEIGHT - TEXT_HEIGHT)/2;
				break;
			}
				 

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
			widget->mouseBoundaries.left = widget->widgetCorner.x;
			widget->mouseBoundaries.right = widget->widgetCorner.x + widget->widgetSize.x;
			widget->mouseBoundaries.top = widget->widgetCorner.y;
			widget->mouseBoundaries.bottom = widget->widgetCorner.y + widget->widgetSize.y;
		}

		if(widget->select_list)
			update_select_subwidgets(widget);
	}
}

static void M_DeselectWidget(menu_widget_t *current)
{
	// jitodo - apply changes to field, call this func during keyboard arrow selection
	current->slider_hover = SLIDER_SELECTED_NONE;
	current->slider_selected = SLIDER_SELECTED_NONE;
	current->selected = false;
	current->hover = false;
}

static menu_widget_t *find_widget_under_cursor(menu_widget_t *widget)
{
	menu_widget_t *selected = NULL;
	menu_widget_t *sub_selected;

	while(widget)
	{
		if (widget_is_selectable(widget) &&
			m_mouse.x > widget->mouseBoundaries.left &&
			m_mouse.x < widget->mouseBoundaries.right &&
			m_mouse.y < widget->mouseBoundaries.bottom &&
			m_mouse.y > widget->mouseBoundaries.top)
		{
			selected = widget;
		}
		else
		{
			// clear selections
			M_DeselectWidget(widget);
		}

		if(widget->subwidget)
		{
			sub_selected = find_widget_under_cursor(widget->subwidget);
			if(sub_selected)
				selected = sub_selected;
		}

		widget = widget->next;
	}

	return selected;
}


// figure out what portion of the slider to highlight
// (buttons, knob, or bar)
static void M_HilightSlider(menu_widget_t *widget, qboolean selected)
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
static void M_UpdateSlider(menu_widget_t *widget)
{
	float value;
	if(widget->cvar)
		value = Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->value;
	else
		return;

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

static void toggle_checkbox(menu_widget_t *widget)
{
	if(widget->cvar)
	{
		if(Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->value)
			Cvar_SetValue(widget->cvar, 0.0f);
		else
			Cvar_SetValue(widget->cvar, 1.0f);	
	}
}

static void M_HilightNextWidget(menu_screen_t *menu)
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

static void M_HilightPreviousWidget(menu_screen_t *menu)
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

static void field_adjustCursor(menu_widget_t *widget)
{
	int pos, string_len=0;
	
	pos = widget->field_cursorpos;

	// jitodo -- compensate for color formatting
	if(widget->cvar)
		string_len = strlen(Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->string);

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


static void M_AdjustWidget(menu_screen_t *menu, int direction, qboolean keydown)
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
		case WIDGET_TYPE_SELECT:
			if(keydown)
			{
				widget->select_pos += direction;
				if(widget->select_pos < 0)
					widget->select_pos = 0;
				if(widget->select_pos >= widget->select_totalitems)
					widget->select_pos = widget->select_totalitems - 1;
				widget->modified = true;
				if(widget->cvar)
				{
					if(widget->flags & WIDGET_FLAG_USEMAP)
						Cvar_Set(widget->cvar, widget->select_map[widget->select_pos]);
					else
						Cvar_Set(widget->cvar, widget->select_list[widget->select_pos]);
				}
				select_widget_center_pos(widget);
			}

			break;
		default:
			break;
		}
	}
}

static void field_activate(menu_widget_t *widget)
{
	widget->field_cursorpos = (m_mouse.x - widget->widgetCorner.x)/(TEXT_WIDTH) + widget->field_start;
	field_adjustCursor(widget);
}

static void widget_execute(menu_widget_t *widget)
{
	if(widget->selected)
	{
		//MENU_SOUND_OPEN;
		if(widget->command)
		{
			Cbuf_AddText(widget->command);
			Cbuf_AddText("\n");
		}

		if(widget->callback)
		{
			widget->callback(widget);
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


static void M_KBAction(menu_screen_t *menu, MENU_ACTION action)
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

static qboolean M_MouseAction(menu_screen_t* menu, MENU_ACTION action)
{
	menu_widget_t* newSelection=NULL;
	m_mouse.cursorpic = i_cursor;

	if (!menu)
		return false;

	// If we're actually over something
	if (NULL != (newSelection = find_widget_under_cursor(menu->widget))) 
	{
		newSelection->modified = true;

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


static void M_PushMenuScreen(menu_screen_t *menu, qboolean samelevel)
{
	MENU_SOUND_OPEN;
	if(m_menudepth < MAX_MENU_SCREENS)
	{
		if(samelevel)
		{
			m_menu_screens[m_menudepth-1] = menu;
		}
		else
		{
			m_menu_screens[m_menudepth] = menu;
			m_menudepth++;
		}
		cls.key_dest = key_menu;
	}
}

static void M_PopMenu (void)
{
	MENU_SOUND_CLOSE;
	if (m_menudepth < 1)
		//Com_Error (ERR_FATAL, "M_PopMenu: depth < 1");
		m_menudepth = 1;
	m_menudepth--;

	if(oldscale && (oldscale != cl_hudscale->value))
		Cvar_SetValue("cl_hudscale", oldscale);
/*
	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;
*/
	if (!m_menudepth)
		M_ForceMenuOff ();
}

// find the first widget that is selected or hilighted and return it
// otherwise return null.
static menu_widget_t *M_GetActiveWidget(menu_screen_t *menu)
{
	menu_widget_t *widget;

	if(!menu)
		menu = m_menu_screens[m_menudepth-1];

	for(widget=menu->widget; widget && !widget->hover && !widget->selected; widget = widget->next);

	return widget;
}


static void M_InsertField (int key)
{
	char s[256];
	menu_widget_t *widget;
	int cursorpos, maxlength;//, start;

	widget = M_GetActiveWidget(NULL);

	if (!widget || widget->type != WIDGET_TYPE_FIELD || !widget->cvar)
		return;

	strcpy(s, Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->string);
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
			if(cursorpos <= widget->field_start)
				widget->field_start -= ((widget->field_width+1) / 2);
			if(cursorpos < 0)
				cursorpos = 0;
			if(widget->field_start < 0)
				widget->field_start = 0;
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
	else if(key >= 32 && key < 127)
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
	if(m_active_bind_command)
	{
		m_active_bind_widget = NULL;
		m_active_bind_command = NULL;
		return;
	}

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
	if(m_active_bind_command)
	{
		if(key == K_ESCAPE || key == '`') // jitodo -- is console toggled before this?
		{
			m_active_bind_widget = NULL;
			m_active_bind_command = NULL;
		}
		else
		{
			if(key != K_MOUSEMOVE) // don't try to bind mouse movements!
			{
				int keys[2];

				M_FindKeysForCommand(m_active_bind_command, keys);

				if(keys[1] != -1) // 2 or more binds, so clear them out.
					M_UnbindCommand(m_active_bind_command);

				Key_SetBinding(key, m_active_bind_command);
				m_active_bind_widget->parent->modified = true;
				m_active_bind_widget = NULL;
				m_active_bind_command = NULL;
			}
		}
	}
	else
	{
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
}



static select_map_list_t *get_new_select_map_list(char *cvar_string, char *string)
{
	select_map_list_t *new_map;

	new_map = Z_Malloc(sizeof(select_map_list_t));
	memset(new_map, 0, sizeof(select_map_list_t));
	
	if(cvar_string)
	{
		new_map->cvar_string = Z_Malloc(strlen(cvar_string)+1);
		strcpy(new_map->cvar_string, cvar_string);
	}

	if(string)
	{
		new_map->string = Z_Malloc(strlen(string)+1);
		strcpy(new_map->string, string);
	}

	return new_map;
}

// get the list from the file, then store it in an array on the widget
static void select_begin_list(menu_widget_t *widget, char *buf)
{
	char *token;
	char cvar_string[MAX_TOKEN_CHARS];
	int count=0, i;

	select_map_list_t *new_map;
	select_map_list_t *list_start = NULL;
	select_map_list_t *finger;

	token = COM_Parse(&buf);

	if(strstr(token, "pair") || strstr(token, "map") || strstr(token, "bind"))
	{
		widget->flags |= WIDGET_FLAG_USEMAP;
		if(strstr(token,"bind"))
			widget->flags |= WIDGET_FLAG_BIND;

		token = COM_Parse(&buf);
	
		// read in map pair
		while(token && strlen(token) && strcmp(token, "end") != 0)
		{
			strcpy(cvar_string, token);
			token = COM_Parse(&buf);
			new_map = get_new_select_map_list(cvar_string, token);
			new_map->next = list_start;
			list_start = new_map;
			count ++;
			token = COM_Parse(&buf);
		}
		widget->select_totalitems = count;
		widget->select_map = Z_Malloc(sizeof(char*)*count);
		widget->select_list = Z_Malloc(sizeof(char*)*count);

		if(count > widget->select_rows)
			widget->flags |= WIDGET_FLAG_VSCROLLBAR;

		// put the list into the widget's array, and free the list.
		for(i=count-1; i>=0; i--, finger=finger->next)
		{
			finger = list_start;
			widget->select_map[i] = finger->cvar_string;
			widget->select_list[i] = finger->string;
			list_start = finger->next;
			Z_Free(finger);
		}
	}
	else if(strstr(token, "single") || strstr(token, "list"))
	{
		// jitodo
	}
}

static void select_strip_from_list(menu_widget_t *widget, const char *striptext)
{
	int i, len;
	char *textpos;

	len = strlen(striptext);

	for(i=0; i<widget->select_totalitems; i++)
	{
		if(textpos = strstr(widget->select_list[i], striptext))
			strcpy(textpos, textpos+len);
	}
}

static void select_begin_file_list(menu_widget_t *widget, char *findname)
{
//	int i, numfiles;
//	char **files;
	extern char **FS_ListFiles( char *, int *, unsigned, unsigned );
//	static void FreeFileList( char **list, int n );


	widget->select_list = FS_ListFiles(findname, &widget->select_totalitems, 0, 0);
	widget->select_totalitems--;
	widget->flags |= WIDGET_FLAG_FILELIST;

/*	files = FS_ListFiles(findname, &numfiles, 0, 0);
	
	widget->select_list = Z_Malloc(sizeof(char*) * numfiles);

	for(i = 0; i < numfiles; i++)
	{
		widget->select_list[i] = Z_Malloc(sizeof(char)*(strlen(files[i])+1));
		strcpy(widget->select_list[i], files[i]);
	}
	
	FreeFileList(files, numfiles);*/
}


static menu_screen_t* M_GetNewMenuScreen(const char *menu_name)
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

static void M_ErrorMenu(menu_screen_t* menu, const char *text)
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

static int M_WidgetGetType(const char *s)
{
	if(!strcmp(s, "text"))
		return WIDGET_TYPE_TEXT;
	if(!strcmp(s, "button"))
		return WIDGET_TYPE_BUTTON;
	if(!strcmp(s, "slider"))
		return WIDGET_TYPE_SLIDER;
	if(!strcmp(s, "checkbox"))
		return WIDGET_TYPE_CHECKBOX;
	if(!strcmp(s, "dropdown") || strstr(s, "select"))
		return WIDGET_TYPE_SELECT;
	if(!strcmp(s, "editbox") || !strcmp(s, "field"))
		return WIDGET_TYPE_FIELD;

	return WIDGET_TYPE_UNKNOWN;
}

static int M_WidgetGetAlign(const char *s)
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

// same thing as strdup, only uses Z_Malloc
char *text_copy(const char *in)
{
	char *out;

	out = Z_Malloc(strlen(in)+1);
	strcpy(out, in);

	return out;
}


// Finished reading in a widget.  Do whatever we need to do
// to initialize it.
static void widget_complete(menu_widget_t *widget)
{
	if(widget->cvar && !widget->cvar_default)
		widget->cvar_default = text_copy("");

	switch(widget->type)
	{
	case WIDGET_TYPE_SLIDER:
		if(!widget->slider_min && !widget->slider_max)
		{
			widget->slider_min = 0.0f;
			widget->slider_max = 1.0f;
		}
		if(!widget->slider_inc)
			widget->slider_inc = (widget->slider_max - widget->slider_min)/24.0;
		break;
	case WIDGET_TYPE_SELECT:
		widget->select_pos = -1;
		update_select_subwidgets(widget);
		select_widget_center_pos(widget);
		break;
	case WIDGET_TYPE_FIELD:
		if(widget->field_width < 3)
			widget->field_width = 3; // can't have fields shorter than this!
		break;
	case WIDGET_TYPE_UNKNOWN:
	case WIDGET_TYPE_TEXT:
		if(widget->text && widget_is_selectable(widget) && !widget->pic && !widget->hoverpic && !widget->selectedpic)
		{
			widget->selectedpic = re.DrawFindPic("text1bg");
			widget->hoverpic = re.DrawFindPic("text1bgh");
			widget->picwidth = strlen_noformat(widget->text) * TEXT_WIDTH_UNSCALED;
			widget->picheight = TEXT_HEIGHT_UNSCALED;
		}
		break;
	}

}

static void menu_from_file(menu_screen_t *menu)
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
							widget_complete(widget);
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
					else if(strcmp(token, "cvar_default") == 0)
						widget->cvar_default = text_copy(COM_Parse(&buf));
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
					else if(strcmp(token, "xabs") == 0 || strcmp(token, "xleft") == 0 || strcmp(token, "x") == 0)
						x = widget->x = atoi(COM_Parse(&buf));
					else if(strcmp(token, "yabs") == 0 || strcmp(token, "ytop") == 0 || strcmp(token, "y") == 0)
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
					else if(strstr(token, "width") || strstr(token, "cols"))
						widget->field_width = atoi(COM_Parse(&buf));
					else if(strcmp(token, "int") == 0)
						widget->flags |= WIDGET_FLAG_INT; // jitodo
					else if(strcmp(token, "float") == 0)
						widget->flags |= WIDGET_FLAG_FLOAT; // jitodo
					// select/dropdown options
					else if(strstr(token, "size") || strstr(token, "rows") || strstr(token, "height"))
						widget->select_rows = atoi(COM_Parse(&buf));
					else if(strstr(token, "begin"))
						select_begin_list(widget, buf);
					else if(strstr(token, "file"))
						select_begin_file_list(widget, COM_Parse(&buf));
					else if(strstr(token, "serverlist"))
						widget->flags |= WIDGET_FLAG_SERVERLIST;
					else if(strstr(token, "strip"))
						select_strip_from_list(widget, COM_Parse(&buf));
					else if(strcmp(token, "nobg") == 0 || strcmp(token, "nobackground") == 0)
						widget->flags |= WIDGET_FLAG_NOBG;

					token = COM_Parse(&buf);
				}

				if(widget)
					widget_complete(widget);
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

static menu_screen_t* M_LoadMenuScreen(const char *menu_name)
{
	menu_screen_t *menu;

	menu = M_GetNewMenuScreen(menu_name);
	menu_from_file(menu);

	return menu;
}

static menu_screen_t* M_FindMenuScreen(const char *menu_name)
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


static void reload_menu_screen(menu_screen_t *menu)
{
//	menu_widget_t *widgetnext;
//	menu_widget_t *widget;

	if(!menu)
		return;

	/*widget = menu->widget;
	
	while(widget)
	{
		widgetnext = widget->next;
		free_widget(widget);
		widget = widgetnext;
	}*/
	menu->widget = free_widgets(menu->widget);

	menu_from_file(menu); // reload data from file
}

// flag widget and all of its children as modified
static void refresh_menu_widget(menu_widget_t *widget)
{
	widget->modified = true;

	widget = widget->subwidget;
	while(widget)
	{
		refresh_menu_widget(widget);
		widget = widget->next;
	}
}

// flag widgets as modified:
static void refresh_menu_screen(menu_screen_t *menu)
{
	menu_widget_t *widget;

	if(!menu)
		return;

	widget = menu->widget;

	while(widget)
	{
		refresh_menu_widget(widget);
		widget = widget->next;
	}
}

// Flag all widgets as modified so they update
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

// Load menu scripts back from disk
void M_ReloadMenu(void)
{
	menu_screen_t *menu;

	menu = root_menu;
	while(menu)
	{
		reload_menu_screen(menu);
		menu = menu->next;
	}
}

// Print server list to console
void M_ServerlistPrint_f(void)
{
	int i;

	for(i=0; i<m_serverlist.numservers; i++)
		Com_Printf("%d) %s %s\n", i+1, m_serverlist.ips[i], m_serverlist.info[i]);
		
}
/*
static serverlist_node_t *new_serverlist_node()
{
	serverlist_node_t *node;
	node = Z_Malloc(sizeof(serverlist_node_t));
	memset(node, 0, sizeof(serverlist_node_t));
	return node;
}*/

// free single node
/*static void free_serverlist_node(serverlist_node_t *node)
{
	if(node->info)
		Z_Free(node->info);
	Z_Free(node);
}

// recursively free all nodes
static void free_serverlist(serverlist_node_t *node)
{
	if(node)
	{
		free_serverlist(node->next);
		free_serverlist_node(node);
	}
}
*/
static void free_menu_serverlist()
{
	if(m_serverlist.info)
		free_string_array(m_serverlist.info, m_serverlist.numservers);
	if(m_serverlist.ips)
		free_string_array(m_serverlist.ips, m_serverlist.numservers);
//	free_serverlist(m_serverlist.list);

	memset(&m_serverlist, 0, sizeof(m_serverlist_t));
}

// parse info string into server name, players, maxplayers
// note! modifies string!
static void update_serverlist_server(m_serverlist_server_t *server, char *info, int ping)
{
	char *s;

	// start at end of string:
	s = strlen(info) + info; 

	// find max players
	while(s > info && *s != '/')
		s--;

	server->maxplayers = atoi(s+1);

	// find current number of players:
	*s = 0;
	s--;
	while(s > info && *s >= '0' && *s <= '9')
		s--;

	server->players = atoi(s+1);
	
	// find map name:
	while(s > info && *s == 32) // clear whitespace;
		s--;
	*(s+1) = 0;
	while(s > info && *s > 32)
		s--;

	server->mapname = text_copy(s+1);

	// servername is what's left over:
	*s = 0;
	server->servername = text_copy(info);

	// and the ping
	server->ping = ping;
}
/*
static m_serverlist_server_t *new_serverlist_server(netadr_t adr, char *info, int ping)
{
	m_serverlist_server_t *server;

	server = Z_Malloc(sizeof(m_serverlist_server_t));
	server->adr = adr;
	update_serverlist_server(server, info, ping);
}*/
#define SERVER_NAME_MAXLENGTH 19
#define MAP_NAME_MAXLENGTH 8
static char *format_info_from_serverlist_server(m_serverlist_server_t *server)
{
	static char info[64];
	char stemp=0, mtemp=0;
	int ping = 999;

	if(server->ping < 999)
		ping = server->ping;

	// jitodo -- should compensate for colored text in server name.

	// truncate name if too long:
	if(strlen_noformat(server->servername) > SERVER_NAME_MAXLENGTH)
	{
		stemp = server->servername[SERVER_NAME_MAXLENGTH];
		server->servername[SERVER_NAME_MAXLENGTH] = 0;
	}
	if(strlen(server->mapname) > MAP_NAME_MAXLENGTH)
	{
		mtemp = server->servername[MAP_NAME_MAXLENGTH];
		server->mapname[MAP_NAME_MAXLENGTH] = 0;
	}

	// assumes SERVER_NAME_MAXLENGTH is 19 vv
	if(ping < 999)
		Com_sprintf(info, sizeof(info), "%-19s %-3d %-8s %d/%d", 
			server->servername, ping, server->mapname,
			server->players, server->maxplayers);
	else
		Com_sprintf(info, sizeof(info), "%c4%-19s %-3d %-8s %d/%d", 
			CHAR_COLOR, server->servername, ping, server->mapname,
			server->players, server->maxplayers);

	if(stemp)
		server->servername[SERVER_NAME_MAXLENGTH] = stemp;
	if(mtemp)
		server->mapname[MAP_NAME_MAXLENGTH] = mtemp;

	return info;
}

void M_AddToServerList (netadr_t adr, char *info, qboolean pinging)
{
	int i;
	char addrip[32];
	int ping;
	qboolean added = false;

	/*if(pinging)
		ping = 999;
	else*/
		ping = Sys_Milliseconds() - m_serverPingSartTime;

	/*if(ping>999)
		ping = 999;*/

	if(adr.type == NA_IP)
	{
		Com_sprintf(addrip, sizeof(addrip), 
			"%d.%d.%d.%d:%d", adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], ntohs(adr.port));
	}
	else if(adr.type == NA_IPX)
	{
		Com_sprintf(addrip, sizeof(addrip), 
			"%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%i", adr.ipx[0], adr.ipx[1],
			adr.ipx[2], adr.ipx[3], adr.ipx[4], adr.ipx[5], adr.ipx[6], adr.ipx[7],
			adr.ipx[8], adr.ipx[9], ntohs(adr.port));
	}
	else if(adr.type == NA_LOOPBACK)
	{
		strcpy(addrip, "loopback");
	}
	else
		return;

	// jitodo
	
	// check if server exists in current serverlist:
	for(i=0; i<m_serverlist.numservers; i++)
	{
		if(strcmp(addrip, m_serverlist.ips[i]) == 0) // address exists in list
		{
			// update info from server:
			Z_Free(m_serverlist.info[i]);

			/*if(pinging)
				m_serverlist.server[i].ping_request_time = Sys_Milliseconds();
			else
			{
				ping = curtime - m_serverlist.server[i].ping_request_time;
				Com_Printf("%d - %d\n", curtime, m_serverlist.server[i].ping_request_time);
			}*/

			update_serverlist_server(&m_serverlist.server[i], info, ping);
			m_serverlist.info[i] = text_copy(format_info_from_serverlist_server(&m_serverlist.server[i]));
			
			added = true;
			break;
			//return;
		}
	}

	if(!added) // doesn't exist.  Add it.
	{
		i++;

		// List too big?  Alloc more memory:
		// STL would be useful about now
		if(i > m_serverlist.actualsize) 
		{
			char **tempinfo;
			char **tempips;
			m_serverlist_server_t *tempserver;

			tempinfo = Z_Malloc(sizeof(char*)*m_serverlist.actualsize*2); // double size
			tempips = Z_Malloc(sizeof(char*)*m_serverlist.actualsize*2); // double size
			tempserver = Z_Malloc(sizeof(m_serverlist_server_t)*m_serverlist.actualsize*2);

			for(i=0; i<m_serverlist.actualsize; i++)
			{
				tempinfo[i] = m_serverlist.info[i];
				tempips[i] = m_serverlist.ips[i];
				tempserver[i] = m_serverlist.server[i];
			}

			Z_Free(m_serverlist.info);
			Z_Free(m_serverlist.ips);
			Z_Free(m_serverlist.server);

			m_serverlist.info = tempinfo; // jitodo - test -- will this work?? (update widget??)
			m_serverlist.ips = tempips;
			m_serverlist.server = tempserver;

			m_serverlist.actualsize *= 2;
		}

		// add data to serverlist:
		m_serverlist.ips[m_serverlist.numservers] = text_copy(addrip);
		//m_serverlist.server[m_serverlist.numservers] = new_serverlist_server(adr, info, ping);

		/*if(pinging)
			m_serverlist.server[m_serverlist.numservers].ping_request_time = Sys_Milliseconds();
		else // this part shouldn't ever get executed, but just in case:
			ping = curtime - m_serverlist.server[m_serverlist.numservers].ping_request_time;*/

		update_serverlist_server(&m_serverlist.server[m_serverlist.numservers], info, ping);
		//m_serverlist.info[m_serverlist.numservers] = text_copy(va("%3d %s", ping, info));
		m_serverlist.info[m_serverlist.numservers] =
			text_copy(format_info_from_serverlist_server(&m_serverlist.server[m_serverlist.numservers]));
		
		m_serverlist.numservers++;
	}



	// Tell the widget the serverlist has updated: (shouldn't this be done after below code?)
	if(m_menudepth)
		refresh_menu_screen(m_menu_screens[m_menudepth-1]);

	//return m_serverlist.numservers - 1;
}

// color servers grey and re-ping them
void M_ServerlistRefresh_f(void)
{
	char *str;
	int i;

	for(i=0; i<m_serverlist.numservers; i++)
	{
		str = text_copy(va("%c4%s", CHAR_COLOR, m_serverlist.info[i])); // color grey
		Z_Free(m_serverlist.info[i]);
		m_serverlist.info[i] = str;
	}
	CL_PingServers_f();
}


// download servers.txt from a remote location and store
// on the client's drive.
#ifdef WIN32
void M_ServerlistUpdate_f(void) // jitodo, this should be called in a separate thread so it doesn't "lock up"
{
	SOCKET serverListSocket;
	char svlist_domain[256];
	//char svlist_dir[256];
	char *s;
	int i;

	serverListSocket = NET_TCPSocket(0);	// Create the socket descriptor
	if (serverListSocket == 0)
	{
		Com_Printf("Unable to create socket.\n");
		return; // No socket created
	}

	s = strstr(serverlist_source->string, "http://");
	if(!s)
	{
		// probably an old version -- going to require the http:// now.
		// jitodo -- allow for sources like "c:\servers.txt" and master server lists.
		Cvar_Set("serverlist_source", "http://www.planetquake.com/digitalpaint/servers.txt");
		M_ServerlistUpdate_f();
		return;
	}
	else
	{
		s += 7; // skip past the "http://"
		i = 0;
		while(*s && *s != '/')
		{
			svlist_domain[i] = *s;
			s++;
			i++;
		}
		svlist_domain[i] = 0; // terminate string
	}
	//s = strstr(serverlist_source->string, "/");
	//if(s)
	//	*s = 0;
	//if(!NET_TCPConnect(serverListSocket, serverlist_source->string, 80))
	if(!NET_TCPConnect(serverListSocket, svlist_domain, 80))
	{
		Com_Printf("Unable to connect to %s\n", svlist_domain);
		return;	// Couldn't connect
	}
	
	// We're connected! Lets ask for the list
	{
		char msg[256];
		int len, bytes_sent;

		//if(s)
		//	*s = '/';
		sprintf(msg, "GET %s HTTP/1.0\n\n", s);

		len = strlen(msg);
		bytes_sent = send(serverListSocket, msg, len, 0);
		if  (bytes_sent < len)
		{
			Com_Printf ("HTTP Server did not accept request, aborting\n");
			closesocket(serverListSocket);
			return;
		}
	}

	// We've sent our request... Lets read in what they've got for us
	{
		char *buffer	= malloc(32767);	
		char *current	= buffer;
		char *found		= NULL;
		int WhichOn		= 0;
		FILE			*serverlistfile;
		int numread = 0;
		int bytes_read = 0;

		serverlistfile = fopen(va("%s/servers.txt", FS_Gamedir()), "w");

		// Read in up to 32767 bytes
		while ( numread < 32760 && 0 < (bytes_read = recv(serverListSocket, buffer + numread, 32766 - numread, 0)))
		{
			numread += bytes_read;
		};

		if (bytes_read == -1)
		{
			Com_Printf ("WARNING: recv(): %s", NET_ErrorString());
			free(buffer);
			closesocket(serverListSocket);
			return;
		}

		closesocket(serverListSocket);

		// Okay! We have our data... now lets parse it! =)
		current = buffer;

		// find \n\n, thats the end of header/beginning of the data
		while (*current != '\n' || *(current+2) != '\n')
		{
			if (current > buffer + numread) {
				free(buffer); return; 
			}
			current ++;
		};
		current = current + 3; // skip the trailing \n.  We're at the beginning of the data now
		
		while (current < buffer + numread) {
			found = current;												// Mark the beginning of the line
			while (*current != 13) {										// Find the end of the line
				current ++; 
				if (current > buffer + numread) { free(buffer); return; }	// Exit if we run out of room
				if (*(current-1) == 'X' && *(current) == 13) {				// Exit if we find a X\n on a new line
					goto done; 
				}
			}
			*current = 0;													// NULL terminate the string
			fprintf(serverlistfile, "%s\n", found);							// Copy line to local file
			current += 2;													// Start at the next line
		};
done:
		fclose(serverlistfile);
		free(buffer);

		M_ServerlistRefresh_f();
		return;
	}
}
// ACT */
// ]===
#else
void M_ServerlistUpdate_f(void)
{
	Con_Print("'nix support under construction!\n"); // todo
}
#endif

void M_Menu_f (void)
{
	char *menuname;
	qboolean samelevel = false;

	menuname = Cmd_Argv(1);
	
	if(strcmp(menuname, "samelevel") == 0)
	{
		menuname = Cmd_Argv(2);
		samelevel = true;
	}

	if(strcmp(menuname, "pop") == 0 || strcmp(menuname, "back") == 0)
		M_PopMenu();
	else if(strcmp(menuname, "off") == 0 || strcmp(menuname, "close") == 0)
		M_ForceMenuOff();
	else
	{
		menu_screen_t *menu;
		menu = M_FindMenuScreen(menuname);

		// hardcoded hack so gamma image is correct:
		if(strcmp(menuname, "setup_gamma") == 0)
		{
			oldscale = cl_hudscale->value;
			Cvar_Set("cl_hudscale", "2");
		}
		else
			oldscale = 0;

		M_PushMenuScreen(menu, samelevel);
	}
}

void M_Init (void)
{
	memset(&m_mouse, 0, sizeof(m_mouse));
	m_mouse.cursorpic = i_cursor;

	// Init server list:
	memset(&m_serverlist, 0, sizeof(m_serverlist_t));
	m_serverlist.info = Z_Malloc(sizeof(char*)*INITIAL_SERVERLIST_SIZE); 
	m_serverlist.ips = Z_Malloc(sizeof(char*)*INITIAL_SERVERLIST_SIZE);
	m_serverlist.server = Z_Malloc(sizeof(m_serverlist_server_t)*INITIAL_SERVERLIST_SIZE);
	m_serverlist.actualsize = INITIAL_SERVERLIST_SIZE;

	Cmd_AddCommand("menu", M_Menu_f);
	Cmd_AddCommand("serverlist_update", M_ServerlistUpdate_f);
	Cmd_AddCommand("serverlist_refresh", M_ServerlistRefresh_f);
	Cmd_AddCommand("serverlist_print", M_ServerlistPrint_f);
}



static void M_DrawSlider(int x, int y, float pos, SLIDER_SELECTED slider_hover, SLIDER_SELECTED slider_selected)
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

static float M_SliderGetPos(menu_widget_t *widget)
{
	float sliderdiff;
	float retval = 0.0f;

	// if they forgot to set the slider max
	if(widget->slider_max == widget->slider_min)
		widget->slider_max++;

	sliderdiff = widget->slider_max - widget->slider_min;

	if(widget->cvar)
		retval = Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->value;
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

static void M_DrawCheckbox(int x, int y, qboolean checked, qboolean hover, qboolean selected)
{
	if(checked)
	{
		if(selected)
			re.DrawStretchPic2(x, y, CHECKBOX_WIDTH, CHECKBOX_HEIGHT, i_checkbox1us);
		else if(hover)
			re.DrawStretchPic2(x, y, CHECKBOX_WIDTH, CHECKBOX_HEIGHT, i_checkbox1ch);
		else
			re.DrawStretchPic2(x, y, CHECKBOX_WIDTH, CHECKBOX_HEIGHT, i_checkbox1c);
	}
	else
	{
		if(selected)
			re.DrawStretchPic2(x, y, CHECKBOX_WIDTH, CHECKBOX_HEIGHT, i_checkbox1cs);
		else if(hover)
			re.DrawStretchPic2(x, y, CHECKBOX_WIDTH, CHECKBOX_HEIGHT, i_checkbox1uh);
		else
			re.DrawStretchPic2(x, y, CHECKBOX_WIDTH, CHECKBOX_HEIGHT, i_checkbox1u);
	}
}




//void M_DrawField(int x, int y, int width, char *cvar_string, qboolean hover, qboolean selected)
static void M_DrawField(menu_widget_t *widget)
{
	//M_DrawField(widget->widgetCorner.x, widget->widgetCorner.y,
			//	widget->field_width, cvar_val, widget->hover, widget->selected);
	int width, x, y;
	char *cvar_string;
	char temp;
	int nullpos;

	if(widget->cvar)
		cvar_string = Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->string;
	else
		cvar_string = "";

	width = widget->field_width;
	x = widget->widgetCorner.x;
	y = widget->widgetCorner.y;

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
	if(strlen(cvar_string) > widget->field_start + widget->field_width)
	{
		nullpos = widget->field_start + widget->field_width;
		temp = cvar_string[nullpos];
		cvar_string[nullpos] = 0;
		re.DrawString(x+(FIELD_LWIDTH-TEXT_WIDTH), y+(FIELD_HEIGHT-TEXT_HEIGHT)/2,
			cvar_string+widget->field_start);
		cvar_string[nullpos] = temp;
	}
	else
	{
		re.DrawString(x+(FIELD_LWIDTH-TEXT_WIDTH), y+(FIELD_HEIGHT-TEXT_HEIGHT)/2,
			cvar_string+widget->field_start);
	}

	if(widget->selected || widget->hover)
	{
		Con_DrawCursor(x + (FIELD_LWIDTH-TEXT_WIDTH) +
			(widget->field_cursorpos - widget->field_start)*TEXT_WIDTH, y+(FIELD_HEIGHT-TEXT_HEIGHT)/2);
	}
}

/*
void M_DrawSelect(menu_widget_t *widget)
{
//	todo;
	int i, width, start, rows;
	char temp;
	
	width = widget->select_width;
	rows = widget->select_rows;
	start = widget->select_vstart;
	
//	if(widget->flags & WIDGET_FLAG_VSCROLLBAR)
//		width--;
//	if(widget->flags & WIDGET_FLAG_HSCROLLBAR)
//		rows --;

	for(i = start; i < (widget->select_totalitems) && (i - start < rows); i++)
	{
		// text too big to fit, enable scrollbar
		// and cut string off before drawing
		if(strlen(widget->select_list[i]) > width)
		{
			widget->flags &= WIDGET_FLAG_HSCROLLBAR;
			temp = widget->select_list[i][width];
			widget->select_list[i][width] = 0;
			
			re.DrawString(widget->x, widget->y + TEXT_HEIGHT*i, widget->select_list[i]);
			
			widget->select_list[i][width] = temp;
		}
		else
			re.DrawString(widget->x, widget->y + TEXT_HEIGHT*i, widget->select_list[i]);
	}

	if(widget->flags & WIDGET_FLAG_VSCROLLBAR)
	{
		// jitodo - draw scrollbar
	}
	if(widget->flags & WIDGET_FLAG_HSCROLLBAR)
	{
	}
}
*/

static void M_DrawWidget (menu_widget_t *widget)
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
				text = va("%c%c%s", CHAR_COLOR, 214, widget->text);

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
				text = va("%c%c%s", CHAR_COLOR, 218, widget->text);
			
			if(widget->hoverpic)
				pic = widget->hoverpic;
			else if(widget->pic)
				pic = widget->pic;
		}

		switch(widget->type) 
		{
		case WIDGET_TYPE_SLIDER:
			M_DrawSlider(widget->widgetCorner.x, widget->widgetCorner.y, M_SliderGetPos(widget),
				widget->slider_hover, widget->slider_selected);
			break;
		case WIDGET_TYPE_CHECKBOX:
			if(widget->cvar && Cvar_Get(widget->cvar, widget->cvar_default, CVAR_ARCHIVE)->value)
				checkbox_checked = true;
			else
				checkbox_checked = false;
			M_DrawCheckbox(widget->widgetCorner.x, widget->widgetCorner.y, 
				checkbox_checked, widget->hover, widget->selected);
			break;
		case WIDGET_TYPE_FIELD:
			//if(widget->cvar)
			//	cvar_val = Cvar_Get(widget->cvar, "0", CVAR_ARCHIVE)->string;
			M_DrawField(widget);
			//M_DrawField(widget->widgetCorner.x, widget->widgetCorner.y,
			//	widget->field_width, cvar_val, widget->hover, widget->selected);
			break;
		case WIDGET_TYPE_SELECT:
			//M_DrawSelect(widget);
			break;
		default:
			break;
		}

		if(pic)
			re.DrawStretchPic2(widget->widgetCorner.x,
								widget->widgetCorner.y,
								widget->widgetSize.x,
								widget->widgetSize.y,
								pic);
		if(text)
			re.DrawString(widget->textCorner.x, widget->textCorner.y, text);
	}

	// Draw subwidgets
	if(widget->subwidget)
	{
		menu_widget_t *subwidget;

		subwidget = widget->subwidget;
		while(subwidget)
		{
			M_DrawWidget(subwidget);
			subwidget = subwidget->next;
		}
	}
}

static void M_DrawCursor(void)
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

		SCR_DirtyScreen ();
		
		menu = m_menu_screens[m_menudepth-1];
		widget = menu->widget;

		while(widget)
		{
			M_DrawWidget(widget);
			widget = widget->next;
		}
		// waiting for user to press bind:
		if(m_active_bind_command)
		{
			re.DrawFadeScreen ();
			M_DrawWidget(m_active_bind_widget);
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


