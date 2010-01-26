// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file servcertwithdialogstep.h
 @internalTechnology	
*/
#ifndef __SERVCERTWITHDIALOGSTEP_H__
#define __SERVCERTWITHDIALOGSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KServCertWithDialogStep, "ServCertWithDialogStep");

class CServCertWithDialogStep : public CTlsStepBase
	{
public:
	CServCertWithDialogStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	// "Yes" or "Not" to be set in ini file.
	TPtrC iDialogOption;
	};


#endif /* __SERVCERTWITHDIALOGSTEP_H__ */
