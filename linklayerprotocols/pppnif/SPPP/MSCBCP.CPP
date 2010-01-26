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

#include <in_iface.h>
#include "MSCBCPC.H"


//


/*
   Exactly one Callback Control Protocol packet is encapsulated
   in the Information field of a PPP Data Link Layer frame.
   A summary of the CBCP packet format is shown below.  The
   fields are transmitted from left to right.
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Code      |  Identifier   |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Data ...   +-+-+-+-+   Code
      The Code field is one octet and identifies the type of CBCP
      packet.  CBCP Codes are assigned as follows:
	     1       Callback Request       ( Answerer -> Caller )
         2       Callback Response      ( Caller   -> Answerer )
         3       Callback Ack           ( Answerer -> Caller )
	Identifier
      The Identifier field is one octet and aids in matching requests
      and replies.   Length
      The Length field is two octets and indicates the length of the
      CBCP packet including the Code, Identifier, Length and Data
      fields.  Octets outside the range of the Length field should be
      treated as Data Link Layer padding and should be ignored on
      reception.


  3.2 Callback Configuration Options

    0                   1                   2
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Callback Type |    Length     |Callback delay |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Callback Address(es) ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

    Callback Type

    1 - No callback
    2 - Callback to a user-specifiable number.
    3 - Callback to a pre-specified or administrator specified number.
    4 - Callback to any of a list of numbers.


    Length
      The Length field is one octet and indicates the length of the
      Callback Option including the Type, Length and Data fields.
      >=3

    Callback delay
        The amount of time is seconds the Answerer MUST wait before
        calling the Caller back.  Answerer sets to 0, Caller MAY change if
        the required value if is different from 0.
        <= 255


    Callback Addresses

    0                   1                   2
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Address Type  |    ASCIIZ address
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        Address Type

            1 - for PSTN/ISDN

            Other? (TBD)

        Address Format

            For PSTN/ISDN this is an NULL terminated ASCII string.

            Valid characters are 0-9, *, #, T, P, W, @, comma, space,
            dash, and parentheses."


+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//-+-----+
|  Code | Id  |    Length     |CBckType| Len | delay | 1   | phone number  |  0  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//-+-----+
    0      1     2       3        4       5      6      7     8    ..
*/


const TUint KPppIdNcpMsCbcp = 0xC029;

const TUint8 KPppMsCbcpOptRequest  = 1;	// Callback Request  ( Answerer -> Caller )
const TUint8 KPppMsCbcpOptResponse = 2;	// Callback Response ( Caller   -> Answerer )
const TUint8 KPppMsCbcpOptAck     = 3;	// Callback Ack      ( Answerer -> Caller )

enum TMsCbcpCallbackOptType
		{
		EMsCbcpCallbackOptTypeNoCallback = 1,
		EMsCbcpCallbackOptTypeUserSpecified = 2,
		EMsCbcpCallbackOptTypeServerSpecified = 3,
		EMsCbcpCallbackOptTypeList = 4
		};

#define PKT_BUF_SIZE			256
#define RESPONSE_BUF_SIZE		PKT_BUF_SIZE
#define KCbcpMaxRequestLength	256	// maximum length allowed for an incoming MsCbcpOptRequest pkt

