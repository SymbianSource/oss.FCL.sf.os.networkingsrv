// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// uscl_packetcontext.cpp - implementation of RPacketContext class
//
 
#include <ETelExt.h>
#include "us_cliserv.h"
#include "uscl_pcktcs.h"
#include "uscl_packet.h"
#include "uscl_packet_internal.h"

//********************************
//RPacketContext
//********************************

EXPORT_C RPacketContext::RPacketContext() : iData(NULL)
    {
	// nothing to do, constructing done in Open-variants
    }

EXPORT_C RPacketContext::~RPacketContext()
    {
	if(iData)
        {
		delete iData;
		iData = NULL;
        }
    }

EXPORT_C TInt RPacketContext::OpenNewContext(RPacketService& aPacketNetwork, TDes& aContextName)
    {
	RSessionBase* simServ = aPacketNetwork.iData->GetServer();

    TIpcArgs args( &aContextName, 
                   EUmtsSimServOpenNewPacketContext,
                   NULL );
    TInt err = CreateSubSession(*simServ, EUmtsSimServCreatePacketContextSubSession, args);
	if(err != KErrNone) return err;

	if(!iData)
        {
		TRAPD(r,iData = CPacketContextInternalData::NewL());
		if(r != KErrNone)
            {
			iData = NULL;
			return r;
            }
        }

	iData->iSimServ = simServ;
	iData->iName = aContextName;
	return KErrNone;
    }

EXPORT_C TInt RPacketContext::OpenNewSecondaryContext(RPacketService& aPacketNetwork,
													  const TDesC& aExistingContextName,
													  TDes& aNewContextName)
    {
	RSessionBase* simServ = aPacketNetwork.iData->GetServer();

    TIpcArgs args( &aNewContextName, 
                   EUmtsSimServOpenNewSecondaryPacketContext, 
                   &aExistingContextName );
    TInt err = CreateSubSession(*simServ, EUmtsSimServCreatePacketContextSubSession, args);
	if(err != KErrNone) return err;

	if(!iData)
        {
		TRAPD(r,iData = CPacketContextInternalData::NewL());
		if(r != KErrNone)
            {
			iData = NULL;
			return r;
            }
        }

	iData->iSimServ = simServ;
	iData->iName = aNewContextName;
	return KErrNone;
    }

EXPORT_C TInt RPacketContext::OpenExistingContext(RPacketService& aPacketNetwork,
												  const TDesC& aContextName)
    {
	RSessionBase* simServ = aPacketNetwork.iData->GetServer();

    TIpcArgs args( &aContextName, 
                   EUmtsSimServOpenExistingPacketContext, 
                   NULL );
    TInt err = CreateSubSession(*simServ, EUmtsSimServCreatePacketContextSubSession, args);
	if(err != KErrNone) return err;

	if(!iData)
        {
		TRAPD(r,iData = CPacketContextInternalData::NewL());
		if(r != KErrNone)
            {
			iData = NULL;
			return r;
            }
        }

	iData->iSimServ = simServ;
	iData->iName = aContextName;
	return KErrNone;
    }

EXPORT_C void RPacketContext::Close()
    {
	if(iData)
        {
		delete iData;
		iData = NULL;
        }

    RSubSessionBase::CloseSubSession(EUmtsSimServClosePacketContextSubSession);
    }

EXPORT_C void RPacketContext::InitialiseContext(TRequestStatus& aStatus, TDes8& aDataChannelPckg) const
    {
    TIpcArgs args( &aDataChannelPckg, 
                   &(iData->iName) );
	SendReceive(EUmtsSimServPacketContextInitialiseContextA, args, aStatus);
    }

EXPORT_C void RPacketContext::Delete(TRequestStatus& aStatus) const
    {
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
    SendReceive(EUmtsSimServPacketContextDeleteA, args, aStatus);
    }

EXPORT_C void RPacketContext::Activate(TRequestStatus& aStatus) const
    {
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
    SendReceive(EUmtsSimServPacketContextActivateA, args, aStatus);
    }

EXPORT_C void RPacketContext::Deactivate(TRequestStatus& aStatus) const
    {
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
    SendReceive(EUmtsSimServPacketContextDeactivateA, args, aStatus);
    }

EXPORT_C TInt RPacketContext::GetStatus(TContextStatus& aContextStatus) const
    {
    TPckg<TContextStatus> pckg(aContextStatus);

    TIpcArgs args( &pckg );
    return SendReceive(EUmtsSimServPacketContextGetStatusS, args);
    }

