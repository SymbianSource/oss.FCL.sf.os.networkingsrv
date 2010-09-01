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
// Implements the DHCPv4 Message format
// 
//

/**
 @file DHCPIP4Msg.cpp
 @internalTechnology
*/

#include "DHCPIP4Msg.h"
#include "DHCP_Std.h"

using namespace DHCPv4;

COptionNode::~COptionNode()
	{
	delete iNext;
	}

struct SOptionHeader
	{
	TInt iCode;
	TInt iExpLength; // length of the payload of the option in case the payload has a fixed length
	TInt iAlignment; // alignment of the payload
	TInt iMinLength; // length of the whole option (including code & length i present)
#ifdef __FLOG_ACTIVE
	const TText8* iName;
#endif
	};

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	const TInt KNumberOfSupportedOptions = 20;
	#else 
const TInt KNumberOfSupportedOptions = 17;
	#endif
#else
	#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	const TInt KNumberOfSupportedOptions = 19;
	#else
const TInt KNumberOfSupportedOptions = 16;
	#endif
#endif 
#ifdef __FLOG_ACTIVE
const SOptionHeader KOptions[KNumberOfSupportedOptions]= 
	{   //iCode,           iExpLength, iAlignment, iMinLength iName
		{EDHCPSubnetMask,       4,       1,          6,       _S8( "EDHCPSubnetMask" )}, //must be the first option
		{EDHCPRouter,           0,       4,          6,       _S8( "EDHCPRouter" )},
#ifdef SYMBIAN_NETWORKING_DHCPSERVER		
		{EDHCPNameServer, 		0,       4,          6,       _S8( "EDHCPNameServer" )},
#endif // #ifdef SYMBIAN_NETWORKING_DHCPSERVER		
		{EDHCPDomainNameServer, 0,       4,          6,       _S8( "EDHCPDomainNameServer" )},
		{EDHCPHostName,         0,       1,          3,       _S8( "EDHCPHostName" )},
		{EDHCPDomainName,       0,       1,          3,       _S8( "EDHCPDomainName" )},
		{EDHCPBroadcastAddr,    4,       4,          6,       _S8( "EDHCPBroadcastAddr" )},
		{EDHCPRequestedIPAddr,  4,       4,          6,       _S8( "EDHCPRequestedIPAddr" )},
		{EDHCPLeaseTime,        4,       4,          6,       _S8( "EDHCPLeaseTime" )},
		{EDHCPMessageType,      1,       1,          3,       _S8( "EDHCPMessageType" )},
		{EDHCPServerID,         4,       4,          6,       _S8( "EDHCPServerID" )},
		{EDHCPParameterReqList, 0,       1,          3,       _S8( "EDHCPParameterReqList" )},
		{EDHCPMaxMsgSize,       2,       2,          4,       _S8( "EDHCPMaxMsgSize" )},
		{EDHCPRenewalT1,        4,       4,          6,       _S8( "EDHCPRenewalT1" )},
		{EDHCPRebindT2,         4,       4,          6,       _S8( "EDHCPRebindT2" )},
//		{EDHCPAuthentication,   0,       1,          12,      _S8( "EDHCPAuthentication" )}, not yet
		{EDHCPSIPServers,		0,		 1,			 3,		  _S8( "EDHCPSIPServers" )},
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		{EDHCPTftpServerName,   0,		 1,			 3,       _S8( "EDHCPTftpServerName")},
		{EDHCPOptionOverload,   0,		 1,			 3,       _S8( "EDHCPOptionOverload")},
		{EDHCPTftpServers,		0,		 1,			 3,		  _S8( "EDHCPTftpServers" )},
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		{EDHCPClientID,         0,       1,          4,       _S8( "EDHCPClientID" )}
	};

void COptionNode::Dump(const TDesC& aTag, const TDesC& aFile)
	{
	RFileLogger f;
	if (f.Connect() == KErrNone)
		{
		f.CreateLog(aTag, aFile, EFileLoggingModeAppend);
		TInt i = 0;
		while (KOptions[i].iCode != OpCode() && ++i < KNumberOfSupportedOptions){/*do nothing*/}
		
		TInt n = iPtr8[1];
		if (i < KNumberOfSupportedOptions)
			{
			f.WriteFormat(_L8("Option Code: %s, length: %d"), KOptions[i].iName, n);
			}
		else
			{
			f.WriteFormat(_L( "Option Code: %d, length %d" ), OpCode(), n );
			}
		if (GetItemLength() > 2)
			{
			f.HexDump(NULL, NULL, GetBodyPtr(), GetLength());
			}
		f.CloseLog();
		f.Close();
		}
	}
