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
// NetMcpr's Default implementation of the TCP receive window interface.
// Licensee should do their own implementation for tweaking the
// TCP receive window.
// 
//

/**
 @file tcpdfltrecvwin.h
 @internalComponent
*/

#ifndef SYMBIAN_IPEXTENSION_H
#define SYMBIAN_IPEXTENSION_H

#include <tcprecvwin.h>
#include <networking/cfbearers.h>
#include <e32hashtab.h> 

/**
 *  TCP receive window for GPRS. 
 */
const TUint KBearerGprsWinSize	= 17520;

/**
 *  TCP receive window for EGPRS. 
 */
const TUint KBearerEdgeWinSize	= 49640;

/**
 *  TCP receive window for UMTS. 
 */
const TUint KBearerUmtsWinSize	= 33580;

/**
 *  TCP receive window for HSDPA. 
 */

const TUint KBearerHsdpaWinSize	= 65533;

/**
 *  TCP receive window for WLAN. 
 */
const TUint KBearerWlanWinSize	= 65534;

/**
 *  TCP receive window for ethernet. 
 */
const TUint KBearerEthernetWinSize	= 65535;

/**
 * Maximum TCP receive window for ethernet.
 */
const TUint KEthernetMaxWinSize	= 131070;

/**
 * Default TCP receive window implementation. 
 * 
 */
class CDfltTCPReceiveWindowSize : public CTCPReceiveWindowSize
{
public:
	enum
	{
		ERealmId= 0x102070ED,
		iId = 10,
	};
	
public:
	//Default constructor 
	CDfltTCPReceiveWindowSize();
    
    //Destructor
    ~CDfltTCPReceiveWindowSize();

	//Populates TCP window lookup table for different bearers
	void Init();
	
	//Set TCP window size
	void SetTcpWin(TUint aBearerType);
	
	//Set max TCP receive window for a network bearer
	void  SetMaxWinSize(TUint aBearerType) ;
		
private:
   	//Associative array for storing TCP window size.
   	RHashMap<TUint,TUint> iBearerInfoMap;  	 
 };

#endif //SYMBIAN_IPEXTENSION_H
