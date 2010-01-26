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

#ifndef __LISTENER_H__
#define __LISTENER_H__

#include "engine.h"
#include "server.h"

#define ANVL_THREAD_NAME                _L("AnvlThreadName")
#define IPV4							4
#define IPV6							6

const TUint KAnvlDefaultHeapSize            =   0x10000;
const TThreadPriority KAnvlThreadPriority   =   EPriorityLess; //EPriorityNormal;

class CAnvltestListener : public CActive
    {
public:
    CAnvltestListener(CAnvltestEngine* control, int ipVersionSelected);
    virtual void ConstructL();
    ~CAnvltestListener();
    TInt StartThread(void);
    static TInt ThreadFunction(TAny* anArg);
    void ProsessPrintf(void);
    void AnvlPrintfDo(int print_type, const char *fmt, int fmt_lth, void *par1, void *par2);
    void anvl_printf(void *obj, const char *fmt, int fmt_lth/*, VA_LIST aList*/);
    void anvl_printf_int(void *obj, const char *fmt, int fmt_lth, int par1);
    void anvl_printf_str(void *obj, const char *fmt, int fmt_lth, char *par1);
    void anvl_printf_int_int(void *obj, const char *fmt, int fmt_lth, int par1, int par2);
    void anvl_printf_str_str(void *obj, const char *fmt, int fmt_lth, char *par1, char *par2);
    void anvl_printf_str_int(void *obj, const char *fmt, int fmt_lth, char *par1, int par2);


//    void HandleCommandL(TInt aCommand);

private:
    // active object stuff, completion and cancel callback functions
    void RunL();
    void DoCancel();

    //RTimer iTimer;

    RThread     iAnvlThread;
    bool        iFirstRunL;
    TThreadId   iOriThreadId;
       
    int         iPrintType;
    const char  *iFmt;
    int         iFmtLth;
    void        *iPar1;
    void        *iPar2;
    

//protected:
public:
    
    CAnvltestEngine *iControl;
    int ipVersion;

	CBrdServer  *iServer;

    };

#endif
