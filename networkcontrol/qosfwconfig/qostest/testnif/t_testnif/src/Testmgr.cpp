// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <f32file.h>
#include <hal.h>
#include "NIFMAN.H"
#include "TestMgr.h"
#include "DummyAgtRef.h"
#include "dummyprotocol.h"
#include "Tlog.h"


CTestMgr::CTestMgr(): CActive(CActive::EPriorityStandard)
	{
	};

CTestMgr* CTestMgr::NewL()
	{
	CTestMgr* self = NewLC();
	CleanupStack::Pop();    // self
	return self;
	};

CTestMgr* CTestMgr::NewLC() 
	{
	CTestMgr * self = new (ELeave) CTestMgr();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	};

void CTestMgr::ConstructL()
	{
	CActiveScheduler::Add(this);
	iContainers=CObjectConIx::NewL();
	iAgentFactories=iContainers->CreateL();
	iIfFactories=iContainers->CreateL();
	ipDummyAgtRef=CDummyNifAgentRef::NewL(_L("aa"),*this);
	iCurrentState=EAgentsLoading;
	ipDummyPrt=CDummyProtocol::NewL();
	TimerConstructL(10);// Construct the Timer for checking Asynchronous Call
	};

CTestMgr::~CTestMgr()
	{
	delete ipDummyAgtRef;
	iInterface = NULL;
	iInterfaceBound = NULL;
	delete ipDummyPrt;
	

	
	
//	CObjectCon* con;
//	TInt i;
//	if(iIfFactories)
//		{
//		con = iIfFactories;
//		for(i=0 ; i<con->Count() ; ++i)
//			{
//			(*con)[i]->Close();
//			delete (*con)[i];
//			}
//		}
//
//	if(iAgentFactories)
//		{
//		con = iAgentFactories;
//		for(i=0 ; i<con->Count() ; ++i)
//			{
//			(*con)[i]->Close();
//			delete(*con)[i];
//			}
//		}
//	iAgentFactories = NULL;
//	iIfFactories = NULL;
//	delete iContainers;
	TimerDelete();
	};

void CTestMgr::DoCancel()
	{
	TimerCancel();
	};

void CTestMgr::StartLoadingAgentL()
	{
    
	TestLog.Printf(_L("Starting loading Agent\n"));

	TAutoClose<RLibrary> lib;
	User::LeaveIfError(lib.iObj.Load(KPppNtRasAgentFileName));
	lib.PushL();

#ifdef _UNICODE
	TUid uid(TUid::Uid(KUidUnicodeNifmanAgent));
#else
	TUid uid(TUid::Uid(KUidNifmanAgent));
#endif

	if(lib.iObj.Type()[1]!=uid)
		User::Leave(KErrCorrupt);

	typedef CNifFactory* (*TNifFactoryNewL)();
	TNifFactoryNewL libEntry=(TNifFactoryNewL)lib.iObj.Lookup(1);
	if (libEntry==NULL)
		User::Leave(KErrCorrupt);

	CNifFactory* f=(*libEntry)(); // Opens CObject
	if (!f)
		User::Leave(KErrCorrupt);

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, f));
	//CObjectCon* pContainer=CObjectCon::NewL();
	//CleanupStack::PushL(pContainer);
	f->InitL(lib.iObj, *iAgentFactories); // Transfers the library object if successful
	// Can pop the library now - auto close will have no effect because handle is null
	//CleanupStack::Pop();//pContainer
	CleanupStack::Pop();//pFactory

	lib.Pop();


//	ipDummyAgtRef->ServiceStarted();



	iAgentFactory=(CNifAgentFactory*)f;
	Notify(EAgentLoaded);
	};


