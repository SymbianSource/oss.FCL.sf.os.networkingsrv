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
* Name        : 6to4.h
* Part of     : 6to4 plugin / 6to4.prt
* Implements 6to4 automatic and configured tunnels, see
* RFC 3056 & RFC 2893
* Version     : 0.2
*
*/




/**
 @internalComponent
*/

#ifndef __6TO4_H
#define __6TO4_H

//  INCLUDES
#include <posthook.h>
#include <es_ini.h>

#include "6to4_tunnel.h"

// CONSTANTS

// Protocol Name
_LIT(K6to4, "6to4");

// File name
_LIT (K6To4IniData, "6to4.ini");

// Sections
_LIT (K6To4IniSectionTunnels, "tunnels");
_LIT (K6To4IniTunnelFile, "filename");

// MACROS
// DATA TYPES
// FUNCTION PROTOTYPES
// FORWARD DECLARATIONS
class CProtocolIPv4;
class CIpAddressDesc;
class C6to4FlowInfo;
class C6to4Listener;
class MEventService;

// CLASS DECLARATION

/**
*  CProtocol6to4
*  Plugin for 6to4 implementation.
*
*  @lib 
*  @since
*/
class CProtocol6to4 : public CProtocolPosthook
	{
	public:  // Constructors and destructor
	
	CProtocol6to4 ();
		
	/**
	 * Destructor.
	 */
	virtual ~CProtocol6to4 ();

	public: // New functions
		
	/**
	 * Compatibility checking for the notification service.
	 * @since 
	 * @param 
	 * @return void
	 */
	void CheckCompatibility ();

	/**
	 * Loads the configuration file and configures the system accordingly.
	 * @since 
	 * @param 
	 * @return TBool ETrue if success.
	 */
	TBool LoadConfigurationFile ();

	/**
	 * Loads the tunnel configuration file and configures the system 
	 * accordingly.
	 * @since 
	 * @param aFile Tunnel configuration file to be parsed. 
	 * @return TInt KErrNone if success.
	 */
	TInt LoadFileL (const TDesC & aFile);

	/**
	 * Private member access
	 * @since 
	 * @param 
	 * @return TUint32
	 */
	inline TUint32 InterfaceIndex () const        { return iInterfaceIndex; }

	public: // Functions from base classes

	/**
	 * From CProtocolBase  
	 * @since 
	 * @param aTag Name
	 * @return void
	 */
	virtual void InitL (TDesC & aTag);

	/**
	 * From CProtocolBase  
	 * @since 
	 * @param aDesc Descriptor to be filled by this method.
	 * @return void
	 */
	virtual void Identify (TServerProtocolDesc * aDesc) const;

	/**
	 * From CProtocolBase  
	 * @since 
	 * @param aDesc Descriptor to be filled by this method.
	 * @return void
	 */
	static void Identify (TServerProtocolDesc & aDesc);

	/**
	* From CProtocolPostHook Handles attach to the stack
	* @since
	* @return void
	*/
	void NetworkAttachedL();

	/**
	* From CProtocolPostHook Handles detach from the stack
	* @since
	* @return void
	*/
	void NetworkDetached();

	/**
	 * From MIp6Hook Handles received packets. 
	 * @since 
	 * @param aPacket Packet received.
	 * @param aInfo Info structure.
	 * @return void
	 */       
	virtual TInt ApplyL (RMBufHookPacket & /* aPacket */ ,
						 RMBufRecvInfo & /* aInfo */ );

	/**
	 * From MIp6Hook Handles outbound opening. 
	 * @since 
	 * @param aHead Flow-specific precomputed data.
	 * @param aFlow Flow.
	 * @return void
	 */       
	virtual MFlowHook *OpenL (TPacketHead & aHead, CFlowContext * aFlow);

	private:    // Data

	// Configuration data
	CESockIniData *iConfig;

	// Non-zero if configuration file is not available
	TInt iConfigErr; 

	// Parser object for the configured tunnels
	TTunnelParser iTunnelParser;

	// Event service
	MEventService *iEventService;

	// Object for receiving events
	C6to4Listener *iEventListener;

	// Interface index of the automatic 6to4 tunnels
	TUint32 iInterfaceIndex;
	};

#endif      // __6TO4_H   
			
// End of File
