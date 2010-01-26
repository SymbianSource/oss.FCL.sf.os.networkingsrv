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
// PPPCCP.CPP
// 
//

#include <in_sock.h>
#include "pppCcp.h"

#include "CCPINI.H"

#if defined(__VC32__) && (_MSC_VER < 1300)
 #define PMF(x) x
#else
 #define PMF(x) &x
#endif

//
// PPP for ESock
//
// warning C4355: 'this' : used in base member initializer list
#pragma warning (disable:4355)
CPppCcp::CPppCcp(CPppLcp* aLcp)
	:   CBase(), MPppFsm(aLcp, EPppPhaseNetwork, KPppIdCcp),
	iRecvr(this, PMF(CPppCcp::Recv), PMF(CPppCcp::SendFlowOn), aLcp, EPppPhaseNetwork, KPppIdCcp, 
		PMF(MPppRecvr::FrameError), PMF(MPppRecvr::KillProtocol))
	{
	__DECLARE_FSM_NAME(_S("CCP"));
	}
#pragma warning (default:4355)

void	CPppCcp::RemoveRegistration()
	{}

CPppCcp::~CPppCcp()
	{
	CObjectCon* con;
	TInt i;

/*	Deregister();
	iRecvr.Deregister();
	delete iSendCallBack;
	*/
  	delete iIniFilePtr;

	//
	// OK Spin around removing all the Compressor Config information
	//
	CPppCompConfig* CompressorData;

	TSglQueIter<CPppCompConfig> Iterator(iCompressorConfig);
	Iterator.SetToFirst();

	CompressorData = Iterator++;
	while(CompressorData)
	{
		delete CompressorData;
		CompressorData = Iterator++;
	}


	if (iCompressor)
		{
//		iPppLcp->PppNewCompressor(NULL);
		iCompressor->Close();
		iCompressor = NULL;
		}

	if (iDeCompressor)
		{
//		iPppLcp->PppNewDeCompressor(NULL);
		iDeCompressor->Close();
		iDeCompressor = NULL;
		}

	//
	// Delete all the Containers
	//
	if(iCompressorCon)
		{
		con = iCompressorCon;

		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
			iCompressorCon = NULL;
		}


	if(iDeCompressorCon)
		{
		con = iDeCompressorCon;
		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
		iDeCompressorCon = NULL;
		}

	delete iContainerForDlls;
	
	}

CPppCcp* CPppCcp::NewL(CPppLcp* aLcp)
	{
	CPppCcp* ccp = new (ELeave) CPppCcp(aLcp);
	CleanupStack::PushL(ccp);
	ccp->ConstructL();
	CleanupStack::Pop();
	return ccp;
	}
void CPppCcp::FsmTerminationPhaseComplete()
	{
	}



void CPppCcp::RemoteCompressorHasReset()
	{
	}

void CPppCcp::ConstructL()
// Construct the Link Protocol Object
// This method now reads the number of compressor and its information
// incrementally. Replaces ini file code.
	{
/*
	// Initialise CCP Finite State Machine
	FsmConstructL();
*/
	iContainerForDlls = CObjectConIx::NewL();
	// Initialise a queue 
	iCompressorConfig.SetOffset(_FOFF(CPppCompConfig,link));
  	iIniFilePtr = CPppIniData::NewL();

	// Initialise CObject containers
	iCompressorCon = iContainerForDlls->CreateL();
	iDeCompressorCon = iContainerForDlls->CreateL();

	//
	// Be warned this code was written expecting the ini file only to be used
	// for compressor configuration, it's not generic
	//
	CPppCompConfig* CompressorData=NULL;
	TBool			Loop;
	TUint			numberOfTurns=0;
	do 
		{
		Loop = FALSE;
		numberOfTurns++;
		CompressorData = CPppCompConfig::NewL();
		if (iIniFilePtr->ReadCompressorInfo(CompressorData, numberOfTurns))
			{
			iCompressorConfig.AddLast(*CompressorData);
			Loop = TRUE;
			}
		}
	while (Loop);

	delete CompressorData;
	CompressorData = NULL;

	}


void CPppCcp::SendResetRequestL()
//
// Create and Send a Reset Request
//
	{
	}

void CPppCcp::Info(TNifIfInfo& /*aInfo*/) const
//
//
//
	{
	}
void CPppCcp::FillInInfo(TNifIfInfo&/* aInfo*/)
//
//
//
	{
	}

TInt CPppCcp::State()
//
//
//
	{
	return 0;
	}
void CPppCcp::SendFlowOn()
	{ 
	}

