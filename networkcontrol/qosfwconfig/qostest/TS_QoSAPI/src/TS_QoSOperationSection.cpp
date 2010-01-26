
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
// This file contains an example Test step implementation 
// This demonstrates the various functions provided
// by the CTestStep base class which are available within
// a test step 
// 
//


// Test system includes
#include "TS_QoSOperationSection.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

/* Open CMultipleArray Objects
 */
CTS_QoSOperationSection2_0::CTS_QoSOperationSection2_0()
{
	iTestStepName = _L("Initialize_Open_QoS_Channel_2.0");
}

CTS_QoSOperationSection2_0::~CTS_QoSOperationSection2_0()
{
}

enum TVerdict CTS_QoSOperationSection2_0::doTestStepL( void )
	{
	// Get Current Number of QoS Channels opened - should be zero, if not then close
	iQoSSuite->beginQoSChannelCount = iQoSSuite->iQoSChannel.Count();

	if (iQoSSuite->iQoSChannel.Count() > 0)
		{
		// For Integration Testing
		// Incase the close connections / sockets / QoS channel methods fail then clean up on a new Scenario when we create new QoS channels

		for (TInt count = 0; count < iQoSSuite->iQoSChannel.Count(); count++)
			{
			// Explicit opening of sockets
			if (iQoSSuite->iQoSChannel[count]->GetConnectionListCount() > 0)
				{
				iQoSSuite->Log(_L("Closing <%d> connection(s)"), iQoSSuite->iQoSChannel[count]->GetConnectionListCount());
				for (TInt i = iQoSSuite->iQoSChannel[count]->GetConnectionListCount(); i > 0; i--)
					{
					iQoSSuite->Log(_L("Close connection <%d>"),i);
					iQoSSuite->iQoSChannel[count]->CloseConnection(i);
					}
				}
			// if any sockets have been left open then close them
			if (iQoSSuite->iQoSChannel[count]->GetSocketListCount() > 0)
				{
				// Close any Open Sockets
				iQoSSuite->Log(_L("Closing<%d> socket(s)"), iQoSSuite->iQoSChannel[count]->GetSocketListCount());
				for (i  = iQoSSuite->iQoSChannel[count]->GetSocketListCount(); i > 0; i--)
					{
					iQoSSuite->Log(_L("Close socket <%d>"), i);
					iQoSSuite->iQoSChannel[count]->CloseSocket(i);
					}
				}
			}
		// if any QoS Channels have been left open then close them
		if (iQoSSuite->iQoSChannel.Count() > 0)
			{
			// Close any Open QoS Channels
			Log(_L("Closing <%d> QoS Channel(s)"), iQoSSuite->iQoSChannel.Count());
			for (i = iQoSSuite->iQoSChannel.Count(); i > 0; i--)
				{
				Log(_L("Close QoS Channel <%d>"), i);
				iQoSSuite->CloseQoSChannelL(i);
				}
			}
		}

	// Reset array values to 0, this is for when multiple Scenarios are run and the Destructor does not get called untill the last cenario has been run
	if (iQoSSuite->iQoSSet.Count() > 0)
		{
		for (i = iQoSSuite->iQoSSet.Count(); i > 0; i--)
			iQoSSuite->iQoSSet[i-1] = 0;
		}
	if (iQoSSuite->iQoSChangeSet.Count() > 0)
		{
		for (i = iQoSSuite->iQoSChangeSet.Count(); i > 0; i--)
			iQoSSuite->iQoSChangeSet[i-1] = 0;
		}
	if (iQoSSuite->iJoin.Count() > 0)
		{
		for (i = iQoSSuite->iJoin.Count(); i > 0; i--)
			iQoSSuite->iJoin[i-1] = 0;
		}
	if (iQoSSuite->iLeave.Count() > 0)
		{
		for (i = iQoSSuite->iLeave.Count(); i > 0; i--)
			iQoSSuite->iLeave[i-1] = 0;
		}

	iQoSSuite->beginQoSChannelCount = 0;

	iQoSSuite->nExtraJoinImplicitTcp=0; iQoSSuite->nExtraJoinExplicitTcp=0;
	iQoSSuite->nExtraJoinImplicitUdp=0; iQoSSuite->nExtraJoinExplicitUdp=0;

	TESTL(GetIntFromConfig(_L("Test_2.0"), _L("extraJoinImplicitTcp"), iQoSSuite->nExtraJoinImplicitTcp));
	TESTL(GetIntFromConfig(_L("Test_2.0"), _L("extraJoinExplicitTcp"), iQoSSuite->nExtraJoinExplicitTcp));
	TESTL(GetIntFromConfig(_L("Test_2.0"), _L("extraJoinImplicitUdp"), iQoSSuite->nExtraJoinImplicitUdp));
	TESTL(GetIntFromConfig(_L("Test_2.0"), _L("extraJoinExplicitUdp"), iQoSSuite->nExtraJoinExplicitUdp));
	
	TInt totalExtraJoinSockets = iQoSSuite->nExtraJoinImplicitTcp + iQoSSuite->nExtraJoinExplicitTcp + iQoSSuite->nExtraJoinImplicitUdp + iQoSSuite->nExtraJoinExplicitUdp;
	
	for (i = 0; i < totalExtraJoinSockets; i++)
		{
		// Create new pointer of type CMultipleArray
		CMultipleArray* myCMultipleArray = CMultipleArray::NewLC();
		iQoSSuite->iQoSChannel.Append(myCMultipleArray);
		CleanupStack::Pop();	// myCMultipleArray
		iQoSSuite->iQoSChannel[iQoSSuite->iQoSChannel.Count()-1]->Initialize(this);
		}

	return EPass;
	}


/* Open Socket(s), Setup Connection Parameter(s), Open QoS Channel(s)
 */
CTS_QoSOperationSection2_1::CTS_QoSOperationSection2_1()
{
	iTestStepName = _L("Open_QoS_Channel_2.1");
}

CTS_QoSOperationSection2_1::~CTS_QoSOperationSection2_1()
{
}

