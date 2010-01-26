// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// esp_eng.cpp - IPv6/IPv4 IPSEC encapsulating security payload
// The IPSEC Encapsulating Security Payload (ESP) packet processing class
//



/**
 @file esp_eng6.cpp
*/

#include "ip6_hdr.h"
#include "ext_hdr.h"
#include "in_chk.h"
#include "sadb.h"
#include "sa_crypt.h"
#include <networking/ipsecerr.h>
#include "esp_eng.h"
#include "ipseclog.h"
#include <comms-infras/mbufallocator.h>

#define KSizeOfNextHdr 2
#define KNumBytesPadLen 2

static TUint8 *Pass1ScanL(RMBufPacketBase &aPacket, TInt &aLength, TInt aLookFor = -1)
	/**
	* Peek some information from the extension headers.
	*
	* @li locate the last extension header (or the next header in IPv6
	*	header, if no extension headers), and return a pointer to it.
	*
	* The packet is not modified by this.
	*
	* @param aPacket	The packet to peek
	* @retval aLength	Offset of the first header after extension headers.
	* @param aLookFor	Stop peek if this extension header found.
	*
	* @return The pointer to next Header/Protocol field in the packet.
	*/
	{
	TPacketPoker pkt(aPacket);
	TInet6HeaderIP4 *ip;
	TUint8 *next_header;

	// There must be at least the first RMBuf, containing the IPv6 or IPv4
	// header in full! "Autodetect" between IPv4 and IPv6
	ip = (TInet6HeaderIP4 *)pkt.ReferenceL(TInet6HeaderIP4::MinHeaderLength());
	if (ip->Version() == 4)
		{
		next_header = (TUint8 *)ip + TInet6HeaderIP4::O_Protocol;
		aLength = ip->HeaderLength();
		// Strictly, for IPv4 only header needs to be skipped, but
		// for now drop through to the normal IPv6 lookup, and
		// implement a "feature" for someone who is crazy enough
		// to use IPv6 extension headers under IPv4.  -- msa
#if 0	// <-- Change to 1 to disable the "feature"
		return next_header
#endif
		}
	else
		{
		next_header = (TUint8 *)ip + TInet6HeaderIP::O_NextHeader;	// assumes O_NextHeader < TInet6HeaderIP4::MinHeaderLength()
		aLength = sizeof(TInet6HeaderIP);
		}
	pkt.SkipL(aLength);
	while (pkt.IsExtensionHeader(*next_header) && pkt.More())
		{
		TInet6HeaderExtension *ext;
		ext = (TInet6HeaderExtension *)pkt.ReferenceL(sizeof(TInet6HeaderExtension));
		// ..the test for AH header is "hard coded", because one cannot
		// go underneath it afterwards. [happens when one specifies a
		// strange policy that lays ESP on top of AH]
		// Note! It is not necessary to test "LookFor != -1", because on input
		// side there cannot be an AH header in the packet head! Ah
		// processing *removes* the header in *CURRENT* implementation
		// --if this is changed, things break here! -- msa
		if (*next_header == KProtocolInetAh)
			break;
		aLength += ext->HeaderLength();
		if (*next_header == aLookFor)
			break;
		next_header = (TUint8 *)ext;
		}
	return next_header;
	}

TInt TIpsecESP::Overhead(const CSecurityAssoc &aSa) const
	/**
	* Return maximum possible overhead caused by this ESP SA.
	*
	* @param aSa The Security Association
	* @return The overhead (bytes).
	*/
	{
	TInt overhead;
	
	if (!aSa.iEeng)
		return 0;

	// A problem? Padding need is always returned as worst
	// case (and will be accumulated from all transforms
	// unnecessarily -- msa)
	overhead = sizeof(TInet6HeaderESP) + aSa.iEeng->IVSize() +
			2 + aSa.iEeng->BlockSize() - 1;

	// If Authentication included, add the digest size
	if (aSa.iAeng)
		overhead += aSa.iAeng->DigestSize();
	return overhead;
	}

