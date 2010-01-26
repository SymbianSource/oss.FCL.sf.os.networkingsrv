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
//

#if !defined(__NIFPDPTSY_H__)
#define __NIFPDPTSY_H__

#include "NifPdpBase.h"

class MPacketTsyObserver
{
public:
	virtual RPacketContext&			PacketContext()=0;
	virtual RPacketQoS&				PacketQoS()=0;
	virtual TContextParameters&		ContextParameters()=0;
	virtual CQoSTestNcp*				NifNcp()=0;
};
class CNifPDPContextTsy : public CNifPDPContextBase, public MPacketTsyObserver
{
public:
	static CNifPDPContextTsy* NewL(CQoSTestNcp* aNif, TDes8& aConfig);
	static CNifPDPContextTsy* NewLC(CQoSTestNcp* aNif, TDes8& aConfig);
	CNifPDPContextTsy(CQoSTestNcp* aNif, TDes8& aConfig);
	~CNifPDPContextTsy();
	void ConstructL();
	void DoCancel();
	virtual void ActivatePrimaryContext(TDes8& aConfig);

	//MPacketTsyObserver methods
	virtual RPacketContext&			PacketContext(){return iContext;};
	virtual RPacketQoS&				PacketQoS(){return iQos;};
	virtual TContextParameters&		ContextParameters(){return iParameter;};
	virtual CQoSTestNcp*				NifNcp(){return iNif;};


//	RPacketContext		iContext;


private:
	// To handle different PDP context operations.
	virtual TInt HandlePDPCreate(TDes8& aConfig);
	virtual TInt HandlePDPDelete(TDes8& aConfig);
	virtual TInt HandleQosSet(TDes8& aConfig);
	virtual TInt HandleContextActivate(TDes8& aConfig);
	virtual TInt HandleModifyActive(TDes8& aConfig);
	virtual TInt HandleTFTModify(TDes8& aConfig);

	// To handle the Pdp Context Operation Completion.
	virtual void PDPCreateComplete();
	virtual void PdpDeleteComplete();
	virtual void PdpActivationComplete();
	virtual void PdpQosSetComplete();
	virtual void PdpModifyActiveComplete();
	virtual void PdpPrimaryComplete();
	virtual void PdpTFTModifyComplete();

	TSubCommand			iSubCommand;
	RPacketContext		iContext;
	RPacketQoS			iQos;
	RPacketContext::TPacketFilterV2 iCurrentfilter;

	RPacketContext::TDataChannelV2  iDataChannel;
	RPacketContext::TDataChannelV2Pckg *iDataChannelPckg;
	
	RPacketContext::TContextConfigGPRS iRel99ContextConfig;
	TPckg<RPacketContext::TContextConfigGPRS>* iRel99ContextConfigPckg;

#ifdef SYMBIAN_NETWORKING_UMTSR5  
	RPacketQoS::TQoSR5Requested iReqR5Qos;
	TPckg<RPacketQoS::TQoSR5Requested>* iReqR5QosPckg;

	RPacketQoS::TQoSR5Negotiated iNegR5Qos;
	TPckg<RPacketQoS::TQoSR5Negotiated>* iNegR5QosPckg;
#else
	RPacketQoS::TQoSR99_R4Requested iReqQos;
	TPckg<RPacketQoS::TQoSR99_R4Requested>* iReqQosPckg;

	RPacketQoS::TQoSR99_R4Negotiated iNegQos;
	TPckg<RPacketQoS::TQoSR99_R4Negotiated>* iNegQosPckg;   
#endif 
// SYMBIAN_NETWORKING_UMTSR5 

	RPacketContext::TPacketFilterV2 iFilter;
	TPckg<RPacketContext::TPacketFilterV2>*  iFilterPckg;


};

class CNifPDPMonitorTsy : public CNifPDPMonitorBase
{
public:
	static CNifPDPMonitorTsy* NewL(MPacketTsyObserver* aObserver);
	static CNifPDPMonitorTsy* NewLC(MPacketTsyObserver* aObserver);
	CNifPDPMonitorTsy(MPacketTsyObserver* aObserver);
	~CNifPDPMonitorTsy(){};
	void DoCancel();
	void RunL();	
	void StartToMonitor();
	void ConstructL();
private:
	MPacketTsyObserver* iObserver;
};

class CNifNetworkMonitorTsy : public CNifNetworkMonitorBase
{
public:
	static CNifNetworkMonitorTsy* NewL(CQoSTestNcp* aLink);
	static CNifNetworkMonitorTsy* NewLC(CQoSTestNcp* aLink);
	CNifNetworkMonitorTsy(CQoSTestNcp* aLink);
	~CNifNetworkMonitorTsy(){};
	void DoCancel();
	void RunL();	
	void StartToMonitor();
	void ConstructL();
};

class CNifQosMonitorTsy : public CNifQosMonitorBase
{
public:
	static CNifQosMonitorTsy* NewL(MPacketTsyObserver* aObserver);
	static CNifQosMonitorTsy* NewLC(MPacketTsyObserver* aObserver);
	CNifQosMonitorTsy(MPacketTsyObserver* aObserver);
	~CNifQosMonitorTsy(){}; 
	void DoCancel();
	void RunL();	
	void StartToMonitor();
	void ConstructL();
private:
	MPacketTsyObserver* iObserver;
	
#ifdef SYMBIAN_NETWORKING_UMTSR5  
	RPacketQoS::TQoSR5Negotiated iNegR5QosMonitor;
	TPckg<RPacketQoS::TQoSR5Negotiated>* iNegR5QosPckgMonitor;
#else
	RPacketQoS::TQoSR99_R4Negotiated iNegQosMonitor;
	TPckg<RPacketQoS::TQoSR99_R4Negotiated>* iNegQosPckgMonitor;
#endif 
// SYMBIAN_NETWORKING_UMTSR5 
};

#endif




