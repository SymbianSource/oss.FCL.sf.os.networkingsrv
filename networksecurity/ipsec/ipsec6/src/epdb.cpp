// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// epdb.cpp - IPSEC Endpoint Database implementation
// @internalComponent	for IPSEC
//

#include "sa_spec.h"
#include "ipaddress.h"
#include "epdb.h"
#include "ipseclog.h"

class CEndPoint : public CIpsecReferenceCountObject
	/**
	* Storage location for internet address with scope id.
	*
	* This object is always a member of some collection of
	* addresses via the RCircularList. The collection may
	* contain only this object. The membership in the
	* collection does not required reference counting -- when
	* object is deleted, it is removed from the collection
	* (and added to a collection containing only the self).
	*/
	{
protected:
	~CEndPoint();
public:
	CEndPoint(const TIp6Addr &aAddr, const TUint32 aScope);
	CEndPoint(RCircularList &aList, TUint aLength);
	static CEndPoint *New(RCircularList &aList, const TDesC &aName);
	static CEndPoint &Cast(RCircularList &aList);
	TBool MatchingName(const TDesC &aName);

	RCircularList iList;	//< Maintain a collection of addresses.
	TIpAddress iAddr;		//< Address and scope id.

	//
	// *WARNING* *WARNING* *WARNING*
	// What now follows, is the TLitC8 structure.
	// The extra space is allocated only, if iTypeLength
	// is non-zero.
	//
	// Why this TLitC8 "hack" instead of traditional
	// C construct with a "length" member and "fake buf[1]"?
	//
	// As far as layout, this is exactly the same. The TLitC8
	// "hack" only forces a Symbian specific layout. When a
	// descriptor is needed, it doesn't need to be constructed,
	// it's already existing and just returning a reference
	// to iTypeLength as TLitC8 is sufficient.
	//
	const TUint iTypeLength;

	inline const TDesC8 &Name() const { return ((TLitC8<1> &)iTypeLength)(); }
	inline TBool IsNamed() const { return iTypeLength != 0; }
	};

//
// CEndPoint implementation //
//

CEndPoint::CEndPoint(const TIp6Addr &aAddr, const TUint32 aScope) : iAddr(aAddr, aScope), iTypeLength(0)
	{
	}

CEndPoint::CEndPoint(RCircularList &aList, TUint aLength) : iTypeLength(aLength)
	{
	aList.Attach(iList);
	}

CEndPoint::~CEndPoint()
	{
	iList.Detach();
	}

CEndPoint &CEndPoint::Cast(RCircularList &aList)
	{
	return *((CEndPoint *)((char *)(&aList) - _FOFF(CEndPoint, iList)));
	}


CEndPoint *CEndPoint::New(RCircularList &aList, const TDesC &aName)
	{
	const TUint len = aName.Length();
	CEndPoint *ep = new (len) CEndPoint(aList, len);
	if (ep)
		{
		// See _LIT and TLitC for this initializer!!!
		TPtr8(&((TLitC8<1> &)ep->iTypeLength).iBuf[0], len).Copy(aName);
		}
	return ep;
	}


TBool CEndPoint::MatchingName(const TDesC &aName)
	/**
	* Test if the end point name matches.
	*
	* This is not simple compare, but
	*
	* - unnamed end point (name length == 0) does not match empty aName
	* - comparison is made only between lower 8 bits of each character (name should only contain ASCII!)
	*
	* @param aName The name to be compared.
	*/
	{
	TInt i = aName.Length();
	if (iTypeLength == 0 || (TUint)i != iTypeLength)
		return EFalse;
	const TText *s = aName.Ptr();
	const TText8 *p = Name().Ptr();
	while (--i >= 0)
		{
		if (s[i] != p[i])
			return EFalse;
		}
	return ETrue;
	}

//
// RIpAddress implementation //
//

RIpAddress::RIpAddress(const RIpAddress &aAddr)
	/**
	* Copy constructor.
	*
	* Update the reference count of the copied object.
	*/
	{
	if ((iAddr = aAddr.iAddr) != NULL)
		iAddr->Open();
	}

