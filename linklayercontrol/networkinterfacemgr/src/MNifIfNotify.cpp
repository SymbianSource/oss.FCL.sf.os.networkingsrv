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
// Implementation of BC & SC 'proof' interfaces to MNifIfNotify.
// 
//

/**
 @file
*/
#include <comms-infras/nifif.h>

/**
 * Read an integer field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadInt
 */
EXPORT_C TInt MNifIfNotify::ReadInt(const TDesC& aField, TUint32& aValue)
	{
	return ReadInt( aField, aValue, NULL );
	}

/**
 * Read an integer field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadInt
 */
EXPORT_C TInt MNifIfNotify::ReadInt(const TDesC& aField, TUint32& aValue, const RMessagePtr2* aMessage )
	{
	return DoReadInt( aField, aValue, aMessage );
	}

/**
 * Write an integer field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteInt
 */
EXPORT_C TInt MNifIfNotify::WriteInt(const TDesC& aField, TUint32 aValue)
	{
	return WriteInt( aField, aValue, NULL );
	}

/**
 * Write an integer field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteInt
 */
EXPORT_C TInt MNifIfNotify::WriteInt(const TDesC& aField, TUint32 aValue, const RMessagePtr2* aMessage)
	{
	return DoWriteInt( aField, aValue, aMessage );
	}

/**
 * Read an 8-bit descriptor field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadDes
 */
EXPORT_C TInt MNifIfNotify::ReadDes(const TDesC& aField, TDes8& aValue)
	{
	return ReadDes( aField, aValue, NULL );
	}

/**
 * Read an 8-bit descriptor field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadDes
 */
EXPORT_C TInt MNifIfNotify::ReadDes(const TDesC& aField, TDes8& aValue, const RMessagePtr2* aMessage )
	{
	return DoReadDes( aField, aValue, aMessage );
	}

/**
 * Read a 16-bit descriptor field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadDes
 */
EXPORT_C TInt MNifIfNotify::ReadDes(const TDesC& aField, TDes16& aValue)
	{
	return ReadDes( aField, aValue, NULL );
	}

/**
 * Read a 16-bit descriptor field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadDes
 */
EXPORT_C TInt MNifIfNotify::ReadDes(const TDesC& aField, TDes16& aValue, const RMessagePtr2* aMessage )
	{
	return DoReadDes( aField, aValue, aMessage );
	}

/**
 * Write an 8-bit descriptor field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteDes
 */
EXPORT_C TInt MNifIfNotify::WriteDes(const TDesC& aField, const TDesC8& aValue)
	{
	return WriteDes( aField, aValue, NULL );
	}

/**
 * Write an 8-bit descriptor field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteDes
 */
EXPORT_C TInt MNifIfNotify::WriteDes(const TDesC& aField, const TDesC8& aValue, const RMessagePtr2* aMessage )
	{
	return DoWriteDes( aField, aValue, aMessage );
	}

/**
 * Write a 16-bit descriptor field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteDes
 */
EXPORT_C TInt MNifIfNotify::WriteDes(const TDesC& aField, const TDesC16& aValue)
	{
	return WriteDes( aField, aValue, NULL );
	}

/**
 * Write a 16-bit descriptor field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteDes
 */
EXPORT_C TInt MNifIfNotify::WriteDes(const TDesC& aField, const TDesC16& aValue, const RMessagePtr2* aMessage)
	{
	return DoWriteDes( aField, aValue, aMessage );
	}

/**
 * Read a boolean field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadBool
 */
EXPORT_C TInt MNifIfNotify::ReadBool(const TDesC& aField, TBool& aValue)
	{
	return ReadBool( aField, aValue, NULL );
	}

/**
 * Read a boolean field from the connection settings provider
 * @param aField The name of the field to read
 * @param aValue On return, contains the value of the field read
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoReadBool
 */
EXPORT_C TInt MNifIfNotify::ReadBool(const TDesC& aField, TBool& aValue, const RMessagePtr2* aMessage )
	{
	return DoReadBool( aField, aValue, aMessage );
	}

/**
 * Write a boolean field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteBool
 */
EXPORT_C TInt MNifIfNotify::WriteBool(const TDesC& aField, TBool aValue)
	{
	return WriteBool( aField, aValue, NULL );
	}

/**
 * Write a boolean field to the connection settings provider
 * @param aField The name of the field to which to write
 * @param aValue The value to be written to the field
 * @param aMessage Message containing security capabilites to be checked
 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
 * @see DoWriteBool
 */
EXPORT_C TInt MNifIfNotify::WriteBool(const TDesC& aField, TBool aValue, const RMessagePtr2* aMessage)
	{
	return DoWriteBool( aField, aValue, aMessage );
	}



