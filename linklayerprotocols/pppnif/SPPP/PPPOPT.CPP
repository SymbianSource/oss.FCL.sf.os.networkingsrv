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

#include "PPPBASE.H"

// Options are based on aligned MBuf Chains.
// Steps are taken to guarantee that the first 128 bytes
// of an option are held in the first MBuf

RPppOption::RPppOption()
	{
	}

RPppOption::RPppOption(RMBufChain& aChain)
//
// Note: this empties the original chain
//
	{
	Assign(aChain);
	Align(KMBufSmallSize);
	}

void RPppOption::SetL(TUint8 aType, const TAny* aPtr, TInt aLen)
	{

	aLen += 2;

	if (aLen>MaxLength())
		{
		Free();
		AllocL(aLen);
		}
	else
		SetLength(aLen);

	First()->Put(aType, 0);
	First()->Put((TUint8)aLen, 1);
	if (aLen>0)
		CopyIn(TPtrC8((TUint8*)aPtr, aLen-2), 2);
	}

void RPppOption::SetValueLength(TInt aLen)
	{
	aLen += 2;
	ASSERT(aLen<=KMBufSmallSize);
	First()->Put((TUint8)aLen, 1);
	First()->SetLength(aLen);
	}

TUint8 RPppOption::OptType() const
	{
	return First()->Get(0);	
	}

void RPppOption::SetType(const TUint8 aType)
	{
	TUint8* Ptr;

	Ptr = First()->Ptr();
	*Ptr = aType;	
	}

TInt RPppOption::ValueLength() const
	{
	return First()->Get(1)-2;	
	}

TUint8* RPppOption::ValuePtr()
	{
	return First()->Ptr()+2;
	}

const TUint8* RPppOption::ValuePtr() const
	{
	return First()->Ptr()+2;
	}

/**
Re-aligns the underlying storage in an MBuf chain
droppng any MBufs that are no longer needed
DOES NOT preserve data.
*/
TInt RPppOption::SetLength(TInt aLength)
	{
	RMBuf* p = NULL;
	RMBuf* m = First();
	while (m!=NULL && aLength>0)
		{
		TInt n = m->Size();
		aLength -= n;
		m->SetData(0, n);
		if (aLength<0)
			m->AdjustEnd(aLength);
		p = m;
		m = m->Next();
		}
	if (m!=NULL && p!=NULL)
		{
		p->Unlink();
		m->Free();
		}
	if (aLength>0)
		return KErrGeneral;
	return KErrNone;
	}

/**
Return the size of underlying storage in an MBuf chain
*/
TInt RPppOption::MaxLength() const
	{
	TInt len = 0;
	const RMBuf* m = First();
	while (m!=NULL)
		{
		len += m->Size();
		m = m->Next();
		}
	return len;
	}

void RPppOptionList::SetL(RMBufChain& aPacket, TInt aLength)
	{
	CleanupStack::PushL(*this);
	CleanupStack::PushL(aPacket);

	if (aLength==0)
		aLength = aPacket.Length();

	RMBufChain tmp;
 	while (aLength>=2)
		{
		aPacket.Align(2);
		TInt len = aPacket.First()->Get(1);
		
		// Carlson97 (p.41) suggests that option length of 0 or 1 
		// should be treated as 2 for the sake of verifying 
		// the packet integrity when doing the length checks:
		//
		// "If, however, and option has an improper Len field for that
		// type of option, but all of the lengths otherwise add up correctly, then
		// the option should, according to the RFC 1661, be included in a Configure Nak message
		// with the Len field changed to the proper length. (Dealing with a Len field set to 00 or 01 is a grey 
		// area in the standard. I recommend treating this as as though it had been 02 for the sake of verifying 
		// the packet integrity when doing the length checks; if these checks succeed, a Configure Nak should be 
		// returned with the correct Len field for those options. ...)"
		//
		// This targets cases where the option fields have correct length, but the option Len value is incorrect.
		// That is, the packet can be correctly parsed.
		// Setting option len to 2 may result in either overruning the packet length during parsing, 
		// or in parsing the packet incorrectly (incorrect option ID, values, etc). Both cases are checked for later.		 
		if (len < 2)
			len = 2;
		
		if (len > aLength) // The option extends beyond the end of the packet.
			User::Leave(KErrCorrupt);

		aPacket.SplitL(len, tmp);
		Append(aPacket);
		aPacket.Assign(tmp);
		aLength -= len;
		}
	// If there is anything left over, then the list and packet
	// are discarded as we probably have a truncated option.
	if (aLength>0)
		User::Leave(KErrCorrupt);

	CleanupStack::Pop(2);
	}

