#include "menu.h"

// Local globals
static sem_t m_sem_serverlist;
static pthread_t updatethread;
static pthread_t pingthread;
static qboolean updating = false;
static qboolean refreshing = false;

#define INITIAL_SERVERLIST_SIZE 32

// Project-wide Globals
int m_serverPingSartTime;
m_serverlist_t m_serverlist;


/*
=================
CL_PingServers_f
=================
*/
void *CL_PingServers_multithreaded (void *ptr) // jitmultithreading
{
	netadr_t	adr;
	char		name[64]; // jitserverlist - increased
	cvar_t		*noudp;
	cvar_t		*noipx;
	FILE		*serverlist; // jitserverlist / jitmenu
	extern int	m_serverPingSartTime;

	NET_Config(true);		// allow remote

	// send a broadcast packet
	Com_Printf ("pinging broadcast...\n");

	noudp = Cvar_Get("noudp", "0", CVAR_NOSET);
	if (!noudp->value)
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint(NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}

	noipx = Cvar_Get("noipx", "0", CVAR_NOSET);
	if (!noipx->value)
	{
		adr.type = NA_BROADCAST_IPX;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint(NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));
	}

	// jitserverlist / jitmenu -- ping all the servers in the list:
	serverlist = fopen(va("%s/servers.txt", FS_Gamedir()), "r");

	m_serverPingSartTime = Sys_Milliseconds(); // jitserverlist

	if(serverlist)
	{
		char buff[256];

		while(!feof(serverlist))
		{
			fscanf(serverlist, "%s", &name);

			if(name && *name)
			{
				Com_Printf ("pinging %s...\n", name);

				if (!NET_StringToAdr(name, &adr))
				{
					Com_Printf ("Bad address: %s\n", name);
					continue;
				}

				if (!adr.port)
					adr.port = BigShort(PORT_SERVER);

				Netchan_OutOfBandPrint(NS_CLIENT, adr, va("info %i", PROTOCOL_VERSION));

				// jitserverlist -- add to list and get ping request time:
				sprintf(buff, "%s --- 0/0", name);
				M_AddToServerList(adr, buff, true);
			}

			Sleep(16);
		}

		fclose(serverlist);
	}

	refreshing = false;
	return NULL;
}

void CL_PingServers_f (void) // jitmultithreading
{
	if (!refreshing)
	{
		refreshing = true;
		pthread_create(&pingthread, NULL, CL_PingServers_multithreaded, NULL);
	}
}

// Print server list to console
void M_ServerlistPrint_f (void)
{
	int i;

	for(i=0; i<m_serverlist.nummapped; i++)
		Com_Printf("%d) %s %s\n", i+1, m_serverlist.ips[i],
			m_serverlist.info[i]);
}

static void free_menu_serverlist (void) // jitodo -- eh, something should call this?
{
	if (m_serverlist.info)
		free_string_array(m_serverlist.info, m_serverlist.nummapped);
	if (m_serverlist.ips)
		free_string_array(m_serverlist.ips, m_serverlist.nummapped);
	//if (m_serverlist.remap)
	//	Z_Free(m_serverlist.remap);
	//if (m_serverlist.adr)
	//	Z_Free(m_serverlist.adr);
	if (m_serverlist.server)
		Z_Free(m_serverlist.server);

	memset(&m_serverlist, 0, sizeof(m_serverlist_t));
}

void Serverlist_Clear_f (void)
{
	register int i, j;

	for (i=0; i<m_serverlist.numservers; i++)
	{
		Z_Free(m_serverlist.server[i].mapname);
		Z_Free(m_serverlist.server[i].servername);
		memset(&m_serverlist.server[i], 0, sizeof(m_serverlist_server_t));
		m_serverlist.server[i].remap = -1;
		j = m_serverlist.server[i].remap;

		if (j >= 0)
		{
			Z_Free(m_serverlist.info[j]);
			m_serverlist.info[j] = NULL;
			Z_Free(m_serverlist.ips[j]);
			m_serverlist.ips[j] = NULL;
		}
	}

	m_serverlist.nummapped = m_serverlist.numservers = 0;
	//memset(m_serverlist.remap, -1, sizeof(int)*m_serverlist.actualsize);
	//memset(m_serverlist.adr, 0, sizeof(netadr_t)*m_serverlist.actualsize);
}


