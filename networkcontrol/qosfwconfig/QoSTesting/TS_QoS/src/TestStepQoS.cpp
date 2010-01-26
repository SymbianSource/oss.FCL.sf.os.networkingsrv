// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

// EPOC includes
#include <e32base.h>

// Test system includes
#include "Log.h"
#include "TestStep.h"
#include "TestStepQoS.h"
#include "TestSuiteQoS.h"
#include "TestIf.h"

#include <CommDbConnPref.h>


CTestStepQoS::CTestStepQoS() 
    : iReceivedEvent(0)
    {
    ClearTestFiles();
    iParameters = new(ELeave)CQoSParameters();
    }

// destructor
CTestStepQoS::~CTestStepQoS()
    {
    if(iParameters)
        {
        delete iParameters;
        }
    }

void CTestStepQoS::WaitForQoSEventL()
    {
    if(iListener)
        {
        delete iListener;
        iListener = 0;
        }

    iListener = CQoSListener::NewL();
    iListener->Start();
    iReceivedEvent = KErrTimedOut;
    iReason = KErrNone;
    CActiveScheduler::Start();

    if(iListener)
        {
        delete iListener;
        iListener = 0;
        }
    }

void CTestStepQoS::WaitForSubConnEventL()
    {
    iEventListener->Start();
    CActiveScheduler::Start();
    }

