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
//


#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <nifmbuf.h>
#include "PKTDRV.H"
#include "ETHINTER.H"
#include "Cardctl.h"


/**
@internalComponent
*/
extern "C"
{
	IMPORT_C CPktDrvFactory * NewPcCardPktDrvFactoryL();	//< Force export 
}

/**
Library file is opened and this is the the first and only export.
@internalComponent
@return A pointer to CPktDrvFactory object.
*/
EXPORT_C CPktDrvFactory* NewPcCardPktDrvFactoryL()
{
	CPktDrvFactory *f=new (ELeave) CPcCardPktDrvFactory;
	return f;
}

/**
Create the packet driver object
@param aParent Pointer to the parent Ethint NIF class.
@return A pointer to CPktDrvBase object.
*/
CPktDrvBase* CPcCardPktDrvFactory::NewDriverL(CLANLinkCommon* aParent)
{
	CPktDrvBase *drv = new (ELeave) CPcCardPktDrv(*this);
	CleanupStack::PushL(drv);
	drv->ConstructL(aParent);
	CleanupStack::Pop();
	return drv;
}

/**
Constructor.
*/
CPcCardPktDrvFactory::CPcCardPktDrvFactory()
{
	
}

/**
Packet Driver version number.
@return Version number of the Packet Driver
*/
TVersion CPcCardPktDrvFactory::Version() const
{
	return(TVersion(KPcCardDrvMajorVersionNumber,KPcCardDrvMinorVersionNumber,KPcCardDrvBuildVersionNumber));
}


/**
Constructor. Packet Driver object. 
*/
CPcCardPktDrv::CPcCardPktDrv(CPktDrvFactory& aFactory) : CPktDrvBase(aFactory)
{
	
}

/**
Only one object owned
Destructor.
*/
CPcCardPktDrv::~CPcCardPktDrv()
{
	delete iControl;
}

/**
Pure Virtual Construction of the CPktDrvBase object
@param aParent Pointer to the CLANLinkCommon class.
*/
void CPcCardPktDrv::ConstructL(CLANLinkCommon* aParent)
{
	iParent = aParent;
	// Card control engine
	iControl=CPcCardControlEngine::NewL(this);
}

/**
Pure Virtual Downstack call. "	Call to LDD or subordinate object to start/initialise the 
Physical device
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::StartInterface()
{
	// Opens the card and queues a read
	// Control engine validates
	TRAPD(err,iControl->StartL());
	return err;
}
 
/**
Call to LDD or subordinate object to stop/de-initialise the Physical device
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::StopInterface()
{
	// Closes the card
	// Control engine validates
	iControl->Stop();
	return KErrNone;
}

/**
Call to LDD or subordinate object to reset/re-initialise the Physical device
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::ResetInterface()
{
	iControl->Stop();
	TRAPD(err,iControl->StartL());
	return err;
}

/**
Sender Class is generic and does not want to know about RMBuf's
Copy to a Heap Buffer and Free the packet. EtherII MAC layer comments
Say we should free the packet buffer
RMBuf could contain a chain so get into a contiguous buffer
@param aPacket Reference to a chain of data buffers to be passed to the line.
@return 0 Tells the higher layer to send no more data.
		1 Tells higher layer that it can send more data.
*/
TInt CPcCardPktDrv::Send(RMBufChain &aPacket)
{
	if(!iControl->CardOpen())
		{
		aPacket.Free();
		return KErrNotReady;
		}
	HBufC8 * buf = NULL;
	TRAPD(err,buf = HBufC8::NewMaxL(aPacket.Length()));
	if(err != KErrNone)
		{
		aPacket.Free();
		return err;
		}
	TPtr8 ptr = buf->Des();
	aPacket.CopyOut(ptr);
	aPacket.Free();
	return(iControl->Send(buf));
}

/**
Read the Available data.
@param aBuffer A Reference to a buffer holding data.
*/
void CPcCardPktDrv::ReadDataAvailable(TDesC8& aBuffer)
{
	RMBufPacket frame;
	TRAPD(ret,frame.CreateL(aBuffer,0));
	if (ret == KErrNone)
		{
		frame.Pack();
		iParent->Process(frame);
		}
}

/**
Resume Sending is a notification call into NIF from the lower layer telling the NIF that a 
previous sending congestion situation has been cleared and it can accept more downstack data.
*/
void CPcCardPktDrv::ResumeSending()
{
	iParent->ResumeSending();
}

/**
Call to LDD or subordinate object to set the receive mode of the LAN Device
@param aMode The mode to be set for the LAN Device.			
@return KErrNotSupported LAN Device does not support.
*/
TInt CPcCardPktDrv::SetRxMode(TRxMode /*aMode*/)
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to Get the receive mode of the LAN Device
@return KErrNotSupported LAN Device does not support.
*/
TInt CPcCardPktDrv::GetRxMode() const
{
	return KErrNotSupported;
}

/**
Specifies the AccessType.
@return KErrNotSupported LAN Device does not support.
*/
TInt CPcCardPktDrv::AccessType()
{
	return KErrNotSupported;
}

/**
Specifies the ReleaseType.
@return KErrNotSupported LAN Device does not support.
*/
TInt CPcCardPktDrv::ReleaseType()
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to get the Hardware address of the LAN Device
@return NULL Failure.
		(NULL Terminated Binary String) The Hardware Address of the interface. LAN Device 
		Specific
*/
TUint8* CPcCardPktDrv::GetInterfaceAddress()const
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
TInt CPcCardPktDrv::SetInterfaceAddress(const THWAddr&)
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to retrieve the Multicast List from the LAN Device
@param aAddr Address of where the Multicast list should be written.
@param n Output Parameter , number of Addresses written
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::GetMulticastList(const THWAddr* /*aAddr*/, TInt& /*n*/) const
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to set the Multicast List for the LAN Device.
@param aAddr Address of where the Multicast list should be written.
@param n Output Parameter , number of Addresses written
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::SetMulticastList(const THWAddr* /*aAddr*/, TInt /*n*/)
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to power up the LAN Device.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::InterfacePowerUp()
{
	return KErrNotSupported;
}
	
/**
Call to LDD or subordinate object to power down the LAN Device
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::InterfacePowerDown()
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to suspend the LAN Device.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::InterfaceSleep()
{
	return KErrNotSupported;
}

/**
Call to LDD or subordinate object to resume the LAN Device.
@return KErrNone		 if Successful 
		KErrNotSupported LAN Device does not support.
		Implementation specific Error Code  Failure
*/
TInt CPcCardPktDrv::InterfaceResume()
{
	return KErrNotSupported;
}

/**
Resume Sending is a notification call into NIF from the lower layer telling the NIF that 
the interface is now up and can accept and transmit data. NIF subsequently calls all the 
bearers' StartSending() methods directly.
*/
void CPcCardPktDrv::LinkLayerUp()
{
	iParent->LinkLayerUp();
}

/**
Receive notifications from agent
*/
TInt CPcCardPktDrv::Notification(enum TAgentToNifEventType /*aEvent*/, void* /*aInfo*/)
{
	return KErrNotSupported;
}
	
/**
Receive Control() calls from agent/nifman/connection
*/
TInt CPcCardPktDrv::Control(TUint /*aLevel*/,TUint /*aName*/,TDes8& /*aOption*/, TAny* /*aSource*/)
{
	return KErrNotSupported;
}

/**
Return the pointer to the ethernet nif.
Purely for access to commdb reading functionality
*/
CLANLinkCommon* CPcCardPktDrv::NifNotify()
{
	return iParent;
}