enum TVerdict CTS_QoSOperationSection2_1::doTestStepL( void )
	{
//	RSocket *sock;
	TInt i, counter = 0;
	
	/*
	 * Open new Implicit / Explicit, Tcp / Udp Sockets, then Bind, Connect
	 */
//	for (i = iQoSSuite->beginQoSChannelCount; i < iQoSSuite->iQoSChannel.Count(); i++)
//		{
		
		if (iQoSSuite->beginQoSChannelCount > 0)
			counter = iQoSSuite->beginQoSChannelCount;

		// Add Extra number of Implicit TCP sockets
		for (i2 = 0; i2 < iQoSSuite->nExtraJoinImplicitTcp; i2++)
			{
			// Set-up connection prefrences
			// Will be used by test steps to setup the connections
			SetupConnectionPrefrencesL(KProtocolInetTcp, counter, 0);
			// Open Implicit Tcp Socket
			iQoSSuite->iQoSChannel[counter]->OpenImplicitTcpSocketL(iQoSSuite->iSocketServer);
			// Connect will perform a Bind() so we dont need to do, Will always be element zero in socket array as we are opening a new QoS Channel
			iQoSSuite->iQoSChannel[counter]->ConnectL(0);
			++counter;
			}

		// Add Extra number of Explicit TCP sockets
		for (i2 = 0; i2 < iQoSSuite->nExtraJoinExplicitTcp; i2++)
			{
			// Set-up connection prefrences
			// Will be used by test steps to setup the connections - Protocol, QoS Channel Number, Socket number will allways be one on creation of QoS Channel
			SetupConnectionPrefrencesL(KProtocolInetTcp, counter, 0);
			// Open Explicit Tcp Socket
			iQoSSuite->iQoSChannel[counter]->OpenExplicitTcpSocketL(0, iQoSSuite->iSocketServer);
			// Connect will perform a Bind() so we dont need to do, Will always be element zero in socket array as we are opening a new QoS Channel
			iQoSSuite->iQoSChannel[counter]->ConnectL(0);
			++counter;
			}

		// Add Extra number of Implicit UDP sockets
		for (i2 = 0; i2 < iQoSSuite->nExtraJoinImplicitUdp; i2++)
			{
			// Set-up connection prefrences
			// Will be used by test steps to setup the connections - Protocol, QoS Channel Number, Socket number will allways be one on creation of QoS Channel
			SetupConnectionPrefrencesL(KProtocolInetUdp, counter, 0);
			//Open Implicit Udp Socket
			iQoSSuite->iQoSChannel[counter]->OpenImplicitUdpSocketL(iQoSSuite->iSocketServer);
			// Bind to local address and make a connection to remote address
			iQoSSuite->iQoSChannel[counter]->BindL(0);
			// Connect will perform a Bind() so we dont need to do, Will always be element zero in socket array as we are opening a new QoS Channel
			iQoSSuite->iQoSChannel[counter]->ConnectL(0);
			++counter;
			}

		// Add Extra number of Explicit UDP sockets
		for (i2 = 0; i2 < iQoSSuite->nExtraJoinExplicitUdp; i2++)
			{
			// Set-up connection prefrences
			// Will be used by test steps to setup the connections - Protocol, QoS Channel Number, Socket number will allways be one on creation of QoS Channel
			SetupConnectionPrefrencesL(KProtocolInetUdp, counter, 0);
			// Open Explicit Udp Socket
			iQoSSuite->iQoSChannel[counter]->OpenExplicitUdpSocketL(0, iQoSSuite->iSocketServer);
			// Bind to local address and make a connection to remote address
			iQoSSuite->iQoSChannel[counter]->BindL(0);
			// Connect will perform a Bind() so we dont need to do, Will always be element zero in socket array as we are opening a new QoS Channel
			iQoSSuite->iQoSChannel[counter]->ConnectL(0);
			++counter;
			}
//		}

	/* 
	 * Open QoS Channel(s)
	 */
	// Open and Add new QoS channels
	for (i = iQoSSuite->beginQoSChannelCount; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		Log(_L("Open QoS Channel <%d> "), i+1);
		Log(_L("Open Socket <%d> "), 1);
		
//		sock = new (ELeave) RSocket;
//		CleanupStack::PushL(sock); 

		// Get Open Socket from array of RSockets
//		sock = iQoSSuite->iQoSChannel[i]->GetSocketHandle(0);
		// Open QoS Channel with Socket
		TInt err = iQoSSuite->iQoSChannel[i]->iQoSChannel->Open(*iQoSSuite->iQoSChannel[i]->GetSocketHandle(0));
		if (err!=KErrNone)
			{
			Log(_L("Failed to open QoS channel: return value = <%d>"), err);
			User::Leave(err);
			}

		// pop sock
//		CleanupStack::Pop();						
		}

	return EPass;
	}


/* Close a QoS Channel
 */
CTS_QoSOperationSection2_2::CTS_QoSOperationSection2_2()
{
	iTestStepName = _L("Close_QoS_Channel_Test2.2");
}

CTS_QoSOperationSection2_2::~CTS_QoSOperationSection2_2()
{
}

enum TVerdict CTS_QoSOperationSection2_2::doTestStepL( void )
	{
	Log(_L("Closing <%d> QoS Channel(s)"), iQoSSuite->iQoSChannel.Count());

	for (TInt i = iQoSSuite->iQoSChannel.Count(); i > 0; i--)
		{
		Log(_L("Close QoS Channel <%d> "),i);
		iQoSSuite->CloseQoSChannelL(i);
		}

	return EPass;	
	}


/* SetUp QoS Parameters
 */
CTS_QoSOperationSection2_3::CTS_QoSOperationSection2_3()
{
	iTestStepName = _L("SetUp_QoS_Parameters_Test2.3");
}

CTS_QoSOperationSection2_3::~CTS_QoSOperationSection2_3()
{
}

enum TVerdict CTS_QoSOperationSection2_3::doTestStepL( void )
	{
	CQoSParameters * parameters;
	TBuf16<12> qosChannelNumber;
	TBuf16<5> qoSParameterRefrence;			// Single QoS Parameter Refrence
	TPtrC16 qoSParametersRefrenceString;	// Multiple QoS Parameter Refrence
	TLex16 theString;
	TInt value = 0, err = 0, parameterPosition = 0;


	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		// If the QoS has not been set on the QoS Channel then set it
		if (iQoSSuite->iQoSSet[i] == 0)
			{
			Log(_L("Setup QoS parameters on the QoS Channel <%d> "), i+1);

			// make up string QoSChannel<i+1> so number of sockets to bind to Qos Channel can be read 
			// from the config file ScenarioX.ini
			qosChannelNumber.Copy(QoSChannel);
			qosChannelNumber.AppendNum(i+1);
					
			TESTL(GetStringFromConfig(qosChannelNumber, _L("QoSParametersRefrence"), qoSParametersRefrenceString));
			// Assign TPtrC16 to a TLex for searching purposes
			theString.Assign(qoSParametersRefrenceString);
			// Get QoS Parameter Refrence and convert from a String to an TInt, Mainly used if the First QoS Parameters get rejected by network 
			// We will then have other QoS Parameters to apply. These are defined in the config file
			while(!theString.Eos())	
				{
				// Skip any white spaces
				theString.SkipSpace();
				// Parse the String and convert to a TInt
				err = theString.Val(value);
				// Two Dimentional Array, Add TInt to an RArray of <TInt>'s within class CMultipleArray
				err = iQoSSuite->iQoSChannel[i]->AppendInteger(value);
				      
				}

			// If we get an Adapt from the network then keep going through all QoS Parameter Refrence(s) 
			// until either no more variations of QoS Parameter Refrences to choose from or we get an Accept or Failure
			while(iQoSSuite->qoSEventOutcome != EQoSEventConfirm && (iQoSSuite->iQoSChannel[i]->Count() != parameterPosition))
				{
				// make up string QoS<1> we know the first setting is at location 0 
				// other QoS Parameter Settings will be used if these Parameter Settings FAIL
				qoSParameterRefrence.Copy(QoS);
				// Multiple array is needed one for the QoS channel number and one for the available Qos Parameter Refrence(s), Located in the config file
				qoSParameterRefrence.AppendNum(iQoSSuite->iQoSChannel[i]->Get(parameterPosition));
				// Store Qos Settings
				SetupQoSChannelParametersL(qoSParameterRefrence, i);
		
				// Register for notification of QoS events
				err = iQoSSuite->iQoSChannel[i]->iQoSChannel->NotifyEvent(*this);
				if (err!=KErrNone)
					{
					Log(_L("The request was not Issued : return value = <%d>"), err);
					User::Leave(err);
					}

				// Create parameters object and push to the stack
				parameters = new (ELeave) CQoSParameters;
				CleanupStack::PushL(parameters);
				// Setup QoS Parameters
				parameters = iQoSSuite->iQoSChannel[i]->SetQoSParametersL();	
				// Setup QoS on the Channel

				TInt err = iQoSSuite->iQoSChannel[i]->iQoSChannel->SetQoS(*parameters);
				if (err!=KErrNone)
					{
					Log(_L("Failed to open QoS channel: return value = <%d>"), err);
					User::Leave(err);
					}

				//
				//											   //
				// Nokia's Wait for QoS Request Implementation //
				//											   // 
				//
				WaitForQoSEvent();

				// If QoS Event Failure then Leave and return Fail
				if (iQoSSuite->qoSEventOutcome == EQoSEventFailure && iQoSSuite->iQoSChannel[i]->Count() == parameterPosition)
					return EFail;
			
				// Increment QoS Parameter Refrence(s) counter
				++parameterPosition;	
				CleanupStack::Pop();	// parameters
				}

			// Reset counter used to count QoS Parameter Refrence(s)
			parameterPosition = 0;  
			// Reset for next QoS Channel, EQoSEventFailure is the only value which can be used else the while loop will not be executed
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;
			}
		}

		return EPass;	
	}

