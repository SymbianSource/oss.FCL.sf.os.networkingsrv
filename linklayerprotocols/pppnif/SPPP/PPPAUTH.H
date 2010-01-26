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

#if !defined(__PPPAUTH_H__)
#define __PPPAUTH_H__

#include "PPPBASE.H"
#include "PppProg.h"

/**
   The PPP authentication algorithm number for CHAP with MD5 - RFC
   1994.
   @internalComponent
*/
const TUint KPppChapMd5AlgorithmNumber = 0x05;

/**
   The PPP authentication algorithm number for MS-CHAP (version 1) -
   RFC 2433.
   @internalComponent
*/
const TUint KPppMsChapAlgorithmNumber = 0x80;

/**
   The PPP authentication algorithm number for MS-CHAP-V2 - RFC 2759.
   @internalComponent
*/
const TUint KPppMsChap2AlgorithmNumber = 0x81;


const TUint KPppApIsClient = 0x01;
const TUint KPppApIsServer = 0x02;
const TUint KPppApAuthReqDone = 0x04;


NONSHARABLE_CLASS(CPppAuthentication) : public CBase, public MPppRecvr
/**
@internalTechnology
Base class for all PPP authentication protocols (PAP, CHAP, etc) 
*/
	{
	friend class CPppAcp;

  public:
/**
   Destructor.
*/
	virtual ~CPppAuthentication();

/**
   Second phase initializer.
   @param aLcp [in] A pointer to the instance of CPppLcp class that
   will use this object for authentication.
   @leave Standard Symbian OS error codes. e.g. KErrNoMemory.
*/
	virtual void InitL(CPppLcp* aPppLcp);

/**
   @copydoc MPppRecvr::FrameError()
   @see MPppRecvr::FrameError()
*/
	virtual void FrameError();

/**
   @copydoc MPppRecvr::KillProtocol()
   @see MPppRecvr::KillProtocol()
*/
	virtual void KillProtocol();

	void AuthenticateRequest();
	void CallAuthenticateComplete(TInt aRes);

  protected:
/**
   Notifies that NifMan has completed the user authentication request.
   If the authentication request has been successful, the username and
   the password have been returned to the NIF.
   @param aStatus [in] Status code of the authentication attempt.
*/
	virtual void AuthenticateComplete(TInt aStatus)=0;

	virtual TPppPhase PppPhase() const;
	
/**
   Returns the PPP protocol number for this authentication protocol.
   @return The PPP protocol number for this authentication protocol.
*/
	virtual TUint PppId() const = 0;

	void DoFail(TInt aErrorCode);
	
	void DoSucceed();

/* Tests if NifMan has completed the user authentication request.  If
   the authentication request has been successful, the username and
   the password have been returned to the NIF.
   @return ETrue if NifMan has completed the user authentication
   request, EFalse otherwise.
*/
	TBool IsAuthenticateRequestDone() const;

/* Tests if the this receiver is inactive and the data frames being
   received will not be delivered to it.
   @return ETrue if the current the receiver is inactive, EFalse is
   the current receiver is active.
*/
	TBool IsInactive() const;

  protected:

	TUint iFlags;

  private:
/** Flag that indicates if this instance is inactive, i.e. it will
process the data link frames received. */
	TBool	iDead;
	};


inline TPppPhase CPppAuthentication::PppPhase() const
	{
	return EPppPhaseAuthenticate;
	}

inline TBool CPppAuthentication::IsAuthenticateRequestDone() const
	{
	return iFlags & KPppApAuthReqDone;
	}

inline void CPppAuthentication::FrameError()
	{	
	}

inline void CPppAuthentication::KillProtocol()
	{
	iDead = ETrue;
	}

inline TBool CPppAuthentication::IsInactive() const
	{
	return iDead;
	}

NONSHARABLE_CLASS(CPppAcp) : public CBase, public MPppOptionHandler
/**
@internalTechnology
Option handler for the PPP authentication option. Uses the current 
authentication mode from CommDb (CHAP, PAP, CHAP with PAP fallback). 

The handler does not initiate the authentication protocol negotiation, 
it is expected that the server will propose an authentication protocol 
if required.
*/
	{
  public:
	CPppAcp(CPppLcp* aLcp);
	virtual ~CPppAcp();
	static CPppAcp* NewL(CPppLcp* aLcp);
	
  protected:
	// MPppOptionHandler
	virtual void OptNegotiationStarted();
	virtual void OptNegotiationAborted();
	virtual void OptNegotiationComplete();
	virtual void OptFillinConfigRequestL(RPppOptionList& aRequestList);
	virtual TPppOptResponse OptCheckConfigRequest(RPppOption& aOption);
	virtual void OptApplyConfigRequest(RPppOption& aOption);
	virtual void OptRecvConfigAck(RPppOption& aOption);
	virtual void OptRecvConfigNak(RPppOption& aOption, RPppOptionList& aReqList);
	virtual void OptRecvConfigReject(RPppOption& aOption, RPppOptionList& aReqList);

	void DeleteProtocols();
	TPppOptResponse ProposeChap(RPppOption& aOption);
	TPppOptResponse ProposePap(RPppOption& aOption);
		
  private:
	TUint iClientId;
	TUint iServerId;
	TUint iClientSubId;
	TUint iServerSubId;
	TUint8 iCHAPTypeOffered;
	TUint8 iProtocolOffered;

	CPppAuthentication* iClientProtocol;
	CPppAuthentication* iServerProtocol;
	CPppLcp* iPppLcp;
	};

#endif
