// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Connection Test Step Header
// 
//

/**
 @file 
*/
#ifndef __TEF_IPSEC_SELECTORCONFLICT_H__
#define __TEF_IPSEC_SELECTORCONFLICT_H__

#include <es_sock.h>
#include <e32base.h> 
#include <testexecutestepbase.h>
//local includes here
#include "te_IPSec_Load_Policy_Base.h"

//---------------------------------------------------------------------------
class CT_SelectorConflict : public CTimer
{
public:
	~CT_SelectorConflict();
	static CT_SelectorConflict* NewL(int aIapId);
	void RecvUDP();
	void SendTcpPacketL();
	void RecvTcpPacketL();
	void ConnectL(TInt aPort, TPtrC16 aIpDAddr);
	void Close();		
	void delay(CT_IPSec_Load_Policy_Base *aBase, TInt KDelayTime);
	TBool GetKeepAlive();	
	void SetKeepAlive( TBool aKeepAlivetimeout);	
	void start(int aIapId);

private:
	CT_SelectorConflict();
	void ConstructL(int aIapId);

	void RunL();
	void DoCancel();	
	TInt RunError(TInt /*aError*/);
private:    
    RSocketServ     iSs;
    RSocket         iSock;
    RSocket         iRecvSock;
    RConnection     iConn;
    TBuf8<256>      iRecvBuf;
    TSockAddr       iSenderAddr;
    TSockXfrLength  iBytesRead;
	TBool iKeepalive;
	CT_IPSec_Load_Policy_Base *iPtrKeepAlive; 
public :
	static CT_SelectorConflict *iSelfPtr;
};

#endif// __TEF_IPSEC_SELECTORCONFLICT_H__

