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

#include <networking/in_std.h>
#include <in_sock.h>
#include "VJLOG.H"
#include "VJ.H"

CVJDeCompressor::CVJDeCompressor()
	{
	}

void CVJDeCompressor::ConstructL( CVJCompFactory* aFactory, TUint aMaxSlot)
/**
Construct the CVJDeCompressor object.

@param aFactory Factory that created this object
@param aMaxSlot Value of the highest VJ connection number to be received
*/
	{
	//
	// Note that aMaxSlot is the index of the maximum slot that will be accessed.
	//
	__ASSERT_DEBUG(aMaxSlot <= KMaxVjSlot, User::Panic(_L("VJ Panic"), 0));
	iNumVJSlots = aMaxSlot;
	iRxStates = new (ELeave) TVJCompHdr[aMaxSlot+1];
	iFactory = aFactory;
	iFactory->Open();
	}

CVJDeCompressor::~CVJDeCompressor()
	{
	delete []iRxStates;
	if (iFactory)
	    {
		iFactory->Close();
		}
	}

void CVJDeCompressor::CRCError()
/**
Sets the discard flag after receiving a bad frame so that future
decompressed packets don't get out of sync.
*/
	{
	LOG(_LIT(string1,"CRC Error");)
	LOG(Log::Write(string1);)
	SetFlag(KVJDiscard);
	}

void CVJDeCompressor::CopyRecvHeader(const TUint aConnection, ThdrIP* aHeader)
/**
Caches a TCP/IP Header to be used as a reference to
reconstruct future compressed headers.

@param aConnection Valid VJ connection number
@param aHeader TCP/IP header
*/
	{
	__ASSERT_DEBUG(aConnection <= iNumVJSlots, User::Panic(_L("VJ Panic"), 0));
	iRxStates[aConnection].StoreTCPIPHeader(aHeader);
	}

void CVJDeCompressor::GetStoredRxHeader(const TUint aConnection, ThdrIP* aIPHeader, ThdrTCP* aTCPHeader)
/**
Retrieves a packet header from the VJ connection cache.

@param aConnection Valid VJ connection number to retrieve
@param aIPHeader IP packet header
@param aTCPHeader TCP header
*/
	{
	__ASSERT_DEBUG(aConnection <= iNumVJSlots, User::Panic(_L("VJ Panic"), 0));
	iRxStates[aConnection].RetrieveTCPIPHeader(aIPHeader, aTCPHeader);
	}


TBool CVJDeCompressor::CheckStoredRxHeader(const TUint aConnection) const
/**
Checks if the given VJ connection number has previously been filled in.

@param aConnection Valid VJ connection number to check

@return ETrue if CopyRecvHeader() has previously been called
        on this connection number
*/
	{
	__ASSERT_DEBUG(aConnection <= iNumVJSlots, User::Panic(_L("VJ Panic"), 0));
	return iRxStates[aConnection].IsValid();
	}


TBool CVJDeCompressor::DecompVJComp(RMBufChain& aPacket)
/**
Decompresses a VJ compressed packet in place.

@param aPacket MBuf chain containing packet

@return ETrue if the frame was successfully decompressed

@see DecompVJUncomp
*/
	{
	TBool		RetCode = EFalse;
	TUint8		Connection;
	TUint16		CurrentFrameLength;

	TUint8* const InitialHeaderPtr = GetVJPtr(aPacket, &CurrentFrameLength);
	TUint8 Changes	= *InitialHeaderPtr;
	TUint Offset = 1;

	if (Changes & KVjCompMaskConn)
		{
		Connection = *(InitialHeaderPtr+Offset);
		Offset++;
		if (Connection <= iNumVJSlots && CheckStoredRxHeader(Connection))
			{
			LOG(_LIT(string1,"Clear error comp");)
			LOG(Log::Write(string1);)
			ClearFlag(KVJDiscard);
			iLastRxConn = Connection;
			}
		else
			{
			//
			//  The received connection number was invalid.
			//	Ignore all further Compressed frames until an Uncompressed frame
			//	or a frame containing an explicit connection number is received
			//
			LOG(_LIT(string1,"Invalid connection");)
			LOG(Log::Write(string1);)
			SetFlag(KVJDiscard);
			return RetCode;
			}
		}
	else
		{
		if(TestFlag(KVJDiscard))
			{
			return RetCode;
			}
		Connection = (TUint8)iLastRxConn;
		}

	TRAPD(ret, DecompressFrameL(aPacket, Connection, Changes, InitialHeaderPtr, Offset, CurrentFrameLength));
	if (ret == KErrNone)
		{
		//
		//	Under all other circumstances the frame is thrown away
		//
		RetCode = ETrue;
		}
		
	return RetCode;
	}

