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
#include "vjlog.h"
#include "vj.h"

CVJDeCompressor::CVJDeCompressor(TUint aSlots)
	{
	// Remove Warning

	aSlots = aSlots;
	return;
	}

void CVJDeCompressor::ConstructL( CVJCompFactory* aFactory, TUint aMaxSlot)
	{
		
	//
	// Note that aMaxSlot is the index of the maximum slot that will be accessed.
	//

	iNumVJSlots = aMaxSlot;
	iRxStates = new (ELeave) TVJCompHdr[aMaxSlot+1];
	iFactory = aFactory;
	iFactory->Open();
	return;
	}

CVJDeCompressor::~CVJDeCompressor()
	{
	delete []iRxStates;
	if (iFactory)
		iFactory->Close();
	}

void CVJDeCompressor::CRCError()
	{
	LOG(_LIT(string1,"CRC Error");)
	LOG(Log::Write(string1);)
	SetFlag(KVJDiscard);
	}

void CVJDeCompressor::CopyRecvHeader(const TUint aConnectionNumber, ThdrIP * aHeader)
	{
	TUint RemoveWarning;

	RemoveWarning = aConnectionNumber;
	/*
	*	The whole point of this protocol thing
	*	Copy a TCP/IP Header to be used as a refence to
	*	reconstruct compressed headers.
	*	Also zero the Checksum.
	*/

	__ASSERT_DEBUG(aConnectionNumber <= iNumVJSlots, User::Panic(_L("VJ Panic"), 0));
	iRxStates[RemoveWarning].StoreTCPIPHeader(aHeader);
	return;
	}

void CVJDeCompressor::GetStoredRxHeader(const TUint aConnectionNumber, ThdrIP * aIPHeader, ThdrTCP * aTCPHeader)
	{
	iRxStates[aConnectionNumber].RetrieveTCPIPHeader(aIPHeader, aTCPHeader);
	}


TBool CVJDeCompressor::DecompVJComp(RMBufChain& aPacket)
	{
	TBool		RetCode = FALSE;
	TUint8*		InitialHeaderPtr;
	TUint		Offset;
	TUint8		Changes;
	TUint8		Connection;
	TUint16		CurrentFrameLength;

	InitialHeaderPtr = GetVJPtr(aPacket, &CurrentFrameLength);
	Changes	= *InitialHeaderPtr;
	Offset = 1;

	if (Changes & KVjCompMaskConn )
		{
		Connection = *(InitialHeaderPtr+Offset);
		Offset ++;
		if (Connection > iNumVJSlots)
			{
			//
			//	Toss away all Compressed frames until an Uncompressed or a C frame
			//	are received
			//
			SetFlag(KVJDiscard);
			return RetCode;
			}
		else
			{
			LOG(_LIT(string1,"Clear error Comp");)
			LOG(Log::Write(string1);)
			ClearFlag(KVJDiscard);
			iLastRxConn = Connection;
			}
		}
	else
		{
		if(TestFlag(KVJDiscard))
			{
			return RetCode;
			}
		else
			{
			Connection = (TUint8)iLastRxConn;
			}
		}

	TRAPD(ret, DecompressFrameL(aPacket, Connection, Changes, InitialHeaderPtr, Offset, CurrentFrameLength));
	if (ret == KErrNone)
		{
		//
		//	Under all other circumstances the frame is thrown away
		//
		RetCode = TRUE;
		}
		
	return RetCode;
	}

TBool CVJDeCompressor::DecompVJUncomp(RMBufChain& aPacket)
	{
	TBool	RetCode = FALSE;
	ThdrIP*	IPHeader;
	TUint	ConnectionNumber;;

	
	//
	//	When we get here, the first packet is some information thing,
	//	we want the IPHeader this was the easiest way I couild see to do it
	//	
	//
		
	IPHeader = GetIPHeader(aPacket);

	ConnectionNumber = IPHeader->NetGetProtocol();

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
		RetCode = TRUE;
		}
	else
		{
		//
		//	Toss away all Compressed frames 
		//	until an Uncompressed or a C frame
		//	are received
		//
//		LOG(_LIT(string1,"UNCOMPRESSED VJ FRAME NOT Received");)
		SetFlag(KVJDiscard);
		
		RetCode = FALSE;
		}

		
	return RetCode;

	}

