// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class CTCPReceiveWindowSize defines the public interface used by Licensee's NetMcpr
// to configure the TCP receive window dynamically. Licensee are required to override
// the SetTcpWin(1) method for setting the TCP receive window for different bearers. 
// 
//

/**
 @file tcprecvwin.h
 @class CTCPReceiveWindowSize
 @brief Declaration of class CTCPReceiveWindowSize
 @publishedPartner
 @released.
*/

#ifndef SYMBIAN_TCP_RECEIVE_WINDOW_H
#define SYMBIAN_TCP_RECEIVE_WINDOW_H

#include <networking/etelbearers.h>

/**
 * Default TCP receive window
 */ 
const TUint KBearerDefaultWinSize    = 49152;

/**
 * Default maximum TCP receive window
 */
const TUint KBearerDefaultMaxWinSize = 131070;


//TCP receive window interface definition.
class CTCPReceiveWindowSize : public CBase, public Meta::SMetaData
{
public:
	enum
	{
		ERealmId= 0x102070ED,
		iId = 9,
	};
	
public:
	//Default TCP receive window
	CTCPReceiveWindowSize() :iWinSize(KBearerDefaultWinSize),
                         iMaxWinSize(KBearerDefaultMaxWinSize)
        {};
	
	//Virtual Destructor
	virtual ~CTCPReceiveWindowSize(){};
	
    //Set TCP window size
	virtual void SetTcpWin(TUint aBearerType) = 0;

	//Get the TCP receive window size for a bearer 
	TUint GetTcpWin() const { return iWinSize; }
	
	//Get maximum  TCP receive window size for a bearer
	TUint GetTcpMaxWin() const { return iMaxWinSize; }
	
public:
	//TCP receive window for a bearer.
   	TUint iWinSize;
   
    //TCP maximum receive window for a bearer.
   	TUint iMaxWinSize;
   	
public:
 	DATA_VTABLE	
};
#endif //SYMBIAN_TCP_RECEIVE_WINDOW_H
