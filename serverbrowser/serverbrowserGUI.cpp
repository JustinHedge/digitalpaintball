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


//ADAPTIONS TO COMPILE WITH GCC:
/*
#define strcpy_s strcpy
#define _snprintf_s _snprintf
#define sprintf_s sprintf
*/



#include "serverbrowser.h"
#include <commctrl.h>
#include <windowsx.h>

#define MAX_LOADSTRING 100
#define WM_TRAY_ICON_NOTIFY_MESSAGE (WM_USER + 1)
#define NUM_ICONS 5
#define ICON_SIZE 16

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
static DWORD g_iIconBlank = 0;
static DWORD g_iIconCertificated = 0;
static DWORD g_iIconNeedPassword = 0;
static DWORD g_iIconGLS1 = 0;
static DWORD g_iIconGLS2 = 0;
static bool g_bAlreadyOpen = false;
static DWORD g_dwExistingProcess = 0;

//Edit by Richard
static HWND g_hSearchPlayerDlg;

char g_szGameDir[256];
string g_sCurrentServerAddress;
#define PID_FILE "%USERPROFILE%\\DPPB2_Serverbrowser.pid"



// Ugh, what a pain.  This is a callback for every window
BOOL CALLBACK AlreadyOpenEnumWindowsProc (HWND hWnd, LPARAM lParam)
{
	DWORD dwProcess;
	GetWindowThreadProcessId(hWnd, &dwProcess);
	
	if (g_dwExistingProcess == dwProcess)
	{
		HWND hWndRoot = GetAncestor(hWnd, GA_ROOT);
		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		g_bAlreadyOpen = true;
		return FALSE;
	}

	return TRUE;
}


// If process already exists, give it focus, otherwise save this process id in case somebody tries to open the server browser twice
bool AlreadyOpen ()
{
	char szPIDFile[1024];
	DWORD dwProcess = GetCurrentProcessId();
	DWORD dwWritten;
	DWORD dwRead;

	ExpandEnvironmentStrings(PID_FILE, szPIDFile, sizeof(szPIDFile));

	HANDLE hFile = CreateFile(szPIDFile, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		return false;
	}

	ReadFile(hFile, &g_dwExistingProcess, sizeof(DWORD), &dwRead, NULL);

	if (g_dwExistingProcess && dwRead == sizeof(DWORD))
	{
		EnumWindows(AlreadyOpenEnumWindowsProc, 0);

		if (g_bAlreadyOpen)
		{
			CloseHandle(hFile);
			return true;
		}
	}

	WriteFile(hFile, &dwProcess, sizeof(dwProcess), &dwWritten, NULL);
	CloseHandle(hFile);

	return false;
}


// Don't leave junk in the user's directory
void DeletePIDFile ()
{
	char szPIDFile[1024];

	ExpandEnvironmentStrings(PID_FILE, szPIDFile, sizeof(szPIDFile));
	DeleteFile(szPIDFile);
}


// Foward declarations of functions included in this code module:
//Edit by Richard: These functions are later defined as static, which was missing here
static LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK About (HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK SearchPlayerDlg (HWND, UINT, WPARAM, LPARAM);


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

	if (AlreadyOpen())
	{
		return 0;
	}

	// Create the application window
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SERVERBROWSER, szWindowClass, MAX_LOADSTRING);
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
	wcex.lpszMenuName	= (LPCSTR)IDC_SERVERBROWSER;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	RegisterClassEx(&wcex);
	g_hInst = hInstance;
	hWnd = CreateWindowEx(0, szWindowClass, szTitle,
		WS_POPUP | WS_BORDER |
		WS_CAPTION | WS_THICKFRAME |
		WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		100, 100, 825, 500, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SERVERBROWSER);
	hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_TRAY));
	g_hMenuTray = GetSubMenu(hMenu, 0);
	g_hMenuRightClick = GetSubMenu(hMenu, 1);
	InitApp();

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccelTable, &msg) && !IsDialogMessage (g_hSearchPlayerDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DestroyMenu(hMenu);
	DeletePIDFile();
	ShutdownApp();
	return msg.wParam;
}


