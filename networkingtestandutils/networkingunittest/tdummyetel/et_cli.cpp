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

#include "dummyEtel.h"
#include <f32file.h>
#include "ETELEXT.H"
#include "Et_clsvr.h"
#include "ET_STRUC.H"
#include "ETELPTR.H"

#define NUM_FAXPTR		1
#define NUM_FAXPTRC		1
#define NUM_CALLPTR		6
#define NUM_CALLPTRC	3
#define NUM_LINEPTR		6
#define NUM_PHONEPTR	3


EXPORT_C void PanicClient(TInt aFault)
//
// Panic the client on client side
//
	{
	_LIT(KETelClientFault,"Etel Client Fault");
	User::Panic(KETelClientFault,aFault);
	}

//
// RTelSubSessionBase
//
EXPORT_C RTelSubSessionBase::RTelSubSessionBase()
	:iTelSession(NULL)
	,iPtrHolder(NULL)
	{}

EXPORT_C TInt RTelSubSessionBase::CancelReq(const TInt aIpc,const TInt aIpcToCancel) const

//
//	Set up a cancel request before pass to server
//
	{
	__ASSERT_ALWAYS(aIpc!=aIpcToCancel,PanicClient(KErrArgument));
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TAny* p[KMaxMessageArguments];
	p[0]=REINTERPRET_CAST(TAny*,aIpcToCancel);
	p[1]=REINTERPRET_CAST(TAny*,EIsaCancelMessage);
	TIpcArgs args;
	args.Set(0,p[0]);
	args.Set(1,p[1]);
	return SendReceive(aIpc,args);
	}

EXPORT_C TInt RTelSubSessionBase::CancelSubSession() const
//
//	Set up a cancel request before pass to server
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,TIpcArgs::ENothing);
	args.Set(1,REINTERPRET_CAST(TAny*,EIsaCancelSubSession));
	return SendReceive(EEtelCancelSubSession,args);
	}

EXPORT_C void RTelSubSessionBase::CancelAsyncRequest(TInt aReqToCancel) const
	{
	TInt cancelIpc;
	cancelIpc = aReqToCancel + EMobileCancelOffset;
	CancelReq(cancelIpc, aReqToCancel);
	}

EXPORT_C TInt RTelSubSessionBase::Blank(const TInt aIpc,TReqPriorityType aType) const
//
//	Set up a Null SYNC request before pass to server
//	There is no info passing between client/server
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,TIpcArgs::ENothing);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNull | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNull));
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Blank(const TInt aIpc,TRequestStatus& aStatus,TReqPriorityType aType) const
//
//	Set up a Null ASYNC request before pass to server
//	There is no info passing between client/server
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,TIpcArgs::ENothing);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNull | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNull));
	SendReceive(aIpc,args,aStatus);
	}


EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC8& aDes,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDesTobeSet));
	
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDesTobeSet));
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes8& aDes,TReqPriorityType aType) const
//
//	Set up an SYNC request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDesTobeRead));
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,TDes8& aDes,TReqPriorityType aType) const
//
//	Set up an async request which the client provide a Des in p[0]
//	Server will write back data to client address space in aPtr
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDesTobeRead));
	
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,
								  TDes8& aDes1,TDes8& aDes2,TReqPriorityType aType) const
//
//	Set up a asynch request - server write data back into descriptors P0 and P2
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDoubleDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDoubleDesTobeRead));
	args.Set(2,&aDes2);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes8& aDes1,TDes8& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request - server write data back into descriptors P0 and P2
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDoubleDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDoubleDesTobeRead));
	args.Set(2,&aDes2);
		
	return SendReceive(aIpc,args);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC8& aDes1,const TDesC8& aDes2,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0, &aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDoubleDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDoubleDesTobeSet));
	args.Set(2,&aDes2);
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes1,
									const TDesC8& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0, &aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaDoubleDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaDoubleDesTobeSet));
	args.Set(2,&aDes2);
		
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC16& aDes,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDesTobeSet));
		
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC16& aDes,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDesTobeSet));
			
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes16& aDes,TReqPriorityType aType) const
//
//	Set up an SYNC request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDesTobeRead));
	
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,TDes16& aDes,TReqPriorityType aType) const
//
//	Set up an async request which the client provide a Des in p[0]
//	Server will write back data to client address space in aPtr
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDesTobeRead));
	
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,
								  TDes16& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a asynch request - server write data back into descriptors P0 and P2
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDoubleDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDoubleDesTobeRead));
	args.Set(2,&aDes2);
	
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes16& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request - server write data back into descriptors P0 and P2
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);	
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDoubleDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDoubleDesTobeRead));
	args.Set(2,&aDes2);
	
	return SendReceive(aIpc,args);
	}
	 
EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,
								  TDes8& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a asynch request - server write data back into descriptors P0 and P2
//	P0 is a narrow descriptor, P2 is a unicode descriptor
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);	
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNarrowAndUnicodeDoubleDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNarrowAndUnicodeDoubleDesTobeRead));
	args.Set(2,&aDes2);
	
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes8& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request - server write data back into descriptors P0 and P2
//	P0 is a narrow descriptor, P2 is a unicode descriptor
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);	
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNarrowAndUnicodeDoubleDesTobeRead | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNarrowAndUnicodeDoubleDesTobeRead));
	args.Set(2,&aDes2);
	
	return SendReceive(aIpc,args);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC16& aDes1,const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDoubleDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDoubleDesTobeSet));
	args.Set(2,&aDes2);
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC16& aDes1,
									const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaUnicodeDoubleDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaUnicodeDoubleDesTobeSet));
	args.Set(2,&aDes2);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC8& aDes1,const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNarrowAndUnicodeDoubleDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNarrowAndUnicodeDoubleDesTobeSet));
	args.Set(2,&aDes2);
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes1,
									const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNarrowAndUnicodeDoubleDesTobeSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNarrowAndUnicodeDoubleDesTobeSet));
	args.Set(2,&aDes2);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C void RTelSubSessionBase::SetAndGet(const TInt aIpc, TRequestStatus& aStatus, const TDesC8& aDes1, TDes8& aDes2, TReqPriorityType aType) const
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNarrowDesToSetAndGet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNarrowDesToSetAndGet));
	args.Set(2,&aDes2);
	
	SendReceive(aIpc,args, aStatus);
	}

