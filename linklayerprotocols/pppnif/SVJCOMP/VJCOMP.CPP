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

#include "VJ.H"
#include "VJLOG.H"
#include <networking/in_std.h>
#include <in_sock.h>

CVJCompressor::CVJCompressor()
	{
	}

void CVJCompressor::ConstructL(CVJCompFactory* aFactory, TUint aMaxSlot, TBool aCompressConnId)
/**
Construct the CVJCompressor object.

@param aFactory Factory that created this object
@param aMaxSlot Value of the highest VJ connection number to be sent
@param aCompressConnId Whether to compress the connection number 
*/
	{
	__ASSERT_DEBUG(aMaxSlot >= KMinVjSlot && aMaxSlot <= KMaxVjSlot, User::Panic(_L("VJ Panic"), 0));
	iMaxVJSlots = aMaxSlot+1;
	iLastTxConn = aMaxSlot+1; // Initialize with an invalid slot number

   	iTxStates = new (ELeave) TVJCompHdr[iMaxVJSlots];

	iTxStates[0].SetNextPtr(&iTxStates[aMaxSlot]);
	iTxStates[0].SetConnectionNumber(0);

	iLastTxHdr = &iTxStates[0];

	TUint i;
	for (i=aMaxSlot; i>0; i--)
		{
		iTxStates[i].SetNextPtr(&iTxStates[i-1]);
		iTxStates[i].SetConnectionNumber(i);
		}

	iCompressConnId = aCompressConnId;
	iFactory = aFactory;
	iFactory->Open();
	}

CVJCompressor::~CVJCompressor()
	{
	delete [] iTxStates;

	if (iFactory)
	    {
		iFactory->Close();
		}
	}

ThdrIP* CVJCompressor::GetIPHeader(RMBufChain &aChain)
/**
Get the IP Header even though there is a buffer of Info on the front.
This is used in VJ, to avoid the awful hack in the main receive path.
PRR 20-11-97

@param aChain MBuf chain containing packet

@return IP header
*/
	{
	RMBuf* Temp = aChain.Remove();
	TUint n = aChain.Align(KInetMaxHeaderSize);
	ThdrIP* IPHeader = (n<KIPMinHeaderSize) ? NULL : (ThdrIP*)aChain.First()->Ptr();
	aChain.Prepend(Temp);
	return IPHeader;
	}

void CVJCompressor::EncodeDelta(TUint8** aVJCompHeader, TInt16 aValue)
/**
Encodes a delta value in VJ compressed format.

@param aVJCompHeader Pointer to pointer into VJ header buffer to store encoded value;
  returns with pointer incremented one past end of value
@param aValue Value to store
*/
	{
	if ((TUint16)aValue >= 256 || aValue == 0)
		{
		*(*aVJCompHeader) = 0;
		(*aVJCompHeader)++;

		BigEndian::Put16(*aVJCompHeader, aValue);
		(*aVJCompHeader) += 2;
		}
	else
		{
		*(*aVJCompHeader) = (TUint8)aValue;
		(*aVJCompHeader)++;
		}
	}

TBool CVJCompressor::SendAsRawIP(	ThdrIP* aIPHeader, 
								    ThdrTCP* aTCPHeader )
/**
Determines if the packet must bypass VJ compression altogether.
This happens with fragmented IP packets and certain TCP flags.

@param aIPHeader First IP packet header
@param aTCPHeader TCP header

@return Whether the packet must be sent raw
*/
	{
	TBool RetCode = ETrue;

	if (!aIPHeader->NetGetFragment())
		{
		//
		//	Frame isn't fragmented, so make sure that no flags are set
		//	which would prevent us from compressing the packet.
		//
		TUint Flags = aTCPHeader->VJGetFlags();
		RetCode = (Flags & (KTcpFIN | KTcpSYN | KTcpRST | KTcpACK)) != KTcpACK;
		}
	
	return RetCode;
	}


TBool CVJCompressor::SuitableForVJCompression(ThdrIP* aIPHeader, 
										  ThdrTCP* aTCPHeader, 
										  TUint* aConnection,
										  ThdrIP* aRetrievedIPHdr,
										  ThdrTCP* aRetrievedTCPHdr)
