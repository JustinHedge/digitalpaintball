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

// net_common.c - to reduce redundant os-specific code.

#include "qcommon.h"
#include "net_common.h"

int		ip_sockets[2];
int		server_port;

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (void)
{
	cvar_t	*ip;
	int		port;
	int		dedicated;

	ip = Cvar_Get("ip", "localhost", CVAR_NOSET);
	dedicated = Cvar_VariableValue("dedicated");

	if (!ip_sockets[NS_SERVER])
	{
		port = Cvar_Get("ip_hostport", "0", CVAR_NOSET)->value;

		if (!port)
		{
			port = Cvar_Get("hostport", "0", CVAR_NOSET)->value;

			if (!port)
				port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_NOSET)->value;
		}

		server_port = port;
		ip_sockets[NS_SERVER] = NET_IPSocket(ip->string, port);

		if (!ip_sockets[NS_SERVER] && dedicated)
			Com_Error (ERR_FATAL, "Couldn't allocate dedicated server IP port");
	}


	// dedicated servers don't need client ports
	if (dedicated)
		return;

	if (!ip_sockets[NS_CLIENT])
	{
		port = Cvar_Get("ip_clientport", "0", CVAR_NOSET)->value;

		if (!port)
		{
			srand(Sys_Milliseconds());
			port = Cvar_Get("clientport", va("%i", PORT_CLIENT), CVAR_NOSET)->value;

			if (!port)
				port = PORT_ANY;
		}

		ip_sockets[NS_CLIENT] = NET_IPSocket (ip->string, port);

		if (!ip_sockets[NS_CLIENT])
			ip_sockets[NS_CLIENT] = NET_IPSocket(ip->string, PORT_ANY);
	}
}

// ===
// jithttp
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
		Com_Printf("GetHTTP(): Unable to create socket.\n");
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
		Com_Printf("GetHTTP(): Unable to connect to %s.\n", szDomain);
		closesocket(socket);
		return -1;
	}

	len = strlen(szRequest);
	bytes_sent = send(socket, szRequest, len, 0);

	if (bytes_sent < len)
	{
		Com_Printf("GetHTTP(): HTTP Server did not accept request, aborting\n");
		closesocket(socket);
		return -1;
	}

	numread = 0;

	while (numread < received_max && 0 < (bytes_read = recv(socket, received + numread, received_max - numread, 0)))
		numread += bytes_read;

	received[numread] = 0; // make sure it's null terminated.

	// Check for a forward:
	if (memcmp(received + 9, "301", 3) == 0 || memcmp(received + 9, "302", 3) == 0)
	{
		char *newaddress, *s, *s1;

		if ((newaddress = strstr(received, "\nLocation: ")))
		{
			newaddress += sizeof("\nLocation: ") - 1;

			// terminate string at LF or CRLF
			s = strchr(newaddress, '\r');
			s1 = strchr(newaddress, '\n');

			if (s && s < s1)
				*s = '\0';
			else
				*s1 = '\0';

			Com_Printf("Redirect: %s to %s\n", url, newaddress);
			return GetHTTP(newaddress, received, received_max);
		}
		else
		{
			// Should never happen
			Com_Printf("GetHTTP(): 301 redirect with no new location\n");
			return -1;
		}
	}

	return numread;
}
// jit
// ===
