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
 @file NIFUTL.H
 @internalTechnology
*/

#if !defined(__NIFUTL_H__)
#define __NIFUTL_H__

#include <networking/bca.h>
#ifndef __C32COMM_H__
#include <c32comm.h>
#endif

class MTimer
/**
Utility classes
@internalTechnology
*/
	{
public:
#if !defined(__EABI__) && !defined(__X86GCC__)
	IMPORT_C MTimer();
#endif
	IMPORT_C void TimerDelete();
	IMPORT_C void TimerConstructL(TInt aPriority);
	IMPORT_C void TimerAt(const TTime& aTime);
	IMPORT_C void TimerAfter(TTimeIntervalMicroSeconds32 aInterval);
	IMPORT_C void TimerLock(TTimerLockSpec aLock);
	IMPORT_C void TimerCancel();

	virtual void TimerComplete(TInt aStatus)=0;

protected:
	CTimer* iTimer;
	};

class MBcaControlObserver
	{
public:
	virtual const TDesC& BcaStack() = 0;
	virtual const TDesC& BcaName() = 0;
	virtual void Stop(TInt aError) = 0;
	virtual const TDesC& Port() = 0;
	virtual TInt IapId() = 0;
	virtual void InitializeComplete() = 0;
	virtual void ShutdownComplete(TInt aError) = 0;
	virtual void SetBca(BasebandChannelAdaptation::MBca* aBca) = 0;
	};

class CCommReaderV2;
class CCommWriterV2;
class CScriptBcaControl;

class MCommV2 : public MBcaControlObserver
/**
@internalTechnology
*/
	{

friend class CCommReaderV2;
friend class CCommWriterV2;

public:
	IMPORT_C void CommDelete();
	IMPORT_C void CommConstructL(TInt aReadPriority, TInt aWritePriority);
	IMPORT_C TInt CommOpen(const TDesC& aChannelId);
	IMPORT_C void CommClose();
	IMPORT_C void CommCancel();
	IMPORT_C void CommWrite(const TDesC8& aDes);
	IMPORT_C void CommWriteReady();
	IMPORT_C void CommWriteCancel();
	IMPORT_C TBool CommIsWriting() const;
	IMPORT_C void CommRead(TDes8& aDes);
	IMPORT_C void CommReadOneOrMore(TDes8& aDes);
	IMPORT_C void CommReadReady();
	IMPORT_C void CommReadCancel();
	IMPORT_C TBool CommIsReading() const;

	virtual void CommReadComplete(TInt aStatus)=0;
	virtual void CommWriteComplete(TInt aStatus)=0;
	
	//MBcaControlObserver
	virtual const TDesC& BcaStack() = 0;
	IMPORT_C virtual const TDesC& BcaName();
	virtual void Stop(TInt aError) = 0;
	virtual TInt IapId() = 0;
	virtual void InitializeComplete() = 0;
	virtual void ShutdownComplete(TInt aError) = 0;
	IMPORT_C virtual void SetBca(BasebandChannelAdaptation::MBca* aBca);
	IMPORT_C virtual const TDesC& Port();

protected:
#if !defined(__EABI__) && !defined(__X86GCC__)
	IMPORT_C MCommV2();
#endif

protected:
	CScriptBcaControl* iBcaControl;
	BasebandChannelAdaptation::MBca* iBca;
	TName iChannelId;
	CCommReaderV2* iCommReader;
	CCommWriterV2* iCommWriter;
	};

//The classes below are derecated and should be replaced with MCommV2

class CCommReader;
class CCommWriter;
class CCommLinkStatus;
class MComm
/**
@internalTechnology
*/
	{

friend class CCommReader;
friend class CCommWriter;
friend class CCommLinkStatus;

public:
#if !defined(__EABI__) && !defined(__X86GCC__)
	IMPORT_C MComm();
#endif
	IMPORT_C void CommDelete();
	IMPORT_C void CommConstructL(TInt aReadPriority, TInt aWritePriority);
	IMPORT_C TInt CommOpen(const TDesC& aName, TCommAccess aAccess);
	IMPORT_C TInt CommOpen(const TDesC& aDll, const TDesC& aName, TCommAccess aAccess);
	IMPORT_C TInt CommOpen(const TDesC& aDll, const TDesC& aName, TCommAccess aAccess, TCommRole aRole);
	IMPORT_C void CommClose();
	IMPORT_C void CommCancel();
	IMPORT_C void CommWrite(const TDesC8& aDes);
	IMPORT_C void CommWriteReady();
	IMPORT_C void CommWriteCancel();
	IMPORT_C TBool CommIsWriting() const;
	IMPORT_C void CommRead(TDes8& aDes);
	IMPORT_C void CommReadOneOrMore(TDes8& aDes);
	IMPORT_C void CommReadReady();
	IMPORT_C void CommReadCancel();
	IMPORT_C TBool CommIsReading() const;

	virtual void CommReadComplete(TInt aStatus)=0;
	virtual void CommWriteComplete(TInt aStatus)=0;

protected:
	RCommServ iCommServer;
	RComm iCommPort;
	CCommReader* iCommReader;
	CCommWriter* iCommWriter;
	CCommLinkStatus* iCommLinkStatus;
	};

#endif