RIpAddress & RIpAddress::operator=(const RIpAddress &aAddr)
	/**
	* Assign operator.
	*
	* Close previous content and update the reference count
	* of the assigned object.
	*/
	{
	if (this != &aAddr)	// Only need do something if not assigning to self.
		{
		Close();
		if ((iAddr = aAddr.iAddr) != NULL)
			iAddr->Open();
		}
	return *this;
	}


RIpAddress::~RIpAddress()
	/**
	* Destructor is implicit Close.
	*/
	{
	Close();
	}

void RIpAddress::Close()
	/**
	* Detach address from the handle.
	*/
	{
	if (iAddr)
		{
		iAddr->Close();
		iAddr = NULL;
		}
	}

TInt RIpAddress::Open(REndPoints &aMgr, const TDesC &aName)
	/**
	* Find an existing named end point and attach.
	*
	* @param	aMgr	The EndPoint Collection
	* @param	aName	The EndPoint Name
	* 
	* @return KErrNone on success, and KErrNotFound if endpoint is not defined.
	*/
	{
	Close();	// Detach previous, if any present.
	TCircularListIter iter(aMgr);
	RCircularList *r;
	while ((r = iter++) != NULL)
		{
		CEndPoint &p = CEndPoint::Cast(*r);
		if (p.MatchingName(aName))
			{
			// An additional reference to an existing named
			// address, the reference count needs to be incremented.
			p.Open();
			iAddr = &p;
			return KErrNone;
			}
		}
	return KErrNotFound;
	}

TInt RIpAddress::Open(REndPoints &aMgr, const TIpAddress &aAddr)
	/**
	* Find or create unnamed end point and attach.
	*
	* @param	aMgr	The EndPoint Collection
	* @param	aAddr	The EndPoint address
	*
	* @return	KErrNone when success, and KErrNoMemory if end point allocation fails.
	*/
	{
	Close();	// Detach previous, if any present.
	TCircularListIter iter(aMgr);
	RCircularList *r;
	while ((r = iter++) != NULL)
		{
		CEndPoint &p = CEndPoint::Cast(*r);

		// *note* currently TIpAddress == operator treats iScope==0 as
		// a wild card. That is not the correct semantic here.
		// The values need to match bit by bit..
		if (!p.IsNamed () && p.iAddr.IsEqual(aAddr) && p.iAddr.iScope == aAddr.iScope)
			{
			// An additional reference to an existing unnamed
			// address, the reference count needs to be incremented.
			p.Open();
			iAddr = &p;
			return KErrNone;
			}
		}
	// Create a new unnamed address. The initial reference
	// from the contruction corresponds to the RIpAddress
	// handle. The membership in the circular list is not
	// counted as a reference.
	iAddr = new CEndPoint(aMgr, 0);
	if (iAddr == NULL)
		return KErrNoMemory;
	iAddr->iAddr = aAddr;
	return KErrNone;
	}
	
TInt RIpAddress::Open(REndPoints &aMgr, const TInetAddr &aAddr)
	{
	TIpAddress addr(aAddr);
	return Open(aMgr, addr);
	}

TInt RIpAddress::Open(REndPoints &aMgr, const TDesC &aName, const TIpAddress &aAddr, TInt aOptional)
	/**
	* Find or create named end point and attach.
	*
	* Search for the named enpoint and attach it if found. If not found,
	* create a new end point into the end point collection.
	*
	* The aAddr is the initial value for the new end point. If the end
	* already exists, the content is changed to aAddr, if aOptional is
	* non-zero. Otherwise old end point content is not changed.
	*
	* @param	aMgr	The EndPoint Collection
	* @param	aName	The EndPoint Name
	* @param	aAddr	The new address of the end point (see aOptional)
	* @param	aOptional	Controls whether existing end point address is overwritten.
	*
	* @return	KErrNone if success, and KErrNoMemory if endpoint cannot be allocated.
	*/
	{
	if (Open(aMgr, aName) != KErrNone)
		{
		// Create a new named end point. The initial reference
		// from the contruction corresponds to the RIpAddress
		// handle. The membership in the circular list is not
		// counted as a reference.
		CEndPoint *p = CEndPoint::New(aMgr, aName);
		if (p == NULL)
			return KErrNoMemory;
		iAddr = p;
		aOptional = 0;
		}
	if (!aOptional)
		{
		iAddr->iAddr = aAddr;
		}
	return KErrNone;
	}



