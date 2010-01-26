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

#include "Et_clsvr.h"
#include "dummyEtel.h"
#include "ETELMMCS.H" // for etelmm functions
#include "ETELMM.H"
#include "etelpckt.h"
#include "pcktcs.h"
#include "etelQoS.h"

#include "Et_sstd.h"
#include "e32svr.h"

#define DOUBLE_COLON		_L("::")
#define ETEL_NAME			_L("Etel")
#define TSY_EXTENSION		_L(".TSY")


const TInt KLengthOfDoubleColon=2;

//=============================================================================
// JGG - test constants
_LIT(KPhoneName,"DMobile");
_LIT(KLineName,"DLine");
_LIT(KTsyName, "DUMMY.TSY");
static const TInt KPhoneId = 0x00000400;
static const TInt KPacketServiceId = 0x00000401;
static const TInt KPacketContextId = 0x00000402;
static const TInt KPacketQoSId = 0x00000403;
static const TInt KCallId = 0x00000404;
static const TInt KLineId = 0x00000405;


// CSD test constants
_LIT(KSerialCsy,"ECUART");
_LIT(KSerialPort,"COMM::0");
//=============================================================================

//
// CTelSession class definitions
//

inline CTelServer* CTelSession::Server() const
	{return static_cast<CTelServer*>(const_cast<CServer2*>(CSession2::Server()));}

CTelSession::CTelSession()
	: iPriorityClientHeap(NULL), iContextStatus(RPacketContext::EStatusInactive)
//
// C'Tor - must pass client to CSession
//
	{
	__DECLARE_NAME(_S("CTelSession"));
	}

CTelSession::~CTelSession()
//
//	clean up and go home.
//
	{
	// Just ensure that the server doing the closure all subsessions
	// if the client do not !
/*
	if(iObjectIx)
		{
		TInt handle;
		CObject* theObj=NULL;
		TInt count=iObjectIx->Count();

		LOGTEXT2(_L8("CTelSession::~CTelSession iObjectIx->Count()=%d"), count );
		
		for(TInt i=0;i<count;i++)
			{
			theObj=iObjectIx->operator[](i);
			if(theObj)
				{
				handle=iObjectIx->At(theObj);
				CTelObject* theTelObj=REINTERPRET_CAST(CTelObject*,theObj); //
				theTelObj->CloseSubSessionPreProcessing(this,handle);
				theTelObj=REINTERPRET_CAST(CTelObject*,theObj->Owner());
				iObjectIx->Remove(handle);

				if(theTelObj)
					theTelObj->TelObjectClose();
				}
			}
		LOGTEXT(_L8("Delete iObjectIx"));
		delete iObjectIx;
		}


	if (iTsyModulesIx)
		{
		LOGTEXT(_L8("Delete TsyModulesIx"));
		delete iTsyModulesIx;
		}
*/
	//=====================
	// JGG test code
	// Ensure TSY has been unloaded by client
	
	// Indicate to client it has been bad!!
	// Don't know if panicing them is the best thing but need to get the info
	// there somehow.
//	LOGTEXT2(_L8("CTelSession::~CTelSession iTsyLoadCount=%d"), iTsyLoadCount );
//	__ASSERT_DEBUG(iTsyLoadCount==0,PanicClient(testpanic1));
	// Ensure all the sub sessions have been closed
//	__ASSERT_DEBUG(iOpenSubSessionCount==0,PanicClient(testpanic2));
	//====================
	//UnsetPriorityClient();
	
	Server()->Dec();
	}

inline const RMessage2& CTelSession::Message() const 
	{return iMessage;}

void CTelSession::CreateL()
//
// Allocate any extra knobbly bits
//
	{
		
	Server()->Inc(); // increment session count by one
		
	iNameIndex=0;
	iSessionNumber = TelServer()->Count();
	if(TelServer()->iTestNumber == 601)
		{
		// Fail the connect - note only do this on session number 3
		// as this should be the AGT
		User::Leave(KErrGeneral);
		}
	else if( (TelServer()->iTestNumber == 20101)&&(iSessionNumber==2))
		{
		// Fail connect and leave with KErrGeneral for this test case
		User::Leave(KErrGeneral);
		}
	}

void CTelSession::ServiceL(const RMessage2& aMessage)
//
// Handle messages for this session
//
	{
	LOGTEXT2(_L8("CTelSession::Service Called, IPC: %d"),aMessage.Function());
	
	iMessage=aMessage;

	switch (aMessage.Function())
		{
	case EEtelPhoneInitialise:
		// completion for phone.Initialise in Psdagx
		aMessage.Complete(KErrNone);
		return;
	case EEtelOpenFromSession:
		//NewTelObject(ETelObjectOpenSourceSession);
		// JGG new call
		OpenFromSession();
		return;
	case EEtelOpenFromSubSession:
		//NewTelObject(ETelObjectOpenSourceSubSession);
		// JGG new call
		OpenFromSubSession();
		return;
	case EEtelOpenByNameFromSession:
		//NewTelObject(ETelObjectOpenByNameSourceSession);
		// JGG - don't think is used by code under test
		aMessage.Complete(KErrNotSupported);
		return;
	case EEtelOpenByNameFromSubSession:
		//NewTelObject(ETelObjectOpenByNameSourceSubSession);
		// JGG - don't think is used by code under test
		aMessage.Complete(KErrNotSupported);
		return;
#if defined (_DEBUG)
	case EEtelDbgMarkHeap:
		__UHEAP_MARK;
		aMessage.Complete(KErrNone);
		return;
	case EEtelDbgCheckHeap:
		__UHEAP_CHECK(aMessage.Int0());
		aMessage.Complete(KErrNone);
		return;
	case EEtelDbgMarkEnd:
		__UHEAP_MARKENDC(aMessage.Int0());
		aMessage.Complete(KErrNone);
		return;
	case EEtelDbgFailNext:
		__UHEAP_FAILNEXT(aMessage.Int0());
		aMessage.Complete(KErrNone);
		return;
#endif

	case EEtelServerLoadPhoneModule:
 		LoadPhoneModule();
		return;

	case EEtelServerClosePhoneModule:
		ClosePhoneModule();
		return;

	case EEtelServerPhoneInfoByIndex:
			{
			PhoneInfo(aMessage.Int1()); // use Int1() same as client side
			return;
			}

	case EEtelServerEnumeratePhones:
		EnumeratePhonesL();
		return;

	case EEtelServerSetPriorityClient:
		//SetPriorityClient();
		// JGG - don't think is used by code under test
		aMessage.Complete(KErrNotSupported);
		return;

	case EEtelServerQueryTsyFunctionality:
		//IsSupportedByTsy();
		// JGG - don't think is used by code under test
		aMessage.Complete(KErrNotSupported);
		return;

	case EEtelServerGetTsyName:
		GetTsyNameByPhone(aMessage.Int0());
		// JGG - don't think is used by code under test
		//aMessage.Complete(KErrNotSupported);
		return;

	case EEtelServerSetExtendedErrorGranularity:
		SetExtendedErrorGranularity();
		return;

	case EEtelServerGetTsyVersionNo:
		//GetTsyVersionNumber();
		// JGG - don't think is used by code under test
		aMessage.Complete(KErrNotSupported);
		return;
	// New client calls for testing
	case EDummyEtelSetTestNumber:
		{
		TelServer()->iTestNumber = aMessage.Int0();
		aMessage.Complete(KErrNone);
		}
		return;
	case EDummyEtelTriggerContextConfigDrop:
		if(TelServer()->iContextConfigChangeReq)
			{
			TelServer()->iContextConfigChangeReq->CompleteL(CContextConfigChangeReq::EDrop);
			delete TelServer()->iContextConfigChangeReq;
			TelServer()->iContextConfigChangeReq=NULL;
			}
		aMessage.Complete(KErrNone);
		return;
	case EDummyEtelTriggerContextConfigRestore:
		if(TelServer()->iContextConfigChangeReq)
			{
			TelServer()->iContextConfigChangeReq->CompleteL(CContextConfigChangeReq::ERestore);
			delete TelServer()->iContextConfigChangeReq;
			TelServer()->iContextConfigChangeReq=NULL;
			}
		aMessage.Complete(KErrNone);
		return;
	case EDummyEtelTriggerContextStatusChange:
		if(TelServer()->iContextStatusChangeReq)
			{
			TelServer()->iContextStatusChangeReq->CompleteL();
			delete TelServer()->iContextStatusChangeReq;
			TelServer()->iContextStatusChangeReq=NULL;
			}
		aMessage.Complete(KErrNone);
		return;
	case EDummyEtelTriggerQoSConfigDrop:
		if(TelServer()->iQoSConfigChangeReq)
			{
			TelServer()->iQoSConfigChangeReq->CompleteL(CQoSConfigChangeReq::EDrop);
			delete TelServer()->iQoSConfigChangeReq;
			TelServer()->iQoSConfigChangeReq=NULL;
			}
		aMessage.Complete(KErrNone);
		return;
	case EDummyEtelTriggerQoSConfigRestore:
		if(TelServer()->iQoSConfigChangeReq)
			{
			TelServer()->iQoSConfigChangeReq->CompleteL(CQoSConfigChangeReq::ERestore);
			delete TelServer()->iQoSConfigChangeReq;
			TelServer()->iQoSConfigChangeReq=NULL;
			}
		aMessage.Complete(KErrNone);
		return;
	case EDummyEtelSetMode:
		TelServer()->iMode = (RTelServer::TMobilePhoneNetworkMode)aMessage.Int0();
		aMessage.Complete(KErrNone);
		return;
	default:
		;
		}

	// Closing
	//CTelObject* theObj=CObjectFromHandle(aMessage.Int3());
	TInt subSessionId = aMessage.Int3();

 	switch (aMessage.Function())
		{
	case EEtelClose:
		{
		//if (theObj==NULL)
		//  aMessage.Complete(KErrBadHandle);
		//else
		//	{

		//	theObj->CloseSubSessionPreProcessing(this,aMessage.Int3());
		//	CTelObject* theObject=REINTERPRET_CAST(CTelObject*,theObj->Owner());
		//	iObjectIx->Remove(Message().Int3());
		//	if(theObject)
		//		theObject->TelObjectClose();
		//	Message().Complete(KErrNone);
		//	}
		//================================
		// JGG test code
		if( (subSessionId==KPhoneId)||(subSessionId==KPacketServiceId)||
			(subSessionId==KPacketContextId)||(subSessionId==KPacketQoSId)||
			(subSessionId==KCallId) ||(subSessionId==KLineId))
			{
			iOpenSubSessionCount--;
			aMessage.Complete(KErrNone);
			}
		else
			{
			aMessage.Complete(KErrBadHandle);
			}
		return;
		}
	default:
		//if (theObj==NULL)
		//	{
		//	PanicClient(EEtelPanicBadPhoneHandle,aMessage);
		//	aMessage.Complete(KErrBadHandle);
		//	return;
		//	}
		//theObj->GeneralReq(aMessage,this,NULL);
		//=======================
		// JGG test code
		if( !((subSessionId==KPhoneId)||(subSessionId==KPacketServiceId)||
			(subSessionId==KPacketContextId)||(subSessionId==KPacketQoSId)||
			(subSessionId==KCallId)||(subSessionId==KLineId)) )
			{
			aMessage.Complete(KErrBadHandle);
			return;
			}
		SimulateSubSessionFunctionsL(aMessage);
		//=====================
		return;
		}
	}

