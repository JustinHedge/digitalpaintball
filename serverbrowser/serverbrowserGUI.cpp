/*
Copyright (C) 2006 Nathan Wulf (jitspoe) / Digital Paint

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

#include "serverbrowser.h"
#include <commctrl.h>
#include <windowsx.h>

#define MAX_LOADSTRING 100
#define WM_TRAY_ICON_NOTIFY_MESSAGE (WM_USER + 1)

// Global Variables:
static HWND g_hStatus;
static HWND g_hServerList;
static HWND g_hPlayerList;
static HWND g_hServerInfoList;
static bool g_abSortDirServers[8] = { false, false, true, false, false }; // Make # players sort highest to lowest by default
static bool g_abSortDirPlayers[8];
static bool g_abSortDirServerInfo[4];
static HINSTANCE g_hInst;
static NOTIFYICONDATA g_tTrayIcon;
static HMENU g_hMenuTray;
static HMENU g_hMenuRightClick;
static int g_nStatusBarHeight = 20;
static DWORD g_dwDefaultTextColor = RGB(0, 0, 0); // todo - obtain defaults from windows api
static DWORD g_dwDefaultTextBackground = RGB(255, 255, 255); // todo
static DWORD g_dwWhiteTextBackground = RGB(255, 255, 255); // todo
static DWORD g_dwRedTextBackground = RGB(180, 5, 0); // todo - might need tweaking
static DWORD g_dwBlueTextBackground = RGB(10, 40, 150);
static DWORD g_dwPurpleTextBackground = RGB(120, 40, 120);
static DWORD g_dwYellowTextBackground = RGB(255, 255, 80);
static DWORD g_dwBlackTextColor = RGB(0, 0, 0);
static DWORD g_dwWhiteTextColor = RGB(255, 255, 255);
static DWORD g_dwFadedTextColor = RGB(160, 160, 160); // Greyed out color
static int g_nServerlistSortColumn = -1;

char g_szGameDir[256];
string g_sCurrentServerAddress;

// Foward declarations of functions included in this code module:
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About (HWND, UINT, WPARAM, LPARAM);


int APIENTRY WinMain (HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR     lpCmdLine,
                      int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	WNDCLASSEX wcex;
	HWND hWnd;
	TCHAR szTitle[MAX_LOADSTRING];
	TCHAR szWindowClass[MAX_LOADSTRING];
	HMENU hMenu;
	hMenu = NULL;

	char Language[10];
	DWORD destSize;
	destSize=10;
	char szServerbrowserINI[256];
	_snprintf(szServerbrowserINI, sizeof(szServerbrowserINI), "%s\\serverbrowser.ini", g_szGameDir);
	GetPrivateProfileString("Serverbrowser","Language","NULL",Language,destSize,szServerbrowserINI);

/*	if(Language == "English"){
		TODO: Load english menu.
	} else {
		TODO: Lade deutsches Menü.
	}
*/
	UINT uState = GetMenuState(hMenu,IDM_LANG_OPTION1,MF_BYCOMMAND);

	// Create the application window
	if(uState & MFS_CHECKED){
		LoadString(hInstance, IDS_APP_TITLE1, szTitle, MAX_LOADSTRING);
		LoadString(hInstance, IDC_SERVERBROWSER1, szWindowClass, MAX_LOADSTRING);
	} else {
		LoadString(hInstance, IDS_APP_TITLE2, szTitle, MAX_LOADSTRING);
		LoadString(hInstance, IDC_SERVERBROWSER2, szWindowClass, MAX_LOADSTRING);
	}
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize			= sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_SERVERBROWSER);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	if(uState & MFS_CHECKED){
		wcex.lpszMenuName	= (LPCSTR)IDC_SERVERBROWSER1;
	} else {
		wcex.lpszMenuName	= (LPCSTR)IDC_SERVERBROWSER2;
	}
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	RegisterClassEx(&wcex);
	g_hInst = hInstance;
	hWnd = CreateWindowEx(0, szWindowClass, szTitle,
		WS_POPUP | WS_BORDER |
		WS_CAPTION | WS_THICKFRAME |
		WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		100, 100, 700, 400, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	if(uState & MFS_CHECKED){
		hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SERVERBROWSER1);
		hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_TRAY1));
	} else {
		hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SERVERBROWSER2);
		hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_TRAY2));
	}
	g_hMenuTray = GetSubMenu(hMenu, 0);
	g_hMenuRightClick = GetSubMenu(hMenu, 1);
	InitApp();

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(hWnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DestroyMenu(hMenu);
	ShutdownApp();
	return msg.wParam;
}


