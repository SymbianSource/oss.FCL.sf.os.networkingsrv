// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 
#ifndef __UMTSNIFFACTORY__
#define __UMTSNIFFACTORY__

#include <comms-infras/nifif.h>

class CUmtsNifController;
class CUmtsNifIfFactory : public CNifIfFactory 
{
protected:
	virtual void InstallL();
	virtual CNifIfBase* NewInterfaceL(const TDesC& aName);
	
	virtual TInt Info(TNifIfInfo& aInfo, TInt aIndex) const;
	~CUmtsNifIfFactory();
	TUint8 iUniqueNifIdCounter;
	
	CUmtsNifController *iController;
};

#endif //__UMTSNIFFACTORY__