static BOOL OnCreate (HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	int i;
	char *pServerList[] = { "C", "PW", "GLS", "Server Name", "Map", "Players", "Ping", "Address" };
//	char *pServerList[] = { "Server Name", "Map", "Players", "Ping", "Address" };
	int iaServerListWidths[] = { 18, 18, 18, 300, 150, 55, 40, -2 };
//	int iaServerListWidths[] = { 350, 150, 55, 40, -2 };
	char *pPlayerList[] = { "Player Name", "Kills", "Ping" };
	int iaPlayerListWidths[] = { 200, 60, -2 };
	char *pInfoList[] = { "Variable", "Value" };
	int iaInfoListWidths[] = { 100, -2 };
	LV_COLUMN lvColumn;
	RECT rect;
    HIMAGELIST hImageList;
    HICON hIcon;

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
	// Edit by Richard: Added LVS_SHOWSELALWAYS style, because when the main program loses focus the other
	// list views will keep their content, so the selected server should still be visible. Also, it improves
	// working with dialogs which select a server in the main window a lot.
	g_hServerList = CreateWindowEx(0, WC_LISTVIEW, "",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_AUTOARRANGE | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, 0, 300, 300, hWnd, NULL, g_hInst, NULL);
	ListView_SetExtendedListViewStyle(g_hServerList, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

	for (i = 0; i < SERVERLIST_OFFSET_MAX; i++)
	{
		lvColumn.cx = 150;
		lvColumn.iSubItem = i;
		lvColumn.pszText = pServerList[i];

		if (ListView_InsertColumn(g_hServerList, i, &lvColumn) == -1)
			return FALSE;

		ListView_SetColumnWidth(g_hServerList, i, iaServerListWidths[i]);
	}

	// Icons for main server list
    if ((hImageList = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_MASK, 0, NUM_ICONS)) == NULL)
	{
        return FALSE;
	}

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_BLANK));
    g_iIconBlank = ImageList_AddIcon(hImageList, hIcon);
	hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_CERTIFICATED));
    g_iIconCertificated = ImageList_AddIcon(hImageList, hIcon);
	hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NEEDPASSWORD));
    g_iIconNeedPassword = ImageList_AddIcon(hImageList, hIcon);
	hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_GLS1));
    g_iIconGLS1 = ImageList_AddIcon(hImageList, hIcon);
	hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_GLS2));
    g_iIconGLS2 = ImageList_AddIcon(hImageList, hIcon);
    ListView_SetImageList(g_hServerList, hImageList, LVSIL_SMALL);

	// Player List
	g_hPlayerList = CreateWindowEx(0, WC_LISTVIEW, "",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_AUTOARRANGE | LVS_SINGLESEL,
		0, 0, 300, 200, hWnd, NULL, g_hInst, NULL);
	ListView_SetExtendedListViewStyle(g_hPlayerList, LVS_EX_FULLROWSELECT);

	for (i = 0; i < PLAYERLIST_OFFSET_MAX; i++)
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
	strcpy_s(g_tTrayIcon.szTip, "Digital Paint: Paintball 2 - Serverbrowser");
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


static void OnRefresh (void)
{
	if (g_hSearchPlayerDlg != 0)
		SendMessage(g_hSearchPlayerDlg, WM_COMMAND, MAKEWPARAM(IDC_SP_UPDATE, 0), (LPARAM) GetDlgItem(g_hSearchPlayerDlg, IDC_SP_UPDATE));
	else
		RefreshList();
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
//			char szCertificated[16];
//			char szNeedPassword[16];
//			char szGLS[16];
			char szPing[16];
			char szMap[32];
			char szPlayers[16];

#if 0
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_CERTIFICATED_OFFSET,
				szCertificated, sizeof(szCertificated));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_NEEDPASSWORD_OFFSET,
				szNeedPassword, sizeof(szNeedPassword));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_GLS_OFFSET,
				szGLS, sizeof(szGLS));
#endif
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_HOSTNAME_OFFSET,
				szHostname, sizeof(szHostname));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_PING_OFFSET,
				szPing, sizeof(szPing));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_MAP_OFFSET,
				szMap, sizeof(szMap));
			ListView_GetItemText(g_hServerList, nItem, SERVERLIST_PLAYERS_OFFSET,
				szPlayers, sizeof(szPlayers));
