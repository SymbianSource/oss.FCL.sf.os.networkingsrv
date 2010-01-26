// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 
#ifndef __TC_H__
#define __TC_H__

#include <e32base.h>
#include <e32std.h>
#include "iface.h"
#include "async_request.h"

class CFlowData : public CBase
    {
    public:
    static CFlowData* NewL(CFlowContext* aHandle, CNif* aNif);
    ~CFlowData();
    inline CFlowContext* FlowContext() const;
    inline CNif* Nif();
    inline TInt ChannelId() { return iChannelId; };
    inline void SetChannelId(TInt aChannelId) { iChannelId = aChannelId; };

    TSglQueLink iLink;

    private:
    CFlowData(CFlowContext* aHandle, CNif* aNif);
    void ConstructL();

    private:
    CNif*				iNif;
    CFlowContext*		iFlowContext;
    TInt	 			iChannelId;
    };

#include "tc.inl"

#endif
