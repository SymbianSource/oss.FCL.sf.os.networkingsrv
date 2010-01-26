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

#ifndef __GENERICSCPRPARAMS_INL__
#define __GENERICSCPRPARAMS_INL__

// implementation, to be copied to .inl file
namespace GenericScprParameters
{
TAny* TChannel::UserPtr() 
/** 
    Holds a pointer to the buffer.
    	
	@return TUint8*.
*/
	{
	return &iBuf;
	}

void TChannel::operator = (TChannel& aParam)
	{
	Mem::Copy(iBuf, aParam.iBuf,KMaxServiceParameterSize);
	}
}

#endif	// __GENERICSCPRPARAMS_INL__
