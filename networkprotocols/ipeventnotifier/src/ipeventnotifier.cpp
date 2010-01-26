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
 @file ipeventnotifier.cpp
 @internalComponent
*/


#include <in_chk.h>
#include <icmp6_hdr.h>
#include <in_sock.h>
#include <in_bind.h>
#include "in6_opt.h"

#include "ipeventnotifier.h"
#include "HookLog.h"
#include <networking/ipeventtypes.h>
#include "ipeventnotifierinterface.h"
#include "ipeventlistener.h"
#include <comms-infras/idquerynetmsg.h>
#include "DHCPUnicastTranslator.h"


CIPEventNotifier* CIPEventNotifier::NewL()
	{
	CIPEventNotifier* self = new(ELeave) CIPEventNotifier();
	CleanupStack::PushL(self);

	self->ConstructL();

	CleanupStack::Pop();
	return self;
	}



void CIPEventNotifier::ConstructL()
	{
	/**
	 * Create the address/interface listener.
	 */
	iIPEventListener = new (ELeave) CIPEventListener(*this);
	}


CIPEventNotifier::CIPEventNotifier()
	{
	}



CIPEventNotifier::~CIPEventNotifier()
	{
	iInterfaces.ResetAndDestroy();

	delete iIPEventListener;
	
	delete iDHCPUnicastTranslator;
	}



void CIPEventNotifier::BindL(CProtocolBase* aProtocol, TUint aId)
	{
	// Do sanity checks
	//
	if ((aId != KProtocolInet6Ip) || (aProtocol == this))
		{
		User::Leave(KErrArgument);
		}

	TUint ourId;
		{
		TServerProtocolDesc info;
		Identify(&info);
		ourId = info.iProtocol;
		}

	if (aId == ourId)
		{
		User::Leave(KErrArgument);
		}

	if ( iProtocolIPv6 != NULL )
		{
		if ( iProtocolIPv6 == aProtocol )
			{
			// We don't need to bind to the same protocol twice.
			//
			return;
			}
		else
			{
			// We don't want to bind to a different protocol either...
			//
			User::Leave(KErrAlreadyExists);
			}
		}

	iProtocolIPv6 = (CProtocolInet6Binder*) aProtocol;

	// Get a pointer to the event service.
	//
	ObtainEventService();

	// Must always register hooks on startup, til the required
	//  initial values have been received.
	//
	RegisterHooksL();
	
	// Create cache of interfaces known to IP stack
	//
	AddKnownInterfacesL();

	/**
	 * Create owned hook to allow unicast DHCP packets to pass.
	 * Notice we are bypassing the standard mechanism for including
	 *  another protocol hook (esk files), and are directly creating
	 *  it here as it is needed for the same lifetime as IPEN itself.
	 */
	if (iDHCPUnicastTranslator == NULL)
		{
		iDHCPUnicastTranslator = new (ELeave) CDHCPUnicastTranslator(*iProtocolIPv6);
		}
	// connect the hook to the stack..
	iProtocolIPv6->BindL(iDHCPUnicastTranslator, BindForwardHook());
	
	}



void CIPEventNotifier::Unbind(CProtocolBase* aProtocol, TUint /* aId */)
	{
	if (iProtocolIPv6 != aProtocol)
		{
		return;
		}

	iProtocolIPv6->Unbind(iDHCPUnicastTranslator, 0);

	UnregisterHooks();

	iProtocolIPv6 = 0;
	iEventService = 0;
	}
	


TInt CIPEventNotifier::GetOption(TUint aLevel, TUint aName,TDes8& aOption,CProtocolBase* aSourceProtocol)
/**
 * Performs one of the following actions, depending on the option name:
 *
 * KHandleAttach: Sets up IPEN to catch and publish events on the interface named in aOption.
 *                  Session handle TUint (the pubsub address key) returned in first 4 bytes of aOption buffer.
 *
 * KHandleRelease: Tells IPEN to stop catching and publishing events to the given session handle
 *                   Handle passed in as TUint in first 4 bytes of aOption buffer.
 *
 * @param aLevel: should be KSolInetIp for this protocol
 * @param aName: see above
 * @param aOption: in/out, dependent on option name. See above
 * @return error status
 *
 */
	{
	(void) aLevel;
	(void) aSourceProtocol;

	TInt retVal = KErrNotSupported;
		
	switch (aName)
		{
		case NetMessages::KHandleAttach:
		    {
			TName buf;
			buf.Copy( aOption );
			TUint *out_handlenum = (TUint *)aOption.Ptr();
			TRAPD(retval, *out_handlenum = OpenSessionL(buf));
			return retval;
			}

		case NetMessages::KHandleRelease:
			{
			TUint *in_handle = (TUint *)aOption.Ptr();
			TInt ret = CloseSession(*in_handle);
			*in_handle = 0;
			return ret;
		    }
			
	    default:
	        break;
		}

	return retVal ;
	}



