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

#ifndef _BOT_MANAGER_H_
#define _BOT_MANAGER_H_

#define MAX_BOTS 64

typedef struct botfollowpath_s {
	qboolean		on_path; // Are we actively following a path?
	int				path_index; // Which path are we following?
	int				index_in_path; // Where are we along that path?
} botfollowpath_t;

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
	botfollowpath_t	path_info;
} botmovedata_t;


typedef struct botmanager_s {
	edict_t*		ents[MAX_BOTS];
	botmovedata_t	movement[MAX_BOTS];
	int				count; // total number of bots currently in the map
	char			names_to_readd[MAX_BOTS][64]; // bots to readd after map change
	int				num_to_readd; // number of botss to readd
	float			game_time;
} botmanager_t;

extern botmanager_t bots;

#endif // _BOT_MANAGER_H_
