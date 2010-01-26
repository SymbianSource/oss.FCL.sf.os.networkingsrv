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
//

/**
 @file dhparamreader.h
 @internalTechnology	
*/
#ifndef __DHPARAMREADER_H__
#define __DHPARAMREADER_H__

#include <e32base.h>
#include <asymmetrickeys.h>

class CDHParamReader : public CBase
	{
public:
	static void DecodeDERL(const TDesC& aDerFile,
		RInteger& aPrimeOut, RInteger& aGeneratorOut);
	};

#endif /* __DHPARAMREADER_H__ */
