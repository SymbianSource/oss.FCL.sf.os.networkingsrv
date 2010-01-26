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
 @file newkeyderivationstep.h
 @internalTechnology	
*/
#ifndef __CNEWKEYDERIVATIONSTEP_H__
#define __CNEWKEYDERIVATIONSTEP_H__

#include "tlsstepbase.h"
#include "newtlsstepbase.h"

//#include <f32file.h>
//#include <tlsprovinterface.h>

#include <asymmetric.h>
#include <asymmetrickeys.h>   
#include <symmetric.h>  
#include <asnpkcs.h>   

_LIT(KNewKeyDerivationStep, "NewKeyDerivationStep");

class CNewKeyDerivationStep : public CNewTlsStepBase
	{
	public:
	
	CNewKeyDerivationStep();
	virtual void doTestL();
	};

#endif /* __CNEWKEYDERIVATIONSTEP_H__ */
