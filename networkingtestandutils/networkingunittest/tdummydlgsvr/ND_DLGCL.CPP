// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Dial Up Networking Dialog Server - Client Side Source
// 
//

#include "Nd_dlgsv.h"

const TUint	KNdDlgSvMessageSlots=2;		// PctNotifier & PctRead concurrent requests

#ifdef _DEBUG
#define SETIFDEBUG(R,FUNCTION) TInt R = FUNCTION
#else
#define SETIFDEBUG(R,FUNCTION) FUNCTION
#endif

//
// RGenConAgentDialogServer
//

EXPORT_C RGenConAgentDialogServer::RGenConAgentDialogServer() 
	: RSessionBase(),
	iIAP(0),
	iModemId(0),
	iLocationId(0),
	iConNames(TIspConnectionNames()),
	iPrefs(TConnectionPrefs()),
	iBool(EFalse)
	{}

EXPORT_C RGenConAgentDialogServer::~RGenConAgentDialogServer()
	{}

EXPORT_C TInt RGenConAgentDialogServer::Connect()
//
// Connect to the  server
//
	{
	TInt ret = CreateSession(DIALOG_SERVER_NAME,Version(),KNdDlgSvMessageSlots);

	if (ret!=KErrNone)
		{
		ret=StartDialogThread();
		if (ret!=KErrNone)
			return(ret);

		ret = CreateSession(DIALOG_SERVER_NAME,Version(),KNdDlgSvMessageSlots);
		}
	return ret;
	}

EXPORT_C void RGenConAgentDialogServer::Close()
	{
	RHandleBase::Close();
	}

EXPORT_C TVersion RGenConAgentDialogServer::Version(void) const
//
// Return the client side version number
//
	{
	return(TVersion(KDialogServMajorVersionNumber,KDialogServMinorVersionNumber,KDialogServBuildVersionNumber));
	}