TUint CIPEventNotifier::OpenSessionL(const TDesC& aIfName)
/**
  * Sets up IPEN to catch and publish events on the interface named by aIfName.
  *                  Session handle TUint (the pubsub address key) is returned.
  *
  * @internalTechnology
  */
	{
	// The IP stack's first interface, a dummy, has an empty name string.
	// Therefore an empty string to open against would match this dummy
	//  interface! So we should report failure if a naughty client is trying to
	//  monitor events on an interface with an empty name string.
	//
	if(aIfName == KNullDesC)
		{
		LOG(
			_LIT(KHookNotifyStr,"ERROR: client attempted to monitor interface with empty name");
			HookLog::Printf( KHookNotifyStr );
			)
		User::Leave(KErrBadName);
		}
	
	TInt ifPos = FindPositionOfInterfaceByName(aIfName);

	CIPEventNotifierInterface* iface = 0;

	if(ifPos == KErrNotFound)
		{
		AddKnownInterfacesL();
		ifPos = FindPositionOfInterfaceByName(aIfName);
		User::LeaveIfError(ifPos);
		}
	
	iface = iInterfaces[ifPos];
	
	if( AnyClientsCurrentlyInterested() == false )
		{
		RegisterHooksL();
		}
		
	__ASSERT_ALWAYS(iface , User::Leave(KErrCorrupt));
	iface->IncreaseRefCount();
	
	PublishLastKnownStates(iface);

	LOG(
		_LIT(KHookNotifyStr2,"CIPEventNotifier::OpenSessionL - now monitoring interface %S (pos #%d)");
		HookLog::Printf(KHookNotifyStr2,&aIfName,ifPos);
		)
		
	return iface->GetInterfaceIndex(); // later we might create a session id and map it to i/f #
	}

	
TInt CIPEventNotifier::CloseSession(TUint aHandle)
	{
	TInt acIfPos = FindPositionOfInterface(aHandle);

	if(acIfPos == KErrNotFound)
		{
		return KErrNotFound;
		}

	CIPEventNotifierInterface* acIf = iInterfaces[acIfPos];

	acIf->DecreaseRefCount();
	
	// shouldn't be more closes than news! naughty client.
	__ASSERT_ALWAYS((acIf->GetRefCount() >= 0), User::Panic(_L("IPEN:CloseSession"),1));
	
	LOG(
		_LIT(KHookNotifyStr2,"CIPEventNotifier::CloseSession handle %d (pos #%d)");
		HookLog::Printf(KHookNotifyStr2,aHandle,acIfPos);
		)	
	
	if( AnyClientsCurrentlyInterested() == false )
		{
		UnregisterHooks();
		}

	return KErrNone;
	}



void CIPEventNotifier::AddInterfaceL(TUint32 aIfIndex, const TDesC& aIfName)
	{
	CIPEventNotifierInterface* newActIf = new (ELeave)CIPEventNotifierInterface(aIfIndex, aIfName);
	CleanupStack::PushL( newActIf );
	iInterfaces.AppendL(newActIf);
	CleanupStack::Pop( newActIf );
	}



