// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @publishedAll
 @released
*/

#ifndef __PUNYCODECONVERTER_H__
#define __PUNYCODECONVERTER_H__

#include <e32base.h>
#include <e32std.h>
#include <e32def.h>
#include <es_sock.h>
#include <networking/dnd_err.h>

const TInt KIDNMaxName = 256;
typedef TBuf8<KIDNMaxName> TIdnHostName; 

class TPunyCodeDndName : public TIdnHostName
	{
public:
	IMPORT_C TInt IdnToPunycode(const THostName &aName);
	IMPORT_C TInt PunycodeToIdn(TDes &aBuf, const TInt aStart=0);
	
private:
	TInt Encode(const THostName &aName, TInt aIndex);
	TInt Decode(TInt aIndex, TDes &aBuf) const;
		
	};

#endif	// __PUNYCODECONVERTER_H__