#else
const SOptionHeader KOptions[KNumberOfSupportedOptions]= 
	{   //iCode,           iExpLength, iAlignment, iMinLength 
		{EDHCPSubnetMask,       4,       1,          6}, //must be the first option
		{EDHCPRouter,           0,       4,          6},
#ifdef SYMBIAN_NETWORKING_DHCPSERVER		
		{EDHCPNameServer, 		0,       4,          6},
#endif 	//  SYMBIAN_NETWORKING_DHCPSERVER	
		{EDHCPDomainNameServer, 0,       4,          6},
		{EDHCPHostName,         0,       1,          3},
		{EDHCPDomainName,       0,       1,          3},
		{EDHCPBroadcastAddr,    4,       4,          6},
		{EDHCPRequestedIPAddr,  4,       4,          6},
		{EDHCPLeaseTime,        4,       4,          6},
		{EDHCPMessageType,      1,       1,          3},
		{EDHCPServerID,         4,       4,          6},
		{EDHCPParameterReqList, 0,       1,          3},
		{EDHCPMaxMsgSize,       2,       2,          4},
		{EDHCPRenewalT1,        4,       4,          6},
		{EDHCPRebindT2,         4,       4,          6},
		{EDHCPClientID,         0,       1,          4},
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		{EDHCPTftpServerName,   0,		 1,			 3},
		{EDHCPOptionOverload,   0,		 1,			 3},
		{EDHCPTftpServers,		0,		1,			3},
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		{EDHCPSIPServers,		0,		1,			3}
	};
#endif

void COptionList::ParseL(TPtr8& aDes8)
/**
  * Parse message to set pointers into descriptor buffer
  * at locations of each option supplied in a response msg
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("COptionList::ParseL")));
	ASSERT(!iRecord.iFirst);

	COptionNode** ppNode = reinterpret_cast<COptionNode**>(&iRecord.iFirst);
	TUint8 nCode = *aDes8.Ptr();
	COptionNode* pNode;
	TInt i, nItemLen, nHeaderLength;
	while (nCode != EDHCPEnd && aDes8.Length() > 0)
		{
		for (i = 0; KOptions[i].iCode != nCode && ++i < KNumberOfSupportedOptions;) {}
		nHeaderLength = (nCode == EDHCPPad) ? KEDHCPPadLength : KOptionHeaderLength; 
		pNode = new(ELeave) COptionNode(nHeaderLength);
		
		/* ppNode is a pointer to a pointer. It has not got assigned any dynamically allocated memory. But coverity has misinterpreted it an issue.*/
		// coverity [SYMBIAN.CLEANUP STACK]
		// coverity [leave_without_push]
		CleanupStack::PushL(pNode);
		
		/* pNode can not be Null here as the memmory assignment is done using new(ELeave). Thus, there is no need of checking for Null, explicitly .*/
		// coverity[deref_ptr_in_call]	
		pNode->ParseL(aDes8);
		nItemLen = pNode->GetItemLength();
		
		if (aDes8.Length() > 0)
			{
			nCode = *aDes8.Ptr();
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("COptionList::ParseL Option list ended without EDHCPEnd flag")));
			}

		if (i < KNumberOfSupportedOptions)
			{
			// now do some sanity checks and leave if bad results
			TInt nPayloadLength = nItemLen - nHeaderLength;
			*ppNode = pNode;

			if (nItemLen < KOptions[i].iMinLength)	// min length check
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("COptionList::ParseL BadDescriptor 2")));
				*ppNode = NULL;
				User::Leave(KErrBadDescriptor);
				}

			if (nPayloadLength % KOptions[i].iAlignment != 0)	// alignment check
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("COptionList::ParseL BadDescriptor 3")));
				*ppNode = NULL;
				User::Leave(KErrBadDescriptor);
				}

			if (KOptions[i].iExpLength > 0 && nPayloadLength != KOptions[i].iExpLength)	//expected length check
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("COptionList::ParseL BadDescriptor 4")));
				*ppNode = NULL;
				User::Leave(KErrBadDescriptor);
				}

			ppNode = reinterpret_cast<COptionNode**>(&(pNode->iNext));
			CleanupStack::Pop(pNode);
			}
		else
			{
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
			//make sure we parse everything becz we support every option now
			if (pNode)
				{
				*ppNode = pNode;
				ppNode = reinterpret_cast<COptionNode**>(&(pNode->iNext));
				CleanupStack::Pop(pNode);
				}
			else
				{
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
 			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("COptionList::ParseL Code Not Found")));

			/* ppNode is a pointer to a pointer. It has not got assigned any dynamically allocated memory. But coverity has misinterpreted it an issue.*/
			// coverity [SYMBIAN.CLEANUP STACK]
			// coverity [memory_leak]
 			CleanupStack::PopAndDestroy(pNode);
 			pNode=NULL;
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
				}
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
			}
		}


	}

