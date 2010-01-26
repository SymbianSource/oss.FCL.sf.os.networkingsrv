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
// See RFC 1662 and "PPP Design and Debugging" by James Carlson (ISBN 0201185393)
// PPP's HDLC support
// 
//

/**
 @file
*/

#include <cdblen.h>
#include <es_ini.h>
#include "PPPHDLC.H"
#include "PPPLOG.H"
#include "ncpip.h"
#include "PPPConfig.h"

#include <commsdattypeinfov1_1.h>

// Receive buffer size for the BCA layer. HDLC has a separate Rx buffer.
static const TInt KReceiveBufferSize=4096; 

// Internal flag bits (iFlags)
static const TUint KPppHdlcSendFcs16 = 0x00000001;
static const TUint KPppHdlcSendFcs32 = 0x00000002;
static const TUint KPppHdlcSendCompProt = 0x00000004;
static const TUint KPppHdlcSendCompAddrCtrl = 0x00000008;
static const TUint KPppHdlcRecvFcs16 = 0x00000010;
static const TUint KPppHdlcRecvFcs32 = 0x00000020;
static const TUint KPppHdlcRecvCompProt = 0x00000040;
static const TUint KPppHdlcRecvCompAddrCtrl = 0x00000080;
//static const TUint KPppHdlcRecvEscSeen = 0x00000100;
//static const TUint KPppHdlcSendXonXoff = 0x00000400;
//static const TUint KPppHdlcRecvXonXoff = 0x00000800;
static const TUint KPppHdlcSendBusy = 0x00001000;
static const TUint KPppHdlcRecvEscPending = 0x00002000;
static const TUint KPppHdlcCommConfigOk = 0x00004000;
static const TUint KPppHdlcApplyPending = 0x00010000;
static const TUint KPppHdlcAtApplyMark = 0x00020000;
static const TUint KPppHdlcOrigConfig = 0x000040000;

/*
#if defined(_DEBUG) && defined (__WINS__)
#include <f32file.h>
#endif
*/

//
// HDLC Support
//

static const TInt KPppLoWat = 16;
static const TInt KPppHiWat = 32;

static const TInt KOptionListLen = 6;
static const TUint8 OptionList[KOptionListLen] =
	{
	KPppLcpOptEscapeCharMap,
	KPppLcpOptProtocolCompress,
	KPppLcpOptAddrCtrlCompress,
	KPppLcpOptFcsType,
	KPppLcpOptPadding,
	KPppLcpOptCompoundFrames
	};

CPppHdlcLink::CPppHdlcLink(CPppLcp* aLcp)
	: CPppLinkBase(aLcp),
	  iError(KErrNone),
	  iLinkDown(ETrue) // By default
	{
	OptRegister(iPppLcp, OptionList, KOptionListLen);
	iSendLoWat = KPppLoWat;
	iSendHiWat = KPppHiWat;
	}

CPppHdlcLink::~CPppHdlcLink()
	{
	iSendPkt.Free();
	iSendQ.Free();
	iRecvPkt.Free();
	if (iRecvMBuf)
		{
		iRecvMBuf->Free();
		}
	iRecvQ.Free();

	delete iBcaReader;
	delete iBcaWriter;
	delete iBcaControl; // Must be deleted last, as it makes sure the BCA is closed.
	
	if(iBca) // Remove the BCA itself, if it was created.
		{
		iBca->Release();
		}
	}
	
/**
Allocates the link resources, associated with BCA 
 
@leave If the link cannot be created.
*/
void CPppHdlcLink::CreateL()
    {
    if (iPppLcp->GetBCAProvision() == NULL)
        {
        User::Leave(KErrNotReady);
        }
        
	LoadBcaL(); 
	__ASSERT_ALWAYS(iBca, User::Panic(_L("PPP Panic"), EPppPanic_NullBca));
	
	iBcaControl = new (ELeave)CBcaControl(*this, *iBca); 
	iBcaReader  = new (ELeave)CBcaReader( *this, *iBca, KPppHdlcCommReadPriority);
	iBcaWriter  = new (ELeave)CBcaWriter( *this, *iBca, KPppHdlcCommWritePriority);	
    }
    
/** 
Loads the BCA based on Comms settings 

@leave if the BCA cannot be loaded
*/
void CPppHdlcLink::LoadBcaL()
	{
	LOG( Logger().Printf(_L("CPppHdlcLink::LoadBcaL: Loading BCA.[%S]"), GetBCAName().Ptr()); )
		
	User::LeaveIfError(iBcaDll.iObj.Load(iPppLcp->GetBCAProvision()->GetBCAName()));
	
	TNewBcaFactoryL newBcaFactoryProcL = (TNewBcaFactoryL)iBcaDll.iObj.Lookup(1);
	if (!newBcaFactoryProcL)
		{
		LOG(Logger().Printf(_L("CPppHdlcLink::LoadBcaL: BCA Factory method lookup failed. ")); )
		User::Leave(KErrBadLibraryEntryPoint);	
		}
	
	MBcaFactory* bcaFactory = (*newBcaFactoryProcL)();
	__ASSERT_ALWAYS(bcaFactory, User::Panic(_L("PPP Panic"), EPppPanic_NullBcaFactory));
	
	CleanupReleasePushL(*bcaFactory);
	
	iBca = bcaFactory->NewBcaL(); 
	__ASSERT_ALWAYS(iBca, User::Panic(_L("PPP Panic"), EPppPanic_NullBca));
	
	CleanupStack::PopAndDestroy(bcaFactory); // We only need once BCA per PPP lifetime.
	LOG(Logger().Printf(_L("CPppHdlcLink::LoadBcaL: BCA successfully loaded. ")); )
	}



void CPppHdlcLink::InitEscMap()
/**
Initializes the send escape map to a conservative value.
iRecvEscMap is not set here, since Carlson says that can cause problems on
LCP renegotiation.

@post iSendEscMap is initialized
*/
	{
	// Always Escape bytes 0..31 initially
	Mem::FillZ(iSendEscMap, sizeof(iSendEscMap));
	iSendEscMap[0] = 0xffffffff;
	// Also escape the two special bytes
	SetEscMapBit(KPppHdlcFlagChar);
	SetEscMapBit(KPppHdlcEscChar);
	}

void CPppHdlcLink::SetEscMapBit(TUint8 aChar)
/**
Marks this character to be escaped when sending.

@param aChar Character
*/
	{
	iSendEscMap[aChar>>5] |= 1<<(aChar&0x1f);
	}

void CPppHdlcLink::DeferredApplyOptions()
/**
Starts using the negotiated PPP options.
Called once LCP has completed.
*/
	{
//	LOG( iPppLcp->iLogger->Printf(_L("PPP Link DeferredApplyOptions()")); )
	iFlags &= ~(KPppHdlcApplyPending|KPppHdlcAtApplyMark);
	iFlags = (iFlags & ~iPendingMask) | iPendingFlags;
	iSendEscMap[0] = iPendingEscMap;
	}

void CPppHdlcLink::GetSendRecvSize(TInt& aMaxRecvSize, TInt& aMaxSendSize)
/**
Returns the link's idea of the maximum receive and send unit sizes.

@param aMaxRecvSize Returns the MRU, or 0 for default
@param aMaxSendSize Returns the MTU, or 0 for default
*/
	{
	aMaxRecvSize = iPppLcp->MaxReceiveSize();
	aMaxSendSize = iPppLcp->MaxTransferSize();
	}

void CPppHdlcLink::OptNegotiationStarted()
	{
//	LOG( iPppLcp->iLogger->Printf(_L("PPP Link OptNegotiationStarted()")); )
	InitEscMap();

// Initialize (most) flags
	iFlags = (iFlags & (KPppHdlcCommConfigOk|KPppHdlcSendBusy|KPppHdlcOrigConfig))
				| (KPppHdlcSendFcs16 | KPppHdlcRecvFcs16);

	iPendingMask = 0;
	iPendingFlags = 0;
	iPendingEscMap = iSendEscMap[0];
	iApplyConfigMarker = NULL;
	}

void CPppHdlcLink::OptNegotiationAborted()
	{
//	LOG( iPppLcp->iLogger->Printf(_L("PPP Link OptNegotiationAborted()")); )
	}

void CPppHdlcLink::OptNegotiationComplete()
	{
//	LOG( iPppLcp->iLogger->Printf(_L("PPP Link OptNegotiationComplete()")); )
	iFlags |= KPppHdlcApplyPending;
/*
	if (!iSendQ.IsEmpty())
		{
		RMBufChain pkt = iSendQ.Last();
		iApplyConfigMarker = pkt.First();
		}
	else
		{
		iFlags |= KPppHdlcAtApplyMark;
		if (iSendPkt.IsEmpty())
			DeferredApplyOptions();
		}
*/
	}

void CPppHdlcLink::OptFillinConfigRequestL(RPppOptionList& aRequestList)
	{
	// Add XON/XOFF bits to the ACCM read from ppp.ini
	TUint32 map = iDesiredRecvEscMap;
	if ((iOrigConfig().iHandshake & KConfigObeyXoff)!=0 ||
		(iOrigConfig().iHandshake & KConfigSendXoff)!=0)
		{
		if (iOrigConfig().iXonChar<32)
			map |= 1<<iOrigConfig().iXonChar;
		if (iOrigConfig().iXoffChar<32)
			map |= 1<<iOrigConfig().iXoffChar;
		}
	aRequestList.CreateAndAddL(KPppLcpOptEscapeCharMap,map);
	aRequestList.CreateAndAddL(KPppLcpOptProtocolCompress);
	aRequestList.CreateAndAddL(KPppLcpOptAddrCtrlCompress);
	}

