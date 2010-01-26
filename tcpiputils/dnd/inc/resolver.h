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
// resolver.h - name resolver
//

#ifndef __RESOLVER_H__
#define __RESOLVER_H__

/**
@file resolver.h
Resolver for a query from single client
@internalComponent	Domain Name Resolver
*/

#include <es_sock.h>

class CDndListener;
class CDndEngine;
class MDnsSource;

/**
// Name resolver side of the RHostResolver
//
// When application opens the RHostResolver, a connection through TCP/IP stack
// is created, using a chain of intermediate objects and a socket. A class
// implementing from a MDndResolver is the other end of the "chain":
//
//   RHostResolver <--- .... ---> MDndResolver
//
// There is a limited pool of MDndResolvers, which are reused for new
// different RHostResolver clients.
//
// It has the following simplified state diagram:
//
@verbatim
                   __________________
                 /      reuse        \
                V                     |
 construct -> Start() --> [work] --> Stop() --> destruct
@endverbatim
*/
class MDndResolver
	{
public:
	// Create a new instance of a resolver
	static MDndResolver *New(const TInt aId, CDndEngine &aControl, MDndListener &aListener, MDnsSource &aDnsclient);
	// Start serving a request
	virtual void Start(const TDnsMessageBuf &aMsg) = 0;
	// Stop processing of the current request
	virtual void Stop() = 0;
	// @return The session id of the current request (or 0, if none)
	virtual TUint16 Session() const = 0;
	// @return reference to the time of the last request
	virtual const TTime &RequestTime() const = 0;
	// @return reference to the current reply message
	virtual const TDesC8 &ReplyMessage() = 0;
	// Allow delete via M-class pointer
	virtual ~MDndResolver() {};
	};

#endif
