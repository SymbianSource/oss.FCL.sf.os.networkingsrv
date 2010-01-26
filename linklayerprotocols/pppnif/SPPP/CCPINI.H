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
 @file
 @internalComponent 
*/

#if !defined(__PPPINI_H__)
#define __PPPINI_H___

#include <es_ini.h>

_LIT(CCP_INI_DATA, "ccp.ini");

class CPppCompConfig;
NONSHARABLE_CLASS(CPppIniData) : public CESockIniData
	{
public:
	static CPppIniData* NewL();
	virtual ~CPppIniData();
	TBool ReadCompressorInfo(CPppCompConfig* aData, TUint aNumberOfTurns);
protected:
	void ConstructL();
	CPppIniData();
protected:
	TPtr iSearchPtr;
	};

#endif //__PPPINI_H__
