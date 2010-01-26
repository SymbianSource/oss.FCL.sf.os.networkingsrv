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
// SSL3.0 and TLS1.0 Connection header file.
// Describes a secure (SSL3.0/TLS1.0) connection.
// 
//

/**
 @file
*/

#ifndef _TLSCONNECTION_H_
#define _TLSCONNECTION_H_
 
#include <securesocketinterface.h>
#include <genericsecuresocket.h>
#include <ssl.h>
#include <tlsprovinterface.h> 
#include <tlstypedef.h>
#include <comms-infras/statemachine.h>
#include "LOGFILE.H"

//Tls protocol Panics
enum TTlsPanic
{
	ETlsPanicClientHelloMsgNotSent = 0,
	ETlsPanicHandshakeMsgAlreadyExists,
	ETlsPanicChangeCipherMsgNotReceived,
	ETlsPanicServerHelloMsgNotReceived,
	ETlsPanicNullHandshakeMsg,
	ETlsPanicNullServerCertificate,
	ETlsPanicInvalidProcessState,
	ETlsPanicInvalidTlsSession,
	ETlsPanicNullTlsSession,
	ETlsPanicTlsProviderNotReady,
	ETlsPanicNoCA,
	ETlsPanicNoDataToProcess,
	ETlsPanicNoUserData,
	ETlsPanicUserDataAlreadySet,
	ETlsPanicNullPointerToHandshakeHeaderBuffer,
	ETlsPanicNullPointerToHandshakeRecordParser,
	ETlsPanicAppDataResumeButNotStarted,
	ETlsPanicHelloRequestRecWhileInAppData,
	ETlsPanicSignatureAlreadyExists,
	ETlsPanicNullStateMachineHistory,
	ETlsPanicNullStateMachine,
	ETlsPanicInvalidStateMachine,
	ETlsPanicStateMachineAlreadyExists,
	ETlsPanicStateMachineStopped,
	ETlsPanicInvalidStatus,
	ETlsPanicAlertReceived,
	ETlsPanicAlreadyActive,
};

// Declaration for global panic function for Tls protocol
GLREF_C void TlsPanic(TTlsPanic aPanic);

// Constant values
const TInt KProtocolDescMinSize = 8;	//< Minimum size of the descriptor for the Protocol name

/*
In RFC2716 (PPP EAP TLS Authentication Protocol) section 3.5, label is defined as "client EAP encryption"

In draft-josefsson-pppext-eap-tls-eap-02 (Protected EAP Protocol (PEAP)) section 2.8, label is
defined as "client PEAP encryption". However, most of radius servers use "client EAP encryption" 
as the constant keying string.

In draft-ietf-pppext-eap-ttls-04 EAP Tunneled TLS Authentication Protocol (TTLS) section 7,
label is defined as "ttls keying material".

Following max size is sufficient to all those cases.
*/
const TInt KKeyingLabelMaxSize = 100;

// TlsConnection supported protocols 
_LIT( KProtocolVerSSL30, "SSL3.0" );	//< SSL 3.0 Protocol
_LIT( KProtocolVerTLS10, "TLS1.0" );	//< TLS 1.0 Protocol


// Forward Declarations
class CRecordParser;
class CRecordComposer;
class CSendAlert;
class CHandshake;
class CSendAppData;
class CRecvAppData;

