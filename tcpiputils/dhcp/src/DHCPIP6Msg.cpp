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
// Implements the DHCP Message format
// 
//

/**
 @file DHCPIP6Msg.cpp
 @internalTechnology
*/

#include "DHCPIP6IA.h"
#include "DHCPAuthentication.h"
#include "DHCP_Std.h"

using namespace DHCPv6;

COptionNode::~COptionNode()
	{
	delete iNext;
	}

COptionNode* COptionNode::NewL()
	{
	return new(ELeave)COptionNode(NULL);
	}

COptionNode* CDHCPOptionAny::NewL()
	{
	return new(ELeave)CDHCPOptionAny(NULL);
	}

COptionNode* CDHCPOptionStatusCode::NewL()
	{
	return new(ELeave)CDHCPOptionStatusCode();
	}

COptionNode* CDHCPOptionRequestOption::NewL()
	{
	return new(ELeave)CDHCPOptionRequestOption();
	}

struct SOptionHeader
	{
	TInt iCode;
#ifdef __FLOG_ACTIVE
	const TText8* iName;
#endif
	};

#ifdef __FLOG_ACTIVE
const TInt KNumberOfSupportedOptions = 23;
const SOptionHeader KOptions[KNumberOfSupportedOptions + 1]= 
	{   //iCode,                     iName
		{EClientId,          _S8( "EClientId" )},
		{EServerId,          _S8( "EServerId" )},
		{EIaNa,              _S8( "EIaNa" )},
		{EIaTa,              _S8( "EIaTa" )},
		{EIaAddr,            _S8( "EIaAddr" )},
		{EOro,               _S8( "EOro" )},
		{EPreference,        _S8( "EPreference" )},
		{EElapsedTime,       _S8( "EElapsedTime" )},
		{ERelayMsg,          _S8( "ERelayMsg" )},
		{EAuthentication,    _S8( "EAuthentication" )},
		{EUnicast,           _S8( "EUnicast" )},
		{EStatusCode,        _S8( "EStatusCode" )},
		{ERapidCommit,       _S8( "ERapidCommit" )},
		{EUserClass,         _S8( "EUserClass" )},
		{EVendorClass,       _S8( "EVendorClass" )},
		{EVendorOpts,        _S8( "EVendorOpts" )},
		{EInterfaceId,       _S8( "EInterfaceId" )},
		{EReconfMsg,         _S8( "EReconfMsg" )},
		{EReconfAccept,      _S8( "EReconfAccept" )},
		{ESipServerD,        _S8( "ESipServerD" )},
		{ESipServerA,        _S8( "ESipServerA" )},
		{EDNSServers,        _S8( "EDNSServers" )},
		{EDomainList,        _S8( "EDomainList" )},
		{0,                          _S8( "EUnknown" )}
	};

void COptionNode::Dump(const TDesC& aTag, const TDesC& aFile)
	{
	RFileLogger f;
	if (f.Connect() == KErrNone)
		{
		f.CreateLog(aTag, aFile, EFileLoggingModeAppend);
		TInt i = 0;
      TInt nCode = OpCode();
		while (KOptions[i].iCode != nCode && ++i < KNumberOfSupportedOptions){/*do nothing*/}
		
		f.WriteFormat(_L8("Option Code: %s, length: %d"), KOptions[i].iName, nCode);
		if (GetItemLength() > KOptionHeaderLength)
			{
			f.HexDump(NULL, NULL, GetBodyPtr(), GetLength());
			}
		f.CloseLog();
		f.Close();
		}
	}
#endif

//simple static const mapping option id to the handling class constructor
//to simplify parsing
namespace DHCPv6
{
typedef COptionNode* (*TOptionCostructor)();


struct SMapOptionNumberToOption
{
   TUint iOption;
   TOptionCostructor iNewL;
};

static const SMapOptionNumberToOption mapOptionNumberToOption[] = {
    {EIaNa                  , CDHCPOptionIA_NA::NewL},       //Identity Association for Non-temporary Addresses 
    {EIaTa                  , CDHCPOptionIA_TA::NewL},          //Identity Association for Temporary Addresses
    {EOro                   , CDHCPOptionRequestOption::NewL},//Option Request - identify a list of options in a message
    {EAuthentication        , CDHCPOptionAuthentication::NewL},
    {EDNSServers		    , CDHCPOptionDNSServers::NewL},
    {ESipServerA			, CDHCPOptionSipServerAddrs::NewL},    	
    {EOptionAny             , COptionNode::NewL}
	};
//the rest of the options uses CDHCPOptionAny::NewL

}//namespace DHCPv6