COptionNode* COptionList::AddNodeL(TInt aInitialLength, TInt aHeaderLength)
/**
  * Create a new node for an option in the message
  *
  * @internalTechnology
  */
	{
	COptionNode* pNode = new(ELeave) COptionNode(aHeaderLength);
	AddNode(pNode);
	pNode->Header().SetInitialValue(aInitialLength);
	return pNode;
	}

COptionNode* COptionList::FindOption(TUint8 aOpCode) const
/**
  * Find the location of an option in the message and return a pointer to it
  *
  * @internalTechnology
  */
	{
	COptionNode* pNode = static_cast<COptionNode*>(iRecord.iFirst);
	while (pNode && pNode->OpCode() != aOpCode)
		{
		pNode = static_cast<COptionNode*>(pNode->iNext);
		}
	return pNode;
	}

TInt COptionList::NumberOfItemsInAddressListOption(TUint8 aOpCode) const
/**
  * Return the number of addresses
  * from the specified server option.
  *
  * @internalTechnology
  */
	{
	COptionNode* pNode = FindOption(aOpCode);
	return pNode ? pNode->GetLength()/KIp4AddrByteLength : 0;
	}

TUint32 COptionList::GetAddressFromAddressListOption(TUint8 aOpCode,TInt aPos) const
/**
  * Return the address at position aPos in the specified server option code
  * in TUint32 format.
  *
  * @param aPos Indicates which one of the addresses in the list to return
  * @return TUint32 The address 
  *
  * @internalTechnology
  */
	{
 	COptionNode* pNode = FindOption(aOpCode);
	return pNode ? BigEndian::Get32(pNode->GetBodyPtr() + aPos * KIp4AddrByteLength) : 0;
	}


TUint32 COptionList::GetValueBigEndian(TUint8 aOpCode) const
/**
  * Return a value from the message in big endian format
  *
  * @internalTechnology
  */
	{
	COptionNode* pNode = FindOption(aOpCode);
	return pNode ? pNode->GetBigEndian() : 0;
	}

TInt COptionList::NumberOfDomainServers() const
/**
  * Return the number of domain name server addresses
  * from the domain name server option.  There are
  * expected to be more than one.
  *
  * @internalTechnology
  */
	{
	return NumberOfItemsInAddressListOption(EDHCPDomainNameServer);
	}

TUint32 COptionList::GetDomainNameServer(TInt aPos) const
/**
  * Return the domain name server address at position aPos
  * in TUint32 format.
  *
  * @param aPos Indicates which one of the domain name server addresses to return
  * @return TUint32 The address of the domain name server
  *
  * @internalTechnology
  */
	{
	return GetAddressFromAddressListOption(EDHCPDomainNameServer,aPos);
	}



TBool COptionList::CopyHostNameL(HBufC8*& aBuf8) const
/**
  * Return a copy of the host name given in the message
  *
  * @param aBuf8 A descriptor for the host name to be written into
  * @return TBool If the copy was successful or not 
  *
  * @internalTechnology
  */
	{
	COptionNode* pNode = FindOption(EDHCPHostName);
	if (pNode)
		{
		pNode->CopyBodyToL(aBuf8);
		}
	return pNode != NULL;
	}

TBool COptionList::CopyDomainNameL(HBufC8*& aBuf8) const
/**
  * Return a copy of the domain name given in the message 
  *
  * @param aBuf8 A descriptor for the domain name to be written into
  * @return TBool If the copy was successful or not 
  */
	{
	COptionNode* pNode = FindOption(EDHCPDomainName);
	if (pNode)
		{
		pNode->CopyBodyToL(aBuf8);
		}
	return pNode != NULL;
	}

TBool COptionList::CopyDomainSearchL(HBufC8*& aBuf8) const
/**
  * Return a copy of the domain search list given in the message 
  *
  * @param aBuf8 A descriptor for the domain search list to be written into
  * @return TBool If the copy was successful or not 
  */
    {
    COptionNode* pNode = FindOption(EDHCPDomainSearch);
    if (pNode)
        {
        pNode->CopyBodyToL(aBuf8);
        }
    return pNode != NULL;
    }

