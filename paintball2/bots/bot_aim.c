/*
Copyright (c) 2015 Nathan "jitspoe" Wulf, Digital Paint

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

#include "bot_main.h"
#include "bot_manager.h"
#include "../game/game.h"
#include "bot_debug.h"


void VecToAngles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;
	
	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = atan2(value1[1], value1[0]) * 180 / M_PI;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = atan2(value1[2], forward) * 180 / M_PI;

		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


void BotSetDesiredAimAnglesFromPoint (int botindex, const vec3_t point)
{
	botmovedata_t *movement = bots.movement + botindex;
	edict_t *bot = bots.ents[botindex];
	vec3_t to_target;
	vec3_t desired_angles;

	VectorSubtract(point, bot->s.origin, to_target);
	VectorNormalize(to_target);
	VecToAngles(to_target, desired_angles);
	movement->desired_yaw = desired_angles[YAW];
	movement->desired_pitch = desired_angles[PITCH];

	if (movement->desired_pitch < -180.0f)
		movement->desired_pitch += 360.0f;

	if (movement->desired_pitch > 180.0f)
		movement->desired_pitch -= 360.0f;

	//bi.bprintf(PRINT_HIGH, "Y: %f, P: %f\n", movement->desired_yaw, movement->desired_pitch);
}


void BotAimTowardDesiredAngles (int botindex, int msec)
{
	botmovedata_t *movement = bots.movement + botindex;
	float dt = (float)msec / 1000.0f;

	movement->aimtimeleft -= dt;

	if (movement->aimtimeleft <= 0.0001f)
	{
		float delta_yaw, delta_pitch;
		edict_t *bot = bots.ents[botindex];
		float old_yawspeed = movement->yawspeed;
		float old_pitchspeed = movement->pitchspeed;
		float new_yawspeed, new_pitchspeed;
		int additional_frames = (int)nu_rand(3.0f);
		float aim_time = dt + dt * additional_frames;
		float aim_time_rand = aim_time + nu_rand(aim_time * 0.3f) - nu_rand(aim_time * 0.3f); // +/- 30% time estimation, so bots "mess up" their aim a little.

		delta_yaw = movement->desired_yaw - bot->s.angles[YAW];
		delta_pitch = movement->desired_pitch - bot->s.angles[PITCH];

		if (delta_yaw > 180.0f)
			delta_yaw -= 360.0f;

		if (delta_yaw < -180.0f)
			delta_yaw += 360.0f;

		if (delta_pitch > 180.0f)
			delta_pitch -= 360.0f;

		if (delta_pitch < -180.0f)
			delta_pitch += 360.0f;

		// Calculate turn speed
		new_yawspeed = delta_yaw / aim_time_rand;
		new_pitchspeed = delta_pitch / aim_time_rand;
		// damp yaw speed to make it less twitchy
		movement->yawspeed = DampIIR(old_yawspeed, new_yawspeed, 0.1f, aim_time);
		movement->pitchspeed = DampIIR(old_pitchspeed, new_pitchspeed, 0.1f, aim_time);
		movement->aimtimeleft = aim_time;
	}
}

// Called every frame -- acquire and shoot at targets.
void BotAimAndShoot (int botindex, int msec)
{
	edict_t *target = bi.GetNextPlayerEnt(NULL, false);
	edict_t *self = bots.ents[botindex];
	edict_t *best_target = NULL;
	float best_dist_sq = 4096.0f * 4096.0f;

	while (target)
	{
		if (target != self)
		{
			vec3_t diff;
			float dist_sq;

			VectorSubtract(self->s.origin, target->s.origin, diff);
			dist_sq = VectorLengthSquared(diff);

			if (dist_sq < best_dist_sq)
			{
				// TODO: Line of sight check
				if (bi.IsEnemy(self, target))
				{
					best_dist_sq = dist_sq;
					best_target = target;
				}
			}
		}

		target = bi.GetNextPlayerEnt(target, false);
	}

	if (best_target)
	{
		botmovedata_t *movement = bots.movement + botindex;
		vec3_t best_target_predicted_location;
		float rand_time = nu_rand(0.6f) - nu_rand(0.2f); // aim at where the target will be somewhere between -200 to 600 ms (might lead target, might lag).

		if (movement->aim_target == best_target)
		{
			// same target as last update.  Calculate velocity between old and new position.
			vec3_t pos_diff;
			vec3_t predicted_velocity;

			VectorSubtract(best_target->s.origin, movement->last_target_pos, pos_diff);
			VectorScale(pos_diff, 1000.0f / (float)msec, predicted_velocity);
			VectorMA(best_target->s.origin, rand_time, predicted_velocity, best_target_predicted_location);
		}
		else
		{
			// New target, just aim directly toward it
			VectorCopy(best_target->s.origin, best_target_predicted_location);
			movement->aim_target = best_target;
		}

		BotSetDesiredAimAnglesFromPoint(botindex, best_target_predicted_location);
		DrawDebugSphere(best_target_predicted_location, 20.0f, 1.0f, 0.4f, 0.1f, 0.2f, -1);
		VectorCopy(best_target->s.origin, movement->last_target_pos);

		if (skill->value > -1)
		{
			movement->shooting = nu_rand(1.0f) > 0.5f; // 50% chance of shooting every frame
		}
		else
		{
			movement->shooting = false;
		}
	}

	BotAimTowardDesiredAngles(botindex, msec);
}
