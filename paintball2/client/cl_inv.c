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
// cl_inv.c -- client inventory screen

#include "client.h"

// ===
// jit
#define MAX_ITEM_STRINGS 8
char item_strings[MAX_ITEM_STRINGS][MAX_QPATH]; // displayed briefly just above hud
int item_string_time[MAX_ITEM_STRINGS];
int startpos = 0;

static void item_print (char *s)
{
	startpos--;
	if(startpos<0)
		startpos = MAX_ITEM_STRINGS - 1;

	strcpy(item_strings[startpos], s);
	item_string_time[startpos] = curtime;
}

void CL_ParsePrintItem (char *s) // jit
{
	char buff[MAX_QPATH];

	if(*s == '+') // item pickup
		sprintf(buff, "%s", cl.configstrings[CS_ITEMS+s[1]]);
	else if(*s == '-') // item drop
		sprintf(buff, "%c%ca%s", CHAR_ITALICS, CHAR_COLOR, cl.configstrings[CS_ITEMS+s[1]]);
	else // unknown? (shouldn't happen)
		sprintf("UNKNOWN %s", cl.configstrings[CS_ITEMS+s[1]]);

	item_print(buff);
}

void CL_DrawItemPickups (void)
{
	int i, j;
	float alpha;

	for (i=0, j=startpos; i<MAX_ITEM_STRINGS; i++, j++, j%=MAX_ITEM_STRINGS)
	{
		if(*item_strings[j])
		{
			alpha = (4000 - (curtime-item_string_time[j])) / 3000.0f;

			if(alpha > 1.0f)
				alpha = 1.0f;

			if(alpha < 0.05f)
			{
				item_strings[j][0] = '\0';
				break;
			}

			re.DrawStringAlpha((160-4*strlen_noformat(item_strings[j]))*hudscale,
				(200-i*8)*hudscale, item_strings[j], alpha);
		}
		else
			break;
	}
}

// jit
// ===

/*
================
CL_ParseInventory
================
*/
void CL_ParseInventory (void)
{
	int		i;

	for (i=0 ; i<MAX_ITEMS ; i++)
		cl.inventory[i] = MSG_ReadShort (&net_message);
}


/*
================
Inv_DrawString
================
*/
void Inv_DrawString (int x, int y, char *string)
{
	while (*string)
	{
		re.DrawChar(x, y, *string);
		x+=8*hudscale; // jithudscale
		string++;
	}
}

void SetStringHighBit (char *s)
{
	while (*s)
		*s++ |= 128;
}

/*
================
CL_DrawInventory
================
*/
#define	DISPLAY_ITEMS	17

void CL_DrawInventory (void)
{
	int		i, j;
	int		num, selected_num, item;
	int		index[MAX_ITEMS];
	char	string[1024];
	int		x, y;
	char	binding[1024];
	char	*bind;
	int		selected;
	int		top;

	selected = cl.frame.playerstate.stats[STAT_SELECTED_ITEM];

	num = 0;
	selected_num = 0;
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		if (i==selected)
			selected_num = num;
		if (cl.inventory[i])
		{
			index[num] = i;
			num++;
		}
	}

	// determine scroll point
	top = selected_num - DISPLAY_ITEMS*0.5;
	if (num - top < DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (viddef.width-256)*0.5;
	y = (viddef.height-240)*0.5;

	// repaint everything next frame
	SCR_DirtyScreen();

	//re.DrawPic2 (x, y+8, i_inventory);

	y += 24;
	x += 24;
	Inv_DrawString(x, y, "hotkey ### item");
	Inv_DrawString(x, y+8, "------ --- ----");
	y += 16;
	for (i=top; i<num && i < top+DISPLAY_ITEMS; i++)
	{
		item = index[i];
		// search for a binding
		Com_sprintf (binding, sizeof(binding), "use %s", cl.configstrings[CS_ITEMS+item]);
		bind = "";

		for (j=0 ; j<256 ; j++)
		{
			if (keybindings[j] && !Q_strcasecmp(keybindings[j], binding))
			{
				bind = Key_KeynumToString(j);
				break;
			}
		}

		Com_sprintf (string, sizeof(string), "%6s %3i %s", bind, cl.inventory[item],
			cl.configstrings[CS_ITEMS+item]);

		if (item != selected)
			SetStringHighBit (string);
		else	// draw a blinky cursor by the selected item
		{
			if ( (int)(cls.realtime*10) & 1)
				re.DrawChar (x-8*hudscale, y, 15);
		}

		Inv_DrawString (x, y, string);
		y += 8*hudscale;
	}
}


