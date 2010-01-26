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
// Authentication Protocol (CHAP) - RFC 1994, containing source code
// common to all PPP authentication protocols derived from CHAP.
// 
//

/**
 @file
 @brief Source file for the implementation of PPP Challenge Handshake
 @internalComponent
*/

#include "PPPCHAP.H"

CPppChap::~CPppChap()
/**
   Destructor.
   @internalComponent
*/
	{
	if (iPppLcp != 0)
		{		
		iChallengePacket.Free();
		TimerDelete();
		}
	}

void CPppChap::InitL(CPppLcp* aLcp)
/**
   @copydoc CPppAuthentication::InitL(CPppLcp*)
   @see CPppAuthentication::InitL(CPppLcp*)
   @internalComponent
*/
	{
	CPppAuthentication::InitL(aLcp);
	TimerConstructL(KPppFsmTimerPriority);
	Register();
	}

void CPppChap::LowerLayerUp()
/**
   @copydoc MPppRecvr::LowerLayerUp()
   @see MPppRecvr::LowerLayerUp()
   @internalComponent
*/
	{
	ASSERT(iPppLcp != 0);
	AuthenticateRequest();
	}


void CPppChap::LowerLayerDown(TInt /*aStatus*/)
/**
   @copydoc MPppRecvr::LowerLayerDown(TInt)
   @see MPppRecvr::LowerLayerDown(TInt)
   @internalComponent
*/
	{
	ASSERT(iPppLcp != 0);
	TimerCancel();
	}


void CPppChap::AuthenticateComplete(TInt aStatus)
/**
   @copydoc CPppAuthentication::AuthenticateComplete(TInt)
   @see CPppAuthentication::AuthenticateComplete(TInt)
   @internalComponent
*/
	{
	ASSERT(iPppLcp != 0);
	if (aStatus==KErrNone)
		{
		if (!iChallengePacket.IsEmpty())
			{
			iResponseRetryCount = 0;
			//ignore Error, if fails it will time out 
			//and try again anyway.
			TRAP_IGNORE(RespondL());
			}
		}
	else
		DoFail(aStatus);
	}


TBool CPppChap::RecvFrame(RMBufChain& aPacket)
/**
   @copydoc MPppRecvr::RecvFrame(RMBufChain&)
   @see MPppRecvr::RecvFrame(RMBufChain&)
   @internalComponent
*/
	{
	ASSERT(iPppLcp != 0);

	RMBufPacket pkt;
	pkt.Assign(aPacket);
	pkt.Unpack();
	RMBufPktInfo* info = pkt.Info();

	if (IsInactive())
		{
		pkt.Free();
		return EFalse;
		}


// Extract and drop LCP header
	pkt.Align(4);

	if (info->iLength < KPppChapCodeFieldSize + 
			    KPppChapIdFieldSize +
			    KPppChapLengthFieldSize)
		{
// Too short!
		pkt.Free();
		return EFalse;
		}
		

// check that the length of the packet is OK
	TUint16 length = BigEndian::Get16(pkt.First()->Ptr() +
			 		KPppChapCodeFieldSize + 
					KPppChapIdFieldSize);

	if (info->iLength < length)
		{
// Too short!
		pkt.Free();
		return EFalse;
		}
	else if (info->iLength > length)
		pkt.TrimEnd(length);

	ASSERT(pkt.Length()==length);

	TUint8 code = *(pkt.First()->Ptr());

	TInt ret;
	switch (code)
		{
	case KPppChapChallengeCode:
			{
			TRAP(ret, ChallengeL(pkt));
			}
		break;

	case KPppChapResponseCode:
//		{
//		TInt vc = *ptr++;
//		TUint8* vp = ptr;
//		TInt nc = len-(vc+1);
//		TUint8* np = ptr+vc;
//		}
		break;

	case KPppChapSuccessCode:
			{
			TRAP(ret, SuccessL(pkt));
			}
		break;

	case KPppChapFailureCode:
			{
			TRAP(ret, FailureL(pkt));

			if (ret!=KErrNone)
				DoFail(KErrIfAuthenticationFailure);
			}
		break;

	// default: invalid CHAP packet code
	// simply ignore the packet
		}

	pkt.Free();
	return EFalse;
	}

// RFC 1994: "The Challenge-Handshake Authentication Protocol (CHAP)
// is used to periodically verify the identity of the peer using a
// 3-way handshake.  This is done upon initial link establishment, and
// MAY be repeated anytime after the link has been established."


