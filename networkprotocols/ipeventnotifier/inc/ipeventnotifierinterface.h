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
//

/**
 @file ipeventnotifierinterface.h
 @internalComponent
*/


#ifndef __CIPEVENTNOTIFIERINTERFACE_H__
#define __CIPEVENTNOTIFIERINTERFACE_H__


using namespace IPEvent;

class IPEvent::CMFlagReceived;
class IPEvent::CIPReady;
class IPEvent::CLinklocalAddressKnown;

class CIPEventNotifierInterface : public CBase
	{
public:
	CIPEventNotifierInterface(TUint32 aIndex, const TDesC& aName) :
		iInterfaceIndex(aIndex),
		iInterfaceName(aName),
		iRefCount(0)
		{}

	inline TUint32 GetInterfaceIndex(void) const {return iInterfaceIndex;}
	inline TName   GetInterfaceName(void)  const {return iInterfaceName;}

	inline TInt GetRefCount(void) const {return iRefCount;}

	inline void IncreaseRefCount(void) { ++iRefCount; }
	inline void DecreaseRefCount(void) { --iRefCount; }

	inline ~CIPEventNotifierInterface()
		{
		if(iMFlagReceived) delete iMFlagReceived;
		if(iIPReady) delete iIPReady;
		if(iLinklocalAddressKnown) delete iLinklocalAddressKnown;
		}

public:

	// Pointers to the last received instance of each event message class. So they're only constructed once
	CMFlagReceived* iMFlagReceived;
	CIPReady* iIPReady;
	CLinklocalAddressKnown* iLinklocalAddressKnown;
	
private:
	TUint32 iInterfaceIndex;
	TName   iInterfaceName;

	TInt iRefCount;

	};


#endif // __CIPEVENTNOTIFIERINTERFACE_H__



