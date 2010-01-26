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
// wideascii.h - The WideAscii Class to deal with UNICODE mess
//



/**
 @internalComponent
*/

#ifndef __WIDENARROW_H__
#define __WIDENARROW_H__

//
//	WideAscii
//	*********
//	The class is only a collection of methods. It does not
//	describe any instantiable object or variable!
//
//	The primary premise is that the most information in the
//	internet protocols is represented as a sequence of 8 bit
//	bytes and is usually encoded as ASCII. When compiling
//	protocol modules for UNICODE variant, clashes easily occur
//	when textual ASCII data is transferred between the protocol
//	message and some generic field that has been declared without
//	explicit 8/16 specification.
//
//	*NOTE*
//		The implementation uses the generic Copy method of the
//		standard descriptor to actually deal with the 8/16
//		conversion. One could use the same in the code directly,
//		but it would be hard to spot those locations. The use of
//		this class documents the intentional conversions
//
//		In addition to current simple ASCII method, one could
//		in future more complex transfers, such as UTF8.
//
class WideNarrow
	{
public:
	static inline void ASCII(TDes &aDst, const TDesC &aSrc)
		{ aDst = aSrc; }
	static inline void ASCII(TDes8 &aDst, const TDesC16 &aSrc)
		{ aDst.Copy(aSrc); }
	static inline void ASCII(TDes16 &aDst, const TDesC8 &aSrc)
		{ aDst.Copy(aSrc); }
	};

#endif
