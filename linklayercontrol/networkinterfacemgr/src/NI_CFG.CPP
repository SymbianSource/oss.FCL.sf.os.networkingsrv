// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file NI_CFG.CPP
*/

#include "IF_DEF.H"
#include <ss_glob.h>
#include <ss_std.h>

NONSHARABLE_CLASS(CLibUnloader) : public CAsyncOneShot
/**
@internalComponent
*/
	{
public:
	static CLibUnloader* NewL(RLibrary &aLibrary);
	inline void Unload();
protected:
	CLibUnloader();
	~CLibUnloader();
	virtual void RunL();
private:
	RLibrary iLib;
	};


void CNifMan::UpdateIniConfigL()
	{
//@@@TODO: INTEGRATE THIS WITH THE YANKEE CHANGES TO REMOVE NIFMAN:: CLIENT CLASS
	}