CTelObject* CTelSession::CObjectFromHandle(TUint aHandle) const
//
// Return an Object Pointer for Given Handle
//
	{
	return REINTERPRET_CAST(CTelObject*,iObjectIx->At(aHandle));
	}

CTelServer* CTelSession::TelServer() const
	{
	return Server();
	}

void CTelSession::EnumeratePhonesL()
//
// get number of phone currently loaded
//
	{
	TPckgBuf<TInt> num;
	//num()=iPhoneManager->EnumeratePhones();
	//======================
	// JGG test code
	num()=NumberOfPhones();
	//================
	const RMessage2& m=Message();
	m.WriteL(0,num);
	m.Complete(KErrNone);
	}

void CTelSession::GetTsyNameByPhone(const TInt aIndexOfPhone) 
//
// get the TSY name a particular phone belongs to
//
	{
/*	TName tsyName;
	TInt ret=KErrArgument;
	
	if ((ret=iPhoneManager->GetTsyName(aIndexOfPhone,tsyName))==KErrNone)
		{
		Write(Message().Ptr1(),tsyName);
		}
	Message().Complete(ret); */
	if(TelServer()->iTestNumber == 607)
		{
		Message().Complete(KErrGeneral);
		}
	else if(TelServer()->iTestNumber == 608)
		{
		TInt ret=KErrArgument;
		if(aIndexOfPhone < NumberOfPhones())
			{
			ret=KErrNone;
			Write(1,_L("FAKETSY"));
			}
		Message().Complete(ret);
		}
	else if(TelServer()->iTestNumber == 609)
		{
		TInt ret=KErrArgument;
		if(aIndexOfPhone < NumberOfPhones())
			{
			ret=KErrNone;
			Write(1,_L("NAME.TSY"));
			}
		Message().Complete(ret);
		}
	// Test cases from TS_CsdAgx start from 10000
	else if(TelServer()->iTestNumber == 10203)
		{
		// Return just KErrGeneral
		Message().Complete(KErrGeneral);
		}
	else if(TelServer()->iTestNumber == 10204)
		{
		// Return always wrong name, FAKETSY
		if(aIndexOfPhone < NumberOfPhones())
			{
			Write(1,_L("FAKETSY"));
			}
		Message().Complete(KErrNone);
		}
	else if(TelServer()->iTestNumber == 10205)
		{
		// Return TSY name with .TSY
		if(aIndexOfPhone < NumberOfPhones())
			{
			Write(1,_L("NAME.TSY"));
			}
		Message().Complete(KErrNone);
		}
	else if(TelServer()->iTestNumber == 10215)
		{
		// Return access denied
		Message().Complete(KErrAccessDenied);
		}
	else if( (TelServer()->iTestNumber == 20104)&&(iSessionNumber==2))
		{
		// Return just KErrGeneral
		Message().Complete(KErrGeneral);
		}
	else if( (TelServer()->iTestNumber == 20105)&&(iSessionNumber==2))
		{
		// Return always wrong name, FAKETSY
		if(aIndexOfPhone < NumberOfPhones())
			{
			Write(1,_L("FAKETSY"));
			}
		Message().Complete(KErrNone);
		}
	else if( (TelServer()->iTestNumber == 20106)&&(iSessionNumber==2))
		{
		if(aIndexOfPhone < NumberOfPhones())
			{
			Write(1,KTsyName);
			}
		Message().Complete(KErrNone);
		}
	else
		{
		TInt ret=KErrArgument;
		if(aIndexOfPhone < NumberOfPhones())
			{
			ret=KErrNone;
			Write(1,KTsyName);
			}
		//===============================
		Message().Complete(ret);
		}
	}

void CTelSession::SetExtendedErrorGranularity()
	{	
	if(TelServer()->iTestNumber == 603)
		{
		Message().Complete(KErrGeneral);
		}
	else
		{
		TPtr8 ptr(REINTERPRET_CAST(TText8*,&iErrorGranularity),
			sizeof(RTelServer::TErrorGranularity),sizeof(RTelServer::TErrorGranularity));
		// JGG may need test code here to return error in some tests
		Read(0,ptr);
		Message().Complete(KErrNone);
		}
	}

TBool CTelSession::IsExpectingExtendedError() const
	{
	if (iErrorGranularity==RTelServer::EErrorExtended)
		return ETrue;
	return EFalse;
	}

void CTelSession::GetTsyVersionNumber() const
	{
	/*
	TFileName name;
	if (GetModuleName(name)!=KErrNone)
		return;
	TInt len=name.Locate('.');
	name.SetLength(len);	// strip off .TSY extension
	CPhoneFactoryBase* p = NULL;
	TRAPD(ret,p = iPhoneManager->OpenPhoneFactoryFromTsyL(name));
	if (ret==KErrNone)
		{
		TVersion v = p->TsyVersionNumber();
		TPtr8 ptr(REINTERPRET_CAST(TText8*,&v),sizeof(TVersion),sizeof(TVersion));
		Write(Message().Ptr1(),ptr);
		}
	*/
	// JGG - Don't think this will be called by the code under test
	Message().Complete(KErrNotSupported);
	}

TInt CTelSession::GetModuleName(TDes& aName) const
	{
	Read(0,aName);
	TInt r=aName.Locate('.');
	if (r==KErrNotFound)
		{
		if (aName.Length()>KMaxFileName-4)
			{
			PanicClient(EEtelPanicBadDescriptor,Message());
			return KErrGeneral;
			}
		aName.Append(TSY_EXTENSION);
		}
	aName.UpperCase();
	return KErrNone;
	}

void CTelSession::LoadPhoneModule()
//
//	Load a phone module by name specified in p[0]
//
	{
	TInt ret=KErrNotFound;
	if(TelServer()->iTestNumber == 602)
		{
		// Fail the load - note only do this on session number 3
		// as this should be the AGT
		ret=KErrGeneral;
		}
	// Test cases from TS_CsdAgx start from 10000
    else if(TelServer()->iTestNumber == 10201)
		{
		// This test case just fails with KErrGeneral
		ret=KErrGeneral;
		}
	else if( (TelServer()->iTestNumber == 20102)&&(iSessionNumber==2))
		{
		// This test case just fails with KErrGeneral
		ret=KErrGeneral;
		}
	else
		{
		TFileName name;
		if (GetModuleName(name)!=KErrNone)
			return;
		//CPhoneFactoryBase* s=NULL;
		//TRAPD(res,(s=iPhoneManager->LoadPhoneModuleL(name)));
		//if (res==KErrNone)
		//	{
		//	TRAP(res,iTsyModulesIx->AddL(s));
		//	}
		// JGG - new code for testing
		if(TelServer()->iTestNumber == 609)
			{
			if(name.Compare(_L("NAME.TSY"))==KErrNone)
				{
				iTsyLoadCount++;
				ret=KErrNone;
				}
			}
		else if(name.Compare(KTsyName)==KErrNone)
			{
			iTsyLoadCount++;
			ret=KErrNone;
			}
		}
	Message().Complete(ret);
	}

