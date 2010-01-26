// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IPProto Connection Provider class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_LINKCPREXTENSIONAPI_H
#define SYMBIAN_LINKCPREXTENSIONAPI_H

#include <comms-infras/ss_log.h>
#include <comms-infras/corecpr.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include "IPProtoCPR.h"

/**
@internalTechnology
@prototype
*/
class CLinkCprExtensionApi : public Meta::SMetaData, 
	public ESock::MLinkCprApiExt
    {
public:
    enum 
    	{
    	EUid = 0x102822F6,
    	ETypeId = 1,
    	};

public:
	static CLinkCprExtensionApi* NewLC(CIPProtoConnectionProvider& aCpr);
	void SetLastProgress(const Elements::TStateChange& aStateChange);

private:

	//ESock::MLinkCprExtensionApi
    virtual void ProgressL(TProgressBuf& aBuffer) const;
    virtual void LastProgressError(TProgressBuf& aBuffer);
    virtual TInt EnumerateSubConnectionsL(TUint& aCount);

	virtual TInt AllSubConnectionNotificationEnable();

protected:
    CLinkCprExtensionApi(CIPProtoConnectionProvider& aCpr);

private:
	Elements::TStateChange iLastProgress;
	Elements::TStateChange iLastProgressError;
	CIPProtoConnectionProvider& iCpr;
	
public:
	DATA_VTABLE //public for ALinkCprExtensionApi::TypeId();
    };
    
#endif
//SYMBIAN_LINKCPREXTENSIONAPI_H
