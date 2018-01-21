// GateCore.h: interface for the CGateCore class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GATECORE_H__2CB6AC22_DFA6_11D2_B145_00001C7030A6__INCLUDED_)
#define AFX_GATECORE_H__2CB6AC22_DFA6_11D2_B145_00001C7030A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif  // _MSC_VER >= 1000

#include <direct.h>
#include <mmsystem.h>
#include <process.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <windows.h>
#include "Client.h"
#include "MessageIndex.h"
#include "NetMessages.h"
#include "PartyManager.h"

#define DEF_MAXCLIENTS 500
#define DEF_MAXGAMESERVERS 20
#define DEF_NORESPONSELIMIT \
  30000  // 30초간 응답이 없으면 서버가 비정상적으로 종료된 것이다.
#define DEF_MAXMONITORS 10

class CGateCore {
 public:
  void PartyOperation(int iClientH, char* pData);
  void SendMsgToMonitor(char* pData, DWORD dwMsgSize);
  void CheckAutoShutdownProcess();
  void SendMsgToAllGameServers(int iClientH, char* pData, DWORD dwMsgSize,
                               BOOL bIsOwnSend = TRUE);
  void ItemLog(char* pData);
  void _SendServerShutDownMsg(int iClientH, char cCode);
  void SendServerShutDownMsg(char cCode, BOOL bISShutdown = FALSE);
  void CheckGlobalCommand();
  void OnKeyDown(WPARAM wParam, LPARAM lParam);
  void _SendTotalGameServerClients(int iClientH, int iTotalClients);
  void SendTotalClientsToGameServer();
  void SendMsgToMonitor(DWORD dwMsg, WORD wMsgType, char* pGameServerName,
                        short sTotalClient = 0);
  void CheckGameServerActivity();
  void OnTimer(int iID);
  void OnKeyUp(WPARAM wParam, LPARAM lParam);
  void ResponseRegisterGameServer(int iClientH, BOOL bRet);
  void RegisterGameServerHandler(int iClientH, char* pData);
  void OnClientRead(int iClientH);
  BOOL bAccept(class XSocket* pXSock);
  void OnClientSocketEvent(UINT message, WPARAM wParam, LPARAM lParam);
  BOOL bReadProgramConfigFile(char* cFn);
  BOOL bInit();

  CGateCore(HWND hWnd);
  virtual ~CGateCore();

  HWND m_hWnd;
  char m_cGateServerAddr[16];
  int m_iGateServerPort;

  BOOL m_bF1pressed, m_bF12pressed;

  BOOL m_bIsAutoShutdownProcess;
  int m_iAutoShutdownCount;
  DWORD m_dwAutoShutdownTime;

  // 2002-11-27일 정진 수정
  char m_cAddress[10][16];

  // v2.17 고광현 수정
  int m_iBuildDate;
  int iTotalWorldServerClients;
  char m_cMonitorStatus[DEF_MAXMONITORS];

  class CClient* m_pClientList[DEF_MAXCLIENTS];
  class PartyManager* m_pPartyManager;
};

#endif  // !defined(AFX_GATECORE_H__2CB6AC22_DFA6_11D2_B145_00001C7030A6__INCLUDED_)
