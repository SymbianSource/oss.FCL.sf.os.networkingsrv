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
// Interface to IR-LAN 
// 
//

/**
 @file
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include "ETHINTER.H"
#include "PKTDRV.H"
#include "IRLAN.H"
#include "IRLANUTL.H"
#include "INTSOCK.H"
#include "irlantimer.h"

#ifdef __TRACEWIN__
  #include <log.h>
#else
  #define LOG(a)
#endif


//#####################################################################

/**
Global Factory Function.
@internalComponent
*/
extern "C"
{
    IMPORT_C CPktDrvFactory * NewIrlanPktDrvFactoryL();	// Force export
}

/**
All PktDrv's export as their first Ordinal this global 'C' function.
This function is called to create construct and return a pointer to an instance of the 
Ethint specific factory class CLANLinkFactory derived from CPktDrvFactory 
@return A pointer to CNifFactory object.
@internalComponent
*/
EXPORT_C CPktDrvFactory* NewIrlanPktDrvFactoryL()
{
	CPktDrvFactory *f=new (ELeave) CIrlanPktDrvFactory;
	LOG(Log::Printf(_L("IRLAN	Creating instance of CIrlanPktDrvFactory\r\n")));
	return f;
}


/**
Need to switch here on the name and open the according driver DLL.
@param aParent Pointer to the parent Ethint NIF class.
@return A pointer to CPktDrvBase object.
*/
CPktDrvBase* CIrlanPktDrvFactory::NewDriverL(CLANLinkCommon* aParent)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CREATING A PKT DRV: CPktDrvFactory::NewDriverL\r\n")));
#endif
	CPktDrvBase *drv = new (ELeave) CIrlanPktDrv(*this);
	CleanupStack::PushL(drv);
	drv->ConstructL(aParent);
	CleanupStack::Pop();
	return drv;
}


/**
Packet Driver version number.
@return The client side version number
*/
TVersion CIrlanPktDrvFactory::Version() const
{
	return(TVersion(KIrlanMajorVersionNumber,KIrlanMinorVersionNumber,KIrlanBuildVersionNumber));
}


//#####################################################################

/**
Constructor.
*/
CIrlanPktDrv::CIrlanPktDrv(CPktDrvFactory& aFactory) : CPktDrvBase(aFactory)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CIrlanPktDrv constructor\r\n")));
#endif
	__DECLARE_NAME(_S("CIrlanPktDrv"));
}

/**
Destructor.
*/
CIrlanPktDrv::~CIrlanPktDrv()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	~CIrlanPktDrv\r\n")));
#endif
	delete iControl;
}

/**
Find and load protocol
Pure Virtual Construction of the CPktDrvBase object.
@param aParent Pointer to the CLANLinkCommon class.
@bug unhandled error codes
*/
void CIrlanPktDrv::ConstructL(CLANLinkCommon* aParent)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CIrlanPktDrv::ConstructL\r\n")));
#endif
	TInt ret;
	iParent=aParent;
	// Creating the Irlan state machine
	TRAP(ret,iControl=CIrlanControlEngine::NewL(this));
	// Now kick off the Irlan state machine active object
  	TRAP(ret,iControl->StartL());
}

/**
Downstack - Supplies the caller with information on the NIF written to the TNifIfInfo class 
defined in nifman.h.
@param aInfo  Class for returning information on the NIF
@param aIndex PPP Specific
*/
TInt CIrlanPktDrv::Info(TNifIfInfo& /*aInfo*/, TInt /*aIndex*/) const
{
	return KErrNone;
}

/**
This method is only for use when an interface does not have the correct MAC address when ARP 
is started
*/
void CIrlanPktDrv::FoundMACAddr()
{
	TRAPD(ret,iParent->FoundMACAddrL());
	if (KErrNone != ret)
		{
#ifdef __TRACEWIN__
		LOG(Log::Printf(_L("IRLAN	CIrlanPktDrv::FoundMACAddrL Failure: %d\r\n"),ret));
#endif		
		}
}


/**
Called after we have managed to get a reply to GetFilterConfigCmd
*/
void CIrlanPktDrv::LinkLayerUp()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CIrlanPktDrv::LinkLayerUp!\r\n")));
#endif
	iParent->LinkLayerUp();
}


