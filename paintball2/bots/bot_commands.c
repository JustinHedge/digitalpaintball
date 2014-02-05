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
#include "bot_manager.h"


static void AddBotCommand (const char *name)
{
	AddBot(name);
}


// The bot will get removed rom the manager in BotHandleDisconnect
static void RemoveBot (edict_t *ent)
{
	bi.DisconnectBot(ent);
}


static void RemoveAllBots (void)
{
	while (bots.count)
	{
		RemoveBot(bots.ents[0]);
	}
}


static void RemoveBotCommand (const char *nameToRemove)
{
	int i;

	if (Q_strcaseeq(nameToRemove, "all"))
		RemoveAllBots();

	// search for exact name:
	for (i = 0; i < bots.count; ++i)
	{
		edict_t *ent = bots.ents[i];
		const char *name = bi.GetClientName(ent);

		if (Q_strcaseeq(name, nameToRemove))
		{
			RemoveBot(ent);
			return;
		}
	}

	// search for partial match
	for (i = 0; i < bots.count; ++i)
	{
		edict_t *ent = bots.ents[i];
		const char *name = bi.GetClientName(ent);

		if (strstr(name, nameToRemove))
		{
			RemoveBot(ent);
			return;
		}
	}
}


qboolean BotCommand (edict_t *ent, const char *cmd, const char *cmd2, const char *cmd3, const char *cmd4)
{
	if (Q_strcaseeq(cmd, "bottest"))
	{
		bi.cprintf(ent, PRINT_HIGH, "Bottest just happened.\n");
		return true;
	}
	else if (Q_strcaseeq(cmd, "addbot"))
	{
		AddBotCommand(cmd2);
		return true;
	}
	else if (Q_strcaseeq(cmd, "removebot"))
	{
		RemoveBotCommand(cmd2);
		return true;
	}
	else if (Q_strcaseeq(cmd, "removeallbots"))
	{
		RemoveAllBots();
		return true;
	}

	return false;
}