void CTS_QoSOperationSection2_3::Event(const CQoSEventBase& aEvent)
	{
	iReceivedEvent = aEvent.EventType();
//    iEvent = &aEvent;

	switch(aEvent.EventType())
		{
        case EQoSEventFailure:
			reason = ((CQoSFailureEvent*)&aEvent)->Reason();
			// do something, for example inform user of failure
			// Network alters the QoS settings EQoSEventFailure will be delivered to the app 
			Log(_L("Network returned EQoSEventFailure : return value = <%d>"), reason);
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;

		case EQoSEventConfirm:
			// QoS setting have been accepted by the application wheather it is requested QoS or Altering QoS, which the application has accepted
			Log(_L("Network returned EQoSEventConfirm, Start to transmit data"));			
			// Indicate that the QoS has been set on the Channel, set to True
			iQoSSuite->iQoSSet[i] = 1;
			// Start to send and receive data 
			TRAPD(err ,iQoSSuite->iQoSChannel[i]->SendAndRecvL(0));
			if(err!=KErrNone)
				{
				iQoSSuite->Log(_L("Failed to send / receive data, error code <%d>"), err);
				//User::Leave(err);
				}

			iQoSSuite->qoSEventOutcome = EQoSEventConfirm;
	
			break;

		// catch rest of the needed events here.
        default:
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;
		}
	}

//iAdaptor=True
//� The set of QoS parameters specified in CQoSPolicy will be requested from the Network.
//� Assuming there is no error, then a PFQOS_EVENT_QOS_CONFIRM indication will be delivered to the 
//  application, along with the QoS parameters returned by the Network.  These parameters may be 
//  below those requested by the application.  It will be up to the application to decide whether 
//  the final QoS parameters are acceptable or not, and whether to continue with the connection or 
//  terminate it.
//� If the Network alters the QoS on an established flow, then a PFQOS_EVENT_QOS_ADAPT indication 
//  will be delivered to the application, along with the new QoS parameters.

//iAdaptor=False
//� The set of QoS parameters specified in CQoSPolicy will be requested from the Network.
//� If the QoS parameters were not downgraded, then a PFQOS_EVENT_QOS_CONFIRM indication will be delivered to 
//  the application, along with the QoS parameters returned by the Network.
//� If the QoS parameters were downgraded, then a PFQOS_EVENT_QOS_FAILURE indication will be delivered to the 
//  application, along with the QoS parameters returned by the Network.
//� If the Network downgrades the QoS on an established flow, then a PFQOS_EVENT_QOS_FAILURE indication will 
//  be delivered to the application.


/* Leave QoS Channel
 * detach the socket from the QoS channel
 */
CTS_QoSOperationSection2_4::CTS_QoSOperationSection2_4()
{
	iTestStepName = _L("Leave_QoS_Channel_Test2.4");
}

CTS_QoSOperationSection2_4::~CTS_QoSOperationSection2_4()
{
}

enum TVerdict CTS_QoSOperationSection2_4::doTestStepL( void )
	{
	TBuf16<12> qosChannelNumber;
//	RSocket * sock;

	RArray<TInt> nSocketsLeaveInt;
	TPtrC16 nSocketsLeaveString;
	TLex16 theString;
	TInt value = 0, err = 0;

	CleanupClosePushL(nSocketsLeaveInt);
		
	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		// check to see if Leave has been done on the QoS Channel, if not then do it
		if (iQoSSuite->iLeave[i] == 0)
			{
			// make up string QoSChannel<i+1> so number of sockets to bind to Qos Channel can be read 
			// from the config file ScenarioX.ini
			qosChannelNumber.Copy(QoSChannel);
			qosChannelNumber.AppendNum(i+1);

			// Get String from ScenarioX.ini, Number of the Sockets to leave for a QoS Channel 
			TESTL(GetStringFromConfig(qosChannelNumber, _L("leave"), nSocketsLeaveString));

			// Assign TPtrC16 to a TLex for searching purposes
			theString.Assign(nSocketsLeaveString);
			
			// Get Socket number(s) and convert from a String to an TInt 
			while(!theString.Eos())	
				{
				// Skip any white spaces
				theString.SkipSpace();
				// Parse the String and convert to a TInt
				err = theString.Val(value);
				// Add TInt to an RArray of <TInt>'s
				err = nSocketsLeaveInt.Append(value);
				if (err!=KErrNone)
					{
					Log(_L("Failed to insert value into nSockLeaveInt RArray : return value = <%d>"), err);
					User::Leave(err);
					}
				}

			// Get number of elements in the array
			TInt length = nSocketsLeaveInt.Count();
			// Socket leaves from QoS Channel, Get socket number as entered in config file
			TInt counter = 0;
			for (i2 = nSocketsLeaveInt[0]; i2 <= nSocketsLeaveInt[length-1]; i2++)
				{
				if (nSocketsLeaveInt[counter] != 0)
					{
					Log(_L("Socket <%d> Leave's QoS Channel <%d>"), i2, i+1);

					// Register for notification of QoS events
					err = iQoSSuite->iQoSChannel[i]->iQoSChannel->NotifyEvent(*this);
					if (err!=KErrNone)
						{
						Log(_L("The request was not issued : return value = <%d>"), err);
						User::Leave(err);
						}

					// Leave QoS Channel
					TInt err = iQoSSuite->iQoSChannel[i]->iQoSChannel->Leave(*iQoSSuite->iQoSChannel[i]->GetSocketHandle(i2-1));

					if (err!=KErrNone)
						{
						Log(_L("Failed to open QoS channel: return value = <%d>"), err);
						User::Leave(err);
						}

					//
					//											   //
					// Nokia's Wait for QoS Request Implementation //
					//											   // 
					//
					WaitForQoSEvent();

					// If QoS Event Failure then Leave and return Fail
					if (iQoSSuite->qoSEventOutcome == EQoSEventFailure)
						return EFail;
					}
				++counter;
				}

			// Close Leave Sockets
			for (i2 = nSocketsLeaveInt[0]; i2 <= nSocketsLeaveInt[length-1]; i2++)
				iQoSSuite->iQoSChannel[i]->CloseSocket(i2);
			}
		}
	CleanupStack::PopAndDestroy();	

	return EPass;	
	}

