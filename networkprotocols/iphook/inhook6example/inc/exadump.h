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
// exadump.h - packet dump plugin example module
//

#ifndef __EXADUMP_H
#define __EXADUMP_H

/**
* @file exadump.h
* Packet dump plugin example module.
* @internalComponent
*/

// The content of this header is only used in exadump.cpp and
// this exists only for technical reasons due to doxygen
// setup.

#include <posthook.h>


class CProtocolExadump : public CProtocolPosthook, public MFlowHook
	/**
	* Dump <em>cooked</em> packets.
	*
	* This is an example of a hook that attaches to the inbound path
	* just before the upper layer (A) and as a flow hook (D) to the
	* outbound path. The (A) and (D) refer to attachement points
	* in @ref packet_flows
	*
	* The term <em>cooked</em> attempts to signify that this hook sees
	* only packets that are accepted for the upper layer processing, and after all
	* extension header processing have been done (for example, if IPSEC
	* is in effect, this hook will see clear incoming packets. For outbound
	* packets, whether it sees clear or encrypted packets, depends on the
	* posisioning of hook in the list of outbound flow hooks).
	*
	* This also demonstrates an architecture where the hook itself works
	* as a MFlowHook instance for all attached flows. This is possible
	* because this hook does not need to maintain any flow specific
	* context.
	*
	* The hook writes each packet (in- or outbound) to a file in
	* TCPDUMP format (can be viewed using ethereal utility).
	*/
	{
public:

	// Constructors and destructors

	CProtocolExadump();
	virtual ~CProtocolExadump();

	// Pure virtual in CProtocolBase, and MUST be implemented here.
	virtual void Identify(TServerProtocolDesc *aDesc) const;

	//
	// Specific to CProtocolExadump implementation
	//
	virtual void NetworkAttachedL();
	virtual TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);
	virtual MFlowHook *OpenL(TPacketHead &aHead, CFlowContext *aFlow);
	//
	// MFlowHook methods
	//
	virtual void Open();
	virtual void Close();
	virtual TInt ReadyL(TPacketHead &aHead);
	virtual TInt ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
	//
	// ProtocolModule "glue"
	//
	static void Describe(TServerProtocolDesc &aDesc);

	//
	// The nature of the following fields is "private", but in this
	// example class they are left public to enable doxygen documentation
	// to be generated from them (so that it can be generated with "no private
	// methods" option).
	//

	void DoOpen(TInt aSnapLen);
	void DoPacket(const RMBufPacketBase &aPacket, const RMBufPktInfo &aInfo);

	/**
	* A temporary buffer to hold the packet in contiguous memory. This
	* used because, it is assumed that doing RMBufChain copyout combined
	* with single write, is faster than doing multiple writes from
	* each RMBuf separately.
	*
	* Also, the Max Length defines the "snap length".
	*/
	HBufC8 *iBuffer;
	//
	// The dump file
	//
	/**
	* Status of the file (iDumpFile)
	*
	* - < 0,
	*	an attempt to open the dump file has been made, but it has
	*	failed for some reason. Do not try to open again.
	* - = 0,
	*	the dump file is not open.
	* - = 1
	*	the dump file is open.
	*/
	TInt iOpen;
	/** The file server handle. */
	RFs iFS;
	/** The dump file handle */
	RFile iDumpFile;
	/** The base for the time stamps in the dump file.  (= 1.1.1970) */
	TTime iBase;
	};

#endif
