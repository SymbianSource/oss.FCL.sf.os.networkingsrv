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

#include "Et_sstd.h"

//
// CReqEntry class definitions
//

CReqEntry* CReqEntry::NewL(TTsyReqHandle aTsyReqHandle,const RMessage2& aMessage,CTelSession* aSession,CBuffer* aBuffer,const CTelObject* aTelObject,TInt aFunction,RHeap* aHeap)
//
//	Create new request entry.  aHeap should be NULL unless this is to be allocated on the 
//	priority client heap.
//
	{
	if (aHeap!=NULL)
		return new(aHeap) CReqEntry(aTsyReqHandle,aMessage,aSession,aBuffer,aTelObject,aFunction,aHeap);

	return new(ELeave) CReqEntry(aTsyReqHandle,aMessage,aSession,aBuffer,aTelObject,aFunction,NULL);
	}

CReqEntry::CReqEntry(TTsyReqHandle aTsyReqHandle,const RMessage2& aMessage,CTelSession* aSession,CBuffer* aBuffer,const CTelObject* aTelObject,TInt aFunction,RHeap* aHeap)
//
//	CReqEntry constructor
//
	: iTsyReqHandle(aTsyReqHandle), iMessage(aMessage), iSession(aSession), iTelObject(aTelObject),
	iClientInterested(ETrue), iCancelFnCalled(EFalse), iFunction(aFunction), iBuffer(aBuffer), 
	iReadByClient(ETrue), iMessageNulled(EFalse), iHeap(aHeap)
	{}

CReqEntry::~CReqEntry()
//
//	CReqEntry destructor
//
	{
	delete iBuffer;
	}

void CReqEntry::Deque()
//
//	Deque CReqEntry list
//
	{
	iLink.Deque();
	iLink.iPrev=iLink.iNext=NULL;
	}

void CReqEntry::CompleteAndDeque(TInt aError)
	{
	LOGTEXT(_L8("CReqEntry::CompleteAndDeque"));
	// if client does not interested in this request do not completed
	if (iClientInterested) 
		iMessage.Complete(aError);
	iLink.Deque();
	}

TAny* CReqEntry::operator new(TUint /*aSize*/,RHeap* /*aHeap*/) __NO_THROW
//
// overloaded new using the priority client heap - can leave
//
	{
/*
	TAny *pM=aHeap->AllocL(aSize);
	Mem::FillZ(pM,aSize);
	return(pM); */
	return NULL;
	}

TAny* CReqEntry::operator new(TUint aSize) __NO_THROW
	{
	return CBase::operator new(aSize);
	}
	
TAny* CReqEntry::operator new(TUint aSize,TLeave aLeave)
	{
	return CBase::operator new(aSize,aLeave);
	}

TAny* CReqEntry::operator new(TUint aSize,TUint anExtraSize) __NO_THROW
	{
	return CBase::operator new(aSize,anExtraSize);
	}

void CReqEntry::operator delete(TAny* aPtr)
//
// overloaded delete - deletes from priority client heap if iHeap is not NULL
//
	{
	if (((CReqEntry*)aPtr)->iHeap!=NULL)
		((CReqEntry*)aPtr)->iHeap->Free(aPtr);
	else
		User::Free(aPtr);
	}

void CReqEntry::operator delete(TAny* /*aPtr*/, TLeave /*aLeave*/)
	{
	}
void CReqEntry::operator delete(TAny* /*aPtr*/, TUint /*anExtraSize*/)
	{
	}
void CReqEntry::operator delete(TAny* /*aPtr*/, RHeap* /*aHeap*/)
	{
	}

	
//
//	CBuffer class definitions
//
CBuffer* CBuffer::NewL(HEtelBufC8* aBuf8,HEtelBufC16* aBuf16,RHeap* aHeap,TInt aSizeOfData1,TInt aSizeOfData2,TInt aNoOfSlots)
	{
	if (aHeap!=NULL)
		return new(aHeap) CBuffer(aBuf8,aBuf16,aHeap,aSizeOfData1,aSizeOfData2,aNoOfSlots);

	return new(ELeave) CBuffer(aBuf8,aBuf16,NULL,aSizeOfData1,aSizeOfData2,aNoOfSlots);
	}

CBuffer::CBuffer(HEtelBufC8* aBuf8,HEtelBufC16* aBuf16,RHeap* aHeap,TInt aSizeOfData1,TInt aSizeOfData2,TInt aNoOfSlots)
	: iBuf8(aBuf8),iBuf16(aBuf16),iHeap(aHeap),iRead(0),iWrite(0),iNoOfSlots(aNoOfSlots),
		iSizeOfData1(aSizeOfData1),iSizeOfData2(aSizeOfData2),
		iOverFlow(EFalse),iBufferFull(EFalse)
	{}

CBuffer::~CBuffer()
	{
	delete iBuf8;
	delete iBuf16;
	}

TAny* CBuffer::operator new(TUint /*aSize*/,RHeap* /*aHeap*/) __NO_THROW
//
// overloaded new using the priority client heap - can leave
//
	{
/*
	TAny *pM=aHeap->AllocL(aSize);
	Mem::FillZ(pM,aSize);
	return(pM); */
	return NULL;
	}

TAny* CBuffer::operator new(TUint aSize) __NO_THROW
	{
	return CBase::operator new(aSize);
	}
	
TAny* CBuffer::operator new(TUint aSize,TLeave aLeave)
	{
	return CBase::operator new(aSize,aLeave);
	}

TAny* CBuffer::operator new(TUint aSize,TUint anExtraSize) __NO_THROW
	{
	return CBase::operator new(aSize,anExtraSize);
	}

void CBuffer::operator delete(TAny* aPtr)
//
// overloaded delete - deletes from priority client heap if iHeap not NULL
//
	{
	if (((CBuffer*)aPtr)->iHeap!=NULL)
		((CBuffer*)aPtr)->iHeap->Free(aPtr);
	else
		User::Free(aPtr);
	}

void CBuffer::operator delete(TAny* /*aPtr*/, TLeave /*aLeave*/)
	{
	}
void CBuffer::operator delete(TAny* /*aPtr*/, TUint /*anExtraSize*/)
	{
	}
void CBuffer::operator delete(TAny* /*aPtr*/, RHeap* /*aHeap*/)
	{
	}


void CBuffer::IncRead()
	{
	if(++iRead==iNoOfSlots)
		iRead=0;
	iOverFlow=EFalse;
	iBufferFull=EFalse;
	LOGTEXT2(_L8("ETel:\tiRead incremented to %d"),iRead);
	}

void CBuffer::IncWrite()
	{
	if(++iWrite==iNoOfSlots)
		iWrite=0;
	if (iOverFlow)
		 iRead=iWrite;
	else
		{
		if (!iBufferFull)
			{
			if (iWrite==iRead)
				iBufferFull=ETrue;
			}
		else
			{
			iRead=iWrite;
			iOverFlow=ETrue;
			}
		}
	LOGTEXT2(_L8("ETel:\tiWrite incremented to %d"),iWrite);
	}

TUint8* CBuffer::CurrentSlotData1(TWhichSlot aWhichSlot) const
//
// iSizeOfData1 and iSizeOfData2 include the sizes of the TPtr's
//
	{
	TInt pos=((aWhichSlot==ESlotRead) ? iRead : iWrite);
	if (iBuf8==NULL || iBuf16==NULL)
		{
		TInt slotSize = iSizeOfData1 + iSizeOfData2;
		return (CONST_CAST(TUint8*,iBuf8->Des().Mid(pos*(slotSize),iSizeOfData1-sizeof(TPtr8)).Ptr()));
		}
	else	// we have a narrow and a unicode parameter. Data1 is always narrow
		return (CONST_CAST(TUint8*,iBuf8->Des().Mid(pos*iSizeOfData1,iSizeOfData1-sizeof(TPtr8)).Ptr()));
	}

TUint8* CBuffer::CurrentSlotData2(TWhichSlot aWhichSlot) const
	{
	if (iSizeOfData2==0)
		return NULL;
	TInt pos=((aWhichSlot==ESlotRead) ? iRead : iWrite);
	if (iBuf8==NULL || iBuf16==NULL)
		{
		TInt slotSize = iSizeOfData1 + iSizeOfData2;
		return (CONST_CAST(TUint8*,iBuf8->Des().Mid(pos*(slotSize)+iSizeOfData1,iSizeOfData2-sizeof(TPtr8)).Ptr()));
		}
	return NULL;
	}

