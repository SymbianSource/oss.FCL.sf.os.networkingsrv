// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This module contains the code for the dummy PPP Nif class which communicates with the upper PPP Nif
// via PPP "proxy"
// 
//

#include <comms-infras/nifif.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <comms-infras/commsdebugutility.h>
#include <connectprog.h>
#include <in_chk.h>
#include <metadatabase.h>
#include <commsdattypeinfov1_1.h>

//New ppp progress states file
#include "PppProg.h"

#include "dummypppnif.h"
#include "DummyPPPNifVar.h"


const TInt KLoWatMark = 2;
const TInt KHiWatMark = 6;


// Internal flag bits used  
const TUint KDummyPPPSendBusy = 0x00001000;
const TUint KDummyPPPApplyPending = 0x00010000;

//only for generating the logs
_LIT(KCodeTextString,"	%s [0x%02x]");
_LIT(KCodeTextWithoutSpacesString,"	%s [0x%02x]");
_LIT(KIdentifierLengthString,"	Id = 0x%02x, Len = %d");
_LIT(KLengthString,"	Length = %d");
_LIT(KLengthString2,"	    Length = 0x%02x");
_LIT(KTCPLengthString,"	    Length = %d, Hdr len = %d");
_LIT(KUDPLengthPortString,"	    Length = %d, Src port = %d, Dst port = %d");
_LIT(KBytesRemainingString,"        %d byte%s remaining");
_LIT(KLcpCodeString,"	Code = %s [0x%02x]");
_LIT(KCodeString,"	Code = 0x%02x");
_LIT(KCodeString2,"	Code = 0x%04x");
_LIT(KProtocolString,"	Protocol = 0x%04x");
_LIT(KNumberString,"	Number = 0x%08x");
_LIT(KSecondsString,"	Seconds = %d");
_LIT(KSizeString,"	    Size = %d");
_LIT(KMapString,"	    Map = 0x%08x");
_LIT(KProtocolTextString,"	    Protocol = %s [0x%04x]");
_LIT(KLcpOptFcsTypeString,"	    Types =%s%s%s");
_LIT(KMaximumBytesString,"	    Maximum bytes = %d");
_LIT(KUserNameString,"	Username = \"%S\"");
_LIT(KPasswordString,"	Password = \"%S\"");
_LIT(KMessageString,"	Message = \"%S\"");
_LIT(KDelayString,"	    Delay=%d\n");
_LIT(KNameString,"	Name = \"%S\"");
_LIT(KChangeMaskString,"	Change Mask = 0x%x: ");
_LIT(KChecksumString,"	Checksum = 0x%x");
_LIT(KChecksumString3,"	    Chksum = 0x%04x (0x%04x) !!!");
_LIT(KConnectionString,"	    Connection = 0x%x");
_LIT(KHdrLengthString,"	Length = %d, Hdr len = %d");
_LIT(KIDFragmentString,"	Id = 0x%04x, Fragment = %d %s%s%s");
_LIT(KSrcDstAddrString,"	Src = %d.%d.%d.%d, Dst = %d.%d.%d.%d");
_LIT(KIpv6SrcAddress,"    Src = %x:%x:%x:%x:%x:%x:%x:%x");
_LIT(KIpv6DstAddress,"    Dst = %x:%x:%x:%x:%x:%x:%x:%x");
_LIT(KIpv6Class,"    Class = %d");
_LIT(KIpv6FlowLabel,"    FlowLabel = %d");
_LIT(KIpv6PayloadLen,"    Payload = %d words");
_LIT(KIpv6NextHeadType,"    Next Header type is [%d]");
_LIT(KIpv6HopLimit,"    Hop Limit is %d");
_LIT(KIpv6UnknownHeadType,"    Unknown next header: [%d]");
_LIT(KConnectionNoString,"	Connection number [0x%02x]");
_LIT(KRemoteAddrString,"	    Remote Address = %d.%d.%d.%d");
_LIT(KOurAddrString,"	    Our Address = %d.%d.%d.%d");
_LIT(KMaxSlotString,"	      Max Slot Id = %d");
_LIT(KCompSlotString,"	      Comp Slot Id = %d");
_LIT(KAddrString,"	    Address = %d.%d.%d.%d");
_LIT(KPortString,"	    Src port = %d, Dst port = %d");
_LIT(KWindowUrgentString,"	    Window = %d, Urgent = %d");
_LIT(KSeqAckString,"	    Seq = 0x%08x, Ack = 0x%08x");
_LIT(KFlagsString,"	    Flags = %s%s%s%s%s%s");

#define EIGHT_SPACE_MARGIN			_S("        ")
#define FOURTEEN_SPACE_MARGIN		_S("              ")

/*
 * This sections defines a whole load of constants etc... not very exciting
 */
#ifdef __FLOG_ACTIVE
_LIT8(KDummyPppLogFolder	,"dummypppnif");
_LIT8(KDummyPppLogFile, "dummypppnif.txt");
#endif // #ifdef __FLOG_ACTIVE

_LIT(KEndOfLine, "\n");

/*
 * The Link class
 */

CDummyPPPLink::CDummyPPPLink(CNifIfFactory& aFactory)
	: CNifIfLink(aFactory)
	{
	CDummyNifLog::Printf(_L("Dummy PPP Link Created\r\n"));
	}

CDummyPPPLink::~CDummyPPPLink()
	{
	CDummyNifLog::Printf(_L("Dummy PPP Link Destroyed\r\n"));
	TimerDelete();
	iNifIf->End();
	iNifIf->CloseLog();
	}
 
void CDummyPPPLink::OpenL()
//
// Start connect establishment
//
	{
	switch (iState)
		{
	case EDummyPPPLinkConnecting:
	case EDummyPPPLinkOpen:
		return;
	case EDummyPPPLinkClosed:
		iState = EDummyPPPLinkConnecting;
		break;
	default:
		return;
		}
	CDummyNifLog::Printf(_L("Dummy PPP LinkOpen\r\n"));
	}

void CDummyPPPLink::LinkDown(TInt /*aStatus*/)
//
// Link down - drop out of packet mode
// and notify reason
//
	{
	CDummyNifLog::Printf(_L("Dummy PPP LinkDown\r\n"));
	iState = EDummyPPPLinkClosed;
	}
 
TInt CDummyPPPLink::Send(RMBufChain& aPdu, TAny* aSource)
	{
	CDummyNifLog::Printf(_L("Dummy PPP LinkSend\r\n"));

	return iNifIf->Send(aPdu, aSource);
	}
TInt CDummyPPPIf::Send(RMBufChain& aPacket, TAny* )
{
	
	//To send over serial port to NTRAS
	//return Send(aPacket, iDummyPPPId);

	CDummyNifLog::Write(_L("DummyPPP::RecvdfromPPP\n"));
	CDummyNifLog::Printf(_L("Dummy PPP Send\r\n"));
	logUser->Dump(aPacket, KDummyPPPRecvChannel);

	//Added to send to PPP interface directly
	if(!ProcessPPPPacket(aPacket))
	{
		if(SwitchIPHeaders(*ptrTransPktPPP))
		{
			SendtoPPP(*ptrTransPktPPP);
		}

	}
	return iSendFlowOn;
}

TInt CDummyPPPIf::SwitchIPHeaders(RMBufChain& aPacket)
{
	CDummyNifLog::Printf(_L("DummyPPP::SwitchIPHeaders\r\n"));
	TInt iResult = 0;
	// this received data has already been looped back...
	// get the ip header from the RMBufChain
	TInet6HeaderIP4* ip4 = (TInet6HeaderIP4*) aPacket.First()->Next()->Ptr();
	// get the udp header as well - assume only udp traffic here
	TInet6HeaderUDP* udp = (TInet6HeaderUDP*) ip4->EndPtr();

	TUint dstPort = udp->DstPort();
	if(KDummyPppNifCmdPort == dstPort)
	{	
		// let's use the first payload byte as the command byte
		if(*(udp->EndPtr())== KForceDisconnect)
		{
			CDummyNifLog::Printf(_L("DummyPPP::Sending LinkLayerDown signal\r\n"));
			// do some action
			iNotify->IfProgress(KLinkLayerClosed, KErrCommsLineFail);
			iNotify->LinkLayerDown(KErrCommsLineFail, MNifIfNotify::EDisconnect);
		}
	}
	else
	{

		// update the headers (addresses, checksums etc) to UpdateHeaders(ip4, udp);
		// swap over the destination and source addresses
		TUint32 tempSrc,tempDest;
		tempSrc = ip4->SrcAddr();
		tempDest = ip4->DstAddr();
    
		ip4->SetSrcAddr(tempDest);
		ip4->SetDstAddr(tempSrc);

		TUint tempChksum;
		tempChksum = ip4->Checksum();
		// we've changed the ip hdr so need to recalculate the ip hdr checksum
		ip4->SetChecksum(0); 
		tempChksum = TChecksum::Calculate((TUint16*)ip4, ip4->HeaderLength());
		tempChksum = TChecksum::ComplementedFold(tempChksum);
		ip4->SetChecksum(tempChksum);
		//ip4->SetChecksum(TChecksum::ComplementedFold(TChecksum::Calculate((TUint16*)ip4, ip4->HeaderLength())));

		// also want to set the udp checksum to zero cos it will be wrong now that we have 
		// changed the ip hdr - just set to zero and it is ignored
		udp->SetChecksum(0);
		iResult = 1;
	}
	return iResult;
}
void CDummyPPPIf::SendtoPPP(RMBufChain& aPacket)
{
	CDummyNifLog::Printf(_L("Dummy PPP SendtoPPP\r\n"));
	iSendNumBufs += aPacket.NumBufs();
	iLink->iSendQ.Append(aPacket);

	switch (iLink->iState)
		{
	case EDummyPPPLinkConnecting:
		aPacket.Free();
		break;
	case EDummyPPPLinkOpen:
		if (iSendFlowOn && iSendNumBufs>=iSendHiWat)
			{
			iSendFlowOn = EFalse;
			CDummyNifLog::Printf(_L("DummyPPP::SendtoPPP() Flow off, iSendNumBufs %d"), iSendNumBufs);
			}
		else if (!(iFlags & KDummyPPPSendBusy))
			DoSendtoPPP();
		break;
	case EDummyPPPLinkClosed:
		{
		TRAPD(err, iLink->OpenL());
		aPacket.Free();
		}
		break;
	default:
		aPacket.Free();
		break;
		}
	
	iSendCallBack->CallBack();
}

TInt CDummyPPPIf::SendCallBack(TAny* aCProtocol)
	{
	((CDummyPPPIf*)aCProtocol)->DoSendtoPPP();
	return 0;
	}
void CDummyPPPIf::DoSendtoPPP()
{
	if (iFlags & KDummyPPPSendBusy)
		return;
	
    if (ipktRecvPPP.IsEmpty())
	{
   		RMBufPacket pkt;
		if (!iLink->iSendQ.Remove(pkt))
		{
			iSendNumBufs=0; // Must be so
			return;
		}
	
		CDummyNifLog::Write(_L("DummyPPP::DoSendtoPPP()\n"));
		logUser->Dump(pkt, KDummyPPPSendChannel);

		// Get next packet from send queue
		do
			{
			iFlags |= KDummyPPPSendBusy;
			iSendNumBufs -= pkt.NumBufs();
			iProtocol->Process(pkt, (CProtocolBase*)this);

			// the following Free() may not actually do anything if the packet has already been freed.
			pkt.Free();

			if (!iSendFlowOn && iSendNumBufs<iSendLoWat)
				{
				CDummyNifLog::Printf(_L("DummyPPP::DoSendtoPPP(): Flow on, iSendNumBufs %d"), iSendNumBufs);
				iSendFlowOn = ETrue;
				//for CppLcp's LinkFlowOn()
				iProtocol->StartSending(iProtocol);
				}
			} while (iLink->iSendQ.Remove(pkt));
		iFlags &= ~KDummyPPPSendBusy;
	}
}

TInt CDummyPPPIf::ProcessPPPPacket(RMBufChain& aPacket)
/**
 * Processes all packets received from PPP
 *
 * @param RMBufChain& aPacket
 * @returns nonzero if completed task
 * @returns 0 if need to send it to PPP
 */
{
	// Check if Handle ConfigRequest, ConfigAck, ConfigNak 
	//not responding to ConfigReject
	CDummyNifLog::Printf(_L("Dummy PPP ProcessPPPPacket\r\n"));
    TUint result = 0;

	result = DecodePacket(aPacket);
	if(result == 1) 
	{	if(iCodeRec == KPppLcpConfigReject)
		{
			//Panic as DpppLink does not respond to protocol reject
		}
		else
		{
			if(((iCodeRec == KPppLcpConfigAck)
				||(iCodeRec == KPppLcpConfigNak)
				||(iCodeRec == KPppLcpConfigRequest))
				&&(iDummyPPPState<EDummyPPPFsmOpened))
			{
				ProcessConfig();
			}
			else
			{
				ProcessPAPandIPCP();
			}
		}
		ipktRecvPPP.Free();
	}
    if(result == 0)
	{
		//Added to initialize the SendPPP index	
		ipktRecvPPP.Pack();
		ptrTransPktPPP = &ipktRecvPPP;
		SetupSendPPP();
	}
	return result;
}