void CTelSession::ClosePhoneModule()
//
// Close a ETEL TSY module
//
	{
/*	
	TName name;
	Read(Message().Ptr0(),name);
	CPhoneFactoryBase* s=NULL;
	
	// strip ".tsy" from name if it is there
	TInt len=name.Locate('.');
	if (len!=KErrNotFound)
		name.SetLength(len);

	TRAPD(res,(s=iPhoneManager->OpenPhoneFactoryFromTsyL(name)));
	if (res==KErrNone)
		{
		TInt handle=iTsyModulesIx->At(s);
		if (handle==KErrNotFound)
			res=KErrNotFound;
		else
			{
			iTsyModulesIx->Remove(handle);
			}
		}
	iPhoneManager->RemoveDuplicatePhoneInfo(name);
	Message().Complete(res);
*/
	// JGG - new code for testing
	TInt ret=KErrNotFound;
	TFileName name;
	if (GetModuleName(name)!=KErrNone)
		return;
	
	if(TelServer()->iTestNumber == 609)
		{
		if(name.Compare(_L("NAME.TSY"))==KErrNone)
			{
			iTsyLoadCount--;
			ret=KErrNone;
			}
		}
	// This needs to fail only for GenConn, not Agx
	else if(TelServer()->iTestNumber == 20134 && (iSessionNumber==2))
		{
		iTsyLoadCount--;
		ret = KErrGeneral;
		}
	else if(name.Compare(KTsyName)==KErrNone)
		{
		// Check has been loaded
		if(iTsyLoadCount>0)
			{
			iTsyLoadCount--;
			ret=KErrNone;
			}
		}
	Message().Complete(ret);
	}
#pragma warning(disable:4702) // Disable "C4702: unreachable code". 
							// Reason is KPriorityClientHeapMinSize is a const TInt but depends
							// on the sizes of CReqEntry and CBuffer. Currently, therefore, 
							// min is less than max. If code change causes sizes of CReqEntry or
							// CBuffer then min may become greater than max.

void CTelSession::SetPriorityClient()
//
// Check to see if there is already a priority client if there is complete
// Else set as priority client and allocate memory
//
	{
/*	TInt ret=Server()->SetPriorityClient(this);
	if (ret==KErrNone)
		{
		TInt min=Max(KMinHeapSize,KPriorityClientHeapMinSize);
		TInt max=KPriorityClientHeapMaxSize;
		if (min>max)			
			max=2*min;

		iPriorityClientHeap=UserHeap::ChunkHeap(NULL,min,max);
		if (iPriorityClientHeap==NULL)
			ret=KErrNoMemory;
		else
			ret=KErrNone;
		}
	Message().Complete(ret);
*/
	}
#pragma warning(default:4702) // Turn it back on.

void CTelSession::UnsetPriorityClient()
//
// If this is the priority client then remove it and deallocate memory
//
	{
/*	TInt ret=Server()->RemovePriorityClient(this);
	if (ret==KErrNone)
		{
		iPriorityClientHeap->Close();
		iPriorityClientHeap=NULL;
		} */
	}

RHeap* CTelSession::PriorityClientHeap(TInt /*aReq*/) const
//
// Only give the priority client heap if this is a priority client request
// If this is not the priority client iPriorityClientHeap will be NULL
//
	{
//	if (IsPriorityClientReq(aReq))
//		return iPriorityClientHeap;
	return NULL;
	}

TBool CTelSession::IsPriorityClientReq(TInt /*aReq*/) const
//
// Check whether aReq is a priority client request
//
	{
//	if (aReq&KPriorityClientReq)
//		return ETrue;
	return EFalse;
	}

TBool CTelSession::IsUnicodeReq(TInt aReq) const
//
// Check whether aReq is involves passing unicode data
//
	{
	if (aReq&KUnicodeReq)
		return ETrue;
	return EFalse;
	}

void CTelSession::GenerateName(TDes& aName)
//
// Append a Default Name to 'aName'
//
	{
	aName.Append(ETEL_NAME);
	TUint32 num=Server()->Count()<<16;
	num|=iNameIndex++;
	aName.AppendNum(num,EHex);
	}

void CTelSession::CheckAndAppendNewName(TDes& aName)
//
// Check if name is empty then fill with Etel default name
//
	{
	if (aName.Length()==0) // client does not give a name - TSY will generate one
		return;
	if (aName.Find(DOUBLE_COLON)== KErrNotFound) // it's a genuine name
		return;
	// ready to parse the name ...
	TFullName fullName=aName;
	aName.SetLength(0);
	TPtrC remaining(fullName);
	TPtrC name(NULL,0); // store individual name NULL if no name
	TInt len=0;
	while( (len=remaining.Find(DOUBLE_COLON))!=KErrNotFound)
		{
		name.Set(remaining.Left(len));
		remaining.Set(remaining.Right(remaining.Length()-len-KLengthOfDoubleColon));
		if (name.Length()!=0) 
			{
			aName.Append(name);
			aName.Append(DOUBLE_COLON);
			}
		
		if (remaining.Find(DOUBLE_COLON)==KErrNotFound)
			{
			if (remaining.Length())
				aName.Append(remaining);
			}
		}
	}

void CTelSession::NewTelObject(TTelObjectOpenSource /*aSource*/)
//
// Handle New Line Request
//
	{
/*
	CPhoneFactoryBase* phoneFactory=NULL ;
	CTelObject* newObject=NULL;
	TFullName fullName;
	Read(Message().Ptr0(),fullName); // Should leave?
	CheckAndAppendNewName(fullName);
	TFullName formName;

	if(aSource==ETelObjectOpenSourceSubSession || aSource==ETelObjectOpenByNameSourceSubSession)
		{
		newObject=CObjectFromHandle(Message().Int2());
		if (newObject)
			formName=newObject->FullName();
		else
			{
			PanicClient(EEtelPanicBadSubSessionHandle,Message());
			return;
			}
		}
	else
		{
		TPtrC remain(fullName);
		TPtrC phoneName(StripOutNextName(remain,fullName)); // get the phone name
		phoneFactory=iPhoneManager->PhoneFactoryObjectFromPhoneName(phoneName);
		if (phoneFactory==NULL)
			{
			Message().Complete(KErrNotFound);
			return;
			}
		formName=phoneFactory->FullName();
		}
	formName.Append(DOUBLE_COLON);
	formName.Append(fullName);
	fullName.Copy(formName);

	TPtrC remaining(fullName);
	TPtrC name(StripOutNextName(remaining,fullName));
	TRAPD(res,(phoneFactory=iPhoneManager->OpenPhoneFactoryFromTsyL(name)));
	if (res!=KErrNone)
		{
		if(phoneFactory)
			phoneFactory->Close();
		Message().Complete(res);
		return;
		}

	__ASSERT_ALWAYS(phoneFactory!=NULL,Fault(EEtelFaultBadPhoneFactoryPointer));
	phoneFactory->Open();
	// Open the Phone from the Phone Factory
	name.Set(StripOutNextName(remaining,fullName));

	TRAP(res,(newObject=iPhoneManager->OpenPhoneFromFactoryL(phoneFactory,name)));
	if(res!=KErrNone)
		{
		LOGTEXT2(_L8("Open Phone from Factory returned %d"),res);
		if(newObject)
			newObject->TelObjectClose();
		else
			phoneFactory->Close();
		Message().Complete(res);
		return;
		}

// Open Sub-session below the phone
	CTelObject* oldObject=newObject;
	__ASSERT_ALWAYS(oldObject!=NULL,Fault(EEtelFaultBadPhonePointer));
	while(remaining.Length()!=0)
		{
		name.Set(StripOutNextName(remaining,fullName)); */
//		TRAP(res,(newObject=iPhoneManager->OpenSubSessionObjectByNameL(newObject,name/*,aSource*/)));
/*		if(res!=KErrNone)
			{
			if(newObject)
				newObject->TelObjectClose();
			else
				oldObject->TelObjectClose();
			Message().Complete(res);
			return;
			}
		oldObject=newObject;
		}
	
	if (fullName.Right(2)==DOUBLE_COLON)	
		{
		__ASSERT_ALWAYS((aSource==ETelObjectOpenSourceSession ||  
						 aSource==ETelObjectOpenSourceSubSession),PanicClient(EEtelPanicBadName,Message()));

		if (fullName.Mid(fullName.Length()-3,2)!=DOUBLE_COLON)
		// if there are three colons at the end, server will have already asked TSY to open
		// create the object and generate a new name for it.
			{
			TName aNewName;
			TRAP(res,(newObject=iPhoneManager->OpenSubSessionObjectL(newObject,aNewName)));
			if (res)
				{	
				if(newObject)
					newObject->TelObjectClose();
				else
					oldObject->TelObjectClose();
				Message().Complete(res);
				return;
				}
			Write(Message().Ptr1(),aNewName);
			}
		else
			Write(Message().Ptr1(),newObject->Name());
		}
	__ASSERT_ALWAYS(newObject!=NULL,Fault(EEtelFaultBadTelObjectPointer));
	TInt handle=0;
	TRAP(res,handle=iObjectIx->AddL(newObject));
	if(res!=KErrNone)
		{
		if(newObject)
			newObject->TelObjectClose();
		else
			oldObject->TelObjectClose();
		Message().Complete(res);
		return;
		}
	newObject->OpenPostProcessing(this,handle);
	TPtrC8 pH(REINTERPRET_CAST(TUint8*,&handle),sizeof(TInt));
	Write(Message().Ptr3(),pH);
	Message().Complete(KErrNone);
*/
	}