TUint16* CBuffer::CurrentSlotData1u(TWhichSlot aWhichSlot) const
//
// Unicode version
//
	{
	TInt pos=((aWhichSlot==ESlotRead) ? iRead : iWrite);
	if (iBuf8==NULL || iBuf16==NULL)
		{
		TInt slotSize = iSizeOfData1 + iSizeOfData2;
		return (CONST_CAST(TUint16*,iBuf16->Des().Mid(pos*(slotSize),iSizeOfData1-sizeof(TPtr16)).Ptr()));
		}
	return NULL;	// Data1 will be narrow if we have a mixture of both 8 and 16 bit
	}

TUint16* CBuffer::CurrentSlotData2u(TWhichSlot aWhichSlot) const
//
// Unicode version
//
	{
	if (iSizeOfData2==0)
		return NULL;
	TInt pos=((aWhichSlot==ESlotRead) ? iRead : iWrite);
	if (iBuf8==NULL || iBuf16==NULL)
		{
		TInt slotSize = iSizeOfData1 + iSizeOfData2;
		return (CONST_CAST(TUint16*,iBuf16->Des().Mid(pos*(slotSize)+iSizeOfData1,iSizeOfData2-sizeof(TPtr16)).Ptr()));
		}
	else	// we have a narrow and a unicode parameter. Data2 is always unicode
		return (CONST_CAST(TUint16*,iBuf16->Des().Mid(pos*iSizeOfData2,iSizeOfData2-sizeof(TPtr16)).Ptr()));
	}


TInt CBuffer::CompareRWPtrs() const
	{
	return (iRead-iWrite);
	}

//
//
// CTelObject class definitions
//

EXPORT_C CTelObject::CTelObject()
//
// CTelObject constructor
//
	:iActiveReqCount(0), iTelServer(NULL), iDestroyDummySubSession(NULL), iCreateDummy(EFalse)
	{
	__DECLARE_NAME(_S("CTelObject"));
	}

EXPORT_C CTelObject::~CTelObject()
//
// Destructor
//
	{}

EXPORT_C void CTelObject::CTelObject_Reserved1()
//
// Reserved virtual function
//
	{}

void CTelObject::CreateDummySession(CTelSession* aSession, const TInt aSubSessionHandle, TBool aCreateDummy)
//
// So the server has to create a dummy session just to keep session alive until completed
// function is called.
//
	{
	LOGTEXT2(_L8("CreateDummySession() with iDestroyDummySubSession = %x"),iDestroyDummySubSession);
	iCreateDummy=aCreateDummy;
	if (iDestroyDummySubSession==NULL)
		{
		if ( IsActiveReq(aSession,aSubSessionHandle) || aCreateDummy)
			{
			LOGTEXT(_L8("About to create dummy session"));
			__ASSERT_ALWAYS(aSession!=NULL,Fault(EEtelFaultBadTelSessionPointer));
			CObject* theObj=this;
			while(theObj->Owner())
				{
				theObj->Open();
				theObj=theObj->Owner();
				}
			theObj->Open();
			iTelServer=aSession->TelServer();
			iTelServer->Inc();
			LOGTEXT2(_L8("Created a Dummy Session with count %d"),iTelServer->Count());
			TRAP_IGNORE(iDestroyDummySubSession=CDestroyDummySubSession::NewL(iTelServer,this));
			}
		}
	}

void CTelObject::TelObjectClose()
//
// Perform an iterative close of Tel Objects
//
	{
	CTelObject* owner=REINTERPRET_CAST(CTelObject*,Owner());
	Close();
	if(owner)
		owner->TelObjectClose();
	}

void CTelObject::CompleteAndDestroyReq(CReqEntry* aReqEntry,const TInt aStatus) const
//
//	complete the entry and remove from entry list and associate heap
//
	{
	if (!aReqEntry->iMessageNulled)
		aReqEntry->iMessage.Complete(aStatus);
	DestroyReq(aReqEntry);
	}

void CTelObject::DestroyReq(CReqEntry* aReqEntry) const
//
//	remove from entry list and associate heap
//
	{
	aReqEntry->Deque();
	delete aReqEntry;
	}

TBool CTelObject::IsActiveReq(CTelSession* aSession,const TInt aSubSessionHandle)
//
// Return True if found the active req still out standing
// going through and counting the total of active req
//
	{
	TBool ret=EFalse;
	CReqEntry* reqEntry=NULL;
	TDblQueIter<CReqEntry> iter(PhoneOwner()->ReqActiveList());
	while(reqEntry=iter++,reqEntry!=NULL) // go through the list from begin to end
		{
		if (aSession==reqEntry->iSession && aSubSessionHandle==reqEntry->iMessage.Int3() &&	reqEntry->iClientInterested )
			{
			iActiveReqCount++;
			ret=ETrue;
			}
		}
	LOGTEXT2(_L8("IsActiveReq found %d active reqs"),iActiveReqCount);
	return ret;
	}

void CTelObject::CancelActiveReq(CTelSession* aSession,const TInt aSubSessionHandle)
//
// active list - must inform tsy by calling CancelService
// this request will be destroyed when Tsy do an up call ReqCompleted with KErrCancel
// Only go through the list once set the count to zero at start to count number of requests 
//
	{
	LOGTEXT(_L8("Entered CancelActiveReq"));
	CReqEntry* reqEntry=NULL;
	TDblQueIter<CReqEntry> iter(PhoneOwner()->ReqActiveList());
	while(reqEntry=iter++,reqEntry!=NULL) // go through the list from begin to end
		{
		if (aSession==reqEntry->iSession && aSubSessionHandle==reqEntry->iMessage.Int3() &&	reqEntry->iClientInterested )
			{
			reqEntry->iClientInterested=EFalse;
			__ASSERT_ALWAYS(iActiveReqCount>=0,Fault(EEtelFaultNegativeActiveReqCount));
			if (reqEntry->iCancelFnCalled==FALSE)
				{
				if (reqEntry->iPlacedRequest)
					{
					LOGTEXT2(_L8("Calling Cancel Service ActiveReq TsyReq=%d"),reqEntry->iTsyReqHandle);
					reqEntry->iCancelFnCalled=ETrue;
					CancelService(reqEntry->iFunction,reqEntry->iTsyReqHandle);
					}
				else
					{
					LOGTEXT(_L8("Destroying request"));
					DestroyReq(reqEntry);
					CheckAndDestroyDummySubSession();
					}
				}
			}
		}
	}

void CTelObject::CancelSubSession(CTelSession* aSession,const TInt aSubSessionHandle)
//
// Flush outstanding requests from the queue for this client only
// Cancel the service if request already passed to Tsy
//
	{
	CReqEntry* reqEntry=NULL;
	while (reqEntry=PhoneOwner()->FindClientReqInWaitList(aSession,aSubSessionHandle),reqEntry)
		CompleteAndDestroyReq(reqEntry,KErrCancel);

	CancelActiveReq(aSession,aSubSessionHandle);
	}

void CTelObject::FlushReqs(CTelSession* aSession,const TInt aSubSessionHandle)
//
// Flush outstanding requests from the queue for this client only
// Cancel the service if request already passed to Tsy
// Return ETrue if it have to create a dummy session
//
	{
	CReqEntry* reqEntry=NULL;
	// wait list
	while (reqEntry=PhoneOwner()->FindClientReqInWaitList(aSession,aSubSessionHandle),reqEntry)
		DestroyReq(reqEntry);

	CancelActiveReq(aSession,aSubSessionHandle);

	}

CPhoneBase* CTelObject::PhoneOwner() const
//
// Get the owner handle
//
	{
 	return iPhoneOwner;
	}

void CTelObject::SetPhoneOwner(CPhoneBase* aPhoneOwner)
//
// Set the owner of phone
//
	{
	iPhoneOwner=aPhoneOwner;
	}