EXPORT_C void RPacketContext::NotifyStatusChange(TRequestStatus& aStatus,
										         TContextStatus& aContextStatus)
    {
	if(iData->iNotifyStatusChangePckg)
        {
		// in case there is already a request on we cancel it
		CancelAsyncRequest(EPacketContextNotifyStatusChange);
        }
	iData->iNotifyStatusChangePckg = new TPckg<TContextStatus> (aContextStatus);

    TIpcArgs args( iData->iNotifyStatusChangePckg );
    SendReceive(EUmtsSimServPacketContextNotifyStatusChangeA, args, aStatus);
    }

EXPORT_C void RPacketContext::SetConfig(TRequestStatus& aStatus, const TDesC8& aConfig) const
    {
    TIpcArgs args( &aConfig );
    SendReceive(EUmtsSimServPacketContextSetConfigA, args, aStatus);
    }

EXPORT_C void RPacketContext::GetConfig(TRequestStatus& aStatus, TDes8& aConfig) const
    {
    TIpcArgs args( &aConfig );
    SendReceive(EUmtsSimServPacketContextGetConfigA, args, aStatus);
    }

EXPORT_C void RPacketContext::NotifyConfigChanged(TRequestStatus& aStatus, TDes8& aConfig) const
    {
    TIpcArgs args( &aConfig );
    SendReceive(EUmtsSimServPacketContextNotifyConfigChangedA, args, aStatus);
    }

EXPORT_C void RPacketContext::EnumeratePacketFilters(TRequestStatus& aRequestStatus, TInt& aCount) const
    {
	if(iData->iEnumeratePacketFiltersPckg)
        {
		// in case there is already a request on we cancel it
		CancelAsyncRequest(EPacketContextEnumeratePacketFilters);
        }
	iData->iEnumeratePacketFiltersPckg = new TPckg<TInt> (aCount);

    TIpcArgs args( iData->iEnumeratePacketFiltersPckg );
    SendReceive(EUmtsSimServPacketContextEnumeratePacketFiltersA, args, aRequestStatus);
    }

EXPORT_C void RPacketContext::GetPacketFilterInfo(TRequestStatus& aRequestStatus,
												  TInt aIndex,
												  TDes8& aPacketFilterInfo) const
    {
    TIpcArgs args( &aPacketFilterInfo, 
                   aIndex );
    SendReceive(EUmtsSimServPacketContextGetPacketFilterInfoA, args, aRequestStatus);
    }

EXPORT_C void RPacketContext::AddPacketFilter(TRequestStatus& aRequestStatus,
											  const TDesC8& aPacketFilterInfo) const
    {
    TIpcArgs args( &aPacketFilterInfo );
    SendReceive(EUmtsSimServPacketContextAddPacketFilterA, args, aRequestStatus);
    }

EXPORT_C void RPacketContext::RemovePacketFilter(TRequestStatus& aRequestStatus, TInt aId) const
    {
    TIpcArgs args( aId );
    SendReceive(EUmtsSimServPacketContextRemovePacketFilterA, args, aRequestStatus);
    }

EXPORT_C void RPacketContext::ModifyActiveContext(TRequestStatus& aStatus) const
    {
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
	SendReceive(EUmtsSimServPacketContextModifyActiveContextA, args, aStatus);
    }

EXPORT_C void RPacketContext::CancelAsyncRequest(TInt aReqToCancel) const
    {
	if((aReqToCancel & KUmtsSimServRqstMask) != KUmtsSimServRqstPacketContext
       || (aReqToCancel & KUmtsSimServRqstCancelBit) == 0)
        {
		User::Invariant(); // should panic thread here with reasonable reason
		return;
        }

    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
	SendReceive(aReqToCancel, args);

    if(aReqToCancel == EUmtsSimServPacketContextNotifyStatusChangeACancel && iData->iNotifyStatusChangePckg)
        {
		delete iData->iNotifyStatusChangePckg;
		iData->iNotifyStatusChangePckg = NULL;
        }
	else if(aReqToCancel == EUmtsSimServPacketContextEnumeratePacketFiltersACancel &&
            iData->iEnumeratePacketFiltersPckg)
        {
		delete iData->iEnumeratePacketFiltersPckg;
		iData->iEnumeratePacketFiltersPckg = NULL;
        }
    }

// *** CPacketContextInternalData *** wraps data needed by RPacketContext in seperately allocated package