void CVJDeCompressor::DecompressFrameL(RMBufChain& aPacket, 
								TUint8 aConnection, 
								TUint8 aChanges, 
								TUint8* aInitialHeaderPtr,
								TUint	aOffset,
								TUint16 aCurrentFrameLength)
{
	TUint16		Checksum;
	ThdrIP		IPHeader;
	ThdrTCP		TCPHeader;
	TUint16		IPHeaderLength;
	TUint16		TCPHeaderLength;
	TUint16		CurrentDataLength;
	TUint16		PreviousDataLength;
	TUint16		OriginalHeaderLength;
	TUint8*		VJCompHeader;

	VJCompHeader = aInitialHeaderPtr + aOffset;

	Checksum = BigEndian::Get16(VJCompHeader);
	VJCompHeader+=2;

	//
	//	Use the aConnection number to retrieve the TCP Header and IP Header
	//
	GetStoredRxHeader(aConnection, &IPHeader, &TCPHeader);

	IPHeaderLength	= (TUint16)IPHeader.NetGetHdrLen();
	TCPHeaderLength = (TUint16)TCPHeader.NetGetHdrLen();
	PreviousDataLength = (TUint16)(IPHeader.NetGetLength() - IPHeaderLength - TCPHeaderLength);

	//
	//	Set the actual checksum
	//
	TCPHeader.VJSetChecksum(Checksum);

	//
	//	Set Push if appropriate, otherwise clear it.
	//
	DecompPushFlag(aChanges, &TCPHeader);

	//
	//	Handle the	Sequence Window	Ack Urgent compressed elements
	//
	DecompSWAU(aChanges, &VJCompHeader, &TCPHeader, PreviousDataLength);

	//
	//	Handle the compressed IP Identification field
	//
	DecompIPId(aChanges, &VJCompHeader, &IPHeader);

	OriginalHeaderLength = (TUint16)(VJCompHeader - aInitialHeaderPtr);

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

	CurrentDataLength = (TUint16)(aCurrentFrameLength - OriginalHeaderLength);

	IPHeaderLength	= (TUint16)IPHeader.NetGetHdrLen();

	IPHeader.NetSetLength((TUint16)(CurrentDataLength + IPHeaderLength + TCPHeaderLength));

	//
	//	Generate the IP Header Checksum.
	//
	DoIPChecksum(&IPHeader, IPHeaderLength);

	//
	//	Now the hard bit, I have reconstructed the packet now need 
	//	to get it into the right format and receive it.
	//
	CopyInNewHeaderL(&aPacket, &IPHeader, &TCPHeader, (VJCompHeader - aInitialHeaderPtr), IPHeaderLength, TCPHeaderLength);

	//TCPHeader.Printf();
	//
	//	Store the rebuilt TCP/IP header for this aConnection
	//
	CopyRecvHeader(aConnection, GetIPHeader(aPacket));	  

	//
	//	Hmmmmmmmmmm I don't like this, but find a simpler way to do it?? PRR
	//	Increment the length in teh Info Header
	//
	((RMBufRecvInfo*)aPacket.First()->Ptr())->iLength += IPHeaderLength + TCPHeaderLength - OriginalHeaderLength;


	return;

}

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


ThdrIP * CVJDeCompressor::GetIPHeader(RMBufChain &aChain)
//
// Get the IP Header even though there is a buffer of Info on the front.
// This is used in VJ, to avoid the awful hack in the main receive path.
// PRR 20-11-97
//
	{

	RMBuf * Temp = aChain.Remove();
	TUint n = aChain.Align(KInetMaxHeaderSize);
	ThdrIP * IPHeader = (n<KIPMinHeaderSize) ? NULL : (ThdrIP*)aChain.First()->Ptr();
	aChain.Prepend(Temp);
	return IPHeader;
	}

void CVJDeCompressor::DoIPChecksum(ThdrIP * aIPHeader, TUint16 aIPHeaderLength)
	{
	TUint		Checksum=0;
	TUint8 *	Ptr;
	TUint		i;

	Ptr = (TUint8*)aIPHeader;

	aIPHeader->VJSetChecksum(0);

	for (i=0;i<aIPHeaderLength;i+=2)
		{
		Checksum += BigEndian::Get16(Ptr);
		Ptr+=2;
		}

	//
	//	Take care of the wrapping over 16 bits
	//
	Checksum = (Checksum & 0xFFFF) + (Checksum >> 16);
	Checksum = (Checksum & 0xFFFF) + (Checksum >> 16);
	Checksum = ~Checksum;

	aIPHeader->VJSetChecksum((TUint16)Checksum);

	return;
	}

TUint8 * CVJDeCompressor::GetVJPtr(RMBufChain &aChain, TUint16 * aCurrentFrameLength)
//
// Get a pointer to the VJ Header even though there is a pseudo (Adam) buffer of Info on the front.
//
	{

	RMBuf * Temp = aChain.Remove();
	TUint n = aChain.Align(KInetMaxHeaderSize);
	TUint8 * IPPtr = aChain.First()->Ptr();

	// Remove warning
	n = n;

	*aCurrentFrameLength = (TUint16)aChain.Length();

	aChain.Prepend(Temp);
	return IPPtr;
	}

void CVJDeCompressor::CopyInNewHeaderL(RMBufChain * aPacket, ThdrIP * aIPHeader, ThdrTCP * aTCPHeader, TUint aCompressedHeaderLength, TUint16 aIPHeaderLength, TUint16 aTCPHeaderLength)
	{
	TUint16 NewHeaderLength;

	NewHeaderLength = (TUint16)(aIPHeaderLength + aTCPHeaderLength);
	//
	//  Take the Info thing off 
	//
	RMBuf* Temp = aPacket->Remove();

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

		{
		TPtrC8 TempDesc = TPtrC8((TUint8*)aIPHeader, aIPHeaderLength);
		aPacket->CopyIn(TempDesc, 0);
		}
	
		{
		TPtrC8 TempDesc = TPtrC8((TUint8*)aTCPHeader, aTCPHeaderLength);
		aPacket->CopyIn(TempDesc, aIPHeaderLength);
		}
		//
		//	Put the Info thing back, do I need to 
		//	modify this???
		//
		aPacket->Prepend(Temp);
	
	return;
	}

