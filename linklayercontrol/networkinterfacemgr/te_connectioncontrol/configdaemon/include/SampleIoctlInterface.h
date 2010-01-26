/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Implements the interface for of a user-defined Ioctl for a configuration daemon.
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __SAMPLEIOCTLINTERFACE_H__
#define __SAMPLEIOCTLINTERFACE_H__

#include <nifman.h>


/** Start of user Ioctl space. */
const TInt KStartOfUserIoctlNames = 200;


/** Sample user Ioctl option name. */
const TInt KSampleDaemonOptionName1 = KConnWriteUserDataBit | KConnReadUserDataBit | (KStartOfUserIoctlNames + 0);
/** Sample user Ioctl option name. */
const TInt KSampleDaemonOptionName2 = KConnWriteUserDataBit | KConnReadUserDataBit | (KStartOfUserIoctlNames + 1);


/** The start of the extension IDs for the sample. */
const TInt KStartOfExtIDs = 0;


/**
 * Possible values of iExtensionId for sample Ioctl parameter.
 *
 * @internalComponent
 */
enum TParameterExtensionIds
	{
	/** A sample parameter */
	ESampleDaemonParameter1V1 = KStartOfExtIDs,
	/** Another sample parameter */
	ESampleDaemonParameter2V1
	};


class TSampleDaemonBaseParameter
/**
 * Sample daemon parameter base class.
 *
 * @internalComponent
 */
	{
public:
	TInt32 ExtensionId() const
	/**
	 * Returns the extension id of the parameter.
	 *
	 * @internalComponent
	 *
	 * @return The extension id of the parameter
	 */
		{
		return iExtensionId;
		}
protected:
	TSampleDaemonBaseParameter()
	/**
	 * Constructor.
	 *
	 * @internalComponent
	 */
		{
		}
protected:
	/** Version provides a unique value for each distinct version of a derived class. */	
	TInt32 iExtensionId;
	};


class TSampleDaemonParameter1 : public TSampleDaemonBaseParameter
/**
 * Sample daemon parameter.
 *
 * @internalComponent
 */
	{
public:
	TSampleDaemonParameter1()
	/**
	 * Constructor.
	 *
	 * @internalComponent
	 */
		{
		iExtensionId = ESampleDaemonParameter1V1;
		}
public:
	/** Sample simple type. */	
	TInt32 iSomeValue;
	/** Sample buffer. */	
	TBuf8<10> iSomeBuf;
	};


/** Typedef for sample Ioctl parameter. */
typedef TPckg<TSampleDaemonParameter1> TSampleDaemonParameter1Pckg;

#endif


