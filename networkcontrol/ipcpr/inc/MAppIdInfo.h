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
// MAppIdInfo definition file.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef MAPPID_INFO_H
#define MAPPID_INFO_H
#ifdef SYMBIAN_NETWORKING_UMTSR5
const TUint32 KConnectionAppInfoInterfaceId = 0x102070EF;
class MConnectionAppIdInfo      
/**
 @internalComponent
 @released Since 9.3
 */
{
public:
	// This function is used the get Secure ID of the application
	
	virtual TUint32 GetAppSecureId() = 0; 
};
#endif // SYMBIAN_NETWORKING_UMTSR5

#endif	// MAPPID_INFO_H
