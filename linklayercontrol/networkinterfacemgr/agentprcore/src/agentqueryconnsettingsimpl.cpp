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


#include <metadatabase.h>
#include <commsdattypeinfov1_1.h>
#include <cdblen.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <comms-infras/ss_log.h>

#include "agentqueryconnsettingsimpl.h"
#include "agentmessages.h"
#include "CAgentAdapter.h"

#ifdef __CFLOG_ACTIVE
#define KAgentCprTag KESockConnectionTag
_LIT8(KAgentCprSubTag, "agentcpr");
#endif


namespace ESock
{
	// Not using it so for the time being just forward declare
	class MPlatsecApiExt; 
}


using namespace ESock;


CAgentQueryConnSettingsImpl::~CAgentQueryConnSettingsImpl()
	{
	}



TInt CAgentQueryConnSettingsImpl::GetInt(CommsDat::TMDBElementId aElementId, TUint32& aValue, ESock::MPlatsecApiExt* /*aPlatsecApiExt*/)
	{
	__CFLOG_VAR((KAgentCprTag, KAgentCprSubTag, _L8("CAgentQueryConnSettingsImpl %08x:\tGetInt()"), this));

	if (aElementId == (CommsDat::KCDTIdIAPRecord | CommsDat::KCDTIdRecordTag))
		{
		aValue = iProvisionInfo.IapId();
		return KErrNone;
		}
	else if (aElementId == CommsDat::KCDTIdMaxConnectionAttempts) 
		{
		aValue = iProvisionInfo.ReconnectAttempts();
		return KErrNone;
		}

	CCommsDatIapView* iapView = NULL;
	TInt err;
	TRAP(err, iapView = CCommsDatIapView::NewL(iProvisionInfo.IapId()));
	CleanupStack::PushL(iapView);
	if (err != KErrNone)
		{
        CleanupStack::PopAndDestroy(iapView);
		return err;
		}
	err = iapView->GetInt(aElementId, aValue);
	CleanupStack::PopAndDestroy(iapView);
	return err;
	}


TInt CAgentQueryConnSettingsImpl::GetInt(CommsDat::TMDBElementId aElementId, TInt& aValue, ESock::MPlatsecApiExt* /*aPlatsecApiExt*/)
	{
	__CFLOG_VAR((KAgentCprTag, KAgentCprSubTag, _L8("CAgentQueryConnSettingsImpl %08x:\tGetInt()"), this));

	if (aElementId == (CommsDat::KCDTIdIAPRecord | CommsDat::KCDTIdRecordTag))
		{
		aValue = iProvisionInfo.IapId();
		return KErrNone;
		}
	else if (aElementId == CommsDat::KCDTIdMaxConnectionAttempts) 
		{
		aValue = iProvisionInfo.ReconnectAttempts();
		return KErrNone;
		}
		
	CCommsDatIapView* iapView = NULL;
	TInt err;
	TRAP(err, iapView = CCommsDatIapView::NewL(iProvisionInfo.IapId()));
	CleanupStack::PushL(iapView);
	if (err != KErrNone)
		{
        CleanupStack::PopAndDestroy(iapView);
		return err;
		}
	err = iapView->GetInt(aElementId, aValue);
	CleanupStack::PopAndDestroy(iapView);
	return err;
	}


TInt CAgentQueryConnSettingsImpl::GetBool(CommsDat::TMDBElementId aElementId, TBool& aValue, ESock::MPlatsecApiExt* /*aPlatsecApiExt*/)
	{
	__CFLOG_VAR((KAgentCprTag, KAgentCprSubTag, _L8("CAgentQueryConnSettingsImpl %08x:\tGetBool()"), this));
	
	//[399TODO] RZ: This is a hack that needs fixing. No way we hardcode element ids here.
	if (aElementId == (CommsDat::KCDTIdIAPRecord | CommsDat::KCDTIdRecordTag))
		{
		return KErrArgument;
		}	

	CCommsDatIapView* iapView = NULL;
	TInt err;
	TRAP(err, iapView = CCommsDatIapView::NewL(iProvisionInfo.IapId()));
	CleanupStack::PushL(iapView);
	if (err != KErrNone)
		{
        CleanupStack::PopAndDestroy(iapView);
		return err;
		}
	err = iapView->GetBool(aElementId, aValue);
	CleanupStack::PopAndDestroy(iapView);
	return err;
	}


