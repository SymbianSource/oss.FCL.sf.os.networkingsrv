// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef _USCL_PACKET_H__
#define _USCL_PACKET_H__

#include <e32base.h>
#include <f32file.h>


// SBLP

const TInt KErrEtelGsmBase = -4000;
const TInt KErrGsmRadioResourceBase=KErrEtelGsmBase;
const TInt KErrGsmMobilityManagementBase=KErrGsmRadioResourceBase-128;
const TInt KErrGprsUserAuthenticationFailure=KErrGsmMobilityManagementBase-29;


//
/* following classes define interface to  TLV ( TYPE- LENGTH-VALUE) structured data:
-------------------------------------------------------------------------------------------------
|       |                         |             |        |                          |           |
|ItemId	| Length Of The Item Data |	Item Data	| ItemId |	Length Of The Item Data	| Item Data |
|       |                         |             |        |                          |           |
-------------------------------------------------------------------------------------------------
*/

/**
Defines interface for specifying the Tag of a TLV object.

@publishedPartner
@released
*/
class MTlvItemIdType
{
public:
/**Externalize object by serializing to provided descriptor*/
	virtual void ExternalizeL(TDes8& aBuffer) const =0;
/** Internalize object by de-serializing of data in provided buffer*/
	virtual void InternalizeL(TDesC8& aBuffer)=0;
/** The length of serialized data member */
	virtual TUint SerializedLength() const =0;
/**  compares whether two objects contains the same data*/
	virtual TBool IsEqual(const MTlvItemIdType&) const=0;
};

/**
Defines interface for specifying the Length of a TLV object.

@publishedPartner
@released
*/
class MTlvItemDataLengthType
{
public:
/**Externalize object by serializing to provided descriptor*/
	virtual void ExternalizeL(TDes8& aBuffer)const=0;
/** Internalize object by de-serializing of data in provided buffer*/
	virtual void InternalizeL(TDesC8& aBuffer)=0;
/** The length of serialized data member */
	virtual TUint SerializedLength() const=0;
/** Sets length of the data it relates to*/
	virtual void SetDataLength(TUint)=0;
/** Gets length of the data it relates to*/
	virtual TUint DataLength() const =0;
};

/**
Provides methods to append, remove or perform iterative lookup for items in container buffer.
Classes ItemIdType and ItemDataLengthType have to implement interfaces MTlvItemIdType and
MTlvItemDataLengthType in order to enable proper encoding and decoding of the first two fields
in the unit.

@internalComponent
@released
*/
class TTlvStructBase
    {
  protected:
  	/** Default constructor initializes data members*/
   	IMPORT_C TTlvStructBase(TPtr8&,TUint8);    
 	/** Base class implementation of methods in the templated class*/
   	IMPORT_C TInt AppendItemL(MTlvItemIdType& aId,MTlvItemDataLengthType& aDataLengthType,const TPtr8& aData);
   	IMPORT_C TInt RemoveNextItemL(MTlvItemIdType& aIdToRemove,MTlvItemIdType& aId, MTlvItemDataLengthType& aDataLength);
   	IMPORT_C TInt AnyNextItemL(MTlvItemIdType& aIdFound,TPtr8& aData,MTlvItemIdType& aId, MTlvItemDataLengthType& aDataLength);
   	IMPORT_C TInt NextItemL(const MTlvItemIdType& aWantedId,TPtr8& aData,MTlvItemIdType& aId, MTlvItemDataLengthType& aDataLength);
  public:
	/** Sets position of the cursor to start position (0)*/
	IMPORT_C void ResetCursorPos();

  protected:
    /** Reference to external buffer that holds encoded TLV data*/
    TPtr8& iPtr;
    /** Cursor indicates last accessed item in the buffer*/
	TUint iCursorPos;
	/** Position in the buffer that indicates start of the zone that hasn't been assigned to any element.
	 	this free zone ends at the end of the buffer*/
	TUint iFreeSpacePos;
	/** Character used to populate the zone that hasn't been assigned to any element.
	this free zone ends at the end of the buffer*/
	TUint8 iFreeSpaceChar;
    };

/**
Provides methods to append, remove or perform iterative lookup for items in container buffer.
Classes ItemIdType and ItemDataLengthType have to implement interfaces MTlvItemIdType and 
MTlvItemDataLengthType in order to enable proper encoding and decoding of the first two fields 
in the unit. 

@publishedPartner
@released
*/  
template <class ItemIdType, class ItemDataLengthType>
class TTlvStruct: public TTlvStructBase
    {
 public:
 	
   	inline TTlvStruct(TPtr8&,TUint8);   
	inline TInt NextItemL(ItemIdType aId,TPtr8& aData);
	inline TInt AnyNextItemL(ItemIdType& aId, TPtr8& aData);
	inline TInt AppendItemL(ItemIdType aId,const TPtr8& aData);
	inline TInt RemoveNextItemL(ItemIdType aId);
	
 protected:
	/** Default constructor is protected in order to enforce proper initialization of reference to external data buffer via provided public constructor*/
    TTlvStruct();    	
    /** Type of the identifier*/
    ItemIdType iItemIdType;
    /** The type used to define length of data portion of the item*/ 
    ItemDataLengthType 	iItemDataLengthType;     
};

