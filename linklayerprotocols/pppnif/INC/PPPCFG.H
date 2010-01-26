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

#if !defined(__PPPCONFIG_H__)
#define __PPPCONFIG_H__

#include <e32base.h>

_LIT(PPP_COMPRESSORS, "Compressors");

//
// Compressor Configuration
//
NONSHARABLE_CLASS(CPppCompConfig) : public CBase
	{
public:
friend class CPppCcp;
	CPppCompConfig();
	~CPppCompConfig();
	static CPppCompConfig* NewL();
	void	AddNameL(TPtrC aName);
	void	AddOptionsL(TPtrC aOptions);
	void	AddID(TUint8 aID);
	TPtrC8	Options();
	TInt	OptionsLength();
	TPtrC	Name();
	TUint8	ID();
private:
	TSglQueLink link;
	HBufC*	iName;
	TUint8	iID;
	HBufC8*	iOptions;
	};

#endif // __PPPCONFIG_H__
