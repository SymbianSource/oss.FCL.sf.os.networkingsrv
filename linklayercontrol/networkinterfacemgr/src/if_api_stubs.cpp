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
// if_api.cpp
// Just the stubs
// 
//
//

/**
 @file
*/

#include "IF_DEF.H"
#include <comms-infras/ss_flowbinders.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif

#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManf_p_stubs, "NifManf_p_stubs");
#endif

EXPORT_C CNifIfBase* Nif::CreateInterfaceL(const TDesC& /*aName*/, MNifIfNotify* /*aNotify*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 1));
    User::Leave(KErrNotSupported);
    return NULL;
    }


EXPORT_C CNifIfBase* Nif::CreateInterfaceL(const TDesC&)
	{
	__ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 2));
	User::Leave(KErrNotSupported);
	return NULL;
	}


EXPORT_C CNifAgentBase* Nif::CreateAgentL(const TDesC& /*aAgentName*/, const TBool /*aNewInstance*/ /* = EFalse */)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 3));
	User::Leave(KErrNotSupported);
	return NULL;
    }


EXPORT_C void Nif::BindL(MNifIfUser& /*aUser*/, TAny* /*aId*/, TDes& /*aResult*/, const TDesC& /*aName*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 4));
    User::Leave(KErrNotSupported);
    }


EXPORT_C void Nif::StartL(TDes& /*aResult*/, const TDesC& /*aName*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 5));
    User::Leave(KErrNotSupported);
    }


EXPORT_C void Nif::Stop(const TDesC& /*aName*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 6));
    }


EXPORT_C void Nif::NetworkLayerClosed(MNifIfUser& /*aUser*/)
    {
    }


EXPORT_C void Nif::CheckInstalledMBufManagerL()
    {
    //DEPRECATED.
    }


EXPORT_C CProtocolBase* Nif::IsProtocolLoaded(const TDesC& /*aName*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 7));
    return NULL;
    }


EXPORT_C TInt Nif::SetSocketState(TNifSocketState aState, CServProviderBase* aProvd)
    {
#if defined(SYMBIAN_NETWORKING_LEGACY_COMPATIBILITY_SUPPORT)
	NONSHARABLE_CLASS(CIPProvider) : public CServProviderBase
		{
	public:
		MSocketNotify* Socket()
			{
			return iSocket;
			}
		};
	if(aState<=ENifSocketError && aState>=ENifSocketNull)
		{
		static_cast<CIPProvider*>(aProvd)->Socket()->Error(aState, ESock::MSessionControlNotify::EErrorLegacySupportRequest);
		return KErrNone;
		}
#else
	// Fixing unused local variable warnings.
	(void)aState;
	(void)aProvd;
#endif
    return KErrNotSupported;
    }


EXPORT_C void Nif::ProgressL(TNifProgress& /*aProgress*/, const TDesC& /*aName*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 8));
    User::Leave(KErrNotSupported);
    }


EXPORT_C void Nif::Stop(TAny* /*aId*/, CNifIfBase* /*aIf*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 9));
    }


EXPORT_C void Nif::ProgressL(TNifProgress& /*aProgress*/, TAny* /*aId*/, CNifIfBase* /*aIf*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 10));
    User::Leave(KErrNotSupported);
    }


EXPORT_C CConnectionProvdBase* Nif::NewConnectionL(MConnectionNotify* /*aInterface*/, TUint /*aId*/)
    {
    __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManf_p_stubs, 11));
    User::Leave(KErrNotSupported);
    return NULL;
    }


EXPORT_C void Nif::CheckInstalledL()
    {
	if(!CNifMan::Global())
	    {
		SocketServExt::InstallExtensionL(KNifManModule, TPtrC());
    	}
    }