/**
'MS CBCP-client' class (a client is the machine that dials out to request callback)
@internalComponent
*/
NONSHARABLE_CLASS(CPppMsCbcpClient) : public CBase, public MPppRecvr
	{
	friend class PppNcpMsCbcpFactory;
public:
	CPppMsCbcpClient(	CPppLcp * aLcp,
						TMSCBCPAction aAction,
						const TUint8 * aCallbackInfo,
						const TInt aCallbackInfoLen );
	~CPppMsCbcpClient();
	void ConstructL();
private:
	// from MPppRecvr: upcalls
	virtual TBool RecvFrame(RMBufChain& aPacket);		// from MPppRecvr
	virtual void LowerLayerUp();						// from MPppRecvr
	virtual void LowerLayerDown(TInt aStatus);			// from MPppRecvr
	virtual void KillProtocol();
	virtual void FrameError();

private:
	INLINE void ValidateArgumentsL() const;
	INLINE void HandleRXOfferFromServer(const TUint8 * aPacket, TInt aLen );
	INLINE TBool GenerateResponseToServersOffer(	TBool&		aServerOfferAcceptable,
									TUint8 *		aResponse,
									TInt&			aResponseLen,
									const TUint8 *	aOfferPktFromServer,	// recvd packet
									TInt			aOfferPktFromServerLen);
	INLINE void ProcessRecvdFrame(const TUint8 * aPacket, TInt aLen );
	INLINE void SendResponse(const TUint8 * aResponse, TInt aResponseLen );
	INLINE TBool IsCallbackOfferAcceptable( TMsCbcpCallbackOptType aRecvdCallbackType ) const;
	INLINE TBool AcceptServerSpecifiedNumber() const;
	INLINE TInt GetCallbackResponsePhoneNumber(
											TUint8 *				aResultPhoneNumber,	// output
											TMsCbcpCallbackOptType	aRecvdCallbackType ) const;
	INLINE void RecvdAck();
	INLINE TInt GetClientPhoneNumber( TUint8 * aResultPhoneNumber ) const;
private:
	CPppLcp * const			iLcp;
	TBool					iServerOfferAcceptable;	// True if the server has made an acceptable offer of callback to use
	const TMSCBCPAction		iAction;
	const TUint8 * const	iCallbackInfo;
	const TInt				iCallbackInfoLen;
	TInt					iResponseLen;
	TUint8					iResponseBuf[ RESPONSE_BUF_SIZE ];
	TBool					iDead;
	};

#define LOG1( p1 )				LOG( Log::Printf(_L(p1) ) )
#define LOG2( p1, p2 )			LOG( Log::Printf(_L(p1), (p2) ) )
#define LOG3( p1, p2, p3 )		LOG( Log::Printf(_L(p1), (p2), (p3) ) )
#define LOG4( p1, p2, p3, p4 )	LOG( Log::Printf(_L(p1), (p2), (p3), (p4) ) )



CPppMsCbcpClient * PppNcpMsCbcpFactory::NewL(	CPppLcp * aLcp,
												TMSCBCPAction aAction,
												const TUint8 * aCallbackInfo,
												const TInt aCallbackInfoLen )
	{
	CPppMsCbcpClient * p;

	p = new (ELeave) CPppMsCbcpClient(aLcp, aAction, aCallbackInfo, aCallbackInfoLen );
	CleanupStack::PushL( p );
	p->ConstructL();
	CleanupStack::Pop();
	return p;
	}

void PppNcpMsCbcpFactory::Delete( CPppMsCbcpClient * instance)
	{
	delete instance;
	}

CPppMsCbcpClient::CPppMsCbcpClient(	CPppLcp *		aLcp,
									TMSCBCPAction	aAction,
									const TUint8 *	aCallbackInfo,
									const TInt		aCallbackInfoLen )
	:	MPppRecvr(aLcp, EPppPhaseLateCallback, KPppIdNcpMsCbcp ),
		iLcp( aLcp ),
		iServerOfferAcceptable( EFalse ),
		iAction( aAction ),
		iCallbackInfo( aCallbackInfo ),
		iCallbackInfoLen( aCallbackInfo ? aCallbackInfoLen : 0 ),
		iDead( EFalse )
	{
	__ASSERT_DEBUG( iCallbackInfoLen >=0, PppPanic(EPppPanic_PPPNoCallbackInfo) );
	__ASSERT_DEBUG( iLcp, PppPanic(EPppPanic_PPPLcpRequired) );
	}

CPppMsCbcpClient::~CPppMsCbcpClient()
	{
	}

void CPppMsCbcpClient::ConstructL()
	{
	ValidateArgumentsL();
	Register();
	}

//
// CRC Error for a frame deliverred to this protocol
// No use to this module
//
void CPppMsCbcpClient::FrameError()
	{
	}

//
// Someone rejected this we should not receive any more 
// frames on it.
//
void CPppMsCbcpClient::KillProtocol()
	{
	iDead = ETrue;
	}

void CPppMsCbcpClient::ValidateArgumentsL() const
	{
	if (RESPONSE_BUF_SIZE < (8+1+iCallbackInfoLen))
		{
		User::Leave(KErrArgument);
		}
	}

void CPppMsCbcpClient::LowerLayerUp()
	{
	// TBS
	}

void CPppMsCbcpClient::LowerLayerDown(TInt /*aStatus*/)
	{
	}


