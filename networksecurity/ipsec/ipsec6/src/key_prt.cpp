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
// key_prt.cpp - IPv6/IPv4 IPSEC PFKEY protocol
//

#include "ipsec.h"
#include "sadb.h"
#include "sa_crypt.h"
#include <networking/pfkeyv2.h>
#include "pfkey.h"
#include "ipseclog.h"
#include <networking/in6_ipsec.h>
#include <es_prot_internal.h>


CProtocolBase *IPSEC::NewPfkeyL()
	/**
	* Return new instance of PFKEY protocol object.
	*
	* @return The PFKEY protocol.
	*/
	{
	return new (ELeave) CProtocolKey();
	}

void IPSEC::IdentifyPfkey(TServerProtocolDesc &aEntry)
	/**
	* Return PFKEY protocol description
	*
	* @retval aEntry The description
	*/
	{
	_LIT(Kpfkey, "pfkey");

	aEntry.iName = Kpfkey;
	aEntry.iAddrFamily = KAfIpsec;
	aEntry.iSockType = KSockRaw;
	aEntry.iProtocol = KProtocolKey;
	aEntry.iVersion = TVersion(1, 0, 0);
	aEntry.iByteOrder = ELittleEndian;
	aEntry.iServiceInfo = KSIConnectionLess | KSIMessageBased;
	aEntry.iNamingServices = 0;
	aEntry.iSecurity = KSocketNoSecurity;
	aEntry.iMessageSize = 0xffff;
	aEntry.iServiceTypeInfo = ESocketSupport | ENeedMBufs;
	aEntry.iNumSockets = KUnlimitedSockets;
	}

MAssociationManager *IPSEC::FindAssociationManager(const CProtocolBase *aProtocol, TUint aId)
	/**
	* Return MAssociationManager, if aProtocol is CProtocolKey instance.
	*
	* @param aProtocol	The protocol instance
		* @param aId The protocol Id
	* @return NULL, if aProtocol is not CProtocolKey, otherwise return MAssociationManager
	*/
	{
	if (aId == KProtocolKey)
		return (CProtocolKey *)aProtocol;
	return NULL;
	}

#pragma warning (disable:4355)
CProtocolKey::CProtocolKey() : iEngineAH(this), iEngineESP(this), iEngineNATT(this)
	/**
	* Constructor.
	*/
	{
	iSAPlist.SetOffset(_FOFF(CProviderKey,iSAPlink));
	}
#pragma warning (default:4355)


void CProtocolKey::Open()
	/**
	* Increase reference count.
	*
	* The specific implementation exists only, because MAssociationManager::Open()
	* must be mapped CProtocolBase::Open.
	*/
	{
	CProtocolBase::Open();
	}

void CProtocolKey::Close()
	/**
	* Descrease reference count.
	*
	* The specific implementation exists only, because MAssociationManager::Close()
	* must be mapped CProtocolBase::Close.
	*/
	{
	CProtocolBase::Close();
	}


CServProviderBase* CProtocolKey::NewSAPL(TUint aSockType)
	/**
	* Create new PFKEY SAP.
	*
	* @param aSockType == KSockRaw
	* @return CProviderKey instance
	*/
	{
	if (aSockType!=KSockRaw)
		User::Leave(KErrNotSupported);
	CProviderKey *pSAP = new (ELeave) CProviderKey(*this);
	iSAPlist.AddLast(*pSAP);
	return pSAP;
	}

CProtocolKey::~CProtocolKey()
	/**
	* Destructor.
	*/
	{
	// release End Points recorded by SetOpt
	for (TInt i = iEndPoints.Count(); --i >= 0; )
		{
		iEndPoints[i].Close();
		}
	iEndPoints.Close();

	delete iCrypto;		// Relese crypto libraries
	delete iTimer;		// Disconnect Timer system.
	//
	// Release Security Associations
	//
	for (TInt j = 0; j < HashSize(); ++j)
		while (iHash[j])
			{
			CSecurityAssoc *sa = iHash[j];
			iHash[j] = sa->iNext;
			delete sa;
			}
	// If assumptions are correct, iSAPlist should be empty!!
	ASSERT(iSAPlist.IsEmpty());
#if __WINS__
	LOG(Log::Printf(_L("CProtocolKey::~CProtocolKey() done IPSEC_OBJECT_COUNT=%d"), IPSEC_OBJECT_COUNT));
#endif
	}

