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

#if !defined(__NIFPDPBASE_H__)
#define __NIFPDPBASE_H__

#include "NifPdpdef.h"
#include <etelpckt.h>
#include <networking/umtsnifcontrolif.h>
#include "testconfigfile.h"

//#define _NIFSIMTSY

class CQoSTestNcp;
class CNifPDPMonitorBase;
class CNifQosMonitorBase;



class CNifPDPContextBase : public CActive
/**
* Base Class for Contexts.
* @internalComponent
* @test
*/
{
public:
	enum TSubCommand
		{
		EInitialise,
		ESetConfig,
		ESetQos,
		EAddTFTFilter,
		ERemoveTFTFilter,
		EActivation,
		EGetQosProfile,
		ELoanCommport,
		EDeletion,
		EDeactivation,
		EModifyActivation,
		ECommandNone
		};

	CNifPDPContextBase(CQoSTestNcp* aNif, TDes8& aConfig);
	virtual ~CNifPDPContextBase();
	TInt					HandleControl(TUint aName, TDes8& aConfig);
	void					EventPrimaryActive();
	void					RunL();	
	// Get functions

	TContextId				ContextId(){return iParameter.iContextInfo.iContextId;};
	TContextType			ContextType(){return iParameter.iContextType;};
	void					ContextPDPType(RPacketContext::TProtocolType& aType)
										{iParameter.iContextConfig.GetPdpType(aType);};
	void					ContextAPNName(RPacketContext::TGSNAddress& aName)
										{iParameter.iContextConfig.GetAccessPointName(aName);};

	/**
	* Returns the Universal time at which the context was activated
	* @return TTime The universal time at which the context was activated
	*/
	TTime					TimeContextActivated(){return iTimeActivated;};
	
	/*TDesC&*/TName					ContextName(){return iContextName;};
	// Primary Context Activation
	virtual void			ActivatePrimaryContext(TDes8& aConfig)=0;

	TInt 					StoreDataSent(const TUint aDataSentSize);
	TInt					StoreDataReceived(const TUint aDataRecvSize);
	TInt					SetDataSentNotificationGranularity(TUint aGranularity);
	TInt					SetDataRecvNotificationGranularity(TUint aGranularity);
	TInt					CancelDataSentNotification();
	TInt					CancelDataReceivedNotification();

	void					NifmanInitiatedDelete() { iNifmanInitiatedDelete = ETrue; }
	void					QoSInitiatedDelete() { iNifmanInitiatedDelete = EFalse; }
	TBool					IsNifmanInitiatedDelete() { return iNifmanInitiatedDelete; }

//protected:
	void					CopyQosReqParameter(TContextParameters& aParameter);

	CQoSTestNcp*				iNif;    // Pointer to NIFNCP
	TUint					iCommand;// Operation Command
	TContextParameters		iParameter;
	TTime					iTimeActivated;
 	#ifdef SYMBIAN_NETWORKING_UMTSR5  
 		RPacketQoS::TQoSR5Negotiated iQosR5Paramter;
 	#else
 		RPacketQoS::TQoSR99_R4Negotiated iQosParamter;
 	#endif 
 	// SYMBIAN_NETWORKING_UMTSR5
	TName					iQosProfileName;
	TName					iContextName;
	CNifPDPMonitorBase*		iContextMonitor;
	CNifQosMonitorBase*		iQosMonitor;	
	TBool					iIsQosOpened;
	TTFTInfo				iModifedTFTInfo;
	TTFTInfo				iTFTInfo;
	TRequestStatus			iReqStatus;
	TInt					iPending;	// we dont want to process two control in one go 
	TUint					iDataSentSize;
	TUint					iDataRecvSize;
	TUint					iDataSentNotificationStartSize;
	TUint					iDataRecvNotificationStartSize;
	TUint					iDataSentGranularity;
	TUint					iDataRecvGranularity;

private:

	// To handle different PDP context operations.
	virtual TInt HandlePDPCreate(TDes8& aConfig)=0;
	virtual TInt HandlePDPDelete(TDes8& aConfig)=0;
	virtual TInt HandleQosSet(TDes8& aConfig)=0;
	virtual TInt HandleContextActivate(TDes8& aConfig)=0;
	virtual TInt HandleModifyActive(TDes8& aConfig)=0;
	virtual TInt HandleTFTModify(TDes8& aConfig)=0;

	// To handle the Pdp Context Operation Completion.
	virtual void PDPCreateComplete()=0;
	virtual void PdpDeleteComplete()=0;
	virtual void PdpActivationComplete()=0;
	virtual void PdpQosSetComplete()=0;
	virtual void PdpModifyActiveComplete()=0;
	virtual void PdpPrimaryComplete()=0;
	virtual void PdpTFTModifyComplete()=0;
	
	void PdpTriggerEvent();

	TBool iNifmanInitiatedDelete;
};

class MMonitor
{
public:
	virtual void StartToMonitor()=0;
};

class CNifPDPMonitorBase : public CActive, public MMonitor
{
public:
	~CNifPDPMonitorBase(){};
protected:
	CNifPDPMonitorBase()
		:CActive(EPriorityStandard){};
	RPacketContext::TContextStatus iContextStatus;

};

class CNifNetworkMonitorBase : public CActive, public MMonitor
{
public:
	~CNifNetworkMonitorBase(){};
protected:
	CNifNetworkMonitorBase(CQoSTestNcp* aLink)
		:CActive(EPriorityStandard), iLink(aLink){};
	RPacketService::TStatus		iNetworkStatus;
	CQoSTestNcp*		iLink;
};

class CNifQosMonitorBase : public CActive, public MMonitor
{
public:
	~CNifQosMonitorBase();
	void DoCancel(){};
	void RunL(){};	
	
protected:
	CNifQosMonitorBase():CActive(EPriorityStandard){};
//	 iQosProfile;

};

#endif