void CPppChap::ChallengeL(RMBufPacket& aPacket)
/**
   Processes a CHAP Challenge packet and attempts to respond to the
   challenge.
   @param aPacket [in] The CHAP Challenge packet to be processed.
   @internalComponent
*/
	{
	TimerCancel();
	ProcessChallengePacketL(aPacket);

	if (IsAuthenticateRequestDone())
		{
		iResponseRetryCount = 0;
		RespondL();
		}
	}


void CPppChap::CheckChallengePacketL(RMBufPacket& aPacket)
/**
   Checks that a CHAP Challenge packet is valid.  Leaves if the packet
   is malformed.
   @param aPacket [in] The CHAP Challenge packet to be checked.
   @internalComponent
*/
	{
// In the case of MS-CHAP-V1 and MS-CHAP-V2 the Challenge Name may be
// empty.  RFC 1994: "The Name field is one or more octets [...]".
// RFC2433, RFC2759: "Microsoft authenticators do not currently
// provide information in the Name field.  This may change in the
// future."
	__ASSERT_ALWAYS(aPacket.Length() >= KPppChapCodeFieldSize +
					KPppChapIdFieldSize + 
					KPppChapLengthFieldSize +
					KPppChapValueSizeFieldSize + 
					KPppChapMinValueSize,
			User::Leave(KErrUnderflow));

	TUint8 valueSize = *(aPacket.First()->Ptr() +
			KPppChapCodeFieldSize +
			KPppChapIdFieldSize +
			KPppChapLengthFieldSize);

	__ASSERT_ALWAYS(valueSize >= KPppChapMinValueSize,
			User::Leave(KErrUnderflow));

	__ASSERT_ALWAYS(valueSize <= aPacket.Length() -
				KPppChapCodeFieldSize - 
				KPppChapIdFieldSize -
				KPppChapLengthFieldSize - 
				KPppChapValueSizeFieldSize,
			User::Leave(KErrOverflow));	
	}


void CPppChap::ProcessChallengePacketL(RMBufPacket& aPacket)
/**
   Processes a CHAP Challenge packet.
   @param aPacket [in] The CHAP Challenge packet to be processed.
   @internalComponent
*/
	{
	CheckChallengePacketL(aPacket);

	iChallengePacket.Free();
	iChallengePacket.Assign(aPacket);

// go past the CHAP Code field and
// read the CHAP Identifier field
	iCurrentId = *(iChallengePacket.First()->Ptr() 
					+ KPppChapCodeFieldSize);

// Go past the CHAP Code field, the CHAP Identifier field and the CHAP
// Length field and read the CHAP Value-Size field.
	TUint8 valueSize = *(iChallengePacket.First()->Ptr() +
			KPppChapCodeFieldSize +
			KPppChapIdFieldSize +
			KPppChapLengthFieldSize);

// Go past the CHAP Code field, the CHAP Identifier field, the CHAP
// Length field and the CHAP Value-Size field and read the CHAP Value
// field
	iChallengeRef.Set(iChallengePacket.First()->Ptr() +
				KPppChapCodeFieldSize +
				KPppChapIdFieldSize +
				KPppChapLengthFieldSize +
				KPppChapValueSizeFieldSize,
			valueSize,
			valueSize);
	}


void CPppChap::RespondL()
/**
   Responds to the latest CHAP Challenge received.
   @internalComponent
*/
	{
	ASSERT(!iChallengePacket.IsEmpty());
	ASSERT(IsAuthenticateRequestDone());

	MakeResponseL(iCurrentId, 
			iChallengeRef, 
			iResponseValueRef,
			iResponseNameRef);
	SendResponseL(iCurrentId, iResponseValueRef, iResponseNameRef);

	if (++iResponseRetryCount < KPppChapMaxResponseRetryCount)
		TimerAfter(KPppChapResponseRetryTimerPeriod*  1000);
	}


void CPppChap::MakeResponsePacketLC(TUint8 aIdentifier, 
				const TDesC8& aValue, 
				const TDesC8& aName, 
				RMBufPacket& aPacket)