TInt CPppCcp::Send(RMBufChain& /*aPacket*/, TAny*)
	{
	return 0;
	}
TInt CPppCcp::FsmLayerStarted()
//
// Open the layer below.  Nothing to do here as this won't be worth running without an NCP!
//
	{
	return KErrNone;
	}

void CPppCcp::FsmLayerFinished(TInt /*aReason*/)
//
// Close the layer below.  CCP should not close the link if it terminates, so there is nothing
// to do here.
//
	{}

void CPppCcp::FsmLayerUp()
//
// Hmm the link is back up.
//
	{
	return;
	}

void CPppCcp::FsmLayerDown(TInt /*aReason*/)
//
// Hmmm the link has gone down 
//
	{
	}
	
void CPppCcp::FsmFillinConfigRequestL(RPppOptionList& /*aRequestList*/)
//
// Fillin Config Request to be sent
//
	{
	}
void CPppCcp::FsmCheckConfigRequest(RPppOptionList& , RPppOptionList& , RPppOptionList& , RPppOptionList& )
	{}
void CPppCcp::FrameError()
	{
	}

void CPppCcp::KillProtocol()
	{
	}
void CPppCcp::FsmApplyConfigRequest(RPppOptionList& )
	{}
void CPppCcp::FsmRecvConfigAck(RPppOptionList& )
	{}
void CPppCcp::FsmRecvConfigNak(RPppOptionList& /*aReplyList*/, RPppOptionList& )
	{}
void CPppCcp::FsmRecvConfigReject(RPppOptionList& , RPppOptionList& )
	{}
TBool CPppCcp::FsmRecvUnknownCode(TUint8 , TUint8 , TInt /*aLength*/, RMBufChain& /*aPacket*/)
	{
	return ETrue;
	}

TBool CPppCcp::FsmOptionsValid(RPppOptionList& /*aList*/, RPppOptionList& /*aRequestList*/)
/**
Perform validation checking on the option list of a ConfigAck or ConfigReject.

@param aList option list of incoming ConfigAck or ConfigReject
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}

TBool CPppCcp::FsmConfigRequestOptionsValid(RPppOptionList& /*aList*/)
/**
Perform validation checking on the option list of a ConfigRequest.

@param aList option list of incoming ConfigRequest
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}

void CPppCcp::Recv(RMBufChain& /*aPacket*/)
	{}
void CPppCcp::ReConfigLink()
	{}

CPppDeCompressor* CPppCcp::LoadDeCompressorL(	CPppCompConfig& aPPPCompConfig,
												TInt aMaxFrameLength)
	{
	CPppCompFactory* Factory=NULL;
	CPppDeCompressor*	DeCompressor;

#ifdef _UNICODE
	Factory = (CPppCompFactory*)FindPppFactoryL(aPPPCompConfig.Name(), TUid::Uid(KUidUnicodePppCompressionModule), *iDeCompressorCon);
#else
	Factory = (CPppCompFactory*)FindPppFactoryL(aPPPCompConfig.Name(), TUid::Uid(KUidPppCompressionModule), *iDeCompressorCon);
#endif

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Factory));
	
	if(aPPPCompConfig.OptionsLength())
		DeCompressor = Factory->NewPppDeCompressorL(this, aMaxFrameLength,&aPPPCompConfig.Options()[0]);
	else
		DeCompressor = Factory->NewPppDeCompressorL(this, aMaxFrameLength);

	CleanupStack::PopAndDestroy(); // Close extra reference on Factory

	return DeCompressor;
	}

CPppCompressor* CPppCcp::LoadCompressorL( CPppCompConfig& aPPPCompConfig,
												TInt aMaxFrameLength)
	{
	CPppCompFactory* Factory=NULL;
	CPppCompressor*	Compressor;

#ifdef _UNICODE										 
	Factory = (CPppCompFactory*)FindPppFactoryL(aPPPCompConfig.Name(), TUid::Uid(KUidUnicodePppCompressionModule), *iCompressorCon);
#else
Factory = (CPppCompFactory*)FindPppFactoryL(aPPPCompConfig.Name(), TUid::Uid(KUidPppCompressionModule), *iCompressorCon);
#endif

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Factory));

	if(aPPPCompConfig.OptionsLength())
		Compressor = Factory->NewPppCompressorL(this, aMaxFrameLength,&aPPPCompConfig.Options()[0]);
	else
		Compressor = Factory->NewPppCompressorL(this, aMaxFrameLength);

	CleanupStack::PopAndDestroy(); // Close extra reference on Factory

	return Compressor;
	}