void CTS_QoSOperationSection2_4::Event(const CQoSEventBase& aEvent)
	{
	TInt reason;
	iReceivedEvent = aEvent.EventType();
//    iEvent = &aEvent;

	switch(aEvent.EventType())
		{
		case EQoSEventLeave:
			reason = ((CQoSLeaveEvent*)&aEvent)->Reason();
			Log(_L("Network returned EQoSEventLeave : return value = <%d>"), reason);
			
			// Set Leave to true, so we know we have done a Leave on this QoS Channel
			iQoSSuite->iLeave[i] = 1;
			iQoSSuite->qoSEventOutcome = EQoSEventLeave;	

			break;

        // catch rest of the needed events here.
        default:
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;

		}
	}


/*
 * Initialize Join QoS Channel
 */
CTS_QoSOperationSection2_9::CTS_QoSOperationSection2_9()
{
	iTestStepName = _L("Initialize_Join_QoS_Channel_2.9");
}

CTS_QoSOperationSection2_9::~CTS_QoSOperationSection2_9()
{
}

enum TVerdict CTS_QoSOperationSection2_9::doTestStepL( void )
	{
	TBuf16<12> qosChannelNumber;
	TPtrC16 nSocketsExtraJoinString;			// number of sockets to join per each QoS channel
	TLex16 theString;
	TInt err = 0, value = 0;

	/*
	 * Loop through number of QoS Channel(s)
	 */
	
	for (TInt i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		// Make up string QoSChannel<i+1> so number of sockets to join to Qos Channel can be read 
		// from the config file ScenarioX.ini
		qosChannelNumber.Copy(QoSChannel);
		qosChannelNumber.AppendNum(i+1);
		
		TESTL(GetIntFromConfig(qosChannelNumber, _L("extraJoinImplicitTcp"), value));
		err = iQoSSuite->nExtraJoinImplicitTcpArray.Insert(value, i);
		if (err!=KErrNone)
			{
			Log(_L("Failed to insert value into nExtraJoinImplicitTcp RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		TESTL(GetIntFromConfig(qosChannelNumber, _L("extraJoinExplicitTcp"), value));
		err = iQoSSuite->nExtraJoinExplicitTcpArray.Insert(value, i);
		if (err!=KErrNone)
			{
			Log(_L("Failed to insert value into nExtraJoinExplicitTcp RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		TESTL(GetIntFromConfig(qosChannelNumber, _L("extraJoinImplicitUdp"), value));
		err = iQoSSuite->nExtraJoinImplicitUdpArray.Insert(value, i);
		if (err!=KErrNone)
			{
			Log(_L("Failed to insert value into nExtraJoinImplicitUdp RArray : return value = <%d>"), err);
			User::Leave(err);
			}
		TESTL(GetIntFromConfig(qosChannelNumber, _L("extraJoinExplicitUdp"), value));
		err = iQoSSuite->nExtraJoinExplicitUdpArray.Insert(value, i);
		if (err!=KErrNone)
			{
			Log(_L("Failed to insert value into nExtraJoinExplicitUdp RArray : return value = <%d>"), err);
			User::Leave(err);
			}

		/*
		 * Parse String, Socket number(s) to Join the QoS channel
		 */
		// Socket number(s) to join the QoS channel
		TESTL(GetStringFromConfig(qosChannelNumber, _L("join"), nSocketsExtraJoinString));
		// Assign TPtrC16 to a TLex for searching purposes
		theString.Assign(nSocketsExtraJoinString);
		// Get Socket number(s) and convert from a String to an TInt 
		while(!theString.Eos())	
			{
			// Skip any white spaces
			theString.SkipSpace();
			// Parse the String and convert to a TInt
			err = theString.Val(value);
			// Add TInt to an RArray of <TInt>'s
			err = iQoSSuite->iQoSChannel[i]->nSocketsExtraJoinInt.Append(value);
			if (err!=KErrNone)
				{
				Log(_L("Failed to insert value into nSocketExtraJoinInt RArray : return value = <%d>"), err);
				User::Leave(err);
				}
			}
		}

	return EPass;
	}


/* Join the QoS Channel
 */
CTS_QoSOperationSection2_5::CTS_QoSOperationSection2_5()
{
/* store the name of this test case
 * this is the name that is used by the script file
 */
	iTestStepName = _L("Join_QoS_Channel_Test2.5");
}

CTS_QoSOperationSection2_5::~CTS_QoSOperationSection2_5()
{
}

enum TVerdict CTS_QoSOperationSection2_5::doTestStepL( void )
	{
	// number of extra socket(s) to join the QoS channel(s)		
//	RSocket * sock;
	TInt extraSocksToJoin = 0, err;

	/*
	 * Loop through number of QoS Channel(s)
	 */
	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{

		// check to see if Join has been done on the QoS Channel, if not then do it,
		if (iQoSSuite->iJoin[i] == 0)
			{	
			/*
			 * Open new Implicit / Explicit, Tcp / Udp Sockets, Bind, Connect
			 */
			// Extra Implicit sockets plus current number of sockets
			iQoSSuite->nExtraJoinImplicitTcpArray[i] += iQoSSuite->iQoSChannel[i]->GetSocketListCount();
			for (extraSocksToJoin = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); extraSocksToJoin < iQoSSuite->nExtraJoinImplicitTcpArray[i]; extraSocksToJoin++)
				{
				// Set-up connection prefrences
				// Will be used by test steps to setup the connections - Protocol, QoS Channel Number, Socket number
				SetupConnectionPrefrencesL(KProtocolInetTcp, i, extraSocksToJoin-1);
				// Open Implicit Tcp Socket
				iQoSSuite->iQoSChannel[i]->OpenImplicitTcpSocketL(iQoSSuite->iSocketServer);
				// Connect will perform a Bind() so we dont need to do
				iQoSSuite->iQoSChannel[i]->ConnectL(extraSocksToJoin);
				}

			// Extra Explicit sockets plus current number of sockets
			iQoSSuite->nExtraJoinExplicitTcpArray[i] += iQoSSuite->iQoSChannel[i]->GetSocketListCount();
			for (extraSocksToJoin = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); extraSocksToJoin < iQoSSuite->nExtraJoinExplicitTcpArray[i]; extraSocksToJoin++)
				{
				// Set-up connection prefrences
				// Will be used by test steps to setup the connections - Protocol, QoS Channel Number, Socket number
				SetupConnectionPrefrencesL(KProtocolInetTcp, i, extraSocksToJoin-1);
				// Open Explicit Tcp Socket
				iQoSSuite->iQoSChannel[i]->OpenExplicitTcpSocketL(extraSocksToJoin, iQoSSuite->iSocketServer);
				// Connect will perform a Bind() so we dont need to do
				iQoSSuite->iQoSChannel[i]->ConnectL(extraSocksToJoin);
				}

			iQoSSuite->nExtraJoinImplicitUdpArray[i] += iQoSSuite->iQoSChannel[i]->GetSocketListCount();
			for (extraSocksToJoin = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); extraSocksToJoin < iQoSSuite->nExtraJoinImplicitUdpArray[i]; extraSocksToJoin++)
				{
				// Set-up connection prefrences
				// Will be used by test steps to setup the connections - Protocol, QoS Channel Number, Socket number
				SetupConnectionPrefrencesL(KProtocolInetUdp, i, extraSocksToJoin-1);
				// Open Implicit Udp Socket
				iQoSSuite->iQoSChannel[i]->OpenImplicitUdpSocketL(iQoSSuite->iSocketServer);
				// Bind to local address and make a connection to remote address
				iQoSSuite->iQoSChannel[i]->BindL(extraSocksToJoin);
				iQoSSuite->iQoSChannel[i]->ConnectL(extraSocksToJoin);
				}

			iQoSSuite->nExtraJoinExplicitUdpArray[i] += iQoSSuite->iQoSChannel[i]->GetSocketListCount();
			for (extraSocksToJoin = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); extraSocksToJoin < iQoSSuite->nExtraJoinExplicitUdpArray[i]; extraSocksToJoin++)
				{
				// Set-up connection prefrences
				// Will be used by test steps to setup the connections - Protocol, QoS Channel Number, Socket number
				SetupConnectionPrefrencesL(KProtocolInetUdp, i, extraSocksToJoin-1);
				// Open Explicit Udp Socket
				iQoSSuite->iQoSChannel[i]->OpenExplicitUdpSocketL(extraSocksToJoin, iQoSSuite->iSocketServer);
				// Bind to local address and make a connection to remote address
				iQoSSuite->iQoSChannel[i]->BindL(extraSocksToJoin);
				iQoSSuite->iQoSChannel[i]->ConnectL(extraSocksToJoin);
				}


			/*
			 * Join Socket(s) to QoS Channel(s)
			 */
			TInt counter = 0;
	
			// Get number of elements in the array
			TInt length = iQoSSuite->iQoSChannel[i]->nSocketsExtraJoinInt.Count();
			// Socket(s) join the QoS channel, Get socket number as entered in config file
			for (i2 = iQoSSuite->iQoSChannel[i]->nSocketsExtraJoinInt[0]; i2 <= iQoSSuite->iQoSChannel[i]->nSocketsExtraJoinInt[length-1] ; i2++)
				{
				if (iQoSSuite->iQoSChannel[i]->nSocketsExtraJoinInt[counter] != 0)
				{
				Log(_L("Socket <%d> Join's QoS Channel <%d>"), i2, i+1);	

//				sock = new (ELeave) RSocket;
//				CleanupStack::PushL(sock); 
				
				// Socket number -1 because of array indexing i.e. get socket number 2 in array position 1
//				sock = iQoSSuite->iQoSChannel[i]->GetSocketHandle(i2-1);

				// Register for notification of QoS events
				err = iQoSSuite->iQoSChannel[i]->iQoSChannel->NotifyEvent(*this);
				if (err!=KErrNone)
					{
					Log(_L("The request has not been issued RArray : return value = <%d>"), err);
					User::Leave(err);
					}
				// Join QoS channel, 
				TInt err = iQoSSuite->iQoSChannel[i]->iQoSChannel->Join(*iQoSSuite->iQoSChannel[i]->GetSocketHandle(i2-1));

				if (err!=KErrNone)
					{
					Log(_L("Failed to Join the socket to the QoS channel: return value = <%d>"), err);
					User::Leave(err);
					}

				//
				//											   //
				// Nokia's Wait for QoS Request Implementation //
				//											   // 
				//
				WaitForQoSEvent();

				// If QoS Event Failure then Leave and return Fail
				if (iQoSSuite->qoSEventOutcome == EQoSEventFailure)
					return EFail;

				//  sock
//				CleanupStack::Pop();
				}
				++counter;
				}
			}
		}
	
	// Remove current socket(s) count from array for next use of array
	iQoSSuite->RemoveRArraySocketCount();

	return EPass;
	}

void CTS_QoSOperationSection2_5::Event(const CQoSEventBase& aEvent)
	{
	TInt reason;
	iReceivedEvent = aEvent.EventType();
//    iEvent = &aEvent;

	switch(aEvent.EventType())
		{
		case EQoSEventJoin:
		   	reason = ((CQoSJoinEvent*)&aEvent)->Reason();
			Log(_L("Network returned EQoSEventJoin : return value = <%d>"), reason);
			// Set Join to true, so we know we have done a Join on this QoS Channel
			iQoSSuite->iJoin[i] = 1;
			iQoSSuite->qoSEventOutcome = EQoSEventJoin;
			// Start to send and receive data 
			TRAPD(err ,iQoSSuite->iQoSChannel[i]->SendAndRecvL(i2-1));
			if(err!=KErrNone)
				{
				iQoSSuite->Log(_L("Failed to send / receive data, error code <%d>"), err);
				//User::Leave(err);
				}

			break;

        // catch rest of the needed events here.
        default:
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;
		}
	}


/* Get the QoS Channel Capabilities
 */
CTS_QoSOperationSection2_6::CTS_QoSOperationSection2_6()
{
/* store the name of this test case
 * this is the name that is used by the script file
 */
	iTestStepName = _L("QoS_Channel_Capabilities_Test2.6");
}

CTS_QoSOperationSection2_6::~CTS_QoSOperationSection2_6()
{
}

enum TVerdict CTS_QoSOperationSection2_6::doTestStepL( void )
	{	
	TUint capabilities=1;

	for (TInt i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		TInt err = iQoSSuite->iQoSChannel[i]->iQoSChannel->GetCapabilities(capabilities);

		if (err!=KErrNone)
			{
			Log(_L("Failed to open QoS channel: return value = <%d>"), err);
			User::Leave(err);
			}
		}
	
	return EPass;
	}


/* Change Network QoS Settings
 *
 *  i.e. A scenario for changing networks - 97/98 network to a 99 network And 99 network to a 97/98 network.
 */
CTS_QoSOperationSection2_7::CTS_QoSOperationSection2_7()
{
	iTestStepName = _L("Change_QoS_Parameters_Test2.7");
}

CTS_QoSOperationSection2_7::~CTS_QoSOperationSection2_7()
{
}

enum TVerdict CTS_QoSOperationSection2_7::doTestStepL( void )
	{
	CQoSParameters * parameters;
	TBuf16<12> qosChannelNumber;
	TBuf16<5> qoSParameterRefrence;			// Single QoS Parameter Refrence
	TPtrC16 qoSParametersRefrenceString;	// Multiple QoS Parameter Refrence
	TLex16 theString;
	TInt value = 0, err = 0, parameterPosition = 0;


	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		// If the QoS has not been set on the QoS Channel then set it
		if (iQoSSuite->iQoSChangeSet[i] == 0)
			{
			Log(_L("Open QoS Channel <%d> "), i+1);

			// make up string QoSChannel<i+1> so number of sockets to bind to Qos Channel can be read 
			// from the config file ScenarioX.ini
			qosChannelNumber.Copy(QoSChannel);
			qosChannelNumber.AppendNum(i+1);
					
			TESTL(GetStringFromConfig(qosChannelNumber, _L("ChangeQoSSettingsRefrence"), qoSParametersRefrenceString));
			// Assign TPtrC16 to a TLex for searching purposes
			theString.Assign(qoSParametersRefrenceString);
			// Get QoS Parameter Refrence and convert from a String to an TInt, Mainly used if the First QoS Parameters get rejected by network 
			// We will then have other QoS Parameters to apply. These are defined in the config file
			while(!theString.Eos())	
				{
				// Skip any white spaces
				theString.SkipSpace();
				// Parse the String and convert to a TInt
				err = theString.Val(value);
				// Two Dimentional Array, Add TInt to an RArray of <TInt>'s within class CMultipleArray
				err = iQoSSuite->iQoSChannel[i]->AppendInteger(value);
				      
				}
			// If we get an Adapt from the network then keep going through all QoS Parameter Refrence(s) 
			// until either no more variations of QoS Parameter Refrences to choose from or we get an Accept or Failure
			while(iQoSSuite->qoSEventOutcome != EQoSEventConfirm && (iQoSSuite->iQoSChannel[i]->Count() != parameterPosition))
				{
				// make up string QoS<1> we know the first setting is at location 0 
				// other QoS Parameter Settings will be used if these Parameter Settings FAIL
				qoSParameterRefrence.Copy(QoS);
				// Multiple array is needed one for the QoS channel number and one for the available Qos Parameter Refrence(s), Located in the config file
				qoSParameterRefrence.AppendNum(iQoSSuite->iQoSChannel[i]->Get(parameterPosition));
				// Store Qos Settings

				if (iQoSSuite->iQoSChannel[i]->Get(parameterPosition) != 0)
				{
				SetupQoSChannelParametersL(qoSParameterRefrence, i);
	
				// Register for notification of QoS events
				err = iQoSSuite->iQoSChannel[i]->iQoSChannel->NotifyEvent(*this);
				if (err!=KErrNone)
					{
					Log(_L("The request has not been issued : return value = <%d>"), err);
					User::Leave(err);
					}
				// Create parameters object and push to the stack
				parameters = new (ELeave) CQoSParameters;
				CleanupStack::PushL(parameters);
				// Setup QoS Parameters
				parameters = iQoSSuite->iQoSChannel[i]->SetQoSParametersL();	
				// Setup QoS on the Channel

				TInt err = iQoSSuite->iQoSChannel[i]->iQoSChannel->SetQoS(*parameters);
				if (err!=KErrNone)
					{
					Log(_L("Failed to open QoS channel: return value = <%d>"), err);
					User::Leave(err);
					}

				//
				//											   //
				// Nokia's Wait for QoS Request Implementation //
				//											   // 
				//
				WaitForQoSEvent();

				// If QoS Event Failure then Leave and return Fail
				if (iQoSSuite->qoSEventOutcome == EQoSEventFailure && iQoSSuite->iQoSChannel[i]->Count() == parameterPosition)
					return EFail;
			
				// Increment QoS Parameter Refrence(s) counter
				++parameterPosition;	
				CleanupStack::Pop();	// parameters
				}
				else
					{
					// we dont want to change QoS on this channel so exit
					iQoSSuite->qoSEventOutcome = EQoSEventConfirm;
					}
				}

				// Reset counter used to count QoS Parameter Refrence(s)
				parameterPosition = 0;
				// Reset for next QoS Channel, EQoSEventFailure is the only value which can be used else the while loop will not be executed
				iQoSSuite->qoSEventOutcome = EQoSEventFailure;
			}
		}

	return EPass;	
	}

void CTS_QoSOperationSection2_7::Event(const CQoSEventBase& aEvent)
	{
	iReceivedEvent = aEvent.EventType();
//    iEvent = &aEvent;

	switch(aEvent.EventType())
		{
        case EQoSEventFailure:
			reason = ((CQoSFailureEvent*)&aEvent)->Reason();
			// do something, for example inform user of failure
			// Network alters the QoS settings EQoSEventFailure will be delivered to the app 
			Log(_L("Network returned EQoSEventFailure : return value = <%d>"), reason);
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;

		case EQoSEventConfirm:
			// QoS setting have been accepted by the application wheather it is requested QoS or Altering QoS, which the application has accepted
			Log(_L("Network returned EQoSEventConfirm, Start to transmit data"));			
			// Indicate that the QoS has been set on the Channel, set to True
			iQoSSuite->iQoSChangeSet[i] = 1;
			iQoSSuite->qoSEventOutcome = EQoSEventConfirm;
			// Start to send and receive data 
			for (i2 = 0; i2 < iQoSSuite->iQoSChannel[i]->GetSocketListCount();i2++)
				{
				TRAPD(err ,iQoSSuite->iQoSChannel[i]->SendAndRecvL(i2));
				if(err!=KErrNone)
					{
					iQoSSuite->Log(_L("Failed to send / receive data, error code <%d>"), err);
					//User::Leave(err);
					}
				}
	
			break;

		// catch rest of the needed events here.
        default:
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;
		}
	}

//iAdaptor=True
//� The set of QoS parameters specified in CQoSPolicy will be requested from the Network.
//� Assuming there is no error, then a PFQOS_EVENT_QOS_CONFIRM indication will be delivered to the 
//  application, along with the QoS parameters returned by the Network.  These parameters may be 
//  below those requested by the application.  It will be up to the application to decide whether 
//  the final QoS parameters are acceptable or not, and whether to continue with the connection or 
//  terminate it.
//� If the Network alters the QoS on an established flow, then a PFQOS_EVENT_QOS_ADAPT indication 
//  will be delivered to the application, along with the new QoS parameters.

//iAdaptor=False
//� The set of QoS parameters specified in CQoSPolicy will be requested from the Network.
//� If the QoS parameters were not downgraded, then a PFQOS_EVENT_QOS_CONFIRM indication will be delivered to 
//  the application, along with the QoS parameters returned by the Network.
//� If the QoS parameters were downgraded, then a PFQOS_EVENT_QOS_FAILURE indication will be delivered to the 
//  application, along with the QoS parameters returned by the Network.
//� If the Network downgrades the QoS on an established flow, then a PFQOS_EVENT_QOS_FAILURE indication will 
//  be delivered to the application.


/* Add further QoS Channel(s)
 */

/* Open CMultipleArray Objects
 */
CTS_QoSOperationSection2_8::CTS_QoSOperationSection2_8()
{
	iTestStepName = _L("Initialize_Open_Extra_QoS_Channels_2.8");
}

CTS_QoSOperationSection2_8::~CTS_QoSOperationSection2_8()
{
}

enum TVerdict CTS_QoSOperationSection2_8::doTestStepL( void )
	{
	// Get Current Number of QoS Channels opened
	iQoSSuite->beginQoSChannelCount = iQoSSuite->iQoSChannel.Count();

	iQoSSuite->nExtraJoinImplicitTcp=0; iQoSSuite->nExtraJoinExplicitTcp=0;
	iQoSSuite->nExtraJoinImplicitUdp=0; iQoSSuite->nExtraJoinExplicitUdp=0;

	TESTL(GetIntFromConfig(_L("Test_2.8"), _L("extraJoinImplicitTcp"), iQoSSuite->nExtraJoinImplicitTcp));
	TESTL(GetIntFromConfig(_L("Test_2.8"), _L("extraJoinExplicitTcp"), iQoSSuite->nExtraJoinExplicitTcp));
	TESTL(GetIntFromConfig(_L("Test_2.8"), _L("extraJoinImplicitUdp"), iQoSSuite->nExtraJoinImplicitUdp));
	TESTL(GetIntFromConfig(_L("Test_2.8"), _L("extraJoinExplicitUdp"), iQoSSuite->nExtraJoinExplicitUdp));
	
	TInt totalExtraJoinSockets = iQoSSuite->nExtraJoinImplicitTcp + iQoSSuite->nExtraJoinExplicitTcp + iQoSSuite->nExtraJoinImplicitUdp + iQoSSuite->nExtraJoinExplicitUdp;
	
	for (TInt i = 0; i < totalExtraJoinSockets; i++)
		{
		// Create new pointer of type CMultipleArray
		CMultipleArray* myCMultipleArray = CMultipleArray::NewLC();
		iQoSSuite->iQoSChannel.Append(myCMultipleArray);
		CleanupStack::Pop();	// myCMultipleArray
		iQoSSuite->iQoSChannel[iQoSSuite->iQoSChannel.Count()-1]->Initialize(this);
		}

	return EPass;
	}


// Network Drops QoS Channel
//
CTS_QoSOperationSection2_10::CTS_QoSOperationSection2_10()
{
	iTestStepName = _L("Network_Drops_QoS_Channel_Test2.10");
}

CTS_QoSOperationSection2_10::~CTS_QoSOperationSection2_10()
{
}

enum TVerdict CTS_QoSOperationSection2_10::doTestStepL( void )
	{
	TInt err = 0;

	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		// Register for notification of QoS events
		err = iQoSSuite->iQoSChannel[i]->iQoSChannel->NotifyEvent(*this);
		if (err!=KErrNone)
			{
			Log(_L("The request was not Issued : return value = <%d>"), err);
			User::Leave(err);
			}
			
		//
		//											   //
		// Nokia's Wait for QoS Request Implementation //
		//											   // 
		//
		WaitForQoSEvent();

		// If default then Leave and return Fail
		if (iQoSSuite->qoSEventOutcome == EQoSEventFailure)
			return EFail;
		}

	return EPass;	
	}

