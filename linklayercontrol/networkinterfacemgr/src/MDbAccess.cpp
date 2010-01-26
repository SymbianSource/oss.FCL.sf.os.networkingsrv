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
//

#include "MDbAccess.h"

//
//
//
EXPORT_C MCommsDbAccess::~MCommsDbAccess()
	{
	}

	/** Data capability checking */
EXPORT_C TInt MCommsDbAccess::CheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	return DoCheckReadCapability( aField, aMessage );
	}

EXPORT_C TInt MCommsDbAccess::CheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	return DoCheckWriteCapability( aField, aMessage );
	}