TPppOptResponse CPppHdlcLink::OptCheckConfigRequest(RPppOption& aOption)
/**
Checks options in a received config request.
Called once for each option.

@param aOption LCP option

@return EPppOptAck, EPppOptNak, or EPppOptReject
*/
	{
	const TInt KBooleanOptDataLen = 0;
	const TInt KOptEscapeCharMapLen = 4;
	
	switch (aOption.OptType())
		{
	case KPppLcpOptEscapeCharMap:
		{		
		TUint32 remoteRecvAccm = 0;
		TBool forceNak = EFalse; // Whether the ACCM must be NAKed regardless of value.
		
		if(KOptEscapeCharMapLen != aOption.ValueLength()) // Invalid length: NAK 
			{
			aOption.SetValueLength(KOptEscapeCharMapLen); // Fix length. (value = 0x0)
			forceNak = ETrue; 
			}
		else // Valid length. Extract the ACCM and negotiate.
			{
			remoteRecvAccm = BigEndian::Get32(aOption.ValuePtr());
			}

		// Create the sender ACCM
		TUint32 localSendAccm = 0; // We want to escape as little as possible.
		// Add XON/XOFF bits to the sender ACCM 
		if ((iOrigConfig().iHandshake & KConfigObeyXoff)!=0 ||
				(iOrigConfig().iHandshake & KConfigSendXoff)!=0)
				{
				if (iOrigConfig().iXonChar<32)
					localSendAccm |= 1<<iOrigConfig().iXonChar;
				if (iOrigConfig().iXoffChar<32)
					localSendAccm |= 1<<iOrigConfig().iXoffChar;
				}
	
		
		// Compare the ACCMs and check if peer's proposal must be NAKed:
		// It is NAKed if there is a disagreement on ACCM, or if the option is invalid
		if ((~remoteRecvAccm & localSendAccm) || forceNak) 
		    {
			BigEndian::Put32( aOption.ValuePtr(), localSendAccm | remoteRecvAccm);
			return EPppOptNak;
			}
		
		
		return EPppOptAck;
		}

	case KPppLcpOptProtocolCompress:
		if(KBooleanOptDataLen == aOption.ValueLength())
			{
			return EPppOptAck;
			}
		else
			{
			aOption.SetValueLength(KBooleanOptDataLen);
			return EPppOptNak;
			}		
	case KPppLcpOptAddrCtrlCompress:
		if(KBooleanOptDataLen == aOption.ValueLength())
			{
			return EPppOptAck;
			}
		else
			{
			aOption.SetValueLength(KBooleanOptDataLen);
			return EPppOptNak;
			}

	case KPppLcpOptFcsType:
		{
		if (aOption.ValueLength()<1)
			return EPppOptReject;
		TUint8 fcs = *aOption.ValuePtr();
		// This only allows one FCS method at a time.
		if (fcs==KPppHdlcFcs0Flag || fcs==KPppHdlcFcs16Flag
#ifdef __LCP_EXTENSION_FCS_32
		    || fcs==KPppHdlcFcs32Flag
#endif
	    )
			{
			return EPppOptAck;
			}
		*aOption.ValuePtr() = KPppHdlcFcs16Flag;
		return EPppOptNak;
		}

	case KPppLcpOptPadding:
	case KPppLcpOptCompoundFrames:
//		if (iPppLcp->DoLcpExts())
//			{
			// handle the options
//			}
		// else fall though and reject
	default:
		return EPppOptReject;
		}
	}


void CPppHdlcLink::CallBackOptRequestGranted()
	{

	iPppLcp->CallbackRequestGranted();
	}


void CPppHdlcLink::OptApplyConfigRequest(RPppOption& aOption)
/**
Applies options in a received config request (that was ACK'd).
Called once for each option.

@param aOption LCP option
*/
	{
	
//	__LOGTEXT2_DEBUG(_L8("CPppHdlcLink::OptApplyConfigRequest() Option = %d\n"),aOption.OptType());
	
	switch (aOption.OptType())
		{
	case KPppLcpOptEscapeCharMap:
		iPendingEscMap = BigEndian::Get32(aOption.ValuePtr());
		break;
	case KPppLcpOptProtocolCompress:
		iPendingMask |= KPppHdlcSendCompProt;
		iPendingFlags |= KPppHdlcSendCompProt;
		break;
	case KPppLcpOptAddrCtrlCompress:
		iPendingMask |= KPppHdlcSendCompAddrCtrl;
		iPendingFlags |= KPppHdlcSendCompAddrCtrl;
		break;
	case KPppLcpOptFcsType:
		{
		TUint8 fcs = *aOption.ValuePtr();
		iPendingMask |= (KPppHdlcSendFcs16|KPppHdlcSendFcs32);
		iPendingFlags &= ~(KPppHdlcSendFcs16|KPppHdlcSendFcs32);
		if (fcs==KPppHdlcFcs16Flag)
			iPendingFlags |= KPppHdlcSendFcs16;

#ifdef __LCP_EXTENSION_FCS_32
		if (fcs==KPppHdlcFcs32Flag)
			iFlags |= KPppHdlcSendFcs32;
#endif
		}
		CallBackOptRequestGranted();
		break;
	case KPppLcpOptCallback:
		CallBackOptRequestGranted();
		break;
	case KPppLcpOptPadding:
	case KPppLcpOptCompoundFrames:
	default:
		break;
		}
	}


void CPppHdlcLink::OptRecvConfigAck(RPppOption& aOption)
/**
Receives a Config Ack - apply the option.
Called once for each option.

@param aOption LCP option
*/
	{
//	__LOGTEXT2_DEBUG(_L8("CPppHdlcLink::OptRecvConfigAck() Option = %d\n"),aOption.OptType());
	switch (aOption.OptType())
		{
	case KPppLcpOptEscapeCharMap:
		// Start using the new map immediately.
		// This goes against the RFC, but is recommended by Carlson
		iRecvEscMap = BigEndian::Get32(aOption.ValuePtr());
		break;
	case KPppLcpOptProtocolCompress:
		// Start using the flag immediately
		iFlags |= KPppHdlcRecvCompProt;
		break;
	case KPppLcpOptAddrCtrlCompress:
		// Start using the flag immediately
		iFlags |= KPppHdlcRecvCompAddrCtrl;
		break;
	case KPppLcpOptFcsType:
		{
		TUint8 fcs = *aOption.ValuePtr();
		iPendingMask |= (KPppHdlcRecvFcs16|KPppHdlcRecvFcs32);
		iPendingFlags &= ~(KPppHdlcRecvFcs16|KPppHdlcRecvFcs32);
		if (fcs==KPppHdlcFcs16Flag)
			iPendingFlags |= KPppHdlcRecvFcs16;

#ifdef __LCP_EXTENSION_FCS_32
		if (fcs==KPppHdlcFcs32Flag)
			iPendingFlags |= KPppHdlcRecvFcs32;
#endif		
		}
		break;
	case KPppLcpOptPadding:
	case KPppLcpOptCompoundFrames:
	default:
		break;
		}
	}

void CPppHdlcLink::OptRecvConfigNak(RPppOption& aOption, RPppOptionList& aReqList)
/**
Modifies request after receiving a Config Nak.
Called once for each option.

@param aOption LCP option
@param aReqList The associated original request to be modified
*/
	{
	switch (aOption.OptType())
		{
	case KPppLcpOptEscapeCharMap:
		// Carlson says to OR this with our original request, but this will
		// work as-is with a correctly-functioning peer
		aReqList.ReplaceOption(aOption);
		break;
	case KPppLcpOptProtocolCompress:
		aReqList.ReplaceOption(aOption);
		break;
	case KPppLcpOptAddrCtrlCompress:
		aReqList.ReplaceOption(aOption);
		break;
	case KPppLcpOptFcsType:
		aReqList.ReplaceOption(aOption);
		break;
	case KPppLcpOptPadding:
	case KPppLcpOptCompoundFrames:
	default:
		break;
		}
	}

void CPppHdlcLink::OptRecvConfigReject(RPppOption& aOption, RPppOptionList& aReqList)
/**
Modifies request after receiving a Config Reject
Called once for each option.

@param aOption LCP option
@param aReqList The associated original request to be modified
*/
	{
//	__LOGTEXT2_DEBUG(_L8("CPppHdlcLink::OptRecvConfigReject() Option = %d\n"),aOption.OptType());
	switch (aOption.OptType())
		{
	case KPppLcpOptEscapeCharMap:
		aReqList.RemoveOption(aOption);
		break;
	case KPppLcpOptProtocolCompress:
		aReqList.RemoveOption(aOption);
		break;
	case KPppLcpOptAddrCtrlCompress:
		aReqList.RemoveOption(aOption);
		break;
	case KPppLcpOptFcsType:
		aReqList.RemoveOption(aOption);
		break;
	case KPppLcpOptPadding:
	case KPppLcpOptCompoundFrames:
	default:
		break;
		}
	}

void CPppHdlcLink::OpenL()
/**
Starts connect establishment
*/
	{
	switch (iState)
		{
	case EPppHdlcConnecting:
	case EPppHdlcOpen:
		return;
	case EPppHdlcClosed:
		iState = EPppHdlcConnecting;
		break;
	case EPppHdlcDisconnecting:
		LOG(Logger().Printf(_L("CPppHdlcLink::OpenL: Link is disconnecting. Leaving with KErrNotReady[%d]"), KErrNotReady); )
		User::Leave(KErrNotReady);
		break;
	default:
		return;
		}
	LOG( Logger().Printf(_L("PPP Link OpenL")); )
	}

  
/**
Drops connection
*/
void CPppHdlcLink::Close()
	{
	LOG( Logger().Printf(_L("CPppHdlcLink::Close(): Link Close requested: ")); )
	iError = KErrNone;
	switch (iState)
		{
	case EPppHdlcConnecting:		
	case EPppHdlcOpen:
		iState = EPppHdlcDisconnecting;
		PacketModeOff();
		break;
	
	case EPppHdlcClosed:
	case EPppHdlcDisconnecting:
		break;
		
	case EPppHdlcReconnecting: // This is not used anywhere in PPP.
	default:
		__ASSERT_DEBUG(EFalse, User::Panic(_L("PPP Panic"), EPppPanic_IllegalHdlcState));
		break;
		}
	}