void COptionList::ParseL(TPtr8& aDes8)
/**
  * Parse message to set pointers into descriptor buffer
  * at locations of each option supplied in a response msg
  *
  * @internalTechnology
  */
	{
	ASSERT(!iRecord.iFirst);
	
    COptionNode** ppNode = reinterpret_cast<COptionNode**>(&iRecord.iFirst);
	
	while(aDes8.Length())
        {
        User::LeaveIfError(aDes8.Length() < KOptionHeaderLength ? KErrBadDescriptor : KErrNone);

        TUint nOptCode = TBigEndian::GetValue(aDes8.Ptr(), KOptionLengthOffset);

		*ppNode = CreateNodeL(nOptCode);
		(*ppNode)->ParseL(aDes8);
		
		ppNode = reinterpret_cast<COptionNode**>(&(*ppNode)->iNext);
		}
	}

TUint32 COptionList::GetL( TInt aIndex, TUint aOpCode ) const
/**
  * Return a value from the message in big endian format
  *
  * @internalTechnology
  */
	{
	COptionNode* pNode = FindOption(aOpCode);
   if ( !pNode || pNode->GetLength() < aIndex * K32bitNumberOctets + K32bitNumberOctets )
      {
      User::Leave( KErrNotFound );
      }
   return TBigEndian::GetValue( pNode->Ptr() + aIndex * K32bitNumberOctets, K32bitNumberOctets );
	}

COptionNode* COptionList::CreateNodeL( TUint aOpCode )
   {
   TInt n = 0;
   while ( mapOptionNumberToOption[n].iOption != aOpCode && 
      mapOptionNumberToOption[n].iOption != EOptionAny )
      {
      n++;
      };
	return (*mapOptionNumberToOption[n].iNewL)();
   }

COptionNode* COptionList::AddNodeL(TOptionCodes aOpCode, TInt aInitialLength)
/**
  * Create a new node for an option in the message
  *
  * @internalTechnology
  */
   {
   COptionNode* pNode = CreateNodeL(aOpCode);
   AddNode(pNode);
   pNode->Header().SetInitialValue(aInitialLength);
   return pNode;
   }

COptionNode* COptionList::FindOption(TUint aOpCode, TInt& aPos) const
	{
	COptionNode* nodeCursor = static_cast<COptionNode*>(iRecord.iFirst);
	TInt counter = 0;
	while (nodeCursor)
		{
		if ( nodeCursor->OpCode() == aOpCode && ++counter > aPos )
			{
			aPos = counter;
			return nodeCursor;
			}
		nodeCursor = static_cast<COptionNode*>(nodeCursor->iNext);
		}
	return NULL;
	}

TUint32 CDHCPOptionStatusCode::GetStatusCode() const
    {
    __ASSERT_DEBUG( GetLength() >= KDHCPOptionStatusCodeLength, User::Panic( KDhcpv6, KErrBadDescriptor ) );
    return TBigEndian::GetValue(iPtr8 + KOptionHeaderLength, KDHCPOptionStatusCodeLength);
    }

void CDHCPOptionStatusCode::SetStatusCode( TUint32 aStatusCode )
    {
   __ASSERT_DEBUG( GetLength() >= KDHCPOptionStatusCodeLength, User::Panic( KDhcpv6, KErrBadDescriptor ) );
	TBigEndian::SetValue(iPtr8 + KOptionHeaderLength, KDHCPOptionStatusCodeLength, aStatusCode);
    }