/**
Sees if a packet header is stored in the VJ connection cache.

@param aIPHeader IP packet header
@param aTCPHeader TCP header
@param aConnection Returns the matching VJ connection number
@param aRetrievedIPHdr Returns the cached IP packet header
@param aRetrievedTCPHdr Returns the cached TCP packet header

@return Whether the packet matches one in the cache
*/
	{
	TBool RetCode = EFalse;
	if (GetStoredTxHeader(	aConnection, 
							aIPHeader, 
							aTCPHeader, 
							aRetrievedIPHdr, 
							aRetrievedTCPHdr))
		{
		//
		//	OK we found a stored header with the right 
		//	addresses; is the header compressible?
		//
		RetCode = IsIPCompressible(aIPHeader, aRetrievedIPHdr) &&
		          IsTCPCompressible(aTCPHeader, aRetrievedTCPHdr);
		}

	return RetCode;
	}

TBool CVJCompressor::IsIPCompressible(ThdrIP* aIPHeader, ThdrIP* aRetrievedHdr)
/**
Compares two IP headers to see if the seldom-changing fields match.

@param aIPHeader First IP packet header
@param aRetrievedHdr Second IP packet header

@return Whether the headers match
*/
	{
	TBool	RetCode = EFalse;

	//
	//	Ensure that the following match
	//			Word 0
	//					Protocol Version,
	//					Header Length,
	//					Type Of Service,
	//			Word 3	
	//					Fragment stuff
	//			Word 4
	//					Time to live,
	//					Protocol,
	//			If appropriate 
	//					IP Options 
	//
	if ((aIPHeader->Word0() == aRetrievedHdr->Word0()) &&
		(aIPHeader->Word3() == aRetrievedHdr->Word3()) &&
		(aIPHeader->Word4() == aRetrievedHdr->Word4()))
		{
		//
		//	Well those are fine now check the IP Options if there are any
		//
		const TUint Length = aIPHeader->NetGetHdrLen() - KIPMinHeaderSize;
		TUint8*	Options = aIPHeader->GetOptions();

		const TUint StoredLength = aRetrievedHdr->NetGetHdrLen() - KIPMinHeaderSize;
		TUint8*	StoredOptions = aRetrievedHdr->GetOptions();
		
		if (Length == StoredLength)
			{
			if (Length == 0 || !Mem::Compare(Options, Length, StoredOptions, StoredLength))
				{
				RetCode = ETrue;
				}
			}
		}
		return RetCode;
	}

TBool CVJCompressor::IsTCPCompressible(ThdrTCP* aHeader, ThdrTCP* aRetrievedHdr)
/**
Compares two TCP headers to see if the options, ECN & reserved bits match.
Everything else in the TCP header is handled specially by the
VJ compression algorithm.

@param aHeader First TCP header
@param aRetrievedHdr Second TCP header

@return Whether the headers match
*/
	{
	TBool   RetCode = EFalse;

    //
    //  Ensure that the ECN and TCP reserved bits match
    //
    if ((aHeader->VJGetReserved() == aRetrievedHdr->VJGetReserved()) &&
       ((aHeader->VJGetFlags() & KTcpECN) == (aRetrievedHdr->VJGetFlags() & KTcpECN)) &&
   	//
	//	Ensure that the header lengths match, if they do then 
	//	check that the options match
	//
	   (aHeader->NetGetHdrLen() == aRetrievedHdr->NetGetHdrLen()))
		{
		//
		//	Now compare the TCP Options 
		//
		const TUint Length = aHeader->NetGetHdrLen() - KTCPHeaderSize;
		TUint8* Options = GetTCPOpts(aHeader);

		const TUint StoredLength = aRetrievedHdr->NetGetHdrLen() - KTCPHeaderSize;
		TUint8* StoredOptions = GetTCPOpts(aRetrievedHdr);

		if (Length == StoredLength)
			{
			// This comparison will inevitably fail if a RFC 1323
			// TCP timestamp option is found, causing VJ compression to be
			// effectively disabled.
			if (StoredLength == 0 || !Mem::Compare(Options, Length, StoredOptions, StoredLength))
				{
				RetCode = ETrue;
				}
			}
		}
	return RetCode;
	}

