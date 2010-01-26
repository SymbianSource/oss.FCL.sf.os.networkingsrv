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
 
#include <nifman.h>
#include <comms-infras/nifprvar.h>
#include "PKTDRV.H"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/pktdrv_internal.h>
#endif

EXPORT_C void CPktDrvFactory::Close()
{
	Dec();
	if (AccessCount()==0)
		{
		RLibrary lib;
		lib.SetHandle(iLib.Handle());
		iLib.SetHandle(0);
		delete this;
		lib.Close();
		}
}
  
void CPktDrvFactory::Install( RLibrary& aLib )
{
	iLib.SetHandle(aLib.Handle());
	aLib.SetHandle(0);
}


/**
Constructor.
@param aFactory A reference to CPktDrvFactory class.
*/
EXPORT_C CPktDrvBase::CPktDrvBase(CPktDrvFactory& aFactory)
{
	iFactory=&aFactory;
	iFactory->Open();
}

/**
Destructor.
*/
EXPORT_C CPktDrvBase::~CPktDrvBase()
{
	if (iFactory)
		iFactory->Close();
	delete iName;
}

/**
Set the name for the factory function.
*/
EXPORT_C void CPktDrvBase::SetNameL(const TDesC& aName)
{	
	iName = aName.AllocL();
}

/**
TIeee802Addr
*/
EXPORT_C TIeee802Addr::TIeee802Addr()
{
	Reset();
}

/**
Constructs a device address from a data buffer.
The buffer is copied directly into the object. The function panics if aDes 
does not have a length of 6 bytes (48 bits).
@param aDes Data buffer for device address 
*/
EXPORT_C TIeee802Addr::TIeee802Addr(const TDesC8& aDes)
{
	__ASSERT_ALWAYS(aDes.Length() == KIeee802AddrSize, Panic(EIeee802AddrBadDescriptor));
	Mem::Copy(&iAddr[0], aDes.Ptr(), KIeee802AddrSize);
}

/**
Constructs a device address from a TInt64.
The function panics if the most significant 16 bits of aInt are non-zero, 
as device addresses are 48 bits in size. 
@param aInt Value for device address.
*/
EXPORT_C TIeee802Addr::TIeee802Addr(const TInt64& aInt)
{
	__ASSERT_ALWAYS((I64HIGH(aInt) & 0xffff0000u) == 0, Panic(EIeee802AddrBadTInt64));
	TInt64 int64 (aInt);
	for (TInt i = KIeee802AddrSize-1; i>=0; --i)
		{
		iAddr[i] = TUint8(I64LOW(int64));
		int64 >>= 8;
		}
}
	
EXPORT_C void TIeee802Addr::Reset()
{
	iAddr.Reset();
}
	
/**
TEthernetAddr
*/
EXPORT_C TEthernetAddr::TEthernetAddr() :
	TIeee802Addr()
{
}

EXPORT_C TEthernetAddr::TEthernetAddr(const TInt64& aInt) :
	TIeee802Addr(aInt)
{
}

EXPORT_C TEthernetAddr::TEthernetAddr(const TDesC8& aDes) :
	TIeee802Addr(aDes)
{
}
	
/**
Global panic fn
*/
GLDEF_C void Panic(TIeee802AddrPanics aCode)
{
	_LIT(KPanicName, "IEEE802Addr");
	User::Panic(KPanicName, aCode);
}
