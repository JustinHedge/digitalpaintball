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

char cl_scores_nums[MAX_CLIENTS][4];
char cl_scores_info[MAX_CLIENTS][64];
int cl_scores_count;

typedef struct cl_score_s {
	char name[32];
	int ping;
	int kills;
	int deaths;
	int grabs;
	int caps;
	char team;
	// jitodo -- shots fired w/each gun, etc.
	qboolean modified;
	qboolean inuse;
} cl_score_t;

static cl_score_t cl_scores[MAX_CLIENTS];



void init_cl_scores() // jitodo - call this on map init
{
	memset(cl_scores, 0, sizeof(cl_scores));
}

void cl_scores_setping(int client, int ping)
{
	cl_scores[client].ping = ping;
}

void cl_scores_setkills(int client, int kills)
{
	cl_scores[client].kills = kills;
}

void cl_scores_setdeaths(int client, int deaths)
{
	cl_scores[client].deaths = deaths;
}

void cl_scores_setgrabs(int client, int grabs)
{
	cl_scores[client].grabs = grabs;
}

void cl_scores_setcaps(int client, int caps)
{
	cl_scores[client].caps = caps;
}

void cl_scores_setteam(int client, char team)
{
	cl_scores[client].team = team;
}

void cl_scores_setinuse(int client, qboolean inuse)
{
	cl_scores[client].inuse = inuse;
}

// put the scores into readable strings for the widget to draw
void cl_scores_prep_select_widget()
{
	int i;

	cl_scores_count = 0;

	for (i=0; i<MAX_CLIENTS; i++)
	{
		if (cl_scores[i].inuse)
		{
			sprintf(cl_scores_nums[cl_scores_count], "%d", i);
			sprintf(cl_scores_info[cl_scores_count], "%c %s %d %d %d %d %d", 
				cl_scores[i].team, cl_scores[i].name,
				cl_scores[i].ping, cl_scores[i].kills,
				cl_scores[i].deaths, cl_scores[i].grabs,
				cl_scores[i].caps);
			cl_scores_count++;
		}
	}
}


void CL_Score_f (void) // jitodo jitscores -- client-side scoreboard
{
	int i;

	for (i=0; i<cl_scores_count; i++)
	{
		Com_Printf("%s\n", cl_scores_info[i]);
	}
	//clientinfo_t *ci;

	//for(i=0; i<MAX_CLIENTS; i++)
	//{
	//	ci = &cl.clientinfo[i];
	//	if(*ci->name)
	//		Com_Printf("%d: %s %s\n", i, ci->name, ci->cinfo);
	//}
}
// jitscores ===
