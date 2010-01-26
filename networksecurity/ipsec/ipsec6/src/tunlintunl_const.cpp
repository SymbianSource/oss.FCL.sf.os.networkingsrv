/*
* Copyright (c) 2007-2008 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


#include <e32def.h>

#ifndef __WINS__
/*
@SYMPatchable This constant controls the tunnel-in-tunnel feature. 
              Value 1 indicates the feature is on and value 0 is off
              Default is 0. Any other value still defaults to 0 ie., 
              the feature would be turned off.
*/

EXPORT_C extern const TUint32 KIPsecTunnelInTunnel = 0;

#endif /*__WINS__*/

