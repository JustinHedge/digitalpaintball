/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cl_parse.c  -- parse a message received from the server

#include "client.h"

char *svc_strings[256] =
{
	"svc_bad",

	"svc_muzzleflash",
	"svc_muzzlflash2",
	"svc_temp_entity",
	"svc_layout",
	"svc_inventory",

	"svc_nop",
	"svc_disconnect",
	"svc_reconnect",
	"svc_sound",
	"svc_print",
	"svc_stufftext",
	"svc_serverdata",
	"svc_configstring",
	"svc_spawnbaseline",	
	"svc_centerprint",
	"svc_download",
	"svc_playerinfo",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_frame"
};

//=============================================================================

void CL_DownloadFileName(char *dest, int destlen, char *fn)
{
	if (strncmp(fn, "players", 7) == 0)
		Com_sprintf (dest, destlen, "%s/%s", BASEDIRNAME, fn);
	else
		Com_sprintf (dest, destlen, "%s/%s", FS_Gamedir(), fn);
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/

// Knightmare- store the names of last downloads that failed, jitdownload
#define NUM_FAIL_DLDS 64
char lastfaileddownload[NUM_FAIL_DLDS][MAX_OSPATH];

void clearfaileddownloads() // jitdownload
{
	int i;
	for(i=0; i<NUM_FAIL_DLDS; i++)
		*lastfaileddownload[i] = '\0';
}

qboolean	CL_CheckOrDownloadFile (char *filename) // jitodo, check for tga and jpg before
{
	FILE *fp;
	char	name[MAX_OSPATH];
	int i;

	if (FS_LoadFile (filename, NULL) != -1)
	{	// it exists, no need to download
		return true;
	}

	if (strstr (filename, "..")) // jitdownload, moved so this error doesn't come up with existing files.
	{
		Com_Printf("Refusing to download a path with ..\n");
		return true;
	}

	// (jitdownload) Knightmare- don't try again to download a file that just failed
	for (i=0; i<NUM_FAIL_DLDS; i++)
		if (lastfaileddownload[i] && strlen(lastfaileddownload[i]) &&
			Q_streq(filename,lastfaileddownload[i]))
		{	// we already tried downlaoding this, server didn't have it
			return true;
		}

	// ===
	// jitdownload, check for jpgs and tga's if pcx's aren't there
	if(strstr(filename, ".pcx") || strstr(filename, ".jpg") || 
		strstr(filename, ".tga") || strstr(filename, ".wal"))
	{
		// look for jpg first:
		COM_StripExtension(filename, name);
		strcat(name, ".jpg");
		if(FS_LoadFile(name, NULL) != -1)
			return true; // jpg exists, don't try to download anything else

		// couldn't find jpg, let's try tga;
		COM_StripExtension(filename, name);
		strcat(name, ".tga");
		if(FS_LoadFile(name, NULL) != -1)
			return true; // tga exists

		// no tga, try wal/pcx:
		COM_StripExtension(filename, name);
		if(strstr(filename, "textures"))
			strcat(name, ".wal");
		else
			strcat(name, ".pcx");
		if(FS_LoadFile(name, NULL) != -1)
			return true; // pcx/wal exists
	}
	// jit
	// ===

	strcpy (cls.downloadname, filename);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");

//ZOID
	// check to see if we already have a tmp for this file, if so, try to resume
	// open the file if not opened yet
	CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

//	FS_CreatePath (name);

	fp = fopen (name, "r+b");
	if (fp) { // it exists
		int len;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		cls.download = fp;

		// give the server an offset to start the download
		Com_Printf ("Resuming %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message,
			va("download %s %i", cls.downloadname, len));
	} else {
		Com_Printf ("Downloading %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message,
			va("download %s", cls.downloadname));
	}

	cls.downloadnumber++;

	return false;
}


/*
===============
CL_Download_f

Request a download from the server
===============
*/
void	CL_Download_f (void)
{
	char filename[MAX_OSPATH];

	if (Cmd_Argc() != 2) {
		Com_Printf("Usage: download <filename>\n");
		return;
	}

	Com_sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf("File already exists.\n");
		return;
	}

	strcpy(cls.downloadname, filename);
	Com_Printf("Downloading %s\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message,
		va("download %s", cls.downloadname));

	cls.downloadnumber++;
}

