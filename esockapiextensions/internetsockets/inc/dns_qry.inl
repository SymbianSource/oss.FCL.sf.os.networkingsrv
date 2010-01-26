/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* DNS queries and results classes inline functions implementation
* 
*
*/



/**
 @file dns_qry.inl
*/

#ifndef __DNS_QRY_INL__
#define __DNS_QRY_INL__


inline TDnsQuery::TDnsQuery()
/**
Default constructor
*/
{
    iQryType  = KDnsRRTypeInvalid;
    iQryClass = KDnsRRClassIN;    // Internet class, default value
}

inline TDnsQuery::TDnsQuery(const TDesC8& aQryDomainName, TUint16 aType, TUint16 aClass/*=KDnsRRClassIN*/)
/**
Constructor

@param aQryDomainName DNS query data (buffer)
@param aType DNS query code
@param aClass Internet class
*/
{
    iQryType  = aType;
    iQryClass = aClass;    // Internet class, default value
    iQryData.Copy(aQryDomainName);
}

inline  TUint16 TDnsQuery::Type()  const 
/**
@return DNS RR type
*/
{
    return iQryType;
}  

inline  TUint16 TDnsQuery::Class() const 
/**
@return DNS RR class
*/
{
    return iQryClass;
}

inline  const TDesC8& TDnsQuery::Data() const
/**
@return const reference to the DNS query data
*/
{
    return iQryData;
}  
        
inline  void TDnsQuery::SetType(TUint16 aType)
/**
Sets the value of DNS RR type
@param aType DNS RR type
*/
{
    iQryType = aType;
}

inline  void TDnsQuery::SetClass(TUint16 aClass)
/**
Sets the value of DNS RR class
@param aClass DNS RR class
*/
{
    iQryClass = aClass;
}

inline  void TDnsQuery::SetData(const TDesC8& aData)
/**
Sets the value of the DNS query data
@param aData const reference to the DNS query data
*/
{
    iQryData.Copy(aData);
}


//-------------------------------------------------------------------------------------

inline  TDnsQryRespBase::TDnsQryRespBase(TUint16 aRespType, TUint16 aRespClass)
: iRespType(aRespType), iRespClass(aRespClass)
/**
Constructor

@param aRespType RR type
@param aRespClass RR Class
*/
{
    iRespTtl   = 0;
}

inline TUint16 TDnsQryRespBase::RRType() const 
/**
@return RR type from DNS response message
*/
{
    return iRespType;
}

inline TUint16 TDnsQryRespBase::RRClass() const 
/**
@return RR class from DNS response message
*/
{
    return iRespClass;
}

inline TUint32 TDnsQryRespBase::RRTtl() const 
/**
@return RR TTL from DNS response message
*/
{
    return iRespTtl;
}

inline void TDnsQryRespBase::SetRRTtl(TUint32 aRRTtl)
/**
Sets the value of RR TTL in the DNS response message
@param aRRTtl RR TTL 
*/
{
    iRespTtl = aRRTtl;
}

inline TDnsRespSRV::TDnsRespSRV()
                   :TDnsQryRespBase(KDnsRRTypeSRV, KDnsRRClassIN)
/**
Constructor
*/
{
    iPriority = 0;
    iWeight   = 0;  
    iPort     = 0;  
}

inline const TDesC8& TDnsRespSRV::Target() const 
/**
@return domain name of the target host.
*/
{
    return iTarget;
}

inline TUint16 TDnsRespSRV::Priority() const 
/**
@return The priority of this target host
*/
{
    return iPriority;
}

inline TUint16 TDnsRespSRV::Weight() const 
/**
@return the value of the weight field
*/
{
    return iWeight;
}

inline TUint16 TDnsRespSRV::Port() const 
/**
@return port number
*/
{
    return iPort;
}

inline void TDnsRespSRV::SetTarget(const TDesC8& aTarget) 
/**
Sets the domain name of the target host.
@param aTarget Domain name of the target host.
*/
{
    iTarget.Copy(aTarget);
}   

inline void TDnsRespSRV::SetPriority(TUint16 aPriority) 
/**
Sets The priority of this target host
@param aPriority The priority of this target host
*/
{
    iPriority = aPriority;
} 

inline void TDnsRespSRV::SetWeight(TUint16 aWeight) 
/**
Sets the value of the weight field
@param aWeight The value of the weight field
*/
{
    iWeight = aWeight;
}   

