// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include <in_iface.h>
#include "PPPAUTH.H"

#include "ppppap.h"
#include "chapmd5.h"
#include "MSCHAP.H"
#include "mschap2.h"
#include <commsdattypeinfov1_1.h>
#include "PPPConfig.h"

#if defined(_DEBUG)
const TInt KLCPConfigOptionTypeFieldSize = 1;
const TInt KLCPConfigOptionLengthFieldSize = 1;
#endif
const TInt KAuthenticationProtocolOptionFieldSize = 2;
const TInt KChapAlgorithmFieldSize = 1;


//

void CPppAuthentication::InitL(
	CPppLcp* aPppLcp)
/**
Initializes the authentication protocol.
@param aPppLcp Parent LCP instance
@leave The method does not leave */			
	{
	MPppRecvr::Init(aPppLcp, PppPhase(), PppId());
	}

CPppAuthentication::~CPppAuthentication()
/**
Destructor, generates a CancelAuthenticate notification to the 
parent LCP instance */			
	{
	if (iPppLcp != 0)
		iPppLcp->CancelAuthenticate(this);
	}

void CPppAuthentication::AuthenticateRequest()
/**
Generates an Authenticate notification to the parent LCP instance */			
	{
	ASSERT(iPppLcp != 0);
	iPppLcp->Authenticate(this);
	}

void CPppAuthentication::CallAuthenticateComplete(
	TInt aRes)
/**
Notifies the derived class that the LCP authentication is completed
@param aRes Code describing the authentication result 
	(KErrNone: authentication OK) */			
	{
	ASSERT(iPppLcp != 0);
	iFlags |= KPppApAuthReqDone;
	AuthenticateComplete(aRes);
	}

void CPppAuthentication::DoFail(
	TInt aErrorCode)
/**
Notifies the parent LCP instance of an error encountered 
during authentication
@param aErrorCode Describes the authentication failure */			
	{
	ASSERT(iPppLcp != 0);
	iPppLcp->Progress(EPppProgressAuthenticationComplete, aErrorCode);
	iPppLcp->PhaseAborted(aErrorCode);
	}


void CPppAuthentication::DoSucceed()
/**
Notifies the parent LCP instance that the 
authentication phase completed successfully */			
	{
	ASSERT(iPppLcp != 0);

	iPppLcp->Progress(EPppProgressAuthenticationComplete, KErrNone);
	iPppLcp->PhaseComplete();
	}


//

static const TInt KOptionListLen = 1;
static const TUint8 OptionList[KOptionListLen] =
	{
	KPppLcpOptAuthenticationProtocol
	};
			
CPppAcp* CPppAcp::NewL(
	CPppLcp* aLcp)
/**
Creates a new CPppAcp instance
@param aLcp Parent LCP instance
@return Newly created CPppAcp instance */		
	{
	CPppAcp* acp = new (ELeave) CPppAcp(aLcp);
	return acp;
	}

CPppAcp::CPppAcp(
	CPppLcp* aLcp)
	: MPppOptionHandler()
/**
Constructor, register the current instance as a handler 
for the authentication option type
@param aLcp Parent LCP instance */		
	{
	iPppLcp = aLcp;
	OptRegister(aLcp, OptionList, KOptionListLen);
	}

CPppAcp::~CPppAcp()
/**
Destructor, deletes the protocol objects created by this instance */		
	{
	DeleteProtocols();
	}

void CPppAcp::DeleteProtocols()
/**
Deletes the member protocol objects if valid. */		
	{
	if (iServerProtocol)
		{
		delete iServerProtocol;
		iServerProtocol = NULL;
		}
	if (iClientProtocol)
		{
		delete iClientProtocol;
		iClientProtocol = NULL;
		}
	}
	
void CPppAcp::OptNegotiationStarted()
/**
Initializes the option handler for a new LCP negotiation session */		
	{
	DeleteProtocols();
		
	iClientId = KPppIdUnknown;
	iClientSubId = KPppIdUnknown;
	iServerId = KPppIdUnknown;
	iServerSubId = KPppIdUnknown;

	iProtocolOffered = 0;
	iCHAPTypeOffered = 0;
	
	// default to fallback
	}