TBool RPppOptionList::IsSubsetOf(RPppOptionList& aList)
/**
Check if the current option list is a subset of the OptionsList presented (aList).

All the options in the current list must be contained and must be in the same order as the options in aList
All the option arguments must also be identical

@param aList option list to compare against.
@return ETrue if option lists match, else EFalse
*/
	{
	TMBufPktQIter iter1(*this);
	TMBufPktQIter iter2(aList);

	RPppOption opt1;
	RPppOption opt2;
	
	opt1 = iter1++;
	opt2 = iter2++;

	while (!opt1.IsEmpty())
		{
		if (opt2.IsEmpty())
			{
			return EFalse;
			}
		if (opt1.OptType() == opt2.OptType())
			{	
			if (opt1.ValueLength() == opt2.ValueLength() &&
				Mem::Compare(opt1.ValuePtr(), opt1.ValueLength(), opt2.ValuePtr(), opt2.ValueLength()) == 0) //Arguments are the same
				{
				opt1 = iter1++; //We can move on to the next option aList is incremented later
				}
			else
				{
				return EFalse; //option arguemts are different therefore comparison fails
				}
			//if the option types are different then we move on to the next option in aList
			}
		opt2 = iter2++;
		}
	return ETrue;	
	}

TBool RPppOptionList::EqualTo(RPppOptionList& aList)
/**
Check if the current option list exactly matches that of the option list specified as argument.

The options in each list must have the same type in the same order and have the same value.
If one option list has extra options at the end, then this also counts as a mismatch.
Note that aList should really be a const, but this would require propagation of the constness
to several related classes.

@param aList option list to compare against.
@return ETrue if option lists match, else EFalse
*/
	{
	TMBufPktQIter iter1(*this);
	TMBufPktQIter iter2(aList);

	RPppOption opt1;
	RPppOption opt2;
	
	opt1 = iter1++;
	opt2 = iter2++;

	while (!opt1.IsEmpty() && !opt2.IsEmpty())
		{
		if (opt1.OptType() != opt2.OptType() || opt1.ValueLength() != opt2.ValueLength() ||
			Mem::Compare(opt1.ValuePtr(), opt1.ValueLength(), opt2.ValuePtr(), opt2.ValueLength()) != 0)
			{
			return EFalse;
			}
		opt1 = iter1++;
		opt2 = iter2++;
		}

	// finally, check for the case where one option list has extra options
	
	return (opt1.IsEmpty() && opt2.IsEmpty());
	}