EXPORT_C void RTelSubSessionBase::SetAndGet(const TInt aIpc, TRequestStatus& aStatus, TDes8& aDes1, const TDesC16& aDes2, TReqPriorityType aType) const
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,REINTERPRET_CAST(TAny*,(EIsaNarrowDesToGetUnicodeDesToSet | KPriorityClientReq)));
	else
		args.Set(1,REINTERPRET_CAST(TAny*,EIsaNarrowDesToGetUnicodeDesToSet));
	args.Set(2,&aDes2);
		
	SendReceive(aIpc,args,aStatus);
	}

//
// RFax
//
EXPORT_C RFax::RFax()
//
//	C'tor
//
	:iProgressPtr(NULL)
	{}

EXPORT_C TInt RFax::Open(RCall& aCall)
//
//	Setup request to open a fax port and attempt to get a handle on chunk of fax progress 
//  which may have already been created server side when a fax call was initiated.
//	Return KErrNone even if the fax chunk is not found.
//
	{
	RSessionBase* session=&aCall.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));

	TInt subSessionHandle=aCall.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TPtrC null(_L("FAX"));		// if this is a NULL, server will assume TSY is going to assign
								// a name.
								// Didn't update to _LIT as GCC is inefficient at short names.
	TIpcArgs args;
	args.Set(0,&null);
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,subSessionHandle);
		
	SetSessionHandle(*session);
	TInt res = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (res)
		{
		Destruct();
		return res;
		}
	TName name;
	name = (_L("*"));
	name.Append(KChunkName);
	TFindChunk cCkFind(name);
	TFullName n;
	res = cCkFind.Next(n);
	if (res==KErrNone)
		res = iChunk.Open(cCkFind);
	if ((res!=KErrNone) && (res!=KErrNotFound))
		{
		Destruct();
		return res;
		}
	if (res==KErrNone)
		iProgressPtr = REINTERPRET_CAST(TProgress*,iChunk.Base());
	return KErrNone;
	}

EXPORT_C void RFax::Close()
//
//	close a fax port
//
 	{
	if (iProgressPtr)
		iChunk.Close();
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RFax::ConstructL()
	{
	__ASSERT_ALWAYS(iPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iPtrHolder = CPtrHolder::NewL(NUM_FAXPTR,NUM_FAXPTRC);
	}

EXPORT_C void RFax::Destruct()
	{
	if (iPtrHolder)
		delete iPtrHolder;
	iPtrHolder=NULL;
	}

EXPORT_C void RFax::Read(TRequestStatus& aStatus,TDes8& aDes)
	{
	__ASSERT_ALWAYS(aDes.MaxLength()!=0,PanicClient(KErrArgument));
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(0)).Set(CONST_CAST(TUint8*,aDes.Ptr()),aDes.Length(),aDes.MaxLength());
	Get(EEtelFaxRead,aStatus,iPtrHolder->Ptr(0));
	}

EXPORT_C void RFax::Write(TRequestStatus& aStatus,const TDesC8& aDes)
	{
	__ASSERT_ALWAYS(aDes.Length()!=0,PanicClient(KErrArgument));
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->PtrC(0)).Set(aDes);
	Set(EEtelFaxWrite,aStatus,iPtrHolder->PtrC(0));
	}

EXPORT_C void RFax::WaitForEndOfPage(TRequestStatus& aStatus) const
	{
	Blank(EEtelFaxWaitForEndOfPage,aStatus);
	}

EXPORT_C TInt RFax::TerminateFaxSession() const
	{
	TInt status=CancelSubSession();
	if (status==KErrNone)
		return Blank(EEtelFaxTerminateFaxSession);
	return(status);
	}

EXPORT_C TInt RFax::GetProgress(TProgress& aProgress) 
// 
//	The handle on the chunk may not have been obtained in RFax::Open() as the chunk may not
//	have existed at that point. Once the fax server is connecting, however, the chunk exists
//	so the handle must be opened here.
//
	{
	if (iProgressPtr==NULL)
		{
		TName name;
		name = (_L("*"));
		name.Append(KChunkName);
		TFindChunk cCkFind(name);
		TFullName n;
		TInt res = cCkFind.Next(n);
		if (res==KErrNone)
			res = iChunk.Open(cCkFind);
		if (res)
			return res;
		iProgressPtr = REINTERPRET_CAST(TProgress*,iChunk.Base());
		}
	aProgress = *iProgressPtr;
	return KErrNone;
	}

//
// RCall
//
EXPORT_C RCall::RCall()
//
//	C'tor
//
	{}

