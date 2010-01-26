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

#ifndef _USCL_PCKTCS_H__
#define _USCL_PCKTCS_H__

#include "us_cliserv.h"

enum TUmtsSimServRqstCancels
    {
	EPacketNotifyContextAdded=EUmtsSimServPacketServiceNotifyContextAddedACancel,
	EPacketAttach=EUmtsSimServPacketServiceAttachACancel,
	EPacketDetach=EUmtsSimServPacketServiceDetachACancel,
	EPacketNotifyStatusChange=EUmtsSimServPacketServiceNotifyStatusChangeACancel,
	EPacketEnumerateContexts=EUmtsSimServPacketServiceEnumerateContextsACancel,
	EPacketGetContextInfo=EUmtsSimServPacketServiceGetContextInfoACancel,
	EPacketEnumerateNifs=EUmtsSimServPacketServiceEnumerateNifsACancel,
	EPacketGetNifInfo=EUmtsSimServPacketServiceGetNifInfoACancel,
	EPacketEnumerateContextsInNif=EUmtsSimServPacketServiceEnumerateContextsInNifACancel,
	EPacketGetContextNameInNif=EUmtsSimServPacketServiceGetContextNameInNifACancel,
	//EPacketNotifyContextActivationRequested,
	//EPacketRejectActivationRequest,
	//EPacketGetNtwkRegStatus,
	//EPacketNotifyChangeOfNtwkRegStatus=,
	//EPacketGetMSClass,
	//EPacketSetMSClass,
	//EPacketSetPrefBearer,
	//EPacketNotifyMSClassChange,
	//EPacketNotifyDynamicCapsChange,
	EPacketContextInitialiseContext=EUmtsSimServPacketContextInitialiseContextACancel,
	EPacketContextSetConfig=EUmtsSimServPacketContextSetConfigACancel,
	EPacketContextGetConfig=EUmtsSimServPacketContextGetConfigACancel,
	EPacketContextNotifyConfigChanged=EUmtsSimServPacketContextNotifyConfigChangedACancel,
	EPacketContextActivate=EUmtsSimServPacketContextActivateACancel,
	EPacketContextDeactivate=EUmtsSimServPacketContextDeactivateACancel,
	EPacketContextDelete=EUmtsSimServPacketContextDeleteACancel,
	EPacketContextNotifyStatusChange=EUmtsSimServPacketContextNotifyStatusChangeACancel,
	EPacketContextEnumeratePacketFilters=EUmtsSimServPacketContextEnumeratePacketFiltersACancel,
	EPacketContextGetPacketFilterInfo=EUmtsSimServPacketContextGetPacketFilterInfoACancel,
	EPacketContextAddPacketFilter=EUmtsSimServPacketContextAddPacketFilterACancel,
	EPacketContextRemovePacketFilter=EUmtsSimServPacketContextRemovePacketFilterACancel,
	EPacketContextModifyActiveContext=EUmtsSimServPacketContextModifyActiveContextACancel,
	//EPacketContextLoanCommPort,
	//EPacketContextRecoverCommPort,
	//EPacketContextNotifyDataTransferred,
	//EPacketContextGetConnectionSpeed,
	//EPacketContextNotifyConnectionSpeedChange,
	EPacketQoSSetProfileParams=EUmtsSimServPacketQoSSetProfileParametersACancel,
	EPacketQoSGetProfileParams=EUmtsSimServPacketQoSGetProfileParametersACancel,
	EPacketQoSNotifyProfileChanged=EUmtsSimServPacketQoSNotifyProfileChangedACancel,
	//EPacketQoSGetProfileCaps,
	EControlNotifyAll=EUmtsSimServControlNotifyAllACancel
    };

#endif
