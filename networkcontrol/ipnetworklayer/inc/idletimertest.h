// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 
#if !defined(IDLETIMERTEST_H_INCLUDED)
#define IDLETIMERTEST_H_INCLUDED

#include <es_sock.h>
#include <es_sock_partner.h>

/**
   Base constant for options that set Idle Timeouts. Used only to define other options.
   @internalTechnology
*/
const TInt KTestSoDummyNifSetNifmanTimeout(125); 

/**
   Set Short (LastSessionClosed) Idle timeout. Level KCOLInterface.
   @internalTechnology
*/
const TInt KTestSoDummyNifSetLastSessionClosedTimeout( (KTestSoDummyNifSetNifmanTimeout + 1) | KConnReadUserDataBit);

/**
   Set Medium (LastSocketClosed) Idle Timeout. Level KCOLInterface.
   @internalTechnology
*/
const TInt KTestSoDummyNifSetLastSocketClosedTimeout(  (KTestSoDummyNifSetNifmanTimeout + 2) | KConnReadUserDataBit);

/**
   Set Long (LastSocketActivity) Idle Timeout. Level KCOLInterface.
   @internalTechnology
*/
const TInt KTestSoDummyNifSetLastSocketActivityTimeout((KTestSoDummyNifSetNifmanTimeout + 3) | KConnReadUserDataBit);

#endif // IDLETIMERTEST_H_INCLUDED