void CTelObject::CompleteOrBufferRead(const RMessage2& aMessage,CReqEntry* aReqEntry) 
//
//	Check and complete if possible this notification/multislot command from 
//	this subsession in the ReqActive list
//	aMessage is the message just received from the client
//	aReqEntry is the Entry previously created when the same client called the same function
//
	{
	if(aReqEntry->iMessageNulled==FALSE)	// same client already posted this notification
		PanicClient(EEtelPanicRequestAsyncTwice,aMessage);
	TInt slots=0;
	TRAP_IGNORE(slots = NumberOfSlotsL(aReqEntry->iFunction));
	if (slots>1)
		{
		TInt comparison = aReqEntry->iBuffer->CompareRWPtrs();
		if (comparison==0 && aReqEntry->iBuffer->BufferFull()==FALSE)
			{	// if the client has already read value,the automatic server-TSY 
				// notify request becomes the "true" request
			aReqEntry->iMessage=aMessage;	
			aReqEntry->iMessageNulled=EFalse;
			}
		else
			{
			if (aReqEntry->iBuffer->OverFlow())
				WriteBackAndCompleteBufferedReq(aMessage,aReqEntry,KErrOverflow);
			else
				WriteBackAndCompleteBufferedReq(aMessage,aReqEntry,KErrNone);
			}
		}
	else			// this is a non-registered notification (ie single-slot)
		{
		if(aReqEntry->iReadByClient)
			{
			aReqEntry->iMessage=aMessage;
			aReqEntry->iMessageNulled=EFalse;
			}
		else	// Client has not read this new value yet
			{
			WriteBackAndCompleteBufferedReq(aMessage,aReqEntry,KErrNone);
			}
		}
	}

void CTelObject::CheckAndResize(TInt& aSizeOfData,const RMessage2& aMessage) const
//
//	Checks that size of data is non-zero - if not, panic client.
//  Also checks that size of data is word-aligned, ie a multiple of 4 bytes. 
//	If not, ensure that in the space in buffer for data is word-aligned.
//
	{
	if(aSizeOfData<0)
		PanicClient(EEtelPanicDesLengthNegative,aMessage);
	aSizeOfData=(aSizeOfData+3)&~3;
	}

HEtelBufC8* CTelObject::CreateNarrowBufferLC(RHeap* aHeap,const RMessage2& aMessage,TInt aIndex1,TInt aIndex2,TInt& aSize1,TInt& aSize2,TInt aNoOfSlots) const
//
//	aSize1 = size of 1st descriptor buffer
//	aSize2 = size of 2nd descriptor buffer (default=0)
//  aHeap may be NULL if the priority heap is not to be used
//	aNoOfSlots will usually be 1. Only if this request is a notification that requires 
//	registering, ie increasing the number of events the server will buffer, will it be greater than 1
//
	{
	TInt desSize1=aSize1;
	TInt desSize2=aSize2;
	CheckAndResize(aSize1,aMessage);
	
	TInt ptrSize=sizeof(TPtr8);
	TInt allocSize=aSize1+ptrSize;
	if (aSize2)
		{
		CheckAndResize(aSize2,aMessage);
		allocSize += aSize2+ptrSize;
		}
	HEtelBufC8* buf = HEtelBufC8::NewMaxLC(allocSize*aNoOfSlots,aHeap);	// allocate a buffer for the Des and Data				

	TPtr8 firstptrDes1(CONST_CAST(TUint8*,buf->Des().Left(ptrSize).Ptr()),ptrSize,ptrSize);			// carve the thing up
	TPtr8 firstdataDes1(CONST_CAST(TUint8*,buf->Des().Mid(ptrSize,aSize1).Ptr()),desSize1,desSize1);
	TPtr8 firstdataDes2(NULL,0,0);
	aMessage.ReadL(aIndex1,firstdataDes1); // Read the Data. If it leaves, buf will be cleaned up
	firstptrDes1.Copy(REINTERPRET_CAST(TUint8*,&firstdataDes1),ptrSize);	// Copy the new Des
	if (aSize2)
		// Construct Second descriptor
		{
		TPtr8 firstptrDes2(CONST_CAST(TUint8*,buf->Des().Mid(ptrSize+aSize1,ptrSize).Ptr()),ptrSize,ptrSize);
		firstdataDes2.Set(CONST_CAST(TUint8*,buf->Des().Mid((ptrSize*2)+aSize1,aSize2).Ptr()),desSize2,desSize2);
		aMessage.ReadL(aIndex2,firstdataDes2);
		firstptrDes2.Copy(REINTERPRET_CAST(TUint8*,&firstdataDes2),ptrSize);
		}
	for (TInt i=1; i<aNoOfSlots;i++)	
		{// Copy descriptors into each slot
		TPtr8 ptrDes1(CONST_CAST(TUint8*,buf->Des().Mid(i*allocSize,ptrSize).Ptr()),ptrSize,ptrSize);
		TPtr8 dataDes1(CONST_CAST(TUint8*,buf->Des().Mid(i*allocSize+ptrSize,aSize1).Ptr()),desSize1,desSize1);
		dataDes1.Copy(firstdataDes1);
		ptrDes1.Copy(REINTERPRET_CAST(TUint8*,&dataDes1),ptrSize);
		if (aSize2)
			{
			TPtr8 ptrDes2(CONST_CAST(TUint8*,buf->Des().Mid(i*allocSize+ptrSize+aSize1,ptrSize).Ptr()),ptrSize,ptrSize);
			TPtr8 dataDes2(CONST_CAST(TUint8*,buf->Des().Mid(i*allocSize+(ptrSize*2)+aSize1).Ptr()),desSize2,desSize2);
			dataDes2.Copy(firstdataDes2);
			ptrDes2.Copy(REINTERPRET_CAST(TUint8*,&dataDes2),ptrSize);
			}
		}
	return buf;
	}

HEtelBufC16* CTelObject::CreateUnicodeBufferLC(RHeap* aHeap,const RMessage2& aMessage,TInt aIndex1,TInt aIndex2,TInt& aSize1,TInt& aSize2,TInt aNoOfSlots) const
//
//	We place both parameters in the same buffer which is either narrow or unicode (unicode in this case)
//	Should a function ever require one parameter to be narrow and the other unicode,
//	will need to review.
//
	{
	TInt desSize1=aSize1;
	TInt desSize2=aSize2;
	CheckAndResize(aSize1,aMessage);
	TInt ptrSize=sizeof(TPtr16);
	TInt allocSize=aSize1+ptrSize;
	if (aSize2)
		{
		CheckAndResize(aSize2,aMessage);
		allocSize += aSize2+ptrSize;
		}
	HEtelBufC16* buf = HEtelBufC16::NewMaxLC(allocSize*aNoOfSlots,aHeap);	// allocate a buffer for the Des and Data				

	TPtr16 firstptrDes1(CONST_CAST(TUint16*,buf->Des().Left(ptrSize).Ptr()),ptrSize,ptrSize);			// carve the thing up
	TPtr16 firstdataDes1(CONST_CAST(TUint16*,buf->Des().Mid(ptrSize,aSize1).Ptr()),desSize1,desSize1);
	TPtr16 firstdataDes2(NULL,0,0);
	
	// Read the Data. If it leaves, buf will be cleaned up
	aMessage.ReadL(aIndex1,firstdataDes1);
	firstptrDes1.Copy(REINTERPRET_CAST(TUint16*,&firstdataDes1),ptrSize);	// Copy the new Des
	if (aSize2)
		// Construct Second descriptor
		{
		TPtr16 firstptrDes2(CONST_CAST(TUint16*,buf->Des().Mid(ptrSize+aSize1,ptrSize).Ptr()),ptrSize,ptrSize);
		firstdataDes2.Set(CONST_CAST(TUint16*,buf->Des().Mid((ptrSize*2)+aSize1,aSize2).Ptr()),desSize2,desSize2);
		aMessage.ReadL(aIndex2,firstdataDes2);
		firstptrDes2.Copy(REINTERPRET_CAST(TUint16*,&firstdataDes2),ptrSize);
		}
	for (TInt i=1; i<aNoOfSlots;i++)	
		{// Copy descriptors into each slot
		TPtr16 ptrDes1(CONST_CAST(TUint16*,buf->Des().Mid(i*allocSize,ptrSize).Ptr()),ptrSize,ptrSize);
		TPtr16 dataDes1(CONST_CAST(TUint16*,buf->Des().Mid(i*allocSize+ptrSize,aSize1).Ptr()),desSize1,desSize1);
		dataDes1.Copy(firstdataDes1);
		ptrDes1.Copy(REINTERPRET_CAST(TUint16*,&dataDes1),ptrSize);
		if (aSize2)
			{
			TPtr16 ptrDes2(CONST_CAST(TUint16*,buf->Des().Mid(i*allocSize+ptrSize+aSize1,ptrSize).Ptr()),ptrSize,ptrSize);
			TPtr16 dataDes2(CONST_CAST(TUint16*,buf->Des().Mid(i*allocSize+(ptrSize*2)+aSize1).Ptr()),desSize2,desSize2);
			dataDes2.Copy(firstdataDes2);
			ptrDes2.Copy(REINTERPRET_CAST(TUint16*,&dataDes2),ptrSize);
			}
		}
	return buf;
	}

