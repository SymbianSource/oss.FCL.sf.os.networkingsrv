// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Logging functions used across the netperf suite device-side code.
// All kept in one place so we can switch easily to using a different logger
// if required.
// 
//

/**
 @file
 @internalTechnology
*/
 
#ifndef __netperftrace_H__
#define __netperftrace_H__

// Varargs support does exist for defined macros, but it's not guaranteed to
//  work the same on all compilers.
#if _NETPERF_TRACE
#define NETPERF_TRACE_1(a1) RDebug::Print(a1)
#define NETPERF_TRACE_2(a1,a2) RDebug::Print(a1,a2)
#define NETPERF_TRACE_3(a1,a2,a3) RDebug::Print(a1,a2,a3)
#define NETPERF_TRACE_4(a1,a2,a3,a4) RDebug::Print(a1,a2,a3,a4)
#define NETPERF_TRACE_5(a1,a2,a3,a4,a5) RDebug::Print(a1,a2,a3,a4,a5)
#define NETPERF_TRACE_6(a1,a2,a3,a4,a5,a6) RDebug::Print(a1,a2,a3,a4,a5,a6)
#define NETPERF_TRACE_7(a1,a2,a3,a4,a5,a6,a7) RDebug::Print(a1,a2,a3,a4,a5,a6,a7)
#else
#define NETPERF_TRACE_1(a1)
#define NETPERF_TRACE_2(a1,a2)
#define NETPERF_TRACE_3(a1,a2,a3)
#define NETPERF_TRACE_4(a1,a2,a3,a4)
#define NETPERF_TRACE_5(a1,a2,a3,a4,a5)
#define NETPERF_TRACE_6(a1,a2,a3,a4,a5,a6)
#define NETPERF_TRACE_7(a1,a2,a3,a4,a5,a6,a7)
#endif

#endif // __netperftrace_H__
