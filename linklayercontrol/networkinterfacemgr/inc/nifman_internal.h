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
// Interface Manager API
// 
//

/**
 @file nifman_internal.h
*/


#if !defined(__NIFMAN_INTERNAL_H__)
#define __NIFMAN_INTERNAL_H__

#include <es_prot.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <es_prot_internal.h>
#include <es_sock_partner.h>
#endif


/**
Static internal API class
@internalComponent
*/
enum TNifSocketState { ENifBuffers2048=-4, ENifBuffers1024, ENifBuffers512, 
					   ENifSocketNull=-1,
                       ENifSocketCreated=0, ENifSocketOpen=3, ENifSocketConnected=4,
					   ENifSocketError=9 };

class MNifIfUser;
class MNifIfNotify;
class CNifIfBase;
class CNifAgentBase;
class CSockSession;
class CConnectionProvdBase;

/**
@internalTechnology
*/
class MConnectionNotify;
class CConnectionProvdBase;
class Nif
	{
public:
	/** Network protocol support */
	IMPORT_C static void BindL(MNifIfUser& aUser, TAny* aId, TDes& aResult, const TDesC& aName=TPtrC(0,0));
	IMPORT_C static void NetworkLayerClosed(MNifIfUser& aUser);
	IMPORT_C static void StartL(TDes& aResult, const TDesC& aName=TPtrC());
	IMPORT_C static void Stop(const TDesC& aName=TPtrC());
	IMPORT_C static CNifIfBase* CreateInterfaceL(const TDesC& aName, MNifIfNotify* aNotify);
	IMPORT_C static CNifAgentBase* CreateAgentL(const TDesC& aAgentName, const TBool aNewInstance = EFalse);
	IMPORT_C static void CheckInstalledMBufManagerL();
	IMPORT_C static CProtocolBase* IsProtocolLoaded(const TDesC& aName);
	IMPORT_C static void CheckInstalledL();
	IMPORT_C static TInt SetSocketState(TNifSocketState aState, CServProviderBase* aProvd);
	IMPORT_C static void ProgressL(TNifProgress& aProgress, const TDesC& aName=TPtrC());
	IMPORT_C static void Stop(TAny* aId, CNifIfBase* aIf=0);
	IMPORT_C static void ProgressL(TNifProgress& aProgress, TAny* aId, CNifIfBase* aIf=0);
	IMPORT_C static CConnectionProvdBase* NewConnectionL(MConnectionNotify* aConnection, TUint aId);
	/** deprecated function */
	IMPORT_C static CNifIfBase* CreateInterfaceL(const TDesC& aName);
	};

#ifdef _DEBUG
/**
Debug-only option level used to pass test-only option names onto PPP.
@internalTechnology
*/
const TUint KCOLLinkLayerTestLevel = 325;
#endif

/**
@internalTechnology
@released 9.1
*/
const TUint KNifSessionSetConnectionAttempt = KConnInternalOptionBit|8;

/**
@internalTechnology
@released 9.1
*/
const TUint KNifSessionGetConnectionAttempt = KConnInternalOptionBit|9;

/**
@internalTechnology
@released Argus
*/
const TUint KNifSessionSetConnectionProvider = KConnInternalOptionBit|10;

/**
@internalTechnology
@released
@ref RConnection::Ioctl
*/
const TUint KConnSetDhcpRawOptionData = KConnWriteUserDataBit|KConnReadUserDataBit|103;

#endif // __NIFMAN_H__