TUint16 CPppHdlcLink::DecodeProtocolID(TUint8*& aPtr) const
/**
Decodes the PPP protocol ID from a buffer.
The ID may be compressed.

@param aPtr Pointer to PPP frame buffer; returns pointing one past protocol ID

@return PPP protocol ID
*/
	{
	// Extract the protocol information - this can be either one or two bytes, but
	// the last byte must be odd, and the first byte (if present) must be even
	TUint16 prot = *aPtr++;
	if(!(prot & 1))
		{
		prot = (TUint16) ((prot << 8) | *aPtr++);
		}
	return prot;
	}

TBool CPppHdlcLink::UnescapeChar(TUint8*& ptr, TUint8*& end, TUint8*& mptr, TUint8*& mend)
/**
Unescape the next character in the buffer. Ignore characters in the receive ACCM.

@param ptr Pointer to input buffer
@param end Pointer to one past the end of the input buffer
@param mptr Pointer to the output buffer
@param mend Pointer to one past the end of the output buffer (must be > mptr on entry)

@return ETrue on error (error will be logged)

@post KPppHdlcRecvEscPending is appropriately set or cleared in iFlags
*/
	{
	// See whether there's still more data in the incoming buffer
	while (ptr<end)
		{
		// Check for flag sequence and silently discard the frame if it is
		if ((*ptr)==KPppHdlcFlagChar)
			{
			LOG( Logger().Printf(_L("PPP: HDLC: Bad PPP frame - flag char received directly after escaping sequence")));
			// Delete the iRecvMBuf
			DoBadRecv();
			return ETrue;
			}

		// If the character is not in the receive ACCM, it's good--stop searching
		if (!IsInRecvEscMap(*ptr))
			{
			break;
			}

		++ptr;
		}
		
	if (ptr >= end)
		{
		// We haven't got the data yet, so set the flag and sort it out next time DoRecv() is called
		iFlags |= KPppHdlcRecvEscPending;
		}

	else {
		// Successfully processing the escaped character
		iFlags &= ~KPppHdlcRecvEscPending;

		// Decode the escaped char and copy it across
		if (DecodeChar(mptr, mend, (TUint8)(*ptr++ ^ KPppHdlcEscBit)))
			{
			return ETrue;	// error
			}
		}
	return EFalse;
	}


TBool CPppHdlcLink::AppendRecvMbuf(TUint8*& aMptr, TUint8*& aMend)
/**
Appends the current Mbuf to the packet and allocate a new one to replace it.

@param aMptr Returns a pointer to the beginning of the new buffer
@param aMend Returns a pointer to the end of the new buffer

@return ETrue when a new buffer could not be allocated
*/
	{
	// Queue this MBuf
	if (iRecvMBuf != NULL)
		{
		iRecvPkt.Append(iRecvMBuf);
		}

	// Allocate a new one
	TRAPD(err, iRecvMBuf = RMBuf::AllocL());
	if (err!=KErrNone)
		{
		// Nuke the whole packet if we fail to allocate a new MBuf
		iRecvMBuf = NULL;
		iRecvPkt.Free();
		LOG( iPppLcp->iLogger->Printf(_L("PPP: couldn't allocate MBuf")));
		return ETrue;
		}
	aMptr = iRecvMBuf->Ptr();
	aMend = iRecvMBuf->EndPtr();
	__ASSERT_DEBUG((aMend - aMptr) == iRecvMBuf->Size(), User::Panic(_L("PPP Panic"), 0));
	return EFalse;
	}
	
void CPppHdlcLink::DoBadRecv()
/**
Adds a special packet to the receive queue with the meaning that
a bad PPP frame was received. Any data in the current MBuf is
used as the data portion of the special packet (and ignored).
*/
	{
	// Need at least one MBuf in the packet to create an info header.
	// Use the current MBuf if available, or create a new one if not.
	if (iRecvPkt.IsEmpty())
		{
		TUint8* ptr, * end;
		if (AppendRecvMbuf(ptr, end))
			{
			return;	// error has been logged; exit
			}
		iRecvMBuf->SetLength(0);
		
		if (iRecvPkt.IsEmpty())
			{
			// There wasn't an MBuf available a moment ago;
			// there is now...
			if (AppendRecvMbuf(ptr, end))
				{
				return;	// error has been logged; exit
				}
			iRecvMBuf->SetLength(0);
			}
		}
	else if (iRecvMBuf != NULL)
		{
		// The MBuf waiting for use may be partially filled.  Since this is
		// DoBadRecv() we know it contains garbage, so clear it out so it's
		// ready for reuse the next time through DoRecv().
		iRecvMBuf->SetLength(0);
		}

	RMBufPacket pkt;
	RMBufPktInfo* info = NULL;
	TRAPD(err, info=pkt.CreateL(iRecvPkt, 0);)
	if(err!=KErrNone)
		{
		// We can't allocate an info header, so log a message and give up.
		LOG( iPppLcp->iLogger->Printf(_L("PPP: couldn't allocate packet")));
		return;
		}
	TPppAddr::Cast(info->iDstAddr).SetCRCError(ETrue);
	pkt.Pack();
	iRecvQ.Append(pkt);
	}

