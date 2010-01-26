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

#include "vj.h"
#include "vjlog.h"
#include <networking/in_std.h>
#include <in_sock.h>

void CVJCompressor::ConstructL(CVJCompFactory* aFactory, TUint aSlots, TBool aCompressConnId)
	{

	TUint	i;

	iMaxVJSlots = aSlots;
	iLastTxConn = aSlots+1;

	iTxStates = new (ELeave)TVJCompHdr[aSlots];

	iTxStates[0].SetNextPtr(&iTxStates[aSlots-1]);

	iLastTxHdr		= &iTxStates[0];

	for (i=(aSlots-1);i>0;i--)
		{
		iTxStates[i].SetNextPtr(&iTxStates[i-1]);
		iTxStates[i].SetConnectionNumber(i);
		}

	iTxStates[0].SetNextPtr(&iTxStates[aSlots-1]);
	iTxStates[0].SetConnectionNumber(0);

	iCompressConnId = aCompressConnId;
	iFactory = aFactory;
	iFactory->Open();
	}

CVJCompressor::CVJCompressor(TUint aSlots, TBool aCompressConnId)
	{
	//Remove Warning
	aSlots = aSlots;
	aCompressConnId = aCompressConnId;
	return;
	}

 CVJCompressor::~CVJCompressor()
	{
	delete [] iTxStates;

	if (iFactory)
		iFactory->Close();
	}

ThdrIP * CVJCompressor::GetIPHeader(RMBufChain &aChain)
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

void CVJCompressor::DoIPChecksum(ThdrIP * aIPHeader, TUint16 aIPHeaderLength)
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

TUint8 * CVJCompressor::GetVJPtr(RMBufChain &aChain, TUint16 * aCurrentFrameLength)
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

void CVJCompressor::EncodeDelta( TUint8 ** aVJCompHeader, TUint16 aValue )
	{
	if (aValue >= 256)
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
	return;
	}

TBool CVJCompressor::SendAsRawIP(	ThdrIP* aIPHeader, 
								ThdrTCP* aTCPHeader )
	{
	TBool RetCode = TRUE;
	TUint Flags;

	if (!aIPHeader->NetGetFragment())
		{
		//
		//	Frame isn't  fargmented
		//

		Flags = aTCPHeader->VJGetFlags();
		Flags &= (KTcpFIN | KTcpSYN | KTcpRST | KTcpPSH | KTcpURG |KTcpACK);

		if (Flags == KTcpACK)
			{
			//
			//	The only flags we can compress are ACK
			//

			RetCode = FALSE;
			}
		}
	
	return RetCode;
	}


TBool CVJCompressor::SuitableForVJCompression(ThdrIP* aIPHeader, 
										  ThdrTCP* aTCPHeader, 
										  TUint* aConnection,
										  ThdrIP* aRetrievedIPHdr,
										  ThdrTCP* aRetrievedTCPHdr)
//
//
//
	{
	TBool RetCode = FALSE;
	if (GetStoredTxHeader(	aConnection, 
									aIPHeader, 
									aTCPHeader, 
									aRetrievedIPHdr, 
									aRetrievedTCPHdr))
		{
		//
		//	OK we found a stored header with the right 
		//	addresses is the header compressible
		//
		if (IsIPCompressible( aIPHeader, aRetrievedIPHdr))
			{
			if (IsTCPCompressible(aTCPHeader, aRetrievedTCPHdr))
				{
				RetCode = TRUE;
				}
			}

		}

	return RetCode;
	}

TBool CVJCompressor::IsIPCompressible(ThdrIP* aIPHeader, ThdrIP* aRetrievedHdr)
	{
	TBool	RetCode = FALSE;
	TUint8*	Options;
	TUint8*	StoredOptions;
	TUint	Length;
	TUint	StoredLength;

   // Remove warning
   StoredOptions = 0;

	//
	//	Ensure that the followin match
	//			Word 0
	//					Protocol Version,
	//					Header Length,
	//					Type Of Service,
	//			Word3	
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
		Length = aIPHeader->NetGetHdrLen() - KIPMinHeaderSize;
		Options = aIPHeader->GetOptions();
		StoredLength = aIPHeader->NetGetHdrLen() - KIPMinHeaderSize;
		StoredOptions = aRetrievedHdr->GetOptions();
		
		if (Length == StoredLength)
			{
			if (!Mem::Compare(Options, Length, StoredOptions, StoredLength))
				{
				RetCode = TRUE;
				}
			}
		}
		return RetCode;
	}