//			nLen = _snprintf_s(szFullText, sizeof(szFullText), "%s|%s|%s|%s|%s|%s|%s|%s",
//				szAddress, szCertificated, szNeedPassword, szGLS, szHostname, szPing, szMap, szPlayers);
			nLen = _snprintf_s(szFullText, sizeof(szFullText), "%s|%s|%s|%s|%s",
				szAddress, szHostname, szPing, szMap, szPlayers);
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
	case IDM_WEBSITE:
		ShellExecute(NULL, "open", "http://www.digitalpaint.org", NULL, NULL, SW_SHOW);
		return TRUE;
	case IDM_WEBSITE_GLS:
		ShellExecute(NULL, "open", "http://www.dplogin.com/index.php?action=main", NULL, NULL, SW_SHOW);
		return TRUE;
	case IDM_WEBSITE_PLAYER:
		ShellExecute(NULL, "open", "http://www.dplogin.com/index.php?action=displaymembers", NULL, NULL, SW_SHOW);
		return TRUE;
	case IDM_WEBSITE_FORUM:
		ShellExecute(NULL, "open", "http://dplogin.com/forums/index.php", NULL, NULL, SW_SHOW);
		return TRUE;
	case IDM_WEBSITE_DOCS:
		ShellExecute(NULL, "open", "http://digitalpaint.org/docs.html", NULL, NULL, SW_SHOW);
		return TRUE;
	case IDM_UPDATE:
		UpdateList();
		return TRUE;
	case IDM_REFRESH:
		OnRefresh();
		return TRUE;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		return TRUE;
	//Edit by Richard
	case IDM_SEARCHPLAYER:
		g_hSearchPlayerDlg = CreateDialog(g_hInst, (LPCTSTR)IDD_SEARCHPLAYER, hWnd, (DLGPROC)SearchPlayerDlg);
		return TRUE;
	case ID_TRAY_EXIT:
		DestroyWindow(hWnd);
		return TRUE;
	case ID_TRAY_RESTORE:
		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		return TRUE;
	case ID_RIGHTCLICK_COPYADDRESS:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_ADDRESS);
	case ID_RIGHTCLICK_COPYURL:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_URL);
	case ID_RIGHTCLICK_COPYFULLSERVERINFO:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_FULL);
	case ID_RIGHTCLICK_COPYNAMEIP:
		return CopyAddress(hWnd, codeNotify, COPYTYPE_NAMEIP);
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
		case SERVERLIST_CERTIFICATED_OFFSET:
			lplvcd->clrText = g_dwWhiteTextColor;
			lplvcd->clrTextBk = g_dwDefaultTextBackground;
			return CDRF_NEWFONT;
		case SERVERLIST_NEEDPASSWORD_OFFSET:
			lplvcd->clrText = g_dwWhiteTextColor;
			lplvcd->clrTextBk = g_dwDefaultTextBackground;
			return CDRF_NEWFONT;
		case SERVERLIST_GLS_OFFSET:
			lplvcd->clrText = g_dwWhiteTextColor;
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
	int iImageCertificated = g_iIconBlank;
	int iImageNeedPassword = g_iIconBlank;
	int iImageGLS = g_iIconBlank;

	
	if (tServerInfo.nCertificated)
	{
		iImageCertificated = g_iIconCertificated;
	}
	if (tServerInfo.nNeedPassword)
	{
		iImageNeedPassword = g_iIconNeedPassword;
	}
	if (tServerInfo.nGLS == 1)
	{
		iImageGLS = g_iIconGLS1;
	}
	else if (tServerInfo.nGLS == 2)
	{
		iImageGLS = g_iIconGLS2;
	}

	// Find a matching address to update
	nID = GetListIDFromAddress(sAddress);

	// Add a new server to the list if the address is not found.
	if (nID < 0)
	{
		LVITEM tLVItem;

		memset(&tLVItem, 0, sizeof(tLVItem));
		tLVItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE | LVIF_DI_SETITEM;
		tLVItem.state = 0;
		tLVItem.stateMask = 0;

		tLVItem.iSubItem = 0;
		tLVItem.iItem = nCount;
		tLVItem.lParam = nCount;
		tLVItem.iImage = iImageCertificated;
		tLVItem.pszText = "";
		nID = ListView_InsertItem(g_hServerList, &tLVItem);

		tLVItem.mask = LVIF_TEXT | LVIF_IMAGE;
		tLVItem.iSubItem = 1;
		tLVItem.iItem = nCount;
		tLVItem.iImage = iImageNeedPassword;
		tLVItem.pszText = "";
		ListView_SetItem(g_hServerList, &tLVItem);

		tLVItem.mask = LVIF_TEXT | LVIF_IMAGE;
		tLVItem.iSubItem = 2;
		tLVItem.iItem = nCount;
		tLVItem.iImage = iImageGLS;
		tLVItem.pszText = "";
		ListView_SetItem(g_hServerList, &tLVItem);
		TweakListviewWidths();
	}

	// set address first so we can reference it
	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_ADDRESS_OFFSET, sAddress) &&
		g_nServerlistSortColumn == SERVERLIST_ADDRESS_OFFSET)
	{
		SortServerList();
		nID = GetListIDFromAddress(sAddress);
	}

	if (UpdateListviewText(g_hServerList, nID, SERVERLIST_CERTIFICATED_OFFSET, _itoa(tServerInfo.nCertificated, szTemp, 10)) &&
		g_nServerlistSortColumn == SERVERLIST_CERTIFICATED_OFFSET)
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

