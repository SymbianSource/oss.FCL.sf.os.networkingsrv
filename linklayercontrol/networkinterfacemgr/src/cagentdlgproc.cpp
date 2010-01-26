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

#include "cagentdlgproc.h"


void CAgentDialogProcessor::Enque(CAgentBase* aItem)
    {
    iQueue.AddLast(aItem);
    iQueueLength++;
    }


TBool CAgentDialogProcessor::IsQueued(CAgentBase* aItem)
    {
   TSglQueIter<CAgentBase*> iter(iQueue);
    CAgentBase** item;
    while ( (item = iter++) != NULL )
        {
        if (*item == aItem)
            {
            return ETrue;
            }
        }
    
    return EFalse;
    }
    
    
CAgentBase* CAgentDialogProcessor::Deque()
	{
	CAgentBase* headOfQueue = NULL;
	if (!IsEmpty())
		{
		headOfQueue = *iQueue.First();
		iQueue.Remove(headOfQueue);
		iQueueLength--;
		}
		
    return headOfQueue;
	}
	

CAgentDialogProcessor::CAgentDialogProcessor()
    {
    }
    
    
void CAgentDialogProcessor::PromptForReconnect(CAgentBase& aAgent)
    {
    if (iCurrentRequest)
        {
        if (iCurrentRequest == &aAgent || IsQueued(&aAgent))
            {
            return;
            }
            
        Enque(&aAgent);
        }
    else
        {
        iCurrentRequest = &aAgent;
        aAgent.DialogProcessor()->Reconnect(*this);
        }
    }
    
    
void CAgentDialogProcessor::CancelPromptForReconnect(CAgentBase& aAgent)
    {
    if (iCurrentRequest == &aAgent)
        {
        iCurrentRequest = NULL;
        aAgent.DialogProcessor()->CancelEverything();
        }
    else
        {
        CAgentBase* a = &aAgent;
        if (IsQueued(a))
        	{
        	iQueue.Remove(a);
        	}        
        }
    }

void CAgentDialogProcessor::MDPOReconnectComplete(TInt aError)
    {
    CAgentBase* completedRequest = iCurrentRequest;
    iCurrentRequest = Deque();
    if (iCurrentRequest)
        {
        // Another prompt to display
        iCurrentRequest->DialogProcessor()->Reconnect(*this);
        }
    if (completedRequest)
        {
        completedRequest->ReconnectComplete(aError);
        }
    }


