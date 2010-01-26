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
// AGENTDIALOG.H
// Generic Connection Dialog Client
// 
//

#ifndef __AGENTDIALOG_H__
#define __AGENTDIALOG_H__

//*******************************************************
//*	NOTE: THIS IS A MODIFIED VERSION OF THE DIALOG SERVER
//* API HEADER FILE FOR TEST PURPOSES ONLY.
//* THE ORIGINAL VERSION IS DEFINED IN \NETWORKING\GENCONN\INC
//*
//*******************************************************

#include <e32std.h>
#include <cdbcols.h>
#include <cdblen.h>

class TConnectionPrefs
/**
@internalTechnology
*/
	{
public:
	TUint32 iRank;
	TCommDbConnectionDirection iDirection;
	TUint32 iBearerSet;
	};

class TIspConnectionNames
/**
@internalTechnology
*/
	{
public:
	TBuf<KCommsDbSvrMaxFieldLength> iServiceName;
	TBuf<KCommsDbSvrMaxColumnNameLength> iServiceTable;
	TBuf<KCommsDbSvrMaxFieldLength> iModemName;
	TBuf<KCommsDbSvrMaxFieldLength> iLocationName;
	TBuf<KCommsDbSvrMaxFieldLength> iChargecardName;
	};

class TAuthenticationPair
/**
@internalTechnology
*/
	{
public:
	TDes* iUsername;
	TDes* iPassword;
	};

class TPctResponse
/**
@internalTechnology
*/
	{
public:
	TDes* iBuffer;
	};

class TNewIapConnectionPrefs
/**
@internalTechnology
*/
	{
public:
	TConnectionPrefs iPrefs;
	TInt iLastError;
	TBuf<KCommsDbSvrMaxFieldLength> iName;
	};


class RDialogNotifier : public RNotifier
/**
Client interface to allow engines or other low level components to communicate with the UI.
Real implementations need three asynchronous message slots instead of RNotifiers default one slot
in order to implement the PCT functionality.  This is not used at all by the test dialog server
implementation.
@internalTechnology
*/
	{
public :
	TInt Connect();
	};


