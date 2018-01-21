// GateCore.cpp: implementation of the CGateCore class.
//
//////////////////////////////////////////////////////////////////////

#include "GateCore.h"

extern char G_cTxt[120];
extern void PutLogList(char *cMsg);
extern void PutItemLogFileList(char *cStr);
extern void PutLogFileList(char *cStr);

extern unsigned __stdcall ThreadProc(void *ch);
extern BOOL G_bThreadFlag;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGateCore::CGateCore(HWND hWnd) {
  int i;

  m_hWnd = hWnd;
  m_bF1pressed = m_bF12pressed = FALSE;

  m_iAutoShutdownCount = 10;

  // v2.17 2002-6-3 고광현 수정
  m_iBuildDate = 0;

  for (i = 0; i < DEF_MAXCLIENTS; i++) m_pClientList[i] = NULL;

  for (i = 0; i < DEF_MAXMONITORS; i++) m_cMonitorStatus[i] = NULL;

  memset(m_cAddress, 0, 10 * 16);

  iTotalWorldServerClients = 0;

  m_pPartyManager = new class PartyManager(this);
}

CGateCore::~CGateCore() {
  int i;

  // 쓰레드를 죽인다.
  G_bThreadFlag = FALSE;

  for (i = 0; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] != NULL) {
      delete m_pClientList[i];
      m_pClientList[i] = NULL;
    }

  delete m_pPartyManager;
}

BOOL CGateCore::bInit() {
  if (bReadProgramConfigFile("GateServer.cfg") == FALSE) return FALSE;

  unsigned uAddr;
  _beginthreadex(0, 0, ThreadProc, 0, 0, &uAddr);

  return TRUE;
}

BOOL CGateCore::bReadProgramConfigFile(char *cFn) {
  FILE *pFile;
  HANDLE hFile;
  DWORD dwFileSize;
  char *cp, *token, cReadMode, cTxt[120];
  char seps[] = "= ,\t\n";
  int iIPindex = 0;

  cReadMode = 0;

  hFile = CreateFile(cFn, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
  dwFileSize = GetFileSize(hFile, NULL);
  if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

  ZeroMemory(m_cGateServerAddr, sizeof(m_cGateServerAddr));

  pFile = fopen(cFn, "rt");
  if (pFile == NULL) {
    // 로그서버의 초기화 파일을 읽을 수 없다.
    PutLogList("(!) Cannot open configuration file.");
    return FALSE;
  } else {
    PutLogList("(!) Reading configuration file...");
    cp = new char[dwFileSize + 1];
    ZeroMemory(cp, dwFileSize + 1);
    fread(cp, dwFileSize, 1, pFile);

    token = strtok(cp, seps);
    while (token != NULL) {
      if (cReadMode != 0) {
        switch (cReadMode) {
          case 1:

            m_iGateServerPort = atoi(token);
            wsprintf(cTxt, "(*) Gate server port : %d", m_iGateServerPort);
            PutLogList(cTxt);
            cReadMode = 0;
            break;

          // v2.17 2002-6-3 고광현 수정
          case 2:
            m_iBuildDate = atoi(token);
            wsprintf(cTxt, "(*) Build Date : %d", m_iBuildDate);
            PutLogList(cTxt);
            cReadMode = 0;
            break;

          case 3:
            strcpy(m_cAddress[iIPindex], token);
            iIPindex++;
            cReadMode = 0;
            break;
          case 4:
            ZeroMemory(m_cGateServerAddr, sizeof(m_cGateServerAddr));

            strcpy(m_cGateServerAddr, token);
            wsprintf(cTxt, "(*) Gate server address : %s", m_cGateServerAddr);
            PutLogList(cTxt);
            cReadMode = 0;
            break;
        }

      } else {
        if (memcmp(token, "gate-server-port", 15) == 0) cReadMode = 1;
        // v2.17 2002-6-3 고광현 수정
        if (memcmp(token, "build-date", 10) == 0) cReadMode = 2;
        if (memcmp(token, "game-server-list", 16) == 0)
          cReadMode = 3;  // 2002-11-27일 정진 수정
        if (memcmp(token, "gate-server-address", 19) == 0)
          cReadMode =
              4;  // 2003-1-17일 문성훈 추가 IP대역 두개 쓰는경우를 위해서
      }
      token = strtok(NULL, seps);
    }

    delete cp;
  }
  if (pFile != NULL) fclose(pFile);

  // 2003-1-17일 문성훈 추가 게이트 서버 아이피를 않넣어 주는 경우
  if (m_cGateServerAddr == NULL) {
    char ServerAddr[50];
    ::gethostname(ServerAddr, 50);
    struct hostent *pHostEnt;
    pHostEnt = ::gethostbyname(ServerAddr);
    if (pHostEnt != NULL) {
      wsprintf(ServerAddr, "%d.%d.%d.%d",
               (pHostEnt->h_addr_list[0][0] & 0x00ff),
               (pHostEnt->h_addr_list[0][1] & 0x00ff),
               (pHostEnt->h_addr_list[0][2] & 0x00ff),
               (pHostEnt->h_addr_list[0][3] & 0x00ff));
    }
    strcpy(m_cGateServerAddr, ServerAddr);

    wsprintf(cTxt, "(*) Gate server address : %s", m_cGateServerAddr);
    PutLogList(cTxt);
  }

  return TRUE;
}

void CGateCore::OnClientSocketEvent(UINT message, WPARAM wParam,
                                    LPARAM lParam) {
  int iClientH, iTemp, iRet;

  iTemp = WM_ONCLIENTSOCKETEVENT;
  iClientH = message - iTemp;

  if (m_pClientList[iClientH] == NULL) return;

  iRet = m_pClientList[iClientH]->m_pXSock->iOnSocketEvent(wParam, lParam);
  switch (iRet) {
    case DEF_XSOCKEVENT_READCOMPLETE:
      // 메시지가 수신되었다.
      OnClientRead(iClientH);
      break;

    case DEF_XSOCKEVENT_BLOCK:
      PutLogList("Socket BLOCKED!");
      break;

    case DEF_XSOCKEVENT_CONFIRMCODENOTMATCH:
      wsprintf(G_cTxt, "<%d> Confirmcode notmatch!", iClientH);
      PutLogList(G_cTxt);
      delete m_pClientList[iClientH];
      m_pClientList[iClientH] = NULL;
      break;
    case DEF_XSOCKEVENT_CRITICALERROR:
      // 치명적인 에러이다.
    case DEF_XSOCKEVENT_QUENEFULL:
      // 큐가 가득찼다.
    case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
      // 수신해야 할 메시지 크기가 버퍼보다 크다.	종료해야만 한다.
    case DEF_XSOCKEVENT_SOCKETERROR:
      // 소켓에 에러가 났다.
    case DEF_XSOCKEVENT_SOCKETCLOSED:
      // 소켓이 닫혔다.
      if (m_pClientList[iClientH]->m_bIsGameServer == TRUE) {
        wsprintf(G_cTxt,
                 "*** CRITICAL ERROR! Game Server(%s) connection lost!!! ***",
                 m_pClientList[iClientH]->m_cName);
        // 서버 다운을 알린다.
        SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM,
                         m_pClientList[iClientH]->m_cName);
        m_pPartyManager->GameServerDown(iClientH);
      } else
        wsprintf(G_cTxt, "WARNING! Monitor(%d) connection lost!", iClientH);
      PutLogList(G_cTxt);
      delete m_pClientList[iClientH];
      m_pClientList[iClientH] = NULL;
      break;
  }
}