inline void TDnsRespSRV::SetPort(TUint16 aPort) 
/**
Sets the value of the port number
@param aPort port number
*/
{
    iPort = aPort;
}

//-------------------------------------------------------------------------------------

inline const TInetAddr&  TDnsRespA::HostAddress() const 
/**
@return Resolved Host Address
*/
{ 
    return iInetAddr; 
}

inline void TDnsRespA::SetHostAddress(const TInetAddr& aInetAddr) 
/**
Sets the value of the resolved Host Address
@param aInetAddr The resolved Host Address
*/
{
    iInetAddr = aInetAddr;
}

inline const TInetAddr&  TDnsRespAAAA::HostAddress() const 
/**
@return Resolved Host Address
*/
{ 
    return iInetAddr; 
}

inline void TDnsRespAAAA::SetHostAddress(const TInetAddr& aInetAddr)
/**
Sets the value of the resolved Host Address
@param aInetAddr The resolved Host Address
*/
{
    iInetAddr = aInetAddr;
}

inline const TDesC8& TDnsRespPTR::HostName() const 
/**
@return The domain name string descriptor
*/
{ 
    return iName; 
}

inline void TDnsRespPTR::SetHostName(const TDesC8& aHostName) 
/**
Sets the value of the domain name 
@param aHostName The domain name string descriptor
*/
{
    iName.Copy(aHostName);
}

inline TDnsRespNAPTR::TDnsRespNAPTR() 
                     : TDnsQryRespBase(KDnsRRTypeNAPTR, KDnsRRClassIN) 
/**
Constructor
*/
{
    iOrder  = 0;
    iPref   = 0;
}

inline TUint16 TDnsRespNAPTR::Order() const 
/**
@return Order field value
*/
{
    return iOrder;
}

inline TUint16 TDnsRespNAPTR::Pref() const 
/**
@return Preference field value
*/
{
    return iPref;
}

inline const TDesC8& TDnsRespNAPTR::Flags() const 
/**
@return Flags string descriptor
*/
{
    return iFlags;
}

inline const TDesC8& TDnsRespNAPTR::Service() const 
/**
@return service name(s) available down this rewrite path.
*/
{
    return iService;
}

inline const TDesC8& TDnsRespNAPTR::Regexp() const 
/**
@return Regexp field
*/
{
    return iRegexp;
}

inline const TDesC8& TDnsRespNAPTR::Replacement() const 
/**
@return Replacement field
*/
{
    return iReplacement;
}

inline void TDnsRespNAPTR::SetOrder(TUint16 aOrder)                   
/**
Sets the value of Order field 
@param aOrder Order field value
*/
{
    iOrder = aOrder;
}

inline void TDnsRespNAPTR::SetPref(TUint16 aPref)                     
/**
Sets the value of Preference field
@param aPref Preference field value
*/
{
    iPref = aPref;
}

inline void TDnsRespNAPTR::SetFlags(const TDesC8& aFlags)             
/**
Sets the value of Flags string 
@param aFlags Flags string 
*/
{
    iFlags = aFlags;
}

inline void TDnsRespNAPTR::SetService(const TDesC8& aService)         
/**
Sets the value of service name(s) available 
@param aService service name(s) available
*/
{
    iService = aService;
}

inline void TDnsRespNAPTR::SetRegexp(const TDesC8& aRegexp)           
/**
Sets the value of Regexp field
@param aRegexp Regexp field value
*/
{
    iRegexp = aRegexp;
}

inline void TDnsRespNAPTR::SetReplacement(const TDesC8& aReplacement) 
/**
Sets the value of Replacement field
@param aReplacement Replacement field value
*/
{
    iReplacement = aReplacement;
}

inline TUint16 TDnsRespMX::Pref() const 
/**
@return Preference field value
*/
{
    return iPref;
}

inline const TDesC8& TDnsRespMX::HostName()  const 
/**
@return Host name descriptor
*/
{
    return iHostName;
}

inline void TDnsRespMX::SetPref(TUint16 aPref) 
/**
Sets the value of Preference field
@param aPref Preference field value
*/
{
    iPref = aPref;
}

inline void TDnsRespMX::SetHostName(const TDesC8& aHostName) 
/**
Sets the value of Host name.
@param aHostName Host name.
*/
{
    iHostName = aHostName;
}

#endif //__DNS_QRY_INL__