void CPppHdlcLink::DoRecv()
/**
Comm Read has completed - decodes and passes to LCP.

We're copying the frame from the iRecvBuf into a series of mbufs - iRecvMBuf -
which are chained together into a RMBufPacket, which is then
added onto the iRecvQ 

@see DoBadRecv()
*/
	{
	// This buffer contains one frame, as we instructed the COMM port to
	// stop reading when it encounters a KPppHdlcFlagChar
	TUint8* ptr = (TUint8*)iRecvBuf.Ptr();
	TUint8* end = ptr+iRecvBuf.Length();
	TUint8* mptr = NULL;
	TUint8* mend = NULL;

	LOG( if (iLogLevel>=5) iPppLcp->iLogger->HexDump(_S("COMM Recv    "), _S("    "), ptr, end-ptr); )
	LOG( if (iLogFormat == EpppDumpLogFormat) iPppLcp->iLogger->DumpFrame(EpppDirectionReceive,iRecvBuf); )

	if (iRecvMBuf==NULL)
		{
		// Allocate a new MBuf
		if (AppendRecvMbuf(mptr, mend))
			{
			return;	// error has been logged; exit
			}
		}
	else
		{
		// Append to the existing MBuf
		// Set the buffer length to the maximum to make room for received data.
		// The correct length is set at the end of this method.
		mptr = iRecvMBuf->EndPtr();
		iRecvMBuf->SetLength(iRecvMBuf->Size());
		mend = iRecvMBuf->EndPtr();
		__ASSERT_DEBUG(mptr < mend, User::Panic(_L("PPP Panic"), 0));
		}
	
	// Deal with pending escape sequence - this happens when part of a frame received, and it ends with an
 	// escaping sequence, but the char that is being escaped will be the first char in the next block of data
 	// to be received.
	if (iFlags & KPppHdlcRecvEscPending)
		{
		if (UnescapeChar(ptr,end,mptr,mend))
			{
			return;	// error has been logged; exit
			}
		}
	
	while (ptr<end)
		{
		TUint8 ch = *ptr++;

		// If this is the beginning or end of a frame...
		if (ch==KPppHdlcFlagChar)
			{
			if (mptr!=iRecvMBuf->Ptr())
				{
				// If we have a partially full MBuf, set the length appropriately
	 			// and stick it onto the receive queue for LCP
				iRecvMBuf->SetLength(mptr - iRecvMBuf->Ptr());
				if (AppendRecvMbuf(mptr, mend))
					{
					return;	// error has been logged; exit
					}
				}

			// This is the end of the frame - so check and remove CRC
			if (!iRecvPkt.IsEmpty())
				{
				// End of frame - check and remove CRC
				TBool crcok = ETrue;

				// Check the CRC for 16-bit FCS
				if (iFlags & KPppHdlcRecvFcs16)
					{
					crcok = iRecvFcs16.IsGood();

					// Initialize the CRC for a new packet
					iRecvFcs16.Init();

					RMBuf* m = iRecvPkt.Last();

					// Attempt to remove 16 bit CRC
					// adjend is not being used until after passing to a Decompressor
					// hence decompressors were getting the CRC that should have been removed.
					// 

					// Replacement code 
					if (m->Length()>2)
						{
						// Remove the CRC; must be at least one data byte in the last MBuf

						m->AdjustEnd(-2);
						}
					else
						// Special case where the CRC is the last byte/s in the buffer
						// There is no way to remove just the last MBuf in the queue,
						// so we must copy each MBuf except the last into a brand new
						// RMBufQ, then copy them all back into the original one.
						//
						// TODO: See if it's possible to just set the length of the
						// last MBuf to 0 and avoid all the copying.
						{
						// Get the odd length 0 -> 2
						TInt oddLen = m->Length();
						// Set up a temporary MBuf Queue and mbuf pointer
						RMBufQ tmpQ;
						tmpQ.Init();
						RMBuf* tmpBuf;
						// Loop removing all mbufs from the member queue
						while((tmpBuf = iRecvPkt.Remove()) != NULL)
							{
							// Is it the last one? m points to last
							if(tmpBuf == m)
								// It is, so free it as it's only got CRC bytes
								m->Free();
							else
								// put it on the temporary queue
								tmpQ.Append(tmpBuf);
							}
						// Loop replacing the mbufs on the member queue
						// NB : The last one was FREE'd in the previous loop
						while((tmpBuf = tmpQ.Remove()) != NULL)
							{
							iRecvPkt.Append(tmpBuf);
							}
						m = iRecvPkt.Last();
						if(m==NULL)
							{
							LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: empty packet after CRC removal")));
							return; // invalid frame has been discarded
							}
						// Subtract the CRC len from the NEW last buffer (Could be 0 -> 2)
						m->AdjustEnd(oddLen - 2);
						}
					
					}

#ifdef __LCP_EXTENSION_FCS_32
				// or calculate the CRC code for 32-bit FCSs
                // TODO This feature is not tested
				if (iFlags & KPppHdlcRecvFcs32)
					{
					crcok = iRecvFcs32.IsGood();

					// Initialize the CRC for a new packet
					if (iFlags & KPppHdlcRecvFcs32)
						{
						iRecvFcs32.Init();
						}

					// This doesn't handle the case of a part of the CRC sitting
					// in a separate mbuf, which the CRC-16 code above handles.
					// There are likely other problems with switching from
					// 16 to 32 bit CRCs, and with sending 48 bits of CRCs
					// (see RFC 1570).  --danfa
					RMBuf* m = iRecvPkt.Last();
					if (m->Length()>4)
						m->AdjustEnd(-4);
					else
						adjend = 4;
					}
#endif
				// We've calculated the CRC now, so continue processing
				if (!crcok)
					{
					// The CRC is bad; queue a metapacket indicating so
					LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: Silently discarding frame due to bad CRC")));
		 			DoBadRecv();
		 			return;
					}

				// The CRC's OK, so sort out the other HDLC framing...
				else
					{
					// Remove link header
					RMBuf* m = iRecvPkt.First();

					// Check that we have a valid frame length (<2 after FCS removal invalid according to 
 					// RFC1662)
 					// If not, silently discard the frame
 					if (m->Length() < 2)
 						{
 						LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: Silently discarding frame due to bad length (<2 bytes after FCS removed)")));
						DoBadRecv();
						return;
 						}
 
					TUint8* p = m->Ptr();
					TUint addr=0, ctrl=0;
 					// Check to see if we've got address and control field compression active
					if (iFlags & KPppHdlcRecvCompAddrCtrl)
						{
						// If we have, but they're still present anyway, adjust the start pointers appropriately
						if (p[0]==KPppHdlcAddrByte && p[1]==KPppHdlcCtrlUIByte)
							p += 2;
						addr = KPppHdlcAddrByte;
						ctrl = KPppHdlcCtrlUIByte;
						}
					else
						{
						// Get the address from the frame (RFC1662 - 1 byte - extensions may be defined later,
 						// and then we'll have to change this code)
 						addr = *p++;
 
 						// Check that the address wasn't even (all addresses must be odd - RFC1662)
 						//
 						// For full RFC compliance, we should == with 0xff here...
 						if (!(addr&0x01)) 
 							{
 							// Discard the frame
 							LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: Silently discarding frame due to bad address field")));
							DoBadRecv();
 							return;
 							}

						// Get control byte from the frame (RFC1662 - 1 byte - extensions may be defined later,
 						// and then we'll have to change this code)
 						ctrl = *p++;
 						
 						// Check that the control field wasn't even (all control fields must be odd - RFC1662)
 						// For full RFC compliance, we should == with KPppHdlcCtrlUIByte here...
 						if (!(ctrl&0x01)) 
 							{
 							// Discard the frame
 							LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: Silently discarding frame due to bad control field")));
 							DoBadRecv();
 							return;
 							}
 						}

					// Extract the protocol information
					TUint prot = DecodeProtocolID(p);

					// Ensure that the header we just parsed doesn't extend beyond the end of the frame.
					TInt hdrlen = p - m->Ptr();
					if (hdrlen <= m->Length())
						{
						// Adjust start point to compensate for removal of address, control and protocol fields
						m->AdjustStart(hdrlen);
						}
					else
						{
						// Catch-all safety check... looks like we've been passed
						// a badly-framed packet.
						LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: Discarding badly framed packet")));
						DoBadRecv();
 						return;
						}

					// Check to see if we're running any compression
					if (prot == KPppIdCompressed)
						{
						// If we are, try and decompress the frame
						if (iPppDecompressor)
							{
							if (iPppDecompressor->Decompress(iRecvPkt))
								{
								m = iRecvPkt.First();
								p = m->Ptr();
								// Decompress OK so...
								// extract the protocol information
								prot = DecodeProtocolID(p);
								
								LOG(iPppLcp->iLogger->Printf(_L("Frame Prot %x"),prot);)
								m->AdjustStart(p-m->Ptr());
								}
							else
								{
								// Otherwise we couldn't decompress it, so throw away the frame
 								LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: Could not decode compressed packet - discarding")));
								DoBadRecv();
								return;
								}
							}
						}
					
					// Prepend and fill in info header and deliver to LCP
					RMBufPacket pkt;
					RMBufPktInfo* info = NULL;
					TRAPD(err, info=pkt.CreateL(iRecvPkt, 0);)
					if(err!=KErrNone)
						{
						// This is assumed to mean that we're OOM, but there might be other situations...
						LOG( iPppLcp->iLogger->Printf(_L("PPP: HDLC: Could not create info packet error %d"), err));
						//__DEBUGGER();
						return;
						}

					TPppAddr::Cast(info->iDstAddr).SetAddress(addr);
					TPppAddr::Cast(info->iDstAddr).SetControl(ctrl);
					TPppAddr::Cast(info->iDstAddr).SetProtocol(prot);

					TPppAddr::Cast(info->iDstAddr).SetCRCError(EFalse);

					pkt.Pack();
					iRecvQ.Append(pkt);
					}
				}
			}
			
		// Check for escaping sequence
		else if (ch==KPppHdlcEscChar)
			{
			if (UnescapeChar(ptr,end,mptr,mend))
				{
				return;	// error has been logged; exit
				}
			}
		// Otherwise copy the char across to the iRecvMBuf - *** most executed case ***
		else
			{
			// If the character is in the receive ACCM, it's bogus--drop it on the floor.
			// Otherwise, XON/XOFF from the local DCE would be erroneously included in the packet.
			if (!IsInRecvEscMap(ch) &&
				DecodeChar(mptr, mend, ch))
				{
				return;	// error has been logged; exit
				}
			}
		}
		
	iRecvMBuf->SetLength(mptr - iRecvMBuf->Ptr());
	__ASSERT_DEBUG(mptr < mend, User::Panic(_L("PPP Panic"), 0));
	}


void CPppHdlcLink::BcaReadComplete(TInt aStatus)
/**
Comm Read has completed - decodes and passes to LCP

@param aStatus Error code from CommRead()
*/
	{
	switch (aStatus)
		{
	case KErrCommsOverrun:
	case KErrCommsFrame:
	case KErrCommsParity:
	case KErrNone:
		{
		if (aStatus == KErrNone)
			{
			// Unframe and queue the received packet
			DoRecv();
			}
		else
			{
			// Queue an error packet
			LOG( Logger().Printf(_L("CPppHdlcLink::BcaReadComplete: error %d. Doing Bad Receive."), aStatus); )
			DoBadRecv();
			}
			
		if (iState == EPppHdlcOpen)
			{
			iRecvBuf.SetMax();
			// Initiate another asynchronous read
			iBcaReader->Read(iRecvBuf);
			}

		// Pass any received packets up the protocol chain
		RMBufChain pkt;
		while (iRecvQ.Remove(pkt))
			{
			// Parse and dump the PPP frame to the log
			LOG( if(iLogLevel>=4)  iPppLcp->iLogger->Dump(pkt, KPppHdlcRecvChannel); )
			LogUserData(pkt,KPppHdlcRecvChannel);	// MS
			DeliverToLcp(pkt);
			}
		}
		break;

	case KErrCommsLineFail: // Usually means that the serial link was broken. (DCD/DTR down)
	default:
		LOG( Logger().Printf(_L("CPppHdlcLink::BcaReadComplete: Read Error %d"), aStatus); )
			
		LinkDown(aStatus);
		break;
		}
	}


/**
Sends more data (if the link is still open).
Called when CBcaWriter::Write() has completed.

@param aStatus (ignored)
*/
void CPppHdlcLink::BcaWriteComplete(TInt /*aStatus*/)
	{
	iFlags &= ~KPppHdlcSendBusy;
	//if (aStatus!=KErrNone)
	//	{
	//	LinkDown(aStatus);
	//	return;
	//	}
	if (iState == EPppHdlcOpen)
		{
		DoSend(EFalse);
		}		
	}

TBool CPppHdlcLink::DecodeChar(TUint8*& aPtr, TUint8*& aPtrEnd, TUint8 aChar)
/**
Copies an unescaped character into a receive buffer and updates the CRC at the
same time. Allocates a new buffer if the current one fills up.

@param aPtr Pointer to buffer; on return, points one past the last location used
@param aPtrEnd Pointer to one past the end of buffer (must be > aPtr on entry)
@param aChar Unescaped character

@return ETrue when a new buffer could not be allocated
*/
	{
	__ASSERT_DEBUG(aPtr < aPtrEnd, User::Panic(_L("PPP Panic"), 0));
	*aPtr++ = aChar;
	if (iFlags & KPppHdlcRecvFcs16)
		{
		iRecvFcs16.CalcByte(aChar);
		}
#ifdef __LCP_EXTENSION_FCS_32
	if (iFlags & KPppHdlcRecvFcs32)
		{
		iRecvFcs32.CalcByte(aChar);
		}
#endif
	if (aPtr>=aPtrEnd)
		{
		// Queue this MBuf and allocate another
		return AppendRecvMbuf(aPtr, aPtrEnd);
		}
	return EFalse;
	}

void CPppHdlcLink::EncodeChar(TUint8*& aPtr, TUint8 aChar)
/**
Copies a character into a transmit buffer and escapes if necessary.
Updates the CRC at the same time.

@param aPtr Pointer to buffer; on return, points one past the last location used
@param aChar Character to encode
*/
	{
	if (IsEscapedChar(aChar))
		{
		*aPtr++=KPppHdlcEscChar;
		*aPtr++=(TUint8)(aChar^KPppHdlcEscBit);
		}
	else
		*aPtr++=aChar;

	if (iFlags & KPppHdlcSendFcs16)
		{
		iSendFcs16.CalcByte(aChar);
		}
#ifdef __LCP_EXTENSION_FCS_32
	if (iFlags & KPppHdlcSendFcs32)
		{
		iSendFcs32.CalcByte(aChar);
		}
#endif
	}