TInt RIpAddress::Set(const TIp6Addr &aAddr, TUint32 aScopeId)
	/**
	* Set content to given internet address.
	*
	* This cannot be used to change address content of an
	* existing named or internal end point. If there is an
	* existing CEndPoint which is shared by someone else
	* (reference count > 0 or a member of collection
	* with other objects), then this detaches from that
	* end point and allocates a new private CEndPoint
	* for the handle.
	*
	* This method is tailored for fast operation when
	* the address information from the packet or flow
	* needs to be packed into RIpAddress. By preallocating
	* this handle and never sharing the content, the
	* Set operation is a simple address copy and no
	* memory allocation.
	*
	* @param aAddr The raw IPv6 address
	* @param aScopeId The scope id of the address
	*
	* @return KErrNone or KErrNoMemory.
	*/
	{
	if (iAddr == NULL || iAddr->IsShared())
		{
		// The End Point does not exist, or is shared in some way. Need to
		// allocate a new object for this handle.

		Close();		// Release previous object.
		iAddr = new CEndPoint(aAddr, aScopeId);
		if (iAddr == NULL)
			return KErrNoMemory;
		}
	else
		{
		if (!iAddr->iList.IsDetached())
			{
			// The object is a member of non-empty collection. However,
			// because this reference is the only reference to it,
			// closing would only delete the object. Might as well
			// just detach it from the collection and reuse the
			// CEndPoint as is. [this is "just in case" branch --
			// the normal intended usage pattern should not make
			// CEndPoint of this handle to be a member of any
			// collection!]
			iAddr->iList.Detach();
			}
		iAddr->iAddr.SetAddress(aAddr, aScopeId);
		}
	return KErrNone;
	}

TInt RIpAddress::Set(const TInetAddr &aAddr)
	/**
	* Set content to given internet address.
	*
	* See the other Set.
	*
	* @param aAddr The internet address in IPv6 format.
	*/
	{
	return Set(aAddr.Ip6Address(), aAddr.Scope());
	}

const TIpAddress& RIpAddress::operator()() const
	/**
	* Returns a reference to the contained IP address.
	*
	* This reference is only valid as long as handle is open. It must not
	* be used after the handle is closed.
	*
	* Always succeeds, and returns all zeroes address, if handle is not open.
	*
	* @return	The Address.
	*/
	{
	// This is actually
	//		static const TIpAddress none;
	// but which cannot be used because TIpAddress has a constructor
	// and it would result "ipsec6.prt has initialized data" error
	// for target build.
	static const char none[sizeof(TIpAddress)] = {0 };

	return iAddr ? iAddr->iAddr : reinterpret_cast<const TIpAddress &>(none);
	}

TBool RIpAddress::IsNamed() const
	{
	return iAddr ? iAddr->IsNamed() : EFalse;	
	}

const TDesC8 &RIpAddress::Name() const
	{
	return IsNamed() ? iAddr->Name() : KNullDesC8();
	}


//
// REndPoints implementation //
//
//
// Debug only //
//

#ifdef _LOG

class TAddressBuf : public TBuf<70>
	{
public:
	TAddressBuf(const TIpAddress &aAddr);
	};

TAddressBuf::TAddressBuf(const TIpAddress &aAddr)
	{
	TInetAddr addr(aAddr, 0);
	addr.SetScope(aAddr.iScope);
	addr.OutputWithScope(*this);
	}

void REndPoints::LogPrint(const TDesC &aFormat) const
	/**
	* Print the current set of end points into IPSEC Log file.
	*
	* The format string must contain exactly two %S format
	* controls. The first is used for the name and the second
	* for the currently assigned address.
	*
	* @param aFormat	The print format.
	*/
	{	
	_LIT(KUnnamed, "<internal>");

	TCircularListIter iter(*this);
	RCircularList *r;
	while ((r = iter++) != NULL)
		{
		const CEndPoint &n = CEndPoint::Cast(*r);
		TAddressBuf addr(n.iAddr);
		if (n.IsNamed())
			{
			TBuf<100> name;
			name.Copy(n.Name());
			Log::Printf(aFormat, &name, &addr);		
			}
		else
			Log::Printf(aFormat, &KUnnamed(), &addr);
		}
	}
#endif

