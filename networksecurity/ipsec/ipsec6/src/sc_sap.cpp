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
// sc_sap.cpp - IPv6/IPv4 IPSEC security policy service access point
//

#include <ip6_hook.h>
#include "ipsec.h"
#include "sc.h"
#include "ipseclog.h"

//
CProviderSecpol::CProviderSecpol(MSecurityPolicyManager& aProtocol) : iProtocol(aProtocol)
	/**
	* Constructor.
	*
	* @param aProtocol The SECPOL protocol object reference
	*/
	{
	}

//	Required Pure virtual methods
//	*****************************
void CProviderSecpol::Start()
	{
	iListening = 1;		// For now, always set. If application does not
						// read the socket, then one packet is queued and
						// the rest dropped (iQueueLimit <= 0).
	}


// CProviderSecpol::Write
// **********************
TUint CProviderSecpol::Write(const TDesC8 &aDesc, TUint /*aOptions*/, TSockAddr* /*aAddr=NULL*/)
	/**
	* Create a new security policy from the policy definition.
	*
	* The policy manager application uses this datagram socket API to load the IPsec policy.
	* Each write (datagram) represents a complete policy and it totally replaces any previously
	* loaded policy.
	*
	* The definition string must entirely be in this one buffer, and if the policy definition
	* is long, then arranging this may require changing the socket send buffer size
	* (#KSOSendBuf option).
	*
	* @param aDesc	The new policy (currently as UNICODE string)
	* @param options Not used
	* @param anAddr Not used.
	* @return 1 (always)
	*
	* If there are any syntax errors or problems with parsing the policy information, write
	* generates a "fake" IPv4 packet  (source and destination are 0.0.0.0) containing the
	* failed line as a payload.
	*
	* This packed is then given to the Deliver method of the policy protocol instance
	* where it is handled like any other dropped packets. The source port of the information
	* block contains the error code.
	*/
	{
	TInt result = KErrNone;

	if (!iSocket)
		return 1;

	for (;;)		// JUST TO PROVIDE NICE "BREAK"-EXITS FOR ERRORS!
		{
		TUint offset = 0;
#if defined(_UNICODE)
		// This assumes the policy socket on UNICODE is written with *WIDE* characters.
		TPtrC ptr((TUint16 *)aDesc.Ptr(), aDesc.Length() / 2);
#else
		TPtrC ptr(aDesc);
#endif
		if ((result = iProtocol.SetPolicy(ptr, offset)) == KErrNone)
			{
			return 1;	// The successful policy load exit!
			}

		// Failure to parse the policy specification
		//
		// Try to extract the line where error occurred and pass
		// this information to someone. Currently, construct a
		// a fake IP packet with the piece of erroneous policy
		// file as a content (speed is of no concern here)
		//
		//	(A quick kludge, some more elegant error reporting
		//	might be designed later... -- msa)
		//
		TInt start = offset;
		TInt end = offset;
		while (start > 0)
			{
			if (ptr[start-1] == '\r' || ptr[start-1] == '\n')
				break;
			--start;
			}
		while (++end < ptr.Length())
			if (ptr[end] == '\r' || ptr[end] == '\n')
				break;
#ifdef _LOG
		const TPtrC tmp(ptr.Mid(start, end-start));
		Log::Printf(_L("Policy syntax error: %S"), &tmp);
#endif
#if defined(_UNICODE)
		TPtrC8 line((TUint8 *)(ptr.Ptr() + start), (end - start - 1) * 2);
#else
		TPtrC8 line(ptr.Ptr() + start, end - start - 1);
#endif
		RMBufRecvPacket packet;
		RMBufRecvInfo* info = NULL;
		TRAPD(err, info = packet.CreateL(line, TInet6HeaderIP4::MinHeaderLength()));
		err = err;  // Clearing "never used" warning caused by TRAPD
		if(info)
			{
			//
			// Setup "fake" IPv4 header
			//
			packet.Align(TInet6HeaderIP4::MinHeaderLength());
			TInet6HeaderIP4 *ip = (TInet6HeaderIP4 *)packet.First()->Ptr();
			ip->Init();
			ip->SetTotalLength(info->iLength);
			ip->SetDstAddr(0);
			ip->SetSrcAddr(0);
			info->iSrcAddr.SetPort(result);
			info->iDstAddr.SetPort(offset - start);
			iProtocol.Deliver(packet);	// Deliver takes "unpacked" packets...
			}
		packet.Free();		// Deliver only makes copies, must release it self!
		//
		// ALWAYS BREAK, to terminate the "fake" loop construct!
		//
		break;
		}

	// Pass the error to socket server
	//
	// Apparently this is not enough for datagram. This does not cause the application
	// Send request to get error status! Something more complex is required... -- msa
	//
	iSocket->Error(result, MSocketNotify::EErrorSend);
	//
	// As long as SECPOL socket is "message based", this must always return 1
	// (no blocking occurs).
	return 1;
	}

TBool CProviderSecpol::IsReceiving()
	/**
	* @return TRUE, if socket can accept dropped packets.
	*/
	{
	if (iQueueLimit < 0 || !iListening)
		{
		// Receive Queue limit is full, cannot receive this packet
		iPacketsDropped++;
		return FALSE;
		}
	return TRUE;
	}