// Parse info string into server name, players, maxplayers
// Warning! Modifies string!
static void update_serverlist_server (m_serverlist_server_t *server, char *info, int ping)
{
	char *s;

	// Free up the old stuff.
	if (server->mapname)
		Z_Free(server->mapname);
	if (server->servername)
		Z_Free(server->servername);

	// start at end of string:
	s = strlen(info) + info; 

	// find max players
	while(s > info && *s != '/')
		s--;
	server->maxplayers = atoi(s+1);

	// find current number of players:
	*s = 0;
	s--;
	while(s > info && *s >= '0' && *s <= '9')
		s--;
	server->players = atoi(s+1);
	
	// find map name:
	while(s > info && *s == 32) // clear whitespace;
		s--;
	*(s+1) = 0;
	while(s > info && *s > 32)
		s--;
	server->mapname = text_copy(s+1);

	// servername is what's left over:
	*s = 0;
	server->servername = text_copy(info);

	// and the ping
	server->ping = ping;
}

static void set_serverlist_server_pingtime (m_serverlist_server_t *server, int pingtime)
{
	server->ping_request_time = pingtime;
}


#define SERVER_NAME_MAXLENGTH 19
#define MAP_NAME_MAXLENGTH 8
static char *format_info_from_serverlist_server(m_serverlist_server_t *server)
{
	static char info[64];
	char stemp=0, mtemp=0;
	int ping = 999;

	if(server->ping < 999)
		ping = server->ping;

	// jitodo -- should compensate for colored text in server name.

	// truncate name if too long:
	if(strlen_noformat(server->servername) > SERVER_NAME_MAXLENGTH)
	{
		stemp = server->servername[SERVER_NAME_MAXLENGTH];
		server->servername[SERVER_NAME_MAXLENGTH] = 0;
	}
	if(strlen(server->mapname) > MAP_NAME_MAXLENGTH)
	{
		mtemp = server->servername[MAP_NAME_MAXLENGTH];
		server->mapname[MAP_NAME_MAXLENGTH] = 0;
	}

	// assumes SERVER_NAME_MAXLENGTH is 19 vv
	if(ping < 999)
		Com_sprintf(info, sizeof(info), "%-19s %-3d %-8s %d/%d", 
			server->servername, ping, server->mapname,
			server->players, server->maxplayers);
	else
		Com_sprintf(info, sizeof(info), "%c4%-19s %-3d %-8s %d/%d", 
			CHAR_COLOR, server->servername, ping, server->mapname,
			server->players, server->maxplayers);

	if(stemp)
		server->servername[SERVER_NAME_MAXLENGTH] = stemp;

	if(mtemp)
		server->mapname[MAP_NAME_MAXLENGTH] = mtemp;

	return info;
}

static qboolean adrs_equal (netadr_t adr1, netadr_t adr2)
{
	if (adr1.type != adr2.type || adr1.port != adr2.port)
		return false;

	if (adr1.type == NA_IP)
		return adr1.ip[0] == adr2.ip[0] && adr1.ip[1] == adr2.ip[1] &&
			adr1.ip[2] == adr2.ip[2] && adr1.ip[3] == adr2.ip[3];

	if (adr1.type == NA_IPX)
	{
		int i;

		for (i = 0; i < 10; i++)
			if (adr1.ipx[i] != adr2.ipx[i])
				return false;
	}

	return true;
}

