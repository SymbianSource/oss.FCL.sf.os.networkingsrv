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
// This is part of an ECOM plug-in
// 
//

#define SYMBIAN_NETWORKING_UPS

#include "netmcpr.h"
#include "netmcprstates.h"
#include "netmcpractivities.h"
#include "policyrequest.h"



#include <comms-infras/ss_log.h>
#include <comms-infras/ss_msgintercept.h>
//#include <comms-infras/ss_roles.h>
#include <ss_glob.h>
#include <cs_subconparams.h>
#include <networking/qos3gpp_subconparams.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <tcpdfltrecvwin.h>
// Custom type for WLAN bearer
const TUint32 KNetMcprWlanBearer = 0x3C;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <commsdattypeinfov1_1_internal.h>

#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KNetMCprTag KESockMetaConnectionTag
_LIT8(KNetMCprSubTag, "netmcpr");
#endif

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace MCprActivities;
using namespace CommsDat;

//
// CNetworkMetaConnectionProvider

#ifndef SYMBIAN_NETWORKING_UPS

CNetworkMetaConnectionProvider* CNetworkMetaConnectionProvider::NewL(CMetaConnectionProviderFactoryBase& aFactory, const TProviderInfo& aProviderInfo)
	{
	__CFLOG_VAR((KNetMCprTag, KNetMCprSubTag, _L8("CNetworkMetaConnectionProvider:\tNewL()")));

	CNetworkMetaConnectionProvider* self = new (ELeave) CNetworkMetaConnectionProvider(aFactory,aProviderInfo,NetMCprActivities::netMCprActivities::Self());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

#endif

CNetworkMetaConnectionProvider::CNetworkMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory, const TProviderInfo& aProviderInfo, const MeshMachine::TNodeActivityMap& aActivityMap)
:	CMobilityMetaConnectionProvider(aFactory,aProviderInfo,aActivityMap)
	{
	LOG_NODE_CREATE(KNetMCprTag, CNetworkMetaConnectionProvider);
	}

void CNetworkMetaConnectionProvider::ConstructL()
	{
	CCoreMetaConnectionProvider::ConstructL();
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	RMetaExtensionContainer mec;
	mec.Open(iAccessPointConfig);
	CleanupClosePushL(mec);
	
	//Append pointer to lookup table which holds the various TCP receive window sizes for different bearer types.
	CTCPReceiveWindowSize* receiveWindow;
	receiveWindow = new (ELeave)CDfltTCPReceiveWindowSize();
	CleanupStack::PushL(receiveWindow);
	mec.AppendExtensionL(receiveWindow);
	CleanupStack::Pop(receiveWindow);
	
	//Append the pointer of CSAPSetOpt which provides generic SetOpt( ) implementation
	CSAPSetOpt* protoOption = new (ELeave)CSAPSetOpt();
	CleanupStack::PushL(protoOption);
	mec.AppendExtensionL(protoOption);
	CleanupStack::Pop(protoOption);

	iAccessPointConfig.Close();
	iAccessPointConfig.Open(mec);
	CleanupStack::PopAndDestroy(&mec);
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	}


CNetworkMetaConnectionProvider::~CNetworkMetaConnectionProvider()
	{
	delete iPolicySelectorRecSet;
	delete iDbSession;
	LOG_NODE_DESTROY(KNetMCprTag, CNetworkMetaConnectionProvider);
	}


void CNetworkMetaConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	__CFLOG_VAR((KNetMCprTag, KNetMCprSubTag, _L8("CNetworkMetaConnectionProvider %08x:\tReceivedL() aMessage=%d"),
	   this, aMessage.MessageId().MessageId()));

	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CNetworkMetaConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
    CMobilityMetaConnectionProvider::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

