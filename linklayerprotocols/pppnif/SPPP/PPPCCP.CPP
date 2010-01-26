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

#include "PPPLOG.H"
#include <in_sock.h>
#include "PPPCCP.H"
#include "PPP_VER.H"
#include "CCPINI.H"
#include <commsdattypeinfov1_1.h>
#include "PPPConfig.h"

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
	{
	Deregister();
	iRecvr.Deregister();
	}

CPppCcp::~CPppCcp()
	{
	CObjectCon* con;
	TInt i;

	Deregister();
	iRecvr.Deregister();
	delete iSendCallBack;
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
		iPppLcp->PppNewCompressor(NULL);
		iCompressor->Close();
		iCompressor = NULL;
		}

	if (iDeCompressor)
		{
		iPppLcp->PppNewDeCompressor(NULL);
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

	iSendQ.Free();

	if (!iRequestWorkList.IsEmpty())
		{
		iRequestWorkList.Free();
		}
	
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
//
//
//
	{
	//
	// Increment the Reset Id
	//
	iResetId++;
	iResetId %= 256;
	}

void CPppCcp::ConstructL()
// Construct the Link Protocol Object
// This method now reads the number of compressor and its information
// incrementally. Replaces ini file code.
	{
	Register();
	iRecvr.Register();
	
	TCallBack scb(SendCallBack, this);	
	iSendCallBack = new(ELeave) CAsyncCallBack(scb, KCcpSendPriority);

	// Initialise CCP Finite State Machine
	FsmConstructL();

	// Initialise a queue 
	iCompressorConfig.SetOffset(_FOFF(CPppCompConfig,link));
  	iIniFilePtr = CPppIniData::NewL();

	// Initialise CObject containers
	iCompressorCon = (iPppLcp->ContainerForDlls())->CreateL();
	iDeCompressorCon = (iPppLcp->ContainerForDlls())->CreateL();

	//
	// Be warned this code was written expecting the ini file only to be used
	// for compressor configuration, it's not generic
	//
	CPppCompConfig* CompressorData;
	TBool			Loop;
	TUint			numberOfTurns=0;
	do 
		{
		Loop = FALSE;
		numberOfTurns++;
		CompressorData = CPppCompConfig::NewL();
		//coverity[leave_without_push]
		//Function ReadCompressorInfo is not leaveable, hence no need to push the object CompressorData into cleanup stack
		if (iIniFilePtr->ReadCompressorInfo(CompressorData, numberOfTurns))
			{
			//
			// Check there is a dll of this name
			//
			//coverity[leave_without_push]
			//Function ReadCompressorInfo is not leaveable, hence no need to push the object CompressorData into cleanup stack
			if (iPppLcp->DoesDllExist(CompressorData->Name()))
				{
				iCompressorConfig.AddLast(*CompressorData);
				}
			else
				{
				delete CompressorData;
				}

			Loop = TRUE;
			}
		}
	while (Loop);

	delete CompressorData;

	FsmOpen();
	}


void CPppCcp::SendResetRequestL()
//
// Create and Send a Reset Request
//
	{
	const TUint		pktLen = 4;
	RMBufPacket		pkt;
	RMBufPktInfo*	info = NULL;
	pkt.AllocL(pktLen);
	CleanupStack::PushL(pkt);
	info = pkt.NewInfoL();
	CleanupStack::Pop();
	info->iLength = pktLen;	

	// Construct packet header
	TUint8* ptr = pkt.First()->Ptr();
	*ptr++ = KPppCcpResetReq;
	*ptr++ = iResetId;
	BigEndian::Put16(ptr, (TUint16)pktLen);

	// Send it
	pkt.Pack();
	SendFrame(pkt);
	}

void CPppCcp::Info(TNifIfInfo& aInfo) const
//
//
//
	{
	FillInInfo(aInfo);
	}

void CPppCcp::FillInInfo(TNifIfInfo& aInfo)
//
//
//
	{

	aInfo.iVersion = TVersion(KPPPMajorVersionNumber, KPPPMinorVersionNumber, KPPPBuildVersionNumber);
	aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfCreatedByLink;
   _LIT(KCcp, "ccp");
	aInfo.iName = KCcp;
	aInfo.iProtocolSupported=KPppIdEsock;
	}

TInt CPppCcp::State()
//
//
//
	{

	if (FsmIsThisLayerOpen())
        return (iUpperFlowOn) ? EIfUp : EIfBusy;
	else
		return EIfDown;
	}

void CPppCcp::SendFlowOn()
	{ 
	LOG( iPppLcp->iLogger->Printf(_L("CCP Flow On iUpperFlowOn=%d\n"), iUpperFlowOn); )	

	iLowerFlowOn = ETrue;
	if (!iSendQ.IsEmpty())
		iSendCallBack->CallBack();
	}

TInt CPppCcp::SendCallBack(TAny* aCProtocol)
	{
	((CPppCcp*)aCProtocol)->DoSend();
	return 0;
	}

void CPppCcp::DoSend()
	{
	if (FsmIsThisLayerOpen())
		{
		RMBufPacket pkt;
		while (iSendQ.Remove(pkt))
			{
			RMBufPktInfo*info = pkt.Unpack();
			TPppAddr addr;
			info->iDstAddr = addr;
			pkt.Pack();
			if (iRecvr.SendFrame(pkt)<=0)
				{
				LOG( iPppLcp->iLogger->Printf(_L("CCP Flow Off\n")); )	
				iLowerFlowOn = EFalse;
				break;
				}
			}
		if (iLowerFlowOn&&!iUpperFlowOn)
			iUpperFlowOn = ETrue;
		}
	}

TInt CPppCcp::Send(RMBufChain& aPacket, TAny*)
//
//
//
	{
	iSendQ.Append(aPacket);
	iSendCallBack->CallBack();
	if (!FsmIsThisLayerOpen() || !iLowerFlowOn)
		iUpperFlowOn = EFalse;
	return iUpperFlowOn;
	}

//
// NCP Upcalls from FSM
//

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
	SendFlowOn();
	return;
	}