void M_AddToServerList (netadr_t adr, char *info, qboolean pinging)
{
	int i;
	char addrip[32];
	int ping;
	qboolean added = false;
	
	//ping = Sys_Milliseconds() - m_serverPingSartTime;

	if(adr.type == NA_IP)
	{
		Com_sprintf(addrip, sizeof(addrip), 
			"%d.%d.%d.%d:%d", adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], ntohs(adr.port));
	}
	else if(adr.type == NA_IPX)
	{
		Com_sprintf(addrip, sizeof(addrip), 
			"%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%i", adr.ipx[0], adr.ipx[1],
			adr.ipx[2], adr.ipx[3], adr.ipx[4], adr.ipx[5], adr.ipx[6], adr.ipx[7],
			adr.ipx[8], adr.ipx[9], ntohs(adr.port));
	}
	else if(adr.type == NA_LOOPBACK)
	{
		strcpy(addrip, "loopback");
	}
	else
		return;

	sem_wait(&m_sem_serverlist); // jitmultithreading

	// check if server exists in current serverlist:
	for (i=0; i<m_serverlist.numservers; i++)
	{
//		if(Q_streq(addrip, m_serverlist.ips[i]))
		if (adrs_equal(adr, m_serverlist.server[i].adr))
		{
			// update info from server:
			if (pinging)
			{
				set_serverlist_server_pingtime(&m_serverlist.server[i], Sys_Milliseconds());
			}
			else
			{
				ping = Sys_Milliseconds() - m_serverlist.server[i].ping_request_time;
				update_serverlist_server(&m_serverlist.server[i], info, ping);
				/*Z_Free(m_serverlist.info[i]);
				m_serverlist.info[i] = text_copy(format_info_from_serverlist_server(&m_serverlist.server[i]));*/

				if (m_serverlist.server[i].remap < 0)
				{
					m_serverlist.server[i].remap = m_serverlist.nummapped++;
					m_serverlist.ips[m_serverlist.server[i].remap] = text_copy(addrip);
				}
				else
					Z_Free(m_serverlist.info[m_serverlist.server[i].remap]);

				m_serverlist.info[m_serverlist.server[i].remap] =
					text_copy(format_info_from_serverlist_server(&m_serverlist.server[i]));
			}

			added = true;
			break;
		}
	}

	if(!added) // doesn't exist.  Add it.
	{
		i++;

		// List too big?  Alloc more memory:
		// STL would be useful about now
		if(i > m_serverlist.actualsize) 
		{
			char **tempinfo;
			char **tempips;
			m_serverlist_server_t *tempserver;
			//int *tempremap;
			//netadr_t *tempadr;

			tempinfo = Z_Malloc(sizeof(char*)*m_serverlist.actualsize*2); // double size
			tempips = Z_Malloc(sizeof(char*)*m_serverlist.actualsize*2); // double size
			tempserver = Z_Malloc(sizeof(m_serverlist_server_t)*m_serverlist.actualsize*2);
			//tempremap = Z_Malloc(sizeof(int)*m_serverlist.actualsize*2);
			//memset(tempremap, -1, sizeof(int)*m_serverlist.actualsize*2);
			//tempadr = Z_Malloc(sizeof(netadr_t)*m_serverlist.actualsize*2);

			for(i=0; i<m_serverlist.actualsize; i++)
			{
				tempinfo[i] = m_serverlist.info[i];
				tempips[i] = m_serverlist.ips[i];
				tempserver[i] = m_serverlist.server[i];
				//tempremap[i] = m_serverlist.remap[i];
				//memcpy(&tempadr[i], &m_serverlist.adr[i], sizeof(netadr_t));
				// jitodo - make sure this works...
			}

			for (i=m_serverlist.actualsize; i<m_serverlist.actualsize*2; i++)
			{
				memset(&tempserver[i], 0, sizeof(m_serverlist_server_t));
				tempserver[i].remap = -1;
			}

			Z_Free(m_serverlist.info);
			Z_Free(m_serverlist.ips);
			Z_Free(m_serverlist.server);
			//Z_Free(m_serverlist.remap);
			//Z_Free(m_serverlist.adr);

			m_serverlist.info = tempinfo;
			m_serverlist.ips = tempips;
			m_serverlist.server = tempserver;
			//m_serverlist.remap = tempremap;
			//m_serverlist.adr = tempadr;

			m_serverlist.actualsize *= 2;
		}

		//memcpy(&m_serverlist.adr[m_serverlist.numservers], &adr, sizeof(netadr_t));
		m_serverlist.server[m_serverlist.numservers].adr = adr; // jitodo - test

		// add data to serverlist:
		if (pinging)
		{
			set_serverlist_server_pingtime(&m_serverlist.server[m_serverlist.numservers], Sys_Milliseconds());
			ping = 999;
			update_serverlist_server(&m_serverlist.server[m_serverlist.numservers], info, ping);
		}
		else
		{
			// this will probably only happen with LAN servers.
			ping = Sys_Milliseconds() - m_serverlist.server[m_serverlist.numservers].ping_request_time;
			m_serverlist.ips[m_serverlist.nummapped] = text_copy(addrip);
			update_serverlist_server(&m_serverlist.server[m_serverlist.numservers], info, ping);
			m_serverlist.info[m_serverlist.nummapped] =
				text_copy(format_info_from_serverlist_server(&m_serverlist.server[m_serverlist.numservers]));
			m_serverlist.server[m_serverlist.numservers].remap = m_serverlist.nummapped;
			m_serverlist.nummapped++;
		}

		m_serverlist.numservers++;
	}

	// Tell the widget the serverlist has updated:
	M_RefreshActiveMenu(); // jitodo - target serverlist window specifically.

	sem_post(&m_sem_serverlist); // jitmultithreading
}