void CNetworkMetaConnectionProvider::ProcessPolicyParamsL(const TRuntimeCtxId& aSender, TCFIPMessage::TPolicyParams& aPolicyParam)
	{
	// this is the best effort call. If db is not there then ignore it
	TRAPD(err, InitDbL());
	if(err == KErrNone)
		{
		RCFParameterFamilyBundleC paramBundle = CreateParameterBundleL(FindMatchingPolicyL(aPolicyParam));

		// create worker node

#ifndef __GCCXML__
		RClientInterface::OpenPostMessageClose(Id(),
			SockManGlobals::Get()->GetPlaneFC(TCFPlayerRole(TCFPlayerRole::ESubConnPlane)),
			TCFConnPolicyRequest(paramBundle, aPolicyParam.iFlowId, aPolicyParam.iSrcNodeId, address_cast<TNodeId>(aSender)));
#endif
		}
	}

void CNetworkMetaConnectionProvider::InitDbL()
	{
	if(iDbSession == NULL)
		{
		iDbSession = CMDBSession::NewL(KCDVersion1_2);
		}

	if(iPolicySelectorRecSet == NULL)
		{
	    iPolicySelectorRecSet = new (ELeave) CMDBRecordSet<CCDPolicySelectorRecord>(KCDTIdPolicySelectorRecord);
		iPolicySelectorRecSet->LoadL(*iDbSession);

		if(iPolicySelectorRecSet->iRecords.Count() == 0)
			{
			User::Leave(KErrNotFound);
			}
		}
	else
		{
		iPolicySelectorRecSet->RefreshL(*iDbSession);

		if(iPolicySelectorRecSet->iRecords.Count() == 0)
			{
			User::Leave(KErrNotFound);
			}
		}
	}

TInt CNetworkMetaConnectionProvider::FindMatchingPolicyL(TCFIPMessage::TPolicyParams& aPolicyParam)
	{
	TBool found(EFalse);
	TUint count(iPolicySelectorRecSet->iRecords.Count());

	CCDPolicySelectorRecord* currentRec(NULL);

	for(TInt i=0;i<count && !found;++i)
		{
		currentRec = static_cast<CCDPolicySelectorRecord*>(iPolicySelectorRecSet->iRecords[i]);

		found = (CheckProtocol(aPolicyParam.iAddrUpdate.iProtocolId, currentRec)
		    && CheckSrcPort(aPolicyParam.iAddrUpdate.iSrcSockAddr.Port(), currentRec)
		    && CheckDstPort(aPolicyParam.iAddrUpdate.iDestSockAddr.Port(), currentRec)
		    && CheckIap(aPolicyParam.iAddrUpdate.iIapId, currentRec)
		    && CheckSrcAddressMatch(aPolicyParam.iAddrUpdate.iSrcSockAddr, currentRec)
		    && CheckDstAddressMatch(aPolicyParam.iAddrUpdate.iDestSockAddr, currentRec)
			&& CheckAppUid(aPolicyParam.iAppSid, currentRec));
		}

	if (!found)
		{
		__CFLOG_VAR((KNetMCprTag, KNetMCprSubTag, _L8("CNetworkMetaConnectionProvider::FindMatchingPolicyL not found")));
		User::Leave(KErrNotFound);
		}
	else
		{
		__CFLOG_VAR((KNetMCprTag, KNetMCprSubTag, _L8("CNetworkMetaConnectionProvider::FindMatchingPolicyL found")));
		return currentRec->iPolicyId;
		}

	// never executed
	return 0;
	}