TInt CDummyPPPIf::DecodePacket(RMBufChain& aPacket)
/**
 * Decodes RMBufChain received from PPP
 *
 * @param RMBufChain& aPacket
 * @returns 1 if need processing further
 * @returns 0 if need to send it to PPP
 * @returns 2 if packet is discarded
 */
{
	TInt result = 0;
	
	// N.B. ipktRecvPPP must be freed at the end of the previous receive / send sequence.
	// Freeing it here merely covers for a defect where it is not freed as expected.
	// Doubly freeing RMBufChain does not result in error. 
	ipktRecvPPP.Free();
	
	// If ipktRecvPPP not Freed, Assign Panics.
	ipktRecvPPP.Assign(aPacket);
	ipktRecvPPP.Unpack();
	RMBufPktInfo* info = ipktRecvPPP.Info();

	CDummyNifLog::Printf(_L("Dummy PPP DecodePacket\r\n"));
	// Extract and drop LCP header
	ipktRecvPPP.Align(4);
	TUint8* ptr = ipktRecvPPP.First()->Ptr();
	iCodeRec = *ptr++;
	iIdRec = *ptr++;
	iLenRec = BigEndian::Get16(ptr);
	
	// Check packet length is OK
	if (info->iLength<iLenRec || info->iLength<4 || info->iLength!=iLenRec)
	{
		// Too short!
		ipktRecvPPP.Free();
		return result;
	}
	else 
	{
		if (info->iLength>iLenRec)
			ipktRecvPPP.TrimEnd(iLenRec);
	}

	if(iCodeRec == KPppLcpConfigReject)
	{
		//not responding to protocol reject
		return result;
	}

	// If op is unknown
	switch (iCodeRec)
		{
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
		// Filter out bad acks
		if (iDummyPPPState>=EDummyPPPFsmReqSent && iIdRec!=iDummyPPPRequestId)
			break;
		else
			iDummyPPPRequestId |= KPppRequestIdAnswered;
		// fall through ...
	case KPppLcpConfigRequest:
		// Option negotiation ops
		// Split the MBuf chain in separate options

		//if(len<=4)
		if(iLenRec<4)
			break;
		
		//
		// Hmmm sometimes we receive a Config request of Length 4
		// i.e. no options Send a Config ACK
		// 
		if ( (iLenRec == 4) && (iCodeRec == KPppLcpConfigRequest))
			{
			result = 1;
			break;
			}
		if (iLenRec == 4)
			{
			break;
			}
		else
			{
			ipktRecvPPP.TrimStart(4);
			//ProcessConfig(op, id, len-4, pkt);
			result = 1;
			break;
			}
	case KPppLcpTerminateAck:
		//To be opened only in case if Dummy PPPNif sends Terminate Request so blocked

		// Filter out bad acks
		//if ((iDummyPPPState==EPppFsmClosing || iDummyPPPState==EPppFsmStopping) && id!=iTerminateId)
		//	break;
		//else
		//	iTerminateId |= KPppRequestIdAnswered;
		// fall through ...
	case KPppLcpTerminateRequest:
		if(iLenRec<4)
			break;

		ipktRecvPPP.TrimStart(4);
		ProcessTerminate();
		result = 2;
		break;
	
	default:
		break;
	}
	//pkt.Free();
	if(iDummyPPPState==EDummyPPPFsmIPOpen)
		result = 0;
	return result;

	}

void CDummyPPPIf::ProcessConfig()
	{	

	// Split the recvd packet into separate options
	// placing each option in a queue where it can be
	// easily parsed and manipulated in the upcall handlers.
	RPppOptionList rcvlist;	// Recvd list of options
 
	
	RPppOptionList acklist;	// List of Ack'd options - only valid of the following are empty
	RPppOptionList naklist;	// List of Nak'd options - only valid of the following is empty
	RPppOptionList rejlist;	// List of Rejected options

	CDummyNifLog::Printf(_L("Dummy PPP ProcessConfig\r\n"));

	enum TDummyPPPResult { ENop, EAck, ENak, ERej };

	TDummyPPPResult reply = ENop;

    TRAPD(err, rcvlist.SetL(ipktRecvPPP));
	if (err!=KErrNone)
		return;

	if (iCodeRec == KPppLcpConfigRequest)
		{
		// About to drop out of opened state to need to reset things
		// BEFORE the new request is processed.
		if (iDummyPPPState == EDummyPPPFsmOpened)
			{
			TRAPD(err, InitialiseConfigRequestL());
			if (err != KErrNone)
				{
				CDummyNifLog::Printf(_L("Dummy PPP InitialiseConfigRequestL failed\r\n"));
				return;
				}
			}
		
		// Check options, and split into list of OK, Bad and Unknown
		// Note - the naklist options will have been updated to
		// contain acceptable values.
		if (iCodeRec==KPppLcpConfigRequest)
			CheckRecvConfigRequest(rcvlist, acklist, naklist, rejlist);

		if (!rejlist.IsEmpty())
			{
			reply = ERej;
			naklist.Free();
			acklist.Free();
			}
		else if (!naklist.IsEmpty())
			{
			reply = ENak;
			acklist.Free();
			}
		else if (!acklist.IsEmpty())
			{
			reply = EAck;
			SetupConfigRequest(acklist);
			}
			
			//Added to initialize the SendPPP index
			ipktRecvPPP.Pack();
			ptrTransPktPPP = &ipktRecvPPP;
		}		

	// State processing

	switch (iDummyPPPState)
		{

	case EDummyPPPFsmInitial:
	case EDummyPPPFsmStarting:
		TRAPD(err, InitialiseConfigRequestL());
		if (err==KErrNone)
			SendConfigRequest();
		SetState(EDummyPPPFsmReqSent);
	case EDummyPPPFsmClosing:
	case EDummyPPPFsmStopping:
		break;
	case EDummyPPPFsmClosed:
		SetPppId(KPppIdLcp);
		SendTerminateAck();
		break;

	case EDummyPPPFsmStopped:
		switch (iCodeRec)
			{
		case KPppLcpConfigRequest:
			TRAPD(err, InitialiseConfigRequestL());
			if (err==KErrNone)
			{
				SendConfigRequest();
				SetPppId(KPppIdLcp);
			}
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck);
				SetState(EDummyPPPFsmAckSent);
				break;
			case ENak:
				SendConfigReply(naklist, KPppLcpConfigNak);
				SetState(EDummyPPPFsmReqSent);
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject);
				SetState(EDummyPPPFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
		case KPppLcpConfigNak:
		//Not responding to a reject
		//case KPppLcpConfigReject:
			SetPppId(KPppIdLcp);
			SendTerminateAck();
			break;
			}
		break;
	
	case EDummyPPPFsmReqSent:
		switch (iCodeRec)
			{
		case KPppLcpConfigRequest:
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck);
				SetState(EDummyPPPFsmAckSent);
				break;
			case ENak:
				// RFC 1661 4.6
				if(CheckMaxFailureExceeded() == TRUE)
					SendConfigReply(naklist, KPppLcpConfigReject);
				else
					{
					DecrementDummyPPPMaxFailureCount();
					SendConfigReply(naklist, KPppLcpConfigNak);
					}
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject);
				SetState(EDummyPPPFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
			InitRestartCountForConfig();
			CheckRecvConfigAck(rcvlist);
			SetState(EDummyPPPFsmAckRecvd);
			break;
		case KPppLcpConfigNak:
			InitRestartCountForConfig();
			CheckRecvConfigNak(rcvlist, iDummyPPPRequestList);
			//SendConfigRequestAfterNak(rcvlist);
			SetNewId();
			SendConfigRequest();
			break;
		case KPppLcpConfigReject:
			//Not responding to a reject
			//InitRestartCountForConfig();
			//CheckRecvConfigReject(rcvlist, iDummyPPPRequestList);
			//SendConfigRequestAfterReject(rcvlist);
			//SetNewId();
			//SendConfigRequest();
			break;
			}
		break;
	
	case EDummyPPPFsmAckRecvd:
		switch (iCodeRec)
			{
		case KPppLcpConfigRequest:
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck);
				SetState(EDummyPPPFsmOpened);
				//As external option negotiation complete
				iFlags |= KDummyPPPApplyPending; 
				break;
			case ENak:
				// RFC 1661 4.6
				if(CheckMaxFailureExceeded() == TRUE)
					SendConfigReply(naklist, KPppLcpConfigReject);
				else
					{
					DecrementDummyPPPMaxFailureCount();
					SendConfigReply(naklist, KPppLcpConfigNak);
					}
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
			//InitRestartCountForConfig();
			//CheckRecvConfigAck(rcvlist);
			//SetState(EDummyPPPFsmOpened);
			//As external option negotiation complete
			//iFlags |= KDummyPPPApplyPending; 
			break;
		case KPppLcpConfigNak:
		//Not responding to a reject
		//case KPppLcpConfigReject:
			SendConfigRequest();
			SetState(EDummyPPPFsmReqSent);
			break;
			}
		break;

	case EDummyPPPFsmAckSent:
		switch (iCodeRec)
			{
		case KPppLcpConfigRequest:
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck);
				break;
			case ENak:
				// RFC 1661 4.6
				if(CheckMaxFailureExceeded() == TRUE)
					SendConfigReply(naklist, KPppLcpConfigReject);
				else
					{
					DecrementDummyPPPMaxFailureCount();
					SendConfigReply(naklist, KPppLcpConfigNak);
					}
				SetState(EDummyPPPFsmReqSent);
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject);
				SetState(EDummyPPPFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
			InitRestartCountForConfig();
			CheckRecvConfigAck(rcvlist);
			SetState(EDummyPPPFsmOpened);
			//As external option negotiation complete
			iFlags |= KDummyPPPApplyPending; 
			break;
		case KPppLcpConfigNak:
			InitRestartCountForConfig();
			CheckRecvConfigNak(rcvlist, iDummyPPPRequestList);
			//SendConfigRequestAfterNak(rcvlist);
			SetNewId();
			SendConfigRequest();
			break;
		case KPppLcpConfigReject:
			//Not responding to a reject
			//InitRestartCountForConfig();
			//CheckRecvConfigReject(rcvlist, iDummyPPPRequestList);
			//SendConfigRequestAfterReject(rcvlist);
			//SetNewId();
			//SendConfigRequest();
			break;
			}
		break;

	case EDummyPPPFsmOpened:
		// Config Reset done above
		switch (iCodeRec)
			{
		case KPppLcpConfigRequest:
			SendConfigRequest();
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck);
				SetState(EDummyPPPFsmAckSent);
				break;
			case ENak:
				SendConfigReply(naklist, KPppLcpConfigNak);
				SetState(EDummyPPPFsmReqSent);
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject);
				SetState(EDummyPPPFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
			SendConfigRequest();
			CheckRecvConfigAck(rcvlist);
			SetState(EDummyPPPFsmReqSent);
			break;
		case KPppLcpConfigNak:
			CheckRecvConfigNak(rcvlist, iDummyPPPRequestList);
			//SendConfigRequestAfterNak(rcvlist);
			SetNewId();
			SendConfigRequest();
			SetState(EDummyPPPFsmReqSent);
			break;
		case KPppLcpConfigReject:
			//Not responding to the reject
			//CheckRecvConfigReject(rcvlist, iDummyPPPRequestList);
			//SendConfigRequestAfterReject(rcvlist);
			//SetNewId();
			//SendConfigRequest();
			//SetState(EDummyPPPFsmReqSent);
			break;
			}
		break;
		default:
			//Do nothing
		break; 
		}

	rcvlist.Free();	
	acklist.Free();
	naklist.Free();
	rejlist.Free();

	}
	
void CDummyPPPIf::ProcessPAPandIPCP()
	{
	CDummyNifLog::Printf(_L("Dummy PPP ProcessPAPandIPCP\r\n"));
	// Split the recvd packet into separate options
	// placing each option in a queue where it can be
	// easily parsed and manipulated in the upcall handlers.
	RPppOptionList rcvlist;	// Recvd list of options
	enum TDummyPPPResult { ENop, EAck, ENak, ERej };

	RPppOptionList acklist;	// List of Ack'd options - only valid of the following are empty
	RPppOptionList naklist;	// List of Nak'd options - only valid of the following is empty
	RPppOptionList rejlist;	// List of Rejected options


	TDummyPPPResult reply = ENop;

    TRAPD(err, rcvlist.SetL(ipktRecvPPP));
	if (err!=KErrNone)
		return;

	TInt iAction = ENop; 

	//For overriding PAP
	if (iDummyPPPState == EDummyPPPFsmOpened)
	{
		SetPppId(KPppIdIpcp);
		SetState(EDummyPPPFsmPapAckSent);
	}
	if(iDummyPPPState < EDummyPPPFsmPapAuthAckRecvd)
		{
		if (iDummyPPPState == EDummyPPPFsmOpened)
		{
			//Just for testing Nak packet
			//SendPapRequest();
			//SendPapNak(rcvlist);
			//SetState(EDummyPPPFsmPapAuthReqSent);
		}
			
		iAction = CheckRecvPapReq();
		reply = (iAction == KPppPapNak)?ENak:reply;
		reply = (iAction == KPppPapAck)?EAck:reply;
			
		}
	else
		CheckRecvIPCPReq(rcvlist, acklist, naklist, rejlist);

	if (!rejlist.IsEmpty())
		{
		reply = ERej;
		naklist.Free();
		acklist.Free();
		}
	else if (!naklist.IsEmpty())
		{
		reply = ENak;
		acklist.Free();
		}
	else if (!acklist.IsEmpty())
		{
		reply = EAck;
		//SetupConfigRequest(acklist);
		}
	 
	
	//Added to initialize the SendPPP index
	ipktRecvPPP.Pack();
	ptrTransPktPPP = &ipktRecvPPP;
	
	// State processing

	switch (iDummyPPPState)
		{
			case EDummyPPPFsmOpened:
				//Commenting Pap nego for the time being as PPP is rejecting it.
				//so skipping to IPCP Nego state
				//SetPppId(KPppIdPap);
				//SendPapRequest();
				//SetState(EDummyPPPFsmPapAuthReqSent);
				SetPppId(KPppIdIpcp);
				SetState(EDummyPPPFsmPapAckSent);

				break;
			case EDummyPPPFsmPapAuthAckRecvd:
					if(reply == EAck)
					{
						SendConfigReply(acklist, KPppLcpConfigAck);
						SetPppId(KPppIdIpcp);
						SetState(EDummyPPPFsmPapAckSent);
						break;
					}
					if(reply == ENak)
					{
						SendConfigReply(naklist, KPppLcpConfigNak);
						break;
					}
				break;
			case EDummyPPPFsmPapAckSent:
				if(iCodeRec == KPppLcpConfigRequest)
				{
					switch(reply)
					{
					case EAck:
						SendConfigReply(acklist, KPppLcpConfigAck);
						SetPppId(KPppIdIpcp);
						SendIPCPReq();
						SetState(EDummyPPPIPCPReqSent);
						break;
					case ENak:
					default:
						SetPppId(KPppIdIpcp);
						SendIPCPDummyNak();
						break;
					}
				}
				else
				{
					switch(reply)
					{
					case EAck:
						SetPppId(KPppIdIpcp);
						SendConfigReply(acklist, KPppLcpConfigAck);
						SetState(EDummyPPPFsmIPOpen);
						break;
					case ENak:
					default:
						SetPppId(KPppIdIpcp);
						SendIPCPDummyNak();
						break;
					}
				}
				break;
			case EDummyPPPIPCPReqSent:
				switch(reply)
					{
					case EAck:
						SendConfigReply(acklist, KPppLcpConfigAck);
						SetState(EDummyPPPFsmIPOpen);
						break;
				
					case ENak:
					default:
						SetPppId(KPppIdIpcp);
						SendIPCPReq();
						break;
					}
				break;
			default:
				//Do nothing
				break;
		}

	}

