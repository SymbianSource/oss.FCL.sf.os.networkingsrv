// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @publishedPartner
 @released since 9.5
*/

#ifndef __MBMSPARAMS_H__
#define __MBMSPARAMS_H__

#include <genericscprparams.h>
#include <e32base.h>
#include <e32std.h>
#include <mbmstypes.h>
#include <in_sock.h>
#include <comms-infras/es_connectionservparameterbundle.h>
#include <comms-infras/es_parameterfamily.h>
/** 
	Represents the implementation Uid for MBMS parameters.  
*/
const TInt32 KConnectionServMBMSParamImplUid = 0x20019D41;


/**
	Represents the implementation Uid for Channel SubConnection parameters.  
*/
const TInt32 KSubConChannelParamsImplUid = 0x20019D42;

/** 
	Represents the implementation Uid for Mbms Extention SubConnection parameters.  
*/
const TInt32  KSubConMBMSExtParamsImplUid = 0x20019D43;
/** 
	Represents SubConneciton Channel parameter set type.  
*/
const TInt KSubConChannelParamsType = 9;

/** 
	Represents MBMS SubConnection Extention  parameter set type.  
*/
const TInt KSubConMBMSSessionExtParamsType =1;

/** 
	Represents the set type for MBMS parameters.  
*/
const TInt32 KMBMSParameterSetType = 1;

/**
    Represents the set type for MBMS query.  
*/
const TInt32 KMBMSQuerySetType = 2;

using namespace GenericScprParameters;
namespace ConnectionServ
{
struct TMBMSInfo
/**Holds the MBMS information passed to ETel. */
	{
	/**Holds the value TMGI */
	TTmgi iTmgi;
	/**Holds the value access bearer.*/
	TMbmsScope iMbmsScope;
	/** Sets the priority for MBMS service. */
	TMbmsServicePriority iServicePriority;
	/**Holds IP address of the host to connect.This is used for later use to
	set multicast address */
	TInetAddr iInetAddr;  
	};
	
class TMBMSChannelInfoV1 : public TChannel
/** Defines getter and setter methods to get and set MBMS parameters.This class is derived from
TChannel which is a buffer of size 256 bytes.
*/
	{
	public:
		inline TMBMSChannelInfoV1();
     	
		inline const TTmgi& GetTmgi() ;
		inline void SetTmgi(const TTmgi& aTmgi);
		
		inline const TMbmsScope GetScope() ;
		inline void SetScope(const TMbmsScope aScope);
					
		inline TMbmsServicePriority GetServicePriority() ;
		inline void SetServicePriority(const TMbmsServicePriority aServicePriority);
	    
		inline const TInetAddr& GetInetAddress() ;
		inline void SetInetAddress(const TInetAddr& aAddress);
		
	private:
	   	inline TMBMSInfo* AddrPtr() ;
     	  
	};

class XMBMSServiceParameterSet:public XConnectionServParameterSet,
public MParameterSetTemplateMethods<CConnectionServParameterSetContainer, 
XMBMSServiceParameterSet,KConnectionServMBMSParamImplUid,KMBMSParameterSetType>
/** This class is used for getting and setting MBMS service parameters from the client to ETel
and vice versa.
*/
	{
   public:
	     inline XMBMSServiceParameterSet();
	     virtual ~XMBMSServiceParameterSet();
	     
         inline TMbmsAvailabilityStatus GetMBMSServiceAvailability() const;
         inline void SetMBMSServiceAvailability(const TMbmsAvailabilityStatus aAvailabilityStatus);
	     	     
	     inline  TMBMSChannelInfoV1* GetChannelInfo() ;
	     	     	     
		 inline TMbmsServiceMode GetServiceMode() const;
		 inline void SetServiceMode(const TMbmsServiceMode aServiceMode);
		
   protected:
   	     DATA_VTABLE
	     
   private:
   		/** Holds the MBMS channel information */
   	    TMBMSChannelInfoV1 iServiceInfo;
   	    /** Holds the MBMS service mode information */
   	    TMbmsServiceMode iServiceMode;
   	    /** Holds the MBMS service availability status information */
        TMbmsAvailabilityStatus iAvailabilityStatus;
	};

class XMBMSServiceQuerySet:public XConnectionServParameterSet,
public MParameterSetTemplateMethods<CConnectionServParameterSetContainer,XMBMSServiceQuerySet,
KConnectionServMBMSParamImplUid,KMBMSQuerySetType>
/** Holds APIs to query PDP Tier Manager for MBMS bearer availability and MBMS  
service notifications.
*/
	{
   public:
	  enum TQueryType
		  {
		  /** Checks whether MBMS is supported by network. */
		  EBearerAvailability, 
		  /** Do MBMS Specific operations on MBMS monitor list. */
		  EAddService, ERemoveService, ERemoveAllService,
		  /** Retrieve number of entries in monitor & activated service list. */
		  ECountActiveServiceList, ECountMonitorList
		  };
	      
	      inline XMBMSServiceQuerySet::TQueryType GetQueryType() const;
	      inline void SetQueryType(const TQueryType aQueryType);
	      
	      inline TMbmsNetworkServiceStatus  GetMBMSBearerAvailability() const;
	      inline void SetMBMSBearerAvailability(const TMbmsNetworkServiceStatus aBearerAvailability);

	      inline TUint GetListCount() const;
     	  inline void SetListCount(const TUint aCurrentCount);
				  
     	  inline TUint GetListMaxCount() const;
          inline void SetListMaxCount(const TUint aMaxCount);
          	      
	      virtual ~XMBMSServiceQuerySet();
	          
   protected:
	      DATA_VTABLE	
	       
   private:
   		  /** Holds the MBMS query */
	      TQueryType iQueryType;
	      /** Holds the MBMS Network service status information */
	      TMbmsNetworkServiceStatus  iBearerAvailability;
	      /** Holds the current count of monitor & activated service list */
	      TUint iCurrentCount;
	      /** Holds the maximum count of monitor & activated service list */
	      TUint iMaxCount;
	 } ;

class CConnectionServMBMSParamsFactory : public CBase
/** Factory used to create instances of MBMS Parameters.
*/
	{
public:
	static XConnectionServParameterSet* NewL(TAny* aConstructionParameters);
	};
	


/** Factory used to create instances of Channel SubConnection Parameters.
@publishedPartner
@released since 9.5
*/	
class CSubConChannelParamsFactory : public CBase
	{
public:
	static CSubConGenericParameterSet* NewL(TAny* aConstructionParameters);
	};	



/**  This class is used to set MBMS SubConnection parameters.
@publishedPartner
@released since 9.5
*/	
class CSubConChannelParamSet : public CSubConGenericParameterSet 
	{
	
