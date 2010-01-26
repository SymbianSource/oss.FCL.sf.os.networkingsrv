// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Psydo protocol class. PPP's supposed to Bind it. As result 
// we can catch the end of PPP negotiation without using real protocols.
// 
//

#ifndef __DUMMYPROTOCOL_H__
#define __DUMMYPROTOCOL_H__

#include "es_prot.h"

//The main principle behind it: do nothing

class CDummyProtocol:public CProtocolBase
{
public:
	static CDummyProtocol* NewL();

	virtual CServProviderBase* NewSAPL(TUint aProtocol);
	virtual CHostResolvProvdBase* NewHostResolverL();
	virtual CServiceResolvProvdBase* NewServiceResolverL();
	virtual CNetDBProvdBase* NewNetDatabaseL();
	CProtocolFamilyBase* ProtocolFamily();
	virtual void Close();
	virtual void Open();
	virtual void CloseNow();
	virtual void StartSending(CProtocolBase* aProtocol);
	virtual void InitL(TDesC &aTag);
	virtual void StartL();

	virtual void BindL(CProtocolBase* protocol, TUint id);
	virtual void BindToL(CProtocolBase* protocol);
	virtual TInt Send(RMBufChain& aPDU,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt Send(TDes8& aPDU,TSockAddr* to,TSockAddr* from=NULL,CProtocolBase* aSourceProtocol=NULL);
	virtual void Process(RMBufChain &,CProtocolBase* aSourceProtocol=NULL);
	virtual void Process(TDes8& aPDU,TSockAddr *from,TSockAddr *to=NULL,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt GetOption(TUint level,TUint name,TDes8& option,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt SetOption(TUint level,TUint name,const TDesC8& option,CProtocolBase* aSourceProtocol=NULL);
	virtual void Error(TInt anError,CProtocolBase* aSourceProtocol=NULL);

// Pure virtual
	virtual void Identify(TServerProtocolDesc* aProtocolDesc)const;

private:
	CDummyProtocol();
};

#endif //__DUMMYPROTOCOL_H__

