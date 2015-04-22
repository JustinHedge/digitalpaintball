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
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);

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
	VecToAngles(to_target, desired_angles);
	movement->desired_yaw = desired_angles[YAW];
	movement->desired_pitch = desired_angles[PITCH];
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
		int additional_frames = (int)nu_rand(4.0f);
		float aim_time = dt; // I don't understand why this doesn't work: + dt * additional_frames;
		float aim_time_rand = aim_time;// + nu_rand(aim_time * 0.5f) - nu_rand(aim_time * 0.5f); // +/- 50% time estimation, so bots "mess up" their aim a little.

		delta_yaw = movement->desired_yaw - bot->s.angles[YAW];
		delta_pitch = movement->desired_pitch - bot->s.angles[PITCH];

		// Calculate turn speed
		if (delta_yaw > 180.0f)
			delta_yaw -= 360.0f;

		if (delta_yaw < -180.0f)
			delta_yaw += 360.0f;

		new_yawspeed = delta_yaw / aim_time_rand;
		new_pitchspeed = delta_pitch / aim_time_rand;
		// damp yaw speed to make it less twitchy
		//movement->yawspeed = DampIIR(old_yawspeed, new_yawspeed, 0.95f, aim_time);
		//movement->pitchspeed = DampIIR(old_pitchspeed, new_pitchspeed, 0.95f, aim_time);
		movement->yawspeed = new_yawspeed;
		movement->pitchspeed = new_pitchspeed;
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
		BotSetDesiredAimAnglesFromPoint(botindex, best_target->s.origin);

		if (skill->value > -1)
		{
			bots.movement[botindex].shooting = nu_rand(1.0f) > 0.5f; // 50% chance of shooting every frame
		}
	}

	BotAimTowardDesiredAngles(botindex, msec);
}