/** 
Default constructor initializes data members and cursor position to 0.

@param aPtr Ptr descriptor to TLV buffer that is to be read or written to.
@param aFreeSpaceChar Character used to populate the zone that hasn't been assigned to any element.
*/
template <class ItemIdType,class ItemDataLengthType>
	TTlvStruct<ItemIdType,ItemDataLengthType>::TTlvStruct(TPtr8& aPtr,TUint8 aFreeSpaceChar):TTlvStructBase(aPtr,aFreeSpaceChar)
	{	
	}
	
/**
Look up in the buffer for an item with specified identifier.
Look-up starts from the position of the cursor; 					
Returns KErrNone if the item is present in the buffer, KErrNotFound otherwise.
Sets supplied pointer so that it has length of item's data portion and points to it.
Internal cursor is moved to first position after the end of the found item 
(subsequent item start position in the buffer).

@param aId Id of item to find.
@param aData Descriptor which will hold the found item.
@return System-wide error code.. If item of requested Id was not found then KErrNotFound will be returned. 
*/
template <class ItemIdType, class ItemDataLengthType>
TInt TTlvStruct<ItemIdType,ItemDataLengthType>::NextItemL(ItemIdType aId,TPtr8& aData)
	{
	return TTlvStructBase::NextItemL(aId,aData,iItemIdType,iItemDataLengthType);	
	}

/**
Look up in the buffer for the item with specified identifier.
Look-up starts from the position of the cursor in the buffer. 
			
Returns KErrNone if item is found, KErrNotFound otherwise (end of buffer is reached).
Sets supplied pointer so that it points to item data portion and has length set to value of data length. 
Internal cursor is moved to first position after the end of the found item (subsequent item start position in the buffer).

@param aId Id of found item.
@param aData Descriptor which will hold the found item.
@return System-wide error code.. If no next item found then KErrNotFound will be returned. 
*/
template <class ItemIdType, class ItemDataLengthType>
TInt TTlvStruct<ItemIdType,ItemDataLengthType>::AnyNextItemL(ItemIdType& aId,TPtr8& aData)
	{
	return TTlvStructBase::AnyNextItemL(aId,aData,iItemIdType,iItemDataLengthType);
	}		
	
/**
Removes item identified by specified identifier (aId) from the buffer, where look-up starts at current cursor position, or 0 if it's reset. 
returns KErrNone if item is found ( and removed), otherwise error code - in the case where there is no more space in the assigned buffer, KErrOverflow is passed back. 

@param aId Id of item to remove.
@return System-wide error code.. If item of requested Id was not found then KErrNotFound will be returned. 
*/	
template <class ItemIdType, class ItemDataLengthType>
TInt TTlvStruct<ItemIdType,ItemDataLengthType>::RemoveNextItemL(ItemIdType aId)
	{
	return TTlvStructBase::RemoveNextItemL(aId,iItemIdType,iItemDataLengthType);	
	}
	
/**
Adds item identified by supplied aId argument to the buffer; content of the item is copied from provided descriptor to the buffer. 
Supplied item identifier (aId) and length of the descriptor are used to set item identifier field and length field at the start of 
item unit within the buffer.
Returns KErrNone if successful, error code otherwise.	
Internal cursor is moved to first position after the end of the found item (subsequent item start position in the buffer).

@param aId Id of item to add.
@param aData Descriptor containing data to add.
@return System-wide error code.. If size of item to be appended is greater than free space in buffer then KErrOverflow will be returned.
*/	
template <class ItemIdType, class ItemDataLengthType>
TInt TTlvStruct<ItemIdType,ItemDataLengthType>::AppendItemL(ItemIdType aId,const TPtr8& aData)
	{
	return TTlvStructBase::AppendItemL(aId,iItemDataLengthType,aData);
	}


//**********************************
// RUmtsSimServ
//**********************************
// The client class which is used instead of RPhone instance with RPacketService.
// The connect function starts the server, if it not already running.

class RUmtsSimServ : public RSessionBase
    {
    public:
      IMPORT_C RUmtsSimServ();
      IMPORT_C TInt Connect();
      IMPORT_C TVersion Version() const;
      IMPORT_C void Close();
    };


//**********************************
// (Utilities)
//**********************************

// TPacketDataConfigBase base class.
class TPacketDataConfigBase
   {
public:
	/**
	Flags identifying the different packet networks.
	*/
	enum 
		{
		/**
		GPRS Release 97/98 packet network.
		*/
		KConfigGPRS= 0x01,				
		/**
		CDMA and CDMA2000 packet networks.
		*/
		KConfigCDMA = 0x02,		
		/**
		GPRS/UMTS Release 99 and UMTS Release 4 networks.
		*/ 
		KConfigRel99Rel4 = 0x04,
		/**
		UMTS/IMS 3GPP Release 5 networks.
		*/ 
		KConfigRel5 = 0x05,		
		};
		
	/** This member returns the type of class.
	
	@return The type of class. */
	inline TInt ExtensionId(){return iExtensionId;};
protected:
	/**
	Defines the type of class: either KConfigGPRS, KConfigCMDA or KConfigRel99Rel4, 
	which identifies the type of packet network (GPRS R97/98, CDMA, Release 99 or 4 or 5)
	*/
	TInt iExtensionId;	
	};

// Packet API version, used here as extension identifiers for TPacketBase derived classes
// (Note: Values are intentionally decimal!)
enum { KETelExtPcktV1 = 4000, KETelExtPcktV2 = 8000, KETelExtPcktV3 = 12000 };


