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
//

#include "dummyprotocol.h"

CDummyProtocol::CDummyProtocol()
{
}

CDummyProtocol* 
CDummyProtocol::NewL()
{
	return new(ELeave) CDummyProtocol();
}

CServProviderBase* 
CDummyProtocol::NewSAPL(TUint /*aProtocol*/)
{
	return 0;
}
 CHostResolvProvdBase* 
CDummyProtocol::NewHostResolverL()
 {
	 return 0;
 }

CServiceResolvProvdBase* 
CDummyProtocol::NewServiceResolverL()
{
	return 0;
}

CNetDBProvdBase* 
CDummyProtocol::NewNetDatabaseL()
{
	return 0;
}

CProtocolFamilyBase* 
CDummyProtocol::ProtocolFamily()
{
	return 0;
}

void 
CDummyProtocol::Close()
{
}

void 
CDummyProtocol::Open()
{
}

void 
CDummyProtocol::CloseNow()
{
}

void 
CDummyProtocol::StartSending(CProtocolBase* /*aProtocol*/){}

void 
CDummyProtocol::InitL(TDesC &/*aTag*/){}

void CDummyProtocol::StartL(){}

void 
CDummyProtocol::BindL(CProtocolBase* /* protocol*/, TUint /*id*/){}

void 
CDummyProtocol::BindToL(CProtocolBase* /*protocol*/){}

TInt 
CDummyProtocol::Send(RMBufChain& /*aPDU*/,CProtocolBase* /*aSourceProtocol=NULL*/){return KErrNone;}

TInt 
CDummyProtocol::Send(TDes8& /*aPDU*/,TSockAddr* /*to*/,TSockAddr* /*from=NULL*/,CProtocolBase* /*aSourceProtocol=NULL*/){return KErrNone;}

void 
CDummyProtocol::Process(RMBufChain &,CProtocolBase* /*aSourceProtocol=NULL*/){}

void 
CDummyProtocol::Process(TDes8& /*aPDU*/,TSockAddr * /*from*/,TSockAddr * /*to=NULL*/,CProtocolBase* /*aSourceProtocol=NULL*/){}

TInt 
CDummyProtocol::GetOption(TUint /*level*/,TUint /*name*/,TDes8& /*option*/,CProtocolBase* /*aSourceProtocol=NULL*/)
{
	return KErrNone;
}

TInt 
CDummyProtocol::SetOption(TUint /*level*/,TUint /*name*/,const TDesC8& /*option*/,CProtocolBase* /*aSourceProtocol=NULL*/)
{
	return KErrNone;
}

void 
CDummyProtocol::Error(TInt /*anError*/,CProtocolBase* /*aSourceProtocol=NULL*/){}

void 
CDummyProtocol::Identify(TServerProtocolDesc* /*aProtocolDesc*/)const{}