void CTestStepQoS::Event(const CQoSEventBase& aEvent)
    {
    // stop the scheduler right after receiving event. Even though scheduler
    // would stop automatically in time of next RunL cycle it is possible
    // that some other event is received before that.
    if(iListener)
        {
        iListener->Cancel();
        CActiveScheduler::Stop();
        delete iListener;
        iListener = 0;
        }

    iReceivedEvent = aEvent.EventType();
    TPtrC errorTxt = QoSEventToText(iReceivedEvent);
    TInt err = KErrNone;
    _LIT(KText1, "Event: %S  Reason: %d");

    switch(aEvent.EventType())
        {
        case EQoSEventFailure:
            {
            TRAP(err, iParameters->CopyL(*((CQoSFailureEvent*)
                                         &aEvent)->Parameters()));
            iReason = ((CQoSFailureEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventConfirm:
            {
            TRAP(err, iParameters->CopyL(*((CQoSConfirmEvent*)
                                         &aEvent)->Parameters()));
            _LIT(KText2, "Event: %S");
            Log( KText2, &errorTxt);
            break;
            }
        
        case EQoSEventAdapt:
            {
            TRAP(err, iParameters->CopyL(*((CQoSAdaptEvent*)
                                         &aEvent)->Parameters()));
            iReason = ((CQoSAdaptEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventJoin:
            {
            iReason = ((CQoSJoinEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventLeave:
            iReason = ((CQoSLeaveEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
        
        case EQoSEventAddPolicy:
            {
            TRAP(err, 
                 iParameters->CopyL(*((CQoSAddEvent*)&aEvent)->Parameters()));
            iReason = ((CQoSAddEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventGetPolicy:
            {
            iReason = ((CQoSGetEvent*)&aEvent)->Reason();
            if(iReason != KErrNotFound)
                {
                TRAP(err, iParameters->CopyL(*((CQoSGetEvent*)
                                             &aEvent)->Parameters()));
                }
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventDeletePolicy:
            {
            iReason = ((CQoSDeleteEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventLoadPolicyFile:
            {
            iReason = ((CQoSLoadEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventUnloadPolicyFile:
            {
            iReason = ((CQoSLoadEvent*)&aEvent)->Reason();
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        case EQoSEventChannel:
            {
            iReason = ((CQoSChannelEvent*)&aEvent)->Reason();
            if(iReason != KErrNotFound)
                {
                TRAP(err, iParameters->CopyL(*((CQoSChannelEvent*)
                                             &aEvent)->Parameters()));
                }
            Log( KText1, &errorTxt, iReason);
            break;
            }
        
        default:
            {    
            // unknown event type
            _LIT(KText3, "Unknown event!!! Event: %d");
            Log( KText3, iReceivedEvent);
        
            break;
            }
        }
    }
 

TPtrC CTestStepQoS::QoSEventToText(TInt aEvent)
    {
    switch(aEvent)
        {
        case EQoSEventFailure:
            {
            return _L("EQoSEventFailure");
            }
        case EQoSEventConfirm:
            {
            return _L("EQoSEventConfirm");
            }
        case EQoSEventAdapt:
            {
            return _L("EQoSEventAdapt");
            }
        case EQoSEventChannel:
            {
            return _L("EQoSEventChannel");
            }
        case EQoSEventJoin:
            {
            return _L("EQoSEventJoin");
            }
        case EQoSEventLeave:
            {
            return _L("EQoSEventLeave");
            }
        case EQoSEventAddPolicy:
            {
            return _L("EQoSEventAddPolicy");
            }
        case EQoSEventGetPolicy:
            {
            return _L("EQoSEventGetPolicy");
            }
        case EQoSEventDeletePolicy:
            {
            return _L("EQoSEventDeletePolicy");
            }
        case EQoSEventLoadPolicyFile:
            {
            return _L("EQoSEventLoadPolicyFile");
            }
        case EQoSEventUnloadPolicyFile:
            {
            return _L("EQoSEventUnloadPolicyFile");
            }
        default:
            {
            // unknown event type
            _LIT(KText15, "Unknown event in QoSEventToText!!! Event: %d");
            Log( KText15, aEvent);
            break;
            }
        }
    return NULL;
    }


// This function just sends something to socket. This is one way 
// to raise nif up
TInt CTestStepQoS::SendData(RSocket& aSocket)
    {
    TRequestStatus status;
    HBufC8 *buf = NULL;
    TRAPD(err, buf = HBufC8::NewL(32));
    if (err != KErrNone)
        {
        Log(_L("HBufC8::NewL error: %d"), err);
        }

    TPtr8 ptr(buf->Des());
    ptr.Fill(42);
    aSocket.Write(ptr, status);
    User::WaitForRequest(status);
    delete buf;

    return status.Int();
    }


// This function wakes up the nif
TInt CTestStepQoS::WakeupNifL()
    {
    TInt err = KErrNone;

    RSocketServ socketServer;
    RConnection conn1;
    CleanupClosePushL(conn1);

    err = socketServer.Connect();
    if (err != KErrNone)
        {
        _LIT(KText16, "CTestStepQoS::WakeupNifL() - Could not connect to \
the socket server");
        Log( KText16 );
        return err;
        }

    err = conn1.Open(socketServer);
    if (err != KErrNone)
        {
        _LIT(KText17, 
            "CTestStepQoS::WakeupNifL() - Could not open the RConnection");
        Log( KText17 );
        return err;
        }

    err = conn1.Start();  //using default CommDb Settings
    if (err != KErrNone)
        {
        _LIT(KText18, 
            "CTestStepQoS::WakeupNifL() - Could not start the RConnection");
        Log( KText18 );
        return err;
        }

    conn1.Close();
    CleanupStack::PopAndDestroy(1);
    socketServer.Close();

    return err;
    }


TInt CTestStepQoS::NifAPIL(RSocket& aSocket, const TUint aCommand)
    {
    TInt err = KErrNone;
    err = WakeupNifL();
    if (err != KErrNone)
        {
        _LIT(KText19, "CTestStepQoS::NifAPIL - Nif could not be waken up");
        Log( KText19 );
        return err;
        }
    
    TTestNifControlIf testStruct;
    _LIT(KText20, "testnif.1:ip");
    testStruct.iInterfaceIdentifier.iName = KText20;
    TPckgBuf<TTestNifControlIf> opt;
    opt() = testStruct;    
    return aSocket.SetOpt(aCommand, KSOLInterface, opt);
    }


TBool CTestStepQoS::CheckTestFile(const TDesC& aName) 
    {
    RFs fs;
    RFile file;
    fs.Connect();
    TInt err = file.Open(fs, aName, EFileStreamText);
    file.Close();
    fs.Close();

    return (err == KErrNone);
    }

TInt CTestStepQoS::CreateTestFile(const TDesC& aName)
    {
    RFs fs;
    RFile file;
    fs.Connect();
    TInt err = file.Create(fs, aName, EFileStreamText);
    file.Close();
    fs.Close();

    return err;
    }

TInt CTestStepQoS::ClearTestFiles()
    {
    RFs fs;
    fs.Connect();
    fs.Delete(KTestFile1);
    fs.Delete(KTestFile2);
    fs.Delete(KUseTestModule);
    fs.Delete(KUseNoModule);
    fs.Delete(KUseTestModuleNonExist);
    fs.Delete(KTestSetDefaultQoSFail);
    fs.Delete(KTestModuleLoaded);
    fs.Delete(KTestModuleUnloaded);
    fs.Delete(KRel99);
    fs.Close();

    return KErrNone;
    }

/* 
 * These methods are valid for Symbian OS 9.0 and onwards.
 */ 
void CTestStepQoS::SetQoSParamDefault(CQoSParameters& aParameters)
    {
    aParameters.SetUplinkBandwidth(0);
    aParameters.SetUpLinkMaximumBurstSize(0);
    aParameters.SetUpLinkMaximumPacketSize(0);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(KQoSLowestPriority);

    aParameters.SetDownlinkBandwidth(0);
    aParameters.SetDownLinkMaximumBurstSize(0);
    aParameters.SetDownLinkMaximumPacketSize(0);
    aParameters.SetDownLinkAveragePacketSize(0);
    aParameters.SetDownLinkDelay(0);
    aParameters.SetDownLinkPriority(KQoSLowestPriority);
    
    aParameters.SetAdaptMode(EFalse);
    }


void CTestStepQoS::SetQoSParamSet1(CQoSParameters& aParameters)
    {
    aParameters.SetUplinkBandwidth(1024);
    aParameters.SetUpLinkMaximumBurstSize(896);
    aParameters.SetUpLinkMaximumPacketSize(768);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(3);

    aParameters.SetDownlinkBandwidth(1024);
    aParameters.SetDownLinkMaximumBurstSize(768);
    aParameters.SetDownLinkMaximumPacketSize(512);
    aParameters.SetDownLinkAveragePacketSize(300);
    aParameters.SetDownLinkDelay(200);
    aParameters.SetDownLinkPriority(KQoSLowestPriority);
    
    aParameters.SetAdaptMode(EFalse);
    }

void CTestStepQoS::SetEsockParamSet1(CSubConQosGenericParamSet& aParameters)
    {
    aParameters.SetUplinkBandwidth(1024);
    aParameters.SetUpLinkMaximumBurstSize(896);
    aParameters.SetUpLinkMaximumPacketSize(768);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(3);

    aParameters.SetDownlinkBandwidth(1024);
    aParameters.SetDownLinkMaximumBurstSize(768);
    aParameters.SetDownLinkMaximumPacketSize(512);
    aParameters.SetDownLinkAveragePacketSize(300);
    aParameters.SetDownLinkDelay(200);
    aParameters.SetDownLinkPriority(KQoSLowestPriority);
    
    aParameters.SetHeaderMode(ETrue);
    // Not yet defined in Esock
    // aParameters.SetAdaptMode(EFalse);
    
    }

void CTestStepQoS::SetEsockParamSet2(CSubConQosGenericParamSet& aParameters)
    {
    aParameters.SetUplinkBandwidth(2000);
    aParameters.SetUpLinkMaximumBurstSize(550);
    aParameters.SetUpLinkMaximumPacketSize(200);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(KQoSLowestPriority);

    aParameters.SetDownlinkBandwidth(800);
    aParameters.SetDownLinkMaximumBurstSize(1350);
    aParameters.SetDownLinkMaximumPacketSize(1000);
    aParameters.SetDownLinkAveragePacketSize(0);
    aParameters.SetDownLinkDelay(0);
    aParameters.SetDownLinkPriority(3);
    }

void CTestStepQoS::SetEsockParamSetWithPriorityOne(CSubConQosGenericParamSet& aParameters)
    {
    aParameters.SetUplinkBandwidth(1024);
    aParameters.SetUpLinkMaximumBurstSize(896);
    aParameters.SetUpLinkMaximumPacketSize(768);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(KQoSHighestPriority);

    aParameters.SetDownlinkBandwidth(1024);
    aParameters.SetDownLinkMaximumBurstSize(768);
    aParameters.SetDownLinkMaximumPacketSize(512);
    aParameters.SetDownLinkAveragePacketSize(300);
    aParameters.SetDownLinkDelay(200);
    aParameters.SetDownLinkPriority(KQoSHighestPriority);
    
    aParameters.SetHeaderMode(ETrue);
    // Not yet defined in Esock
    // aParameters.SetAdaptMode(EFalse);
    
    }
    

TInt CTestStepQoS::CheckEvent(TNotificationEventBuf& aEvent, 
                              const TUint32          aExpectedEvent)
    {
    switch (aEvent.Id())
        {
        case KSubConGenericEventParamsGranted:
            {
            Log(_L("CSubConGenericEventParamsGranted received"));
            break;
            }
        case KSubConGenericEventDataClientJoined:
            {
            Log(_L("CSubConGenericEventDataClientJoined received"));
            break;
            }
        case KSubConGenericEventDataClientLeft:
            {
            Log(_L("CSubConGenericEventDataClientLeft received"));
            break;
            }
        case KSubConGenericEventSubConDown:
            {
            Log(_L("CSubConGenericEventSubConDown received"));
            break;
            }
        case KSubConGenericEventParamsChanged:
            {
            Log(_L("CSubConGenericEventParamsChanged received"));
            break;
            }
        case KSubConGenericEventParamsRejected:
            {
            Log(_L("CSubConGenericEventParamsRejected received"));
            break;
            }
        default:
            {
            Log(_L("Unknown event received.  Id=%d"), aEvent.Id());
            break;
            }
        }
    if (aEvent.Id() == aExpectedEvent)
        {
        return KErrNone;
        }
    else
        {
        return KErrNotFound;
        }
    }
    
   
TInt CTestStepQoS::CheckImsExtensionValueL(RSubConnection &aSubconn, TBool aImsFlagValue)    
	{
	TInt retVal = KErrNone;
	RSubConParameterBundle bundle1;
	CleanupClosePushL(bundle1);
	TInt ret = aSubconn.GetParameters(bundle1);
	TESTL(ret == KErrNone);

	CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);

	CSubConIMSExtParamSet* requestedImsParams = 
		(CSubConIMSExtParamSet*)family1->FindExtensionSet(/*STypeId::*/STypeId(KSubCon3GPPExtParamsFactoryUid, KSubConIMSExtParamsType),
		CSubConParameterFamily::ERequested);
		
	CSubConIMSExtParamSet* acceptableImsParams = 
		(CSubConIMSExtParamSet*)family1->FindExtensionSet(/*STypeId::*/STypeId(KSubCon3GPPExtParamsFactoryUid, KSubConIMSExtParamsType),
		CSubConParameterFamily::EAcceptable);
	
	CSubConIMSExtParamSet* grantedImsParams = 
		(CSubConIMSExtParamSet*)family1->FindExtensionSet(/*STypeId::*/STypeId(KSubCon3GPPExtParamsFactoryUid, KSubConIMSExtParamsType),
		CSubConParameterFamily::EGranted);
	if(grantedImsParams)
		{
		TEST(grantedImsParams->GetIMSSignallingIndicator() == aImsFlagValue);
		}
	else
		{
		Log(_L("CSubConIMSExtParamSet extension not found in the subconnection CSubConParameterFamily::EGranted"));	
		TEST(EFalse);
		retVal= KErrNotFound;
		}
		
	CleanupStack::Pop(&bundle1);	
	return retVal;	
	}
	
/*
*	This method reads the UMTS R5 extension from the subconnection and compare the values of acceptable and granted parameters
*
*/
TInt CTestStepQoS::CheckUmtsR5ExtensionValuesL(RSubConnection &aSubconn, 
												TBool aSignallingIndicator, 
												RPacketQoS::TSourceStatisticsDescriptor aSourceStatisticsDescriptor)    
	{
	TInt retVal = KErrNone;
	RSubConParameterBundle bundle1;
	CleanupClosePushL(bundle1);
	TInt ret = aSubconn.GetParameters(bundle1);
	TESTL(ret == KErrNone);

	CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);


	CSubConQosR5ParamSet* grantedR5Params = 
		(CSubConQosR5ParamSet*)family1->FindExtensionSet(/*STypeId::*/STypeId(KSubCon3GPPExtParamsFactoryUid, KSubConQosR5ParamsType),
		CSubConParameterFamily::EGranted);

	CSubConQosR5ParamSet* acceptableR5Params = 
		(CSubConQosR5ParamSet*)family1->FindExtensionSet(/*STypeId::*/STypeId(KSubCon3GPPExtParamsFactoryUid, KSubConQosR5ParamsType),
		CSubConParameterFamily::EAcceptable);
	/*
	if((acceptableR5Params)&&(grantedR5Params))
		{
		TESTE(acceptableR5Params->GetTrafficClass() == grantedR5Params->GetTrafficClass(), grantedR5Params->GetTrafficClass());
		TESTE(acceptableR5Params->GetDeliveryOrder() == grantedR5Params->GetDeliveryOrder(), grantedR5Params->GetDeliveryOrder());
		TESTE(acceptableR5Params->GetErroneousSDUDelivery() == grantedR5Params->GetErroneousSDUDelivery(), grantedR5Params->GetErroneousSDUDelivery());
		TESTE(acceptableR5Params->GetResidualBitErrorRatio() == grantedR5Params->GetResidualBitErrorRatio(), grantedR5Params->GetResidualBitErrorRatio());
		TESTE(acceptableR5Params->GetSDUErrorRatio() == grantedR5Params->GetSDUErrorRatio(), grantedR5Params->GetSDUErrorRatio());
		TESTE(acceptableR5Params->GetTrafficHandlingPriority() == grantedR5Params->GetTrafficHandlingPriority(), grantedR5Params->GetTrafficHandlingPriority());
		TESTE(acceptableR5Params->GetTransferDelay() == grantedR5Params->GetTransferDelay(), grantedR5Params->GetTransferDelay());
		TESTE(acceptableR5Params->GetMaxSduSize() == grantedR5Params->GetMaxSduSize(), grantedR5Params->GetMaxSduSize());
		TESTE(acceptableR5Params->GetMaxBitrateUplink() == grantedR5Params->GetMaxBitrateUplink(), grantedR5Params->GetMaxBitrateUplink());
		TESTE(acceptableR5Params->GetMaxBitrateDownlink() == grantedR5Params->GetMaxBitrateDownlink(), grantedR5Params->GetMaxBitrateDownlink());
		TESTE(acceptableR5Params->GetGuaBitrateUplink() == grantedR5Params->GetGuaBitrateUplink(), grantedR5Params->GetGuaBitrateUplink());
		TESTE(acceptableR5Params->GetGuaBitrateDownlink() == grantedR5Params->GetGuaBitrateDownlink(), grantedR5Params->GetGuaBitrateDownlink());
		
		TESTE(acceptableR5Params->GetSignallingIndicator() == grantedR5Params->GetSignallingIndicator(), grantedR5Params->GetSignallingIndicator());
		TESTE(acceptableR5Params->GetSourceStatisticsDescriptor() == grantedR5Params->GetSourceStatisticsDescriptor(), grantedR5Params->GetSourceStatisticsDescriptor());
		}
	*/
	if(grantedR5Params)
		{
		TESTE(grantedR5Params->GetSignallingIndicator() == aSignallingIndicator, grantedR5Params->GetSignallingIndicator());
		TESTE(grantedR5Params->GetSourceStatisticsDescriptor() == aSourceStatisticsDescriptor, grantedR5Params->GetSourceStatisticsDescriptor());
		}
	else
		{
		Log(_L("CSubConQosR5ParamSet extension not found in the subconnection CSubConParameterFamily::EGranted"));	
		TEST(EFalse);
		retVal= KErrNotFound;
		}
		
	CleanupStack::Pop(&bundle1);	
	return retVal;	
	}

void CTestStepQoS::SetIPLinkR99HighDemand(CSubConQosIPLinkR99ParamSet& 
    aParameters)
    {
    aParameters.SetTrafficClass(RPacketQoS::ETrafficClassConversational);
    aParameters.SetDeliveryOrder(RPacketQoS::EDeliveryOrderRequired);
    aParameters.SetErroneousSDUDelivery(
        RPacketQoS::EErroneousSDUDeliveryRequired);
    aParameters.SetResidualBitErrorRatio(RPacketQoS::EBEROnePerHundred);
    aParameters.SetSDUErrorRatio(
        RPacketQoS::ESDUErrorRatioOnePerHundredThousand);
    aParameters.SetTrafficHandlingPriority(RPacketQoS::ETrafficPriority1);
    aParameters.SetTransferDelay(100);
    aParameters.SetMaxSduSize(1520);
    aParameters.SetMaxBitrateUplink(2000);
    aParameters.SetMaxBitrateDownlink(1000);
    aParameters.SetGuaBitrateUplink(500);
    aParameters.SetGuaBitrateDownlink(300);
    }

void CTestStepQoS::SetIPLinkR99HighDemandMin(CSubConQosIPLinkR99ParamSet& 
    aParameters)
    {
    aParameters.SetTrafficClass(RPacketQoS::ETrafficClassStreaming);
    aParameters.SetDeliveryOrder(RPacketQoS::EDeliveryOrderNotRequired);
    aParameters.SetErroneousSDUDelivery(
        RPacketQoS::EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBitErrorRatio(RPacketQoS::EBEROnePerMillion);
    aParameters.SetSDUErrorRatio(
        RPacketQoS::ESDUErrorRatioOnePerHundredThousand);
    aParameters.SetTrafficHandlingPriority(RPacketQoS::ETrafficPriority3);
    aParameters.SetTransferDelay(1000);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1000);
    aParameters.SetMaxBitrateDownlink(800);
    aParameters.SetGuaBitrateUplink(400);
    aParameters.SetGuaBitrateDownlink(200);
    }

void CTestStepQoS::SetIPLinkR99HighDemandNegotiated(
    CSubConQosIPLinkR99ParamSet& aParameters)
    {
    aParameters.SetTrafficClass(RPacketQoS::ETrafficClassStreaming);
    aParameters.SetDeliveryOrder(RPacketQoS::EDeliveryOrderRequired);
    aParameters.SetErroneousSDUDelivery(
        RPacketQoS::EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBitErrorRatio(RPacketQoS::EBEROnePerThousand);
    aParameters.SetSDUErrorRatio(RPacketQoS::ESDUErrorRatioOnePerTenThousand);
    aParameters.SetTrafficHandlingPriority(RPacketQoS::ETrafficPriority2);
    aParameters.SetTransferDelay(500);
    aParameters.SetMaxSduSize(1510);
    aParameters.SetMaxBitrateUplink(1500);
    aParameters.SetMaxBitrateDownlink(900);
    aParameters.SetGuaBitrateUplink(450);
    aParameters.SetGuaBitrateDownlink(250);
    }

void CTestStepQoS::SetIPLinkR99Background(CSubConQosIPLinkR99ParamSet& 
    aParameters)
    {
    aParameters.SetTrafficClass(RPacketQoS::ETrafficClassBackground);
    aParameters.SetDeliveryOrder(RPacketQoS::EDeliveryOrderRequired);
    aParameters.SetErroneousSDUDelivery(
        RPacketQoS::EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBitErrorRatio(RPacketQoS::EBEROnePerThousand);
    aParameters.SetSDUErrorRatio(RPacketQoS::ESDUErrorRatioOnePerTenThousand);
    aParameters.SetTrafficHandlingPriority(
        RPacketQoS::ETrafficPriorityUnspecified);
    aParameters.SetTransferDelay(500);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1500);
    aParameters.SetMaxBitrateDownlink(900);
    aParameters.SetGuaBitrateUplink(450);
    aParameters.SetGuaBitrateDownlink(250);
    }

void CTestStepQoS::SetIPLinkR99BackgroundMin(CSubConQosIPLinkR99ParamSet& 
    aParameters)
    {
    aParameters.SetTrafficClass(RPacketQoS::ETrafficClassBackground);
    aParameters.SetDeliveryOrder(RPacketQoS::EDeliveryOrderNotRequired);
    aParameters.SetErroneousSDUDelivery(
        RPacketQoS::EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBitErrorRatio(RPacketQoS::EBEROnePerMillion);
    aParameters.SetSDUErrorRatio(
        RPacketQoS::ESDUErrorRatioOnePerHundredThousand);
    aParameters.SetTrafficHandlingPriority(
        RPacketQoS::ETrafficPriorityUnspecified);
    aParameters.SetTransferDelay(1000);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1000);
    aParameters.SetMaxBitrateDownlink(800);
    aParameters.SetGuaBitrateUplink(400);
    aParameters.SetGuaBitrateDownlink(200);
    }

// same as SetQoSParamSet1 but adapt flag is 'true'
void CTestStepQoS::SetQoSParamSet12(CQoSParameters& aParameters)
    {
    aParameters.SetUplinkBandwidth(1024);
    aParameters.SetUpLinkMaximumBurstSize(896);
    aParameters.SetUpLinkMaximumPacketSize(768);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(3);

    aParameters.SetDownlinkBandwidth(1024);
    aParameters.SetDownLinkMaximumBurstSize(768);
    aParameters.SetDownLinkMaximumPacketSize(512);
    aParameters.SetDownLinkAveragePacketSize(300);
    aParameters.SetDownLinkDelay(200);
    aParameters.SetDownLinkPriority(KQoSLowestPriority);
    
    aParameters.SetAdaptMode(ETrue);
    }


void CTestStepQoS::SetQoSParamSet2(CQoSParameters& aParameters)
    {
    aParameters.SetUplinkBandwidth(2000);
    aParameters.SetUpLinkMaximumBurstSize(550);
    aParameters.SetUpLinkMaximumPacketSize(200);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(KQoSLowestPriority);

    aParameters.SetDownlinkBandwidth(800);
    aParameters.SetDownLinkMaximumBurstSize(1350);
    aParameters.SetDownLinkMaximumPacketSize(1000);
    aParameters.SetDownLinkAveragePacketSize(0);
    aParameters.SetDownLinkDelay(0);
    aParameters.SetDownLinkPriority(3);
    }


void CTestStepQoS::SetQoSParamSet3(CQoSParameters& aParameters)
    {
    aParameters.SetUplinkBandwidth(2000);
    aParameters.SetUpLinkMaximumBurstSize(550);
    aParameters.SetUpLinkMaximumPacketSize(200);
    aParameters.SetUpLinkAveragePacketSize(0);
    aParameters.SetUpLinkDelay(0);
    aParameters.SetUpLinkPriority(KQoSLowestPriority);

    aParameters.SetDownlinkBandwidth(800);
    aParameters.SetDownLinkMaximumBurstSize(1350);
    aParameters.SetDownLinkMaximumPacketSize(1000);
    aParameters.SetDownLinkAveragePacketSize(0);
    aParameters.SetDownLinkDelay(0);
    aParameters.SetDownLinkPriority(3);
    
    // adapt flag true
    aParameters.SetAdaptMode(ETrue);
    }


//
// Invalid parameters that don't pass guqos checkings.
//
void CTestStepQoS::SetQoSParamSetArbitrary(CQoSParameters& aParameters)
    {
    aParameters.SetUplinkBandwidth(400);
    aParameters.SetUpLinkMaximumBurstSize(8192);
    aParameters.SetUpLinkMaximumPacketSize(1);
    aParameters.SetUpLinkAveragePacketSize(6552222);
    aParameters.SetUpLinkDelay(10000);
    aParameters.SetUpLinkPriority(1);

    aParameters.SetDownlinkBandwidth(10);
    aParameters.SetDownLinkMaximumBurstSize(10);
    aParameters.SetDownLinkMaximumPacketSize(20480000);
    aParameters.SetDownLinkAveragePacketSize(0);
    aParameters.SetDownLinkDelay(0);
    aParameters.SetDownLinkPriority(KQoSHighestPriority);
    
    aParameters.SetAdaptMode(EFalse);
    }


/******************************************
	To create non-real time traffic class
******************************************/
/*
void CTestStepQoS::SetQoSParamSetForNRT(CQoSParameters& aParameters)
{
   
	aParameters.SetTokenRateUplink(1024);
	aParameters.SetTokenBucketSizeUplink(896);
	aParameters.SetMaxPacketSizeUplink(768);
	aParameters.SetMinPolicedUnitUplink(0);
	aParameters.SetDelayUplink(0);
	aParameters.SetPriorityUplink(4);

	aParameters.SetTokenRateDownlink(1024);
	aParameters.SetTokenBucketSizeDownlink(768);
	aParameters.SetMaxPacketSizeDownlink(512);
	aParameters.SetMinPolicedUnitDownlink(300);
	aParameters.SetDelayDownlink(0);
	aParameters.SetPriorityDownlink(4);

	aParameters.SetAdaptMode(EFalse);
}
*/


void CTestStepQoS::SetUmtsQoSHighDemand(TUmtsQoSParameters& aParameters)
    {
    aParameters.SetTrafficClass(ETrafficClassConversational);
    aParameters.SetDeliveryOrder(EDeliveryOrderRequired);
    aParameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryRequired);
    aParameters.SetResidualBer(EBEROnePerHundred);
    aParameters.SetErrorRatio(ESDUErrorRatioOnePerHundredThousand);
    aParameters.SetPriority(ETrafficPriority1);
    aParameters.SetTransferDelay(100);
    aParameters.SetMaxSduSize(1520);
    aParameters.SetMaxBitrateUplink(2000);
    aParameters.SetMaxBitrateDownlink(1000);
    aParameters.SetGuaranteedBitrateUplink(500);
    aParameters.SetGuaranteedBitrateDownlink(300);
    }

void CTestStepQoS::SetUmtsQoSHighDemandMin(TUmtsQoSParameters& aParameters)
    {
    aParameters.SetTrafficClass(ETrafficClassStreaming);
    aParameters.SetDeliveryOrder(EDeliveryOrderNotRequired);
    aParameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBer(EBEROnePerMillion);
    aParameters.SetErrorRatio(ESDUErrorRatioOnePerHundredThousand);
    aParameters.SetPriority(ETrafficPriority3);
    aParameters.SetTransferDelay(1000);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1000);
    aParameters.SetMaxBitrateDownlink(800);
    aParameters.SetGuaranteedBitrateUplink(400);
    aParameters.SetGuaranteedBitrateDownlink(200);
    }

void CTestStepQoS::SetUmtsQoSHighDemandNegotiated(TUmtsQoSParameters& 
                                                  aParameters)
    {
    aParameters.SetTrafficClass(ETrafficClassStreaming);
    aParameters.SetDeliveryOrder(EDeliveryOrderRequired);
    aParameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBer(EBEROnePerThousand);
    aParameters.SetErrorRatio(ESDUErrorRatioOnePerTenThousand);
    aParameters.SetPriority(ETrafficPriority2);
    aParameters.SetTransferDelay(500);
    aParameters.SetMaxSduSize(1510);
    aParameters.SetMaxBitrateUplink(1500);
    aParameters.SetMaxBitrateDownlink(900);
    aParameters.SetGuaranteedBitrateUplink(450);
    aParameters.SetGuaranteedBitrateDownlink(250);
    }

void CTestStepQoS::SetUmtsQoSBackground(TUmtsQoSParameters& aParameters)
    {
    aParameters.SetTrafficClass(ETrafficClassBackground);
    aParameters.SetDeliveryOrder(EDeliveryOrderRequired);
    aParameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBer(EBEROnePerThousand);
    aParameters.SetErrorRatio(ESDUErrorRatioOnePerTenThousand);
    aParameters.SetPriority(ETrafficPriorityUnspecified);
    aParameters.SetTransferDelay(500);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1500);
    aParameters.SetMaxBitrateDownlink(900);
    aParameters.SetGuaranteedBitrateUplink(450);
    aParameters.SetGuaranteedBitrateDownlink(250);
    }

void CTestStepQoS::SetUmtsQoSBackgroundMin(TUmtsQoSParameters& aParameters)
    {
    aParameters.SetTrafficClass(ETrafficClassBackground);
    aParameters.SetDeliveryOrder(EDeliveryOrderNotRequired);
    aParameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBer(EBEROnePerMillion);
    aParameters.SetErrorRatio(ESDUErrorRatioOnePerHundredThousand);
    aParameters.SetPriority(ETrafficPriorityUnspecified);
    aParameters.SetTransferDelay(1000);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1000);
    aParameters.SetMaxBitrateDownlink(800);
    aParameters.SetGuaranteedBitrateUplink(400);
    aParameters.SetGuaranteedBitrateDownlink(200);
    }

#ifdef SYMBIAN_NETWORKING_UMTSR5
void CTestStepQoS::SetUmtsR5QoSBackground(TUmtsR5QoSParameters& aParameters)
{
    aParameters.SetTrafficClass(ETrafficClassBackground);
    aParameters.SetDeliveryOrder(EDeliveryOrderRequired);
    aParameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBer(EBEROnePerThousand);
    aParameters.SetErrorRatio(ESDUErrorRatioOnePerTenThousand);
    aParameters.SetPriority(ETrafficPriorityUnspecified);
    aParameters.SetTransferDelay(500);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1500);
    aParameters.SetMaxBitrateDownlink(900);
    aParameters.SetGuaranteedBitrateUplink(450);
    aParameters.SetGuaranteedBitrateDownlink(250);
    
    aParameters.SetSignallingIndicator(EFalse);
    aParameters.SetSourceStatisticsDescriptor(ESourceStatisticsDescriptorSpeech);

}

void CTestStepQoS::SetUmtsR5QoSBackgroundMin(TUmtsR5QoSParameters& aParameters)
{
    aParameters.SetTrafficClass(ETrafficClassBackground);
    aParameters.SetDeliveryOrder(EDeliveryOrderNotRequired);
    aParameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryNotRequired);
    aParameters.SetResidualBer(EBEROnePerMillion);
    aParameters.SetErrorRatio(ESDUErrorRatioOnePerHundredThousand);
    aParameters.SetPriority(ETrafficPriorityUnspecified);
    aParameters.SetTransferDelay(1000);
    aParameters.SetMaxSduSize(1500);
    aParameters.SetMaxBitrateUplink(1000);
    aParameters.SetMaxBitrateDownlink(800);
    aParameters.SetGuaranteedBitrateUplink(400);
    aParameters.SetGuaranteedBitrateDownlink(200);

	aParameters.SetSignallingIndicator(EFalse);
    aParameters.SetSourceStatisticsDescriptor(ESourceStatisticsDescriptorSpeech);
}


void CTestStepQoS::SetUmtsR5QoSRequested1(CSubConQosR5ParamSet& aParameters)
{
    aParameters.SetTrafficClass(RPacketQoS::ETrafficClassInteractive);                       
    aParameters.SetDeliveryOrder(RPacketQoS::EDeliveryOrderRequired);                        
    aParameters.SetErroneousSDUDelivery(RPacketQoS::EErroneousSDUDeliveryNotRequired);       
    aParameters.SetResidualBitErrorRatio(RPacketQoS::EBEROnePerThousand);                    
    aParameters.SetSDUErrorRatio(RPacketQoS::ESDUErrorRatioOnePerTenThousand);               
    aParameters.SetTrafficHandlingPriority(RPacketQoS::ETrafficPriorityUnspecified);         
    aParameters.SetTransferDelay(500);                                                       
    aParameters.SetMaxSduSize(1500);                                                         
    aParameters.SetMaxBitrateUplink(1500);                                                   
    aParameters.SetMaxBitrateDownlink(900);                                                  
    aParameters.SetGuaBitrateUplink(450);                                                    
    aParameters.SetGuaBitrateDownlink(250);                                                  
                                                                                         
	aParameters.SetSignallingIndicator(ETrue);                                               
    aParameters.SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
}

void CTestStepQoS::SetUmtsR5QoSAcceptable1(CSubConQosR5ParamSet& aParameters)
{
    aParameters.SetTrafficClass(RPacketQoS::ETrafficClassInteractive);                           
    aParameters.SetDeliveryOrder(RPacketQoS::EDeliveryOrderNotRequired);                         
    aParameters.SetErroneousSDUDelivery(RPacketQoS::EErroneousSDUDeliveryNotRequired);           
    aParameters.SetResidualBitErrorRatio(RPacketQoS::EBEROnePerMillion);                         
    aParameters.SetSDUErrorRatio(RPacketQoS::ESDUErrorRatioOnePerHundredThousand);               
    aParameters.SetTrafficHandlingPriority(RPacketQoS::ETrafficPriorityUnspecified);             
    aParameters.SetTransferDelay(1000);                                                          
    aParameters.SetMaxSduSize(1500);                                                             
    aParameters.SetMaxBitrateUplink(1000);                                                       
    aParameters.SetMaxBitrateDownlink(800);                                                      
    aParameters.SetGuaBitrateUplink(400);                                                        
    aParameters.SetGuaBitrateDownlink(200);                                                      
                                                                                                 
	aParameters.SetSignallingIndicator(ETrue);                                                   
    aParameters.SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);    
}



#endif // SYMBIAN_NETWORKING_UMTSR5


//
//CQoSListener implementation //

CQoSListener::CQoSListener()
: CActive(0)
{
}


CQoSListener* CQoSListener::NewL()
    {
    CQoSListener *self = new(ELeave) CQoSListener();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }


void CQoSListener::ConstructL()
    {
    iInterval = 200000; // 0.Xs
    iMaxTimeSpent = 60000000; // 60s

    User::LeaveIfError(iTimer.CreateLocal());
    CActiveScheduler::Add(this);
    }


CQoSListener::~CQoSListener()
    {
    Cancel();
    iTimer.Close();
    }


void CQoSListener::DoCancel()
    {
    iTimer.Cancel();
    }


void CQoSListener::RunL()
    {
    iTimeSpent += iInterval;
    if(iTimeSpent < iMaxTimeSpent)
        {
        iTimer.After(iStatus, iInterval);
        SetActive();
        }
    else
        {
        Cancel();
        CActiveScheduler::Stop();
        }
    }

void CQoSListener::Start()
    {
    _LIT(KText21, "CTimer::Start");
    __ASSERT_ALWAYS(!IsActive(), User::Panic(KText21, 1));
    iTimer.After(iStatus, iInterval);
    SetActive();
    }


// End of CQoSListener //
//

//
// CSubConnListener implementation //

CSubConnListener* CSubConnListener::NewL()
    {
    CSubConnListener *self = new(ELeave) CSubConnListener();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }


CSubConnListener::CSubConnListener()
    : CActive(EPriorityStandard), 
      iInterval(200000),         //  0.2 s
      iMaxTimeSpent(60000000)    // 60.0 s
    {
    CActiveScheduler::Add(this);
    }


void CSubConnListener::ConstructL()
    {
    iTimer.CreateLocal();
    }


CSubConnListener::~CSubConnListener()
    {
    Cancel();
    iTimer.Close();
    }


void CSubConnListener::Start()
    {
    _LIT(KText21, "CSubConnListener::Start");
    __ASSERT_ALWAYS(!IsActive(), User::Panic(KText21, 1));
    iTimer.After(iStatus, iInterval);
    SetActive();
    }


void CSubConnListener::RunL()
    {
    iTimeSpent += iInterval;
    if(iTimeSpent < iMaxTimeSpent)
        {
        iTimer.After(iStatus, iInterval);
        SetActive();
        }
    else
        {
        Cancel();
        CActiveScheduler::Stop();
        }
    }


void CSubConnListener::DoCancel()
    {
    iTimer.Cancel();
    }


// End of CSubConnListener //
//


//
// CEventListener implementation //

CEventListener* CEventListener::NewL()
    {
    CEventListener *self = new(ELeave) CEventListener();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }


CEventListener::CEventListener()
    : CActive(EPriorityStandard) 
    {
    CActiveScheduler::Add(this);
    }


void CEventListener::ConstructL()
    {
    }


CEventListener::~CEventListener()
    {
    Cancel();
    }


void CEventListener::Start()
    {
    _LIT(KText21, "CEventListener::Start");
    __ASSERT_ALWAYS(!IsActive(), User::Panic(KText21, 1));
    SetActive();
    }


void CEventListener::RunL()
    {
    Cancel();
    CActiveScheduler::Stop();
    }


void CEventListener::DoCancel()
    {
    }


// End of CEventListener //
//
