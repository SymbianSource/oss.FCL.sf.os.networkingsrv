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
// DHCPv4 Message wrapper header file
// 
//

/**
 @file DHCPMsg.h
*/

#ifndef __DHCPIP4MSG_H__
#define __DHCPIP4MSG_H__

#include <babitflags.h>
#include "DHCPMsg.h"
#include "DomainNameDecoder.h"

class TInetAddr;
class RSocket;
class TSockAddr;


namespace DHCPv4
{

const TUint8	KHwAddrTypeOffset = 1;
const TUint8	KHwAddrLengthOffset = 2;
const TUint8	KHopOffset = 3;
const TUint8    KLengthUptoSecs = 8;
const TUint8    KSecsElapsedLength = 2;

const TUint8 KDhcpSipEncodingOffset = 2;
const TUint8 KDhcpSipLengthOffset = 1;
const TUint8 KDhcpSipEncodingDomains = 0;
const TUint8 KDhcpSipEncodingAddresses = 1;

enum TDHCPv4MessageOpCode
/**
  * TDHCPv4MessageOpCode
  *
  * The message op code constants for
  * DHCPv4 messages. A message is either
  * a request of reply. DHCP clients always
  * send requests to DHCP servers that always
  * send replies.
  *
  * @internalTechnology
  */
	{
	EDHCPBootRequest = 1,
	EDHCPBootReply
	};

enum TDHCPv4MessageType
/**
  * TDHCPv4MessageType
  *
  * The message type constants for the DHCPv4
  * message type option for inclusion in DHCPv4
  * messages.
  *
  * @internalTechnology
  */
	{
	EDHCPDiscover = 0x01,		// request
	EDHCPOffer,					// reply
	EDHCPRequest,				// request
	EDHCPDecline,				// request
	EDHCPAck,					// reply
	EDHCPNak,					// reply
	EDHCPReleaseMsg,			// request
	EDHCPInform					// request
	};

enum TDHCPOptionCodes
/**
  * TDHCPOptionCodes
  *
  * The constant values for each supported
  * option type.
  *
  * @internalTechnology
  */
	{
	EDHCPPad = 0,
	EDHCPSubnetMask = 1,
	EDHCPRouter = 3,
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	EDHCPNameServer = 5,
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	EDHCPDomainNameServer = 6,
	EDHCPHostName = 12,
	EDHCPDomainName = 15,
	EDHCPBroadcastAddr = 28,
	EDHCPRequestedIPAddr = 50,
	EDHCPLeaseTime = 51,
	EDHCPMessageType = 53,
	EDHCPServerID = 54,
	EDHCPParameterReqList = 55,
	EDHCPMaxMsgSize = 57,
	EDHCPRenewalT1 = 58,
	EDHCPRebindT2 = 59,
	EDHCPClientID = 61,
   	EDHCPDNSUpdate = 81,
   	EDHCPAuthentication = 90,
	EDHCPDomainSearch = 119,
	EDHCPSIPServers = 120,
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	EDHCPOptionOverload = 52,
	EDHCPTftpServerName = 66,			
	EDHCPTftpServers = 150,	
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	EDHCPEnd = 255
	};
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
enum TDHCPOverloadOptions
	{
	EDHCPFile = 1,
	EDHCPSname = 2,
	EDHCPBoth = 3
	};
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	

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
	ASSERT(this->iValueLength <= 2);
	return this->iValueLength == 2 ? LAYOUT::GetValue(aPtr8 + 1, this->iValueLength - 1) : 0;
	}

template <typename LAYOUT>
void TOptionHeader<LAYOUT>::SetValue(TUint8* aPtr8, TInt aValue) const
	{
	ASSERT(this->iValueLength <= 2);
	if (this->iValueLength == 2)
		{
		LAYOUT::SetValue(aPtr8 + 1, this->iValueLength - 1, aValue);
		}
	}

class COption : public CItem<TOptionHeader<TLittleEndian> >
	{
public:
	COption(CItemBase* aNext, TInt aValue) :
		CItem<TOptionHeader<TLittleEndian> >(aNext, aValue)
		{
		}

	};

// lengths of headers for different option codes
const TInt KEDHCPPadLength = 1;
const TInt KOptionHeaderLength = 2;

