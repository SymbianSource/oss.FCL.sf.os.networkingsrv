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

#if !defined(__ET_SSTD_H__)
#define __ET_SSTD_H__

#if !defined(__ETEL_H__)
#include "dummyEtel.h"
#endif

#if !defined(__ET_PHONE_H__)
#include "ET_PHONE.H"
#endif

#if !defined(__ET_CLSVR_H__)
#include "Et_clsvr.h"
#endif

#if !defined(__ETELEXT_H__)
#include "ETELEXT.H"
#endif

#if !defined(__SLOGGER_H__)
#include "ET_SLOG.H"
#endif

#include "etelpckt.h"
#include "etelQoS.h"

/**
@internalComponent
*/
_LIT(KETelSemaphoreName,"EtelSrvStartupSem");
_LIT(KEtelDLLName,"DummyETEL.DLL");

class CDestroyDummySubSession : public CAsyncOneShot
/**
Asychronously Destroy Dummy Session
@internalComponent
*/
	{
public:
	static CDestroyDummySubSession* NewL(CTelServer* aTelServer,CTelObject* aTelObject);
	~CDestroyDummySubSession();

protected:
	CDestroyDummySubSession(CTelServer* aTelServer,CTelObject* aTelObject);
	virtual void RunL();
public:
	CTelObject* iTelObject;
	CTelServer* iTelServer;
	};

class CTelSchedulerStop : public CAsyncOneShot
/**
Asychronously Stop the Scheduler
@internalComponent
*/
	{
public:
	static CTelSchedulerStop* NewL();
	~CTelSchedulerStop();
protected:
	CTelSchedulerStop();
	virtual void RunL();
	};

class CPhoneManager;
class CContextConfigChangeReq;
class CContextStatusChangeReq;
class CQoSConfigChangeReq;
class CTelServer : public CServer2
/**
ETel Server Definition
@internalComponent
*/
	{
public:
	enum {EPriority=1000};
public:
	static CTelServer* New();
	void ConstructL();
	~CTelServer();
	virtual CSession2* NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const;

	void Dec();
	void Inc();
	TInt Count() const;
	TBool IsPriorityClient(const CTelSession* aSession) const;
	TInt SetPriorityClient(CTelSession* aSession);
	TInt RemovePriorityClient(CTelSession* aSession);
private:
	CTelServer(TInt aPriority);
	TInt iSessionCount;
	CTelSchedulerStop* iSch;
	//CPhoneManager* iPhoneManager;
	CTelSession* iPriorityClientSession;
public: // JGG added
	CContextConfigChangeReq* iContextConfigChangeReq;
	CContextStatusChangeReq* iContextStatusChangeReq;
	CQoSConfigChangeReq* iQoSConfigChangeReq;
	TInt iTestNumber;
	RTelServer::TMobilePhoneNetworkMode iMode;
	RPacketContext::TContextConfigGPRS iGPRSContextConfig;
	RPacketQoS::TQoSGPRSRequested iGPRSRequestedQoSConfig;
	};

/**
@internalComponent
*/
enum TTelObjectOpenSource
	{
	ETelObjectOpenSourceSession,
	ETelObjectOpenSourceSubSession,
	ETelObjectOpenByNameSourceSession,
	ETelObjectOpenByNameSourceSubSession
	};

