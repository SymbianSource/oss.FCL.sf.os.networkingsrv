// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation of packet driver interface for the ethernet packet driver
// 
//

/**
 @file 
 @internalComponent 
*/

#if !defined(__CARDDRV_H__)
#define __CARDDRV_H__

#include "eth_log.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#endif


const TInt KEtherBufSize = 1540;
const TUint KPcCardDrvMajorVersionNumber=1;
const TUint KPcCardDrvMinorVersionNumber=0;
const TUint KPcCardDrvBuildVersionNumber=1;

/**
@internalComponent
*/
NONSHARABLE_CLASS(CPcCardPktDrvFactory) : public CPktDrvFactory
{
public:
	CPcCardPktDrvFactory();
	virtual CPktDrvBase* NewDriverL(CLANLinkCommon* aParent);
	virtual TVersion Version() const;
};

class CPcCardControlEngine;
/**
@internalComponent
*/
NONSHARABLE_CLASS(CPcCardPktDrv) : public CPktDrvBase
{
public: 
	CPcCardPktDrv(CPktDrvFactory& aFactory);
	virtual ~CPcCardPktDrv();
	virtual void ConstructL(CLANLinkCommon* aParent);
	virtual TInt StartInterface();
	virtual TInt StopInterface();
	virtual TInt ResetInterface();
	virtual TInt SetRxMode(TRxMode AMode); 
	virtual TInt GetRxMode() const;
	virtual TInt AccessType();
	virtual TInt ReleaseType();
	virtual TInt SetInterfaceAddress(const THWAddr&);
	virtual TUint8* GetInterfaceAddress()const;
	virtual TInt GetMulticastList(const THWAddr* aAddr, TInt& n) const;
	virtual TInt SetMulticastList(const THWAddr* aAddr, TInt n);
	virtual TInt InterfacePowerUp();
	virtual TInt InterfacePowerDown();
	virtual TInt InterfaceSleep();
	virtual TInt InterfaceResume();
	virtual TInt Notification(enum TAgentToNifEventType aEvent, void* aInfo);
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource=0);

	// Upcall from Control Object
	void ReadDataAvailable(TDesC8& aBuffer);
	void ResumeSending();
	CLANLinkCommon* NifNotify();

	virtual TInt Send(RMBufChain& aPkt); //< CPktDrvBase
	void LinkLayerUp();

private: 
	CPcCardControlEngine* iControl;
#ifdef __LOGDEB__
	RFs iFs;
#endif

};

#endif
