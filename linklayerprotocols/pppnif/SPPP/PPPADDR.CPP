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

#include "PPPSOCK.H"

//
// PPP Socket Address
//

EXPORT_C TPppAddr::TPppAddr()
	: TSockAddr()
//
//
//
	{
	SetFamily(KAfPpp);
	SetUserLen(sizeof(SPppLinkAddr));
	SetProtocol(KPppIdUnknown);
	SetAddress(0);
	SetControl(0);
	SetCRCError(EFalse);
	SetPhase(EPppPhaseAll);
	}

EXPORT_C TPppAddr& TPppAddr::operator = (const TSockAddr& aAddr)
	{
	TPppAddr& src = TPppAddr::Cast(aAddr);
	SetUserLen(sizeof(SPppLinkAddr));
	SetFamily(src.Family());
	SetPort(src.Port());
	SetProtocol(src.GetProtocol());
	SetAddress(src.GetAddress());
	SetControl(src.GetControl());
	SetCRCError(src.CRCError());
	SetPhase(src.GetPhase());
	return *this;
	}