TInt TIpsecESP::ApplyL(CSecurityAssoc &aSa, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, RMBufAllocator &aBufAllocator)
	/**
	* Apply IPsec ESP processing to outbound packet.
	*
	* @li	aPacket is the outgoing packet. It is *FULL* packet
	*		and already includes the outgoing IP header.
	* @li	aInfo is the associated info block. The info->iLength
	*		*MUST* be maintained, if any change affects it.
	*
	* @param aSa The security association (ESP type)
	* @param aPacket The packet to process
	* @param aInfo The packet info
	* @param aBufAllocator The MBufAllocator used for the allocation of mbuf chains
	*
	* @return
	*	@li	Normally leaves on any error condition, which causes
	*		the packet to be dropped,
	*	@li	KErrNone, the ESP header has been added.
	* 
	*	@li < 0, some other unexpected error.
	*/
	{
	RMBufChain payloadChainIn, payloadChainOut;
	TInt result = KErrNone;
	TUint8 *dat;
	
	if (!aSa.iEeng)
		User::Leave(EIpsec_EspEncrAlg);		// Required algorithm not installed

	TInt split;
	TUint8 *next_header = Pass1ScanL(aPacket, split /*, KProtocolInet6DestinationOptions*/);
	aPacket.Split(split, payloadChainIn, aBufAllocator);	// <-- Must have "split > 0" due SplitL "feature"!! -- msa

	if (payloadChainIn.IsEmpty())
		return EIpsec_CorruptPacketIn;
	
	TCleanupItem mbufInCleanupItem(payloadChainIn);
	CleanupStack::PushL(mbufInCleanupItem);

	TCleanupItem mbufOutCleanupItem(payloadChainOut);
	CleanupStack::PushL(mbufOutCleanupItem);
	
	// coverity[unreachable];	
	for (;;)		// Only for easy exits...
		{
		// Compute the required extensions to the head and tail
		const TInt esphlen = sizeof(TInet6HeaderESP) + aSa.iEeng->IVSize();

		// The payload need to be padded to a length that is BOTH
		// multiple of 4 and BlockSize(). Compute the smallest
		// integer bsize, that is divisible by both!
		TInt bsize = aSa.iEeng->BlockSize();
		if ((bsize & 0x3) != 0)
			{
			// BlockSize not divisible by 4
			bsize <<= 1;	// multiply by 2
			if ((bsize & 0x3) !=0)
				{
				// BlockSize*2 still not divisible by 4 
				bsize <<= 1;	// multiply by 2, BlockSize*4 is ok
				}
			}
		if (bsize >= 256)
			{
			result = EIpsec_EspBadCipherBlockSize;
			break;
			}

		TInt psize = aInfo.iLength - split + KSizeOfNextHdr;
		const TInt padlen = (((psize + bsize - 1) / bsize) * bsize - psize);

		TRAP(result, payloadChainIn.AppendL(padlen + KSizeOfNextHdr));  // pad length and next header fields
		if (result != KErrNone)
			{
			result = EIpsec_RMBUF;
		   	break;
			}
		
		// Allocate a buy the size of the first payloadChainIn RMBuf, should enough space for esphdr
		// we allocate additional blocks as required
		TRAP(result,payloadChainOut.AllocL(payloadChainIn.First()->Size(), aBufAllocator));
		if (result != KErrNone)
			{
			result = EIpsec_RMBUF;
		   	break;
			}

		aInfo.iLength += esphlen + padlen + KSizeOfNextHdr;
	
		TInet6Packet<TInet6HeaderESP> esp;
		esp.Set(payloadChainOut, 0, esphlen);
		if (esp.iHdr == NULL)
			{
			result = EIpsec_RMBUF;
			goto done;
			}
		esp.iHdr->SetSPI(aSa.iSPI);
		esp.iHdr->SetSequence(++aSa.iSendSeq);
		
		// The sequence number wrap around is significant only
		// if replay window is enabled.
		if (aSa.iReplayCheck && aSa.iSendSeq == 0)
			{
			// Unless extended sequence numbering is enabled, low order overflow
			// is sufficient, otherwise test the high bits too.
			if ((aSa.iFlags & SABD_SAFLAGS_ESN) == 0 || aSa.iSendSeq.High() == 0)
				{
				// Sequence number is not allowed to wrap around
				// SA needs to be deleted
				// This does not generate PFKEY expired message! Should it? -- msa
				iManager->Delete(&aSa);
				result = EIpsec_EspSequenceWrap;
				break;
				}
			}
		// Yechh!! Icky coding required to fill the tail end of the
		// packet, because there is no guarantee that the required
		// bytes are in contiguous memory (from experience: they
		// usually are not!!!)
		TInt offset, len;
		RMBuf *p;
		
		if (!payloadChainIn.Goto(psize - KSizeOfNextHdr, p, offset, len))
			{
			result = EIpsec_RMBUF;
			goto done;				// Something is not okay.
			}
		dat = p->Buffer() + offset;			// p->Ptr() would be incorrect here!
		for (int i = 0; i < padlen; *dat++ = (TUint8)++i, --len)
			while (len == 0)	// Should loop only once, but 'while' just in
								// case someone wants RMBuf with zero length...
				{
				p = p->Next();
				// Can't run out here, we just appended enough bytes!!
				ASSERT(p != NULL);
				len = p->Length();
				dat = p->Ptr();
				}
		while (len == 0)	// Should loop only once, but 'while' just in
							// case someone wants RMBuf with zero length...
			{	
			p = p->Next();
			// Can't run out here, we just appended enough bytes!!
			ASSERT(p != NULL);
			len = p->Length();
			dat = p->Ptr();
			}
		*dat++ = (TUint8)padlen;
		--len;
		while (len == 0)	// Should loop only once, but 'while' just in
							// case someone wants RMBuf with zero length...
			{
			p = p->Next();
			// Can't run out here, we just appended enough bytes!!
			ASSERT(p != NULL);
			len = p->Length();
			dat = p->Ptr();
			}
		//
		// Update the next header chaining
		//
		*dat++ = *next_header;
		*next_header = KProtocolInetEsp;
		// End icky things...

		TPtr8 iv1(esp.iHdr->IV(aSa.iEeng->IVSize()));
		iv1 = *aSa.iIV;				// Copy current IV into packet

		TMBufIter buffersToBeEncrypted(payloadChainIn);

		RMBuf *buf = NULL;
		RMBuf *mBufIn = buffersToBeEncrypted++;
		TInt bufInLen = mBufIn->Length();
		
		ASSERT(bufInLen >= 0);
		
		TPtr8 bufIn(mBufIn->Ptr(), bufInLen, bufInLen);

		// Skip over ESP Header in payload output chain
		RMBuf *mBufOut = payloadChainOut.First();
		TInt bufOutLen = mBufOut->Length();

		TPtr8 bufOut(mBufOut->Ptr(), 0, bufOutLen);
		TInt remainder;
		TInt bufInOffset = 0;
		TInt bufOutOffset = esphlen;
		TInt outputLen;
		
		// Reset the encryptor before we start the encryption
		aSa.iEeng->Reset();
 
		aSa.iEeng->EncryptL(iv1);

		while (buffersToBeEncrypted.More())
			{
			outputLen = aSa.iEeng->GetOutputLength(bufIn.Length());
			outputLen += bufOutOffset;
			if (bufOutLen >= outputLen)
				{
				bufOut.Set(mBufOut->Ptr()+bufOutOffset, 0, outputLen);
				aSa.iEeng->UpdateL(bufIn, bufOut);
				// Trim RMbuf
				mBufOut->SetData(mBufOut->Offset(), outputLen);

				// move to next buffer in
				mBufIn = payloadChainIn.Remove();
				mBufIn->Free();
				buffersToBeEncrypted = payloadChainIn;
				mBufIn = buffersToBeEncrypted++;					
				bufIn.Set(mBufIn->Ptr(), mBufIn->Length(), mBufIn->Length());
				bufInOffset = 0;
				}
			// round down to the nearest block size, and encrypt what we can
			else 
				{
				bufOutLen -= bufOutOffset;
				TInt inputLen = ((bufOutLen/bsize) * bsize);
				outputLen = aSa.iEeng->GetOutputLength(inputLen);
				outputLen += bufOutOffset;
				
				bufOut.Set(mBufOut->Ptr() + bufOutOffset, 0, outputLen);
				TPtr8 bufTemp(mBufIn->Ptr() + bufInOffset, inputLen, inputLen);
				aSa.iEeng->UpdateL(bufTemp, bufOut);
				
				// Adjust bufIn
				remainder = mBufIn->Length() - bufInOffset - inputLen;
				bufInOffset += inputLen;
				bufIn.Set(mBufIn->Ptr() + bufInOffset, remainder, remainder);
				// Trim mBufOut
				mBufOut->SetData(mBufOut->Offset(), outputLen);
				// don't move onto the next mbufIn in the chain
				}
			// move to the next mBufOut in chain
			if (mBufOut->Next() == 0)
				{
				TRAP(result, buf=RMBuf::AllocL(mBufIn->Size(), aBufAllocator));
				if (result != KErrNone)
					{
					result = EIpsec_RMBUF;
					goto done;
					}

				// link new buffer
				mBufOut->Link(buf);
				}

			// move to next buffer out
			mBufOut = mBufOut->Next();
			bufOutLen = mBufOut->Length();
			bufOut.Set(mBufOut->Ptr(), 0, bufOutLen);
			bufOutOffset =  0;
			}

		// Do final block
		while (ETrue)
			{
			outputLen = aSa.iEeng->GetFinalOutputLength(bufIn.Length());
			outputLen += bufOutOffset;
			if (bufOutLen >= outputLen)
				{
				bufOut.Set(mBufOut->Ptr()+bufOutOffset, 0, outputLen);
				aSa.iEeng->UpdateFinalL(bufIn, bufOut);
				// Trim RMbuf
				mBufOut->SetData(mBufOut->Offset(), outputLen);
				break;
				}
			// round down to the nearest block size
			else 
				{
				TInt remainder;
				bufOutLen -= bufOutOffset;
				TInt inputLen = ((bufOutLen/bsize) * bsize);
				outputLen = aSa.iEeng->GetOutputLength(inputLen);
				outputLen += bufOutOffset;
				
				bufOut.Set(mBufOut->Ptr()+bufOutOffset, 0, outputLen);
				TPtr8 bufTemp(mBufIn->Ptr()+bufInOffset, inputLen, inputLen);
				aSa.iEeng->UpdateL(bufTemp, bufOut);

				// Adjust bufIn
				remainder = mBufIn->Length() - bufInOffset - inputLen;
				bufInOffset += inputLen;
				bufIn.Set(mBufIn->Ptr() + bufInOffset, remainder, remainder);
				// Trim mBufOut
				mBufOut->SetData(mBufOut->Offset(), outputLen);
				}
			// Only allocate a new buffer if required
			if (bufOut.Length())
				{
				// move to the next mBufOut in chain
				if (mBufOut->Next() == 0)
					{
					TRAP(result, buf=RMBuf::AllocL(mBufIn->Size(), aBufAllocator));
					if (result != KErrNone)
						{
						result = EIpsec_RMBUF;
						goto done;
						}
					mBufOut->Link(buf);
					}

				mBufOut = mBufOut->Next();
				bufOutLen = mBufOut->Length();
				bufOut.Set(mBufOut->Ptr(), 0, bufOutLen);
				bufOutOffset = 0;
				}				
			}

		aSa.iCurrent.iBytes += psize + padlen;

		// If SA specifies authentication, the iAeng is present
		if (aSa.iAeng)
			{
			aSa.iAeng->Init();
			TMBufIter m(payloadChainOut);
			while ((p = m++) != NULL)
				aSa.iAeng->Update(TPtrC8(p->Ptr(), p->Length()));

			// ** If ESN: Add high order bits to digest
			if (aSa.iFlags & SABD_SAFLAGS_ESN)
				{
				TUint8 buf[4];
				const TUint32 seq = aSa.iSendSeq.High();
				LOG(Log::Printf(_L("Send ESN %u:%u"), seq, (TInt)aSa.iSendSeq));
				buf[3] = (TUint8)seq;
				buf[2] = (TUint8)(seq >> 8);
				buf[1] = (TUint8)(seq >> 16);
				buf[0] = (TUint8)(seq >> 24);
				aSa.iAeng->Update(TPtrC8(buf, 4));
				aSa.iCurrent.iBytes += 4;	// Algorithm applied to 4 additional bytes.
				}

			int dsize = aSa.iAeng->DigestSize();
			TRAP(result, payloadChainOut.AppendL(dsize));
			if (result != KErrNone)
				{
				result = EIpsec_RMBUF;
				goto done;
				}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
			if (aSa.iAalg == SADB_AALG_AES_XCBC_MAC)
				{
				payloadChainOut.CopyIn(aSa.iAeng->Final(dsize),esphlen + psize + padlen);
				}
			else
				{
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
				payloadChainOut.CopyIn(aSa.iAeng->Final(dsize), esphlen + psize + padlen);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
				}
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
				aInfo.iLength += dsize;

			// Count only ESP+iv size against the bytes lifetime
			// (the remaining bytes are counted later with decrypt)
			aSa.iCurrent.iBytes += esphlen;
			}
		else if (aSa.iAalg)
			{
			result = EIpsec_EspAuthAlg;	// required algorithm is missing!
			goto done;
			}
		// Assume IPv4 header length is enough for accessing both IPv6 and IPv4 payload fields -- msa
		TInet6Checksum<TInet6HeaderIP4> ip(aPacket);
		if (ip.iHdr == NULL)
			{
			result = EIpsec_RMBUF;
			goto done;
			}

		if (ip.iHdr->Version() == 4)
			{
			ip.iHdr->SetTotalLength(aInfo.iLength);
			// Needs to be looked into. In ip6.cpp a checksum is computed
			// unnecessarily when IPSEC is applied.Should delay checksum
			// after hooks...
			ip.ComputeChecksum();
			}
		else
			((TInet6HeaderIP *)ip.iHdr)->SetPayloadLength(aInfo.iLength - sizeof(TInet6HeaderIP));
		result = aSa.MarkUsed(*iManager); // + Expire test for counts
		break;		// -- NO REAL LOOP, BREAK ALWAYS --
		}
done:
	CleanupStack::Pop(); // mbufOutCleanupItem
	if (!payloadChainOut.IsEmpty())
		{
		aPacket.Append(payloadChainOut);
		}
	// Reset the encryptor state before returning
	aSa.iEeng->Reset();
		
	CleanupStack::PopAndDestroy(); // mbufInCleanupItem
	// *NOTE* On result < KErrNone, the payload length of the IPv6 header in
	// the packet may be incorrect.
	return result;
	}

