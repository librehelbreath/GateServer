// -------------------------------------------------------------- 
//                      Gate Server  						  
//
//                      1999.3 by Soph
//
// --------------------------------------------------------------


// --------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <mmsystem.h>
#include <time.h>
#include "winmain.h"
#include "GateCore.h"
#include "UserMessages.h"
#include "XSocket.h"


// --------------------------------------------------------------

#define WM_USER_TIMERSIGNAL		WM_USER + 500

char			szAppClass[32];
HWND			G_hWnd = NULL;
char			G_cMsgList[120*50];
BOOL            G_cMsgUpdated =	FALSE;
char            G_cTxt[120];

class XSocket   * G_pListenSock = NULL;
class CGateCore * G_pGateCore  = NULL;

MMRESULT          G_mmTimer0 = NULL;

int               G_iQuitProgramCount = 0;
BOOL			 G_bThreadFlag = TRUE;

// --------------------------------------------------------------

unsigned __stdcall  ThreadProc(void *ch)
{
 char cTemp[256] ;
 int i, iCurrentUserTime;
	iCurrentUserTime = 0 ;
	i = 0;
	while (G_bThreadFlag == TRUE) {
		Sleep(1000);

		iCurrentUserTime++ ; 
		if (iCurrentUserTime > 60*1) {
			// 1분마다 한번씩 현재 사용자 기록함  
			ZeroMemory(cTemp,sizeof(cTemp)) ;

			iCurrentUserTime = 0 ;

			for (i = 1; i < 500; i++) 
			{
				if ((G_pGateCore->m_pClientList[i] != NULL) && (G_pGateCore->m_pClientList[i]->m_bIsGameServer == TRUE)) {
					wsprintf(cTemp,"(%s) Total user(%d)",G_pGateCore->m_pClientList[i]->m_cName,G_pGateCore->m_pClientList[i]->m_iTotalClients) ;
					PutGameServerList(cTemp);
				}
			}
			
		}
	}

	_endthread();
	return 0;
}


LRESULT CALLBACK WndProc( HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam )
{ 
	switch (message) {

	case WM_USER_TIMERSIGNAL:
		if (wParam == G_mmTimer0) G_pGateCore->OnTimer(0);
		break;

	case WM_USER_ACCEPT:
		OnAccept();
		break;

	case WM_KEYDOWN:
		G_pGateCore->OnKeyDown(wParam, lParam);
		break;

	case WM_KEYUP:
		G_pGateCore->OnKeyUp(wParam, lParam);
		break;

	case WM_PAINT:
		OnPaint();
		break;

	case WM_DESTROY:
		OnDestroy();
		break;

	case WM_CLOSE:
		if (MessageBox(NULL, "Quit gate server?", "GATE-SERVER", MB_ICONEXCLAMATION | MB_YESNO) == IDYES) {
			return (DefWindowProc(hWnd, message, wParam, lParam));
		}
		break;

	default: 
		if ((message >= WM_ONCLIENTSOCKETEVENT) && (message < WM_ONCLIENTSOCKETEVENT + DEF_MAXCLIENTS)) 
			G_pGateCore->OnClientSocketEvent(message, wParam, lParam);

		return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	
	return NULL;
}



int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
               LPSTR lpCmdLine, int nCmdShow )
{
	sprintf( szAppClass, "Gate-Server%d", hInstance);
	if (!InitApplication( hInstance))		return (FALSE);
    if (!InitInstance(hInstance, nCmdShow)) return (FALSE);
	
	Initialize();
	EventLoop();

    return 0;
}
			   


BOOL InitApplication( HINSTANCE hInstance)
{     
 WNDCLASS  wc;

	wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS);
	wc.lpfnWndProc   = (WNDPROC)WndProc;             
	wc.cbClsExtra    = 0;                            
	wc.cbWndExtra    = sizeof (int);             
	wc.hInstance     = hInstance;                    
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);  
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	 
	wc.lpszMenuName  = NULL;                    		 
	wc.lpszClassName = szAppClass;                   
        
	return (RegisterClass(&wc));
}



BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
 char cTitle[100];
	
	// 서버 부팅시간 기록 
	wsprintf(cTitle, "Helbreath GateServer 2002-6-20", 0);
	
	G_hWnd = CreateWindowEx(0,  // WS_EX_TOPMOST,
        szAppClass,
        cTitle,
        WS_VISIBLE | // so we don't have to call ShowWindow
        WS_POPUP |   // non-app window
        WS_CAPTION | // so our menu doesn't look ultra-goofy
        WS_SYSMENU |  // so we get an icon in the tray
		WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        620, //GetSystemMetrics(SM_CXSCREEN),
        700, //GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        hInstance,
        NULL );

    if (!G_hWnd) return (FALSE);
    
	ShowWindow(G_hWnd, nCmdShow);    
	UpdateWindow(G_hWnd);            

	return (TRUE);                 
}



int EventLoop()
{
 static unsigned short _usCnt = 0; 
 register MSG msg;

	while( 1 ) {
		if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
			if( !GetMessage( &msg, NULL, 0, 0 ) ) {
				return msg.wParam;
			}
            TranslateMessage(&msg);
            DispatchMessage(&msg);
			UpdateScreen();
		}
		else WaitMessage();
	}
}