void CPppHdlcLink::CreateHdlcHeader(TUint8*& aPtr, TBool aRestart, TUint16 aProt)
/**
Creates and encodes an HDLC header into the output buffer and updates the CRC.
There must be sufficient space available in the output buffer.

@param aPtr Pointer into buffer (must have at least 9 bytes free);
       on return, points one past the last location used
@param aRestart EFalse if this frame immediately follows a previous frame
@param aProt PPP protocol ID
*/
	{
	// Only send flag byte if not immediately following previous frame
	if (aRestart)
		*aPtr++ = KPppHdlcFlagChar;

	// never apply to LCP
	if (aProt==KPppIdLcp || !(iFlags & KPppHdlcSendCompAddrCtrl))
		{
		EncodeChar(aPtr, KPppHdlcAddrByte);
		EncodeChar(aPtr, KPppHdlcCtrlUIByte);
		}

	// never apply to LCP (LCP > 255 anyway!)
	if (aProt>0xff || !(iFlags & KPppHdlcSendCompProt))
		EncodeChar(aPtr, (TUint8)(aProt>>8));
	EncodeChar(aPtr, (TUint8)(aProt&0xff));
	}		

TInt CPppHdlcLink::Send(RMBufChain& aPacket, TUint aPppId/*=KPppIdAsIs*/)
/**
Queues outgoing packet and ensures writer is active.
If PPP compression is enabled, the compresses the frame first then reinserts
it into the queue.

@param aPacket MBuf chain containing packet
@param aPppId PPP protocol number

@return KErrNoMemory when out of memory, or 1 if packet was queued,
        or KErrNone if queue is above high water mark or HDLC is not open
*/
	{
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
	TPppAddr::Cast(info->iDstAddr).SetAddress(KPppHdlcAddrByte);
	TPppAddr::Cast(info->iDstAddr).SetControl(KPppHdlcCtrlUIByte);

	if ((aPppId >= 0x0021) &&
		(aPppId <= 0x00fa))
		{
		if (iPppCompressor)
			{
			// Remove the Info Header before compressing
			RMBuf* Temp = aPacket.Remove();

			//__DEBUGGER();
			TPppCompressReturnValue ret = iPppCompressor->Compress(aPacket, (TUint16)aPppId);
			
			if (ret == EPppCompressedOK)
				{
				// Set packet length to the new, compressed length
				info->iLength = aPacket.Length();
				
				aPacket.Prepend(Temp);
				aPppId = KPppIdCompressed;
				}

			else if(ret ==EPppCompressedFrameExpanded)
				{
				// The frame expanded during compression; send original frame
				aPacket.Prepend(Temp);
				}
			
			else //(ret == EPppCompressedNotOK)
				{
				// Throw everything away
				//__DEBUGGER();
				aPacket.Free();
				Temp->Free();
				return KErrNoMemory;
				}			
			}

		
/*++		
		if (iPppCompressor)
			{
			RMBuf* Temp;

			Temp = aPacket.Remove();

			//
			// Remove the Info Header
			//
			//__DEBUGGER();
			if (iPppCompressor->Compress(aPacket, (TUint16)aPppId))
				{
				//
				// Set the length to the new length
				//
				info->iLength = aPacket.Length();
				
				}
			else
				{
				//
				// Throw everything away
				//
				//__DEBUGGER();
				aPacket.Free();
				Temp->Free();
				return KErrNoMemory;
				}
			aPacket.Prepend(Temp);
			aPppId = KPppIdCompressed;
			}
--*/

		}

	if (aPppId!=KPppIdAsIs && aPppId!=KPppIdUnknown && aPppId!=KPppIdCopyAll)
		{
		TPppAddr::Cast(info->iDstAddr).SetProtocol(aPppId);
		}
	iSendNumBufs += aPacket.NumBufs();
	iSendQ.Append(aPacket);

	switch (iState)
		{
	case EPppHdlcConnecting:
		break;
	case EPppHdlcOpen:
		if (iSendFlowOn && iSendNumBufs>=iSendHiWat)
			iSendFlowOn = EFalse;
		if (!(iFlags & KPppHdlcSendBusy))
			DoSend(ETrue);
		return iSendFlowOn ? 1 : 0;
	case EPppHdlcClosed:
		{
		 TRAPD(err, OpenL());
		 if (KErrNone != err) 
			{
			LOG( iPppLcp->iLogger->Printf(_L("PPP Link Open Failure with error %d"), err); )
			}
		}
		break;
	default:
		break;
		}
	return 0;
	}

void CPppHdlcLink::DoSend(TBool aRestart)
/**
Writes more data to the comm port.
The send buffer is filled with as much of the encoded PPP frame as will
fit, then is transmitted to the comm port. The next call will send the
remaining data (if any), or start with the next PPP frame from the queue.

@param aRestart EFalse if this call immediately follows a previous frame
*/
	{
	__ASSERT_DEBUG(!(iFlags & KPppHdlcSendBusy), User::Panic(_L("PPP Panic"), 0));

	TUint8* ptr = (TUint8*)(iSendBuf.Ptr());
	TUint8* end = ptr+(iSendBuf.MaxLength()-1);	// -1 allows safe encoding
	
	// Starting a new packet?
	if (iSendPkt.IsEmpty())
		{
		// Get next packet from send queue
		RMBufPacket pkt;
		if (!iSendQ.Remove(pkt))
			{
			iSendNumBufs=0; // Must be so
			return;
			}

		// Parse and dump the PPP frame to the log
		LOG(if(iLogLevel>=4)  iPppLcp->iLogger->Dump(pkt, KPppHdlcSendChannel); )
		LogUserData(pkt,KPppHdlcSendChannel);	//MS
		
		if (iFlags & KPppHdlcSendFcs16)
			{
			iSendFcs16.Init();
			}
#ifdef __LCP_EXTENSION_FCS_32
		if (iFlags & KPppHdlcSendFcs32)
			{
			iSendFcs32.Init();
			}
#endif
		
/*		
		// Check if have reached the point after which new
		// link config options have to be applied
		if (iFlags & KPppHdlcApplyPending)
			{
			if (pkt.First()==iApplyConfigMarker)
				iFlags |= KPppHdlcAtApplyMark;
			}
*/		
		RMBufPktInfo* info = pkt.Unpack();
		TUint16 prot = (TUint16) TPppAddr::Cast(info->iDstAddr).GetProtocol();

/*
		// Apply new configuration if a non LCP frame
		if (prot!=KPppIdLcp && (iFlags & KPppHdlcAtApplyMark))
			DeferredApplyOptions();
*/
		// Apply new configuration if a non LCP frame
		// Note: This might pose a problem if a defective Cisco router sends
		// an ACP packet too early.
		if (prot!=KPppIdLcp)
			{
			if (iFlags & KPppHdlcApplyPending)
				DeferredApplyOptions();
			}

		// Create and encode the frame header
		CreateHdlcHeader(ptr, aRestart, prot);
		__ASSERT_DEBUG(ptr <= end, User::Panic(_L("PPP Panic"), 0));
		
		iSendPkt.Assign(pkt);
		pkt.FreeInfo();
		--iSendNumBufs;
		}		

	// Sending Packet data...
	TInt ensure = KMBufSmallSize<<1; // worst-case is every byte is escaped
	if (iFlags & KPppHdlcSendFcs16)
		ensure += 4;	// enough space for 2 escaped bytes
#ifdef __LCP_EXTENSION_FCS_32
	if (iFlags & KPppHdlcSendFcs32)
		ensure += 8;	// enough space for 4 escaped bytes
#endif
    // Encode all buffers in the chain (that will fit) into the send buffer
	while ((ptr+ensure)<end)
		{
		RMBuf *m = iSendPkt.First();
		TUint8* mptr = m->Ptr();
		TUint8* mend = m->EndPtr();
		// Encode frame data into send buffer & calculate CRC
		while (mptr<mend)
			{
			EncodeChar(ptr, *mptr++);
			}
		__ASSERT_DEBUG(ptr <= end, User::Panic(_L("PPP Panic"), 0));
		iSendPkt.Remove();
		m->Free();
		--iSendNumBufs;
		if (!iSendFlowOn && iSendNumBufs<iSendLoWat)
			{
			// The backlog of buffers is clearing, so allow more to come
			iSendFlowOn = ETrue;
			iPppLcp->LinkFlowOn();
			}
		if (iSendPkt.IsEmpty())
			break;
		}

    // If the mbuf chain is empty, we've encoded the entire packet, so end it
	// by appending the CRC and flag character to the send buffer.
	// We left ourselves enough space in the buffer for this in the loop above.
	if (iSendPkt.IsEmpty())
		{
		if (iFlags & KPppHdlcSendFcs16)
			{
			TUint16 fcs = iSendFcs16.Fcs();	// save FCS before it changes
			EncodeChar(ptr, (TUint8)(fcs & 0xff));
			EncodeChar(ptr, (TUint8)(fcs >> 8));
			}
#ifdef __LCP_EXTENSION_FCS_32
		if (iFlags & KPppHdlcSendFcs32)
			{
			TUint32 fcs = iSendFcs32.Fcs();	// save FCS before it changes
			EncodeChar(ptr, (TUint8)(fcs & 0xff));
			EncodeChar(ptr, (TUint8)((fcs>>8) & 0xff));
			EncodeChar(ptr, (TUint8)((fcs>>16) & 0xff));
			EncodeChar(ptr, (TUint8)((fcs>>24) & 0xff));
			}
#endif
		// Terminating flag byte
		*ptr++ = KPppHdlcFlagChar;
		__ASSERT_DEBUG(ptr <= end, User::Panic(_L("PPP Panic"), 0));
		}
	iSendBuf.SetLength(ptr - iSendBuf.Ptr());

	// Log after HDLC framing and escaping
	// DoSend() might be called several times to send one (long) frame.
	// This is supported by EpppDumpLogFormat but not necessarily by other formats.
	LOG ( if (iLogLevel>=5) iPppLcp->iLogger->HexDump(_S("COMM Send    "), _S("    "), iSendBuf.Ptr(), iSendBuf.Length()); )
	LOG ( if (iLogFormat == EpppDumpLogFormat) iPppLcp->iLogger->DumpFrame(EpppDirectionSend,iSendBuf); )

	iFlags |= KPppHdlcSendBusy;	// indicate that we have an outstanding write
	iBcaWriter->Write(iSendBuf);
	}


