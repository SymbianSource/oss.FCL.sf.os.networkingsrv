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
 @file ipeventnotifier.h
 @internalTechnology
*/


#ifndef __IPEVENTNOTIFIER_H__
#define __IPEVENTNOTIFIER_H__

#include <e32std.h>
#include <e32property.h>
#include <ip6_hook.h>

#include <networking/ipeventtypes.h>
#include "ipeventnotifierinterface.h"


using namespace IPEvent;


class CProtocolInet6Binder;
class MEventService;
class CIPEventListener;
class CDHCPUnicastTranslator;

/**
 *  IP Event Notifier protocol module class.
 *   Coordinates opening and closing sessions, and
 *    catching and publishing events to those sessions
 */
class CIPEventNotifier : public CIp6Hook
	{
 //
// Object lifetime //
//

public:

	static CIPEventNotifier* NewL();

	virtual ~CIPEventNotifier();

protected:

	 // Private constructor so can only be constructed with NewL
	CIPEventNotifier();

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

	// Gets hold of the event service if possible, storing pointer to it in member variable
	void ObtainEventService();




 //
// Event type accessors //
//

public:

	inline Meta::SMetaData& MFlagEventType() const;
    inline Meta::SMetaData& IPReadyEventType() const;
	inline Meta::SMetaData& LinkLocalAddressEventType() const;




 //
// Publishing / interfaces management //
//

public:

	/**
	 * Performs one of the following actions, depending on the option name:
	 *
	 * KHandleAttach: Sets up IPEN to catch and publish events on the interface named in aOption.
	 *                  Session handle TUint (the pubsub address key) returned in first 4 bytes of aOption buffer.
	 *
	 * KHandleRelease: Tells IPEN to stop catching and publishing events to the given session handle
	 *                   Handle passed in as TUint in first 4 bytes of aOption buffer.
	 *
	 * @param aLevel: should be KSolInetIp for this protocol
	 * @param aName: see above
	 * @param aOption: in/out, dependent on option name. See above
	 * @return error status
	 *
	 */
	TInt GetOption(TUint aLevel, TUint aName, TDes8& aOption, CProtocolBase* aSourceProtocol=NULL);


protected:

	// Opens a session on the given interface name. Returns session handle.
	TUint OpenSessionL(const TDesC& aIfName);

	// Closes session with given handle. Returns error status.
	TInt  CloseSession(TUint aHandle);

public:
	void AddInterfaceL(TUint32 aIfIndex, const TDesC& aIfName);
	TBool IsInterfaceKnown(TUint32 aIfIndex) const;

protected:
	void AddKnownInterfacesL();
	static HBufC8* GetBuffer(HBufC8* apBuf, TInt aBufLenRequired);

	TInt FindPositionOfInterface(TUint32 aIfIndex) const;
	TInt FindPositionOfInterfaceByName(const TDesC& aIfName) const;
	CIPEventNotifierInterface* GetInterfaceWithIndexL(TInt aIpIfIndex);

	TBool AnyClientsCurrentlyInterested(void) const;


	void RegisterHooksL(void);
	void UnregisterHooks(void);
	void InterfaceDetached(const TDesC & aName, CNifIfBase *aIf);
#ifdef _DEBUG	
	void LogKnownInterfaces();
#endif

public:

	// Defines pub/sub properties
	TInt DefineProperty(Meta::SMetaData& aEvent, TUint aHandle);

	// Whether or not we should publish given message for given interface index.
	// N.B. creates interface with given index if none known, which is why it's not const
	TBool ShouldPublishEvent(TUint aInterfaceIndex);

	// Perform publish of given message for given session handle
	void PublishToHandle(TUint aHandle, Meta::SMetaData& aData);

	void PublishMFlagL(TInt aIpIfIndex, TBool aMflag, TBool aOflag);
	void PublishIPReadyL(TInt aIpIfIndex, const TInetAddr& tmpInetAddr, TBool addressValid);
	void PublishLinklocalAddressKnownL(TInt aIpIfIndex, const TInetAddr& tmpInetAddr);

	void PublishLastKnownStates(CIPEventNotifierInterface* aIface);


 //
// Member data /
//
private:

	CProtocolInet6Binder* iProtocolIPv6;					// the IP stack protocol itself

	MEventService* iEventService;							// the stack's event service
	CIPEventListener* iIPEventListener;						// our handler for iEventService events

	RPointerArray<CIPEventNotifierInterface> iInterfaces;	// network interfaces that we may be monitoring

	RProperty iPublisher;									// publish/subscribe interface

	CDHCPUnicastTranslator* iDHCPUnicastTranslator;			// hook to detect unicast BOOTP packets and convert them to broadcast

	};


#include "ipeventnotifier.inl"


#endif // __IPEVENTNOTIFIER_H__