EXPORT_C TInt RCall::OpenNewCall(RTelServer& aServer,const TDesC& aName,TDes& aNewName)
//
//	Setup a request to open a RCall
//	assume RTelServer is connected and phone module loaded
//
	{
	__ASSERT_ALWAYS(aServer.Handle()!=0,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(aName.Length()!=0,PanicClient(KErrBadName));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TIpcArgs args;
	args.Set(0,&aName);
	args.Set(1,&aNewName);
	SetSessionHandle(aServer);
	ret = CreateSubSession(aServer,EEtelOpenFromSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C TInt RCall::OpenNewCall(RTelServer& aServer,const TDesC& aName)
	{
	TName aNewName;
	return OpenNewCall(aServer,aName,aNewName);
	}

EXPORT_C TInt RCall::OpenNewCall(RPhone& aPhone,const TDesC& aName,TDes& aNewName)
//
//	Setup a request to open a RCall
//	assume RTelServer is connected and phone module loaded
//
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(aName.Length()!=0,PanicClient(KErrBadName));
	TIpcArgs args;
	args.Set(0,&aName);
	args.Set(1,&aNewName);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C TInt RCall::OpenNewCall(RPhone& aPhone,const TDesC& aName)
	{
	TName aNewName;
	return OpenNewCall(aPhone,aName,aNewName);
	}

EXPORT_C TInt RCall::OpenNewCall(RLine& aLine,TDes& aNewName)
//
//	Setup a request to open a RCall
//	assume RTelServer is connected and phone module loaded
//
	{
	RSessionBase* session=&aLine.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aLine.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	TPtrC name(_L("::"));	// necessary so that server knows to ask TSY for new name
	TIpcArgs args;
	args.Set(0,&name);
	args.Set(1,&aNewName);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C TInt RCall::OpenNewCall(RLine& aLine)
//
//	Setup a request to open a RCall
//	assume RTelServer is connected and phone module loaded
//	pass a null descriptor so the server know and generate a dafault name
//
	{
	RSessionBase* session=&aLine.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aLine.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	TPtrC null(_L("::"));
	TName newName;	// which is passed back but is not needed
	TIpcArgs args;
	args.Set(0,&null);
	args.Set(1,&newName);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C TInt RCall::OpenExistingCall(RTelServer& aServer,const TDesC& aName)
//
//	Opens a handle on existing call object. Returns KErrNotFound if it does not exist
//
	{
	__ASSERT_ALWAYS(aServer.Handle()!=0,PanicClient(EEtelPanicNullHandle));
	//Don't panic the client for zero length names, just return the error code
	if(aName.Length()==0) return KErrBadName;
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TIpcArgs args;
	args.Set(0,&aName);
	SetSessionHandle(aServer);
	ret = CreateSubSession(aServer,EEtelOpenByNameFromSession,args);
	if (ret)
		Destruct();
	return ret;
	}
EXPORT_C TInt RCall::OpenExistingCall(RPhone& aPhone,const TDesC& aName)
//
//	Opens a handle on existing call object. Returns KErrNotFound if it does not exist
//
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	//Don't panic the client for zero length names, just return the error code
	if(aName.Length()==0) return KErrBadName;
	TIpcArgs args;
	args.Set(0,&aName);
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	
	ret = CreateSubSession(*session,EEtelOpenByNameFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}
EXPORT_C TInt RCall::OpenExistingCall(RLine& aLine,const TDesC& aName)
//
//	Opens a handle on existing call object. Returns KErrNotFound if it does not exist
//
	{
	RSessionBase* session=&aLine.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aLine.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
		//Don't panic the client for zero length names, just return the error code
	if(aName.Length()==0) return KErrBadName;
	TIpcArgs args;
	args.Set(0,&aName);
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenByNameFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RCall::ConstructL()
//
//	virtual function which extensions may overload. Called in Open()
//
	{
	__ASSERT_ALWAYS(iPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iPtrHolder = CCallPtrHolder::NewL(NUM_CALLPTR,NUM_CALLPTRC);
	}

EXPORT_C void RCall::Destruct()
//
//	virtual function which extensions may overload. Called in Close()
//
	{
	if (iPtrHolder)
		delete iPtrHolder;
	iPtrHolder=NULL;
	}

EXPORT_C void RCall::Close()
//
//	Close RCall
//
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C TInt RCall::LoanDataPort(TCommPort& aDataPort) const
//
//	Return with infos comm port currently used ( CSY module and COMM::number)
//	TSY then block any requests to the server until  RecoverDataPort() is called
//
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aDataPort),sizeof(TCommPort),sizeof(TCommPort));
	return Get(EEtelCallLoanDataPort,ptr);
	}

EXPORT_C void RCall::LoanDataPort(TRequestStatus& aStatus,TCommPort& aDataPort)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(0)).Set(REINTERPRET_CAST(TText8*,&aDataPort),sizeof(TCommPort),sizeof(TCommPort));
	Get(EEtelCallLoanDataPort,aStatus,iPtrHolder->Ptr(0));
	}

EXPORT_C void RCall::LoanDataPortCancel() const
	{
	CancelReq(EEtelCallLoanDataPortCancel,EEtelCallLoanDataPort);
	}

EXPORT_C TInt RCall::RecoverDataPort() const
//
//	Recover a comm port after loaning it
//
	{
	return Blank(EEtelCallRecoverDataPort);
	}

EXPORT_C TInt RCall::GetStatus(TStatus& aCallStatus) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aCallStatus),sizeof(TStatus),sizeof(TStatus));
	return Get(EEtelCallGetStatus,ptr);
	}

EXPORT_C TInt RCall::GetCaps(TCaps& aCaps) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aCaps),sizeof(TCaps),sizeof(TCaps));
	return Get(EEtelCallGetCaps,ptr);
	}

EXPORT_C void RCall::NotifyCapsChange(TRequestStatus& aStatus, TCaps& aCaps)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(1)).Set(REINTERPRET_CAST(TText8*,&aCaps),sizeof(TCaps),sizeof(TCaps));
	Get(EEtelCallCapsChangeNotification,aStatus,iPtrHolder->Ptr(1));
	}

EXPORT_C void RCall::NotifyCapsChangeCancel() const
	{
	CancelReq(EEtelCallCapsChangeNotificationCancel,EEtelCallCapsChangeNotification);
	}

EXPORT_C TInt RCall::Dial(const TDesC8& aCallParams,const TTelNumberC& aTelNumber) const
	{
	if(aTelNumber.Length()==0)
		return KErrArgument;
	return Set(EEtelCallDial,aCallParams,aTelNumber);
	}

EXPORT_C void RCall::Dial(TRequestStatus& aStatus,const TDesC8& aCallParams,const TTelNumberC& aTelNumber)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	if(aTelNumber.Length()==0)
		{
		TRequestStatus* status=&aStatus;
		User::RequestComplete(status,KErrArgument);
		return;
		}
	(iPtrHolder->PtrC(0)).Set(aCallParams);
	Set(EEtelCallDial,aStatus,iPtrHolder->PtrC(0),aTelNumber);
	}

