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
//

#ifndef _US_CLISERV_H__
#define _US_CLISERV_H__

#include <e32base.h>

//server name

_LIT(KUmtsSimServerName,"UmtsSimulatorServer");
_LIT(KUmtsSimServerExeFile, "umtssim_server.exe");
_LIT(KUmtsSimServerExePath, "Y:\\System\\Libs\\");

//the server version
const TUint KUmtsSimServMajorVersionNumber=0;
const TUint KUmtsSimServMinorVersionNumber=9;
const TUint KUmtsSimServBuildVersionNumber=0;
//
IMPORT_C TInt StartThread();

//opcodes used in message passing between client and server
const TUint KUmtsSimServRqstMask			= 0xfff00000;
const TUint KUmtsSimServRqstGeneral			= 0x00000000;
const TUint KUmtsSimServRqstPacketService   = 0x00100000;
const TUint KUmtsSimServRqstPacketContext	= 0x00200000;
const TUint KUmtsSimServRqstPacketQoS		= 0x00300000;
const TUint KUmtsSimServRqstControl			= 0x00400000;
const TUint KUmtsSimServRqstCancelBit		= 0x00080000;
enum TUmtsSimServRqst
    {
    EUmtsSimServCloseSession					= KUmtsSimServRqstGeneral | 0x1,
    EUmtsSimServCreatePacketServiceSubSession   = KUmtsSimServRqstPacketService | 0x1,
    EUmtsSimServClosePacketServiceSubSession    = KUmtsSimServRqstPacketService | 0x2,
    EUmtsSimServCreatePacketContextSubSession	= KUmtsSimServRqstPacketContext | 0x1,
    EUmtsSimServClosePacketContextSubSession	= KUmtsSimServRqstPacketContext | 0x2,
    EUmtsSimServCreatePacketQoSSubSession		= KUmtsSimServRqstPacketQoS | 0x1,
    EUmtsSimServClosePacketQoSSubSession		= KUmtsSimServRqstPacketQoS | 0x2,
	EUmtsSimServCreateControlSubSession			= KUmtsSimServRqstControl | 0x1,
	EUmtsSimServCloseControlSubSession			= KUmtsSimServRqstControl | 0x2,

    // From ETel Packet Data Api / PACKET SERVICE
    EUmtsSimServPacketServiceGetStatusS					= KUmtsSimServRqstPacketService | 0x10,
    EUmtsSimServPacketServiceNotifyStatusChangeA		= KUmtsSimServRqstPacketService | 0x11,
	//EUmtsSimServPacketServiceAttachS					= KUmtsSimServRqstPacketService | 0x12,
	EUmtsSimServPacketServiceAttachA					= KUmtsSimServRqstPacketService | 0x13,
	//EUmtsSimServPacketServiceDetachS					= KUmtsSimServRqstPacketService | 0x14,
	EUmtsSimServPacketServiceDetachA					= KUmtsSimServRqstPacketService | 0x15,
	EUmtsSimServPacketServiceEnumerateContextsA			= KUmtsSimServRqstPacketService | 0x16,
	//EUmtsSimServPacketServiceGetContextInfoS			= KUmtsSimServRqstPacketService | 0x17,
	EUmtsSimServPacketServiceGetContextInfoA			= KUmtsSimServRqstPacketService | 0x18,
	EUmtsSimServPacketServiceNotifyContextAddedA		= KUmtsSimServRqstPacketService | 0x19,
	EUmtsSimServPacketServiceEnumerateNifsA				= KUmtsSimServRqstPacketService | 0x1a,
	EUmtsSimServPacketServiceGetNifInfoA				= KUmtsSimServRqstPacketService | 0x1b,
	EUmtsSimServPacketServiceEnumerateContextsInNifA	= KUmtsSimServRqstPacketService | 0x1c,
	EUmtsSimServPacketServiceGetContextNameInNifA		= KUmtsSimServRqstPacketService | 0x1d,

    EUmtsSimServPacketServiceNotifyStatusChangeACancel	= EUmtsSimServPacketServiceNotifyStatusChangeA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceAttachACancel				= EUmtsSimServPacketServiceAttachA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceDetachACancel				= EUmtsSimServPacketServiceDetachA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceNotifyContextAddedACancel	= EUmtsSimServPacketServiceNotifyContextAddedA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceEnumerateContextsACancel	= EUmtsSimServPacketServiceEnumerateContextsA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceGetContextInfoACancel		= EUmtsSimServPacketServiceGetContextInfoA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceEnumerateNifsACancel		= EUmtsSimServPacketServiceEnumerateNifsA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceGetNifInfoACancel			= EUmtsSimServPacketServiceGetNifInfoA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceEnumerateContextsInNifACancel	= EUmtsSimServPacketServiceEnumerateContextsInNifA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketServiceGetContextNameInNifACancel	= EUmtsSimServPacketServiceGetContextNameInNifA | KUmtsSimServRqstCancelBit,

	// From ETel Packet Data Api / PACKET CONTEXT
	//EUmtsSimServPacketContextActivateS					= KUmtsSimServRqstPacketContext | 0x10,
	EUmtsSimServPacketContextActivateA					= KUmtsSimServRqstPacketContext | 0x11,
	//EUmtsSimServPacketContextDeactivateS				= KUmtsSimServRqstPacketContext | 0x12,
	EUmtsSimServPacketContextDeactivateA				= KUmtsSimServRqstPacketContext | 0x13,
	EUmtsSimServPacketContextGetStatusS					= KUmtsSimServRqstPacketContext | 0x14,
	EUmtsSimServPacketContextNotifyStatusChangeA		= KUmtsSimServRqstPacketContext | 0x15,
	//EUmtsSimServPacketContextSetConfigS					= KUmtsSimServRqstPacketContext | 0x16,
	EUmtsSimServPacketContextSetConfigA					= KUmtsSimServRqstPacketContext | 0x17,
	//EUmtsSimServPacketContextGetConfigS					= KUmtsSimServRqstPacketContext | 0x18,
	EUmtsSimServPacketContextGetConfigA					= KUmtsSimServRqstPacketContext | 0x19,
	//EUmtsSimServPacketContextDeleteS					= KUmtsSimServRqstPacketContext | 0x1a,
	EUmtsSimServPacketContextDeleteA					= KUmtsSimServRqstPacketContext | 0x1b,
	EUmtsSimServPacketContextEnumeratePacketFiltersA	= KUmtsSimServRqstPacketContext | 0x1c,
	EUmtsSimServPacketContextGetPacketFilterInfoA		= KUmtsSimServRqstPacketContext | 0x1d,
	EUmtsSimServPacketContextAddPacketFilterA			= KUmtsSimServRqstPacketContext | 0x1e,
	EUmtsSimServPacketContextRemovePacketFilterA		= KUmtsSimServRqstPacketContext | 0x1f,
	EUmtsSimServPacketContextModifyActiveContextA		= KUmtsSimServRqstPacketContext | 0x20,
	EUmtsSimServPacketContextNotifyConfigChangedA		= KUmtsSimServRqstPacketContext | 0x21,
	EUmtsSimServPacketContextInitialiseContextA			= KUmtsSimServRqstPacketContext | 0x22,

	EUmtsSimServPacketContextActivateACancel			= EUmtsSimServPacketContextActivateA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextDeactivateACancel			= EUmtsSimServPacketContextDeactivateA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextNotifyStatusChangeACancel	= EUmtsSimServPacketContextNotifyStatusChangeA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextSetConfigACancel			= EUmtsSimServPacketContextSetConfigA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextGetConfigACancel			= EUmtsSimServPacketContextGetConfigA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextDeleteACancel				= EUmtsSimServPacketContextDeleteA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextNotifyConfigChangedACancel	= EUmtsSimServPacketContextNotifyConfigChangedA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextEnumeratePacketFiltersACancel	= EUmtsSimServPacketContextEnumeratePacketFiltersA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextGetPacketFilterInfoACancel	= EUmtsSimServPacketContextGetPacketFilterInfoA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextAddPacketFilterACancel		= EUmtsSimServPacketContextAddPacketFilterA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextRemovePacketFilterACancel	= EUmtsSimServPacketContextRemovePacketFilterA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextModifyActiveContextACancel	= EUmtsSimServPacketContextModifyActiveContextA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketContextInitialiseContextACancel	= EUmtsSimServPacketContextInitialiseContextA | KUmtsSimServRqstCancelBit,

	// From ETel Packet Data Api / PACKET QOS
	EUmtsSimServPacketQoSSetProfileParametersA			= KUmtsSimServRqstPacketQoS | 0x10,
	EUmtsSimServPacketQoSGetProfileParametersA			= KUmtsSimServRqstPacketQoS | 0x11,
	EUmtsSimServPacketQoSNotifyProfileChangedA			= KUmtsSimServRqstPacketQoS | 0x12,

	EUmtsSimServPacketQoSSetProfileParametersACancel	= EUmtsSimServPacketQoSSetProfileParametersA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketQoSGetProfileParametersACancel	= EUmtsSimServPacketQoSGetProfileParametersA | KUmtsSimServRqstCancelBit,
	EUmtsSimServPacketQoSNotifyProfileChangedACancel	= EUmtsSimServPacketQoSNotifyProfileChangedA | KUmtsSimServRqstCancelBit,

	// From SIMULATION CONTROL
	EUmtsSimServControlNotifyAllA					= KUmtsSimServRqstControl | 0x10,
	EUmtsSimServControlReconfigureSimulatorS		= KUmtsSimServRqstControl | 0x11,
	EUmtsSimServControlConfigureRequestHandlerS		= KUmtsSimServRqstControl | 0x12,
	EUmtsSimServControlConfigureEventS				= KUmtsSimServRqstControl | 0x13,

	EUmtsSimServControlNotifyAllACancel				= EUmtsSimServControlNotifyAllA | KUmtsSimServRqstCancelBit
    };

enum TUmtsSimServContextOpenMode
    {
	EUmtsSimServOpenNewPacketContext,
	EUmtsSimServOpenNewSecondaryPacketContext,
	EUmtsSimServOpenExistingPacketContext
    };

enum TUmtsSimServQoSOpenMode
    {
	EUmtsSimServOpenNewPacketQoS,
	EUmtsSimServOpenExistingPacketQoS
    };

enum TUmtsSimServLeave
    {
    ENonNumericString
    };

#endif
