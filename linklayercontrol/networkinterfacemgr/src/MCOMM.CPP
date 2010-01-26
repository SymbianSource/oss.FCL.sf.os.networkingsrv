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

class CCommLinkStatus;
NONSHARABLE_CLASS(CCommReader) : public CActive
/**
@internalComponent
*/
	{
friend class MComm;
friend class CCommLinkStatus;
public:
	CCommReader(MComm* aComm, TInt aPriority);
	virtual ~CCommReader();
	virtual void RunL();
	virtual void DoCancel();
private:
	void CompleteNow(TInt aError);
private:
	MComm *iComm;
	};

NONSHARABLE_CLASS(CCommWriter) : public CActive
/**
@internalComponent
*/
	{
friend class MComm;
friend class CCommLinkStatus;
public:
	CCommWriter(MComm* aComm, TInt aPriority);
	virtual ~CCommWriter();
	virtual void RunL();
	virtual void DoCancel();
private:
	void CompleteNow(TInt aError);
private:
	MComm *iComm;
	};

NONSHARABLE_CLASS(CCommLinkStatus) : public CActive
/**
@internalComponent
*/
	{
friend class MComm;
public:
	CCommLinkStatus(MComm* aComm);
	virtual ~CCommLinkStatus();
	virtual void RunL();
	virtual void DoCancel();
	virtual void NotifyDisconnect();
	virtual void SetRole(TCommRole aRole);
private:
	MComm *iComm;
	TUint iSignals;
	TUint iSavedSignalState;
	TCommRole iRole;
	TUint iNotifyChangeSignalMask;
	};

#if !defined(__EABI__) && !defined(__X86GCC__)
EXPORT_C MComm::MComm()
/**
Constructor
*/
	{
	}
#endif

EXPORT_C void MComm::CommDelete()
	{
	delete iCommReader;
	delete iCommWriter;
	delete iCommLinkStatus;
	iCommPort.Close();
	iCommServer.Close();
	}

	
EXPORT_C TInt MComm::CommOpen(const TDesC& aDll, const TDesC& aName, TCommAccess aMode)
	{
	return CommOpen(aDll, aName, aMode, ECommRoleDTE);
	}

