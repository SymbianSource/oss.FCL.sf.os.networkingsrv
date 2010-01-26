// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#ifndef CAGENTDLGPROC_H
#define CAGENTDLGPROC_H

#include <e32base.h>
#include <comms-infras/cagentbase.h>
#include <comms-infras/cnetworkcontrollerbase.h>


class CAgentDialogProcessor : public CBase, private MDialogProcessorObserver
    {
public:
    CAgentDialogProcessor();
    
    void PromptForReconnect(CAgentBase& aAgent);
    void CancelPromptForReconnect(CAgentBase& aAgent);
    
private:
    // Implementation of MDialogProcessorObserver
    // All other interface members will panic if called
	virtual void MDPOReconnectComplete(TInt aError);

    // Queuing methods
	inline TBool IsEmpty() const;
	void Enque(CAgentBase* aItem);
	CAgentBase* Deque();
	TBool IsQueued(CAgentBase* aItem);

private:
    CAgentBase* iCurrentRequest;
    TSglQue<CAgentBase*> iQueue;
	TUint iQueueLength;
    };

    
inline TBool CAgentDialogProcessor::IsEmpty() const
    {
    return iQueueLength == 0;
    }

#endif
// CAGENTDLGPROC_H

