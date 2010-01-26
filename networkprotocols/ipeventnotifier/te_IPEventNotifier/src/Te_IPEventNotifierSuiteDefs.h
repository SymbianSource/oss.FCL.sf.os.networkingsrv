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
// This file define all the common values thoughout the test project
// 
//

/**
 @file
 @internalComponent
*/


#if (!defined __TE_IPEVENTNOTIFIER_SUITEDEFS_H__)
#define __TE_IPEVENTNOTIFIER_SUITEDEFS_H__


// these are the names of the possible variables in the test ini file
//
_LIT(KTe_IPENIAPToUse,						    "IAPToUse");
_LIT(KTe_IPENMFlagExpectedResultBool,           "MFlagExpectedResult");
_LIT(KTe_IPENOFlagExpectedResultBool,           "OFlagExpectedResult");
_LIT(KTe_IPENAddressToPush,      				"AddressToPush");
_LIT(KTe_IPENExpectedReady, 					"ExpectedReady");
_LIT(KTe_IPENNetworkInterfacePrefixToMonitor,	"NetworkInterfacePrefixToMonitor");


// constants representing the timeouts for various events
//
const TInt KTenSecondDADCompletionDelay = 10000000;
const TInt KFiveSecondDelayToCatchRouterAdvertisementEveryThreeSeconds = 5000000;

#endif
