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
#include "bot_waypoints.h"
#include "bot_debug.h"

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


void AStarInitPath (void)
{
	g_astar_list_size = 0;
	memset(g_astar_nodes, 0, sizeof(g_astar_nodes));
}


qboolean AStarFindPath (int start_node, int end_node)
{
	int current_node = start_node;
	int connection_index;
	int list_index;

	AStarInitPath();

	if (end_node < 0 || end_node >= MAX_WAYPOINTS)
		return false;

	if (start_node < 0 || start_node >= MAX_WAYPOINTS)
		return false;

	while (g_astar_nodes[end_node].node_status == ASTAR_UNSET)
	{
		assert(current_node < MAX_WAYPOINTS);

		// Put current node on closed list
		g_astar_nodes[current_node].node_status = ASTAR_CLOSED; // todo: opt: remove from g_astar_list.
		DrawDebugSphere(g_bot_waypoints.positions[current_node], 7, 1, 0, 0, 5, -1);

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
					DrawDebugSphere(g_bot_waypoints.positions[connected_node], 7, 1, 1, 1, 5, -1);
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

	// backtrack
	// temp: just debug draw for now
	current_node = end_node;

	while (current_node != start_node)
	{
		DrawDebugSphere(g_bot_waypoints.positions[current_node], 10.0f, 0.0f, 1.0f, 0.2f, 5.0f, -1);
		current_node = g_astar_nodes[current_node].parent_node;
	}

	return true;
}

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

	AStarFindPath(g_astar_debug_start, debug_end);
}

