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

#include <e32svr.h>
#include "Et_sstd.h"
#include "etelsize.h"


GLDEF_C void Fault(TEtelFault aFault)
//
// Panic the server
//
	{
	User::Panic(_L("Etel Fault"),aFault);
	}

GLDEF_C void PanicClient(TEtelPanic aPanic,const RMessage2& aMessage)
//
// Panic the client on server side
//
	{
	_LIT(KETelClientFault,"Etel Server");
	aMessage.Panic(KETelClientFault,aPanic);
	}

// When Security ETEL is introduced it is no longer possible to create a thread under C32 process
// This function will create a thread under current process. If some of the test are relying on 
// scheduling threads under c32 process, rather then current lower priority process, solution with
// creation of high priority process carrying only this thread should be introduced.
GLDEF_C TInt CreateThreadInCurrentProc(const TDesC& aLibraryName, const TDesC& aThreadName, TThreadFunction aFunction, TInt aStackSize, TInt aHeapMinSize, TInt aHeapMaxSize)
	{
	TInt res;

	RLibrary lib;
	res = lib.Load(aLibraryName);
	if (res==KErrAlreadyExists)
		res=KErrNone;
	if (res==KErrNone)
		{
		RThread t;
		res=t.Create(aThreadName,aFunction,aStackSize,aHeapMinSize,aHeapMaxSize,NULL);
		if (res==KErrNone)
			{
			t.Resume();
			t.Close();
			}
		lib.Close();
		}
	return res;
	}

GLDEF_C TInt StartEtel(TAny* /*aPtr*/)
//
// Request on the comm server to start ETel's thread function from within the C32 process
// No need for a mutex since C32 will only attempt to start an ETel server thread one at a time.
// C32 does not guarantee that the thread has already started on the completion of
//	StartServerThread(), so we implement the semaphore here.
//
	{
	RSemaphore s;
	TInt res=KErrNone;
	if(s.CreateGlobal(KETelSemaphoreName,0)!=KErrNone)
		res=s.OpenGlobal(KETelSemaphoreName);
	if (res==KErrNone)
		{
			res = CreateThreadInCurrentProc(KEtelDLLName,ETEL_SERVER_NAME,EtelServerThread,KETelDefaultStackSize,KETelDefaultHeapSize,KETelMaxHeapSize);
		if (res==KErrNone)
			s.Wait();
		s.Close();
		}
	return res;
	}

