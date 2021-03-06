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
//

/**
 @file ipeventlistener.cpp
 @internalComponent
*/



#include <in_bind.h>
#include <in6_opt.h>

#include "ipeventlistener.h"
#include "ipeventnotifier.h"
#include "HookLog.h"


CIPEventListener::CIPEventListener(CIPEventNotifier& aNotifier)
 : CBase(), iNotifier(aNotifier) {}

  
void CIPEventListener::Notify(TUint aEventClass, TUint aEventType, const void* aData)
/*
 * Handler for the address and interface events generated by the tcp/ip stack for which we have registered.
 */
	{
	LOG	(
		_LIT(KNotifyStr, "CIPEventListener::Notify() - Event aEventClass=%d aEventType=%d");
		HookLog::Printf( KNotifyStr, aEventClass, aEventType);
		)

	(void)aEventType;               // 'make use of' argument in release builds.

	switch (aEventClass)
		{
		case EClassAddress:
			{
			const TInetAddressInfo* info = static_cast<const TInetAddressInfo*>(aData);
	
			const TUint addressAssignState = info->iState;
	
			if (addressAssignState==TInetAddressInfo::EAssigned || // IP ready
				addressAssignState==TInetAddressInfo::EDuplicate || // IP not ready
				aEventType==EventTypeDelete)  // IP not ready
				{
	
				const TUint     unknownPortId  = 0;
				const TInetAddr tmpInetAddr(info->iAddress, unknownPortId);
	
				const TUint32 interfaceIndex = info->iInterface;
				const TBool   isLinkLocal    = tmpInetAddr.IsLinkLocal();
	
	
				// We have captured the assignment of an address to an interface, which may also be a link-local address.
	
	
				if ( iNotifier.ShouldPublishEvent( interfaceIndex ) )
					{
					// A client has registered to be notified of this event.
	
					TBool addressValid = (addressAssignState == TInetAddressInfo::EAssigned);
	
					LOG
						(
						TBuf<70> tmp;
						tmpInetAddr.OutputWithScope(tmp);
						_LIT(KHookNotifyStr, "CIPEventListener::Notify() - IP Event on if# %d: DAD notification - Address=%S state=%d ready=%d");
						HookLog::Printf( KHookNotifyStr, interfaceIndex, &tmp, addressAssignState, addressValid);
						)
	
					TRAPD(err, iNotifier.PublishIPReadyL(interfaceIndex, tmpInetAddr, addressValid));
					if(err!=KErrNone)
						{
						LOG	(
						_LIT(KLogStr, "Can't publish IP ready message! Err %d\n");
						HookLog::Printf(KLogStr, err);
						)
						}
					}
	
	
				if ( isLinkLocal && iNotifier.ShouldPublishEvent(interfaceIndex) )
					{
					// A client has registered to be notified of this event.
	
					LOG
						(
						TBuf<70> tmp;
						tmpInetAddr.OutputWithScope(tmp);
						_LIT(KHookNotifyStr, "CIPEventListener::Notify() - IP Event on if# %d: Found link local address=%S state=%d");
						HookLog::Printf( KHookNotifyStr, interfaceIndex, &tmp, addressAssignState);
						)
	
					TRAPD(err,iNotifier.PublishLinklocalAddressKnownL(interfaceIndex, tmpInetAddr));
					if(err!=KErrNone)
						{
						LOG	(
						_LIT(KLogStr, "Can't publish IP ready message! Err %d\n");
						HookLog::Printf(KLogStr, err);
						)
						}
					}
	
				}
	
			break;
	
			}
		case EClassInterface:
			{
			// Inform the event notifier of the association between interface name and the stack's interface index.
	
			const TInetInterfaceInfo* info = static_cast<const TInetInterfaceInfo*>(aData);
	
	
			const TUint32 interfaceIndex = info->iIndex;
			const TName&  interfaceName  = info->iName;
			TBool alreadyKnown = iNotifier.IsInterfaceKnown(interfaceIndex);
	
			LOG
				(
				_LIT(KHookNotifyStr, "CIPEventListener::Notify() - Interface association - interfaceIndex=%d interfaceName=%S");
				HookLog::Printf( KHookNotifyStr, interfaceIndex, &interfaceName );
				)
	
			LOG(
				const TInetInterfaceInfo& iface = *info;
				_LIT(KKnown, "known");
				_LIT(KNew, "new");
		        TBuf<2048> tmpBuf;
				tmpBuf.Format(_L("Notified of %S i/f: index:%d, name: "),alreadyKnown? &KKnown(): &KNew(), iface.iIndex);
		        tmpBuf.Append(iface.iName);
		        tmpBuf.AppendFormat(_L(", state:%d, SMti:%d, RMtu:%d, SpeedMetric:%d, feat:%d,"),
		        			iface.iState,iface.iSMtu,iface.iRMtu,iface.iSpeedMetric,iface.iFeatures/*,iface.iHwAddr*/);
		        HookLog::Printf(tmpBuf);
	        	)


			if(!alreadyKnown)
				{
				TRAPD(ret, iNotifier.AddInterfaceL(interfaceIndex, interfaceName));
				
				if(ret!=KErrNone)
					{
					LOG(
						_LIT(KHookNotifyStr, "CIPEventListener::Notify() - Problem creating interface association: %d");
						HookLog::Printf( KHookNotifyStr, ret );
					)
					}
				}
			}
			break;
	
		default:
			return;
		}
	} // CIPEventListener::Notify

