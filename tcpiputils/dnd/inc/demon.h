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
// demon.h - API between main (ui) and engine for daemons
//

#ifndef __DEMON_H
#define __DEMON_H

#include <e32base.h>
/**
@file demon.h
DNS main
@internalComponent	Domain Name Resolver
*/

/**
// Services offered by the appication main to the engine.
//
// A bare bones API definition which allows to write
// the DND <b>engine</b> part without a need to make two versions
// if daemon application is with, or without the user interface.
*/
class MDemonMain
	{
public:
	/**
	// Write message to the log device.
	//
	// @param aText	to be written
	*/
	virtual void Write(const TDesC &aText) = 0;
	/*
	// Write message with format to the log device
	//
	// @param aFmt	the format string to be applied
	// @param aList	the parameters to the format string
	*/
	virtual void WriteList(const TDesC &aFmt, VA_LIST aList) = 0;
	/**
	// Write message, if error
	//
	// Just a convenience method to make it easy to write code
	// which is "mostly silent" until exceptional situations
	// occur.
	//
	// @param aText	to be written, if aResult != KErrNone
	// @param aResult	is the result code to be tested
	//
	// @return	aResult (always)
	*/
	TInt virtual CheckResult(const TDesC &aText, TInt aResult) = 0;
	};

/**
// Services offered by the engine to the application main.
*/
class MDemonEngine
	{
public:
	// Allow delete via M-class pointer
	virtual ~MDemonEngine() {}
	// Allow ConstructL via M-class pointer
	virtual void ConstructL() = 0;
	// Handle commands from the user
	virtual void HandleCommandL(TInt aCommand) = 0;
	// Create the real engine instance [there can be only one]
	static MDemonEngine *NewL(MDemonMain &aMain);
	};

#endif
