// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef _USSE_QOS_H__
#define _USSE_QOS_H__

#include "usse_server.h"
#include "uscl_qos.h"

class CPacketContext;
//**********************************
// CPacketQoS
//**********************************

class CPacketQoS : public CBase
{
	// construct/destruct
public:
	static CPacketQoS* NewL(CUmtsSimulator* aSimulator, CPacketContext* aContext);
	~CPacketQoS();
private:
	CPacketQoS(CUmtsSimulator* aSimulator, CPacketContext* aContext);
	void ConstructL();

public:
	TInt SetToDefault(); // creates "default" R99_R4 profile
	TInt SetToCopy(const CPacketQoS& aQoS);

	void SetName(const TDesC& aName);
	const TDesC& GetName() const;
	CPacketContext* GetContext() const;

	void IncRefCount() { iRefCount++; }
	void DecRefCount() { iRefCount--; }
	TInt GetRefCount() { return iRefCount; }

public:
	// FROM WRAPPER
	TUint RequestSettingProfileParametersL(TPacketDataConfigBase* aConfig, void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
	void RequestSettingProfileParametersCancel(TUint aRequest);
	TUint RequestGettingProfileParameters(void (*aCallback)(TInt aStatus, const TDesC8& aConfig, TAny* aParam), TAny* aParam);
	void RequestGettingProfileParametersCancel(TUint aRequest);

public:
	// FROM SIMULATOR
	static void RequestSettingProfileParametersReady(TAny* aParam);
	static void RequestGettingProfileParametersReady(TAny* aParam);

	TInt NotifyProfileToWrapper(CPacketQoSApiWrapper* aWrapper);

	void DebugCheck(CUmtsSimulator* aSimulator); // note call order in DebugChecks!

private:
	void DoSetProfileParameters(RPacketQoS::TQoSR5Requested* aRequested);

public:
	TSglQueLink iLink;
private:
	TInt iRefCount;
	TBuf<65> iName;

	CUmtsSimulator* iSimulator;
	CPacketContext* iContext;
	RPacketQoS::TQoSR5Negotiated* iParamsUMTS;
};

#endif