EXPORT_C TInt RCall::Dial(const TTelNumberC& aTelNumber) const
	{
	if(aTelNumber.Length()==0)
		return KErrArgument;
	TPtr8 null(NULL,0);
	return Dial(null,aTelNumber);
	}

EXPORT_C void RCall::Dial(TRequestStatus& aStatus,const TTelNumberC& aTelNumber)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	if(aTelNumber.Length()==0)
		{
		TRequestStatus* status=&aStatus;
		User::RequestComplete(status,KErrArgument);
		return;
		}
	(iPtrHolder->PtrC(0)).Set(NULL,0);
	Set(EEtelCallDial,aStatus,iPtrHolder->PtrC(0),aTelNumber);
	}

EXPORT_C void RCall::DialCancel() const
	{
	CancelReq(EEtelCallDialCancel,EEtelCallDial);
	}

EXPORT_C TInt RCall::Connect(const TDesC8& aCallParams) const
	{
	return Set(EEtelCallConnect,aCallParams);
	}
EXPORT_C void RCall::Connect(TRequestStatus& aStatus,const TDesC8& aCallParams)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->PtrC(1)).Set(aCallParams);
	Set(EEtelCallConnect,aStatus,iPtrHolder->PtrC(1));
	}

EXPORT_C TInt RCall::Connect() const
	{
	TPtr8 null(NULL,0);
	return Connect(null);
	}

EXPORT_C void RCall::Connect(TRequestStatus& aStatus)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->PtrC(1)).Set(NULL,0);
	Set(EEtelCallConnect,aStatus,iPtrHolder->PtrC(1));
	}

EXPORT_C void RCall::ConnectCancel() const
	{
	CancelReq(EEtelCallConnectCancel,EEtelCallConnect);
	}

EXPORT_C TInt RCall::AnswerIncomingCall(const TDesC8& aCallParams) const
	{
	return Set(EEtelCallAnswer,aCallParams);
	}

EXPORT_C void RCall::AnswerIncomingCall(TRequestStatus& aStatus,const TDesC8& aCallParams) 
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->PtrC(2)).Set(aCallParams);
	Set(EEtelCallAnswer,aStatus,iPtrHolder->PtrC(2));
	}

EXPORT_C TInt RCall::AnswerIncomingCall() const
	{
	TPtr8 null(NULL,0);
	return AnswerIncomingCall(null);
	}

EXPORT_C void RCall::AnswerIncomingCall(TRequestStatus& aStatus)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->PtrC(2)).Set(NULL,0);
	Set(EEtelCallAnswer,aStatus,iPtrHolder->PtrC(2));
	}

EXPORT_C void RCall::AnswerIncomingCallCancel() const
	{
	CancelReq(EEtelCallAnswerCancel,EEtelCallAnswer);
	}

EXPORT_C TInt RCall::HangUp() const
	{
	return Blank(EEtelCallHangUp);
	}

EXPORT_C void RCall::HangUp(TRequestStatus& aStatus) const
	{
	Blank(EEtelCallHangUp,aStatus);
	}

EXPORT_C void RCall::HangUpCancel() const
	{
	CancelReq(EEtelCallHangUpCancel,EEtelCallHangUp);
	}

EXPORT_C TInt RCall::TransferOwnership() const
	{
	return Blank(EEtelCallTransferOwnership);
	}

EXPORT_C void RCall::AcquireOwnership(TRequestStatus& aStatus) const
	{
	Blank(EEtelCallAcquireOwnership,aStatus);
	}

EXPORT_C void RCall::AcquireOwnershipCancel() const
	{
	CancelReq(EEtelCallAcquireOwnershipCancel,EEtelCallAcquireOwnership);
	}

EXPORT_C TInt RCall::GetBearerServiceInfo(TBearerService& aBearerService) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aBearerService),sizeof(TBearerService),sizeof(TBearerService));
	return Get(EEtelCallGetBearerServiceInfo,ptr);
	}

EXPORT_C void RCall::NotifyHookChange(TRequestStatus& aStatus,THookStatus& aHookStatus)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(2)).Set(REINTERPRET_CAST(TText8*,&aHookStatus),sizeof(THookStatus),sizeof(THookStatus));
	Get(EEtelCallNotifyHookChange,aStatus,iPtrHolder->Ptr(2));
	}

EXPORT_C void RCall::NotifyHookChangeCancel() const
	{
	CancelReq(EEtelCallNotifyHookChangeCancel,EEtelCallNotifyHookChange);
	}

EXPORT_C TInt RCall::GetOwnershipStatus(TOwnershipStatus& aOwnershipStatus) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aOwnershipStatus),sizeof(TOwnershipStatus),sizeof(TOwnershipStatus));
	return Get(EEtelCallGetOwnershipStatus,ptr);
	}

EXPORT_C TInt RCall::GetCallParams(TDes8& aParams) const
	{
	return Get(EEtelCallGetCallParams,aParams);
	}

EXPORT_C RCall::TCallParams::TCallParams()
	: iSpeakerControl(RCall::EMonitorSpeakerControlOnUntilCarrier)
	,iSpeakerVolume(RCall::EMonitorSpeakerVolumeLow),iInterval(4),
	iWaitForDialTone(RCall::EDialToneWait),iIsDefault(ETrue)
	{
	iExtensionId=0;
	}

EXPORT_C RCall::TCallParams::TCallParams(TMonitorSpeakerControl aSpeakerControl,
										 TMonitorSpeakerVolume aSpeakerVolume,
										 TUint aInterval,
										 TWaitForDialTone aWaitForDialTone)
		: iSpeakerControl(aSpeakerControl),iSpeakerVolume(aSpeakerVolume),
		  iInterval(aInterval),iWaitForDialTone(aWaitForDialTone),iIsDefault(EFalse)
//
// C'tor for RCall::TCallParams
//
	{
	iExtensionId=0;
	}