#ifdef USE_DOWNLOAD2
void	CL_Download2_f (void) // jitdownload
{
	char filename[MAX_OSPATH];

	if (Cmd_Argc() != 2) {
		Com_Printf("Usage: download2 <filename>\n");
		return;
	}

	Com_sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));
	
	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile (filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf("File already exists.\n");
		return;
	}

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message,
		va("download2 %s", filename));
	cls.downloadnumber++; // what is this used for, anyway?!
}
#endif

/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds (void)
{
	int		i;

	S_BeginRegistration ();
	CL_RegisterTEntSounds ();
	for (i=1 ; i<MAX_SOUNDS ; i++)
	{
		if (!cl.configstrings[CS_SOUNDS+i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound (cl.configstrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}
	S_EndRegistration ();
}


/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void)
{
	int		size, percent;
	char	name[MAX_OSPATH];
	int		r, i;

	// read the data
	size = MSG_ReadShort (&net_message);
	percent = MSG_ReadByte (&net_message);
	if (size == -1)
	{
		Com_Printf ("Server does not have this file.\n");

		if (cls.downloadname)	// (jitdownload) Knightmare- save name of failed download
		{
			qboolean found = false;
			qboolean added = false;

			// check if this name is already in the table
			for (i=0; i<NUM_FAIL_DLDS; i++)
			{
				if (lastfaileddownload[i] && strlen(lastfaileddownload[i])
					&& Q_streq(cls.downloadname, lastfaileddownload[i]))
				{
					found = true;
					break;
				}
			}
			// if it isn't already in the table, then we need to add it
			if (!found)
			{
				char *file_ext;
				for (i=0; i<NUM_FAIL_DLDS; i++)
					if (!lastfaileddownload[i] || !strlen(lastfaileddownload[i]))
					{	// found an open spot, fill it
						sprintf(lastfaileddownload[i], "%s", cls.downloadname);
						added = true;
						break;
					}
				// if there wasn't an open spot, we'll have to move one over to make room
				// and add it in the last spot
				if (!added)
				{
					for (i=0; i<NUM_FAIL_DLDS-1; i++)
					{
						sprintf(lastfaileddownload[i], "%s", lastfaileddownload[i+1]);
					}
					sprintf(lastfaileddownload[NUM_FAIL_DLDS-1], "%s", cls.downloadname);
				}
				
				// jitdownload -- if jpg failed, try tga, then pcx or wal
				if (file_ext=strstr(cls.downloadname, ".jpg"))
				{
					*file_ext=0;
					strcat(cls.downloadname, ".tga");
					CL_CheckOrDownloadFile(cls.downloadname);
					return;
				}
				else if (file_ext=strstr(cls.downloadname, ".tga"))
				{
					*file_ext=0;
					if(strstr(cls.downloadname, "textures"))
						strcat(cls.downloadname, ".wal");
					else
						strcat(cls.downloadname, ".pcx");
					CL_CheckOrDownloadFile(cls.downloadname);
					return;
				}
			}	
		}
		// end Knightmare

		if (cls.download)
		{
			// if here, we tried to resume a file but the server said no
			fclose (cls.download);
			cls.download = NULL;
		}

		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

		FS_CreatePath (name);

		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			net_message.readcount += size;
			Com_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	fwrite (net_message.data + net_message.readcount, 1, size, cls.download);
	net_message.readcount += size;

	if (percent != 100)
	{
		// request next block
// change display routines by zoid
#if 0
		Com_Printf (".");
		if (10*(percent/10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10*(percent/10);
			Com_Printf ("%i%%", cls.downloadpercent);
		}
#endif
		cls.downloadpercent = percent;

		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "nextdl");
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

//		Com_Printf ("100%%\n");

		fclose (cls.download);

		// rename the temp file to it's final name
		CL_DownloadFileName(oldn, sizeof(oldn), cls.downloadtempname);
		CL_DownloadFileName(newn, sizeof(newn), cls.downloadname);
		r = rename (oldn, newn);
		if (r)
			Com_Printf ("failed to rename.\n");

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed

		CL_RequestNextDownload ();
	}
}

#ifdef USE_DOWNLOAD2

#define DL2_CHUNK_TIMEOUT  2000 // jitodo -- this should be scaled according to connection. // 2 seconds before we re-request a packet
unsigned int dl2_chunk_timeout;
#define SIMULT_DL2_PACKETS 16  // download up to 16 packets at a time for more efficient bandwidth use
typedef struct {
	unsigned int offset;
	unsigned int datasize;
	char data[DOWNLOAD2_CHUNKSIZE];
	qboolean used;
} download_chunk_t; // jitdownload

typedef struct {
	unsigned int offset;
	unsigned int timesent;
	qboolean waiting;
} download_waiting_t;

download_chunk_t download_chunks[SIMULT_DL2_PACKETS];
download_waiting_t download_waiting[SIMULT_DL2_PACKETS];
int total_dl2_waiting_on; // how many download2 packets are up in the air?
unsigned int total_download_size; // how big is the file?
unsigned int next_download_offset_request;
unsigned int next_write_offset;

static void reset_download()
{
	memset(&download_chunks, 0, sizeof(download_chunk_t));
	memset(&download_waiting, 0, sizeof(download_waiting_t));
	total_dl2_waiting_on = 0;
	total_download_size = 0;
	next_write_offset = 0;
	next_download_offset_request = 0;
	dl2_chunk_timeout = DL2_CHUNK_TIMEOUT; // start at 2 seconds

	if (cls.download)
	{
		Com_Printf("Cancelled current download.\n");
		// May cause some lingering packets to come into the wrong file (but unlikely)
		fclose(cls.download);
		cls.download = NULL;
	}

	cls.download2active = false;
	cls.downloadpercent = 0;
}

void CL_StartDownload2(void)
{
	char *filename;
	char	name[MAX_OSPATH];
	int compression_type;

	reset_download();

	total_download_size = MSG_ReadLong(&net_message); // how big is the file?
	compression_type = MSG_ReadByte(&net_message); // what compression algorithm? 0 == none.
	filename = MSG_ReadString(&net_message); // get the filename

	strcpy(cls.downloadname, filename);

	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile (filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf("File already exists.\n");
		return;
	}

	strcpy (cls.downloadname, filename);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");
	CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);
	FS_CreatePath (name);
	cls.download = fopen(name, "r+b");

	if(cls.download) // file already exists, so resume
	{
		fseek(cls.download, 0, SEEK_END);
		next_write_offset = next_download_offset_request = ftell(cls.download);
		Com_Printf("Resuming %s\n", cls.downloadname);
	}
	else
	{
		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			Com_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
		Com_Printf ("Downloading %s\n", cls.downloadname);
	}

	cls.download2active = true;
}

void CL_RequestNextDownload2 (void)
{
	if(total_dl2_waiting_on <= SIMULT_DL2_PACKETS) // try to keep 16 downloads going at once.
	{
		char downloadcmd[128];
		int i;
		unsigned int request_offset;

		// Check for timed out packets:
		for(i=0; i<SIMULT_DL2_PACKETS; i++)
		{
			if(download_waiting[i].waiting && (curtime - download_waiting[i].timesent > dl2_chunk_timeout))
			{
				dl2_chunk_timeout *= 2; // double the timeout
				if(dl2_chunk_timeout > DL2_CHUNK_TIMEOUT)
					dl2_chunk_timeout = DL2_CHUNK_TIMEOUT;
				// requested chunk has timed out.  re-request it.
				request_offset = download_waiting[i].offset;
				download_waiting[i].timesent = curtime;
				goto request_chunk;
			}
		}

		if(total_dl2_waiting_on == SIMULT_DL2_PACKETS) // too many requests -- don't add any more.
			return;
		
		// none of the chunks timed out -- request a new one
		request_offset = next_download_offset_request;
		next_download_offset_request += DOWNLOAD2_CHUNKSIZE;
		total_dl2_waiting_on++;

		// keep track of our new request in case it times out.
		for(i=0; i<SIMULT_DL2_PACKETS; i++)
		{
			if(!download_waiting[i].waiting)
			{
				download_waiting[i].waiting = true;
				download_waiting[i].offset = request_offset;
				download_waiting[i].timesent = curtime;
				goto request_chunk;
			}
		}
		Com_Printf("SHOULD NOT HAPPEN! No download waiting slots open.\n");

request_chunk:
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		sprintf(downloadcmd, "nextdl2 %d", request_offset/DOWNLOAD2_CHUNKSIZE); // request next chunk
		SZ_Print (&cls.netchan.message, downloadcmd);
		Com_Printf("%cXCL Reqst: %d\n", CHAR_COLOR, request_offset/DOWNLOAD2_CHUNKSIZE);
	}
}

// if any of our download chunks are next in line to be written, write them!

void CL_WriteReceivedDownload2()
{
	int i;

	for(i=0; i<SIMULT_DL2_PACKETS; i++)
	{
		if(download_chunks[i].used && download_chunks[i].offset == next_write_offset)
		{
			download_chunks[i].used = false;

			Com_Printf("%cACL Wrote: %d\n", CHAR_COLOR, next_write_offset/DOWNLOAD2_CHUNKSIZE);
			fwrite(download_chunks[i].data, 1, download_chunks[i].datasize, cls.download);
			next_write_offset += DOWNLOAD2_CHUNKSIZE;
			total_dl2_waiting_on --; // safe to request another packet

			CL_WriteReceivedDownload2(); // can we write another one?
			break;
		}
	}
}

void CL_ParseDownload2 (void) // jitdownload
{
	int		size, percent;
	unsigned int offset;
	int		r, i;
	unsigned int trip_time;

	// read the data
//	size = MSG_ReadShort(&net_message);
	offset = MSG_ReadLong(&net_message) * DOWNLOAD2_CHUNKSIZE;

/*	if (size == -1)
	{
		Com_Printf ("Server does not have this file.\n");

		CL_RequestNextDownload ();
		return;
	}*/

	size = total_download_size - offset;
	if(size > DOWNLOAD2_CHUNKSIZE)
		size = DOWNLOAD2_CHUNKSIZE;

	// open the file if not opened yet (shouldn't happen).
	if (!cls.download)
	{
		Com_Printf ("SHOULD NOT HAPPEN! Recieved download2 packet w/o acknowledgement.\n");
		
		CL_RequestNextDownload ();
		return;
	}

	// remove request from the waiting table:
	for(i=0; i<SIMULT_DL2_PACKETS; i++)
	{
		if(download_waiting[i].waiting && download_waiting[i].offset == offset)
		{
			download_waiting[i].waiting = false;
			trip_time = curtime - download_waiting[i].timesent;
			goto request_removed;
		}
	}
	Com_Printf("SHOULD NOT HAPPEN! Chunk %d not found in waiting table!\n", offset/DOWNLOAD2_CHUNKSIZE);
request_removed:
	Com_Printf("%cECL Recvd: %d (%dms)\n", CHAR_COLOR, offset/DOWNLOAD2_CHUNKSIZE, trip_time);

	dl2_chunk_timeout = (dl2_chunk_timeout + trip_time*2) / 2; // packet timeout is 2x's average packet trip time.

	// find a free slot
	for(i=0; i<SIMULT_DL2_PACKETS; i++)
	{
		if(!download_chunks[i].used || download_chunks[i].offset == offset) // free slot or a repeat
		{
			goto a_ok;
		}
	}
	Com_Printf("SHOULD NOT HAPPEN!  No temp download2 chunks free!\n");
	net_message.readcount += size; // just dump the data.
	return;

a_ok:
	// save downloaded chunk into a free slot.
	download_chunks[i].datasize = size;
	download_chunks[i].used = true;
	download_chunks[i].offset = offset;

	memcpy(download_chunks[i].data, net_message.data + net_message.readcount, size);
	net_message.readcount += size;

	CL_WriteReceivedDownload2();

	percent = 100*next_write_offset / total_download_size;
	cls.downloadpercent = percent;

	if (percent < 100)
	{
		CL_RequestNextDownload2();
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

		Com_Printf ("CL Download Complete!\n");
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "dl2complete"); // tell the server we're done.

		fclose(cls.download);
		cls.download = NULL;

		// rename the temp file to it's final name
		CL_DownloadFileName(oldn, sizeof(oldn), cls.downloadtempname);
		CL_DownloadFileName(newn, sizeof(newn), cls.downloadname);
		r = rename (oldn, newn);
		if (r)
			Com_Printf ("failed to rename.\n");

		reset_download();

		// get another file if needed
		CL_RequestNextDownload ();
	}
}
#endif

/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData (void)
{
	extern cvar_t	*fs_gamedirvar;
	char	*str;
	int		i;
	
	Com_DPrintf ("Serverdata packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();
	cls.state = ca_connected;

// parse protocol version number
	i = MSG_ReadLong (&net_message);
	cls.serverProtocol = i;

	// BIG HACK to let demos from release work with the 3.0x patch!!!
	if (Com_ServerState() && PROTOCOL_VERSION == 34)
	{
	}
	else if (i != PROTOCOL_VERSION)
		Com_Error (ERR_DROP,"Server returned version %i, not %i", i, PROTOCOL_VERSION);

	cl.servercount = MSG_ReadLong (&net_message);
	cl.attractloop = MSG_ReadByte (&net_message);

	// game directory
	str = MSG_ReadString (&net_message);
	strncpy (cl.gamedir, str, sizeof(cl.gamedir)-1);

	// set gamedir
	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string || !Q_streq(fs_gamedirvar->string, str))) || (!*str && (fs_gamedirvar->string || *fs_gamedirvar->string)))
		Cvar_Set("game", str); // jitodo -- don't set game on demos (cl.attractloop)

	// parse player entity number
	cl.playernum = MSG_ReadShort (&net_message);

	// get the full level name
	str = MSG_ReadString (&net_message);

	if (cl.playernum == -1)
	{	// playing a cinematic or showing a pic, not a level
		SCR_PlayCinematic (str);
	}
	else
	{
		// seperate the printfs so the server message can have a color
		//Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf("\n\n %c%c\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n", CHAR_COLOR, ';'); // jittext
		//Com_Printf ("%c%s\n", 2, str);
		Com_Printf ("%c%c%s\n\n", CHAR_COLOR, COLOR_MAPNAME, str); // jittext

		// need to prep refresh at next oportunity
		cl.refresh_prepped = false;
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (void)
{
	entity_state_t	*es;
	int				bits;
	int				newnum;
	entity_state_t	nullstate;

	memset (&nullstate, 0, sizeof(nullstate));

	newnum = CL_ParseEntityBits (&bits);
	es = &cl_entities[newnum].baseline;
	CL_ParseDelta (&nullstate, es, newnum, bits);
}


/*
================
CL_LoadClientinfo

================
*/
void CL_LoadClientinfo (clientinfo_t *ci, char *s)
{
	int i;
	char		*t;
	char		model_name[MAX_QPATH];
	char		skin_name[MAX_QPATH];
	char		model_filename[MAX_QPATH];
	char		skin_filename[MAX_QPATH];
	char		weapon_filename[MAX_QPATH];

	strncpy(ci->cinfo, s, sizeof(ci->cinfo));
	ci->cinfo[sizeof(ci->cinfo)-1] = 0;

	// isolate the player's name
	strncpy(ci->name, s, sizeof(ci->name));
	ci->name[sizeof(ci->name)-1] = 0;
	t = strstr (s, "\\");
	if (t)
	{
		ci->name[t-s] = 0;
		s = t+1;
	}

	if (cl_noskins->value || *s == 0)
	{
		Com_sprintf (model_filename, sizeof(model_filename), "players/male/tris.md2");
		Com_sprintf (weapon_filename, sizeof(weapon_filename), "players/male/weapon.md2");

		//===
		//jit
		switch(s ? s[strlen(s)-1] : 0)
		{
		case 'r':
			Com_sprintf (skin_filename, sizeof(skin_filename), "players/male/pb2r.pcx");
			break;
		case 'b':
			Com_sprintf (skin_filename, sizeof(skin_filename), "players/male/pb2b.pcx");
			break;
		case 'p':
			Com_sprintf (skin_filename, sizeof(skin_filename), "players/male/pb2p.pcx");
			break;
		default:
			Com_sprintf (skin_filename, sizeof(skin_filename), "players/male/pb2y.pcx");
		}
		
		Com_sprintf (ci->iconname, sizeof(ci->iconname), "/players/male/pb2b_i.pcx");
		//jit
		//===

		ci->model = re.RegisterModel (model_filename);
		memset(ci->weaponmodel, 0, sizeof(ci->weaponmodel));
		ci->weaponmodel[0] = re.RegisterModel (weapon_filename);
		ci->skin = re.RegisterSkin (skin_filename);
		ci->icon = re.RegisterPic (ci->iconname);
	}
	else
	{
		// isolate the model name
		strcpy (model_name, s);
		t = strstr(model_name, "/");
		if (!t)
			t = strstr(model_name, "\\");
		if (!t)
			t = model_name;
		*t = 0;

		// isolate the skin name
		strcpy (skin_name, s + strlen(model_name) + 1);

		// model file
		Com_sprintf (model_filename, sizeof(model_filename), "players/%s/tris.md2", model_name);
		ci->model = re.RegisterModel (model_filename);
		if (!ci->model)
		{
			strcpy(model_name, "male");
			Com_sprintf (model_filename, sizeof(model_filename), "players/male/tris.md2");
			ci->model = re.RegisterModel (model_filename);
		}

		// skin file
		Com_sprintf (skin_filename, sizeof(skin_filename), "players/%s/%s.pcx", model_name, skin_name);
		ci->skin = re.RegisterSkin (skin_filename);

		// if we don't have the skin and the model wasn't male,
		// see if the male has it (this is for CTF's skins)
 		if (!ci->skin && Q_strcasecmp(model_name, "male"))
		{
			// change model to male
			strcpy(model_name, "male");
			Com_sprintf (model_filename, sizeof(model_filename), "players/male/tris.md2");
			ci->model = re.RegisterModel (model_filename);

			// see if the skin exists for the male model
			Com_sprintf (skin_filename, sizeof(skin_filename), "players/%s/%s.pcx", model_name, skin_name);
			ci->skin = re.RegisterSkin (skin_filename);
		}

		// if we still don't have a skin, it means that the male model didn't have
		// it, so default to grunt
		if (!ci->skin) {
			// see if the skin exists for the male model
			Com_sprintf (skin_filename, sizeof(skin_filename), "players/%s/grunt.pcx", model_name, skin_name); // jitodo -- check for appropriate pball skin
			ci->skin = re.RegisterSkin (skin_filename);
		}

		// weapon file
		for (i = 0; i < num_cl_weaponmodels; i++) {
			Com_sprintf (weapon_filename, sizeof(weapon_filename), "players/%s/%s", model_name, cl_weaponmodels[i]);
			ci->weaponmodel[i] = re.RegisterModel(weapon_filename);
			if (!ci->weaponmodel[i] && Q_streq(model_name, "cyborg"))
			{
				// try male
				Com_sprintf (weapon_filename, sizeof(weapon_filename), "players/male/%s", cl_weaponmodels[i]);
				ci->weaponmodel[i] = re.RegisterModel(weapon_filename);
			}
			if (!cl_vwep->value)
				break; // only one when vwep is off
		}

		// icon file
		Com_sprintf (ci->iconname, sizeof(ci->iconname), "/players/%s/%s_i.pcx", model_name, skin_name);
		ci->icon = re.RegisterPic (ci->iconname);
	}

	// must have loaded all data types to be valud
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0])
	{
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponmodel[0] = NULL;
		return;
	}
}