CPacketContextInternalData* CPacketContextInternalData::NewL()
    {
	CPacketContextInternalData* self = new (ELeave) CPacketContextInternalData();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CPacketContextInternalData::CPacketContextInternalData()
	: iNotifyStatusChangePckg(NULL), iEnumeratePacketFiltersPckg(NULL)
    {
	// ConstructL does the work
    }

CPacketContextInternalData::~CPacketContextInternalData()
    {
	if(iNotifyStatusChangePckg)
        {
		delete iNotifyStatusChangePckg;
		iNotifyStatusChangePckg = NULL;
        }
	if(iEnumeratePacketFiltersPckg)
        {
		delete iEnumeratePacketFiltersPckg;
		iEnumeratePacketFiltersPckg = NULL;
        }
    }

RSessionBase* CPacketContextInternalData::GetServer() const
    {
	return iSimServ;
    }

const TDesC& CPacketContextInternalData::GetName() const
    {
	return iName;
    }

// *** TContextConfigGPRS *** GPRS (UMTS?) configuration

EXPORT_C RPacketContext::TContextConfigGPRS::TContextConfigGPRS()
	: iPdpType(EPdpTypeIPv6), iPdpCompression(0), iAnonymousAccessReqd(ENotApplicable)
    {
	iExtensionId = TPacketDataConfigBase::KConfigGPRS;
    }



EXPORT_C RPacketContext::TDnsInfoV2::TDnsInfoV2()
	: TPacketBase(), iPrimaryDns(NULL), iSecondaryDns(NULL)
   /**
    * Constructor for TDnsInfoV2 class
    *
    * This class will hold DNS Server information, for primary and secondary servers
    *
    */
   	{
	iExtensionId = KETelExtPcktV2;
   	}


EXPORT_C RPacketContext::TProtocolConfigOptionV2::TProtocolConfigOptionV2() : TPacketBase(), iChallenge(NULL), iResponse(NULL), iId(0), iDnsAddresses(),iMiscBuffer(NULL)
	/**
	* Constructor for TProtocolConfigOption class
	* Constructor for TProtocolConfigOptionV2 class
	*
	* This class will hold authentication data encapsulated in TAuthInfo as well as further data that 
	* may be required for CHAP protocol authentication, such a challenge and response
	*
	*/
	{
		
	iExtensionId = KETelExtPcktV2;

	//
	// Initialise the TAuthInfo structure...
	//
	iAuthInfo.iProtocol = EProtocolNone;

	iAuthInfo.iUsername.Zero();
	iAuthInfo.iPassword.Zero();
	};

//SBLP

RPacketContext::CTFTMediaAuthorizationV3::CTFTMediaAuthorizationV3()
{
	iExtensionId = KETelExtPcktV3;
}

EXPORT_C RPacketContext::CTFTMediaAuthorizationV3* RPacketContext::CTFTMediaAuthorizationV3::NewL()
{
	CTFTMediaAuthorizationV3* self = new(ELeave) CTFTMediaAuthorizationV3;
	return self;
}

EXPORT_C RPacketContext::CTFTMediaAuthorizationV3::~CTFTMediaAuthorizationV3()
{
	iFlowIds.Reset();
	iFlowIds.Close();
}

EXPORT_C void RPacketContext::CTFTMediaAuthorizationV3::ExternalizeL(HBufC8*& aBuffer) const
{
/**
Serialize data to the buffer.
If aBuffer points to already allocated memory, client has to be aware that it won't have
access to this location anymore as method reallocates memory and updates aBuffer accordingly.
Therefore, value passed in should not be freed upon completion of the method neither put onto
the cleanup stack before method is invoked.

After completion of the API, it's clients responsibility to control life scope
of allocated memory passed back via aBuffer.

@param aBuffer Heap-based buffer which will contain the serialized data.
*/
	TUint uintLen=sizeof(TUint);
	TUint uintLen16 = sizeof(TUint16);
	TUint size= uintLen/* extension id*/ + uintLen /* Authorization Token length*/ + iAuthorizationToken.Length();
	TUint count=iFlowIds.Count();
	size+=uintLen; /** number of iFlowIds elements*/
	size+= count*(uintLen16+uintLen16);

	if(aBuffer)
		{
	 	delete aBuffer;
	 	aBuffer=NULL;
	 	}
	aBuffer=HBufC8::NewL(size);
	aBuffer->Des().SetLength(size);

	TInt cursor(0);

	Mem::Copy(const_cast<TUint8*>(aBuffer->Des().Ptr()+cursor),(const TUint8*)(&iExtensionId),uintLen);
	cursor+=uintLen;

	TUint len=iAuthorizationToken.Length();
	Mem::Copy(const_cast<TUint8*>(aBuffer->Des().Ptr()+cursor),&len,uintLen);
	cursor+=uintLen;
	Mem::Copy(const_cast<TUint8*>(aBuffer->Des().Ptr()+cursor),iAuthorizationToken.Ptr(),len);
	cursor+=len;

	Mem::Copy(const_cast<TUint8*>(aBuffer->Des().Ptr()+cursor),(const TUint8* )(&count),uintLen);
	cursor+=uintLen;

	for(TUint i=0;i<count;i++)
		{
		Mem::Copy((TUint16*)(aBuffer->Des().Ptr()+cursor),(const TUint16* )(&iFlowIds[i].iMediaComponentNumber),uintLen16);
		cursor+=uintLen16;

		Mem::Copy((TUint16*)(aBuffer->Des().Ptr()+cursor),(const TUint16* )(&iFlowIds[i].iIPFlowNumber),uintLen16);
		cursor+=uintLen16;
		}

}

EXPORT_C void RPacketContext::CTFTMediaAuthorizationV3::InternalizeL(TDes8& aBuffer)
{
	iFlowIds.Reset();

	TInt uintLen=sizeof(TUint);
	TInt cursor=0;

	if(aBuffer.Length() < cursor + uintLen)
		User::Leave(KErrOverflow);	

	Mem::Copy(&iExtensionId,aBuffer.Ptr(),uintLen);
	cursor+=uintLen; 
	
	if(aBuffer.Length() < cursor + uintLen)
		User::Leave(KErrOverflow);

	TInt len(0);
	Mem::Copy(&len,aBuffer.Ptr()+cursor,uintLen);
	cursor+=uintLen;
		
	if(aBuffer.Length() < cursor + len)
		User::Leave(KErrOverflow);
	iAuthorizationToken.Copy(aBuffer.MidTPtr(cursor,len));
	cursor+=len;


	if(aBuffer.Length() < cursor + uintLen)
		User::Leave(KErrOverflow);
	TUint count(0);
	Mem::Copy(&count,aBuffer.Ptr()+cursor,uintLen);
	cursor+=uintLen;

	RPacketContext::CTFTMediaAuthorizationV3::TFlowIdentifier flowIdentifier;
	
	TInt uintLen16 = sizeof(TUint16);
	for(TUint i=0;i<count;i++)
		{
		if(aBuffer.Length() < cursor + uintLen16 + uintLen16)
			User::Leave(KErrOverflow);
		Mem::Copy(&flowIdentifier.iMediaComponentNumber,aBuffer.Ptr()+cursor,uintLen16);
		cursor+=uintLen16;
		Mem::Copy(&flowIdentifier.iIPFlowNumber,aBuffer.Ptr()+cursor,uintLen16);
		cursor+=uintLen16;
		
		iFlowIds.Append(flowIdentifier);
		}
}
	
EXPORT_C TUint RPacketContext::CTFTMediaAuthorizationV3::ExtensionId() const
{
	return iExtensionId;	
}

EXPORT_C TTlvStructBase::TTlvStructBase(TPtr8& aPtr,TUint8 aFreeSpaceChar):iPtr(aPtr),iCursorPos(0),iFreeSpacePos(aPtr.Length()),iFreeSpaceChar(aFreeSpaceChar)
	{	
	TPtr8 tempPtr(iPtr);
	tempPtr.SetLength(iPtr.MaxLength());	
	TPtr8 ptr(tempPtr.MidTPtr(iFreeSpacePos,tempPtr.MaxLength()-iFreeSpacePos));
	ptr.Fill(iFreeSpaceChar);
	}
			
EXPORT_C void TTlvStructBase::ResetCursorPos()
	{
	iCursorPos=0;
	}


EXPORT_C TInt TTlvStructBase::AppendItemL(MTlvItemIdType& aId, MTlvItemDataLengthType& aDataLength,const TPtr8& aData)
	{		
	TPtr8 tempPtr(iPtr);
	tempPtr.SetLength(iPtr.MaxLength());
	
	TUint itemLen=aId.SerializedLength() + aDataLength.SerializedLength() +aData.Length();
	if((TInt)(tempPtr.MaxLength()-itemLen) < (TInt)iFreeSpacePos)
		return KErrOverflow;
	TPtr8 ptr(tempPtr.MidTPtr(iFreeSpacePos,aId.SerializedLength()));
	aId.ExternalizeL(ptr);	
	ptr.Set(tempPtr.MidTPtr(iFreeSpacePos+aId.SerializedLength(),aDataLength.SerializedLength()));
	
	aDataLength.SetDataLength(aData.Length());
	
	aDataLength.ExternalizeL(ptr);	
	ptr.Set(tempPtr.MidTPtr(iFreeSpacePos+aId.SerializedLength()+aDataLength.SerializedLength()));
	Mem::Copy((TUint8* )(ptr.Ptr()),aData.Ptr(),aData.Length());	
	iFreeSpacePos+=itemLen;
	iPtr.SetLength(iFreeSpacePos);	
	return KErrNone;
	}
	
EXPORT_C TInt TTlvStructBase::RemoveNextItemL(MTlvItemIdType& aIdToRemove,MTlvItemIdType& aId, MTlvItemDataLengthType& aDataLength)
	{
	TPtr8 tempPtr(iPtr);
	tempPtr.SetLength(iPtr.MaxLength());
	
	TBool found(EFalse);
	TUint pos=iCursorPos;
	TPtr8 ptr(0,0);
	while(!found && pos + aId.SerializedLength()+ aDataLength.SerializedLength() < iFreeSpacePos)
		{	
		ptr.Set(tempPtr.MidTPtr(pos,aId.SerializedLength()));
		aId.InternalizeL(ptr);			
		ptr.Set(tempPtr.MidTPtr(pos+aId.SerializedLength(),aDataLength.SerializedLength()));	
		aDataLength.InternalizeL(ptr);	
		if(aIdToRemove.IsEqual(aId))
			{								
			TUint itemLen=aId.SerializedLength()+aDataLength.SerializedLength()+aDataLength.DataLength();			
			if(pos+itemLen>tempPtr.Length())
				return KErrNotFound;								
			found=ETrue;			
			TUint nextItemPos=pos+itemLen;
			TUint8* dest= (TUint8* )(tempPtr.Ptr()+pos);
			TUint8* src= (TUint8* )(dest+itemLen);
			while(nextItemPos<iFreeSpacePos)
				{
				*dest++=*src++;
				nextItemPos++;
				}
			iFreeSpacePos-=itemLen;
			TPtr8 ptr(tempPtr.MidTPtr(iFreeSpacePos,tempPtr.MaxLength()-iFreeSpacePos));
			ptr.Fill(iFreeSpaceChar);
			}
		else
			{
			pos+=aId.SerializedLength()+aDataLength.SerializedLength()+aDataLength.DataLength();
			}
		}
	if(found)
		{
		iPtr.SetLength(iFreeSpacePos);	
	 	return KErrNone;
	 	}
	return KErrNotFound;
	}

EXPORT_C TInt TTlvStructBase::AnyNextItemL(MTlvItemIdType& aIdFound,TPtr8& aData,MTlvItemIdType& aId, MTlvItemDataLengthType& aDataLength)
	{	
	if(iCursorPos>=iFreeSpacePos)
		return KErrNotFound;
	
	if(iCursorPos+aId.SerializedLength() + aDataLength.SerializedLength()> iFreeSpacePos)
		return KErrNotFound;
	TPtr8 ptr(iPtr.MidTPtr(iCursorPos,aId.SerializedLength()));	
	aIdFound.InternalizeL(ptr);			
	
	ptr.Set(iPtr.MidTPtr(iCursorPos+aId.SerializedLength(),aDataLength.SerializedLength()));	
	aDataLength.InternalizeL(ptr);					
	
	if(iCursorPos+aId.SerializedLength() + aDataLength.SerializedLength() + aDataLength.DataLength() > iFreeSpacePos)
		return KErrNotFound;
	
	ptr.Set(iPtr.MidTPtr(iCursorPos+aId.SerializedLength()+aDataLength.SerializedLength(),aDataLength.DataLength()));	
	aData.Set(ptr);		
	iCursorPos+=aId.SerializedLength()+aDataLength.SerializedLength()+aDataLength.DataLength();				
	return KErrNone;
	}	

EXPORT_C TInt TTlvStructBase::NextItemL(const MTlvItemIdType& aWantedId,TPtr8& aData,MTlvItemIdType& aId, MTlvItemDataLengthType& aDataLength)
	{
	TBool found(EFalse);	
	TUint pos=iCursorPos;
	TPtr8 ptr(0,0);	
	while(!found && pos + aId.SerializedLength()+aDataLength.SerializedLength() <iFreeSpacePos)
		{			
		ptr.Set(iPtr.MidTPtr(pos,aId.SerializedLength()));	
		aId.InternalizeL(ptr);			
		ptr.Set(iPtr.MidTPtr(pos+aId.SerializedLength(),aDataLength.SerializedLength()));	
		aDataLength.InternalizeL(ptr);	
		if(aId.IsEqual(aWantedId))
			{		
			if(pos+aId.SerializedLength()+aDataLength.SerializedLength()+aDataLength.DataLength()>iFreeSpacePos)					
				return KErrNotFound;
			ptr.Set(iPtr.MidTPtr(pos+aId.SerializedLength()+aDataLength.SerializedLength(),aDataLength.DataLength()));	
			aData.Set(ptr);	
			found=ETrue;
			iCursorPos=pos+aId.SerializedLength()+aDataLength.SerializedLength()+aDataLength.DataLength();
			}
		else
			{
			pos+=aId.SerializedLength()+aDataLength.SerializedLength()+aDataLength.DataLength();
			}
		};
	if(found)
		{	
	 	return KErrNone;
	 	}
	return KErrNotFound;
	}


/** 
Default constructor.
*/
EXPORT_C RPacketContext::TPcoId::TPcoId():iId(0)
	{}

/** 
Constructor that initialises Id by provided value.

@param aId Id ofIE identifier.
*/	
EXPORT_C RPacketContext::TPcoId::TPcoId(TUint16 aId):iId(aId)
	{}

/** 
Sets PCO Id.

@param aId PCO Id to set.
*/	
EXPORT_C void RPacketContext::TPcoId::SetId(TUint16 aId)
	{
	iId=aId;
	}
	
/** 
Retrieves PCO Id.

@return PCO id.
*/
EXPORT_C TUint16 RPacketContext::TPcoId::Id() const
	{
	return iId;
	}
	
/** 
Compares whether the PCO Id of other PCoId is the same.

@param PCO Id to compare.
*/		
EXPORT_C TBool RPacketContext::TPcoId::IsEqual(const MTlvItemIdType& aOtherIdType) const
	{
	return iId==(static_cast<const TPcoId&>(aOtherIdType)).Id();
	}

/** 
Length of serialised data.

@return Length of id when serialized.
*/
EXPORT_C TUint RPacketContext::TPcoId::SerializedLength()const
	{
	return sizeof(TUint16);
	}

/** 
Serialise to the descriptor provided

Converts internally stored little-endian data 		
to big-endian encoded data as specified in 3GPP TS 24.008, table 10.5.154.

@param aData On completion contains serialized PCO item identifier.
*/		
EXPORT_C void RPacketContext::TPcoId::ExternalizeL(TDes8& aData) const
	{
	if(aData.Length()<2)
		User::Leave(KErrOverflow);
	aData[0]=(iId & 0xff00)>>8;
	aData[1]=iId & 0xff;	
	}

/** 
Internalize data from the buffer provided.

Converts internally stored little-endian data 		
to big-endian encoded data as specified in 3GPP TS 24.008, table 10.5.154.

@param aData Buffer containing PCO item Id to be internalized.
*/
EXPORT_C void RPacketContext::TPcoId::InternalizeL(TDesC8& aData)
	{
	if(aData.Length()<2)
		User::Leave(KErrUnderflow);
	iId=aData[1];
	iId|=aData[0]<<8;	
	}
	
/** 
Constructor initialise length by provided value.

@param aLen Length of PCO item.
*/
EXPORT_C RPacketContext::TPcoItemDataLength::TPcoItemDataLength(TUint8 aLen):iDataLength(aLen)
	{}

/** 
Default constructor.
*/		
EXPORT_C RPacketContext::TPcoItemDataLength::TPcoItemDataLength():iDataLength(0)
	{}
	
/** 
Sets length of associated data.

@param aLength Length of PCO item.
*/	
EXPORT_C void RPacketContext::TPcoItemDataLength::SetDataLength(TUint aLength)
	{
	iDataLength=(TUint8)aLength;
	}

/** 
Length when serialised.

@return Seriliazed length.
 */	
EXPORT_C TUint RPacketContext::TPcoItemDataLength::SerializedLength() const
	{
	return sizeof(TUint8);
	}

/** 
Length of associated item data.

@return Length of PCO item.
*/		
EXPORT_C TUint RPacketContext::TPcoItemDataLength::DataLength() const
	{
	return iDataLength;
	};

/** 
Serialize data member into provoded descriptor.

@param aData buffer into which length will be serialized.
*/	
EXPORT_C void RPacketContext::TPcoItemDataLength::ExternalizeL(TDes8& aData) const
	{
	if(aData.Length()<1)
		User::Leave(KErrOverflow);
	aData[0]=iDataLength;
	}

/** 
Deserialize data from provided buffer

@param aData Buffer containing length to be internalized.
*/
EXPORT_C void RPacketContext::TPcoItemDataLength::InternalizeL(TDesC8& aData)
	{	
	if(aData.Length()<1)
		User::Leave(KErrUnderflow);
	iDataLength=aData[0];
	}
	



EXPORT_C void PanicClient(TInt aFault)
/**
Panics the client on client side.

@internalComponent
*/
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
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(aIpc!=aIpcToCancel,PanicClient(KErrArgument));
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	return SendReceive(aIpc,TIpcArgs(aIpcToCancel,EIsaCancelMessage));
	}