static BOOL OnCreate (HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	int i;
	char *pServerList[] = { "C", "PW", "GLS", "Server Name", "Map", "Players", "Ping", "Address" };
	int iaServerListWidths[] = { 25, 30, 35, 250, 100, 50, 38, -2 };
	char *pPlayerList[] = { "Player Name", "Kills", "Ping" };
	int iaPlayerListWidths[] = { 200, 60, -2 };
	char *pInfoList[] = { "Variable", "Value" };
	int iaInfoListWidths[] = { 100, -2 };
	LV_COLUMN lvColumn;
	RECT rect;

	memset(&lvColumn, 0, sizeof(lvColumn));
	lvColumn.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;

	// Status Bar
	g_hStatus = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS |
		WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP,
		"Ready.", hWnd, IDC_STATUSBAR);
	ShowWindow(g_hStatus, SW_SHOW);
	
	if (SendMessage(g_hStatus, SB_GETRECT, 0, (LPARAM)&rect))
		g_nStatusBarHeight = rect.bottom;

	// Main Server List
	g_hServerList = CreateWindowEx(0, WC_LISTVIEW, "",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_AUTOARRANGE | LVS_SINGLESEL,
		0, 0, 300, 200, hWnd, NULL, g_hInst, NULL);
	ListView_SetExtendedListViewStyle(g_hServerList, LVS_EX_FULLROWSELECT);

	for (i = 0; i < 8; i++)
	{
		lvColumn.cx = 150;
		lvColumn.iSubItem = i;
		lvColumn.pszText = pServerList[i];

		if (ListView_InsertColumn(g_hServerList, i, &lvColumn) == -1)
			return FALSE;

		ListView_SetColumnWidth(g_hServerList, i, iaServerListWidths[i]);
	}

	// Player List
	g_hPlayerList = CreateWindowEx(0, WC_LISTVIEW, "",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_AUTOARRANGE | LVS_SINGLESEL,
		0, 0, 300, 200, hWnd, NULL, g_hInst, NULL);
	ListView_SetExtendedListViewStyle(g_hPlayerList, LVS_EX_FULLROWSELECT);

	for (i = 0; i < 3; i++)
	{
		lvColumn.cx = 150;
		lvColumn.iSubItem = i;
		lvColumn.pszText = pPlayerList[i];

		if (ListView_InsertColumn(g_hPlayerList, i, &lvColumn) == -1)
			return FALSE;

		ListView_SetColumnWidth(g_hPlayerList, i, iaPlayerListWidths[i]);
	}

	// Server Info List
	g_hServerInfoList = CreateWindowEx(0, WC_LISTVIEW, "",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_AUTOARRANGE | LVS_SINGLESEL,
		0, 0, 300, 200, hWnd, NULL, g_hInst, NULL);
	ListView_SetExtendedListViewStyle(g_hServerInfoList, LVS_EX_FULLROWSELECT);

	for (i = 0; i < 2; i++)
	{
		lvColumn.cx = 150;
		lvColumn.iSubItem = i;
		lvColumn.pszText = pInfoList[i];

		if (ListView_InsertColumn(g_hServerInfoList, i, &lvColumn) == -1)
			return FALSE;

		ListView_SetColumnWidth(g_hServerInfoList, i, iaInfoListWidths[i]);
	}

	// Tray icon
	memset(&g_tTrayIcon, 0, sizeof(g_tTrayIcon));
	g_tTrayIcon.cbSize = sizeof(g_tTrayIcon);
	g_tTrayIcon.hIcon = LoadIcon(g_hInst, (LPCTSTR)IDI_SMALL);
	g_tTrayIcon.hWnd = hWnd;
	strcpy_s(g_tTrayIcon.szTip, "Server Browser");
	g_tTrayIcon.uCallbackMessage = WM_TRAY_ICON_NOTIFY_MESSAGE;
	g_tTrayIcon.uFlags = NIF_ICON|NIF_TIP|NIF_MESSAGE;
	g_tTrayIcon.uID = 0;
	Shell_NotifyIcon(NIM_ADD, &g_tTrayIcon);

	return TRUE;
}


