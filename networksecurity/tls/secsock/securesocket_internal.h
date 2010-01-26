// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Secure Sockets
// 
//

/**
 @file securesocket_internal.h
*/


#ifndef __SECURESOCKET_INTERNAL_H__
#define __SECURESOCKET_INTERNAL_H__

/** 
 * Maximum length of the protocol name. 
 * 
 * @internalComponent
 */
const TInt KMaxProtocolName = 32;

class TSecSocketProtocol
/** 
 * The secure socket protocol class. 
 * 
 * @internalComponent
 *
 * @since v7.0 
 */
	{
public:
	/** Protocol name. */
	TBuf<KMaxProtocolName> iName;
	/** Handle to the DLL. */
	RLibrary iLibrary;
	static inline TInt Offset()
	/** 
	* Gets the offset to the iSlink member.
	* 
	* @return The offset to the iSlink member. */
		{return _FOFF(TSecSocketProtocol,iSlink);}
	// Moved the implementation to the cpp file
	virtual ~TSecSocketProtocol();
private:
    TSglQueLink   iSlink;
	};


class TSecureSocketGlobals
/** 
 * Class to store the Secure Sockets Globals. 
 * 
 * @internalComponent
 * 
 * @since v7.0 
 */
	{
public:
	inline TSecureSocketGlobals():
			iSecureSocketProtocols(TSecSocketProtocol::Offset()),
	/** Constructor. */
			iSecureSocketProtocolsIter(iSecureSocketProtocols),
			iUseCount(0){};	
	/** List of supported protocols. */
	TSglQue<TSecSocketProtocol> iSecureSocketProtocols;
	/** A templated class that provides for iterating through the list of supported 
	* protocols. */
	TSglQueIter<TSecSocketProtocol> iSecureSocketProtocolsIter;
	/** Use counter. */
	TInt iUseCount;
	};

class MGenericSecureSocket;

/** 
 * Definition for the entry point function exported by Secure Socket modules. 
 * 
 * @internalComponent
 */
typedef TInt (*TSecSockDllLibraryFunction)( RSocket& aSocket, const TDesC& aProtocol );
typedef TInt (*TSecSockDllLibraryGenericFunction)(MGenericSecureSocket& aSocket, const TDesC& aProtocol);

/** 
 * Definition for the entry point for the cleanup function exported by secureSocket modules
 * 
 * @internalComponent
 */
typedef void (*TSecSockDllUnloadFunction)( TAny* );

class CSecureSocketLibraryLoader : public CBase
/** 
 * Factory class for creating secure sockets. 
 * 
 * @internalAll
 *
 * @since v6.2 */
 // Create and reference Secure Sockets
	{
public:
	static  TInt OpenL(const TDesC& aProtocolName,TSecSockDllLibraryFunction& anEntryPoint);
	static  TInt OpenL(const TDesC& aProtocolName, TSecSockDllLibraryGenericFunction& aEntryPoint);
	static void FindItemInDbL(const TDesC& aProtocolName, TDes& aLibraryName);
	IMPORT_C static	void Unload();

private:
	static  void OpenWithIdL(TInt aId, const TDesC& aProtocolName, TLibraryFunction& aEntryPoint);
	};



#endif // __SECURESOCKET_H__
