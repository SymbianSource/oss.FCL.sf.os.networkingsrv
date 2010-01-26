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
// DHCPv6 Message wrapper header file
// 
//

/**
 @file
*/

#ifndef DHCPIP6MSG_H
#define DHCPIP6MSG_H

#include "DHCPMsg.h"
#include "DHCPIP6_Std.h"
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
#include "DomainNameDecoder.h"
#endif //SYMBIAN_TCPIPDHCP_UPDATE
//!!!!BEWARE all message handling so far (except message receiver) expects there's a buffer long enough
//to hold whatever necessary.
//We should be able to compute the length of needed buffer beforehand based on the message structure

_LIT( KDhcpv6, "DHCPv6" );

class TInetAddr;
class TDhcpRnd;

namespace DHCPv6
{

enum TMessageType
/**
  * DHCPv6::TMessageType
  *
  * The message type constants for the DHCPv6
  * message type option for inclusion in DHCPv6
  * messages.
  *
  * @internalTechnology
  */
	{
	ESolicit = 0x01,		   // request
	EAdvertise,		   		// reply
	ERequest,				   // request
	EConfirm,				   // request
	ERenew,					   // request
	ERebind,		   		   // request
	EReply,			         // reply
	ERelease,					// request
	EDecline,					// request
	EReconfigure,				// reply
	EInformationRequest,		// request
//	ERelayForw,	   			// not supported
//	ERelayRepl 		   		// not supported
   EUnknown = 0xFAFA
	};

/**
 * DHCPv6::TStatusCodes
 * A Status Code option may appear in the options field of a DHCP
 * message and/or in the options field of another option.  If the Status
 * Code option does not appear in a message in which the option could
 * appear, the status of the message is assumed to be Success.
*/
enum TStatusCodes
   {
	//RFC status codes
   ESuccess        = 0, //Success.
   EUnspecFail     = 1, //Failure, reason unspecified; this
                        //status code is sent by either a client
                        //or a server to indicate a failure
                        //not explicitly specified in this
                        //document.
   ENoAddrsAvail   = 2, //Server has no addresses available to assign to
                        //the IA(s).
   ENoBinding      = 3, //Client record (binding) unavailable.
   ENotOnLink      = 4, //The prefix for the address is not appropriate for
                        //the link to which the client is attached.
   EUseMulticast   = 5, //Sent by a server to a client to force the
                        //client to send messages to the server.
                        //using the All_DHCP_Relay_Agents_and_Servers
                        //address.
	//internal component status codes
	EMarkForRelease = 128,
	EMarkForDecline = 129,
	EMarkToRequest	 = 130,
	EStatusUnknown	 = 0xFF, //highest possible status code

   };
/**
  * DHCPv6 negotiation constants (the time values are in seconds)
  *
  * @internalTechnology
  */
const TInt  KSolMaxDelay   = 1;   //sec   Max delay of first Solicit
const TInt  KSolTimeout    = 1;   //sec   Initial Solicit timeout
const TInt  KSolMaxRt      = 120; //secs  Max Solicit timeout value
const TInt  KReqTimeout    = 1;   //sec   Initial Request timeout
const TInt  KReqMaxRt      = 30;  //secs  Max Request timeout value
const TInt  KReqMaxRc      = 10;  //     Max Request retry attempts
const TInt  KCnfMaxDelay   = 1;   //sec   Max delay of first Confirm
const TInt  KCnfTimeout    = 1;   //sec   Initial Confirm timeout
const TInt  KCnfMaxRt      = 4;   //secs  Max Confirm timeout
const TInt  KCnfMaxRd      = 10;  //secs  Max Confirm duration
const TInt  KRenTimeout    = 10;  //secs  Initial Renew timeout
const TInt  KRenMaxRt      = 600; //secs  Max Renew timeout value
const TInt  KRebTimeout    = 10;  //secs  Initial Rebind timeout
const TInt  KRebMaxRt      = 600; //secs  Max Rebind timeout value
const TInt  KInfMaxDelay   = 1;   //sec   Max delay of first Information-request
const TInt  KInfTimeout    = 1;   //sec   Initial Information-request timeout
const TInt  KInfMaxRt      = 120; //secs  Max Information-request timeout value
const TInt  KRelTimeout    = 1;   //sec   Initial Release timeout
const TInt  KRelMaxRc      = 5;   //    MAX Release attempts
const TInt  KDecTimeout    = 1;   //sec   Initial Decline timeout
const TInt  KDecMaxRc      = 5;   //    Max Decline attempts
const TInt  KRecTimeout    = 2;   //secs  Initial Reconfigure timeout
const TInt  KRecMaxRc      = 8;   //    Max Reconfigure attempts
const TInt  KHopCountLimit = 32;  //     Max hop count in a Relay-forward message

/******DHCP extension template []->optional - the code dependent******
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          option-code          |           option-len          |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          option-data                          |
      |                      (option-len octets)                      |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

      option-code   An unsigned integer identifying the specific option
                    type carried in this option.

      option-len    An unsigned integer giving the length of the
                    option-data field in this option in octets.

      option-data   The data for the option; the format of this data
                    depends on the definition of the option.
                    +-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-++-+-+-+-+-+
                    | fixed part      | variable part (option list)   |
                    +-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-++-+-+-+-+-+
*********************************************************************/
enum TOptionCodes
/**
  * DHCPv6::TOptionCodes
  *
  * The constant values for each supported
  * option type.
  *
  * @internalTechnology
  */
	{
	EClientId = 1,          //Client DUID
	EServerId = 2,          //Server DUID
	EIaNa = 3,              //Identity Association for Non-temporary Addresses 
	EIaTa = 4,              //Identity Association for Temporary Addresses
	EIaAddr = 5,            //IPv6 addresses associated with an IA_NA(EIaNa) or an IA_TA(EIaTa)
	EOro = 6,               //Option Request - identify a list of options in a message
	EPreference = 7,
   EElapsedTime = 8,
   ERelayMsg   = 9,        //for relay agents only
	EAuthentication = 11,
	EUnicast = 12,          //server sends to indicate the client is allowed to unicast messages
	EStatusCode = 13,
	ERapidCommit = 14,
	EUserClass = 15,
	EVendorClass = 16,
	EVendorOpts = 17,
	EInterfaceId = 18,      //for relay agents only
	EReconfMsg = 19,
   EReconfAccept = 20,
   ESipServerD = 21,       //SIP Server Domain Name List
   ESipServerA = 22,        //SIP Servers IPv6 Address List
   //the options 23/24 are just in  Solicit, Advertise, Request, Renew, Rebind, Information-Request, and Reply
   EDNSServers = 23,       
   EDomainList = 24,       //Domain Search List option
   EOptionAny = 0xFAFA
   //prefix delegation
   //otion lifetime
	};

enum TReconfigureTypes
    {
    EReconfigureRenew  = 5,
    EReconfigureInformRequest = 11
    };

const TInt KDHCPHeaderLength = 4;
const TInt KXidLength = 3;
const TInt KXidOffset = 1;
const TInt KOptionHeaderLength = 4;
const TInt K32bitNumberOctets = 4;
const TInt KOptionLengthOffset = 2;
const TInt KOptionCodeOffset = 0;
const TInt KOptionCodeLen = 2;
const TInt KElapsedTimeOptionLen = 2;
const TInt KOptionLengthLen = 2;

const TInt KDuidTypeLen         = 2;
const TInt KDuidLLTypeCode      = 3;
const TInt KDuidHardwareTypeLen = 2;
const TInt KDuidEthMacAddrSize	= 6;

template <typename LAYOUT>
class TOptionHeader : public TValue< LAYOUT >
	{
public:
	TOptionHeader(TInt aValueLength) :
		TValue<LAYOUT>(aValueLength)
		{
		};