CReqEntry* CTelObject::ReqAnalyserL(const RMessage2& aMessage,CTelSession* aSession,TReqMode& aReqMode) 
//
// Analyse the request. First search for an entry from the same client for the same request,
// and if found either panic if this is a non-buffered request(ie the request is already
// pending) or look at the buffer to see if there is any unread data to complete with.
// If the identical entry was not found and this is a request of type multiple completion, search 
// for a similar one by a different client (on this TelObject) and if found simply create a new
// entry with the same request handle that will be completed at the same time as the current
// posted notification. 
// If this request has not been found in the active or wait lists, create the necessary 
// address space on server side
//
	{
	HEtelBufC8* buf8=NULL;
	HEtelBufC16* buf16=NULL;
	CReqEntry* reqEntry=NULL;
	
	reqEntry = PhoneOwner()->FindSameClientEntry(aSession,aMessage.Int3(),aMessage.Function());
	if (reqEntry) 
		{	
		if (aReqMode&KReqModeRePostImmediately)	
			{
			// same notification/buffered command has been found 
			// and either the read has completed or it has been stored in
			// the request entry waiting for the completion from the TSY.
			CompleteOrBufferRead(aMessage,reqEntry);
			}
		else	// found an outstanding async req - panic
			{
			PanicClient(EEtelPanicRequestAsyncTwice,aMessage);
			}
		return NULL;
		}
	TInt messageType=aMessage.Int1();
	TInt size1=0;
	TInt size2=0;
	TInt noOfSlots = 1;
	if (aReqMode&KReqModeRePostImmediately)
		noOfSlots = NumberOfSlotsL(aMessage.Function());

	if (messageType!=EIsaNull &&
		messageType!=EIsaCancelMessage &&
		messageType!=EIsPriorityClientReqWithNull)
		{						// we're going to create server-side buffer
		TInt basicMsgType = messageType&(~(KUnicodeReq|KPriorityClientReq));

		if (messageType==EIsaNarrowAndUnicodeDoubleDesTobeSet)
			basicMsgType=EIsaDoubleDesTobeSet;
		if (messageType==EIsaNarrowAndUnicodeDoubleDesTobeRead)
			basicMsgType=EIsaDoubleDesTobeRead;

		switch (basicMsgType)
			{
		case EIsaDoubleDesTobeSet:// if we're setting data, get length
			size2=aMessage.GetDesLength(2);	// no break
		case EIsaDesTobeSet:	
			size1=aMessage.GetDesLength(0);
			break;
		case EIsaDoubleDesTobeRead:// if we're reading data, get max length
			size2=aMessage.GetDesMaxLength(2);	// no break
		case EIsaDesTobeRead:	
			size1=aMessage.GetDesMaxLength(0);
			break;
		case EIsaNarrowDesToSetAndGet:
			size1=aMessage.GetDesLength(0);    // set data
			size2=aMessage.GetDesMaxLength(2); // get data
			break;
		case EIsaNarrowDesToGetUnicodeDesToSet:
			size1=aMessage.GetDesMaxLength(0); // get data
			size2=aMessage.GetDesLength(2);    // set data
			break;
		default:
			PanicClient(EEtelPanicInvalidRequestType,aMessage);
			break;
			}

		if (size1<0 || size2<0)
			{
			PanicClient(EEtelPanicBadDescriptor,aMessage);
			return NULL;
			}
		if (messageType==EIsaNarrowAndUnicodeDoubleDesTobeSet || 
			messageType==EIsaNarrowAndUnicodeDoubleDesTobeRead ||
			messageType==EIsaNarrowDesToGetUnicodeDesToSet)
			{
			TInt zeroSize=0;
			buf8 = CreateNarrowBufferLC(aSession->PriorityClientHeap(messageType),aMessage,0,2,size1,zeroSize,noOfSlots);
			buf16 = CreateUnicodeBufferLC(aSession->PriorityClientHeap(messageType),aMessage,2,0,size2,zeroSize,noOfSlots);	// the second client parameter pointer is passed as the first, so that the second parameter will be copied as the first
			}
		else
			if (!(aSession->IsUnicodeReq(messageType)))
				buf8 = CreateNarrowBufferLC(aSession->PriorityClientHeap(messageType),aMessage,0,2,size1,size2,noOfSlots);
			else	
				buf16 = CreateUnicodeBufferLC(aSession->PriorityClientHeap(messageType),aMessage,0,2,size1,size2,noOfSlots);	
		size1+=sizeof(TPtr);	// passed into CBuffer::NewL() later as these sizes
		if (size2!=0)	// remain zero otherwise
			size2+=sizeof(TPtr);	//sizeof(TPtr)==sizeof(TPtr8)==sizeof(TPtr16)
		}
	//
	//	Now create the wrapper class, which manages the circular buffer. 
	//
	CBuffer* buffer = CBuffer::NewL(buf8,buf16,aSession->PriorityClientHeap(messageType),size1,size2,noOfSlots);
	if (buf8)
		CleanupStack::Pop();	// pops HEtelBufC8 off cleanup stack
	if (buf16)
		CleanupStack::Pop();	// pops HEtelBufC16 off cleanup stack
	CleanupStack::PushL(buffer);	// since buffer is still not owned by anything
	//
	// Now create the request entry for this client request - even if it's not to be passed
	// down to the TSY in the case of MultipleCompletion.
	//
	reqEntry=PhoneOwner()->NewReqL(aMessage,aSession,buffer,this,aMessage.Function());
	CleanupStack::Pop();
	if (aReqMode&KReqModeMultipleCompletionEnabled)
				// this client has not currently got this request outstanding
				// but another client may have 
		{
		CReqEntry* previousReq=NULL;
		previousReq=PhoneOwner()->FindByIpcAndTelObject(aMessage.Function(),this);	// search in active list
		if (previousReq)
			{
			reqEntry->iTsyReqHandle=previousReq->iTsyReqHandle;
			reqEntry->iReqMode=aReqMode;
			PhoneOwner()->AddReqToActive(reqEntry);
			reqEntry=NULL;
			}
		else if (aReqMode&KReqModeFlowControlObeyed)
			{
			previousReq=PhoneOwner()->FindByIpcAndTelObjectInWaiting(aMessage.Function(),this);
			if (previousReq)
				{
				reqEntry->iTsyReqHandle=previousReq->iTsyReqHandle;
				reqEntry->iReqMode=aReqMode;
				PhoneOwner()->AddReqToWaiting(reqEntry);
				reqEntry=NULL;
				}
			}
		}
	return reqEntry;
	}

