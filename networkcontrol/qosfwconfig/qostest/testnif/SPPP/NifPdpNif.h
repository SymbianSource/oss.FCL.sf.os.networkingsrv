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

#if !defined(__NIFPDPNIF_H__)
#define __NIFPDPNIF_H__

#include "NifPdpBase.h"
#include <nifutl.h>
#include "QosTestNcp.h"

class CNifPDPContextTNif : public CNifPDPContextBase, public MTimer
{
public:
	static CNifPDPContextTNif* NewL(CQoSTestNcp* aNif, TDes8& aConfig);
	static CNifPDPContextTNif* NewLC(CQoSTestNcp* aNif, TDes8& aConfig);
	CNifPDPContextTNif(CQoSTestNcp* aNif, TDes8& aConfig);
	~CNifPDPContextTNif();
	void ConstructL();
	virtual void ActivatePrimaryContext(TDes8& aConfig);
	void DoCancel();
	virtual void TimerComplete(TInt aStatus);

	const CTestConfigSection*	CfgFile();
	

private:
	// To handle different PDP context operations.
	virtual TInt HandlePDPCreate(TDes8& aConfig);
	virtual TInt HandlePDPDelete(TDes8& aConfig);
	virtual TInt HandleQosSet(TDes8& aConfig);
	virtual TInt HandleContextActivate(TDes8& aConfig);
	virtual TInt HandleModifyActive(TDes8& aConfig);
	virtual TInt HandleTFTModify(TDes8& aConfig);

	// To handle the Pdp Context Operation Completion.
	virtual void PDPCreateComplete();
	virtual void PdpDeleteComplete();
	virtual void PdpActivationComplete();
	virtual void PdpQosSetComplete();
	virtual void PdpModifyActiveComplete();
	virtual void PdpPrimaryComplete();
	virtual void PdpTFTModifyComplete();


	// The section name for this PDP Context in the configuration file. 
	TBuf8<KMaxName>		iSectionName;
	
	//SBLP Parameter
	RPointerArray<RPacketContext::CTFTMediaAuthorizationV3> iSblpParams;
};

#endif