	TInt GetValue(const TUint8* aPtr8) const;
	void SetValue(TUint8* aPtr8, TInt aValue) const;
	};

template <typename LAYOUT>
TInt TOptionHeader<LAYOUT>::GetValue(const TUint8* aPtr8) const
	{
   __ASSERT_DEBUG( this->iValueLength == KOptionHeaderLength, User::Panic( KDhcpv6, KErrBadDescriptor ) );
	return LAYOUT::GetValue(aPtr8 + KOptionLengthOffset, this->iValueLength - KOptionLengthOffset);
	}

template <typename LAYOUT>
void TOptionHeader<LAYOUT>::SetValue(TUint8* aPtr8, TInt aValue) const
	{
   __ASSERT_DEBUG( this->iValueLength == KOptionHeaderLength, User::Panic( KDhcpv6, KErrBadDescriptor ) );
	LAYOUT::SetValue(aPtr8 + KOptionLengthOffset, this->iValueLength - KOptionLengthOffset, aValue);
	}

template <class TLAYOUT>
class COption : public CItem<TOptionHeader<TLAYOUT> >
	{
public:
	COption(CItemBase* aNext) :
		CItem<TOptionHeader<TLAYOUT> >(aNext, KOptionHeaderLength)
		{
		}

	TUint OpCode() const
		{
      return TLAYOUT::GetValue( this->CItemBase::iPtr8, KOptionCodeLen );
		}
	void SetOpCode(TUint aOpCode)
		{
      TLAYOUT::SetValue( this->CItemBase::iPtr8, KOptionCodeLen, aOpCode );
		}
	};

class COptionNode : public COption<TBigEndian>
/**
  * represents one DHCPv6 option in a message
  * 
  *
  * @internalTechnology
  */
	{
	friend class COptionList;

public:
    
#ifdef __FLOG_ACTIVE
	virtual void Dump(const TDesC& aTag, const TDesC& aFile);
#endif

   static COptionNode* NewL();

public:
  	TRecord iRecord;

protected:
	COptionNode( CItemBase* aNext );
	virtual ~COptionNode();
   };
   

inline COptionNode::COptionNode( CItemBase* aNext ) :
	COption<TBigEndian>(aNext),
   iRecord( NULL )
	{
	iRecord.iFirst = this; //to get rid of too cautious warning C4355 (assignment of uncompleted this)
	}

class COptionList : public CListItem
	{
public:
	COptionList(CItemBase* aNext);
   