void CTestMgr::StartLoadingNifL(const TDesC& aInterfaceName, const TDesC& aBaseName)
	{

	TestLog.Printf(_L("Starting loading Nif\n"));
	// load the required interface
	TInt errCode=KErrNone;
	//create the interface as nifman does. The code taken from NifMan (with custom error check)
	TAutoClose<RLibrary> lib;
	//errCode=lib.iObj.Load(KPppNifFileNameVerify);
	errCode=lib.iObj.Load(KPppNifFileName);
	if (errCode!=KErrNone)
	{
 		TestLog.Printf(_L("Can't load tppp.nif (error=%d) -> Leaving"),errCode);
		User::Leave(errCode);
	}
	lib.PushL();
	// The Uid check
#ifdef _UNICODE
	TUid uid(TUid::Uid(KUidUnicodeNifmanInterface));
#else
	TUid uid(TUid::Uid(KUidNifmanInterface));
#endif
	if(lib.iObj.Type()[1]!=uid)
	{
		TestLog.Printf(_L("Wrong UID of ppp.nif -> Leaving"));
		User::Leave(KErrCorrupt);
	}
	//get the entry point
	typedef CNifFactory* (*TNifFactoryNewL)();
	TNifFactoryNewL libEntry=(TNifFactoryNewL)lib.iObj.Lookup(1);//the factory function has ordinal ==1
	if (libEntry==NULL)
	{
		TestLog.Printf(_L("No factory method in ppp.nif -> Leaving"));
		User::Leave(KErrCorrupt);
	}

	CNifFactory* pFactory =(*libEntry)(); // Opens CObject
	if (!pFactory)
	{
		TestLog.Printf(_L("Can't create factory in ppp.nif -> Leaving"));
		User::Leave(KErrCorrupt);
	}

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, pFactory));
	pFactory->InitL(lib.iObj, *iIfFactories); // Transfers the library object if successful
	// Can pop the library now - auto close will have no effect because handle is null
	CleanupStack::Pop();//pFactory
	lib.Pop();

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, pFactory));
	//another dirty hack
	CCheekyNifIfFactory* pFactoryExt=(CCheekyNifIfFactory*)pFactory;
    iInterface = REINTERPRET_CAST(CNifIfLink*,pFactoryExt->CreateInterfaceL(aInterfaceName));
    CleanupStack::PopAndDestroy(); // close extra reference on Factory

	if(!iInterface)
	{
		TestLog.Printf(_L("Failed to create interface -> Leaving"));
		User::Leave(KErrCorrupt);
	}
	//a hack to do the same as "iInterface->iNotify=this" in the real CNifAgentRef class
	CCheekyNifIfBase* ptr=(CCheekyNifIfBase*)iInterface;
	if (!ptr) 
	{
		TestLog.Printf(_L("Can't register notify in agent  -> Leaving"));
		User::Leave(KErrCorrupt);
	}

	ptr->SetNotify(ipDummyAgtRef);//pretend to be "normal" agentref

	//use hardcoded only if the ini file read or search failed
	iInterfaceBound=iInterface->GetBinderL(aBaseName);  //Do as if we bind to the ipv6 protocol to LCP
	iInterfaceBound->BindL(ipDummyPrt);					// bind the Dummy protocol to the NCP

	ipDummyAgtRef->SetInterface(iInterface);		//???
	ipDummyAgtRef->SetNifIfBase(iInterfaceBound);	//???
	ipDummyPrt->SetiNifL(iInterfaceBound);			// finish loading the TestNif 
	iCurrentState = EBothLoaded;					// so change state and signal 

	};


// in our case there is only one timer event so all we need to do is to 
// signal RunL to kick again .
void CTestMgr::TimerComplete(TInt /*aStatus*/)  
	{
	// Trigger the Active object.
	SetActive();
	TRequestStatus* theStatus=&iStatus;
	User::RequestComplete(theStatus, KErrNone);	
	};

void CTestMgr::Notify(TEvent aEvent)
	{
	iEvent = aEvent;
	SetActive();
	TRequestStatus* theStatus=&iStatus;//for the next line to compile
	User::RequestComplete(theStatus, KErrNone);
	};