void CPppCcp::FsmLayerDown(TInt aReason)
//
// Hmmm the link has gone down 
//
	{

	if(aReason==KErrNone)
		return;
	//
	// Make a note of which compressor we were using
	//
	}
	
void CPppCcp::FsmFillinConfigRequestL(RPppOptionList& aRequestList)
//
// Fillin Config Request to be sent
//
	{
	if ((iSoftwareCompressionOn = iPppLcp->GetLcpConfig()->GetEnableSwComp()) == TRUE)
		{
		TSglQueIter<CPppCompConfig> Iterator(iCompressorConfig);
		Iterator.SetToFirst();

		//
		// This is the initial Config Request.
		// We then just want to fill in the 1:st option from the ini-file
		//
	
		AddToRequestListL(aRequestList,Iterator);
		//
		// Add _all_ supported to a temporary working list
		//
		if (!iRequestWorkList.IsEmpty())
		iRequestWorkList.Free();	
		Iterator.SetToFirst();
		do 
			{
			AddToRequestListL(iRequestWorkList,Iterator);
			}
		while(Iterator);
		}
	}

void CPppCcp::AddToRequestListL(RPppOptionList& aRequestList, TSglQueIter<CPppCompConfig>& aIterator)
	{
	
	CPppCompConfig* Data = aIterator++;
	if (Data)
		{
		//
		// Add this option to the list
		//
		if (Data->OptionsLength())
			{
			TPtrC8 Temp = Data->Options();
			aRequestList.CreateAndAddL(Data->ID(), Temp);
			}
		else
			{
			//
			// No options
			//
			aRequestList.CreateAndAddL(Data->ID());
			}
		}
	}
	