BOOL CGateCore::bAccept(class XSocket *pXSock) {
  register int i;
  class XSocket *pTmpSock;
  BOOL bTrue = FALSE;

  // 비어있는 배열을 찾는다.
  for (i = 1; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] == NULL) {
      m_pClientList[i] = new class CClient(m_hWnd);

      pXSock->bAccept(m_pClientList[i]->m_pXSock, WM_ONCLIENTSOCKETEVENT + i);
      m_pClientList[i]->m_pXSock->bInitBufferSize(DEF_MSGBUFFERSIZE);

      m_pClientList[i]->m_pXSock->iGetPeerAddress(
          m_pClientList[i]->m_cAddress);  // 2002-11-27일 정진 수정

      for (int j = 0; j < 10; j++)  // 2002-11-27일 정진 수정 허가된 아이피 체크
      {
        if (memcmp(m_cAddress[j], m_pClientList[i]->m_cAddress, 16) == 0) {
          bTrue = TRUE;
          break;
        }
      }

      if (bTrue != TRUE) {
        wsprintf(G_cTxt, "<%d> <%s> 씨바 해커닷.", i,
                 m_pClientList[i]->m_cAddress);
        PutLogFileList(G_cTxt);

        delete m_pClientList[i];
        m_pClientList[i] = NULL;

        return FALSE;
      }

      wsprintf(G_cTxt, "<%d> Accept! Add New Client.", i);
      PutLogList(G_cTxt);

      m_pClientList[i]->m_dwAliveTime = timeGetTime();
      return TRUE;
    }

  // 비어있는 배열이 없어 접속을 받을 수 없다. Accept한 후 그냥 끊는다.
  pTmpSock = new class XSocket(m_hWnd, 300);
  pXSock->bAccept(pTmpSock, 0);
  delete pTmpSock;
  // delete pXSock;

  return FALSE;
}