void CTelObject::GeneralReq(const RMessage2& aMessage,CTelSession* aSession,CReqEntry* aNewReqEntry)
//
// On any CTelObject based class, do the general processing around sequential and parallel
// request modes for the server, before calling the pure virtual Service in the class to do
// the actual command.
//
	{

	LOGTEXT2(_L8("CTelObject::GeneralReq - IPC=%d"),aMessage.Function());
	TInt type=aMessage.Int1();
	CReqEntry* reqEntry=NULL;
	TInt basicMsgType = type&(~(KUnicodeReq|KPriorityClientReq));
	switch(basicMsgType)
		{
	case EIsaNull:
	case EIsaDesTobeSet:
	case EIsaDesTobeRead:
	case EIsaDoubleDesTobeSet:
	case EIsaDoubleDesTobeRead:
	case EIsaNarrowAndUnicodeDoubleDesTobeSet:
	case EIsaNarrowAndUnicodeDoubleDesTobeRead:
	case EIsaNarrowDesToSetAndGet:
	case EIsaNarrowDesToGetUnicodeDesToSet:
		{
		TInt ipc=aMessage.Function();

		TReqMode reqMode=0;
		TRAPD(res,(reqMode=ReqModeL(ipc)));
		if (res)
			{
			aMessage.Complete(res);
			return;
			}
		if (aNewReqEntry==NULL)
			{
			TRAP(res,reqEntry=ReqAnalyserL(aMessage,aSession,reqMode));
			if (res)
				{
				aMessage.Complete(res);
				return;				
				}
			else 
				if (reqEntry==NULL) // there was no need to construct another reqEntry, 
						// or another reqEntry with identical IPC for a different client 
						// has been added inside ReqAnalyserL()

					return;
			reqEntry->iReqMode = reqMode;	// so ReqCompleted() needn't ask for it again
			}
		else
			reqEntry=aNewReqEntry;	// aNewReqEntry has just come from the waiting list
				
		if (reqMode&KReqModeFlowControlObeyed) // flow control obeyed
			{
			if (PhoneOwner()->FlowControl())
				PhoneOwner()->AddReqToWaiting(reqEntry);
			else 
				{
				PhoneOwner()->FlowControlInc();
				if (aNewReqEntry==NULL)
					PhoneOwner()->AddReqToActive(reqEntry);
				res=Service(aMessage,reqEntry);
				if (res!=KErrNone) // can not do the service to tsy properly
					{
					CompleteAndDestroyReq(reqEntry,res);
					PhoneOwner()->FlowControlDec(); // Dec() as service is being abandoned
					}
				}
			}
		else // Ignored 
			{
			PhoneOwner()->AddReqToActive(reqEntry);
			if (reqMode&KReqModeRePostImmediately)
				res=RegisterNotification(ipc);	// this tells the TSY the first time any client
												// calls a particular notification.
			if (res!=KErrNone)
				CompleteAndDestroyReq(reqEntry,res);
			else
				{	
				res=Service(aMessage,reqEntry);	// down to the TSY
				if (res!=KErrNone)
					CompleteAndDestroyReq(reqEntry,res);
				}
			}
		break;
		}
	case EIsaCancelMessage:
		// This is for Cancel Req - Int0 contains the original IPC 
		{
		TInt cancelIpc=aMessage.Int0();
		reqEntry=PhoneOwner()->FindClientInWaiting(aSession,aMessage.Int3(),cancelIpc);
		if(reqEntry!=NULL)				// Is it Waiting to be passed to the TSY?
			{
			CompleteAndDestroyReq(reqEntry,KErrCancel);	// If yes then complete it.
			aMessage.Complete(KErrNone);
			return;
			}
		reqEntry=PhoneOwner()->FindClientInActive(aSession,aMessage.Int3(),cancelIpc);
		if(reqEntry) // found in active list
			{
			TReqMode reqMode=0;
			TRAPD(res,(reqMode=ReqModeL(reqEntry->iFunction)));
			if (res)
				{
				reqEntry->iMessage.Complete(res);	// client cannot check return value of Cancel()
													// so use return value of original function call
				return;
				}
			if (reqMode&KReqModeRePostImmediately && reqEntry->iMessageNulled)	
				{
				aMessage.Complete(KErrNone);	// client has called Cancel on a notification
												// after it had completed to the client
				return;
				}
				// so now we know the client must have the request outstanding
			if (reqMode&KReqModeMultipleCompletionEnabled && !(reqEntry->iPlacedRequest))
				{
				CompleteAndDestroyReq(reqEntry,KErrCancel);	// the request hadn't been passed to the TSY
				aMessage.Complete(KErrNone);
				return;
				}
			// so the request is outstanding on the TSY
			if (reqEntry->iCancelFnCalled==FALSE)
				{
				reqEntry->iCancelFnCalled=ETrue;
				TInt status=CancelService(aMessage.Int0(),reqEntry->iTsyReqHandle);
				if(status!=KErrNone)
					CompleteAndDestroyReq(reqEntry,status);
				}
			}
		aMessage.Complete(KErrNone);
		}
		break;

	case EIsaCancelSubSession:
		// This is a special case for cancelling all asynchronous requests for this subsession
		{
		CancelSubSession(aSession,aMessage.Int3());
		aMessage.Complete(KErrNone);
		}
		break;

	default:
		PanicClient(EEtelPanicInvalidRequestType,aMessage);
		break;
		}
	}

EXPORT_C void CTelObject::ReqCompleted(const TTsyReqHandle aTsyReqHandle,const TInt aError)
//
// General complete function for all CTelObject derived classes
//
	{
	LOGTEXT3(_L8("CTelObject::ReqCompleted TsyHandle=%d with error %d"),aTsyReqHandle,aError);
	__ASSERT_ALWAYS(aTsyReqHandle!=TSY_HANDLE_INIT_VALUE,Fault(EEtelFaultNotRecognisedTsyHandle));
	TInt ipc=0;
	CReqEntry* updatedReqEntry = NULL;
	CReqEntry* nextPostedReqEntry = NULL;
	updatedReqEntry=PhoneOwner()->FindByTsyHandleAndPlacedRequest(aTsyReqHandle);
	__ASSERT_ALWAYS(updatedReqEntry!=NULL, Fault(EEtelFaultCompletionReceivedWithNoReqPackage));

	TInt error = ResolveError(updatedReqEntry->iSession,aError);		// set error as either low or high byte

	ipc=updatedReqEntry->iFunction;
	TReqMode reqMode = updatedReqEntry->iReqMode;
	TBool ret=EFalse;
	if (error!=KErrCancel)
		{
		//  Multiple-completion malarky.
		//	Don't copy data across to other buffers and complete their reqs if TSY knows about
		//  each client that has called it. In that case, 
		//  TSY will fill in the appropriate client's buffer and complete each separately.
		if (reqMode&KReqModeMultipleCompletionEnabled)
			PhoneOwner()->CheckAndCompleteAllActive(updatedReqEntry,reqMode,ipc,aError);

		if (reqMode&KReqModeRePostImmediately && error==KErrNone)
			nextPostedReqEntry = updatedReqEntry;					
		updatedReqEntry->iBuffer->IncWrite();
		}
	else	// if a cancel comes from the TSY, then if it is a notification 
			// there may be other clients who have also called the notification and who 
			// have entries in active list, so one of these must be re-posted.
		{
		if (reqMode&KReqModeMultipleCompletionEnabled)
			nextPostedReqEntry = PhoneOwner()->FindThisReqByAnotherClient(updatedReqEntry->iSession,updatedReqEntry->iMessage.Int3(),ipc,this);
		if (!nextPostedReqEntry)
			// then we don't want to post any other client's requests in place of this one
			{
			nextPostedReqEntry = PhoneOwner()->FindNonCancelledClientReq(updatedReqEntry->iSession,updatedReqEntry->iMessage.Int3(),ipc);
			__ASSERT_DEBUG(updatedReqEntry!=nextPostedReqEntry, Fault(EEtelFaultCancelErrorWithoutCancelled));
			if (nextPostedReqEntry==NULL && reqMode&KReqModeRePostImmediately)
				{
				TInt ret = DeregisterNotification(ipc);	// no more clients are interested so
														// tell TSY to stop looking at notifies
				if (ret!=KErrNone)
					error=ret;	// so the KErrCancel wouldn't reach the client
				}
			}
		}
	if (reqMode & KReqModeRePostImmediately)
		UpdateAndCompleteIfNecessary(updatedReqEntry,error);	// this will destroy the reqEntry
																// if an error occurred.
	else
		WriteBackAndCompleteReq(updatedReqEntry,error);
	if (nextPostedReqEntry)	// will be NULL if no following request for TSY
		Service(nextPostedReqEntry->iMessage,nextPostedReqEntry);
	
	if (!(reqMode&KReqModeFlowControlObeyed)) // If flow control not enabled, go home
		ret=ETrue;

// So everything below assumes it obeyed flow control
// Resume Flow control ...
	if (!ret)
		FlowControlResume();
//
// Check and destroying the dummy session
//
	CheckAndDestroyDummySubSession();
	}

TInt CTelObject::ResolveError(CTelSession* aSession, const TInt aError) const
	{
	if ((aError==KErrNone) || (aError==KErrCancel))
		return aError;

	TInt basicError;
	TInt extendedError;
	
	basicError = (aError | 0xFFFF0000);

	// Find out if client wants extended errors or not
	if (aSession->IsExpectingExtendedError())	
		{
		extendedError = (aError >> 16);
		extendedError = (extendedError | 0xFFFF0000);

		// Check that extended error information is available
		if (extendedError==KErrNotFound)
			return basicError;
		else
			return extendedError;
		}
	else
		return basicError;
	}