void Initialize()
{

	if (_InitWinsock() == FALSE) {
		MessageBox(G_hWnd, "Socket 1.1 not found! Cannot execute program.","ERROR", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return;
	}

	G_pGateCore = new class CGateCore(G_hWnd);

	if (G_pGateCore == NULL) {
		MessageBox(G_hWnd, "Init fail!","ERROR", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return;
	}

	if (G_pGateCore->bInit() == FALSE) {
		MessageBox(G_hWnd, "Init fail!","ERROR", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return;	
	}

	G_pListenSock = new class XSocket(G_hWnd, 300);
	G_pListenSock->bListen(G_pGateCore->m_cGateServerAddr, G_pGateCore->m_iGateServerPort, WM_USER_ACCEPT);

	// 서버 검사용 타이머 시작 
	G_mmTimer0 = _StartTimer(3000);

	PutLogList("(!) Gate Server Listening...");
}



void OnDestroy()
{
	if (G_pGateCore != NULL)    delete G_pGateCore;
	if (G_pListenSock != NULL)  delete G_pListenSock;

	_TermWinsock();
  	if (G_mmTimer0 != NULL) _StopTimer(G_mmTimer0);

	PostQuitMessage(0);
}



void PutLogList(char * cMsg)
{
 char cTemp[120*50];
	
	G_cMsgUpdated = TRUE;
	ZeroMemory(cTemp, sizeof(cTemp));
	memcpy((cTemp + 120), G_cMsgList, 120*49);
	memcpy(cTemp, cMsg, strlen(cMsg));
	memcpy(G_cMsgList, cTemp, 120*50);

}

void UpdateScreen()
{
	if (G_cMsgUpdated == TRUE) {
		InvalidateRect(G_hWnd, NULL, TRUE);
		G_cMsgUpdated = FALSE;
	}
}


void OnPaint()
{
 HDC hdc;
 PAINTSTRUCT ps;
 register short i;
 char * cMsg;

	hdc = BeginPaint(G_hWnd, &ps);

	SetBkMode(hdc,TRANSPARENT);

	// 이벤트 리스트를 출력한다.
	for (i = 0; i < 35; i++) {
		cMsg = (char *)(G_cMsgList + i*120);
		TextOut(hdc, 5, 5 + 600 - i*16, cMsg, strlen(cMsg));
	}
	
	TextOut(hdc, 10, 10, "[F1 + F4]  Shut down all game servers", strlen("F1 + F4 : Shut down all game servers"));
	TextOut(hdc, 10, 25, "[F2] Shows not activated game server list", strlen("[F2] Shows not activated game server list"));
	TextOut(hdc, 10, 40, "[F3] Shows activated game server list", strlen("[F3] Shows activated game server list"));

	TextOut(hdc, 0, 620, "________________________________________________________________________________", strlen("________________________________________________________________________________"));
	if (G_pGateCore != NULL) {
		for (i = 0; i < DEF_MAXMONITORS; i++) {
			wsprintf(G_cTxt, "%d", i);
			TextOut(hdc, 20 +i*30 +1, 640, G_cTxt, strlen(G_cTxt));
		
			switch (G_pGateCore->m_cMonitorStatus[i]) {
			default: TextOut(hdc, 20 +i*30, 655, "?", 1); break;break;
			case 1: TextOut(hdc, 20 +i*30, 655, "X", 1); break;
			case 2: TextOut(hdc, 20 +i*30, 655, "O", 1); break;
			}
		}
	}

	EndPaint(G_hWnd, &ps);
}

void OnAccept()
{
	G_pGateCore->bAccept(G_pListenSock);
}


void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2)
{
	PostMessage(G_hWnd, WM_USER_TIMERSIGNAL, wID, NULL);
}


MMRESULT _StartTimer(DWORD dwTime)
{
 TIMECAPS caps;
 MMRESULT timerid;

	timeGetDevCaps(&caps, sizeof(caps));
	timeBeginPeriod(caps.wPeriodMin);
	timerid = timeSetEvent(dwTime,0,_TimerFunc,0, (UINT)TIME_PERIODIC);

	return timerid;
}



void _StopTimer(MMRESULT timerid)
{
 TIMECAPS caps;

	if (timerid != 0) {
		timeKillEvent(timerid);
		timerid = 0;
		timeGetDevCaps(&caps, sizeof(caps));
		timeEndPeriod(caps.wPeriodMin);
	}
}


void PutItemLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("ItemEvents.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}


void PutGameServerList(char * cStr)
{
 FILE * pFile = NULL ;
 char cBuffer[512];

 SYSTEMTIME SysTime;
	
    _mkdir("GameServerLog");
	GetLocalTime(&SysTime);

	ZeroMemory(cBuffer, sizeof(cBuffer)) ;
	wsprintf(cBuffer,"GameServerLog\\GameServerLog%04d%02d%02d.log",SysTime.wYear,SysTime.wMonth,SysTime.wDay) ;

	pFile = fopen(cBuffer, "at");

	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
    if (pFile != NULL)	fclose(pFile);
}


void PutLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("HackAttacker.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}