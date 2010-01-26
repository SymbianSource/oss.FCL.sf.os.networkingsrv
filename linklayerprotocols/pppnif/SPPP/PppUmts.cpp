// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This module contains the code for the PPP "proxy" class which communicates with the lower Nif.
// 
//

#include "PppUmts.h"
#include "PppUmts.inl"
#include "in_iface.h"

/****************/
/* CProtocolPpp */
/****************/

CProtocolPpp::CProtocolPpp()
{
}

CProtocolPpp* CProtocolPpp::NewL()
{
	return new(ELeave) CProtocolPpp();
}

CProtocolPpp::~CProtocolPpp()
{
}

void CProtocolPpp::SetValue(CPppUmtsLink* aPppUmtsLink)
{
	iPppUmtsLink = aPppUmtsLink;
}

/****************/
/* CPppUmtsLink */
/****************/

CPppUmtsLink::CPppUmtsLink(CPppLcp* aLcp, const TDesC& aParams)
: CPppLinkBase(aLcp)
	{
	iNifName.Copy(aParams);						// save away the lower layer Nif name
	}

CPppUmtsLink::~CPppUmtsLink()
/**
 * Close lower layer Nif.
 */
	{
	// Close the Link layer of the lower Nif.
	if(iUmtsNif)
		iUmtsNif->Close();

	delete iProtocolPpp;
	
	// Close the NCP layer of the lower Nif.  Note: this causes the factory to be closed.
	if(iUmtsNifPppBinder)
		iUmtsNifPppBinder->Close();
	}

void CPppUmtsLink::CreateL()
/**
 * Construct the lower layer Nif and bind it to PPP.
 *
 * Called on PPP startup if CommDb indicates a lower layer Nif is to be used
 * (otherwise HDLC link class is created).
 *
 * @exception leaves if could not allocate memory opr create the Nif.
 */
	{
	iProtocolPpp = CProtocolPpp::NewL();

	iUmtsNif = (CNifIfLink*)Nif::CreateInterfaceL(iNifName,this);
	iUmtsNif->Open();
	_LIT(Kppp,"ppp");
	iUmtsNifPppBinder = iUmtsNif->GetBinderL(Kppp);
	iUmtsNifPppBinder->Open();
	iUmtsNifPppBinder->BindL(iProtocolPpp);
	iProtocolPpp->SetValue(this);
	}

void CPppUmtsLink::Stop(TInt aReason, TBool aLinkDown)
/**
 * Called from PPP to terminate the lower Nif.
 */
	{
	iUmtsNif->Stop(aReason,MNifIfNotify::EDisconnect);
	if(aLinkDown)
		iPppLcp->LinkLayerDown(aReason);
	}

TInt CPppUmtsLink::SpeedMetric()
	{
	TPckgBuf<TSoIfInfo> info_buf;
	TInt err = iUmtsNif->Control(KSOLInterface, KSoIfInfo, info_buf);
	if (err != KErrNone)
		return err;	// No IPv4 support (no change)
	else
		{
		const TSoIfInfo &info = info_buf();
		return info.iSpeedMetric;
		}
	}

void CPppUmtsLink::BinderLayerDown(CNifIfBase*, TInt, TAction)
	{ }
