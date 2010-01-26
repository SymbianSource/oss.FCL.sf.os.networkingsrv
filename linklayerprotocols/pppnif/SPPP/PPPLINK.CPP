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

#include "PPPLCP.H"
#include "PPPBASE.H"
#include "PPPHDLC.H"
#include "PppUmts.h"

//
// PPP Link Protocols Base
//

CPppLinkBase::CPppLinkBase(CPppLcp* aLcp)
	: iPppLcp(aLcp)
	{
	}

CPppLinkBase::~CPppLinkBase()
	{
	}

CPppLinkBase* CPppLinkBase::PppCreateLinkL(CPppLcp* aLcp, const TDesC& aNifName)
	{
	//Instantiate relevant next layer
	CPppLinkBase* link=NULL;

	if(aNifName.Length() == 0)
		{
		link = new (ELeave) CPppHdlcLink(aLcp);
		CleanupStack::PushL(link);
		link->CreateL();
		}
	else
		{
		User::Leave(KErrArgument);
		}
	CleanupStack::Pop();
	return link;
	}
