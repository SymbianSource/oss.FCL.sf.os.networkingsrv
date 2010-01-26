// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Network interface (nif) APIs.
// This file contains all the Internal APIs required to implement a NIF for Symbian OS.
// 
//

/**
 @file nifif_internal.h
 @internalTechnology
 @released
*/

#if !defined(__NIFIF_INTERNAL_H__)
#define __NIFIF_INTERNAL_H__

#define SYMBIAN_NETWORKING_UPS

class CNifIfBase;
class CProtocolBase;
class MNifIfUser
/**
 * Interface between nifman and a layer 3 protocol
 * This interface is used by nifman to perform operations on the layer 3 protocol
 *
 * @internalTechnology
 * @released
 * @since v5.0
 */
	{
public:

	/**
	 *
	 */
	virtual void IfUserBindFailure(TInt aResult, TAny* aId)=0;

	/**
	 *
	 */
	virtual void IfUserNewInterfaceL(CNifIfBase* aIf, TAny* aId)=0;

	/**
	 *
	 */
	virtual void IfUserInterfaceDown(TInt aResult, CNifIfBase* aIf)=0;

	/**
	 *
	 */
	virtual void IfUserOpenNetworkLayer()=0;

	/**
	 *
	 */
	virtual void IfUserCloseNetworkLayer()=0;

	/**
	 *
	 */
	virtual CProtocolBase* IfUserProtocol()=0;

	/**
	 *
	 */
 	virtual TBool IfUserIsNetworkLayerActive()=0;
	};

/**
 * Option level for the nif - used with Get/SetOpt/Control/Ioctl methods
 *
 * @internalTechnology
 * @released
 * @since v5.0
 */
const TUint KNifOptLevel = 0x191;

/**
 * Options for controlling a nif through the RSocket::SetOption() and RSocket::GetOption() interface
 * @defgroup RSocketSetGetOptoptions Options for use with RSocket::SetOption() and RSocket::GetOption()
 *
 * @internalTechnology
 * @released
 * @since v5.0
 */
//@{
const TUint KNifOptStartInterface = 1;		//< Start the specified nif by CObject-name using Nif::Start
const TUint KNifOptStopInterface = 2;		//< Stop the specified nif by CObject-name using Nif::Stop
const TUint KNifOptInterfaceProgress = 3;	//< Fetch the current progress from the interface by CObject-name using Nif::ProgressL()
//@}

/**
 * @internalComponent
 */
typedef TPckgBuf<MNifIfUser*> TNifIfUser;

#ifdef SYMBIAN_NETWORKING_UPS
/**
@internalTechnology

Special NetworkId value to pass in TSoIfConnectionInfo to indicate that the NetworkId
should be determined from the socket address.  This should only be issued in special
circumstances (see TCP/IP stack and ESock server for detailed comments).
*/
const TUint32 KNetworkIdFromAddress = -1;
#endif

#endif