// TPacketBase base class.
class TPacketBase
    {
    public:
      inline TInt ExtensionId(){return iExtensionId;};
    protected:
      TInt iExtensionId; // identifies packet api version (for example KETelExtPcktV2)
    };


/**
Used by extension APIs to offset their cancel IPC values
*/
#define EMobileCancelOffset					500

//
// None
//
enum
	{
	EEtelOpenFromSession = 0,                                    // 0
	EEtelOpenFromSubSession,                                     // 1
	EEtelOpenByNameFromSession,                                  // 2
	EEtelOpenByNameFromSubSession,                               // 3
	EEtelClose,                                                  // 4
	EEtelCancelSubSession,                                       // 5
	EETelLineCapsChangeNotification,                             // 6
	EETelLineCapsChangeNotificationCancel,                       // 7
	EETelPhoneCapsChangeNotification,                            // 8
	EETelPhoneCapsChangeNotificationCancel,                      // 9
	EEtelCallCapsChangeNotification,                             // 10
	EEtelCallCapsChangeNotificationCancel,                       // 11
	EEtelCallGetBearerServiceInfo,                               // 12
	EEtelCallGetCallDuration,                                    // 13
	EEtelCallGetCallParams,                                      // 14
	EEtelCallGetCaps,                                            // 15
	EEtelCallGetFaxSettings,                                     // 16
	EEtelCallGetInfo,                                            // 17
	EEtelCallGetOwnershipStatus,                                 // 18
	EEtelCallGetStatus,                                          // 19
	EEtelCallNotifyDurationChange,                               // 20
	EEtelCallNotifyDurationChangeCancel,                         // 21
	EEtelCallNotifyHookChange,                                   // 22
	EEtelCallNotifyHookChangeCancel,                             // 23
	EEtelCallNotifyStatusChange,                                 // 24
	EEtelCallNotifyStatusChangeCancel,                           // 25
	EEtelCallReferenceCount,                                     // 26
	EEtelDbgCheckHeap,                                           // 27
	EEtelDbgDoDumpDebugInfo,                                     // 28
	EEtelDbgFailNext,                                            // 29
	EEtelDbgMarkEnd,                                             // 30
	EEtelDbgMarkHeap,                                            // 31
	EEtelDbgSetDebugPrintMask,                                   // 32
	EEtelFaxWaitForEndOfPage,                                    // 33
	EEtelLineEnumerateCall,                                      // 34
	EEtelLineGetCallInfo,                                        // 35
	EEtelLineGetCaps,                                            // 36
	EEtelLineGetHookStatus,                                      // 37
	EEtelLineGetInfo,                                            // 38
	EEtelLineGetStatus,                                          // 39
	EEtelLineNotifyCallAdded,                                    // 40
	EEtelLineNotifyCallAddedCancel,                              // 41
	EEtelLineNotifyHookChange,                                   // 42
	EEtelLineNotifyHookChangeCancel,                             // 43
	EEtelLineNotifyStatusChange,                                 // 44
	EEtelLineNotifyStatusChangeCancel,                           // 45
	EEtelPhoneEnumerateLines,                                    // 46
	EEtelPhoneGetCaps,                                           // 47
	EEtelPhoneGetInfo,                                           // 48
	EEtelPhoneGetLineInfo,                                       // 49
	EEtelPhoneGetStatus,                                         // 50
	EEtelPhoneNotifyModemDetected,                               // 51
	EEtelPhoneNotifyModemDetectedCancel,                         // 52
	EEtelServerClosePhoneModule,                                 // 53
	EEtelServerEnumeratePhones,                                  // 54
	EEtelServerGetTsyName,                                       // 55
	EEtelServerGetTsyVersionNo,                                  // 56
	EEtelServerLoadPhoneModule,                                  // 57
	EEtelServerOpen,                                             // 58
	EEtelServerPhoneInfoByIndex,                                 // 59
	EEtelServerQueryTsyFunctionality,                            // 60
	EEtelServerSetExtendedErrorGranularity                       // 61
	};


IMPORT_C void PanicClient(TInt aFault);