void CDHCPOptionDNSServers::ParseL(TPtr8& aDes8)
	{
	COptionNode::ParseL(aDes8);
	
	TPtr8 ptr(GetBodyDes());
	
	User::LeaveIfError(ptr.Length() % KIp6AddressLength ? KErrBadDescriptor : KErrNone);
	}

TBool CDHCPOptionDNSServers::GetDomainNameServer( TInt aIndex, TInetAddr& addr )
	{
	TInt pos = aIndex * KIp6AddressLength;
	TBool ret = GetLength() >= pos + KIp6AddressLength;
	if ( ret )
		{
		// Must ensure IP6 address is word aligned! So declare it locally
		TIp6Addr ip6addr;
		Mem::Copy(&ip6addr,GetBodyPtr() + pos,KIp6AddressLength);
		
		addr.SetAddress(ip6addr);
		}
	return ret;
	}
	
void CDHCPOptionSipServerAddrs::ParseL(TPtr8& aDes8)
	{
	COptionNode::ParseL(aDes8);
	
	TPtr8 ptr(GetBodyDes());
	
	User::LeaveIfError(ptr.Length() % KIp6AddressLength ? KErrBadDescriptor : KErrNone);
	}

TBool CDHCPOptionSipServerAddrs::GetSipServerAddr(TInt aIndex, TInetAddr& addr)
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPOptionSipServerAddrs::GetSipServerAddr index = %d"),aIndex));
	TInt pos = aIndex * KIp6AddressLength;
	TInt len = GetLength();
	TBool ret = len >= pos + KIp6AddressLength;
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("pos = %d, len = %d, ret = %d"), pos, len, ret));

	if (ret)
		{
		// Must ensure IP6 address is word aligned! So declare it locally
		TIp6Addr ip6addr;
		Mem::Copy(&ip6addr,GetBodyPtr() + pos,KIp6AddressLength);  

		addr.SetAddress(ip6addr);
		}
	return ret;
	}


void CDHCPOptionSipServerDomains::ParseL(TPtr8& aDes8)
	{
	COptionNode::ParseL(aDes8);
	}

TBool CDHCPOptionSipServerDomains::GetSipServerDomains(TInt aIndex, THostName& aName)
	{
	TInt domainIdx = 0;
	TUint8* pChar = const_cast<TUint8*>(GetBodyDes().Ptr());
	TUint labelLength = 0;
	
	// Walk the list of domain names
	while(*pChar++ != NULL && domainIdx != aIndex)
		{
		labelLength = *(pChar - 1);
		*(pChar - 1) = '.';
		
		TBuf8<0x100> tmp;
		tmp.Copy(pChar, labelLength);
		aName.Copy(tmp);

		pChar += labelLength;
		}
	
	return EFalse;
	}

//---------------------------------
void CDHCPOptionRequestOption::AppendRequestedOptions()
   {
	// the array is in network byte order(big-endian) already => could be used as a descriptor
	// authentication option and IA for temporary addresses not involved so far
	TUint8 requestedOptions[KDHCPOptionRequestLen] = {0,EServerId, 0,EIaNa, 0,ESipServerD, 0,ESipServerA,
														0,EDNSServers, 0,EDomainList};


	TPtr8 ptr(requestedOptions, KDHCPOptionRequestLen, KDHCPOptionRequestLen );
	GetBodyDes().Copy(ptr);
   }

CDHCPMessageHeaderIP6::~CDHCPMessageHeaderIP6()
	{
	Close();
	}

COptionNode* CDHCPMessageHeaderIP6::AddOptionL(TOptionCodes aOpCode, TInt aLength)
/**
  * First stage of adding an option to the message. Calls AddNode to create space
  * for message.
  *
  * @param aOpCode The opcode of the options to be added
  * @param aLength The length of the option data
  * @return COptionNode* The pointer to the option in the message
  *
  * @internalTechnology
  */
	{
	COptionNode* pNode = iOptions.AddNodeL(aOpCode, aLength);
	TPtr8 ptr = iMsg->Des();
	pNode->iRecord.InitialiseL(ptr);
	pNode->SetOpCode(static_cast<TUint>(aOpCode));
	return pNode;
	}
	
