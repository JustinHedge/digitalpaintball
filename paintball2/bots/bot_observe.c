// Copyright (c) 2014 Nathan "jitspoe" Wulf
// To be released under the GPL
//
// This file deals with functions that observe and record player movement, so bots can replay that information later to navigate maps with complex jumps.
//

#include "bot_main.h"
#include "../game/game.h"
#include "bot_manager.h"
#include "bot_observe.h"


#define PLAYER_OBSERVE_MIN_MOVE_SPEED 280 // start recording players moving this fast



player_observation_t		g_player_observations[MAX_PLAYERS_TO_RECORD];
player_recorded_paths_t		g_player_paths;


usercmd_t g_lastplayercmd; // used for BotCopyPlayer
usercmd_t g_playercmd; // used for BotCopyPlayer


void BotAddPlayerObservation (player_observation_t *observation)
{
	int i, max_index = observation->path_index;
	int path_index = g_player_paths.num_paths;

	if (!g_player_paths.path_capacity)
	{
		g_player_paths.path_capacity = MAX_RECORDED_PATHS;
		g_player_paths.paths = bi.TagMalloc(sizeof(player_recorded_path_t) * MAX_RECORDED_PATHS, TAG_LEVEL);
	}

	if (path_index < g_player_paths.path_capacity)
	{
		int total_msec = 0;
		player_recorded_path_t *recorded_path = g_player_paths.paths + path_index;
		player_input_data_t *input_data = bi.TagMalloc(sizeof(player_input_data_t) * max_index, TAG_LEVEL);

		for (i = 0; i < max_index; ++i)
		{
			// todo: Compression (only record points where input has changed)
			input_data[i] = observation->input_data[i];
			total_msec += observation->input_data[i].msec;
		}
		
		VectorCopy(observation->start_pos, recorded_path->start_pos);
		VectorCopy(observation->end_pos, recorded_path->end_pos);
		recorded_path->time = total_msec / 1000.0f;
		recorded_path->input_data = input_data;
		bi.dprintf("Path %d added.\n", path_index);
		g_player_paths.num_paths++;
	}
	else
	{
		bi.dprintf("Paths full.\n");
	}

	// todo: when full, either increase capacity or start replacing similar paths, or randomly replace paths
}

// todo: load/save paths on map change.

// todo: clear everything on map change.

float XYPMVelocitySquared(short *vel)
{
	float x = (float)vel[0] * 0.125f; // velocity is increased 8x's when cast to a short.
	float y = (float)vel[1] * 0.125f;

	return x * x + y * y;
}


// TODO: Call this when the player dies:
void BotCancelObservation (player_observation_t *observation)
{
	bi.dprintf("Observation path cancelled.\n");
	observation->path_active = false;
	observation->path_index = 0;
}


void BotConvertPmoveToObservationPoint (player_input_data_t *input_data, pmove_t *pm)
{
	input_data->angle = pm->viewangles[0]; // 0 or 1 here?  We may need both angles for water movement.
	input_data->forward = pm->cmd.forwardmove >> 8; // no need for this much precision.  8 bits should be plenty, even if a joystick is used.
	input_data->side = pm->cmd.sidemove >> 8;
	input_data->up = pm->cmd.upmove >> 8;
	input_data->msec = pm->cmd.msec;
}


/* scratch this - we'll need to do the compression after collecting all the points.
qboolean InputAlmostEqualToObservation (player_input_data_t *observation_data_2stepsback, player_input_data_t *observation_data_last, pmove_t *pm)
{
	signed char forward = pm->cmd.forwardmove >> 8;
	signed char side = pm->cmd.sidemove >> 8;
	signed char up = pm->cmd.upmove >> 8;

	if (observation_data_last->forward == forward && observation_data_last->side == side && observation_data_last->up != up)
	{
		short angle = pm->viewangles[0];
		short anglediff_prev = observation_data_last->angle - observation_data_2stepsback->angle;
		short anglediff_new = pm->viewangles[0] - observation_data_2stepsback->angle;
		int timediff_prev = observation_data_last->msec - observation_data_2stepsback->msec;
		int timediff_new = timediff_prev + pm->cmd.msec;
		float angular_velocity_prev

		if (
	}

	return false;
}
*/


void BotCompleteObservationPath (edict_t *ent, player_observation_t *observation)
{
	bi.dprintf("Stopping path\n");
	VectorCopy(ent->s.origin, observation->end_pos);
	BotAddPlayerObservation(observation);
	observation->path_active = false;
}


void BotAddObservationPoint (player_observation_t *observation, pmove_t *pm)
{
/* scratch this - we'll need to do the compression after collecting all the points.
	if (observation->path_index > 1)
	{
		int msec_to_add = 0;
		// If the input hasn't changed between this point and the last point, just drop the last one and replace it with the current and combine the total msec
		if (InputAlmostEqualToObservation(observation->input_data + observation->path_index - 1, pmove_t *pm))
		{
			observation->path_index--;
			msec_to_add = observation->input_data[observation->path_index].msec;
		}
		

		BotConvertPmoveToObservationPoint(observation->input_data + observation->path_index, pm);
		observation->input_data[observation->path_index].msec += msec_to_add;
		observation->path_index++;
	}
	else
	{
		// First 2 points in the player observation path, copy input verbatim, as there's no way to compress:
		BotConvertPmoveToObservationPoint(observation->input_data, pm);
		observation->path_index++;
	}
	*/

	if (observation->path_index < MAX_PLAYER_OBSERVATION_PATH_POINTS)
	{
		BotConvertPmoveToObservationPoint(observation->input_data, pm);
		observation->path_index++;
	}
	else
	{
		bi.dprintf("Path length exceeded.\n");
		//BotCompleteObservationPath(observation);
	}
}


// Called for each player input packet sent, while the player is alive
void BotObservePlayerInput (unsigned int player_index, edict_t *ent, pmove_t *pm)
{
	// todo: cvar to disable this.
	// todo: reset observation data on player disconnect/map change/etc.

	if (player_index < MAX_PLAYERS_TO_RECORD)
	{
		player_observation_t *observation = g_player_observations + player_index;

		if (!observation->path_active)
		{
			float xy_velocity_sq = XYPMVelocitySquared(pm->s.velocity);

			// todo: start on jump as well
			if (xy_velocity_sq > PLAYER_OBSERVE_MIN_MOVE_SPEED * PLAYER_OBSERVE_MIN_MOVE_SPEED)
			{
				bi.dprintf("Starting path\n");
				VectorCopy(ent->s.origin, observation->start_pos);
				observation->path_active = true;
				observation->path_index = 0;
			}
		}
		else
		{
			// todo: don't stop unless touching ground (slow air control may be necessary for some jumps)
			if (XYPMVelocitySquared(pm->s.velocity) < PLAYER_OBSERVE_MIN_MOVE_SPEED * PLAYER_OBSERVE_MIN_MOVE_SPEED)
			{
				BotCompleteObservationPath(ent, observation);
			}
		}

		if (observation->path_active)
			BotAddObservationPoint(observation, pm);

		VectorCopy(ent->s.origin, observation->last_pos);
		observation->last_pm = *pm;
	}

	g_playercmd = pm->cmd;
}

