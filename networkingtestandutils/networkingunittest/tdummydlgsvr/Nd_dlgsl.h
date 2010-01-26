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

#ifndef __ND_DLGSL_H
#define	__ND_DLGSL_H

/**
@internalComponent
*/

/*
#include <e32cons.h>
#include <commdb.h>
#include <agentdialog.h>
#include <networking/nd_std.h>

const TInt KGenericBufferSize=80;
const TInt KDefaultPriority = 0;

class CNetDialDialogSession;

class CDialogBase : public CActive
	{
public:
	CDialogBase(CNetDialDialogSession* aSession, TInt aPriority);
	virtual ~CDialogBase();
protected:
	virtual void ConstructL();
protected:
	CNetDialDialogSession* iSession;
	RNotifier* iNotifier;
	CAsyncCallBack* iCallBack;
	TBuf<KGenericBufferSize> iTitleBuffer;
	TBuf<KGenericBufferSize> iPromptBuffer;
	TInt iNotifyResult;
	TBool iCancelFlag;
	};

class CModemAndLocationSelection : public CDialogBase
	{
public:
	static CModemAndLocationSelection* NewL(CCommsDatabase* aDb,CNetDialDialogSession* aSession, TInt aPriority=KDefaultPriority);
	virtual ~CModemAndLocationSelection();
	void GetModemAndLocationL(TUint32& aModemId,TUint32& aLocationId,TCallBack aCallBack);
	TInt Status();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	// CDialogBase
	virtual void ConstructL();
	//
	CModemAndLocationSelection(CCommsDatabase* aDb,CNetDialDialogSession* aSession, TInt aPriority);
	void StartModemSelectL();
	void StartLocationSelectL();
	void DisplayNoRecordsL();
	void SelectL(const TDesC& aTable, TInt aError);
	void ReselectL();
private:
	enum {
		EEnterModem,
		EEnterLocation,
		ENoRecordFound
		} iState;

	CCommsDatabase* iDb;
	CCommsDbTableView* iTable;
	
	TInt iResultStatus;
	TUint32* iModemId;
	TUint32* iLocationId;
	};

class CIAPSelection : public CDialogBase
	{
public:
	static CIAPSelection* NewL(CCommsDatabase* aDb,CNetDialDialogSession* aSession, TInt aPriority=KDefaultPriority);
	~CIAPSelection();
	void GetIAPL(TUint32& aIAPId, const TConnectionPrefs& aPrefs, TInt aError, TCallBack aCallBack);
	TInt Status();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	// CDialogBase
	virtual void ConstructL();
	//
	CIAPSelection(CCommsDatabase* aDb,CNetDialDialogSession* aSession, TInt aPriority);
	void SelectL(TInt aError);
	void ReselectL();
private:
	CCommsDatabase* iDb;
	CCommsDbTableView* iTable;
	TBool iRecordsPresent;
	TInt iResultStatus;
	TUint32* iIAPId;
	};

class CIAPWarning : public CDialogBase
	{
public:
	static CIAPWarning* NewL(CNetDialDialogSession* aSession, TInt aPriority = KDefaultPriority);
	~CIAPWarning();
	void WarnIAPL(const TConnectionPrefs& aPrefs, TInt aLastError, const TDesC& aNewIapName, TBool& aResponse, TCallBack aCallBack);
	TInt Status();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	// CDialogBase
	virtual void ConstructL();
	//
	CIAPWarning(CNetDialDialogSession* aSession, TInt aPriority);
private:
	TInt iResultStatus;
	TBool* iResponse;
	};

class CLogin : public CDialogBase
	{
public:
	static CLogin* NewL(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
	~CLogin();
	void GetUserPass(TDes& aUsername, TDes& aPassword,TCallBack aCallBack);
	TInt Status();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	//
	CLogin(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
	void StartGetUsername();
	void StartGetPassword();	
private:
	enum {
		EEnterName,
		EEnterPass
		} iState;
	
	TInt iResultStatus;
	TDes* iUsername;
	TDes* iPassword;
	};

class CAuthenticate : public CDialogBase
	{
public:
	static CAuthenticate* NewL(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
	~CAuthenticate();
	void GetUserPassL(TDes& aUsername, TDes& aPassword,TCallBack aCallBack);
	TInt Status();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	//
	CAuthenticate(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
	void StartGetUsername();
	void StartGetPassword();
private:
	enum {
		EEnterName,
		EEnterPass
		} iState;

	TInt iResultStatus;
	TDes* iUsername;
	TDes* iPassword;
	};

class CReconnect : public CDialogBase
	{
public:
	static CReconnect* NewL(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
	~CReconnect();
	void ReconnectL(TCallBack aCallBack);
	TBool Status();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	//
	CReconnect(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
private:
	TBool iResultStatus;
	};

class CPctTimer;
class CPct : public CDialogBase
	{
public:
	static CPct* NewL(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
	~CPct();
	void WritePct(TDes& aBuffer);
	void ReadPctL(TDes& aBuffer,TCallBack& aCallBack);
	void ClosePct();
	void DestroyPctNotificationL(TCallBack& aCallBack);
	void PctTimerComplete(TInt aStatus);
	void CancelEverything();
	TInt Status();
	void DoNotificationCancel();
	void DoReadCancel();
	void DoTimerCancel();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	// CDialogBase
	virtual void ConstructL();
	//
	CPct(CNetDialDialogSession* aSession, TInt aPriority =KDefaultPriority);
	void StartRead();
	void DoWrite();
private:
	enum {
		ENone,
		EWrite,
		ERead
		} iState;

	TInt iResultStatus;
	TBuf<KRxBufferSize> iWriteBuffer;
	TInt iStartBuffer;
	TDes* iReadBuffer;
	CPctTimer* iTimer;
	CAsyncCallBack iDestroyCallBack;
	TBool iDestroyCallBackOutstanding;
	};

//
// CPctTimer 
//

class CPctTimer : public CTimer
	{
public:
	static CPctTimer* NewL(CPct* aNotifier);
	~CPctTimer();
	void Start();
private:
	CPctTimer(CPct* aNotifier);
	// CTimer
	virtual void RunL();
private:	
	CPct* iNotifier;
	};

class CQoSWarning : public CDialogBase
	{
public:
	static CQoSWarning* NewL(CNetDialDialogSession* aSession, TInt aPriority = KDefaultPriority);
	~CQoSWarning();
	void WarnQoSL(TBool& aResponse, TCallBack aCallBack);
	TInt Status();
	// CActive
	virtual void DoCancel();
private:
	virtual void RunL();
	// CDialogBase
	virtual void ConstructL();
	//
	CQoSWarning(CNetDialDialogSession* aSession, TInt aPriority);
private:
	TInt iResultStatus;
	TBool* iResponse;
	};
*/
#endif
