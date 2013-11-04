
#define BOT_API_VERSION 1

typedef struct edict_s edict_t;

typedef struct
{
	int			apiversion;

	// The init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init.
	void		(*Init) (void);
	void		(*Shutdown) (void);
	void		(*GameEvent) (game_event_t event, edict_t *ent, void *data1, void *data2);
	void		(*RunFrame) (int msec); // should be called each game frame
	qboolean	(*Command) (edict_t *ent, const char *cmd, const char *cmd2, const char *cmd3, const char *cmd4);
	void		(*ExitLevel) (void); // called when level ends
	void		(*SpawnEntities) (void); // called when level starts

	// Block of unset data that will be zeroed out, in case of API changes, this will make new function pointers null,
	// so crashes will be more obvious.
	char unset[256];
} bot_export_t;

typedef struct
{
	int			apiversion;

	void		(*bprintf) (int printlevel, char *fmt, ...);
	void		(*cprintf) (edict_t *ent, int printlevel, char *fmt, ...);
	void		(*dprintf) (char *fmt, ...);
	trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passent, int contentmask);
	qboolean	(*inPVS) (vec3_t p1, vec3_t p2);
	qboolean	(*inPHS) (vec3_t p1, vec3_t p2);
	void		(*Pmove) (pmove_t *pmove);		// player movement code common with client prediction
	void		*(*TagMalloc) (int size, int tag);
	void		(*TagFree) (void *block);
	void		(*FreeTags) (int tag);
	cvar_t		*(*cvar) (char *var_name, char *value, int flags);

	void		(*ClientThink) (edict_t *ent, usercmd_t *ucmd);
	edict_t		*(*AddBotClient) (char *userinfo);
	const char	*(*GetClientName) (edict_t *ent);
	void		(*DisconnectBot) (edict_t *ent);
} bot_import_t;