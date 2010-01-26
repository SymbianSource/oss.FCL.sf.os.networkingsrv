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
// Comms reader and writer mixin
// 
//

/**
 @file
*/


#include <nifutl.h>
#include "NI_STD.H"
#include "Ni_Log.h"
#include "bcacontrol.h"

NONSHARABLE_CLASS(CCommReaderV2) : public CActive
/**
@internalComponent
*/
	{
friend class MCommV2;
public:
	CCommReaderV2(MCommV2* aComm, TInt aPriority);
	virtual ~CCommReaderV2();
	virtual void RunL();
	virtual void DoCancel();
private:
	void CompleteNow(TInt aError);
private:
	MCommV2 *iComm;
	};

NONSHARABLE_CLASS(CCommWriterV2) : public CActive
/**
@internalComponent
*/
	{
friend class MCommV2;
public:
	CCommWriterV2(MCommV2* aComm, TInt aPriority);
	virtual ~CCommWriterV2();
	virtual void RunL();
	virtual void DoCancel();
private:
	void CompleteNow(TInt aError);
private:
	MCommV2 *iComm;
	};

#if !defined(__EABI__) && !defined(__X86GCC__)
EXPORT_C MCommV2::MCommV2()
/**
Constructor
*/
	{
	}
#endif

EXPORT_C void MCommV2::CommDelete()
	{
	LOG( NifmanLog::Printf(_L("MCommV2 %08x:\tCommDelete()"), this); )
	delete iCommReader;
	iCommReader = NULL;
	delete iCommWriter;
	iCommWriter = NULL;
	delete iBcaControl;
	iBcaControl = NULL;
	}

	
EXPORT_C TInt MCommV2::CommOpen(const TDesC& aChannelId)
	{
	iChannelId = aChannelId;
	LOG( NifmanLog::Printf(_L("MCommV2 %08x:\tCommOpen('%S')"), this, &aChannelId); )
	TRAPD(err, iBcaControl->StartLoadL(););
	LOG(
		if (err != KErrNone)
			{
			NifmanLog::Printf(_L("MCommV2 %08x:\tCommOpen() returned error %d"), this, err);
			}
	)
	return err;
	}

EXPORT_C void MCommV2::CommClose()
	{
	LOG( NifmanLog::Printf(_L("MCommV2 %08x:\tCommClose()"), this); )
	iCommReader->Cancel();
	iCommWriter->Cancel();
	iBcaControl->Shutdown(KErrNone);
	iChannelId.Zero();
	}

EXPORT_C void MCommV2::CommConstructL(TInt aReadPriority, TInt aWritePriority)
	{
	iCommReader = new (ELeave) CCommReaderV2(this, aReadPriority);
	iCommWriter = new (ELeave) CCommWriterV2(this, aWritePriority);
	iBcaControl = new (ELeave) CScriptBcaControl (this);
	}

EXPORT_C void MCommV2::CommWrite(const TDesC8& aDes)
	{
	__ASSERT_ALWAYS(iCommWriter!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	__ASSERT_ALWAYS(iBca!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iBca->Write(iCommWriter->iStatus, aDes);
	iCommWriter->SetActive();
	}

EXPORT_C void MCommV2::CommWriteReady()
	{
	__ASSERT_ALWAYS(iCommWriter!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	__ASSERT_ALWAYS(iBca!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iBca->Write(iCommWriter->iStatus, TPtrC8(NULL, 0));
	iCommWriter->SetActive();
	}

EXPORT_C TBool MCommV2::CommIsWriting() const
	{

	return iCommWriter->IsActive();
	}

EXPORT_C void MCommV2::CommRead(TDes8& aDes)
	{
	__ASSERT_ALWAYS(iCommReader!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	__ASSERT_ALWAYS(iBca!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iBca->Read(iCommReader->iStatus, aDes);
	iCommReader->SetActive();
	}

EXPORT_C void MCommV2::CommReadOneOrMore(TDes8& aDes)
	{
	__ASSERT_ALWAYS(iCommReader!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	__ASSERT_ALWAYS(iBca!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iBca->Read(iCommReader->iStatus, aDes);
	iCommReader->SetActive();
	}

EXPORT_C void MCommV2::CommReadReady()
	{
	__ASSERT_ALWAYS(iCommReader!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iCommReader->SetActive();
	}

EXPORT_C TBool MCommV2::CommIsReading() const
	{

	return iCommReader->IsActive();
	}

EXPORT_C void MCommV2::CommCancel()
	{
	if (iCommWriter)
		iCommWriter->Cancel();
	if (iCommReader)
		iCommReader->Cancel();
	if (iBcaControl)
		{
		iBcaControl->Cancel();
		}
	}

EXPORT_C void MCommV2::CommWriteCancel()
	{
	if (iCommWriter)
		iCommWriter->Cancel();
	}

EXPORT_C void MCommV2::CommReadCancel()
	{
	if (iCommReader)
		iCommReader->Cancel();
	}

EXPORT_C const TDesC& MCommV2::BcaName() 
	{ 
	return BcaStack();
	}

EXPORT_C void MCommV2::SetBca(BasebandChannelAdaptation::MBca* aBca)
	{
	iBca = aBca;
	}
EXPORT_C const TDesC& MCommV2::Port()
	{
	return iChannelId;
	}

//

CCommWriterV2::CCommWriterV2(MCommV2* aComm, TInt aPriority)
	: CActive(aPriority), iComm(aComm)
	{
	CActiveScheduler::Add(this);
	}

CCommWriterV2::~CCommWriterV2()
	{
	Cancel();
	}

void CCommWriterV2::RunL()
	{
	iComm->CommWriteCancel();
	iComm->CommWriteComplete(iStatus.Int());
	}

void CCommWriterV2::DoCancel()
	{
	__ASSERT_ALWAYS(iComm->iBca!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iComm->iBca->CancelWrite();
	}

void CCommWriterV2::CompleteNow(TInt aError)
	{
	TRequestStatus* statusPtr=&iStatus;
	User::RequestComplete(statusPtr,aError);
	}

//

CCommReaderV2::CCommReaderV2(MCommV2* aComm, TInt aPriority)
	: CActive(aPriority), iComm(aComm)
	{
	CActiveScheduler::Add(this);
	}

CCommReaderV2::~CCommReaderV2()
	{
	Cancel();
	}

void CCommReaderV2::RunL()
	{
	iComm->CommReadCancel();
	iComm->CommReadComplete(iStatus.Int());
	}

void CCommReaderV2::DoCancel()
	{
	__ASSERT_ALWAYS(iComm->iBca!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iComm->iBca->CancelRead();
	}

void CCommReaderV2::CompleteNow(TInt aError)
	{
	TRequestStatus* statusPtr=&iStatus;
	User::RequestComplete(statusPtr,aError);
	}