void CPppCcp::FsmCheckConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList)
//
// Check options in a recvd config request
//
	{

	// Check that software compression is turned on.
	// This may not occur as LCP also reads the database
	// and sends a Protocol Reject for CCP in this case
	if(!iSoftwareCompressionOn)
		{
		// Reject all options
		aRequestList.RemoveOptions(aRejList);
		return;
		}

	RPppOption requestedOption;
	CPppCompConfig* nakOption = NULL;
	CPppCompConfig* supportedOption = NULL;
	TSglQueIter<CPppCompConfig> supportedOptionsIter(iCompressorConfig);
	
	// This loop control variable ensures that after we have successfully matched
	// an algorithm we don't keep searching through supportedOptionsIter
	TBool optionMatched = EFalse;

	// If we have matched the option to an algorithm that we support, do we support the 
	// parameters of the algorithm in the request?
	TBool paramsMatched = EFalse;

	// Used as a ptr to the parameters of requestedOption during param match
	TPtrC8 requestedParams;
	
	// An acknowledgement should only contain one algorithm - the one that we want to accept.
	// Whilst processing, add only the first algorithm that we can acknowledge to aAckList.
	// Successive algorithms that are found to be acceptable are added to the reject list.
	TBool ackListFull = EFalse;

	//
	// Process all options in the config-request
	//
	while (aRequestList.Remove(requestedOption))
		{

		// Search supported compressor data for a match
		supportedOptionsIter.SetToFirst();
		optionMatched = EFalse;
		nakOption = NULL;

		do
			{
			supportedOption = supportedOptionsIter++;
			if (!supportedOption)
				continue;

			// Is this algorithm supported?
			if (supportedOption->ID() == requestedOption.OptType())
				{
				// Found an algorithm match - now match the parameters.
				paramsMatched = EFalse;
				requestedParams.Set(requestedOption.ValuePtr(), requestedOption.ValueLength());
				
				if (supportedOption->Options() == requestedParams)
					paramsMatched = ETrue;

				// Do the algorithm parameters match the ones that we use?
				if(paramsMatched)
					{
					// The parameters matched so add this option to the ack list,
					// or if we already have an option that we want to Ack, reject this option
					if (!ackListFull)
						{
						aAckList.Append(requestedOption);
						ackListFull = ETrue;
						}
					else
						aRejList.Append(requestedOption);
					
					// return program flow to the outer while loop
					// because we don't need to continue searching
					optionMatched = ETrue;
					}
				else
					{
					// This algorithm is a potential nak, so keep a pointer to it.
					// It cannot be added to the nak list yet, because
					// there may be a better match later on in the iterator.
					// However, we only want to keep a pointer to the first potential
					// NAK that we find - to preserve the priority implied by the order 
					// of compressors in the ini file.
					if (nakOption==NULL)
						nakOption = supportedOption;
					}
				}

			} while (supportedOptionsIter && !optionMatched);

		// Finished searching through supported compressor config
		if (!optionMatched)
			{
			// Decide whether to Nak or Reject
			if (nakOption==NULL)
				{
				// Reject this algorithm as it is not recognised
				aRejList.Append(requestedOption);
				}
			else
				{
				// Nak this algorithm as it is recognised, but the parameters are not supported
				// Modify requestedOption to contain the params from nakOption and add requestedOption to aNakList
				TRAPD(ret, requestedOption.SetL(requestedOption.OptType(), (nakOption->Options()).Ptr(), nakOption->OptionsLength()));
				if (ret == KErrNone)
					{
					aNakList.Append(requestedOption);
					}
				else
					{
					// Could not modify the RPppOption - probably not enough memory
					// Rejecting this option may not be appropriate, but it is probably better than sending a Nak with
					// the same parameters as the received request - this would result in an endless loop
					LOG( iPppLcp->iLogger->Printf(_L("Could not construct the Config-Nak (error %d) - Rejecting this option instead"), ret); )
					aRejList.Append(requestedOption);					
					}
				}
			}
		
		}
	// Finished processing the options in the request

	return;
	}