void CDummyPPPIf::ProcessTerminate()
	{
	if (iCodeRec==KPppLcpTerminateRequest)
		{
		switch (iDummyPPPState)
			{
		case EDummyPPPFsmInitial:
		case EDummyPPPFsmStarting:
			// bad event
			break;
		case EDummyPPPFsmClosed:
		case EDummyPPPFsmStopped:
		case EDummyPPPFsmClosing:
		case EDummyPPPFsmStopping:
			SendTerminateAck();
			break;
		case EDummyPPPFsmReqSent:
		case EDummyPPPFsmAckRecvd:
		case EDummyPPPFsmAckSent:
			iNotify->LinkLayerDown(KErrTimedOut, MNifIfNotify::EDisconnect);
			iLink->LinkDown(KErrCouldNotConnect);
			SendTerminateAck();
			SetState(EDummyPPPFsmReqSent);
			break;
		case EDummyPPPFsmOpened:
		case EDummyPPPFsmPapAuthReqSent:
		case EDummyPPPFsmPapAuthAckRecvd:
		case EDummyPPPFsmPapAckSent:
		case EDummyPPPIPCPReqSent:
			//SendTerminateAck();
		case EDummyPPPFsmIPOpen:
			// Fix for DEF002615
			// Dummy PPP should not convey KErrCommsLineFail to PPP when a Terminate Request 
			// is received. So blocking these two lines.
			//iLink->Stop(KErrCommsLineFail,MNifIfNotify::EDisconnect);
			//iLink->LinkDown(KErrCommsLineFail);
			iDummyPPPRestartCount = 0;
			SetState(EDummyPPPFsmInitial);
			break;
			}
		}
	else // KPppLcpTerminateAck
		{
			{
			switch (iDummyPPPState)
				{
			case EDummyPPPFsmInitial:
			case EDummyPPPFsmStarting:
				// bad event
				break;
			case EDummyPPPFsmClosed:
			case EDummyPPPFsmStopped:
				iLink->LinkDown(KErrCommsLineFail);
				iNotify->LinkLayerDown(KErrNone, MNifIfNotify::EDisconnect);
				break;
			case EDummyPPPFsmClosing:
				iLink->LinkDown(KErrCommsLineFail);
				SetState(EDummyPPPFsmClosed);
				break;
			case EDummyPPPFsmStopping:
				iLink->LinkDown(KErrCommsLineFail);
				SetState(EDummyPPPFsmStopped);
				iNotify->LinkLayerDown(KErrNone, MNifIfNotify::EDisconnect);
				break;
			case EDummyPPPFsmReqSent:
			case EDummyPPPFsmAckSent:
				break;
			case EDummyPPPFsmAckRecvd:
				SetState(EDummyPPPFsmReqSent);
				break;
			case EDummyPPPFsmOpened:
			case EDummyPPPFsmPapAuthReqSent:
			case EDummyPPPFsmPapAuthAckRecvd:
			case EDummyPPPFsmPapAckSent:
			case EDummyPPPIPCPReqSent:
			case EDummyPPPFsmIPOpen:
				iLink->Stop(KErrCommsLineFail,MNifIfNotify::EDisconnect);
				SendConfigRequest();
				SetState(EDummyPPPFsmReqSent);
				break;
				}
			}
		}

	}
void CDummyPPPIf::SetupSendPPP()
{	
	CDummyNifLog::Printf(_L("Dummy PPP SetupSendPPP\r\n"));
	if(ipktRecvPPP.IsEmpty())
	{
		
		//Storing data received into Recv Q
		iRecvQ.Append(ipktRecvPPP);
		//iSendNumBufs += ipktRecvPPP.NumBufs();
		
		iRecvQ.Remove(*ptrTransPktPPP);

		//ptrTransPktPPP = &ipktRecvPPP;

	}
}

void CDummyPPPIf::SendPapRequest()
	{
	 CDummyNifLog::Printf(_L("Dummy PPP SendPapRequest\r\n"));
#ifdef _UNICODE
	TBuf8<256> user,pass;

	//Only for testing
	//ConvertTo8Bit(user, iLink->UserName());
	//ConvertTo8Bit(pass, iProtocol->PassWord());
	
	user = _L8("EpochUser");
	pass = _L8("EpochPass");
	
#else
	TPtrC8 user, pass;
	user.Set(iProtocol->UserName());
	pass.Set(iProtocol->PassWord());
#endif

	RMBufPacket pkt;
	RMBufPktInfo* info = NULL;

	TInt len = 4+user.Length()+1+pass.Length()+1;
	TRAPD(err, pkt.AllocL(len));
	if (err!=KErrNone)
		return;
	TRAP(err, info = pkt.NewInfoL());
	if (err!=KErrNone)
		{
		pkt.Free();
		return;
		}
	TUint8* ptr = pkt.First()->Ptr();
	*ptr++ = KPppPapRequest;
	//Considering it as a New request
	//if (aNewRequest)
	//	{
		iDummyPPPTryCount = KDummyPPPPapRetries;
		if (++iCurrentDummyPPPId==0)
			++iCurrentDummyPPPId;
	//	}
	*ptr++ = iCurrentDummyPPPId;
	BigEndian::Put16(ptr, (TUint16)len);
	ptr += 2;
	*ptr++ = (TUint8)user.Length();	
	Mem::Copy(ptr, (TUint8*)user.Ptr(), user.Length());
	ptr += user.Length();
	*ptr++ = (TUint8)pass.Length();	
	Mem::Copy(ptr, (TUint8*)pass.Ptr(), pass.Length());
	ptr += pass.Length();
	info->iLength = len;	
	TPppAddr::Cast((info->iDstAddr)).SetProtocol(KPppIdPap);
	pkt.Pack();
	SendtoPPP(pkt);
	iLink->TimerCancel();
	iLink->TimerAfter(KDummyPPPPapWaitTime*1000);
	--iDummyPPPTryCount;

	}
void CDummyPPPIf::SendPapAck(RPppOptionList& aReplyList)
	{
	
	CDummyNifLog::Printf(_L("Dummy PPP SendPapAck\r\n"));
	RPppOption opt;
	RMBufPacket pkt;
	RPppOptionList iAckList;	// List of Ack'd options - only valid of the following are empty
 
	while (aReplyList.Remove(opt))
		{
		opt.OptType();
		iAckList.Append(opt);
		}
		
	TRAPD(err, iAckList.CreatePacketL(pkt, KPppIdPap, KPppPapAck, iIdRec));
	if (err==KErrNone)
		{
		SendtoPPP(pkt);
		}
	}
void CDummyPPPIf::SendPapNak(RPppOptionList& aReplyList)
	{
		
	CDummyNifLog::Printf(_L("SendPapNak\r\n"));
	RPppOption opt;
	RMBufPacket pkt;
	RMBufPacket* Packet;
	RPppOptionList iNakList;	// List of Nak'd options - only valid of the following is empty

	while (aReplyList.Remove(opt))
		{
		opt.OptType();
		iNakList.Append(opt);
		}
	TRAPD(err, iNakList.CreatePacketL(pkt, KPppIdPap, KPppPapNak, iIdRec));
	if (err==KErrNone)
		{
		Packet = (RMBufPacket*)&pkt;
		RMBufPktInfo*info = Packet->Unpack();
	    TPppAddr::Cast((info->iDstAddr)).SetProtocol(KPppIdPap);
		TPppAddr::Cast(info->iDstAddr).SetAddress(iLocalAddr);
		Packet->Pack();
		SendtoPPP(pkt);
		}
	}

TInt CDummyPPPIf::CheckRecvPapReq()
	{
		CDummyNifLog::Printf(_L("Dummy PPP CheckRecvPapReq\r\n"));
		switch (iCodeRec)
		{
		case KPppPapRequest:
			{
			TPtrC8 user;
			TPtrC8 pass;
			user.Set(&iCodeRec,sizeof(iCodeRec));   
			pass.Set(&iIdRec,sizeof(iIdRec));   
			}
			return 0;
		case KPppPapAck:
			if (iIdRec==iCurrentDummyPPPId)
			{
				iLink->TimerCancel();
				//Fix for DEF03548 Changing progress states to match that in pppprog.h
				iNotify->IfProgress(EPppProgressAuthenticationComplete,KErrNone);
				return KPppPapNak;
			}
			else
			{
				return KPppPapAck;
			}
		case KPppPapNak:
			if (iIdRec==iCurrentDummyPPPId)
			{
				iLink->TimerCancel();
				//Fix for DEF03548 Changing progress states to match that in pppprog.h
				iNotify->IfProgress(EPppProgressAuthenticationComplete,KErrIfAuthenticationFailure);
			}
			else
			{
				return KPppPapNak;
			}
		break;
		default:
			return KPppPapNak;
		}
		return KPppPapRequest;
	}
void CDummyPPPIf::SendIPCPDummyNak()
	{
		CDummyNifLog::Printf(_L("Dummy PPP SendIPCPDummyNak\r\n"));
		TRAPD(err, InitializeIPCPConfigRequestL());
		if (err==KErrNone)
		{
			if (iDummyPPPRequestList.IsEmpty())
			{
				CDummyNifLog::Printf(_L("Request list empty\r\n"));
				return;
			}

			RMBufPacket Packet;	
			//iDummyPPPRequestId = iIdRec;
			TRAPD(err, iDummyPPPRequestList.CreatePacketL(Packet, KPppIdIpcp, KPppLcpConfigNak, (TUint8)iIdRec));
			if (err!=KErrNone)
			{
				//__DEBUGGER();
				return;
			}
	
			ptrTransPktPPP = &Packet;

			RMBufPktInfo*info = Packet.Unpack();
		 
			TPppAddr::Cast((info->iDstAddr)).SetProtocol(KPppIdIpcp);
			TPppAddr::Cast(info->iDstAddr).SetAddress(iLocalAddr);
			Packet.Pack();

			SendtoPPP(*ptrTransPktPPP);
		}
	}
void CDummyPPPIf::InitializeIPCPConfigRequestL()
	{
	CDummyNifLog::Printf(_L("Dummy PPP InitializeIPCPConfigRequestL()\r\n"));
	SetNewId();
	InitRestartCountForConfig();
	
	if (!iDummyPPPRequestList.IsEmpty())
		iDummyPPPRequestList.Free();
	
	iDummyPPPRequestList.CreateAndAddL(KPppIpcpOptIpAddress, iLocalAddr);
	
	//BigEndian::Put32(opt.ValuePtr(), iRemoteAddr);
	//iDummyPPPRequestList.CreateAndAddL(KPppIpcpOptPrimaryDnsAddress, iPrimaryDns);
	//iDummyPPPRequestList.CreateAndAddL(KPppIpcpOptSecondaryDnsAddress, iSecondaryDns);

	}
void CDummyPPPIf::SendIPCPReq()
	{
		CDummyNifLog::Printf(_L("Dummy PPP SendIPCPReq\r\n"));
		TRAPD(err, InitializeIPCPConfigRequestL());
		if (err==KErrNone)
		{
			if (iDummyPPPRequestList.IsEmpty())
			{
				CDummyNifLog::Printf(_L("Request list empty\r\n"));
				return;
			}

			RMBufPacket Packet;	
			//iDummyPPPRequestId = iIdRec;
			TRAPD(err, iDummyPPPRequestList.CreatePacketL(Packet, KPppIdIpcp, KPppLcpConfigRequest, (TUint8)iIdRec));
			if (err!=KErrNone)
			{
			//__DEBUGGER();
				return;
			}
	
			ptrTransPktPPP = &Packet;

			RMBufPktInfo*info = Packet.Unpack();
		 
			TPppAddr::Cast((info->iDstAddr)).SetProtocol(KPppIdIpcp);
			TPppAddr::Cast(info->iDstAddr).SetAddress(iLocalAddr);
			Packet.Pack();

			SendtoPPP(*ptrTransPktPPP);
		}
	}