//
//
// RTelSubSessionBase
//
//
class CPtrHolder;
class RFile;
class RTelSubSessionBase : public RSubSessionBase
/** A base class used in the derivation of RCall, RLine, and RPhone. It has no user 
accessible functions. 
@publishedAll
@released
*/
	{
public:
	inline RSessionBase& SessionHandle() const;
	inline void SetSessionHandle(RSessionBase& aSession);
	inline TInt SubSessionHandle();

	IMPORT_C void CancelAsyncRequest(TInt aReqToCancel) const;

	enum TReqPriorityType
		{
		EIsNotaPriorityRequest,
		EIsaPriorityRequest
		};
protected:

	IMPORT_C RTelSubSessionBase();
	IMPORT_C TInt Blank(const TInt aIpc,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Blank(const TInt aIpc,TRequestStatus& aStatus,TReqPriorityType aType = EIsNotaPriorityRequest) const;

	IMPORT_C TInt Set(const TInt aIpc,const TDesC8& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C TInt Get(const TInt aIpc,TDes8& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Get(const TInt aIpc,TRequestStatus& aStatus,TDes8& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;

	IMPORT_C TInt Set(const TInt aIpc,const TDesC8& aDes1,const TDesC8& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes1,const TDesC8& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C TInt Get(const TInt aIpc,TDes8& aDes1,TDes8& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Get(const TInt aIpc,TRequestStatus& aStatus,TDes8& aDes1,TDes8& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	
	IMPORT_C TInt Set(const TInt aIpc,const TDesC16& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC16& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C TInt Get(const TInt aIpc,TDes16& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Get(const TInt aIpc,TRequestStatus& aStatus,TDes16& aDes,TReqPriorityType aType = EIsNotaPriorityRequest) const;

	IMPORT_C TInt Set(const TInt aIpc,const TDesC16& aDes1,const TDesC16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC16& aDes1,const TDesC16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C TInt Set(const TInt aIpc,const TDesC8& aDes1,const TDesC16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Set(const TInt aIpc,TRequestStatus& aStatus,const TDesC8& aDes1,const TDesC16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;

	IMPORT_C TInt Get(const TInt aIpc,TDes16& aDes1,TDes16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Get(const TInt aIpc,TRequestStatus& aStatus,TDes16& aDes1,TDes16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C TInt Get(const TInt aIpc,TDes8& aDes1,TDes16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void Get(const TInt aIpc,TRequestStatus& aStatus,TDes8& aDes1,TDes16& aDes2,TReqPriorityType aType = EIsNotaPriorityRequest) const;

	IMPORT_C void SetAndGet(const TInt aIpc, TRequestStatus& aStatus, const TDesC8& aDes1, TDes8& aDes2, TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void SetAndGet(const TInt aIpc, TRequestStatus& aStatus, TDes8& aDes1, const TDesC16& aDes2, TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void SetAndGet(const TInt aIpc, TRequestStatus& aStatus, const TDesC8&, TDes16& aDes2, TReqPriorityType aType = EIsNotaPriorityRequest) const;
	IMPORT_C void SetAndGet(const TInt aIpc, TRequestStatus& aStatus, const TDesC16& aDes1, TDes16& aDes2, TReqPriorityType aType = EIsNotaPriorityRequest) const;

	IMPORT_C TInt Set(const TInt aIpc, const RFile& aFile, TReqPriorityType aType = EIsNotaPriorityRequest) const;

	IMPORT_C TInt CancelReq(const TInt aIpc,const TInt aIpcToCancel) const;
	IMPORT_C TInt CancelSubSession() const;

private:
	RTelSubSessionBase(const RTelSubSessionBase& aTelSubSessionBase);
	RSessionBase* iTelSession;
protected:	
	CPtrHolder* iPtrHolder;
	};


//**********************************
// RPacketContext
//**********************************
// This class implements context specific part of ETel packet data api.
	

class RPacketService;
class CPacketContextInternalData;
//class RPacketContext : public RSubSessionBase
class RPacketContext : public RTelSubSessionBase
    {
      friend class RPacketQoS;
    public:


	
	/**
	Protocol identifiers, as defined in 24.008, section 10.5.6.3
	*/
	enum TPcoProtocolId
	{
	/** PCO identifier for LCP.*/
	EETelPcktProtocolLcpId = 0xc021,	
	/** PCO identifier for PAP.*/
	EETelPcktProtocolPapId = 0xc023,
	/** PCO identifier for CHAP.*/
	EETelPcktProtocolChapId = 0xc223,
	/** PCO identifier for IPCP.*/
	EETelPcktProtocolIpcpId = 0x8021
	};

	/**
	PCO additional parameter identifiers in MS to network direction, as defined in 3GPP TS 24.008, section 10.5.6.3
	*/
	enum TPcoIDMSToNetwork
	{
	/** PCO MS additional parameter identifier for a PCSCF address request.*/
	EEtelPcktPCSCFAddressRequest = 0x01,
	/** PCO MS additional parameter identifier for an IM CN subsystem signalling flag.*/
	EEtelPcktIMCNMSSubsystemSignallingFlag = 0x02,
	/** PCO MS additional parameter identifier for a DNS server address request.*/
	EEtelPcktDNSServerAddressRequest = 0x03,
	/** Not currently supported.*/
	EEtelPcktNotSupported = 0x04,
	};

	/**
	PCO additional parameter identifiers in  network to MS direction, as defined in 3GPP TS 24.008, section 10.5.6.3
	*/
	enum TPcoIDNetworkToMS
	{
	/** PCO network additional parameter identifier for a PCSCF address.*/
	EEtelPcktPCSCFAddress = 0x01,
	/** PCO network additional parameter identifier for an IM CN subsystem signalling flag.*/
	EEtelPcktIMCNNetworkSubsystemSignallingFlag = 0x02,
	/** PCO network additional parameter identifier for a DNS server address.*/
	EEtePcktDNSServerAddress= 0x03,
	/** PCO network additional parameter identifier for a policy control rejection code.*/
	EEtelPcktPolicyControlRejectionCode = 0x04,
	};


	/** 
	Helper class that implements PCO IE identifier.
	
	@publishedPartner
	@released
	*/
	class TPcoId : public MTlvItemIdType
	{
	public:
		IMPORT_C TPcoId();
		IMPORT_C TPcoId(TUint16 aId);
		IMPORT_C void ExternalizeL(TDes8&)const;
		IMPORT_C void InternalizeL(TDesC8&);
		IMPORT_C TUint SerializedLength() const;	
		IMPORT_C TUint16 Id()const;
		IMPORT_C void SetId(TUint16);
		IMPORT_C TBool IsEqual(const MTlvItemIdType& aOtherIdType)const;	
	protected:
		/** PCO Id*/
  		TUint16 iId;
	};
	
	/** 
	Helper class that implements PCO IE identifier length.
	
	@publishedPartner
	@released
	*/
	class TPcoItemDataLength: public MTlvItemDataLengthType
	{
	public:
		IMPORT_C TPcoItemDataLength();
		IMPORT_C TPcoItemDataLength(TUint8 aLen);
		IMPORT_C void SetDataLength(TUint aLength);
		IMPORT_C TUint DataLength() const;
		IMPORT_C void ExternalizeL(TDes8&) const;
		IMPORT_C void InternalizeL(TDesC8&);
		IMPORT_C TUint SerializedLength()const;
	protected:
		/** length of associated item data*/
  		TUint8 iDataLength;
	};



      IMPORT_C RPacketContext();
      IMPORT_C ~RPacketContext();
      IMPORT_C TInt OpenNewContext(RPacketService& aPacketNetwork, TDes& aContextName);
      IMPORT_C TInt OpenNewSecondaryContext(RPacketService& aPacketService, const TDesC& aExistingContextName, TDes& aNewContextName);
      IMPORT_C TInt OpenExistingContext(RPacketService& aPacketNetwork, const TDesC& aContextName);
      IMPORT_C void Close();

      class TDataChannelV2 : public TPacketBase
          {
          public:
            TDataChannelV2() { iExtensionId = KETelExtPcktV2; };
          public:
            TFileName iCsy; // deprecated
            TName iPort; // deprecated
            TName iChannelId;
          };
      typedef TPckg<TDataChannelV2> TDataChannelV2Pckg;

      IMPORT_C void InitialiseContext(TRequestStatus& aStatus, TDes8& aDataChannelV2Pckg) const;

      enum {KIPAddressSize = 16};
      typedef TUint8 TIPAddress[KIPAddressSize];

      class TPacketFilterV2 : public TPacketBase
          {
          public:
            TPacketFilterV2() { iExtensionId = KETelExtPcktV2; };
          public:
            TInt iId;
            TInt iEvaluationPrecedenceIndex;
            TIPAddress iSrcAddr;
            TIPAddress iSrcAddrSubnetMask;
            TInt iProtocolNumberOrNextHeader;
            TInt iSrcPortMin;
            TInt iSrcPortMax;
            TInt iDestPortMin;
            TInt iDestPortMax;
            TUint32 iIPSecSPI;
            TInt16 iTOSorTrafficClass;
            TInt32 iFlowLabel;
          };

      typedef TPckg<TPacketFilterV2> TPacketFilterV2Pckg;

      enum {KGSNNameLength = 252};
      enum {KMaxPDPAddressLength = 50}; // KCommsDbSvrMaxFieldLength = 50
      enum
          {
          KPdpDataCompression		= 0x01,
          KPdpHeaderCompression	= 0x02
          };

	  typedef TBuf8<KGSNNameLength> TGSNAddress;				//< GPRS Support Node (GSN) name
	  typedef TBuf8<KMaxPDPAddressLength> TProtocolAddress;	//< GPRS Rel97/98, CDMA, Rel99 and Rel4.
  
      // TProtocolType defines the protocol used to connect to the packet data gateway
      enum TProtocolType
          {
          EPdpTypeIPv4,	// GPRS
          EPdpTypeIPv6,	// GPRS
          EPdpTypeX25,	// GPRS
          EPdpTypePPP,	// GPRS, CDMA
          EPdpTypeCDPD	// CDMA
          };

      // TServiceOption - Service Option numbers are CDMA specific & used in the API only
      enum TServiceOption
          {
          KLowSpeedData	= 0x01,			// Service Options 7 and 15, 8-16
          KHighSpeedData	= 0x02,			// Service Options 22-25 IP and 26-29 CDPD
          KHighSpeedCDMA2000Data = 0x04	// Service Option 33 on CDMA2000 network only
          };

      enum TAnonymousAccess
          {
          ENotApplicable,	// GPRS, CDMA
          ERequired,		// GPRS
          ENotRequired	// GPRS
          };

      enum {KPasswordLength = 10};				// 3GPP TS 22.004, 4 digit length, range 0000 to 9999
      //
      typedef TBuf<KPasswordLength> TPassword;	

      enum { KMaxAuthDataLength = 50 };

	typedef TBuf8<KMaxAuthDataLength> TAuthData;
   
	//
	// TPacketFlowIdentifier - indicates the Packet Flow Identifier for a
 	// Packet Flow Context.  See Table 10.5.161/3GPP TS 24.008.
 	//
 	enum TPacketFlowIdentifier
         {
         EBestEffort = 0,
         ESignalling = 1,
         ESms        = 2
         };
 
 	//
 	// TMiscProtocolBuffer - this buffer can be used to store protocol related data
 	// (such as "PPP config options") which do not fit into the other fields of the
 	// TProtocolConfigOptionV2 class (see below).  The length of the buffer is based on
 	// the previous buffer used (before TProtocolConfigOptionV2 was implemented), which 
 	// was based on 3GPP TS 24.008 (253 octets).
 	//
 	enum {KMiscProtocolBufferLength = 253};
 	typedef TBuf8<KMiscProtocolBufferLength> TMiscProtocolBuffer;


      //
      // TAuthProtocol - this enables the client to set the protocol type used on the context
      // 
      enum TAuthProtocol
          {
          EProtocolNone,
          EProtocolPAP,
          EProtocolCHAP
          };

      //
      // TAuthInfo - this enables the client to set the authentification data used on the context
      //
      struct TAuthInfo
          {
            TAuthProtocol iProtocol;
            TAuthData iUsername;
            TAuthData iPassword;
          };

      //
      // TDnsInfoV2 - this class enables the client to set the primary and secondary DNS server 
      // names used on the context 
      //
      class TDnsInfoV2 : TPacketBase
          {
          public:
            IMPORT_C TDnsInfoV2();
          public:
            TProtocolAddress iPrimaryDns;
            TProtocolAddress iSecondaryDns;
          };
      typedef TPckg<TDnsInfoV2> TTDnsInfoV2Pckg;

      /**
          * @class TProtocolConfigOptionV2 
          * The Protocol Config Option can contain the DNS server names, 
          * the username, password, and CHAP associated data. Rather than keep 
          * the Protocol Config Option as a buffer it is encapsulated as a class. This enables 
          * data extraction to occur easily. 
          */
    class TProtocolConfigOptionV2 : public TPacketBase
          {
          public:

			IMPORT_C TProtocolConfigOptionV2();
			
			
				

          public:
            TAuthInfo iAuthInfo;
            TAuthData iChallenge;
            TAuthData iResponse;
            TUint8 iId;
            TDnsInfoV2 iDnsAddresses;
			TMiscProtocolBuffer iMiscBuffer;
          };

      //
      class TContextConfigGPRS : public TPacketDataConfigBase
          {
            // GPRS packet data support
          public:
            IMPORT_C TContextConfigGPRS();
          public:
            TProtocolType iPdpType;
            TGSNAddress iAccessPointName;
            TProtocolAddress iPdpAddress;
            TUint iPdpCompression;
            TAnonymousAccess iAnonymousAccessReqd;
            TBool iUseEdge;
            //
			TProtocolConfigOptionV2 iProtocolConfigOption; 
			TBool iNWIContext; // Network initiated context indication.
            //
          };



     	/**
     	 * @class TContextConfigUMTS etelpckt.h "inc/etelpckt.h"
     	 * @brief This class enables UMTS context configuration.
     	 */
     	class TContextConfigR99_R4 : public TPacketDataConfigBase
     		{
     	public:
     		IMPORT_C TContextConfigR99_R4();
     	public:
     		TProtocolType iPdpType;
     		TGSNAddress iAccessPointName;
     		TProtocolAddress iPdpAddress;		
      		TBool iUseEdge; // True for EGPRS
	 		TProtocolConfigOptionV2 iProtocolConfigOption;
			TBool iNWIContext; // Network initiated context indication.
 			TPacketFlowIdentifier iPFI; // Packet Flow indicator.
 		};





      class TContextConfigCDMA : public TPacketDataConfigBase
          {
            // CDMA/CDMA2000 packet data & high speed packet data support
          public:
            IMPORT_C TContextConfigCDMA();
          public:
            TUint iServiceOption; // identifies the CDMA packet service option range (Low, High, High CDMA2000)
            TProtocolType iProtocolType;	// identifies a PPP or CDPD based connection
          };

	
	
	enum TFQDNLength
	{
	KMaxFQDNLength=255,
	};
	
	typedef TBuf8<KMaxFQDNLength> TAuthorizationToken;
	
	//SBLP
      class CTFTMediaAuthorizationV3 : public CBase	
        {
        
        public:
		    IMPORT_C ~CTFTMediaAuthorizationV3();
		    IMPORT_C static CTFTMediaAuthorizationV3* NewL();			
	    public:	
		    IMPORT_C virtual void ExternalizeL(HBufC8*& aBuffer) const;
		    IMPORT_C virtual void InternalizeL(TDes8& aBuffer);
		    IMPORT_C TUint ExtensionId()const;
		    
	    
        public:
		    /** Flow identifier */	
            struct TFlowIdentifier
			    {
			    /** Media component number. */
			    TUint16 iMediaComponentNumber;
			    /** IP flow number. */
			    TUint16 iIPFlowNumber;
			    };				
		    /** Authorization token */		
		    TAuthorizationToken iAuthorizationToken;			 
		    /** List of flow identifiers authorization token is granted for*/
		    RArray<TFlowIdentifier>	iFlowIds;
	    protected:
	    	CTFTMediaAuthorizationV3();
		    TUint iExtensionId;
		};

      // Configuration-related calls
      IMPORT_C void SetConfig(TRequestStatus& aStatus, const TDesC8& aConfig) const;
      IMPORT_C void GetConfig(TRequestStatus& aStatus, TDes8& aConfig) const;
      IMPORT_C void NotifyConfigChanged(TRequestStatus& aStatus, TDes8& aConfig) const;

      IMPORT_C void Activate(TRequestStatus& aStatus) const;
      IMPORT_C void Deactivate(TRequestStatus& aStatus) const;
      IMPORT_C void Delete(TRequestStatus& aStatus) const;

      struct  TCommPort		// should be in RCall in etel.h
          {
            TFileName iCsy;
            TName iPort;
          };

      IMPORT_C void LoanCommPort(TRequestStatus& aStatus, TCommPort& aDataPort) const; // deprecated
      IMPORT_C void RecoverCommPort(TRequestStatus& aStatus) const; // deprecated
      //
      IMPORT_C void GetDnsInfo(TRequestStatus& aStatus, TDes8& aDnsInfo) const; 
      //

      enum TContextStatus
          {
          EStatusUnknown,
          EStatusInactive,
          EStatusActivating,
          EStatusActive,
          EStatusDeactivating,
          EStatusSuspended,
          EStatusDeleted
          };

      IMPORT_C TInt GetStatus(TContextStatus& aContextStatus) const;
      IMPORT_C void NotifyStatusChange(TRequestStatus& aStatus,TContextStatus& aContextStatus);

      IMPORT_C TInt GetProfileName(TName& aQoSProfile) const;

      IMPORT_C void EnumeratePacketFilters(TRequestStatus& aRequestStatus, TInt& aCount) const;
      IMPORT_C void GetPacketFilterInfo(TRequestStatus& aRequestStatus, TInt aIndex, TDes8& aPacketFilterInfo) const;
      IMPORT_C void AddPacketFilter(TRequestStatus& aRequestStatus, const TDesC8& aPacketFilterInfo) const;
      IMPORT_C void RemovePacketFilter(TRequestStatus& aRequestStatus, TInt aId) const;
      IMPORT_C void ModifyActiveContext(TRequestStatus& aStatus) const;

      struct TDataVolume
          {
            TUint32 iBytesSent;
            TUint32 iOverflowCounterSent;
            TUint32 iBytesReceived;
            TUint32 iOverflowCounterReceived;
          };

      struct TNotifyDataTransferredRequest
          {
            TUint iRcvdGranularity;
            TUint iSentGranularity;
          };

      IMPORT_C TInt GetDataVolumeTransferred(TDataVolume& aVolume) const; // deprecated
      IMPORT_C TInt GetDataVolumeTransferred(TRequestStatus& aStatus, TDataVolume& aVolume) const;
      IMPORT_C void NotifyDataTransferred(TRequestStatus& aStatus, TDataVolume& aVolume,
                                          TUint aRcvdGranularity=0x1000, TUint aSentGranularity=0x1000) const;

      IMPORT_C void GetConnectionSpeed(TRequestStatus& aStatus, TUint& aRate) const; // bits per second
      IMPORT_C void NotifyConnectionSpeedChange(TRequestStatus& aStatus, TUint& aRate) const;

      IMPORT_C TInt GetLastErrorCause(TInt& aError) const;

      IMPORT_C void CancelAsyncRequest(TInt aReqToCancel) const;

    private:
      CPacketContextInternalData* iData;
    };



//**********************************
// RPacketService
//**********************************
// This class implements non context specific part of ETel packet data api.

class CPacketServiceInternalData;
class RPacketService : public RSubSessionBase
    {
      friend class RPacketContext;
    public:
      IMPORT_C RPacketService();
      IMPORT_C ~RPacketService();

      IMPORT_C TInt Open(RUmtsSimServ &aServer);
      IMPORT_C void Close();

      // From ETel Packet Data Api / Packet Service

      IMPORT_C void NotifyContextAdded(TRequestStatus& aStatus, TDes& aContextId) const;

      IMPORT_C void Attach(TRequestStatus& aStatus) const;
      IMPORT_C void Detach(TRequestStatus& aStatus) const;

      enum TStatus
          {
          EStatusUnattached,
          EStatusAttached,
          EStatusActive,
          EStatusSuspended
          };
      IMPORT_C TInt GetStatus(TStatus& aPacketServiceStatus) const;
      IMPORT_C void NotifyStatusChange(TRequestStatus& aStatus,
                                       TStatus& aPacketServiceStatus) const;

      IMPORT_C void NotifyContextActivationRequested(TRequestStatus& aStatus,
                                                     TDes8& aContextParameters) const;
      IMPORT_C void RejectActivationRequest(TRequestStatus& aStatus) const;

      struct TContextInfo
          {
            TName iName;
            RPacketContext::TContextStatus iStatus;
          };

      IMPORT_C void EnumerateContexts(TRequestStatus& aStatus, TInt& aCount, TInt& aMaxAllowed) const;
      IMPORT_C void GetContextInfo(TRequestStatus & aStatus, TInt aIndex, TContextInfo& aInfo) const;

      enum TContextType
          {
          EUnspecified,
          EInternalContext,
          EExternalContext
          };

      class TNifInfoV2 : public TPacketBase
          {
          public:
            TNifInfoV2() { iExtensionId = KETelExtPcktV2; };
          public:
            TName iContextName;
            TInt iNumberOfContexts;
            RPacketContext::TContextStatus iNifStatus;
            RPacketContext::TProtocolAddress iPdpAddress;
            TContextType iContextType;
          };

      typedef TPckg<TNifInfoV2> TNifInfoV2Pckg;

      IMPORT_C void EnumerateNifs(TRequestStatus& aStatus, TInt& aCount) const;
      IMPORT_C void GetNifInfo(TRequestStatus& aStatus, TInt aIndex, TDes8& aNifInfo) const;
      IMPORT_C void EnumerateContextsInNif(TRequestStatus& aRequestStatus, const TDesC& aExistingContextName, TInt& aCount) const;
      IMPORT_C void GetContextNameInNif(TRequestStatus& aRequestStatus, const TDesC& aExistingContextName,
                                        TInt aIndex, TDes& aContextName) const;

      enum TRegistrationStatus
          {
          ENotRegisteredNotSearching,
          ERegisteredOnHomeNetwork,
          ENotRegisteredSearching,
          ERegistrationDenied,
          EUnknown,
          ERegisteredRoaming,
          ENotRegisteredButAvailable,
          ENotRegisteredAndNotAvailable
          };

      IMPORT_C void GetNtwkRegStatus(TRequestStatus& aStatus, TRegistrationStatus& aRegistrationStatus) const;
      IMPORT_C void NotifyChangeOfNtwkRegStatus(TRequestStatus& aStatus,
                                                TRegistrationStatus& aRegistrationStatus) const;

      enum TMSClass
          {
          EMSClassDualMode,
          EMSClassSuspensionRequired,
          EMSClassAlternateMode,
          EMSClassCircuitSwitchedOnly,
          EMSClassPacketSwitchedOnly,
          EMSClassUnkown
          };

      IMPORT_C void GetMSClass(TRequestStatus& aStatus, TMSClass& aCurrentClass,
                               TMSClass& aMaxClass) const;
      IMPORT_C void SetMSClass(TRequestStatus& aStatus, TMSClass aClass) const;
      IMPORT_C void NotifyMSClassChange(TRequestStatus& aStatus, TMSClass& aNewClass) const;

      enum TStaticMiscCaps
          {
          // GPRS-specific
          KCapsSuspendSupported				= 0x0001,
          KCapsAASupported					= 0x0002,
          KCapsNetworkAvailabilitySupported	= 0x0004,
          KCapsSetDefaultContextSupported		= 0x0008,
          KCapsChangeAttachModeSupported		= 0x0010,
          KCapsGetDataTransferredSupported	= 0x0020,
          KCapsNotifyDataTransferredSupported	= 0x0040,
          KCapsPreferredBearerSupported		= 0x0080,
          KCapsPdpDataCompSupported			= 0x0100,
          KCapsPdpHeaderCompSupported			= 0x0200,
          KCapsMSClassSupported				= 0x0400,
          KCapsNotifyMSClassSupported			= 0x0800,
          // CDMA
          KCapsCDMAOneLowSpeedDataSupported	= 0x00001000,
          KCapsCDMAOneHighSpeedDataSupported	= 0x00002000,
          // CDMA2000
          KCapsCDMA2000HighSpeedDataSupported	= 0x00004000,
          KCapsProtocolPPPSupported			= 0x00008000,
          KCapsProtocolCDPDSupported			= 0x00010000,
          KCapsPacketReleaseModeSupported		= 0x00020000,
          KCapsNotifyReleaseModeChangeSupported=0x00040000
          };

      IMPORT_C TInt GetStaticCaps(TUint& aCaps, RPacketContext::TProtocolType aPdpType) const;

      enum TDynamicCaps
          {
          // Common GPRS & CDMA caps
          KCapsActivate						= 0x00000001,
          KCapsRxCSCall						= 0x00000002,
          KCapsRxContextActivationReq			= 0x00000004,
          // GPRS-specific caps
          KCapsManualAttach					= 0x00000008,
          KCapsManualDetach					= 0x00000010,
          KCapsSMSTransfer					= 0x00000020,
          // CDMA-specific caps
          KCapsCDMAOneLowSpeedDataAllowed		= 0x00000040,	// Service Option (7,15) or (8,16)
          KCapsCDMAOneHighSpeedDataAllowed	= 0x00000080,	// Service Option (22-25) or (26-29)
          KCapsCDMA2000HighSpeedDataAllowed	= 0x00000100	// Service Option 33 or 34
          };

      typedef TUint TDynamicCapsFlags;

      IMPORT_C TInt GetDynamicCaps(TDynamicCapsFlags& aCaps) const;
      IMPORT_C void NotifyDynamicCapsChange(TRequestStatus& aStatus, TDynamicCapsFlags& aCaps) const;

      enum TPreferredBearer
          {
          EBearerPacketSwitched,
          EBearerCircuitSwitched
          };

      IMPORT_C void SetPreferredBearer(TRequestStatus& aStatus, TPreferredBearer aBearer) const;
      IMPORT_C TInt GetPreferredBearer(TPreferredBearer& aBearer) const;

      enum TAttachMode
          {
          EAttachWhenPossible,
          EAttachWhenNeeded
          };

      IMPORT_C TInt SetAttachMode(TAttachMode aMode) const; // deprecated
      IMPORT_C TInt GetAttachMode(TAttachMode& aMode) const; // deprecated
      IMPORT_C TInt SetAttachMode(TRequestStatus& aStatus, TAttachMode aMode) const;
      IMPORT_C TInt GetAttachMode(TRequestStatus& aStatus, TAttachMode& aMode) const;

      IMPORT_C TInt SetDefaultContextParams(const TDesC8& aPckg) const; // deprecated
      IMPORT_C TInt GetDefaultContextParams(TDes8& aPckg) const; // deprecated
      IMPORT_C TInt SetDefaultContextParams(TRequestStatus& aStatus, const TDesC8& aPckg) const;
      IMPORT_C TInt GetDefaultContextParams(TRequestStatus& aStatus, TDes8& aPckg) const;

      enum TPacketReleaseMode
          {
          EReleaseModeUnknown,
          EReleaseModeUnregistered,
          EReleaseMode97_98,
          EReleaseMode99,
          EReleaseMode4
          };

      IMPORT_C void GetCurrentReleaseMode(TRequestStatus& aStatus, TPacketReleaseMode& aReleaseMode) const;
      IMPORT_C void NotifyReleaseModeChange(TRequestStatus& aStatus, TPacketReleaseMode& aReleaseMode) const;

      IMPORT_C void CancelAsyncRequest(TInt aReqToCancel) const;

    private:
      CPacketServiceInternalData* iData;
    };

#endif
