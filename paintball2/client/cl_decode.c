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

// === jitmenu / jitscores
// encode_bin_str.c -- Functions to encode/decode binary data stored as a string

#include "client.h"

#define MARKER_CHAR 255
#define MARKER_CHAR_0 1
#define MARKER_CHAR_SHORT 254
#define MARKER_CHAR_LONG 253

#define item_from_index(a) cl.configstrings[CS_ITEMS+(a)]

static unsigned char *outpos;
static const char *decode_ptr;
static unsigned int decode_char_unsigned (void);

static void encode_addmarker (const unsigned char marker)
{
	*outpos = MARKER_CHAR;
	outpos++;
	*outpos = marker;
	outpos++;
}

static void encode_addchar (unsigned int c)
{
	c &= 255;

	if (c == 0)
	{
		*outpos = MARKER_CHAR;
		outpos++;
		*outpos = MARKER_CHAR_0;
	}
	else if (c == MARKER_CHAR)
	{
		*outpos = MARKER_CHAR;
		outpos++;
		*outpos = MARKER_CHAR;
	}
	else
	{
		*outpos = c;
	}

	outpos++;
}

static void encode_end (void)
{
	*outpos = '\0';
}

static unsigned int decode_short_unsigned (void)
{
	return (decode_char_unsigned() << 8) | decode_char_unsigned();
}

static unsigned int decode_long_unsigned (void)
{
	return (decode_short_unsigned() << 16) | decode_short_unsigned();
}

static unsigned int decode_char_unsigned (void)
{
	unsigned char c;
	
	c = *decode_ptr;
	decode_ptr++;

	if (c == MARKER_CHAR)
	{
		c = *decode_ptr;
		decode_ptr++;

		switch (c)
		{
			case MARKER_CHAR:
				return MARKER_CHAR;
				break;
			case MARKER_CHAR_0:
				return 0;
				break;
			case MARKER_CHAR_SHORT:
				return decode_short_unsigned();
				break;
			case MARKER_CHAR_LONG:
				return decode_long_unsigned();
				break;
			default:
				Com_Printf("ERROR: Bad encode!\n");
				return 0;
				break;
		}
	}
	else
	{
		return c;
	}
}

void encode_unsigned (unsigned int count, unsigned int *in, unsigned char *out)
{
	unsigned int i, out_i = 0;
	unsigned int j;
	unsigned char c;
	unsigned short s;

	outpos = out;

	for (i = 0; i < count; i++)
	{
		j = in[i] + 1; // since 0 requires an extra char and is common, we want to avoid it.
		s = (short)j;
		c = (char)j;

		if (c == j) // j < 256, just need 1 char to store it.
		{
			encode_addchar(c);
		}
		else if (s == j) // can fit in 2 bytes (short)
		{
			encode_addmarker(MARKER_CHAR_SHORT);
			encode_addchar(s>>8);
			encode_addchar(s);
		}
		else // full 32bit integer (4 bytes)
		{
			encode_addmarker(MARKER_CHAR_LONG);
			encode_addchar(j>>24);
			encode_addchar(j>>16);
			encode_addchar(j>>8);
			encode_addchar(j);
		}
	}
	encode_end();
}

// in: string encoded with encode_unsigned
// out: integer array
// max: max number of elements in int array.
// return: number of integers decoded
int decode_unsigned(const unsigned char *in, unsigned int *out, int max)
{
	int i = 0;

	if (!in || !out || !max)
		return 0;

	decode_ptr = in;

	while (*decode_ptr && i < max)
	{
		out[i] = decode_char_unsigned() - 1; // - 1 is to compensate for 1 added in encode_unsigned
		i++;
	}

	return i;
}


#define MAX_DECODE_ARRAY 128
static int index_array[MAX_DECODE_ARRAY];
static int num_elements;
static int current_element;

static void translate_string (char *out_str, const char *in_str)
{
	// Translate string...
	// %s = index to event string
	// %i = index to item string
	// %n = index to name string
	// %% = %
	// %c = single charcter (like in printf)
	while(*in_str)
	{
		while(*in_str && *in_str != '%')
		{
			*out_str = *in_str;
			in_str++;
			out_str++;
		}

		if(*in_str)
		{
			in_str++;

			if (current_element < num_elements)
			{
				switch(*in_str)
				{
				case 's': // event string
					translate_string(out_str, cl.configstrings[CS_EVENTS+index_array[current_element++]]);
					break;
				case 'i': // item
					strcpy(out_str, item_from_index(index_array[current_element++]));
					out_str += strlen(out_str);
					break;
				case 'n': // name
					strcpy(out_str, name_from_index(index_array[current_element++]));
					out_str += strlen(out_str);
					break;
				case '%': // %
					*out_str = '%';
					out_str++;
					break;
				case 'c': // character
					*out_str = index_array[current_element++];
					out_str++;
					break;
				default: // unknown? (shouldn't happen)
					*out_str = '?';
					out_str++;
					break;
				}

				in_str++;
			}
		}
	}

	*out_str = 0; // terminate string
}