TBool CPppMsCbcpClient::RecvFrame(RMBufChain& aPacket)
	{
	RMBufPacket pkt;
	pkt.Assign(aPacket);
	pkt.Unpack();

	if (iDead)
		{
		// Link dead
		pkt.Free();
		return EFalse;
		}

	const TUint rmbuflen = pkt.Info()->iLength;

	// Extract and drop LCP header
	pkt.Align(4);
	const TUint8 * ptr = pkt.First()->Ptr();

	TUint len = BigEndian::Get16( ptr+2 );
	
	// Check packet length is OK
	if (rmbuflen < len)
		{
		// Too short!
		pkt.Free();
		return EFalse;
		}
	ProcessRecvdFrame( ptr, len );
	pkt.Free();
	return EFalse;
	}

void CPppMsCbcpClient::HandleRXOfferFromServer(const TUint8 * aPacket, TInt aLen )
	{
	if (! GenerateResponseToServersOffer(	iServerOfferAcceptable,
									iResponseBuf,
									iResponseLen,
									aPacket,
									aLen))
		{
		// if invalid or unacceptable, take it all down
		iPppLcp->TerminateLink(MNifIfNotify::EDisconnect, KErrIfCallbackNotAcceptable);	// all done, shut it all down
		return;	
		}

	SendResponse( iResponseBuf, iResponseLen );
	}


void CPppMsCbcpClient::RecvdAck()
	{

	iLcp->CallbackGrantedAndAuthenticated();
	iPppLcp->TerminateLink(MNifIfNotify::ECallBack);	// all done, shut it all down
	}


void CPppMsCbcpClient::SendResponse(const TUint8 * aResponsePkt, TInt aResponsePktLen )
	{
	RMBufPacket pkt;
	RMBufPktInfo* info = NULL;
	const TInt len = aResponsePktLen;

	TRAPD(err, pkt.AllocL(len));
	if (err!=KErrNone)
		return;

	TRAP(err, info = pkt.NewInfoL());
	if (err!=KErrNone)
		{
		pkt.Free();
		return;
		}
	TUint8 * const ptr = pkt.First()->Ptr();
	Mem::Copy( ptr, aResponsePkt, aResponsePktLen );
	info->iLength = len;	
	TPppAddr::Cast((info->iDstAddr)).SetProtocol(iPppId);
	pkt.Pack();
	SendFrame(pkt);
	}

const TInt KMsCbcpCallbackDelay = 2;	// callback delay in seconds suggested to server

TBool CPppMsCbcpClient::IsCallbackOfferAcceptable( TMsCbcpCallbackOptType aRecvdCallbackType ) const
// Return TRUE if the servers offer of callback is acceptable
	{
	switch ( aRecvdCallbackType )
		{
	case EMsCbcpCallbackOptTypeNoCallback:	// 1
		return EFalse;
	case EMsCbcpCallbackOptTypeUserSpecified:	// 2
		return ETrue;
	case EMsCbcpCallbackOptTypeServerSpecified:	// 3
		return AcceptServerSpecifiedNumber();
	case EMsCbcpCallbackOptTypeList: // 4
		return EFalse;
	default:
		return EFalse;	// recvd a bad garbage thing
		}
	}

INLINE TBool CPppMsCbcpClient::AcceptServerSpecifiedNumber() const
// Returns true if we are willing to accept a phone number specified by the server, rather than one we've specified ourselves.
	{
	return iAction != EMSCBCPActionRequireClientSpecifiedNumber;
	}



INLINE TInt ResponseLength( TMsCbcpCallbackOptType responseCallbackType, TInt responsePhoneNumberLen, TInt aRequestFromSrvLen )
// Return the total length of a response packet of type 'responseCallbackType'
// including a phone number of length responsePhoneNumberLen, if applicable
// Note: responsePhoneNumberLen does NOT include any trailing zero on the phone number
	{
	switch ( responseCallbackType )
		{
	case EMsCbcpCallbackOptTypeUserSpecified:
		return (8 + 1 + responsePhoneNumberLen);	// +1 for zt
	case EMsCbcpCallbackOptTypeServerSpecified:
		return aRequestFromSrvLen+3;
	default:
		return 0;				
		}
	}

TBool CPppMsCbcpClient::GenerateResponseToServersOffer(	TBool&			aServerOfferAcceptable,	// out
												TUint8 *		aResponse,				// out: resulting response packet
												TInt&			aResponseLen,			// out: len of above
												const TUint8 *	aOfferPktFromServer,			// recvd packet
												TInt			aOfferPktFromServerLen)
