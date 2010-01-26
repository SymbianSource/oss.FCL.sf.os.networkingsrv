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
// in_bind.cpp - inet protocol base
//

#include "in_bind.h"

//
//  CProtocolInet6Base
//
//


EXPORT_C TInt CProtocolInet6Binder::DoBindTo(CProtocolBase *aProtocol)
	/**
	* Performs the BindToL processing for the IP binding.
	*
	* This method exists for derived classes that override the default
	* CProtocolInet6Binder::BindToL for their own processing. They can first
	* call DoBindTo to handle the network bind, and then handle other binds.
	* If the bound protocol is a network, the CProtocolInet6Binder::iNetwork
	* is initialized and this protocol is bound to the network using
	* the BindL(this, id) function, where id is the value of the
	* TServerProtocolDesc::iProtocol field from this protocol. This must
	* be in range [1..255] and it maps directly the to protocol field
	* (next header in IPv6) of the IP header (TInet6HeaderIP and
	* TInet6HeaderIP4).
	*
	* Only one network bind is accepted. If protocol is bound
	* to multiple protocols which have either #KProtocolInet6Ip
	* or #KProtocolInetIp as their TServerProtocolDesc::iProtocol,
	* then the first one used in DoBindTo is the effective one,
	* the remaining ones are returned as if bind wast to some
	* other protocol.
	*
	* @param aProtocol
	*	This protocol is recognized as a "network", if the
	*	TServerProtocolDesc::iProtocol field contains the
	*	special value KProtocolInet6Ip (or KProtocolInetIp).
	*
	* @return
	* @li	= 0 (= KErrNone),
	*	It was the network bind, iNetwork
	*	has a non-NULL value and network layer can be accessed.
	* @li	> 0 (= protocol id value from aProtocol's
	*	TServerProtocolDesc::iProtocol).
	*	aProtocol was not the network (or network was already
	*	attached). It is up to the caller to decide what to do
	*	with this protocol.
	* @li	< 0 (= error code)
	*	An error detected.
	*
	* @note
	*	Because the return value is used for both error indication
	*	(= negative return), and for unprocessed protocol number, the
	*	implementation prevents use of protocol numbers that would
	*	be negative numbers when treated as signed integers. If such
	*	is required, the derived class must handle them by other
	*	means.
	*
	* Example, assuming your ESK contains
@verbatim
[upper]
filename= upper.prt
index= 1
bindto= ip6,other
@endverbatim
	* the socket server will call your BindToL twice, once for
	* 'ip6' and once for 'other'. If your 'upper' protocol does
	* not override the BindToL function, then the default implementation
	* of CProtocolInet6Binder handles the 'ip6' case, but leaves
	* with 'other'. To handle the above case, your overriding
	* BindToL could look as follows:
@code
CProtocolUpper::BindToL(CProtocolBase *aProtocol)
	{
	TInt id = DoBindTo(aProtocol);
	if (id == your_other_protocol_id)
		{
		// handle your bind to other, for example
		iOther = aProtocol;
		iOther->Open();
		iOther->BindL(this, your_id_or_something_else);
		}
	else if (id != KErrNone)
		{
		// Not ip6 or 'other
		User::Leave(KErrNotSupported);
		}
	}

CProtocolUpper::~CProtocolUpper
	{
	if (iOther)
		{
		// iOther->Unbind(this); // maybe needed, depends on 'other'
		iOther->Close();
		iOther = NULL;
		}
	}
@endcode
	*/
	{
	ASSERT(this != aProtocol);

	TServerProtocolDesc info;
	aProtocol->Identify(&info);
	// here bind_id is declared as TInt (because it is used as
	// return value). However, in info they are TUint.
	// A confusion may result if someone speficies "negative" protocol
	// ids -- msa
	TInt bind_id = (TInt)info.iProtocol;
	if (bind_id < 0)
		return KErrNotSupported;

	if (iNetwork == NULL &&
		(bind_id == (TInt)KProtocolInetIp || bind_id == (TInt)KProtocolInet6Ip))
		{
		//
		// The first network bind detected
		//
		iNetwork = ((CProtocolInet6Binder *)aProtocol)->NetworkService();
		aProtocol->Open();
		// If anything after this fails, the matching Unbind/Close()
		// will happen in the destructor... (Unbind is safe to call,
		// even if there is no matching succesful BindL).
		Identify(&info);
		TRAP(bind_id, aProtocol->BindL(this, info.iProtocol));
		}
	return bind_id;
	}

EXPORT_C void CProtocolInet6Binder::BindToL(CProtocolBase *aProtocol)
	/**
	* Bind to another protocol.
	*
	* This default implementation only handles the network bind. The
	* implementation is just a call to CProtocolInet6Binder::DoBindTo
	* and a leave with KErrNotSupported, if the protocol was not
	* the network (= IPv6) instance.
	*
	* If this handling is not sufficient, the derived class must
	* override this method and call first the CProtocolInet6Binder::DoBindTo
	* for network bind, and then handle the other allowed bindings.
	*
	* @param aProtocol
	*	Protocol to bind (see CProtocolInet6Binder::DoBindTo)
	*/
	{
	TInt err = DoBindTo(aProtocol);
	if (err == KErrNone)
		return;
	if (err > 0)
		err = KErrNotSupported;
	User::Leave(err);
	}

EXPORT_C CProtocolInet6Binder::~CProtocolInet6Binder()
	/**
	* Destructor.
	*
	* This handles unbinding from the network (iNetwork), if
	* it has been bound.
	*/
	{
	if (iNetwork != NULL)
		{
		CProtocolInet6Binder *const prt = iNetwork->Protocol();
		iNetwork = NULL;
		prt->Unbind(this);
		prt->Close();
		}
	}

//
// Provide default for the name services through the MNetworkService
//

EXPORT_C CHostResolvProvdBase* CProtocolInet6Binder::NewHostResolverL()
	/**
	* Gets the default name services provider from the network layer.
	* @return Default name services provider
	*/
	{
	if (iNetwork == NULL)
		User::Leave(KErrNotSupported);
	return iNetwork->NewHostResolverL();
	}

EXPORT_C CServiceResolvProvdBase* CProtocolInet6Binder::NewServiceResolverL()
	/**
	* Gets the default service resolver provider from the network layer.
	* @return Default service resolver provider
	*/
	{
	if (iNetwork == NULL)
		User::Leave(KErrNotSupported);
	return iNetwork->NewServiceResolverL();
	}


EXPORT_C CNetDBProvdBase* CProtocolInet6Binder::NewNetDatabaseL()
	/**
	* Gets the default net database provider from the network layer.
	* @return Default net database provider
	*/
	{
	if (iNetwork == NULL)
		User::Leave(KErrNotSupported);
	return iNetwork->NewNetDatabaseL();
	}
