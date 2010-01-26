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
 @internalComponent
*/
#ifndef __QOS_INTERFACE_H__
#define __QOS_INTERFACE_H__

#include <e32std.h>

class CProtocolQoS;
class RModule;
class CNifIfBase;
class TSoIfControllerInfo;

class CInterface : public CBase
	{
public:
	static CInterface* NewL(CProtocolQoS& aProt, CNifIfBase* aNif);
	~CInterface();

	inline RModule* TrafficControl();
	inline void SetTrafficControl(RModule* aModule);
	inline TInt TrafficControlStatus() const;
	inline CProtocolQoS& Protocol();
	inline const TDesC& Name() const;
	inline const TDesC& InterfaceName() const;
	inline TUint32 IapId() const;
	inline CNifIfBase* Nif();

	TSglQueLink iLink;
private:

	CInterface(CProtocolQoS& aProt, CNifIfBase* aNif);
	void ConstructL();
	void GetInterfaceInfo();
	TInt GetPlugIn(TSoIfControllerInfo& aPluginInfo);
	TInt LoadControlModule();

private:
	CProtocolQoS& iProtocol;		// Reference to protocol class (immutable)
	CNifIfBase *const iNif;			// Nif (immutable)
	RModule* iTrafficControl;		// Traffic control module
	TInt iTrafficControlStatus;		// Traffic control module status (indicates error, if load fails)
	TName iName;					// Name of the interface in TCP/IP stack
	TPtrC iInterfaceName;			// Interface name (i.e. instance id is not visible)
	TUint32 iIapId;					// IAP Id of the Nif
	};


class CInterfaceManager : public CBase
	{
public:
	static CInterfaceManager* NewL(CProtocolQoS& aProtocol);
	~CInterfaceManager();

	CInterface* AddInterfaceL(CNifIfBase* aNif);
	void RemoveInterface(CInterface* aInterface);
	CInterface* FindInterface(CNifIfBase* aNif);

protected:
	CInterfaceManager(CProtocolQoS& aProtocol);

private:
	CProtocolQoS& iProtocol;
	TSglQue<CInterface>	iInterfaces;
	};

#include "interface.inl"

#endif
