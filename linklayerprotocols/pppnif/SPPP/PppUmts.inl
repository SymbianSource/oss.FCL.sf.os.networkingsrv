// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This module contains the code for the PPP "proxy" class which communicates with the lower Nif.
// 
//

#ifndef __PPPUMTS_INL__
#define __PPPUMTS_INL__

#include "PppUmts.h"
#include <e32std.h>

/****************/
/* CProtocolPpp */
/****************/

//
// The following functions are called by the NCP layer of lower Nif
//
inline void CProtocolPpp::Process(RMBufChain &aPacket,CProtocolBase* aSourceProtocol)
/**
 * Called by lower Nif to communicate received data to PPP.
 */
	{
	iPppUmtsLink->Process(aPacket, aSourceProtocol);
	}

inline void CProtocolPpp::Process(TDes8& aPDU,TSockAddr *from,TSockAddr *to,CProtocolBase* aSourceProtocol)
/**
 * Called by lower Nif to communicate received data to PPP (not used).
 */
	{
	iPppUmtsLink->Process(aPDU,from,to,aSourceProtocol);
	}

inline void CProtocolPpp::StartSending(CProtocolBase* /*aProtocol*/)
/**
 * Called by lower Nif to unblock flow control.
 */
	{
	iPppUmtsLink->StartSending(this);
	}

void CProtocolPpp::Identify(TServerProtocolDesc* /*aProtocolDesc*/) const
	{}

/****************/
/* CPppUmtsLink */
/****************/

inline void CPppUmtsLink::Process(RMBufChain &aPacket,CProtocolBase* /*aSourceProtocol*/)
/**
 * Called from CProtocolPpp when it receives data from lower Nif.  Pass data to LCP.
 *
 * @param aPacket MBuf chain containing received packet.
 */
	{
	DeliverToLcp(aPacket);
	}

inline void CPppUmtsLink::Process(TDes8& /*aPDU*/,TSockAddr * /*from*/,TSockAddr * /*to*/,CProtocolBase* /*aSourceProtocol*/)	// Up
	{
	PppPanic(EPppPanic_PPPNotSupported);
	}

inline void CPppUmtsLink::StartSending(CProtocolBase* /*aProtocol*/)
/**
 * Called from CProtocolPpp when it receives a request to unblock flow control from lower Nif.  Pass request to LCP.
 */
	{
	iPppLcp->LinkFlowOn();
	}

// CPppLinkBase API to PPP Nif

inline TInt CPppUmtsLink::Send(RMBufChain& aPacket, TUint aPppId/*=KPppIdAsIs*/)
/**
 * Called from PPP to send data to lower Nif.
 *
 * @param aPacket MBuf chain containing packet to transmit.
 * @returns 0 if Nif requested flow control block, else 1.
 */
	{
	return iUmtsNifPppBinder->Send(aPacket,(TAny *)aPppId);
	}

inline void CPppUmtsLink::OpenL()
	{
	}

inline void CPppUmtsLink::Close()
	{
	// Required only for the ts_dummyoveralltest
	// If LinkLayerDown must be called to let LCP know that physical layer has disconnected
	// Otherwise, PPP never terminates.
	iPppLcp->LinkLayerDown(KErrNone);
	}

inline void CPppUmtsLink::StartL()
/**
 * Called when Nifman calls Start() on PPP.  Call Start() on the lower Nif.
 */
	{
	iUmtsNif->Start();
	}

inline void CPppUmtsLink::GetSendRecvSize(TInt& aMaxRecvSize, TInt& aMaxSendSize)
	{
	aMaxRecvSize = 0;	// Dont Care
	aMaxSendSize = 0;	// Dont Care
	}

inline void CPppUmtsLink::GetDataTransfer(RPacketContext::TDataVolume& aData)
	{
	TPckg<RPacketContext::TDataVolume> info(aData);
	iUmtsNif->Notification(EAgentToNifEventTypeGetDataTransfer, &info);
	}

//
// Following functions implement the MNifIfNotify API to lower Nif.
//

inline void CPppUmtsLink::LinkLayerDown(TInt aReason, TAction /*aAction*/)
/**
 * Called from lower Nif to indicate that link layer has gone down.
 */
	{
	NotifyLinkDown(aReason);
	}

inline void CPppUmtsLink::LinkLayerUp()
/**
 * Called from lower Nif to indicate that link layer has come up (after a Start() call).
 */
	{
	NotifyLinkUp();
	}

inline void CPppUmtsLink::NegotiationFailed(CNifIfBase* aIf, TInt aReason)
	{iPppLcp->Notify()->NegotiationFailed(aIf, aReason);}

inline TInt CPppUmtsLink::Authenticate(TDes& aUsername, TDes& aPassword)
	{return iPppLcp->Notify()->Authenticate(aUsername, aPassword);}