void CGateCore::OnClientRead(int iClientH) {
  char *cp, *pData, cTemp[22], cTxt[256];
  DWORD *dwpMsgID, dwMsgSize, dwTime, *dwp;
  WORD *wp;
  int i, iRet;
  BOOL bFindGameServer = FALSE;

  dwTime = timeGetTime();
  pData = m_pClientList[iClientH]->m_pXSock->pGetRcvDataPointer(&dwMsgSize);

  dwpMsgID = (DWORD *)(pData + DEF_INDEX4_MSGID);

  switch (*dwpMsgID) {
    case MSGID_PARTYOPERATION:
      PartyOperation(iClientH, pData);
      break;

    case MSGID_SERVERSTOCKMSG:
      // 받은 메시지를 모든 게임 서버들에게 재 전송
      SendMsgToAllGameServers(iClientH, pData, dwMsgSize, FALSE);
      break;
    case MSGID_REQUEST_GAMESERVER_SHUTDOWN:
      cp = (char *)(pData + DEF_INDEX2_MSGTYPE);
      wp = (WORD *)cp;
      if (*wp != 0x5A8E) {
        // 확인코드 불일치. 삭제.
        delete m_pClientList[iClientH];
        m_pClientList[iClientH] = NULL;
        return;
      }
      cp += 2;
      if (memcmp(cp, "1dkld$#@01", 10) != 0) return;

      cp += 10;

      strcpy(cTemp, cp);

      for (i = 1; i < DEF_MAXCLIENTS; i++)
        if ((m_pClientList[i] != NULL) &&
            (strcmp(m_pClientList[i]->m_cName, cTemp) == 0) &&
            (m_pClientList[i]->m_bIsGameServer == TRUE)) {
          ZeroMemory(cTxt, sizeof(cTxt));
          dwp = (DWORD *)(cTxt + DEF_INDEX4_MSGID);
          *dwp = MSGID_GAMESERVERSHUTDOWNED;
          wp = (WORD *)(cTxt + DEF_INDEX2_MSGTYPE);
          *wp = DEF_MSGTYPE_CONFIRM;

          iRet = m_pClientList[i]->m_pXSock->iSendMsg(cTxt, 6);

          switch (iRet) {
            case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
            case DEF_XSOCKEVENT_QUENEFULL:
            case DEF_XSOCKEVENT_SOCKETERROR:
            case DEF_XSOCKEVENT_CRITICALERROR:
            case DEF_XSOCKEVENT_SOCKETCLOSED:
              // 메시지를 보낼때 에러가 발생했다면 제거한다.
              memcpy(cTemp, m_pClientList[i]->m_cName, 20);

              delete m_pClientList[i];
              m_pClientList[i] = NULL;

              wsprintf(
                  G_cTxt,
                  "*** CRITICAL ERROR! Game Server(%s) connection lost!!! ***",
                  cTemp);
              PutLogList(G_cTxt);
              // 서버 다운을 알린다.
              SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM,
                               cTemp);
          }
          bFindGameServer = TRUE;
        }

      if (bFindGameServer == FALSE) {
        wsprintf(cTxt, "(!) AUTO SHUTDOWN PROCESS (%s) SERVER CAN'T FIND !",
                 cTemp);
        PutLogList(cTxt);
      }

      break;

    case MSGID_REQUEST_ALLGAMESERVER_SHUTDOWN:
      cp = (char *)(pData + DEF_INDEX2_MSGTYPE);
      wp = (WORD *)cp;
      if (*wp != 0x5A8E) {
        // 확인코드 불일치. 삭제.
        delete m_pClientList[iClientH];
        m_pClientList[iClientH] = NULL;
        return;
      }
      cp += 2;

      if (memcmp(cp, "1dkld$#@01", 10) != 0) return;

      PutLogList(" ");
      PutLogList("(!) AUTO SHUTDOWN PROCESS STARTED!");

      for (i = 1; i < DEF_MAXCLIENTS; i++)
        if ((m_pClientList[i] != NULL) &&
            (m_pClientList[i]->m_bIsGameServer == TRUE))
          m_pClientList[i]->m_bIsShutDown = TRUE;

      break;

    case MSGID_REQUEST_ALL_SERVER_REBOOT:
    case MSGID_REQUEST_EXEC_1DOTBAT:
    case MSGID_REQUEST_EXEC_2DOTBAT:
      cp = (char *)(pData + DEF_INDEX2_MSGTYPE);
      wp = (WORD *)cp;
      if (*wp != 0x5A8E) {
        // 확인코드 불일치. 삭제.
        delete m_pClientList[iClientH];
        m_pClientList[iClientH] = NULL;
        return;
      }
      // 받은 메시지를 그대로 모든 모니터에게 전송.
      SendMsgToMonitor(pData, dwMsgSize);
      break;

    case MSGID_GATEWAY_CURRENTCONN:
      SendMsgToMonitor(pData, dwMsgSize);
      break;

    case MSGID_MONITORALIVE:
      cp = (char *)(pData + DEF_INDEX2_MSGTYPE);
      wp = (WORD *)cp;

      if (m_cMonitorStatus[*wp] != 1)
        m_cMonitorStatus[*wp] = 1;
      else
        m_cMonitorStatus[*wp] = 2;
      InvalidateRect(m_hWnd, NULL, TRUE);

      // 받은 메시지를 그대로 모든 모니터에게 전송.
      char cTemp[20];
      ZeroMemory(cTemp, sizeof(cTemp));
      wsprintf(cTemp, "Total User %d", iTotalWorldServerClients);
      SendMsgToMonitor(*dwpMsgID, *wp, cTemp);
      break;

    case MSGID_ITEMLOG:
      // 아이템 관련 로그
      ItemLog(pData);
      break;

    case MSGID_REQUEST_REGISTERGAMESERVER:
      // 게임서버로부터 등록 요청
      RegisterGameServerHandler(iClientH, pData);
      break;

    case MSGID_GAMESERVERALIVE:
      // 게임서버가 살아 있음을 알리는 메시지
      m_pClientList[iClientH]->m_dwAliveTime = dwTime;
      cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);
      wp = (WORD *)cp;
      m_pClientList[iClientH]->m_iTotalClients = (int)*wp;

      if (m_pClientList[iClientH]->m_sIsActivated >= 0)
        m_pClientList[iClientH]->m_sIsActivated = 1;

      SendMsgToMonitor(MSGID_GAMESERVERALIVE, DEF_MSGTYPE_CONFIRM,
                       m_pClientList[iClientH]->m_cName,
                       m_pClientList[iClientH]->m_iTotalClients);
      break;
  }
}