void CIPEventNotifier::AddKnownInterfacesL()
    {
    TUint   bufsize = 2048;
    HBufC8 *buffer = 0;
    TInt exceed;
    buffer = GetBuffer(buffer, bufsize);
    User::LeaveIfNull(buffer);
    TPtr8 bufdes = buffer->Des();
    
	do
		{
	    //-- get list of network interfaces
	    // if exceed>0, all interface could not fit into the buffer.
	    // In that case allocate a bigger buffer and retry.
	    // There should be no reason for this while loop to take more than 2 rounds.
	    bufdes.Set(buffer->Des());
	    exceed = iProtocolIPv6->GetOption(KSolInetIfQuery, KSoInetInterfaceInfo, bufdes);
	    if(exceed > 0)
	        {
	        bufsize += exceed * sizeof(TInetInterfaceInfo);
	        buffer = GetBuffer(buffer, bufsize);
	        User::LeaveIfNull(buffer);
	        }
	    } while (exceed > 0);
    
    TOverlayArray<TInetInterfaceInfo> infoIface(bufdes);
    
    TInt idx;
    for(idx=0; idx < infoIface.Length(); ++idx)
        {
        TInetInterfaceInfo& iface = infoIface[idx];

		if(FindPositionOfInterface(iface.iIndex) == KErrNotFound)
			{
			LOG(
		        TBuf<2048> tmpBuf;
	        	tmpBuf.Format(_L("Detected new i/f: index:%d, name: "),iface.iIndex);
		        tmpBuf.Append(iface.iName);
		        tmpBuf.AppendFormat(_L(", state:%d, SMti:%d, RMtu:%d, SpeedMetric:%d, feat:%d,"),
		        			iface.iState,iface.iSMtu,iface.iRMtu,iface.iSpeedMetric,iface.iFeatures/*,iface.iHwAddr*/);
		        HookLog::Printf(tmpBuf);
	        	)

			TRAP_IGNORE(AddInterfaceL(iface.iIndex,iface.iName));
			}
        }
	LOG(LogKnownInterfaces());
	delete buffer;
    }


HBufC8* CIPEventNotifier::GetBuffer(HBufC8* apBuf, TInt aBufLenRequired)
    {
    HBufC8* pBufResult = NULL;
    
    if(aBufLenRequired <= 0 ) 
        return NULL; //-- invalid argument
    
    if(!apBuf)
        {//-- the buffer is not allocated at all, try allocate it
        pBufResult = HBufC8::New(aBufLenRequired);
        }
    else
        {//-- the buffer is already allocated. Check its size and try to reallocate if
        //-- required size is bigger than existing. Otherwise do nothing.   
        
        if(apBuf->Des().MaxSize() >= aBufLenRequired)
            pBufResult = apBuf; //-- do nothing, existing buffer is big enough
        else
            {  //-- delete and allocate new buffer to avoid data copying
            delete apBuf;
            pBufResult = HBufC8::New(aBufLenRequired);
            }
        }
    
    return pBufResult;
    }


TInt CIPEventNotifier::FindPositionOfInterface(TUint32 aIfIndex) const
	{
	TInt i;
	for( i=0 ; i<iInterfaces.Count() /*inline call*/ ; ++i )
		{
		if(iInterfaces[i]->GetInterfaceIndex() == aIfIndex)
			{
			return i;
			}
		}
	return KErrNotFound;
	}



TInt CIPEventNotifier::FindPositionOfInterfaceByName(const TDesC& aIfName) const
	{
	TInt i;
	for( i=0 ; i<iInterfaces.Count() ; ++i )
		{
		if(iInterfaces[i]->GetInterfaceName() == aIfName)
			{
			return i;
			}
		}

	return KErrNotFound;
	}


CIPEventNotifierInterface* CIPEventNotifier::GetInterfaceWithIndexL(TInt aIpIfIndex)
	{
		TInt aIfPos;
		aIfPos = FindPositionOfInterface(aIpIfIndex);
		if(aIfPos < 0)
			{
			LOG (
				_LIT(KHookNotifyStr,"ERROR: couldn't find interface %d. err %d");
				HookLog::Printf( KHookNotifyStr,aIpIfIndex, aIfPos);
				)
			User::Leave(aIfPos);
			}
		
		return iInterfaces[aIfPos];
	}


