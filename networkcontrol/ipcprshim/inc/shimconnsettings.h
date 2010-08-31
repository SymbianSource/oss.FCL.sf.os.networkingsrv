/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file SS_SHIMCONNSETTINGS.H
 @internalComponent
*/

#if !defined(__SS_SHIMCONNSETTINGS_H__)
#define __SS_SHIMCONNSETTINGS_H__

#include "ss_connsettings.h"

class CConnectionProvdBase;
class CConnectionSettingsShim : public CConnectionSettings
	{

public:
	CConnectionSettingsShim( CConnectionProvdBase& aConnectionProvdBase ) :
		iConnectionProvdBase( aConnectionProvdBase )
		{
		}
	
protected:
	virtual void DoGetBoolSettingL(const TDesC& aSettingName, TBool& aValue, const RMessagePtr2* aMessage );
	virtual void DoGetDes16SettingL(const TDesC& aSettingName, TDes16& aValue, const RMessagePtr2* aMessage );
	virtual void DoGetDes8SettingL(const TDesC& aSettingName, TDes8& aValue, const RMessagePtr2* aMessage );
	virtual void DoGetIntSettingL(const TDesC& aSettingName, TUint32& aValue, const RMessagePtr2* aMessage );
	virtual HBufC* DoGetLongDesSettingLC(const TDesC& aSettingName, const RMessagePtr2* aMessage );
	
private:
	CConnectionProvdBase& iConnectionProvdBase;
	};
	
#endif	// __SS_CONNSETTINGS_H__
