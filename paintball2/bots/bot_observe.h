// Copyright (c) 2014 Nathan "jitspoe" Wulf
// To be released under the GPL

#define MAX_PLAYERS_TO_RECORD 32
#define MAX_PLAYER_OBSERVATION_PATH_POINTS 2048
#define MAX_RECORDED_PATHS 1024


typedef struct {
	signed char forward;
	signed char side;
	signed char up;
	unsigned short msec;
	short angle;
} player_input_data_t;


typedef struct {
	vec3_t				start_pos; // should we use shorts instead?  What about for larger maps?
	player_input_data_t	input_data[MAX_PLAYER_OBSERVATION_PATH_POINTS]; // todo: dynamically size?
	int					path_index;
	qboolean			path_active;
	vec3_t				end_pos;
	pmove_t				last_pm;
	vec3_t				last_pos;
} player_observation_t;


typedef struct {
	vec3_t					start_pos;
	vec3_t					end_pos;
	float					time;
	int						bot_failures; // todo: each time a bot fails to complete this path, increment this value and discard the path if it fails too frequently
	player_input_data_t		*input_data;
} player_recorded_path_t;


typedef struct {
	int						num_paths;
	int						path_capacity;
	player_recorded_path_t	*paths;
} player_recorded_paths_t;

