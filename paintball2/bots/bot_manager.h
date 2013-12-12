// Copyright (c) 2013 Nathan "jitspoe" Wulf
// To be released under the GPL

#ifndef _BOT_MANAGER_H_
#define _BOT_MANAGER_H_

#define MAX_BOTS 64

typedef struct botmovedata_s {
	int				timeleft; // time left for ucmd's.
	float			last_trace_dist;
	float			yawspeed;
	float			pitchspeed;
	short			forward; // forward/back
	short			side; // strafe left/right
	short			up; // jump/crouch
	qboolean		shooting;
	short			time_since_last_turn; // used for wandering
	float			last_yaw;
	float			last_pitch;
} botmovedata_t;


typedef struct botmanager_s {
	edict_t*		ents[MAX_BOTS];
	botmovedata_t	movement[MAX_BOTS];
	int				count; // total number of bots currently in the map
	char			names_to_readd[MAX_BOTS][64]; // bots to readd after map change
	int				num_to_readd; // number of botss to readd
} botmanager_t;

extern botmanager_t bots;

#endif // _BOT_MANAGER_H_