EXPORT_C TInt RTelSubSessionBase::CancelSubSession() const
//
//	Set up a cancel request before pass to server
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	return SendReceive(EEtelCancelSubSession,TIpcArgs(TIpcArgs::ENothing, EIsaCancelSubSession));
	}

EXPORT_C void RTelSubSessionBase::CancelAsyncRequest(TInt aReqToCancel) const
/**
@capability None
*/
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
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set (0,TIpcArgs::ENothing);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNull | KPriorityClientReq));
	else
		args.Set(1,EIsaNull);
	return SendReceive(aIpc, args);
	}

EXPORT_C void RTelSubSessionBase::Blank(const TInt aIpc,TRequestStatus& aStatus,TReqPriorityType aType) const
//
//	Set up a Null ASYNC request before pass to server
//	There is no info passing between client/server
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,TIpcArgs::ENothing);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNull | KPriorityClientReq));
	else
		args.Set(1,(EIsaNull));
	SendReceive(aIpc,args,aStatus);
	}


EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC8& aDes,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set (0, CONST_CAST(TDesC8*,&aDes));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaDesTobeSet);
	return SendReceive(aIpc, args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC8*,&aDes));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaDesTobeSet);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes8& aDes,TReqPriorityType aType) const
//
//	Set up an SYNC request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaDesTobeRead);
	
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,TDes8& aDes,TReqPriorityType aType) const
//
//	Set up an async request which the client provide a Des in p[0]
//	Server will write back data to client address space in aPtr
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaDesTobeRead);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,
								  TDes8& aDes1,TDes8& aDes2,TReqPriorityType aType) const