TPtrC CTelSession::StripOutNextName(TPtrC& aRemainingName,const TFullName& aFullName)
//
// Return Descriptor of the the Next Name
//
	{
	TInt flen=aRemainingName.Find(DOUBLE_COLON);
	if (flen==KErrNotFound)	 // it is the last name in string
		{
		aRemainingName.Set(NULL,0);
		return aFullName;
		}
	else
		{
		aRemainingName.Set(aRemainingName.Right(aRemainingName.Length()-flen-KLengthOfDoubleColon));
		return aFullName.Left(aFullName.Length()-aRemainingName.Length()-KLengthOfDoubleColon);
		}
	}

void CTelSession::PhoneInfo(TInt aIndex)
//
//	Get phone info by index
//
	{
	RTelServer::TPhoneInfo phone;
	phone.iNetworkType=RTelServer::ENetworkTypeMobileDigital;
	phone.iName=KPhoneName;
	phone.iNumberOfLines=0;
	phone.iExtensions=KETelExtMultimodeV1;

	TInt ret=KErrArgument;
	//if((ret=iPhoneManager->GetPhoneInfo(aIndex,phone))==KErrNone)
	//	{
	//	CPhoneFactoryBase* fact = iPhoneManager->PhoneFactoryObjectFromPhoneIndex(aIndex);
		
	//	iPhoneManager->ConvertPhoneNameFromOriginal(fact->Name(),phone.iName);	
			// replace phone name with munged name if it has been munged

	//	TPckgC<RTelServer::TPhoneInfo> p(phone);
	//	Write(Message().Ptr0(),p);
	//	}
	//==============================
	// Test cases from TS_CsdAgx start from 10000
	if(TelServer()->iTestNumber == 10206)
		{
		// Return KErrNotSupported for this test case
		ret = KErrNotSupported;
		}
	else if( (TelServer()->iTestNumber == 20107)&&(iSessionNumber==2))
		{
		// Return KErrNotSupported for this test case
		ret = KErrGeneral;
		}
	// JGG test code 
	// Ensure the client is not asking for an invalid phone
	else if(aIndex < NumberOfPhones())
		{
		ret=KErrNone;
		TPckgC<RTelServer::TPhoneInfo> p(phone);
		Write(0,p);
		}
	//===============================
	Message().Complete(ret);
	}

void CTelSession::IsSupportedByTsy()
	{
/*	TFileName name;
	if (GetModuleName(name)!=KErrNone)
		return;
	TInt len=name.Locate('.');
	name.SetLength(len);	// strip off .TSY extension
	CPhoneFactoryBase* phoneFactory=NULL;
	TRAPD(res,(phoneFactory=iPhoneManager->OpenPhoneFactoryFromTsyL(name)));
	if (res==KErrNone)
		{
		__ASSERT_ALWAYS(phoneFactory,Fault(EEtelFaultBadPhoneFactoryPointer));
		TInt mixin;
		TPtr8 ptr1(REINTERPRET_CAST(TText8*,CONST_CAST(TInt*,&mixin)),sizeof(TInt),sizeof(TInt));
		Read(Message().Ptr1(),ptr1);
		TBool result = phoneFactory->IsSupported(mixin);
		TPtr8 ptr2(REINTERPRET_CAST(TText8*,&result),sizeof(TBool),sizeof(TBool));
		Write(Message().Ptr2(),ptr2);
		}
	Message().Complete(res); */
	}

TInt CTelSession::Write(TInt aIndex,const TDesC8& aDes,TInt aOffset) const
//
// Write and kill the client if it leaves.
//
	{
	TRAPD(ret,Message().WriteL(aIndex,aDes,aOffset);)
	if (ret!=KErrNone)
		PanicClient(EEtelPanicBadDescriptor,Message());
	return ret;
	}

TInt CTelSession::Read(TInt aIndex,TDes8 &aDes,TInt aOffset) const
//
// Write and kill the client if it leaves.
//
	{
	TRAPD(ret,Message().ReadL(aIndex,aDes,aOffset);)
	if (ret!=KErrNone)
		PanicClient(EEtelPanicBadDescriptor,Message());
	return ret;
	}

TInt CTelSession::Write(TInt aIndex,const TDesC16& aDes,TInt aOffset) const
//
// Write and kill the client if it leaves.
//
	{
	TRAPD(ret,Message().WriteL(aIndex,aDes,aOffset);)
	if (ret!=KErrNone)
		PanicClient(EEtelPanicBadDescriptor,Message());
	return ret;
	}

TInt CTelSession::Read(TInt aIndex,TDes16 &aDes,TInt aOffset) const
//
// Write and kill the client if it leaves.
//
	{
	TRAPD(ret,Message().ReadL(aIndex,aDes,aOffset);)
	if (ret!=KErrNone)
		PanicClient(EEtelPanicBadDescriptor,Message());
	return ret;
	}


//=============================================================================
// JGG Test functions



void CTelSession::OpenFromSession()
	{
	// Should just be an RPhone or an RCall being opened
	TInt ret=KErrNotSupported;
	TInt subSessionHandle=0;
	// Check name is same as one returned in GetPhoneInfo()
	TFullName fullName;
	Read(0,fullName);
	TInt res = fullName.Compare(KPhoneName);
	if(res==KErrNone)
		{
		// Is phone
		if(TelServer()->iTestNumber == 604)
			{
			Message().Complete(KErrGeneral);
			return;
			}
		// Test cases from TS_CsdAgx start from 10000
		else if( (TelServer()->iTestNumber == 20108)&&(iSessionNumber==2))
			{
			// Return KErrGeneral for this test case
			Message().Complete(KErrGeneral);
			return;
			}
		ret=KErrNone;
		iOpenSubSessionCount++;
		subSessionHandle=KPhoneId;
		}
	else if(res>0) // Check if is call
		{
		// 1st part of name started with the phone name assume that the
		// rest of the name is "::<line name>::" and assume is a call
		ret=KErrNone;
		iOpenSubSessionCount++;
		subSessionHandle=KCallId;
		// Test cases from TS_CsdAgt start from 10000
		if(TelServer()->iTestNumber == 10207)
			{
			// As the code above is "common", and this CSD case should fail,
			// decrement the count here:
			iOpenSubSessionCount--;
			// Return KErrGeneral for this test case
			ret = KErrGeneral;
			}
		}
	TPtrC8 pH(REINTERPRET_CAST(TUint8*,&subSessionHandle),sizeof(TInt));
	Write(3,pH);
	Message().Complete(ret);
	}

void CTelSession::OpenFromSubSession()
	{
	// Could be an RPacketService, RPacketContext, RPacketQoS
	TInt ret=KErrNotSupported;
	TInt subSessionHandle=0;
	// Need to figure out what is being asked for
	// The name should indicate what type of object should be created
	TFullName fullName;
	Read(0,fullName);
	TInt parentId = Message().Int2(); // Contains the parent sub session handle this gives a clue as to what is being asked for

	if(fullName.Compare(_L("PACKET_NAME"))==KErrNone)
		{
		// Creating a PacketService object
		// Check the parent is a Phone
		if(parentId == KPhoneId)
			{
			if(TelServer()->iTestNumber == 605)
				{
				Message().Complete(KErrGeneral);
				return;
				}
			// Test cases from TS_GenConn start from 20000
			else if( ( (TelServer()->iTestNumber == 20116) ||
					   (TelServer()->iTestNumber == 20117) )&&(iSessionNumber==2))
				{
				// Return KErrGeneral for this test case
				Message().Complete(KErrNotSupported);
				return;
				}
			// Test cases for TS_GenConn from 20201 to 20215
			else if(TelServer()->iTestNumber >= 20201 &&
					TelServer()->iTestNumber <= 20215)
				{
				// Return KErrNotSupported for this test case
				Message().Complete(KErrNotSupported);
				return;
				}
			// TS_GenConn from 20247 to 20250
			else if(TelServer()->iTestNumber >= 20247 &&
					TelServer()->iTestNumber <= 20250)
				{
				// Return KErrNotSupported for this test case
				Message().Complete(KErrNotSupported);
				return;
				}
			subSessionHandle=KPacketServiceId;
			iOpenSubSessionCount++;
			ret=KErrNone;
			}
		}
	else if(fullName.Compare(_L("::"))==KErrNone)
		{
		// Could be RPacketContext or RPacketQoS, the parent sub session
		// will allow us to determine which
		switch(parentId)
			{
		case KPacketServiceId:
			// Creating a PacketContext
			if(TelServer()->iTestNumber==801)
				{
				Message().Complete(KErrGeneral);
				return;
				}
			subSessionHandle=KPacketContextId;
			iOpenSubSessionCount++;
			ret=KErrNone;
			break;
		case KPacketContextId:
			// Creating a PacketQoS
			if(TelServer()->iTestNumber==901)
				{
				Message().Complete(KErrGeneral);
				return;
				}
			subSessionHandle=KPacketQoSId;
			iOpenSubSessionCount++;
			ret=KErrNone;
			break;
		default:
			// Error
			break;
			}
		}
	else if (fullName.Compare(KLineName) == KErrNone)	
	        {
		subSessionHandle=KLineId;
		iOpenSubSessionCount++;
		ret=KErrNone;
	        }
	// Complete client
	TPtrC8 pH(REINTERPRET_CAST(TUint8*,&subSessionHandle),sizeof(TInt));
	Write(3,pH);
	Message().Complete(ret);
	}