void CPppCcp::FrameError()

	{
	return;
	}


void CPppCcp::KillProtocol()
	{
	/*
	*	OK now this protocol has been rejected
	*	so throw away all frames received by it.
	*/
	iDead = ETrue;
	SetState(EPppFsmClosed);
    return;
	}


void CPppCcp::FsmApplyConfigRequest(RPppOptionList& aRequestList)
//
// Apply options in a recvd config request (that was ACK'd)
//
	{
	TMBufPktQIter iter(aRequestList);
	TSglQueIter<CPppCompConfig> Iterator(iCompressorConfig);
	CPppCompConfig* Data = 0;
	// Used as a ptr to the parameters of requestedOption during param match
	TPtrC8 requestedParams;
	RPppOption opt;
	TBool compressorFound = false ;

	while (opt = iter++, !opt.IsEmpty())
		{
		Iterator.SetToFirst();

		//
		// Is the option type one we support
		//
		do 
			{
			Data = Iterator++;
			if (Data)
				{
				//
				// Add this option to the list
				//
				if ( Data->ID()== opt.OptType() )
					{
					// Can we match the parameters
					requestedParams.Set(opt.ValuePtr(), opt.ValueLength());
					if (Data->Options() == requestedParams)
						{
						compressorFound = true ;
#ifdef _DEBUG
						LOG(iPppLcp->iLogger->Printf(_L("Attempting to load Compressor DLL\n"));)
						switch (opt.OptType())
							{
							case 0x11:
								LOG(iPppLcp->iLogger->Printf(_L("STAC LZS compression requested\n"));)
								LOG(iPppLcp->iLogger->Printf(_L("STAC LZS parameters = 0x%04x 0x%02x\n"),
								                   ((requestedParams[0]<<8) | requestedParams[1]), requestedParams[2]);)
								break ;

							case 0x12:
								LOG(iPppLcp->iLogger->Printf(_L("M$ PPC/PPE compression requested\n"));)
								LOG(iPppLcp->iLogger->Printf(_L("M$ PPC/PPE parameters = 0x%02x 0x%02x 0x%02x 0x%02xr\n"),
								                      requestedParams[0], requestedParams[1],
													  requestedParams[2], requestedParams[3]);)
								break ;

							default:
								LOG(iPppLcp->iLogger->Printf(_L("Unsupported compression algorithm!\n"));)
								break ;
							}
#endif // _DEBUG
					

						//
						// Now we load the compressor based on the file name
						//

						if (iCompressor != NULL)
							{
							iPppLcp->PppNewCompressor(NULL);
							iCompressor->Close();
							iCompressor = NULL;
							}

						TRAPD( Ret, iCompressor = LoadCompressorL(*Data, iPppLcp->MaxTransferSize() ));

						if (Ret == KErrNone)
							{
							iPppLcp->PppNewCompressor(iCompressor);
							LOG(iPppLcp->iLogger->Write(_L("Compressor DLL loaded OK\n"));)
							}
						else
							{
							//
							// Can't load the Dll, only reason I can think off 
							// is not enough memory, so time to close down.
							//
							LOG(iPppLcp->iLogger->Write(_L("Couldn't load Compressor DLL\n"));)
							iPppLcp->Stop(Ret, MNifIfNotify::EDisconnect);
							}
						}
					}
				}
				if (compressorFound) 
					break;
			}
		while(Iterator);
		}
	if (!compressorFound)
		{
		LOG(iPppLcp->iLogger->Write(_L("Couldn't match requested compression settings in ppp.ini\n"));)
		}
    return;
	}

