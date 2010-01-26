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
// This file provides the implementation of the methods for NetUps Tls
// @internalAll
// @prototype
// 
//

#include <e32svr.h> // Included here, since removal of Platform headers from public headers[f32file.h] for TB92SDK 
#include "netupstls.h"

#include <e32def.h>		  			// defines TInt

#ifdef __WINS__
#include <u32hal.h>
#endif

#include "e32std.h"					// defines TLS, User
#include "es_ini.h"					// defines CESockIniData.

#include "netupsimpl.h" // is this needed here, should relevant definitions be ported

#include "netupsconst.h"
#include "netupsassert.h"

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

class CNetUpsImpl;
	
CNetUpsTls* CNetUpsTls::NewL(TInt32 aServiceId)
	{
	CNetUpsTls* self = new (ELeave) CNetUpsTls();

	CleanupStack::PushL(self);
	self->ConstructL(aServiceId);
	CleanupStack::Pop(self);

	return self;		
	}

void CNetUpsTls::ConstructL(TInt32 aServiceId)
	{
	(void) aServiceId;

#ifdef __WINS__

	// for the emulator allow the constant to be patched via epoc.ini
	TUint32 value;

	TInt ret = UserSvr::HalFunction(EHalGroupEmulator, EEmulatorHalIntProperty, (TAny*)"NETWORKING_UPS_DISABLE", &value);

	if (ret == KErrNone)
		{
		if ((value == ENetUpsEnabled) || (value == ENetUpsDisabled))
			// i.e. Net Ups Enabled.
			{
			iUpsIpActive = static_cast<TNetUpsEnableConst>(value);
			if (iUpsIpActive==ENetUpsDisabled)
				{
				User::Leave(KErrNotSupported);
				}
			}	
		else if (value == EReadStatusFromFile)
			{
			ReadConfigurableSettingsL(EEnabledStatus);
			}
		else 
			{
			User::Leave(KErrCorrupt);
			}
		}
	else
		{
		iUpsIpActive = ENetUpsEnabled;
		}
				
	ret = UserSvr::HalFunction(EHalGroupEmulator, EEmulatorHalIntProperty, (TAny*) "NETWORKING_UPS_SESSION", &value);

	if (ret == KErrNone)
		{
		if ((value == EProcessLifeTime) || (value == ENetworkLifeTime))
			{
			iUpsSessionType = static_cast<TNetUpsSessionConst>(value);
			}
		else if (value == EReadStatusFromFile)
			{
			ReadConfigurableSettingsL(ESessionMode);
			}
		else
			{
			User::Leave(KErrCorrupt);					
			}
		}
	else 
		{
		iUpsSessionType = EProcessLifeTime;
		}				

#else // NOT WINSCW

	iUpsIpActive = static_cast<TNetUpsEnableConst>(KUpsIpDisabled);
	iUpsSessionType = static_cast<TNetUpsSessionConst>(KUpsSessionType); // validate input ?
	if (iUpsIpActive == EReadStatusFromFile)
		{
#ifdef 	_DEBUG
		ReadConfigurableSettingsL(EEnabledStatus);
#else   // _DEBUG
		User::Leave(KErrNotSupported);	
#endif  // _DEBUG
		}
	else if (iUpsIpActive != ENetUpsEnabled)
		{
		User::Leave(KErrNotSupported);				
		}

	if (iUpsSessionType == EReadSessionDefFromFile)
		{
#ifdef 	_DEBUG
		ReadConfigurableSettingsL(ESessionMode);
#endif  // _DEBUG
		}

#endif // __WINS__

	iNetUpsImpl = CNetUpsImpl::NewL();

	if (iUpsSessionType ==  EProcessLifeTime)
		{
		iNetUpsImpl->SetLifeTimeMode(CNetUpsImpl::EProcessLifeTimeMode);	
		}
	else
		{
		iNetUpsImpl->SetLifeTimeMode(CNetUpsImpl::ENetworkLifeTimeMode);		
		}

	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_3(_L("CNetUpsTls %08x:\tConstructL(), aServiceId = %d, iLifeTimeMode = %d"), this, aServiceId, iUpsSessionType);		
	}

void CNetUpsTls::ReadConfigurableSettingsL(TConfigurableSetting aConfigurableSetting)
	{
	__FLOG_1(_L("CNetUpsTls %08x:\tReadConfigurableSettingsL()"), this);	

	CESockIniData* ini = NULL;
	ini=CESockIniData::NewL(_L("netups.ini"));
	CleanupStack::PushL(ini);
	TInt value(0);

	switch(aConfigurableSetting)
		{
		case EEnabledStatus:
			if(ini->FindVar(_L("upsIpMode"),_L("NETWORKING_UPS_DISABLE"), value))
				{
				if (value==ENetUpsEnabled)
					{
					iUpsIpActive = static_cast<TNetUpsEnableConst>(value);
					__FLOG_2(_L("CNetUpsTls %08x:\tReadConfigurableSettingsL(), iUpsIpActive = %d"), this, iUpsIpActive);	
					}
				else if (value==ENetUpsDisabled)
					{
					User::Leave(KErrNotSupported);
					}
				else
					{
					User::Leave(KErrCorrupt);
					}
				}
			break;
		case ESessionMode:
			if(ini->FindVar(_L("upsIpMode"),_L("NETWORKING_UPS_SESSION"), value))
				{
				if ((value >=  EProcessLifeTime) && (value <= ENetworkLifeTime))
					{
					iUpsSessionType = static_cast<TNetUpsSessionConst>(value);
					__FLOG_2(_L("CNetUpsTls %08x:\tReadConfigurableSettingsL(), iUpsSessionType = %d"), this, iUpsSessionType);	
					}
				else
					{
					User::Leave(KErrCorrupt);
					}
				}
			break;
		default:
			User::Panic(KNetUpsPanic, KPanicInvalidLogic);
			break;
		}

	CleanupStack::PopAndDestroy(ini);	
	}

CNetUpsTls::CNetUpsTls()
  : iUpsIpActive(ENetUpsEnabled), iUpsSessionType(EProcessLifeTime)
	{
	}
	
CNetUpsTls::~CNetUpsTls()
	{
	__FLOG_1(_L("CNetUpsTls %08x:\t~CNetUpsTls"), this);	
	delete iNetUpsImpl;
	__FLOG_CLOSE;			
	}

CNetUpsImpl* CNetUpsTls::GetNetUpsImpl()
	{
	__FLOG_2(_L("CNetUpsTls %08x:\tGetNetUpsImpl(), iNetUpsImpl = %08x"), this, iNetUpsImpl);	
	return iNetUpsImpl;
	}		
		
TUint32 CNetUpsTls::GetUpsIpDisabled()
	{
	__FLOG_2(_L("CNetUpsTls %08x:\tGetUpsIpDisabled(), iUpsIpActive = %d"), this, iUpsIpActive);	
	return iUpsIpActive;
	}

TUint32 CNetUpsTls::GetUpsSessionType()  	
	{
	__FLOG_2(_L("CNetUpsTls %08x:\tGetUpsSessionType(), iUpsSessionType = %d"), this, iUpsSessionType);	
	return iUpsSessionType;
	}

}  // end of namespace