// For now hard code the number of phones in future may be test dependent
static const TInt KNumberOfPhones=1;
TInt CTelSession::NumberOfPhones()
	{
	if(TelServer()->iTestNumber == 606)
		{
		return 0;
		}
	// Test cases from TS_CsdAgt start from 10000
	else if(TelServer()->iTestNumber == 10202)
		{
		// Return zero count for this test case
		return 0;
		}
	else if( (TelServer()->iTestNumber == 20103)&&(iSessionNumber==2))
		{
		// Return zero count for this test case
		return 0;
		}
	else
		{
		return KNumberOfPhones;
		}
	}

void CTelSession::SimulateSubSessionFunctionsL(const RMessage2 &aMessage)
	{
	// Figure out which subsession type is making the request
	TInt subSessionId = aMessage.Int3();
	switch(subSessionId)
		{
	case KPhoneId:
		SimulatePhoneFunctions(aMessage);
		break;
	case KPacketServiceId:
		SimulatePacketServiceFunctionsL(aMessage);
		break;
	case KPacketContextId:
		SimulatePacketContextFunctionsL(aMessage);
		break;
	case KPacketQoSId:
		SimulatePacketQoSFunctionsL(aMessage);
		break;
	case KCallId:
		SimulateCallFunctions(aMessage);
		break;
	case KLineId:
		SimulatePhoneFunctions(aMessage);
		break;
	default:
		Message().Complete(KErrNotSupported);
		break;
		}
	}

void CTelSession::SimulatePhoneFunctions(const RMessage2 &aMessage)
	{
	switch (aMessage.Function())
		{
	case EEtelPhoneGetCaps:
		{
		// Test cases from TS_GenConn start from 20000
		if( (TelServer()->iTestNumber == 20109)&&(iSessionNumber==2))
			{
			// Return KErrGeneral for this test case
			Message().Complete(KErrGeneral);
			}
		else if( ( (TelServer()->iTestNumber == 20110) || 
				   (TelServer()->iTestNumber == 20111) )&&(iSessionNumber==2) )
			{
			// Support only voice
			RPhone::TCaps caps;
			caps.iFlags = RPhone::KCapsVoice;
			TPtrC8 pH(REINTERPRET_CAST(TText8*,&caps),sizeof(RPhone::TCaps));
			Write(0,pH);
			Message().Complete(KErrNone);
			}
		else if (TelServer()->iTestNumber == 20226 ||
				TelServer()->iTestNumber == 20241)
			{
			// Support only voice
			RPhone::TCaps caps;
			caps.iFlags = RPhone::KCapsVoice;
			TPtrC8 pH(REINTERPRET_CAST(TText8*,&caps),sizeof(RPhone::TCaps));
			Write(0,pH);
			Message().Complete(KErrNone);
			}
		else
			{
			RPhone::TCaps caps;
			caps.iFlags = RPhone::KCapsData|RPhone::KCapsVoice;
			TPtrC8 pH(REINTERPRET_CAST(TText8*,&caps),sizeof(RPhone::TCaps));
			Write(0,pH);
			Message().Complete(KErrNone);
			}
		break;
		}
	case EMobilePhoneGetCurrentMode:
		{
		// Test cases from TS_GenConn start from 20000
		if( (TelServer()->iTestNumber == 20112)&&(iSessionNumber==2))
			{
			// Return KErrGeneral for this test case
			Message().Complete(KErrNotSupported);
			}
		else if( (TelServer()->iTestNumber == 20113)&&(iSessionNumber==2))
			{
			RTelServer::TMobilePhoneNetworkMode mode = RTelServer::ENetworkModeUnknown;
			TPtrC8 pH(REINTERPRET_CAST(TText8*,&mode),sizeof(RTelServer::TMobilePhoneNetworkMode));
			Write(0,pH);
			Message().Complete(KErrNone);
			}
		else if( ( (TelServer()->iTestNumber == 20114) ||
				   (TelServer()->iTestNumber == 20115) )&&(iSessionNumber==2))
			{
			RTelServer::TMobilePhoneNetworkMode mode = RTelServer::ENetworkModeUnregistered;
			TPtrC8 pH(REINTERPRET_CAST(TText8*,&mode),sizeof(RTelServer::TMobilePhoneNetworkMode));
			Write(0,pH);
			Message().Complete(KErrNone);
			}
		// GSM modes for TS_GenConn
		else if(TelServer()->iTestNumber == 20262)
			{
			RTelServer::TMobilePhoneNetworkMode mode = RTelServer::ENetworkModeGsm;
			TPtrC8 pH(REINTERPRET_CAST(TText8*,&mode),sizeof(RTelServer::TMobilePhoneNetworkMode));
			Write(0,pH);
			Message().Complete(KErrNone);
			}
		else
			{
			RTelServer::TMobilePhoneNetworkMode mode;
			mode=TelServer()->iMode;
			TPtrC8 pH(REINTERPRET_CAST(TText8*,&mode),sizeof(RTelServer::TMobilePhoneNetworkMode));
			Write(0,pH);
			Message().Complete(KErrNone);
			}
		break;
		}
	case EMobilePhoneGetMultimodeCaps:
		{
		TUint32 mmCaps=RMobilePhone::KCapsGprsSupported; // JGG this will change depending on the test#
		TPtrC8 pH(REINTERPRET_CAST(TText8*,&mmCaps),sizeof(TUint32));
		Write(0,pH);
		Message().Complete(KErrNone);
		break;
		}
	case EEtelPhoneGetStatus:
		{
		RPhone::TStatus status;
		
		if(TelServer()->iTestNumber == 611)		
			status.iModemDetected=RPhone::EDetectedNotPresent;
		else if(TelServer()->iTestNumber == 612)
			status.iModemDetected=RPhone::EDetectedUnknown;
		else status.iModemDetected=RPhone::EDetectedPresent;
		
		TPtrC8 pH(REINTERPRET_CAST(TText8*,&status),sizeof(RPhone::TStatus));
		Write(0,pH);

		if(TelServer()->iTestNumber == 610)
			Message().Complete(KErrGeneral);
		else Message().Complete(KErrNone);
		break;
		}
	case EMobilePhoneGetSignalStrength:
		{
		// TSY does not support Signal strength, ie =0
		TInt32 signalStrength=0;
		TPtrC8 pH(REINTERPRET_CAST(TText8*,&signalStrength),sizeof(TInt32));
		Write(0,pH);
		Message().Complete(KErrNone);
		break;
		}
	
	case EEtelPhoneEnumerateLines:
	        {
		TInt32 nbOfLines = 1;
		TPtrC8 pH(REINTERPRET_CAST(TText8*,&nbOfLines),sizeof(TInt32));
		Write(0,pH);
		Message().Complete(KErrNone);
		break;
	        }
	        
	case EEtelPhoneGetLineInfo:
	        {
	        TLineInfoIndex lineInfoIdx;
	        lineInfoIdx.iInfo.iStatus = RCall::EStatusIdle;
		lineInfoIdx.iInfo.iLineCapsFlags = RLine::KCapsData;
		lineInfoIdx.iInfo.iName = KLineName;
		TPtrC8 pH(REINTERPRET_CAST(TText8*,&lineInfoIdx),sizeof(RPhone::TLineInfo));
		Write(0,pH);
	        Message().Complete(KErrNone);
		break;
		}
	
	case EEtelLineGetCaps:
	        {
	        RLine::TCaps caps;
		caps.iFlags = RLine::KCapsData;
		TPtrC8 pH(REINTERPRET_CAST(TText8*,&caps),sizeof(RLine::TCaps));
		Write(0,pH);
		Message().Complete(KErrNone);
	        break;
	        }
	
	
	default:
		Message().Complete(KErrNotSupported);
		break;
		}
	}