TUint8* CVJCompressor::GetTCPOpts(ThdrTCP* aTCPHeader)
	{
	TUint8* aPtr;
	aPtr = (TUint8*)aTCPHeader+KTCPHeaderSize;
	return aPtr;
	}


TBool CVJCompressor::IsTCPCompressible(ThdrTCP* aHeader, ThdrTCP* aRetrievedHdr)
	{
	TBool	RetCode = FALSE;
	TUint	Length;
	TUint	StoredLength;
	TUint8* Options;
	TUint8* StoredOptions;

   	//
	//	Ensure that the header lengths match, if they do then 
	//	check the options match
	//
	if (aHeader->NetGetHdrLen() == aRetrievedHdr->NetGetHdrLen())
		{
		//
		//	Now check the TCP Options 
		//
		Length = aHeader->NetGetHdrLen() - KTCPHeaderSize;
		StoredLength = aRetrievedHdr->NetGetHdrLen() - KTCPHeaderSize;

		Options = GetTCPOpts(aHeader);
		StoredOptions = GetTCPOpts(aRetrievedHdr);

		if (Length == StoredLength)
			{
			if (Mem::Compare(Options, Length, StoredOptions, StoredLength) == 0)
				{
				RetCode = TRUE;
				}
			}
		}
	return RetCode;
	}

TBool CVJCompressor::CompressUrgentPtr(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr)
	{
	TBool	RetCode = TRUE;
	TUint	Flags;
	TUint16	UrgPtr;
	
	Flags = aTCPHeader->VJGetFlags();
	if (Flags & KTcpURG)
		{
		(*aChanges) |= KVjCompMaskUrgent;
		UrgPtr = aTCPHeader->NetGetUrgPtr();
		if (UrgPtr > 255)
			{
			RetCode = FALSE; // RFC didn't do this, but I'm not happy otherwise PRR
			}
		*(*aVJPtr) = (TUint8)(UrgPtr & 0xFF);
		(*aVJPtr)++;				
		}
	else if (aTCPHeader->NetGetUrgPtr() != aRetrievedTCPHdr->NetGetUrgPtr())
		{
		//
		//	They can change the Urgent Ptr without setting the flag but 
		//	it's not recommended, just send uncompressed frame
		//
		RetCode = TRUE;
		}
		return RetCode;
	}

