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
// uscl_qos.cpp - implementation of RPacketQoS class
//
 
#include "us_cliserv.h"
#include "uscl_qos.h"
#include "uscl_qos_internal.h"
#include "uscl_packet_internal.h"

//********************************
//RPacketQoS
//********************************

EXPORT_C RPacketQoS::RPacketQoS() : iData(NULL)
    {
	// nothing to do, constructing done in Open-variants
    }

EXPORT_C RPacketQoS::~RPacketQoS()
    {
	if(iData)
        {
		delete iData;
		iData = NULL;
        }
    }

EXPORT_C TInt RPacketQoS::OpenNewQoS(RPacketContext& aPacketContext, TDes& aProfileName)
    {
    // TAny *p[KMaxMessageArguments];
	// p[0] = (TAny*) &aProfileName;
	// p[1] = (TAny*) EUmtsSimServOpenNewPacketQoS;
	// p[2] = (TAny*) &aPacketContext.iData->GetName();
    // TInt err = CreateSubSession(*aPacketContext.iData->GetServer(),
	// 							EUmtsSimServCreatePacketQoSSubSession,
	// 							&p[0]);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aProfileName, 
                   EUmtsSimServOpenNewPacketQoS, 
                   &aPacketContext.iData->GetName() );
    TInt err = CreateSubSession(*aPacketContext.iData->GetServer(),
								EUmtsSimServCreatePacketQoSSubSession,
								args);
	if(err != KErrNone) return err;

	if(!iData)
        {
		TRAPD(r,iData = CPacketQoSInternalData::NewL());
		if(r != KErrNone)
            {
			iData = NULL;
			return r;
            }
        }
	return KErrNone;
    }

EXPORT_C TInt RPacketQoS::OpenExistingQoS(RPacketContext& aPacketContext,
										  TDesC& aProfileName)
    {
    // TAny *p[KMaxMessageArguments];
	// p[0] = (TAny*) &aProfileName;
	// p[1] = (TAny*) EUmtsSimServOpenExistingPacketQoS;
	// p[2] = (TAny*) &aPacketContext.iData->GetName();
    // TInt err = CreateSubSession(*aPacketContext.iData->GetServer(),
	// 							EUmtsSimServCreatePacketQoSSubSession,
	//							&p[0]);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aProfileName, 
                   EUmtsSimServOpenExistingPacketQoS, 
                   &aPacketContext.iData->GetName() );
    TInt err = CreateSubSession(*aPacketContext.iData->GetServer(),
								EUmtsSimServCreatePacketQoSSubSession,
								args);
	if(err != KErrNone) return err;

	if(!iData)
        {
		TRAPD(r,iData = CPacketQoSInternalData::NewL());
		if(r != KErrNone)
            {
			iData = NULL;
			return r;
            }
        }
	return KErrNone;
    }

EXPORT_C void RPacketQoS::Close()
    {
	if(iData)
        {
		delete iData;
		iData = NULL;
        }

    RSubSessionBase::CloseSubSession(EUmtsSimServClosePacketQoSSubSession);
    }

EXPORT_C void RPacketQoS::SetProfileParameters(TRequestStatus& aStatus, TDes8& aProfile) const
    {
	// TAny *p[KMaxMessageArguments];
	// p[0] = (TAny*) &aProfile;
    // SendReceive(EUmtsSimServPacketQoSSetProfileParametersA, &p[0], aStatus);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aProfile );
    SendReceive(EUmtsSimServPacketQoSSetProfileParametersA, args, aStatus);
    }

EXPORT_C void RPacketQoS::GetProfileParameters(TRequestStatus& aStatus, TDes8& aProfile) const
    {
	// TAny *p[KMaxMessageArguments];
	// p[0] = (TAny*) &aProfile;
    // SendReceive(EUmtsSimServPacketQoSGetProfileParametersA, &p[0], aStatus);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aProfile );
    SendReceive(EUmtsSimServPacketQoSGetProfileParametersA, args, aStatus);
    }

EXPORT_C void RPacketQoS::NotifyProfileChanged(TRequestStatus& aStatus, TDes8& aProfile) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = &aProfile;
	// SendReceive(EUmtsSimServPacketQoSNotifyProfileChangedA, p, aStatus);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aProfile );
	SendReceive(EUmtsSimServPacketQoSNotifyProfileChangedA, args, aStatus);
    }

EXPORT_C void RPacketQoS::CancelAsyncRequest(TInt aReqToCancel) const
    {
	if((aReqToCancel & KUmtsSimServRqstMask) != KUmtsSimServRqstPacketQoS
       || (aReqToCancel & KUmtsSimServRqstCancelBit) == 0)
        {
		User::Invariant(); // should panic thread here with reasonable reason
		return;
        }

	// SendReceive(aReqToCancel, 0);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
	SendReceive(aReqToCancel, args);
    }

// *** CPacketQoSInternalData *** wraps data needed by RPacketQoS in seperately allocated package

CPacketQoSInternalData* CPacketQoSInternalData::NewL()
    {
	CPacketQoSInternalData* self = new (ELeave) CPacketQoSInternalData();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CPacketQoSInternalData::CPacketQoSInternalData()
    {
	// ConstructL does the work
    }

CPacketQoSInternalData::~CPacketQoSInternalData()
    {
    }

// *** QoS requested/negotiated/caps constructors ***

EXPORT_C RPacketQoS::TQoSR99_R4Requested::TQoSR99_R4Requested()
    {
	iExtensionId = TPacketDataConfigBase::KConfigRel99Rel4;
    }
/* -- done in header
    EXPORT_C RPacketQoS::TQoSUMTSNegotiated::TQoSUMTSNegotiated()
    {
	iExtensionId = TPacketDataConfigBase::KConfigRel99Rel4;
    }
    */
    
	    
EXPORT_C RPacketQoS::TQoSCapsR99_R4::TQoSCapsR99_R4()
    {
	iExtensionId = TPacketDataConfigBase::KConfigRel99Rel4;
    }

EXPORT_C RPacketQoS::TQoSGPRSRequested::TQoSGPRSRequested()
    {
	iExtensionId = TPacketDataConfigBase::KConfigGPRS;
    }
/* -- done in header
    EXPORT_C RPacketQoS::TQoSGPRSNegotiated::TQoSGPRSNegotiated()
    {
	iExtensionId = TPacketDataConfigBase::KConfigGPRS;
    }
    */
EXPORT_C RPacketQoS::TQoSCapsGPRS::TQoSCapsGPRS()
    {
	iExtensionId = TPacketDataConfigBase::KConfigGPRS;
    }

EXPORT_C RPacketQoS::TQoSR5Requested::TQoSR5Requested()
: TQoSR99_R4Requested(),
  iSignallingIndication(EFalse),
  iSourceStatisticsDescriptor(ESourceStatisticsDescriptorUnknown)
 /**
  * Constructor - The values are initialized to zero and unknown.
    Sets iExtentionId to KConfigRel5.
 */
	{
	iExtensionId = TPacketDataConfigBase::KConfigRel5;
	}
