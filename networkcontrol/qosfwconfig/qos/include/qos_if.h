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
// qos_if.h - CNifIfBase::Control() API between the QoS framework and Nif interfaces
// CNifIfBase::Control() API between the QoS framework and Nif interfaces
//



/**
 @file qos_if.h
 @internalTechnology
 @released
*/

#ifndef __QOS_IF_H__
#define __QOS_IF_H__

#include <in_iface.h>

//	CNifIfBase::Control(aLevel, aName, aOption, ..)
//  aLevel is KSOLInterface defined in in_iface.h in standard EPOC


//	QoS specific Option names
const TInt KSoIfControllerPlugIn	= 999;	// Get the name of QoS plugin module

class TSoIfControllerInfo					// aOption when aName == KSoIfControllerPlugIn
{
public:
	TFileName iPlugIn;						// Name of QoS plugin module
	TUint iProtocolId;						// Protocol Id of plugin module
};


#endif
