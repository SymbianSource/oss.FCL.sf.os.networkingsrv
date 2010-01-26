// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @internalTechnology
*/
 
#ifndef __TLSCACHECLIENT_H__
#define __TLSCACHECLIENT_H__

#include <e32base.h>
#include <x509cert.h>

#include "tlsclientserver.h"

class RTlsCacheClient : public RSessionBase
	{
public:
	IMPORT_C TInt Open(const CX509Certificate& aCert);
	IMPORT_C TCacheEntryState GetStateL();
	IMPORT_C void SetStateL(TCacheEntryState aState);
	IMPORT_C void RequestNotify(TRequestStatus& aStatus);
	IMPORT_C void Cancel();
	
	};

#endif /* __TLSCACHECLIENT_H__ */