//
//	Set up a asynch request - server write data back into descriptors P0 and P2
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);

	if (aType == EIsaPriorityRequest)
		args.Set(1,EIsaDoubleDesTobeRead | KPriorityClientReq);
	else
		args.Set(1,EIsaDoubleDesTobeRead);
	args.Set(2,&aDes2);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes8& aDes1,TDes8& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request - server write data back into descriptors P0 and P2
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaDoubleDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaDoubleDesTobeRead);
	args.Set(2,&aDes2);
	return SendReceive(aIpc,args);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC8& aDes1,const TDesC8& aDes2,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC8*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaDoubleDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaDoubleDesTobeSet);
	args.Set(2,CONST_CAST(TDesC8*,&aDes2));
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes1,
									const TDesC8& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC8*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaDoubleDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaDoubleDesTobeSet);
	args.Set(2,CONST_CAST(TDesC8*,&aDes2));
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC16& aDes,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC16*,&aDes));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDesTobeSet);
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC16& aDes,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC16*,&aDes));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDesTobeSet);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes16& aDes,TReqPriorityType aType) const
//
//	Set up an SYNC request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0, &aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDesTobeRead);
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,TDes16& aDes,TReqPriorityType aType) const
//
//	Set up an async request which the client provide a Des in p[0]
//	Server will write back data to client address space in aPtr
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDesTobeRead);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,
								  TDes16& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a asynch request - server write data back into descriptors P0 and P2
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDoubleDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDoubleDesTobeRead);
	args.Set(2,&aDes2);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes16& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request - server write data back into descriptors P0 and P2
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDoubleDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDoubleDesTobeRead);
	args.Set(2,&aDes2);
	return SendReceive(aIpc,args);
	}
	 
