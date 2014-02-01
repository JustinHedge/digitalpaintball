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

#define PLAYER_PATH_MIN_DIST 250 // quake units.  to cut back on short paths from strafing back and forth.



player_observation_t		g_player_observations[MAX_PLAYERS_TO_RECORD];
player_recorded_paths_t		g_player_paths;


usercmd_t g_lastplayercmd; // used for BotCopyPlayer
usercmd_t g_playercmd; // used for BotCopyPlayer

// TODO: discard small paths

void BotAddPlayerObservation (player_observation_t *observation)
{
	int i, total_points = observation->current_index;
	int path_index = g_player_paths.num_paths;

	if (!g_player_paths.path_capacity)
	{
		g_player_paths.path_capacity = MAX_RECORDED_PATHS;
		g_player_paths.paths = bi.TagMalloc(sizeof(player_recorded_path_t) * MAX_RECORDED_PATHS, TAG_LEVEL);
	}

	if (VectorSquareDistance(observation->start_pos, observation->end_pos) < PLAYER_PATH_MIN_DIST * PLAYER_PATH_MIN_DIST)
	{
		bi.dprintf("Path too short.  Discarded.\n");
		return;
	}

	if (path_index < g_player_paths.path_capacity)
	{
		int total_msec = 0;
		player_recorded_path_t *recorded_path = g_player_paths.paths + path_index;
		player_input_data_t *input_data = bi.TagMalloc(sizeof(player_input_data_t) * total_points, TAG_LEVEL);

		for (i = 0; i < total_points; ++i)
		{
			// todo: Compression (only record points where input has changed)
			input_data[i] = observation->input_data[i];
			total_msec += observation->input_data[i].msec;
		}
		
		VectorCopy(observation->start_pos, recorded_path->start_pos);
		VectorCopy(observation->end_pos, recorded_path->end_pos);
		recorded_path->time = total_msec / 1000.0f;
		recorded_path->input_data = input_data;
		recorded_path->total_points = total_points;
		bi.dprintf("Path %d added.\n", path_index);
		++g_player_paths.num_paths;
		//ri.DrawDebugLine(observation->start_pos, observation->end_pos, 1.0, .8, .1, 20.0f, -1);
		ri.DrawDebugSphere(observation->end_pos, 16.0f, 1.0f, 0.f, 0.5f, 20.0f, -1);
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
	observation->current_index = 0;
}


void BotConvertPmoveToObservationPoint (player_input_data_t *input_data, const edict_t *ent, const pmove_t *pm)
{
	short forward = pm->cmd.forwardmove;
	short side = pm->cmd.sidemove;
	short up = pm->cmd.upmove;

	// It's possible players have these set to wonky values, so limit them to +/-400
	if (forward > 400)
		forward = 400;
	else if (forward < -400)
		forward = -400;

	if (up > 400)
		up = 400;
	else if (up < -400)
		up = -400;

	if (side > 400)
		side = 400;
	else if (side < -400)
		side = -400;

	input_data->angle = pm->cmd.angles[YAW] + ent->client->ps.pmove.delta_angles[YAW]; // We may need both angles for water movement...
	input_data->forward = forward / PMOVE_8BIT_SCALE; // no need for this much precision.  8 bits should be plenty, even if a joystick is used.
	input_data->side = side / PMOVE_8BIT_SCALE;
	input_data->up = up / PMOVE_8BIT_SCALE;
	input_data->msec = pm->cmd.msec;
}


void BotCompleteObservationPath (edict_t *ent, player_observation_t *observation)
{
	bi.dprintf("Stopping path\n");
	VectorCopy(ent->s.origin, observation->end_pos);
	BotAddPlayerObservation(observation);
	observation->path_active = false;
}


void BotAddObservationPoint (player_observation_t *observation, const edict_t *ent, const pmove_t *pm)
{
	if (observation->current_index < MAX_PLAYER_OBSERVATION_PATH_POINTS)
	{
		if (pm->cmd.msec > 0)
		{
			BotConvertPmoveToObservationPoint(observation->input_data + observation->current_index, ent, pm);
			++observation->current_index;
		}
		else
		{
			bi.dprintf("Observed msec == 0.\n"); // breakpoint
		}
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
			if (xy_velocity_sq > PLAYER_OBSERVE_MIN_MOVE_SPEED * PLAYER_OBSERVE_MIN_MOVE_SPEED && pm->groundentity)
			{
				bi.dprintf("Starting path\n");
				VectorCopy(ent->s.origin, observation->start_pos);
				observation->path_active = true;
				observation->current_index = 0;
			}
		}
		else
		{
			if (XYPMVelocitySquared(pm->s.velocity) < PLAYER_OBSERVE_MIN_MOVE_SPEED * PLAYER_OBSERVE_MIN_MOVE_SPEED && pm->groundentity)
			{
				BotCompleteObservationPath(ent, observation);
			}
		}

		if (observation->path_active)
			BotAddObservationPoint(observation, ent, pm);

		// temp debug
		{
			float vel = sqrt(pm->s.velocity[0] * pm->s.velocity[0] + pm->s.velocity[1] * pm->s.velocity[1] + pm->s.velocity[2] * pm->s.velocity[2]);
			float b = vel / 8000.0f;
			float g = vel / 16000.0f;
			float r = vel / 4000.0f;

			if (b > 1.0f)
				b = 1.0f;

			if (g > 1.0f)
				g = 1.0f;

			if (r > 1.0f)
				r = 1.0f;

			ri.DrawDebugLine(ent->s.origin, observation->last_pos, r, g, b, 70.0f, -1);
		}

		VectorCopy(ent->s.origin, observation->last_pos);
		observation->last_pm = *pm;
	}

	g_playercmd = pm->cmd;
}

