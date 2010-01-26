// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qoslib.h"


const TUint KMaxPort = 65535;

//
// TQoSSelector
//

/**
Constructor. 

Initialises all variables to specified, but null, values. 
@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
*/
//lint -e{1927} would like some of the following to be in initializer list!
EXPORT_C TQoSSelector::TQoSSelector()
	{
	iSrc.SetAddress(KInet6AddrNone);
	iDst.SetAddress(KInet6AddrNone);
	iSrcMask.SetAddress(KInet6AddrNone);
	iDstMask.SetAddress(KInet6AddrNone);
	iProtocol = 0;
	iSrcPortMax = 0;
	iDstPortMax = 0;
	iIapId = 0;
	}

/** 
Compares if the selectors are equal.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aSelector TQoSSelector object that is compared with this object.
@return ETrue, if selectors are equal to this object; otherwise, EFalse. 
*/
EXPORT_C TInt TQoSSelector::operator==(const TQoSSelector& aSelector) const
	{
	if (iProtocol == aSelector.Protocol() && iSrcPortMax == aSelector.MaxPortSrc() &&
		iDstPortMax == aSelector.MaxPortDst() && iIapId == (TUint)aSelector.IapId() &&
		iSrc.CmpAddr(aSelector.GetSrc()) && iDst.CmpAddr(aSelector.GetDst()))
		return TRUE;
	else
		return FALSE;
	}

/** 
Compares if the selector matches aSocket. 

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aSocket RSocket that is compared with this object.
@return ETrue, if selector created from aSocket is equal to this object; 
otherwise, EFalse. 
*/
EXPORT_C TBool TQoSSelector::Match(RSocket& aSocket) const
	{
	TQoSSelector sel;
	TInt ret = sel.SetAddr(aSocket);
	if (ret!=KErrNone)
		return EFalse;
	return (*this == sel);
	}

/**
Sets the addresses for selector.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aSrcAddr Source address. Port must have value <= 65535 
(0 is used as uspecified value for a port).
@param aSrcAddrMask Source address mask. Port must have value <= 65535 
(0 is used as uspecified value for a port).
@param aDstAddr Destination address. Port must have value <= 65535 
(0 is used as uspecified value for a port).
@param aDstAddrMask Destination address mask. Port must have value <= 65535
(0 is used as uspecified value for a port).
@param aProtocol Protocol ID.
@param aSrcPortMax Maximum source port. Must have value <= 65535 
(0 is used as uspecified value for a port).
@param aDstPortMax Maximum destination port. Must have value <= 65535 
(0 is used as uspecified value for a port).
@return KErrNone, if parameters have valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetAddr(const TInetAddr& aSrcAddr, 
									const TInetAddr& aSrcAddrMask, 
									const TInetAddr& aDstAddr, 
									const TInetAddr& aDstAddrMask, 
									TUint aProtocol, 
									TUint aSrcPortMax, 
									TUint aDstPortMax)
	{
	if (aSrcAddr.Port() > KMaxPort || aDstAddr.Port() > KMaxPort || 
		aSrcPortMax > KMaxPort || aDstPortMax > KMaxPort)
		return KErrArgument;

	iSrc = aSrcAddr;
	iDst = aDstAddr;

	iSrcMask = aSrcAddrMask;
	iDstMask = aDstAddrMask;
	iProtocol = aProtocol;
	iSrcPortMax = aSrcPortMax;
	iDstPortMax = aDstPortMax;
	iIapId = 0;
	return KErrNone;
	}

//lint -e{708} does not like union initializers
const TIp6Addr KInet6AddrMask = {{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
								  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}}};

//
// RSocket must be connected before this!!
//
/**
Sets the addresses for selector. 
 
RSocket is used to fetch addresses and ports for a selector. 
The resulting selector will match only for one socket. 
 
Note that RSocket::Connect() must be called before calling this function.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aSocket RSocket object that is used to set the selector variables. 
Note: RSocket must be connected
@return KErrNone, if parameters have valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetAddr(RSocket& aSocket)
	{
	TSockAddr aRemoteAddr, aLocalAddr;
	TProtocolDesc aDesc;
	aSocket.LocalName(aLocalAddr);
	aSocket.RemoteName(aRemoteAddr);

	if (aLocalAddr.Port() < 1 || aLocalAddr.Port() > KMaxPort)
		return KErrArgument;
	if (aRemoteAddr.Port() < 1 || aRemoteAddr.Port() > KMaxPort)
		return KErrArgument;
	if (iDst.Family() == KAFUnspec)
		return KErrArgument;

	iSrc = TInetAddr(aLocalAddr);
	iDst = TInetAddr(aRemoteAddr);

	// Srcaddr is not used when a selector is made from RSocket: 
	// srcaddr might not be set in Connect()
	iSrc.SetFamily(KAFUnspec);
	iSrc.SetAddress(KInet6AddrNone);

	if (iDst.Family() == KAfInet)
		iDst.ConvertToV4Mapped();
	//
	// Set prefixes.
	//
	iSrcMask.SetAddress(KInet6AddrMask);
	iDstMask.SetAddress(KInet6AddrMask);

	iSrcPortMax = iSrc.Port();
	iDstPortMax = iDst.Port();
	aSocket.Info(aDesc);
	iProtocol = aDesc.iProtocol;
	return KErrNone;
	}

/**
Sets the source address for selector.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aAddr Source address. Port must have value <= 65535 (0 is used as 
uspecified value for a port).
@return KErrNone, if parameters have valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetSrc(const TInetAddr& aAddr)
	{
	if (aAddr.Port() > KMaxPort)
		return KErrArgument;
	iSrc = aAddr;
	return KErrNone;
	}

/**
Sets the destination address for selector.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aAddr Destination address. Port must have value <= 65535 (0 is used 
as uspecified value for a port).
@return KErrNone, if parameters have valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetDst(const TInetAddr& aAddr)
	{
	if (aAddr.Port() > KMaxPort)
		return KErrArgument;
	iDst = aAddr;
	return KErrNone;
	}

/**
Gets the current source address.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Source address.
*/
EXPORT_C TInetAddr TQoSSelector::GetSrc() const
	{
	return iSrc;
	}