TBool CVJDeCompressor::DecompVJUncomp(RMBufChain& aPacket)
/**
Handles a VJ uncompressed packet.
Stores the packet header in the appropriate VJ connection slot.

@param aPacket MBuf chain containing packet

@return ETrue if the frame was successfully processed

@see DecompVJComp
*/
	{
	TBool	RetCode = EFalse;

	//
	//	When we get here, the first packet is some information thing,
	//	we want the IPHeader this was the easiest way I could see to do it
	//	
	//
	ThdrIP*	IPHeader = GetIPHeader(aPacket);
	
	if (IPHeader == NULL)
	    {
		//
		//  The packet was too short.
		//	Ignore all further Compressed frames until an Uncompressed frame
		//	or a frame containing an explicit connection number is received
		//
		LOG(_LIT(string1,"Short VJ frame received");)
		LOG(Log::Write(string1);)
		SetFlag(KVJDiscard);
		
		RetCode = EFalse;
		}
    else
        {
    	TUint ConnectionNumber = IPHeader->NetGetProtocol();

    	if (ConnectionNumber <= iNumVJSlots)
    		{
    		//
    		//	Set the real protocol header to TCP
    		//
    		IPHeader->NetSetProtocol(KProtocolInetTcp);
    		
    		iLastRxConn = ConnectionNumber;
    		LOG(_LIT(string1,"Clear error uncomp");)
    		LOG(Log::Write(string1);)
    		ClearFlag(KVJDiscard);

    		//
    		//	Copy the header into one of our connection things
    		//	and zero the checksum
    		//
    		CopyRecvHeader(ConnectionNumber, IPHeader);

    		//
    		//	Receive this frame properly
    		//
    		RetCode = ETrue;
    		}
    	else
    		{
    		//
			//  The received connection number was invalid.
			//	Ignore all further Compressed frames until an Uncompressed frame
			//	or a frame containing an explicit connection number is received
    		//
    		LOG(_LIT(string1,"Invalid VJ connection received");)
    		LOG(Log::Write(string1);)
    		SetFlag(KVJDiscard);
    		
    		RetCode = EFalse;
    		}
		}

		
	return RetCode;

	}

void CVJDeCompressor::DecompressFrameL(RMBufChain& aPacket, 
								TUint8 aConnection, 
								TUint8 aChanges, 
								TUint8* const aInitialHeaderPtr,
								TUint	aOffset,
								TUint16 aCurrentFrameLength)
/**
Uncompresses a VJ compressed TCP/IP header.

@param aPacket MBuf chain containing packet to be updated
@param aConnection VJ connection number
@param aChanges The VJ change mask
@param aInitialHeaderPtr Beginning of the compressed VJ header
@param aOffset Offset from start of packet to TCP checksum field
@param aCurrentFrameLength Length of the compressed packet

@leave KErrCorrupt if aPacket is corrupt
*/							   
{
	TUint8*	VJCompHeader = aInitialHeaderPtr + aOffset;

	TUint16 Checksum = BigEndian::Get16(VJCompHeader);
	VJCompHeader+=2;

	//
	//	Use the aConnection number to retrieve the TCP Header and IP Header
	//
	ThdrIP		IPHeader;
	ThdrTCP		TCPHeader;
	GetStoredRxHeader(aConnection, &IPHeader, &TCPHeader);

	TUint16 IPHeaderLength	= (TUint16)IPHeader.NetGetHdrLen();
	TUint16 TCPHeaderLength = (TUint16)TCPHeader.NetGetHdrLen();
	TUint16 PreviousDataLength = (TUint16)(IPHeader.NetGetLength() - IPHeaderLength - TCPHeaderLength);
	
	//
	//	Set the actual checksum
	//
	TCPHeader.VJSetChecksum(Checksum);

	//
	//	Set Push if appropriate, otherwise clear it.
	//
	DecompPushFlag(aChanges, &TCPHeader);

	//
	//	Handle the Sequence, Window, Ack, Urgent compressed elements
	//
	DecompSWAU(aChanges, &VJCompHeader, &TCPHeader, PreviousDataLength);

	//
	//	Handle the compressed IP Identification field
	//
	DecompIPId(aChanges, &VJCompHeader, &IPHeader);

	TUint16 OriginalHeaderLength = (TUint16)(VJCompHeader - aInitialHeaderPtr);

	//
	// Check if the received frame is shorter than the VJ header, and discard it if
	// so.  This condition is detected by VJCompHeader being incremented beyond the end of
	// the packet during processing.
	//
	if (OriginalHeaderLength > aCurrentFrameLength)
		{
		LOG(_LIT(string1,"Short frame discarded");)
		LOG(Log::Write(string1);)
		User::Leave(KErrCorrupt);
		}

	TUint16 CurrentDataLength = (TUint16)(aCurrentFrameLength - OriginalHeaderLength);

	IPHeaderLength	= (TUint16)IPHeader.NetGetHdrLen();

	IPHeader.NetSetLength((TUint16)(CurrentDataLength + IPHeaderLength + TCPHeaderLength));

	//
	//	Generate the IP Header Checksum.
	//
	DoIPChecksum(&IPHeader, IPHeaderLength);

	//
	//	Now the hard bit; I have reconstructed the packet, now need 
	//	to get it into the right format and receive it.
	//
	CopyInNewHeaderL(&aPacket, &IPHeader, &TCPHeader, (VJCompHeader - aInitialHeaderPtr), IPHeaderLength, TCPHeaderLength);

	//TCPHeader.Printf();
	//
	//	Store the rebuilt TCP/IP header for this aConnection
	//
	//  GetIPHeader will never return NULL here because we check the
	//  packet size above.
	//
	CopyRecvHeader(aConnection, GetIPHeader(aPacket));	  
	
	//
	//	Increment the length in the Info Header
	//
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
	info->iLength += IPHeaderLength + TCPHeaderLength - OriginalHeaderLength;
}