EXPORT_C TInt RCall::TCallParams::ExtensionId()
//
// Retrieve Extension Ids supported by this class
//
	{
	return iExtensionId;
	}

EXPORT_C TInt RCall::TCallParams::IsDefault()
//
// Determine whether this struct has been created default or not
//
	{
	return iIsDefault;
	}

EXPORT_C TInt RCall::GetCallDuration(TTimeIntervalSeconds& aTime) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,CONST_CAST(TTimeIntervalSeconds*,&aTime)),sizeof(TTimeIntervalSeconds),sizeof(TTimeIntervalSeconds));
	return Get(EEtelCallGetCallDuration,ptr);
	}

EXPORT_C void RCall::NotifyCallDurationChange(TRequestStatus& aStatus,TTimeIntervalSeconds& aTime)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(3)).Set(REINTERPRET_CAST(TText8*,&aTime),sizeof(TTimeIntervalSeconds),sizeof(TTimeIntervalSeconds));
	Get(EEtelCallNotifyDurationChange,aStatus,iPtrHolder->Ptr(3));
	}

EXPORT_C void RCall::NotifyCallDurationChangeCancel() const
	{
	CancelReq(EEtelCallNotifyDurationChangeCancel,EEtelCallNotifyDurationChange);
	}

EXPORT_C void RCall::NotifyStatusChange(TRequestStatus& aStatus,TStatus& aCallStatus)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(4)).Set(REINTERPRET_CAST(TText8*,&aCallStatus),sizeof(TStatus),sizeof(TStatus));
	Get(EEtelCallNotifyStatusChange,aStatus,iPtrHolder->Ptr(4));
	}

EXPORT_C void RCall::NotifyStatusChangeCancel() const
	{
	CancelReq(EEtelCallNotifyStatusChangeCancel,EEtelCallNotifyStatusChange);
	}

EXPORT_C RCall::TCallInfo::TCallInfo()
		:	iHookStatus(EHookStatusUnknown),
			iStatus(EStatusUnknown),
			iDuration(0)
	{}

EXPORT_C TInt RCall::GetInfo(TCallInfo& aCallInfo) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aCallInfo),sizeof(TCallInfo),sizeof(TCallInfo));
	return Get(EEtelCallGetInfo,ptr);
	}

EXPORT_C TInt RCall::GetFaxSettings(TFaxSessionSettings& aSettings) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aSettings),sizeof(TFaxSessionSettings),sizeof(TFaxSessionSettings));
	return Get(EEtelCallGetFaxSettings,ptr);
	}

EXPORT_C TInt RCall::SetFaxSettings(const TFaxSessionSettings& aSettings) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,CONST_CAST(TFaxSessionSettings*,&aSettings)),sizeof(TFaxSessionSettings),sizeof(TFaxSessionSettings));
	return Set(EEtelCallSetFaxSettings,ptr);
	}

EXPORT_C TInt RCall::ReferenceCount() const
 	{
 	TInt count;
 	TPtr8 ptr(REINTERPRET_CAST(TText8*,&count),sizeof(TInt),sizeof(TInt));
 	Get(EEtelCallReferenceCount,ptr);
 	return count;
	}

//
// RLine
//
EXPORT_C RLine::RLine()
//
//	C'tor
//
	{}

EXPORT_C TInt RLine::Open(RTelServer& aServer,const TDesC& aName)
//
//	Setup a request to open a RLine
//	assume RTelServer is connected and phone module loaded
//
	{
	__ASSERT_ALWAYS(aServer.Handle()!=0,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(aName.Length()!=0,PanicClient(KErrBadName));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TIpcArgs args;
	args.Set(0,&aName);
	SetSessionHandle(aServer);
	ret = CreateSubSession(aServer,EEtelOpenFromSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C TInt RLine::Open(RPhone& aPhone,const TDesC& aName)
//
//	open a line by name
//
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(aName.Length()!=0,PanicClient(KErrBadName));
	TIpcArgs args;
	args.Set(0,&aName);
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,subSessionHandle);
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RLine::Close()
//
//	close a line
//
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RLine::ConstructL()
//
//	virtual function which extensions may overload. Called in Open()
//
	{
	__ASSERT_ALWAYS(iPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iPtrHolder = CPtrHolder::NewL(NUM_LINEPTR);
	}

EXPORT_C void RLine::Destruct()
//
//	virtual function which extensions may overload. Called in Close()
//
	{
	if (iPtrHolder)
		delete iPtrHolder;
	iPtrHolder=NULL;
	}

EXPORT_C TInt RLine::GetCaps(TCaps& aCaps) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aCaps),sizeof(TCaps),sizeof(TCaps));
	return Get(EEtelLineGetCaps,ptr);
	}

EXPORT_C TInt RLine::GetStatus(RCall::TStatus& aStatus) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aStatus),sizeof(RCall::TStatus),sizeof(RCall::TStatus));
	return Get(EEtelLineGetStatus,ptr);
	}

EXPORT_C TInt RLine::GetHookStatus(RCall::THookStatus& aHookStatus) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aHookStatus),sizeof(RCall::THookStatus),sizeof(RCall::THookStatus));
	return Get(EEtelLineGetHookStatus,ptr);
	}

EXPORT_C TInt RLine::EnumerateCall(TInt& aCount) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aCount),sizeof(TInt),sizeof(TInt));
	return Get(EEtelLineEnumerateCall,ptr);
	}

EXPORT_C TInt RLine::GetCallInfo(TInt aIndex,TCallInfo& aCallInfo) const
	{
	TCallInfoIndex info;
	info.iIndex=aIndex;
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&info),sizeof(TCallInfoIndex),sizeof(TCallInfoIndex));
	TInt ret=Get(EEtelLineGetCallInfo,ptr);
	//
	// MP - do bitwise copy of a whole structure this helps
	// later on if decided to add another item in the structure
	//
	if(ret==KErrNone)
		aCallInfo=info.iInfo;
	return ret;
	}