static void TweakListviewWidths (void)
{
	ListView_SetColumnWidth(g_hServerList, SERVERLIST_ADDRESS_OFFSET, -2);
	ListView_SetColumnWidth(g_hPlayerList, 2, -2);
	ListView_SetColumnWidth(g_hServerInfoList, 1, -2);
}


static void OnResize (HWND hWnd, UINT state, int cx, int cy)
{
	if (state != SIZE_MINIMIZED)
	{
		RECT rc;

		GetClientRect(hWnd, &rc);
		rc.bottom -= g_nStatusBarHeight;
		MoveWindow(g_hServerList, rc.left, rc.top, rc.right, (rc.bottom + 3) / 2, TRUE);
		MoveWindow(g_hPlayerList, rc.left, (rc.bottom + 1) / 2, (rc.right + 3) / 2, rc.bottom / 2, TRUE);
		MoveWindow(g_hServerInfoList, (rc.right + 1) / 2, (rc.bottom + 1) / 2, rc.right / 2, rc.bottom / 2, TRUE);
		TweakListviewWidths();
		FORWARD_WM_SIZE(g_hStatus, state, cx, cy, SendMessage);
	}
}


static BOOL CopyAddress (HWND hWnd, int nItem, copytype_t eCopyType)
{
	char szAddress[64];
	char szHostname[64];
	char szFullText[1024];
	int nLen = 0;
	char *pClipboardData;
	HGLOBAL hClipboard;

	ListView_GetItemText(g_hServerList, nItem, SERVERLIST_ADDRESS_OFFSET,
		szAddress, sizeof(szAddress));

	switch (eCopyType)
	{
	case COPYTYPE_ADDRESS:
		strncpyz(szFullText, szAddress, sizeof(szFullText));
		nLen = strlen(szFullText);
		break;
	case COPYTYPE_URL:
		nLen = _snprintf_s(szFullText, sizeof(szFullText), "paintball2://%s", szAddress);
		szFullText[sizeof(szFullText)-1] = 0;
		break;
	case COPYTYPE_NAMEIP:
		ListView_GetItemText(g_hServerList, nItem, SERVERLIST_HOSTNAME_OFFSET,
				szHostname, sizeof(szHostname));
		nLen = _snprintf_s(szFullText, sizeof(szFullText), "%s - %s", szHostname, szAddress);
		szFullText[sizeof(szFullText)-1] = 0;
		break;
	case COPYTYPE_FULL:
		{
			char szCertificatedServer[16];
			char szNeedPassword[16];
			char szGLS[16];
			char szPing[16];
			char szMap[32];
			char szPlayers[16];

			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_CERTIFICATEDSERVER_OFFSET,
				szCertificatedServer, sizeof(szCertificatedServer));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_NEEDPASSWORD_OFFSET,
				szNeedPassword, sizeof(szNeedPassword));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_GLS_OFFSET,
				szGLS, sizeof(szGLS));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_HOSTNAME_OFFSET,
				szHostname, sizeof(szHostname));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_PING_OFFSET,
				szPing, sizeof(szPing));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_MAP_OFFSET,
				szMap, sizeof(szMap));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_PLAYERS_OFFSET,
				szPlayers, sizeof(szPlayers));
			nLen = _snprintf_s(szFullText, sizeof(szFullText), "%s|%s|%s|%s|%s|%s|%s|%s",
				szAddress, szCertificatedServer, szNeedPassword, szGLS, szHostname, szPing, szMap, szPlayers);
			szFullText[sizeof(szFullText)-1] = 0;
			break;
		}
	}

	if (!nLen)
		return FALSE;

	if (!OpenClipboard(hWnd))
        return FALSE;

	hClipboard = GlobalAlloc(GMEM_MOVEABLE, (nLen + 1));

	if (!hClipboard)
	{
		CloseClipboard();
		return FALSE;
	}

	EmptyClipboard();
	pClipboardData = (char *)GlobalLock(hClipboard);
	strcpy(pClipboardData, szFullText);
	GlobalUnlock(hClipboard);
	SetClipboardData(CF_TEXT, pClipboardData);
	CloseClipboard();

	return TRUE;
}


