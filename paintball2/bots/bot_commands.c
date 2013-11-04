// Copyright (c) 2013 Nathan "jitspoe" Wulf
// To be released under the GPL

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
