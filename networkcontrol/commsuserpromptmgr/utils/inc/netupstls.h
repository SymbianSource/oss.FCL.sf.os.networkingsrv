// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file provides the interface into the Networking UPS TLS Component.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSTLS_H
#define NETUPSTLS_H

#include <e32def.h>			// defines TInt
#include <e32base.h>		// defines CActive

#include "netupsconst.h" 	// defines the patchable constant enums

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
class CNetUpsImpl;

NONSHARABLE_CLASS(CNetUpsTls) : public CBase
	{
private:
	enum TConfigurableSetting
		{
		EEnabledStatus			= 0,
		ESessionMode			= 1
		};

	enum TNetUpsEnableConst
		{
		ENetUpsEnabled  		= 0,
		ENetUpsDisabled 		= 1,
		EReadStatusFromFile 	= 2
		};

	enum TNetUpsSessionConst
		{
		EProcessLifeTime		= 0,
		ENetworkLifeTime 		= 1,	
		EReadSessionDefFromFile = 2
		};
public:
	static CNetUpsTls* NewL(TInt32 aServiceId);
	virtual ~CNetUpsTls();

	// declared but not defined:
	CNetUpsTls(const CNetUpsTls&);
	void operator=(const CNetUpsTls&);
	bool operator==(const CNetUpsTls&);
				
	CNetUpsImpl* GetNetUpsImpl();
	TUint32 GetUpsIpDisabled();  
	TUint32 GetUpsSessionType();  
private:
	void ConstructL(TInt32 aServiceId);
	CNetUpsTls();

	void ReadConfigurableSettingsL(TConfigurableSetting);
private:
	CNetUpsImpl* iNetUpsImpl;
	TNetUpsEnableConst iUpsIpActive;  
	TNetUpsSessionConst iUpsSessionType;  	

	__FLOG_DECLARATION_MEMBER;				
	};
	
} // end of namespace

#endif // NETUPSTLS_H