/**
Build an MBuf chain by copying the underlying packet queue,
leaving space on the front for a protocol header.
*/
TInt RPppOptionList::CopyL(RMBufChain& aPacket, TInt aHdrSize) const
	{
	__ASSERT_ALWAYS(aHdrSize<KMBufSmallSize, PppPanic(EPppPanic_PacketHeaderTooBig));

	RMBufChain i;

	// Get total length of chain needed
	TInt pktlen, len = 0;
	i = First();
	while (!i.IsEmpty())
		{
		len += i.Length();
		i = i.Next();
		}
	
	// Allocate the chain
	pktlen = len+aHdrSize;
	aPacket.AllocL(pktlen);
	
	// Do the copy
	TInt n, n1, n2;
	TUint8* p1, * p2;
	RMBuf* m1, * m2;

	i = First();
	m1 = i.First();
	p1 = m1->Ptr();
	n1 = m1->Length();

	m2 = aPacket.First();
	p2 = m2->Ptr();
	n2 = m2->Length();

	if (aHdrSize>0)
		{
		if (aHdrSize==KMBufSmallSize)
			{
			m2 = m2->Next();
			p2 = m2->Ptr();
			n2 = m2->Length();
			}
		else
			{
			p2 += aHdrSize;		
			n2 -= aHdrSize;
			}
		}

	while (len>0)
		{
		n = n1<n2 ? n1 : n2;
		Mem::Copy(p2, p1, n);
		if (n1 -= n, n1==0)
			{
			if (m1 = m1->Next(), m1==NULL)
				{
				i = i.Next();
				m1 = i.First();
				}
			if (m1!=NULL)
				{
				p1 = m1->Ptr();
				n1 = m1->Length();
				}
			}
		else
			p1 += n;
		if (n2 -= n, n2==0)
			{
			if (m2 = m2->Next(), m2!=NULL)
				{
				p2 = m2->Ptr();
				n2 = m2->Length();
				}
			}
		else
			p2 += n;
		len -= n;
		}

	return pktlen;	
	}

/**
For each option in aReplaceList, delete the original in this list
and replace it with the given one.
*/
TInt RPppOptionList::ReplaceOptions(RPppOptionList& aReplaceList)
	{
	RPppOption opt;
	while (aReplaceList.Remove(opt))
		{
		TInt err;
		if (err = ReplaceOption(opt), err!=KErrNone)
			return err;
		}
	return KErrNone;
	}

/**
For each option in aRejectList, delete the original in this list
*/
TInt RPppOptionList::RemoveOptions(RPppOptionList& aRejectList)
	{
	RPppOption opt;
	while (aRejectList.Remove(opt))
		{
		TInt err;
		if (err = RemoveOption(opt), err!=KErrNone)
			return err;
		opt.Free();
		}
	return KErrNone;
	}

TInt RPppOptionList::ReplaceOption(RPppOption& aOption)
//
//
//
	{
	TMBufPktQIter iter(*this);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		if (opt.OptType()==aOption.OptType())
			{
			opt.SetValueLength(aOption.ValueLength());
			Mem::Copy(opt.ValuePtr(), aOption.ValuePtr(), aOption.ValueLength());
			return KErrNone;
			}
		}
	return KErrNotFound;
	}


TInt RPppOptionList::RemoveOption(RPppOption& aOption)
//
//
//
	{
	TMBufPktQIter iter(*this);
	RPppOption opt;
	while (opt = iter.Current(), !opt.IsEmpty())
		{
		if (opt.OptType()==aOption.OptType())
			{
			opt.Init();
			iter.Remove(opt);
			opt.Free();
			return KErrNone;
			}
		iter++;
		}
	return KErrNotFound;
	}


/**
Calculate the FCS of an Option list
Used for detecting non-convergence
*/
void RPppOptionList::Crc32(TPppFcs32& aFcs, TBool aInitFcs)
	{
	if (aInitFcs)
		aFcs.Init();
	
	RPppOption opt;
	TMBufPktQIter iter(*this);
	
	while (opt = iter++, !opt.IsEmpty())
		{
		RMBuf* m = opt.First();
		while (m!=NULL)
			{
			aFcs.Calc(m->Ptr(), m->EndPtr());
			m = m->Next();
			}
		}
	}

