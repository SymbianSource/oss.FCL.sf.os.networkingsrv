// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @internalComponent
*/
#ifndef __PFQOS_STREAM_H__
#define __PFQOS_STREAM_H__

#include <in_sock.h>

#include <comms-infras/eintsock.h>

#include <networking/pfqos.h>
#include "qosparameters.h"


// Build a byte stream message to be sent to PF_QOS socket.
class CPfqosStream : public CBase
{
public:
	static IMPORT_C CPfqosStream* NewL(TUint aBufSize);
	IMPORT_C ~CPfqosStream();
    
	IMPORT_C void Init(TUint8 aMsgType, TUint32 aSeq=0);
	IMPORT_C void AddSelector(TUint8 aProtocol, const TUidType& aUid, TUint32 aPolicyType, TUint32 aIapId, TUint32 aPriority, const TDesC& aName);
	IMPORT_C void AddChannel(TUint32 aChannelId);
	IMPORT_C void AddConfigFile(const TDesC& aName);
	IMPORT_C void AddQoSParameters(const TQoSParameters& aParameters);
	IMPORT_C void AddModulespec(TUint32 aProtocolId, TUint32 aFlags, const TDesC& aModuleName, const TDesC& aFileName, const TDesC8& aConfigData);
	IMPORT_C void AddExtensionPolicy(TDesC8 &aData);
	IMPORT_C void AddExtensionHeader(TUint16 aExtension);
	IMPORT_C void AddSrcAddress(const TInetAddr &anAddr, const TInetAddr &aMask, TUint16 aPortMax);
	IMPORT_C void AddDstAddress(const TInetAddr &anAddr, const TInetAddr &aMask, TUint16 aPortMax);    
	IMPORT_C TInt Send(RSocket &aSocket);    
	IMPORT_C void Send(RSocket &aSocket, TRequestStatus& aStatus);

	IMPORT_C TInt Send(RInternalSocket &aSocket);    
	IMPORT_C void Send(RInternalSocket &aSocket, TRequestStatus& aStatus);
    
protected:

	IMPORT_C CPfqosStream();
	IMPORT_C void ConstructL(TUint aBufSize);
	IMPORT_C void AddAddress(const TInetAddr &anAddr, const TInetAddr &aMask, TUint8 aType, TUint16 aPortMax);
    
protected:
	TUint16 iLength;
	TPtr8 iSendBuf;
	HBufC8* iBuf;
};

#endif