void CTS_QoSOperationSection2_10::Event(const CQoSEventBase& aEvent)
	{
	iReceivedEvent = aEvent.EventType();
//    iEvent = &aEvent;

	switch(aEvent.EventType())
		{
        case EQoSEventFailure:
			reason = ((CQoSFailureEvent*)&aEvent)->Reason();
			// do something, for example inform user of failure
			// Network alters the QoS settings EQoSEventFailure will be delivered to the app 
			Log(_L("Context has been dropped close context"));	

			// Explicit opening of sockets
			if (iQoSSuite->iQoSChannel[i]->GetConnectionListCount() > 0)
				{
				iQoSSuite->Log(_L("Closing <%d> connection(s)"), iQoSSuite->iQoSChannel[i]->GetConnectionListCount());
				for (i2 = iQoSSuite->iQoSChannel[i]->GetConnectionListCount(); i2 > 0; i2--)
					{
					iQoSSuite->Log(_L("Close connection <%d>"),i2);
					iQoSSuite->iQoSChannel[i]->CloseConnection(i2);
					}
				}
			// if any sockets have been left open then close them
			if (iQoSSuite->iQoSChannel[i]->GetSocketListCount() > 0)
				{
				// Close any Open Sockets
				iQoSSuite->Log(_L("Closing<%d> socket(s)"), iQoSSuite->iQoSChannel[i]->GetSocketListCount());
				for (i2  = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); i2 > 0; i2--)
					{
					iQoSSuite->Log(_L("Close socket <%d>"), i2);
					iQoSSuite->iQoSChannel[i]->CloseSocket(i2);
					}
				}					
			Log(_L("Close QoS Channel <%d>"), i);
			
			TRAPD(err ,iQoSSuite->CloseQoSChannelL(i));
			if(err!=KErrNone)
				{
				iQoSSuite->Log(_L("Failed to close QoS channel, error code <%d>"), err);
				User::Leave(err);
				}

            break;
       
	   case EQoSEventAdapt:
			reason = ((CQoSAdaptEvent*)&aEvent)->Reason();
			// Network alters the QoS settings EQoSEventAdapt will be delivered to the app and the new QoS settings
			Log(_L("Context has been dropped close context"));	
		
			// Explicit opening of sockets
			if (iQoSSuite->iQoSChannel[i]->GetConnectionListCount() > 0)
				{
				iQoSSuite->Log(_L("Closing <%d> connection(s)"), iQoSSuite->iQoSChannel[i]->GetConnectionListCount());
				for (i2 = iQoSSuite->iQoSChannel[i]->GetConnectionListCount(); i2 > 0; i2--)
					{
					iQoSSuite->Log(_L("Close connection <%d>"),i2);
					iQoSSuite->iQoSChannel[i]->CloseConnection(i2);
					}
				}
			// if any sockets have been left open then close them
			if (iQoSSuite->iQoSChannel[i]->GetSocketListCount() > 0)
				{
				// Close any Open Sockets
				iQoSSuite->Log(_L("Closing<%d> socket(s)"), iQoSSuite->iQoSChannel[i]->GetSocketListCount());
				for (i2  = iQoSSuite->iQoSChannel[i]->GetSocketListCount(); i2 > 0; i2--)
					{
					iQoSSuite->Log(_L("Close socket <%d>"), i2);
					iQoSSuite->iQoSChannel[i]->CloseSocket(i2);
					}
				}					
			Log(_L("Close QoS Channel <%d>"), i);
			iQoSSuite->CloseQoSChannelL(i);
			
			break;

		// catch rest of the needed events here.
        default:
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;
		}
	}

