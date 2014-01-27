// Copyright (c) 2014 Nathan "jitspoe" Wulf
// To be released under the GPL

#ifndef _BOT_MAIN_H_
#define _BOT_MAIN_H_

#include "../game/q_shared.h"
#include "bot_importexport.h"

// memory tags to allow dynamic memory to be cleaned up
#define	TAG_GAME	765		// clear when unloading the dll
#define	TAG_LEVEL	766		// clear when loading a new level

#define nu_rand(x)	((float)((float)(x) * (float)rand() / ((float)RAND_MAX + 1.0f)))

// bot_main.c
void BotInitLibrary (void);
void BotShutdown (void);
void BotHandleGameEvent (game_event_t event, edict_t *ent, void *data1, void *data2);
void BotRunFrame (int msec);
qboolean BotCommand (edict_t *ent, const char *cmd, const char *cmd2, const char *cmd3, const char *cmd4);
void BotExitLevel (void);
void BotSpawnEntities (void);
void AddBot (const char *name);

extern bot_import_t bi;

// bot_move.c
void BotUpdateMovement (int msec);
void BotObservePlayerInput (unsigned int player_index, edict_t *ent, pmove_t *pm);

#endif