static BOOL OnCommand (HWND hWnd, int wmId, HWND hWndCtl, UINT codeNotify)
{
	char szServerbrowserINI[256];
	_snprintf(szServerbrowserINI, sizeof(szServerbrowserINI), "%s\\serverbrowser.ini", g_szGameDir);

	switch (wmId)
	{
	case IDM_ABOUT:
		DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
		return TRUE;
	case IDM_UPDATE:
		UpdateList();
		return TRUE;
	case IDM_REFRESH:
		RefreshList();
		return TRUE;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		return TRUE;
	case ID_TRAY_EXIT:
		DestroyWindow(hWnd);
		return TRUE;
/*	case ID_TRAY_RESTORE:
		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		return TRUE;*/
	case ID_RIGHTCLICK_COPYADDRESS:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_ADDRESS);
	case ID_RIGHTCLICK_COPYURL:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_URL);
	case ID_RIGHTCLICK_COPYFULLSERVERINFO:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_FULL);
	case ID_RIGHTCLICK_COPYNAMEIP:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_NAMEIP);
	case IDM_LANG_OPTION1:
		SetMenu(hWnd, LoadMenu(g_hInst,MAKEINTRESOURCE(IDC_SERVERBROWSER1)));
//		SetMenu(hWnd, LoadMenu(g_hInst,MAKEINTRESOURCE(IDR_MENU_TRAY1)));
		WritePrivateProfileString(TEXT("Serverbrowser"),TEXT("Language"),TEXT("English"),TEXT(szServerbrowserINI));
		SetStatus("English loaded. Please restart the serverbrowser to see all menus in this language.");
		break;
	case IDM_LANG_OPTION2:
	    SetMenu(hWnd, LoadMenu(g_hInst,MAKEINTRESOURCE(IDC_SERVERBROWSER2)));
//	    SetMenu(hWnd, LoadMenu(g_hInst,MAKEINTRESOURCE(IDR_MENU_TRAY2)));
		WritePrivateProfileString(TEXT("Serverbrowser"),TEXT("Language"),TEXT("Deutsch"),TEXT(szServerbrowserINI));
		SetStatus("Deutsch geladen. Bitte den Serverbrowser neu starten, um alle Menüs in dieser Sprache zu sehen.");
		break;
	default:
		return FALSE;
	}
}


static void OnDestroy (HWND hWnd)
{
	// TODO: save registry data
	Shell_NotifyIcon(NIM_DELETE, &g_tTrayIcon);
	PostQuitMessage(0);
}


static BOOL OnErase (HWND hWnd, HDC hdc)
{
	// Erasing makes the window flicker, so do nothing and pretend we handled it:
	return TRUE;
}


// Function for sorting the player list
static int CALLBACK PlayerCompareProc (LPARAM lParam1, LPARAM lParam2, LPARAM lParam3)
{
	char szBuff1[64];
	char szBuff2[64];
	LVFINDINFO lvFindInfo;
	int nItem1, nItem2;

	lvFindInfo.flags = LVFI_PARAM;
	lvFindInfo.lParam = lParam1;
	nItem1 = ListView_FindItem(g_hPlayerList, -1, &lvFindInfo);
	lvFindInfo.lParam = lParam2;
	nItem2 = ListView_FindItem(g_hPlayerList, -1, &lvFindInfo);
	ListView_GetItemText(g_hPlayerList, nItem1, lParam3, szBuff1, sizeof(szBuff1));
	ListView_GetItemText(g_hPlayerList, nItem2, lParam3, szBuff2, sizeof(szBuff2));

	if (lParam3 == PLAYERLIST_NAME_OFFSET)
	{
		if (g_abSortDirPlayers[lParam3])
			return _stricmp(szBuff1, szBuff2);
		else
			return _stricmp(szBuff2, szBuff1);
	}
	else
	{
		if (g_abSortDirServers[lParam3])
			return atoi(szBuff1) - atoi(szBuff2);
		else
			return atoi(szBuff2) - atoi(szBuff1);
	}
}


// Function for sorting the server list
static int CALLBACK ServerInfoCompareProc (LPARAM lParam1, LPARAM lParam2, LPARAM lParam3)
{
	char szBuff1[64];
	char szBuff2[64];
	LVFINDINFO lvFindInfo;
	int nItem1, nItem2;

	lvFindInfo.flags = LVFI_PARAM;
	lvFindInfo.lParam = lParam1;
	nItem1 = ListView_FindItem(g_hServerInfoList, -1, &lvFindInfo);
	lvFindInfo.lParam = lParam2;
	nItem2 = ListView_FindItem(g_hServerInfoList, -1, &lvFindInfo);
	ListView_GetItemText(g_hServerInfoList, nItem1, lParam3, szBuff1, sizeof(szBuff1));
	ListView_GetItemText(g_hServerInfoList, nItem2, lParam3, szBuff2, sizeof(szBuff2));

	if (g_abSortDirServerInfo[lParam3])
		return _stricmp(szBuff1, szBuff2);
	else
		return _stricmp(szBuff2, szBuff1);
}