//Edit by Richard: SearchThread which will refesh servers, then wait iDelay ms, then redo the search
struct SearchThreadArgs
{
	HWND hDlg;
	std::vector <std::pair<std::string, int> > * pvFound;
	int iDelay;
};
void SearchThread (SearchThreadArgs* args)
{
	std::string sContentBuffer;
	char szNameBuffer[256];
	char szOldEntry[256];
	int index;
	HWND hListBox = GetDlgItem(args->hDlg, IDC_SP_LIST);
	HWND hEdit = GetDlgItem(args->hDlg, IDC_SP_EDIT);
	
	//animate the button while waiting so the user sees that something is loading
	if (args->iDelay)
	{
		EnableWindow(GetDlgItem(args->hDlg, IDC_SP_UPDATE), FALSE);
		EnableWindow(GetDlgItem(args->hDlg, IDC_SP_EDIT), FALSE);

		GetDlgItemText(args->hDlg, IDC_SP_UPDATE, szOldEntry, sizeof(szOldEntry) / sizeof(szOldEntry[0]));
		for (index = 0; index < 10; index++)
		{
			sprintf_s(szNameBuffer, "%.*s", index, "................");
			SetDlgItemText(args->hDlg, IDC_SP_UPDATE, szNameBuffer);
			Sleep(args->iDelay / 10);
		}
		SetDlgItemText(args->hDlg, IDC_SP_UPDATE, szOldEntry);
	}
	
	index = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
	SendMessage(hListBox, LB_GETTEXT, index, (LPARAM) szOldEntry);

	GetWindowText(hEdit, szNameBuffer, sizeof(szNameBuffer) / sizeof (szNameBuffer[0]));
	SearchPlayer(szNameBuffer, args->pvFound);
	
	if (args->pvFound->size() == 0) //if no results, add the "no player found" text in the correct language
	{
		SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
		LoadString(g_hInst, IDS_SP_NOPLAYERFOUND, szNameBuffer, sizeof(szNameBuffer)/sizeof(szNameBuffer[0]));
		index = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM) szNameBuffer);
		SendMessage(hListBox, LB_SETITEMDATA, index, (-1));
		
		if (args->iDelay)
		{
			EnableWindow(GetDlgItem(args->hDlg, IDC_SP_UPDATE), TRUE);
			EnableWindow(GetDlgItem(args->hDlg, IDC_SP_EDIT), TRUE);
		}
		delete args;
		return;
	}
	
	SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
	for (size_t i = 0; i < args->pvFound->size(); i++) //add found results, also set itemdata to index in vPlayers
	{
		sContentBuffer.assign(g_mServers[args->pvFound->at(i).first].vPlayers[args->pvFound->at(i).second].sName);
		sContentBuffer.append (" on ");
		sContentBuffer.append (g_mServers[args->pvFound->at(i).first].sHostName);
		int index = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM) sContentBuffer.c_str());
		SendMessage(hListBox, LB_SETITEMDATA, index, i);
	}
	index = SendMessage(hListBox, LB_FINDSTRING, -1, (LPARAM) szOldEntry);
	SendMessage(hListBox, LB_SETCURSEL, index, 0);

	if (args->iDelay)
	{
		EnableWindow(GetDlgItem(args->hDlg, IDC_SP_UPDATE), TRUE);
		EnableWindow(GetDlgItem(args->hDlg, IDC_SP_EDIT), TRUE);
	}
	delete args;
}




//Edit by Richard: Message handlers for search players dialog
int OnSearchPlayerDlgUpdate (HWND hDlg, int iDelay, std::vector <std::pair<std::string, int> > * pvFound)
{
	RefreshList();
	SearchThreadArgs* args = new SearchThreadArgs;
	args->hDlg = hDlg;
	args->pvFound = pvFound;
	args->iDelay = iDelay;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SearchThread, (LPVOID)args, 0, NULL);
	return TRUE;
}