void CDummyPPPIf::CheckRecvIPCPReq(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList)
	{
	RPppOption opt;
	TUint8 temp;
	CDummyNifLog::Printf(_L("Dummy PPP CheckRecvIPCPReq\r\n"));
	while (aRequestList.Remove(opt))
		{
		temp = opt.OptType();
		switch (temp)
			{
		case KPppIpcpOptIpAddress:
			{
			TUint32 addr = BigEndian::Get32(opt.ValuePtr());
			if (addr==0)
				{
					aNakList.Append(opt);
				}
			else
				aAckList.Append(opt);
			}
			break;
		case KPppIpcpOptIpAddresses:
         {
			TUint32 remoteAddr = BigEndian::Get32(opt.ValuePtr());
			TUint32 addr = BigEndian::Get32(opt.ValuePtr()+4);

         if (remoteAddr == 0)
            {
				aRejList.Append(opt);
            }
         else if (addr == 0)
            {
				aNakList.Append(opt);
				}
			else
            {
				iRemoteAddr = remoteAddr;
				aAckList.Append(opt);
            }
         }
		 break;
			}
		}
	}

void CDummyPPPIf::SetState(TDummyPPPToPPPFsmState aState)
	{
	CDummyNifLog::Write(_L("DummyPPP::SetState()"));

	_LIT(KStateString,"State %s -> %s");


	CDummyNifLog::Printf(KStateString,CDummyNifLog::StateToText(iDummyPPPState), CDummyNifLog::StateToText(aState));

	iDummyPPPState = aState;
	}
void CDummyPPPIf::SetPppId(TUint aProt)
	{
	CDummyNifLog::Printf(_L("DummyPPP::SetPppId to %d\n"),aProt);
	iSendPppId = aProt;
	}

void CDummyPPPIf::InitRestartCountForConfig()
	{
	CDummyNifLog::Printf(_L("Dummy PPP InitRestartCountForConfig\r\n"));
	// RFC 1661 4.6 Max-Configure
	iDummyPPPWaitTime = KDummyPPPWaitTimeConfig;
	iDummyPPPRestartCount = KDummyPPPMaxRestartConfig;
	}

void CDummyPPPIf::SetNewId()
	{
	TUint8 id = ++iCurrentDummyPPPId;
	if (iCurrentDummyPPPId==0)
		++iCurrentDummyPPPId;
	iDummyPPPRequestId = id;
	CDummyNifLog::Printf(_L("Dummy PPP SetNewId %d\r\n"),iDummyPPPRequestId);
	}

void CDummyPPPIf::InitialiseConfigRequestL()
	{
	TUint32 iDummyPPPRecvEscMap = 0;
	SetNewId();
	InitRestartCountForConfig();
	SetPppId(KPppIdLcp);
	
	CDummyNifLog::Printf(_L("Dummy PPP InitialiseConfigRequest\r\n"));
	if (!iDummyPPPRequestList.IsEmpty())
		iDummyPPPRequestList.Free();
	
	// Create a new magic number
	iPPPRemMagicNumber = 0;
	
	if (iDummyPPPLocMagicNumber == 0)
	{
		//CPppLcp::NewMagicNumber(iDummyPPPLocMagicNumber);
		//Only for testing
		iDummyPPPLocMagicNumber = 54321;
	}
	
	iDummyPPPRequestList.CreateAndAddL(KPppLcpOptMagicNumber, (TUint32)iDummyPPPLocMagicNumber);
	if ((iOrigConfig().iHandshake & KConfigSendXoff)!=0
		|| (iOrigConfig().iHandshake & KConfigSendXoff)!=0)
		{
		if (iOrigConfig().iXonChar<32)
			iDummyPPPRecvEscMap |= 1<<iOrigConfig().iXonChar;
		if (iOrigConfig().iXoffChar<32)
			iDummyPPPRecvEscMap |= 1<<iOrigConfig().iXoffChar;
		}
	//for testing options
	//iDummyPPPRequestList.CreateAndAddL(KPppLcpOptEscapeCharMap,iDummyPPPRecvEscMap);
	//iDummyPPPRequestList.CreateAndAddL(KPppLcpOptProtocolCompress);
	//iDummyPPPRequestList.CreateAndAddL(KPppLcpOptAddrCtrlCompress);

	}

void CDummyPPPIf::SetupConfigRequest(RPppOptionList& aRequestList)
//
// Apply options in a recvd config request (that was ACK'd)
//
	{
	CDummyNifLog::Printf(_L("Dummy PPP SetupConfigRequest\r\n"));
	TMBufPktQIter iter(aRequestList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			//iMaxSendSize = BigEndian::Get16(opt.ValuePtr());
			break;
		case KPppLcpOptMagicNumber:
			iPPPRemMagicNumber = BigEndian::Get32(opt.ValuePtr());
			break;
		default:
			//ExtOptApplyConfigRequest(opt);
			break;
			}
		}
	}

void CDummyPPPIf::CheckRecvConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList)
//
// Check options in a recvd config request
//
	{
	CDummyNifLog::Printf(_L("Dummy PPP CheckRecvConfigRequest\r\n"));
	RPppOption opt;
	//only for debugging
	TUint8 temp;

	while (aRequestList.Remove(opt))
		{
		temp = opt.OptType();
		switch (temp)
			{
		case KPppLcpOptMaxRecvUnit:
			if (opt.ValueLength()<0)
				aNakList.Append(opt);
			else
				{
				if (opt.ValueLength()<2)
					aRejList.Append(opt);
				else
					aAckList.Append(opt);
				}
			break;

		case KPppLcpOptMagicNumber:
			if (opt.ValueLength()<0)
				aNakList.Append(opt);
			else
				{
				if (opt.ValueLength()!=4)
					aRejList.Append(opt);
				else
					{
					iPPPRemMagicNumber = BigEndian::Get32(opt.ValuePtr());
					if (iPPPRemMagicNumber!=iDummyPPPLocMagicNumber)
						{
						aAckList.Append(opt);
						}
					else
						{
						//CPppLcp::NewMagicNumber(iPPPRemMagicNumber);
						//Only for testing
						iPPPRemMagicNumber = 11111;
						BigEndian::Put32(opt.ValuePtr(), iPPPRemMagicNumber);
						aNakList.Append(opt);
						}
					}
				}
			break;
		case KPppLcpOptMultiLinkEndPointDescriminator:
	        aAckList.Append(opt);
			break;

		default:
			//ExtOptCheckConfigRequest(aPacket, (CProtocolBase*)this);
    		iPppOptions->ExtOptCheckConfigRequest(opt, aAckList, aNakList, aRejList);
			}
		}
	}

void CDummyPPPIf::CheckRecvConfigAck(RPppOptionList& aReplyList)
//
// Recvd a Config Ack - apply the options
//
	{
	CDummyNifLog::Printf(_L("Dummy PPP CheckRecvConfigAck\r\n"));
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
		
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			//iMaxRecvSize = BigEndian::Get16(opt.ValuePtr());
			break;
		case KPppLcpOptMagicNumber:
			// Magic number is OK
			//SendDummyPPPIdentification();
			break;
		default:
			iPppOptions->ExtOptRecvConfigAck(opt);
			break;
			}
		}
	}

void CDummyPPPIf::CheckRecvConfigNak(RPppOptionList& aReplyList, RPppOptionList& aReqList)
//
// Recvd a Config Nak - The associated original request is in aReqList
//
	{
	CDummyNifLog::Printf(_L("DummyPPP::CheckRecvConfigNak\r\n"));
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			aReqList.ReplaceOption(opt);
			break;
		case KPppLcpOptMagicNumber:
			{
			iDummyPPPLocMagicNumber = BigEndian::Get32(opt.ValuePtr());
			aReqList.ReplaceOption(opt);
			}
			break;
		default:
			iPppOptions->ExtOptRecvConfigNak(opt, aReqList);
			break;
			}
		}
	}

void CDummyPPPIf::CheckRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList)
//
// Recvd a Config Reject - The associated original request is in aReqList
//
	{
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	CDummyNifLog::Printf(_L("Dummy PPP CheckRecvConfigReject\r\n"));
	
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			aReqList.RemoveOption(opt);
			break;
		case KPppLcpOptMagicNumber:
			aReqList.RemoveOption(opt);
			break;
		default:
			iPppOptions->ExtOptRecvConfigReject(opt, aReqList);
			break;
			}
		}
	}


void CDummyPPPIf::SendConfigRequest()
//
// Send the config request in iDummyPPPRequestList
//
	{

	if (iDummyPPPRequestList.IsEmpty())
		{
		CDummyNifLog::Write(_L("DummyPPP Request list empty\r\n"));
		return;
		}

	CDummyNifLog::Printf(_L("Dummy PPP SendConfigRequest\r\n"));
	RMBufPacket pkt;	
	iDummyPPPRequestId &= 0xff;
	TRAPD(err, iDummyPPPRequestList.CreatePacketL(pkt, iSendPppId, KPppLcpConfigRequest, (TUint8)iDummyPPPRequestId));
	if (err==KErrNone)
		{
		SendtoPPP(pkt);
		--iDummyPPPRestartCount;
		iLink->TimerCancel();
		TInt temp=iDummyPPPWaitTime;
		iLink->TimerAfter(iDummyPPPWaitTime*1000);
		iDummyPPPWaitTime=temp;
		}	
	}

void CDummyPPPIf::SendConfigReply(RPppOptionList& aOptList,TUint8 aType)
	{
	RMBufPacket pkt;
	CDummyNifLog::Printf(_L("Dummy PPP SendConfigReply\r\n"));
	TRAPD(err, aOptList.CreatePacketL(pkt, iSendPppId, aType, iIdRec));
	if (err==KErrNone)
		{
		SendtoPPP(pkt);
		iLink->TimerCancel();
		TInt temp=iDummyPPPWaitTime;
		iLink->TimerAfter(iDummyPPPWaitTime*1000);
		iDummyPPPWaitTime=temp;
		}
	}

void CDummyPPPIf::SendTerminateAck()
	{
	RMBufPacket pkt;
	RMBufPktInfo* info=NULL;
	CDummyNifLog::Printf(_L("Dummy PPP SendTerminateAck\r\n"));
	TRAPD(err, pkt.AllocL(4));
	if (err ==KErrNone)
		{
		TRAP(err,info = pkt.NewInfoL());
		}
	if (err ==KErrNone)
		{
		info->iLength = 4;
		TPppAddr::Cast((info->iDstAddr)).SetProtocol(iSendPppId);
		TUint8* ptr = pkt.First()->Ptr();
		*ptr++ = KPppLcpTerminateAck;
		*ptr++ = iIdRec;
		BigEndian::Put16(ptr, (TUint16)4);
		pkt.Pack();
		SendtoPPP(pkt);
		}
	else
		{
		pkt.Free();
		__ASSERT_DEBUG(EFalse,User::Invariant());
		}
	}

void CDummyPPPIf::SendDummyPPPIdentification()
	{

	CDummyNifLog::Printf(_L("Dummy PPP SendDummyPPPIdentification\r\n"));
	TInt err;
	TPtrC8 id = _L8("Symbian Epoc!");
	RMBufPacket pkt;
	RMBufPktInfo* info = NULL;
	TRAP(err, info = pkt.NewInfoL());
	if (err!=KErrNone)
		return;
	info->iLength = 4+4+id.Length();
	TRAP(err, pkt.AllocL(info->iLength));
	if (err!=KErrNone)
		{
		pkt.FreeInfo();
		return;
		}
	TUint8* ptr = pkt.First()->Ptr();
	*ptr++ = (TUint8)KPppLcpIdentification;
	SetNewId();
	*ptr++ = iDummyPPPRequestId;
	BigEndian::Put16(ptr, (TUint16)info->iLength);
	BigEndian::Put32(ptr+2, (TUint32)iDummyPPPLocMagicNumber);
	pkt.CopyIn(id, 8);
	pkt.Pack();
	SendtoPPP(pkt);
	}
TInt CDummyPPPLink::Start()
	{
	CDummyNifLog::Printf(_L("Dummy PPP Link::Start\r\n"));
	
	//As CreateL is not called before Start calling it here
	TRAPD(err, iNifIf->CreateL());
	if (err != KErrNone)
		return err;
	iState = EDummyPPPLinkOpen;
 
	iNotify->IfProgress(KLinkLayerOpen, KErrNone);
	iNotify->LinkLayerUp();

	iNifIf->OpenFSM();
	iNifIf->InitRestartCountForConfig();
	//Setting iSendFlowOn to start
	iNifIf->SetFlowOnFlag(ETrue);
	iNifIf->iProtocol->StartSending((CProtocolBase*)iNifIf);
	
	return KErrNone;
	}
