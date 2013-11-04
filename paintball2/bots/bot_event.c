// Copyright (c) 2013 Nathan "jitspoe" Wulf
// To be released under the GPL
#include "bot_main.h"
#include "bot_manager.h"


static void BotHandleDisconnect (const edict_t *ent)
{
	int i;
	int count = bots.count;
	qboolean removed = false;

	// Check to see if the entity that disconnected was one of the bots.  If so, remove it from the manager.
	for (i = 0; i < count; ++i)
	{
		// Shift all entries over 1 since a previous entry was removed.
		if (removed)
		{
			bots.ents[i - 1] = bots.ents[i];
		}
		else if (bots.ents[i] == ent)
		{
			--bots.count;
			removed = true;
		}
	}
}


void BotHandleGameEvent (game_event_t event, edict_t *ent, void *data1, void *data2)
{
	switch (event)
	{
	case EVENT_DISCONNECT:
		BotHandleDisconnect(ent);
		break;
	}
	// Events like round starts, flag grabs, eliminations, etc. will be passed in here
}


// todo: check if a bot has been removed or replaced.  Update bot manager

