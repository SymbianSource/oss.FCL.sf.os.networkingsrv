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
// dummy PPPFSM.CPP
// 
//

#include "NCPIP.H" // for KPppIdIpcp
//#include <es_ini.h>

//
// PPP State machine
//

MPppFsm::MPppFsm(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId)
	: MPppRecvr(aPppLcp, aPhase, aPppId),__iFsmName(NULL),iCurrentId(0),iTerminateId(0),
	iRequestId(0),iRequestList(),iMaxRestartConfig(0),iRestartCount(0),iState(EPppFsmStopped),iWaitTimeConfig(0),
	iWaitTime(0),iConsecCfgReq(0),iLastCfgReqFcs(0),iMaxFailureConfig(0),iMaxFailureCount(0),iLengthenTimers(EFalse)
	{
	}

MPppFsm::~MPppFsm()
	{
	}

void MPppFsm::FsmConstructL()
	{
	}

void MPppFsm::TerminateLink()
	{
	}

//
// Open/Close calls from higher level protocols
//

TInt MPppFsm::FsmOpen()
	{
	return ETrue;
	}

void MPppFsm::FsmClose(TInt )
	{
	}

void MPppFsm::FsmAbort(TInt )
//
// 29.11.00.  Changed slighty so that this does the same as RXJ- in the PPP RFC for all
// states.
//
	{
	}



//
// Upcall from Timer
//

void MPppFsm::TimerComplete(TInt /*aStatus*/)
	{
	}

//
// Upcalls from Recvr
//

void MPppFsm::LowerLayerUp()
	{
	}


void MPppFsm::LowerLayerDown(TInt )
	{
	}

void MPppFsm::FrameError()
	{
	}

void MPppFsm::KillProtocol()
	{
	//
	// This came to light as a result of the CCP work
	// This happens is a protocol is told to shut down,
	// I'm not sure how to handle it?? Do we just Stop it???
	//
	}

TBool MPppFsm::RecvFrame(RMBufChain& )
	{
	return EFalse;
	}
	
//
// Rest of PPP
//

TBool MPppFsm::FsmRecvUnknownCode(TUint8 /*aCode*/, TUint8 /*aId*/, TInt /*aLength*/, RMBufChain& /*aPacket*/)
//
// Default implementation
//
	{
	return EFalse;
	}

void MPppFsm::ProcessReject(TUint8 , TUint8 /*aId*/, TInt /* aLength */, RMBufChain& )
//
// If the reject rejects a code (or protocol) that should
// be known terminate layer.
// (Note, As the behaviour of protocol reject is the same
// as code reject, this fn handles protocol reject for LCP)
//
	{
	}
	
void MPppFsm::ProcessTerminate(TUint8 , TUint8 , TInt /*aLength*/, RMBufChain& /*aPacket*/)
	{
	}

TBool MPppFsm::ProcessEmptyConfigReq()
//
// Handle ConfigRequest with no options
//
	{
	return ETrue;
	}

void MPppFsm::ProcessConfig(TUint8 , TUint8 /*aId*/, TInt /* aLength */, RMBufChain& )
//
// Handle ConfigRequest, ConfigAck, ConfigNak and ConfigReject
//
	{
	}

void MPppFsm::SendConfigRequest()
//
// Send the config request in iRequestList
//
	{
	}


TInt MPppFsm::InitialiseConfigRequest()
//
// Delete exist request list, create a new one
// Initialise counters for sending configs requests
//
	{
	return 0;
	}

void MPppFsm::SendInitialConfigRequest()
//
// Initialise request list and send a request
//
	{
	}

TBool MPppFsm::FsmAckOptionsValid(RPppOptionList& /*aList*/, RPppOptionList& /*aRequestList*/)
/**
Perform validation checking on the option list of a ConfigAck or ConfigReject.

@param aList option list of incoming ConfigAck or ConfigReject
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}
TBool MPppFsm::FsmRejectOptionsValid(RPppOptionList& /*aList*/, RPppOptionList& /*aRequestList*/)
/**
Perform validation checking on the option list of a ConfigAck or ConfigReject.

@param aList option list of incoming ConfigAck or ConfigReject
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}

TBool MPppFsm::FsmConfigRequestOptionsValid(RPppOptionList& /*aList*/)
/**
Perform validation checking on the option list of a ConfigRequest.

@param aList option list of incoming ConfigRequest
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}