void CPppCcp::FsmRecvConfigAck(RPppOptionList& aReplyList)
//
// Recvd a Config Ack - apply the options
//
	{
	TSglQueIter<CPppCompConfig> Iterator(iCompressorConfig);
	CPppCompConfig* Data = 0;

	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	
	while (opt = iter++, !opt.IsEmpty())
		{
		Iterator.SetToFirst();

		//
		// Is the option type one we support
		//
		do 
			{
			Data = Iterator++;
			if (Data)
				{
				//
				// Add this option to the list
				//
				if ( Data->ID()== opt.OptType() )
					{
					LOG(iPppLcp->iLogger->Printf(_L("New DeCompressor\n"));)

					//
					// Dynamically load the DeCompressor
					//
					if (iDeCompressor != NULL)
						{
						iPppLcp->PppNewDeCompressor(NULL);
						iDeCompressor->Close();
						iDeCompressor = NULL;
						}

					TRAPD(	Ret, iDeCompressor = LoadDeCompressorL(*Data, 
							iPppLcp->MaxTransferSize()));

					if (Ret == KErrNone)
						{
						iPppLcp->PppNewDeCompressor(iDeCompressor);
						}
					else
						{
						//
						// Can't load the Dll, only reason I can think off 
						// is not enough memory, so time to close down.
						//
						iPppLcp->Stop(Ret, MNifIfNotify::EDisconnect);
						}

					break;
					}
				}
			}
		while (Iterator);
		}

	LOG(iPppLcp->iLogger->Write(_L("(Outgoing Compression On)\n"));)

	return;
	}

void CPppCcp::FsmRecvConfigNak(RPppOptionList& /*aReplyList*/, RPppOptionList& aReqList)
//
// Recvd a Config Nak - The associated original request is in aReqList
//
	{
	
	TMBufPktQIter iterReq(aReqList);
	RPppOption optReq;
	optReq = iterReq++;
	
	TMBufPktQIter iterWork(iRequestWorkList);
	RPppOption optWork;
	
	//
	// Find First option of the requested type and delete it from the iRequestWorkList
	//
	while (optWork = iterWork.Current(), !optWork.IsEmpty())
		{
		if(optWork.OptType()==optReq.OptType())
			{
			optWork.Init(); //??
			iterWork.Remove(optWork);
			optWork.Free();
			break;
			}
		iterWork++;
		}
	
	//
	// Add next option of the requested type, if it doesn't exist add a new optiontype
	//
	iterWork.SetToFirst();

	TBool noOptions = EFalse;
	while (optWork = iterWork++, !optWork.IsEmpty())
		{
		if(optWork.OptType()==optReq.OptType())
			{
			if (!aReqList.IsEmpty())
				aReqList.Free();
			//PG 09/11/2001
			//Following needs to be changed to use ReplaceOptions so as not to create new memory
			//The whole method needs to be rewritten
			TRAPD(err,aReqList.CreateAndAddL(optWork.OptType(), optWork.ValuePtr(), optWork.ValueLength()));
			__ASSERT_DEBUG(err == KErrNone,PppPanic(EPppPanic_PPPNoMemory));
			if(err)
				return;
			noOptions = ETrue;
			break;
			}		
		}
	//
	// No more options where found try to add next option in turn
	//
	if(noOptions == EFalse)
		{
		//
		// Clear the List...
		//
		if (!aReqList.IsEmpty())
			aReqList.Free();

		//
		// ...and put next option in the list that we support(taken from 
		// the options left in the iRequestWorkList)
		//
		iterWork.SetToFirst();

		if(optWork = iterWork++, !optWork.IsEmpty())
			{
			TRAPD(err,aReqList.CreateAndAddL(optWork.OptType(), optWork.ValuePtr(), optWork.ValueLength()));	
			__ASSERT_DEBUG(err == KErrNone,PppPanic(EPppPanic_PPPNoMemory));
			err = err; //remove remark in UREL
			}	
		}
	}

