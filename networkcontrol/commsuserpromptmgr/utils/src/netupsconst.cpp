// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file provides the definition of the patchable constants
// used in debugging. The associated enumerations are to
// to be found in netupstls.h.
// @internalAll
// @prototype
// 
//

namespace NetUps 
{
#ifndef __WINS__

#include <e32def.h>

/** @SYMPatchable Disable UPS IP functionality
*/
EXPORT_C extern const TUint32 KUpsIpDisabled = 0;  

/** @SYMPatchable UPS Session type 
*/
EXPORT_C extern const TUint32 KUpsSessionType = 0;  

#endif
}