EXPORT_C RLine::TLineInfo::TLineInfo()
		:	iHookStatus(RCall::EHookStatusUnknown),
			iStatus(RCall::EStatusUnknown),
			iNameOfLastCallAdded(NULL),
			iNameOfCallForAnswering(NULL)
	{}

EXPORT_C TInt RLine::GetInfo(TLineInfo& aLineInfo) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aLineInfo),sizeof(TLineInfo),sizeof(TLineInfo));
	return Get(EEtelLineGetInfo,ptr);
	}

EXPORT_C void RLine::NotifyCapsChange(TRequestStatus& aStatus, TCaps& aCaps)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(1)).Set(REINTERPRET_CAST(TText8*,&aCaps),sizeof(TCaps),sizeof(TCaps));
	Get(EETelLineCapsChangeNotification,aStatus,iPtrHolder->Ptr(1));
	}

EXPORT_C void RLine::NotifyCapsChangeCancel() const
	{
	CancelReq(EETelLineCapsChangeNotificationCancel,EETelLineCapsChangeNotification);
	}

EXPORT_C void RLine::NotifyIncomingCall(TRequestStatus& aStatus, TName& aName)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(2)).Set(REINTERPRET_CAST(TText8*,&aName),sizeof(TName),sizeof(TName));
	Get(EEtelLineNotifyIncomingCall,aStatus,iPtrHolder->Ptr(2));
	}

EXPORT_C void RLine::NotifyIncomingCallCancel() const
	{
	CancelReq(EEtelLineNotifyIncomingCallCancel,EEtelLineNotifyIncomingCall);
	}

EXPORT_C void RLine::NotifyHookChange(TRequestStatus& aStatus,RCall::THookStatus& aHookStatus)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(3)).Set(REINTERPRET_CAST(TText8*,&aHookStatus),sizeof(RCall::THookStatus),sizeof(RCall::THookStatus));
	Get(EEtelLineNotifyHookChange,aStatus,iPtrHolder->Ptr(3));
	}

EXPORT_C void RLine::NotifyHookChangeCancel() const
	{
	CancelReq(EEtelLineNotifyHookChangeCancel,EEtelLineNotifyHookChange);
	}

EXPORT_C void RLine::NotifyStatusChange(TRequestStatus& aStatus,RCall::TStatus& aLineStatus)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(4)).Set(REINTERPRET_CAST(TText8*,&aLineStatus),sizeof(RCall::TStatus),sizeof(RCall::TStatus));
	Get(EEtelLineNotifyStatusChange,aStatus,iPtrHolder->Ptr(4));
	}

EXPORT_C void RLine::NotifyStatusChangeCancel() const
	{
	CancelReq(EEtelLineNotifyStatusChangeCancel,EEtelLineNotifyStatusChange);
	}

EXPORT_C void RLine::NotifyCallAdded(TRequestStatus& aStatus,TName& aName)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(5)).Set(REINTERPRET_CAST(TText8*,&aName),sizeof(TName),sizeof(TName));
	Get(EEtelLineNotifyCallAdded,aStatus,iPtrHolder->Ptr(5));
	}

EXPORT_C void RLine::NotifyCallAddedCancel() const
	{
	CancelReq(EEtelLineNotifyCallAddedCancel,EEtelLineNotifyCallAdded);
	}

//
// RPhone
//
EXPORT_C RPhone::RPhone()
//
// Constructor
//
	{}
			
EXPORT_C void RPhone::Close()
//
//	Close a phone
//
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C TInt RPhone::Open(RTelServer& aServer,const TDesC &aName)
//
//	open a phone by name
//
	{
	__ASSERT_ALWAYS(aServer.Handle()!=0,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(aName.Length()!=0,PanicClient(KErrBadName));
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	
	SetSessionHandle(aServer);
	TIpcArgs args;
	args.Set(0,&aName);
	args.Set(1,REINTERPRET_CAST(TAny*,0));
	ret = CreateSubSession(aServer,EEtelOpenFromSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RPhone::ConstructL()
//
//	virtual function which extensions may overload. Called in Open()
//
	{
	__ASSERT_ALWAYS(iPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iPtrHolder = CPtrHolder::NewL(NUM_PHONEPTR);
	}

EXPORT_C void RPhone::Destruct()
//
//	virtual function which extensions may overload. Called in Close()
//
	{
	if (iPtrHolder)
		delete iPtrHolder;
	iPtrHolder=NULL;
	}

EXPORT_C TInt RPhone::Initialise()
//
// Synchronous Initialisation
//
	{
	return Blank(EEtelPhoneInitialise);
	}

EXPORT_C void RPhone::Initialise(TRequestStatus& aStatus)
//
// Asynchronous Initialisation
//
	{
	Blank(EEtelPhoneInitialise,aStatus);
	}

EXPORT_C void RPhone::InitialiseCancel()
	{
	CancelReq(EEtelPhoneInitialiseCancel,EEtelPhoneInitialise);
	}

EXPORT_C RPhone::TPhoneInfo::TPhoneInfo() : iDetection(EDetectedUnknown)
	{}

EXPORT_C TInt RPhone::GetInfo(TPhoneInfo& aPhoneInfo) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aPhoneInfo),sizeof(TPhoneInfo),sizeof(TPhoneInfo));
	return Get(EEtelPhoneGetInfo,ptr);
	}

EXPORT_C void RPhone::NotifyModemDetected(TRequestStatus& aStatus,TModemDetection& aDetection)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(1)).Set(REINTERPRET_CAST(TText8*,&aDetection),sizeof(TModemDetection),sizeof(TModemDetection));
	Get(EEtelPhoneNotifyModemDetected,aStatus,iPtrHolder->Ptr(1));
	}

EXPORT_C void RPhone::NotifyModemDetectedCancel() const
	{
	CancelReq(EEtelPhoneNotifyModemDetectedCancel,EEtelPhoneNotifyModemDetected);
	}

