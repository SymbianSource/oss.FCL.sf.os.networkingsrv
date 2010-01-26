/*
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
* Name        : 6to4_flow.h
* Part of     : 6to4 plugin / 6to4.prt
* Implements 6to4 automatic and configured tunnels, see
* RFC 3056 & RFC 2893
* Version     : 0.2
*
*/




/**
 @internalComponent
*/


#ifndef __6TO4_FLOW_H
#define __6TO4_FLOW_H

//  INCLUDES
#include <e32base.h>
#include <ip6_hook.h>

#include "6to4.h"
#include "6to4_tunnel.h"

// CONSTANTS
// MACROS
// DATA TYPES
// FUNCTION PROTOTYPES
// FORWARD DECLARATIONS
// CLASS DECLARATION

class MNetworkService;

/**
*  6to4 flow hook
*  Keeps information on a 6to4 flow.
*
*  @lib 
*  @since 
*/
class C6to4FlowInfo : public CBase, public MFlowHook
	{
	public:	// Constructors & destructors
	
	C6to4FlowInfo(const TPacketHead &aHead);
	~C6to4FlowInfo();

	public: // New functions

	public: // Functions from base classes

	/**
	 * From MFlowHook Open and Close implements reference counting system.
	 * @since
	 * @param
	 * @return void
	 */
	void Open ();

	/**
	 * From MFlowHook Informs stack if the hook is ready. Can update address
	 * information.
	 * @since
	 * @param aHead Address information of the flow.
	 * @return TInt 0 => ready, >0 => pending, <0 => error.
	 */
	TInt ReadyL (TPacketHead & aHead);

	/**
	 * Called by IP when processing outbound packet.
	 * @since
	 * @param aPacket Complete packet
	 * @param aInfo Information block associated with the packet
	 * @return TInt 0 => processed, <0 => discarded, >0 => restart processing
	 */
	TInt ApplyL (RMBufSendPacket & aPacket, RMBufSendInfo & aInfo);

	/**
	 * From MFlowHook Open and Close implements reference counting system.
	 * @since
	 * @param
	 * @return void
	 */
	void Close ();

	private:    // Data

	// Reference count
	TInt iRefs;
	
	// Inner interface index
	const TUint32 iInterfaceIndex;
	
	TInet6HeaderIP iInnerIp;
	TUint32 iSrcId;	// inner only
	TUint32 iDstId;	// inner only
	TInet6HeaderIP iOuterIp;

	TUint16 iSrcPort;
	TUint16 iDstPort;
	TUint8 iIcmpType;
	TUint8 iIcmpCode;
	TUint8 iProtocol;	

	// Fixed packet content. Hooks may add in their ReadyL some fixed
	// packet content (extension headers in IPv6) into (TPacketHead::iOffset,
	// TPacketHead::iPacket). The ReadyL of a tunneling hook *MUST* save
	// and remove this information from the TPacketHead, because it belongs
	// under the outer IP header (which the tunneling hook inserts). The
	// tunneling hook, in addition to adding the outer IP header in ApplyL,
	// must also copy the fixed packet content, if any is present.
	TInt iOffset;			// > 0, if there is fixed packet content.
	RMBufChain iPacket;

	// TODO
	TUint16 iPacketID;
	};

#endif      // __6TO4_FLOW_H   
			
// End of File
