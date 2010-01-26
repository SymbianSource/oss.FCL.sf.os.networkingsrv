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
// @file qos_ini.h
// Header file for qos ini data
// @internalTechnology
// @released
//

#ifndef __QOS_INI_H__
#define __QOS_INI_H__

/**
 * @file qos_ini.h
 * 
 * INI File literals
 * 
 * @internalTechnology 
 */

#include <e32std.h>

// the names of the ini-files
_LIT(KQosIniData,           "qos.ini");
_LIT(KQosPoliciesIniFile,   "z:\\private\\101F7989\\esock\\qospolicies.ini");
_LIT(KQosPoliciesIniFileC,  "c:\\private\\101F7989\\esock\\qospolicies.ini");

_LIT(KQosIniConfig,         "qosconfig");       // [qosconfig]
_LIT(KQosIniUnloadDelay,    "unload-delay");    // unload-delay= ms

#endif

