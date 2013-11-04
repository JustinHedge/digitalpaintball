// Copyright (c) 2013 Nathan "jitspoe" Wulf
// To be released under the GPL

#include "bot_main.h"
#include "bot_manager.h"
#include "../game/game.h"

#define MOVE_SPEED 400 // 400 is the running speed in Quake2
#define BOT_UCMD_TIME 25 // 25ms is 40fps.  Should be enough for trick jumps


// Spin and move randomly. :)
void BotDance (unsigned int botindex)
{
	botmovedata_t *movement = bots.movement + botindex;

	movement->yawspeed = 3.0f * 360.0f;
	movement->forward = (short)nu_rand(MOVE_SPEED * 2) - MOVE_SPEED;
	movement->side = (short)nu_rand(MOVE_SPEED * 2) - MOVE_SPEED;
	movement->up = (short)nu_rand(MOVE_SPEED * 2) - MOVE_SPEED;
	movement->shooting = false;
}


// Wander around with no navmesh
#define BOT_WANDER_TRACE_DISTANCE 500.0f
#define BOT_WANDER_SPIN_DIST 16.0f // 16 units from a wall - practically touching, time to turn around
#define BOT_WANDER_RAND_ANGLE_FAR 10.0f
#define BOT_WANDER_RAND_ANGLE_NEAR 90.0f
#define BOT_WANDER_TRACE_FAR 400.0f // Beyond this point, use far angles
#define BOT_WANDER_TRACE_NEAR 48.0f // Interpolate between this and far for the rand angles, use near rand angles below this value
#define WALKABLE_NORMAL_Z 0.707106f // for walkable slopes (hills/ramps)
#define BOT_WANDER_MAX_TURN_SPEED 180.0f // degrees/sec
#define BOT_WANDER_CAST_INTERVAL 200 // cast every 200ms
void BotWanderNoNav (unsigned int botindex, int msec)
{
	botmovedata_t *movement = bots.movement + botindex;
	edict_t *ent = bots.ents[botindex];
	vec_t *pos = ent->s.origin;
	float yaw = ent->s.angles[YAW];
	float rand_angle;
	float yaw_right;
	float yaw_left;
	vec3_t vec_right;
	vec3_t vec_left;
	trace_t trace_left, trace_right;
	vec3_t mins;
	vec3_t maxs;

	if (movement->time_since_last_turn > BOT_WANDER_CAST_INTERVAL)
	{
		rand_angle = BOT_WANDER_RAND_ANGLE_FAR;

		if (movement->last_trace_dist < BOT_WANDER_TRACE_FAR)
		{
			float ratio = (movement->last_trace_dist - BOT_WANDER_TRACE_NEAR) / BOT_WANDER_TRACE_FAR;

			if (ratio < 0.0f)
				ratio = 0.0f;

			rand_angle = BOT_WANDER_RAND_ANGLE_FAR * ratio + BOT_WANDER_RAND_ANGLE_NEAR * (1.0f - ratio);
		}

		yaw_right = (yaw + nu_rand(rand_angle));
		yaw_left = (yaw - nu_rand(rand_angle));

		// Cast out 9000 units in a couple random directions, and see which cast goes further.
		vec_right[0] = pos[0] + cos(DEG2RAD(yaw_right)) * BOT_WANDER_TRACE_DISTANCE;
		vec_right[1] = pos[1] + sin(DEG2RAD(yaw_right)) * BOT_WANDER_TRACE_DISTANCE;
		vec_right[2] = pos[2];
		vec_left[0] = pos[0] + cos(DEG2RAD(yaw_left)) * BOT_WANDER_TRACE_DISTANCE;
		vec_left[1] = pos[1] + sin(DEG2RAD(yaw_left)) * BOT_WANDER_TRACE_DISTANCE;
		vec_left[2] = pos[2];

		VectorCopy(ent->mins, mins);
		VectorCopy(ent->maxs, maxs);
		mins[2] += 16.0f; // Account for steps and slopes

		if (mins[2] > 0)
			mins[2] = -1;

		trace_left = bi.trace(pos, mins, maxs, vec_left, ent, MASK_PLAYERSOLID);
		trace_right = bi.trace(pos, mins, maxs, vec_right, ent, MASK_PLAYERSOLID);

		// If tracing to the left is further, go toward the left direction.  If tracing toward the right is further, go right.
		// If we hit a hill or ramp on the shorter trace, it's impossible to tell which distance is effectively longer, so carry on with what we were doing.
		if (trace_left.fraction > trace_right.fraction && trace_right.plane.normal[2] < WALKABLE_NORMAL_Z) 
		{
			float tracedist = trace_left.fraction * BOT_WANDER_TRACE_DISTANCE;

			if (tracedist < BOT_WANDER_SPIN_DIST)
				yaw_left += 170.0f;

			movement->yawspeed = (yaw_left - yaw) * 1000.0f / (float)BOT_WANDER_CAST_INTERVAL;
			movement->last_trace_dist = tracedist;
			movement->time_since_last_turn = 0;
		}
		else if (trace_right.fraction > trace_left.fraction && trace_left.plane.normal[2] < WALKABLE_NORMAL_Z)
		{
			float tracedist = trace_right.fraction * BOT_WANDER_TRACE_DISTANCE;

			if (tracedist < BOT_WANDER_SPIN_DIST)
				yaw_right += 170.0f;

			movement->yawspeed = (yaw_right - yaw) * 1000.0f / (float)BOT_WANDER_CAST_INTERVAL;
			movement->last_trace_dist = tracedist;
			movement->time_since_last_turn = 0;
		}
	}

	if (movement->time_since_last_turn > 500)
	{
		movement->yawspeed = nu_rand(BOT_WANDER_RAND_ANGLE_FAR) - nu_rand(BOT_WANDER_RAND_ANGLE_FAR);
		movement->time_since_last_turn = 0;
	}

	if (movement->yawspeed < -BOT_WANDER_MAX_TURN_SPEED)
		movement->yawspeed = -BOT_WANDER_MAX_TURN_SPEED;

	if (movement->yawspeed > BOT_WANDER_MAX_TURN_SPEED)
		movement->yawspeed = BOT_WANDER_MAX_TURN_SPEED;

	movement->forward = MOVE_SPEED;
}


// Called every game frame (100ms) for each bot
void BotMove (unsigned int botindex, int msec)
{
	if (botindex < MAX_BOTS)
	{
		edict_t *ent = bots.ents[botindex];

		if (ent)
		{
			usercmd_t cmd;
			botmovedata_t *movement = bots.movement + botindex;

			movement->time_since_last_turn += msec;

			//BotDance(botindex);
			BotWanderNoNav(botindex, msec);

			// Break movement up into smaller steps so strafe jumping and such work better
			movement->timeleft += msec;

			while (movement->timeleft > 0)
			{
				float yaw = ent->s.angles[YAW];
				movement->timeleft -= BOT_UCMD_TIME;
				yaw += movement->yawspeed * msec / 1000.0f;
				memset(&cmd, 0, sizeof(cmd));
				cmd.angles[YAW] = ANGLE2SHORT(yaw) - ent->client->ps.pmove.delta_angles[YAW];
				cmd.msec = BOT_UCMD_TIME;
				cmd.forwardmove = movement->forward;
				cmd.sidemove = movement->side;
				cmd.upmove = movement->up;
				cmd.buttons = movement->shooting ? 1 : 0;
				bi.ClientThink(ent, &cmd);
			}
		}
	}
}
