// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Progress enumerations for dummy nifman agent and interface
// 
//

#ifndef __TNIFPROG_H__
#define __TNIFPROG_H__

#include <nifvar.h>

const TUint KADummySetInitialValue = (1 | KConnReadUserDataBit);
const TUint KADummyIssueStop = 		 (2 | KConnReadUserDataBit);

enum TTNifmanProgress
	{
	// from the CSimpleAgent
	ETNifmanProg2 = KMinAgtProgress,			// used to be 2		(indicates starting Connect())
	ETNifmanProg6,								// used to be 6		(indicates ConnectComplete())

	// from the CSimpleIf
	ETNifmanProg3 = KMinNifProgress,			// used to be 3		(indicates Start() failure or LinkLayerDown(Disconnect)
	ETNifmanProg4,								// used to be 4		(indicates LinkLayerDown(Reconnect)
	ETNifmanProg5,								// used to be 5		(indicates AuthenticateComplete())
	ETNifmanProg7,								// indicates MNifIfNotify::BinderLayerDown() has been called
	ETNifmanProg8,								// indicates CNifIfLink::Restart() has been called
	};

#endif