void CPppCcp::FsmRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList)
//
// Recvd a Config Reject - The associated original request is in aReqList
//
	{
//
	// Remove the unsupported compressor(s) from the Temporary iRequestWorkList
	//
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		//
		// Remove _all_ unsupported options
		//
		TInt error = KErrNone;
		while(error == KErrNone)
			{
			error = iRequestWorkList.RemoveOption(opt);
			}
		}
	
	//
	// Clear the Request List (aReqList)...
	//
	if (!aReqList.IsEmpty())
		aReqList.Free();

	//
	// ...and put next option in the list that we support(taken from 
	// the options left in the iRequestWorkList)
	//
	TMBufPktQIter iterWork(iRequestWorkList);
	RPppOption optWork;

	if(optWork = iterWork++, !optWork.IsEmpty())
		{
		TRAPD(err,aReqList.CreateAndAddL(optWork.OptType(), optWork.ValuePtr(), optWork.ValueLength()));
		__ASSERT_DEBUG(err==KErrNone,PppPanic(EPppPanic_PPPNoMemory));
		err = err; //Remove remark in UREL
		}
	}

void CPppCcp::SendResetAckL(TUint8 aId)
	{
	const TUint		pktLen = 4;
	RMBufPacket		pkt;
	RMBufPktInfo*	info = NULL;
	pkt.AllocL(pktLen);
	CleanupStack::PushL(pkt);
	info = pkt.NewInfoL();
	CleanupStack::Pop();
	info->iLength = pktLen;

	// Construct packet header
	TUint8* ptr = pkt.First()->Ptr();
	*ptr++ = KPppCcpResetAck;
	*ptr++ = aId;
	BigEndian::Put16(ptr, (TUint16)pktLen);

	// Send it
	pkt.Pack();
	SendFrame(pkt);

	//
	// Not all compressors use the Ack, so I have a generic function
	// to complete the Reset
	//
	RemoteCompressorHasReset();
	}

TBool CPppCcp::FsmRecvUnknownCode(TUint8 aCode, TUint8 aId, TInt /*aLength*/, RMBufChain& aPacket)
//
// Recvd an unrecognised opcode - No we have two opcodes that no-one else is 
// interested in 
//
	{
	TBool RetCode;

	switch (aCode)
		{
		case KPppCcpResetReq:
			{
			//
			// Tell the compressor about this, and send an ACK to the other end
			//
 			//PG none of the currrently existing compressors use the packet passed as parameter
 			// Following line caused a memory leak when the packet len is 0
 			// aPacket.TrimStart(4);

			//
			// Some compressors require a Reset ACK some don't
			//
			if (iCompressor)
				{
//				if (iCompressor->ResetCompressor(aLength-4, aPacket) == TRUE)
				if (iCompressor->ResetCompressor(0, aPacket) == TRUE)
					{
					TRAPD(Ret,SendResetAckL(aId));
					if (KErrNone!=Ret)
						{
						LOG( iPppLcp->iLogger->Printf(_L("ERROR:\tSendResetAck Failed\n")); )
						}
					}
				}

			RetCode = ETrue;
			break;
			}
		case KPppCcpResetAck:
			{
//			aPacket.TrimStart(4);
			if (aId == iResetId)
				{
//				iDeCompressor->ResetDecompressor(aLength-4, aPacket);
				iDeCompressor->ResetDecompressor(0, aPacket);
				RetCode = ETrue;
				}
			else
				{
				RetCode = EFalse;
				}
			break;
			}
		default:
			{
			RetCode = EFalse;
			break;
			}
		}
	return RetCode;
	}

//


void CPppCcp::Recv(RMBufChain& aPacket)
	{
	aPacket.Free();
	return;
	}

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

void CPppCcp::ReConfigLink()
// Called from compressor/decompressor to force send of Config Request which
// "downs" CCP. CCP then comes "up" again when the server replies with Config Ack.
// Has been tested using predictor.
// Currently only required by Predictor decompressor - RFC 1978
	{
	FsmOpen();
	}

void CPppCcp::UnloadCompressor() 
{
	if (iCompressor != NULL)
		{
		LOG( iPppLcp->iLogger->Printf(_L("Unloading PPP compressor\n")); )	
		iPppLcp->PppNewCompressor(NULL);
		iCompressor->Close();
		iCompressor = NULL;
		}
}