void CPppAcp::OptNegotiationAborted()
/**
Executed when the negotiation is aborted, 
resets the negotiable options */		
	{
	iClientId = KPppIdUnknown;
	iClientSubId = KPppIdUnknown;
	iServerId = KPppIdUnknown;
	iServerSubId = KPppIdUnknown;
	}

void CPppAcp::OptNegotiationComplete()
/**
Executed when the negotiation is finalized, 
creates the protocol objects according to the 
negotiated options */		
	{
	TRAPD(ret, 
	 switch (iClientId)
		 {
	 case KPppIdPap:
		 iClientProtocol = CPppPap::NewL();
		 break;

	 case KPppIdChap:
		 switch (iClientSubId)
			 {
		 case KPppChapMd5AlgorithmNumber:
			 iClientProtocol = CPppChapMd5::NewL();
			 break;

		 case KPppMsChapAlgorithmNumber:
			 iClientProtocol = CPppMsChap::NewL();
			 break;

		 case KPppMsChap2AlgorithmNumber:
			 iClientProtocol = CPppMsChap2::NewL();
			 break;
			 }
		 break;
		 }

	 if (iClientProtocol)
		 {
		 iClientProtocol->iFlags = KPppApIsClient;
		 if (iClientId==iServerId)
			 iClientProtocol->iFlags |= KPppApIsServer;
		 iClientProtocol->InitL(iPppLcp);
		 }

	 if (iClientId!=iServerId)
		 {
		 switch (iServerId)
			 {
		 case KPppIdPap:
			 iServerProtocol = CPppPap::NewL();
			 break;

		 case KPppIdChap:
			 switch (iServerSubId)
				 {
			 case KPppChapMd5AlgorithmNumber:
				 iServerProtocol = CPppChapMd5::NewL();
				 break;

			 case KPppMsChapAlgorithmNumber:
				 iServerProtocol = CPppMsChap::NewL();
				 break;

			 case KPppMsChap2AlgorithmNumber:
				 iServerProtocol = CPppMsChap2::NewL();
				 break;
				 }
			 break;
			 }

		 if (iServerProtocol)
			 {
			 iServerProtocol->iFlags = KPppApIsServer;
			 iServerProtocol->InitL(iPppLcp);
			 }
		 }
		); // TRAPD

	if (ret!=KErrNone)
		{
		iPppLcp->PhaseAborted(ret);
		return;
		}
	}

void CPppAcp::OptFillinConfigRequestL(
	RPppOptionList& /*aRequestList*/)
/**
Called when the LCP state machine prepares the initial 
configuration request. No authentication protocol is proposed
here.
@param aRequestList Not used */	
	{
	// do not add anything to the list
	}
	
TPppOptResponse CPppAcp::ProposeChap(
	RPppOption& aOption)
/**
Sets the option argument to CHAP and the next supported algorithm. 
The order of negotiation of PPP CHAP algorithms:

MS-CHAP-V2
MS-CHAP-V1
CHAP with MD5

When all algorithms were tried the method returns EPppOptReject.
@param aOption Returns the updated option
@return EPppOptNak if aOption contains valid info 
		EPppOptReject otherwise */	
	{
	switch (iCHAPTypeOffered)
		{
	case 0:
		iCHAPTypeOffered = KPppMsChap2AlgorithmNumber;
		break;			
	case KPppMsChap2AlgorithmNumber:
		iCHAPTypeOffered = KPppMsChapAlgorithmNumber;
		break;			
	case KPppMsChapAlgorithmNumber:
		iCHAPTypeOffered = KPppChapMd5AlgorithmNumber;
		break;
	case KPppChapMd5AlgorithmNumber:			
	default:
		return EPppOptReject;
		}
		
	TUint8* p = aOption.ValuePtr();
	
	BigEndian::Put16(p, (TUint16)KPppIdChap);

	// There is a problem here due to using aOption as an input/output
	// parameter if the size of the Data field in the
	// Authentication-Protocol Configuration Option received in the
	// aOption parameter is not large enough to allow suggesting CHAP
	// (i.e. the size is of Data field is 0).  This is outside the scope
	// of the current work, so it is assumed here that RPppOption/RMBuf
	// meets the requirements.
	ASSERT(aOption.First()->Size() >= 
				KLCPConfigOptionTypeFieldSize +
				KLCPConfigOptionLengthFieldSize +
				KAuthenticationProtocolOptionFieldSize + 
				KChapAlgorithmFieldSize);

	aOption.SetValueLength(
		KAuthenticationProtocolOptionFieldSize + 
		KChapAlgorithmFieldSize);
	p[2] = iCHAPTypeOffered;
		
	return EPppOptNak;		
	}		
		