/**
Creates an options packet from an options list

@param aPacket Return value for the constructed packet
@param aPppId Protocol (e.g. LCP, IPCP etc).
@param aType Type field of packet (e.g. Config-Request etc).
@param aId Id field of packet
@param aCreateEmptyPacket Set to ETrue if an empty packet is required.
@see RFC1661, 5.
@see MPppFsm::NewPacket
*/
void RPppOptionList::CreatePacketL(RMBufPacket& aPacket, TUint aPppId, TUint8 aType, TUint8 aId, TBool aCreateEmptyPacket)
	{
	RMBufPktInfo* info=NULL;
	TInt len;

	CleanupStack::PushL(aPacket);
	if (aCreateEmptyPacket)
		{
		// An empty packet contains just a header (Type (1) + Id (1) + Length (2))
		len = 4;
		aPacket.AllocL(len);
		}
	else
		{
	len = CopyL(aPacket, 4);
		}
	info = aPacket.NewInfoL();
	CleanupStack::Pop();

	info->iLength = len;
	TPppAddr::Cast(info->iDstAddr).SetProtocol(aPppId);
	TUint8* ptr = aPacket.First()->Ptr();
	*ptr++ = aType;
	*ptr++ = aId;
	BigEndian::Put16(ptr, (TUint16)len);
	aPacket.Pack();
	}

void RPppOptionList::CreateAndAddL(TUint8 aType)
	{
	RPppOption opt;
	opt.SetL(aType, NULL, 0);
	Append(opt);
	}

void RPppOptionList::CreateAndAddL(TUint8 aType, TUint8 aValue)
	{
	RPppOption opt;
	opt.SetL(aType, &aValue, sizeof(aValue));
	Append(opt);
	}

void RPppOptionList::CreateAndAddL(TUint8 aType, TUint16 aValue)
	{
	RPppOption opt;
	TUint8 swapped[2];
	// swap the byte order if necessary
	BigEndian::Put16(swapped, aValue);
	opt.SetL(aType, swapped, sizeof(swapped));
	Append(opt);
	}

void RPppOptionList::CreateAndAddL(TUint8 aType, TUint32 aValue)
	{
	RPppOption opt;
	TUint8 swapped[4];
	// swap the byte order if necessary
	BigEndian::Put32(swapped, aValue);
	opt.SetL(aType, swapped, sizeof(swapped));
	Append(opt);
	}

void RPppOptionList::CreateAndAddL(TUint8 aType, const TUint8 * aBuf, TInt aLen )
	{
	RPppOption opt;
	opt.SetL(aType, aBuf, aLen);
	Append(opt);
	}

void RPppOptionList::CreateAndAddL(TUint8 aType, TPtrC8& aValue )
	{
	RPppOption opt;
	opt.SetL(aType, aValue.Ptr(), aValue.Length());
	Append(opt);
	}



MPppOptionsExtender::MPppOptionsExtender()
	{
	iExtOptHandlerList.SetOffset(_FOFF(MPppOptionHandler, iOptHandlerLink));
	}

void MPppOptionsExtender::ExtOptRegister(MPppOptionHandler* aHandler)
	{
	iExtOptHandlerList.AddLast(*aHandler);
	}

void MPppOptionsExtender::ExtOptDeregister(MPppOptionHandler* aHandler)
	{
	aHandler->iOptHandlerLink.Deque();
	}

void MPppOptionsExtender::ExtOptNegotiationStarted()
	{
	TDblQueIter<MPppOptionHandler> iter(iExtOptHandlerList);
	MPppOptionHandler* handler;
	
	while (handler = iter++, handler!=NULL)
		handler->OptNegotiationStarted();
	}

void MPppOptionsExtender::ExtOptNegotiationAborted()
	{
	TDblQueIter<MPppOptionHandler> iter(iExtOptHandlerList);
	MPppOptionHandler* handler;
	
	while (handler = iter++, handler!=NULL)
		handler->OptNegotiationAborted();
	}

void MPppOptionsExtender::ExtOptNegotiationComplete()
	{
	TDblQueIter<MPppOptionHandler> iter(iExtOptHandlerList);
	MPppOptionHandler* handler;
	
	while (handler = iter++, handler!=NULL)
		handler->OptNegotiationComplete();
	}

void MPppOptionsExtender::ExtOptFillinConfigRequestL(RPppOptionList& aRequestList)
	{
	TDblQueIter<MPppOptionHandler> iter(iExtOptHandlerList);
	MPppOptionHandler* handler;
	while (handler = iter++, handler!=NULL)
		handler->OptFillinConfigRequestL(aRequestList);
	}