/**
Configures the comm port for our needs.  Also initializes flags
in preparation for receiving the first packet.

*/
void CPppHdlcLink::PacketModeOnL()
	{
	if (iFlags & KPppHdlcCommConfigOk) // Already configured.
		{
		return;
		}

	// Open the BCA channel now:
	iBcaControl->StartStartupSequence(); // Next we are in CBcaControl::RunL	
	}

/** Saves original serial port configuration.

@param aHdlcConfig the config to save */
void CPppHdlcLink::SetOriginalSerialPortConfig(TCommConfig& aHdlcConfig)
	{
	if(iFlags & KPppHdlcOrigConfig) // We already have the original port settings
		{
		return; // What we have may not be original (could have been set by us)
		// This can happen if the link is being restarted 
		// It's easier to protect the settings from here, than from the BCA control
		}
	
	iFlags |= KPppHdlcOrigConfig;
	iOrigConfig = aHdlcConfig;
	}
	
/** Configures the BCA channel serial parameters for use by HDLC.

@param aHdlcConfig the desirable serial configuration.
*/
void CPppHdlcLink::GetSerialPortConfigForHdlc(TCommConfig& aHdlcConfig) const
	{
	LOG( iPppLcp->iLogger->Printf(_L("CPppHdlcLink::GetSerialPortConfigForHdlc()")); )

	aHdlcConfig = iOrigConfig;

	aHdlcConfig().iTerminatorCount = 1;
	aHdlcConfig().iTerminator[0] = KPppHdlcFlagChar;

	// if 7 + parity, correct to 8N (for ISPs such as CompuServe)
	if ( aHdlcConfig().iDataBits == EData7 && aHdlcConfig().iParity != EParityNone )
		{
		aHdlcConfig().iDataBits = EData8;
		aHdlcConfig().iParity   = EParityNone;
		}

	aHdlcConfig().iHandshake = GetHandShaking();
	}

/**
Returns original serial configuration of the BCA channel

@return the original serial port configuration.
 */
TCommConfig CPppHdlcLink::OriginalSerialPortConfig() const
	{
	return iOrigConfig;
	}

	
void CPppHdlcLink::StartPPP()
/** Brings up PPP link and read a packet.*/
	{
	InitEscMap();
		
	// Always expect bytes 0..31 to be escaped initially
	iRecvEscMap = 0xffffffff;

	iFlags = KPppHdlcSendFcs16 | KPppHdlcRecvFcs16;

	iFlags |= KPppHdlcCommConfigOk;
	
	iState = EPppHdlcOpen;
	iSendFlowOn = ETrue;
	iPppLcp->LinkLayerUp();
	
	iRecvBuf = GetExcessData();
    if(iRecvBuf.Length())
		{
		// Simulate a completed read in order to force processing of excess data received from the Agent.
		iBcaReader->ReadReady();
		}
	else
		{
		iRecvBuf.SetMax();
  		iBcaReader->Read(iRecvBuf);
		} 
	}

/**
Triggers BCA shutdown, if applicable. This restores the original Comm port config.
Note: If the BCA was not opened, it will not be shut down. In this case, there is no
need to restore original config, because config can be set on open BCA only.

We can be called twice: Once because PPP shuts down the link, the second time when BCA
reports that it has gone down. In this case, we guard against shutting down the BCA for 
the second time, and report link down to LCP. 
*/
void CPppHdlcLink::PacketModeOff()
	{
	LOG( iPppLcp->iLogger->Printf(_L("CPppHdlcLink::PacketModeOff(): Releasing the link.")); )

	iBcaReader->Cancel();
	iBcaWriter->Cancel();
	iBcaControl->Cancel();

	if (iFlags & KPppHdlcCommConfigOk)
		{
		iFlags &= ~KPppHdlcCommConfigOk;
		}
			
	if(iBcaControl->BcaIsOpen())
		{
		LOG( iPppLcp->iLogger->Printf(_L("CPppHdlcLink::PacketModeOff(): BCA channel is open. Shutting it down.")); )

		iBcaControl->StartShutdownSequence(); // Next we are in CBcaControl::RunL
		// which will call LinkTerminationComplete eventually.	
		}
	else // Nothing to shutdown, the link is finished now.
		{
		LinkTerminationComplete(); 
		}		
	}

/**
Called when the HDLC link termination is complete. 
Releases the send buffer & notifies upper layers 

This call MUST be made after HDLC is finished, else LCP may wait for link to report termination forever.
*/
void CPppHdlcLink::LinkTerminationComplete()
	{
	LOG( iPppLcp->iLogger->Printf(_L("CPppHdlcLink::LinkTerminationComplete: HDLC Link is finished, iLinkDown[%d]: 1 Notifying upper layer."), iLinkDown); )

	iState = EPppHdlcClosed;
	FreeBuf();
	if(iLinkDown) // LCP can ask us not to notify it.
		{
		iPppLcp->LinkLayerDown(iError);		// This is essentially PPP 'Down' event on LCP
		}
	iLinkDown = ETrue; // reset to default
	}

/**
Does the HDLC link require graceful shutdown (as opposed to an immediate close) of the BCA channel?

@return ETrue if yes.
*/ 
TBool CPppHdlcLink::DoShutdownBcaGracefully() const
	{
	return KErrConnectionTerminated != iError;
	}
	

/** Frees the send buffering resources of the BCA.*/
void CPppHdlcLink::FreeBuf()
	{
	LOG( iPppLcp->iLogger->Printf(_L("CPppHdlcLink::FreeBuf()")); )
	iSendPkt.Free();
	iSendQ.Free();
	iSendNumBufs=0;
	}


void CPppHdlcLink::LinkDown(TInt aStatus)
/**
Link down - drops out of packet mode and notifies with reason.

@param aStatus Error code indicating the reason the link is going down
*/
	{
	iError = (KErrNone == iError) ? aStatus : iError; // Try to get an actual error code instead of KErrNone
	// We can stay with KErrNone if the link closure was requested by the upper layer. This is OK, the upper layer
	// will take care of the error code.
	PacketModeOff(); // Disconnect from the link. 
	}

void CPppHdlcLink::ReadIniFileL()
/**
Reads settings from the ini file.
Default values are assumed if the file does not exist.

@leave Error if the ini file is found but cannot be read
*/
	{
	// Set up defaults in case they aren't set in the .ini file
	iDesiredRecvEscMap = 0x00000000;
	iPppLcp->SetMaxReceiveSize(0);

	// Read the correct .ini file depending on whether we're configured for server mode
	CESockIniData* ini = NULL;
	TRAPD(res,
		if (iPppLcp->PppLinkMode() == CPppLcpConfig::EPppLinkIsServer)
			{
			ini = CESockIniData::NewL(PPP_SERVER_INI_FILE);
			}
		else
			{
			ini = CESockIniData::NewL(PPP_INI_FILE);
			}
		)
	if(res!=KErrNone)
		{
		if(res==KErrNotFound)
			{
			return;
			}
		User::Leave(res);
		}

	CleanupStack::PushL(ini);

	// Each port can have its own high and low watermark settings by putting
	// them in a section like [link::comm::0]
    TPortName port(iPppLcp->GetBCAProvision()->GetPortName());
	_LIT(KLink, "link");
	TName name(KLink);
	port.LowerCase();
	name.AppendFormat(_L("::%S"), &port);
	
	TInt lo, hi;
   _LIT(KLowMark, "LoMark");
   _LIT(KHighMark, "HiMark");

	if (ini->FindVar(name, KLowMark, lo) && ini->FindVar(name, KHighMark, hi))
		{
		iSendLoWat = lo;
		iSendHiWat = hi;
		}
	else if (ini->FindVar(KLink, KLowMark, lo) && ini->FindVar(KLink, KHighMark, hi))
		{
		iSendLoWat = lo;
		iSendHiWat = hi;
		}

#if defined (_DEBUG) 
//Log level
//
//Options are:
//n=0 Finite State Machine transition are logged.
//n=1 Reserved for future use.
//n=2 Reserved for future use.
//n=3 Reserved for future use.
//n=4 Log parsed PPP packets *SLOW* (superseded by TcpLog.log).
//n=5 HexDump of PPP packets *VERY SLOW* (superseded by TcpLog.log).
//
//		A new log is generated if the \logs\tcpdump folder is created.
//		TcpDump.log is a log created in a binary format,
//		hence non readable with a text editor.
//		It can be used as an input file for Ethereal..
//	Ethereal is a general purpose packet analyser, that can parse a wide variety of protocols.
//	It is freeware and can be fetched from: www.ethereal.com.
	_LIT(KLogSection, "log");
	_LIT(KLogLevel, "level");
	TInt logLevel;
	if (ini->FindVar(KLogSection, KLogLevel, logLevel) && (logLevel>=0 ))
		{
		iLogLevel = logLevel;
		}
	else
		{
		iLogLevel = 1;
		}

//Log File Format
//
//Options are:
//n=0 PPPdump format
   _LIT(KlogFormat, "logFormat");
	ini->FindVar(KLogSection, KlogFormat, iLogFormat);
	if(iLogFormat<0 || iLogFormat>=ElastLogFormat)
		{
		iLogFormat = EpppDumpLogFormat;
		}

//
//Link format
//
//Options are:
//n=0 Entire PPP frame is logged (for debugging - incompatible with tcptrace)
//n=1 Only IP frames are logged (for performance analysis with tcptrace)
   _LIT(KlogLinkFormat, "linkFormat");
	ini->FindVar(KLogSection, KlogLinkFormat, iLogLinkFormat);
	if(iLogLinkFormat<0 || iLogLinkFormat>=ElastLogLinkFormat)
		{
		iLogLinkFormat = EpppLogLinkFormat;
		}
#endif

//
	// Added for ACCM configuration
	// These two strings are the ones in ppp.ini :-
	// The example below would enable accm config and negate the need to escape values 4 - 7 inclusive
	// [link]
	// PPPEnableAccm= 1
	// PPPAccm= FFFFFF0F
	// XON and XOFF characters are added no matter what
	_LIT(KEnableACCM, "PPPEnableAccm");
	_LIT(KACCMValue, "PPPAccm");
	TInt enable;
	// Check ACCM configuration is enabled first
	if (ini->FindVar(KLink,KEnableACCM, enable) && enable != 0)
		{
		TPtrC hexStr;
		// We are enabled, read the 8 Digit ASCII HEX value
		if (ini->FindVar(KLink,KACCMValue,hexStr))
			{
			// Convert it to a 32 bit integer
			TLex lex(hexStr);
			lex.Val(iDesiredRecvEscMap,EHex);
			}
		}

//
	// Added for MRU configuration
	// These two strings are the ones in ppp.ini :-
	// [link]
	// PPPEnableMru= 1
	// PPPMru= 1500
	_LIT(KEnableMRU, "PPPEnableMru");
	_LIT(KMRUValue, "PPPMru");
	if (ini->FindVar(KLink,KEnableMRU, enable) && enable != 0)
		{
		TInt mru;
		// We are enabled, read a 32 Bit value
		if (ini->FindVar(KLink,KMRUValue,mru))
			{
			iPppLcp->SetMaxReceiveSize(mru);
			}
		}
//

	CleanupStack::PopAndDestroy();
	}