void CTelSession::SimulatePacketServiceFunctionsL(const RMessage2 &aMessage)
	{
	switch (aMessage.Function())
		{
	case EPacketGetMSClass:
		{
		if( (TelServer()->iTestNumber == 20118)&&(iSessionNumber==2))
			{
			Message().Complete(KErrNotSupported);
			}
		else if( (TelServer()->iTestNumber == 20119)&&(iSessionNumber==2))
			{
			RPacketService::TMSClass currentClass=RPacketService::EMSClassDualMode;
			RPacketService::TMSClass maxClass=RPacketService::EMSClassSuspensionRequired;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&currentClass),sizeof(RPacketService::TMSClass));
			Write(0,ptr1);
			TPtrC8 ptr2(REINTERPRET_CAST(TText8*,&maxClass),sizeof(RPacketService::TMSClass));
			Write(2,ptr2);
			Message().Complete(KErrNone);
			}
		else if( (TelServer()->iTestNumber == 20120)&&(iSessionNumber==2))
			{
			RPacketService::TMSClass currentClass=RPacketService::EMSClassSuspensionRequired;
			RPacketService::TMSClass maxClass=RPacketService::EMSClassSuspensionRequired;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&currentClass),sizeof(RPacketService::TMSClass));
			Write(0,ptr1);
			TPtrC8 ptr2(REINTERPRET_CAST(TText8*,&maxClass),sizeof(RPacketService::TMSClass));
			Write(2,ptr2);
			Message().Complete(KErrNone);
			}
		else if( (TelServer()->iTestNumber == 20121)&&(iSessionNumber==2))
			{
			RPacketService::TMSClass currentClass=RPacketService::EMSClassPacketSwitchedOnly;
			RPacketService::TMSClass maxClass=RPacketService::EMSClassSuspensionRequired;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&currentClass),sizeof(RPacketService::TMSClass));
			Write(0,ptr1);
			TPtrC8 ptr2(REINTERPRET_CAST(TText8*,&maxClass),sizeof(RPacketService::TMSClass));
			Write(2,ptr2);
			Message().Complete(KErrNone);
			}
		else if( (TelServer()->iTestNumber == 20122)&&(iSessionNumber==2))
			{
			RPacketService::TMSClass currentClass=RPacketService::EMSClassCircuitSwitchedOnly;
			RPacketService::TMSClass maxClass=RPacketService::EMSClassSuspensionRequired;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&currentClass),sizeof(RPacketService::TMSClass));
			Write(0,ptr1);
			TPtrC8 ptr2(REINTERPRET_CAST(TText8*,&maxClass),sizeof(RPacketService::TMSClass));
			Write(2,ptr2);
			Message().Complete(KErrNone);
			}
		else if( (TelServer()->iTestNumber == 20123)&&(iSessionNumber==2))
			{
			RPacketService::TMSClass currentClass=RPacketService::EMSClassUnknown;
			RPacketService::TMSClass maxClass=RPacketService::EMSClassSuspensionRequired;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&currentClass),sizeof(RPacketService::TMSClass));
			Write(0,ptr1);
			TPtrC8 ptr2(REINTERPRET_CAST(TText8*,&maxClass),sizeof(RPacketService::TMSClass));
			Write(2,ptr2);
			Message().Complete(KErrNone);
			}
		else if( (TelServer()->iTestNumber == 20133)&&(iSessionNumber==2))
			{
			RPacketService::TMSClass currentClass=RPacketService::EMSClassAlternateMode;
			RPacketService::TMSClass maxClass=RPacketService::EMSClassSuspensionRequired;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&currentClass),sizeof(RPacketService::TMSClass));
			Write(0,ptr1);
			TPtrC8 ptr2(REINTERPRET_CAST(TText8*,&maxClass),sizeof(RPacketService::TMSClass));
			Write(2,ptr2);
			Message().Complete(KErrNone);
			}
		else
			{
			RPacketService::TMSClass currentClass;
			RPacketService::TMSClass maxClass;
			currentClass=RPacketService::EMSClassSuspensionRequired;// JGG this will change depending on the test#
			maxClass=RPacketService::EMSClassSuspensionRequired;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&currentClass),sizeof(RPacketService::TMSClass));
			Write(0,ptr1);
			TPtrC8 ptr2(REINTERPRET_CAST(TText8*,&maxClass),sizeof(RPacketService::TMSClass));
			Write(2,ptr2);
			Message().Complete(KErrNone);
			}
		}
		break;
	case EPacketGetNtwkRegStatus:
		{
		switch(TelServer()->iTestNumber)
			{
			case 701:
				{
				Message().Complete(KErrNotSupported);
				break;
				}
			case 702:
				{
				Message().Complete(KErrGeneral);
				break;
				}
			case 704:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = RPacketService::ERegisteredRoaming;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 705:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = RPacketService::ENotRegisteredButAvailable;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 706:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = RPacketService::EUnknown;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 707:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = RPacketService::ERegistrationDenied;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 708:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = RPacketService::ENotRegisteredNotSearching;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 709:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = RPacketService::ENotRegisteredAndNotAvailable;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 710:
				{
				RPacketService::TRegistrationStatus regStatus;
				if(iPacketServiceGetNtwkRegStatusCallCount==0)
					{
					regStatus = RPacketService::ENotRegisteredSearching;// JGG this will change depending on the test#
					}
				else
					{
					regStatus = RPacketService::ERegisteredOnHomeNetwork;// JGG this will change depending on the test#
					}
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 711:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = (RPacketService::TRegistrationStatus)9999;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			case 703:
			default:
				{
				RPacketService::TRegistrationStatus regStatus;
				regStatus = RPacketService::ERegisteredOnHomeNetwork;// JGG this will change depending on the test#
				TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
				Write(0,ptr1);
				Message().Complete(KErrNone);
				}
				break;
			}
		iPacketServiceGetNtwkRegStatusCallCount++;
		}
		break;
	case EPacketGetStatus:
		{
		if(TelServer()->iTestNumber == 20133)
			{
			RPacketService::TStatus status;
			status = RPacketService::EStatusUnattached;// JGG this will change depending on the test#
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&status),sizeof(RPacketService::TStatus));
			Write(0,ptr1);
			Message().Complete(KErrNone);
			}
		else
			{
			RPacketService::TStatus status;
			status = RPacketService::EStatusActive;// JGG this will change depending on the test#
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&status),sizeof(RPacketService::TStatus));
			Write(0,ptr1);
			Message().Complete(KErrNone);
			}
		}
		break;
	case EPacketGetStaticCaps:
		{
		TUint caps = KErrNone;
		switch(TelServer()->iTestNumber)
			{
		case 501:
		case 1403:
			// TSY does not support data transfer
			caps&=~RPacketService::KCapsGetDataTransferredSupported;
			break;
		case 502:
		case 503:
			// TSY does suppoert data transfer
			caps|=RPacketService::KCapsGetDataTransferredSupported;
			break;
		default:
			break;
			}
		Message().Complete(KErrNone);
		}
		break;
	case EPacketNotifyChangeOfNtwkRegStatus:
		{
		RPacketService::TRegistrationStatus regStatus;
		regStatus= RPacketService::ERegisteredRoaming;
		TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&regStatus),sizeof(RPacketService::TRegistrationStatus));
		Write(0,ptr1);
		Message().Complete(KErrNone);
		}
		break;
	default:
		Message().Complete(KErrNotSupported);
		break;
		}
	}

