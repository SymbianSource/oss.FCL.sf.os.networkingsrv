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
//

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef AGENTQUERYCONNSETTINGSIMPL_H
#define AGENTQUERYCONNSETTINGSIMPL_H

#include <comms-infras/ss_commsdataobject.h>

class CAgentProvisionInfo;


/**
Provides a basic implementation of MQueryConnSettingsApiExt
*/
class CAgentQueryConnSettingsImpl : public CBase, public ESock::MQueryConnSettingsApiExt
	{
public:
	CAgentQueryConnSettingsImpl(const CAgentProvisionInfo& aProvisionInfo, const ESock::RMetaExtensionContainerC& aAccessPointConfig)
		: iProvisionInfo(aProvisionInfo), iAccessPointConfig(aAccessPointConfig)
		{
		}

	~CAgentQueryConnSettingsImpl();

	// From MQueryConnSettingsApiExt Interface
	virtual TInt GetInt(CommsDat::TMDBElementId aElementId, TUint32& aValue, ESock::MPlatsecApiExt* aPlatsecApiExt);
	virtual TInt GetInt(CommsDat::TMDBElementId aElementId, TInt& aValue, ESock::MPlatsecApiExt* aPlatsecApiExt);
	virtual TInt GetBool(CommsDat::TMDBElementId aElementId, TBool& aValue, ESock::MPlatsecApiExt* aPlatsecApiExt);
	virtual TInt GetText(CommsDat::TMDBElementId aElementId, HBufC8*& aValue, ESock::MPlatsecApiExt* aPlatsecApiExt);
	virtual TInt GetText(CommsDat::TMDBElementId aElementId, HBufC16*& aValue, ESock::MPlatsecApiExt* aPlatsecApiExt);

private:
	TInt AnswerTextQuery(const TDesC8& aSourceValue, HBufC8*& aValue);
	TInt AnswerTextQuery(const TDesC16& aSourceValue, HBufC16*& aValue);

	
private:
	const CAgentProvisionInfo& iProvisionInfo;
	const ESock::RMetaExtensionContainerC& iAccessPointConfig;
	};


#endif
// AGENTQUERYCONNSETTINGSIMPL_H