TBool CVJCompressor::CompressUrgentPtr(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr)
/**
Compresses the TCP Urgent pointer field.

@param aVJPtr Pointer to the pointer within the VJ header to be modified;
returns pointing one past the last header location used
@param aChanges The VJ change mask to be updated
@param aTCPHeader TCP header
@param aRetrievedTCPHdr Cached TCP header from which to make a delta for aTCPHeader

@return ETrue if the field was compressed
*/
	{
	TBool   RetCode = ETrue;
	TUint   Flags = aTCPHeader->VJGetFlags();
	if (Flags & KTcpURG)
		{
		(*aChanges) |= KVjCompMaskUrgent;
		TUint16 UrgPtr = aTCPHeader->NetGetUrgPtr();
		EncodeDelta(aVJPtr, UrgPtr);
		}
	else if (aTCPHeader->NetGetUrgPtr() != aRetrievedTCPHdr->NetGetUrgPtr())
		{
		//
		//	They can change the Urgent Ptr without setting the flag but 
		//	it's not recommended, just send uncompressed frame
		//
		RetCode = EFalse;
		}
	return RetCode;
	}

TBool CVJCompressor::CompressWindow(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr)
/**
Compresses the TCP Window field.

@param aVJPtr Pointer to the pointer within the VJ header to be modified;
returns pointing one past the last header location used
@param aChanges The VJ change mask to be updated
@param aTCPHeader TCP header
@param aRetrievedTCPHdr Cached TCP header from which to make a delta for aTCPHeader

@return ETrue if the field was compressed
*/
	{
	TBool	RetCode = ETrue;

	TInt16 DeltaWindow = (TUint16)(aTCPHeader->NetGetWindow() - aRetrievedTCPHdr->NetGetWindow());
	if (DeltaWindow)
		{
		LOG(_LIT(string1,"\nWindow Delta");)
		LOG(Log::Write(string1);)
		LOG(_LIT(string2,"%d\n");)
		LOG(Log::Printf(string2,DeltaWindow);)
		*aChanges |= KVjCompMaskWindow;
		EncodeDelta(aVJPtr, DeltaWindow);
		}

	return RetCode;
	}

TBool CVJCompressor::CompressAck(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr)
/**
Compresses the TCP Acknowledgement Number field.

@param aVJPtr Pointer to the pointer within the VJ header to be modified;
returns pointing one past the last header location used
@param aChanges The VJ change mask to be updated
@param aTCPHeader TCP header
@param aRetrievedTCPHdr Cached TCP header from which to make a delta for aTCPHeader

@return ETrue if the field was compressed
*/
	{
	TBool	RetCode = ETrue;

	TInt32 DeltaAck = aTCPHeader->NetGetAckNum() - aRetrievedTCPHdr->NetGetAckNum();
	if (DeltaAck)
		{
		if (IsDeltaCompressible(DeltaAck))
			{
			EncodeDelta(aVJPtr, (TInt16)DeltaAck);
			*aChanges |= KVjCompMaskAck;
			}
		else
			{
			// Can't compress this large a difference
			RetCode = EFalse;
			}
		}
		return RetCode;
	}


TBool CVJCompressor::CompressSeq(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr)
/**
Compresses the TCP Sequence Number field.

@param aVJPtr Pointer to the pointer within the VJ header to be modified;
returns pointing one past the last header location used
@param aChanges The VJ change mask to be updated
@param aTCPHeader TCP header
@param aRetrievedTCPHdr Cached TCP header from which to make a delta for aTCPHeader

@return ETrue if the field was compressed
*/
	{
	TBool	RetCode = ETrue;

	TInt32 DeltaSeq = aTCPHeader->NetGetSeqNum() - aRetrievedTCPHdr->NetGetSeqNum();
	if (DeltaSeq)
		{
		if (IsDeltaCompressible(DeltaSeq))
			{
			EncodeDelta(aVJPtr, (TInt16)(DeltaSeq));
			*aChanges |= KVjCompMaskSeq;
			}
		else
			{
			// Can't compress this large a difference
			RetCode = EFalse;
			}
		}
	return RetCode;
	}

TBool CVJCompressor::CompressSpecialCases(TUint8** aVJPtr, 
						   TUint8* const aVJInitialDeltaPtr, 
						   TUint8* aChanges, 
						   ThdrTCP* aTCPHeader, 
						   ThdrTCP* aRetrievedTCPHdr, 
						   ThdrIP*	aIPHeader,
						   ThdrIP* aRetrievedIPHdr)