TBool CIPEventNotifier::AnyClientsCurrentlyInterested(void) const
	{
#ifdef ALWAYS_PUBLISH_IP_EVENTS
	return true;
#else
	TInt i;
	for( i=0 ; i<iInterfaces.Count() ; ++i )
		{
		if(iInterfaces[i]->GetRefCount() != 0       ||
		   iInterfaces[i]->iMFlagReceived == 0         ||
		   iInterfaces[i]->iLinklocalAddressKnown == 0   )
		   		// (We must always listen until we've received the M flag and a LL addr.
		   		//  This is because DHCP must have access to these events, even if it hasn't started yet.)
			{
			return true;
			}
		}
	return false;
#endif
	}


TBool CIPEventNotifier::ShouldPublishEvent(TUint aInterfaceIndex)
	{
	TInt aIfPos;
	aIfPos = FindPositionOfInterface(aInterfaceIndex);
	if(aIfPos == KErrNotFound)
		{
		LOG ( HookLog::Printf(_L("UNEXPECTED: Interface %d not yet known. Trying to find it"),aInterfaceIndex); )
		TRAP_IGNORE(AddKnownInterfacesL())
		aIfPos = FindPositionOfInterface(aInterfaceIndex);
		if(aIfPos == KErrNotFound)
			{
			LOG ( HookLog::Printf(_L("VERY UNEXPECTED: Couldn't add interface %d"),aInterfaceIndex); )
			return EFalse;
			}
		aIfPos = FindPositionOfInterface(aInterfaceIndex);
		}
	
#ifdef ALWAYS_PUBLISH_IP_EVENTS
    return ETrue;
#else
	CIPEventNotifierInterface* aIf = iInterfaces[aIfPos];
	// if the M flag / LL addr has not yet been received we should try to catch it
	return( (aIf->iMFlagReceived == 0)  ||
	        (aIf->iLinklocalAddressKnown == 0)  ||
	        (aIf->GetRefCount() > 0)      );
	    
#endif
	}


void CIPEventNotifier::InterfaceDetached(const TDesC & aName, CNifIfBase *aIf)
	{
	(void)aIf;
	TInt pos = FindPositionOfInterfaceByName(aName);
	LOG(
		_LIT(KHookNotifyStr,"CIPEventNotifier::InterfaceDetached %S ptr=%08x [pos %d of %d interfaces]");
		HookLog::Printf(KHookNotifyStr,&aName,aIf,pos,iInterfaces.Count());
		)	
	if(pos != KErrNotFound)
		{
		CIPEventNotifierInterface* notIf = iInterfaces[pos];
		delete notIf;
		iInterfaces.Remove(pos);
		}
	LOG(LogKnownInterfaces());
	}
	
#ifdef _DEBUG
void CIPEventNotifier::LogKnownInterfaces()
	{
	LOG(
		_LIT(KHookNotifyHeader,"Dumping iInterfaces:");
		HookLog::Printf(KHookNotifyHeader);
		)
			
	for(TInt i=0 ; i<iInterfaces.Count() ; ++i )
		{
		LOG(
			_LIT(KHookNotifyStr,"\tPos %d: %S, stack's index %d");
			TName name = iInterfaces[i]->GetInterfaceName();
			TUint32 idx = iInterfaces[i]->GetInterfaceIndex();
			HookLog::Printf(KHookNotifyStr, i, &name, idx);
			)	
		}
	}
#endif	


void CIPEventNotifier::RegisterHooksL(void)
/**
 * Registers the hook to catch IP stack events
 */
	{
	if(!(iProtocolIPv6 && iEventService && iIPEventListener))
		{
		User::Leave(KErrNotReady);
		}

	// bind to IP for incoming packets
	iProtocolIPv6->BindL((CProtocolBase*)this, BindHookAll());


	// Register our listener class with the event service. We're interested in:
	//
	// i.  address events
	//     for determining when DAD has completed and when link-local addresses have been assigned
	//     to an interface.
	// ii. interface events to allow the notifier to associate interface names with the stack's
	//     internal indices.
	//
	iEventService->RegisterListener(iIPEventListener, EClassAddress);
	iEventService->RegisterListener(iIPEventListener, EClassInterface);
	}


	