// TODO: This is a funny place for this declaration
void ThdrTCP::Printf()
	{	
	TInt i;
	LOG(_LIT(string1,"TCP Header.");)
	LOG(Log::Write(string1);)
	LOG(_LIT(string2,"%02x ");)
	for (i=0;i<20;i++)
		{
		LOG(Log::Printf(string2,u.iData8[i]);)
		}
	}

ThdrIP* CVJDeCompressor::GetIPHeader(RMBufChain &aChain)
/**
Extracts the IP Header from the packet even though there is a buffer
of Info on the front.
This is used in VJ, to avoid the awful hack in the main receive path.
PRR 20-11-97

@param aChain MBuf chain containing packet

@return Pointer to packet data in chain, or NULL if the packet is too short
*/
	{
	RMBuf* Temp = aChain.Remove();
	TUint n = aChain.Align(KInetMaxHeaderSize);
	ThdrIP* IPHeader = (n<KIPMinHeaderSize) ? NULL : (ThdrIP*)aChain.First()->Ptr();
	aChain.Prepend(Temp);
	return IPHeader;
	}

void CVJDeCompressor::DoIPChecksum(ThdrIP* aIPHeader, TUint16 aIPHeaderLength)
/**
Calculate the IP checksum and store it in the given IP packet header.
See RFC 1071.

@param aIPHeader IP packet header
@param aIPHeaderLength Length of IP header in bytes (must be even)
*/
	{
	__ASSERT_DEBUG(!(aIPHeaderLength % 2), User::Panic(_L("VJ Panic"), 0));	// length must be even

	aIPHeader->VJSetChecksum(0);	// Clear the header checksum before calculating

	TUint16*	Ptr = (TUint16*)aIPHeader;
	TUint32		Checksum=0;
	TInt		i;
	for (i=aIPHeaderLength; i>0; i-=2)
		{
		Checksum += *Ptr++;
		}

	//
	//	Take care of the wrapping over 16 bits
	//  Doing it twice is sufficient for IP headers.
	//
	Checksum = (Checksum & 0xFFFF) + (Checksum >> 16);
	Checksum = (Checksum & 0xFFFF) + (Checksum >> 16);
	Checksum = ~Checksum;

	//
	// Do the endian conversion once, after the checksum is calculated
	//
	Checksum = BigEndian::Get16((TUint8*)&Checksum);

	aIPHeader->VJSetChecksum((TUint16)Checksum);
	}

TUint8* CVJDeCompressor::GetVJPtr(RMBufChain &aChain, TUint16* aCurrentFrameLength)
/**
Gets a pointer to the VJ Header even though there is a pseudo (Adam) buffer of Info on the front.

@param aChain MBuf chain containing packet
@param aCurrentFrameLength Returns the length of the packet

@return Pointer to beginning of packet data
*/
	{
	RMBuf* Temp = aChain.Remove();
	aChain.Align(KInetMaxHeaderSize);
	TUint8* IPPtr = aChain.First()->Ptr();

	*aCurrentFrameLength = (TUint16)aChain.Length();

	aChain.Prepend(Temp);
	return IPPtr;
	}