/**
Compresses special case TCP/IP packets.
See RFC 1144 sec. 3.2.3

@pre aChanges must reflect the compressed header except for the
KVjCompMaskPush and KVjCompMaskIp bits which must be clear.

@param aVJPtr Pointer to the pointer within the VJ header to be modified;
returns pointing one past the last header location used
@param aVJInitialDeltaPtr Beginning of the compressed VJ header
@param aChanges The VJ change mask to be updated
@param aTCPHeader TCP header
@param aRetrievedTCPHdr Cached TCP header from which to make a delta for aTCPHeader
@param aIPHeader IP header
@param aRetrievedIPHdr Cached IP header from which to make a delta for aIPHeader

@return ETrue if the packet can be compressed
*/
	{
	
	__ASSERT_DEBUG(!(*aChanges & (KVjCompMaskIp | KVjCompMaskPush)), User::Panic(_L("VJ Panic"), 0));
	TBool RetCode = ETrue;

	TUint DeltaSeq = aTCPHeader->NetGetSeqNum() - aRetrievedTCPHdr->NetGetSeqNum();
	TUint DeltaAck = aTCPHeader->NetGetAckNum() - aRetrievedTCPHdr->NetGetAckNum();

	switch (*aChanges)
		{
		case 0:
		    // No change from the last packet; this frame may be a retransmission.
		    // See RFC 1144 §4.2
			if (((aIPHeader->NetGetLength() - aIPHeader->NetGetHdrLen()) == aTCPHeader->NetGetHdrLen()) ||
			    ((aRetrievedIPHdr->NetGetLength() - aRetrievedIPHdr->NetGetHdrLen()) != aRetrievedTCPHdr->NetGetHdrLen()))
				{
				//
				//	The frame has been sent compressed and was probably missed at the other
				//	end, so send it uncompressed.
				//
				LOG(_LIT(string1,"Missed frame");)
				LOG(Log::Write(string1);)
				RetCode = EFalse;				
				}
			break;

		case KVjCompMaskSpecialD:
		case KVjCompMaskSpecialI:
			// Can't send SWU and SWAU, so send Uncompressed
			RetCode = EFalse;
			break;

		case KVjCompMaskSeq | KVjCompMaskAck:
			if (DeltaAck == DeltaSeq)
				{
				if (DeltaSeq == (aRetrievedIPHdr->NetGetLength() - aRetrievedIPHdr->NetGetHdrLen() - aRetrievedTCPHdr->NetGetHdrLen()))
					{
					// Terminal traffic
					*aChanges = KVjCompMaskSpecialI;
					*aVJPtr = aVJInitialDeltaPtr; // Reset back to just after CSum
					}
				}
			break;

		case KVjCompMaskSeq:
			if (DeltaSeq == (aRetrievedIPHdr->NetGetLength() - aRetrievedIPHdr->NetGetHdrLen() - aRetrievedTCPHdr->NetGetHdrLen()))
				{
				// Data Xfer e.g. FTP
				*aChanges = KVjCompMaskSpecialD;
				*aVJPtr = aVJInitialDeltaPtr; // Reset back to just after CSum
				}
			break;

	    default:
	        // Not a special case
	        break;
		}

	return RetCode;
	}

TBool CVJCompressor::CompressIPId(TUint8** aVJPtr, TUint8* aChanges, ThdrIP* aIPHeader, ThdrIP* aRetrievedIPHdr)
/**
Compresses the IP Packet ID field.

@param aVJPtr Pointer to the pointer within the VJ header to be modified;
returns pointing one past the last header location used
@param aChanges The VJ change mask to be updated
@param aIPHeader IP header
@param aRetrievedIPHdr Cached IP header from which to make a delta for aIPHeader

@return ETrue
*/
	{
	TInt16 DeltaIPId = (TInt16) ((TInt)aIPHeader->NetGetId() - (TInt)aRetrievedIPHdr->NetGetId());
	if (DeltaIPId != 1)
		{
		EncodeDelta(aVJPtr, DeltaIPId);
		*aChanges |= KVjCompMaskIp;
		}

	return ETrue;
	}

TBool CVJCompressor::CompressPush(TUint8* aChanges,  ThdrTCP* aTCPHeader)
/**
Compresses the TCP Push flag.

@param aChanges The VJ change mask to be updated
@param aTCPHeader TCP header

@return ETrue
*/
	{
	TUint Flags = aTCPHeader->VJGetFlags();
	if (Flags & KTcpPSH )
		{
		*aChanges |= KVjCompMaskPush;
		}

	return ETrue;
	}