void CProtocolKey::InitL(TDesC& /*aTag*/)
	/**
	* Initialize.
	*
	* Initialize the CProtocolKey. Create cryptographic library manager
	* and timer handler.
	*/
	{
	iCrypto = CIpsecCryptoManager::NewL();
	MAssociationManager *const mgr = this;
	iTimer = TimeoutFactory::NewL(1, mgr);
	}

// Unnecessary function, could use the default in CProtocolBase
void CProtocolKey::StartL()
	/**
	* StartL has nothing to do.
	*/
	{
	}

void CProtocolKey::BindToL(CProtocolBase* aProtocol)
	/**
	* Bind requests protocols "below".
	*
	* The PFKEY allows here only cryptographic libraries, which are
	* recognized from their protocol number #KProtocolCrypto .
	*
	* @param aProtocol The Cryptographic library.
	*/
	{
	ASSERT(this != aProtocol);

	// Find out the ID of given protocol object
	TUint id;
		{
		TServerProtocolDesc info;
		aProtocol->Identify(&info);
		id = info.iProtocol;
		}

	if (id == STATIC_CAST(TUint, KProtocolCrypto))
		iCrypto->AddLibraryL((CProtocolCrypto *)aProtocol);
	else
		User::Leave(KErrGeneral);	// Only Crypto libraries can be bound to PFKEY!
	}


void CProtocolKey::BindL(CProtocolBase * /*aProtocol*/, TUint /*id*/)
	/**
	* Bind request from a protocol "above".
	*
	* This is normally only used by the SECPOL protocol. The PFKEY
	* does not keep track of these binds, and thus it accepts
	* all BindL's as is (without any checks).
	*
	* @param aProtocol The protocol doing the bind.
	* @param id The protocol number
	*/
	{
	// This function is just to allow standard BindToL sequence
	// to complete, currently PFKEY is not insterested in keeping
	// track of who binds to it, thus nothing is done here.
	}

void CProtocolKey::Identify(TServerProtocolDesc *aInfo) const
	/**
	* Return PFKEY protocol description
	*
	* @retval aEntry The description
	*/
	{
	IPSEC::IdentifyPfkey(*aInfo);
	}
	

TInt CProtocolKey::GetOption(TUint aLevel, TUint aName, TDes8& aOption, CProtocolBase* /*aSourceProtocol*/)
	/**
	* Supported GetOpt's for the PFKEY socket
	*
	* PFKEY SAP GetOption calls this.
	*
	* @param aLevel The option level
	* @param aName	The option name
	* @param aOption The option argument
	* @param aSourceProtocol Not used.
	*
	* Get Named EndPoint:
	* @code aLevel = KSolIpsecControl, aName = KSoIpsecEndpoint, aOption = TNameRecord
	* @endcode
	*/
	{
	if (aLevel == KSolIpsecControl && aName == KSoIpsecEndpoint)
		{
		// A problematic option: the aOption is both input (iName) and output (iAddr)!


		// Return the address (TNameRecord::iAddr) of a named end point (TNameRecord::iName)
		// If the named endpoint does not exist, return KErrNotFound and set address to unspecified.

		if (aOption.Length() != sizeof(TNameRecord))
			return KErrArgument;
		TNameRecord &record = (TNameRecord &)*aOption.Ptr();
		// ** should check that TNameRecord::iName and iAddr are valid TBuf
		// ** descriptors which do not cause a panic, when accessed! How?
		RIpAddress ep;
		const TInt res = ep.Open(iEndPointCollection, record.iName);
		// (if Open fails, ep() return unspecified address)
		TInetAddr::Cast(record.iAddr).SetAddress(ep());
		TInetAddr::Cast(record.iAddr).SetScope(ep().iScope);
		// ep is closed implicitly by destructor
		return res;
		}
	return KErrNotSupported;
	}