// color servers grey and re-ping them
void M_ServerlistRefresh_f(void)
{
	char *str;
	int i;

	for(i=0; i<m_serverlist.nummapped; i++)
	{
		str = text_copy(va("%c4%s", CHAR_COLOR, m_serverlist.info[i])); // color grey
		Z_Free(m_serverlist.info[i]);
		m_serverlist.info[i] = str;
	}

	// jitodo -- make this multithreaded and add some time between each server request.
	CL_PingServers_f();
}


// download servers.txt from a remote location and store
// on the client's drive.
static void M_ServerlistUpdate (char *sServerSource)
{
	SOCKET serverListSocket;
	char svlist_domain[256];
	char *s;
	int i;

	if (!*sServerSource)
		return;

	serverListSocket = NET_TCPSocket(0);	// Create the socket descriptor

	if (serverListSocket == 0)
	{
		Com_Printf("Unable to create socket.\n");
		return; // No socket created
	}

	s = strstr(sServerSource, "http://");

	if(!s)
	{
		// probably an old version -- going to require the http:// now.
		// jitodo -- allow for sources like "c:\servers.txt" and master server lists.
		Cvar_Set("serverlist_source", "http://www.planetquake.com/digitalpaint/servers.txt");
		M_ServerlistUpdate(sServerSource);
		return;
	}
	else
	{
		s += 7; // skip past the "http://"
		i = 0;

		while(*s && *s != '/')
		{
			svlist_domain[i] = *s;
			s++;
			i++;
		}

		svlist_domain[i] = 0; // terminate string
	}

	if(!NET_TCPConnect(serverListSocket, svlist_domain, 80))
	{
		Com_Printf("Unable to connect to %s\n", svlist_domain);
		return;	// Couldn't connect
	}

	// We're connected! Lets ask for the list
	{
		char msg[256];
		int len, bytes_sent;

		//sprintf(msg, "GET %s HTTP/1.0\n\n", s);
		sprintf(msg, "GET %s HTTP/1.0\n\n", sServerSource);

		len = strlen(msg);
		bytes_sent = send(serverListSocket, msg, len, 0);

		if  (bytes_sent < len)
		{
			Com_Printf ("HTTP Server did not accept request, aborting\n");
			closesocket(serverListSocket);
			return;
		}
	}

	// We've sent our request... Lets read in what they've got for us
	{
		char *buffer	= malloc(32767);	
		char *current	= buffer;
		char *found		= NULL;
		int WhichOn		= 0;
		FILE			*serverlistfile;
		int numread = 0;
		int bytes_read = 0;

		// Read in up to 32767 bytes
		while (numread < 32760 && 0 < (bytes_read = recv(serverListSocket, buffer + numread, 32766 - numread, 0)))
		{
			numread += bytes_read;
		};

		if (bytes_read == -1)
		{
			Com_Printf("WARNING: recv(): %s\n", NET_ErrorString());
			free(buffer);
			closesocket(serverListSocket);
			return;
		}

		if (strstr(buffer, "<html>"))
		{
			Com_Printf("WARNING: unable to read %s\n", sServerSource);
			free(buffer);
			return;
		}

		closesocket(serverListSocket);

		// Okay! We have our data... now lets parse it! =)
		buffer[numread] = 0; // terminate data.
		current = buffer;

		// find \n\n, thats the end of header/beginning of the data
		while (*current != '\n' || *(current+2) != '\n')
		{
			if (current > buffer + numread)
			{
				free(buffer);
				return; 
			}

			current++;
		};

		current = current + 3; // skip the trailing \n.  We're at the beginning of the data now

		serverlistfile = fopen(va("%s/servers.txt", FS_Gamedir()), "w");

		if(!serverlistfile)
			return;
		
		while (current < buffer + numread)
		{
			found = current;						// Mark the beginning of the line

			while (*current && *current != 13)
			{										// Find the end of the line
				current ++; 
				
				if (current > buffer + numread)
				{
					free(buffer);
					return;	// Exit if we run out of room
				}

				if (*(current-1) == 'X' && *(current) == 13)
				{
					goto done; // Exit if we find a X\n on a new line
				}
			}

			*current = 0;								// NULL terminate the string
			fprintf(serverlistfile, "%s\n", found);		// Copy line to local file
			current += 2;								// Start at the next line
		};

done:
		fclose(serverlistfile);
		free(buffer);

		return;
	}
}

