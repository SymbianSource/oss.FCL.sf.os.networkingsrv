// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @publishedPartner
 @released since 9.5
*/

#ifndef __GENERICSCPRPARAMS_H__
#define __GENERICSCPRPARAMS_H__

#include <e32base.h>
#include <e32std.h>

namespace GenericScprParameters
{
const TUint KMaxServiceParameterSize=0x100;
class TChannel : public TBuf8<KMaxServiceParameterSize>
/** Holds a buffer to store technology specific information.
*/ 
	{
	public:
	    IMPORT_C TChannel();
	    inline TAny* UserPtr();
	    inline void operator = (TChannel& aParam);
	   	IMPORT_C void SetUserLength(const TInt aLen);
	
	};
}
#include <genericscprparams.inl>
#endif	// __GENERICSCPRPARAMS_H__