TInt TIpsecESP::ApplyL(CSecurityAssoc* &aSa, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, RMBufAllocator &aBufAllocator)
	/**
	* Unwrap IPSEC ESP from a received packet.
	*
	* TIpsecESP::ApplyL changes the info.iLength, because it removes
	* the ESP from the packet.
	*	
	* -	the current "scanning point" in the aPacket is indicated
	* 	by the aInfo.iOffset. The remaining "unscanned" packet length
	*	is aInfo.iLength - aInfo.iOffset.
	*
	* @param aSa The Security Association (AH type)
	* @param aPacket The packet
	* @param aInfo The packet infomrmation
	* @param aBufAllocator The MBufAllocator used for the allocation of mbuf chains
	*
	* @return
	*	@li	Signals some error conditions with leaving,
	*	@li < 0, Error detected.
	*	@li > 0, Next Protocol (ESP passed checks and has been removed)
	*
	*	TIpsecESP::ApplyL will never return 0, because it always either
	*	fails or successfully unwraps the ESP and returns the next
	*	protocol layer underneath!
	*/
	{
	RMBufChain payloadChainIn, payloadChainOut;
	TInt result = KErrNone;

 	// After SplitL plen MUST always be same as payload.Length()!
	TInt plen = aInfo.iLength - aInfo.iOffset;
	
	aPacket.Split(aInfo.iOffset, payloadChainIn, aBufAllocator); // <-- Must have "aInfo.iOffset > 0" due SplitL "feature"!! -- msa
	if (payloadChainIn.IsEmpty())
		return EIpsec_CorruptPacketIn;
	
	TCleanupItem mbufInCleanupItem(payloadChainIn);
	CleanupStack::PushL(mbufInCleanupItem);

	TCleanupItem mbufOutCleanupItem(payloadChainOut);
	CleanupStack::PushL(mbufOutCleanupItem);
	
	// coverity[unreachable];	
	for (;;)		// Just for simple exits
		{
		TInt offset;

		TInet6Packet<TInet6HeaderESP> esp(payloadChainIn);
		if (esp.iHdr == NULL)
			{
			result = EIpsec_CorruptPacketIn;
			break;
			}
		aSa = iManager->Lookup(SADB_SATYPE_ESP, esp.iHdr->SPI(), TIpAddress(aInfo.iDstAddr));
		if (!aSa)
			{
			result = EIpsec_EspInboundSA;		// --> No usable SA, drop packet!
			break;
			}

		const TInt ivlen = aSa->iEeng->IVSize();
		const TInt esphlen = ivlen + sizeof(TInet6HeaderESP);

		if (aSa->iAeng)
			{
			// SA defines Authentication, process it...
			const TInt dsize = aSa->iAeng->DigestSize();

			// Sequence number must only be enabled if SA includes authentication
			// (RFC-2406, 3.4.3)
			if (!aSa->ReplayCheck(esp.iHdr->Sequence()))
				{
				result = EIpsec_ReplayDuplicate;	//	--> Duplicate discarded
				break;
				}

			// Compute the authentication value of the payload
			// including all but the last dsize octets.
			TMBufIter m(payloadChainIn);
			RMBuf *p;
			offset = plen - dsize;
			if (offset < esphlen)
				{
				result = EIpsec_CorruptPacketIn;
				break;
				}
			aSa->iAeng->Init();
			for (;;)
				{
				if ((p = m++) == NULL)
					{
					result = EIpsec_CorruptPacketIn;
					goto done;
					}
				const TInt len = p->Length();
				if (offset <= len)
					break;
				aSa->iAeng->Update(TPtrC8(p->Ptr(), len));
				offset -= len;
				}
			// Always true: offset > 0 after above!
			ASSERT(offset > 0);
			aSa->iAeng->Update(TPtrC8(p->Ptr(), offset));
			// Count only ESP+iv size against the bytes lifetime
			// (the remaining bytes are counted later with decrypt)
			TInt bytes = esphlen;
			// ** If ESN: Add high order bits to digest
			if (aSa->iFlags & SABD_SAFLAGS_ESN)
				{
				TUint8 buf[4];
				const TUint32 seq = aSa->iTestSeq.High();
				LOG(Log::Printf(_L("Recv ESN %u:%u"), seq, (TInt)aSa->iTestSeq));
				buf[3] = (TUint8)seq;
				buf[2] = (TUint8)(seq >> 8);
				buf[1] = (TUint8)(seq >> 16);
				buf[0] = (TUint8)(seq >> 24);
				aSa->iAeng->Update(TPtrC8(buf, 4));
				bytes += 4;	// ...update bytes used count.
				}

			RMBufChain tail(p);
			// Use of Access may be heavier than just simply copying the data
			// away (if Access needs to split and copy RMBuf!!) -- msa
			if (aSa->iAeng->Compare(((RMBufPacketPeek &)tail).Access(dsize, offset)) != 0)
				{
				result = EIpsec_EspAuthentication;	// authentication fail!
				break;
				}
 			tail.TrimEnd(offset);
			plen -= dsize;
			
			// Authentication has passed, commit the bytes count
			aSa->iCurrent.iBytes += bytes;
			// ..and conclude the Sequence Number processing
			aSa->ReplayUpdate(esp.iHdr->Sequence());
			}

		TInt bsize = aSa->iEeng->BlockSize();

		TRAP(result,payloadChainOut.AllocL(payloadChainIn.First()->Size(), aBufAllocator));
		if (result != KErrNone)
			{
			result = EIpsec_RMBUF;		
			goto done;
			}

		// Decrypt the payload
		//
		// Make an assumption that ESP+IV can be aligned into single RMBuf!
		esp.Set(payloadChainIn, 0, esphlen);
		if (esp.iHdr == NULL)
			{
			result = EIpsec_RMBUF;			// -- assumption incorrect??!
			break;
			}

		// Reset the decryptor state before we start the decryption
		if(aSa && aSa->iEeng)
			{
			aSa->iEeng->Reset();
			}

		TPtr8 iv1 = esp.iHdr->IV(ivlen);
		aSa->iEeng->DecryptL(iv1);

		TMBufIter buffersToBeDecrypted(payloadChainIn);
		RMBuf *mBufIn = buffersToBeDecrypted++; 
		RMBuf *mBufOut = payloadChainOut.First();
		RMBuf *buf = NULL;

		// The first RMBuf contains ESP+IV, and potentially something else.
		// Decrypt "something else" part, if present.
 		TInt bufInLen = mBufIn->Length() - esphlen;
		TPtr8 bufIn(mBufIn->Ptr() + esphlen, bufInLen, bufInLen);
		TInt bufOutLen = mBufOut->Length();
		TPtr8 bufOut(mBufOut->Ptr(), 0, bufOutLen);
		TInt outputLen;
		TInt bufInOffset = esphlen;

		if (mBufIn->Length() - bufInOffset > 0)
			{
			while (buffersToBeDecrypted.More())
				{
				if (bufOutLen >= aSa->iEeng->GetOutputLength(bufIn.Length()))
					{
					outputLen = aSa->iEeng->GetOutputLength(bufIn.Length());
					bufOut.Set(mBufOut->Ptr(), 0, outputLen);
					aSa->iEeng->UpdateL(bufIn, bufOut);
					// Trim RMbuf
					mBufOut->SetData(mBufOut->Offset(), outputLen);
					// move to next buffer in
					mBufIn = payloadChainIn.Remove();
					mBufIn->Free();
					buffersToBeDecrypted = payloadChainIn;
					mBufIn = buffersToBeDecrypted++;					
					bufIn.Set(mBufIn->Ptr(), mBufIn->Length(), mBufIn->Length());
					bufInOffset = 0;
					}
				// round down to the nearest block size, and encrypt what we can
				else 
					{
					TInt remainder;
					TInt inputLen = ((bufOutLen/bsize) * bsize);
					TInt outputLen = aSa->iEeng->GetOutputLength(inputLen);
					
					bufOut.Set(mBufOut->Ptr(), 0, outputLen);
					bufIn.Set(mBufIn->Ptr() + bufInOffset, inputLen, inputLen);
				
					aSa->iEeng->UpdateL(bufIn, bufOut);
					// Adjust bufIn
					remainder = mBufIn->Length() -bufInOffset - inputLen;
					bufInOffset += inputLen;
					bufIn.Set(mBufIn->Ptr() + bufInOffset, remainder, remainder);
					// Trim mBufOut
					mBufOut->SetData(mBufOut->Offset(), outputLen);
					// don't move onto the next mbuf in chain
					}
				// move to the next mBufOut in chain
				if (mBufOut->Next() == 0)
					{
					TRAP(result, buf=RMBuf::AllocL(mBufIn->Size(), aBufAllocator));
					if (result != KErrNone)
						{
						result = EIpsec_RMBUF;		
						goto done;
						}

					// link new buffer
					mBufOut->Link(buf);
					}

				// move to next buffer out
				mBufOut = mBufOut->Next();
				bufOutLen = mBufOut->Length();
				bufOut.Set(mBufOut->Ptr(), 0, bufOutLen);
				
				}
			
			// Do final block
			while (ETrue)
				{
				outputLen = aSa->iEeng->GetFinalOutputLength(bufIn.Length());
				if (bufOutLen >= outputLen)
					{
					bufOut.Set(mBufOut->Ptr(), 0, outputLen);
					aSa->iEeng->UpdateFinalL(bufIn, bufOut);
					// Trim RMbuf
					mBufOut->SetData(mBufOut->Offset(), outputLen);
					break;
					}
				// round down to the nearest block size
				else 
					{
					TInt remainder;
					TInt inputLen = ((bufOutLen/bsize) * bsize);
					outputLen = aSa->iEeng->GetOutputLength(inputLen);
					
					bufOut.Set(mBufOut->Ptr(), 0, outputLen);
					bufIn.Set(mBufIn->Ptr() + bufInOffset, inputLen, inputLen);
					
					aSa->iEeng->UpdateL(bufIn, bufOut);
					// Adjust bufIn
					remainder = mBufIn->Length() - bufInOffset - inputLen;
					bufInOffset += inputLen;
					bufIn.Set(mBufIn->Ptr() + bufInOffset, remainder, remainder);
					// Trim mBufOut
					mBufOut->SetData(mBufOut->Offset(), outputLen);
					// don't move onto the next mBufIn in chain
					}
				// Only allocate a new buffer if required
				if (bufOut.Length())
					{			
					// move to the next mBufOut in chain
					if (mBufOut->Next() == 0)
						{
						TRAP(result, buf=RMBuf::AllocL(mBufIn->Size(), aBufAllocator));
						if (result != KErrNone)
							{
							result = EIpsec_RMBUF;		
							goto done;
							}

						mBufOut->Link(buf);
						}

					mBufOut = mBufOut->Next();
					bufOutLen = mBufOut->Length();
					bufOut.Set(mBufOut->Ptr(), 0, bufOutLen);
					}
				}
			}

		RMBuf *p;
		TInt len;

		// Process the tail end padding and next header fields
		if (!payloadChainOut.Goto(plen - KNumBytesPadLen  - esphlen, p, offset, len))
			{
			result = EIpsec_RMBUF;	// Something is not okay.
			break;
			}
		const TInt padlen = p->Buffer()[offset];

		// ... just simple optimization below: if the pad starts from the current
		// RMbuf, just adjust the offset and len, otherwise, do the heavy stuff
		// with the Goto.
		if (padlen > offset - p->Offset())
			{
			const TInt start = plen - KNumBytesPadLen - padlen - esphlen;
			if (start < 0)
				{
				result = EIpsec_EspPadLength;	// Bad pad length value
				break;
				}
			else if (!payloadChainOut.Goto(start, p, offset, len))
				{
				result = EIpsec_RMBUF;
				break;
				}
			}
		else
			{
			offset -= padlen;
			len += padlen;
			}

		unsigned char *dat = p->Buffer() + offset;	// dat -> 1st pad byte (if any) 
		for (int i = 0; i < padlen; )
			{
			if (*dat++ != ++i)
				{
				result = EIpsec_EspPadByte;	// --> Invalid pad byte
				goto done;
				}
			if (--len == 0)
				{
				p = p->Next();
				// Can't run out here, always have the pad and next header bytes!
				ASSERT(p != NULL);
				len = p->Length();
				dat = p->Ptr();
				}
			}

		ASSERT(len == 1 || len == 2);

		dat++;			// Skip the pad length byte
		if (--len == 0)
			{
			p = p->Next();
			// Can't run out here, always have the pad and next header bytes!
			ASSERT(p != NULL);
			len = p->Length();
			dat = p->Ptr();
			}
		// *dat == next header byte now!

		ASSERT(len == 1);

		// Update final bytes used for the SA and check the
		// expiration of the SA.
		aSa->iCurrent.iBytes += plen - esphlen;
		result = aSa->MarkUsed(*iManager);
		if (result != KErrNone)
			{
			aSa = NULL;
			break;					// --> Oops, SA expired during this packet, reject!
			}

		// Fixup the next header chain
		// (ESP processing does not change the iPrevNextHdr offset value!)
		aPacket.CopyIn(TPtrC8(dat, 1), aInfo.iPrevNextHdr);
		result = *dat;				// Need also return this!
		
		// Remove the ESP stuff from the packet
		plen -= (2 + padlen + esphlen);
		payloadChainOut.TrimEnd(plen);

		if (plen < 0)
			{
			result = EIpsec_CorruptPacketIn;
			break;
			}
		// The real packet length has been modified, modify the original outermost
		// payload length in the IPv6 header (only for the case where an ICMP error
		// might be generated from this packet!)
		TInet6Packet<TInet6HeaderIP4> ip(aPacket);
		if (ip.iHdr->Version() == 4)
			ip.iHdr->SetTotalLength(aInfo.iOffset + plen);
			// IPv4 header checksum not recomputed... should it? -- msa
		else
			((TInet6HeaderIP *)ip.iHdr)->SetPayloadLength(aInfo.iOffset - sizeof(TInet6HeaderIP) + plen);
		break;		// -- NO REAL LOOP, BREAK ALWAYS --
		}
done:
	CleanupStack::Pop(); // mbufOutCleanupItem
	if(!payloadChainOut.IsEmpty())
		{
		aPacket.Append(payloadChainOut);
		}

	CleanupStack::PopAndDestroy(); // mbufInCleanupItem
	
	// Reset the decryptor state before returning
	if(aSa != NULL && (aSa->iEeng) != NULL)
		{
		aSa->iEeng->Reset();
		}
	
	aInfo.iLength = aInfo.iOffset + plen;
	// *NOTE* On result < KErrNone, the payload length of the IPv6 header in
	// the packet may be incorrect.
	return result;
	}