static bool ServerActive (int nRow)
{
	char szAddress[64];

	ListView_GetItemText(g_hServerList, nRow, SERVERLIST_ADDRESS_OFFSET, szAddress, sizeof(szAddress));

	return ServerActive(szAddress);
}


// Function for sorting the server list
static int CALLBACK ServerCompareProc (LPARAM lParam1, LPARAM lParam2, LPARAM lParam3)
{
	char szBuff1[64];
	char szBuff2[64];
	LVFINDINFO lvFindInfo;
	int nItem1, nItem2;
	bool bActive1, bActive2;

	lvFindInfo.flags = LVFI_PARAM;
	lvFindInfo.lParam = lParam1;
	nItem1 = ListView_FindItem(g_hServerList, -1, &lvFindInfo);
	lvFindInfo.lParam = lParam2;
	nItem2 = ListView_FindItem(g_hServerList, -1, &lvFindInfo);
	ListView_GetItemText(g_hServerList, nItem1, lParam3, szBuff1, sizeof(szBuff1));
	ListView_GetItemText(g_hServerList, nItem2, lParam3, szBuff2, sizeof(szBuff2));
	bActive1 = ServerActive(nItem1);
	bActive2 = ServerActive(nItem2);

#ifdef SEPARATE_INACTIVE
	if (bActive1 == bActive2)
	{
#endif
		if (lParam3 == SERVERLIST_PLAYERS_OFFSET || lParam3 == SERVERLIST_PING_OFFSET)
		{
			if (g_abSortDirServers[lParam3])
				return atoi(szBuff1) - atoi(szBuff2);
			else
				return atoi(szBuff2) - atoi(szBuff1);
		}
		else
		{
			if (g_abSortDirServers[lParam3])
				return _stricmp(szBuff1, szBuff2);
			else
				return _stricmp(szBuff2, szBuff1);
		}
#ifdef SEPARATE_INACTIVE // Note: does not (always) work because serverlist isn't resorted on activity change
	}
	else
	{
		// Put inactive (greyed-out) servers at the bottom of the list
		return bActive1 < bActive2;
	}
#endif
}


static bool MapExists (int nRow)
{
	char szAddress[64];

	ListView_GetItemText(g_hServerList, nRow, SERVERLIST_ADDRESS_OFFSET, szAddress, sizeof(szAddress));

	return MapExistsForAddress(szAddress);
}


static void LaunchGameFromList (int nRow)
{
	char szAddress[64];

	ListView_GetItemText(g_hServerList, nRow, SERVERLIST_ADDRESS_OFFSET, szAddress, sizeof(szAddress));
	LaunchGame(szAddress, NULL);
}


static LRESULT ProcessCustomDraw (LPARAM lParam)
{
    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

    switch (lplvcd->nmcd.dwDrawStage) 
    {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;
	case CDDS_SUBITEM|CDDS_ITEMPREPAINT:
		switch (lplvcd->iSubItem)
		{
		case SERVERLIST_MAP_OFFSET:
			lplvcd->clrText = ServerActive(lplvcd->nmcd.dwItemSpec) ? (MapExists(lplvcd->nmcd.dwItemSpec) ? g_dwDefaultTextColor : RGB(255, 0, 0)) : g_dwFadedTextColor;
			lplvcd->clrTextBk = g_dwDefaultTextBackground;
			return CDRF_NEWFONT;
		default:
			lplvcd->clrText = ServerActive(lplvcd->nmcd.dwItemSpec) ? g_dwDefaultTextColor : g_dwFadedTextColor;
			lplvcd->clrTextBk = g_dwDefaultTextBackground;
			return CDRF_DODEFAULT;
		}
    }

    return CDRF_DODEFAULT;
}


static char GetPlayerColor (int nRow)
{
	map<string, serverinfo_t>::iterator mIterator;
	mIterator = g_mServers.find(g_sCurrentServerAddress);

	if (mIterator != g_mServers.end())
	{
		return mIterator->second.mPlayerColors[nRow];
	}
	else
	{
		return '?'; // todo
	}
}


