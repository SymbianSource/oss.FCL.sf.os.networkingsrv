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
// exaout.h - outbound plugin example protocol module (dummy)
//

#ifndef __EXAOUT_H
#define __EXAOUT_H
/**
* @file exaout.h
* Outbound plugin example protocol module (dummy).
* @internalComponent
*/

#include <posthook.h>

/**
* @name The exaout identification
*
* The protocol is identified by (address family, protocol number). This
* should be unique among all protocols known to the socket server.
*
* Unfortunately there are no rules or registration for these values, and
* the protocol writer just has to pick a combination that is supposed to
* be unique.
*
* In this example, neither of these values have any significance to the
* implementation. Any values will work.
* @{
*/
/** The address family constant. Use the UID value of this protocol module. */
const TUint KAfExaout			= 0x10000942;
/** The protocol number. Because the family is unique, 1000 should not confuse anyone. */
const TUint KProtocolExaout	= 1000;
/** @} */

class CExaoutFlowInfo;

class CProtocolExaout : public CProtocolPosthook
	/**
	* A protocol plugin for outbound flows.
	*
	* This is a minimal definition for a protocol plugin class
	* (hook), which wants to attach outbound flows to monitor or
	* modify the packets.
	*/
	{
public:
	CProtocolExaout();
	virtual ~CProtocolExaout();

	// CProtocolBase
	virtual void Identify(TServerProtocolDesc *aDesc) const;

	// CProtocolPosthook
	virtual void NetworkAttachedL();
	virtual void NetworkDetached();

	//Outbound Flow
	virtual MFlowHook *OpenL(TPacketHead& /*aHead*/, CFlowContext *aFlow);

	// ProtocolModule glue
	static void Describe(TServerProtocolDesc& anEntry);
private:
	TDblQue<CExaoutFlowInfo>	iFlowList;
	};

class CExaoutFlowInfo : public CBase, public MFlowHook
	/**
	* An internal flow context within the plugin.
	*
	* The exaout example plugin attaches an instance of
	* this internal  context to each outbound flow.
	* A per flow instance is required, if the plugin
	* needs to maintain own flow specific context.
	*
	* The exadump is an alternative example for a plugin
	* which does not need any flow specific context.
	*/
	{
public:
	CExaoutFlowInfo(CFlowContext &aFlow);
	virtual ~CExaoutFlowInfo();

	virtual void Open();
	virtual TInt ReadyL(TPacketHead &);
	virtual TInt ApplyL(RMBufSendPacket &, RMBufSendInfo &);
	virtual void Close();

	/**
	* The flow context. This is a direct reference to the
	* object inside the TCP/IP stack. Be careful with it!
	*/ 
	CFlowContext&		iFlow;

	/**
	* A back pointer to the protocol itself.
	* NULL, if not attaced to the protocol (via iDLink)
	*/
	CProtocolExaout*	iMgr;

	/**
	* Object reference count. This object is automaticly
	* deleted when the last reference disappears. (This is
	* the required from MFlowHook)
	*/
	TInt				iRefs;
	/** Links context information together. The head is in CProtocolExaout. */
	TDblQueLink			iDLink;
	static const TInt	iOffset;
	};

#endif
