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
// @file policy_mgr.h
// Header file for qos policy manager
// @internalTechnology
// @released
//

#ifndef __POLICY_MGR_H__
#define __POLICY_MGR_H__

#include <e32std.h>
#include <in_sock.h>
#include <es_ini.h>
#include <in_sock.h>
#include "pfqoslib.h"
#include "pfqosparser.h"
#include "qos_ini.h"
#include "policies.h"

#include "qosparameters.h"

class CQoSProvider;
class CProtocolQoS;
class CFlowContext;
class CFlowHook;

/** @internalTechnology */
typedef TDblQue<CSelectorBase> TPolicyQueue;
/** @internalTechnology */
typedef TDblQueIter<CSelectorBase> TPolicyQueIter;
/** @internalTechnology */
const TUint KPolicyTableSize = 7; 


/** @internalTechnology */
class TPolicyCombine
	{
public:
	static TInt CombineDefault(const TQoSParameters& aDefault, TQoSParameters& aSpec);
	static TInt CombineOverride(const TQoSParameters& aOverride, TQoSParameters& aSpec);
	};

/** @internalTechnology */
class TPolicyFile
	{
public:
	TPolicyFile(const TDesC& aFile) : iName(aFile) {}
	~TPolicyFile() {}

public:
	TFileName iName;
	TDblQueLink iLink;
	};

/** @internalTechnology */
class CQoSPolicyMgr : public CBase
	{
public:
	static CQoSPolicyMgr* NewL(const TDesC& aFileName);
	~CQoSPolicyMgr();
	
	CPolicySelector* AddPolicyL(TPfqosMessage &aMsg);
	void AddModulePolicyL(TPfqosMessage &aMsg, TUint aOwner);
	void AddExtensionPolicyL(TPfqosMessage &aMsg, TUint aOwner);

	CSelectorBase* FindPolicy(const CFlowContext* iContext, TUint aType, const TUidType& aUid, TUint32 aIapId, TUint aPriority, const TDesC& aName=TPtr(NULL,0));
	CSelectorBase* ExactMatch(const TInetAddr &aLocal, const TInetAddr &aRemote, TUint aProtocol, TUint iSrcPortMax, TUint iDstPortMax, TUint aType, 
		const TUidType& aUid, TUint32 aIapId, TUint aPriority=EPfqosApplicationPriority, const TDesC& aName=TPtr(NULL,0));
	CSelectorBase* ExactMatch(TPfqosMessage& aMsg, TUint aPolicyType, TUint aPriority=EPfqosApplicationPriority, const TDesC& aName=TPtr(NULL,0));

	CSelectorBase* Match(const TIp6Addr &aLocal, const TIp6Addr &aRemote, TUint aProtocol, 
		TUint aLocalPort, TUint aRemotePort, TUint aType, const TUidType& aUid, TUint32 aIapId, TUint aPriority=EPfqosApplicationPriority, const TDesC& aName=TPtr(NULL,0));
	TInt LoadFileL(const TDesC& aFile);
	TInt UnloadFile(const TDesC& aFile, CProtocolQoS& aProtocol);
	void AddPolicy(CSelectorBase &aPolicy);
	void DoCleanUp(CProtocolQoS& aProtocol, TUint aOwner);

	void Flush();
	
	inline TPolicyQueue& Policies(TInt aHashKey) { return iPolicies[aHashKey]; }

protected:
	CQoSPolicyMgr();
	void ConstructL(const TDesC& aFileName);
	TInt CreatePolicies(TPolicyParser& aParser, TUint aOwner=0);
	TInt CreateFlowSpecPolicy(CExtensionPolicy& aPolicy, TUint aOwner);
	TInt CreateModuleSpecPolicy(CExtensionPolicy& aPolicy, TUint aOwner);
	TInt DeletePolicies(TPolicyParser& aParser);
	TPolicyFile* FindPolicyFile(const TDesC& aName);
	
private:
	inline TUint HashKey(TUint aType) { return (aType % KPolicyTableSize); }
	TPolicyQueue iPolicies[KPolicyTableSize];
	TDblQue<TPolicyFile> iFiles;
	};

#endif