void CIPEventNotifier::UnregisterHooks(void)
/**
 * Detaches this hook from the running of the stack.
 *  This is so IPEN doesn't slow down the stack while it isn't
 *   needed by any clients.
 *
 *  e.g. without this mechanism, every incoming packet would pass through
 *   CIPEventNotifier::ApplyL pointlessly.
 *
 */
	{
	if(!(iProtocolIPv6 && iEventService && iIPEventListener))
		{
		return;
		}

	// If IPv6 exists, unbind the hook. Covers both in/out hooks (I hope)
	iProtocolIPv6->Unbind(this);

	// Unregister the address/interface listener.
	iEventService->RemoveListener(iIPEventListener);
	}

	

void CIPEventNotifier::FillIdentification(TServerProtocolDesc& anEntry)
/**
 * Fills in an existing protocol description structure with details of this protocol.
 *
 * @param aProtocolDesc: reference to the structure to be filled in
 */
	{
	anEntry.iName=_S("ipen");
	anEntry.iAddrFamily=KAfInet; //KAfExain;
	anEntry.iSockType=KSockDatagram;
	anEntry.iProtocol=KProtocolId;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=KSIDatagram | KSIConnectionLess;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=KSocketNoSecurity;
	anEntry.iMessageSize=0xffff;
	anEntry.iServiceTypeInfo=0; // 1 for ability to create sockets
	anEntry.iNumSockets=KUnlimitedSockets;
	}


void CIPEventNotifier::Identify(TServerProtocolDesc* aProtocolDesc) const
/**
 * Fills in an existing protocol description structure with details of this protocol.
 *
 * @param aProtocolDesc: pointer to the structure to be filled in
 */
	{
	FillIdentification(*aProtocolDesc);
	}


TInt CIPEventNotifier::DefineProperty(Meta::SMetaData& aEvent, TUint aHandle)
	{
	TSecurityPolicy readPol(TSecurityPolicy::EAlwaysPass);
	_LIT_SECURE_ID(ipeventsid,0x102045B9);
	_LIT_SECURITY_POLICY_S0(writePol,ipeventsid);

	TInt err = iPublisher.Define(TUid::Uid(aEvent.GetTypeId().iType), aHandle,
						RProperty::EByteArray/*, readPol, writePol*/);
						
	// Does not matter if property has already been defined - we ignore the error.
	if( err == KErrAlreadyExists )
		{
		err = KErrNone;
		}
		
	return err;
	}


void CIPEventNotifier::PublishToHandle(TUint aHandle, Meta::SMetaData& aData)
	{
	TBuf8<RProperty::KMaxPropertySize> buf;

	TInt res =  aData.Store(buf);
	if(res != KErrNone)
		{
		LOG(HookLog::Printf( _L("IPEN can't store: %d"), res));
		return;
		}

	TPtrC8 toSet = buf;
	res = iPublisher.Set(TUid::Uid(aData.GetTypeId().iType), aHandle, toSet);
	if(res != KErrNone)
		{
		LOG(HookLog::Printf( _L("IPEN can't publish: %d"), res));
		}
	}



/**
 * Incoming packet
 */
TInt CIPEventNotifier::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
	{

	LOG	(
		_LIT(KHookNotifyStr,"CIPEventNotifier::ApplyL hit with protocol: %d");
		HookLog::Printf( KHookNotifyStr , aInfo.iProtocol );
		)

	if(aInfo.iProtocol == static_cast<TInt>(KProtocolInet6Icmp) &&  // quickest first
		ShouldPublishEvent(aInfo.iInterfaceIndex) )
		{

		LOG	(
			_LIT(KHookNotifyStr," ICMPv6 packet detected");
			HookLog::Printf( KHookNotifyStr );
			)

		/**
		 * Could switch on ICMP type here, in case the hook needs to catch
		 *   other ICMP packet types..
		 */
		TInet6Checksum<TInet6HeaderICMP_RouterAdv> icmp(aPacket,aInfo.iOffset);

		if(icmp.iHdr)
			{
			if (icmp.iHdr->Type() == KInet6ICMP_RouterAdv &&
				icmp.VerifyChecksum( aPacket,
							  aInfo.iProtocol == static_cast<TInt>(KProtocolInet6Icmp) ? &aInfo : NULL,
							  aInfo.iOffset) )
				{
				TBool mflag = (icmp.iHdr->M() ? ETrue : EFalse) ;
				TBool oflag = (icmp.iHdr->O() ? ETrue : EFalse) ;

				LOG	(
					_LIT(KHookNotifyStr," IP EVENT on if# %d: Publishing M flag: %d");
					HookLog::Printf( KHookNotifyStr, aInfo.iInterfaceIndex, mflag );
					_LIT(KHookOFlagNotifyStr," IP EVENT on if# %d: Publishing O flag: %d");
					HookLog::Printf( KHookOFlagNotifyStr, aInfo.iInterfaceIndex, oflag );
					)

				TRAPD(err, PublishMFlagL(aInfo.iInterfaceIndex, mflag, oflag));
				if(err!=KErrNone)
					{
					LOG	(
					_LIT(KLogStr, "Can't publish M and O flags message! Err %d\n");
					HookLog::Printf(KLogStr, err);
					)
					}
				}
			}

		}
			
	return KIp6Hook_PASS;   // ensure we don't affect the control flow
	}