	public:
		enum	
			{
			EUid = KSubConChannelParamsImplUid,
			EId = KSubConChannelParamsType,
			};
			
	public:	
		inline static CSubConChannelParamSet* NewL(CSubConParameterFamily& aFamily,CSubConParameterFamily::TParameterSetType aType);
		inline static CSubConChannelParamSet* NewL(RParameterFamily& aFamily,RParameterFamily::TParameterSetType aType);
		inline static CSubConChannelParamSet* NewL();
		
		/* Channel   Getter Functions */
		inline TAny* GetChannelInfo() ; 
        
        /** public constructors so that it can be accessed by factory. */         
        inline CSubConChannelParamSet();	
		virtual ~CSubConChannelParamSet();                               
   protected:                                     
		DATA_VTABLE
	private:
		GenericScprParameters::TChannel iServiceInfo;
};


/** Factory used to create instances of Mbms Extention SubConnection Parameters.

@publishedPartner
@released since 9.5
*/
class CSubConMBMSExtParamsFactory : public CBase
	{
public:
	static CSubConExtensionParameterSet* NewL(TAny* aConstructionParameters);
	};
	

/** This class is used to set MBMS Extension Parameters.

@publishedPartner
@released since 9.5
*/	
class CSubConMBMSExtensionParamSet :public CSubConExtensionParameterSet
	{
	public:
	/* Different operation types to add,remove and list sessions. */
	enum TOperationType
	     {
	     EAddSession,ERemoveSession, ERemoveAll,ESessionList
	     };         

	enum 
		{
		EUid = KSubConMBMSExtParamsImplUid,
		EId = KSubConMBMSSessionExtParamsType,
		};
	public:
		inline static 	CSubConMBMSExtensionParamSet* NewL(CSubConParameterFamily& aFamily,CSubConParameterFamily::TParameterSetType aType);
		inline static 	CSubConMBMSExtensionParamSet* NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType);
		inline static 	CSubConMBMSExtensionParamSet* NewL();
		
		/* Mbms Service Mode(Broadcast, Multicast or Selected Broadcast) Setter & Getter Functions */
		inline void SetServiceMode( const TMbmsServiceMode aServiceMode);
		inline TMbmsServiceMode GetServiceMode() const; 
		
		/* Mbms Extension Operation Setter & Getter Functions */
		inline TOperationType GetOperationType () const;
		inline void SetOperationType(const TOperationType aOperationType);
		
		/* Session Id Setter & Getter Functions */
		inline void SetSessionId(const TUint aSessionId);
		inline TInt GetSessionCount() const;
		inline TInt GetSessionId(const TUint aIndex=0);
		
		 /** public constructors so that it can be accessed by factory. */         
		inline CSubConMBMSExtensionParamSet();	
		virtual ~CSubConMBMSExtensionParamSet();          
	protected:
		DATA_VTABLE
	private:
		/** Array of Session Ids in a Service */	
		RArray<TUint> iSessionIds; 
		/** Mbms Service Mode */	
		TMbmsServiceMode iServiceMode; 	
		/** Mbms Extn Set Operation Type */
		TOperationType iOperationType;	
		
	};	
	
} // namespace ConnectionServ



#include <networking/mbmsparams.inl>
#endif	// __MBMSPARAMS_H__
