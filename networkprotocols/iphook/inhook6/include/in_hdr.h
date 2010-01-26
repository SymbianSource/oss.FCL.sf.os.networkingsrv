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
// in_hdr.h - IPv6 header structure
// Defines the basic classes for accessing the header
// structures within IPv6 packets.
//



/**
 @publishedAll
 @deprecated
*/

#ifndef __IN_HDR_H__
#define __IN_HDR_H__

#include <e32std.h>

//
// TInet6HeaderBase
//	A base class for headers.
//
//	Currently no declarations, just here ready for potential need
//
//class TInet6HeaderBase
//	{
//private:
	//
	// The following methods are "private" on purpose. They only
	// document what derived class should implement, and cause an
	// error message at compile time when a method is used, but
	// not declared in the derived class. There is NO IMPLEMENTATION
	// for these in this base class!!
	//
	// Every derived class must implement the following methods
	// (These are compile time constants)
	//
//	static TInt MinHeaderLength();
//	static TInt MaxHeaderLength();

	// Actual header length (based on on dynamic information
	// retrieved from the successfully mapped header).
	//					
//	TInt HeaderLength();
	//
	// There is no need for a method accessing the offset to the
	// beginning of payload data. This offset is *ALWAYS* same as
	// HeaderLength(). It includes the padding, if any is required.
//	};

#endif