void CGateCore::RegisterGameServerHandler(int iClientH, char *pData) {
  char cTxt[100], cName[11], cAddress[16], *cp;
  int i, iPort, iTotalMaps;
  DWORD *dwp, dwBuildDate;
  WORD *wp;
  BOOL bIsSuccess = TRUE;
  DWORD dwProcess;

  // 게임 서버를 등록한다.

  ZeroMemory(cName, sizeof(cName));
  ZeroMemory(cAddress, sizeof(cAddress));

  cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);
  memcpy(cName, cp, 10);
  cp += 10;

  memcpy(cAddress, cp, 16);
  cp += 16;

  wp = (WORD *)cp;
  iPort = (int)*wp;
  cp += 2;

  iTotalMaps = (int)*cp;
  cp++;

  for (i = 0; i < iTotalMaps; i++) {
    cp += 11;
  }

  dwp = (DWORD *)cp;
  dwProcess = (DWORD)*dwp;
  // v2.17 고광현 수정
  cp += 4;

  dwp = (DWORD *)cp;
  dwBuildDate = *dwp;
  cp += 4;

  m_pClientList[iClientH]->m_dwProcessHandle = dwProcess;
  m_pClientList[iClientH]->m_bIsGameServer = TRUE;

  if (m_iBuildDate != (int)dwBuildDate) {
    // 빌드 날짜가 맞지 않는 서버 발견
    ResponseRegisterGameServer(iClientH, FALSE);
    wsprintf(cTxt,
             "(!) Game Server Registration - fail : Server(%s:%s:%d) build "
             "date mismatch.",
             cName, cAddress, (int)dwBuildDate);
    PutLogList(cTxt);
    m_pClientList[iClientH]->m_sIsActivated = -1;
    // 서버 이름을 등록한다.
    memcpy(m_pClientList[iClientH]->m_cName, cName, 10);
    return;
  }
  // 여기까지
  // 먼저 같은 이름을 갖고 있는 서버가 있는지 검색한다.
  for (i = 1; i < DEF_MAXCLIENTS; i++)
    if ((m_pClientList[i] != NULL) &&
        (memcmp(m_pClientList[i]->m_cName, cName, 10) == 0)) {
      // 같은 이름을 갖는 게임 서버가 이미 접속되어 있다.
      ResponseRegisterGameServer(iClientH, FALSE);
      wsprintf(
          cTxt,
          "(!) Game Server Registration - fail : Server(%s) already exists.",
          cName);
      PutLogList(cTxt);
      m_pClientList[iClientH]->m_sIsActivated = -2;
      // 서버 이름을 등록한다.
      memcpy(m_pClientList[iClientH]->m_cName, cName, 10);

      return;
    }

  m_pClientList[iClientH]->m_sIsActivated = 1;
  // 성공적으로 서버등록을 마쳤다.
  wsprintf(
      cTxt,
      "(!) Game Server Registration - Success( Name:%s | Hd:%lx | date %d)",
      cName, dwProcess, (int)dwBuildDate);
  PutLogList(cTxt);
  // 서버 이름을 등록한다.
  memcpy(m_pClientList[iClientH]->m_cName, cName, 10);

  ResponseRegisterGameServer(iClientH, TRUE);
  return;

  // 더이상 등록할 서버공간이 없다.
  ResponseRegisterGameServer(iClientH, FALSE);
  PutLogList(
      "(!) Game Server Registeration - Fail : No more Game Server can be "
      "connected.");
}

void CGateCore::ResponseRegisterGameServer(int iClientH, BOOL bRet) {
  char cData[20], cTempName[22];
  DWORD *dwp;
  WORD *wp;
  int iRet;

  ZeroMemory(cData, sizeof(cData));
  ZeroMemory(cTempName, sizeof(cTempName));

  dwp = (DWORD *)(cData + DEF_INDEX4_MSGID);
  *dwp = MSGID_RESPONSE_REGISTERGAMESERVER;
  wp = (WORD *)(cData + DEF_INDEX2_MSGTYPE);
  if (bRet == TRUE)
    *wp = DEF_MSGTYPE_CONFIRM;
  else
    *wp = DEF_MSGTYPE_REJECT;

  iRet = m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 6);

  switch (iRet) {
    case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
    case DEF_XSOCKEVENT_QUENEFULL:
    case DEF_XSOCKEVENT_SOCKETERROR:
    case DEF_XSOCKEVENT_CRITICALERROR:
    case DEF_XSOCKEVENT_SOCKETCLOSED:
      // 메시지를 보낼때 에러가 발생했다면 제거한다.
      memcpy(cTempName, m_pClientList[iClientH]->m_cName, 20);

      delete m_pClientList[iClientH];
      m_pClientList[iClientH] = NULL;

      wsprintf(G_cTxt,
               "*** CRITICAL ERROR! Game Server(%s) connection lost!!! ***",
               cTempName);
      PutLogList(G_cTxt);

      // 서버 다운을 알린다.
      SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM, cTempName);
      return;
  }
}

void CGateCore::OnKeyUp(WPARAM wParam, LPARAM lParam) {
  int i, iCnt;
  char cTxt[120];

  switch (wParam) {
    case VK_F1:
      m_bF1pressed = FALSE;
      break;

    // v2.17 2002-6-3 고광현수정
    case VK_F2:
      // 등록되었지만 Activated되지 않은 게임 서버의 리스트를 출력한다.
      PutLogList(" ");
      iCnt = 0;
      for (i = 1; i < DEF_MAXCLIENTS; i++)
        if ((m_pClientList[i] != NULL) &&
            (m_pClientList[i]->m_bIsGameServer == TRUE) &&
            (m_pClientList[i]->m_sIsActivated <= 0)) {
          if (m_pClientList[i]->m_sIsActivated == 0)
            wsprintf(cTxt, "(X) Server(%s) Registered but not activated.",
                     m_pClientList[i]->m_cName);
          else if (m_pClientList[i]->m_sIsActivated == -1)
            wsprintf(cTxt, "(X) Server(%s) build date mismatch !",
                     m_pClientList[i]->m_cName);
          else if (m_pClientList[i]->m_sIsActivated == -2)
            wsprintf(cTxt, "(X) Server(%s) have Same Game Name !",
                     m_pClientList[i]->m_cName);

          PutLogList(cTxt);
          iCnt++;
        }
      wsprintf(cTxt, "%d servers are not activated.", iCnt);
      PutLogList(cTxt);
      break;

    case VK_F3:
      // 작동중인 서버 리스트를 보인다.
      PutLogList(" ");
      iCnt = 0;
      for (i = 1; i < DEF_MAXCLIENTS; i++)
        if ((m_pClientList[i] != NULL) &&
            (m_pClientList[i]->m_bIsGameServer == TRUE) &&
            (m_pClientList[i]->m_sIsActivated > 0)) {
          wsprintf(cTxt, "(!) Server(%s) activated.",
                   m_pClientList[i]->m_cName);
          PutLogList(cTxt);
          iCnt++;
        }
      wsprintf(cTxt, "%d servers are activated.", iCnt);
      PutLogList(cTxt);
      break;
      // 여기까지

    case VK_F4:
      // Auto-Shut down process
      if (m_bF1pressed == TRUE) {
        m_iAutoShutdownCount = 10;
        m_dwAutoShutdownTime = timeGetTime();

        for (i = 1; i < DEF_MAXCLIENTS; i++)
          if ((m_pClientList[i] != NULL) &&
              (m_pClientList[i]->m_bIsGameServer == TRUE))
            m_pClientList[i]->m_bIsShutDown = TRUE;

        PutLogList(" ");
        PutLogList("(!) AUTO SHUTDOWN PROCESS STARTED!");
      }
      break;

      /*	case VK_F5:
                      // for Test
                      PutLogList("(!) Testing Monitor activity...");
                      SendMsgToMonitor(MSGID_GAMESERVERDOWN,
         DEF_MSGTYPE_CONFIRM, m_pClientList[i]->m_cName); break;
      */
    case VK_F6:
      if (m_bF1pressed == TRUE) {
        PutLogList("(!) Send Server shutdown Message-1");
        SendServerShutDownMsg(1, TRUE);
        break;
      }

    case VK_F7:
      if (m_bF1pressed == TRUE) {
        PutLogList("(!) Send Server shutdown Message-2");
        SendServerShutDownMsg(2, TRUE);
        break;
      }

    case VK_F12:
      m_bF12pressed = FALSE;
      break;
  }
}

