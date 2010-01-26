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

#ifndef __NIFPDP_DEF_H__
#define __NIFPDP_DEF_H__

#include <e32std.h>

// Define Config file and section
_LIT(KConfigFileDir,"testdata\\configs");
_LIT(KConfigFilename,"PdpConfig.txt");
_LIT8(KSectionNameFormat,"PDP%d");
_LIT8(KSBLPSectionNameFormat,"SBLP%d");
_LIT(KSBLPTokenFilename,"SblpTokens.txt");

// Definition of the Context operations in test configuration files
_LIT8(KContextConfigSetEntry,"ContextConfigSetEntry");
_LIT8(KContextQosSetEntry,"ContextQosSetEntry");
_LIT8(KContextActivateEntry,"ContextActivateEntry");
_LIT8(KContextModifyActivate,"ContextModifyActivate");
_LIT8(KContextDeleteEntry,"ContextDeleteEntry");
_LIT8(KContextTFTModifyEntry,"ContextTFTModifyEntry");

// Define the Context Name and Qos Name
_LIT8(KContextName,"ContextName");
_LIT8(KQosProfileName,"QosProfileName");

// Negotiate Qos Parameters
_LIT8(KQosNegTrafficClass,"QosNegTrafficClass");
_LIT8(KQosNegDeliveryOrder,"QosNegDeliveryOrder");
_LIT8(KQosNegErroneousSDUDelivery,"QosNegErroneousSDUDelivery");
_LIT8(KQosNegMaxSDUSize,"QosNegMaxSDUSize");
_LIT8(KQosNegUpBitRate,"QosNegUpBitRate");
_LIT8(KQosNegDownBitRate,"QosNegDownBitRate");
_LIT8(KQosNegBitErrorRatio,"QosNegBitErrorRatio");
_LIT8(KQosNegSDUErrorRatio,"QosNegSDUErrorRatio");
_LIT8(KQosNegTrafficHandlingPriority,"QosNegTrafficHandlingPriority");
_LIT8(KQosNegTransferDelay,"QosNegTransferDelay");
_LIT8(KQosNegGuaranteedUpRate,"QosNegGuaranteedUpRate");
_LIT8(KQosNegGuaranteedDownRate,"QosNegGuaranteedDownRate");

#ifdef SYMBIAN_NETWORKING_UMTSR5 
_LIT8(KQosNegSourceStatisticsDescriptor,"QosNegSourceStatisticsDescriptor");
_LIT8(KQosNegSignallingIndication,"QosNegSignallingIndication");
#endif 
// SYMBIAN_NETWORKING_UMTSR5

//Definition of the SBLP Tokens
_LIT8(KAuthorizationToken,"AuthorizationToken");
_LIT8(KFlowIdCount,"NumberofFlowIds");
_LIT8(KFlowIdentifier,"FlowIdentifier%d");



// Define default value
_LIT8(KDefaultName,"");
const TInt KDefaultValue = 0;
const TInt KDefaultTimerPriority = 10;
const TInt KDefaultGranularity = 2;
const TUint KStdDelimiter=',';

const TInt KDefaultDeletionTime = 200;
const TInt KDefaultQosSettingTime = 200;
const TInt KDefaultActivationTime = 200;
const TInt KDefaultModifyActivateTime = 200;
const TInt KDefaultTFTModifyTime = 200;

_LIT8(KDefaultAuthorizationToken,"");
const TInt KDefaultFlowIdCount = 0;
const TInt KDefaultFlowIdValues = 0;

#endif
