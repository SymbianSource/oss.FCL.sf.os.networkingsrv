/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file for the IP SubConnection Parameters
* 
*
*/



/**
 @file ip_subconparams.h
*/

#ifndef __IP_SUBCONPARAMS_H__
#define __IP_SUBCONPARAMS_H__

#include <es_sock.h>
#include <networking/Qos3GPP_subconparams.h>

 
// NOTE: This Uid may be changed - See #ifdef at bottom of the file
const TInt KSubConIPParamsUid = 0x10204309;

const TInt KSubConQosIPLinkR99ParamsType = 1;

#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
#include <networking/qos3gpp_subconparams.h>

#else
const TInt KSubConnSBLPR5ExtensionParamsType = 2;

/** Constant definitions particular to the Generic Parameters. */
const TInt KMAuthTokenLength = 255;
/** Typedef for the AuthToken Holder. */
typedef TBuf8<KMAuthTokenLength> TAuthToken;
#endif

#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
/** Provides Implementation of IP QoS Parameters

@publishedAll
@released since v9.1
@deprecated Since Intulo. Use CSubConQosR99ParamSet from Qos3GPP_Subconparams.h instead.
*/
#else
/** Provides Implementation of IP QoS Parameters

@publishedAll
@released since v9.1
*/
#endif

class CSubConQosIPLinkR99ParamSet : public CSubConExtensionParameterSet
{
public:
	inline static CSubConQosIPLinkR99ParamSet* NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType);
	inline static CSubConQosIPLinkR99ParamSet* NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType);
	inline static CSubConQosIPLinkR99ParamSet* NewL();

	inline CSubConQosIPLinkR99ParamSet();

	inline RPacketQoS::TTrafficClass GetTrafficClass() const;
	inline RPacketQoS::TDeliveryOrder GetDeliveryOrder() const;
	inline RPacketQoS::TErroneousSDUDelivery GetErroneousSDUDelivery() const;
	inline RPacketQoS::TBitErrorRatio GetResidualBitErrorRatio() const;
	inline RPacketQoS::TSDUErrorRatio GetSDUErrorRatio() const;
	inline RPacketQoS::TTrafficHandlingPriority GetTrafficHandlingPriority() const;
	inline TInt GetTransferDelay() const;
	inline TInt GetMaxSduSize() const;
	inline TInt GetMaxBitrateUplink() const;
	inline TInt GetMaxBitrateDownlink() const;
	inline TInt GetGuaBitrateUplink() const;
	inline TInt GetGuaBitrateDownlink() const;
	inline void SetTrafficClass(RPacketQoS::TTrafficClass aTrafficClass);
	inline void SetDeliveryOrder(RPacketQoS::TDeliveryOrder aDeliveryOrder);
	inline void SetErroneousSDUDelivery(RPacketQoS::TErroneousSDUDelivery aDeliveryOfErroneusSdu);
	inline void SetResidualBitErrorRatio(RPacketQoS::TBitErrorRatio aResidualBer);
	inline void SetSDUErrorRatio(RPacketQoS::TSDUErrorRatio aErrorRatio);
	inline void SetTrafficHandlingPriority(RPacketQoS::TTrafficHandlingPriority aPriority);
	inline void SetTransferDelay(TInt aTransferDelay);
	inline void SetMaxSduSize(TInt aMaxSduSize);
	inline void SetMaxBitrateUplink(TInt aMaxBitrateUplink);
	inline void SetMaxBitrateDownlink(TInt aMaxBitrateDownlink);
	inline void SetGuaBitrateUplink(TInt aGuaBitrateUplink);
	inline void SetGuaBitrateDownlink(TInt aGuaBitrateDownlink);

protected:

	DATA_VTABLE