CDHCPMessageHeaderIP4::CDHCPMessageHeaderIP4(HBufC8*& aMsg) :
	CDHCPMessageHeader(&iXid, 4, aMsg), //|     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
		iXid(&iSecs, 4), iSecs(&iFlags, 2), iFlags(&iCiaddr, 2),
		iCiaddr(&iYiaddr, 4), iYiaddr(&iSiaddr, 4), iSiaddr(&iGiaddr, 4),
		iGiaddr(&iChaddr, 4), iChaddr(&iSname, 16), iSname(&iFile, 64),
		iFile(&iCookie, 128), iCookie(&iOptions, 4),
		iOptions(NULL)
		{
#ifdef _FLOG_ACTIVE
		iName = _L("op ht hl ho");
		iXid.iName = _L("Xid");
		iSecs.iName = _L("Secs");
		iFlags.iName = _L("Flags");
		iCiaddr.iName = _L("Ciaddr");
		iYiaddr.iName = _L("Yiaddr");
		iSiaddr.iName = _L("Siaddr");
		iGiaddr.iName = _L("Giaddr");
		iChaddr.iName = _L("Chaddr");
		iSname.iName = _L("Sname");
		iFile.iName = _L("File");
		iCookie.iName = _L("Cookie");
#endif
		}
	
CDHCPMessageHeaderIP4::~CDHCPMessageHeaderIP4()
	{
	Close();
	}

COptionNode* CDHCPMessageHeaderIP4::AddOptionL(TDHCPOptionCodes aOpCode, TInt aLength)
/**
  * First stage of adding an option to the message. Calls AddNode to create space
  * for message.
  *
  * @param aOpCode The opcode of the options to be added
  * @param aLength The length of the option data
  * @return COptionNode* The pointer to the option in the message
  *
  @ @internalTechnology
  */
	{
	COptionNode* pNode = iOptions.AddNodeL(aLength, aLength > 0 ? KOptionHeaderLength : 1);
	TPtr8 ptr = iMsg->Des();
	pNode->InitialiseL(ptr);
	pNode->SetOpCode(static_cast<TUint8>(aOpCode));
	return pNode;
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
void CDHCPMessageHeaderIP4::ParseClientMsgL()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP4::ParseL")));
	
	TPtr8 ptr = iMsg->Des();
	iRecord.ParseL(ptr);
	// now it is parsed, first check whether this is actually a REPLY message from a DHCP Server...
	if (GetOpCode() != EDHCPBootRequest)
		{
		User::Leave(KErrNotFound);
		}
	}	

void CDHCPMessageHeaderIP4::GetCHAddr(TSockAddr& aSockAddr) 
/**
  * Retrieve Your IP Address from message
  *
  * @internalTechnology  *
  */
	{
	aSockAddr.SetFamily(KAFUnspec);
	aSockAddr.SetLength(KHwAddrOffset);
	aSockAddr.Append(iChaddr.GetBodyDes());
	}	
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
void CDHCPMessageHeaderIP4::GetClientHwAddr(TSockAddr& aSockAddr) 
	{
/**
  * Return the Client Hardware Address.
  *
  * @internalTechnology
  */
	aSockAddr.Append(iChaddr.GetBodyDes());
	}
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#endif // SYMBIAN_NETWORKING_DHCPSERVER		
	
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
void CDHCPMessageHeaderIP4::FinishL(TDesC8& aClientId, const TDesC8* aOptionsPtr)
/**
  * Put finishing touches to message for sending. Mainly used for sending DHCPINFORM message 
  * Puts the requested parameter list to 'Parameter Request List' 
  * If aFlag is set to TRUE, then all parameters have to requested including default parameters+extra requested parameters
  * Basically copies in the magic cookie (99.130.83.99)
  * and the end marker then set the length of the descriptor
  * as we have been pushing data into the descriptor manually
  *
  * @see RFC 2131 for explanation of the magic cookie!
  * @param aClientId The client ID string to be added to the message
  * @param aOptionsPtr Contains a list of opcodes to be sent in the parameter list of the DHCPINFORM message
  *
  * @internalTechnology
  */