void CVJDeCompressor::DecompSWAU(const TUint aChanges, TUint8** aVJCompHeader, ThdrTCP * aTCPHeader, TUint16 aPreviousFrameLength)
	{
	TUint32	SequenceNumber;
	TUint32	AckNumber;

	switch ( aChanges & KVjCompMaskSpecials )
		{
		case KVjCompMaskSpecialI: // Echoed data e.g. Telnet
			{
			//
			//	The sequence and acknowledge numbers increment by the data 
			//	portion of the frame
			//

			SequenceNumber = aTCPHeader->NetGetSeqNum();
			aTCPHeader->NetSetSeqNum(SequenceNumber + aPreviousFrameLength);
			AckNumber = aTCPHeader->NetGetAckNum();
			aTCPHeader->NetSetAckNum(AckNumber + aPreviousFrameLength);
			break;
			}
		case KVjCompMaskSpecialD: //Unidirectional Data e.g. ftp
			{
			//
			//	The sequence numbers increment by the data portion of the frame.	
			//
			SequenceNumber = aTCPHeader->NetGetSeqNum();
			aTCPHeader->NetSetSeqNum(SequenceNumber + aPreviousFrameLength);
			break;
			}
		default:
			{
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

	return;

	}

//
//	Decompress the Urgent field
//
void CVJDeCompressor::DecompUrgent(TUint8 ** aVJCompHeader, ThdrTCP *aTCPHeader, TUint aChanges)
	{
	TUint16 Delta;
	TUint Flags;

	Flags = aTCPHeader->VJGetFlags();

	if ( aChanges & KVjCompMaskUrgent )
		{
		Flags |= KTcpURG;
		Delta = DecodeDelta(aVJCompHeader);
		aTCPHeader->NetSetUrgPtr(Delta);
		}
	else
		{
		Flags &= ~KTcpURG;
		}

	aTCPHeader->VJSetFlags(Flags);

	return;
	}

void CVJDeCompressor::DecompIPId(const TUint aChanges, TUint8** aVJCompHeader, ThdrIP * aIPHeader)
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

	return;
	}

void CVJDeCompressor::DecompSeq(TUint8 ** aVJCompHeader, ThdrTCP *aTCPHeader)
	{
	TUint16 Delta;
	TUint32 SequenceNumber;

	Delta = DecodeDelta(aVJCompHeader);

	SequenceNumber = aTCPHeader->NetGetSeqNum();

	SequenceNumber += Delta;

	aTCPHeader->NetSetSeqNum(SequenceNumber);

	return;
	}

void CVJDeCompressor::DecompAck(TUint8 ** aVJCompHeader, ThdrTCP *aTCPHeader)
	{
	TUint16 Delta;
	TUint32 AckNumber;

	Delta = DecodeDelta(aVJCompHeader);

	AckNumber = aTCPHeader->NetGetAckNum();

	AckNumber += Delta;

	aTCPHeader->NetSetAckNum(AckNumber);

	return;
	}


void CVJDeCompressor::DecompWindow(TUint8 ** aVJCompHeader, ThdrTCP *aTCPHeader)
	{
	TInt16 Delta;
	TUint16 Window;

	Delta = DecodeDelta(aVJCompHeader);

	Window = aTCPHeader->NetGetWindow();

	LOG(_LIT(string1,"Window %d");)
	LOG(Log::Printf(string1, Window);)
	LOG(_LIT(string2,"Window change %d");)
	LOG(Log::Printf(string2, Delta);)

	Window = (TUint16)(Delta + Window);
	LOG(_LIT(string3,"New Window %d");)
	LOG(Log::Printf(string3, Window);)

	aTCPHeader->NetSetWindow(Window);

	return;
	}

void CVJDeCompressor::DecompPushFlag(const TUint aChanges, ThdrTCP * aTcpHeader)
	{
	TUint Flags;
	Flags = aTcpHeader->VJGetFlags();

	if ( aChanges & KVjCompMaskPush )
		{
		Flags |= KTcpPSH;
		}
	else
		{
		Flags &= ~KTcpPSH;
		}

	aTcpHeader->VJSetFlags(Flags);

	return;
}

TUint16	CVJDeCompressor::DecodeDelta( TUint8 ** aVJCompHeader )
	{
	TUint8 Value = **aVJCompHeader;
	(*aVJCompHeader)++;
	
	if (Value == 0)
		{
		//
		//	Zero is an extension the next two bytes give the 16 bit value
		//
		TUint16 Value16Bit = BigEndian::Get16(*aVJCompHeader);
		*aVJCompHeader += 2;
		return Value16Bit;
		}
	else
		{
		return (TUint16)Value;
		}
	}


