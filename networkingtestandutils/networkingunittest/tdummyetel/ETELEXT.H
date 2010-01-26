// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#if !defined(__ETELEXT_H__)
#define __ETELEXT_H__

#if !defined(__E32BASE_H__)
#include <e32base.h>
#endif

/**
TEtelDataPackageType
Data package description that passing between client to Server
specified in p[1]
@internalComponent
*/
enum TEtelDataPackageType
	{
	EIsaNull =0,
	EIsaDesTobeSet =1,
	EIsaDesTobeRead =2,
	EIsaDoubleDesTobeSet =3,
	EIsaDoubleDesTobeRead =4,
	EIsaCancelMessage =5,
	EIsaCancelSubSession =6,
	EIsaNarrowAndUnicodeDoubleDesTobeSet = 7,	// narrow comes first then unicode
	EIsaNarrowAndUnicodeDoubleDesTobeRead = 8,
	EIsaNarrowDesToSetAndGet = 9,
	EIsaNarrowDesToGetUnicodeDesToSet = 10,

	EIsaUnicodeDesTobeSet =0x0101,
	EIsaUnicodeDesTobeRead = 0x0102,
	EIsaUnicodeDoubleDesTobeSet =0x0103,
	EIsaUnicodeDoubleDesTobeRead =0x0104,

	EIsPriorityClientReqWithNull =0x1000,
	EIsPriorityClientReqWithaDesTobeSet =0x1001,
	EIsPriorityClientReqWithaDesTobeRead =0x1002,
	EIsPriorityClientReqWithaDoubleDesTobeSet =0x1003,
	EIsPriorityClientReqWithaDoubleDesTobeRead =0x1004,
	EIsPriorityClientReqWithaNarrowAndUnicodeDoubleDesTobeSet = 0x1007,
	EIsPriorityClientReqWithaNarrowAndUnicodeDoubleDesTobeRead = 0x1008,
	EIsPriorityClientReqWithaNarrowDesToSetAndGet = 0x1009,
	EIsPriorityClientReqWithaNarrowDesToGetUnicodeDesToSet  = 0x1010,

	EIsPriorityClientReqWithaUnicodeDesTobeSet =0x1101,
	EIsPriorityClientReqWithaUnicodeDesTobeRead = 0x1102,
	EIsPriorityClientReqWithaUnicodeDoubleDesTobeSet = 0x1103,
	EIsPriorityClientReqWithaUnicodeDoubleDesTobeRead = 0x1104

	};

/**
@internalComponent
*/
const TInt KUnicodeReq			=0x0100;
const TInt KPriorityClientReq	=0x1000;

/**
ETel Panics
@internalComponent
*/
enum TEtelPanic
	{
	EEtelPanicBadPhoneHandle,
	EEtelPanicBadDescriptor,
	EEtelPanicInvalidRequestType,
	EEtelPanicRequestAsyncTwice,
	EEtelPanicBadSubSessionHandle,
	EEtelPanicZeroLengthDes,
	EEtelPanicDesLengthNegative,
	EEtelPanicNullPtrArray,
	EEtelPanicNullHandle,
	EEtelPanicHandleNotOpen,
	EEtelPanicIndexOutOfRange,
	EEtelPanicHandleNotClosed,
	EEtelPanicBadCallGroupType,
	EEtelPanicBadName
	};

/**
ETel Faults
@internalComponent
*/
enum TEtelFault
	{
	EEtelFaultMainSchedulerError,
	EEtelFaultSvrCreateServer,
	EEtelFaultSvrStartServer,
	EEtelFaultCommServerNotRunning,
	EEtelFaultCreateTrapCleanup,
	EEtelFaultCompletionReceivedWithNoReqPackage,
	EEtelFaultWaitingReqLostCObject,
	EEtelFaultCompleteReqWithoutReqEntry,
	EETelFaultRequestWithoutBuffer,
	EEtelFaultCallTsyServiceWithoutReqPackage,
	EEtelFaultMessageTypeCorrupted,
	EEtelFaultNotRecognisedTsyHandle,
	EEtelFaultInvalidIpcForCancel,
	EEtelFaultCanNotRelinquishOwnership,
	EEtelFaultBadTelSessionPointer,
	EEtelFaultBadSubSessionHandle,
	EEtelFaultBadPhoneFactoryPointer,
	EEtelFaultBadPhonePointer,
	EEtelFaultBadTelObjectPointer,
	EEtelFaultDes1DoesNotExist,
	EEtelFaultDes2DoesNotExist,
	EEtelFaultNegativeFlowcontrolCount,
	EEtelFaultNegativeSessionCount,
	EEtelFaultNegativeActiveReqCount,
	EEtelFaultBadMessageType,
	EEtelFaultClientNotPostedNotification,
	EEtelFaultUpdatingBufferOfDifferentIpc,
	EEtelFaultUpdatingIdenticalBuffer,
	EEtelFaultMaxDesLengthNegative,
	EEtelFaultPreviousReqNonNull,
	EEtelFaultNullBooleanArrayPointer,
	EEtelFaultCancelErrorWithoutCancelled
	};

/**
@internalComponent
*/
#define ETEL_SERVER_NAME	  _L("DummyEtelServer")

#endif