EXPORT_C void RPhone::NotifyCapsChange(TRequestStatus& aStatus, TCaps& aCaps)
	{
	__ASSERT_ALWAYS(iPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	(iPtrHolder->Ptr(2)).Set(REINTERPRET_CAST(TText8*,&aCaps),sizeof(TCaps),sizeof(TCaps));
	Get(EETelPhoneCapsChangeNotification,aStatus,iPtrHolder->Ptr(2));
	}

EXPORT_C void RPhone::NotifyCapsChangeCancel() const
	{
	CancelReq(EETelPhoneCapsChangeNotificationCancel,EETelPhoneCapsChangeNotification);
	}

EXPORT_C TInt RPhone::GetCaps(TCaps& aCaps) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aCaps),sizeof(TCaps),sizeof(TCaps));
	return Get(EEtelPhoneGetCaps,ptr);
	}

EXPORT_C TInt RPhone::GetStatus(TStatus& aStatus) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aStatus),sizeof(TStatus),sizeof(TStatus));
	return Get(EEtelPhoneGetStatus,ptr);
	}

EXPORT_C TInt RPhone::EnumerateLines(TInt& aCount) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aCount),sizeof(TInt),sizeof(TInt));
	return Get(EEtelPhoneEnumerateLines,ptr);
	}

EXPORT_C TInt RPhone::GetLineInfo(const TInt aIndex,TLineInfo& aLineInfo) const
	{
	TLineInfoIndex info;
	info.iIndex=aIndex;
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&info),sizeof(TLineInfoIndex),sizeof(TLineInfoIndex));
	TInt ret=Get(EEtelPhoneGetLineInfo,ptr);
	//
	// MP - do bitwise copy of a whole structure this helps
	// later on if decided to add another item in the structure
	//
	if(ret==KErrNone)
		aLineInfo=info.iInfo;
	return ret;
	}

//
// RTelServer
//
EXPORT_C RTelServer::RTelServer()
//
//	C'tor
//
	{}

#if !defined(__EPOC32__)
const TInt KThreadShutdownPause=500000;
#endif

TInt StartEtel(TAny*);

								// It is a default parameter to RTelServer::CreateSession()
								// It also switches off a warning that the call to RTelServer::Version()
								// which is an inline function, is not expanded
EXPORT_C TInt RTelServer::Connect(TInt aMessageSlots)
//
//	connect server with specified message slots
//
	{
#if !defined(__EPOC32__)
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus status;
	timer.After(status,KThreadShutdownPause);
	User::WaitForRequest(status);
	timer.Close();
#endif
	TInt result=CreateSession(ETEL_SERVER_NAME,Version(),aMessageSlots);
	if(result==KErrNotFound)
		{
		result=StartEtel(NULL);
		if(result!=KErrNone)
			return result;
		result=CreateSession(ETEL_SERVER_NAME,Version(),aMessageSlots);
		}
	return result;
	}

EXPORT_C TInt RTelServer::GetPhoneInfo(const TInt aIndex,TPhoneInfo& aPhoneInfo) const
//
//	Get phone info
//
	{
	TPckg<TPhoneInfo> result(aPhoneInfo);
	TIpcArgs args;
	args.Set(0,&result);
	args.Set(1,aIndex);
	
	return SendReceive(EEtelServerPhoneInfoByIndex,args);
	}

EXPORT_C TInt RTelServer::EnumeratePhones(TInt& aCount) const
//
//	Get number of phones
//
	{
	TPckg<TInt> result(aCount);
	TIpcArgs args;
	args.Set(0,&result);
	return SendReceive(EEtelServerEnumeratePhones,args);
	}

EXPORT_C TInt RTelServer::GetTsyName(const TInt aIndexOfPhone, TDes& aTsyName) const
	{
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,aIndexOfPhone));
	args.Set(1,&aTsyName);
	return SendReceive(EEtelServerGetTsyName,args);
	}

EXPORT_C TInt RTelServer::LoadPhoneModule(const TDesC& aName) const
//
//	load phone module by name
//
	{
	TIpcArgs args;
	args.Set(0,&aName);
	return SendReceive(EEtelServerLoadPhoneModule,args);
	}

EXPORT_C TInt RTelServer::UnloadPhoneModule(const TDesC& aName) const
//
//	Unload phone module by name
//
	{
	TIpcArgs args;
	args.Set(0,&aName);
	return SendReceive(EEtelServerClosePhoneModule,args);
	}

EXPORT_C TInt RTelServer::SetPriorityClient() const
//
// Request that this client be made the priority client and write
// the name of the priority call back to aCallName
//
	{
	TIpcArgs args;
	return SendReceive(EEtelServerSetPriorityClient,args);
	}

EXPORT_C TInt RTelServer::IsSupportedByModule(const TDesC& aTsyName, const TInt aMixin, TBool& aResult) const
	{
	TAny* p[KMaxMessageArguments];
	TPtr8 ptr1(REINTERPRET_CAST(TText8*,CONST_CAST(TInt*,&aMixin)),sizeof(TInt),sizeof(TInt));
	TPtr8 ptr2(REINTERPRET_CAST(TText8*,&aResult),sizeof(TBool),sizeof(TBool));
	p[0]=REINTERPRET_CAST(TAny*,CONST_CAST(TDesC*,&aTsyName));
	p[1]=REINTERPRET_CAST(TAny*,&ptr1);
	p[2]=REINTERPRET_CAST(TAny*,&ptr2);
	TIpcArgs args;
	args.Set(0,&aTsyName);
	args.Set(1,p[1]);
	args.Set(2,p[2]);
	return SendReceive(EEtelServerQueryTsyFunctionality,args);
	}

EXPORT_C TInt RTelServer::GetTsyVersionNumber(const TDesC& aTsyName,TVersion& aVersion) const
	{
	TAny* p[KMaxMessageArguments];
	TPtr8 ptr(REINTERPRET_CAST(TText8*,&aVersion),sizeof(TVersion),sizeof(TVersion));
	p[0]=REINTERPRET_CAST(TAny*,CONST_CAST(TDesC*,&aTsyName));
	p[1]=REINTERPRET_CAST(TAny*,&ptr);
	TIpcArgs args;
	args.Set(0,&aTsyName);
	args.Set(1,p[1]);
	return SendReceive(EEtelServerGetTsyVersionNo,args);
	}