ESock::RCFParameterFamilyBundleC CNetworkMetaConnectionProvider::CreateParameterBundleL(TUint aPolicyId)
	{
	CMDBRecordSet<CCDPolicySelector2ParamsRecord>* iRecSet = new(ELeave) CMDBRecordSet<CCDPolicySelector2ParamsRecord>(KCDTIdPolicySelector2ParamsRecord);
	CleanupStack::PushL(iRecSet);

	// create new record
	CCDPolicySelector2ParamsRecord* iRec = static_cast<CCDPolicySelector2ParamsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdPolicySelector2ParamsRecord));

	// Prime record
	iRec->iPolicySelectorId = aPolicyId;
	// Append primed record to recordset
	TInt err = iRecSet->iRecords.Append(iRec);
	if(err != KErrNone)
		{
		delete iRec;
		User::Leave(err);
		}

	TBool searchResult = iRecSet->FindL(*iDbSession);
	if(!searchResult)
		{
		// Params not found
		User::Leave(KErrNotFound);
		}

	TUint count = iRecSet->iRecords.Count();
	err = KErrNotFound;

	RCFParameterFamilyBundle tempBundle;
	RCFParameterFamilyBundleC paramBundle;
	if(count>0)
		{
		paramBundle.CreateL();
		CleanupClosePushL(paramBundle);
		tempBundle.CreateL();
		CleanupClosePushL(tempBundle);


		// create family
		paramBundle.Open(tempBundle);
		RParameterFamily family = tempBundle.CreateFamilyL(KSubConQoSFamily);
		CleanupStack::Pop(/*tempBundle*/);
		
		//[401TODO]: Replace with fency code with uid's */
		for (TUint i=0; i<count ; ++i)
			{
			// check if at least one param is filled in
			if(FillInParamsL(static_cast<CCDPolicySelector2ParamsRecord*>(iRecSet->iRecords[i])->iParamsId, family) == KErrNone)
				{
				// found at least one param, it is not an error any more
				err = KErrNone;
				}
			}
		}

	if(err != KErrNone)
		{
		CleanupStack::PopAndDestroy(/*paramBundle*/);
		}
	else
		{
		CleanupStack::Pop(/*paramBundle*/);
		}

	CleanupStack::PopAndDestroy(iRecSet);

	return paramBundle;
	}

TInt CNetworkMetaConnectionProvider::FillInParamsL(TUint aParamId, RParameterFamily& aFamily)
	{
	TInt err(KErrGeneral);

	switch(aParamId & KCDMaskShowRecordType)
		{
		case KCDTIdGenericQosRecord:
			err = FillInGenericQosParamsL(aParamId, aFamily);
				break;
		case KCDTIdUmtsR99QoSAndOnTableRecord:
			err = FillInUMTSParamsL(aParamId, aFamily);
				break;
		default: ;
		}

	return err;
	}

TInt CNetworkMetaConnectionProvider::FillInGenericQosParamsL(TUint aParamId, RParameterFamily& aFamily)
	{
	CCDGenericQosRecord *rec = static_cast<CCDGenericQosRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdGenericQosRecord));
	CleanupStack::PushL(rec);

	rec->SetRecordId(aParamId & ~KCDMaskShowRecordType);
	TRAPD(err, rec->LoadL(*iDbSession);)
	if(err == KErrNone)
		{
	    CSubConQosGenericParamSet* param = CSubConQosGenericParamSet::NewL(aFamily,RParameterFamily::ERequested);

		param->SetHeaderMode(rec->iHeaderMode);
		param->SetDownlinkBandwidth(rec->iDownlinkBandwidth);
		param->SetUplinkBandwidth(rec->iUplinkBandwidth);
		param->SetDownLinkMaximumBurstSize(rec->iDownLinkMaximumBurstSize);
		param->SetUpLinkMaximumBurstSize(rec->iUpLinkMaximumBurstSize);
		param->SetDownLinkAveragePacketSize(rec->iDownLinkAveragePacketSize);
		param->SetUpLinkAveragePacketSize(rec->iUpLinkAveragePacketSize);
		param->SetDownLinkMaximumPacketSize(rec->iDownLinkMaximumPacketSize);
		param->SetUpLinkMaximumPacketSize(rec->iUpLinkMaximumPacketSize);
		param->SetDownLinkDelay(rec->iDownLinkDelay);
		param->SetUpLinkDelay(rec->iUpLinkDelay);
		param->SetDownLinkDelayVariation(rec->iDownLinkDelayVariation);
		param->SetUpLinkDelayVariation(rec->iUpLinkDelayVariation);
		param->SetDownLinkPriority(rec->iDownLinkPriority);
		param->SetUpLinkPriority(rec->iUpLinkPriority);
		}

	CleanupStack::PopAndDestroy(rec);
	return err;
	}

