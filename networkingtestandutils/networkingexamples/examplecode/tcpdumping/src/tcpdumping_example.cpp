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
// Packet Logger Example Code
// To enable packet logging, ensure that the C:\logs\HookLogs\ directory is present.
// Logging will be silently disabled otherwise.
// 
//

#include "in_bind.h"
#include "posthook.h"
#include "dumper.h"

const TUint16 KProtocolTcpdumping	= 0xfb5;	

enum TTcpdumpingPanic
	{
	ETcpdumpingPanicBadBind,
	ETcpdumpingPanicNotSupported
	};

_LIT(KTcpdumping, "tcpdumping");
void Panic(TTcpdumpingPanic aPanic)
	{
	User::Panic(KTcpdumping, aPanic);
	}

/******************************************************************************
******************************************************************************
********************class CProtocolTcpdumping**********************************
******************************************************************************
******************************************************************************/

static void FillIdentification(TServerProtocolDesc& aDesc);

class CProtocolTcpdumping : public CProtocolPosthook
	{
public:
	static CProtocolTcpdumping* NewL();
	~CProtocolTcpdumping();

	void Identify(TServerProtocolDesc *aDesc) const;
	void NetworkAttachedL();
	TInt Send(RMBufChain &aPacket, CProtocolBase* aSrc);
	void Process(RMBufChain &aPacket, CProtocolBase* aSrc);

private:
	CPktLogger* iTcpDumper;
	HBufC8* iBuf;

private:
	void Dump(RMBufChain &aPacket);
	CProtocolTcpdumping();
	void ConstructL();
	};

CProtocolTcpdumping* CProtocolTcpdumping::NewL()
	{
	CProtocolTcpdumping* self = new (ELeave) CProtocolTcpdumping;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

void CProtocolTcpdumping::Dump(RMBufChain &aPacket)
	{
	TInt offset = aPacket.First()->Length();
	TInt length = aPacket.Length() - offset;

	// Allocate a buffer large enough.  For efficiency, re-use the
	// same buffer, increasing its size as required.
	if (iBuf == NULL || iBuf->Des().MaxLength() < length)
		{
		HBufC8* newBuf = HBufC8::New(length);
		if (newBuf)
			{
			delete iBuf;
			iBuf = newBuf;
			}
		else
			{
			return;
			}
		}

	TPtr8 buf = iBuf->Des();
	buf.SetLength(length);
	aPacket.CopyOut(buf, offset);

	iTcpDumper->DumpLibPCapRecordHeader( length, length );
	iTcpDumper->DumpLibPCapRecordData(buf);
	}

TInt CProtocolTcpdumping::Send(RMBufChain &aPacket, CProtocolBase* aSrc)
	{
	if (iTcpDumper)
		{
		Dump(aPacket);
		}
	return CProtocolPosthook::Send(aPacket,aSrc);
	}

void CProtocolTcpdumping::Process(RMBufChain &aPacket, CProtocolBase* aSrc)
	{
	if (iTcpDumper)
		{
		Dump(aPacket);
		}
	CProtocolPosthook::Process(aPacket,aSrc);
	}

CProtocolTcpdumping::CProtocolTcpdumping()
	{
	}

void CProtocolTcpdumping::ConstructL()
	{
	// If the logging directory is not present, CPktLogger::NewL() will leave and
	// iTcpDumper will remain as NULL.  In this case, we will silently not dump packets.
	TRAP_IGNORE(iTcpDumper = CPktLogger::NewL());
	}

CProtocolTcpdumping::~CProtocolTcpdumping()
	{
	delete iBuf;
	delete iTcpDumper;
	}

void FillIdentification(TServerProtocolDesc& anEntry)
	{
	anEntry.iName=_S("tcpdumping");
	anEntry.iAddrFamily=KAfInet; //KAfExain;
	anEntry.iSockType=KSockDatagram;
	anEntry.iProtocol=KProtocolTcpdumping;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=KSIDatagram | KSIConnectionLess;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=KSocketNoSecurity;
	anEntry.iMessageSize=0xffff;
	anEntry.iServiceTypeInfo=0;
	anEntry.iNumSockets=KUnlimitedSockets;
	}

void CProtocolTcpdumping::Identify(TServerProtocolDesc *aDesc) const
	{
	FillIdentification(*aDesc);
	}

void CProtocolTcpdumping::NetworkAttachedL()
	{
	NetworkService()->BindL((CProtocolBase*)this, BindPostHook());
	NetworkService()->BindL((CProtocolBase*)this, BindPreHook());
	}


/******************************************************************************
******************************************************************************
********************class CProtocolFamilyTcpdumping****************************
******************************************************************************
******************************************************************************/

class CProtocolFamilyTcpdumping : public CProtocolFamilyBase
	{
public:
	CProtocolFamilyTcpdumping();
	~CProtocolFamilyTcpdumping();
	TInt Install();
	TInt Remove();
	TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	CProtocolBase* NewProtocolL(TUint /*aSockType*/, TUint aProtocol);
	};

CProtocolFamilyTcpdumping::CProtocolFamilyTcpdumping()
	{
	__DECLARE_NAME(_S("CProtocolFamilyTcpdumping"));
	}

CProtocolFamilyTcpdumping::~CProtocolFamilyTcpdumping()
	{
	}

TInt CProtocolFamilyTcpdumping::Install()
	{
	return KErrNone;
	}

TInt CProtocolFamilyTcpdumping::Remove()
	{
	return KErrNone;
	}

TUint CProtocolFamilyTcpdumping::ProtocolList(TServerProtocolDesc *& aProtocolList)
	{
	TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[1]; // Esock catches this leave
	FillIdentification(p[0]);
	aProtocolList = p;
	return 1;
	}

CProtocolBase* CProtocolFamilyTcpdumping::NewProtocolL(TUint /*aSockType*/,
												   TUint aProtocol)
	{
	if (aProtocol != KProtocolTcpdumping)
		User::Leave(KErrNotSupported);

	return CProtocolTcpdumping::NewL();
	}

//
// Entrypoint
//
GLDEF_C TInt E32Dll()
	{
	return KErrNone;
	}

// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase* Install(void); }
EXPORT_C CProtocolFamilyBase* Install(void)
	{
	CProtocolFamilyTcpdumping* protocol = new CProtocolFamilyTcpdumping();
	if (protocol)
		{
		return protocol;
		}
	else 
		{
		return NULL;
		}
	}
