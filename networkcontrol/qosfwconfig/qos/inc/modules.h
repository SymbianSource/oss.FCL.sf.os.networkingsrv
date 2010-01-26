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
#ifndef __MODULES_H__
#define __MODULES_H__

#include <e32std.h>
#include <e32base.h>

#include "timeout.h"	// ...because inline StartWatchDog needs Start() from CModuleTimeout

class CInterface;
class CModuleManager;
class CProtocolQoS;
class CModuleSpec;

// module data
class CModuleData : public CBase
	{
	friend class CModuleManager;
private:
	// CModuleData is "almost reference counted object". When the iRefCount==0, the
	// object is only present in CModuleManagers iModuleList (and a watchdog timer
	// is running for it's destruction. Delete can only be issued by self or by
	// CModuleManager.
	~CModuleData();
public:
	static CModuleData* NewLC(const TDesC& aModuleName, TUint aProtocolId, TUint aTimeoutDelay, TDblQue<CModuleData>& aList);
	inline void Open();
	void Close();
	inline CModuleBase* Module();
	inline void StartWatchDog();

public:
	CModuleBase* iModule;
	CProtocolFamilyBase* iFamily;
	TUint iProtocolId;
	TInt iRefCount;
	TUint iFlags;
	RLibrary iLib;
#ifdef _LOG
	TProtocolName iName; // Store the module name for easy debugging.
#endif

protected:
	CModuleData(TUint aProtocolId, TUint aTimeoutDelay);
	void ConstructL(const TDesC& aModuleName);

private:
	static TInt WatchDogCallBack(TAny* aProvider);
	CModuleTimeout *iTimeout;	//?? only used for "shutdown watchdog" -- could try to avoid need of this?
	TUint iTimeoutDelay;
	TDblQueLink iLink;
	};

// inline methods
inline void CModuleData::Open()
	{ iRefCount++; };

inline CModuleBase* CModuleData::Module()
	{ return iModule; };

inline void CModuleData::StartWatchDog()
	{ iTimeout->Start(iTimeoutDelay); };


class RModule;

// library loader
class CModuleManager : public CBase
	{
public:
	static CModuleManager* NewL(CProtocolQoS& aProtocol);
	~CModuleManager();

	RModule* LoadModuleL(CModuleSpec& aModuleSpec, CProtocolBase* aProtocol);
	RModule* LoadModuleL(CProtocolBase* aProtocol, const TDesC& aModuleName, TUint aProtocolId, CExtension* aData = NULL);
	RModule* OpenModuleL(TUint aProtocolId);
	TBool IsLoaded(TUint aProtocolId);
	void Unbind(CProtocolBase* aProtocol, TInt aId);

protected:
	CModuleManager(CProtocolQoS& aProtocol);

private:
	CModuleData* Lookup(TUint aProtocolId);
	inline CProtocolQoS& Protocol();
private:
	CProtocolQoS& iProtocol;
	TDblQue<CModuleData> iModuleList;
	};
	
// inline methods
inline CProtocolQoS& CModuleManager::Protocol()
	{ return iProtocol; }


// A handle to CModuleBase. CFlowHook keeps a list of RModules
class RModule
	{
public:
	RModule(CModuleData& aModuleData);
	RModule(const RModule& aModule);
	~RModule();
	inline CModuleBase* Module();
	inline TUint Flags() const;
	inline TUint ProtocolId() const;
#ifdef _LOG
	inline const TDesC& Name() const;
#endif
private:
	CModuleData& iModuleData;
	};

// inline methods
inline CModuleBase* RModule::Module()
	{ return iModuleData.Module(); };

inline TUint RModule::Flags() const
	{ return iModuleData.iFlags; };

inline TUint RModule::ProtocolId() const
	{ return iModuleData.iProtocolId; };

#ifdef _LOG
inline  const TDesC& RModule::Name() const
	{ return iModuleData.iName; }
#endif

#endif
