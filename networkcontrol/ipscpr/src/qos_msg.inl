/**
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
* Inline functions file for the QoS Mapping Messages
* 
*
*/



/**
 @file qos_msg.inl
*/

#ifndef __QOS_MSG_INL__
#define __QOS_MSG_INL__

inline void CQoSMsg::AddConnInfo(TUint32 aProtocol, const TUidType& aUid, TUint32 aIapId )
/**
Add Connection Information to the QoS PRT Message

@param aProtocol Protocol Id
@param aUid Process UID Type
@param aIapId Internet Access Point Id
*/
	{
    iMsg->AddSelector((TUint8)aProtocol, aUid, EPfqosFlowspecPolicy, aIapId, EPfqosApplicationPriority, TPtr(0,0));
	}


inline void CQoSMsg::AddChannel(TInt aChannelId)
	{
/**
Adds QoS Channel Id to the QoS PRT Message

@param aChannelId QoS Channel Id
*/
    iMsg->AddChannel(aChannelId); 
	}

inline void CQoSMsg::AddQoSParameters(const TQoSParameters& aParameters)
/** 
Adds Standard QoS Parameters to the QoS PRT Message

@param aParameters QoS Parameters
*/
	{
    iMsg->AddQoSParameters(aParameters);
	}


// ###########################################################


inline void CQoSMsgWriter::DoCancel()
/**
Cancel Writing to internal socket
*/
    {
    iSocket.CancelWrite();
    }


// ###########################################################


inline void CQoSMsgReader::DoCancel()
/**
Cancel Reading from internal socket
*/
	{
    iSocket.CancelRecv();
	}

#endif // __QOS_MSG_INL__
