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
// prt.h - Packet Probe Hook
//



/**
 @internalComponent
*/
#ifndef __PROBE_PRT_H
#define __PROBE_PRT_H

#include <e32std.h>
#include <es_sock.h>
#include <nifman.h>

#include "family.h"
#include "in_bind.h"
#include "posthook.h"

class CProviderProbe;

class CProtocolProbe : public CProtocolPosthook
{
	friend class CProviderProbe;
	CProtocolProbe(TUint aId);
	virtual ~CProtocolProbe();
public:
	virtual void InitL(TDesC& aTag);
	virtual void StartL();
	virtual void Identify(TServerProtocolDesc *aDesc) const;
	static void FillIdentification(TServerProtocolDesc& aDesc, TUint aId);
	static CProtocolProbe *NewL(TUint aId);

	virtual void NetworkAttachedL();
	virtual TInt Send(RMBufChain &aPacket, CProtocolBase* aSrc);
	virtual void Process(RMBufChain &aPacket, CProtocolBase* aSrc);
	
	virtual CServProviderBase* NewSAPL(TUint aProtocol);
	void CancelSAP(const CServProviderBase* aSAP);
protected:
	void Deliver(RMBufChain &aPacket);
	const TUint iId;
	CProviderProbe *iList;
};

#endif
