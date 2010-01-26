// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "asyncwriter.h"


CAsyncWriter* CAsyncWriter::NewL(CQoSMsgWriter* aWriter)
	{
	CAsyncWriter* self = new (ELeave) CAsyncWriter(aWriter);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CAsyncWriter::CAsyncWriter(CQoSMsgWriter* aWriter) :
	CActive(EPriorityNormal),
	iWriter(aWriter)
	{
    }

void CAsyncWriter::ConstructL()
	{
	CActiveScheduler::Add(this);
	}

void CAsyncWriter::Send(CQoSMsg* aMessage)
	{
	iMessage = aMessage;
	iStatus = KRequestPending;
 		SetActive();
 		TRequestStatus* stat = &iStatus;
 		User::RequestComplete(stat,KErrNone);
	}
	
void CAsyncWriter::RunL()
	{
	ASSERT(iMessage);
	ASSERT(iWriter);
	iWriter->Send(iMessage);
	}
	
void CAsyncWriter::DoCancel()
	{
	//No code needed here becuase the request is completed immediately on activation
	}
