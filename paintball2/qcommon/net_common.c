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