void MPppOptionsExtender::ExtOptCheckConfigRequest(RPppOption& aOption, RPppOptionList &aAckList, RPppOptionList &aNakList, RPppOptionList &aRejList)
	{ 
	MPppOptionHandler* handler = ExtOptLookup(aOption.OptType());
	TPppOptResponse res = (handler==NULL) ? EPppOptReject : handler->OptCheckConfigRequest(aOption);
	switch (res)
		{
	case EPppOptAck:
		aAckList.Append(aOption);
		break;
	case EPppOptNak:
		aNakList.Append(aOption);
		break;
	case EPppOptReject:
		aRejList.Append(aOption);
		break;
		}
	}

void MPppOptionsExtender::ExtOptApplyConfigRequest(RPppOption& aOption)
	{
	MPppOptionHandler* handler = ExtOptLookup(aOption.OptType());
	if (handler!=NULL)
		handler->OptApplyConfigRequest(aOption);
	}

void MPppOptionsExtender::ExtOptRecvConfigAck(RPppOption& aOption)
	{
	MPppOptionHandler* handler = ExtOptLookup(aOption.OptType());
	if (handler!=NULL)
		handler->OptRecvConfigAck(aOption);
	}

void MPppOptionsExtender::ExtOptRecvConfigNak(RPppOption& aOption, RPppOptionList& aReqList)
	{
	MPppOptionHandler* handler = ExtOptLookup(aOption.OptType());
	if (handler!=NULL)
		handler->OptRecvConfigNak(aOption, aReqList);
	}

void MPppOptionsExtender::ExtOptRecvConfigReject(RPppOption& aOption, RPppOptionList& aReqList)
	{
	MPppOptionHandler* handler = ExtOptLookup(aOption.OptType());
	if (handler!=NULL)
		handler->OptRecvConfigReject(aOption, aReqList);
	}

MPppOptionHandler* MPppOptionsExtender::ExtOptLookup(TUint8 aOptId)
	{
	TDblQueIter<MPppOptionHandler> iter(iExtOptHandlerList);
	MPppOptionHandler* handler;
	
	while (handler = iter++, handler!=NULL)
		{
		TInt i;
		for (i=0; i<handler->iNumOptions; i++)
			{
			if (handler->iOptionList[i]==aOptId)
				return handler;
			}
		}
	return NULL;
	}

MPppOptionHandler::MPppOptionHandler()
	{
	iNumOptions = 0;
	iOptionList = NULL;
	}

void MPppOptionHandler::OptRegister(MPppOptionsExtender* aExtender, const TUint8* aOptList, TInt aNumOpts)
	{
	iNumOptions = aNumOpts;
	iOptionList = aOptList;
	aExtender->ExtOptRegister(this);
	}

// Default Implementations of option handler functions

void MPppOptionHandler::OptNegotiationStarted()
	{
	return;
	}

void MPppOptionHandler::OptNegotiationAborted()
	{
	return;
	}

void MPppOptionHandler::OptNegotiationComplete()
	{
	return;
	}

void MPppOptionHandler::OptFillinConfigRequestL(RPppOptionList& /*aRequestList*/)
	{
	return;
	}

TPppOptResponse MPppOptionHandler::OptCheckConfigRequest(RPppOption& /*aOption*/)
	{
	return EPppOptReject;
	}

void MPppOptionHandler::OptApplyConfigRequest(RPppOption& /*aOption*/)
	{
	return;
	}

void MPppOptionHandler::OptRecvConfigAck(RPppOption& /*aOption*/)
	{
	return;
	}

void MPppOptionHandler::OptRecvConfigNak(RPppOption& aOption, RPppOptionList& aReqList)
	{
	aReqList.ReplaceOption(aOption);
	}

void MPppOptionHandler::OptRecvConfigReject(RPppOption& aOption, RPppOptionList& aReqList)
	{
	aReqList.RemoveOption(aOption);
	}