inline void CPppUmtsLink::CancelAuthenticate()
	{iPppLcp->Notify()->CancelAuthenticate();}

inline TInt CPppUmtsLink::GetExcessData(TDes8& aBuffer)
	{return iPppLcp->Notify()->GetExcessData(aBuffer);}

/**
 * Called by lower Nif to read a CommDb field
 *
 * @param aField Name of CommDb field
 * @param aValue Receives the value of the field.
 * @param aMessage Message containing security capabilities to validate
 * @returns A system wide error code.
 */
inline TInt CPppUmtsLink::DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage)
	{ return iPppLcp->Notify()->ReadInt(aField,aValue,aMessage); }

/**
 * Called by lower Nif to write a CommDb field
 *
 * @param aField Name of CommDb field
 * @param aValue Value to write.
 * @param aMessage Message containing security capabilities to validate
 * @returns A system wide error code.
 */
inline TInt CPppUmtsLink::DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage)
	{ return iPppLcp->Notify()->WriteInt(aField,aValue,aMessage); }

inline TInt CPppUmtsLink::DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage)
	{return iPppLcp->Notify()->ReadDes(aField, aValue,aMessage);}

inline TInt CPppUmtsLink::DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage)
	{return iPppLcp->Notify()->ReadDes(aField, aValue,aMessage);}

inline TInt CPppUmtsLink::DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage)
	{return iPppLcp->Notify()->WriteDes(aField,aValue,aMessage);}

inline TInt CPppUmtsLink::DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage)
	{return iPppLcp->Notify()->WriteDes(aField,aValue,aMessage);}

inline TInt CPppUmtsLink::DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage)
	{return iPppLcp->Notify()->ReadBool(aField, aValue,aMessage);}

inline TInt CPppUmtsLink::DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage)
	{return iPppLcp->Notify()->WriteBool(aField, aValue,aMessage);}

inline void CPppUmtsLink::IfProgress(TInt /*aStage*/, TInt /*aError*/)
/**
 * Called by lower Nif when signalling progress.  This does not need to be conveyed to Nifman.
 */
	{ }

inline void CPppUmtsLink::OpenRoute()
	{ iPppLcp->Notify()->OpenRoute(); }

inline void CPppUmtsLink::CloseRoute()
	{ iPppLcp->Notify()->CloseRoute(); }

inline TInt CPppUmtsLink::PacketActivity(TDataTransferDirection /*aDirection*/, TUint /*aBytes*/, TBool /*aResetTimer*/)
	{ return KErrNone; }

inline void CPppUmtsLink::IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError)
/**
 * Upcall from lower level nif to report progress on a subconnection
 * @param aSubConnectionUniqueId The id of the subconnection to which this notification refers
 * @param aStage The progress stage that has been reached
 * @param aError Any error relating to that progress stage
 * @note When PPP is running over another nif, only one subconnection is supported.
 * @note Since PPP.nif governs the state of the connection, the only progress that should be passed up is "suspended" from GPRS nifs
 */
	{
	if(aStage==KPsdSuspended)
		{
		iPppLcp->Notify()->IfProgress(aSubConnectionUniqueId, aStage, aError);
		}
	}

inline void CPppUmtsLink::NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume)
/**
 * Upcall from lower level nif to report data sent on a subconnection
 * @param aSubConnectionUniqueId The id of the subconnection to which this notification refers
 * @param aUplinkVolume The total volume of data sent on the subconnection
 */
	{
	iPppLcp->Notify()->NotifyDataSent(aSubConnectionUniqueId, aUplinkVolume);
	}

inline void CPppUmtsLink::NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume)
/**
 * Upcall from lower level nif to report data received on a subconnection
 * @param aSubConnectionUniqueId The id of the subconnection to which this notification refers
 * @param aUplinkVolume The total volume of data received on the subconnection
 */
	{
	iPppLcp->Notify()->NotifyDataReceived(aSubConnectionUniqueId, aDownlinkVolume);
	}


inline void CPppUmtsLink::NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource)
/**
 * Upcall from the nif to report an event
 * @param aSubConnectionUniqueId The id of the subconnection to which this event refers
 * @param aEvent The event from the nif
 * @todo Alter this when nifman is defined correctly
 * @see MNifIfNotify::NifEvent
 */
	{
	iPppLcp->Notify()->NifEvent(aEventType, aEvent, aEventData, aSource);
	}


inline TInt CPppUmtsLink::Notification(TNifToAgentEventType aEvent, void * aInfo)
/**
 * Called by lower Nif for passing notifications to the Agent.
 */
	{ return iPppLcp->Notify()->Notification(aEvent, aInfo); }

#endif
