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
// PPP CFProtocol message implementations
// 
//

/**
 @file
 @internalComponent
*/


#ifndef PPPMESSAGES_H
#define PPPMESSAGES_H

#include <comms-infras/agentmessages.h>

class TPPPMessage
    {
public:
    enum { ERealmId = 0x10281C4E };
    
private:
    //
    // Messages are grouped according to direction in an attempt to make it easier
    // to decipher message direction whilst debugging.
    //
    enum
    /**
    Definition of generic Link Tier message ids
    */
    	{
    	KPppLinkExpectingCallback = Messages::KNullMessageId + 1,
    	};
public:
    typedef Messages::TMessageSigVoid<KPppLinkExpectingCallback, TPPPMessage::ERealmId> TPppLinkExpectingCallback;
    };

#endif
// PPPMESSAGES_H