EXPORT_C TInt MComm::CommOpen(const TDesC& aDll, const TDesC& aName, TCommAccess aMode, TCommRole aRole)
	{

	LOG( NifmanLog::Printf(_L("MComm:\t\t\tOpening CSY '%S' Port '%S' Role '%d'"), &aDll, &aName, aRole); )
	
	__ASSERT_ALWAYS(iCommLinkStatus!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
    iCommLinkStatus->SetRole(aRole);

	TInt err;
	if (err = iCommServer.Connect(), err!=KErrNone)
		{
		LOG( NifmanLog::Printf(_L("MComm:\t\t\tConnect failed (error is %d)"), err); )
		return err;
		}
	if (aDll.Length()>0)
		{
		if (err = iCommServer.LoadCommModule(aDll), err!=KErrNone)
			{
			LOG( NifmanLog::Printf(_L("MComm:\t\t\tCould not open CSY '%S' (error is %d)"), &aDll, err); )

			iCommServer.Close();
			return err;
			}
		}
	if (err = iCommPort.Open(iCommServer, aName, aMode, aRole), err!=KErrNone)
		{
		LOG( NifmanLog::Printf(_L("MComm:\t\t\tCould not open Port '%S' (error is %d)"), &aName, err); )

		iCommServer.Close();
		return err;
		}
	return KErrNone;
	}

EXPORT_C TInt MComm::CommOpen(const TDesC& aName, TCommAccess aMode)
	{

	LOG( NifmanLog::Printf(_L("MComm:\t\t\tOpening Port '%S'"), &aName); )

	TInt err;
	if (err = iCommServer.Connect(), err!=KErrNone)
		return err;
	if (err = iCommPort.Open(iCommServer, aName, aMode), err!=KErrNone)
		{
		LOG( NifmanLog::Printf(_L("MComm:\t\t\tCould not open Port '%S' (error is %d)"), &aName, err); )

		iCommServer.Close();
		return err;
		}
	return KErrNone;
	}

EXPORT_C void MComm::CommClose()
	{
	iCommReader->Cancel();
	iCommWriter->Cancel();
	iCommLinkStatus->Cancel();
	iCommPort.Close();
	iCommServer.Close();
	}

EXPORT_C void MComm::CommConstructL(TInt aReadPriority, TInt aWritePriority)
	{
	iCommReader = new (ELeave) CCommReader(this, aReadPriority);
	iCommWriter = new (ELeave) CCommWriter(this, aWritePriority);
	iCommLinkStatus = new (ELeave) CCommLinkStatus(this);
	}

EXPORT_C void MComm::CommWrite(const TDesC8& aDes)
	{
	__ASSERT_ALWAYS(iCommWriter!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iCommPort.Write(iCommWriter->iStatus, aDes);
	iCommWriter->SetActive();
	iCommLinkStatus->NotifyDisconnect();
	}

EXPORT_C void MComm::CommWriteReady()
	{
	__ASSERT_ALWAYS(iCommWriter!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iCommPort.Write(iCommWriter->iStatus, TPtrC8(NULL, 0));
	iCommWriter->SetActive();
	iCommLinkStatus->NotifyDisconnect();
	}

EXPORT_C TBool MComm::CommIsWriting() const
	{

	return iCommWriter->IsActive();
	}

EXPORT_C void MComm::CommRead(TDes8& aDes)
	{
	__ASSERT_ALWAYS(iCommReader!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iCommPort.Read(iCommReader->iStatus, aDes, aDes.Length());
	iCommReader->SetActive();
	iCommLinkStatus->NotifyDisconnect();
	}

EXPORT_C void MComm::CommReadOneOrMore(TDes8& aDes)
	{
	__ASSERT_ALWAYS(iCommReader!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iCommPort.ReadOneOrMore(iCommReader->iStatus, aDes);
	iCommReader->SetActive();
	iCommLinkStatus->NotifyDisconnect();
	}

EXPORT_C void MComm::CommReadReady()
	{
	__ASSERT_ALWAYS(iCommReader!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iCommReader->SetActive();
	iCommLinkStatus->NotifyDisconnect();
	}

EXPORT_C TBool MComm::CommIsReading() const
	{

	return iCommReader->IsActive();
	}

EXPORT_C void MComm::CommCancel()
	{
	if (iCommWriter)
		iCommWriter->Cancel();
	if (iCommReader)
		iCommReader->Cancel();
	if (iCommLinkStatus)
		iCommLinkStatus->Cancel();
	}

EXPORT_C void MComm::CommWriteCancel()
	{
	if (iCommWriter)
		iCommWriter->Cancel();
	}

EXPORT_C void MComm::CommReadCancel()
	{
	if (iCommReader)
		iCommReader->Cancel();
	}

//

CCommWriter::CCommWriter(MComm* aComm, TInt aPriority)
	: CActive(aPriority), iComm(aComm)
	{
	CActiveScheduler::Add(this);
	}

CCommWriter::~CCommWriter()
	{
	Cancel();
	}

void CCommWriter::RunL()
	{
	iComm->CommWriteCancel();
	iComm->CommWriteComplete(iStatus.Int());
	}

void CCommWriter::DoCancel()
	{
	iComm->iCommPort.WriteCancel();
	}

void CCommWriter::CompleteNow(TInt aError)
	{
	TRequestStatus* statusPtr=&iStatus;
	User::RequestComplete(statusPtr,aError);
	}

//

CCommReader::CCommReader(MComm* aComm, TInt aPriority)
	: CActive(aPriority), iComm(aComm)
	{
	CActiveScheduler::Add(this);
	}

CCommReader::~CCommReader()
	{
	Cancel();
	}

void CCommReader::RunL()
	{
	iComm->CommReadCancel();
	iComm->CommReadComplete(iStatus.Int());
	}

void CCommReader::DoCancel()
	{
	iComm->iCommPort.ReadCancel();
	}

void CCommReader::CompleteNow(TInt aError)
	{
	TRequestStatus* statusPtr=&iStatus;
	User::RequestComplete(statusPtr,aError);
	}

//

CCommLinkStatus::CCommLinkStatus(MComm* aComm)
	: CActive(EPriorityHigh), iComm(aComm)
	{
	iRole = ECommRoleDTE;
	iNotifyChangeSignalMask = KSignalDCD; // Defaults to KSignalDCD
	CActiveScheduler::Add(this);
	}

CCommLinkStatus::~CCommLinkStatus()
	{
	Cancel();
	}

void CCommLinkStatus::SetRole(TCommRole aRole)
    {
    iRole = aRole;
	if (iRole==ECommRoleDCE)
	    {
	    iNotifyChangeSignalMask = KSignalDTR;
	    }
	else
	    {
	    iNotifyChangeSignalMask = KSignalDCD; // Defaults to KSignalDCD
	    }
    
    }


void CCommLinkStatus::NotifyDisconnect()
	{
	if(IsActive())	// if we have already set up notification
		return;		// there is nothing to do now

	// get the current state of the line so when we get notification we can tell if it really has changed	
	iSavedSignalState = iComm->iCommPort.Signals (iNotifyChangeSignalMask) & iNotifyChangeSignalMask ;
	iComm->iCommPort.NotifySignalChange(iStatus, iSignals, iNotifyChangeSignalMask);

	SetActive();
	}

void CCommLinkStatus::RunL()
	{
	// We should only tear down the connection if DCD/DTR (depending on role) is low.
	//   and if the signal has changed (i.e. it was high)
	if (!(iNotifyChangeSignalMask & iSignals) && (iNotifyChangeSignalMask & iSavedSignalState)) 
		{
		if (iComm->CommIsReading())
			{
			iComm->CommReadCancel();
			iComm->CommReadComplete(KErrCommsLineFail);
			}

		if (iComm->CommIsWriting())
			{
			iComm->CommWriteCancel();
			iComm->CommWriteComplete(KErrCommsLineFail);
			}
		}
		iSavedSignalState = iSignals & iNotifyChangeSignalMask ;	// update saved state
	}

void CCommLinkStatus::DoCancel()
	{
	iComm->iCommPort.NotifySignalChangeCancel();
	}
