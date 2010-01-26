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
*
*/



/**
 @file DialogProcessor.H
 @internalTechnology
*/

#ifndef __ND_DLGPC_H__
#define __ND_DLGPC_H__

#include <agentdialog.h>
#include <comms-infras/connectionsettings.h>

class CDialogDestroyPCTNotification;

class MDialogProcessorObserver
/**
MDialogProcessorObserver

Mixin observer class to be derived from by classes wishing to use CDialogProcessor.
When the relevant asynchronous function call completes the appropriate function will
be called on the observer.  Only completion functions used need to be implemented.
@internalTechnology
*/
	{
public:
	IMPORT_C virtual void MDPOSelectComplete(TInt aError, const TConnectionSettings& aSettings);
	IMPORT_C virtual void MDPOSelectModemAndLocationComplete(TInt aError, const TConnectionSettings& aSettings);
	IMPORT_C virtual void MDPOWarnComplete(TInt aError, TBool aResponse);
	IMPORT_C virtual void MDPOLoginComplete(TInt aError);
	IMPORT_C virtual void MDPOAuthenticateComplete(TInt aError);
	IMPORT_C virtual void MDPOReconnectComplete(TInt aError);
	IMPORT_C virtual void MDPOReadPctComplete(TInt aError);
	IMPORT_C virtual void MDPODestroyPctComplete(TInt aError);
	IMPORT_C virtual void MDPOQoSWarningComplete(TInt aError, TBool aResponse);
	};

/**
CDialogDestroyPCTNotification

Interface between CDialogProcessor and dialog server for destroy PCT notification only.  
This needs to be a sepearate class to CDailogProcesor because it will be active
the whole time the PCT is open. One per CDialogProcessor object and it uses the 
CScriptEngD object to return destroy PCt notification to the scrpt engine. Has an 
RGenConAgentDialogServer - class with fixed API to be implemented by dialog server.

This class is private to CDialogProcessor implementation and so its definition
has been moved to dialogprocessor.cpp
*/
class CDialogDestroyPCTNotification;

class CDialogProcessor : public CActive
/**
CDialogProcessor

Interface between the agent and the dialog server.  One per CAgentController object and it itself
uses the CAgentController object to return results of dialogs to state machine. Also uses a
CScriptEngD to return results of PCT read to the script engine.  Has a 
CDialogDestroyPCTNotification to request that a notification is sent when the PCT is 
destroyed. Has an RGenConAgentDialogServer - class with fixed API to be implemented by 
dialog server.
@internalTechnology
*/
	{
	friend class CDialogDestroyPCTNotification;
public:
	IMPORT_C static CDialogProcessor* NewL(TInt aPriority = CActive::EPriorityStandard);
	IMPORT_C ~CDialogProcessor();

	/** Call from the agent controller (which require a response) */
	IMPORT_C void SelectConnection(MDialogProcessorObserver& aObserver, const TConnectionPrefs& aPrefs);
	IMPORT_C void SelectConnection(MDialogProcessorObserver& aObserver, const TConnectionPrefs& aPrefs, TInt aLastError);
	IMPORT_C void SelectModemAndLocation(MDialogProcessorObserver& aObserver);
	IMPORT_C void Authenticate(MDialogProcessorObserver& aObserver, TDes& aUsername, TDes& aPassword, TBool aIsReconnect);
	IMPORT_C void Reconnect(MDialogProcessorObserver& aObserver);

	/** Call from the states (which require a response) */
	IMPORT_C void WarnNewConnection(MDialogProcessorObserver& aObserver, const TConnectionPrefs& aPrefs, const TDesC* aNewIapName, const TIspConnectionNames* aNewConnectionNames, TInt aLastError);
	IMPORT_C void Login(MDialogProcessorObserver& aObserver, TDes& aUsername, TDes& aPassword, TBool aIsReconnect);
	IMPORT_C void DestroyPctNotification(MDialogProcessorObserver& aObserver);
	IMPORT_C void ReadPct(MDialogProcessorObserver& aObserver, TDes& aData);
	IMPORT_C void QoSWarning(MDialogProcessorObserver& aObserver);
//
// Calls from the states (which do not require a response)
//
	IMPORT_C TInt OpenPct();
	IMPORT_C TInt WritePct(const TDesC& aData);
	IMPORT_C void ClosePct();

	/** Should always call this rather than Cancel() if using PCT */
	IMPORT_C void CancelEverything(); 
private:
	enum TDPState
		{
		ENoState =0,
		ESelectConnection,
		EWarnNewConnection,
		ESelectModemAndLocation,
		ELogin,
		EAuthentication,
		EReconnect,
		EReadPct,
		EQoSWarning
		};
private:
	CDialogProcessor(TInt aPriority);
	void ConstructL();
	void CompleteDestroyPctNotification(TInt aStatus);
	void SetActive(MDialogProcessorObserver& aObserver, TDPState aState);

	/** From CActive */
	virtual void DoCancel();
	virtual void RunL();
private:
	MDialogProcessorObserver* iCurrentObserver;
	MDialogProcessorObserver* iPctDestructionObserver;
	CDialogDestroyPCTNotification* iDestroyPctNotification;
	RGenConAgentDialogServer iDlgServ;
	TDPState iState;
	TConnectionSettings iSettings;
	TBool iReconResponse;
	TBool iWarnNewConnectResponse;
	TBool iQoSWarningResponse;
	};

#endif