/**
   Creates a CHAP response Packet.
   @param aIdentifier [in] The CHAP Response Identifier.
   @param aValue [in] The CHAP Response Value.
   @param aName [in] The CHAP Response Name.
   @param aPacket [out] The CHAP Response packet.
   @internalComponent
*/
	{
	ASSERT(aValue.Length() <= KMaxTInt8);
	ASSERT(aName.Length() <= 
		   KMaxTInt16 - 
		   KPppChapCodeFieldSize -
		   KPppChapIdFieldSize - 
		   KPppChapLengthFieldSize -
		   KPppChapValueSizeFieldSize -
		   aValue.Length());

	TUint16 length = static_cast<TUint16>(KPppChapCodeFieldSize +
			 	KPppChapIdFieldSize + 
				KPppChapLengthFieldSize +
				KPppChapValueSizeFieldSize + 
				aValue.Length() +
				aName.Length());

	aPacket.AllocL(length);
	CleanupStack::PushL(aPacket);
	RMBufPktInfo* info = aPacket.NewInfoL();

// Construct packet header
	TUint8* ptr = aPacket.First()->Ptr();

// write the CHAP Code field
	*ptr = KPppChapResponseCode;
	ptr += KPppChapCodeFieldSize;

// write the CHAP Identifier field
	*ptr = aIdentifier;
	ptr += KPppChapIdFieldSize;

// write the CHAP Length field
	BigEndian::Put16(ptr, length);
	ptr += KPppChapLengthFieldSize;

// write the CHAP Value-Size field
	*ptr = static_cast<TUint8>(aValue.Length());
	ptr += KPppChapValueSizeFieldSize;

	Mem::Copy(ptr, aValue.Ptr(), aValue.Length());
	ptr += aValue.Length();
	
	Mem::Copy(ptr, aName.Ptr(), aName.Length());
	ptr += aName.Length();

	info->iLength = length;
	TPppAddr::Cast((info->iDstAddr)).SetProtocol(KPppIdChap);
	aPacket.Pack();
	}


void CPppChap::SendResponseL(TUint8 aResponseId, 
			const TDesC8& aResponseValue, 
			const TDesC8& aResponseName)
/**
   Generates a CHAP Response packet and sends it to the peer.
   @param aResponseId [in] The CHAP Response Identifier.
   @param aResponseValue [in] The CHAP Response Value.
   @param aResponseName [in] The CHAP Response Name.
   @internalComponent
*/
	{
	RMBufPacket packet;
	MakeResponsePacketLC(aResponseId,
			aResponseValue,
			aResponseName,
			packet);
	SendFrame(packet);
	CleanupStack::Pop(); // packet
	}


void CPppChap::SuccessL(RMBufPacket& aPacket)
/**
   Processes a CHAP Success packet and takes action to complete the
   authentication.
   @param aPacket [in] The CHAP Success packet to be processed.
   @internalComponent
*/
	{
	__ASSERT_ALWAYS(aPacket.Length() >= KPppChapCodeFieldSize +
					KPppChapIdFieldSize + 
					KPppChapLengthFieldSize,
			User::Leave(KErrUnderflow));

// check the id
	if (!CheckIdentifier(aPacket))
		User::Leave(KErrGeneral);

	TimerCancel();
	DoSucceed();
	if (iPppLcp->CallbackEnabled() &&
		iPppLcp->CallbackRequestType() !=
			ECallbackIETFRequestTypeMSCBCP)
		{
		iPppLcp->CallbackGrantedAndAuthenticated();
		iPppLcp->TerminateLink(MNifIfNotify::ECallBack);
		}
	}


void CPppChap::FailureL(RMBufPacket& aPacket)
/**
   Processes a CHAP Failure packet and takes action to handle the
   authentication failure.
   @param aPacket [in] The CHAP Failure packet to be processed.
   @internalComponent
*/
	{
	__ASSERT_ALWAYS(aPacket.Length() >= KPppChapCodeFieldSize +
					KPppChapIdFieldSize + 
					KPppChapLengthFieldSize,
			User::Leave(KErrUnderflow));

// check the id
	if (!CheckIdentifier(aPacket))
		User::Leave(KErrGeneral);

	TimerCancel();
	DoFail(KErrIfAuthenticationFailure);
	}


void CPppChap::TimerComplete(TInt /*aStatus*/)
/**
   Signals that the response retry timer has expired.
   @param aStatus [in] A status code.
   @see MTimer::TimerComplete(TInt)
   @internalComponent
*/
	{
	ASSERT(iPppLcp != 0);
	//ignore Error, if fails it will time out 
	//and try again anyway.
	TRAP_IGNORE(RetryResponseL());
	}


void CPppChap::RetryResponseL()
/**
   Resends the latest CHAP Response sent.
   @internalComponent
*/
	{
	SendResponseL(iCurrentId, iResponseValueRef, iResponseNameRef);

	if (++iResponseRetryCount < KPppChapMaxResponseRetryCount)
		TimerAfter(KPppChapResponseRetryTimerPeriod*  1000);
	}