// Network Changes QoS Settings
//
CTS_QoSOperationSection2_11::CTS_QoSOperationSection2_11()
{
	iTestStepName = _L("Network_Changes_QoS_Settings_Test2.11");
}

CTS_QoSOperationSection2_11::~CTS_QoSOperationSection2_11()
{
}

enum TVerdict CTS_QoSOperationSection2_11::doTestStepL( void )
	{
	TInt err = 0;

	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		// Register for notification of QoS events
		err = iQoSSuite->iQoSChannel[i]->iQoSChannel->NotifyEvent(*this);
		if (err!=KErrNone)
			{
			Log(_L("The request was not Issued : return value = <%d>"), err);
			User::Leave(err);
			}
			
		//
		//											   //
		// Nokia's Wait for QoS Request Implementation //
		//											   // 
		//
		WaitForQoSEvent();

		// If default then Leave and return Fail
		if (iQoSSuite->qoSEventOutcome == EQoSEventFailure)
			return EFail;
		}

	return EPass;	
	}

void CTS_QoSOperationSection2_11::Event(const CQoSEventBase& aEvent)
	{
	iReceivedEvent = aEvent.EventType();
//    iEvent = &aEvent;

	switch(aEvent.EventType())
		{
        case EQoSEventFailure:
			reason = ((CQoSFailureEvent*)&aEvent)->Reason();
			Log(_L("The QoS has been changed by the network"));	
			// Start to send and receive data 
			for (i2 = 0; i2 < iQoSSuite->iQoSChannel[i]->GetSocketListCount();i2++)
					{
					TRAPD(err ,iQoSSuite->iQoSChannel[i]->SendAndRecvL(i2));
					if(err!=KErrNone)
						{
						iQoSSuite->Log(_L("Failed to send / receive data, error code <%d>"), err);
						//User::Leave(err);
						}
					}	

            break;

	   case EQoSEventAdapt:
			reason = ((CQoSAdaptEvent*)&aEvent)->Reason();
			Log(_L("The QoS has been changed by the network"));	
			// Start to send and receive data 
			for (i2 = 0; i2 < iQoSSuite->iQoSChannel[i]->GetSocketListCount();i2++)
				{
				TRAPD(err ,iQoSSuite->iQoSChannel[i]->SendAndRecvL(i2));
				if(err!=KErrNone)
					{
					iQoSSuite->Log(_L("Failed to send / receive data, error code <%d>"), err);
					//User::Leave(err);
					}
				}
						
			break;

		// catch rest of the needed events here.
        default:
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;

            break;
		}
	}