TInt CDHCPMessageHeaderIP6::Parse(const TDhcpRnd& aXid, const TDesC8& aClientId, RBuf8& aServerId)
/**
  * When a response is received to a message
  * this function will unpack the response by creating
  * option objects which can be easily used to retrieve 
  * the values returned in these options.  Providing 
  * this functionality hides the nasty descriptor operations. 
  *
  * Each option class has a pointer set to the start point
  * of their data in the response message.
  *
  * There is no need to explicitly unpack the mandatory fields
  * of the DHCP message that has been returned.  Accessor functions
  * simply extract the data sraight out of the decsriptor.
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse")));
	
	iOptions.RemoveAllNodes();
	TPtr8 ptr = iMsg->Des();
	TRAPD(ret,iRecord.ParseL(ptr););
	
   if ( ret == KErrNone )
		{
		/* Reject the message if:
		 * 
		 * 1) Not an Advertise, Reply, or potentially a Reconfigure message. 
		 * 2) The server or client ID values are nonexistent
		 * 3) The client ID being advertised does not match ours
		 * 4) Bad status code
		 */
	   if (GetXid()!=aXid.Xid())
		   {
		   __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse - non-matching xid")));
		   return KErrBadDescriptor;	// does not match so we are not interested in this message
		   }
      CDHCPOptionStatusCode* pStatus = static_cast<CDHCPOptionStatusCode*>(iOptions.FindOption( EStatusCode ));
      TInt code = pStatus ? pStatus->GetStatusCode() : ESuccess;
		if (code != ESuccess )
		   {
		   __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse - status code %d"), code));
		   return KErrBadDescriptor;	// bad status code so we are not interested in this message
		   }
      TUint8 nType = GetMessageType();
      COptionNode* pServerId = iOptions.FindOption( EServerId );
      COptionNode* pClientId = iOptions.FindOption( EClientId );
		switch(nType)
			{
			case EAdvertise:
				if(pClientId == NULL || pServerId == NULL)
					{
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse - invalid message type %d or options"), nType));
					return KErrBadDescriptor;
					}
					break;			
			case EReply:
#if defined(DHCP_RECONFIGURE_NO_AUTHENTICATION)
			case EReconfigure:
#endif				
				if (pClientId == NULL || pServerId == NULL || 
				    (aServerId.Length() && pServerId->GetBodyDes() != aServerId))
					{
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse - invalid message type %d or options"), nType));
					return KErrBadDescriptor;
					}
				if ( nType == EReconfigure && !iOptions.FindOption( EReconfMsg ) )
				    {
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse - Reconfigure msg with no reconfigure option")));
					return KErrBadDescriptor;
				    }
				break;
			 default:
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse - invalid message type %d or options"), nType));
				return KErrBadDescriptor;
			}

		if(pClientId->GetBodyDes() != aClientId)
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP6::Parse - ClientIds don't match")));
			return KErrBadDescriptor;
			}
		if ( !aServerId.Length() )
		    {
            aServerId.Close();
            TRAP(ret, aServerId.CreateL( pServerId->GetBodyDes() ));
		    }
      }
   return ret;
	}

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
void CDHCPOptionDomainSearchList::ParseL(TPtr8& aDes8)
	{
	COptionNode::ParseL(aDes8);
	}

TBool CDHCPOptionDomainSearchList::GetDomainSearchList(TInt aIndex, THostName& aName)
	{
	TInt domainIdx = 0;
	TUint8* pChar = const_cast<TUint8*>(GetBodyDes().Ptr());
	TUint labelLength = 0;
	TBuf8<0x100> tmp;
	
	if(*pChar)
		{
		// Walk the list of domain names
		while(*pChar++ != NULL && domainIdx != aIndex)
			{
			labelLength = *(pChar - 1);
			*(pChar - 1) = '.';
				
			tmp.Copy(pChar, labelLength);
			aName.Copy(tmp);

			pChar += labelLength;
			}
		return EFalse;
		}
	else
		{
		return ETrue;
		}
	}
#endif //SYMBIAN_TCPIPDHCP_UPDATE
