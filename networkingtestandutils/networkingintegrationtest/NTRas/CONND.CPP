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
//

#include "rasdef.h"

CConnectBox::CConnectBox(CNtRas* aNtRas)
	: CActive(KKeyReaderPriority), iNtRas(aNtRas)
	{
	__DECLARE_NAME(_S("CConnectBox"));
	CActiveScheduler::Add(this);
   _LIT(KYes, "yes");
	iYes=KYes;
   _LIT(KNo, "no");
	iNo=KNo;
   _LIT(KNTRas, "NT Ras");
	iLine1=KNTRas;
	}

CConnectBox::~CConnectBox()
	{
	Cancel();
	iNotifier.Close();
	}

CConnectBox* CConnectBox::NewL(CNtRas* aNtRas, TBool aReconnect)
	{
	
	CConnectBox* p = new (ELeave) CConnectBox(aNtRas);
	CleanupStack::PushL(p);
	p->ConstructL(aReconnect);
	CleanupStack::Pop();
	return p;
	}

void CConnectBox::ConstructL(TBool aReconnect)
	{

	TUint32 servermode, yestoconnect;

	iNtRas->ReadInt(NIF_IFSERVERMODE, servermode);
	if(aReconnect)
	    iNtRas->ReadInt(RAS_YESTORECONNECT, yestoconnect);
	else
		iNtRas->ReadInt(RAS_YESTOCONNECT, yestoconnect);

	if(servermode||yestoconnect)
		{
		iButtonVal=1;
		TRequestStatus* p = &iStatus;
		User::RequestComplete(p, KErrNone);
		SetActive();
		return;
		}
	User::LeaveIfError(iNotifier.Connect());
	if (aReconnect)
		{
		_LIT(KReconnect, "Reconnect ?");
		iLine2=KReconnect;
		}
	else
		{
		_LIT(KConnect, "Connect ?");
		iLine2=KConnect;
		}
	iNotifier.Notify(iLine1, iLine2, iNo, iYes, iButtonVal, iStatus);
	SetActive();
	}

void CConnectBox::RunL()
	{
	if (iStatus!=KErrNone)
		{
		iNtRas->ConnectDialogComplete(iStatus.Int());
		return;
		}

	switch (iButtonVal)
		{		
	case 1:
		iNtRas->ConnectDialogComplete(KErrNone);
		break;
	
	default:
		iNtRas->ConnectDialogComplete(KErrCancel);
		break;
		}
	}

void CConnectBox::DoCancel()
	{

	}
