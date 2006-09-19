/*
Copyright (C) 2006 Digital Paint

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

// cl_profile.c -- profile related functions

#include "client.h"
#include "../qcommon/md5.h"
#include "../qcommon/simplecrypt.h"

#define PROFILE_LOGIN_NAME_LEN 64
#define PROFILE_PASSWORD_LEN 64

cvar_t *menu_profile_pass;
static char g_szPassHash[256];
static char g_szRandomString[256];

static const char *GetUniqueSystemString (void)
{
	static char szString[1024] = "";

	if (!*szString)
	{
#ifdef WIN32
		DWORD dwDrives;
		int nDrive;
		char szRootPath[4] = "X:\\";

		dwDrives = GetLogicalDrives();

		// Get the serial number off of the first fixed volume.  That should be pretty unique.
		for (nDrive = 'A'; nDrive <= 'Z'; ++nDrive)
		{
			if (dwDrives & 0x01)
			{
				szRootPath[0] = nDrive;

				if (GetDriveType(szRootPath) == DRIVE_FIXED)
				{
					char szVolumeName[MAX_PATH + 1];
					char szFileSystemName[MAX_PATH + 1];
					DWORD dwSerial, dwMaximumComponentLength, dwSysFlags;

					GetVolumeInformation(szRootPath, szVolumeName, sizeof(szVolumeName),
						&dwSerial, &dwMaximumComponentLength, &dwSysFlags,
						szFileSystemName, sizeof(szFileSystemName));

					if (dwSerial)
					{
						BinToHex(&dwSerial, sizeof(DWORD), szString, sizeof(szString));
						return szString;
					}
				}
			}

			dwDrives >>= 1;
		}

		strcpy(szString, "NODRIVES");
#else
		strcpy(szString, "NONWIN32"); // todo: get some unique string from system.
#endif
	}

	return szString;
}

void CL_ProfileEdit_f (void)
{
	if (Cmd_Argc() < 2)
	{
		Cbuf_AddText("menu profile_mustselect\n");
	}
	else
	{
		char szProfilePath[MAX_OSPATH];

		Com_sprintf(szProfilePath, sizeof(szProfilePath), "pball/profiles/%s.prf", Cmd_Argv(1));

		if (!FileExists(szProfilePath))
			Cbuf_AddText("menu profile_mustselect\n");
		else
			Cbuf_AddText("menu profile_edit\n");
	}
}


static void StripNonAlphaNum (const char *sIn, char *sOut, size_t sizeOut)
{
	int i = 0;
	int c;

	// strip non alpha-numeric characters from profile name
	while ((c = *sIn) && i < sizeOut - 1)
	{
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
			sOut[i++] = c;

		sIn++;
	}

	assert(i < sizeOut - 1);
	sOut[i] = 0;
}

static void WriteProfileFile (const char *sProfilePath, const char *sLoginName, const char *sPassword, qboolean bRawPass)
{
	FILE *fp;

	fp = fopen(sProfilePath, "wb");

	if (fp)
	{
		char szLoginName[PROFILE_LOGIN_NAME_LEN];
		char szPassword[PROFILE_PASSWORD_LEN];

		memset(szLoginName, 0, sizeof(szLoginName));
		Q_strncpyz(szLoginName, sLoginName, sizeof(szLoginName));
		fwrite("PB2PROFILE1.0\0", 14, 1, fp);
		fwrite(szLoginName, sizeof(szLoginName), 1, fp);

		if (bRawPass)
		{
			fwrite(sPassword, PROFILE_PASSWORD_LEN, 1, fp);
		}
		else
		{
			// TODO: encrypt this using something unique to this machine and store as MD5 string
			memset(szPassword, 0, sizeof(szPassword));
			Q_strncpyz(szPassword, sPassword, sizeof(szPassword));
			fwrite(szPassword, sizeof(szPassword), 1, fp);
		}

		fclose(fp);
	}

}


void CL_ProfileEdit2_f (void)
{
	if (Cmd_Argc() == 4)
	{
		char szProfileInPath[MAX_OSPATH];
		char szProfileOutPath[MAX_OSPATH];
		char szProfileData[2048];
		char szProfileOutName[64];
		int nLen;
		FILE *fp;
		char *sLogin = Cmd_Argv(3);

		StripNonAlphaNum(Cmd_Argv(2), szProfileOutName, sizeof(szProfileOutName));
		
		if (!*szProfileOutName)
			return;

		Com_sprintf(szProfileInPath, sizeof(szProfileInPath), "%s/profiles/%s.prf", FS_Gamedir(), Cmd_Argv(1));
		Com_sprintf(szProfileOutPath, sizeof(szProfileOutPath), "%s/profiles/%s.prf", FS_Gamedir(), szProfileOutName);

		if (fp = fopen(szProfileInPath, "rb"))
		{
			nLen = fread(szProfileData, 1, sizeof(szProfileData), fp);
			fclose(fp);

			if ((!FileExists(szProfileOutPath) || Q_strcasecmp(szProfileInPath, szProfileOutPath) == 0))
			{
				char *sPassword = "";
				qboolean bRawPass = false;

				if (memcmp(szProfileData, "PB2PROFILE1.0", sizeof("PB2PROFILE1.0")) == 0)
				{
					sPassword = szProfileData + sizeof("PB2PROFILE1.0") + PROFILE_LOGIN_NAME_LEN;
					bRawPass = true;
				}

				WriteProfileFile(szProfileOutPath, sLogin, sPassword, bRawPass);

				if (Q_strcasecmp(szProfileInPath, szProfileOutPath) != 0)
					remove(szProfileInPath);

				Cbuf_AddText("menu pop\n");
				M_ReloadMenu();
			}
		}
	}
}


void CL_ProfileAdd_f (void)
{
	char szProfileName[64];
	char szProfilePath[MAX_OSPATH];
	const char *sLoginName;

	if (Cmd_Argc() < 3)
		return;

	StripNonAlphaNum(Cmd_Argv(1), szProfileName, sizeof(szProfileName));
	sLoginName = Cmd_Argv(2);

	if (strlen(szProfileName) < 1)
		return;

	Com_sprintf(szProfilePath, sizeof(szProfilePath), "%s/profiles", FS_Gamedir());
	Sys_Mkdir(szProfilePath);
	Com_sprintf(szProfilePath, sizeof(szProfilePath), "%s/profiles/%s.prf", FS_Gamedir(), szProfileName);
	WriteProfileFile(szProfilePath, sLoginName, "", false);
	Cbuf_AddText("menu pop\n");
	M_ReloadMenu();
}


void CL_ProfileSelect_f (void)
{
	char szProfilePath[1024];
	FILE *fp;

	if (Cmd_Argc() < 2)
		return;

	Com_sprintf(szProfilePath, sizeof(szProfilePath), "%s/profiles/%s.prf", FS_Gamedir(), Cmd_Argv(1));
	
	if (fp = fopen(szProfilePath, "rb"))
	{
		char szProfileData[2048];
		int nDataLen;
		char *s;

		memset(szProfileData, 0, sizeof(szProfileData));
		nDataLen = fread(szProfileData, 1, sizeof(szProfileData), fp);
		fclose(fp);

		if (memcmp(szProfileData, "PB2PROFILE1.0", sizeof("PB2PROFILE1.0")) != 0)
		{
			Cvar_Set("menu_profile_pass", "");
			Cvar_SetValue("menu_profile_remember_pass", 0.0f);
			return;
		}

		// TODO: Decrypt this once encryption implemented.
		s = szProfileData + sizeof("PB2PROFILE1.0") + PROFILE_LOGIN_NAME_LEN;
		Cvar_Set("menu_profile_pass", s);
		Cvar_SetValue("menu_profile_remember_pass", *s ? 1.0f : 0.0f);
	}

	menu_profile_pass->modified = false;
}


int GetHTTP (const char *url, char *received, int received_max)
{
	char szRequest[1024];
	char szDomain[1024];
	const char *s, *s2;
	int len, bytes_sent;
	int bytes_read, numread;
	int socket;

	// TODO: check for redirects.  Put this in the net/tcp file?
	received_max -= 1;
	Com_sprintf(szRequest, sizeof(szRequest), "GET %s HTTP/1.0\n\n", url);
		socket = NET_TCPSocket(0);	// Create the socket descriptor

	if (socket == 0)
	{
		Com_Printf("Unable to create socket.\n");
		return -1; // No socket created
	}

	s = strstr(url, "://");

	if (s)
		s += 3;
	else
		s = url;

	s2 = strchr(s, '/');
	
	if (s2)
		len = min(sizeof(szDomain) - 1, s2 - s);
	else
		len = strlen(s);

	memcpy(szDomain, s, len);
	szDomain[len] = 0;

	if (!NET_TCPConnect(socket, szDomain, 80))
	{
		Com_Printf("Unable to connect to login server.\n");
		closesocket(socket);
		return -1;
	}

	len = strlen(szRequest);
	bytes_sent = send(socket, szRequest, len, 0);

	if (bytes_sent < len)
	{
		Com_Printf("HTTP Server did not accept request, aborting\n");
		closesocket(socket);
		return -1;
	}

	numread = 0;

	while (numread < received_max && 0 < (bytes_read = recv(socket, received + numread, received_max - numread, 0)))
		numread += bytes_read;

	received[numread] = 0; // make sure it's null terminated.
	return numread;
}


void CL_ProfileLogin_f (void)
{
	char szPassword[256];
	char szPassHash2[256];
	char szPassHash[256];
	char szRequest[1024];
	char szDataBack[4096];
	char *sPassword;
	char szUserName[64];
	char *s;
	int i;

	if (Cmd_Argc() < 3)
		return;

	strip_garbage(szUserName, Cmd_Argv(1));
	sPassword = Cmd_Argv(2);

	if (!*szUserName || !*sPassword)
		return; // blank value (shouldn't happen)

	if (menu_profile_pass->modified)
	{
		// Generate password hash (raw password is never sent or stored).
		Com_sprintf(szPassword, sizeof(szPassword), "DPLogin001%s", sPassword);
		Com_MD5HashString(szPassword, strlen(szPassword), szPassHash, sizeof(szPassHash));
	}
	else
	{
		// Password was saved.  Already hashed.
		Q_strncpyz(szPassHash, sPassword, sizeof(szPassword));
	}

	Com_sprintf(szRequest, sizeof(szRequest), "http://www.dplogin.com/gamelogin.php?init=1&username=%s", szUserName);

	if (GetHTTP(szRequest, szDataBack, sizeof(szDataBack)) < 1)
		return;

	// Obtain random string 
	s = strstr(szDataBack, "randstr:");

	if (!s)
	{
		if (s = strstr(szDataBack, "ERROR:"))
			Com_Printf("%s\n", s);
		else
			Com_Printf("ERROR: Unknown response from login server.\n");

		Cbuf_AddText("menu profile_loginfailed\n");
		return;
	}

	s += sizeof("randstr:");
	i = 0;

	while (*s >= '0' && i < sizeof(g_szRandomString) - 1)
		g_szRandomString[i++] = *s++;

	g_szRandomString[i] = 0;

	// re-hash password hash with random string and send to server for validation
	Com_sprintf(szPassword, sizeof(szPassword), "%s%s", g_szRandomString, szPassHash);
	Com_MD5HashString(szPassword, strlen(szPassword), szPassHash2, sizeof(szPassHash2));
	Com_sprintf(szRequest, sizeof(szRequest), "http://www.dplogin.com/gamelogin.php?init=2&pwhash=%s&username=%s",
		szPassHash2, szUserName);

	if (GetHTTP(szRequest, szDataBack, sizeof(szDataBack)) < 1)
		return;

	s = strstr(szDataBack, "GameLoginStatus: PASSED");

	if (!s)
	{
		if (s = strstr(szDataBack, "ERROR:"))
			Com_Printf("%s\n", s);
		else if (s = strstr(szDataBack, "GameLoginStatus: FAILED"))
			Com_Printf("Invalid password for username %s\n", szUserName);
		else
			Com_Printf("ERROR: Unknown response from login server.\n");

		Cbuf_AddText("menu profile_loginfailed\n");
		return;
	}

	s = strstr(szDataBack, "randstr:");

	if (!s)
	{
		if (s = strstr(szDataBack, "ERROR:"))
			Com_Printf("%s\n", s);
		else
			Com_Printf("ERROR: Unknown response from login server.\n");

		Cbuf_AddText("menu profile_loginfailed\n");
		return;
	}

	s += sizeof("randstr:");
	i = 0;

	while (*s >= '0' && i < sizeof(g_szRandomString) - 1)
		g_szRandomString[i++] = *s++;

	g_szRandomString[i] = 0;

	if (Cvar_Get("menu_profile_remember_pass", "0", 0)->value && menu_profile_pass->modified)
	{
		char szProfileOutPath[MAX_OSPATH];

		// TODO:
		Com_sprintf(szProfileOutPath, sizeof(szProfileOutPath), "%s/profiles/%s.prf", FS_Gamedir(), Cvar_Get("menu_profile_file", "unnamed", 0)->string);
		WriteProfileFile(szProfileOutPath, szUserName, szPassHash, false);
	}

	Cbuf_AddText("menu pop\n");
}


void CL_WebLoad_f (void)
{
	char *sURL;
#ifndef WIN32
	char szCommand[1024];
#endif

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: webload http://<url>\n");
		return;
	}

	sURL = Cmd_Argv(1);
	
	if (strncmp(sURL, "http://", 7) != 0 && strncmp(sURL, "https://", 8) != 0)
	{
		Com_Printf("Usage: webload http://<url>\n");
		return;
	}

#ifdef WIN32
	ShellExecute(NULL, "open", sURL, NULL, NULL, SW_SHOWNORMAL);
#else
	// TODO: figure out how to load the default browser in linux, etc.
	Com_sprintf(szCommand, sizeof(szCommand), "firefox \"%s\"", sURL);
	system(sURL);
#endif
}


void AddProfileCommands (void) // TODO: Change to InitProfile or something more appropriate.
{
	InitCrypt();
	menu_profile_pass = Cvar_Get("menu_profile_pass", "", 0);
	Cmd_AddCommand("webload", CL_WebLoad_f);
	Cmd_AddCommand("profile_edit", CL_ProfileEdit_f);
	Cmd_AddCommand("profile_edit2", CL_ProfileEdit2_f);
	Cmd_AddCommand("profile_add", CL_ProfileAdd_f);
	Cmd_AddCommand("profile_select", CL_ProfileSelect_f);
	Cmd_AddCommand("profile_login", CL_ProfileLogin_f);
}