class CContextConfigChangeReq;
class CTelSession : public CSession2
/**
ETel Session Definition
@internalComponent
*/
	{
public:
	CTelSession();
	virtual ~CTelSession();
	virtual void ServiceL(const RMessage2& aMessage);
	CTelObject* CObjectFromHandle(TUint aHandle) const;
	CTelServer* TelServer() const;
	RHeap* PriorityClientHeap(TInt aReq) const;
	TBool IsExpectingExtendedError() const;
	TBool IsPriorityClientReq(TInt aReq) const;
	TBool IsUnicodeReq(TInt aReq) const;
private:
	inline CTelServer* Server() const;
	void CreateL();
	TInt GetModuleName(TDes& aName) const;
	void LoadPhoneModule();
	void ClosePhoneModule();
	void EnumeratePhonesL();
	void IsSupportedByTsy();
	void NewTelObject(TTelObjectOpenSource aSource);
	void PhoneInfo(TInt aIndex);
	void GetTsyNameByPhone(const TInt aIndexOfPhone);
	void SetPriorityClient();
	void UnsetPriorityClient();
	void GetTsyVersionNumber() const;
	TInt Write(TInt aIndex,const TDesC8& aDes,TInt aOffset=0) const;
	TInt Read(TInt aIndex,TDes8& aDes,TInt aOffset=0) const;
	TInt Write(TInt aIndex,const TDesC16& aDes,TInt aOffset=0) const;
	TInt Read(TInt aIndex,TDes16& aDes,TInt aOffset=0) const;
	TPtrC StripOutNextName(TPtrC& aRemainingName,const TFullName& aFullName);
	void CheckAndAppendNewName(TDes& aName);
	void GenerateName(TDes& aName);
	void SetExtendedErrorGranularity();
private:
	// JGG new functions for testing
	void OpenFromSession();
	void OpenFromSubSession();
	TInt NumberOfPhones();
	void SimulateSubSessionFunctionsL(const RMessage2 &aMessage);
	void SimulatePhoneFunctions(const RMessage2 &aMessage);
	void SimulatePacketServiceFunctionsL(const RMessage2 &aMessage);
	void SimulatePacketContextFunctionsL(const RMessage2 &aMessage);
	void SimulatePacketQoSFunctionsL(const RMessage2 &aMessage);
	void SimulateCallFunctions(const RMessage2 &aMessage);
	inline const RMessage2& Message() const; 
private: // data
	//CPhoneManager* iPhoneManager;
	CObjectIx* iTsyModulesIx;
	CObjectIx* iObjectIx;
	TUint32 iNameIndex;
	RHeap* iPriorityClientHeap;
	RTelServer::TErrorGranularity iErrorGranularity;
	// JGG extra member data for test
	TInt iTsyLoadCount;
	TInt iOpenSubSessionCount;
	RPacketContext::TContextStatus iContextStatus;
	TInt iPacketContextGetDataVolumeTransferredCallCount;
	TInt iPacketServiceGetNtwkRegStatusCallCount;
	TInt iPacketContextGetStatusCallCount;
	TInt iPacketContextNotifyStatusChangeCallCount;
	TInt iPacketContextLoanCommPortCallCount;
	TInt iLoanDataPortCallCount;
	TInt iSessionNumber;
	CPeriodic* iCallbackTimer;
	RMessage2 iMessage;
	};

class CTelScheduler : public CActiveScheduler
/**
@internalComponent
*/
	{
public:
	static CTelScheduler* New();
	void Error(TInt anError) const;
	};
/*
class CDuplicatePhoneInfo : public CBase
	{
public:
	static CDuplicatePhoneInfo* NewLC(const TDesC* aTsyName,const TDesC* aOriginalName,const TDesC* aNewName);
	~CDuplicatePhoneInfo();
protected:
	CDuplicatePhoneInfo();
	void ConstructL(const TDesC* aTsyName,const TDesC* aOriginalName,const TDesC* aNewName);
public:
	HBufC* iTsyName;
	HBufC* iOriginalName;	
	HBufC* iNewName;
	};
*/
//
// Phone Manager Definition
//
/*
class CPhoneManager : public CBase
	{
public:
	static CPhoneManager* NewL();
	CPhoneManager();
	void ConstructL();
	~CPhoneManager();

	CPhoneFactoryBase* LoadPhoneModuleL(const TDesC& aFileName);
	CPhoneFactoryBase* OpenPhoneFactoryFromTsyL(const TDesC& aName);
	CPhoneBase* OpenPhoneFromFactoryL(CPhoneFactoryBase* aParent,const TDesC& aName);
	CTelObject* OpenSubSessionObjectL(CTelObject* aParent, TDes& aNewName); */