void CVJDeCompressor::CopyInNewHeaderL(RMBufChain* aPacket, ThdrIP* aIPHeader, ThdrTCP* aTCPHeader,
    TUint aCompressedHeaderLength, TUint16 aIPHeaderLength, TUint16 aTCPHeaderLength)
/**
Copy the uncompressed header into the proper location in the packet.

@param aPacket MBuf chain containing packet to be updated
@param aIPHeader IP packet header
@param aTCPHeader TCP header
@param aCompressedHeaderLength Length of VJ compression header
@param aIPHeaderLength Length of IP header
@param aTCPHeaderLength Length of TCP header
*/
	{
	TUint16 NewHeaderLength = (TUint16)(aIPHeaderLength + aTCPHeaderLength);
	//
	//  Take the Info thing off 
	//
	RMBuf* info = aPacket->Remove();

	//
	//	Now it is just possible that the frame contains no data
	//
	if ( aPacket->Length() == (TInt)aCompressedHeaderLength )
		{
		//
		//	Have finished with the compressed header
		//
		aPacket->TrimStart(aCompressedHeaderLength);

		aPacket->AllocL(NewHeaderLength);

		}
	else
		{
		//
		//	Have finished with the compressed header
		//
		aPacket->TrimStart(aCompressedHeaderLength);

		//
		//	Make way for the uncompressed header
		//
		aPacket->PrependL(NewHeaderLength);
		}

	TPtrC8 TempDescIP((TUint8*)aIPHeader, aIPHeaderLength);
	aPacket->CopyIn(TempDescIP, 0);

	TPtrC8 TempDescTCP((TUint8*)aTCPHeader, aTCPHeaderLength);
	aPacket->CopyIn(TempDescTCP, aIPHeaderLength);

	//
	//	Put the Info header back unchanged; it must be updated by the caller.
	//
	aPacket->Prepend(info);
	}

void CVJDeCompressor::DecompSWAU(const TUint aChanges, TUint8** aVJCompHeader, ThdrTCP* aTCPHeader, TUint16 aPreviousFrameLength)
/**
Decompresses the special case SWAU type packet.

@param aChanges The VJ change mask
@param aVJCompHeader Pointer to the pointer to the VJ compressed header
@param aTCPHeader TCP header to be updated
@param aPreviousFrameLength Length of cached packet for this VJ connection
*/
	{
	TUint32	SequenceNumber;
	TUint32	AckNumber;

	switch ( aChanges & KVjCompMaskSpecials )
		{
		case KVjCompMaskSpecialI: // Echoed data e.g. Telnet
			//
			//	The sequence and acknowledge numbers increment by the data 
			//	portion of the frame
			//

			SequenceNumber = aTCPHeader->NetGetSeqNum();
			aTCPHeader->NetSetSeqNum(SequenceNumber + aPreviousFrameLength);
			AckNumber = aTCPHeader->NetGetAckNum();
			aTCPHeader->NetSetAckNum(AckNumber + aPreviousFrameLength);
			break;

		case KVjCompMaskSpecialD: //Unidirectional Data e.g. ftp
			//
			//	The sequence numbers increment by the data portion of the frame.	
			//
			SequenceNumber = aTCPHeader->NetGetSeqNum();
			aTCPHeader->NetSetSeqNum(SequenceNumber + aPreviousFrameLength);
			break;

		default:
			//
			//	One (or more)of the SWAU bits are set and it's not a special case.
			//
			DecompUrgent(aVJCompHeader, aTCPHeader, aChanges);

			if( aChanges & KVjCompMaskWindow )
				{
				DecompWindow(aVJCompHeader, aTCPHeader);
				}

			if( aChanges & KVjCompMaskAck )
				{
				DecompAck(aVJCompHeader, aTCPHeader);
				}

			if( aChanges & KVjCompMaskSeq )
				{
				DecompSeq(aVJCompHeader, aTCPHeader);
				}

			break;
		}
	}