// *****************************************************************************************
// *****************************************************************************************
//Test :
// each time we run call the test the state increment  so the next time we will execute the next command 
// the first parameter we send is cuase sometime we testing call that we know need to fail 
// in that case fail is success 
// the timetowait that we returning is the time we want the mgr to wait upto the next call to Test.
//

/*
TInt CTestMgr::Test1L(void)
{
static TInt state=0;
TInt	timetowait=0;
	switch(state)
	{
	case 0 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;												
	case 1 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 2 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 3 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 4 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 5 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 6 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 7 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 8 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 9 : ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 10: ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 11: ipDummyPrt->NewPdpContextL(1);timetowait=5000; //expected to fail 
		break;
	case 12: ipDummyPrt->DeleteL(0,3);timetowait=5000;
		break;
	case 13: ipDummyPrt->DeleteL(0,5);timetowait=5000;
		break;
	case 14: ipDummyPrt->DeleteL(1,3);timetowait=5000;
		break;
	case 15: ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 16: 
		break;
	default:TestLog.Printf(_L("Test 1 Ended \n"));
			state = 0;				// be readyt to run again.
			return -1;
		}
	state++;
	return timetowait;// return the time to wait to next control call
}
*/
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
//Test :
// each time we run call the test the state increment  so the next time we will execute the next command 
// the first parameter we send is cuase sometime we testing call that we know need to fail 
// in that case fail is success 
// the timetowait that we returning is the time we want the mgr to wait upto the next call to Test.
//

