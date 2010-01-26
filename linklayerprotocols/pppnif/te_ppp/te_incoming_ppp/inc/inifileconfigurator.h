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
// Interface for CIniFileConfigurator class 
// 
//

/**
 @file 
 @internalComponent
*/

#ifndef __INIFILECONFIGURATOR_H__
#define __INIFILECONFIGURATOR_H__

#include <e32base.h> 
#include <e32std.h> 
#include <f32file.h>

// ****** TODO: refactor into base "ini configurator" and derived "ppp ini configurator"

namespace te_ppploopback 
{

/**
 * Utility to write PPP ini configuration files.
 * @internalComponent
 * @test
 */
class CIniFileConfigurator: public CBase
  	{
public:
	static CIniFileConfigurator* NewLC(TDesC& aIniFileName);
	static CIniFileConfigurator* NewL(TDesC& aIniFileName);
	~CIniFileConfigurator();
	
	void CreateFileSectionL(const TDesC& aSectionName);	

	void EnableMaxFailureL();	
	void SetMaxFailureCountL(TDesC& aValue);
	
	void EnableMaxRestartL();
	void SetMaxRestarteCountL(TDesC& aValue);
	
	void EnableRestartTimerL();
	void SetMaxRestartPeriodL(TDesC& aValue);
	
	void EnableLrdL();
	void SetLrdPeriodL(TDesC& aValue);
	
	void CreateEntryL(const TDesC& aEntry, const TDesC& aValue);
			

protected:
	CIniFileConfigurator();
	void ConstructL(TDesC& aIniFileName);
	void WriteUTF7L(const TDesC& aCharacters);
	

private:
	RFs iFileServerSess;
	RFile iIniFile;	
	};
	


} // namespace te_ppploopback

#endif