void CGateCore::OnTimer(int iID) {
  switch (iID) {
    case 0:
      // 게임 서버가 죽었는지를 체크한다.
      CheckGameServerActivity();
      // 전체 이용자 수를 각 게임서버에 알려준다.
      SendTotalClientsToGameServer();
      // Global command를 보내야 한다면 보낸다.
      CheckGlobalCommand();
      // 자동 셧다운 중인지 판단
      CheckAutoShutdownProcess();
      // 파티 멤버 접속 확인 체크
      m_pPartyManager->CheckMemberActivity();
      break;
  }
}

void CGateCore::CheckGameServerActivity() {
  register int i;
  DWORD dwTime;
  char cTxt[120];

  dwTime = timeGetTime();

  for (i = 1; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] != NULL) {
      if ((m_pClientList[i]->m_bIsGameServer == TRUE) &&
          ((dwTime - m_pClientList[i]->m_dwAliveTime) > DEF_NORESPONSELIMIT)) {
        // 이 게임 서버는 현재 동작이 중단되었다. 소켓이 끊어지지도 않았으니
        // 비정상적 실행 중단 상태이다.
        wsprintf(cTxt, "*** CRITICAL ERROR! Game server(%s) does not response!",
                 m_pClientList[i]->m_cName);
        PutLogList(cTxt);
        if (m_pClientList[i]->m_sIsActivated > 0)
          m_pClientList[i]->m_sIsActivated = 0;
        // Monitor 클라이언트에게 알린다.
        SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM,
                         m_pClientList[i]->m_cName);
      }
    }
}

void CGateCore::SendMsgToMonitor(DWORD dwMsg, WORD wMsgType,
                                 char *pGameServerName, short sTotalClient) {
  int i, iRet;
  char *cp, cTxt[120], cData[200];
  DWORD *dwp;
  WORD *wp;
  short *sp;
  ZeroMemory(cData, sizeof(cData));

  dwp = (DWORD *)(cData + DEF_INDEX4_MSGID);
  *dwp = dwMsg;  // MSGID_GAMESERVERDOWN;
  wp = (WORD *)(cData + DEF_INDEX2_MSGTYPE);
  *wp = wMsgType;  // DEF_MSGTYPE_CONFIRM;

  cp = (char *)(cData + DEF_INDEX2_MSGTYPE + 2);
  memcpy(cp, pGameServerName, 20);
  cp += 20;

  sp = (short *)cp;
  *sp = sTotalClient;
  cp += 2;

  for (i = 1; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] != NULL) {
      if (m_pClientList[i]->m_bIsGameServer == FALSE) {
        // 모니터들에게 메시지를 전송한다.
        iRet = m_pClientList[i]->m_pXSock->iSendMsg(cData, 28);

        switch (iRet) {
          case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
          case DEF_XSOCKEVENT_QUENEFULL:
          case DEF_XSOCKEVENT_SOCKETERROR:
          case DEF_XSOCKEVENT_CRITICALERROR:
          case DEF_XSOCKEVENT_SOCKETCLOSED:
            // 메시지를 보낼때 에러가 발생했다면 제거한다.
            delete m_pClientList[i];
            m_pClientList[i] = NULL;

            wsprintf(cTxt,
                     "(*** CRITICAL ERROR! Monitor(%d) connection lost!!! ***",
                     i);
            PutLogList(cTxt);
            return;
        }
      }
    }
}

void CGateCore::SendMsgToMonitor(char *pData, DWORD dwMsgSize) {
  int i, iRet;
  char cTxt[120];

  for (i = 1; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] != NULL) {
      if (m_pClientList[i]->m_bIsGameServer == FALSE) {
        // 모니터들에게 메시지를 전송한다.
        iRet = m_pClientList[i]->m_pXSock->iSendMsg(pData, dwMsgSize);

        switch (iRet) {
          case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
          case DEF_XSOCKEVENT_QUENEFULL:
          case DEF_XSOCKEVENT_SOCKETERROR:
          case DEF_XSOCKEVENT_CRITICALERROR:
          case DEF_XSOCKEVENT_SOCKETCLOSED:
            // 메시지를 보낼때 에러가 발생했다면 제거한다.
            delete m_pClientList[i];
            m_pClientList[i] = NULL;

            wsprintf(cTxt,
                     "(*** CRITICAL ERROR! Monitor(%d) connection lost!!! ***",
                     i);
            PutLogList(cTxt);
            return;
        }
      }
    }
}

