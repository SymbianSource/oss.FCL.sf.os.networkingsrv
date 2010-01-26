/**
* Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Agent external errors and error strings
* 
*
*/



/**
 @file AGENTERRORS.H
 @internalComponent
*/



#if !defined (__AGENTERRORS_H__)
#define __AGENTERRORS_H__

/**
Agent Errors
@internalComponent
*/
const TInt KAgentErrorBase = -3600;
const TInt KErrAgentDatabaseDefaultUndefined = KAgentErrorBase - 6;
const TInt KErrAgentDatabaseTypeUnknown = KAgentErrorBase - 7;
const TInt KErrAgentDatabaseNotFound = KAgentErrorBase - 8;
const TInt KErrAgentNoGPRSNetwork = KAgentErrorBase -9;
const TInt KErrAgentIncorrectMSClass = KAgentErrorBase -10;
const TInt KErrAgentInadequateSignalStrengh = KAgentErrorBase -11;
const TInt KErrAgentStateMachineNotAvailable = KAgentErrorBase -12;

// Define Error code for when an interface needs to indicate that it's configuration might change
const TInt KErrLinkConfigChanged = -3060;

#endif // #ifndef __AGENTERRORS_H__