#else 
void CDHCPMessageHeaderIP4::FinishL(TDesC8& aClientId)
/**
  * Put finishing touches to message for sending
  * Basically copies in the magic cookie (99.130.83.99)
  * and the end marker then set the length of the descriptor
  * as we have been pushing data into the descriptor manually
  *
  * @param aClientId The client ID string to be added to the message
  * @see RFC 2131 for explanation of the magic cookie!
  *
  * @internalTechnology
  */
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageBase::FinishL")));
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	if(!iDHCPServerImpl)
	{
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	
	TUint8 reqListArray[KDhcpParameterRequestListLen] = {EDHCPHostName, EDHCPDomainNameServer, 
							EDHCPDomainName, EDHCPSubnetMask, EDHCPRouter, EDHCPBroadcastAddr, EDHCPSIPServers};
	TPtr8 ptr(reqListArray, KDhcpParameterRequestListLen, KDhcpParameterRequestListLen);
	
	// +++++++++++++++++++++++ Client ID +++++++++++++++++++++++++++++++++++++++++/
	AddOptionL(EDHCPClientID, aClientId.Length())->GetBodyDes().Copy(aClientId);
		
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	RBuf8 appendOpCodeList;
	appendOpCodeList.CreateL(ptr);
	if (aOptionsPtr)
		{
		TInt optLen=aOptionsPtr->Length();
		appendOpCodeList.ReAllocL(KDhcpParameterRequestListLen+optLen);
		appendOpCodeList.Append(aOptionsPtr->Ptr(),optLen);
		}
	AddOptionL(EDHCPParameterReqList, appendOpCodeList.Length())->GetBodyDes().Copy(appendOpCodeList);
	appendOpCodeList.Close();
#else
	// +++++++++++++++++++++++ Parameter Request List ++++++++++++++++++++++++++++/
	AddOptionL(EDHCPParameterReqList, ptr.Length())->GetBodyDes().Copy(ptr);
#endif		
	// +++++++++++++++++++++++ Maximum message size (2 bytes) ++++++++++++++++++++/
	AddOptionL(EDHCPMaxMsgSize, 2)->SetBigEndian(KDhcpMaxMsgSizeIP4);
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
		
	TInetAddr magic;
	_LIT(magicCookie, "99.83.130.99");
	User::LeaveIfError(magic.Input(magicCookie));	// essential the magic cookie is correct
	iCookie.SetLittleEndian(magic.Address());
		
	ASSERT(!iOptions.FindOption(EDHCPEnd));
	AddOptionL(EDHCPEnd, 0);

   //add padding if msg shorter than 300 bytes
 	TInt len=iMsg->Length();
 	TPtr8 des=iMsg->Des();
	if (len<300)
      {
		des.AppendFill(EDHCPPad, 300-len);
      }
	}
	
void CDHCPMessageHeaderIP4::ParseL()
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
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP4::ParseL")));
	
	TPtr8 ptr = iMsg->Des();
	iRecord.ParseL(ptr);
	// now it is parsed, first check whether this is actually a REPLY message from a DHCP Server...
	if (GetOpCode() != EDHCPBootReply)
		{
		User::Leave(KErrNotFound);
		}
	}
	
	
void CDHCPMessageHeaderIP4::SetHeader(TUint8 aOpCode,TUint8 aType,TUint8 aLength,TUint8 aHops)
/**
  * Sets Op Code in message
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPMessageHeaderIP4::SetHeader")));
		
	TUint8* ptr = &(GetBodyPtr()[0]);
	*ptr = aOpCode;
	*++ptr = aType;
	*++ptr = aLength;
	*++ptr = aHops;
	}
	
void CDHCPMessageHeaderIP4::SetCHAddr(TSockAddr& aAddr)
/**
  * Sets If Hardware Address in message
  *
  * @internalTechnology
  * 
  */
	{
	TUint len = aAddr.Length()-KHwAddrOffset;
	if(len > KIp4ChAddrMaxLength) { len = KIp4ChAddrMaxLength; }
	iChaddr.GetBodyDes().Copy(aAddr.Mid(KHwAddrOffset, len));
	}

void CDnsUpdateOption::ToStringL(RBuf8& aBuf8) const
/**
  * Writes DNS update option data to a string suitable for sending out
  * on to the network
  *
  * @internalTechnology
  * 
  */
	{
	RBuf8 encodedDomainName;
	encodedDomainName.CleanupClosePushL();

	TDomainNameArray domainNames;
	CleanupClosePushL(domainNames);
	domainNames.AppendL(iDomainName);

	CDomainNameCodec* domainNameEncoder = new(ELeave) CDomainNameCodec();
	CleanupStack::PushL(domainNameEncoder);


	domainNameEncoder->EncodeL(domainNames, encodedDomainName);


	CleanupStack::PopAndDestroy(domainNameEncoder);
	CleanupStack::PopAndDestroy(&domainNames); // closes the RBuf8	


	aBuf8.Zero();

	aBuf8.ReAllocL( 3 + encodedDomainName.Length() );

	aBuf8.Append((TChar)(iFlags.Value()));
	aBuf8.Append((TChar)iRCode1);
	aBuf8.Append((TChar)iRCode2);
	aBuf8.Append(encodedDomainName);

	CleanupStack::PopAndDestroy(&encodedDomainName); // closes the array
	}