EXPORT_C void RGenConAgentDialogServer::ModemAndLocationSelection(TUint32 &aModemId,TUint32 &aLocationId, TRequestStatus &aStatus)
	{
	::new(&iModemId) TPckg<TUint32>(aModemId);
	::new(&iLocationId) TPckg<TUint32>(aLocationId);

	TAny* p[KMaxMessageArguments];
	p[0]=&iModemId;
	p[1]=&iLocationId;
	
	TIpcArgs args;
	args.Set(0,p[0]);
	args.Set(1,p[1]);
	SendReceive(EGetModemAndLocation,args,aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::IapConnection(TUint32& aIAP, const TConnectionPrefs& aPrefs, TRequestStatus& aStatus)
	{
	IapConnection(aIAP, aPrefs, KErrNone, aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::IapConnection(TUint32& aIAP, const TConnectionPrefs& aPrefs, TInt aLastError, TRequestStatus& aStatus)
	{
	::new(&iIAP) TPckg<TUint32>(aIAP);
	iPrefs() = aPrefs;

	TAny* p[KMaxMessageArguments];
	p[0]=&iIAP;
	p[1]=&iPrefs;
	p[2]=(TAny*)aLastError;

	TIpcArgs args;
	args.Set(0,p[0]);
	args.Set(1,p[1]);
	args.Set(2,p[2]);	
	SendReceive(EGetIAP,args,aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::WarnNewIapConnection(const TConnectionPrefs& aPrefs, TInt aLastError, const TDesC& aNewIapName, TBool& aResponse, TRequestStatus& aStatus)
	{
	iPrefs() = aPrefs;
	iConNames().iServiceName = aNewIapName;
	::new(&iBool) TPckg<TBool>(aResponse);

	TAny* p[KMaxMessageArguments];
	p[0]=&iPrefs;
	p[1]=(TAny*)aLastError;
	p[2]=(TAny*)&iConNames().iServiceName;
	p[3]=&iBool;

	TIpcArgs args;
	args.Set(0,p[0]);
	args.Set(1,p[1]);
	args.Set(2,p[2]);
	args.Set(3,p[3]);
	SendReceive(EWarnNewIAP,args,aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::Login(TDes& aUsername,TDes& aPassword,TBool aIsReconnect,TRequestStatus& aStatus)
	{
	TAny* p[KMaxMessageArguments];
	p[0]=&aUsername;
	p[1]=&aPassword;
	p[2]=(TAny*)aIsReconnect;

	TIpcArgs args;
	args.Set(0,p[0]);
	args.Set(1,p[1]);
	args.Set(2,p[2]);
	SendReceive(EGetLogin,args,aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::Authenticate(TDes& aUsername,TDes& aPassword,TBool aIsReconnect,TRequestStatus& aStatus)
	{
	TAny* p[KMaxMessageArguments];
	p[0]=&aUsername;
	p[1]=&aPassword;
	p[2]=(TAny*)aIsReconnect;

	TIpcArgs args;
	args.Set(0,p[0]);
	args.Set(1,p[1]);
	args.Set(2,p[2]);
	SendReceive(EGetAuthentication,args,aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::Reconnect(TBool& aBool, TRequestStatus& aStatus)
	{
	::new(&iBool) TPckg<TBool>(aBool);

	TAny* p[KMaxMessageArguments];
	p[0]=&iBool;

	TIpcArgs args;
	args.Set(0,p[0]);
	SendReceive(EGetReconnectReq,args,aStatus);
	}

EXPORT_C TInt RGenConAgentDialogServer::OpenPct()
	{
	TIpcArgs args;
 	return SendReceive(EOpenPct,args);	
	}

EXPORT_C TInt RGenConAgentDialogServer::WritePct(const TDesC& aData)
	{
	TAny* p[KMaxMessageArguments];
	p[0]=(TAny*)&aData;
	TIpcArgs args;
 	args.Set(0,p[0]);
	return SendReceive(EWritePct,args);	
	}

EXPORT_C void RGenConAgentDialogServer::ReadPct(TDes& aData, TRequestStatus& aStatus)
	{
	TAny* p[KMaxMessageArguments];
	p[0]=&aData;
	TIpcArgs args;
 	args.Set(0,p[0]);
	SendReceive(EReadPct,args,aStatus);	
	}

EXPORT_C void RGenConAgentDialogServer::DestroyPctNotification(TRequestStatus& aStatus)
	{
	TIpcArgs args;
 	SendReceive(EDestroyPctNotification,args,aStatus);	
	}

EXPORT_C void RGenConAgentDialogServer::ClosePct()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(EClosePct,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrClosePctReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::QoSWarning(TBool& aResponse, TRequestStatus& aStatus)
	{
	::new(&iBool) TPckg<TBool>(aResponse);

	TAny* p[KMaxMessageArguments];
	p[0]=&iBool;
	TIpcArgs args;
 	args.Set(0,p[0]);
	SendReceive(EWarnQoS,args,aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::CancelModemAndLocationSelection()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelGetModemAndLocation,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::CancelIapConnection()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelGetIAP,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}	

EXPORT_C void RGenConAgentDialogServer::CancelWarnNewIapConnection()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelWarnIAP,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}	

EXPORT_C void RGenConAgentDialogServer::CancelLogin()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelLogin,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::CancelAuthenticate()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelAuthenticate,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::CancelReconnect()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelReconnect,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::CancelReadPct()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelReadPct,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::CancelDestroyPctNotification()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelDestroyPctNotification,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::CancelQoSWarning()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelWarnQoS,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C void RGenConAgentDialogServer::RequestDialogAppearanceNotification(TRequestStatus& aStatus)
	{
	TIpcArgs args;
 	SendReceive(EObserveDialogs,args,aStatus);
	}

EXPORT_C void RGenConAgentDialogServer::SetReconnectDialogResponse(TBool aResponse)
	{
	TAny* p[KMaxMessageArguments];
	p[0]=REINTERPRET_CAST(TAny*,aResponse);
	TIpcArgs args;
 	args.Set(0,p[0]);
	SendReceive(ESetReconnectResponse,args);
	}

EXPORT_C void RGenConAgentDialogServer::SetQoSWarnDialogResponse(TBool aResponse)
	{
	TAny* p[KMaxMessageArguments];
	p[0]=REINTERPRET_CAST(TAny*,aResponse);
	TIpcArgs args;
 	args.Set(0,p[0]);
	SendReceive(ESetQoSWarnResponse,args);
	}

EXPORT_C void RGenConAgentDialogServer::CancelDialogAppearanceNotification()
	{
	TIpcArgs args;
 	SETIFDEBUG(ret, SendReceive(ECancelObserveDialogs,args));

	__ASSERT_DEBUG(ret==KErrNone, PanicServer(ESvrCancelReturnedError));
	}

EXPORT_C TInt RGenConAgentDialogServer::SetTestNumber(const TInt aTestNumber)
	{
	TAny* p[KMaxMessageArguments];
	p[0]=REINTERPRET_CAST(TAny*,aTestNumber);
	TIpcArgs args;
 	args.Set(0,p[0]);
	return SendReceive(ESetTestNumber,args);
	}
//===================================
