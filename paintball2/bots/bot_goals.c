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

#include "bot_manager.h"
#include "bot_goals.h"


void BotSetGoal (int bot_index, botgoaltype_t goaltype, vec3_t position)
{
	bots.goals[bot_index].type = goaltype;
	bots.goals[bot_index].changed = true;
	VectorCopy(position, bots.goals[bot_index].pos);
}


void BotUpdateGoals (int msec)
{
	int bot_index;

	for (bot_index = 0; bot_index < bots.count; ++bot_index)
	{
		if (bots.goals[bot_index].changed)
		{
			bots.goals[bot_index].changed = false;
			//todo;
		}
	}
}

// todo: bot goal to reach location.
// todo: path finding.