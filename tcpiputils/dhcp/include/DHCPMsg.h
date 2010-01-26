// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// DHCP Message wrapper header file
// 
//

/**
 @file DHCPMsg.h
*/

#ifndef __DHCPMSG_H__
#define __DHCPMSG_H__

#include <e32base.h>
#include <comms-infras/recorditem.h>

class CDHCPMessageHeader : public CConstItem
	{

protected:
	CDHCPMessageHeader(CItemBase* aNext, TInt aValue, HBufC8*& aMsg) :
		CConstItem(aNext, aValue),
		iRecord(NULL),
		iMsg(aMsg)
		{
		iRecord.iFirst = this; //to get rid of too cautious warning C4355
   	}

public:
	HBufC8& Message();
	void Dump();
	void InitialiseL();
	virtual void RemoveAllOptions() = 0;

public:
	TRecord iRecord;

protected:
	HBufC8*& iMsg;	//< Not owned by CDHCPMessageHeader
	};

#ifndef __FLOG_ACTIVE
inline void CDHCPMessageHeader::Dump()
	{
	}
#endif

inline HBufC8& CDHCPMessageHeader::Message()
/**
  * Return the descriptor of message contents
  *
  * @internalTechnology
  *
  * @return TDesC8 - The message contents
  *
  */
	{
	return *iMsg;
	}


#endif