EXPORT_C TInt RTelServer::SetExtendedErrorGranularity(const TErrorGranularity aGranularity) const
	{
	TPtr8 ptr(REINTERPRET_CAST(TText8*,CONST_CAST(TErrorGranularity*,&aGranularity)),
		sizeof(TErrorGranularity),sizeof(TErrorGranularity));
	TIpcArgs args;
	args.Set(0,&ptr);
	return SendReceive(EEtelServerSetExtendedErrorGranularity,args);
	}

EXPORT_C TInt RTelServer::__DbgMarkHeap()
//
// Set a heap mark in the comm server
//
	{
#if defined(_DEBUG)
	TIpcArgs args;
	return SendReceive(EEtelDbgMarkHeap,args);
#else
	return KErrNone;
#endif
	}

EXPORT_C TInt RTelServer::__DbgCheckHeap(TInt aCount)
//
// Set a heap mark in the comm server
//
	{
#if defined(_DEBUG)
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,aCount));
	return SendReceive(EEtelDbgCheckHeap,args);
	
#else
	aCount=KErrNone;
	return aCount;
#endif
	}

EXPORT_C TInt RTelServer::__DbgMarkEnd(TInt aCount)
//
// Set a heap mark in the comm server
//
	{
#if defined(_DEBUG)
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,aCount));
	return SendReceive(EEtelDbgMarkEnd,args);
#else 
	aCount=KErrNone;
	return aCount;
#endif
	}

EXPORT_C TInt RTelServer::__DbgFailNext(TInt aCount)
//
// Set a heap mark in the comm server
//
	{
#if defined(_DEBUG)
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,aCount));
	return SendReceive(EEtelDbgFailNext,args);
#else
	aCount=KErrNone;
	return aCount;
	
#endif
	}

//=========================
// JGG additional functions for test code to drive the dummy etel
EXPORT_C TInt RTelServer::SetTestNumber(const TInt aTestNumber)
	{
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,aTestNumber));
	return SendReceive(EDummyEtelSetTestNumber,args);
	}

EXPORT_C TInt RTelServer::SetMode(const TMobilePhoneNetworkMode aMode)
	{
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,aMode));
	return SendReceive(EDummyEtelSetMode,args);
	}

EXPORT_C TInt RTelServer::TriggerContextConfigDrop()
	{
	TIpcArgs args;
	return SendReceive(EDummyEtelTriggerContextConfigDrop,args);
	}

EXPORT_C TInt RTelServer::TriggerContextConfigRestore()
	{
	TIpcArgs args;
	return SendReceive(EDummyEtelTriggerContextConfigRestore,args);
	}

EXPORT_C TInt RTelServer::TriggerContextStatusChange()
	{
	TIpcArgs args;
	return SendReceive(EDummyEtelTriggerContextStatusChange,args);
	}

EXPORT_C TInt RTelServer::TriggerQoSConfigDrop()
	{
	TIpcArgs args;
	return SendReceive(EDummyEtelTriggerQoSConfigDrop,args);
	}

EXPORT_C TInt RTelServer::TriggerQoSConfigRestore()
	{
	TIpcArgs args;
	return SendReceive(EDummyEtelTriggerQoSConfigRestore,args);
	}
//================================
CPtrHolder* CPtrHolder::NewL(const TInt aSizeOfPtrArray,const TInt aSizeOfPtrCArray)
	{
	CPtrHolder* p = new (ELeave) CPtrHolder();
	CleanupStack::PushL(p);
	p->ConstructL(aSizeOfPtrArray,aSizeOfPtrCArray);
	CleanupStack::Pop();
	return p;
	}

CPtrHolder::CPtrHolder()

	{}

CPtrHolder::~CPtrHolder()
	{
	iPtrArray.Close();
	iPtrCArray.Close();
	}

void CPtrHolder::ConstructL(const TInt aSizeOfPtrArray,const TInt aSizeOfPtrCArray)
	{
	TPtr8 ptr(NULL,0);
	TInt i;
	for (i=0;i<aSizeOfPtrArray;i++)
		User::LeaveIfError(iPtrArray.Append(ptr));
	TPtrC8 ptrC(NULL,0);
	for (i=0;i<aSizeOfPtrCArray;i++)
		User::LeaveIfError(iPtrCArray.Append(ptrC));
	}

TPtr8& CPtrHolder::Ptr(const TInt aIndex)
	{
	__ASSERT_ALWAYS(aIndex<iPtrArray.Count(),PanicClient(EEtelPanicIndexOutOfRange));
	return iPtrArray[aIndex];
	}

TPtrC8& CPtrHolder::PtrC(const TInt aIndex)
	{
	__ASSERT_ALWAYS(aIndex<iPtrCArray.Count(),PanicClient(EEtelPanicIndexOutOfRange));
	return iPtrCArray[aIndex];
	}

RCall::TCallParamsPckg* CPtrHolder::CallParamsPckg()
	{
	return NULL;
	}

CCallPtrHolder* CCallPtrHolder::NewL(const TInt aSizeOfPtrArray,const TInt aSizeOfPtrCArray)
	{
	CCallPtrHolder* p = new (ELeave) CCallPtrHolder();
	CleanupStack::PushL(p);
	p->ConstructL(aSizeOfPtrArray,aSizeOfPtrCArray);
	CleanupStack::Pop();
	return p;
	}

CCallPtrHolder::CCallPtrHolder()
	:CPtrHolder()
	,iCallParams()
	,iCallParamsPckg(iCallParams)
	{}

CCallPtrHolder::~CCallPtrHolder()
	{}

RCall::TCallParamsPckg* CCallPtrHolder::CallParamsPckg()
	{
	return &iCallParamsPckg;
	}
