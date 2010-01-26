// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// TestStepSelfTest.cpp
// This contains CTestCase which is the base class for all the TestCase DLLs
// 
//

// Test system includes
#include "TS_QoSStep.h"
#include "TS_QoSSuite.h"


CTS_QoSStep::CTS_QoSStep() 
{
}

CTS_QoSStep::~CTS_QoSStep()
{
}


/* Get the IP address from the configuration file
 */
TBool CTS_QoSStep::GetIpAddressFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TInetAddr &anAddr)
	{
/* get string from config file
 * convert the string to a TInetAddr
 */
	TPtrC result;

	TBool bRet = GetStringFromConfig(aSectName, aKeyName, result);

	// if True
	if (bRet)
		{
		TInt nRet = anAddr.Input(result);
		// Invalid IP Address
		if (nRet!=KErrNone)
			{
			Log(_L("Invalid IP address, section:%S key:%S "),&aSectName, &aKeyName );
			bRet = EFalse;
			}
		}

	return bRet;
	}


void CTS_QoSStep::SetupConnectionPrefrencesL(TUint aKProtocolInet, TInt aQoSChannelNum, TInt aSocketNum)
	{
	// Will be used to setup the connections
	// Set Source Address, Destination Address, Port number, Packet Size and Number Of Packets To Send
	TInetAddr src, dst;
	TInt port, packetSize, packets, iap, err;
	TBuf16<20> ChannelNumSocketNum; 
	TPtrC16 bearerSet;

	// make up channel name
	ChannelNumSocketNum.Copy(Channel);
	ChannelNumSocketNum.AppendNum(aQoSChannelNum+1);
	// make up socket name
	ChannelNumSocketNum.Append(Socket);
	ChannelNumSocketNum.AppendNum(aSocketNum+1);

	// Get socket x details or common details from config file
	// Bearer Set
	if (GetStringFromConfig(ChannelNumSocketNum, _L("BearerSet"), bearerSet)==1)
		{}
	else
		TESTL(GetStringFromConfig(_L("Common"), _L("BearerSet"), bearerSet));

	// IAP
	if (GetIntFromConfig(ChannelNumSocketNum, _L("ConnIAP"), iap)==1)
		{}
	else
		TESTL(GetIntFromConfig(_L("Common"), _L("ConnIAP"), iap));

	// IP Address Local
	if (GetIpAddressFromConfig(ChannelNumSocketNum, _L("ipAddressLocal"), src)==1)
		{}
	else
		TESTL(GetIpAddressFromConfig(_L("Common"), _L("ipAddressLocal"), src));
	
	// IP Address Remote
	if (GetIpAddressFromConfig(ChannelNumSocketNum, _L("ipAddressRemote"), dst)==1)
		{}
	else
		TESTL(GetIpAddressFromConfig(_L("Common"), _L("ipAddressRemote"), dst));
	
	// Port Number
	if (GetIntFromConfig(ChannelNumSocketNum, _L("port"), port)==1)
		{}
	else
		TESTL(GetIntFromConfig(_L("Common"), _L("port"), port));
	
	// Packet Size
	if (GetIntFromConfig(ChannelNumSocketNum, _L("packetsize"), packetSize)==1)
		{}
	else
		TESTL(GetIntFromConfig(_L("Common"), _L("packetsize"), packetSize));
	
	// Number of Packets
	if (GetIntFromConfig(ChannelNumSocketNum, _L("numberofpackets"), packets)==1)
		{}
	else
		TESTL(GetIntFromConfig(_L("Common"), _L("numberofpackets"), packets));
	
	/* 
	 * Insert Connection data into an Array
	 */
	if (bearerSet.Compare(_L("KCommDbBearerCSD"))==0)
		{
		err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iSetBearerSet.Append(KCommDbBearerCSD);
		if (err!=KErrNone)
			{
			Log(_L("Failed to insert value into iSetBearerSet RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		}
	else
	if (bearerSet.Compare(_L("KCommDbBearerLAN"))==0)
		{
		err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iSetBearerSet.Append(KCommDbBearerLAN);
		if (err!=KErrNone)
			{
			Log(_L("Failed to insert value into iSetBearerSet RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		}
	else
	if (bearerSet.Compare(_L("KCommDbBearerUnknown"))==0)
		{
		err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iSetBearerSet.Append(KCommDbBearerUnknown);
		if (err!=KErrNone)
			{
			Log(_L("Failed to insert value into iSetBearerSet RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		}

	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iIAP.Append(iap);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iIAP RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iProtocol.Append(aKProtocolInet);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iProtocol RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iScrAddr.Append(src);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iScrAddr RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iDestAddr.Append(dst);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iDestAddr RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iPort.Append(port);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iPort RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iPacketSize.Append(packetSize);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iPacketSize RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iPackets.Append(packets);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iPackets RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	// Not done a bind to local address yet
	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iBind.Append(FALSE);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iBind RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	// Not done a connect to remote address yet
	err = iQoSSuite->iQoSChannel[aQoSChannelNum]->iConnect.Append(FALSE);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iConnect RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	// Not yet Set the QoS Parameters
	err = iQoSSuite->iQoSSet.Append(FALSE);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iQosSet RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iQoSChangeSet.Append(FALSE);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iQoSChangeSet RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iJoin.Append(FALSE);
	if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iJoin RArray : return value = <%d>"), err);
		User::Leave(err);
		}

	err = iQoSSuite->iLeave.Append(FALSE);
		if (err!=KErrNone)
		{
		Log(_L("Failed to insert value into iLeave RArray : return value = <%d>"), err);
		User::Leave(err);
		}
	}


void CTS_QoSStep::SetupQoSChannelParametersL(const TDesC &qoSParameterRefrence, TInt i)
	{
	// Uplink Parameters
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("TokenRateUplink"), iQoSSuite->iQoSChannel[i]->iTokenRateUplink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("TokenBucketSizeUplink"), iQoSSuite->iQoSChannel[i]->iTokenBucketSizeUplink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("MaxTransferRateUplink"), iQoSSuite->iQoSChannel[i]->iMaxTransferRateUplink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("MaxPacketSizeUplink"), iQoSSuite->iQoSChannel[i]->iMaxPacketSizeUplink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("MinPolicedUnitUplink"), iQoSSuite->iQoSChannel[i]->iMinPolicedUnitUplink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("DelayUplink"), iQoSSuite->iQoSChannel[i]->iDelayUplink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("PriorityUplink"), iQoSSuite->iQoSChannel[i]->iPriorityUplink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("DropModeUplink"), iQoSSuite->iQoSChannel[i]->iDropModeUplink));

	// Downlink Parameters
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("TokenRateDownlink"), iQoSSuite->iQoSChannel[i]->iTokenRateDownlink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("TokenBucketSizeDownlink"), iQoSSuite->iQoSChannel[i]->iTokenBucketSizeDownlink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("MaxTransferRateDownlink"), iQoSSuite->iQoSChannel[i]->iMaxTransferRateDownlink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("MaxPacketSizeDownlink"), iQoSSuite->iQoSChannel[i]->iMaxPacketSizeDownlink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("MinPolicedUnitDownlink"), iQoSSuite->iQoSChannel[i]->iMinPolicedUnitDownlink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("DelayDownlink"), iQoSSuite->iQoSChannel[i]->iDelayDownlink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("PriorityDownlink"), iQoSSuite->iQoSChannel[i]->iPriorityDownlink));
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("DropModeDownlink"), iQoSSuite->iQoSChannel[i]->iDropModeDownlink));

	// Adaptor Settings
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("AdaptorPriority"), iQoSSuite->iQoSChannel[i]->iAdaptationPriority));
	// Takes a TBool but used a TInt
	TESTL(GetIntFromConfig(qoSParameterRefrence, _L("AdaptMode"), iQoSSuite->iQoSChannel[i]->iAdaptMode));
	}


//
//											   //
// Nokia's Wait for QoS Request Implementation //
//											   // 
//

void CTS_QoSStep::WaitForQoSEvent()
{	
	CQoSListener *listener=NULL;
//	TRAPD(err ,listener->NewL(this));
	TRAPD(err ,listener = CQoSListener::NewL(this));
	if(err!=KErrNone)
		{
		iQoSSuite->Log(_L("Failed to create new CQoSListener object, error code <%d>"), err);
		User::Leave(err);
		}
	listener->Start();
	iReceivedEvent = KErrNotReady;	
	// Starts a new wait loop under the control of the current active scheduler.
	CActiveScheduler::Start();

	delete listener;
}


//
//									  //
// Class for listening the qos events //
//									  //
//

// implementation:
/* Constructor
 *
 */
CQoSListener::CQoSListener()
: CActive(0)
{}

CQoSListener* CQoSListener::NewL(CTS_QoSStep *aTestStepQoS)
{
	CQoSListener *self = new(ELeave) CQoSListener();
	CleanupStack::PushL(self);
	self->ConstructL(aTestStepQoS);
	CleanupStack::Pop();
	return self;
}


void CQoSListener::ConstructL(CTS_QoSStep* aTestStepQoS)
{
	// Pointer from CQoSListener to CTS_QoSStep - all test steps inherit this class
	iTestStepQoS = aTestStepQoS;
	iInterval = 200000; // 0.Xs
	iMaxTimeSpent = 30000000; // 30s

	User::LeaveIfError(iTimer.CreateLocal());
	// Adds the specified active object to the current active scheduler.
	CActiveScheduler::Add(this);
}

/* Destructor
 *
 */
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
	// if iReceiveEvent within CTS_QoSStep returns 0 or grater than
	if(iTestStepQoS->iReceivedEvent >= 0)	// any QoS event
		{
		// CActive Cancel - Cancels the wait for completion of an outstanding request. 
		// If there is no request outstanding, then the function does nothing.
		Cancel();
		// Stops the wait loop started by the most recent call to Start(). 
		CActiveScheduler::Stop();
		return;
		}

	iTimeSpent += iInterval;
	if(iTimeSpent < iMaxTimeSpent)
		{
		iTimer.After(iStatus, iInterval);
		// Indicates that the active object has issued a request and that it is now outstanding. 
		// Derived classes must call this function after issuing a request.
		SetActive();
		}
	else
		{
		iTestStepQoS->iReceivedEvent = KErrGeneral;
		// CActive Cancel - Cancels the wait for completion of an outstanding request. 
		// If there is no request outstanding, then the function does nothing.
		Cancel();
		// Stops the wait loop started by the most recent call to Start(). 
		CActiveScheduler::Stop();
		}

	//iTestStepQoS->iReceivedEvent = KPfqosEventConfirm; //xxx for testing

}

void CQoSListener::Start()
{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(_L("CTimer::Start"), 1));
	iTimer.After(iStatus, iInterval);
	// Indicates that the active object has issued a request and that it is now outstanding. 
	// Derived classes must call this function after issuing a request.
	SetActive();
}