//we may want to have an derived class for options needing special attention 
//(permitted value cheking)
class COptionNode : public COption
/**
  * we may want to have a derived class for options needing special attention 
  * (permitted value cheking)
  *
  * @internalTechnology
  */
	{
	friend class COptionList;

public:
	TUint8 OpCode() const
		{
		return iPtr8[0];
		}
	void SetOpCode(TUint8 aOpCode)
		{
		iPtr8[0] = aOpCode; //should panic if not long enough
		}
#ifdef __FLOG_ACTIVE
	virtual void Dump(const TDesC& aTag, const TDesC& aFile);
#endif

protected:
	COptionNode(TInt aHeaderLength);
	virtual ~COptionNode();
	};

inline COptionNode::COptionNode(TInt aHeaderLength) :
	COption(NULL, aHeaderLength)
	{
	}

/******DHCP extension template []->optional - the code dependent******
   Code   [Len]  [Octets]
   +-----+-----+-----+-----+-----+---
   |Code |  n  |  o0 |  o1 |  o2 | ...
   +-----+-----+-----+-----+-----+---
*********************************************************************/
class COptionList : public CListItem
	{
public:
	COptionList(CItemBase* aNext);
   
	COptionNode* AddNodeL(TInt aInitialLength, TInt aHeaderLength);
	COptionNode* FindOption(TUint8 aOpCode) const;

	virtual void ParseL(TPtr8& aDes8);

	TInt NumberOfItemsInAddressListOption(TUint8 aOpCode) const;
	TUint32 GetAddressFromAddressListOption(TUint8 aOpCode,TInt aPos) const;
	
	TUint32 GetSubnetMask() const;
	TUint32 GetRouterAddress() const;
	TInt NumberOfDomainServers() const;
	TUint32 GetDomainNameServer(TInt aPos) const;
	TUint32 GetBroadcastAddress() const;
	TUint32 GetLeaseTime() const;
	TUint8 GetMessageType() const;
	TUint32 GetServerId() const;
	TUint32 GetRenewalTime() const;
	TUint32 GetRebindTime() const;
	TInt CopyHostNameL(HBufC8*& aBuf8) const;
	TInt CopyDomainNameL(HBufC8*& aBuf8) const;

#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	TUint32 GetRequestedIPAddress() const;
#endif // SYMBIAN_NETWORKING_DHCPSERVER	

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	TUint8 GetOptionOverload() const;
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
protected:
	TUint32 GetValueBigEndian(TUint8 aOpCode) const;
public:
	TInt CopyDomainSearchL(HBufC8*& aBuf8) const; //For retreiving option data value for DHCP option 119 from server's offer packet
	};

inline COptionList::COptionList(CItemBase* aNext) : CListItem(aNext, 0)
	{
	}

inline TUint32 COptionList::GetSubnetMask() const
	{
	return GetValueBigEndian(EDHCPSubnetMask);
	}
	
inline TUint32 COptionList::GetRouterAddress() const
	{
	// Only look at the first item, because tcpip6 only supports 1 router
	//  Subsequent addresses shall be ignored.
	return GetAddressFromAddressListOption(EDHCPRouter,0);
	}

inline TUint32 COptionList::GetBroadcastAddress() const
	{
	return GetValueBigEndian(EDHCPBroadcastAddr);
	}

inline TUint32 COptionList::GetLeaseTime() const
	{
	return GetValueBigEndian(EDHCPLeaseTime);
	}

inline TUint8 COptionList::GetMessageType() const
	{
	return (TUint8)GetValueBigEndian(EDHCPMessageType);
	}

inline TUint32 COptionList::GetServerId() const
	{
	return GetValueBigEndian(EDHCPServerID);
	}

inline TUint32 COptionList::GetRenewalTime() const
	{
	return GetValueBigEndian(EDHCPRenewalT1);
	}