static LRESULT CustomDrawPlayerList (LPARAM lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	char cColor;

	switch (lplvcd->nmcd.dwDrawStage) 
    {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		cColor = GetPlayerColor(lplvcd->nmcd.lItemlParam);

		switch (cColor)
		{
		case 'r':
		case 'b':
		case 'p':
			lplvcd->clrText = g_dwWhiteTextColor;
			break;
		case 'y':
		case 'o':
		default:
			lplvcd->clrText = g_dwBlackTextColor;
		}

		switch (cColor)
		{
		case 'r':
			lplvcd->clrTextBk = g_dwRedTextBackground;
			break;
		case 'b':
			lplvcd->clrTextBk = g_dwBlueTextBackground;
			break;
		case 'p':
			lplvcd->clrTextBk = g_dwPurpleTextBackground;
			break;
		case 'y':
			lplvcd->clrTextBk = g_dwYellowTextBackground;
			break;
		case 'o':
		default:
			lplvcd->clrTextBk = g_dwWhiteTextBackground;
			break;
		}

		return CDRF_NEWFONT;
    }

    return CDRF_DODEFAULT;
}


static void UpdateInfoLists (int nID, bool bRefresh)
{
	char szAddress[128];
	map<string, serverinfo_t>::iterator mIterator;

	ListView_GetItemText(g_hServerList, nID, SERVERLIST_ADDRESS_OFFSET, szAddress, sizeof(szAddress));

	if (bRefresh)
		PingServer(szAddress);

	ListView_DeleteAllItems(g_hPlayerList);
	ListView_DeleteAllItems(g_hServerInfoList);
	mIterator = g_mServers.find(szAddress);

	if (mIterator != g_mServers.end())
	{
		int i;
		LVITEM tLVItem;
		vector<playerinfo_t>::iterator itrPlayers;
		vector<playerinfo_t>::iterator itrPlayersEnd;
		map<string, string>::iterator itrCvars;
		map<string, string>::iterator itrCvarsEnd;
		char szTemp[128];
		string sStatusBar;
		const char *sTimeLeft = NULL;
		const char *sScores = NULL;

		memset(&tLVItem, 0, sizeof(tLVItem));
		tLVItem.mask = LVIF_TEXT|LVIF_PARAM;
		g_sCurrentServerAddress = szAddress;

		// Update the player list
		itrPlayers = mIterator->second.vPlayers.begin();
		itrPlayersEnd = mIterator->second.vPlayers.end();

		for (i = 0; itrPlayers != itrPlayersEnd; ++i, ++itrPlayers)
		{
			tLVItem.iItem = i;
			tLVItem.lParam = i;
			tLVItem.pszText = (char *)itrPlayers->sName.c_str();
			nID = ListView_InsertItem(g_hPlayerList, &tLVItem);
			ListView_SetItemText(g_hPlayerList, nID, 1, _itoa(itrPlayers->nScore, szTemp, 10));
			ListView_SetItemText(g_hPlayerList, nID, 2, _itoa(itrPlayers->nPing, szTemp, 10));
		}

		// Update server cvar list
		itrCvars = mIterator->second.mVars.begin();
		itrCvarsEnd = mIterator->second.mVars.end();

		for (i = 0; itrCvars != itrCvarsEnd; ++i, ++itrCvars)
		{
			tLVItem.iItem = i;
			tLVItem.lParam = i;
			tLVItem.pszText = (char *)itrCvars->first.c_str();
			nID = ListView_InsertItem(g_hServerInfoList, &tLVItem);
			ListView_SetItemText(g_hServerInfoList, nID, 1, (char *)itrCvars->second.c_str());

			if (itrCvars->first == "_scores")
				sScores = itrCvars->second.c_str();

			if (itrCvars->first == "TimeLeft")
				sTimeLeft = itrCvars->second.c_str();
		}

		if (sScores)
		{
			sStatusBar = sScores;
		
			if (sTimeLeft)
				sStatusBar += "with ";
		}

		if (sTimeLeft)
		{
			sStatusBar += sTimeLeft;
			sStatusBar += " left in match.";
		}

		SetStatus(sStatusBar.c_str());
	}

	TweakListviewWidths();
}


