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

#ifndef __ANVLMAIN_H
#define __ANVLMAIN_H

#define ANVL_PRINTF             1
#define ANVL_PRINTF_INT         2
#define ANVL_PRINTF_STR         3
#define ANVL_PRINTF_INT_INT     4
#define ANVL_PRINTF_STR_STR     5
#define ANVL_PRINTF_STR_INT     6

#ifdef __cplusplus
extern "C" {
#endif
    
int mntcpapp_main(unsigned short localPort, int ipVersionSelected); 

#ifdef __cplusplus
}
#endif

#endif