TInt CProtocolKey::SetOption(TUint aLevel, TUint aName,const TDesC8& aOption, CProtocolBase* /*aSourceProtocol*/)
	/**
	* Supported SetOpt's for the PFKEY socket
	*
	* PFKEY SAP SetOption calls this.
	*
	* @param aLevel The option level
	* @param aName	The option name
	* @param aOption The option argument
	* @param aSourceProtocol Not used.
	*
	* Set Named EndPoint.
	* @code aLevel = KSolIpsecControl, aName = KSoIpsecEndpoint, aOption = TNameRecord
	* @endcode
	*/
	{
	if (aLevel == KSolIpsecControl && aName == KSoIpsecEndpoint)
		{

		// Set the named end point (TNameRecord::iName) to specified address (TNameRecord::iAddr).
		if (aOption.Length() != sizeof(TNameRecord))
			return KErrArgument;
		const TNameRecord &record = (const TNameRecord &)*aOption.Ptr();
		// ** should check that TNameRecord::iName and iAddr are valid TBuf
		// ** descriptors which do not cause a panic, when accessed! How?

		RIpAddress ep;
		TInt res = ep.Open(iEndPointCollection, record.iName, TIpAddress(record.iAddr));
		if (res != KErrNone)
			return res;
		// Need to look if this EP is already recorded
		// If the EP reference is not stored any other place, it would
		// get deleted automaticly. Thus, need to record the EP's that
		// have been set with SetOpt, so that they stay around even if
		// no other reference exist yet.
		const TInt count = iEndPoints.Count();
		for (TInt i = count; --i >= 0; )
			{
			if (&iEndPoints[i]() == &ep())	// (Compare TIpAddress pointers directly)
				return KErrNone;			// Already recorded, nothing else to do.
			}
		// Unfortunately RArray Append does not execute RIpAddress copy
		// constructor or assignment, need to do some arm twisting here...
		RIpAddress dummy;
		res = iEndPoints.Append(dummy);
		if (res == KErrNone)
			{
			// Try with assigment...
			iEndPoints[count] = ep;
			}
		return res;
		// ep is closed implicitly by destructor
		}
	return KErrNotSupported;
	}

void CProtocolKey::SetAlgorithms(CAlgorithmList*& aList)
	/**
	* Receive Algorithm map from the policy.
	*
	* @param aList The algorithm mapping.
	*/
	{
	ASSERT(iCrypto != NULL);	// Should never happen.
	iCrypto->SetAlgorithms(aList);
	}

void CProtocolKey::Deliver(const TPfkeyMessage &aMsg)
	/**
	* Deliver PFKEY Messages to SAP's that are willing to receive them.
	*
	* @param aMsg	The PFKEY message
	*/
	{
	CProviderKey* sap;
	TDblQueIter<CProviderKey> iter(iSAPlist);

	while (sap = iter++, sap != NULL)
		{
		sap->Deliver(aMsg);
		}
	LOG(aMsg.LogPrint(_L("---->"), iCrypto->Algorithms()));
	}

void CProtocolKey::DeliverRegistered(const TPfkeyMessage &aMsg)
	/**
	* Deliver PFKEY Messages selectively.
	*
	* Same as CProtocolKey::Deliver, but sends message only to SAP's that
	* have REGISTERED for this type of Security Associtation.
	*
	* @param aMsg	The PFKEY message
	*/
	{
	CProviderKey* sap;
	TDblQueIter<CProviderKey> iter(iSAPlist);

	TUint i = aMsg.iBase.iMsg->sadb_msg_satype;
	TUint8 m = (TUint8)(1 << (i % 8));
	i = i / 8;
	while (sap = iter++, sap != NULL)
		if (sap->iRegistered[i] & m)
			{
			sap->Deliver(aMsg);
			}
	LOG(aMsg.LogPrint(_L("---->"), iCrypto->Algorithms()));
	}
