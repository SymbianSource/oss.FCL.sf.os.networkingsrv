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

#if !defined(QOSTESTNCP_H)
#define QOSTESTNCP_H

#include <f32file.h>
#include "QoSTestEventBase.h"
#include "testconfigfile.h"
#include "NifPdpBase.h"


class CQoSTestNcp : public CQoSTestEventBase
	{
public:

    //-=========================================================
    // MLowerControl methods
    //-=========================================================	
	virtual TInt GetName(TDes& aName);
	virtual TInt BlockFlow(TBlockOption aOption);
	virtual TInt GetConfig(TBinderConfig& aConfig);
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);
	
    static CQoSTestNcp* NewL(CQosTestLcp* aLcp, MLowerControl& alowerControl, CPppLcp::TPppProtocol aProtocolType);
	    
	~CQoSTestNcp();
	TInt				StartPrimaryContext();
	void				PrimaryContextReady();
	void				PppNegotiationComplete();

	RTelServer&			TelServer(){return iTelServer;};
	RPhone&				Phone(){return iPhone;};
	RPacketService&		PacketService(){return iPacketNetwork;};
	RCall::TCommPort&	CommPort(){return iCommport;};
	CTestConfig*		CfgFile(){return iConfigFile;};

	TContextId			ContextId(){iNextId++;return iNextId;};
	TUint8				FindAvailId(void);
	void				FreeId(TInt contextId);
	
	void				RemovePDPContext(CNifPDPContextBase* aContext);
	void				AddPDPContext(CNifPDPContextBase* aContext);
	CNifPDPContextBase* LookForPDPContext(TContextId aNextId);
	CNifPDPContextBase* PrimaryPDPContext();
	CNifPDPContextBase* FirstExistingPDPContext();
	
	//Extensions to support CPppLcp's inheritance of MNifAgentExtendedManagementInterface

	TUint				EnumerateSubConnections();
	TInt				GetSubConnectionInfo(const TUint aIndex, TSubConnectionInfo& aSubConnectionInfo);
	TInt				GetSubConnectionInfo(TSubConnectionInfo& aSubConnectionInfo);
	TInt				LookForIndex(const TSubConnectionUniqueId aSubConnectionUniqueId);

	//Extensions to support CPppLcp's inheritance of MNifIfExtendedManagementInterface
	void				StoreDataSent(const TUint aPortNo, const TUint aDataSentSize);
	void				StoreDataReceived(RMBufChain& aPacket);
	TInt				GetDataTransferred(TSubConnectionUniqueId aSubConnectionUniqueId, TUint& aSentBytes, TUint& aReceivedBytes);
	TInt				SetDataSentNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aGranularity);
	TInt				CancelDataSentNotification(TSubConnectionUniqueId aSubConnectionUniqueId);
	TInt				SetDataReceivedNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aGranularity);
	TInt				CancelDataReceivedNotification(TSubConnectionUniqueId aSubConnectionUniqueId);

	CNifPDPContextBase* FindContext(const TContextId aContextId);
	CNifPDPContextBase* MatchContext(const RMBufChain& aPacket);

	// Utility functions called from CQoSTestLcp
	
	void SetUpperControl(const ESock::MUpperControl* aControl);
	TBool MatchesUpperControl(const ESock::MUpperControl* aControl) const;

protected:
	CQoSTestNcp(CQosTestLcp* aLcp, MLowerControl& alowerControl, CPppLcp::TPppProtocol aProtocolType);
	void                ConstructL();
	
#ifdef _NIFSIMTSY	
	void				InitETelL();
	void				RecoverETel();
#endif

	void				GetPhoneInfoL(const TDesC& aLoadedTsyName, RTelServer::TPhoneInfo& aInfo);
	TInt8								iNextId;	// Next Context ID Assigned by Nif	
	CArrayFixFlat<TBool>*				iNextAvailableId;  // array to hold all available context 
	CArrayFixFlat<CNifPDPContextBase*>* iContexts;	// Multiple PDP context queue.
	RTelServer							iTelServer;	
	RPhone								iPhone;			
	RPacketService						iPacketNetwork;
	RFs									iFs;	
	CTestConfig*						iConfigFile;	
	TBool								iIsTsyLoaded;
	TBool								iIsPrimaryContextReady;
	TBool								iIsPPPReady;
	TPckg<TContextParameters>   		iDefaultPrimary;// Default Configuration & Qos Parameter for Primary Pdp Context.
 	TContextParameters					iDefaultPrimaryObj;
	RCall::TCommPort					iCommport;
	CNifNetworkMonitorBase*				iNetworkMonitor;
	CQosTestLcp*						iLcp; // pointer to link layer to notify when last subconn goes down
	ESock::MLowerControl&				iLowerControl;
	CPppLcp::TPppProtocol               iProtocolType;
	const ESock::MUpperControl*			iUpperControl;
	};
	
#endif
//QOSTESTNCP_H
