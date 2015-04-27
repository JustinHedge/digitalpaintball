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

	VectorSubtract(point, bot->s.origin, to_target);
	VectorNormalize(to_target);
	VecToAngles(to_target, movement->desired_angles);

	if (movement->desired_angles[PITCH] < -180.0f)
		movement->desired_angles[PITCH] += 360.0f;

	if (movement->desired_angles[PITCH] > 180.0f)
		movement->desired_angles[PITCH] -= 360.0f;

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

		delta_yaw = movement->desired_angles[YAW] - bot->s.angles[YAW];
		delta_pitch = movement->desired_angles[PITCH] - bot->s.angles[PITCH];

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
	botmovedata_t *movement = bots.movement + botindex;
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
	
	// By default, don't shoot.
	movement->shooting = false;

	if (best_target)
	{
		float dist_to_target;
		vec3_t to_target;
		vec3_t best_target_predicted_location;
		float rand_time, rand_min, rand_max;

		// Distance to target
		VectorSubtract(self->s.origin, best_target->s.origin, to_target);
		dist_to_target = VectorLength(to_target);
		rand_max = 0.2f + dist_to_target / 3000.0f;
		rand_min = 80.0f / dist_to_target;

		if (rand_max < 0.2f)
			rand_max = 0.2f;

		if (rand_min > 0.2f || dist_to_target < 0.01f)
			rand_min = 0.2f;

		rand_time = nu_rand(rand_max) - nu_rand(rand_min); // randomly lead targets a bit, based on distance.
		//bi.dprintf("r: %g, min: %g, max: %g\n", rand_time, rand_min, rand_max);

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

		best_target_predicted_location[2] += nu_rand(48.0f) - nu_rand(48.0f); // add some randomization to the verticality to maybe hit crouching/jumping targets by chance.
		BotSetDesiredAimAnglesFromPoint(botindex, best_target_predicted_location);
		DrawDebugSphere(best_target_predicted_location, 20.0f, 1.0f, 0.4f, 0.1f, 0.2f, -1);
		VectorCopy(best_target->s.origin, movement->last_target_pos);

		// Shoot if we're aiming close enough to our desired target.
		if (skill->value > -1)
		{
			vec3_t desired_forward;
			vec3_t current_forward;
			float r = nu_rand(1.0f);
			float dot;
			
			// Find difference between desired aim and current aim.
			AngleVectors(self->s.angles, current_forward, NULL, NULL);
			AngleVectors(movement->desired_angles, desired_forward, NULL, NULL);
			dot = DotProduct(current_forward, desired_forward);

			// Arbitrary calculation -- the further away something is, the more precise we want to be before shooting.  It looks stupid if the bot fails to shoot at a point-blank target because he's only a degree or two off.
			if (dist_to_target > 0.0f)
				r = powf(r, 16.0f / dist_to_target);

			// The closer we're aiming to where we need to be, the more likely we are to shoot.
			if (r < dot)
				movement->shooting = true;

			bi.dprintf("%s d: %g, r: %g\n", movement->shooting ? "S" : " ", dot, r);
		}
	}

	BotAimTowardDesiredAngles(botindex, msec);
}