	COptionNode* AddNodeL(TOptionCodes aOpCode, TInt aInitialLength);
	COptionNode* FindOption(TUint aOpCode) const;
	COptionNode* FindOption(TUint aOpCode, TInt& aPos) const;

	virtual void ParseL(TPtr8& aDes8);

   //get 32bit value (offset == aIndex * 4bytes)
	TUint32 GetL( TInt aIndex, TUint aOpCode ) const;

protected:
   static COptionNode* CreateNodeL( TUint aOpCode );
	};

inline COptionList::COptionList(CItemBase* aNext) : 
	CListItem(aNext, 0)
	{
	}

inline COptionNode* COptionList::FindOption(TUint aOpCode) const
/**
  * Find the location of an option in the message and return a pointer to it
  *
  * @internalTechnology
  */
	{
	TInt dummy = 0;
	return FindOption(aOpCode,dummy);
	}

class CDHCPOptionAny : public COptionNode
/**
  * Class parsing any DHCP option that has a structure given by a subclass and
  * followed by unspecified trailing option data (member iOptionData)
  *
  * @internalTechnology
  */
	{

public:
	CDHCPOptionAny(CItemBase* aNext) :
      COptionNode( aNext )
      {
      }

   static COptionNode* NewL();

#if 0
public:
	virtual void ParseL( TPtr8& aDes8 );

public:
	TPtr8 OptionData();
	
protected:
	TUint8* iPtrOptionData;
	TInt iLenOptionData;
#endif
	};

/**
  * This DNS Recursive Name Server option(option 23) returns the Recursive DNS server addresses
  * Ref: RFC 3646
  * @internalTechnology
  */
class CDHCPOptionDNSServers : public COptionNode
	{
public:
	CDHCPOptionDNSServers() :
      COptionNode(NULL)
      {
      }
	
	virtual void ParseL( TPtr8& aDes8 );

	TBool GetDomainNameServer( TInt aIndex, TInetAddr& addr );
	};


/**
  * This option returns the SIP server addresses
  *
  * @internalTechnology
  */
class CDHCPOptionSipServerAddrs : public COptionNode
	{
public:
	CDHCPOptionSipServerAddrs() :
      COptionNode(NULL)
      {
      }
	
	virtual void ParseL(TPtr8& aDes8);

	TBool GetSipServerAddr(TInt aIndex, TInetAddr& addr);
	};

/**
  * This option returns the SIP server domain names
  *
  * @internalTechnology
  */
class CDHCPOptionSipServerDomains : public COptionNode
	{
public:
	CDHCPOptionSipServerDomains() :
      COptionNode(NULL)
      {
      }
	
	virtual void ParseL(TPtr8& aDes8);

	TBool GetSipServerDomains(TInt aIndex, THostName& aName);
	};
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
/**
  * This Domain Search List option(option 24) returns the Domain names while doing
  * name resolution
  * Ref: RFC 3646
  * @internalTechnology
  */
class CDHCPOptionDomainSearchList  : public COptionNode
	{
public:
	CDHCPOptionDomainSearchList() :
      COptionNode(NULL)
      {
      }
	
