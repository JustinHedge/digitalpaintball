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
#include "bot_manager.h"
#include "bot_observe.h"

//#define MAX_WAYPOINTS 2048

#define MAX_WAYPOINTS 100 // small number to test - todo: use larger value when done
#define MAX_WAYPOINT_CONNECTIONS 6 // waypoint will connect with 6 closest possible nodes
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
	int			usage_weights[MAX_WAYPOINTS]; // how often players touch this waypoint.
	int			debug_ids[MAX_WAYPOINTS];
	int			num_points;
} bot_waypoints_t;


bot_waypoints_t g_bot_waypoints;


void PositionToColor (const vec3_t pos, float *r, float *g, float *b)
{
	int some_val = (int)pos[0] + (int)pos[1] * 2 + (int)pos[2] * 3;
	int ri, gi, bi;

	some_val *= 4623460215; // abritrary number
	ri = some_val & 0xFF;
	gi = (some_val >> 8) & 0xFF;
	bi = (some_val >> 16) & 0xFF;

	*r = (float)ri / 255.0f;
	*g = (float)gi / 255.0f;
	*b = (float)bi / 255.0f;
}


void WeightToColor (int waypoint_index, float *r, float *g, float *b)
{
	*r = 1.0f;
	*b = 1.0f;
	*g = (float)g_bot_waypoints.usage_weights[waypoint_index] / 10.0f;

	if (*g > 1.0f)
		*g = 1.0f;
}


void BotInitWaypoints (void)
{
	memset(&g_bot_waypoints, 0, sizeof(g_bot_waypoints));
	memset(&g_bot_waypoints.debug_ids, -1, sizeof(g_bot_waypoints.debug_ids));
	memset(&g_bot_waypoints.connections, -1, sizeof(g_bot_waypoints.connections));
}

// Helper function to find the X closest points, given a squared distance
void TryInsertCloserPoint (const edict_t *ent, int *closest_points, float *closest_points_dist_sq, const vec3_t pos1, const vec3_t pos2, int point_index)
{
	int closest_point_index;
	float old_dist_sq;
	vec3_t diff_vec;
	float dist_sq;

	VectorSubtract(pos1, pos2, diff_vec);
	dist_sq = VectorLengthSquared(diff_vec);

	for (closest_point_index = 0; closest_point_index < MAX_WAYPOINT_CONNECTIONS; ++closest_point_index)
	{
		old_dist_sq = closest_points_dist_sq[closest_point_index];

		if (dist_sq < old_dist_sq && dist_sq < MAX_WAYPOINT_DIST_SQ)
		{
			trace_t trace;
			qboolean hit_something = false;

			trace = bi.trace(pos1, ent->mins, ent->maxs, pos2, ent, MASK_PLAYERSOLID);

			if (trace.fraction < 1.0f)
				hit_something = true;

			if (trace.ent && trace.ent->client)
				hit_something = false; // we need a better way to do this, as there could be a solid wall behind the client, but we don't want other clients blocking paths...

			// If we hit something, move the origin points up slightly to see if it's something small we can step over.
			if (hit_something)
			{
				vec3_t step_pos1, step_pos2;

				VectorCopy(pos1, step_pos1);
				VectorCopy(pos2, step_pos2);
				step_pos1[2] += 16.0f;
				step_pos2[2] += 1.0f; // keep some directionality on these.  For example, if I'm on a ledge, I might have a clear shot down, but I won't necessarily be able to get up.

				trace = bi.trace(step_pos1, ent->mins, ent->maxs, step_pos2, ent, MASK_PLAYERSOLID);

				if (trace.fraction == 1.0f)
					hit_something = false;
			}

			if (!hit_something)
			{
				int shift_index;

				// Shift all the previous points down and insert the new one.
				for (shift_index = MAX_WAYPOINT_CONNECTIONS - 1; shift_index > closest_point_index; --shift_index)
				{
					closest_points_dist_sq[shift_index] = closest_points_dist_sq[shift_index - 1];
					closest_points[shift_index] = closest_points[shift_index - 1];
				}

				closest_points_dist_sq[closest_point_index] = dist_sq;
				closest_points[closest_point_index] = point_index;
				return;
			}
		}
	}
}


void BotWaypointUpdateConnections (int waypoint_index, const edict_t *ent)
{
	int i;
	int closest_points[MAX_WAYPOINT_CONNECTIONS];
	float closest_points_dist_sq[MAX_WAYPOINT_CONNECTIONS];

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
			TryInsertCloserPoint(ent, closest_points, closest_points_dist_sq, g_bot_waypoints.positions[waypoint_index], g_bot_waypoints.positions[i], i);
		}
	}

	for (i = 0; i < MAX_WAYPOINT_CONNECTIONS; ++i)
	{
		int debug_id = g_bot_waypoints.connections[waypoint_index].debug_ids[i];

		g_bot_waypoints.connections[waypoint_index].nodes[i] = closest_points[i];
		g_bot_waypoints.connections[waypoint_index].weights[i] = 1.0f / Q_rsqrt(closest_points_dist_sq[i]); // just store distance for now.  We may try to convert this to a time value later.

		if (g_bot_waypoints.connections[waypoint_index].nodes[i] >= 0)
		{
			float r, g, b;

			WeightToColor(waypoint_index, &r, &g, &b);
			g_bot_waypoints.connections[waypoint_index].debug_ids[i] = DrawDebugLine(g_bot_waypoints.positions[waypoint_index], g_bot_waypoints.positions[g_bot_waypoints.connections[waypoint_index].nodes[i]], r, g, b, 99999999.9f, debug_id);
		}
		else if (debug_id >= 0)
		{
			// "draw" a line with 0 time so it clears out the old debug line.
			DrawDebugLine(g_bot_waypoints.positions[waypoint_index], g_bot_waypoints.positions[waypoint_index], 1, .5, 1, 0, debug_id);
			g_bot_waypoints.connections[waypoint_index].debug_ids[i] = -1;
		}
	}
}