void CTelObject::UpdateAndCompleteIfNecessary(CReqEntry* aReqEntry,TInt aError) const
//
//	If client has this request outstanding on the server, complete it, otherwise set Read
//	flag to false. Destroy the request if an error has occurred.
//
	{
	if (aReqEntry->iMessageNulled)
		aReqEntry->iReadByClient=EFalse;	
	else 
		WriteBackAndCompleteBufferedReq(aReqEntry->iMessage,aReqEntry,aError);
	if (aError)
		DestroyReq(aReqEntry);
	}

void CTelObject::CheckAndDestroyDummySubSession()
	{
	if (iActiveReqCount)
		{
		iActiveReqCount--;
		LOGTEXT2(_L8("In CheckAndDestroyDummySubSession(), iActiveReqCount down to %d"),iActiveReqCount);
		if ((iActiveReqCount==0) && (iCreateDummy==FALSE))
			{
			__ASSERT_ALWAYS(iDestroyDummySubSession!=NULL,Fault(EEtelFaultBadTelSessionPointer));
			if (!(iDestroyDummySubSession->IsActive())) // only set going if not already active!
				iDestroyDummySubSession->Call();
			}
		}
	else if (iCreateDummy)
		{
		__ASSERT_ALWAYS(iDestroyDummySubSession!=NULL,Fault(EEtelFaultBadTelSessionPointer));
		if (!(iDestroyDummySubSession->IsActive())) // only set going if not already active!
			iDestroyDummySubSession->Call();
		iCreateDummy=EFalse;
		}
	}

void CTelObject::WriteBackAndCompleteBufferedReq(const RMessage2& aNewMessage,CReqEntry* aReqEntry,TInt aError) const
	{
	TInt basicMessageType =0; 
	basicMessageType = (aNewMessage.Int1() & ~KPriorityClientReq);
	switch (basicMessageType)
		{
	case EIsaNull:
		if (aReqEntry->iClientInterested)
			aNewMessage.Complete(aError);
		break;
	case EIsaDesTobeRead:
	case EIsaDoubleDesTobeRead:
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf8!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TDes8* dataDes1=BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TRAPD(ret,aNewMessage.WriteL(0,*dataDes1)); // if this fails, just carry on as normal. What else can we do??
			if (basicMessageType==EIsaDoubleDesTobeRead)
				{
				TDes8* dataDes2=BufferDes2(aReqEntry->iBuffer,CBuffer::ESlotRead);
				__ASSERT_ALWAYS(dataDes2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
				TRAP(ret,aNewMessage.WriteL(2,*dataDes2));
				}
			aNewMessage.Complete(aError);
			}
		break;
	case EIsaUnicodeDesTobeRead:
	case EIsaUnicodeDoubleDesTobeRead:
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf16!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			// The TDes16 will be at the start of the HBufC buffer
			TDes16* unicodeDataDes1=BufferDes1u(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(unicodeDataDes1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TRAPD(ret,aNewMessage.WriteL(0,*unicodeDataDes1));
			if (basicMessageType==EIsaUnicodeDoubleDesTobeRead)
				{
				TDes16* unicodeDataDes2=BufferDes2u(aReqEntry->iBuffer,CBuffer::ESlotRead);
				__ASSERT_ALWAYS(unicodeDataDes2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
				TRAP(ret,aNewMessage.WriteL(2,*unicodeDataDes2));
				}
			aNewMessage.Complete(aError);
			}
		}
		break;
	
	case EIsaNarrowAndUnicodeDoubleDesTobeRead:
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf16!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TDes8* dataDes1=BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TRAPD(ret,aNewMessage.WriteL(0,*dataDes1));
			TDes16* unicodeDataDes2=BufferDes2u(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(unicodeDataDes2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
			TRAP(ret,aNewMessage.WriteL(2,*unicodeDataDes2));
			aNewMessage.Complete(aError);
			}
		}
		break;

    case EIsaNarrowDesToSetAndGet:
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf8!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TDes8* dataDes2=BufferDes2(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
			TRAP_IGNORE(aNewMessage.WriteL(2,*dataDes2));
			aNewMessage.Complete(aError);
			}
		}
		break;

    case EIsaNarrowDesToGetUnicodeDesToSet:
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf8!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TDes8* dataDes1=BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes1!=NULL,Fault(EEtelFaultDes1DoesNotExist));
			TRAP_IGNORE(aNewMessage.WriteL(0,*dataDes1));
			aNewMessage.Complete(aError);
			}
		}
		break;

	case EIsaDesTobeSet:
	case EIsaDoubleDesTobeSet:
	case EIsaUnicodeDesTobeSet:
	case EIsaUnicodeDoubleDesTobeSet:
	case EIsaNarrowAndUnicodeDoubleDesTobeSet:
	case EIsaCancelMessage:
	case EIsaCancelSubSession:

	default:
		Fault(EEtelFaultBadMessageType);
		break;
		}
	if (aError==KErrNone || aError==KErrOverflow)
		{
		aReqEntry->iReadByClient=ETrue;
		aReqEntry->iMessageNulled=ETrue;
		aReqEntry->iBuffer->IncRead();
		}
	}

void CTelObject::WriteBackAndCompleteReq(CReqEntry* aReqEntry,const TInt aError) const
//
//	Complete a request entry and write data back to client
//
	{
	__ASSERT_ALWAYS(aReqEntry!=NULL,Fault(EEtelFaultCompleteReqWithoutReqEntry));
	LOGTEXT(_L8("CTelObject::WriteBackAndCompleteReq"));
	TInt basicMessageType = (aReqEntry->iMessage.Int1() & ~KPriorityClientReq);

	switch (basicMessageType)
		{
	case EIsaNull:
		aReqEntry->CompleteAndDeque(aError);
		break;
	case EIsaDesTobeSet:
	case EIsaDoubleDesTobeSet:
		__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf8!=NULL,Fault(EETelFaultRequestWithoutBuffer));
		aReqEntry->CompleteAndDeque(aError);
		break;
	case EIsaUnicodeDesTobeSet:
	case EIsaUnicodeDoubleDesTobeSet:
	case EIsaNarrowAndUnicodeDoubleDesTobeSet:
		__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf16!=NULL,Fault(EETelFaultRequestWithoutBuffer));
		aReqEntry->CompleteAndDeque(aError);
		break;
	case EIsaDesTobeRead:
	case EIsaDoubleDesTobeRead:		
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf8!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			// The TPtr8 will be at the start of the HBufC buffer
			TDes8* dataDes1=BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TRAPD(ret,aReqEntry->iMessage.WriteL(0,*dataDes1));
			if (basicMessageType==EIsaDoubleDesTobeRead)
				{
				TDes8* dataDes2=BufferDes2(aReqEntry->iBuffer,CBuffer::ESlotRead);
				TRAP(ret,aReqEntry->iMessage.WriteL(2,*dataDes2));
				}
			}
		aReqEntry->CompleteAndDeque(aError);
		}
		break;
	case EIsaUnicodeDesTobeRead:
	case EIsaUnicodeDoubleDesTobeRead:		
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf16!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TDes16* dataDes1=BufferDes1u(aReqEntry->iBuffer,CBuffer::ESlotRead);
			TRAPD(ret,aReqEntry->iMessage.WriteL(0,*dataDes1));
			if (basicMessageType==EIsaDoubleDesTobeRead)
				{
				TDes16* dataDes2=BufferDes2u(aReqEntry->iBuffer,CBuffer::ESlotRead);
				TRAP(ret,aReqEntry->iMessage.WriteL(2,*dataDes2));
				}
			}
		aReqEntry->CompleteAndDeque(aError);
		}
		break;
	case EIsaNarrowAndUnicodeDoubleDesTobeRead:
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf16!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TDes8* dataDes1=BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TRAPD(ret,aReqEntry->iMessage.WriteL(0,*dataDes1));
			TDes16* unicodeDataDes2=BufferDes2u(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(unicodeDataDes2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
			TRAP(ret,aReqEntry->iMessage.WriteL(2,*unicodeDataDes2));
			}
		aReqEntry->CompleteAndDeque(aError);
		}
		break;

    case EIsaNarrowDesToSetAndGet:
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf8!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			// only write back the second parameter		
			TDes8* dataDes2=BufferDes2(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes2!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TRAP_IGNORE(aReqEntry->iMessage.WriteL(2,*dataDes2));
			}
		aReqEntry->CompleteAndDeque(aError);
		}
		break;

    case EIsaNarrowDesToGetUnicodeDesToSet:
		{
		if (aReqEntry->iClientInterested)
			{
			__ASSERT_ALWAYS(aReqEntry->iBuffer->iBuf8!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			// only write back the first parameter		
			TDes8* dataDes1=BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotRead);
			__ASSERT_ALWAYS(dataDes1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
			TRAP_IGNORE(aReqEntry->iMessage.WriteL(0,*dataDes1));
			}
		aReqEntry->CompleteAndDeque(aError);
		}
		break;


	default:
		Fault(EEtelFaultBadMessageType);
		break;
		}
	delete aReqEntry;
	}