/**
Gets the current destination address.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Destination address.
*/
EXPORT_C TInetAddr TQoSSelector::GetDst() const
	{
	return iDst;
	}

/**
Sets the source address mask.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aMask Source address mask. Port must have value <= 65535 (0 is used 
as uspecified value for a port).
@return KErrNone, if parameters have valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetSrcMask(const TInetAddr& aMask)
	{
	if (aMask.Port() > KMaxPort)
		return KErrArgument;
	iSrcMask = aMask;
	return KErrNone;
	}

/**
Gets the current source address mask.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Source address mask.
*/
EXPORT_C TInetAddr TQoSSelector::GetSrcMask() const
	{
	return iSrcMask;
	}

/**
Sets the destination address mask.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aMask Destination address mask. Port must have value <= 65535 (0 is 
used as uspecified value for a port).
@return KErrNone, if parameters have valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetDstMask(const TInetAddr& aMask)
	{
	if (aMask.Port() > KMaxPort)
		return KErrArgument;
	iDstMask = aMask;
	return KErrNone;
	}

/**
Gets the current destination address mask.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Destination address mask.
*/
EXPORT_C TInetAddr TQoSSelector::GetDstMask() const
	{
	return iDstMask;
	}

/**
Sets the Internet access point identifier. 

0 is used as unspecified value.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aIapId Value to which to set the IapId.
*/
EXPORT_C void TQoSSelector::SetIapId(TInt aIapId)
	{
	iIapId = aIapId;
	}

/**
Gets the current Internet access point identifier.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Internet access point identifier.
*/
EXPORT_C TInt TQoSSelector::IapId() const
	{
	return iIapId;
	}

/**
Sets the protocol identifier. 

0 is used as unspecified value.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aProtocol Value to which to set the protocol ID.
*/
EXPORT_C void TQoSSelector::SetProtocol(TUint aProtocol)
	{
	iProtocol = aProtocol;
	}

/**
Gets the current protocol identifier.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Protocol identifier.
*/
EXPORT_C TUint TQoSSelector::Protocol() const
	{
	return iProtocol;
	}

/**
Sets the maximum source port. 

Port must have value <= 65535 (0 is used as unspecified value).

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aMaxPort Value to which to set maximum source port.
@return KErrNone, if aMaxPort has valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetMaxPortSrc(TUint aMaxPort)
	{
	if (aMaxPort > KMaxPort)
		return KErrArgument;
	iSrcPortMax = aMaxPort;
	return KErrNone;
	}

/**
Sets the maximum destination port. 

Port must have value <= 65535 (0 is used as unspecified value).

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@param aMaxPort Value to which to set maximum destination port.
@return KErrNone, if aMaxPort has valid values; otherwise, KErrArgument.
*/
EXPORT_C TInt TQoSSelector::SetMaxPortDst(TUint aMaxPort)
	{
	if (aMaxPort > KMaxPort)
		return KErrArgument;
	iDstPortMax = aMaxPort;
	return KErrNone;
	}

/**
Gets the maximum source port.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Maximum source port.
*/
EXPORT_C TUint TQoSSelector::MaxPortSrc() const
	{
	return iSrcPortMax;
	}

/**
Gets the maximum destination port.

@publishedPartner
@released
@capability NetworkControl Restrict QoS policy operations because they may 
affect several data flows.
@return Maximum destination port.
*/
EXPORT_C TUint TQoSSelector::MaxPortDst() const
	{
	return iDstPortMax;
	}