#define MAX_WAYPOINT_USAGE_WEIGHT 255

void BotAddWaypointAtIndex (const edict_t *ent, int waypoint_index, const vec3_t pos, qboolean replacing, int usage_weight)
{
	int i;
	int debug_sphere_id = g_bot_waypoints.debug_ids[waypoint_index]; // This value should be initialized to -1, so a new ID will be generated when the debug sphere is drawn

	if (usage_weight > MAX_WAYPOINT_USAGE_WEIGHT)
		usage_weight = MAX_WAYPOINT_USAGE_WEIGHT;

	g_bot_waypoints.usage_weights[waypoint_index] = usage_weight;
	VectorCopy(pos, g_bot_waypoints.positions[waypoint_index]);

	if (waypoint_index >= g_bot_waypoints.num_points)
		g_bot_waypoints.num_points = waypoint_index + 1;

	if (replacing)
	{
		for (i = 0; i < g_bot_waypoints.num_points; ++i)
		{
			int j;

			 for (j = 0; j < MAX_WAYPOINT_CONNECTIONS; ++j)
			 {
				 if (g_bot_waypoints.connections[i].nodes[j] == waypoint_index)
				 {
					 BotWaypointUpdateConnections(i, ent);
				 }
			 }
		}
	}

	{
		float r, g, b;

		WeightToColor(waypoint_index, &r, &g, &b);
		g_bot_waypoints.debug_ids[waypoint_index] = DrawDebugSphere(pos, 5.0f, r, g, b, 99999.0f, debug_sphere_id);
	}

	BotWaypointUpdateConnections(waypoint_index, ent);

	// Since we just added a new waypoint, update all the ones it connects to, since they should probably connect to it as well.
	for (i = 0; i < MAX_WAYPOINT_CONNECTIONS; ++i)
	{
		if (g_bot_waypoints.connections[waypoint_index].nodes[i] >= 0)
			BotWaypointUpdateConnections(g_bot_waypoints.connections[waypoint_index].nodes[i], ent);
	}
}


#define RANDOM_WAYPOINT_REMOVAL_POOL_SIZE 4 // When randomly removing a waypoint, pick between 4 at random, and see which is "best" to remove.


void BotTryAddWaypoint (const edict_t *ent, const vec3_t pos)
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
		{
			int waypoint_usage = g_bot_waypoints.usage_weights[i];

			BotAddWaypointAtIndex(ent, i, pos, true, waypoint_usage + 1);
			return;
		}
	}

	if (g_bot_waypoints.num_points < MAX_WAYPOINTS)
	{
		BotAddWaypointAtIndex(ent, g_bot_waypoints.num_points, pos, false, 1);
	}
	else
	{
		int waypoint_connection_count = 0;
		int best_waypoint_index = -1;
		float total_weight;
		float lowest_total_weight = FLT_MAX;
		int i, j;

		// Randomly try a few times to find a waypoint that has max connections and the lowest total weight of connections.
		// This means it's likely surrounded by nearby nodes and can be removed without affecting much
		for (i = 0; i < RANDOM_WAYPOINT_REMOVAL_POOL_SIZE; ++i)
		{
			new_waypoint_index = (int)nu_rand(MAX_WAYPOINTS);
			waypoint_connection_count = 0;
			total_weight = 0.0f;

			for (j = 0; j < MAX_WAYPOINT_CONNECTIONS; ++j)
			{
				if (g_bot_waypoints.connections[new_waypoint_index].nodes[j] >= 0)
				{
					waypoint_connection_count++;
					total_weight += g_bot_waypoints.connections[new_waypoint_index].weights[j];
				}
			}

			if (waypoint_connection_count == MAX_WAYPOINT_CONNECTIONS && total_weight < lowest_total_weight)
			{
				best_waypoint_index = new_waypoint_index;
				lowest_total_weight = total_weight;
			}
		}

		// If we didn't find a good candidate, the new_waypoint_index will just be whatever we last tried.
		if (best_waypoint_index >= 0)
			new_waypoint_index = best_waypoint_index;

		BotAddWaypointAtIndex(ent, new_waypoint_index, pos, true, 1);
	}
}

#define WAYPOINT_ADD_TIME 1.0f // if this much time has elapsed since the last time a player has added a waypoint, try adding another.
#define WAYPOINT_ADD_DIST 128.0f // if a player has moved at least this distance after adding the last waypoint, try adding another.

void BotAddPotentialWaypointFromPmove (player_observation_t *observation, const edict_t *ent, const pmove_t *pm)
{
	if (pm->groundentity) // only add waypoints on the ground
	{
		vec3_t diff;
		float dist_sq;

		VectorSubtract(ent->s.origin, observation->last_waypoint_pos, diff);
		dist_sq = VectorLengthSquared(diff);

		if (dist_sq >= WAYPOINT_ADD_DIST * WAYPOINT_ADD_DIST || bots.game_time - observation->last_waypoint_time >= WAYPOINT_ADD_TIME)
		{
			BotTryAddWaypoint(ent, ent->s.origin);
			VectorCopy(ent->s.origin, observation->last_waypoint_pos);
			observation->last_waypoint_time = bots.game_time;
		}
	}
}