EXPORT_C void CTelObject::FlowControlSuspend()
//
// Suspend the Request Flow
//
	{
	PhoneOwner()->FlowControlInc();
	}

EXPORT_C void CTelObject::FlowControlResume()
//
// Resume the Request Flow
//
	{
 	PhoneOwner()->FlowControlDec();
 	if(PhoneOwner()->FlowControl()>0)			// check for outstanding request
		return;

	CReqEntry* reqEntry=PhoneOwner()->ActivateNextWaitingReq();
	if (reqEntry==NULL)
		return;

	RMessage2& message=reqEntry->iMessage;
	CTelObject* theObj=reqEntry->iSession->CObjectFromHandle(message.Int3());
	__ASSERT_ALWAYS(theObj!=NULL, Fault(EEtelFaultWaitingReqLostCObject));
	theObj->GeneralReq(message,reqEntry->iSession,reqEntry);
	}

//
//	Future proofing in server
//
EXPORT_C TInt CTelObject::DownCallOption(TInt /*aOptionNumber*/, TAny* /*aData*/)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt CTelObject::UpCallOption(TInt /*aOptionNumber*/, TAny* /*aData*/)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt CTelObject::ServerVersion() const
//
// Lets TSY know which version of server this is, ie which extra options the server will support.
//
	{
	return KETelServerVersion;
	}

TDes8* CTelObject::BufferDes1(const CBuffer* aBuffer,CBuffer::TWhichSlot aWhichSlot) const
//
//	Returns pointer to first descriptor in current slot. More specifically, a pointer to the
//	TPtr which is stored just before the data that it points to.

//  The buffer is circular, and there are
//	two "current" slots - one slot for where the next data should be written to by the TSY,
//	the other from which the client should next read from. aWhichSlot specifies which of these
//	should be returned.
//
	{
	TDes8* des1=NULL;
	if (aBuffer->iBuf8)
		{
		TUint8* pos = aBuffer->CurrentSlotData1(aWhichSlot);
		if (pos)
			des1=REINTERPRET_CAST(TDes8*,pos);
		}
	return des1;
	}

TDes8* CTelObject::BufferDes2(const CBuffer* aBuffer,CBuffer::TWhichSlot aWhichSlot) const 
	{
	TDes8* des2=NULL;
	if (aBuffer->iBuf8)
		{
		TUint8* pos = aBuffer->CurrentSlotData2(aWhichSlot);
		if (pos)
			des2=REINTERPRET_CAST(TDes8*,pos);
		}
	return des2;
	}

TDes16* CTelObject::BufferDes1u(const CBuffer* aBuffer,CBuffer::TWhichSlot aWhichSlot) const
	{
	TDes16* des1=NULL;
	if (aBuffer->iBuf16)
		{
		TUint16* pos = aBuffer->CurrentSlotData1u(aWhichSlot);
		if (pos)
			des1=REINTERPRET_CAST(TDes16*,pos);
		}
	return des1;
	}

TDes16* CTelObject::BufferDes2u(const CBuffer* aBuffer,CBuffer::TWhichSlot aWhichSlot) const 
	{
	TDes16* des2=NULL;
	if (aBuffer->iBuf16)
		{
		TUint16* pos = aBuffer->CurrentSlotData2u(aWhichSlot);
		if (pos)
			des2=REINTERPRET_CAST(TDes16*,pos);
		}
	return des2;
	}

TUint8* CTelObject::Ptr1(const TDes8* aDes1) const
//
// return the pointer to the data of the first descriptor
// return NULL if does not exist
//
	{
	TUint8* ptr1=NULL;
	if (aDes1)
		ptr1=CONST_CAST(TUint8*,aDes1->Ptr());
	return ptr1;
	}

void CTelObject::RemoveDummySubSessionDestroyer()
	{
	iDestroyDummySubSession = NULL;
	}
//
//
// CSubSessionExtBase
//
//
EXPORT_C CSubSessionExtBase::CSubSessionExtBase()
//
//	C'Tor
//
	{
	__DECLARE_NAME(_S("CSubSessionExtBase"));
	}

EXPORT_C CSubSessionExtBase::~CSubSessionExtBase()
//
//	D'Tor
//
	{}