protected:
	RPacketQoS::TTrafficClass				iTrafficClass;			// Traffic class
	RPacketQoS::TDeliveryOrder				iDeliveryOrder;			// Delivery order
	RPacketQoS::TErroneousSDUDelivery		iDeliveryOfErroneusSdu;	// Delivery of erroneous SDUs
	RPacketQoS::TBitErrorRatio				iResidualBer;			// Residual BER
	RPacketQoS::TSDUErrorRatio				iErrorRatio;			// SDU error ratio
	RPacketQoS::TTrafficHandlingPriority	iPriority;				// Traffic handling priority
	TInt									iTransferDelay;			// Transfer delay
	TInt									iMaxSduSize;			// Maximum SDU size
	TInt									iMaxBitrateUplink;		// Maximum bit rate for uplink
	TInt									iMaxBitrateDownlink;	// Maximum bit rate for downlink
	TInt									iGuaBitrateUplink;		// Guaranteed bit rate for uplink
	TInt									iGuaBitrateDownlink;	// Guaranteed bit rate for downlink
	};



#ifndef SYMBIAN_NETWORKING_3GPPDEFAULTQOS

/** Flow Identifires
ECOM Implementation Id for SBLP Extension parameters.

@publishedAll
@released since v9.1 */
struct TFlowId
	{
	/** Getter Function for the Components of Flow ids. */
	inline TUint16 GetMediaComponentNumber() const;
	inline TUint16 GetIPFlowNumber() const;
	/**	Setter Function For Flow Id Components.	*/
	inline void SetMediaComponentNumber(TUint16 aMediaComponentNumber);
	inline void SetIPFlowNumber(TUint16 aIPFlowNumber);

private:	
	TUint16 iMediaComponentNumber;
	TUint16 iIPFlowNumber;
	};	

typedef RArray<TFlowId> RFlowIdentifiers;

/** Extension Parameter Sets, Consise of MAT and FI(s).
Provides Implementation of Extension parameters of the SBLP Family.

@publishedAll
@released since v9.1 */
class CSubConSBLPR5ExtensionParamSet : public CSubConExtensionParameterSet
	{
public:
	inline static CSubConSBLPR5ExtensionParamSet* NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType);
	inline static CSubConSBLPR5ExtensionParamSet* NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType);
	inline static CSubConSBLPR5ExtensionParamSet* NewL();

	/**	Media Authorization Token setter and getter functions. */
	inline const TAuthToken& GetMAT() const;
	inline void SetMAT(const TAuthToken& aAuthToken);
	
	/** Flow identifires setter and getter functions. */
	inline TInt GetNumberOfFlowIds() const;
	inline const TFlowId& GetFlowIdAt(TInt aIndex) const;
	
	/** Adding of Flow Identifires into an array. */
	inline void AddFlowIdL(const TFlowId& aFlowId);
	
	/** public constructors so that it can be accessed by factory. */
	inline CSubConSBLPR5ExtensionParamSet();	
	inline ~CSubConSBLPR5ExtensionParamSet();
	
protected:
	DATA_VTABLE

	/** Single Media Authorization Token (MAT). */
	TAuthToken iAuthToken;
	
	/** Multiple Flow Identifiers. */
	RFlowIdentifiers	iFlowIds;
	};
#endif

/**
Factory used to create instances of IP SubConnection Parameters.

@internalComponent
@released since v9.1
*/
class CSubConIPExtensionParamsFactory : public CBase
	{
public:
	static CSubConExtensionParameterSet* NewL(TAny* aConstructionParameters);
	};

#include <ip_subconparams.inl>

#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
/*
 Umts QoS extension classes have been relocated to the UmtsGprsSCPR component.
 So now we need to make sure that the correct class factory TUid, class name,
 and class Id are used
*/
#define KSubConIPParamsUid KSubCon3GPPExtParamsFactoryUid
#define KSubConQosIPLinkR99ParamsType KSubConQosR99ParamsType
#define CSubConQosIPLinkR99ParamSet CSubConQosR99ParamSet
#endif
// SYMBIAN_NETWORKING_3GPPDEFAULTQOS

#endif
// __IP_SUBCONPARAMS_H__