TInt CNetworkMetaConnectionProvider::FillInUMTSParamsL(TUint aParamId, RParameterFamily& aFamily)
	{
	CCDUmtsR99QoSAndOnTableRecord *rec = static_cast<CCDUmtsR99QoSAndOnTableRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdUmtsR99QoSAndOnTableRecord));
	CleanupStack::PushL(rec);

	rec->SetRecordId(aParamId & ~KCDMaskShowRecordType);
	TRAPD(err, rec->LoadL(*iDbSession);)
	if(err == KErrNone)
		{
	    //Fill up the structures
	    CSubConQosR5ParamSet* param = CSubConQosR5ParamSet::NewL(aFamily,RParameterFamily::ERequested);

	    param->SetTrafficClass(rec->iGPRSReqTrafficClass);
		param->SetDeliveryOrder(rec->iGPRSReqDeliveryOrder);
		param->SetErroneousSDUDelivery(rec->iGPRSReqDeliverErroneousSDU);
		param->SetResidualBitErrorRatio(rec->iGPRSReqBER);
		param->SetSDUErrorRatio(rec->iGPRSReqSDUErrorRatio);
		param->SetTrafficHandlingPriority(rec->iGPRSReqTrafficHandlingPriority);
		param->SetTransferDelay(rec->iGPRSReqTransferDelay);
		param->SetMaxSduSize(rec->iGPRSReqMaxSDUSize);
		param->SetMaxBitrateUplink(rec->iGPRSReqMaxUplinkRate);
		param->SetMaxBitrateDownlink(rec->iGPRSReqMaxDownlinkRate);
		param->SetGuaBitrateUplink(rec->iGPRSReqGuaranteedUplinkRate);
		param->SetGuaBitrateDownlink(rec->iGPRSReqGuaranteedDownlinkRate);
		param->SetSignallingIndicator(rec->iGPRSSignallingIndication);
		param->SetSourceStatisticsDescriptor(rec->iGPRSSourceStatisticsDescriptor);
		}

	CleanupStack::PopAndDestroy(rec);
	return err;
	}

TBool CNetworkMetaConnectionProvider::CheckSrcAddressMatch(const TSockAddr& aFirst, CCDPolicySelectorRecord* aRecord)
	{
	if(aRecord && (!(aRecord->iSrcAddress.IsNull() || aRecord->iSrcMask.IsNull())))
		{
		TInetAddr first(aFirst);

		TInetAddr second;
		second.Input(aRecord->iSrcAddress);

		TInetAddr mask;
		mask.Input(aRecord->iSrcMask);

		return first.Match(second, mask);
		}

	return ETrue;
	}

TBool CNetworkMetaConnectionProvider::CheckDstAddressMatch(const TSockAddr& aFirst, CCDPolicySelectorRecord* aRecord)
	{
	if(aRecord && (!(aRecord->iDstAddress.IsNull() || aRecord->iDstMask.IsNull())))
		{
		TInetAddr first(aFirst);

		TInetAddr second;
		second.Input(aRecord->iDstAddress);

		TInetAddr mask;
		mask.Input(aRecord->iDstMask);

		return first.Match(second, mask);
		}

	return ETrue;
	}

TBool CNetworkMetaConnectionProvider::CheckProtocol(TUint aProtocolId, CCDPolicySelectorRecord* aRecord)
	{
	// compare protocolId
	if(aRecord && !aRecord->iProtocolId.IsNull())
		{
		return aProtocolId == aRecord->iProtocolId;
		}

	return ETrue;
	}


TBool CNetworkMetaConnectionProvider::CheckSrcPort(TUint aPort, CCDPolicySelectorRecord* aRecord)
	{
	// match src port
	if(aRecord && (!(aRecord->iSrcPort.IsNull() || aRecord->iSrcPortMax.IsNull())))
		{
		return (aPort >= aRecord->iSrcPort)  && (aPort <= aRecord->iSrcPortMax);
		}

	return ETrue;
	}

TBool CNetworkMetaConnectionProvider::CheckDstPort(TUint aPort, CCDPolicySelectorRecord* aRecord)
	{
	// match src port
	if(aRecord && (!(aRecord->iDstPort.IsNull() || aRecord->iDstPortMax.IsNull())))
		{
		return (aPort >= aRecord->iDstPort)  && (aPort <= aRecord->iDstPortMax);
		}

	return ETrue;
	}

