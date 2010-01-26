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
//

/**
 @file ramod.h
 @internalTechnology
*/


#ifndef __ramod_H__
#define __ramod_H__

#include <e32std.h>
#include <e32property.h>
#include <ip6_hook.h>

#include "ipeventtypes.h"


const TUint KMyProtocolId = 0x333;

//using namespace IPEvent;


class CProtocolInet6Binder;

/**
 *  IP Event Notifier protocol module class.
 *   Coordinates opening and closing sessions, and
 *    catching and publishing events to those sessions
 */
class Cramod : public CIp6Hook
	{
 //
// Object lifetime //
//

public:

	static Cramod* NewL();

	virtual ~Cramod();

protected:

	 // Private constructor so can only be constructed with NewL
	Cramod();

	// Initialisation function- only to be called by NewL (so private).
	void ConstructL();



 //
// Hook management //
//

public:

	// Fills in structure with info about this protocol
	void Identify(TServerProtocolDesc* aProtocolDesc)const;
	static void FillIdentification(TServerProtocolDesc& anEntry);

	// Binds this protocol from the given protocol
	void BindL(CProtocolBase* protocol, TUint id);

	// From CProtocolBaseUnbind. Unbinds this protocol
	void Unbind(CProtocolBase*, TUint);

	// Called by the stack for every incoming packet
	TInt ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo);

protected:
	void RegisterHooksL(void);
	void UnregisterHooks(void);


 //
// Member data /
//
private:

	CProtocolInet6Binder* iProtocolIPv6;					// the IP stack protocol itself

	};


#endif // __ramod_H__