TBool CVJCompressor::IsSameConnAsLast(TUint* aCompressedHdrLen, TUint aConnection)
/**
Determines whether the given VJ connection number is the same as that of the
last compressed packet sent and returns the size of the compressed header.

@param aCompressedHdrLen Returns the length of the compressed header
@param aConnection VJ connection number

@return Whether this is the same connection number as the last one
*/
	{
	TBool RetCode;

	if ((aConnection != iLastTxConn) || (iCompressConnId == EFalse))
		{
		RetCode = EFalse;
		*aCompressedHdrLen = 4; // Changes, Connection, CSum
		}
	else
		{
		RetCode = ETrue;
		*aCompressedHdrLen = 3; // Changes, CSum
		}
	return RetCode;
	}


void CVJCompressor::SetFirstFewBytes(TUint8* aChanges, TBool aNewConnection, TUint8** aVJHeader, ThdrTCP* aTCPHeader, TUint aConnection)
/**
Sets the change mask, connection number (if necessary) and TCP checksum bytes
in the VJ compressed header.

@param aChanges The VJ change mask to be copied and potentially modified
@param aNewConnection Whether this is a new connection, requiring the number in the header
@param aVJHeader Pointer to the pointer to the VJ header to be modified
@param aTCPHeader TCP header
@param aConnection VJ connection number
*/
	{
	TUint8* ChecksumPtr;

	//
	// Be warned this value is NOT stored in native byte order;
	// the sixteen bit value is just loaded from the frame -- it's NOT
	// byte swapped 
	//
	TUint16 Checksum = (TUint16)aTCPHeader->NetGetChecksum();
	LOG(_LIT(logString1,"Checksum is %x");)
	LOG(Log::Printf(logString1,Checksum);)

	if (aNewConnection)
		{
		(*aVJHeader)[1] = (TUint8)aConnection;
		ChecksumPtr = (*aVJHeader)+2;
		LittleEndian::Put16(ChecksumPtr,Checksum);
		*aChanges |= KVjCompMaskConn;
		}
	else
		{
		ChecksumPtr = (*aVJHeader)+1;
		LittleEndian::Put16(ChecksumPtr,Checksum);
		}

	*(*aVJHeader) = *aChanges;
	}

void CVJCompressor::CopyInNewTxHeader(RMBufChain& aPacket, TUint8* aCompressedHdr, TUint aOldHeaderLength, TUint aNewHeaderLength )
/**
Copies the compressed header into the packet.

@param aPacket MBuf chain containing packet
@param aCompressedHdr VJ compressed header
@param aOldHeaderLength Length of the current TCP/IP header in aPacket
@param aNewHeaderLength Length of VJ compressed header
*/
	{
	//
	// Remove the first info buffer thing
	//
	RMBuf* Temp = aPacket.Remove();

	//
	//	Trim the Packet length to take into account the new header
	//
	aPacket.TrimStart(aOldHeaderLength-aNewHeaderLength);
	TPtrC8 TempDesc(aCompressedHdr, aNewHeaderLength);
	aPacket.CopyIn(TempDesc, 0);
	
	aPacket.Prepend(Temp);
	}

void CVJCompressor::DecrementPacketLen( RMBufChain& aPacket, TUint SizeDecrease )
/**
There is a header prepended to the frame, one element is the length 
which has now changed, so change it

@param aPacket MBuf chain containing packet
@param SizeDecrease Amount by which to reduce the length metadata
*/
	{
	RMBufPktInfo* info = RMBufPacketBase::PeekInfoInChain(aPacket);
	info->iLength -= SizeDecrease;
	}

TBool CVJCompressor::CompressFrame(RMBufChain& aPacket, 
							   ThdrIP* aIPHeader, 
							   ThdrTCP* aTCPHeader, 
							   TUint aConnection,
							   ThdrIP* aRetrievedIPHdr,
							   ThdrTCP* aRetrievedTCPHdr)