class CTlsConnection : public CActive, public MSecureSocket, public MStateMachineNotify
/**
  * A secure (SSL v3.0 or TLS vl.0) connection.
  * Implements the MSecureSocket interface used by the SECURESOCKET.DLL to talk to 
  * the protocol implementation. Note that it only implements Client-mode support
  * for the SSL v3.0 and TLS v1.0 protocols.
  * Server-mode operation is NOT supported. 
  */
{
public:
	IMPORT_C static MSecureSocket* NewL(RSocket& aSocket, const TDesC& aProtocol);
	IMPORT_C static MSecureSocket* NewL(MGenericSecureSocket& aSocket, const TDesC& aProtocol);

	IMPORT_C static void UnloadDll(TAny* /*aPtr*/);

	 ~CTlsConnection();

	// MSecureSocket interface
	virtual TInt AvailableCipherSuites(TDes8& aCiphers);
	virtual void CancelAll();
	virtual void CancelHandshake();
	virtual void CancelRecv();
	virtual void CancelSend();
	virtual const CX509Certificate* ClientCert();
	virtual TClientCertMode ClientCertMode(); 
	virtual void Close();
	virtual TInt CurrentCipherSuite(TDes8& aCipherSuite);
	virtual TDialogMode	DialogMode(); 
	virtual void FlushSessionCache();
	virtual TInt GetOpt(TUint aOptionName,TUint aOptionLevel,TDes8& aOption);
	virtual TInt GetOpt(TUint aOptionName,TUint aOptionLevel,TInt& aOption);
	virtual TInt Protocol(TDes& aProtocol);
	virtual void Recv(TDes8& aDesc, TRequestStatus & aStatus);
	virtual void RecvOneOrMore(TDes8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen);
	virtual void RenegotiateHandshake(TRequestStatus& aStatus);
	virtual void Send(const TDesC8& aDesc, TRequestStatus& aStatus);
	virtual void Send(const TDesC8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen);
	virtual const CX509Certificate* ServerCert();
	virtual TInt SetAvailableCipherSuites(const TDesC8& aCiphers);
	virtual TInt SetClientCert(const CX509Certificate& aCert);
	virtual TInt SetClientCertMode(const TClientCertMode aClientCertMode);
	virtual TInt SetDialogMode(const TDialogMode aDialogMode);
	virtual TInt SetOpt(TUint aOptionName,TUint aOptionLevel, const TDesC8& aOption=KNullDesC8());
	virtual TInt SetOpt(TUint aOptionName,TUint aOptionLevel,TInt aOption);
	virtual TInt SetProtocol(const TDesC& aProtocol);
	virtual TInt SetServerCert(const CX509Certificate& aCert);
	virtual void StartClientHandshake(TRequestStatus& aStatus);
	virtual void StartServerHandshake(TRequestStatus& aStatus);

	// MStateMachineNotify interface
	virtual TBool OnCompletion(CStateMachine* aStateMachine); 

	// Internal functions (these 2 functions are only used by this class)
	void StartRenegotiation( TRequestStatus* aStatus );
	void StartClientHandshakeStateMachine( TRequestStatus* aStatus );

   void ResetCryptoAttributes();

   //helper getter
	CTLSProvider&		TlsProvider();
	CTLSSession*&		TlsSession();
	CRecordParser&		RecordParser() const;
	CRecordComposer&	RecordComposer() const;
	CSendAppData*		SendAppData() const;
	TBool				SessionReUse() const;
   void GetServerAddrInfo( TTLSServerAddr& serverInfo );

	// Retrieve or confirm the Connection states
	TBool IsHandshaking() const;
	TBool IsReNegotiating() const;
	TBool IsInDataMode() const;
	TBool IsIdle() const;

	// Methods from CActive
	void RunL();
	void DoCancel();

protected:
	CTlsConnection(); 
	void ConstructL(RSocket& aSocket, const TDesC& aProtocol);
	void ConstructL(MGenericSecureSocket& aSocket, const TDesC& aProtocol);
	void DeleteStateMachines();
	void CancelAll(TInt aError);
	TBool SendData(const TDesC8& aDesc, TRequestStatus& aStatus);
	TBool RecvData(TDes8& aDesc, TRequestStatus& aStatus);

	TInt GetKeyingMaterial(TDes8& aKeyingMaterial);

protected:
	CTLSProvider*		iTlsProvider;		//< TLS Provider interface 
	CTLSSession*		iTlsSession;		//< TLS Session interface
	CRecordParser*		iRecordParser;		//< Record parser object
	CRecordComposer*	iRecordComposer;	//< Record composer object
	CHandshake*			iHandshake;			//< Handshake negotiation state machine
	CSendAppData*		iSendAppData;		//< Application data transmission state machine
	CRecvAppData*		iRecvAppData;		//< Application data reception state machine
	TDialogMode			iDialogMode;		//< Dialog settings

   //to keep the certificates given to client so that the client doesn't have to delete them
   //=> compatible behaviour with the old tls
	CX509Certificate* iClientCert;
	CX509Certificate* iServerCert;

	CGenericSecureSocket<RSocket>* iGenericSocket;
};


// Inline functions

/**
 * Returns a pointer to an Application data tx. state machine.
 */
inline CSendAppData* CTlsConnection::SendAppData() const
{
	LOG(Log::Printf(_L("CTlsConnection::CSendAppData()"));)
	return iSendAppData;
}

/**
 * Returns a reference to a TlsProvider object.
 */ 
inline CTLSProvider& CTlsConnection::TlsProvider()
{
	LOG(Log::Printf(_L("CTlsConnection::TlsProvider()"));)
	return *iTlsProvider;
}

/**
 * Returns a reference to a TlsSession object.
 */ 
inline CTLSSession*& CTlsConnection::TlsSession()
{
	LOG(Log::Printf(_L("CTlsConnection::TlsSession()"));)
	return iTlsSession;
}

/**
 * Returns a reference to a Record parser object.
 */
inline CRecordParser& CTlsConnection::RecordParser() const
{
	LOG(Log::Printf(_L("CTlsConnection::RecordParser()"));)
	return *iRecordParser;
}

/**
 * Returns a reference to a Record composer object.
 */
inline CRecordComposer& CTlsConnection::RecordComposer() const
{
	LOG(Log::Printf(_L("CTlsConnection::RecordComposer()"));)
	return *iRecordComposer;
}

/**
 * Returns a Boolean value that indicates whether a connection is
 * handshaking.
 */
inline TBool CTlsConnection::IsHandshaking() const
{
	LOG(Log::Printf(_L("CTlsConnection::IsHandshaking()"));)
	return iHandshake && !IsInDataMode();
}

/**
 * Returns a Boolean value that indicates whether a connection is
 * renegotiating a handshake.
 */
inline TBool CTlsConnection::IsReNegotiating() const
{
	LOG(Log::Printf(_L("CTlsConnection::IsReNegotiating()"));)
	return iHandshake && IsInDataMode();
}

/**
 * Returns a Boolean value that indicates whether a connection is
 * in data mode (transmission or reception).
 */
inline TBool CTlsConnection::IsInDataMode() const
{
	LOG(Log::Printf(_L("CTlsConnection::IsInDataMode()"));)
	return iSendAppData != 0; // iSendAppData and iRecvAppData always go together;
}

inline TBool CTlsConnection::SessionReUse() const
{
	LOG(Log::Printf(_L("CTlsConnection::SessionReUse()"));)
	return ETrue;	//used to return a value set in the FlushSessionChance 
					//don't think this is a good idea PS
}

#endif

