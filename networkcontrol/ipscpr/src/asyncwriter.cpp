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
        iPendingMsg.SetOffset(_FOFF(CQoSMsg, iLink));
        }

void CAsyncWriter::ConstructL()
	{
	CActiveScheduler::Add(this);
	}

void CAsyncWriter::Send(CQoSMsg* aMessage)
	{
	if (IsActive())
 		{
 		iPendingMsg.AddLast(*aMessage);
 		}
 	else
 		{
 		iWriter->Send(aMessage);
 		SetActive();
 		TRequestStatus* stat = &iStatus;
 		User::RequestComplete(stat,KErrNone);
 		}
	}
	
void CAsyncWriter::RunL()
	{
	ASSERT(iWriter);
	if (!iPendingMsg.IsEmpty())
 		{
 		CQoSMsg* msg = iPendingMsg.First();
 		iPendingMsg.Remove(*msg);
 		iWriter->Send(msg);
 		SetActive();
 		}
	}
	
void CAsyncWriter::DoCancel()
	{
	//No code needed here becuase the request is completed immediately on activation
	}