	virtual void ParseL(TPtr8& aDes8);

	TBool GetDomainSearchList(TInt aIndex, THostName& aName);
	};

#endif //SYMBIAN_TCPIPDHCP_UPDATE	

const TInt KDHCPOptionStatusCodeLength = 2; //bytes

class CDHCPOptionStatusCode : public CDHCPOptionAny
/**
  * This option returns a status indication related to the DHCP message
  * or option in which it appears.
  *
  * @internalTechnology
  */
	{

public:
    CDHCPOptionStatusCode() : CDHCPOptionAny(NULL) 
        {
        }
    
    COptionNode* NewL();

public:
	TUint32 GetStatusCode() const;
	void SetStatusCode( TUint32 aStatusCode );
	};

const TInt KDHCPOptionRequestLen = 12;

class CDHCPOptionRequestOption : public COptionNode
/**
  * This option returns a status indication related to the DHCP message
  * or option in which it appears.
  *
  * @internalTechnology
  */
	{

public:
	CDHCPOptionRequestOption() :
		COptionNode( NULL )
		{
     	}

   static COptionNode* NewL();

public:
	void AppendRequestedOptions();

	};

/**********************DHCPv6 message header****************************
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |    msg-type   |               transaction-id                  |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      .                            options                            .
      .                           (variable)                          .
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

      msg-type             Identifies the DHCP message type; the
                           available message types are listed in
                           section 5.3.

      transaction-id       The transaction ID for this message exchange.

      options              Options carried in this message; options are
                           described in section 22.
**********************************************************************/
class CDHCPMessageHeaderIP6 : public CDHCPMessageHeader
	{	
public:
	CDHCPMessageHeaderIP6(HBufC8*& aMsg) :
		CDHCPMessageHeader(&iOptions, KDHCPHeaderLength, aMsg),
		iOptions(NULL)
		{
#ifdef _FLOG_ACTIVE
		iOptions.iName = _L("Options");
#endif
	    }
	~CDHCPMessageHeaderIP6();

public:
	TInt Parse(const TDhcpRnd& aXid, const TDesC8& aClientId, RBuf8& aServerId);
	virtual void RemoveAllOptions();
   
	DHCPv6::COptionNode* AddOptionL(DHCPv6::TOptionCodes aOpCode, TInt aLength);

	void SetMessageType(TUint8 aMsgType);		
	void SetXid(TUint32 aXid); 				// Transaction ID
	TUint8 GetMessageType() const;
	TUint32 GetXid() const; 				// Transaction ID
   DHCPv6::TStatusCodes GetStatusCode() const;  //returns status code for the message 
                                       //if no status option present it returns success

	inline DHCPv6::COptionList& GetOptions() { return iOptions; };

	void Close() //so as it could be put on the stack and on the cleanup stack
		{
		iOptions.RemoveAllNodes();
		}
	
protected:
	DHCPv6::COptionList iOptions;

	};

inline DHCPv6::TStatusCodes CDHCPMessageHeaderIP6::GetStatusCode() const
   {//for the moment inline
   return DHCPv6::ESuccess;
   }

inline void CDHCPMessageHeaderIP6::RemoveAllOptions()
	{
	iOptions.RemoveAllNodes();
	}

inline TUint8 CDHCPMessageHeaderIP6::GetMessageType() const
/**
  * Retrieve message type
  *
  * @internalTechnology
  *
  */
	{
	return static_cast<TUint8>(*Ptr());
	}

inline void CDHCPMessageHeaderIP6::SetMessageType(TUint8 aMsgType)
/**
  * Set Message Type
  *
  * @internalTechnology
  *
  */
	{
   TUint8* ptr = Ptr();
   *ptr = aMsgType;
	}

inline void CDHCPMessageHeaderIP6::SetXid(TUint32 aXid)
/**
  * Set Transaction ID in message
  *
  * @internalTechnology
  *
  */
	{//set it as a big endian
    TBigEndian::SetValue( Ptr() + KXidOffset, KXidLength, aXid );
	}

inline TUint32 CDHCPMessageHeaderIP6::GetXid() const
/**
  * Retrieve Transaction ID from message
  *
  * @internalTechnology
  *
  */
	{//get as a big endian
   return TBigEndian::GetValue( Ptr() + KXidOffset, KXidLength );
	}

} //namespace DHCPv6
#endif
