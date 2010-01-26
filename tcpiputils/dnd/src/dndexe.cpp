// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// dnd.cpp - name resolver daemon main module
//

#include "demon.h"
#include "inet6log.h"
#include <cflog.h>

//Logging name for the DND
#define DND_COMPNAME "dnd"

//
// class CDndMain
//
class CDndMain : public CBase, public MDemonMain
    {
public:
	~CDndMain();
	void ConstructL();
	void Write(const TDesC &aDes);
	void WriteList(const TDesC &aFmt, VA_LIST aList);
	TInt CheckResult(const TDesC &aText, TInt aResult);
	
	MDemonEngine *iModel;


private:
#ifdef _LOG
	TBuf<1024> iBuf;	//< A work buffer for formatting messages (WriteList)
#endif
 __CFLOG_DECLARATION_MEMBER;
   };


// **************
// "Main Program"
// **************
//


CDndMain::~CDndMain()
	{
	delete iModel;
	 __CFLOG_CLOSE;
  	 __CFLOG_DELETE;
	}
    

void CDndMain::ConstructL()
	{
	__CFLOG_CREATEL;
  	__CFLOG_OPEN;
	iModel = MDemonEngine::NewL(*this);
	iModel->ConstructL();
	}


void CDndMain::Write(const TDesC &aMsg)
	{
#ifndef _LOG
	(void)aMsg;	// ..just to silence compiler warning
#else
	Log::Write(aMsg);
#endif
	}

void CDndMain::WriteList(const TDesC &aFmt, VA_LIST aList)
	{
#ifndef _LOG
	(void)aFmt;	// ..just to silence compiler warning
	(void)aList;
#else
	iBuf.FormatList(aFmt, aList);
	Log::Write(iBuf);
#endif
	}

TInt CDndMain::CheckResult(const TDesC &/*aText*/, TInt aResult)
	{
	return aResult;
	}

static void DaemonL()
	{
	CDndMain *main = new (ELeave) CDndMain;
	CleanupStack::PushL(main);
	CActiveScheduler::Install(new (ELeave) CActiveScheduler);
	main->ConstructL();
	CActiveScheduler::Start();
	CleanupStack::PopAndDestroy();
	delete CActiveScheduler::Current();
	}

#ifdef __DLL__
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
	{
	return(KErrNone);
	}
#endif

EXPORT_C TInt E32Main()
	{
	_LIT(KDnd,"DND");
	(void)RThread::RenameMe(KDnd);
	CTrapCleanup* cleanup = CTrapCleanup::New(); // get clean-up stack
	TRAPD(error, DaemonL());
	delete cleanup; // destroy clean-up stack
	__ASSERT_ALWAYS(!error,User::Panic(KDnd,error));
	return KErrNone;
	}
