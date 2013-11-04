// Copyright (c) 2013 Nathan "jitspoe" Wulf
// To be released under the GPL

#ifndef _BOT_MAIN_H_
#define _BOT_MAIN_H_

#include "../game/q_shared.h"
#include "bot_importexport.h"

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
void BotMove (unsigned int botindex, int msec);

#endif
