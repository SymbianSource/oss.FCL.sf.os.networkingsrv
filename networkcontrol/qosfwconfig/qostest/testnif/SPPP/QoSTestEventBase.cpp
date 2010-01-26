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
// CQoSTestEventBase.CPP
// 
//

#include <in_iface.h>
#include <e32std.h>
#include <e32std.h>
#include "QoSTestEventBase.h"
#include "QoSTestLog.h"

//
// CQoSTestEventBase Class
//
CQoSTestEventBase::CQoSTestEventBase()
: iPrimaryEventCallBack(CActive::EPriorityStandard)
	{
	iPrimaryEventCallBack.Set(TCallBack(PrimaryCallBack, this));
	iSendprimEvent = true;
	};

CQoSTestEventBase::~CQoSTestEventBase()
{
	delete iPrimaryParam;
	iPrimaryParam = NULL;	
};

TInt CQoSTestEventBase::PrimaryCallBack(TAny* aArg)
	{
	CQoSTestEventBase* p = STATIC_CAST(CQoSTestEventBase*, aArg);

	return p->RaisePrimaryEvent();
	}

TInt CQoSTestEventBase::RaisePrimaryEvent()
	{
	ASSERT(iPrimaryParam != NULL);
	TPtr8 ptr(iPrimaryParam->Des()); 
	RaiseEvent(KPrimaryContextCreated, ptr);
	iSendprimEvent = false;

	LOG(_LIT(string1,"Raise Primary Event");)
	LOG(PdpLog::Write(string1);)						
	return KErrNone;
	}

TInt CQoSTestEventBase::Control(TUint aLevel,TUint aName,TDes8& aOption)
	{
	if (aLevel == KSOLInterface)
		{
		TNifEvent& opt = *(TNifEvent*)aOption.Ptr();
		switch (aName)
			{

			case KRegisterEventHandler:
				LOG(_LIT(string1,"Control(): Register Event Handler");)
				LOG(PdpLog::Write(string1);)						

				iNifEvent.iEvent = (MNifEvent*) opt.iEvent;
				return KErrNone;
				

			case KContextSetEvents:
				LOG(_LIT(string2,"Control(): Set status of Event Handler");)
				LOG(PdpLog::Write(string2);)				

				iNifEvent.iIsOn = !iNifEvent.iIsOn ;			// what will happend here if we wont find it ?????????
					
				if ((iPrimaryParam != NULL) && (iSendprimEvent))
					{
					iPrimaryEventCallBack.CallBack();
					}
				if (!iNifEvent.iIsOn)
					{
					iSendprimEvent = true;
					}
				return KErrNone;
			}
		}
	return KErrNotSupported;
	};



TInt CQoSTestEventBase::RaiseEvent(TUint aName, TDes8& aOption)
{
	if ((iNifEvent.iIsOn) && 
		((aName != KPrimaryContextCreated) || ((aName == KPrimaryContextCreated) && (iSendprimEvent))))
		{
		MNifEvent* nifevent;
		nifevent = iNifEvent.iEvent;
		nifevent->Event((CProtocolBase*)this, aName, aOption);
		iSendprimEvent = false;
		}
		
	if (aName == KPrimaryContextCreated)
		{
		// When RaiseEvent() is called from RaisePrimaryEvent(), then aOption and
		// iPrimaryParam point to the same object (!).  tmpBufPtr is used to avoid
		// deleting the object via iPrimaryParam, only to get a crash when we do
		// aOption.Alloc().
		HBufC8 *tmpBufPtr = aOption.Alloc();
		
 		if (iPrimaryParam)
			{
			delete iPrimaryParam;
			}
		iPrimaryParam = tmpBufPtr;
		tmpBufPtr = NULL;
		ASSERT(iPrimaryParam != NULL);
		}

	return KErrNone;
};

//
// TNifEvent Class
//
TNifEvent::TNifEvent()
	{
	iIsOn =EFalse;
	};
TNifEvent::~TNifEvent()
	{
	iEvent = NULL;
	iIsOn = EFalse;
	};