EXPORT_C void RTelSubSessionBase::Get(const TInt aIpc,TRequestStatus& aStatus,
								  TDes8& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a asynch request - server write data back into descriptors P0 and P2
//	P0 is a narrow descriptor, P2 is a unicode descriptor
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNarrowAndUnicodeDoubleDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaNarrowAndUnicodeDoubleDesTobeRead);
	args.Set(2,&aDes2);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Get(const TInt aIpc,TDes8& aDes1,TDes16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request - server write data back into descriptors P0 and P2
//	P0 is a narrow descriptor, P2 is a unicode descriptor
//
/**
@capability None
*/
	{
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNarrowAndUnicodeDoubleDesTobeRead | KPriorityClientReq));
	else
		args.Set(1,EIsaNarrowAndUnicodeDoubleDesTobeRead);
	args.Set(2,&aDes2);
	return SendReceive(aIpc,args);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC16& aDes1,const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC16*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDoubleDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDoubleDesTobeSet);
	args.Set(2,CONST_CAST(TDesC16*,&aDes2));
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC16& aDes1,
									const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC16*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDoubleDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDoubleDesTobeSet);
	args.Set(2,CONST_CAST(TDesC16*,&aDes2));
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc,const TDesC8& aDes1,const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a SYNCH. request which the client provide a Des in p[0] for server to Set
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC8*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNarrowAndUnicodeDoubleDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaNarrowAndUnicodeDoubleDesTobeSet);
	args.Set(2,CONST_CAST(TDesC16*,&aDes2));
	return SendReceive(aIpc,args);
	}