void CGateCore::SendTotalClientsToGameServer() {
  int i;

  // 총 사용자 수를 계산한다.
  iTotalWorldServerClients = 0;
  for (i = 0; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] != NULL) {
      if (m_pClientList[i]->m_bIsGameServer == TRUE)
        iTotalWorldServerClients += m_pClientList[i]->m_iTotalClients;
    }

  // 각 게임서버에게 알려준다.
  for (i = 0; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] != NULL) {
      if (m_pClientList[i]->m_bIsGameServer == TRUE)
        _SendTotalGameServerClients(i, iTotalWorldServerClients);
    }
}

void CGateCore::_SendTotalGameServerClients(int iClientH, int iTotalClients) {
  char cData[120], cTempName[22];
  DWORD *dwp;
  WORD *wp;
  char *cp;
  int iRet;

  ZeroMemory(cData, sizeof(cData));
  ZeroMemory(cTempName, sizeof(cTempName));

  dwp = (DWORD *)(cData + DEF_INDEX4_MSGID);
  *dwp = MSGID_TOTALGAMESERVERCLIENTS;
  wp = (WORD *)(cData + DEF_INDEX2_MSGTYPE);
  *wp = DEF_MSGTYPE_CONFIRM;

  cp = (char *)(cData + DEF_INDEX2_MSGTYPE + 2);
  wp = (WORD *)cp;
  *wp = iTotalClients;
  cp += 2;

  iRet = m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 8);

  switch (iRet) {
    case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
    case DEF_XSOCKEVENT_QUENEFULL:
    case DEF_XSOCKEVENT_SOCKETERROR:
    case DEF_XSOCKEVENT_CRITICALERROR:
    case DEF_XSOCKEVENT_SOCKETCLOSED:
      // 메시지를 보낼때 에러가 발생했다면 제거한다.
      memcpy(cTempName, m_pClientList[iClientH]->m_cName, 20);

      delete m_pClientList[iClientH];
      m_pClientList[iClientH] = NULL;

      wsprintf(G_cTxt,
               "*** CRITICAL ERROR! Game Server(%s) connection lost!!! ***",
               cTempName);
      PutLogList(G_cTxt);

      // 서버 다운을 알린다.
      SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM, cTempName);
      return;
  }
}

void CGateCore::OnKeyDown(WPARAM wParam, LPARAM lParam) {
  switch (wParam) {
    case VK_F1:
      m_bF1pressed = TRUE;
      break;
    case VK_F12:
      m_bF12pressed = TRUE;
      break;
  }
}

void CGateCore::CheckGlobalCommand() {
  register int i, iRet;
  char cData[256], cTempName[22];
  DWORD *dwp;
  WORD *wp;

  ZeroMemory(cTempName, sizeof(cTempName));

  if ((m_bF1pressed == TRUE) && (m_bF12pressed == TRUE)) {
    PutLogList("(!) SEND GLOBAL COMMAND: SHUTDOWN ALL GAME SERVER");

    for (i = 0; i < DEF_MAXCLIENTS; i++)
      if ((m_pClientList[i] != NULL) &&
          (m_pClientList[i]->m_bIsGameServer == TRUE)) {
        ZeroMemory(cData, sizeof(cData));
        dwp = (DWORD *)(cData + DEF_INDEX4_MSGID);
        *dwp = MSGID_GAMESERVERSHUTDOWNED;
        wp = (WORD *)(cData + DEF_INDEX2_MSGTYPE);
        *wp = DEF_MSGTYPE_CONFIRM;

        iRet = m_pClientList[i]->m_pXSock->iSendMsg(cData, 6);

        switch (iRet) {
          case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
          case DEF_XSOCKEVENT_QUENEFULL:
          case DEF_XSOCKEVENT_SOCKETERROR:
          case DEF_XSOCKEVENT_CRITICALERROR:
          case DEF_XSOCKEVENT_SOCKETCLOSED:
            // 메시지를 보낼때 에러가 발생했다면 제거한다.
            memcpy(cTempName, m_pClientList[i]->m_cName, 20);

            delete m_pClientList[i];
            m_pClientList[i] = NULL;

            wsprintf(
                G_cTxt,
                "*** CRITICAL ERROR! Game Server(%s) connection lost!!! ***",
                cTempName);
            PutLogList(G_cTxt);

            // 서버 다운을 알린다.
            SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM,
                             cTempName);
            return;
        }
      }
  }
}

void CGateCore::SendServerShutDownMsg(char cCode, BOOL bISShutdown) {
  int i;
  char cTxt[256];

  // 각 게임서버에게 알려준다.
  for (i = 0; i < DEF_MAXCLIENTS; i++)
    if (m_pClientList[i] != NULL) {
      if ((m_pClientList[i]->m_bIsGameServer == TRUE) &&
          ((m_pClientList[i]->m_bIsShutDown == TRUE) ||
           (bISShutdown == TRUE))) {
        _SendServerShutDownMsg(i, cCode);
        wsprintf(cTxt, "(!) SEND SHUTDOWN MESSAGE TO (%s) SERVER !",
                 m_pClientList[i]->m_cName);
        PutLogList(cTxt);
      }
    }
}

void CGateCore::_SendServerShutDownMsg(int iClientH, char cCode) {
  char cData[120], cTempName[22];
  DWORD *dwp;
  WORD *wp;
  char *cp;
  int iRet;

  ZeroMemory(cData, sizeof(cData));
  ZeroMemory(cTempName, sizeof(cTempName));

  dwp = (DWORD *)(cData + DEF_INDEX4_MSGID);
  *dwp = MSGID_SENDSERVERSHUTDOWNMSG;
  wp = (WORD *)(cData + DEF_INDEX2_MSGTYPE);
  *wp = DEF_MSGTYPE_CONFIRM;

  cp = (char *)(cData + DEF_INDEX2_MSGTYPE + 2);
  wp = (WORD *)cp;
  *wp = cCode;
  cp += 2;

  iRet = m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 8);

  switch (iRet) {
    case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
    case DEF_XSOCKEVENT_QUENEFULL:
    case DEF_XSOCKEVENT_SOCKETERROR:
    case DEF_XSOCKEVENT_CRITICALERROR:
    case DEF_XSOCKEVENT_SOCKETCLOSED:
      // 메시지를 보낼때 에러가 발생했다면 제거한다.
      memcpy(cTempName, m_pClientList[iClientH]->m_cName, 20);

      delete m_pClientList[iClientH];
      m_pClientList[iClientH] = NULL;

      wsprintf(G_cTxt,
               "*** CRITICAL ERROR! Game Server(%s) connection lost!!! ***",
               cTempName);
      PutLogList(G_cTxt);

      // 서버 다운을 알린다.
      SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM, cTempName);
      return;
  }
}

