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

#ifndef _USSE_QOS_WRAPPER_H__
#define _USSE_QOS_WRAPPER_H__

#include "usse_qos.h"
#include "us_cliserv.h"

class CPacketContext;
// *************************
// CPacketQoSApiWrapper
// for message delivery & wrapping
// *************************

class CPacketQoSApiWrapper : public CObject
{
public:
    static CPacketQoSApiWrapper* NewL(CUmtsSimServSession* aSession);
	~CPacketQoSApiWrapper();
    void CloseWrapper();

private:
	CPacketQoSApiWrapper(CUmtsSimServSession* aSession);
    void ConstructL(TUmtsSimServQoSOpenMode aMode, const TAny* aName, const TAny* aContext);

public:
    const RMessage2& Message() const;

public:
	// FROM ETEL PACKET DATA API / PACKET QOS
	// NOTE: Some of these methods may leave!
	void SetProfileParametersA();
	void SetProfileParametersACancel();
	void GetProfileParametersA();
	void GetProfileParametersACancel();

	void NotifyProfileChangedA();
	void NotifyProfileChangedACancel();

public:
	// FROM SIMULATOR
	static void SimSetProfileParametersAReady(TInt aStatus, TAny* aSelf);
	static void SimGetProfileParametersAReady(TInt aStatus, const TDesC8& aParameters, TAny* aSelf);

	void SimProfileChanged(CPacketContext* aContext, const TDesC8& aParameters);
	void SimContextDeleted(CPacketContext* aContext);

	// note call order in DebugChecks!
	void DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession);

private:
	void LogWithName(const TDesC& aMsg);
	TInt TrappedMethodL(RMessage2& aMsg, TInt aLength);

private:
	TBool iSetProfileParametersA;
	TUint iSetProfileParametersAReq;
	RMessage2 iSetProfileParametersAMessage;
	TBool iGetProfileParametersA;
	TUint iGetProfileParametersAReq;
	RMessage2 iGetProfileParametersAMessage;

	TBool iNotifyProfileChangedA;
	RMessage2 iNotifyProfileChangedAMessage;

protected:
    CUmtsSimServSession* iSession; //session owning us
	CUmtsSimulator* iSimulator;    //umts simulator state machine
	CPacketQoS* iQoS;			   //our qos
};

#endif