void CTelSession::SimulatePacketContextFunctionsL(const RMessage2 &aMessage)
	{
	switch (aMessage.Function())
		{
	case EPacketContextActivate:
		if(TelServer()->iTestNumber == 1002)
			{
			Message().Complete(KErrGeneral);
			return;
			}
		iContextStatus = RPacketContext::EStatusActive;
		Message().Complete(KErrNone);
		break;
	case EPacketContextDeactivate:
		iContextStatus = RPacketContext::EStatusInactive;
		Message().Complete(KErrNone);
		break;
	case EPacketContextDelete:
		Message().Complete(KErrNone);
		break;
	case EPacketContextGetDataVolumeTransferred:
		{
		RPacketContext::TDataVolume vol;
		vol.iBytesSent = 100000;
		vol.iOverflowCounterSent = 100;
		vol.iBytesReceived = 400000;
		vol.iOverflowCounterReceived = 400;
		switch(TelServer()->iTestNumber)
			{
		case 502:
			if(iPacketContextGetDataVolumeTransferredCallCount==0)
				{
				vol.iBytesSent = 451;
				vol.iBytesReceived = 452;
				}
			else if(iPacketContextGetDataVolumeTransferredCallCount==1)
				{
				vol.iBytesSent = 498;
				vol.iBytesReceived = 452;
				}
			break;
		case 503:
			vol.iBytesSent = 0;
			vol.iBytesReceived = 0;
			break;
		default:
			break;
			}
		iPacketContextGetDataVolumeTransferredCallCount++;
		TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&vol),sizeof(RPacketContext::TDataVolume));
		Write(0,ptr1);
		Message().Complete(KErrNone);
		}
		break;
	case EPacketContextGetLastErrorCause:
		{
		TInt error;
		error = KErrGsmMMNetworkFailure;
		TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&error),sizeof(TInt));
		Write(0,ptr1);
		Message().Complete(KErrNone);
		}
		break;
	case EPacketContextGetStatus:
		{
		if(TelServer()->iTestNumber == 1001)
			{
			RPacketContext::TContextStatus status;
			status = RPacketContext::EStatusActive;
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&status),sizeof(RPacketContext::TContextStatus));
			Write(0,ptr1);
			Message().Complete(KErrNone);
			return;
			}
		else if(TelServer()->iTestNumber == 1101)
			{
			RPacketContext::TContextStatus status;
			if(iPacketContextGetStatusCallCount < 2)
				{
				// return inactive twice
				status = RPacketContext::EStatusInactive;
				}
			else
				{
				status = RPacketContext::EStatusActive;
				}
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&status),sizeof(RPacketContext::TContextStatus));
			Write(0,ptr1);
			Message().Complete(KErrNone);
			iPacketContextGetStatusCallCount++;
			return;
			}
		else if(TelServer()->iTestNumber == 1201)
			{
			RPacketContext::TContextStatus status;
			if(iPacketContextGetStatusCallCount == 0)
				{
				status = RPacketContext::EStatusInactive;
				}
			else if(iPacketContextGetStatusCallCount == 1)
				{
				status = RPacketContext::EStatusActive;
				}
			else
				{
				status = RPacketContext::EStatusInactive;
				}
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&status),sizeof(RPacketContext::TContextStatus));
			Write(0,ptr1);
			Message().Complete(KErrNone);
			iPacketContextGetStatusCallCount++;
			return;
			}

		TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&iContextStatus),sizeof(RPacketContext::TContextStatus));
		Write(0,ptr1);
		Message().Complete(KErrNone);
		}
		break;
	case EPacketContextLoanCommPort:
		{
		if( (TelServer()->iTestNumber == 1003)||
			((TelServer()->iTestNumber == 1402)&&(iPacketContextLoanCommPortCallCount>0)) )
			{
			Message().Complete(KErrGeneral);
			return;
			}

		RCall::TCommPort dataPort;
		dataPort.iCsy=_L("ECUART");
		dataPort.iPort=_L("COMM::0");
		TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&dataPort),sizeof(RCall::TCommPort));
		Write(0,ptr1);
		Message().Complete(KErrNone);
		iPacketContextLoanCommPortCallCount++;
		}
		break;
	case EPacketContextNotifyConfigChanged:
		__ASSERT_DEBUG(TelServer()->iContextConfigChangeReq==NULL, User::Invariant());
		TelServer()->iContextConfigChangeReq = CContextConfigChangeReq::NewL(Message(),TelServer());
		break;
	case EPacketContextNotifyStatusChange:
		__ASSERT_DEBUG(TelServer()->iContextStatusChangeReq==NULL, User::Invariant());
		TelServer()->iContextStatusChangeReq = CContextStatusChangeReq::NewL(Message(),TelServer(),iPacketContextNotifyStatusChangeCallCount);
		iPacketContextNotifyStatusChangeCallCount++;
		break;
	case EPacketContextRecoverCommPort:
		Message().Complete(KErrNone);
		break;
	case EPacketContextSetConfig:
		{
		if(TelServer()->iMode == RTelServer::ENetworkModeGsm)
			{
			// GPRS
			TPtr8 ptr(REINTERPRET_CAST(TText8*,&(TelServer()->iGPRSContextConfig)),
				sizeof(RPacketContext::TContextConfigGPRS),sizeof(RPacketContext::TContextConfigGPRS));
			Read(0,ptr);
			}
		Message().Complete(KErrNone);
		}
		break;
	case EPacketContextNotifyStatusChangeCancel:
		if(TelServer()->iContextStatusChangeReq)
			{
			Message().Complete(KErrNone);
			TelServer()->iContextStatusChangeReq->Cancel();
			delete TelServer()->iContextStatusChangeReq;
			TelServer()->iContextStatusChangeReq = NULL;
			}
		else 
			{
			// No request outstanding
			Message().Complete(KErrCorrupt);
			}
		break;
	case EPacketContextNotifyConfigChangedCancel:
		
		if(TelServer()->iContextConfigChangeReq)
			{
			Message().Complete(KErrNone);
			TelServer()->iContextConfigChangeReq->Cancel();
			delete TelServer()->iContextConfigChangeReq;
			TelServer()->iContextConfigChangeReq = NULL;
			}
		else 
			{
			// No request outstanding
			Message().Complete(KErrCorrupt);
			}
		break;
	default:
		Message().Complete(KErrNotSupported);
		break;
		}
	}

void CTelSession::SimulatePacketQoSFunctionsL(const RMessage2 &aMessage)
	{
	switch (aMessage.Function())
		{
	case EPacketQoSSetProfileParams:
		if(TelServer()->iMode == RTelServer::ENetworkModeGsm)
			{
			// GPRS
			TPtr8 ptr(REINTERPRET_CAST(TText8*,&(TelServer()->iGPRSRequestedQoSConfig)),
				sizeof(RPacketQoS::TQoSGPRSRequested),sizeof(RPacketQoS::TQoSGPRSRequested));
			Read(0,ptr);
			}
		Message().Complete(KErrNone);
		break;
	case EPacketQoSNotifyProfileChanged:
		__ASSERT_DEBUG(TelServer()->iQoSConfigChangeReq==NULL, User::Invariant());
		TelServer()->iQoSConfigChangeReq = CQoSConfigChangeReq::NewL(Message(),TelServer());
		break;
	case EPacketQoSNotifyProfileChangedCancel:
		if(TelServer()->iQoSConfigChangeReq)
			{
			Message().Complete(KErrNone);
			TelServer()->iQoSConfigChangeReq->Cancel();
			delete TelServer()->iQoSConfigChangeReq;
			TelServer()->iQoSConfigChangeReq = NULL;
			}
		else 
			{
			// No request outstanding
			Message().Complete(KErrCorrupt);
			}
		break;
	default:
		Message().Complete(KErrNotSupported);
		break;
		}
	}