void *M_ServerlistUpdate_multithreaded (void *ptr)
{
	M_ServerlistUpdate(serverlist_source->string);
	// jitodo - serverlist_source2->string
	M_ServerlistRefresh_f();
	updating = false;
	return NULL;
}

void M_ServerlistUpdate_f (void)
{
	if (!updating)
	{
		updating = true;
		pthread_create(&updatethread, NULL, M_ServerlistUpdate_multithreaded, NULL);;
	}
}


static void create_serverlist (int size)
{
	register int i;

	memset(&m_serverlist, 0, sizeof(m_serverlist_t));
	m_serverlist.actualsize = size;
	m_serverlist.info = Z_Malloc(sizeof(char*)*size); 
	m_serverlist.ips = Z_Malloc(sizeof(char*)*size);
	m_serverlist.server = Z_Malloc(sizeof(m_serverlist_server_t)*size);

	for (i=0; i<size; i++)
	{
		memset(&m_serverlist.server[i], 0, sizeof(m_serverlist_server_t));
		m_serverlist.server[i].remap = -1;
	}
	//m_serverlist.remap = Z_Malloc(sizeof(int)*size);
	//memset(m_serverlist.remap, -1, sizeof(int)*size);
	//m_serverlist.adr = Z_Malloc(sizeof(netadr_t)*size);
}

static __inline char *skip_string (char *s)
{
	while (*s)
		s++;
	s++;
	return s;
}