void CDummyPPPIf::CloseFSM()
	{
	CDummyNifLog::Printf(_L("Dummy PPP Close FSM Request\r\n"));
	
	//Setting response to PPP state machine
	if((iDummyPPPState == EDummyPPPFsmInitial)
		||(iDummyPPPState == EDummyPPPFsmStarting))
		{
		//Do nothing
		}
	if((iDummyPPPState == EDummyPPPFsmClosed)
		||(iDummyPPPState == EDummyPPPFsmClosing))
		{
		SetState(EDummyPPPFsmInitial);
		}
    if((iDummyPPPState == EDummyPPPFsmStopped)
		||(iDummyPPPState == EDummyPPPFsmStopping)
		||(iDummyPPPState == EDummyPPPFsmReqSent)
		||(iDummyPPPState == EDummyPPPFsmAckRecvd)
		||(iDummyPPPState == EDummyPPPFsmAckSent)
		||(iDummyPPPState == EDummyPPPFsmOpened))
		{
		SetState(EDummyPPPFsmStarting);
		}
}
void CDummyPPPIf::ChangeStateforTimerComplete()
	{
	
	CDummyNifLog::Printf(_L("Dummy PPP Timer Expired %d \r\n"),iDummyPPPRestartCount);
	if((iDummyPPPState == EDummyPPPFsmInitial)
		||(iDummyPPPState == EDummyPPPFsmStarting)
		||(iDummyPPPState == EDummyPPPFsmClosed)
		||(iDummyPPPState == EDummyPPPFsmStopped)
		||(iDummyPPPState == EDummyPPPFsmOpened))
		// Bad Event
		return;
   
	if (iDummyPPPRestartCount>0)
		{
		switch (iDummyPPPState)
			{
		case EDummyPPPFsmClosing:
		case EDummyPPPFsmStopping:
			//SendTerminateRequest();
			break;
		case EDummyPPPFsmReqSent:
		case EDummyPPPFsmAckRecvd:
		case EDummyPPPFsmAckSent:
			//PG RFC 1661 4.6 double timeout period
			//if (iDummyPPPWaitTime*2<KDummyPPPFsmRequestMaxTimeout)
			//	iDummyPPPWaitTime = iDummyPPPWaitTime*2;
			//SendConfigRequest();
			
			//if(iDummyPPPState == EDummyPPPFsmAckRecvd)
			//	SetState(EDummyPPPFsmReqSent);
			break;
		default:
			break;
			}
		}
	else
		{
	
		switch (iDummyPPPState)
			{
		case EDummyPPPFsmClosing:
			SetState(EDummyPPPFsmClosed);
			break;
		case EDummyPPPFsmStopping:
		case EDummyPPPFsmReqSent:
		case EDummyPPPFsmAckRecvd:
		case EDummyPPPFsmAckSent:
			SetState(EDummyPPPFsmStopped);
			break;
		default:
			break;
			}
		}

	if (iDummyPPPTryCount>0)
		SendPapRequest();
	}

void CDummyPPPLink::TimerComplete(TInt)
	{

	CDummyNifLog::Write(_L("DummyPPP Link::TimerComplete()"));

	//
	// Upcall from Timer
	//
    iNifIf->ChangeStateforTimerComplete();

	//Here while closing down it crashes!
	//iNifIf->CloseFSM();
    //iNotify->LinkLayerDown(KErrTimedOut, MNifIfNotify::EDisconnect);
	
	}


void CDummyPPPLink::Stop(TInt aError, MNifIfNotify::TAction aAction)		
	{
	switch (iState)
		{
	case EDummyPPPLinkClosed:
		return;
	case EDummyPPPLinkConnecting:
		iState = EDummyPPPLinkClosed;
		break;
	case EDummyPPPLinkOpen:
		//iNifIf->PacketModeOff();
		iState = EDummyPPPLinkClosed;
		break;
	default:
		break;
		}
	CDummyNifLog::Printf(_L("DummyPPP Link::Stop(aError %d, TAction %d)"), aError, aAction);
	//Commenting as this has to be conveyed by PPP and not DummyPPP Nif
	//iNotify->IfProgress(KLinkLayerClosed, aError);
	iNifIf->CloseFSM();
	//Commenting as this has to be conveyed by PPP and not DummyPPP Nif
	iNotify->LinkLayerDown(aError, aAction);
	
	}

void CDummyPPPLink::IfProgress(TInt /*aResult*/)
	{
	}
void CDummyPPPIf::BindL(TAny *aId)
	{
	CDummyNifLog::Printf(_L("BindL(aId %x)\r\n"),aId);
	if (iProtocol)
		User::Leave(KErrAlreadyExists);
	iProtocol = (CProtocolBase*)aId;
	}

void CDummyPPPIf::SetFlowOnFlag(TBool aFlag)
	{
		CDummyNifLog::Printf(_L("Dummy PPP SetFlowOnFlag\r\n"));
		iSendFlowOn = aFlag;
	}

CNifIfBase* CDummyPPPLink::GetBinderL(const TDesC& aName)
	{
	CDummyNifLog::Printf(_L("DummyPPP LinkGetBinderL(%S)"), &aName);

	iNifIf = new(ELeave) CDummyPPPIf(this);
	return iNifIf;

	}

TInt CDummyPPPLink::Notification(TAgentToNifEventType aEvent,void* aInfo)
	{
	CDummyNifLog::Printf(_L("Dummy PPP LinkNotification\r\n"));
	if ((aEvent!=EAgentToNifEventTypeDisableTimers) 
		|| (aInfo!=NULL))
		return KErrUnknown;
	return KErrNone;
	}

void CDummyPPPLink::Restart(CNifIfBase*)
	{}

CDummyPPPIf::CDummyPPPIf(CDummyPPPLink* aLink)
	: CNifIfBase(*aLink)
	{
	CDummyNifLog::Printf(_L("DummyPPP Constructed"));

	//__DECLARE_FSM_NAME(_S("DUMMY_PPP FSM"));

	iLink = aLink;
	
	//Settings for flow control
	iSendLoWat = KLoWatMark;
	iSendHiWat = KHiWatMark;

	// generate my local ip address (ip4) - vals potentially will be overwritten by any derived classes
	iLocalAddress = KDummyPppNifLocalAddressBase;//0x0A220C1BB; //+ ((TUint32)this)%255;
    
	//For IPCP Nego.
	//PresetAddr(iLocalAddr, TPtrC(KCDTypeNameIpAddr));
	iLocalAddr = iLocalAddress;

	//PresetAddr(iPrimaryDns, TPtrC(KCDTypeNameIpNameServer1));
	//PresetAddr(iSecondaryDns, TPtrC(KCDTypeNameIpNameServer2));
	//iPrimaryNbns = IPADDR(0,0,0,0);
	//iSecondaryNbns = IPADDR(0,0,0,0);
	//PresetAddr(iNetworkMask, TPtrC(KCDTypeNameIpNetMask));

	// need to give each if a unique id so we will number and count them as we create them
	//++iNumOfIfs;
	//iIfNum = iNumOfIfs;

	//Even if this is given, when the identification command for the nif is given
	//it always turns up as a ipcp.comm as PPP Nif gives the information overlooking this.
	iIfName.Format(_L("dummypppnif[0x%08x]"), this);



	}
TInt CDummyPPPIf::PresetAddr(TUint32& aAddr, const TDesC& aVarName)
/**
 * Preset IP adress in aAddr with value from CommDB. The field name
 * is given in aVarName. Examples are "IpAddr" and "IpNameServer1"
 *
 * @param aAddr    IP address to be set
 * @param aVarName name of CommDB field
 * @return KErrNone if success, global error code otherwise
 */
	{
	if (aAddr!=0)
		return KErrNone;
	
	TBuf<KCommsDbSvrMaxFieldLength> name;
	TInetAddr addr;
    

	(void)iNotify->ReadDes(aVarName, name); // ignore return value

	const TInt ret = addr.Input(name);
	if (ret==KErrNone)
		aAddr = addr.Address();

	return ret;
	}


void CDummyPPPIf::Info(TNifIfInfo& aInfo) const
	{
	CDummyNifLog::Printf(_L("Dummy PPP Info\r\n"));
	aInfo.iVersion = TVersion(1,1,1);
	aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfCreatedByLink;
	aInfo.iName.Copy(iIfName);
	aInfo.iProtocolSupported = 0;
	}

void CDummyPPPLink::FillInInfo(TNifIfInfo& aInfo, TAny* aPtr)
	{
	aInfo.iProtocolSupported=0;
	aInfo.iVersion = TVersion(1,1,1);
	aInfo.iFlags = KNifIfIsBase | KNifIfIsLink | KNifIfUsesNotify | KNifIfCreatedByFactory;
    aInfo.iName = _L("dummyppplink");
	aInfo.iName.AppendFormat(_L("[0x%08x]"), aPtr);
	aInfo.iFlags |= KNifIfCreatesBinder;
	}

void CDummyPPPLink::Info(TNifIfInfo& aInfo) const
	{
	FillInInfo(aInfo,(TAny*) this);
	CDummyNifLog::Printf(_L("Dummy PPP LinkInfo\r\n"));
	}

TInt CDummyPPPIf::Control(TUint aLevel, TUint aName, TDes8& aOption, TAny* /* aSource */)
	{
	CDummyNifLog::Printf(_L("DummyPPP::Control(aLevel %x, aName %x, ...)"), aLevel, aName);

	if (aLevel==KSOLInterface)
		{
		switch (aName)
			{
		case KSoIfInfo:
			{
			TSoIfInfo& opt = *(TSoIfInfo*)aOption.Ptr();

			_LIT(KName, "MyName");
			opt.iName.Copy(KName);
			opt.iFeatures = KIfCanBroadcast | KIfCanMulticast;
			opt.iMtu = 1500;
			opt.iSpeedMetric = 0;
			return KErrNone;
			}

		case KSoIfHardwareAddr:
			return KErrNotSupported;

		case KSoIfConfig:
			{
			TSoInetIfConfig& opt = *(TSoInetIfConfig*)aOption.Ptr();
			if (opt.iFamily!=KAfInet)
				return KErrNotSupported;

			TUint32 address;
			const TInt KPort = 65;

			opt.iConfig.iAddress.SetAddress(iLocalAddress);
			opt.iConfig.iAddress.SetPort(KPort);

			// network mask
			opt.iConfig.iNetMask.Input(KNetworkMask);
			opt.iConfig.iNetMask.SetPort(KPort);

			// broadcast address
			address = KDummyPppNifLocalAddressBase + KBroadcastAddressSuffix;
			opt.iConfig.iBrdAddr.SetAddress(address);
			opt.iConfig.iBrdAddr.SetPort(KPort);

			// default gateway
			address = KDummyPppNifLocalAddressBase + KDefaultGatewayAddressSuffix;
			opt.iConfig.iDefGate.SetAddress(address);
			opt.iConfig.iDefGate.SetPort(KPort);

			// primary DNS, just make same as default gateway
			opt.iConfig.iNameSer1.SetAddress(address);
			opt.iConfig.iNameSer1.SetPort(KPort);

			// secondary DNS
			address = KDummyPppNifLocalAddressBase + KSecondaryDnsAddressSuffix;
			opt.iConfig.iNameSer2.SetAddress(address);
			opt.iConfig.iNameSer2.SetPort(KPort);

			return KErrNone;
			}

		case KSoIfGetConnectionInfo:
			TSoIfConnectionInfo& opt = *(TSoIfConnectionInfo*)aOption.Ptr();
			TInt err = KErrNone;
			TBuf<2*KCommsDbSvrMaxColumnNameLength+1> fieldName;
			_LIT(KSlashChar, "\\");

			fieldName.Copy(TPtrC(KCDTypeNameIAP));
			fieldName.Append(KSlashChar);
			fieldName.Append(TPtrC(KCDTypeNameRecordTag));
			if ((err = iNotify->ReadInt(fieldName, opt.iIAPId)) != KErrNone)
				return err;

			fieldName.Copy(TPtrC(IAP));
			fieldName.Append(KSlashChar);
			fieldName.Append(TPtrC(KCDTypeNameIAPNetwork));
			if ((err = iNotify->ReadInt(fieldName, opt.iNetworkId)) != KErrNone)
				return err;

			return KErrNone;
			}
		}
	return KErrNotSupported;
	}

TInt CDummyPPPIf::Notification(TAgentToNifEventType /*aEvent*/, void * /*aInfo*/)
	{
	CDummyNifLog::Printf(_L("Dummy PPP Notification\r\n"));	
	return KErrNone;
	}

void CDummyNifLog::Write(const TDesC& aDes)
//
// Write aText to the log
//
	{	
#ifndef __FLOG_ACTIVE
	(void)aDes;
#else
	__FLOG_STATIC0(KDummyPppLogFolder, KDummyPppLogFile, aDes);
#endif
	}
 
void CDummyNifLog::Printf(TRefByValue<const TDesC> aFmt,...)
//
// Write a mulitple argument list to the log, trapping and ignoring any leave
//
	{
#ifndef __FLOG_ACTIVE
	(void)aFmt;
#else
	VA_LIST list;
	VA_START(list, aFmt);

	TBuf <1024> LineBuf;
	LineBuf.AppendFormatList( aFmt, list);

	__FLOG_STATIC0(KDummyPppLogFolder, KDummyPppLogFile, LineBuf);
#endif // #ifdef __FLOG_ACTIVE
	}

void CDummyNifLog::HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen, TInt aWidth)
	{
	TBuf<0x100> buf;
	TInt i = 0;
	const TText* p = aHeader;
	while (aLen>0)
		{
		TInt n = aLen>aWidth ? aWidth : aLen;
		if (p!=NULL)
			{
			_LIT(string1,"%s%04x : ");
			buf.AppendFormat(string1, p, i);
			}
		TInt j;
		_LIT(string2,"%02x ");
		for (j=0; j<n; j++)
			buf.AppendFormat(string2, aPtr[i+j]);
		_LIT(string3,"   ");
		while (j++<KHexDumpWidth)
			buf.Append(string3);
		_LIT(string4," ");
		buf.Append(string4);
		_LIT(string5,"%c");
		for (j=0; j<n; j++)
			buf.AppendFormat(string5, aPtr[i+j]<32 || aPtr[i+j]>126 ? '.' : aPtr[i+j]);
		buf.Append(KEndOfLine);
		Write(buf);
		buf.SetLength(0);
		aLen -= n;
		i += n;
		p = aMargin;
		}
	}



void CDummyPPPIf::CreateL()
	{
	CDummyNifLog::Printf(_L("Dummy PPP CreateL"));
	logUser = new(ELeave) CDummyNifLog;
	TCallBack scb(SendCallBack, this);	
	iSendCallBack = new(ELeave) CAsyncCallBack(scb, KDummyPPPSendPriority);
	}