class RGenConAgentDialogServer : public RSessionBase
/**
@internalTechnology
*/
	{
public:
	/**
	* Constructor.
	*/
	IMPORT_C RGenConAgentDialogServer();

	/**
	* Destructor.
	*/
	IMPORT_C ~RGenConAgentDialogServer();

	/**
	* Version of the dialog server API.
	* @return The API version.
	*/
	IMPORT_C TVersion Version() const;

	/**
	* Connect to the notifier server. Must be called before any other function (c'tor excepted).
	* @return KErrNone if connection succeeded and a standard error code otherwise.
	*/
	IMPORT_C TInt Connect();

	/**
	*Disconnect from the notifier server.
	*/
	IMPORT_C void Close();

	/**
	*Shows a dialog to enable the user to choose which modem cum location to use for data transactions
	*@param aModemId The id of of the record in the modem table to be used for data transactions
	*@param aLocationId The id of the record in the location table to be used for the modem for data transactions
	*@param aStatus Any error code, or KErrNone if no error
	*/
	IMPORT_C void ModemAndLocationSelection(TUint32& aModemId,TUint32& aLocationId,TRequestStatus& aStatus);
	
	/**
	* Shows a connection dialog when CommDb database is of IAP type.
	* @param aIAP The id of the IAP service.
	* @param aPrefs Specifies the rank and desired direction of the connection and bearer.
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void IapConnection(TUint32& aIAP, const TConnectionPrefs& aPrefs, TRequestStatus& aStatus);

	/**
	* Shows a connection dialog when CommDb database is of IAP type.
	* @param aIAP The id of the IAP service.
	* @param aPrefs Specifies the rank and desired direction of the connection and bearer.
	* @param aLastError The error with which previous connection failed.
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void IapConnection(TUint32& aIAP, const TConnectionPrefs& aPrefs, TInt aLastError, TRequestStatus& aStatus);

	/**
	* Shows a dialog warning when the CommDb database is of IAP type that the previous attempt to connect
	* failed and that a new connection is to be attempted.
	* @param aPrefs Specifies the rank and desired direction of the connection and bearer.
	* @param aLastError The error with which previous connection failed.
	* @param aNewIapName The name of the IAP to be used for next connection.
	* @param aResponse Specifies whether to proceed with the connection or stop the connection attempt.
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void WarnNewIapConnection(const TConnectionPrefs& aPrefs, TInt aLastError, const TDesC& aNewIapName, TBool& aResponse, TRequestStatus& aStatus);

	/**
	* Shows a login dialog when login information for use with a script is required ny the NetDial agent.
	* @param aUsername Username.
	* @param aPassword Password.
	* @param aIsReconnect Whether this is a reconnect attempt or not
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void Login(TDes& aUsername, TDes& aPassword, TBool aIsReconnect, TRequestStatus& aStatus);

	/**
	* Shows a authentication dialog when the NetDial agent requests PPP authentication information.
	* @param aUsername Username.
	* @param aPassword Password.
	* @param aIsReconnect Whether this is a reconnect attempt or not
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void Authenticate(TDes& aUsername, TDes& aPassword, TBool aIsReconnect, TRequestStatus& aStatus);

	/**
	* Shows a reconnect dialog when connection has been broken during data transfer.
	* @param aResponse Specifies whether to reconnect or cancel the reconnection attempt.
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void Reconnect(TBool& aResponse, TRequestStatus& aStatus);

	/**
	* Opens a Post Connect Terminal dialog.
	* @return Any error code, or KErrNone if no error.
	*/
	IMPORT_C TInt OpenPct();

	/**
	* Called by NetDial agent to write incoming data into the PCT window.
	* @param aData Incoming data.
	* @return Any error code, or KErrNone if no error.
	*/
	IMPORT_C TInt WritePct(const TDesC& aData);

	/**
	* Called by NetDial agent when the script indicates that user needs to enter some information.
	* @param aData Data entered by user.
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void ReadPct(TDes& aData, TRequestStatus& aStatus);

	/**
	* Used by the NetDial agent to request that if the user cancels the dialog, then the dialog gives
	* notification of this.
	* @param aStatus KErrNone if a cancel occurs, or error code otherwise.
	*/
	IMPORT_C void DestroyPctNotification(TRequestStatus& aStatus);

	/**
	* Closes the Post Connect Terminal dialog.
	*/
	IMPORT_C void ClosePct();

	// JGG new function
	/**
	* Shows a dialog warning that the QoS of the connection has fallen below the minimum
	* values specified in commdb. Expects a response from the dialog indicating if the
	* connection is to be terminated.
	* @param aResponse Specifies whether to terminate the connection or not i.e. ETrue means terminate connection
	* @param aStatus Any error code, or KErrNone if no error.
	*/
	IMPORT_C void QoSWarning(TBool& aResponse, TRequestStatus& aStatus);

	/**
	*Cancels the ModemAndLocation connection dialog
	*/
	IMPORT_C void CancelModemAndLocationSelection();

	/**
	* Cancels the IAP type connection dialog.
	*/
	IMPORT_C void CancelIapConnection();

	/**
	* Cancels the IAP type new connection warning dialog.
	*/
	IMPORT_C void CancelWarnNewIapConnection();

	/**
	* Cancels the login dialog.
	*/
	IMPORT_C void CancelLogin();

	/**
	* Cancels the authentication.
	*/
	IMPORT_C void CancelAuthenticate();

	/**
	* Cancels the reconnect dialog.
	*/
	IMPORT_C void CancelReconnect();

	/**
	* Cancels the Read Post Connect Terminal request.
	*/
	IMPORT_C void CancelReadPct();

	/**
	* Cancels the Destroy Post Connect Terminal Notification request.
	*/
	IMPORT_C void CancelDestroyPctNotification();

	// JGG new function
	/**
	* Cancels the QoS warning dialog
	*/
	IMPORT_C void CancelQoSWarning();
	//=====================================
	// JGG test functions
	IMPORT_C void RequestDialogAppearanceNotification(TRequestStatus& aStatus);
	IMPORT_C void SetReconnectDialogResponse(TBool aResponse);
	IMPORT_C void SetQoSWarnDialogResponse(TBool aResponse);
	IMPORT_C void CancelDialogAppearanceNotification();
	IMPORT_C TInt SetTestNumber(const TInt aTestNumber);
	//======================================
private:
	RDialogNotifier* iNotifier;
	TPckg<TUint32> iIAP;
	TPckg<TUint32> iModemId;
	TPckg<TUint32> iLocationId;
	TPckgBuf<TIspConnectionNames> iConNames;
	TPckgBuf<TConnectionPrefs> iPrefs;
	TPckg<TBool> iBool;
	TPckgBuf<TUint32> iPctBuffer;
	TPckgBuf<TPctResponse> iPctResponse;
	TPckgBuf<TAuthenticationPair> iAuthenticationPair;
	TPckgBuf<TNewIapConnectionPrefs> iNewIapPrefsBuffer;
	TPckgBuf<TUint32> iNotUsed;		// Parameters not used by plugin
	TRequestStatus iStatus;
	};

/**
Deprecated since this implementation utilises the RNotifier framework instead of
creating a server. This function must still be exported not to break BC and also 
to allow the test dialog server to be implemented in a simple way, i.e. not using
UIKON.

Attempt to start a thread for the socket server in the C32 process.
@return Any error code, or KErrNone if no error.
@internalTechnology
*/
IMPORT_C TInt StartDialogThread();

#endif