EXPORT_C void RTelSubSessionBase::Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes1,
									const TDesC16& aDes2,TReqPriorityType aType) const
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set and a Des in p[1] to Get
//
/**
@capability None
*/
	{
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC8*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNarrowAndUnicodeDoubleDesTobeSet | KPriorityClientReq));
	else
		args.Set(1,EIsaNarrowAndUnicodeDoubleDesTobeSet);
	args.Set(2,CONST_CAST(TDesC16*,&aDes2));
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C void RTelSubSessionBase::SetAndGet(const TInt aIpc, TRequestStatus& aStatus, const TDesC8& aDes1, TDes8& aDes2, TReqPriorityType aType) const
/**
@capability None
*/
	{
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set
//
	
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC8*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNarrowDesToSetAndGet | KPriorityClientReq));
	else
		args.Set(1,EIsaNarrowDesToSetAndGet);
	args.Set(2,&aDes2);
	SendReceive(aIpc,args, aStatus);
	}

EXPORT_C void RTelSubSessionBase::SetAndGet(const TInt aIpc, TRequestStatus& aStatus, TDes8& aDes1, const TDesC16& aDes2, TReqPriorityType aType) const
/**
@capability None
*/
	{
//
//	Set up a synch request which the client provide a Des in p[0] for server to Get and a Des in p[1] to Set
//
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,&aDes1);
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaNarrowDesToGetUnicodeDesToSet | KPriorityClientReq));
	else
		args.Set(1,EIsaNarrowDesToGetUnicodeDesToSet);
	args.Set(2,CONST_CAST(TDesC16*,&aDes2));
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C void RTelSubSessionBase::SetAndGet(const TInt aIpc, TRequestStatus& aStatus, const TDesC16& aDes1, TDes16& aDes2, TReqPriorityType aType) const
/**
@capability None
*/
	{	
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set and a Des in p[1] to Get
//
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC16*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDesToSetAndGet | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDesToSetAndGet);
	args.Set(2, &aDes2);
	SendReceive(aIpc,args, aStatus);
	}

