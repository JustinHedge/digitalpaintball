/*
Copyright (c) 2014 Nathan "jitspoe" Wulf, Digital Paint

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


#include <float.h>

#include "bot_main.h"
#include "../game/game.h"
#include "bot_debug.h"
#include "bot_observe.h"

//#define MAX_WAYPOINTS 2048

#define MAX_WAYPOINTS 100 // small number to test - todo: use larger value when done
#define MAX_WAYPOINT_CONNECTIONS 4 // waypoint will connect with 4 closest possible nodes
#define MAX_WAYPOINT_DIST 256.0f
#define MAX_WAYPOINT_DIST_SQ 65536.0f
#define MIN_WAYPOINT_DIFF 32.0f
#define MIN_WAYPOINT_DIFF_SQ 1024.0f

//typedef short shortpos_t[3]; // net positions are compressed to shorts at 1/8 unit, anyway, so might as well store waypoints at that resolution to save memory.


typedef struct {
	float	weights[MAX_WAYPOINT_CONNECTIONS];
	int		nodes[MAX_WAYPOINT_CONNECTIONS];
	int		debug_ids[MAX_WAYPOINT_CONNECTIONS];
} bot_waypoint_connection_t;


typedef struct {
	vec3_t		positions[MAX_WAYPOINTS];
	bot_waypoint_connection_t connections[MAX_WAYPOINTS];
	//shortpos_t	pos[MAX_WAYPOINTS];
	int			debug_ids[MAX_WAYPOINTS];
	int			num_points;
} bot_waypoints_t;


bot_waypoints_t g_bot_waypoints;


void BotInitWaypoints (void)
{
	memset(&g_bot_waypoints, 0, sizeof(g_bot_waypoints));
	memset(&g_bot_waypoints.debug_ids, -1, sizeof(g_bot_waypoints.debug_ids));
	memset(&g_bot_waypoints.connections, -1, sizeof(g_bot_waypoints.connections));
}

// Helper function to find the X closest points, given a squared distance
void TryInsertCloserPoint (int *closest_points, float *closest_points_dist_sq, float dist_sq, int point_index)
{
	int closest_point_index;
	float old_dist_sq;
	int old_point_index;

	for (closest_point_index = 0; closest_point_index < MAX_WAYPOINT_CONNECTIONS; ++closest_point_index)
	{
		old_dist_sq = closest_points_dist_sq[closest_point_index];

		if (dist_sq < old_dist_sq && dist_sq < MAX_WAYPOINT_DIST_SQ)
		{
			// todo: collision test
			old_point_index = closest_points[closest_point_index];
			closest_points_dist_sq[closest_point_index] = dist_sq;
			closest_points[closest_point_index] = point_index;
			TryInsertCloserPoint(closest_points, closest_points_dist_sq, old_dist_sq, old_point_index); // we've replaced this one, but it might still be a shorter distance than other waypoints in the array, so see if one of those can be replaced...
			return;
		}
	}
}


void BotWaypointUpdateConnections (int waypoint_index)
{
	int i;
	int closest_points[MAX_WAYPOINT_CONNECTIONS];
	float closest_points_dist_sq[MAX_WAYPOINT_CONNECTIONS];
	vec3_t diff_vec;
	float dist_sq;

	// Initialize, to make sure we don't have any invalid data set from a previous use of the waypoint (in case it was replaced)
	for (i = 0; i < MAX_WAYPOINT_CONNECTIONS; ++i)
	{
		g_bot_waypoints.connections[waypoint_index].nodes[i] = -1;
		g_bot_waypoints.connections[waypoint_index].weights[i] = FLT_MAX;
		closest_points[i] = -1;
		closest_points_dist_sq[i] = FLT_MAX;
	}

	// Loop through all the existing waypoints and find the closest positions.
	for (i = 0; i < g_bot_waypoints.num_points; ++i)
	{
		if (i != waypoint_index) // ignore self
		{
			VectorSubtract(g_bot_waypoints.positions[waypoint_index], g_bot_waypoints.positions[i], diff_vec);
			dist_sq = VectorLengthSquared(diff_vec);
			TryInsertCloserPoint(closest_points, closest_points_dist_sq, dist_sq, i);
		}
	}

	for (i = 0; i < MAX_WAYPOINT_CONNECTIONS; ++i)
	{
		int debug_id = g_bot_waypoints.connections[waypoint_index].debug_ids[i];

		g_bot_waypoints.connections[waypoint_index].nodes[i] = closest_points[i];
		g_bot_waypoints.connections[waypoint_index].weights[i] = 1.0f / Q_rsqrt(closest_points_dist_sq[i]); // just store distance for now.  We may try to convert this to a time value later.

		if (g_bot_waypoints.connections[waypoint_index].nodes[i] >= 0)
		{
			g_bot_waypoints.connections[waypoint_index].debug_ids[i] = DrawDebugLine(g_bot_waypoints.positions[waypoint_index], g_bot_waypoints.positions[g_bot_waypoints.connections[waypoint_index].nodes[i]], 1, .5, 1, 99999999.9f, debug_id);
		}
		else if (debug_id >= 0)
		{
			// "draw" a line with 0 time so it clears out the old debug line.
			DrawDebugLine(g_bot_waypoints.positions[waypoint_index], g_bot_waypoints.positions[waypoint_index], 1, .5, 1, 0, debug_id);
			g_bot_waypoints.connections[waypoint_index].debug_ids[i] = -1;
		}
	}
}


void BotAddWaypointAtIndex (int waypoint_index, const vec3_t pos, qboolean replacing)
{
	int debug_sphere_id = g_bot_waypoints.debug_ids[waypoint_index]; // This value should be initialized to -1, so a new ID will be generated when the debug sphere is drawn

	VectorCopy(pos, g_bot_waypoints.positions[waypoint_index]);

	if (waypoint_index >= g_bot_waypoints.num_points)
		g_bot_waypoints.num_points = waypoint_index + 1;

	if (replacing)
	{
		int i;

		for (i = 0; i < g_bot_waypoints.num_points; ++i)
		{
			int j;

			 for (j = 0; j < MAX_WAYPOINT_CONNECTIONS; ++j)
			 {
				 if (g_bot_waypoints.connections[i].nodes[j] == waypoint_index)
				 {
					 BotWaypointUpdateConnections(i);
				 }
			 }
		}
	}

	g_bot_waypoints.debug_ids[waypoint_index] = DrawDebugSphere(pos, 6, 1, 1, 1, 99999, debug_sphere_id);
	BotWaypointUpdateConnections(waypoint_index);
}

void BotTryAddWaypoint (const vec3_t pos)
{
	vec3_t diff_vec;
	float dist_sq;
	int new_waypoint_index;
	int i;

	// Make sure we're more than the min distance away from existing waypoints before inserting a new one.
	// While we're looping through, might as well generate a list of the closest waypoints as well
	for (i = 0; i < g_bot_waypoints.num_points; ++i)
	{
		VectorSubtract(g_bot_waypoints.positions[i], pos, diff_vec);
		dist_sq = VectorLengthSquared(diff_vec);

		if (dist_sq < MIN_WAYPOINT_DIFF_SQ)
			return;
	}

	if (g_bot_waypoints.num_points < MAX_WAYPOINTS - 1)
	{
		BotAddWaypointAtIndex(g_bot_waypoints.num_points, pos, false);
	}
	else
	{
		new_waypoint_index = (int)nu_rand(MAX_WAYPOINTS);
		BotAddWaypointAtIndex(new_waypoint_index, pos, true);
	}
}


void BotAddPotentialWaypointFromPmove (player_observation_t *observation, const edict_t *ent, const pmove_t *pm)
{
	if (pm->groundentity) // only add waypoints on the ground
	{
		BotTryAddWaypoint(ent->s.origin); // todo - throttle this with a timer or something
		/*
		vec3_t diff;
		vec_t len_sq;
		trace_t trace;
		qboolean hit_something = false;

		VectorSubtract(ent->s.origin, observation->last_waypoint_pos, diff);
		len_sq = VectorLengthSquared(diff);

		trace = bi.trace(ent->s.origin, pm->mins, pm->maxs, observation->last_waypoint_pos,  MASK_PLAYERSOLID);

		if (trace.fraction < 1.0f)
			hit_something = true;

		//if (trace.ent && trace.ent->client)
		// todo: figure out how to ignore paths blocked by players.

		// Player width is 32 units, so if we're inside of that range, don't even bother checking anything else.
		if (len_sq >= 128.0f * 128.0f || hit_something)
		{
			BotAddWaypoint(ent->s.origin, observation->last_waypoint_pos);
		}*/
	}
}