void CDummyPPPIf::OpenFSM()
{
	logUser->Printf(_L("Open FSM Request\r\n"));
	
	iDummyPPPMaxFailureCount = KDummyPPPMaxRestartConfig;
	
	if(iDummyPPPState == EDummyPPPFsmInitial)
	{
		SetState(EDummyPPPFsmStarting);
		TRAPD(err, InitialiseConfigRequestL());
		if (err==KErrNone)
			SendConfigRequest();
		SetState(EDummyPPPFsmReqSent);
	}
}

void CDummyPPPIf::CloseLog()
	{	
	logUser->Printf(_L("Dummy PPP CloseLog\r\n"));	
	delete logUser;
	}
void CDummyPPPIf::End()
	{
	CDummyNifLog::Printf(_L("Dummy PPP End\r\n"));	
	ipktRecvPPP.Free();
	delete iSendCallBack;
	iSendCallBack = NULL;
	}

//The following code is for logs only
const TText* CDummyNifLog::StateToText(TDummyPPPToPPPFsmState aState)
	{

	// States
	switch (aState)
		{
	case EDummyPPPFsmInitial:
		return _S("INITIAL");
	case EDummyPPPFsmStarting:
		return _S("STARTING");
	case EDummyPPPFsmClosed:
		return _S("CLOSED");
	case EDummyPPPFsmStopped:
		return _S("STOPPED");
	case EDummyPPPFsmStopping:
		return _S("STOPPING");
	case EDummyPPPFsmClosing:
		return _S("CLOSING");
	case EDummyPPPFsmReqSent:
		return _S("REQUEST SENT");
	case EDummyPPPFsmAckRecvd:
		return _S("ACK RECEIVED");
	case EDummyPPPFsmAckSent:
		return _S("ACK SENT");
	case EDummyPPPFsmOpened:
		return _S("OPENED");
	case EDummyPPPFsmPapAuthReqSent:
		return _S("PAP REQUEST SENT");
	case EDummyPPPFsmPapAuthAckRecvd:
		return _S("PAP ACK RECEIVED");
	case EDummyPPPFsmPapAckSent:
		return _S("PAP ACK SENT");
	case EDummyPPPIPCPReqSent:
		return _S("IPCP REQUEST SENT");
	case EDummyPPPFsmIPOpen:
		return _S("IP OPENED");
	default:	// Should never happen in theory!
		return _S("UNKNOWN");
		}
	}


const TText* CDummyNifLog::LcpCodeToText(TUint aValue)
	{

	// LCP Codes
	switch (aValue)
		{
	case KPppLcpEchoRequest:
		return _S("Echo request");
	case KPppLcpEchoReply:
		return _S("Echo reply");
	case KPppLcpDiscardRequest:
		return _S("Discard request");
	case KPppLcpIdentification:
		return _S("Identification");
	case KPppLcpTimeRemaining:
		return _S("Time remaining");
	default:
		return FsmCodeToText(aValue);
		}
	}

const TText* CDummyNifLog::FsmCodeToText(TUint aValue)
	{

	// LCP Codes
	switch (aValue)
		{
	case KPppLcpConfigRequest:
		return _S("Config Request");
	case KPppLcpConfigAck:
		return _S("Config ACK");
	case KPppLcpConfigNak:
		return _S("Config NAK");
	case KPppLcpConfigReject:
		return _S("Config Reject");
	case KPppLcpTerminateRequest:
		return _S("Terminate request");
	case KPppLcpTerminateAck:
		return _S("Terminate ACK");
	case KPppLcpCodeReject:
		return _S("Code Reject");
	case KPppLcpProtocolReject:
		return _S("Protocol Reject");
	case KPppCcpResetReq:
		// Only used by CCP, but this is the easiest place to put it
		return _S("Reset Request");
	case KPppCcpResetAck:
		// Only used by CCP, but this is the easiest place to put it
		//__DEBUGGER();
		return _S("Reset Ack");
	default:
		return _S("Unknown");
		}
	}

const TText* CDummyNifLog::PapCodeToText(TUint aValue)
	{

	// PAP Codes
	switch (aValue)
		{
	case 1:
		return _S("Authenticate Request");
	case 2:
		return _S("Authenticate ACK");
	case 3:
		return _S("Authenticate NAK");
	default:
		return _S("Unknown");
		}
	}

const TText* CDummyNifLog::CbcpCodeToText(TUint aValue)
	{

	// CHAP Codes
	switch (aValue)
		{
	case 1:
		return _S("Callback Request");
	case 2:
		return _S("Callback Response");
	case 3:
		return _S("Callback Ack");
	default:
		return _S("Unknown");
		}
	}

const TText* CDummyNifLog::ChapCodeToText(TUint aValue)
	{

	// CHAP Codes
	switch (aValue)
		{
	case 1:
		return _S("Challenge");
	case 2:
		return _S("Response");
	case 3:
		return _S("Success");
	case 4:
		return _S("Failure");
	default:
		return _S("Unknown");
		}
	}

const TText* CDummyNifLog::IpProtocolToText(TUint aValue)
	{

	// IP Protocols
	switch (aValue)
		{
	case 1:
		return _S("ICMP");
	case 58:
		return _S("ICMPv6");
	case 17:
		return _S("UDP");
	case 6:
		return _S("TCP");

	default:
		return _S("Unknown");
		}
	}

const TText* CDummyNifLog::IpcpCodeToText(TUint aValue)
	{

	// IPCP Codes
	switch (aValue)
		{
	case KPppIpcpOptIpAddresses:
		return _S("IP Addresses");
	case KPppIpcpOptIpCompressionProtocol:
		return _S("IP Compression Protocol");
	case KPppIpcpOptIpAddress:
		return _S("IP Address");
	case KPppIpcpOptPrimaryDnsAddress:
		return _S("Primary DNS Address");
	case KPppIpcpOptSecondaryDnsAddress:
		return _S("Secondary DNS Address");
	case KPppIpcpOptPrimaryNbnsAddress:
		return _S("Primary NBNS Address");
	case KPppIpcpOptSecondaryNbnsAddress:
		return _S("Secondary NBNS Address");
	default:
		return _S("Unknown");
		}
	}

const TText* CDummyNifLog::Ip6cpCodeToText(TUint aValue)
	{
	// IP6CP Codes
	switch (aValue)
		{
	case KPppIp6cpOptInterfaceIdentifier:
		return _S("Interface Identifier");
	case KPppIp6cpOptCompressionProtocol:
		return _S("Compression Protocol");
	default:
		return _S("Unknown");
		}
	}
const TText* CDummyNifLog::CcpCodeToText(TUint aValue)
	{

	// CCP Codes
	switch (aValue)
		{
	case KPppCcpOptOui:
		return _S("OUI");
	case KPppCcpOptPred1:
		return _S("Predictor type1");
	case KPppCcpOptPred2:
		return _S("Predictor type2");
	case KPppCcpOptPuddle:
		return _S("Puddle");
	case KPppCcpOptHP:
		return _S("HP PPC");
	case KPppCcpOptStac:
		return _S("Stac LZS");
	case KPppCcpOptMsoft:
		return _S("Microsoft PPC");
	case KPppCcpOptGandalf:
		return _S("Gandalf");
	case KPppCcpOptV42bis:
		return _S("V42bis");
	case KPppCcpOptReserved:
		return _S("Reserved");
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		return _S("unassigned");
	default:
		return _S("Unknown");
		}
	}


TInt CDummyNifLog::DumpLcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	if(len>aDes.Length())
		{
		Printf(_L("WARNING: Length reported in header was more than actual frame length\n"));
		return len;
		}
	else if (len<aDes.Length())
		Printf(_L("WARNING: Length reported in header was less than actual frame length!\n"));

	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);
	
	Printf(KCodeTextString, LcpCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			TInt tmp = 0;
			while (n>0 && aDes.Length()>0)
				{
				tmp = DumpLcpOption(aDes);
				if (tmp == 0)	//Check for an error in the DumpLcpOption (0 is interpreted as an error)
					{
					Printf(_L("WARNING: Length reported in LCP Option was incorrect! Dumping data instead\n"));
					DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
					break;
					}
				n=n-tmp;
				}	

			if (n>0)
				Printf(KBytesRemainingString, n, (n>1)?"s":"");
			}

		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = *ptr;
			Printf(KCodeString, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);		
			Printf(KProtocolString, val);
			}
		break;
	case KPppLcpEchoRequest:
			{
			TUint val = BigEndian::Get32(ptr);	
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpEchoReply:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpDiscardRequest:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpIdentification:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			if (len>0)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+4, len-8);
			}
		break;
	case KPppLcpTimeRemaining:
			{
			TUint val = BigEndian::Get32(ptr);
			ptr+=4;
			Printf(KNumberString, val);
			val = BigEndian::Get32(ptr);
			Printf(KSecondsString, val);
			}
		break;
	default:
		break;
		}

	return len;
	}

TInt CDummyNifLog::DumpLcpOption(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 opt = *ptr++;
	TUint8 len = *ptr++;

	if(len > aDes.Length())
		{
		Printf(_L("WARNING: Length reported in option header was more than actual frame length\n"));
		return 0;	//Return an incorrect length to flag this error.
		}

	if(len < 2)
		{
		Printf(_L("WARNING: Length reported in option header was less than a correct frame length\n"));
		return 0;	//Return an incorrect length. (the minimum correct length of an option is 2)
		}
	
	Printf(KLcpCodeString, LcpOptToText(opt), opt);
	Printf(KLengthString, len);
	
	switch (opt)
		{
	case KPppLcpOptMaxRecvUnit:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KSizeString, val);
			}
		break;
	case KPppLcpOptEscapeCharMap:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KMapString, val);
			}
		break;
	case KPppLcpOptAuthenticationProtocol:
			{
			TUint val = BigEndian::Get16(ptr);

			Printf(KProtocolTextString, ProtocolToText(val), val);
			if (val != KPppIdChap)
				{
				if (len>4)
					DumpBytes(EIGHT_SPACE_MARGIN, ptr+2, len-4);
				}
			else
				{
				if(len>4)
					DumpChapType(ptr+2);
				}
			}
		break;
	case KPppLcpOptQualityProtocol:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolTextString, ProtocolToText(val), val);
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+2, len-4);
			}
		break;
	case KPppLcpOptMagicNumber:
			{
			TUint val = BigEndian::Get32(ptr);
			Printf(KNumberString, val);
			}
		break;
	case KPppLcpOptProtocolCompress:
	case KPppLcpOptAddrCtrlCompress:
		break;
	case KPppLcpOptFcsType:
			{
			TUint val = *ptr++;
			//only for display
			//Printf(KLcpOptFcsTypeString, (val&KPppDummyPPPFcs0Flag)?_S(" None"):_S(""), (val&KPppDummyPPPFcs16Flag)?_S(" 16 bit"):_S(""), (val&KPppDummyPPPFcs32Flag)?_S(" 32 bit"):_S(""));
			Printf(KLcpOptFcsTypeString, val?_S(" None"):_S(""));
			}
		break;
	case KPppLcpOptPadding:
			{
			TUint val = *ptr++;
			Printf(KMaximumBytesString, val);
			}
		break;
	case KPppLcpOptCallback:
			{
			TUint val = *ptr++;
			Printf(KCodeTextString, CallbackOpToText(val), val);
			if (len>3)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr+3, len-3);
			}
		break;
	case KPppLcpOptCompoundFrames:
		break;
		}
	aDes.Set((TUint8*)aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

TInt CDummyNifLog::DumpPap(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;

	Printf(KCodeTextString, PapCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	if (code==1)
		{
		TInt n = *ptr++;
		TPtrC8 v;
		v.Set(ptr, n);    ptr += n;
		TBuf16<KLogBufferSize> temp;
		temp.Copy(v.Left(Min(v.Length(),KLogBufferSize)));
		Printf(KUserNameString, &temp);
		n = *ptr++;
		v.Set(ptr, n);    ptr += n;
		temp.Copy(v.Left(Min(v.Length(),KLogBufferSize)));
		Printf(KPasswordString, &temp);
		}
	else if (code==2 || code==3)
		{
		TInt n = *ptr++;
		TPtrC8 v;
		v.Set(ptr, n);    ptr += n;
		TBuf16<KLogBufferSize> temp;
		temp.Copy(v.Left(Min(v.Length(),KLogBufferSize)));
		Printf(KMessageString, &temp);
		}

	aDes.Set(aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

TInt CDummyNifLog::DumpCbcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	TInt length = len;
	ptr += 2;

	Printf(KCodeTextString, CbcpCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);

	//
	// OK we can be offered many options at once, so 
	// loop for the length and decode them.
	//
	len -= 4; // Subtract the stuff we know about

	TUint8	type;
	TUint	typeLen;
	while (len)
		{
		// Yes I know we can have a permanent loop but I just don't care
		type = *ptr++;
		switch(type)
			{
		case 1:
				{
				_LIT(string1,"        No CallBack\n");
				Write(string1);
				}
			break;
		case 2:
				{
				_LIT(string2,"        Callback to a user specified No.\n");
				Write(string2);
				}
			break;
		case 3:
				{
				_LIT(string3,"        Callback to a pre specified No.\n");
				Write(string3);
				}
			break;
		case 4:
				{
				_LIT(string4,"        Callback to one of a list of numbers\n");
				Write(string4);
				}
			break;
		default:
				{
				_LIT(string5,"        Unknown Callback type\n");
				Write(string5);
				}
			break;
		}

		if (len == 1)
			len = 0;

		typeLen = *ptr++;
			Printf(KLengthString,typeLen);

		if (typeLen > 2)
		 	Printf(KDelayString,*ptr++);

		if (typeLen > 3)
			{
			//
			// Dump the Address Type field
			//
			if (*ptr++ == 1)
				{
				_LIT(string6,"            PSTN/ISDN\n");
				Write(string6);
				}
			else
				{
				_LIT(string7,"            Other\n");
				Write(string7);
				}
			}

		//
		// Dump the NULL terminated ASCII string
		//
 		DumpBytes(EIGHT_SPACE_MARGIN, ptr, (typeLen-4));
		len -= typeLen;
		}

	aDes.Set(aDes.Ptr()+length, aDes.Length()-length);
	return length;
	}

TInt CDummyNifLog::DumpChap(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;

	Printf(KCodeTextWithoutSpacesString,FsmCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	if (code==1 || code==2)
		{
		TInt n = *ptr++;
		DumpBytes(EIGHT_SPACE_MARGIN, ptr, n);
		ptr += n;

		n = len-(4+1+n);
		TPtrC8 v;
		v.Set(ptr, n);    ptr += n;
		TBuf16<KLogBufferSize> temp;
		temp.Copy(v);
		Printf(KNameString, &temp);
		}
	else if (code==3 || code==4)
		{
		TInt n = (len-4);
		TPtrC8 v;
		v.Set(ptr, n);
		ptr += n;
		TBuf16<KLogBufferSize> temp;
		temp.Copy(v);
		Printf(KMessageString, &temp);
		}

	aDes.Set(aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}


TInt CDummyNifLog::DumpCcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);

	Printf(KCodeTextWithoutSpacesString,FsmCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			TInt tmp = 0;
			while (n>0 && aDes.Length()>0)
				{
				tmp = DumpCcpOption(aDes);
				if (tmp == 0) 	//Check for an error in the DumpCcpOption (0 is interpreted as an error)
					{
					Printf(_L("WARNING: Length reported in CCP Option was incorrect! Dumping data instead\n"));
					DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
					break;
					}
				n=n-tmp;
				}	

			if (n>0)
				Printf(KBytesRemainingString, n, (n>1)?"s":"");
			}
		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KCodeString, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolString, val);
			}
		break;
		}
	return len;
	}