/**
This is where we would send out the first ethernet packets.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::StartInterface()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	PKTDRV:StartInterface\r\n")));
#endif
	return KErrNone;
}

/**
Call to LDD or subordinate object to stop/de-initialise the Physical device
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::StopInterface()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	PKTDRV:StopInterface\r\n")));
#endif

	return KErrNone;
}

/**
Call to LDD or subordinate object to reset/re-initialise the Physical device
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::ResetInterface()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	PKTDRV:ResetInterface\r\n")));
#endif
	return KErrNone;
}


/**
Tx flow control stuff
Sender Class is generic and does not want to know about RMBuf's
Copy to a Heap Buffer and Free the packet. EtherII MAC layer comments
Say we should free the packet buffer
RMBuf could contain a chain so get into a contiguous buffer
@param aPacket Reference to a chain of data buffers to be passed to the line.
@return 0 Tells the higher layer to send no more data.
		1 Tells higher layer that it can send more data.
*/
TInt CIrlanPktDrv::Send(RMBufChain &aPacket)
{
	TInt ret = KErrNone;
	TRAPD(err,ret = iControl->Send(aPacket));
	if(err != KErrNone)
		{
		ret = err;
		}
	return ret;
}

/**
Here what the pkt drv must do is go thro' it's list of MAC layers and
keep trying to process until it gets back a KErrNone.
@param aPdu A reference to the Packet to process.
*/
void CIrlanPktDrv::Process(RMBufChain& aPdu)
{
	iParent->Process(aPdu);
}

/**
Call to LDD or subordinate object to set the receive mode of the LAN Device
@param aMode The mode to be set for the LAN Device.			
@return KErrNotSupported LAN Device does not support.
*/
TInt CIrlanPktDrv::SetRxMode(TRxMode /*AMode*/)
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to Get the receive mode of the LAN Device
@return KErrNotSupported LAN Device does not support.
*/
TInt CIrlanPktDrv::GetRxMode() const
{
	return KErrNotSupported;
}

/**
Specifies the AccessType.
@return KErrNotSupported LAN Device does not support.
*/
TInt CIrlanPktDrv::AccessType()
{
	return KErrNotSupported;
}

/**
Specifies the ReleaseType.
@return KErrNotSupported LAN Device does not support.
*/
TInt CIrlanPktDrv::ReleaseType()
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to get the Hardware address of the LAN Device
@return NULL Failure.
		(NULL Terminated Binary String) The Hardware Address of the interface. LAN Device 
		Specific
*/
TUint8* CIrlanPktDrv::GetInterfaceAddress()const
{
	return (iControl->GetInterfaceAddress());
}

/**
Call to LDD or subordinate object to set the Hardware address of the LAN Device.
@param THWAddr Address of where the Multicast list should be written.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::SetInterfaceAddress(const THWAddr&)
{
	return KErrNotSupported;
}

/**
This corresponds to getting the relevant values from IrLAN data model
@param aAddr Address of where the Multicast list should be written.
@param n Output Parameter , number of Addresses written
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::GetMulticastList(const THWAddr* /*aAddr*/, TInt& /*n*/) const
{
	return KErrNotSupported;
}

/**
This corresponds to setting the relevant values from IrLAN data model
@param aAddr Address of where the Multicast list should be written.
@param n Output Parameter , number of Addresses written
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::SetMulticastList(const THWAddr* /*aAddr*/, TInt /*n*/)
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to power up the LAN Device.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::InterfacePowerUp()
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to power down the LAN Device
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::InterfacePowerDown()
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to suspend the LAN Device.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::InterfaceSleep()
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to resume the LAN Device.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CIrlanPktDrv::InterfaceResume()
{
	return KErrNotSupported;
}

/**
Receive notifications from agent
*/
TInt CIrlanPktDrv::Notification(enum TAgentToNifEventType /*aEvent*/, void* /*aInfo*/)
{
	return KErrNotSupported;	
}
	
/**
Receive Control() calls from agent/nifman/connection
*/
TInt CIrlanPktDrv::Control(TUint /*aLevel*/,TUint /*aName*/,TDes8& /*aOption*/, TAny* /*aSource*/)
{
	return KErrNotSupported;	
}