TInt CAgentQueryConnSettingsImpl::GetText(CommsDat::TMDBElementId aElementId, HBufC8*& aValue, ESock::MPlatsecApiExt* /*aPlatsecApiExt*/)
	{
	__CFLOG_VAR((KAgentCprTag, KAgentCprSubTag, _L8("CAgentQueryConnSettingsImpl %08x:\tGetText()"), this));

	//[399TODO] RZ: This is a hack that needs fixing. No way we hardcode element ids here.
	if (aElementId == (CommsDat::KCDTIdIAPRecord | CommsDat::KCDTIdRecordTag))
		{
		return KErrArgument;
		}	

	CCommsDatIapView* iapView = NULL;
	TInt err;
	TRAP(err, iapView = CCommsDatIapView::NewL(iProvisionInfo.IapId()));
	CleanupStack::PushL(iapView);
	if (err != KErrNone)
		{
        CleanupStack::PopAndDestroy(iapView);
		return err;
		}
	err = iapView->GetText(aElementId, aValue);
	CleanupStack::PopAndDestroy(iapView);
	return err;		
	}


TInt CAgentQueryConnSettingsImpl::GetText(CommsDat::TMDBElementId aElementId, HBufC16*& aValue, ESock::MPlatsecApiExt* /*aPlatsecApiExt*/)
	{
	__CFLOG_VAR((KAgentCprTag, KAgentCprSubTag, _L8("CAgentQueryConnSettingsImpl %08x:\tGetText()"), this));

	//[399TODO] RZ: This is a hack that needs fixing. No way we hardcode element ids here.
	if (aElementId == (CommsDat::KCDTIdIAPRecord | CommsDat::KCDTIdRecordTag))
		{
		return KErrArgument;
		}	

	TInt err;
	switch (aElementId)
		{
		// Fields of type EText to send to the agent
		case CommsDat::KCDTIdPortName:
			{
			RBuf16 buffer;
			err = buffer.Create(KCommsDbSvrMaxFieldLength);
			if (err != KErrNone) 
				{
				return err;
				}
			
			err = iProvisionInfo.AgentAdapter()->ReadPortName(buffer);
			if (err == KErrNone) 
				{
				err = AnswerTextQuery(buffer, aValue);
				}
			buffer.Close();
			return err;
			}
					
		// Fields of type EText to be obtained from the provision info
		case CommsDat::KCDTIdBearerAgent:
			{
			err = AnswerTextQuery(iProvisionInfo.AgentName(), aValue);
			return err;
			}
		}

	CCommsDatIapView* iapView = NULL;
	TRAP(err, iapView = CCommsDatIapView::NewL(iProvisionInfo.IapId()));
	CleanupStack::PushL(iapView);
	if (err != KErrNone)
		{
        CleanupStack::PopAndDestroy(iapView);
		return err;
		}
	err = iapView->GetText(aElementId, aValue);
	CleanupStack::PopAndDestroy(iapView);
	return err;		
	}


TInt CAgentQueryConnSettingsImpl::AnswerTextQuery(const TDesC8& aSourceValue, HBufC8*& aValue)
	{
	aValue = HBufC8::NewMax(aSourceValue.Length());
	if (aValue != NULL)
		{
		*aValue = aSourceValue;
		return KErrNone;
		}
	return KErrNoMemory;
	}
	
TInt CAgentQueryConnSettingsImpl::AnswerTextQuery(const TDesC16& aSourceValue, HBufC16*& aValue)
	{
	aValue = HBufC16::NewMax(aSourceValue.Length());
	if (aValue != NULL)
		{
		*aValue = aSourceValue;
		return KErrNone;
		}
	return KErrNoMemory;
	}
	

	

