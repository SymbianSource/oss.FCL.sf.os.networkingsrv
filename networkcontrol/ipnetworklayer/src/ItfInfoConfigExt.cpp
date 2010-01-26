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
//

/**
 @file 
 @internalTechnology
*/

#include "ItfInfoConfigExt.h"
#include <comms-infras/metatype.h>

using namespace Meta;

START_ATTRIBUTE_TABLE(TItfInfoConfigExt, KIpProtoConfigExtUid, EItfInfoConfigExt)
    // No attributes registered as no serialisation takes place.
END_ATTRIBUTE_TABLE()

START_ATTRIBUTE_TABLE(XInterfaceNames, KIpProtoConfigExtUid, EInterfaceName)
	// No attributes registered as no serialisation takes place.
END_ATTRIBUTE_TABLE()

const TUint KMaxInterfaceNames = 10;

XInterfaceNames* XInterfaceNames::NewL()
	{
	XInterfaceNames* self = new(ELeave) XInterfaceNames();
	
	CleanupStack::PushL(self);
	self->ConstructL();	
	CleanupStack::Pop(self);
	
	return self;
	}

void XInterfaceNames::ConstructL()
	{
	iInterfaceNames.ReserveL(KMaxInterfaceNames);
	User::LeaveIfError(iLock.CreateLocal());
	}

XInterfaceNames::~XInterfaceNames()
	{
	iInterfaceNames.Close();
	iLock.Close();	
	}

/**
	Retrieves an interface name from the store.
	
	@param aIndex The index of the interface name to retrieve.
	@param aInterfaceName The buffer to be populated with the name of the interface.
*/
TInt XInterfaceNames::InterfaceName(TUint aIndex, TInterfaceName& aInterfaceName)
	{
	iLock.Wait();
		{	
		if(aIndex >= iInterfaceNames.Count() || aIndex >= KMaxInterfaceNames)
			{
			iLock.Signal();
			return KErrArgument;
			}
		aInterfaceName = iInterfaceNames[aIndex].iName;
		}
	iLock.Signal();
	
	return KErrNone;
	}

/**
	Adds an interface name to the store.
	
	@param aInterfaceName The name of the interface.
	@leave A system-wide error code if the interface name could not be stored.
*/
void XInterfaceNames::AddInterfaceNameL(const TAny* aOwner, const TInterfaceName& aInterfaceName)
	{
	TInt ret = KErrNone;
	
	iLock.Wait();
		{
		if(iInterfaceNames.Count() >= KMaxInterfaceNames)
			{
			iLock.Signal();
			User::Leave(KErrOverflow);
			}
		
		ret = iInterfaceNames.Append(TNameAndOwner(aOwner, aInterfaceName));
		}
	iLock.Signal();
	
	User::LeaveIfError(ret);
	}
	
/**
	Removes all interfacesName entries matching the supplied owner.
	
	@param aOwner The owner of the entry
*/
void XInterfaceNames::RemoveInterfaceName(const TAny* aOwner)
	{
	iLock.Wait();
		{
		for(TUint i=0;i<iInterfaceNames.Count();i++)
			{
			if(iInterfaceNames[i].iOwner == aOwner)
				{
				iInterfaceNames.Remove(i);
				}
			}
		}
	iLock.Signal();
	}