static qboolean serverlist_load (void)
{
	int size;
	char *data;
	char *ptr;

	size = FS_LoadFile("serverlist.dat", &data);

	if (size > -1)
	{
		int endiantest;
		int actualsize;
		register int i;

		// Check header to make sure it's a valid file:
		if (memcmp(data, "PB2Serverlist1.00", sizeof("PB2Serverlist1.00")-1) != 0)
		{
			FS_FreeFile(data);
			return false;
		}
		ptr = data + sizeof("PB2Serverlist1.00")-1;
		memcpy(&endiantest, ptr, sizeof(int));
		if (endiantest != 123123123)
		{
			FS_FreeFile(data);
			return false;
		}

		// Read our data:
		ptr += sizeof(int);
		memcpy(&actualsize, ptr, sizeof(int));
		ptr += sizeof(int);
		create_serverlist(actualsize);
		memcpy(&m_serverlist.numservers, ptr, sizeof(int));
		ptr += sizeof(int);
		memcpy(&m_serverlist.nummapped, ptr, sizeof(int));
		ptr += sizeof(int);
		//memcpy(m_serverlist.adr, ptr, sizeof(netadr_t)*m_serverlist.numservers);
		//ptr += sizeof(netadr_t)*m_serverlist.numservers;
		//memcpy(m_serverlist.remap, ptr, sizeof(int)*m_serverlist.nummapped);
		//ptry += sizeof(int)*m_serverlist.nummapped;

		for (i=0; i<m_serverlist.numservers; i++)
		{
			memcpy(&m_serverlist.server[i].adr, ptr, sizeof(netadr_t));
			ptr += sizeof(netadr_t);
			m_serverlist.server[i].remap = *(short*)ptr;
			ptr += sizeof(short);
			m_serverlist.server[i].servername = text_copy(ptr);
			ptr = skip_string(ptr);
			m_serverlist.server[i].mapname = text_copy(ptr);
			ptr = skip_string(ptr);
			m_serverlist.server[i].ping = *(unsigned short*)ptr;
			ptr += sizeof(unsigned short);
			m_serverlist.server[i].players = *(unsigned char*)ptr;
			ptr += sizeof(unsigned char);
			m_serverlist.server[i].maxplayers = *(unsigned char*)ptr;
			ptr += sizeof(unsigned char);
		}

		for (i=0; i<m_serverlist.nummapped; i++)
		{
			m_serverlist.ips[i] = text_copy(ptr);
			ptr = skip_string(ptr);
			m_serverlist.info[i] = text_copy(ptr);
			ptr = skip_string(ptr);
		}

		FS_FreeFile(data);
		return true;
	}

	return false;
}


void Serverlist_Init (void)
{
	// Init semaphores:
	sem_init(&m_sem_serverlist, 0, 1);

	// Init server list:
	if (!serverlist_load())
	{
		create_serverlist(INITIAL_SERVERLIST_SIZE);
	}

	// Add scommands:
	Cmd_AddCommand("serverlist_update", M_ServerlistUpdate_f);
	Cmd_AddCommand("serverlist_refresh", M_ServerlistRefresh_f);
	Cmd_AddCommand("serverlist_print", M_ServerlistPrint_f);
	Cmd_AddCommand("serverlist_clear", Serverlist_Clear_f);
}


static void serverlist_save (void)
{
	FILE *fp;
	char szFilename[MAX_QPATH];

	sprintf(szFilename, "%s/serverlist.dat", FS_Gamedir());

	if (fp = fopen(szFilename, "wb"))
	{
		int endiantest = 123123123;
		register int i;
		short stemp;
		unsigned char ctemp;
		char *s;

		// Write Header:
		fwrite("PB2Serverlist1.00", sizeof("PB2Serverlist1.00")-1, 1, fp);
		fwrite(&endiantest, sizeof(int), 1, fp);
		
		// Write our data:
		fwrite(&m_serverlist.actualsize, sizeof(int), 1, fp);
		fwrite(&m_serverlist.numservers, sizeof(int), 1, fp);
		fwrite(&m_serverlist.nummapped, sizeof(int), 1, fp);
		
		for (i=0; i<m_serverlist.numservers; i++)
		{
			fwrite(&m_serverlist.server[i].adr, sizeof(netadr_t), 1, fp);
			stemp = m_serverlist.server[i].remap;
			fwrite(&stemp, sizeof(short), 1, fp);
			s = m_serverlist.server[i].servername;
			fwrite(s, strlen(s)+1, 1, fp);
			s = m_serverlist.server[i].mapname;
			fwrite(s, strlen(s)+1, 1, fp);
			stemp = (short)m_serverlist.server[i].ping;
			fwrite(&stemp, sizeof(short), 1, fp);
			ctemp = (unsigned char)m_serverlist.server[i].players;
			fwrite(&ctemp, sizeof(unsigned char), 1, fp);
			ctemp = (unsigned char)m_serverlist.server[i].maxplayers;
			fwrite(&ctemp, sizeof(unsigned char), 1, fp);
		}

		for (i=0; i<m_serverlist.nummapped; i++)
		{
			s = m_serverlist.ips[i];
			fwrite(s, strlen(s)+1, 1, fp);
			s = m_serverlist.info[i];
			fwrite(s, strlen(s)+1, 1, fp);
		}

		fclose(fp);
	}
}


void Serverlist_Shutdown (void)
{
	serverlist_save();
}

