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

#if !defined(QOSTESTEVENTBASE_H)
#define QOSTESTEVENTBASE_H

#include <networking/umtsnifcontrolif.h>
#include <ss_protflow.h>
#include "QosTestLcp.h"

class TNifEvent
	{
public:
	TNifEvent();
	~TNifEvent();
	MNifEvent	*iEvent;
	TBool		iIsOn;
	};


class CQoSTestEventBase : public CBase, public ESock::MLowerControl
	{
public:
	~CQoSTestEventBase();
	
    //-=========================================================
    // MLowerControl methods
    //-=========================================================	
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);
	
    //-=========================================================
    // Custom methods
    //-=========================================================		
	TInt				RaiseEvent(TUint aName, TDes8& aOption);
	static TInt			PrimaryCallBack(TAny* aArg);
	
protected:
	CQoSTestEventBase();
	TInt				RaisePrimaryEvent();
	TNifEvent			iNifEvent;

	HBufC8*				iPrimaryParam;
	TBool				iSendprimEvent;
	CAsyncCallBack		iPrimaryEventCallBack;
	};

#endif
//QOSTESTEVENTBASE_H