inline TUint32 COptionList::GetRebindTime() const
	{
	return GetValueBigEndian(EDHCPRebindT2);
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
inline TUint32 COptionList::GetRequestedIPAddress() const
	{
	return GetValueBigEndian(EDHCPRequestedIPAddr);
	}	
#endif // SYMBIAN_NETWORKING_DHCPSERVER			

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
inline TUint8 COptionList::GetOptionOverload() const
 	{
 	return (TUint8)GetValueBigEndian(EDHCPOptionOverload);
 	}
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS		
/**********************DHCP message header****************************
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
   +---------------+---------------+---------------+---------------+
   |                            xid (4)                            |
   +-------------------------------+-------------------------------+
   |           secs (2)            |           flags (2)           |
   +-------------------------------+-------------------------------+
   |                          ciaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          yiaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          siaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          giaddr  (4)                          |
   +---------------------------------------------------------------+
   |                                                               |
   |                          chaddr  (16)                         |
   |                                                               |
   |                                                               |
   +---------------------------------------------------------------+
   |                                                               |
   |                          sname   (64)                         |
   +---------------------------------------------------------------+
   |                                                               |
   |                          file    (128)                        |
   +---------------------------------------------------------------+
   |                          cookie  (4)                          |
   +---------------------------------------------------------------+
   |                                                               |
   |                          options (variable)                   |
   +---------------------------------------------------------------+
**********************************************************************/

class CDHCPMessageHeaderIP4 : public CDHCPMessageHeader
/**
  * This class implements the specifics for the DHCPv4 messages
  */
	{
public:
	CDHCPMessageHeaderIP4(HBufC8*& aMsg);
	~CDHCPMessageHeaderIP4();

public:
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	TUint32 GetSIAddr() const;//IP Address of the Next Server	
	TPtr8 GetSName() const;	//Hostname of next available server
	void FinishL(TDesC8& aClientId, const TDesC8* aOptionsPtr=NULL);
#else  // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	void FinishL(TDesC8& aClientId);
#endif
	void ParseL();
#ifdef SYMBIAN_NETWORKING_DHCPSERVER 	
	void ParseClientMsgL();
	void SetYIAddr(TUint32 aYiaddr); 				// Offered Client address
	void GetCHAddr(TSockAddr& aSockAddr);
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	void GetClientHwAddr(TSockAddr& aSockAddr);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
	TUint16 GetFlags();
#endif 	// SYMBIAN_NETWORKING_DHCPSERVER
	
	COptionNode* AddOptionL(TDHCPOptionCodes aOpCode, TInt nLength);
	virtual void RemoveAllOptions();

	void SetHeader(TUint8 aOpCode, TUint8 aType, TUint8 aLength, TUint8 aHops);
	void SetXid(TUint32 aXid); 				// Transaction ID
	void SetSecs(TUint16 aSecs);			// Seconds elapsed since clien began address aquisition
	void SetFlags(TUint16 aFlag);
	void SetCIAddr(TUint32 aAddr);			// If IP Address
	void SetCHAddr(TSockAddr& aAddr); 			// If Hardware Address
	TUint8 GetOpCode() const;
	TUint32 GetXid() const; 				// Transaction ID
	TUint32 GetCIAddr() const;				// If IP Address
	TUint32 GetYIAddr() const;				// "your" (If) IP address

	void Close() //so as it could be put on the stack and on the cleanup stack
		{
		iOptions.RemoveAllNodes();
		}

public:
	CConstItem  iXid;
	CConstItem  iSecs;
	CConstItem  iFlags;
	CConstItem  iCiaddr;
	CConstItem  iYiaddr;
	CConstItem  iSiaddr;
	CConstItem  iGiaddr;
	CConstItem  iChaddr;
	CConstItem  iSname;
	CConstItem  iFile;
	CConstItem  iCookie;
	COptionList iOptions;
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	TBool iDHCPServerImpl;
#endif 	// SYMBIAN_NETWORKING_DHCPSERVER
	};

inline void CDHCPMessageHeaderIP4::RemoveAllOptions()
	{
	iOptions.RemoveAllNodes();
	}

inline TUint8 CDHCPMessageHeaderIP4::GetOpCode() const
/**
  * Retrieve Op code from message
  *
  * @internalTechnology
  *
  */
	{
	return GetBodyPtr()[0];
	}

inline void CDHCPMessageHeaderIP4::SetCIAddr(TUint32 aAddr)
/**
  * Set If IP Address in message
  *
  * @internalTechnology
  *
  */
	{
	iCiaddr.SetBigEndian(aAddr);
	}

inline void CDHCPMessageHeaderIP4::SetXid(TUint32 aXid)
/**
  * Set Transaction ID in message
  *
  * @internalTechnology
  *
  */
	{
	iXid.SetLittleEndian(aXid);
	}
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER 	
inline void CDHCPMessageHeaderIP4::SetYIAddr(TUint32 aYiaddr)
/**
  * Set Transaction ID in message
  *
  * @internalTechnology
  *
  */
	{
	iYiaddr.SetBigEndian(aYiaddr);
	}
	
inline TUint32 CDHCPMessageHeaderIP4::GetCIAddr() const
/**
  * Retrieve Client IP Address from message
  *
  * @internalTechnology
  *
  */
	{
	return iCiaddr.GetBigEndian();
	}

inline TUint16 CDHCPMessageHeaderIP4::GetFlags()
/**
  * Set flags in message
  *
  * @internalTechnology
  *
  */
	{
	return iFlags.GetBigEndian();
	}
	
#endif // SYMBIAN_NETWORKING_DHCPSERVER

inline void CDHCPMessageHeaderIP4::SetSecs(TUint16 aSecs)
/**
  * SSet Number of seconds since commencement of configuration process in message
  *
  * @internalTechnology
  *
  */
	{
	iSecs.SetBigEndian(aSecs);
	}

inline void CDHCPMessageHeaderIP4::SetFlags(TUint16 aFlag)
/**
  * Set flags in message
  *
  * @internalTechnology
  *
  */
	{
	iFlags.SetBigEndian(aFlag);
	}

inline TUint32 CDHCPMessageHeaderIP4::GetXid() const
/**
  * Retrieve Transaction ID from message
  *
  * @internalTechnology
  *
  */
	{
	return iXid.GetLittleEndian();
	}

inline TUint32 CDHCPMessageHeaderIP4::GetYIAddr() const
/**
  * Retrieve Your IP Address from message
  *
  * @internalTechnology
  *
  */
	{
	return iYiaddr.GetBigEndian();
	}

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

inline TUint32 CDHCPMessageHeaderIP4::GetSIAddr() const
/**
  * Get next server IP Address in message
  *
  * @internalTechnology
  *
  */
	{
	return iSiaddr.GetBigEndian();
	}
	
inline TPtr8 CDHCPMessageHeaderIP4::GetSName() const
/**
  * Get next server hostname in message
  *
  * @internalTechnology
  *
  */
  {
  return iSname.GetBodyDes();
  }	
 
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
const TUint8 KDnsUpdateRCode1Offset = 0x01;
const TUint8 KDnsUpdateRCode2Offset = 0x02;

class CDnsUpdateOption : public COptionNode
	{
public:
	enum TDnsUpdateFlag
		{
		EDnsUpdateFlagN = 0x03,
		EDnsUpdateFlagE = 0x02,
		EDnsUpdateFlagO = 0x01,
		EDnsUpdateFlagS = 0x00
		};
	
public:
	CDnsUpdateOption() : COptionNode(KOptionHeaderLength)
		{
		};
		
	virtual ~CDnsUpdateOption()
		{
		};
	
	TBool GetFlag(const TDnsUpdateFlag aFlag) const;
	void SetFlag(const TDnsUpdateFlag aFlag);	
	void ClearFlag(const TDnsUpdateFlag aFlag);
	TUint8 GetRCode1();
	TUint8 GetRCode2();
	const TDomainName& GetDomainName() const;
	void SetDomainName(const TDomainName& aDomainName);
	void ToStringL(RBuf8& aString) const;
	
private:
	TBitFlags8 iFlags;
	TInt8 iRCode1;
	TInt8 iRCode2;
	TDomainName iDomainName;
	};

inline TBool CDnsUpdateOption::GetFlag(const TDnsUpdateFlag aFlag) const
	{
	return iFlags.IsSet(aFlag);
	}

inline void CDnsUpdateOption::SetFlag(const TDnsUpdateFlag aFlag)
	{
	iFlags.Set(aFlag);
	}

inline void CDnsUpdateOption::ClearFlag(const TDnsUpdateFlag aFlag)
	{
	iFlags.Clear(aFlag);
	}

inline TUint8 CDnsUpdateOption::GetRCode1() 
	{
	return iRCode1;
	}

inline TUint8 CDnsUpdateOption::GetRCode2() 
	{
	return iRCode2;
	}
	
inline const TDomainName& CDnsUpdateOption::GetDomainName() const
	{
	return iDomainName;
	}
	
inline void CDnsUpdateOption::SetDomainName(const TDomainName& aDomainName)
	{
	iDomainName.Copy(aDomainName);
	}
	
}//namespace DHCPv4
#endif