EXPORT_C void RTelSubSessionBase::SetAndGet(const TInt aIpc, TRequestStatus& aStatus, const TDesC8& aDes1, TDes16& aDes2, TReqPriorityType aType) const
/**
@capability None
*/
	{	
//
//	Set up a synch request which the client provide a Des in p[0] for server to Set and a Des in p[1] to Get
//
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));
	TIpcArgs args;
	args.Set(0,CONST_CAST(TDesC8*,&aDes1));
	if (aType == EIsaPriorityRequest)
		args.Set(1,(EIsaUnicodeDesToGetNarrowDesToSet | KPriorityClientReq));
	else
		args.Set(1,EIsaUnicodeDesToGetNarrowDesToSet);
	args.Set(2,&aDes2);
	SendReceive(aIpc,args,aStatus);
	}

EXPORT_C TInt RTelSubSessionBase::Set(const TInt aIpc, const RFile& aFile, TReqPriorityType aType) const
/**
@capability None
*/
	{	
//
//	Set up a synch request which the client provides an RFile handle for the server to Set.
//	Slots 0 and 2 are used to transfer the session and file handles respectively.
//
	__ASSERT_ALWAYS(&(SessionHandle()),PanicClient(EEtelPanicNullHandle));	
	TIpcArgs args;
	aFile.TransferToServer(args, 0, 2);
	if (aType == EIsaPriorityRequest) //use EIsaNull as we don't want to set up any buffers for the args in CTelObject::ReqAnalyserL
		args.Set(1,(EIsaNull | KPriorityClientReq)); 
	else
		args.Set(1,(EIsaNull));

	return SendReceive(aIpc,args);	
	}

//
// RTelSubSessionBase
//
inline RSessionBase& RTelSubSessionBase::SessionHandle() const
/**
@publishedAll
@released
*/
	{ 
	return *iTelSession;
	}

inline void RTelSubSessionBase::SetSessionHandle(RSessionBase& aTelSession)
/**
@publishedAll
@released
*/
	{ 
	iTelSession=&aTelSession;
	}

inline TInt RTelSubSessionBase::SubSessionHandle()
/**
@publishedAll
@released
*/
	{
	return(RSubSessionBase::SubSessionHandle());
	}
