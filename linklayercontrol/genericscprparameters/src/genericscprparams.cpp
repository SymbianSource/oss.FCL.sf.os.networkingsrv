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

#include "genericscprparams.h"
#include <e32std.h>

using namespace GenericScprParameters;

EXPORT_C TChannel::TChannel()
/** 
    Standard constructor.
  
*/
	{
	}
EXPORT_C void TChannel::SetUserLength(const TInt aLen)
/** 
    Sets the length of the buffer
    	
	@param aLen of type TInt.
*/
    {
    SetLength(aLen);
    }
