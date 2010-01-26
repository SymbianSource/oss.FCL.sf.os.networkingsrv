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

CLoginBox::CLoginBox(CNtRas* aNtRas)
	: CActive(KKeyReaderPriority), iNtRas(aNtRas)
	{
	__DECLARE_NAME(_S("CLoginBox"));
	CActiveScheduler::Add(this);
	}

CLoginBox::~CLoginBox()
	{
	Cancel();
	}

CLoginBox* CLoginBox::NewL(CNtRas* aNtRas, TDes& aUsername, TDes& aPassword)
	{
	
	CLoginBox* p = new (ELeave) CLoginBox(aNtRas);
	CleanupStack::PushL(p);
	p->ConstructL(aUsername, aPassword);
	CleanupStack::Pop();
	return p;
	}

void CLoginBox::ConstructL(TDes& aUsername, TDes& aPassword)
	{
	TInt res=iNtRas->ReadDes(RAS_AUTHENTICATENAME, aUsername);
	if(res==KErrNone)
	    res=iNtRas->ReadDes(RAS_AUTHENTICATEPASSWORD, aPassword);

	SetActive();
	TRequestStatus* sp = &iStatus;
	User::RequestComplete(sp, res);
	}


void CLoginBox::RunL()
	{

	iNtRas->AuthenticateDialogComplete(iStatus.Int());
	}

void CLoginBox::DoCancel()
	{
	}
