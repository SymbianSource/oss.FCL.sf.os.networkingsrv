// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#ifndef __ANVLSOCK_H__
#define __ANVLSOCK_H__

#include <c32comm.h>
#include <es_sock.h>
//#include <in_sock.h>
#include <in_sock.h>

#include "mntcpos.h"
#include "mntcpapp.h"

#include "server.h"

#define ANVLSOCK_CHECK_SOCK_VALUE_IN_USE   0x12af01f5
#define ANVLSOCK_CHECK_SOCK_VALUE_RELEASED 0x30e367c4

// NEW SOCKET OPTION
#define TCP_CORK 0x1020

class CAnvlSockMain : public CBase
    {
public:
    void InitL(void);
    RSocketServ &SocketServer(void);
    void Close();
    void PrintInt(TInt number);
    void PrintStr(char *str);
    void PrintfStr(char *format, char *str);
    void PrintfInt(char *format, TInt number);

private:

private:
    RSocketServ                 iSocketServer;
    RBrdBroadcastServerSession  iOwnServer;
    };


class CAnvlSock : public CBase
    {
public:
    static CAnvlSock *OpenBlankAnvlSocketL(void);

    void PrintErrorSetSockOpt(char *str);
    void PrintErrorGetSockOpt(char *str);
    
    void AllocCheckSockValue(void);
    TBool ReleaseCheckSockValue(void);
    TBool TestCheckSockValueOK(void);

    void SetSockMain(void);
    void CheckL(TInt aValue, TInt aCheckValue, TInt aLineNumber);
    int OSSockClose(void);
    int OSSockBind(unsigned long int srcAddr, unsigned short srcPort);
    MNTCPAPP_OS_SOCKET OSSockSocketL(unsigned short sType);
    int OSSockSendTo(const void *msg, int msgLen, MNTCPAppSocketInfo_t dst);
    int OSSockRecvFrom(void *buff, int bufflen, MNTCPAppSocketInfo_t *from);
    int OSSockConnect(unsigned long int dstIPAddr, unsigned short dstPort);
    MNTCPAPP_OS_SOCKET OSSockAccept(MNTCPAppSocketInfo_t *nameInfo);
    int OSSockFcntl(unsigned short cmd);
    int OSSockListen(int backlog);
    int OSSockSetSockOpt(SockOpt_t opt, int value1, int value2);
    int OSSockSend(char *buff, unsigned int nBytes, boolean urgent);
    int OSSockRecv(char *buff, unsigned int nBytes);
    int OSSockGetSockOpt(SockOpt_t opt, int *value);
    int OSSockShutDn(int type);
    
private:

private:
    TInt            iCheckSockValue;
    RSocket         iSock;
    CAnvlSockMain   *iSockMain;
    TRequestStatus  iReqStatus;
    };

CAnvlSockMain *GetSockMain(void);

void PrintErrorSocketIsZero(char *str);


#endif