void CGateCore::ItemLog(char *pData) {
  char *cp, cGiveName[11], cRecvName[11], cItemName[21], cStr[256];
  short *sp, sItemType, sItemV1, sItemV2, sItemV3;

  ZeroMemory(cGiveName, sizeof(cGiveName));
  ZeroMemory(cRecvName, sizeof(cRecvName));
  ZeroMemory(cItemName, sizeof(cItemName));
  ZeroMemory(cStr, sizeof(cStr));

  cp = (char *)(pData + DEF_INDEX2_MSGTYPE + 2);
  memcpy(cGiveName, cp, 10);
  cp += 10;

  memcpy(cRecvName, cp, 10);
  cp += 10;

  memcpy(cItemName, cp, 20);
  cp += 20;

  sItemType = (short)*cp;
  cp++;

  sp = (short *)cp;
  sItemV1 = *sp;
  cp += 2;

  sp = (short *)cp;
  sItemV2 = *sp;
  cp += 2;

  sp = (short *)cp;
  sItemV3 = *sp;
  cp += 2;

  wsprintf(cStr, "Item(%s %d %d %d %d) From(%s)-To(%s)", cItemName, sItemType,
           sItemV1, sItemV2, sItemV3, cGiveName, cRecvName);
  PutItemLogFileList(cStr);

  PutLogList(cStr);
}

void CGateCore::SendMsgToAllGameServers(int iClientH, char *pData,
                                        DWORD dwMsgSize, BOOL bIsOwnSend) {
  int i;

  if (bIsOwnSend == TRUE) {
    for (i = 0; i < DEF_MAXCLIENTS; i++)
      if ((m_pClientList[i] != NULL) &&
          (m_pClientList[i]->m_bIsGameServer == TRUE)) {
        m_pClientList[i]->m_pXSock->iSendMsg(pData, dwMsgSize);
      }
  } else {
    for (i = 0; i < DEF_MAXCLIENTS; i++)
      if ((i != iClientH) && (m_pClientList[i] != NULL) &&
          (m_pClientList[i]->m_bIsGameServer == TRUE)) {
        m_pClientList[i]->m_pXSock->iSendMsg(pData, dwMsgSize);
      }
  }
}

void CGateCore::CheckAutoShutdownProcess() {
  int i, iRet;
  DWORD dwTime = timeGetTime();
  char cData[128], cTempName[22];
  WORD *wp;
  DWORD *dwp;
  ZeroMemory(cTempName, sizeof(cTempName));

  m_bIsAutoShutdownProcess = FALSE;

  for (i = 1; i < DEF_MAXCLIENTS; i++)
    if ((m_pClientList[i] != NULL) &&
        (m_pClientList[i]->m_bIsGameServer == TRUE) &&
        (m_pClientList[i]->m_bIsShutDown == TRUE))
      m_bIsAutoShutdownProcess = TRUE;

  if (m_bIsAutoShutdownProcess == FALSE) return;

  if ((dwTime - m_dwAutoShutdownTime) > 20000) {
    m_dwAutoShutdownTime = dwTime;
  } else
    return;

  m_iAutoShutdownCount--;
  if (m_iAutoShutdownCount <= 0) m_iAutoShutdownCount = 0;
  switch (m_iAutoShutdownCount) {
    default:
      PutLogList(" ");
      SendServerShutDownMsg(1);
      break;

    case 2:

      PutLogList("LAST SHUTDOWN MESSAGE");
      SendServerShutDownMsg(2);
      break;

    case 1:

      for (i = 0; i < DEF_MAXCLIENTS; i++)
        if ((m_pClientList[i] != NULL) &&
            (m_pClientList[i]->m_bIsGameServer == TRUE) &&
            (m_pClientList[i]->m_bIsShutDown == TRUE)) {
          wsprintf(G_cTxt, "(!) SEND GLOBAL COMMAND: SHUTDOWN (%s) GAME SERVER",
                   m_pClientList[i]->m_cName);
          PutLogList(G_cTxt);

          ZeroMemory(cData, sizeof(cData));
          dwp = (DWORD *)(cData + DEF_INDEX4_MSGID);
          *dwp = MSGID_GAMESERVERSHUTDOWNED;
          wp = (WORD *)(cData + DEF_INDEX2_MSGTYPE);
          *wp = DEF_MSGTYPE_CONFIRM;

          iRet = m_pClientList[i]->m_pXSock->iSendMsg(cData, 6);

          switch (iRet) {
            case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
            case DEF_XSOCKEVENT_QUENEFULL:
            case DEF_XSOCKEVENT_SOCKETERROR:
            case DEF_XSOCKEVENT_CRITICALERROR:
            case DEF_XSOCKEVENT_SOCKETCLOSED:
              // 메시지를 보낼때 에러가 발생했다면 제거한다.
              memcpy(cTempName, m_pClientList[i]->m_cName, 20);

              delete m_pClientList[i];
              m_pClientList[i] = NULL;

              wsprintf(
                  G_cTxt,
                  "*** CRITICAL ERROR! Game Server(%s) connection lost!!! ***",
                  cTempName);
              PutLogList(G_cTxt);
              // 서버 다운을 알린다.
              SendMsgToMonitor(MSGID_GAMESERVERDOWN, DEF_MSGTYPE_CONFIRM,
                               cTempName);
              return;
          }
        }

      break;
  }
}