void CPppHdlcLink::StartL()
/**
Starts the PPP link.

@leave Error if comm port cannot be initialized
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("PPP Link StartL")); )
    LOG( iPppLcp->iLogger->DumpFrameFileHeader(iLogFormat, iLogLinkFormat); );

	ReadIniFileL();

	// Configure the Link & BCA.
	PacketModeOnL();
	}
	

void CPppHdlcLink::Stop(TInt aReason, TBool aLinkDown)
/**
Shuts down the PPP link.

@param aReason Error code indicating the reason the link has gone down
@param aLinkDown ETrue if the link is actually going down,
or EFalse if it is just being reset
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("CPppHdlcLink::Stop: Stopping HDLC Link.")); )
	iError = aReason;
	iLinkDown = aLinkDown;
	PacketModeOff(); 
	}

TInt CPppHdlcLink::SpeedMetric()
/**
Determines the speed of the communications port.

@pre PacketModeOnL() must have been called

@return Speed in bps or 0 if unknown
*/
	{
	switch (iOrigConfig().iRate)
		{
	case EBps50:
		return 50;
	case EBps75:
		return 75;
	case EBps110:
		return 110;
	case EBps134:
		return 134;
	case EBps150:
		return 150;
	case EBps300:
		return 300;
	case EBps600:
		return 600;
	case EBps1200:
		return 1200;
	case EBps1800:
		return 1800;
	case EBps2000:
		return 2000;
	case EBps2400:
		return 2400;
	case EBps3600:
		return 3600;
	case EBps4800:
		return 4800;
	case EBps7200:
		return 7200;
	case EBps9600:
		return 9600;
	case EBps19200:
		return 19200;
	case EBps38400:
		return 38400;
	case EBps57600:
		return 57600;
	case EBps115200:
		return 115200;
	case EBps230400:
		return 230400;
	case EBps460800:
		return 460800;
	case EBps576000:
		return 576000;
	case EBps1152000:
		return 1152000;
	case EBps4000000:
		return 4000000;
	case EBpsSpecial:
		return iOrigConfig().iSpecialRate;
	default:
		;	// unknown speed
		}
	return 0;
	}

//=======================================
// JGG PPP CHANGE
void CPppHdlcLink::GetDataTransfer(RPacketContext::TDataVolume& aData)
//void CPppHdlcLink::GetDataTransfer(RGprsContext::TDataVolume& aData)
//=======================================
/**
Returns the current total amount of IP data transferred.

@param aData Receives the data count
*/
	{
	aData.iBytesSent = iSentData;
	aData.iOverflowCounterSent = 0;
	aData.iBytesReceived = iRecvdData;
	aData.iOverflowCounterReceived = 0;
	}

void CPppHdlcLink::LogUserData(RMBufChain& aPacket, TInt aChannel)
/**
Updates the current total of PPP payload bytes transferred.
Separate counts are maintained for bytes received and transmitted.

@param aPacket MBuf chain containing packet
@param aChannel KPppHdlcRecvChannel for the receiving channel
or KPppHdlcSendChannel for the sending channel

@see GetDataTransfer()
*/
	{
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
	const TInt len = info->iLength;
    const TBool CrcError = TPppAddr::Cast(info->iDstAddr).CRCError();
    if (CrcError)
    	{
    	// This is a metapacket containing no useful payload
    	}
	else if (aChannel==KPppHdlcSendChannel)
		{
		iSentData += len;
		}
	else if (aChannel==KPppHdlcRecvChannel)
		{
		iRecvdData += len;
		}
	}

/**
 * Constructor. Performs standard active object initialisation.
 *
 * @param aObserver Reference to the observer of this PPP HdlcLink.
 * 
 */
CBcaControl::CBcaControl(CPppHdlcLink& aUser, MBca& aBca)
	: CActive(EPriorityStandard), 
	  iUser(aUser), 
	  iBca(aBca),
	  iControlStep(ENone)
	{
	CActiveScheduler::Add(this);
	}
	
/**
 * Destructor.
 */
CBcaControl::~CBcaControl()
	{
	Cancel();
	//Make sure the BCA is closed. This is not strictly necessary, as BCA destructor should do it.
	if(iBcaIsOpen)
		{
		iBca.Close();
		}
	}

