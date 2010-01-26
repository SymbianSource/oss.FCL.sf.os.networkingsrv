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
// Implementation CLanxBearer class, a derived from CNifIfBase.
// History
// 15/11/01 Started by Julian Skidmore. 
// 00572     RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
// 00573     TUint prot = TPppAddr::Cast(info->iDstAddr).GetProtocol();
// 00574 
// 
//

/**
 @file
*/

#include <in_sock.h> // Header is retained, but in_sock.h is modified for ipv6
#include <in_iface.h> // Gone.
#include "CLanxBearer.h"
#include "EthProto.h"

using namespace ESock;

/**
Constructor. Initialises the link layer object iLink  and the notifier iNotify (inherited 
from CNIfIfBase) to aLink and aNotify respectively.
@param aLink A pointer to the Link Layer object.
@param aNotify A pointer to the notifier.
*/
CLanxBearer::CLanxBearer(CLANLinkCommon* aLink):iLink(aLink), iSoIfConnectionInfoCached(false)
{
}

/**
ConstructL method. Does nothing (CLanxBearer has no memory allocating objects).
*/
void  CLanxBearer::ConstructL()
{
}

/**
StartSending notifies the protocol that this object is ready to transmit and process data. 
CLanxBearer provides a default implementation which calls the iProtocol's StartSending method, 
passing this as input.
@param aProtocol A pointer to the object which signalled it is ready to StartSending.
*/
void CLanxBearer::StartSending(CProtocolBase* /*aProtocol*/)
	{
	// Default implementation.
	iUpperControl->StartSending();
	}

void CLanxBearer::UpdateMACAddr()
	{	
	} // default implementation

// MLowerControl methods

TInt CLanxBearer::GetName(TDes& aName)
/**
Return the interface name

@param aName Out parameter to return the name
@return KErrNone
*/
	{
	aName.Copy(iIfName);
	return KErrNone;
	}

	
TInt CLanxBearer::BlockFlow(MLowerControl::TBlockOption /*aOption*/)
	{
	return KErrNotSupported;
	}

//
// Utilities
// 

void CLanxBearer::SetUpperPointers(MUpperDataReceiver* aReceiver, MUpperControl* aControl)
	{
	ASSERT(iUpperReceiver == NULL && iUpperControl == NULL);
	iUpperReceiver = aReceiver;
	iUpperControl = aControl;
	}

TBool CLanxBearer::MatchesUpperControl(const ESock::MUpperControl* aUpperControl) const
/**
Check whether the passed MUpperControl matches that associated with the current instance
*/
	{
	ASSERT(iUpperControl);
	return iUpperControl == aUpperControl;
	}
 
