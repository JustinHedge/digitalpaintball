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

#include "bot_main.h"
#include "../game/game.h"
#include "bot_debug.h"



void BotAddPotentialNavmeshFromPmove (const edict_t *ent, const pmove_t *pm)
{
	return; // not doing this for now.

	if (pm->groundentity) // only do this if touching the ground.
	{
		pmove_t test_pm;
		int i;
		vec3_t test_origin;

		DrawDebugSphere(ent->s.origin, 8, 1, 0, 0, 10, -1);
		memcpy(&test_pm, pm, sizeof(pmove_t));
		memset(&test_pm.cmd, 0, sizeof(test_pm.cmd));
		test_pm.cmd.forwardmove = 400;
		test_pm.cmd.msec = 50; // 20 fps

		for (i = 0; i < 20; ++i)
		{
			bi.Pmove(&test_pm);
		}

		PmoveOriginToWorldOrigin(&test_pm, test_origin);
		DrawDebugSphere(test_origin, 8, 0, 1, 0, 10, -1);
		//test_pm.touchents[test_pm.numtouch++] = ent; // Don't collide with player.
		//test_pm.cmd.forwardmove = 400;

		//pm->s.origin
		//client->ps.pmove.pm_type = PM_NORMAL;
		//client->ps.pmove.gravity = sv_gravity->value;
		/*
			// perform a pmove
	gi.Pmove(&pm);

	// save results of pmove
	client->ps.pmove = pm.s;
	client->old_pmove = pm.s;*/
	}
}