TInt CTestMgr::Test1L(void)
{
static TInt state=0;
TUint8 tmpArray[6]={10,01,10,10,10,10};
TUint8 tmpArrayMask[6]={10,01,10,10,10,10};
TInt	timetowait=100;

	switch(state)
	{
	case 0 : TestLog.Printf(_L("\n Open New Context 2 \n")); ipDummyPrt->NewPdpContextL(0);timetowait=3000;
		break;												
	case 1 : //ipDummyPrt->ActivateL(1,2);timetowait=3000;
		break;
	case 2 : //ipDummyPrt->ActivateL(1,5);timetowait=3000;
		break;
	case 3 :TestLog.Printf(_L("\n Adding TFT to context 2  filter 1 \n"));
			
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			ipDummyPrt->ModifyTftL(0,2,1);timetowait=5000; 
		break;
	case 4 : TestLog.Printf(_L("\n Open New Context 3 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=3000;
		break;			 
	case 5 : TestLog.Printf(_L("\n Set Qos Context 2 Parameter1   \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500 ,1, 0);
			 #else
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500);
			 #endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,2);timetowait=5000;
		break;
	case 6 : TestLog.Printf(_L("\n Activate context 2 \n"));ipDummyPrt->ActivateL(0,2);timetowait=3000;
		break;
	case 7 : TestLog.Printf(_L("\n Activate again context 2  \n"));ipDummyPrt->ActivateL(0,2);timetowait=3000;
		break;
	case 8 : 
		break;
	case 9 : 
		break;
	case 10: 
		break;
	case 11: 
		break;
	case 12: 
		break;
	case 13: 
		break;
	case 14: 
		break;
	case 15: TestLog.Printf(_L("\n End test delete context 2  \n"));ipDummyPrt->DeleteL(0,2);timetowait=5000;
		break;
	case 16: TestLog.Printf(_L("\n End test delete context 3  \n"));ipDummyPrt->DeleteL(0,3);timetowait=5000;
		break;
	default:TestLog.Printf(_L("Test 1 Ended \n"));
			state = 0;				
			return -1;
		}
	state++;
	return timetowait;// return the time to wait to next control call
}

// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
//Test :
// each time we run call the test the state increment  so the next time we will execute the next command 
// the first parameter we send is cuase sometime we testing call that we know need to fail 
// in that case fail is success 
// the timetowait that we returning is the time we want the mgr to wait upto the next call to Test.
//

TInt CTestMgr::Test2L(void)
{
static TInt state=0;
TInt	timetowait=0;
	switch(state)
	{
	case 0 : TestLog.Printf(_L("\n Create context 2 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=3000;
		break;												
	case 1 : TestLog.Printf(_L("\n Set qos context 2 param 1  \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500,1,0);
			#else
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,2);timetowait=5000;
		break;
	case 2 : TestLog.Printf(_L("\n Set qos context X param 1 \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500 ,1,0);
			#else
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500 );
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(1,3);timetowait=5000;
		break;
	case 3 : TestLog.Printf(_L("\n Set qos context 2 param X \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x2, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00);
			#else
				ipDummyPrt->SetQosReqParameter(0x2, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,2);timetowait=5000;
		break;
	case 4 : TestLog.Printf(_L("\n Activate context 2 \n"));
		     ipDummyPrt->ActivateL(0,2);timetowait = 3000;
		break;
	case 5 : TestLog.Printf(_L("\n Set qos context 2 param 2 \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQosReqParameter(0x2, 0x03, 0x02, 0x04, 0x01, 0x04, 0x02, 1000, 200, 2000, 2000, 100, 100, 0x04, 0x02, 0x04, 0x04, 0x02, 0x02, 500, 50, 500, 500, 50, 50,1,0	);
			#else
			 ipDummyPrt->SetQosReqParameter(0x2, 0x03, 0x02, 0x04, 0x01, 0x04, 0x02, 1000, 200, 2000, 2000, 100, 100, 0x04, 0x02, 0x04, 0x04, 0x02, 0x02, 500, 50, 500, 500, 50, 50	);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,2);timetowait=5000;
		break;
	case 6 : TestLog.Printf(_L("\n Creatre context 3 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=3000;
		break;
	case 7 : TestLog.Printf(_L("\n Set qos context 3 param X \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x3 ,0x09, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x00, 0x00);
			 #else
				ipDummyPrt->SetQosReqParameter(0x3 ,0x09, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02);
			 #endif 
            // SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,3);timetowait=5000;
		break;
	case 8 : TestLog.Printf(_L("\n Set qos context 3 param 1 \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500,1,0	);
			#else
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,3);timetowait=5000;
		break;
	case 9 : TestLog.Printf(_L("\n Set qos context 3 param 1 \n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500,1,0	);
			 #else
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500);
			 #endif 
			  // SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,3);timetowait=5000;
		break;
	case 10: 
		break;
	case 11: 
		break;
	case 12: 
		break;
	case 13: 
		break;
	case 14: 
		break;
	case 15: TestLog.Printf(_L("\n End test delete context 2 \n"));ipDummyPrt->DeleteL(0,2);timetowait=5000;
		break;
	case 16: TestLog.Printf(_L("\n End test delete context 3 \n"));ipDummyPrt->DeleteL(0,3);timetowait=5000;
		break;
	default:TestLog.Printf(_L("Test 2 Ended \n"));
			state = 0;				
			return -1;
		}
	state++;
	return timetowait;// return the time to wait to next control call
}

// *****************************************************************************************
// *****************************************************************************************

TInt CTestMgr::Test3L(void)
{
static TInt state=0;
TInt	timetowait=0;
TUint8 tmpArray[6]={10,01,10,10,10,10};
TUint8 tmpArrayMask[6]={10,01,10,10,10,10};
TInt DeleteLId[8]={0,0,0,0,0,0,0,0};


	switch(state)
	{
	case 0 : TestLog.Printf(_L("\n Creatre context 2 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=3000;
		break;												
	case 1 :TestLog.Printf(_L("\n Adding TFT to context 2  filter 1 \n"));
			
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			ipDummyPrt->ModifyTftL(0,2,1);timetowait=5000;
		break;
	case 2 :TestLog.Printf(_L("\n Adding TFT to context X  filter 1 \n"));
			
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(4, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			ipDummyPrt->ModifyTftL(1,3,1);timetowait=5000;
		break;
	case 3 : TestLog.Printf(_L("\n Activating context 2 in tsy mode we need to fail \n"));
			 ipDummyPrt->ActivateL(1,2);timetowait=3000;
		break;
	case 4 :TestLog.Printf(_L("\n Adding TFT to context 2  filter 2 \n"));
			
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(2, 0, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			ipDummyPrt->ModifyTftL(0,2,1);timetowait=3000;
		break;
	case 5 :TestLog.Printf(_L("\n Adding TFT to context 4  filter X \n"));
			
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(4, 0, tmpArray, tmpArrayMask, 10, 0, 10, 20, 0, 0, 10, 20) ;
			ipDummyPrt->ModifyTftL(1,4,1);timetowait=3000;
		timetowait=300;
		break;
	case 6 :TestLog.Printf(_L("\n Adding TFT to context 2  filter 3,4,5,6,7,8 \n"));
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->ModifyTftL(0,2,1);timetowait=3000;
		break;
	case 7 :TestLog.Printf(_L("\n Adding TFT to context 2  filter 1  After 8 already in \n"));
			
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			ipDummyPrt->ModifyTftL(1,2,1);timetowait=3000;  
		break;
	case 8 : timetowait=30;
		break;
	case 9 : timetowait=30;
		break;
	case 10: timetowait=30;
		break;
	case 11:TestLog.Printf(_L("\n Deleeing TFT to context 2  filter 2 \n"));
			ipDummyPrt->newTFTFilters();
			DeleteLId[0] = 2;
			ipDummyPrt->deleteTFT(2,DeleteLId);
			ipDummyPrt->ModifyTftL(0,2,2);timetowait=3000;
		break;
	case 12:TestLog.Printf(_L("\n Deleteing TFT to context 2  filter 3,4,6 \n"));
			ipDummyPrt->newTFTFilters();
			DeleteLId[0] = 6;DeleteLId[1] = 3;DeleteLId[2] = 4;
			ipDummyPrt->deleteTFT(2,DeleteLId);
			ipDummyPrt->ModifyTftL(0,2,2);timetowait=3000; 
		break;
	case 13:TestLog.Printf(_L("\n Deleteing TFT to context 2  filter 2 after already deleted \n"));
			ipDummyPrt->newTFTFilters();
			DeleteLId[0] = 2;
			ipDummyPrt->deleteTFT(2,DeleteLId);
			ipDummyPrt->ModifyTftL(0,2,2);timetowait=3000;
		break;
	case 14:TestLog.Printf(_L("\n Adding TFT to context 2  filter X \n"));
			ipDummyPrt->newTFTFilters();
			DeleteLId[0] = 20;
			ipDummyPrt->deleteTFT(2,DeleteLId);
			ipDummyPrt->ModifyTftL(0,2,2);timetowait=3000; 
		break;
	case 15:TestLog.Printf(_L("\n Creatre context 3 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=3000;
		break;
	case 16:TestLog.Printf(_L("\n Adding TFT to context 3  filter 1,2,3,4,5,6 \n"));
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(3, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(3, 0, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(3, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(3, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(3, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->addTFTFilterParameter(3, 96, tmpArray, tmpArrayMask, 6, 7, 7, 0, 0, 0, 0, 0) ;
			
			ipDummyPrt->ModifyTftL(0,3,1);timetowait=3000;
		break;
	case 17:TestLog.Printf(_L("\n Deleteing TFT to context 2  All filter  \n"));
			ipDummyPrt->ModifyTftL(0,2,3);timetowait=3000; 
		break;
	case 18:TestLog.Printf(_L("\n End test delete context 2 \n"));ipDummyPrt->DeleteL(0,2);timetowait=5000;
		break;
	case 19:TestLog.Printf(_L("\n End test delete context 3 \n"));ipDummyPrt->DeleteL(0,3);timetowait=5000;
		break;
	default:TestLog.Printf(_L("Test 3 Ended \n"));
			ipDummyPrt->cleanDummy();
			state = 0;	
			return -1;
		}
	state++;
	return timetowait;// return the time to wait to next control call
}
// *****************************************************************************************
// *****************************************************************************************

TInt CTestMgr::Test4L(void)
{
static TInt state=0;
TInt	timetowait=0;
TUint8 tmpArray[6];
TUint8 tmpArrayMask[6];

	switch(state)
	{
	case 0 : TestLog.Printf(_L("\n Creatre context 2 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;												
	case 1 :TestLog.Printf(_L("\n Adding TFT to context 2  filter 1 \n"));
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(2, 96, tmpArray, tmpArrayMask,  6, 7, 7, 0, 0, 0, 0, 0) ;
			ipDummyPrt->ModifyTftL(0,2,1);timetowait=5000;
		break;
	case 2 : TestLog.Printf(_L("\n Modify Active context 2 before active or setqos \n"));
			 ipDummyPrt->ModifyActiveL(0,2);timetowait=5000;
		break;
	case 3 : TestLog.Printf(_L("\n SetQos context 2  paramet 1\n"));
			 #ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500,1,0);
			 #else
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500);
			 #endif 
			 // SYMBIAN_NETWORKING_UMTSR5 
			 ipDummyPrt->SetQoSL(0,2);timetowait=5000;
		break;
	case 4 : TestLog.Printf(_L("\n Activate context 2 \n"));
		     ipDummyPrt->ActivateL(0,2);timetowait=5000;
		break;
	case 5 : TestLog.Printf(_L("\n Adding TFT to context 2  filter 3 \n"));
			ipDummyPrt->newTFTFilters();
			ipDummyPrt->addTFTFilterParameter(2, 0, tmpArray, tmpArrayMask,  6, 7, 7, 0, 0, 0, 0, 0) ;
			ipDummyPrt->ModifyTftL(0,2,1);timetowait=3000;
		break;
	case 6 :TestLog.Printf(_L("\n SetQos context 2  paramet 2\n"));
			#ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x2, 0x03, 0x02, 0x04, 0x01, 0x04, 0x02, 1000, 200, 2000, 2000, 100, 100, 0x04, 0x02, 0x04, 0x04, 0x02, 0x02, 500, 50, 500, 500, 50, 50,1,0);
			#else
				ipDummyPrt->SetQosReqParameter(0x2, 0x03, 0x02, 0x04, 0x01, 0x04, 0x02, 1000, 200, 2000, 2000, 100, 100, 0x04, 0x02, 0x04, 0x04, 0x02, 0x02, 500, 50, 500, 500, 50, 50);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			ipDummyPrt->SetQoSL(0,2);timetowait=5000;
		break;
	case 7 :TestLog.Printf(_L("\n Modify Active context 2  \n"));
			ipDummyPrt->ModifyActiveL(0,2);timetowait=5000;
		break;
	case 8 :TestLog.Printf(_L("\n Modify Active context X  \n"));
			ipDummyPrt->ModifyActiveL(1,4);timetowait=5000;
		break;
	case 9 :TestLog.Printf(_L("\n Modify Active context 2 again but with no chane in between  \n"));
			ipDummyPrt->ModifyActiveL(0,2);timetowait=5000;
		break;
	case 10:
		break;
	case 11: 
		break;
	case 12: 
		break;
	case 13: 
		break;
	case 14: 
		break;
	case 15: 
		break;
	case 16:TestLog.Printf(_L("\n End test delete context 2 \n"));ipDummyPrt->DeleteL(0,2);timetowait=5000;				
		break;
	default:TestLog.Printf(_L("Test 4 Ended \n"));
			state = 0;
			return -1;
		}
	state++;
	return timetowait;// return the time to wait to next control call
}
// *****************************************************************************************
// *****************************************************************************************


TInt CTestMgr::Test5L(void)
{
static TInt state=0;
TInt	timetowait=0;

	switch(state)
	{
	case 0 : TestLog.Printf(_L("\n Creatre context 3 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;												
	case 1 :TestLog.Printf(_L("\n SetQos context 2  paramet 1\n"));
			#ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500,1,0);
			#else
				ipDummyPrt->SetQosReqParameter(0x2, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			ipDummyPrt->SetQoSL(0,2);timetowait=5000;
		break;
	case 2 :TestLog.Printf(_L("\n Creatre context 3 \n"));ipDummyPrt->NewPdpContextL(0);timetowait=5000;
		break;
	case 3 :TestLog.Printf(_L("\n SetQos context 3  paramet 1\n"));
			#ifdef SYMBIAN_NETWORKING_UMTSR5 
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500,1,0);
			#else
				ipDummyPrt->SetQosReqParameter(0x3, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 1500, 100, 1000, 1000, 500, 500, 0x04, 0x02, 0x08, 0x04, 0x02, 0x02, 1000, 100, 1000, 1000, 500, 500);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
			ipDummyPrt->SetQoSL(0,3);timetowait=5000;
		break;
	case 4 :TestLog.Printf(_L("\n Activate context 2 \n"));ipDummyPrt->ActivateL(0,2);timetowait=5000;
		break;
	case 5 :TestLog.Printf(_L("\n Activate context 3 \n"));ipDummyPrt->ActivateL(0,3);timetowait=5000;
		break;
	case 6 :TestLog.Printf(_L("\n End test delete context 2 \n"));ipDummyPrt->DeleteL(0,2);timetowait = 5000;
		break;
	case 7 :TestLog.Printf(_L("\n End test delete context 3 \n"));ipDummyPrt->DeleteL(0,3);timetowait = 5000;
		break;
	case 8 :
		break;
	case 9 :
		break;
	case 10:
		break;
	case 11: 
		break;
	case 12: 
		break;
	case 13: 
		break;
	case 14: 
		break;
	case 15: 
		break;
	case 16:
		break;
	default:TestLog.Printf(_L("Test 5 Ended \n"));
			state = 0;			
			return -1;
		}
	state++;
	return timetowait;// return the time to wait to next control call
}
// *****************************************************************************************
// *****************************************************************************************



// RunL :
// the mgr first lodingthe agent (ras) and then loading and starting the TestNif 
// after that we are fetching testcases from the Test function one by one 
// and each test case determine the time we need to go sleep up to the next testcase 
// in that way we can issue comand while the testnif still working on the early one 
// and check the functionality of the testnif.
void CTestMgr::RunL()
	{
TInt	timetowait=10;
static TInt TestNum=1;
	switch(iCurrentState)
        {
        case EAgentsLoading:
			// Start to connect
			ipDummyAgtRef->StartL(iAgentFactory);
			iCurrentState=EAgentConnecting;
            break;
		case EBothLoaded:
			TestLog.Printf(_L("Initialize  Process finish start to test... \n"));
			iCurrentState = ERunTestEntry;
			TimerCancel();
			TimerAfter(5000000);
			break;
		case ERunTestEntry:
			if (TestNum == 1 )		timetowait = Test1L();			
			else if (TestNum == 2 ) timetowait = Test2L();			
			else if (TestNum == 3 ) timetowait = Test3L();			
			else if (TestNum == 4 ) timetowait = Test4L();			
			else if (TestNum == 5 ) timetowait = Test5L();			
			else if (TestNum == 6 ) {timetowait = 100;iCurrentState = AllTestRun;}			
			
			if ( timetowait == -1) {timetowait=10;TestNum++;}
			ipDummyPrt->CheckTestEntry(timetowait); 
			TimerCancel();
			TimerAfter(timetowait*1000+1);			// we ned to sleep at least 1 MicroSec .
			break;
		case AllTestRun:
			TestLog.Printf(_L("No More Test to run !!! \n"));
			CActiveScheduler::Stop();			
			break;
        default:
            __ASSERT_ALWAYS(ETrue,User::Panic(_L("Wrong state"),KErrGeneral));
            break;
        }
	}