//	CTelObject* OpenSubSessionObjectByNameL(CTelObject* aParent,const TDesC& aName/*,TTelObjectOpenSource aSource*/);
/*	TInt EnumeratePhones() const;
	TInt GetPhoneInfo(TUint aIndex,RTelServer::TPhoneInfo& aInfo) const;
	TInt GetTsyName(const TInt aIndexOfPhone,TDes& aTsyName) const;
	CPhoneFactoryBase* PhoneFactoryObjectFromPhoneName(const TDesC& aPhoneName) const;
	CPhoneFactoryBase* PhoneFactoryObjectFromPhoneIndex(TUint aIndex) const;
//	TBool DuplicatePhoneName() const;
	TBool DuplicatePhoneName(CPhoneFactoryBase* aPhoneFac) const;
	TInt RenameDuplicatePhoneName() const;

	void StoreDuplicateNameL(const TDesC& aTsyName,const TDesC& aOriginalName,const TDesC& aNewName) const;
	TInt ConvertPhoneNameToOriginal(TDes& aName) const;
	TInt ConvertPhoneNameFromOriginal(const TDesC& aTsyName, TDes& aPhoneName) const;
	void RemoveDuplicatePhoneInfo(const TDesC& aTsyName);
private:
	CObjectConIx* iContainerIx;
	CObjectCon* iTsyModulesCon;
	CObjectCon* iObjectCon;
	CArrayPtrFlat<CDuplicatePhoneInfo>* iDuplicatePhones;
	};
*/
class CContextConfigChangeReq : public CBase
/**
@internalComponent
*/
	{
public:
	enum TChangeType
		{
		EDrop,
		ERestore
		};
public:
	static CContextConfigChangeReq* NewL(const RMessage2& aMessage, CTelServer* aServer);
	~CContextConfigChangeReq();
	void CompleteL(TChangeType aChangeType);
	void Cancel();
private:
	CContextConfigChangeReq(const RMessage2& aMessage, CTelServer* aServer);
	void ConstructL();
private:
	RMessage2 iOutstandingReq;
	CTelServer* iTelServer;
	};

class CContextStatusChangeReq : public CBase
/**
@internalComponent
*/
	{
public:
	static CContextStatusChangeReq* NewL(const RMessage2& aMessage, CTelServer* aServer, TInt aCallCount);
	~CContextStatusChangeReq();
	void CompleteL();
	void Cancel();
private:
	CContextStatusChangeReq(const RMessage2& aMessage, CTelServer* aServer, TInt aCallCount);
	void ConstructL();
	static TInt TimerCallback(TAny* aPtr);
private:
	RMessage2 iOutstandingReq;
	CPeriodic* iCallbackTimer;
	CTelServer* iTelServer;
	TInt iCallCount;
	};

class CQoSConfigChangeReq : public CBase
/**
@internalComponent
*/
	{
public:
	enum TChangeType
		{
		EDrop,
		ERestore
		};
public:
	static CQoSConfigChangeReq* NewL(const RMessage2& aMessage, CTelServer* aServer);
	~CQoSConfigChangeReq();
	void CompleteL(TChangeType aChangeType);
	void Cancel();
private:
	CQoSConfigChangeReq(const RMessage2& aMessage, CTelServer* aServer);
	void ConstructL();
private:
	RMessage2 iOutstandingReq;
	CTelServer* iTelServer;
	};

/**
@internalComponent
*/
GLREF_C TInt EtelServerThread(TAny*);
GLREF_C void PanicClient(TEtelPanic aPanic,const RMessage2& aMessage);
GLREF_C void Fault(TEtelFault aFault);

#endif
