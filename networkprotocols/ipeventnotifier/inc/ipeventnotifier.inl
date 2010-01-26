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
 @file ipeventnotifier.inl
 @internalComponent
*/


#ifndef __IPEVENTNOTIFIER_INL__
#define __IPEVENTNOTIFIER_INL__

inline TBool CIPEventNotifier::IsInterfaceKnown(TUint32 aIfIndex) const
	{
	return FindPositionOfInterface(aIfIndex) != KErrNotFound;
	}



#endif // __IPEVENTNOTIFIER_INL__