TInt CDummyNifLog::DumpCcpOption(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 opt = *ptr++;
	TUint8 len = *ptr++;

	if(len > aDes.Length())
		{
		Printf(_L("WARNING: Length reported in option header was more than actual frame length\n"));
		return 0;	//Return an incorrect length to flag this error.
		}

	if(len < 2)
		{
		Printf(_L("WARNING: Length reported in option header was less than a correct frame length\n"));
		return 0;	//Return an incorrect length. (the minimum correct length of an option is 2)
		}

	Printf(KCodeTextString, CcpCodeToText(opt), opt);
	Printf(KLengthString, len);

	DumpBytes(FOURTEEN_SPACE_MARGIN, ptr, len-2);
	aDes.Set((TUint8*)aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

TInt CDummyNifLog::DumpIp6cp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);

	Printf(KCodeTextWithoutSpacesString, FsmCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			//only for testing display
			//while (n>0 && aDes.Length()>0)
			//	n -= DumpIp6cpOption(aDes);
			if (n>0)
				Printf(KBytesRemainingString, n, (n>1)?"s":"");
			}
		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KCodeString2, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolString, val);
			}
		break;
		}
	return len;
	}
TInt CDummyNifLog::DumpIp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 c = *ptr++;
//	TUint ver = c >> 4;
	TUint hlen = (c & 0xf) << 2;
//	TUint8 tos = *ptr;
	ptr++;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 id = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 frag = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TBool zf = (frag & 0x8000);
	TBool df = (frag & 0x4000);
	TBool mf = (frag & 0x2000);
	frag = (TUint16)((frag & 0x1fff)<<3);
//	TUint8 ttl = *ptr;
	ptr++;
	TUint8 proto = *ptr++;
//	TUint16 chksum = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint32 srca = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 dsta = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
//	TBool opts = (hlen>20);

	Printf(KHdrLengthString, len, hlen);
	Printf(KSrcDstAddrString,(srca&0xff000000)>>24,(srca&0x00ff0000)>>16,(srca&0x0000ff00)>>8,(srca&0x000000ff),
							 (dsta&0xff000000)>>24,(dsta&0x00ff0000)>>16,(dsta&0x0000ff00)>>8,(dsta&0x000000ff));
	Printf(KIDFragmentString, id, frag, df?_S("<DF>"):_S(""), mf?_S("<MF>"):_S(""), zf?_S("<Z>"):_S(""));
//	Printf(KTOSTTLChksumString, tos, ttl, chksum);
	Printf(KCodeTextString, IpProtocolToText(proto), proto);
	
	if (hlen>20)
		ptr += (hlen-20);
	
	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;
	aDes.Set(ptr, tlen);

	switch (proto)
		{
	case 1:
		return n+DumpIcmp(aDes, tlen);
	case 6:
		return n+DumpTcp(aDes, srca, dsta, tlen);
	case 17:
		return n+DumpUdp(aDes, srca, dsta, tlen);
	default:
		return n;
		}
	}

TInt CDummyNifLog::DumpIpcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 code = *ptr++;
	TUint8 id = *ptr++;
	TInt len = BigEndian::Get16(ptr);
	ptr += 2;
	aDes.Set(ptr, aDes.Length()-4);

	Printf(KCodeTextWithoutSpacesString, FsmCodeToText(code), code);
	Printf(KIdentifierLengthString, id, len);
	
	switch (code)
		{
	case KPppLcpConfigRequest:
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
			{
			TInt n = len-4;
			TInt tmp = 0;
			while (n>0 && aDes.Length()>0)
				{
				tmp = DumpIpcpOption(aDes);
				if (tmp == 0)  	//Check for an error in the DumpIpcpOption (0 is interpreted as an error)
					{
					Printf(_L("WARNING: Length reported in IPCP Option was incorrect! Dumping data instead\n"));
					DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
					break;
					}
				n=n-tmp;
				}	

			if (n>0)
				Printf(KBytesRemainingString, n, (n>1)?"s":"");
			}
		break;
	case KPppLcpTerminateRequest:
	case KPppLcpTerminateAck:
			{
			if (len>4)
				DumpBytes(EIGHT_SPACE_MARGIN, ptr, len-4);
			}
		break;
	case KPppLcpCodeReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KCodeString2, val);
			}
		break;
	case KPppLcpProtocolReject:
			{
			TUint val = BigEndian::Get16(ptr);
			Printf(KProtocolString, val);
			}
		break;
		}
	return len;
	}

TInt CDummyNifLog::DumpIpcpOption(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 opt = *ptr++;
	TUint8 len = *ptr++;

	if(len > aDes.Length())
		{
		Printf(_L("WARNING: Length reported in option header was more than actual frame length\n"));
		return 0;	//Return an incorrect length to flag this error.
		}

	if(len < 2)
		{
		Printf(_L("WARNING: Length reported in option header was less than a correct frame length\n"));
		return 0;	//Return an incorrect length. (the minimum correct length of an option is 2)
		}

	Printf(KCodeTextString, IpcpCodeToText(opt), opt);
	Printf(KLengthString2, len);
	
   TUint32 val;

	switch (opt)
		{
	case KPppIpcpOptIpAddresses:
		val = BigEndian::Get32(ptr);
		Printf(KRemoteAddrString,(val&0xff000000)>>24,(val&0x00ff0000)>>16,(val&0x0000ff00)>>8,(val&0x000000ff));
		val = BigEndian::Get32(ptr+4);
		Printf(KOurAddrString,(val&0xff000000)>>24,(val&0x00ff0000)>>16,(val&0x0000ff00)>>8,(val&0x000000ff));
		break;
	case KPppIpcpOptIpCompressionProtocol:
			{
			TUint prot = BigEndian::Get16(ptr);
			ptr += 2;
			Printf(KProtocolTextString, ProtocolToText(prot), prot);
			if (len>4)
				{
				switch (prot)
					{
				case KPppIdVjCompTcp:
					Printf(KMaxSlotString, *ptr++);
					Printf(KCompSlotString, *ptr++);
					break;
				default:
					DumpBytes(FOURTEEN_SPACE_MARGIN, ptr+4, len-4);
					break;
					}
				}
			}
		break;
	case KPppIpcpOptIpAddress:
	case KPppIpcpOptPrimaryDnsAddress:
	case KPppIpcpOptSecondaryDnsAddress:
	case KPppIpcpOptPrimaryNbnsAddress:
	case KPppIpcpOptSecondaryNbnsAddress:
			{
			val = BigEndian::Get32(ptr);
			Printf(KAddrString,(val&0xff000000)>>24,(val&0x00ff0000)>>16,(val&0x0000ff00)>>8,(val&0x000000ff));
			}
		break;
		}
	aDes.Set((TUint8*)aDes.Ptr()+len, aDes.Length()-len);
	return len;
	}

TInt CDummyNifLog::DumpIp6(TPtrC8& aDes)
	{
	// This dumps the main IPv6 Header, no support for option headers
	// IPv6 Headers are FIXED length, but are chainable...
	TUint16* ptr = (TUint16*)aDes.Ptr();
	
	// First 32 bits contain version(4), class(8), Flow Label(20)
	
	TUint32 first = ByteOrder::Swap32((TUint32) *ptr);
	//TUint8 ver = (TUint8)((first >> 28) & 0xf);
	TUint8 cls = (TUint8)((first >> 20) & 0xff);
	TUint32 flowLab = (first & 0xfffff);
	ptr+=2;

	// Second 32 bits contain payload length (16), next header type (8), hop limit (8)
	TUint16 payLoadLen = ByteOrder::Swap16( *ptr++ );
	TUint16 next = ByteOrder::Swap16( *ptr++ ); 
	TUint8 nextHead = (TUint8)((next >> 8) & 0xff);
	TUint8 hopLimit = (TUint8)(next & 0xff);
	
	// Source address (128 bits)
	TUint16 s1 = ByteOrder::Swap16( *ptr++ );
	TUint16 s2 = ByteOrder::Swap16( *ptr++ );
	TUint16 s3 = ByteOrder::Swap16( *ptr++ );
	TUint16 s4 = ByteOrder::Swap16( *ptr++ );
	TUint16 s5 = ByteOrder::Swap16( *ptr++ );
	TUint16 s6 = ByteOrder::Swap16( *ptr++ );
	TUint16 s7 = ByteOrder::Swap16( *ptr++ );
	TUint16 s8 = ByteOrder::Swap16( *ptr++ );
	
	// Destination address (128 bits)
	TUint16 a1 = ByteOrder::Swap16( *ptr++ );
	TUint16 a2 = ByteOrder::Swap16( *ptr++ );
	TUint16 a3 = ByteOrder::Swap16( *ptr++ );
	TUint16 a4 = ByteOrder::Swap16( *ptr++ );
	TUint16 a5 = ByteOrder::Swap16( *ptr++ );
	TUint16 a6 = ByteOrder::Swap16( *ptr++ );
	TUint16 a7 = ByteOrder::Swap16( *ptr++ );
	TUint16 a8 = ByteOrder::Swap16( *ptr++ );
	
	//Printf(_L("    Version is %d"),ver);	// Should be 6 for IPv6 !!
	Printf(KIpv6Class, cls);
	Printf(KIpv6FlowLabel, flowLab);
	Printf(KIpv6PayloadLen, payLoadLen);
	Printf(KIpv6NextHeadType, nextHead);
	Printf(KIpv6HopLimit, hopLimit);
	Printf(KIpv6SrcAddress, s1,s2,s3,s4,s5,s6,s7,s8);
	Printf(KIpv6DstAddress, a1,a2,a3,a4,a5,a6,a7,a8);
	
	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;
	aDes.Set( (TUint8*) ptr, tlen);

	switch (nextHead)
		{
		case 6:		// TCP Packet
			Printf(_L("        TCP Data"));
			//DumpTcp(aDes);
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 17:	// UDP Packet
			Printf(_L("        UDP Data"));
			DumpUdp(aDes,s8,a8,tlen);
			break;
		case 43:	// Routing Header
			Printf(_L("        Routing Header"));
			//DumpRouter
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 44:	// Fragment Header
			Printf(_L("        Packet Fragment"));
			//DumpFragment
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 50:	// ESP Payload
			Printf(_L("        ESP Payload"));
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		case 58:	// ICMPv6
			Printf(_L("        ICMPv6"));
			DumpIcmp(aDes,tlen);
			break;
		case 59:	// No Next Header
			Printf(_L("        NO NEXT HEADER"));
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
			break;
		default:
			Printf(KIpv6UnknownHeadType, nextHead);
			HexDump(EIGHT_SPACE_MARGIN,EIGHT_SPACE_MARGIN, aDes.Ptr(), tlen);
		}

	return n;	
	}