#define MAX_EVENT_STRINGS 8
static char event_strings[MAX_EVENT_STRINGS][MAX_QPATH]; // displayed briefly just above hud
static int event_string_time[MAX_EVENT_STRINGS];
static int startpos = 0;

static void event_print (char *s)
{
	startpos--;
	if(startpos<0)
		startpos = MAX_EVENT_STRINGS - 1;

	strcpy(event_strings[startpos], s);
	event_string_time[startpos] = curtime;
}

void CL_DrawEventStrings (void)
{
	int i, j;
	float alpha;

	for (i=0, j=startpos; i<MAX_EVENT_STRINGS; i++, j++, j%=MAX_EVENT_STRINGS)
	{
		if(*event_strings[j])
		{
			alpha = (4000 - (curtime-event_string_time[j])) / 3000.0f;

			if(alpha > 1.0f)
				alpha = 1.0f;

			if(alpha < 0.05f)
			{
				event_strings[j][0] = '\0';
				break;
			}

			re.DrawStringAlpha(viddef.width/2-4*hudscale*strlen_noformat(event_strings[j]),
				viddef.height/2+(16+i*8)*hudscale, event_strings[j], alpha);
		}
		else
		{
			break;
		}
	}
}

void CL_ParsePrintEvent (const char *str) // jitevents
{
	unsigned int event;
	char event_text[128];

	num_elements = decode_unsigned(str, index_array, MAX_DECODE_ARRAY);

	if (num_elements < 1)
		return;

	event = index_array[0];

	if (num_elements > 1)
	{
		current_element = 2;
		translate_string(event_text, cl.configstrings[CS_EVENTS+index_array[1]]);
	}
	else
	{
		sprintf(event_text, "(null)");
	}
	
	// handle special cases of events here...
	switch(event)
	{
	case EVENT_ENTER:
		Com_Printf("%s\n", event_text);
		if (num_elements > 1)
		{
			cl_scores_clear(index_array[2]);
			cl_scores_setinuse(index_array[2], true);
			cl_scores_setteam(index_array[2], (num_elements > 3) ? index_array[3] : 0);
		}
		break;
	case EVENT_JOIN:
		Com_Printf("%s\n", event_text);
		if (num_elements > 3)
			cl_scores_setteam(index_array[2], index_array[3]);
		break;
	case EVENT_ROUNDOVER:
		Com_Printf("%s\n", event_text);
		event_print(event_text);
		break;
	case EVENT_ADMINKILL: // jitodo - fix all these offsets
		Com_Printf("%s\n", event_text);
		if (num_elements > 2 && index_array[2] == cl.playernum)
		{
			if (num_elements > 3)
				sprintf(event_text, "Admin (%s) killed you.", name_from_index(index_array[3]));
			else
				sprintf(event_text, "Admin killed you.");
			event_print(event_text);
		}
		break;
	case EVENT_KILL: // jitodo - fix all these offsets
		Com_Printf("%s\n", event_text);

		// Update scoreboard:
		if (current_element < num_elements)
			cl_scores_setkills(index_array[2], index_array[current_element++]);
		if (current_element < num_elements)
			cl_scores_setdeaths(index_array[4], index_array[current_element++]);

		if (num_elements < 8)
			break;
		
		if (index_array[2] == cl.playernum)
			sprintf(event_text, "You eliminated %s (%s).",
				name_from_index(index_array[4]), item_from_index(index_array[5]));
		else if (index_array[4] == cl.playernum)
			sprintf(event_text, "%s (%s) eliminated you.",
				name_from_index(index_array[2]), item_from_index(index_array[3]));
		else
			break;

		event_print(event_text);
		break;
	case EVENT_SUICIDE: // jitodo - fix all these offsets
		Com_Printf("%s\n", event_text);

		if (num_elements < 3)
			break;

		// Scoreboard:
		if (current_element < num_elements)
			cl_scores_setdeaths(index_array[2], index_array[current_element++]);

		if (index_array[2] == cl.playernum)
		{
			sprintf(event_text, "You eliminated yourself!");
			event_print(event_text);
		}

		break;
	case EVENT_FFIRE: // jitodo - fix all these offsets
		Com_Printf("%s\n", event_text);

		if (num_elements < 4)
			break;

		if (index_array[2] == cl.playernum)
			sprintf(event_text, "%cYou eliminated your teammate!!", CHAR_ITALICS);
		else if (index_array[3] == cl.playernum)
			sprintf(event_text, "Your teammate (%s) eliminated you.", name_from_index(index_array[2]));
		else
			break;

		event_print(event_text);
		break;
	default:
		Com_Printf("%s\n", event_text);
		break;
	}
}

