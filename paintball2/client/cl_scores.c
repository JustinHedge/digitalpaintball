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
// cl_scores.c -- Client side scoreboard.

#include "client.h"

typedef struct cl_score_s {
	int ping;
	int kills;
	int deaths;
	int grabs;
	int caps;
	char team;
	// jitodo -- shots fired w/each gun, etc.
	qboolean inuse;
} cl_score_t;

static cl_score_t cl_scores[MAX_CLIENTS];
static qboolean cl_scores_modified;

char **cl_scores_nums;
char **cl_scores_info;
int cl_scores_count;

int splat(int teamnum) 
{
	if (teamnum < 1 || teamnum > 4)
		return 154;

	//switch (teamindex[teamnum-1] + 1) 
	switch (cl.configstrings[CS_TEAMINDEXES][teamnum-1]) 
	{
	case 1:
		return 15;
	case 2:
		return 14;
	case 3:
		return 5;
	case 4:
		return 28;
	default:
		return ' ';
	}
}

void init_cl_scores (void)
{
	int i;

	// allocating memory for the scoreboard...
	cl_scores_nums = Z_Malloc(sizeof(char*)*MAX_CLIENTS);
	cl_scores_info = Z_Malloc(sizeof(char*)*MAX_CLIENTS);

	for (i=0; i<MAX_CLIENTS; i++)
	{
		cl_scores_nums[i] = Z_Malloc(sizeof(char)*64);
		cl_scores_info[i] = Z_Malloc(sizeof(char)*5);
	}

	memset(cl_scores, 0, sizeof(cl_scores));

	cl_scores_modified = true;
}

void shutdown_cl_scores (void)
{
	Z_Free(cl_scores_nums);
	Z_Free(cl_scores_info);
}

void cl_scores_setping (int client, int ping)
{
	cl_scores[client].ping = ping;
	cl_scores_modified = true;
}

void cl_scores_setkills (int client, int kills)
{
	cl_scores[client].kills = kills;
	cl_scores_modified = true;
}

void cl_scores_setdeaths (int client, int deaths)
{
	cl_scores[client].deaths = deaths;
	cl_scores_modified = true;
}

void cl_scores_setgrabs (int client, int grabs)
{
	cl_scores[client].grabs = grabs;
	cl_scores_modified = true;
}

void cl_scores_setcaps (int client, int caps)
{
	cl_scores[client].caps = caps;
	cl_scores_modified = true;
}

void cl_scores_setteam (int client, char team)
{
	cl_scores[client].team = team;
	cl_scores_modified = true;
}

void cl_scores_setinuse (int client, qboolean inuse) // jitodo - call this on player enter and unset on quit.
{
	cl_scores[client].inuse = inuse;
	cl_scores_modified = true;
}

void cl_scores_clear (int client)
{
	memset(&cl_scores[client], 0, sizeof(cl_score_t));
	cl_scores_modified = true;
}

// put the scores into readable strings for the widget to draw
void cl_scores_prep_select_widget (void)
{
	int i;

	if (!cl_scores_modified)
		return;

	cl_scores_count = 0;

	for (i=0; i<MAX_CLIENTS; i++)
	{
		if (cl_scores[i].inuse)
		{
			Com_sprintf(cl_scores_nums[cl_scores_count], 4, "%d", i);
			Com_sprintf(cl_scores_info[cl_scores_count], 63, "%c%s %d %d %d %d %d", 
				splat(cl_scores[i].team), name_from_index(i),
				cl_scores[i].ping, cl_scores[i].kills,
				cl_scores[i].deaths, cl_scores[i].grabs,
				cl_scores[i].caps);
			cl_scores_count++;
		}
	}

	cl_scores_modified = false;
}


void CL_Score_f (void) // jitodo jitscores -- client-side scoreboard
{
	int i;

	cl_scores_prep_select_widget();

	for (i=0; i<cl_scores_count; i++)
	{
		Com_Printf("%s\n", cl_scores_info[i]);
	}
}

#define MAX_DECODE_ARRAY 256
static unsigned int temp_array[MAX_DECODE_ARRAY];
void CL_ParesScoreData (const unsigned char *data) // jitscores
{
	unsigned int i, count;

	count = decode_unsigned(data, temp_array, MAX_DECODE_ARRAY);

	i = temp_array[0];

	if (i > 255)
		return;

	cl_scores[i].ping = temp_array[1];
	cl_scores[i].kills = temp_array[2];
	cl_scores[i].deaths = temp_array[3];
	cl_scores[i].grabs = temp_array[4];
	cl_scores[i].caps = temp_array[5];
	cl_scores[i].team = temp_array[6];
}
// jitscores ===