void CTelSession::SimulateCallFunctions(const RMessage2 &aMessage)
	{
	TInt ret=KErrNone;

	switch (aMessage.Function())
		{
	case EMobileCallGetMobileDataCallCaps:
		{
		if(TelServer()->iTestNumber == 10208)
			{
			// Return KErrGeneral for this test case
			ret = KErrGeneral;
			}
		else
			{
			// Create data call caps variables and read the caps package
			RMobileCall::TMobileCallDataCapsV1 dataCallCaps;
			TPckg<RMobileCall::TMobileCallDataCapsV1> capsPckg( dataCallCaps );
			Read(0,capsPckg);

			// Set the data call caps, currently everything is supported
			dataCallCaps.iSpeedCaps = RMobileCall::KCapsSpeedAutobauding
										+ RMobileCall::KCapsSpeed2400
										+ RMobileCall::KCapsSpeed4800
										+ RMobileCall::KCapsSpeed9600
										+ RMobileCall::KCapsSpeed14400
										+ RMobileCall::KCapsSpeed19200
										+ RMobileCall::KCapsSpeed28800
										+ RMobileCall::KCapsSpeed32000
										+ RMobileCall::KCapsSpeed33600
										+ RMobileCall::KCapsSpeed38400
										+ RMobileCall::KCapsSpeed43200
										+ RMobileCall::KCapsSpeed48000
										+ RMobileCall::KCapsSpeed56000
										+ RMobileCall::KCapsSpeed57600
										+ RMobileCall::KCapsSpeed64000;

			dataCallCaps.iProtocolCaps = (TUint32)(RMobileCall::KCapsProtocolV22bis
										+ RMobileCall::KCapsProtocolV32
										+ RMobileCall::KCapsProtocolV34
										+ RMobileCall::KCapsProtocolV110
										+ RMobileCall::KCapsProtocolV120
										+ RMobileCall::KCapsProtocolBitTransparent
										+ RMobileCall::KCapsProtocolX31FlagStuffing
										+ RMobileCall::KCapsProtocolPIAFS
										+ RMobileCall::KCapsPstnMultimediaVoiceFallback
										+ RMobileCall::KCapsPstnMultimedia
										+ RMobileCall::KCapsIsdnMultimedia
										+ RMobileCall::KCapsProtocolExtended);

			dataCallCaps.iServiceCaps	= (TUint32)(RMobileCall::KCapsDataCircuitAsynchronous
										+ RMobileCall::KCapsDataCircuitAsynchronousRDI
										+ RMobileCall::KCapsDataCircuitSynchronous
										+ RMobileCall::KCapsDataCircuitSynchronousRDI
										+ RMobileCall::KCapsPADAsyncUDI 
										+ RMobileCall::KCapsPADAsyncRDI
										+ RMobileCall::KCapsPacketAccessSyncUDI
										+ RMobileCall::KCapsPacketAccessSyncRDI
										+ RMobileCall::KCapsServiceExtended);

			dataCallCaps.iQoSCaps			= RMobileCall::KCapsTransparent
											+ RMobileCall::KCapsNonTransparent
											+ RMobileCall::KCapsTransparentPreferred
											+ RMobileCall::KCapsNonTransparentPreferred;

			dataCallCaps.iHscsdSupport		= ETrue;
			dataCallCaps.iMClass			= 0x6;
			dataCallCaps.iMaxRxTimeSlots	= 0x3;
			dataCallCaps.iMaxTxTimeSlots	= 0x2;
			dataCallCaps.iTotalRxTxTimeSlots = 0x04;

			dataCallCaps.iCodingCaps		= RMobileCall::KCapsAiurCoding48
												+ RMobileCall::KCapsAiurCoding96
												+ RMobileCall::KCapsAiurCoding144;

			dataCallCaps.iAsymmetryCaps		= RMobileCall::KCapsAsymmetryNoPreference
												+ RMobileCall::KCapsAsymmetryDownlink
												+ RMobileCall::KCapsAsymmetryUplink;
			dataCallCaps.iUserInitUpgrade	= ETrue;
			dataCallCaps.iRLPVersionCaps	= RMobileCall::KCapsRLPSingleLinkVersion0
												+ RMobileCall::KCapsRLPSingleLinkVersion1
												+ RMobileCall::KCapsRLPMultiLinkVersion2;

			dataCallCaps.iV42bisCaps		= RMobileCall::KCapsV42bisTxDirection
												+ RMobileCall::KCapsV42bisRxDirection
												+ RMobileCall::KCapsV42bisBothDirections;

			// Pass the changes to client side and complete request
			Write(0,capsPckg);
			ret = KErrNone;
			}
		// Complete request with ret, by default KErrNone
		Message().Complete(ret);
		// Break GetMobileDataCallCaps
		break;
		}
	case EEtelCallDial:
		{
		if(TelServer()->iTestNumber == 10101)
			{
			// Return KerrCommsLineFail for this test case
			ret = KErrCommsLineFail;
			}
		else if(TelServer()->iTestNumber == 10103)
			{
			// Return KErrNotSupported for this test case
			ret = KErrNotSupported;
			}
		else if(TelServer()->iTestNumber == 10209)
			{
			// Return KErrGeneral for this test case
			ret = KErrGeneral;
			}
		else if(TelServer()->iTestNumber == 10210)
			{
			// Return KErrNotSupported for this test case
			ret = KErrNotSupported;
			}
		// Complete request with ret, by default KErrNone
		Message().Complete(ret);
		// Break Dial
		break;
		}
	case EEtelCallLoanDataPort:
		{
		if(TelServer()->iTestNumber == 10211)
			{
			// Return KErrGeneral for this test case
			Message().Complete(KErrGeneral);
			return;
            }
		else if(TelServer()->iTestNumber == 10303)
			{
			// Return KErrGeneral for this test case
			Message().Complete(KErrGeneral);
			return;
			}
		// If already in use, complete with error
		if(iLoanDataPortCallCount>0)
			{
			Message().Complete(KErrGeneral);
			return;
			}
		// Else complete with the right params
		else
			{
			RCall::TCommPort dataPort;
			dataPort.iCsy.Copy(KSerialCsy);
			dataPort.iPort.Copy(KSerialPort);
			TPtrC8 ptr1(REINTERPRET_CAST(TText8*,&dataPort),sizeof(RCall::TCommPort));
			Write(0,ptr1);
			// Increment count of loans
			iLoanDataPortCallCount++;
			}

		// Complete request with ret, by default KErrNone
		Message().Complete(ret);
		// Break LoanDataPort
		break;
		}
	case EEtelCallRecoverDataPort:
		{
		if(TelServer()->iTestNumber == 10212)
			{
			// Return KErrGeneral for this test case
			ret = KErrGeneral;
			}
		// Decrement count of loans
		iLoanDataPortCallCount--;
		// Complete request with ret, by default KErrNone
		Message().Complete(ret);
		// Break RecoverDataPort
		break;
		}
	case EEtelCallHangUp:
		{
		if(TelServer()->iTestNumber == 10213)
			{
			// Return KErrGeneral for this test case
			ret = KErrGeneral;
			}
		// Complete request with ret, by default KErrNone
		Message().Complete(ret);
		// Break RecoverDataPort
		break;
		}
	// Just plain complete without any errors
	case EEtelCallAnswer:
		{
		Message().Complete(KErrNone);
		break;
		}
	// Do not handle cancels
	case EEtelCallDialCancel:
	case EEtelCallHangUpCancel:
	case EEtelCallLoanDataPortCancel:
		{
		break;
		}
	default:
		Message().Complete(KErrNotSupported);
		break;
		}
	}

CContextConfigChangeReq* CContextConfigChangeReq::NewL(const RMessage2& aMessage, CTelServer* aServer)
	{
	CContextConfigChangeReq* self = new(ELeave)CContextConfigChangeReq(aMessage,aServer);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(); //self
	return self;
	}

CContextConfigChangeReq::CContextConfigChangeReq(const RMessage2& aMessage, CTelServer* aServer) : iOutstandingReq(aMessage), iTelServer(aServer)
	{
	}

void CContextConfigChangeReq::ConstructL()
	{
	}

CContextConfigChangeReq::~CContextConfigChangeReq()
	{
	}

void CContextConfigChangeReq::CompleteL(TChangeType )
	{
	// Callback for when config changes
	// This will differ depending on test#
	iOutstandingReq.Complete(KErrNone); // Note don't do anything for GPRS just complete the request
	}

void CContextConfigChangeReq::Cancel()
	{
	iOutstandingReq.Complete(KErrCancel);
	}

CContextStatusChangeReq* CContextStatusChangeReq::NewL(const RMessage2& aMessage, CTelServer* aServer, TInt aCallCount)
	{
	CContextStatusChangeReq* self = new(ELeave)CContextStatusChangeReq(aMessage,aServer,aCallCount);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(); //self
	return self;
	}

CContextStatusChangeReq::~CContextStatusChangeReq()
	{
	iCallbackTimer->Cancel();
	delete iCallbackTimer;
	}

void CContextStatusChangeReq::CompleteL()
	{
	if(iCallCount==0)
		{
		RPacketContext::TContextStatus status;
		switch(iTelServer->iTestNumber)
			{
		case 1101:
			status = RPacketContext::EStatusActive;
			break;
		case 1102:
			status = RPacketContext::EStatusSuspended;
			break;
		case 1103:
			status = RPacketContext::EStatusDeactivating;
			break;
		case 1104:
			status = RPacketContext::EStatusInactive;
			break;
		case 1105:
			status = RPacketContext::EStatusDeleted ;
			break;
		case 1106:
			status = RPacketContext::EStatusActivating;
			break;
		case 1107:
			status = RPacketContext::EStatusUnknown;
			break;
		default:
			status = RPacketContext::EStatusActive;
			}
		TPckg<RPacketContext::TContextStatus> ptr(status);
		iOutstandingReq.WriteL(0,ptr);
		iOutstandingReq.Complete(KErrNone);
		iCallbackTimer->Cancel();
		// Reset the server status change request object
		iTelServer->iContextStatusChangeReq=NULL;
		delete this;
		}
	}

void CContextStatusChangeReq::Cancel()
	{
	iOutstandingReq.Complete(KErrCancel);
	}

CContextStatusChangeReq::CContextStatusChangeReq(const RMessage2& aMessage, CTelServer* aServer, TInt aCallCount) : iOutstandingReq(aMessage), iTelServer(aServer), iCallCount(aCallCount)
	{
	}

void CContextStatusChangeReq::ConstructL()
	{
	iCallbackTimer = CPeriodic::NewL(CActive::EPriorityStandard);
	if( (iTelServer->iTestNumber == 1101)||
		(iTelServer->iTestNumber == 1102)||
		(iTelServer->iTestNumber == 1103)||
		(iTelServer->iTestNumber == 1104)||
		(iTelServer->iTestNumber == 1105)||
		(iTelServer->iTestNumber == 1106)||
		(iTelServer->iTestNumber == 1107) )
		{
		iCallbackTimer->Start(15000000,15000000,TCallBack(TimerCallback,this));
		}
	}

TInt CContextStatusChangeReq::TimerCallback(TAny* aPtr)
	{
	CContextStatusChangeReq* self = (CContextStatusChangeReq*)aPtr;
	TInt err = KErrNone;
	TRAP(err,self->CompleteL());
	return err;
	}

CQoSConfigChangeReq* CQoSConfigChangeReq::NewL(const RMessage2& aMessage, CTelServer* aServer)
	{
	CQoSConfigChangeReq* self = new(ELeave)CQoSConfigChangeReq(aMessage,aServer);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(); //self
	return self;
	}

CQoSConfigChangeReq::~CQoSConfigChangeReq()
	{
	}

void CQoSConfigChangeReq::CompleteL(TChangeType )
	{
	iOutstandingReq.Complete(KErrNone); // Note don't do anything for GPRS as a drop below the minimum QoS will cause the context to drop
	}

void CQoSConfigChangeReq::Cancel()
	{
	iOutstandingReq.Complete(KErrCancel);
	}

CQoSConfigChangeReq::CQoSConfigChangeReq(const RMessage2& aMessage, CTelServer* aServer) : iOutstandingReq(aMessage), iTelServer(aServer)
	{
	}

void CQoSConfigChangeReq::ConstructL()
	{
	}