void CVJDeCompressor::DecompUrgent(TUint8** aVJCompHeader, ThdrTCP*aTCPHeader, TUint aChanges)
/**
Decompresses the Urgent field.

@param aVJCompHeader Pointer to the pointer to the VJ compressed header
@param aTCPHeader TCP header to be updated
@param aChanges The VJ change mask
*/
	{
	TUint Flags = aTCPHeader->VJGetFlags();

	if ( aChanges & KVjCompMaskUrgent )
		{
		Flags |= KTcpURG;
		TUint16 Delta = DecodeDelta(aVJCompHeader);
		aTCPHeader->NetSetUrgPtr(Delta);
		}
	else
		{
		Flags &= ~KTcpURG;
		}

	aTCPHeader->VJSetFlags(Flags);
	}

void CVJDeCompressor::DecompIPId(const TUint aChanges, TUint8** aVJCompHeader, ThdrIP* aIPHeader)
/**
Decompresses the IP Identification field.

@param aChanges The VJ change mask
@param aVJCompHeader Pointer to the pointer to the VJ compressed header
@param aIPHeader IP packet header to be updated
*/
	{
	TUint16 ID;
	if ( aChanges & KVjCompMaskIp )
		{
		ID = (TUint16)aIPHeader->NetGetId();
		ID = (TUint16)(ID + DecodeDelta(aVJCompHeader));
		aIPHeader->NetSetId(ID);
		}
	else
		{
		ID = (TUint16)aIPHeader->NetGetId();
		ID++;
		aIPHeader->NetSetId(ID);
		}
	}

void CVJDeCompressor::DecompSeq(TUint8** aVJCompHeader, ThdrTCP* aTCPHeader)
/**
Decompresses the Sequence Number field.

@param aVJCompHeader Pointer to the pointer to the VJ compressed header
@param aTCPHeader TCP header to be updated
*/
	{
	TUint16 Delta = DecodeDelta(aVJCompHeader);
	TUint32 SequenceNumber = aTCPHeader->NetGetSeqNum();

	SequenceNumber += Delta;

	aTCPHeader->NetSetSeqNum(SequenceNumber);
    }

void CVJDeCompressor::DecompAck(TUint8** aVJCompHeader, ThdrTCP* aTCPHeader)
/**
Decompresses the Acknowledgement Number field.

@param aVJCompHeader Pointer to the pointer to the VJ compressed header
@param aTCPHeader TCP header to be updated
*/
	{
	TUint16 Delta = DecodeDelta(aVJCompHeader);
	TUint32 AckNumber = aTCPHeader->NetGetAckNum();

	AckNumber += Delta;

	aTCPHeader->NetSetAckNum(AckNumber);
	}


void CVJDeCompressor::DecompWindow(TUint8** aVJCompHeader, ThdrTCP* aTCPHeader)
/**
Decompresses the Window field.

@param aVJCompHeader Pointer to the pointer to the VJ compressed header
@param aTCPHeader TCP header to be updated
*/
	{
	TInt16 Delta = DecodeDelta(aVJCompHeader);
	TUint16 Window = aTCPHeader->NetGetWindow();

	LOG(_LIT(string1,"Window %d");)
	LOG(Log::Printf(string1, Window);)
	LOG(_LIT(string2,"Window change %d");)
	LOG(Log::Printf(string2, Delta);)

	Window = (TUint16)(Delta + Window);
	LOG(_LIT(string3,"New Window %d");)
	LOG(Log::Printf(string3, Window);)

	aTCPHeader->NetSetWindow(Window);
	}

void CVJDeCompressor::DecompPushFlag(const TUint aChanges, ThdrTCP* aTCPHeader)
/**
Decompresses the Push flag.

@param aChanges The VJ change mask
@param aTCPHeader TCP header to be updated
*/
	{
	TUint Flags = aTCPHeader->VJGetFlags();

	if ( aChanges & KVjCompMaskPush )
		{
		Flags |= KTcpPSH;
		}
	else
		{
		Flags &= ~KTcpPSH;
		}

	aTCPHeader->VJSetFlags(Flags);
    }

TUint16	CVJDeCompressor::DecodeDelta( TUint8** aVJCompHeader )
/**
Decodes a compressed delta value.

@param aVJCompHeader Pointer to pointer into VJ header buffer holding encoded value;
returns with pointer incremented one past end of value

@return Decoded value
*/
	{
	TUint16 Value = (TUint16) **aVJCompHeader;
	(*aVJCompHeader)++;
	
	if (Value == 0)
		{
		//
		//	Zero is an extension; the next two bytes give the 16 bit value
		//
		Value = BigEndian::Get16(*aVJCompHeader);
		*aVJCompHeader += 2;
		}
	return Value;
	}

