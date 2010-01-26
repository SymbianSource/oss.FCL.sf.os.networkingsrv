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
 @internalComponent
*/
#ifndef __MODULE_TIMEOUT_H__
#define __MODULE_TIMEOUT_H__

#include <e32std.h>
#include <e32base.h>

// priority for timeout active object
const TUint KQoSDefaultPriority=10;

// CModuleTimeout is used to delay unloading of modules in order to avoid unnecessary loading/unloading
class CModuleTimeout : public CTimer
	{
public:
	static CModuleTimeout* NewL(TCallBack& aCallback, TInt aPriority = KQoSDefaultPriority);
	inline void RunL();
	void Start(TUint aMicroSeconds);
	void Restart(TUint aMicroSeconds);

protected:
	CModuleTimeout(TCallBack& aCallBack, TInt aPriority); 
	void InitL();

private:
	TCallBack iCallback;
	};

// inline methods
inline void CModuleTimeout::RunL()
	{ iCallback.CallBack(); };

#endif