// Analyses the given received recvd MsCbcpOptRequest packet 'aOfferPktFromServer'
// If it is valid,
//		writes an appropriate response packet at aResponse, aResponseLen
// Sets caller's aServerOfferAcceptable true/false according to whether the server's offer of callback is acceptable
// Returns True if the recvd MsCbcpOptRequest packet is valid and acceptable
// Returns False if its corrupt or unacceptable, in which case we must terminate the link
	{

	if ( aOfferPktFromServerLen > KCbcpMaxRequestLength )
		return EFalse;
	if ( aOfferPktFromServerLen <= 4 )
		return EFalse;

	//
	// We can receive multiple options in that case we have to decide which 
	// to use. Specifically in the case off a user specified number to be called
	// back on the options are:
	// 1. No Callback and
	// 2. Caller specifies the number
	// PRR 8/4/98
	//
	TInt	lengthProcessed=4; // Used to keep track when we're offerred multiple options
	TBool	callbackok = EFalse;
	TInt	responsePhoneNumberLen;
	TMsCbcpCallbackOptType recvdCallbackType;

	do
		{
		recvdCallbackType = TMsCbcpCallbackOptType( aOfferPktFromServer[lengthProcessed] );

		//
		// If someone offers us No Call back then we try to match any other option 
		// they may send, but we have this as a default to go back to.
		callbackok = IsCallbackOfferAcceptable( recvdCallbackType );

		if (callbackok)
			{
			break;
			}

		lengthProcessed += aOfferPktFromServer[lengthProcessed+1];
		} while ( lengthProcessed < aOfferPktFromServerLen );

	if (!callbackok)
		{
		return EFalse;	// cannot agree
		}

	TMsCbcpCallbackOptType responseCallbackType = recvdCallbackType;

	responsePhoneNumberLen = GetCallbackResponsePhoneNumber(	aResponse+8,	// result phone number
																recvdCallbackType );


	*(aResponse+8 + responsePhoneNumberLen) = 0;	// zero-terminate it

	TInt responselen = ResponseLength( responseCallbackType, responsePhoneNumberLen, aOfferPktFromServerLen-lengthProcessed );

	aResponse[0] = KPppMsCbcpOptResponse;
	aResponse[1] = aOfferPktFromServer[1];		// id
	BigEndian::Put16( aResponse+2, TUint16(responselen) );
	aResponse[4] = TUint8(responseCallbackType);
	aResponse[5] = TUint8( responselen - 4 ); 
	if (recvdCallbackType == EMsCbcpCallbackOptTypeUserSpecified)
		{
		aResponse[6] = KMsCbcpCallbackDelay;
		}
	else
		{
		aResponse[6] = aOfferPktFromServer[6+lengthProcessed];
		}
	aResponse[7] = 1;	// PSTN Address type

	aResponseLen = responselen;				// return result
	aServerOfferAcceptable = callbackok;	// return result
	return ETrue;
	}

TInt CPppMsCbcpClient::GetClientPhoneNumber( TUint8 * aResultPhoneNumber ) const
	{
	Mem::Copy( aResultPhoneNumber, iCallbackInfo, iCallbackInfoLen );
	return iCallbackInfoLen;
	}

TInt CPppMsCbcpClient::GetCallbackResponsePhoneNumber(
											TUint8 *				aResultPhoneNumber,	// output
											TMsCbcpCallbackOptType	aRecvdCallbackType ) const
// Works out what the phone number should be in our response packet - our response to the server's offer
// Outputs:
//	Writes the phone number at aResultPhoneNumber
//	Returns the length of the result
//
// Given the received
	{
	switch ( aRecvdCallbackType )
		{
	case EMsCbcpCallbackOptTypeUserSpecified: //= 2,
		return GetClientPhoneNumber( aResultPhoneNumber );
	case EMsCbcpCallbackOptTypeServerSpecified: //= 3,
		return 0; // We have no say over the phone number, we do as we're bid PRR
	default:		
        __ASSERT_DEBUG( EFalse, PppPanic(EPppPanic_PPPInvalidCallback) );
		}
	return 0;
	}


void CPppMsCbcpClient::ProcessRecvdFrame(const TUint8 * aPacket, TInt aLen )
	{
	TUint op = aPacket[0];

	TInt len = BigEndian::Get16( aPacket + 2);
	if ( len <=4 || len > aLen )
		return;

	switch (op)
		{
	case KPppMsCbcpOptRequest:
		HandleRXOfferFromServer( aPacket, len );
		break;
	case KPppMsCbcpOptAck:
		RecvdAck();
		break;
	default:
		break;
		}
	}