TBool CVJCompressor::CompressWindow(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr)
	{
	TInt16	DeltaWindow;
	TBool	RetCode = TRUE;

	DeltaWindow = (TUint16)(aTCPHeader->NetGetWindow() - aRetrievedTCPHdr->NetGetWindow());
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
	{
	TBool	RetCode = TRUE;
	TUint	DeltaAck;

	DeltaAck = aTCPHeader->NetGetAckNum() - aRetrievedTCPHdr->NetGetAckNum();
	if (DeltaAck)
		{
		if (DeltaAck > 0xFFFF)
			{
			RetCode = FALSE;
			}
		else
			{
			EncodeDelta(aVJPtr, (TUint16)DeltaAck);
			*aChanges |= KVjCompMaskAck;
			}
		}
		return RetCode;
	}


TBool CVJCompressor::CompressSeq(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr)
	{
	TBool	RetCode = TRUE;
	TUint	DeltaSeq;

	DeltaSeq = aTCPHeader->NetGetSeqNum() - aRetrievedTCPHdr->NetGetSeqNum();
	if (DeltaSeq)
		{
		if (DeltaSeq > 0xFFFF)
			{
			RetCode = FALSE;
			}
		else
			{
			EncodeDelta(aVJPtr, (TUint16)(DeltaSeq));
			*aChanges |= KVjCompMaskSeq;
			}
		}
	return RetCode;
	}

TBool CVJCompressor::CompressSpecialCases(TUint8** aVJPtr, 
						   TUint8* aVJInitialDeltaPtr, 
						   TUint8* aChanges, 
						   ThdrTCP* aTCPHeader, 
						   ThdrTCP* aRetrievedTCPHdr, 
						   ThdrIP*	aIPHeader,
						   ThdrIP* aRetrievedIPHdr)
	{
	TBool RetCode = TRUE;
	TUint DeltaAck;
	TUint DeltaSeq;

	DeltaSeq = aTCPHeader->NetGetSeqNum() - aRetrievedTCPHdr->NetGetSeqNum();
	DeltaAck = aTCPHeader->NetGetAckNum() - aRetrievedTCPHdr->NetGetAckNum();

	switch (*aChanges)
		{
		case 0:
			if ((aIPHeader->NetGetLength() == aRetrievedIPHdr->NetGetLength()) ||
				(aRetrievedIPHdr->NetGetLength() != aRetrievedIPHdr->NetGetHdrLen()))
				{
				//
				//	The frame has been sent compressed and was probably missed at the other
				//	end, so send it uncompressed.
				//
				LOG(_LIT(string1,"Missed frame");)
				LOG(Log::Write(string1);)
				RetCode = FALSE;				
				}
				break;
		case KVjCompMaskSpecialD:
		case KVjCompMaskSpecialI:
			{
			// Can't send SWU and SWAU, so send Uncompressed
			RetCode = FALSE;
			}
		case KVjCompMaskSeq | KVjCompMaskAck:
			{
			if (DeltaAck == DeltaSeq)
				{
				if (DeltaSeq == (aRetrievedIPHdr->NetGetLength() - aRetrievedIPHdr->NetGetHdrLen()))
					{
					// Terminal traffic
					*aChanges = KVjCompMaskSpecialI;
					*aVJPtr = aVJInitialDeltaPtr; // Reset back to just after CSum
					}
				}
			break;
			}
		case KVjCompMaskSeq:
			{
			if (DeltaSeq == (aRetrievedIPHdr->NetGetLength() - aRetrievedIPHdr->NetGetHdrLen()))
				{
				// Data Xfer e.g. FTP
				*aChanges = KVjCompMaskSpecialD;
				*aVJPtr = aVJInitialDeltaPtr; // Reset back to just after CSum
				}
			break;

			}
		}

	return RetCode;
	}

TBool CVJCompressor::CompressIPId(TUint8** aVJPtr, TUint8* aChanges, ThdrIP* aIPHeader, ThdrIP* aRetrievedIPHdr)
	{
	TBool RetCode = TRUE;
	TUint	DeltaIPId;

	DeltaIPId = aIPHeader->NetGetId() - aRetrievedIPHdr->NetGetId();

	if (DeltaIPId != 1)
		{
		EncodeDelta(aVJPtr, (TUint16)DeltaIPId);
		*aChanges |= KVjCompMaskIp;
		}

	return RetCode;
	}

TBool CVJCompressor::CompressPush(TUint8* aChanges,  ThdrTCP* aTCPHeader)
	{
	TBool 	RetCode = TRUE;
	TUint	Flags;

	Flags = aTCPHeader->VJGetFlags();
	if (Flags & KTcpPSH )
		{
		*aChanges |= KVjCompMaskPush;
		}

	return RetCode;
	}

TBool CVJCompressor::IsSameConnAsLast(TUint* aCompressedHdrLen, TUint aConnection)
	{
	TBool RetCode;

	if ((aConnection != iLastTxConn) || (iCompressConnId == FALSE))
		{
		RetCode = FALSE;
		*aCompressedHdrLen = 4; // Changes, Connection, CSum
		}
	else
		{
		RetCode = TRUE;
		*aCompressedHdrLen = 3; // Changes, CSum
		}
	return RetCode;
	}


void CVJCompressor::SetFirstFewBytes(TUint8* aChanges, TBool aNewConnection, TUint8** aVJHeader, ThdrTCP* aTCPHeader, TUint aConnection)
	{
	TUint16	Checksum;
	TUint8* ChecksumPtr;

	//
	// Be warned this value is NOT stored in native byte order
	// the sixteen bit value is just loaded from the frame it's NOT
	// byte swapped 
	//
	Checksum = (TUint16)aTCPHeader->NetGetChecksum();
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

	return;

	}

void CVJCompressor::CopyInNewTxHeader(RMBufChain& aPacket, TUint8* aCompressedHdr, TUint aOldHeaderLength, TUint aNewHeaderLength )
	{
	//
	// Remove the first info buffer thing
	//
	RMBuf* Temp = aPacket.Remove();

	//
	//	Trim the Packet length to take into account the new header
	//
	aPacket.TrimStart(aOldHeaderLength-aNewHeaderLength);


	{
	TPtrC8 TempDesc = TPtrC8(aCompressedHdr, aNewHeaderLength);
	aPacket.CopyIn(TempDesc, 0);
	}
	
	aPacket.Prepend(Temp);

	return;
	}

//
// There is a header prepended to the frame, one element is the length 
// which has now changed, so change it
//
void CVJCompressor::DecrementPacketLen( RMBufChain& aPacket, TUint SizeDecrease )
	{

	RMBufPktInfo* info = RMBufPacketBase::PeekInfoInChain(aPacket);
	info->iLength -= SizeDecrease;
	return;
	}

TBool CVJCompressor::CompressFrame(RMBufChain& aPacket, 
							   ThdrIP* aIPHeader, 
							   ThdrTCP* aTCPHeader, 
							   TUint aConnection,
							   ThdrIP* aRetrievedIPHdr,
							   ThdrTCP* aRetrievedTCPHdr)
	{
	TBool	RetCode=FALSE;
	TBool	NewConnection;
	TUint8	Changes=0;
	TUint8	VJHeader[16];	// The VJ Header without changes, checksum and connection Number
	TUint8* VJPtr;
	TUint8*	StartVJDeltaPtr;
	TUint	CompressedHdrLen;
	TUint	OriginalHdrLen;


	//
	//	Is Connection ID Compression turned on
	//
	NewConnection = !IsSameConnAsLast(&CompressedHdrLen, aConnection);


	VJPtr = VJHeader + CompressedHdrLen;	
	StartVJDeltaPtr = VJPtr;

	if (!CompressUrgentPtr(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
		return RetCode;

	if (!CompressWindow(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
		return RetCode;

	if (!CompressAck(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
		return RetCode;

	if (!CompressSeq(&VJPtr, &Changes, aTCPHeader, aRetrievedTCPHdr))
		return RetCode;

	if (!CompressSpecialCases(&VJPtr, StartVJDeltaPtr, &Changes, aTCPHeader, aRetrievedTCPHdr, aIPHeader, aRetrievedIPHdr))
		return RetCode;

	if (!CompressIPId(&VJPtr, &Changes, aIPHeader, aRetrievedIPHdr))
		return RetCode;

	if (!CompressPush(&Changes, aTCPHeader))
		return RetCode;

	RetCode = TRUE;
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

	OriginalHdrLen = aIPHeader->NetGetHdrLen() + aTCPHeader->NetGetHdrLen();

	CopyInNewTxHeader(aPacket, VJHeader, OriginalHdrLen, CompressedHdrLen);

	DecrementPacketLen( aPacket, (OriginalHdrLen - CompressedHdrLen));

	return RetCode;
	}

void CVJCompressor::ConvertFrameToUncompVJ(ThdrIP* aIPHeader, TUint aConnection)
//
//	The frame is to be sent as an Uncompressed VJ frame, so set the 
//	connection number in place of the protocol type, and save the header
//
	{
		CopyTxHeader(aIPHeader, aConnection);

		aIPHeader->NetSetProtocol(aConnection);

		iLastTxConn = aConnection;

		return;

	}

TInt CVJCompressor::VJCompressFrame(RMBufChain& aPacket)
//
//
//
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
				//_LIT(string1,"Stored ");
				//LOG(Log::Write(string1);)
				if (CompressFrame(aPacket, IPHeader, TCPHeader, ConnectionNumber, &StoredIPHdr, &StoredTCPHdr))
					{
					return KPppIdVjCompTcp;
					}
				else
					{
					//
					//	In some cases one discovers whilst compressing a frame
					//	that it can't be done, so send a VJ Uncompressed frame
					//
					ConvertFrameToUncompVJ(IPHeader, ConnectionNumber);
					return KPppIdVjUncompTcp;
					}
				}
				else
					{
					//
					//	Change the frame to VJ UNcompressed type
					//
					ConvertFrameToUncompVJ(IPHeader, ConnectionNumber);
					return KPppIdVjUncompTcp;
				}
			}
		}



	//
	//	Send normal IP frame
	//
	//
	return KPppIdIp;
	}




void CVJCompressor::CopyTxHeader( ThdrIP* aIPHeader, TUint aConnection)
	{
		iTxStates[aConnection].StoreTCPIPHeader(aIPHeader);
	}

TBool CVJCompressor::IPAddressesMatch(ThdrIP* aIPHeader, ThdrIP* aNotherIPHdr)
	{
	TBool	RetCode = FALSE;
	if ((aNotherIPHdr->GetSrcAddr() == aIPHeader->GetSrcAddr()) &&
		(aNotherIPHdr->GetDstAddr() == aIPHeader->GetDstAddr()))
		{
		RetCode = TRUE;
		}
	return RetCode;
	}

TBool CVJCompressor::TCPPortsMatch(ThdrTCP* aHeader, ThdrTCP* aNotherHdr)
	{
	TBool	RetCode;
	if((aNotherHdr->NetGetSrcPort() == aHeader->NetGetSrcPort()) &&
		(aNotherHdr->NetGetDstPort() == aHeader->NetGetDstPort()))
		{
		RetCode = TRUE;
		}
	else
		{
		RetCode = FALSE;
		}
	return RetCode;
	}




TBool CVJCompressor::GetStoredTxHeader(TUint* aConnection, ThdrIP * aIPHeader, ThdrTCP * aTCPHeader, ThdrIP* aRetrievedIPHdr, ThdrTCP* aRetrievedTCPHdr)
	{
	TBool		RetCode = FALSE;
	TVJCompHdr * LastPtr;
	TVJCompHdr * Ptr;
	TVJCompHdr * PrevPtr;

	//
	//	OK Ladies and Gentleman listen up we have a ...
	//
	//	The transmit buffer is maintained
	//	in a least recently used circular linked
	//	list
   //
   // Make a special case of the common occurence that the 
   // most recently used header is re-used (for speed).
	//

	LastPtr = iLastTxHdr;  // Pointer to the last element in the circular list
	Ptr = LastPtr->NextPtr();
	Ptr->RetrieveTCPIPHeader(aRetrievedIPHdr, aRetrievedTCPHdr);
		  
   if (  !IPAddressesMatch(aIPHeader, aRetrievedIPHdr) ||
         !TCPPortsMatch(aTCPHeader, aRetrievedTCPHdr))
      {  
      //
      // Oh well we have to play with linked lists
      //
      do 
         {
         PrevPtr = Ptr;
         Ptr = Ptr->NextPtr();
	      Ptr->RetrieveTCPIPHeader(aRetrievedIPHdr, aRetrievedTCPHdr);

         if (  IPAddressesMatch(aIPHeader, aRetrievedIPHdr) &&
               TCPPortsMatch(aTCPHeader, aRetrievedTCPHdr))
            {
            //
            // We found it!! 
            //
            RetCode = TRUE;
            break;
            }
         }
      while(Ptr != LastPtr);

      if (RetCode == TRUE)
         {
         //
         // Right take the buffer we just found and move it to
         // the start, with all the fiddling with pointers that 
         // implies
         //
         if (LastPtr == Ptr)
            {
            iLastTxHdr = PrevPtr;
            }
         else
            {
            PrevPtr->SetNextPtr(Ptr->NextPtr());
            Ptr->SetNextPtr(LastPtr->NextPtr());
            LastPtr->SetNextPtr(Ptr);
            }
         }
      else
         {
         //
         // We didn't find it so use the current last frame
         //
         iLastTxHdr = PrevPtr;
         }
		*aConnection = Ptr->ConnectionNumber();
      }
   else
      {
		*aConnection = Ptr->ConnectionNumber();
		RetCode = TRUE;
      }

  
	return RetCode;
	}



