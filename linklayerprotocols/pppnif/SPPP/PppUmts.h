// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @internalComponent 
*/

#ifndef __PPPUMTS_H__
#define __PPPUMTS_H__
#include "PPPBASE.H"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#endif

const TInt KMaxLinkInterfaceName = 20;

/****************/
/* CProtocolPpp */
/****************/

class CPppUmtsLink;
NONSHARABLE_CLASS(CProtocolPpp) : public CProtocolBase
/**
 * Class used by PPP to receive data from lower layer Nif and send it up to LCP.
 */
	{
public:
	static CProtocolPpp* NewL();
	CProtocolPpp();
	~CProtocolPpp();

	// CProtocolBase mirrored functions to allow passing of data from lower layer Nif up to LCP
	inline virtual void Process(RMBufChain &,CProtocolBase* aSourceProtocol=NULL);	// Up
	inline virtual void Process(TDes8& aPDU,TSockAddr *from,TSockAddr *to=NULL,CProtocolBase* aSourceProtocol=NULL);	// Up
	inline virtual void StartSending(CProtocolBase* aProtocol);	// Up
	inline virtual void Identify(TServerProtocolDesc* aProtocolDesc) const;

	void SetValue(CPppUmtsLink* aPppUmtsLink);
private:
	CPppUmtsLink* iPppUmtsLink;
	};

/****************/
/* CPppUmtsLink */
/****************/

NONSHARABLE_CLASS(CPppUmtsLink) : public CPppLinkBase, public MNifIfNotify
/**
 * Definition of the PPP "proxy" class which acts as a bridge between PPP and the lower layer Nif.
 */
	{
public:
	CPppUmtsLink(CPppLcp* aLcp, const TDesC& aParams);
	~CPppUmtsLink();

	// CProtocolBase API to lower layer Nif
	inline void Process(RMBufChain &,CProtocolBase* aSourceProtocol=NULL);	// Up
	inline void Process(TDes8& aPDU,TSockAddr *from,TSockAddr *to=NULL,CProtocolBase* aSourceProtocol=NULL);	// Up
	inline void StartSending(CProtocolBase* aProtocol);	// Up

	// CPppLinkBase API to lower layer Nif
	virtual void CreateL();
	inline virtual TInt Send(RMBufChain& aPacket, TUint aPppId=KPppIdAsIs);
	inline virtual void OpenL();
	inline virtual void Close();
	inline virtual void StartL();
	virtual void Stop(TInt aReason, TBool aLinkDown=ETrue);
	inline virtual void GetSendRecvSize(TInt& aMaxRecvSize, TInt& aMaxSendSize);
	virtual TInt SpeedMetric();
	inline virtual void GetDataTransfer(RPacketContext::TDataVolume&);

	// MNifIfNotify API to lower layer Nif
    inline virtual void LinkLayerDown(TInt aReason, TAction aAction);		
	inline virtual void LinkLayerUp();
    inline virtual void NegotiationFailed(CNifIfBase* aIf, TInt aReason);
    inline virtual TInt Authenticate(TDes& aUsername, TDes& aPassword);
    inline virtual void CancelAuthenticate();
	inline virtual TInt GetExcessData(TDes8& aBuffer);
	inline virtual void IfProgress(TInt aStage, TInt aError);
	inline virtual void IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);

	inline virtual void OpenRoute();
	inline virtual void CloseRoute();
	inline virtual TInt Notification(TNifToAgentEventType aEvent, void * aInfo = NULL);
	virtual void BinderLayerDown(CNifIfBase* aBinderIf, TInt aReason, TAction aAction);
	virtual TInt PacketActivity(TDataTransferDirection aDirection, TUint aBytes, TBool aResetTimer);

	inline virtual void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume);
	inline virtual void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume);
	inline virtual void NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);


protected:
	inline virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage);
    inline virtual TInt DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage);
    inline virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage);
    inline virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage);
    inline virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage);
	inline virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage);
	inline virtual TInt DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage);
    inline virtual TInt DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage);


private:
	CNifIfLink* iUmtsNif;
	CNifIfBase* iUmtsNifPppBinder;
	CProtocolPpp* iProtocolPpp;
	TBuf<KMaxLinkInterfaceName> iNifName;		// Lower layer Nif name
	};

#endif