// Verify that the QoS parameters set, were set correctly 
// This is done by setting the QoS parameters from the app, 
// then getting the Simtsy (simulated network) to send the QoS parameters back, 
// and to verify what was set in the app is what we get back from the Simtsy (simulated network). 
CTS_QoSOperationSection2_12::CTS_QoSOperationSection2_12()
{
	iTestStepName = _L("Verify_QoS_Settings_Test2.12");
}

CTS_QoSOperationSection2_12::~CTS_QoSOperationSection2_12()
{
}

enum TVerdict CTS_QoSOperationSection2_12::doTestStepL( void )
	{
	TInt err = 0;

	for (i = 0; i < iQoSSuite->iQoSChannel.Count(); i++)
		{
		// Register for notification of QoS events
		err = iQoSSuite->iQoSChannel[i]->iQoSChannel->NotifyEvent(*this);
		if (err!=KErrNone)
			{
			Log(_L("The request was not Issued : return value = <%d>"), err);
			User::Leave(err);
			}
			
		//
		//											   //
		// Nokia's Wait for QoS Request Implementation //
		//											   // 
		//
		WaitForQoSEvent();

		// The test has failed with EQoSEventFailure or Default or the QoS parameters are incorrect
		if (iQoSSuite->qoSEventOutcome == EQoSEventFailure)
			return EFail;
		}

	return EPass;
}