TBool CNetworkMetaConnectionProvider::CheckIap(TUint aIapId, CCDPolicySelectorRecord* aRecord)
	{
	// check IAP
	if(aRecord && !aRecord->iIapId.IsNull())
		{
		return aIapId == aRecord->iIapId;
		}

	return ETrue;
	}

TBool CNetworkMetaConnectionProvider::CheckAppUid(TUid aAppUid, CCDPolicySelectorRecord* aRecord)
	{
	// check AppUid
	if(aRecord && !aRecord->iAppUid.IsNull())
		{
		return aAppUid == aRecord->iAppUid;
		}

	return ETrue;
	}

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

CDfltTCPReceiveWindowSize::CDfltTCPReceiveWindowSize()
                          :CTCPReceiveWindowSize() 	
                      
/**
 *  Populate TCP window lookup table.
 */
	{
	Init();
	}

void CDfltTCPReceiveWindowSize::Init()
/**
 * Initialize TCP receive window lookup table
 */
	{  		
	//TCP receive window size for GPRS
	iBearerInfoMap.Insert(KGprsBearer,KBearerGprsWinSize);

	//TCP receive window size for EGPRS
	iBearerInfoMap.Insert(KEGprsBearer,KBearerEdgeWinSize);

	//TCP receive window size for UMTS
	iBearerInfoMap.Insert(KUmtsBearer,KBearerUmtsWinSize);

	//TCP receive window size for HSDPA
	iBearerInfoMap.Insert(KHsdpaBearer,KBearerHsdpaWinSize);
	
   	//TCP receive window size for ethernet
   	iBearerInfoMap.Insert(KEthernetBearer,KBearerEthernetWinSize);

    //TCP receive window size for WLAN bearer
    iBearerInfoMap.Insert(KNetMcprWlanBearer,KBearerWlanWinSize);
   	
   	//TCP receive window size for other bearer
   	iBearerInfoMap.Insert(KDefaultBearer,KBearerDefaultWinSize);
	}

void CDfltTCPReceiveWindowSize::SetTcpWin(TUint aBearerType)
/**
 * Set TCP receive window 
 */
	{
    // Get bearer window size from hash table
    TUint* iWinSizePtr = static_cast<TUint*>(iBearerInfoMap.Find(aBearerType));

    // Check whether bearer type was known 
    if ( iWinSizePtr != NULL )
        {
        // Set the TCP Receive Window
        iWinSize = *iWinSizePtr;
        }
    else
        {
        // Use default window
        iWinSize = KBearerDefaultWinSize;
        }
	  
	//Set the Max TCP receive Window.
	SetMaxWinSize(aBearerType);
	}
	

void CDfltTCPReceiveWindowSize::SetMaxWinSize(TUint aBearerType)
/**
 * Set Max TCP receive Window.
 * @Param Bearer type
 */
	{
	switch(aBearerType)
		{
		case KGprsBearer:
		case KEGprsBearer:
		case KUmtsBearer:
		case KHsdpaBearer:
		//
		// TCP receive window size will be maximum for HSDPA bearers.
		//
			iMaxWinSize = KBearerHsdpaWinSize; 
			break; 
		case KNetMcprWlanBearer:
			iMaxWinSize = KBearerWlanWinSize;
			break;
   
        case KEthernetBearer:
            iMaxWinSize = KEthernetMaxWinSize;
            break;
			
		default:
			iMaxWinSize = KBearerDefaultMaxWinSize;
			break;
		}
	}


CDfltTCPReceiveWindowSize::~CDfltTCPReceiveWindowSize()
/**
 * Close TCP receive window lookup table
 */
	{
	iBearerInfoMap.Close();
	}
	
/**
 * Register the variables of TCP receive window class,
 * since it derives from SMetaData class
 */
START_ATTRIBUTE_TABLE(CTCPReceiveWindowSize,CTCPReceiveWindowSize::ERealmId, CTCPReceiveWindowSize::iId)
  REGISTER_ATTRIBUTE(CTCPReceiveWindowSize, iMaxWinSize, TMetaNumber)
  REGISTER_ATTRIBUTE(CTCPReceiveWindowSize, iWinSize, TMetaNumber)
END_ATTRIBUTE_TABLE()

#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