/**
Compresses a TCP/IP header if possible.

@param aPacket MBuf chain containing packet
@param aIPHeader IP header
@param aTCPHeader TCP header
@param aConnection VJ connection number
@param aRetrievedIPHdr Cached IP packet header
@param aRetrievedTCPHdr Cached TCP packet header

@return Whether the header was compressed
*/							   
	{
	TBool	RetCode=EFalse;
	TUint8	Changes=0;
	TUint8	VJHeader[16];	// The VJ Header without changes, checksum and connection Number
	TUint CompressedHdrLen;
	
	//
	//	Is Connection ID Compression turned on?
	//
	TBool NewConnection = !IsSameConnAsLast(&CompressedHdrLen, aConnection);

	TUint8*	VJPtr = VJHeader + CompressedHdrLen;	
	TUint8*	StartVJDeltaPtr = VJPtr;

	if (!CompressUrgentPtr(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
	{
		return RetCode;
    }
    
	if (!CompressWindow(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
	{
		return RetCode;
    }

	if (!CompressAck(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
	{
		return RetCode;
    }

	if (!CompressSeq(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
	{
		return RetCode;
    }

	if (!CompressSpecialCases(&VJPtr, StartVJDeltaPtr, &Changes, aTCPHeader, aRetrievedTCPHdr, aIPHeader, aRetrievedIPHdr))
	{
		return RetCode;
    }

	if (!CompressIPId(&VJPtr, &Changes, aIPHeader, aRetrievedIPHdr))
	{
		return RetCode;
    }

	if (!CompressPush(&Changes, aTCPHeader))
	{
		return RetCode;
    }

	RetCode = ETrue;
	CompressedHdrLen += (VJPtr - StartVJDeltaPtr);
	VJPtr = VJHeader;
	SetFirstFewBytes(&Changes, NewConnection, &VJPtr, aTCPHeader, aConnection);
	
	//
	//	We now have a compressed header,
	//	Save the uncompressed header and 
	//	replace the IP header on the outgoing packet
	//

	iLastTxConn = aConnection;

	CopyTxHeader(aIPHeader, aConnection);

	TUint OriginalHdrLen = aIPHeader->NetGetHdrLen() + aTCPHeader->NetGetHdrLen();

	CopyInNewTxHeader(aPacket, VJHeader, OriginalHdrLen, CompressedHdrLen);

	DecrementPacketLen( aPacket, (OriginalHdrLen - CompressedHdrLen));

	return RetCode;
	}

void CVJCompressor::ConvertFrameToUncompVJ(ThdrIP* aIPHeader, TUint aConnection)
/**
The frame is to be sent as an Uncompressed VJ frame, so set the 
connection number in place of the protocol type, and save the header.

@param aIPHeader TCP/IP header
@param aConnection VJ connection number
*/
	{
		CopyTxHeader(aIPHeader, aConnection);
		aIPHeader->NetSetProtocol(aConnection);
		iLastTxConn = aConnection;
	}

TInt CVJCompressor::VJCompressFrame(RMBufChain& aPacket)
/**
Takes a packet and transforms it for sending.
It may end up VJ compressed, VJ uncompressed or untouched.

@param aPacket MBuf chain containing packet

@return PPP protocol type for the converted packet
*/
	{
	ThdrIP*		IPHeader;
	ThdrTCP*	TCPHeader;
	ThdrIP		StoredIPHdr;
	ThdrTCP		StoredTCPHdr;
	TUint		ConnectionNumber;

	IPHeader = GetIPHeader(aPacket);
	if (IPHeader->NetGetProtocol() == KProtocolInetTcp)
		{
		TCPHeader = ThdrTCP::NetPtr(IPHeader);
		if (!SendAsRawIP(IPHeader, TCPHeader))
			{
			if (SuitableForVJCompression( IPHeader, TCPHeader, &ConnectionNumber, &StoredIPHdr, &StoredTCPHdr))
				{
				//
				//	Better Compress it then
				//
				if (CompressFrame(aPacket, IPHeader, TCPHeader, ConnectionNumber, &StoredIPHdr, &StoredTCPHdr))
					{
					return KPppIdVjCompTcp;
					}
				}
			//
			//	Change the frame to VJ Uncompressed type
			//
			ConvertFrameToUncompVJ(IPHeader, ConnectionNumber);
			return KPppIdVjUncompTcp;
			}
		}

	//
	//	Send normal IP frame
	//
	//
	return KPppIdIp;
	}


void CVJCompressor::CopyTxHeader( ThdrIP* aIPHeader, TUint aConnection)
/**
Copies the given TCP/IP header into the storage slot for the given
connection number.

@param aIPHeader TCP/IP header
@param aConnection VJ connection number
*/
	{
	__ASSERT_DEBUG(aConnection < iMaxVJSlots, User::Panic(_L("VJ Panic"), 0));
	iTxStates[aConnection].StoreTCPIPHeader(aIPHeader);
	}


TBool CVJCompressor::IPAddressesMatch(ThdrIP* aIPHeader, ThdrIP* aNotherIPHdr)
/**
Compares the addresses in two IP headers.

@param aIPHeader First IP header
@param aNotherIPHdr Second IP header

@return Whether source and dest addresses in both headers are the same
*/
	{
	return (aNotherIPHdr->GetSrcAddr() == aIPHeader->GetSrcAddr()) &&
		   (aNotherIPHdr->GetDstAddr() == aIPHeader->GetDstAddr());
	}

TBool CVJCompressor::TCPPortsMatch(ThdrTCP* aHeader, ThdrTCP* aNotherHdr)
/**
Compares the TCP port numbers TCP headers.

@param aHeader First TCP header
@param aNotherHdr Second TCP header

@return Whether both source and dest ports in both headers are the same
*/
	{
	return (aNotherHdr->NetGetSrcPort() == aHeader->NetGetSrcPort()) &&
		   (aNotherHdr->NetGetDstPort() == aHeader->NetGetDstPort());
	}


TBool CVJCompressor::GetStoredTxHeader(TUint* aConnection, ThdrIP* aIPHeader, ThdrTCP* aTCPHeader, ThdrIP* aRetrievedIPHdr, ThdrTCP* aRetrievedTCPHdr)
/**
Retrieves a packet header from the VJ connection cache.
The packet must match on the IP source and destination address and on
the TCP source and destination port numbers.

@param aConnection Returns the matching VJ connection number
@param aIPHeader IP packet header
@param aTCPHeader TCP header
@param aRetrievedIPHdr Returns the cached IP packet header
@param aRetrievedTCPHdr Returns the cached TCP packet header

@return Whether a matching packet was found in the cache
*/
	{
	TBool RetCode = EFalse;

	//
	//	The transmit buffer is maintained
	//	in a least recently used circular linked list.
	//  iLastTxHdr always points to the least recently used entry and
    //  iLastTxHdr->NextPtr() points to the most recently used.
    //
    
    //
    //  Make a special case of the common occurrence that the 
    //  most recently used header is re-used (for speed).
	//

	TVJCompHdr* LastPtr = iLastTxHdr;  // Pointer to the last element in the circular list
	TVJCompHdr* Ptr = LastPtr->NextPtr();
	Ptr->RetrieveTCPIPHeader(aRetrievedIPHdr, aRetrievedTCPHdr);

    if ( Ptr->IsValid() &&
    	 IPAddressesMatch(aIPHeader, aRetrievedIPHdr) &&
         TCPPortsMatch(aTCPHeader, aRetrievedTCPHdr))
      {
	  //
      // We found a match without having to search the list
	  //
	  RetCode = ETrue;
      }
    else
      {
      //
      // Oh well, we have to play with linked lists
      //
      TVJCompHdr* PrevPtr;
      do 
         {
         PrevPtr = Ptr;
         Ptr = Ptr->NextPtr();

		 //
     	 // Don't bother comparing addresses and ports if the connection has
     	 // never been used. It's tempting to break out of the search loop
     	 // in that case (the remainder of the list is guaranteed to be empty
     	 // because it's stored in MRU order), but ghdn PrevPtr wouldn't
     	 // be correct.
     	 //
         if (Ptr->IsValid())
         	{
		    Ptr->RetrieveTCPIPHeader(aRetrievedIPHdr, aRetrievedTCPHdr);
			if (   IPAddressesMatch(aIPHeader, aRetrievedIPHdr) &&
	               TCPPortsMatch(aTCPHeader, aRetrievedTCPHdr))
	         	{
	         	//
	         	// We found it!! 
	         	//
	         	RetCode = ETrue;
	         	break;
	         	}
	         }
         }
      while(Ptr != LastPtr);

      if (RetCode && LastPtr != Ptr)
         {
         //
         // Right, take the buffer we just found and move it to
         // the start, with all the fiddling with pointers that 
         // implies.
         //
         PrevPtr->SetNextPtr(Ptr->NextPtr());
         Ptr->SetNextPtr(LastPtr->NextPtr());
         LastPtr->SetNextPtr(Ptr);
         }
      else
         {
         //
         // We didn't find it so use the current last frame.
         //
         iLastTxHdr = PrevPtr;
         }
      }
  
	*aConnection = Ptr->ConnectionNumber();
	return RetCode;
	}