static void CorrectRowIds (HWND hList)
{
	int nCount, i;

	nCount = ListView_GetItemCount(hList);
//todo;
	for (i = 0; i < nCount; ++i)
	{
		LVITEM tLVItem;

		memset(&tLVItem, 0, sizeof(tLVItem));
		ListView_GetItem(hList, &tLVItem);
	}
}


static void SortServerList (void)
{
	if (g_nServerlistSortColumn > -1)
		ListView_SortItems(g_hServerList, ServerCompareProc, g_nServerlistSortColumn);
}


static BOOL OnNotify (HWND hWnd, int idFrom, NMHDR FAR *pnmhdr)
{
	BOOL bRet;
	POINT pt;

	if (!pnmhdr)
		return FALSE; // shouldn't happen

	if (pnmhdr->hwndFrom == g_hServerList)
	{
		NM_LISTVIEW *pNMLV = (NM_LISTVIEW *)pnmhdr;
		LPNMLVKEYDOWN pnmlvkd = (LPNMLVKEYDOWN)pnmhdr;

		switch (pnmhdr->code)
		{
		case LVN_COLUMNCLICK:
			g_abSortDirServers[pNMLV->iSubItem] = !g_abSortDirServers[pNMLV->iSubItem];
			g_nServerlistSortColumn = pNMLV->iSubItem;
			SortServerList();
			break;
		case LVN_KEYDOWN:

			if (pnmlvkd->wVKey == VK_RETURN)
				LaunchGameFromList(ListView_GetSelectionMark(g_hServerList));
			
			break;
		case LVN_ITEMCHANGED:
			UpdateInfoLists(ListView_GetSelectionMark(g_hServerList), false);
			break;
		case NM_CLICK:
			UpdateInfoLists(pNMLV->iItem, true);
			break;
		case NM_DBLCLK:
			UpdateInfoLists(pNMLV->iItem, true);
			LaunchGameFromList(pNMLV->iItem);
			break;
		case NM_RCLICK:
			UpdateInfoLists(pNMLV->iItem, true);
			GetCursorPos(&pt);
			bRet = TrackPopupMenu(g_hMenuRightClick, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
			return OnCommand(hWnd, bRet, NULL, pNMLV->iItem);
		case NM_CUSTOMDRAW:
			return ProcessCustomDraw((LPARAM)pnmhdr);
		}
	}

	if (pnmhdr->hwndFrom == g_hPlayerList)
	{
		NM_LISTVIEW *pNMLV = (NM_LISTVIEW *)pnmhdr;

		switch (pnmhdr->code)
		{
		case LVN_COLUMNCLICK:
			g_abSortDirPlayers[pNMLV->iSubItem] = !g_abSortDirPlayers[pNMLV->iSubItem];
			ListView_SortItems(g_hPlayerList, PlayerCompareProc, pNMLV->iSubItem);
			break;
		case NM_CUSTOMDRAW:
			return CustomDrawPlayerList((LPARAM)pnmhdr);
		}
	}

	if (pnmhdr->hwndFrom == g_hServerInfoList)
	{
		NM_LISTVIEW *pNMLV = (NM_LISTVIEW *)pnmhdr;

		switch (pnmhdr->code)
		{
		case LVN_COLUMNCLICK:
			g_abSortDirServerInfo[pNMLV->iSubItem] = !g_abSortDirServerInfo[pNMLV->iSubItem];
			ListView_SortItems(g_hServerInfoList, ServerInfoCompareProc, pNMLV->iSubItem);
			break;
		}
	}

	return FORWARD_WM_NOTIFY(hWnd, idFrom, pnmhdr, DefWindowProc);
}


static BOOL OnTrayNotify (HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	BOOL bRet;

	switch (lParam)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		return TRUE;
	case WM_RBUTTONDOWN:
		GetCursorPos(&pt);
		bRet = TrackPopupMenu(g_hMenuTray, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
		return OnCommand(hWnd, bRet, NULL, 0);
	default:
		return FALSE;
	}
}


static BOOL OnSysCommand (HWND hWnd, UINT cmd, WPARAM wParam, LPARAM lParam)
{
	switch (cmd)
	{
	case SC_MINIMIZE:
		ShowWindow(hWnd, SW_HIDE);
		return TRUE;
	default:
		FORWARD_WM_SYSCOMMAND(hWnd, cmd, wParam, lParam, DefWindowProc);
		return FALSE;
	}
}


static LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		HANDLE_MSG(hWnd, WM_NOTIFY, OnNotify);
		HANDLE_MSG(hWnd, WM_SIZE, OnResize);
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hWnd, WM_SYSCOMMAND, OnSysCommand);
		HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hWnd, WM_ERASEBKGND, OnErase);
		HANDLE_MSG(hWnd, WM_TRAY_ICON_NOTIFY_MESSAGE, OnTrayNotify);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