int OnSearchPlayerDlgEditChange(HWND hDlg, std::vector <std::pair<std::string, int> > * pvFound)
{
	SearchThreadArgs* args = new SearchThreadArgs;
	args->hDlg = hDlg;
	args->pvFound = pvFound;
	args->iDelay = 0;
	SearchThread(args);
	return TRUE;
}

int OnSearchPlayerDlgSelChange (HWND hDlg, std::vector <std::pair<std::string, int> > * pvFound)
{
	int iFoundIndex;
	int iListId;

	iFoundIndex = SendMessage(
		GetDlgItem(hDlg, IDC_SP_LIST),
		LB_GETITEMDATA,
		SendDlgItemMessage(hDlg, IDC_SP_LIST, LB_GETCURSEL, 0, 0),
		0);
	
	if (iFoundIndex == (-1))
		return TRUE;

	//select the server the player is playing on in the main windoww
	iListId = GetListIDFromAddress(pvFound->at(iFoundIndex).first.c_str());
	if (iListId >= 0)
	{
		ListView_SetItemState(g_hServerList, -1, 0, 0x000F);
		ListView_SetItemState(g_hServerList, iListId, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		ListView_EnsureVisible(g_hServerList, iListId, FALSE);
		UpdateInfoLists(iListId, false);
	}
	return TRUE;
}

int OnSearchPlayerDlgResize (HWND hDlg)
{
	RECT rcWin;
	DWORD dwBaseUnits;
	GetClientRect(hDlg, &rcWin);
	dwBaseUnits = GetDialogBaseUnits();
	int iMW = LOWORD(dwBaseUnits) / 4; //Multiplier width for base units to pixels
    int iMH = HIWORD(dwBaseUnits) / 8; //Multiplier height for base units to pixels
	
	MoveWindow (GetDlgItem(hDlg, IDC_SP_EDIT), 5*iMW, 16*iMH, rcWin.right - 10*iMW,  12*iMH, false);
	MoveWindow (GetDlgItem(hDlg, IDC_SP_LIST), 5*iMW, 50*iMH, rcWin.right - 10*iMW,  rcWin.bottom - 72*iMH, false);
	MoveWindow (GetDlgItem(hDlg, IDC_SP_UPDATE), rcWin.right - 84*iMW, rcWin.bottom - 17*iMH, 37*iMW, 11*iMH, false);
	MoveWindow (GetDlgItem(hDlg, IDOK),          rcWin.right - 42*iMW, rcWin.bottom - 17*iMH, 37*iMW, 11*iMH, false);

	RedrawWindow(hDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
	return TRUE;
}

int OnSearchPlayerDlgInit (HWND hDlg)
{
	OnSearchPlayerDlgResize(hDlg);
	//fill the listbox with all players currently playing, manually calling the function is not possible
	//because we dont have the pvFound in this scope.
	SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_SP_EDIT, EN_CHANGE), (LPARAM) GetDlgItem(hDlg, IDC_SP_EDIT));
	return TRUE;
}

int OnSearchPlayerDlgCommand (HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	static std::vector <std::pair<std::string, int> > vFound;

	if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
	{
		g_hSearchPlayerDlg = 0;
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
	}

	if (LOWORD(wParam) == IDC_SP_UPDATE)
		return OnSearchPlayerDlgUpdate (hDlg, 1000, &vFound);

	if ((LOWORD(wParam) == IDC_SP_EDIT) && (HIWORD(wParam) == EN_CHANGE))
		return OnSearchPlayerDlgEditChange(hDlg, &vFound);

	if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_SP_LIST)
		return OnSearchPlayerDlgSelChange(hDlg, &vFound);
	
	return FALSE;
}

static LRESULT CALLBACK SearchPlayerDlg (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		return OnSearchPlayerDlgResize(hDlg);

	case WM_INITDIALOG:
		return OnSearchPlayerDlgInit(hDlg);

	case WM_COMMAND:
		return OnSearchPlayerDlgCommand(hDlg, wParam, lParam);

	case WM_GETMINMAXINFO:
		{
			DWORD dwBaseUnits = GetDialogBaseUnits();
			((MINMAXINFO*) lParam)->ptMinTrackSize.x = MulDiv(100, LOWORD(dwBaseUnits), 4);
			((MINMAXINFO*) lParam)->ptMinTrackSize.y = MulDiv(120, HIWORD(dwBaseUnits), 8);
			return DefWindowProc (hDlg, message, wParam, lParam);
		}
	}
    return FALSE;
}