void CIPEventNotifier::ObtainEventService()
	{
	MInterfaceManager* ifacer = iProtocolIPv6->NetworkService()->Interfacer();

	TRAPD(err, iEventService = IMPORT_API_L (ifacer, MEventService));
	if (err != KErrNone)
		{
		LOG	(
			_LIT(KHookNotifyStr, "MEventService not available (%d). Some features may be missing\n");
			HookLog::Printf(KHookNotifyStr, err);
			)
		}
	}




void CIPEventNotifier::PublishMFlagL(TInt aIpIfIndex, TBool aMflag, TBool aOflag)
	{
	CIPEventNotifierInterface* iface = GetInterfaceWithIndexL(aIpIfIndex);

	if(iface)
		{
		if(iface->iMFlagReceived == 0)
			{
			iface->iMFlagReceived = CMFlagReceived::NewL();
			User::LeaveIfError(DefineProperty(*(iface->iMFlagReceived), aIpIfIndex));
			}
		
		iface->iMFlagReceived->SetMFlag(aMflag);
		iface->iMFlagReceived->SetOFlag(aOflag);
		PublishToHandle(aIpIfIndex, *(iface->iMFlagReceived));
		}
	}

		
void CIPEventNotifier::PublishIPReadyL(TInt aIpIfIndex, const TInetAddr& tmpInetAddr, TBool addressValid)
	{
	CIPEventNotifierInterface* iface = GetInterfaceWithIndexL(aIpIfIndex);

	if(iface)
		{
		if(iface->iIPReady == 0)
			{
			iface->iIPReady = CIPReady::NewL();			
			User::LeaveIfError(DefineProperty(*(iface->iIPReady), aIpIfIndex));
			}
		
		iface->iIPReady->SetIPAddress(tmpInetAddr);
		iface->iIPReady->SetAddressValid(addressValid);
		PublishToHandle(aIpIfIndex, *(iface->iIPReady));
		}
	}

void CIPEventNotifier::PublishLinklocalAddressKnownL(TInt aIpIfIndex, const TInetAddr& tmpInetAddr)
	{
	CIPEventNotifierInterface* iface = GetInterfaceWithIndexL(aIpIfIndex);

	if(iface)
		{
		if(iface->iLinklocalAddressKnown == 0)
			{
			iface->iLinklocalAddressKnown = CLinklocalAddressKnown::NewL();
			User::LeaveIfError(DefineProperty(*(iface->iLinklocalAddressKnown), aIpIfIndex));
			}			
		iface->iLinklocalAddressKnown->SetIPAddress(tmpInetAddr);
		PublishToHandle(aIpIfIndex, *(iface->iLinklocalAddressKnown));
		}
	}


void CIPEventNotifier::PublishLastKnownStates(CIPEventNotifierInterface* aIface)
{
	TUint32 ifIdx = aIface->GetInterfaceIndex();
	
	// For each message from which we need state, not just events
	//  i.e. everything that might happen before DHCP starts and registers
	//
	if(aIface->iMFlagReceived)
		{
		PublishToHandle(ifIdx, *(aIface->iMFlagReceived));
		}
	if(aIface->iLinklocalAddressKnown)
		{
		PublishToHandle(ifIdx, *(aIface->iLinklocalAddressKnown));
		}
}