/** 
This is the central control routine of the BCA. It is called in 2 cases:
1. An outstanding control request on the BCA has completed.
2. A control sequence (startup or shutdown) has started. In this case, it is triggered by our User.

This way, all the control actions on the BCA are dispatched from here only. 
*/	
void CBcaControl::RunL()
	{
	LOG(iUser.Logger().Printf(_L("CBcaControl::RunL(): Step[%d] completed with error[%d]."), iControlStep, iStatus.Int()); )
	 		
	if(KErrNone != iStatus.Int())
		{
		// Note: For clarity, the logic below is not optimized. It called only during BCA startup /shutdown,
		// so the performance penalty is minimal.
		const TBool KOpening       = EOpeningChannel   == iControlStep;
		const TBool KIoctlError  = (!IsShuttingDown() && !KOpening ) && // not opening, & not shutting down.
						(iStatus.Int() != KErrNotSupported && // BCA may not support this Ioctl, this is OK. 
					     iStatus.Int() != KErrAlreadyExists); // This ioctl may not be necessary.
	
		if(KOpening || // Any error on open is fatal, 
		   			  // All errors on shutdown are ignored. 
		   KIoctlError ) // Ioctl errored out, i.e. BCA tried to do it and failed, rather than refuse to do it.
		    {
		    LOG( iUser.Logger().Printf(_L("CBcaControl: operation failed. Link down with [%d]."), iStatus.Int());)
			iControlStep = ENone; // We are not doing anything.
			iUser.LinkDown(iStatus.Int());
			return;
			}
		}
				
	// Our state: the previous operation has completed successfully, or we don't care about failure.
	
	// Execute the specified control action. The switch statement is structured such that a control sequence
	// can be read from top to bottom. (I.e. case = step, on completion go to the case below).
	switch(iControlStep) // Which step did just complete?
		{
		//
		// The startup sequence.
		//
		case EStartingStartup:
			{
			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Setting Iap Id[%d]."), iUser.GetIAPid()); )
			
			iControlStep = ESettingIap;
			TPckg<TUint32> aOpt(iUser.GetIAPid());
			iBca.Ioctl(iStatus,KBcaOptLevelGeneric, KBCASetIapId, aOpt);
			break;
			}
			
		case ESettingIap:
			{
			iControlStep  = ESettingBcaStack;
			TPtrC bcaStack(iUser.GetBCAStack());
			if(bcaStack.Length())
				{
				LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Setting Bca Stack [%S]."), &bcaStack); )
				
				TBuf8<KMaxName> remainingBcaStack8;
				remainingBcaStack8.Copy(bcaStack);
				iBca.Ioctl(iStatus, KBcaOptLevelGeneric,KBCASetBcaStack,remainingBcaStack8);
				}
			else
				{
				TRequestStatus* statusPtr=&iStatus;
				User::RequestComplete(statusPtr,KErrNone);
				}
			break;
			}
		
		case ESettingBcaStack:
			{
			LOG(TPtrC portName = iUser.GetPortName(); iUser.Logger().Printf(_L("CBcaControl: Opening BCA channel [%S]."), &portName); )

            iControlStep = EOpeningChannel;
			__ASSERT_DEBUG(!iBcaIsOpen, User::Panic(_L("PPP Panic"), EPppPanic_UnexpectedBcaOpen));
			iBca.Open(iStatus,iUser.GetPortName());
			break;	
			}
					
		case EOpeningChannel:
			{
			iBcaIsOpen = ETrue;
			
			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Enabling link monitoring for control line failure.")); )
			
			iControlStep = EnablingLinkMonitoring;
			TPckgBuf<TUint32> argPckg(KFailBcaSpecificOnly);
			iBca.Ioctl(iStatus, KBcaOptLevelExtSerial, KSerialMonitorControlLines, argPckg);
			break;
			// We won't have to disable it when we are shutting down, BCA will do it for us.
			}
		
		case EnablingLinkMonitoring:
			{
			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Getting Serial Port Config")); )
			
			iControlStep = EGettingSerialConfig;

			TPckgBuf<TCommConfig> argPckg(iSerialConfig);
			iBca.Ioctl(iStatus, KBcaOptLevelExtSerial, KSerialConfig, argPckg);
			iSerialConfig = argPckg();
			break;
			}			
			
		case EGettingSerialConfig:
			{
			iUser.SetOriginalSerialPortConfig(iSerialConfig); // Save our existing serial port settings, retrieved in the previous step.
			// N.B. If we are being restarted, we could have modified the port settings already, so what get now
			// would NOT be the original settings. It's users job to keep track of that, otherwise the logic here
			// gets too complicated. If this is not the original settings, user will not set them, i.e. the call above
			// should have not effects.
	
			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Configuring serial port for HDLC.")); )
						
			iControlStep = ESettingSerialConfigForHdlc;
			iUser.GetSerialPortConfigForHdlc(iSerialConfig);

			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Setting Serial Config for HDLC, handshaking [%x]."), iSerialConfig().iHandshake); )

			TPckgBuf<TCommConfig> argPckg(iSerialConfig);
			iBca.Ioctl(iStatus, KBcaOptLevelExtSerial, KSerialSetConfig, argPckg);
			break;
			}
					
		case ESettingSerialConfigForHdlc:
			{
			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Setting Rx Tx Buffer size to %d."), KReceiveBufferSize); )
			
			iControlStep = ESettingBufferSize;
			TPckg<TInt> bufSizeOpt(KReceiveBufferSize);
			iBca.Ioctl(iStatus,KBcaOptLevelExtSerial, KSerialSetTxRxBufferSize, bufSizeOpt);
			break;
			}
			
		case ESettingBufferSize:
			{
			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Resetting Buffers.")); )
			
			iControlStep = EResettingBuffers;
			TPckg<TInt> resetMask(KResetRxBuf);
			iBca.Ioctl(iStatus, KBcaOptLevelGeneric, KBCAResetBuffers, resetMask);
			break;	
			}
			
		case EResettingBuffers:
			{
			iControlStep = ENone; // We are finished with the startup sequence.
			iUser.StartPPP();
			break;
			}
			
		//
		// The shutdown sequence		
		// Some operations may fail. If we are shutting down as a result of link failure, we have no way of knowing what
		// exactly has failed. Errors will be ignored, so the shutdown sequence runs fully always. 
		
		case EStartingShutdown:
			{
			LOG(iUser.Logger().Printf(_L("CBcaControl: Bca Ioctl: Restoring original Serial port config.")); )
			
			iControlStep = ERestoringOrigSerialConfig;
			iSerialConfig = iUser.OriginalSerialPortConfig(); // In case user modifies the config before request completion.

			TPckgBuf<TCommConfig> argPckg(iSerialConfig);
			iBca.Ioctl(iStatus, KBcaOptLevelExtSerial, KSerialSetConfig,  argPckg);			
			// This may fail, because the underlying channel provider may have caused the shutdown to start 
			// in the first case. Error will be ignored, we'll always execute the next step.
			break;
			}
			
		case ERestoringOrigSerialConfig:
			{
			iControlStep = EShuttingDownChannel;
			if(iUser.DoShutdownBcaGracefully()) 
				{
				LOG(iUser.Logger().Printf(_L("CBcaControl: Shutting down BCA.")); )
			
				iBca.Shutdown(iStatus);
				// This may fail. If the link is shutting down because of channel provider failure, it may not be possible
				// to shut it down cleanly. Error will be ignored, we'll always execute the next step.
				}
			else // No need to shutdown, BCA is closed
				{
				LOG(iUser.Logger().Printf(_L("CBcaControl: Closing BCA.")); )

				iBca.Close(); // This is a synchronous call, so we need to complete the request ourselves to move on.
				TRequestStatus* statusPtr = &iStatus;
				User::RequestComplete(statusPtr, KErrNone);
				}
			break;
			}
			
		case EShuttingDownChannel:
			{
			iBcaIsOpen = EFalse;
			
			iControlStep = ENone; // We are finished with the shutdown sequence.
			iUser.LinkTerminationComplete(); // We don't pass error code, because erros on termination are ignored.
			break;
			}
			
		default: // Unrecognized step. RFC1661 says we SHOULD not freeze or reset if internal FSM transition is invalid.
			{
			LOG(iUser.Logger().Printf(_L("CBcaControl: ERROR: Illegal control step [%d]"), iControlStep); )
			__ASSERT_DEBUG(EFalse, User::Panic(_L("PPP Panic"), EPppPanic_IllegalBcaControlStep));
			break;
			}
			
		}
		// Complete the step.
		if(ENone != iControlStep) // ENone means we have no step to complete.
			{
			SetActive();
			}
	}

	
/**
 *	Cancels outstanding Control action. 
 */
void CBcaControl::DoCancel()
	{
	LOG(iUser.Logger().Printf(_L("CBcaControl::DoCancel: Cancelling Ioctl. iControlStep[%d]."), iControlStep); )
	
	if(EOpeningChannel == iControlStep ||  EShuttingDownChannel== iControlStep)
		{
		iBca.Close(); // "Cancels" Open & graceful shutdown.
		}
	else if(ENone != iControlStep) // We have an Ioctl request outstanding.
		{
		iBca.CancelIoctl(); 
		// Some Ioctl states may not have an Ioctl outstanding. Canceling is OK, BCA ignores it.		
		}
	iControlStep = ENone;
		
	// N.B. IMPORTANT ASSUMPTION:
	// We are only going to be called if things went wrong, and 
	// the link is being terminated. I.e., after this call, the BCA is closed. 
		
	// We can't always execute an Ioctl more than once an opened BCA. I.e, we can't
	// always set different parameters, because the BCA may have been opened with them.
	
	// If the BCA is closed, its internal state is reset, so it is safe to restart the startup sequence.
	}
	

/**
Launches the BCA channel startup sequence */	
void CBcaControl::StartStartupSequence()
	{
	__ASSERT_DEBUG(!IsActive(), User::Panic(_L("PPP Panic"), EPppPanic_BcaStartupAlreadyActive));
		
	// Jump to startup.
	iControlStep = EStartingStartup;
	TRequestStatus* stat = &iStatus;
	User::RequestComplete(stat, KErrNone);	
	SetActive();
	LOG(iUser.Logger().Printf(_L("CBcaControl: Startup sequence started.")); )
	}
	
/** 
Launches the BCA channel shutdown sequence */	
void CBcaControl::StartShutdownSequence()
	{
	if(IsShuttingDown())
		{
		LOG(iUser.Logger().Printf(_L("CBcaControl: Warning: Shutdown sequence already in progress.")); )
		return; // We are already shutting down.
		}
		
	Cancel(); // stop whatever we are doing and shut ourselves down.
	// Jump to shutdown.
	iControlStep = EStartingShutdown;
	TRequestStatus* stat = &iStatus;
	User::RequestComplete(stat, KErrNone);	
	SetActive();
	LOG(iUser.Logger().Printf(_L("CBcaControl: Shutdown sequence started.")); )
	}

/**
Is the controller executing the shutdown sequence on the BCA? 

@return ETrue if Yes.
*/
TBool CBcaControl::IsShuttingDown()const
	{
	return (
		iControlStep == EStartingShutdown ||
		iControlStep == ERestoringOrigSerialConfig ||
		iControlStep == EShuttingDownChannel);
	}

/**
  C++ Constructor. Performs standard active object initialisation.
 
  @param aUser Our user: PPP HdlcLink.
  @param aBca the BCA we read from on behalf of the user
  @param aPriority the AO priority 
*/
CBcaReader::CBcaReader(CPppHdlcLink& aUser, MBca& aBca, TInt aPriority):
	CActive(aPriority), 
	iUser(aUser),
	iBca(aBca)	  
	{
	CActiveScheduler::Add(this);
	}

/** Destructor */
CBcaReader::~CBcaReader()
	{
	Cancel();
	}
	
/** Called when Read request completes, i.e. BCA can process another read.*/
void CBcaReader::RunL()
	{
	iUser.BcaReadComplete(iStatus.Int());		
	}
	
/** Cancels an outstanding read request.*/
void CBcaReader::DoCancel()
	{
	iBca.CancelRead();
	}

/**
Triggers completion of the read */	
void CBcaReader::ReadReady()
	{
	TRequestStatus* statusPtr = &iStatus;
	User::RequestComplete(statusPtr,KErrNone);
	SetActive();
	}
	
/** Queues a read on the BCA.*/
void CBcaReader::Read(TDes8& aDes)
	{
	iBca.Read(iStatus,aDes);
	SetActive();
	}
	

/**
C++ constructor 

@param aUser: the BCA user - the HDLC link
@param aBca: the BCA to use for writes
@param aPriority write AO priority 
*/	
CBcaWriter::CBcaWriter(CPppHdlcLink& aUser, MBca& aBca, TInt aPriority)
	: CActive(aPriority), 
	  iUser(aUser),
	  iBca(aBca)	  
	{
	CActiveScheduler::Add(this);
	}

/** C++ Destructor */
CBcaWriter::~CBcaWriter()
	{
	Cancel();
	}
	
/** Called on Write completion, indicating a readiness to handle another write.*/
void CBcaWriter::RunL()
	{
	iUser.BcaWriteComplete(iStatus.Int());		
	}
	
/** Cancels an outstanding write request.*/
void CBcaWriter::DoCancel()
	{
	iBca.CancelWrite();
	}
	
/** Writes on the BCA channel.

@param aDes the buffer to write.
*/	
void CBcaWriter::Write(const TDesC8& aDes)
	{
	iBca.Write(iStatus,aDes);
	SetActive();
	}