// Mesage handler for about box.
static LRESULT CALLBACK About (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}

    return FALSE;
}


void SetStatus (const char *s)
{
	SendMessage(g_hStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)s);
}


// Returns true if the text changes
static bool UpdateListviewText (HWND hList, int nItem, int nColumn, const char *sText)
{
	char szField[1024];

	ListView_GetItemText(hList, nItem, nColumn, szField, sizeof(szField));
	ListView_SetItemText(hList, nItem, nColumn, (char *)sText);

	return strcmp(sText, szField) != 0;
}


int GetListIDFromAddress (const char *sAddress)
{
	int nID;
	int nCount = ListView_GetItemCount(g_hServerList);
	char szAddress[128];

	for (nID = 0; nID < nCount; ++nID)
	{
		ListView_GetItemText(g_hServerList, nID, SERVERLIST_ADDRESS_OFFSET, szAddress, sizeof(szAddress));

		if (strcmp(szAddress, sAddress) == 0)
			return nID;
	}

	return -1;
}


void UpdateServerListGUI (const char *sAddress, serverinfo_t &tServerInfo)
{
	int nID;
	char szTemp[128];
	bool bFound = false;
	bool bUpdated = false;
	int nCount = ListView_GetItemCount(g_hServerList);

	// Find a matching address to update
	nID = GetListIDFromAddress(sAddress);

	// Add a new server to the list if the address is not found.
	if (nID < 0)
	{
		LVITEM tLVItem;

		memset(&tLVItem, 0, sizeof(tLVItem));
		tLVItem.mask = LVIF_TEXT|LVIF_PARAM;
		tLVItem.iSubItem = 0;
		tLVItem.iItem = nCount;
		tLVItem.lParam = nCount;
		tLVItem.pszText = "";
		nID = ListView_InsertItem(g_hServerList, &tLVItem);
		TweakListviewWidths();
	}

	// set address first so we can reference it
	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_ADDRESS_OFFSET, sAddress) &&
		g_nServerlistSortColumn == SERVERLIST_ADDRESS_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_CERTIFICATEDSERVER_OFFSET, _itoa(tServerInfo.nCertificatedServer, szTemp, 10)) &&
		g_nServerlistSortColumn == SERVERLIST_CERTIFICATEDSERVER_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_NEEDPASSWORD_OFFSET, _itoa(tServerInfo.nNeedPassword, szTemp, 10)) &&
		g_nServerlistSortColumn == SERVERLIST_NEEDPASSWORD_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_GLS_OFFSET, _itoa(tServerInfo.nGLS, szTemp, 10)) &&
		g_nServerlistSortColumn == SERVERLIST_GLS_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_HOSTNAME_OFFSET, tServerInfo.sHostName.c_str()) &&
		g_nServerlistSortColumn == SERVERLIST_HOSTNAME_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_MAP_OFFSET, tServerInfo.sMapName.c_str()) &&
		g_nServerlistSortColumn == SERVERLIST_MAP_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	sprintf_s(szTemp, "%d / %d", tServerInfo.nPlayers, tServerInfo.nMaxPlayers);

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_PLAYERS_OFFSET, szTemp) &&
		g_nServerlistSortColumn == SERVERLIST_PLAYERS_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_PING_OFFSET, _itoa(tServerInfo.nPing, szTemp, 10)) &&
		g_nServerlistSortColumn == SERVERLIST_PING_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}
}


void LoadSettings (void)
{
	HKEY hKey;
	DWORD dwSize;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Digital Paint\\Paintball2",
		0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS ||
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Digital Paint\\Paintball2",
		0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		dwSize = sizeof(g_szGameDir);
		RegQueryValueEx(hKey, "INSTDIR", NULL, NULL, (LPBYTE)g_szGameDir, &dwSize);
		RegCloseKey (hKey);
	}
	else
	{
		MessageBox(NULL, "Paintball2 installation directory not found.", "PB2 Server Browser Error", MB_OK);
		strcpy(g_szGameDir, "c:\\games\\paintball2");
	}
}

