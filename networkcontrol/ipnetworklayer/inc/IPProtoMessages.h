// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IPProto-proprietary messages
// 
//

/**
 @file
 @internalTechnology
*/


#ifndef SYMBIAN_IPPROTOMESSAGES_H
#define SYMBIAN_IPPROTOMESSAGES_H

#include <elements/nm_signatures.h>
#include <networking/pdpdef.h>

enum IPProtoCustomActivities
    {
	ECFActivityOpenCloseRoute,
	ECFActivityDataMonitoring,
	ECFActivityConfigureNetwork,
	ECFActivityIoctl,
	ECFIpProtoCprActivityDataClientStatusChange
    };

class CIPProtoSubConnParameterFactory : public CBase
	{
public:
	static CSubConExtensionParameterSet* NewL(TAny* aConstructionParameters);
    enum
        {
        EUid = 0x1028300A,
        };
	};

class CSubConTFTParameterSet : public CSubConExtensionParameterSet
/** Extension Parameter Sets,
Provides Implementation of Extension parameters of the
TFT (Traffic Flow Template) Family.

@internalTechnology
@released Since 9.4
*/
	{
public:
	inline static CSubConTFTParameterSet* NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType);
	inline static CSubConTFTParameterSet* NewL();

    enum
        {
        EMaxPacketFilterCount = 8,
        ETFTInternalFamily = 1024,
        };

    enum
        {
        EUid = 0x1028300A,
        ETypeId = 1,
        };

	/** public constructors so that it can be accessed by factory. */
	inline CSubConTFTParameterSet();
	inline ~CSubConTFTParameterSet();

    inline TTFTOperationCode GetOperationCode() const;
    inline void SetOperationCode(TTFTOperationCode aOpCode);

    inline const TTFTInfo& GetTftInfo() const;
    inline void SetTftInfo(const TTFTInfo& aTftInfo);

protected:
	DATA_VTABLE
    TTFTInfo iTftInfo;
    TTFTOperationCode iOperationCode;
	};



class TCFIPProtoMessage
/**
IPProto message realm (messages specific to IPProto layer)

@internalComponent
*/
	{
  public:
    enum { ERealmId = 0x10281DED };

  private:
  	enum
  	    {
        EIpProtoCprOpenCloseRoute = Messages::KNullMessageId + 1,
		EConfigureNetwork         = 2,
		ENetworkConfigured        = 3
	    };

  public:


    //--Idle Timer--
    typedef Messages::TMessageSigNumber<EIpProtoCprOpenCloseRoute, TCFIPProtoMessage::ERealmId> TOpenCloseRoute;

	//--NetCfgExt--
	typedef Messages::TMessageSigVoid<EConfigureNetwork, TCFIPProtoMessage::ERealmId> TConfigureNetwork;
	typedef Messages::TMessageSigNumber<ENetworkConfigured, TCFIPProtoMessage::ERealmId> TNetworkConfigured;

    static void RegisterL();
    static void DeRegister();
	};

inline CSubConTFTParameterSet* CSubConTFTParameterSet::NewL(CSubConParameterFamily& aFamily, CSubConParameterFamily::TParameterSetType aType)
    {
	CSubConTFTParameterSet* tftExtn = CSubConTFTParameterSet::NewL();
	CleanupStack::PushL(tftExtn);
	aFamily.AddExtensionSetL(*tftExtn, aType);
	CleanupStack::Pop(tftExtn);
	return tftExtn;
    }

CSubConTFTParameterSet* CSubConTFTParameterSet::NewL()
	{
	STypeId typeId = STypeId::CreateSTypeId(CSubConTFTParameterSet::EUid, CSubConTFTParameterSet::ETypeId);
	return static_cast<CSubConTFTParameterSet*>(CSubConParameterSet::NewL(typeId));
	}

inline CSubConTFTParameterSet::CSubConTFTParameterSet()
:CSubConExtensionParameterSet()
    {
    }

inline CSubConTFTParameterSet::~CSubConTFTParameterSet()
    {
    }


inline const TTFTInfo& CSubConTFTParameterSet::GetTftInfo() const
    {
    return iTftInfo;
    }

inline void CSubConTFTParameterSet::SetTftInfo(const TTFTInfo& aTftInfo)
    {
    iTftInfo.Set(aTftInfo);
    }

inline TTFTOperationCode CSubConTFTParameterSet::GetOperationCode() const
    {
    return iOperationCode;
    }

inline void CSubConTFTParameterSet::SetOperationCode(TTFTOperationCode aOpCode)
    {
    iOperationCode = aOpCode;
    }

#endif
// SYMBIAN_IPPROTOMESSAGES_H