/*
================
CL_ParseClientinfo

Load the skin, icon, and model for a client
================
*/
void CL_ParseClientinfo (int player)
{
	char			*s;
	clientinfo_t	*ci;

	s = cl.configstrings[player+CS_PLAYERSKINS];

	ci = &cl.clientinfo[player];

	CL_LoadClientinfo (ci, s);
}


/*
================
CL_ParseConfigString
================
*/
void CL_ParseConfigString (void)
{
	int		i;
	char	*s;
	char	olds[MAX_QPATH];

	i = MSG_ReadShort(&net_message);

	if (i < 0 || i >= MAX_CONFIGSTRINGS)
		Com_Error (ERR_DROP, "configstring > MAX_CONFIGSTRINGS");

	s = MSG_ReadString(&net_message);

	strncpy(olds, cl.configstrings[i], sizeof(olds));
	olds[sizeof(olds) - 1] = 0;

	strcpy(cl.configstrings[i], s);

	// do something apropriate 

	if (i >= CS_LIGHTS && i < CS_LIGHTS+MAX_LIGHTSTYLES)
	{
		CL_SetLightstyle (i - CS_LIGHTS);
	}
	else if (i == CS_CDTRACK)
	{
		if (cl.refresh_prepped)
			CDAudio_Play (atoi(cl.configstrings[CS_CDTRACK]), true);
	}
	else if (i >= CS_MODELS && i < CS_MODELS+MAX_MODELS)
	{
		if (cl.refresh_prepped)
		{
			cl.model_draw[i-CS_MODELS] = re.RegisterModel (cl.configstrings[i]);
			if (cl.configstrings[i][0] == '*')
				cl.model_clip[i-CS_MODELS] = CM_InlineModel (cl.configstrings[i]);
			else
				cl.model_clip[i-CS_MODELS] = NULL;
		}
	}
	else if (i >= CS_SOUNDS && i < CS_SOUNDS+MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.sound_precache[i-CS_SOUNDS] = S_RegisterSound (cl.configstrings[i]);
	}
	else if (i >= CS_IMAGES && i < CS_IMAGES+MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.image_precache[i-CS_IMAGES] = re.RegisterPic (cl.configstrings[i]);
	}
	else if (i >= CS_PLAYERSKINS && i < CS_PLAYERSKINS+MAX_CLIENTS)
	{
		if (cl.refresh_prepped && !Q_streq(olds, s))
			CL_ParseClientinfo (i-CS_PLAYERSKINS);
	}
	else if (i == CS_SERVERGVERSION) // jitversion
	{
		s = strstr(cl.configstrings[CS_SERVERGVERSION], "Gamebuild:");

		if (s)
			cls.server_gamebuild = atoi(s+10);
		else
			cls.server_gamebuild = 0;
	}
}