TInt CSubSessionExtBase::ServiceExtFunc(const RMessage2& aMessage,CReqEntry* aReqEntry)
//
// Sort out Des1 and Des2 before calling ExtFunc()
//
	{
	__ASSERT_ALWAYS(aReqEntry!=NULL,Fault(EEtelFaultCallTsyServiceWithoutReqPackage));
	TTsyReqHandle tsyReqHandle=aReqEntry->iTsyReqHandle;

	LOGTEXT2(_L8("Entered ServiceExtFunc with TSY handle %d"),tsyReqHandle);
	TInt ret=KErrNone;

	TInt basicMessageType = aMessage.Int1() & ~(KPriorityClientReq);
	switch (basicMessageType)
		{
	case EIsaNull:
	case EIsaCancelMessage:
		{
		TDataPackage package;	// a package with all data pointers nulled
		ret=ExtFunc(tsyReqHandle,aMessage.Function(),package);
		}
		break;
	case EIsaDesTobeSet:
	case EIsaDesTobeRead:
	case EIsaDoubleDesTobeSet:
	case EIsaDoubleDesTobeRead:	// may have been multi-slot registered
	case EIsaNarrowDesToSetAndGet:
		{
		TDes8* des1 = BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotWrite);
		__ASSERT_ALWAYS(des1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
		TDes8* des2=NULL;
		if (basicMessageType==EIsaDoubleDesTobeSet || 
			basicMessageType==EIsaDoubleDesTobeRead ||
			basicMessageType==EIsaNarrowDesToSetAndGet)
			{
			des2=BufferDes2(aReqEntry->iBuffer,CBuffer::ESlotWrite);
			__ASSERT_ALWAYS(des2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
			}
		TDataPackage package(des1,des2);
		ret=ExtFunc(tsyReqHandle,aReqEntry->iFunction,package);
		}
		break;
	case EIsaUnicodeDesTobeSet:
	case EIsaUnicodeDesTobeRead:
	case EIsaUnicodeDoubleDesTobeSet:
	case EIsaUnicodeDoubleDesTobeRead:
		{
		TDes16* des1 = BufferDes1u(aReqEntry->iBuffer,CBuffer::ESlotWrite);
		__ASSERT_ALWAYS(des1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
		TDes16* des2=NULL;
		if (basicMessageType==EIsaUnicodeDoubleDesTobeSet || basicMessageType==EIsaUnicodeDoubleDesTobeRead)
			{
			des2 = BufferDes2u(aReqEntry->iBuffer,CBuffer::ESlotWrite);
			__ASSERT_ALWAYS(des2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
			}	
		TDataPackage package(des1,des2);
		ret=ExtFunc(tsyReqHandle,aReqEntry->iFunction,package);
		}
		break;
	case EIsaNarrowAndUnicodeDoubleDesTobeSet:
	case EIsaNarrowAndUnicodeDoubleDesTobeRead:
	case EIsaNarrowDesToGetUnicodeDesToSet:
		{
		TDes8* des1 = BufferDes1(aReqEntry->iBuffer,CBuffer::ESlotWrite);
		__ASSERT_ALWAYS(des1!=NULL,Fault(EETelFaultRequestWithoutBuffer));
		TDes16* des2=NULL;
		des2=BufferDes2u(aReqEntry->iBuffer,CBuffer::ESlotWrite);
		__ASSERT_ALWAYS(des2!=NULL,Fault(EEtelFaultDes2DoesNotExist));
		TDataPackage package(des1,des2);
		ret=ExtFunc(tsyReqHandle,aReqEntry->iFunction,package);
		}
		break;
	default:
		Fault(EEtelFaultMessageTypeCorrupted);
		break;
		}
	return ret;
	}

EXPORT_C TInt CSubSessionExtBase::Service(const RMessage2& aMessage, CReqEntry* aReqEntry)
//
// The ServiceL functionality for the ext base
//
	{
	aReqEntry->iPlacedRequest=ETrue;
	return ServiceExtFunc(aMessage,aReqEntry);
	}

EXPORT_C void CSubSessionExtBase::CloseSubSessionPreProcessing(CTelSession* aSession,const TInt aSubSessionHandle)
	{
	CreateDummySession(aSession,aSubSessionHandle);
	FlushReqs(aSession,aSubSessionHandle);
	}

EXPORT_C void CSubSessionExtBase::OpenPostProcessing(CTelSession*,const TInt)
//
// Perform post-processing after object has been added to session's CObjectIx.
//
	{}

EXPORT_C void CSubSessionExtBase::NullMethod1()
	{}

EXPORT_C void CFaxBase::NullMethod2()
	{}

//
//
// CLibUnloader
//
//
CLibUnloader::CLibUnloader()
//
// C'tor
//
	:CAsyncOneShot(CActive::EPriorityHigh)
	{
	__DECLARE_NAME(_S("CLibUnloader"));
	}

CLibUnloader* CLibUnloader::NewL(RLibrary &aLib)
//
// New Library Unloader
//
	{
	CLibUnloader *s=new(ELeave)CLibUnloader;
	s->iLib=aLib;
	return s;
	}

CLibUnloader::~CLibUnloader()
	{
	Cancel();
	iLib.Close();
	}

void CLibUnloader::RunL()
//
// Unload TSY module
//
	{
	delete this;
	}

//
//
// CPhoneFactoryBase definition
//
//
EXPORT_C TBool CPhoneFactoryBase::QueryVersionSupported( TVersion const & aVersion )const
//
// Pass through to default QVS
//
	{
	return(User::QueryVersionSupported(iVersion,aVersion));
	}

EXPORT_C CPhoneFactoryBase::CPhoneFactoryBase()
//
// C'Tor
//
	{
	__DECLARE_NAME(_S("CPhoneFactoryBase"));
	}

void CPhoneFactoryBase::ConstructL(RLibrary& aLib)
//
// Allocate a CLibrary Unloader
//
	{
	iLibUnloader=CLibUnloader::NewL(aLib);
	}

EXPORT_C CPhoneFactoryBase::~CPhoneFactoryBase()
//
// D'Tor
//
	{
	if (iLibUnloader)
		{
		LOGTEXT(_L8("About to call iLibUnloader"));
		iLibUnloader->Call();
		}
	}

EXPORT_C void CPhoneFactoryBase::CPhoneFactoryBase_Reserved1()
//
// Reserved virtual function
//
	{}

//
// HEtelBufC definitions
//


//
// HEtelBufC8
//
HEtelBufC8* HEtelBufC8::NewMaxLC(TInt aMaxLength,RHeap* aHeap)
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Fault(EEtelFaultMaxDesLengthNegative));

	HEtelBufC8* pH=NULL;
	TInt size=sizeof(HEtelBufC8)+(aMaxLength*sizeof(TUint8));
	if (aHeap!=NULL)
		pH=(HEtelBufC8*)aHeap->AllocLC(size);
	else
		pH=(HEtelBufC8*)User::AllocLC(size);
	pH->iLength=aMaxLength;
	pH->iHeap=aHeap;
	pH->Des().SetLength(aMaxLength);
	return(pH);
	}

TPtr8 HEtelBufC8::Des()
	{
	TUint8* ptr=REINTERPRET_CAST(TUint8*,(REINTERPRET_CAST(TInt,this)+sizeof(HEtelBufC8)));
	return TPtr8(ptr,iLength,iLength);
	}

const TUint8* HEtelBufC8::Ptr() const
	{
	return REINTERPRET_CAST(const TUint8*,(REINTERPRET_CAST(TInt,this)+sizeof(HEtelBufC8)));
	}
void HEtelBufC8::operator delete(TAny* aPtr)
//
// overloaded delete - deletes from priority client heap if iHeap is not NULL
//
	{
	RHeap* heap=NULL;
	if (aPtr!=NULL)
		heap=REINTERPRET_CAST(HEtelBufC8*,aPtr)->iHeap;

	if (heap!=NULL)
		REINTERPRET_CAST(HEtelBufC8*,aPtr)->iHeap->Free(aPtr);
	else
		User::Free(aPtr);
	}

//
// HEtelBufC16
//			  
HEtelBufC16* HEtelBufC16::NewMaxLC(TInt aMaxLength,RHeap* aHeap)
	{
	__ASSERT_ALWAYS(aMaxLength>=0,Fault(EEtelFaultMaxDesLengthNegative));

	HEtelBufC16* pH=NULL;
	TInt size=sizeof(HEtelBufC16)+(aMaxLength*sizeof(TUint16));
	if (aHeap!=NULL)
		pH=(HEtelBufC16*)aHeap->AllocLC(size);
	else
		pH=(HEtelBufC16*)User::AllocLC(size);
	pH->iLength=aMaxLength;
	pH->iHeap=aHeap;
	pH->Des().SetLength(aMaxLength);
	return(pH);
	}

TPtr16 HEtelBufC16::Des()
	{
	TUint16* ptr=REINTERPRET_CAST(TUint16*,(REINTERPRET_CAST(TInt,this)+sizeof(HEtelBufC16)));
	return TPtr16(ptr,iLength,iLength);
	}

const TUint16* HEtelBufC16::Ptr() const
	{
	return REINTERPRET_CAST(const TUint16*,(REINTERPRET_CAST(TInt,this)+sizeof(HEtelBufC16)));
	}

void HEtelBufC16::operator delete(TAny* aPtr)
//
// overloaded delete - deletes from priority client heap if iHeap is not NULL
//
	{
	RHeap* heap=NULL;
	if (aPtr!=NULL)
		heap=REINTERPRET_CAST(HEtelBufC16*,aPtr)->iHeap;

	if (heap!=NULL)
		REINTERPRET_CAST(HEtelBufC16*,aPtr)->iHeap->Free(aPtr);
	else
		User::Free(aPtr);
	}

//
//	TDataPackage class contains the data pointers to be passed down to the TSY in ExtFunc()
//
TDataPackage::TDataPackage()
	: iDes1(NULL),iDes2(NULL),iDes1u(NULL),iDes2u(NULL)
	{}
TDataPackage::TDataPackage(TDes8* aDes1, TDes8* aDes2)
	: iDes1(aDes1),iDes2(aDes2),iDes1u(NULL),iDes2u(NULL)
	{}

TDataPackage::TDataPackage(TDes16* aDes1, TDes16* aDes2)
	: iDes1(NULL),iDes2(NULL),iDes1u(aDes1),iDes2u(aDes2)
	{}
TDataPackage::TDataPackage(TDes8* aDes1, TDes16* aDes2)
	: iDes1(aDes1),iDes2(NULL),iDes1u(NULL),iDes2u(aDes2)
	{}

EXPORT_C TDes8* TDataPackage::Des1n() const
	{
	__ASSERT_ALWAYS(iDes1,Fault(EEtelFaultDes1DoesNotExist));
	return iDes1;
	}
EXPORT_C TDes8* TDataPackage::Des2n() const
	{
	__ASSERT_ALWAYS(iDes2,Fault(EEtelFaultDes2DoesNotExist));
	return iDes2;
	}
EXPORT_C TDes16* TDataPackage::Des1u() const
	{
	__ASSERT_ALWAYS(iDes1u,Fault(EEtelFaultDes1DoesNotExist));
	return iDes1u;
	}
EXPORT_C TDes16* TDataPackage::Des2u() const
	{
	__ASSERT_ALWAYS(iDes2u,Fault(EEtelFaultDes2DoesNotExist));
	return iDes2u;
	}

EXPORT_C TAny* TDataPackage::Ptr1() const
	{
	if (iDes1u)
		return REINTERPRET_CAST(TAny*,CONST_CAST(TUint16*,iDes1u->Ptr()));		
	if (iDes1)
		return REINTERPRET_CAST(TAny*,CONST_CAST(TUint8*,iDes1->Ptr()));
	return NULL;
	}

EXPORT_C TAny* TDataPackage::Ptr2() const
	{
	if (iDes2u)
		return REINTERPRET_CAST(TAny*,CONST_CAST(TUint16*,iDes2u->Ptr()));
	if (iDes2)
		return REINTERPRET_CAST(TAny*,CONST_CAST(TUint8*,iDes2->Ptr()));
	return NULL;
	}