TInt CDummyNifLog::DumpVjCompTcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint16	change = *ptr++;
	TUint8	urgent=0;
	TInt16	window=0;
	TUint16	ack=0;
	TUint16	sequence=0;
	TBuf<50> changeMaskBuf;;
	TUint8	connection=0;

	changeMaskBuf.Append(KChangeMaskString);

	if (change & KVjCompMaskConn)
		{	
		_LIT(string7,"C");
		changeMaskBuf.Append(string7);
		connection = *ptr++;
		}

	TUint16 checksum = BigEndian::Get16(ptr);
	ptr += 2;

	if (change & KVjCompMaskPush)
		{
		_LIT(string8,"P");
		changeMaskBuf.Append(string8);
		}

	/*
	*	Don't reorder SpecialI && SpecialD, they are like this
	*	as SpecialD is the SWAU bits and SpecialI is SWU
	*
	*/

	if ((change & KVjCompMaskSpecials) == KVjCompMaskSpecialD)
		{
		_LIT(string1,"	Special D");
		Write(string1);
		}
	else if ((change & KVjCompMaskSpecials) == KVjCompMaskSpecialI)
		{
		_LIT(string2,"	Special I");
		Write(string2);
		}
	else
		{
		if (change & KVjCompMaskUrgent)
			{
			_LIT(string3,"U");
			changeMaskBuf.Append(string3);
			urgent = *ptr++;
			}

		if (change & KVjCompMaskWindow )
			{
			//only for display
			//window = (TInt16)DecodeSignedDelta(ptr);
			_LIT(string4,"W");
			changeMaskBuf.Append(string4);
			}

		if (change & KVjCompMaskAck )
			{
			//only for display
			//ack = DecodeDelta(ptr);
			_LIT(string5,"A");
			changeMaskBuf.Append(string5);
			}

		if (change & KVjCompMaskSeq)
			{	
			//only for display
			//sequence = DecodeDelta(ptr);
			_LIT(string6,"S");
			changeMaskBuf.Append(string6);
			}
		}


	TUint16	ipId=0;
	if (change & KVjCompMaskIp)
		{	
		//only for display
	    //ipId = DecodeDelta(ptr);
		_LIT(string9,"I");
		changeMaskBuf.Append(string9);
		}

	Printf(TRefByValue<const TDesC>(changeMaskBuf), change);

	Printf(KChecksumString, checksum);

	if (change & KVjCompMaskConn)
		{
		Printf(KConnectionString,connection);
		}

	if	(urgent)
		{
		_LIT(string10,"	Urgent Delta = 0x%x");
		Printf(string10,urgent);
		}

	if (window)
		{
		_LIT(string11,"	Window Delta = %d");
		Printf(string11,window);
		}

	if (ack)
		{
		_LIT(string12,"	Ack Delta = 0x%x");
		Printf(string12,ack);
		}

	if (sequence)
		{
		_LIT(string13,"	Sequence Delta = 0x%x");
		Printf(string13,sequence);
		}

	if (ipId)
		{
		_LIT(string14,"	IPId = 0x%x");
		Printf(string14,ipId);
		}

	return 1;
	}
TInt CDummyNifLog::DumpVjUncompTcp(TPtrC8& aDes)
	{

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 c = *ptr++;
//	TUint ver = c >> 4;
	TUint hlen = (c & 0xf) << 2;
//	TUint8 tos = *ptr;
	ptr++;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr++ | *ptr++<<8));
	TUint16 id = ByteOrder::Swap16((TUint16)(*ptr++ | *ptr++<<8));
	TUint16 frag = ByteOrder::Swap16((TUint16)(*ptr++ | *ptr++<<8));
	TBool zf = (frag & 0x8000);
	TBool df = (frag & 0x4000);
	TBool mf = (frag & 0x2000);
	frag = (TUint16)((frag & 0x1fff)<<3);
//	TUint8 ttl = *ptr;
	ptr++;
	TUint8 proto = *ptr++;
//	TUint16 chksum = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint32 srca = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 dsta = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
//	TBool opts = (hlen>20);

	Printf(KHdrLengthString, len, hlen);
	Printf(KSrcDstAddrString, (srca&0xff000000)>>24,(srca&0x00ff0000)>>16,(srca&0x0000ff00)>>8,(srca&0x000000ff),
							  (dsta&0xff000000)>>24,(dsta&0x00ff0000)>>16,(dsta&0x0000ff00)>>8,(dsta&0x000000ff));
	Printf(KIDFragmentString, id, frag, df?_S("<DF>"):_S(""), mf?_S("<MF>"):_S(""), zf?_S("<Z>"):_S(""));
//	Printf(KTOSTTLChksumString, tos, ttl, chksum);
	Printf(KConnectionNoString, proto);
	
	if (hlen>20)
		ptr += (hlen-20);
	
	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;
	aDes.Set(ptr, tlen);
	return n+DumpTcp(aDes, srca, dsta, tlen);
	}

TInt CDummyNifLog::DumpTcp(TPtrC8& aDes, TUint32 aSrcA, TUint32 aDstA, TInt aLength)
	{
	
	TInt n = Min(aLength, aDes.Length());
	TInt len = n;
	
	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 osum0 = ptr[16];
	ptr[16] = 0;
	TUint8 osum1 = ptr[17];
	ptr[17] = 0;

	TUint32 sum = 0;
	sum += (aSrcA >> 16);
	sum += (aSrcA & 0xffff);
	sum += (aDstA >> 16);
	sum += (aDstA & 0xffff);
	sum += 6;
	sum += n;
	while (n>1)
		{
		sum += (ptr[0]<<8);
		sum += (ptr[1]);
		ptr += 2;
		n -= 2;
		}
	if (n>0)
		sum += (ptr[0]<<8);
	while (sum>0xffff)
		{
		sum = (sum & 0xffff) + (sum>>16);
		}
	sum = ~sum & 0xffff;
	ptr = (TUint8*)aDes.Ptr();
	ptr[16] = osum0;
	ptr[17] = osum1;
	
	TUint srcp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint dstp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint32 seqnum = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 acknum = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;

	TUint8 c = *ptr++;
	TUint hlen = (c>>4)<<2;

	c = *ptr++;
	TUint urgf = c & 0x20;
	TUint ackf = c & 0x10;
	TUint pshf = c & 0x08;
	TUint rstf = c & 0x04;
	TUint synf = c & 0x02;
	TUint finf = c & 0x01;

	TUint window = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	TUint chksum = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	TUint urgptr = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	
	Printf(KTCPLengthString, len, hlen);
	Printf(KPortString, srcp, dstp);
//	Printf(KDestPortString, dstp);
//	Printf(KTCPHeaderLengthString, hlen);
	Printf(KSeqAckString, seqnum, acknum);
	Printf(KFlagsString, urgf?_S(" <URG>"):_S(""), ackf?_S(" <ACK>"):_S(""), pshf?_S(" <PSH>"):_S(""),
		                      rstf?_S(" <RST>"):_S(""), synf?_S(" <SYN>"):_S(""), finf?_S(" <FIN>"):_S(""));
	Printf(KWindowUrgentString, window, urgptr);
	if (chksum != sum)
		Printf(KChecksumString3, chksum, sum);

	if (hlen>20)
		{
		_LIT(string2,"	TCP Options %d bytes");
		Printf(string2, hlen-20);
		TInt h, i, opt, optlen=0;
		h = hlen-20;
		for (i=0; i<h; i+=optlen)
			{
			opt = ptr[i];
			if (opt == 0) // KTcpOptEol
				break;
			if (opt == 1) // KTcpOptNop
				optlen = 1;
			else
				{
				if (i+1 >= h)
					break;
				optlen = ptr[i+1];
				if (optlen < 2)
					optlen = 2;
				}

			switch (opt)
				{
			case 1:
					{
					_LIT(string3,"	    NOP");
					Write(string3);
					}
				break;
			case 2:
					{
					_LIT(string4,"	    Max Seg Size = %d");
					Printf(string4, BigEndian::Get16(ptr+i+2));
					}
				break;
			default:
					{
					_LIT(string5,"	    Unknown [0x%02x]");
					Printf(string5, opt);
					}
				break;
				}
			}
		}
	
	ptr += (hlen-20);
	TInt n1 = (TInt)aDes.Ptr()-(TInt)ptr;
	aDes.Set(ptr, aDes.Length()-n1);
	return n1;
	}

TInt CDummyNifLog::DumpIcmp(TPtrC8& aDes, TInt aLength)
	{

	if (aLength < 2)
		return 0;

	TUint8* ptr = (TUint8*)aDes.Ptr();

	_LIT(string1,"	    Type = %d, Code = %d\n");
	Printf(string1, *ptr, *(ptr+1));
	HexDump(FOURTEEN_SPACE_MARGIN, FOURTEEN_SPACE_MARGIN, ptr, aLength);
	return 0;
	}

TInt CDummyNifLog::DumpUdp(TPtrC8& aDes, TUint32 /* aSrcA */, TUint32 /*aDstA */, TInt aLength)
	{
	if (aLength < 6)
		return 0;

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint srcp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint dstp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr += 2;

	ptr += 2;						// skip checksum
	Printf(KUDPLengthPortString, len, srcp, dstp);
	//Printf(KPortString, srcp, dstp);

	//for testing only
	return 0;
	}

void CDummyNifLog::Dump(RMBufChain& aPacket, TInt aChannel)
	{

	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
	TUint prot = TPppAddr::Cast(info->iDstAddr).GetProtocol();
	
	TBuf8<1536> buf;
	buf.SetMax();
	aPacket.CopyOut(buf, aPacket.First()->Length());
	
	if (aChannel==KDummyPPPSendChannel)
		{
		_LIT(string1,"Dummy PPP Send %d bytes");
		Printf(string1, info->iLength);
		}
	else if (aChannel==KDummyPPPRecvChannel)
		{
		_LIT(string2,"Dummy PPP Recv %d bytes");
		Printf(string2, info->iLength);
		}
	else
		{
		_LIT(string3,"Dummy PPP %d bytes");
		Printf(string3, info->iLength);
		}

	TPtrC8 des(buf);
	switch (prot)
		{
	case KPppIdLcp:
		DumpLcp(des);
		break;
	case KPppIdPap:
		DumpPap(des);
		break;
	case KPppIdChap:
		DumpChap(des);
		break;
	case KPppIdIpcp:
		DumpIpcp(des);
		break;
	case KPppIdIp6cp:
		DumpIp6cp(des);
		break;
	case KPppIdIp:
		DumpIp(des);
		break;
	case KPppIdIp6:
		DumpIp6(des);
		break;
	case KPppIdVjCompTcp:
		DumpVjCompTcp(des);
		break;
	case KPppIdVjUncompTcp:
		DumpVjUncompTcp(des);
		break;

	default:
		Printf(_L("Raw Bytes:"));
		HexDump(NULL,_S(""),des.Ptr(), des.Length(),8);
		Write(KEndOfLine);
	}
}

void CDummyNifLog::DumpChapType(const TUint8* aPtr)
	{

	switch (*aPtr)
		{
	case 5:
			{
			_LIT(string1,"	    Algorithm=MD5\n");
			Write(string1);
			}
		break;
	case 0x80:
			{
			_LIT(string2,"	    Algorithm=Microsoft\n");
			Write(string2);
			}
		break;
	default:
			{
			_LIT(string3,"	    Algorithm=Unknown\n");
			Write(string3);
			}
		break;
		}
	}

const TText* CDummyNifLog::CallbackOpToText(TUint aValue)
	{

	// CB Codes
	switch (aValue)
		{
	case 0:
		return _S("Location preset");
	case 1:
		return _S("Dialing string");
	case 2:
		return _S("Location identifier");
	case 3:
		return _S("E.164 number");
	case 4:
		return _S("Distinguished name");
	case 5:
		return _S("E.165 number");
	case 6:
		return _S("MS CBCP");
	default:
		return _S("Unknown operation");
		}
	}

const TText* CDummyNifLog::LcpOptToText(TUint aValue)
	{

	// LCP Options
	switch (aValue)
		{
	case KPppLcpOptMaxRecvUnit:
		return _S("Max receive size");
	case KPppLcpOptEscapeCharMap:
		return _S("Escape char map");
	case KPppLcpOptAuthenticationProtocol:
		return _S("Authentication protocol");
	case KPppLcpOptQualityProtocol:
		return _S("Quality protocol");
	case KPppLcpOptMagicNumber:
		return _S("Magic number");
	case KPppLcpOptProtocolCompress:
		return _S("Protocol field compression");
	case KPppLcpOptAddrCtrlCompress:
		return _S("Addr & Ctrl field compression");
	case KPppLcpOptFcsType:
		return _S("Fcs Type");
	case KPppLcpOptPadding:
		return _S("Padding");
	case KPppLcpOptCallback:
		return _S("Callback protocol");
	case KPppLcpOptCompoundFrames:
		return _S("Compound frames");
	case KPppLcpOptMRRU:
 		return _S("MRRU");
 	case KPppLcpOptMultiLinkEndPointDescriminator:
 		return _S("Multi-link End-Point Descriminator");
	default:
		return _S("Unknown");
		}
	}

const TText* CDummyNifLog::ProtocolToText(TUint aValue)
	{

	// Protocols
	switch (aValue)
		{
	case KPppIdLcp:
		return _S("LCP");
	case KPppIdPap:
		return _S("PAP");
	case KPppIdChap:
		return _S("CHAP");
	case KPppIdMsCbcp:
		return _S("MSCBCP");
	case KPppIdIpcp:
		return _S("IPCP");
	case KPppIdIp6cp:
		return _S("IP6CP");
	case KPppIdIp:
		return _S("IP");
	case KPppIdIp6:
		return _S("IPv6");
	case KPppIdVjCompTcp:
		return _S("VJ Comp tcp");
	case KPppIdVjUncompTcp:
		return _S("VJ Uncomp tcp");
	case KPppIdCcp:
		return _S("CCP");
	case KPppIdCompressed:
		return _S("PPP Compressed");
	default:
		return _S("Unknown");
		}
	}

void CDummyNifLog::LogUserData(RMBufChain& aPacket, TInt aChannel)
//
// Stores the current total of data transferred
//
{
	TBuf8<1536> buf;
	buf.SetMax();
	aPacket.CopyOut(buf, aPacket.First()->Length());
	
	TPtrC8 des(buf);

	TUint8* ptr = (TUint8*)des.Ptr();
	ptr+=2;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));

	if (aChannel==KDummyPPPSendChannel)
	{
		iSentData+=len;
	}
	else
	{
		if (aChannel==KDummyPPPRecvChannel)
			iRecvdData+=len;
	}

}

void CDummyNifLog::DumpBytes(const TText* aMargin, const TUint8* aPtr, TInt aLen)
	{
	HexDump(NULL,aMargin,aPtr,aLen,8);
	}