/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos_v;
	float	*pos;
    int 	channel, ent;
    int 	sound_num;
    float 	volume;
    float 	attenuation;  
	int		flags;
	float	ofs;

	flags = MSG_ReadByte (&net_message);
	sound_num = MSG_ReadByte (&net_message);

    if (flags & SND_VOLUME)
		volume = MSG_ReadByte (&net_message) / 255.0;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte (&net_message) / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;	

    if (flags & SND_OFFSET)
		ofs = MSG_ReadByte (&net_message) / 1000.0;
	else
		ofs = 0;

	if (flags & SND_ENT)
	{	// entity reletive
		channel = MSG_ReadShort(&net_message); 
		ent = channel>>3;
		if (ent > MAX_EDICTS)
			Com_Error (ERR_DROP,"CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS)
	{	// positioned in space
		MSG_ReadPos (&net_message, pos_v);
 
		pos = pos_v;
	}
	else	// use entity number
		pos = NULL;

	if (!cl.sound_precache[sound_num])
		return;

	S_StartSound (pos, ent, channel, cl.sound_precache[sound_num], volume, attenuation, ofs);
}       


void SHOWNET(char *s)
{
	if (cl_shownet->value>=2)
		Com_Printf ("%3i:%s\n", net_message.readcount-1, s);
}



/*
=====================
CL_ParseServerMessage
=====================
*/
extern cvar_t *cl_timestamp; // jit
void CL_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i;
	// ECHON / jit:
	char        timestamp[24];
	_strtime( timestamp );

//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
		Com_Printf ("%i ",net_message.cursize);
	else if (cl_shownet->value >= 2)
		Com_Printf ("------------------\n");


//
// parse the message
//
	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte (&net_message);

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (cl_shownet->value>=2)
		{
			if (!svc_strings[cmd])
				Com_Printf ("%3i:BAD CMD %i\n", net_message.readcount-1,cmd);
			else
				SHOWNET(svc_strings[cmd]);
		}
	
	// other commands
		switch (cmd)
		{
		default:
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
			break;
			
		case svc_nop:
//			Com_Printf ("svc_nop\n");
			break;
			
		case svc_disconnect:
			Com_Error(ERR_DISCONNECT,"Server disconnected\n");
			break;

		case svc_reconnect:
			Com_Printf("Server disconnected, reconnecting\n");
			if (cls.download)
			{
				//ZOID, close download
				fclose(cls.download);
				cls.download = NULL;
			}
			cls.state = ca_connecting;
			cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
			break;

		case svc_print:
			i = MSG_ReadByte(&net_message);

			switch(i) // jit
			{
			case PRINT_CHAT:
				S_StartLocalSound("misc/talk.wav");

				if(cl_timestamp->value) // jittext / jitcolor
					Com_Printf("%c%c[%s] %s", CHAR_COLOR, COLOR_CHAT, timestamp, MSG_ReadString(&net_message));
				else
					Com_Printf("%c%c%s", CHAR_COLOR, COLOR_CHAT, MSG_ReadString(&net_message));

				break;
			case PRINT_ITEM: // jit
				CL_ParsePrintItem(MSG_ReadString(&net_message));
				break;
			case PRINT_EVENT: // jit
				CL_ParsePrintEvent(MSG_ReadString(&net_message));
				break;
			case PRINT_SCOREDATA: // jitscores
				CL_ParesScoreData(MSG_ReadString(&net_message));
				break;
			default:
				if(cl_timestamp->value) // jit:
					Com_Printf("[%s] %s", timestamp, MSG_ReadString(&net_message));
				else
					Com_Printf("%s", MSG_ReadString(&net_message));
				break;
			}

			con.ormask = 0;
			break;
			
		case svc_centerprint:
			SCR_CenterPrint (MSG_ReadString (&net_message));
			break;
			
		case svc_stufftext:
			s = MSG_ReadString(&net_message);
			Com_DPrintf("stufftext: %s\n", s);
			Cbuf_AddText(s);
			break;
			
		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CL_ParseServerData ();
			break;
			
		case svc_configstring:
			CL_ParseConfigString ();
			break;
			
		case svc_sound:
			CL_ParseStartSoundPacket();
			break;
			
		case svc_spawnbaseline:
			CL_ParseBaseline ();
			break;

		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_muzzleflash:
			CL_ParseMuzzleFlash ();
			break;

		case svc_muzzleflash2:
			CL_ParseMuzzleFlash2 ();
			break;

		case svc_download:
			CL_ParseDownload ();
			break;
#ifdef USE_DOWNLOAD2
		case svc_download2: // jitdownload
			CL_ParseDownload2();
			break;

		case svc_download2ack: // jitdownload
			CL_StartDownload2();
			break;
#endif
		case svc_frame:
			CL_ParseFrame ();
			break;

		case svc_inventory:
			CL_ParseInventory ();
			break;

		case svc_layout:
			s = MSG_ReadString (&net_message);
			strncpy (cl.layout, s, sizeof(cl.layout)-1);
			break;

		case svc_playerinfo:
		case svc_packetentities:
		case svc_deltapacketentities:
			Com_Error (ERR_DROP, "Out of place frame data");
			break;
		}
	}

	CL_AddNetgraph ();

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (cls.demorecording && !cls.demowaiting)
		CL_WriteDemoMessage ();
}


