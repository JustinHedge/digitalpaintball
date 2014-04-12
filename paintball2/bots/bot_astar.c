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
#include "bot_waypoints.h"
#include "bot_debug.h"
#include "../game/game.h"

typedef enum {
	ASTAR_UNSET = 0,
	ASTAR_OPEN,
	ASTAR_CLOSED
} astar_node_status_t;

typedef struct {
	astar_node_status_t	node_status;
	int					parent_node;
	float				g;
	int					h;
} astar_node_t;

astar_node_t g_astar_nodes[MAX_WAYPOINTS]; // note: this is clearly not threadsafe for multiple searches.  If we want to multithread pathfinding, we'll need local copies for each thread.
int g_astar_list[MAX_WAYPOINTS];
int g_astar_list_size;


qboolean AStarFindPathFromNodeIndexes (int start_node, int end_node, bot_waypoint_path_t *path)
{
	int current_node = start_node;
	int connection_index;
	int list_index;
	
	bi.dprintf("AStarFindPathFromNodeIndexes\n");

	if (path)
	{
		path->num_points = 0;
		path->current_node = 0;
	}

	if (end_node < 0 || end_node >= MAX_WAYPOINTS)
		return false;

	if (start_node < 0 || start_node >= MAX_WAYPOINTS)
		return false;

	g_astar_list_size = 0;
	memset(g_astar_nodes, 0, sizeof(g_astar_nodes));

	while (g_astar_nodes[end_node].node_status == ASTAR_UNSET)
	{
		assert(current_node < MAX_WAYPOINTS);

		// Put current node on closed list
		g_astar_nodes[current_node].node_status = ASTAR_CLOSED; // todo: opt: remove from g_astar_list.
		//DrawDebugSphere(g_bot_waypoints.positions[current_node], 7, 1, 1, 1, 5, -1);

		// Put adjacent nodes on open list
		for (connection_index = 0; connection_index < MAX_WAYPOINT_CONNECTIONS; ++connection_index)
		{
			int connected_node = g_bot_waypoints.connections[current_node].nodes[connection_index];

			if (connected_node >= 0 && g_astar_nodes[connected_node].node_status != ASTAR_CLOSED)
			{
				float dist = g_bot_waypoints.connections[current_node].weights[connection_index];
				float new_g = g_astar_nodes[current_node].g + dist;

				if (g_astar_nodes[connected_node].node_status == ASTAR_UNSET)
				{
					float dist_sq = VectorSquareDistance(g_bot_waypoints.positions[end_node], g_bot_waypoints.positions[connected_node]);

					g_astar_nodes[connected_node].node_status = ASTAR_OPEN;
					g_astar_nodes[connected_node].parent_node = current_node;
					g_astar_nodes[connected_node].g = new_g;
					g_astar_nodes[connected_node].h = dist_sq ? 1.0f / Q_rsqrt(dist_sq) : 0.0f;
					assert(g_astar_list_size < MAX_WAYPOINTS);
					g_astar_list[g_astar_list_size] = connected_node;
					++g_astar_list_size;
					//DrawDebugSphere(g_bot_waypoints.positions[connected_node], 8, 1, 1, .5, 5, -1);
				}
				else if (new_g < g_astar_nodes[connected_node].g)
				{
					g_astar_nodes[connected_node].parent_node = current_node;
					g_astar_nodes[connected_node].g = new_g;
				}
			}
		}

		// Find the best node to look at next.
		{
			float best_val = FLT_MAX;
			int best_node = -1;

			for (list_index = 0; list_index < g_astar_list_size; ++list_index)
			{
				int node = g_astar_list[list_index];

				if (g_astar_nodes[node].node_status == ASTAR_OPEN)
				{
					float val = g_astar_nodes[node].g + g_astar_nodes[node].h;

					if (val < best_val)
					{
						best_val = val;
						best_node = node;
					}
				}
			}

			current_node = best_node;
		}
		
		if (current_node == -1)
			return false;
	}

	// backtrack and create path.
	{
		int num_points = 1;
		int i;

		current_node = end_node;

		while (current_node != start_node)
		{
			//DrawDebugSphere(g_bot_waypoints.positions[current_node], 10.0f, 0.0f, 1.0f, 0.2f, 5.0f, -1);
			current_node = g_astar_nodes[current_node].parent_node;
			++num_points;
		}

		if (path)
		{
			path->num_points = num_points;
			current_node = end_node;

			for (i = num_points - 1; i >= 0; --i)
			{
				path->nodes[i] = current_node;
				current_node = g_astar_nodes[current_node].parent_node;
			}
		}
	}

	return true;
}


// TODO: Trace check, as the closest waypoint may be behind a wall.
int ClosestWaypointToPosition (vec3_t pos)
{
	int i;
	float best_dist = FLT_MAX;
	float dist;
	int best_node = -1;

	for (i = 0; i < g_bot_waypoints.num_points; ++i)
	{
		dist = VectorSquareDistance(pos, g_bot_waypoints.positions[i]);

		if (dist < best_dist)
		{
			best_dist = dist;
			best_node = i;
		}
	}

	return best_node;
}


static int g_astar_debug_start = -1;

void AStarDebugStartPoint (vec3_t pos)
{
	g_astar_debug_start = ClosestWaypointToPosition(pos);
}


void AStarDebugEndPoint (vec3_t pos)
{
	int debug_end = ClosestWaypointToPosition(pos);

	AStarFindPathFromNodeIndexes(g_astar_debug_start, debug_end, NULL);
}


qboolean AStarFindPathFromPositions (vec3_t start_pos, vec3_t end_pos, bot_waypoint_path_t *path)
{
	return AStarFindPathFromNodeIndexes(ClosestWaypointToPosition(start_pos), ClosestWaypointToPosition(end_pos), path);
}


qboolean AStarFindPathFromEntityToPos (edict_t *ent, vec3_t end_pos, bot_waypoint_path_t *path)
{
	int closest_start_node = ClosestWaypointToPosition(ent->s.origin);
	qboolean need_new_point = false;

	if (closest_start_node < 0)
	{
		need_new_point = true;
	}
	else
	{
		vec3_t diff_vec;
		vec_t dist_sq;

		VectorSubtract(ent->s.origin, g_bot_waypoints.positions[closest_start_node], diff_vec);
		dist_sq = VectorLengthSquared(diff_vec);

		if (dist_sq > MIN_WAYPOINT_DIFF_SQ)
		{
			if (BotCanReachPosition(ent, ent->s.origin, end_pos, NULL))
			{
				need_new_point = true;
			}
		}
	}

	if (need_new_point)
	{
		// No reachable node nearby, try adding one
		BotTryAddWaypoint(ent, ent->s.origin);

		return AStarFindPathFromPositions(ent->s.old_origin, end_pos, path);
	}
	else
	{
		return AStarFindPathFromNodeIndexes(closest_start_node, ClosestWaypointToPosition(end_pos), path);
	}
}

