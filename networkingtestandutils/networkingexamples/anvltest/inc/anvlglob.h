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

#ifndef __ANVLGLOB_H
#define __ANVLGLOB_H

//#define __MNTCPOS_H__
#include "mntcpos.h"
#include "mntcpapp.h"

struct _AnvlGlob
    {
    struct ApplicationState_s /*ApplicationState_t*/  appState;
    char                tempRecv[MAX_MESG_SIZE + 1];  /* +1 to add NULL at the end */
    void                *printObj;
    FILE                *pfile;
    void                *anvl_main_blk;
    int                 sock_connect_last_status;

	
    };

typedef struct _AnvlGlob AnvlGlob;

AnvlGlob *AnvlCreateGlobalsL(void);
void AnvlDeleteGlobals(void);
AnvlGlob *AnvlGetGlobalsPlusPlus(void);

#ifdef __cplusplus
extern "C" {
#endif
    
AnvlGlob *AnvlGetGlobals(void);

#ifdef __cplusplus
}
#endif

#endif
