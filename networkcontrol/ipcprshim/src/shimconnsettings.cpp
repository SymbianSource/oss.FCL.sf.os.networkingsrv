// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file SS_CONNSETTINGS.CPP
*/

#include "shimconnsettings.h"
#include "es_prot.h"

void CConnectionSettingsShim::DoGetBoolSettingL(const TDesC& aSettingName, TBool& aValue, const RMessagePtr2* aMessage )
	{
	iConnectionProvdBase.DoGetBoolSettingL( aSettingName, aValue, aMessage );
	}
	
void CConnectionSettingsShim::DoGetDes16SettingL(const TDesC& aSettingName, TDes16& aValue, const RMessagePtr2* aMessage )
	{
	iConnectionProvdBase.DoGetDes16SettingL( aSettingName, aValue, aMessage );
	}
	
void CConnectionSettingsShim::DoGetDes8SettingL(const TDesC& aSettingName, TDes8& aValue, const RMessagePtr2* aMessage )
	{
	iConnectionProvdBase.DoGetDes8SettingL( aSettingName, aValue, aMessage );
	}
	
void CConnectionSettingsShim::DoGetIntSettingL(const TDesC& aSettingName, TUint32& aValue, const RMessagePtr2* aMessage )
	{
	iConnectionProvdBase.DoGetIntSettingL( aSettingName, aValue, aMessage );
	}
	
HBufC* CConnectionSettingsShim::DoGetLongDesSettingLC(const TDesC& aSettingName, const RMessagePtr2* aMessage )
	{
	return iConnectionProvdBase.DoGetLongDesSettingLC( aSettingName, aMessage );
	}
