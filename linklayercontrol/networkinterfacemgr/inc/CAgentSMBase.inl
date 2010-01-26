/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Inline functions for base classes in states and state machines.
* 
*
*/



/**
 @file CAGENTSMBASE.inl
 @publishedPartner
 @deprecated since v9.5. Use MCPRs/CPRs/SCPRs instead of agents.
*/


#if !defined(__CAGENTSMBASE_INL__)
#define __CAGENTSMBASE_INL__

inline TBool CAgentSMBase::IsReconnect() const
	{return iIsReconnect;}

inline TBool CAgentSMBase::CallBack() const
	{return iCallBack;}

#endif