TPppOptResponse CPppAcp::ProposePap(
	RPppOption& aOption)
/**
Sets the option argument to PAP. 
@param aOption Returns the updated option
@return EPppOptNak if aOption contains valid info 
		EPppOptReject otherwise */	
	{
	aOption.SetValueLength(KAuthenticationProtocolOptionFieldSize);

	TUint8* p = aOption.ValuePtr();
	BigEndian::Put16(p, (TUint16)KPppIdPap);

	return EPppOptNak;
	}	

TPppOptResponse CPppAcp::OptCheckConfigRequest(
	RPppOption& aOption)
/**
Checks the specified authentication option. The currently supported 
authentication mode/protocol is specified in the CDMA_SIMIP_AUTH_ALGORITHM
field in the packet service table (TCommDbCdmaSimpIpAuthAlgorithm enum).
@param aOption Returns the updated option
@return EPppOptAck if the proposed authentication option is accepted
		EPppOptNak if the option is not accepted and another option is being proposed
		EPppOptReject if the option is not accepted and no other option is proposed */	
	{	
	// MobileIP, Don't negotiate PAP or CHAP if CDMA MIP.
	if (iPppLcp->QueryExternalIPConfiguration())
		{
		// Check if we must authenticate always, regardless of the mode.
		// If the SERVICE_IF_EXTERN_IP_CONFIG_ALWAYS_REJECT_AUTH field is not in CommDB, 
		// doAlwaysAuthenticate remains EFalse.
		}

	TUint8* p = aOption.ValuePtr();
	
	if (aOption.ValueLength() < 
		KAuthenticationProtocolOptionFieldSize)
		return EPppOptReject;

	// always return from every conditional branch of the following
	switch (BigEndian::Get16(p))
		{
	case KPppIdPap:
	    if (aOption.ValueLength() !=
	        KAuthenticationProtocolOptionFieldSize)
	        {      
            // malformed PAP Authentication-Protocol Option - reject
            return EPppOptReject;
            }
	    break;
	case KPppIdChap:
		if (aOption.ValueLength() != 
			KAuthenticationProtocolOptionFieldSize + 
			KChapAlgorithmFieldSize)
			{		
			// malformed CHAP Authentication-Protocol Option - reject
			return EPppOptReject;
			}
            break;
	default:
		// unrecognized authentication protocol

		break;
		}
	#ifndef SYMBIAN_EXCLUDE_CDMA
		return TPppOptResponse();
	#endif				
	}

void CPppAcp::OptApplyConfigRequest(
	RPppOption& aOption)
/**
The specified configuration option from the server config request
has been accepted, the negotiation result is stored.
@param aOption Accepted authentication option */	
	{
	iClientId = BigEndian::Get16(aOption.ValuePtr());
	TInt n = aOption.ValueLength()-2;
	if (n>0)
		iClientSubId = *(aOption.ValuePtr()+2);
	}

void CPppAcp::OptRecvConfigAck(
	RPppOption& aOption)
/**
The specified configuration option has been acked by the server, 
the negotiation result is stored.
@param aOption Acked authentication option */
	{
	iServerId = BigEndian::Get16(aOption.ValuePtr());
	TInt n = aOption.ValueLength()-2;
	if (n>0)
		iServerSubId = *(aOption.ValuePtr()+2);
	}

void CPppAcp::OptRecvConfigNak(
	RPppOption& aOption, 
	RPppOptionList& aReqList)
/**
The specified configuration option has been nacked by the server,
the option is replaced in the option list
@param aOption Nacked authentication option 
@param aReqList List to be updated */	
	{
	aReqList.ReplaceOption(aOption);
	}

void CPppAcp::OptRecvConfigReject(
	RPppOption& aOption, 
	RPppOptionList& aReqList)
/**
The specified configuration option has been rejected by the server,
the option is removed from the option list
@param aOption Rejected authentication option 
@param aReqList List to be updated */	
	{
	aReqList.RemoveOption(aOption);
	}
