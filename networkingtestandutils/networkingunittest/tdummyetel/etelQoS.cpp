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
// Multimode Quality of Service (QoS) Support
// GPRS, CDMAOne, CDMA2000
// 
//

#include "Et_clsvr.h"
#include "ETELEXT.H"

// ETel Packet header files
#include "etelQoS.h"
#include "pcktptr.h"
#include "pcktcs.h"

// Used by ETel to instruct TSY to create a name for the newly opened object
_LIT(KETelNewQoSName, "::");

/***********************************************************************************/
//
// RPacketQoS
//
/***********************************************************************************/

EXPORT_C RPacketQoS::RPacketQoS()
	:iEtelPacketQoSPtrHolder(NULL)
	{
	}

EXPORT_C void RPacketQoS::ConstructL()
	{
	__ASSERT_ALWAYS(iEtelPacketQoSPtrHolder == NULL, PanicClient(EEtelPanicHandleNotClosed));
	iEtelPacketQoSPtrHolder = CPacketQoSPtrHolder::NewL(CEtelPacketPtrHolder::EMaxNumPacketQoSPtrSlots);
	}

EXPORT_C void RPacketQoS::Destruct()
	{
	delete iEtelPacketQoSPtrHolder;
	iEtelPacketQoSPtrHolder = NULL;
	}

EXPORT_C TInt RPacketQoS::OpenNewQoS(RPacketContext& aPacketContext, TDes& aProfileName)
	{
	RSessionBase session = aPacketContext.SessionHandle();
	
	TRAPD(ret,ConstructL());
	if (ret)
		{
		return ret;
		}
	TInt subSessionHandle=aPacketContext.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle)); // client has no existing sub-session!
	TPtrC name(KETelNewQoSName);	// necessary so that server knows to ask TSY for new name
	TIpcArgs args;
	args.Set(0,&name);
	args.Set(1,&aProfileName); // name to be created & passed back
	args.Set(2,subSessionHandle);
	SetSessionHandle(session);
	ret = CreateSubSession(session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C TInt RPacketQoS::OpenExistingQoS(RPacketContext& aPacketContext, const TDesC& aProfileName)
//
//	Opens a handle on existing QoS object. Returns KErrNotFound if it does not exist
//
	{
	RSessionBase session = aPacketContext.SessionHandle();
	
	TRAPD(ret,ConstructL());
	if (ret)
		{
		Destruct();
		return ret;
		}
	TInt subSessionHandle=aPacketContext.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle != 0,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(aProfileName.Length() != 0,PanicClient(KErrBadName));

	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,CONST_CAST(TDesC*,&aProfileName)));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(session);
	ret = CreateSubSession(session,EEtelOpenByNameFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RPacketQoS::Close()
//
// Close the client's current sub-session with ETel
//
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RPacketQoS::SetProfileParameters(TRequestStatus& aStatus, TDes8& aProfile) const
	{
	Set(EPacketQoSSetProfileParams, aStatus, aProfile);
	}

EXPORT_C void RPacketQoS::GetProfileParameters(TRequestStatus& aStatus, TDes8& aProfile) const
	{
	Get(EPacketQoSGetProfileParams, aStatus, aProfile);
	}

EXPORT_C void RPacketQoS::GetProfileCapabilities(TRequestStatus& aStatus, TDes8& aProfileCaps) const
	{
	Get(EPacketQoSGetProfileCaps, aStatus, aProfileCaps);
	}

EXPORT_C void RPacketQoS::NotifyProfileChanged(TRequestStatus& aStatus, TDes8& aProfile) const
	{
	Get(EPacketQoSNotifyProfileChanged, aStatus, aProfile);
	}


EXPORT_C RPacketQoS::TQoSCapsGPRS::TQoSCapsGPRS()
:TPacketDataConfigBase()
	{
	iExtensionId = KConfigGPRS;

	iPrecedence = EUnspecifiedPrecedence;
	iDelay = EUnspecifiedDelayClass;
	iReliability = EUnspecifiedReliabilityClass;
	iPeak = EUnspecifiedPeakThroughput;
	iMean = EUnspecifiedMeanThroughput;
	}

EXPORT_C RPacketQoS::TQoSGPRSRequested::TQoSGPRSRequested()
:TPacketDataConfigBase()
	{
	iExtensionId = KConfigGPRS;

	iReqPrecedence = EUnspecifiedPrecedence;
	iMinPrecedence = EUnspecifiedPrecedence;
	iReqDelay = EUnspecifiedDelayClass;
	iMinDelay = EUnspecifiedDelayClass;
	iReqReliability = EUnspecifiedReliabilityClass;
	iMinReliability = EUnspecifiedReliabilityClass;
	iReqPeakThroughput = EUnspecifiedPeakThroughput;
	iMinPeakThroughput = EUnspecifiedPeakThroughput;
	iReqMeanThroughput = EUnspecifiedMeanThroughput;
	iMinMeanThroughput = EUnspecifiedMeanThroughput;

	}

EXPORT_C RPacketQoS::TQoSGPRSNegotiated::TQoSGPRSNegotiated()
:TPacketDataConfigBase()
	{
	iExtensionId = KConfigGPRS;

	iPrecedence = EUnspecifiedPrecedence;
	iDelay = EUnspecifiedDelayClass;
	iReliability = EUnspecifiedReliabilityClass;
	iPeakThroughput = EUnspecifiedPeakThroughput;
	iMeanThroughput = EUnspecifiedMeanThroughput;

	}

