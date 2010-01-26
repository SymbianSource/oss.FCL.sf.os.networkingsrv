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
// This file specifies the constants which are used to configure
// the NetUps during testing. The associated enumerations are to
// to be found in netupstls.h.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSCONST_H
#define NETUPSCONST_H

namespace NetUps
{
#ifndef __WINS__
#include <e32def.h>

/** @SYMPatchable Disable UPS IP functionality 
*/
IMPORT_C extern const TUint32 KUpsIpDisabled;

/** @SYMPatchable UPS Session type 
*/
IMPORT_C extern const TUint32 KUpsSessionType;

#endif

} // end of namespace

#endif // NETUPSCONST_H