void CTS_QoSOperationSection2_12::Event(const CQoSEventBase& aEvent)
	{
	iReceivedEvent = aEvent.EventType();

	switch(aEvent.EventType())
		{
        case EQoSEventFailure:
			reason = ((CQoSFailureEvent*)&aEvent)->Reason();
			// do something, for example inform user of failure
			Log(_L("Network returned EQoSEventFailure : return value = <%d>"), reason);
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;
            break;

		case EQoSEventConfirm:
			// QoS setting have been accepted by the application wheather it is requested QoS or Altering QoS, which the application has accepted
			Log(_L("Network returned EQoSEventConfirm, Start to transmit data"));			
			
			// 1. Get current QoS Parameters set in the app 
			// Done - These are stored in iQoSSuite -> iQoSChannel [x] (QoS Channel Number)

			// 2. Get current QoS Parameters that has been set on the network	
			CQoSParameters *iQoSNetworkParameters;

			TRAP_IGNORE(iQoSNetworkParameters = new (ELeave) CQoSParameters);
			if (iQosNetworkParameters)
				{
				CleanupStack::PushL(iQoSNetworkParameters);
	
				iQoSNetworkParameters->CopyL(*((CQoSChannelEvent*)&aEvent)->Parameters());

				// Not Done - 3. Compare QoS app Parameters with QoS network Parameters
				// Done     - 4. Exit with Pass or Fail
				if (iQoSSuite->iQoSChannel[i]->CompareQoSParameters(iQoSNetworkParameters))	// pass network QoS paramaters
					{	
					// Start to send and receive data 
					TRAPD(err ,iQoSSuite->iQoSChannel[i]->SendAndRecvL(0));
					if(err!=KErrNone)
						{
						iQoSSuite->Log(_L("Failed to send / receive data, error code <%d>"), err);
						//User::Leave(err);
						}
					iQoSSuite->qoSEventOutcome = EQoSEventConfirm;
					}
				else
					{
					// The QoS parameters set by the app are not the same as the current network parameters
					// So the test has failed, QoS is set but incorrectly
					iQoSSuite->qoSEventOutcome = EQoSEventFailure;
					}

				CleanupStack::Pop();	// iQoSNetworkParameters
				}
			else
				{
					iQoSSuite->qoSEventOutcome = EQoSEventFailure;
				}
			break;

		// catch rest of the needed events here.
        default:
			iQoSSuite->qoSEventOutcome = EQoSEventFailure;
            break;
		}
	}