void CGateCore::PartyOperation(int iClientH, char *pData) {
  char *cp, cName[12], cData[120];
  DWORD *dwp;
  WORD *wp, wRequestType;
  int iRet, iGSCH, iPartyID;
  BOOL bRet;

  cp = (char *)pData;
  cp += 4;
  wp = (WORD *)cp;
  wRequestType = *wp;
  cp += 2;

  wp = (WORD *)cp;
  iGSCH = (WORD)*wp;
  cp += 2;

  ZeroMemory(cName, sizeof(cName));
  memcpy(cName, cp, 10);
  cp += 10;

  wp = (WORD *)cp;
  iPartyID = (WORD)*wp;
  cp += 2;

  // testcode
  // wsprintf(G_cTxt, "Party Operation Type: %d Name: %s PartyID:%d",
  // wRequestType, cName, iPartyID);  PutLogList(G_cTxt);

  ZeroMemory(cData, sizeof(cData));
  cp = (char *)cData;
  dwp = (DWORD *)cp;
  *dwp = MSGID_PARTYOPERATION;
  cp += 4;
  wp = (WORD *)cp;

  switch (wRequestType) {
    case 1:  // 파티 생성 요청
      iPartyID = m_pPartyManager->iCreateNewParty(iClientH, cName);
      if (iPartyID == NULL) {
        // 파티 생성 실패!
        *wp = 1;  // 파티 생성 관련 요청에 대한 응답이다.
        cp += 2;
        *cp = 0;  // 생성 실패
        cp++;
        wp = (WORD *)cp;
        *wp = iGSCH;
        cp += 2;
        memcpy(cp, cName, 10);
        cp += 10;
        wp = (WORD *)cp;
        *wp = (WORD)iPartyID;
        cp += 2;
        iRet = m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 22);
      } else {
        // 파티를 만들었다.
        *wp = 1;  // 파티 생성 관련 요청에 대한 응답이다.
        cp += 2;
        *cp = 1;  // 생성 성공
        cp++;
        wp = (WORD *)cp;
        *wp = iGSCH;
        cp += 2;
        memcpy(cp, cName, 10);
        cp += 10;
        wp = (WORD *)cp;
        *wp = (WORD)iPartyID;
        cp += 2;
        iRet = m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 22);
      }
      break;

    case 2:  // 파티 해산 요청
      break;

    case 3:  // 멤버 추가 요청
      bRet = m_pPartyManager->bAddMember(iClientH, iPartyID, cName);
      if (bRet == TRUE) {
        // 멤버 추가 성공
        *wp = 4;  // 멤버 추가에 대한 응답이다.
        cp += 2;
        *cp = 1;  // 추가 성공
        cp++;
        wp = (WORD *)cp;
        *wp = iGSCH;
        cp += 2;
        memcpy(cp, cName, 10);
        cp += 10;
        wp = (WORD *)cp;
        *wp = (WORD)iPartyID;
        cp += 2;
        // 파티 멤버가 추가되었다는 내용은 모든 서버에 전송한다.
        SendMsgToAllGameServers(NULL, cData, 22, TRUE);
      } else {
        // 멤버 추가 실패
        *wp = 4;  // 멤버 추가에 대한 응답이다.
        cp += 2;
        *cp = 0;  // 추가 실패
        cp++;
        wp = (WORD *)cp;
        *wp = iGSCH;
        cp += 2;
        memcpy(cp, cName, 10);
        cp += 10;
        wp = (WORD *)cp;
        *wp = (WORD)iPartyID;
        cp += 2;
        iRet = m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 22);
      }
      break;

    case 4:  // 멤버 제거 요청
      bRet = m_pPartyManager->bRemoveMember(iPartyID, cName);
      if (bRet == TRUE) {
        // 멤버 제거 성공
        *wp = 6;  // 멤버 제거에 대한 응답이다.
        cp += 2;
        *cp = 1;  // 제거 성공
        cp++;
        wp = (WORD *)cp;
        *wp = iGSCH;
        cp += 2;
        memcpy(cp, cName, 10);
        cp += 10;
        wp = (WORD *)cp;
        *wp = (WORD)iPartyID;
        cp += 2;
        // 파티 멤버가 제거되었다는 내용은 모든 서버에 전송한다.
        SendMsgToAllGameServers(NULL, cData, 22, TRUE);
      } else {
        // 멤버 제거 실패
        *wp = 6;  // 멤버 제거에 대한 응답이다.
        cp += 2;
        *cp = 0;  // 제거 실패
        cp++;
        wp = (WORD *)cp;
        *wp = iGSCH;
        cp += 2;
        memcpy(cp, cName, 10);
        cp += 10;
        wp = (WORD *)cp;
        *wp = (WORD)iPartyID;
        cp += 2;
        iRet = m_pClientList[iClientH]->m_pXSock->iSendMsg(cData, 22);
      }
      break;

    case 5:  // 파티 멤버 확인 요청. 서버 이동 상태를 클리어한다.
      m_pPartyManager->bCheckPartyMember(iClientH, iGSCH, iPartyID, cName);
      break;

    case 6:  // 파티 상태 정보 요청. 파티의 총 구성원 수와 이름을 알려준다.
      m_pPartyManager->bGetPartyInfo(iClientH, iGSCH, cName, iPartyID);
      break;

    case 7:  // 멤버의 서버 이동 상태 전환: 이게 세팅된 다음 일정 시간 내에
             // 확인이 안되면 파티에서 제거한다.
      m_pPartyManager->SetServerChangeStatus(cName, iPartyID);
      break;
  }
}
