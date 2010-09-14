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
// Control engine for ethernet packet driver 
// 
//

/**
 @file
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <es_ini.h>
#include <ir_sock.h>
#include "PKTDRV.H"
#include "ETHINTER.H"
#include "Cardctl.h"
#include <f32file.h>
#include <e32svr.h>

//#define __DebugCardCtl__ 1

#if defined(__DebugCardCtl__)
#include <e32svr.h>
 #define DPRINT(x) RDebug::Print x
 #pragma message ("Warning: this code contains RDebug::Print statements!  Do not submit to the mainline with __DebugCardCtl__ defined.")
#else
 #define DPRINT(x)
#endif

/**
Create a new CPcCardControlEngine object.
@param aPktDrv Pointer to PC Card Packet Driver.
@return A pointer to CPcCardControlEngine object.
*/
CPcCardControlEngine *CPcCardControlEngine::NewL(CPcCardPktDrv* aPktDrv)
{
	CPcCardControlEngine *cc=new (ELeave) CPcCardControlEngine(aPktDrv);
	CleanupStack::PushL(cc);
	cc->ConstructL();
	CleanupStack::Pop();
	return cc;
}

/**
Physical device driver settings
@internalComponent
*/
_LIT(KPddSection,"pdd_settings");
 
/**
PCMCIA Socket Number
@internalComponent
*/
_LIT(KSocketNumber,"PCMCIA_socket"); 

/**
Create the CPcCardControlEngine object.
*/
void CPcCardControlEngine::ConstructL()
{
	iSender = CPcCardSender::NewL(this);
	iReceiver = CPcCardReceiver::NewL(this);
	iEventHandler = CPcCardEventHandler::NewL(this);

	LoadDeviceDriversL();

	CESockIniData* ini = CESockIniData::NewL(ETHER802_CONFIG_FILENAME);
	CleanupStack::PushL(ini);

	TInt socket;
	if(!ini->FindVar(KPddSection,KSocketNumber,socket))
		{
		User::Leave(KErrNotFound);
		}
	
	CleanupStack::PopAndDestroy(ini);

	TInt error = iCard.Open(socket);
	User::LeaveIfError(error);
}

/**
Open the Card LDD
*/
void CPcCardControlEngine::StartL()
{
	iCardOpen = ETrue;
	iReceiver->QueueRead();
	LinkLayerUp();
}

/**
Find and loads the LDD and the PDD if the logical device driver loaded OK.
The driver names are read from the LAN bearer table in commdb.
*/
void CPcCardControlEngine::LoadDeviceDriversL()
{
	TInt err;

	//
	// Get the device driver filenames for loading
	//
	TBuf<KCommsDbSvrDefaultTextFieldLength> pddOrLddFileName;
	
	// LDD first...
	TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(LAN_BEARER);
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(LAN_BEARER_LDD_FILENAME));
	
	err = iNotify->NifNotify()->ReadDes(columnName, pddOrLddFileName); // Get the LDD name from commdb
	
	if(err!=KErrNone)
		{
		__FLOG_STATIC(KEther802LogTag1,KEthLogTag3, _L("Could not find LDD filename in commdb - is .cfg file up-to-date?  See ether802.ini for information on required fields in commdb."));
		User::Leave(err);
		}
	
	err=User::LoadLogicalDevice(pddOrLddFileName);
	if(err != KErrNone && err != KErrAlreadyExists)
		{
		User::Leave(err);
		}
	
	// ...and now the PDD
	columnName.Zero();
	columnName.Append(TPtrC(LAN_BEARER));
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(LAN_BEARER_PDD_FILENAME));

	err = iNotify->NifNotify()->ReadDes(columnName, pddOrLddFileName); // Get the PDD filename from commdb

	if(err!=KErrNone)
		{
		__FLOG_STATIC(KEther802LogTag1,KEthLogTag3, _L("Could not find PDD filename in commdb - is .cfg file up-to-date?  See ether802.ini for information on required fields in commdb."));
		User::Leave(err);
		}

	err = User::LoadPhysicalDevice(pddOrLddFileName);
	if (err != KErrNone && err != KErrAlreadyExists)
		{
		User::Leave(err);
		}

	//
	// Get device driver names for unloading
	//
	columnName.Zero();
	columnName.Append(TPtrC(LAN_BEARER));
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(LAN_BEARER_PDD_NAME));

	err = iNotify->NifNotify()->ReadDes(columnName, iPDDName); // Get the PDD name from commdb (so we can close it when we've finished with it)

	if(err!=KErrNone)
		{
		__FLOG_STATIC(KEther802LogTag1,KEthLogTag3, _L("Could not find PDD name in commdb - is .cfg file up-to-date?  See ether802.ini for information on required fields in commdb."));

#ifdef _DEBUG
		// Not being able to unload the device drivers is not a fatal error, so don't worry too 
		// much if we can't read the field out of commdb in release builds, but if the user is 
		// using a debug nif they ought to get it right...
		User::Leave(err);
#endif // _DEBUG
		}

	// Rather than fiddle around trying to reuse the contents of the descriptor (problems with field name lengths), just zero and start again
	columnName.Zero();
	columnName.Append(TPtrC(LAN_BEARER));
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(LAN_BEARER_LDD_NAME));

	err = iNotify->NifNotify()->ReadDes(columnName, iLDDName); // Get the LDD name from commdb (so we can close it when we've finished with it)
	
	if(err!=KErrNone)
		{
		__FLOG_STATIC(KEther802LogTag1,KEthLogTag3, _L("Could not find LDD name in commdb - is .cfg file up-to-date?  See ether802.ini for information on required fields in commdb."));

#ifdef _DEBUG
		User::Leave(err);	// see reasoning for LDD above
#endif // _DEBUG
		}

	__FLOG_STATIC(KEther802LogTag1,KEthLogTag3, _L("Device drivers loaded"));
}

/**
Cancel I/O and close the Card LDD.
*/
void CPcCardControlEngine::Stop()
{
	// LDD Performs status checks on Read and Write
	// Completes requests with an error code if they are pending
	iCard.WriteCancel();
	iSender->EmptyQueue();
	iSender->Cancel();

	iCard.ReadCancel();
	iCardOpen = EFalse;
	iCard.Close();
}

CPcCardControlEngine::CPcCardControlEngine(CPcCardPktDrv* aPktDrv)
:iCardOpen(EFalse), iNotify(aPktDrv)
/**
Constructor.
@param aPktDrv Pointer to PC Card Packet Driver.
*/
{

}		

/**
Destructor.
*/
CPcCardControlEngine::~CPcCardControlEngine()
{
#ifdef _DEBUG
	// see reasoning for only doing this in debug builds in LoadPacketDrivers()
	User::FreeLogicalDevice(iLDDName);
	User::FreePhysicalDevice(iPDDName);
#endif

	delete iReceiver;
	delete iSender;
	delete iEventHandler;
}

/**
Upwards notify
@param aBuffer A Reference to a buffer holding data.
*/
void CPcCardControlEngine::ProcessReceivedPacket(TDesC8& aBuffer)
{
	iNotify->ReadDataAvailable(aBuffer);
}

/**
Resume Sending is a notification call into NIF from the lower layer telling the NIF that a 
previous sending congestion situation has been cleared and it can accept more downstack data.
*/
void CPcCardControlEngine::ResumeSending()
{
	iNotify->ResumeSending();
}

/**
Resume Sending is a notification call into NIF from the lower layer telling the NIF that 
the interface is now up and can accept and transmit data. NIF subsequently calls all the 
bearers' StartSending() methods directly.
*/
void CPcCardControlEngine::LinkLayerUp()
{
	iNotify->LinkLayerUp();
}

/**
Sender class handles queueing and takes ownership of the HBuf and the CIOBuffer.
@param aBuffer The data to be send is set.
@return 0 Tells the higher layer to send no more data.
		1 Tells higher layer that it can send more data.
*/
TInt CPcCardControlEngine::Send(HBufC8* aBuffer)
{
	DPRINT((_L(">CPcCardControlEngine::Send")));
	CIOBuffer* buf = NULL;
	TRAPD(err,buf = CIOBuffer::NewL(aBuffer));
	if(err != KErrNone)
		{
		DPRINT((_L(">CPcCardControlEngine::Send No Buffer.")));
		delete aBuffer;
		}
	else
		{
		DPRINT((_L(">CPcCardControlEngine::Send About to iSender.")));
		err = iSender->Send(buf);
		}
	DPRINT((_L("<CPcCardControlEngine::Send.")));
	return err;
}

/**
Call to LDD or subordinate object to get the Hardware address of the LAN Device
@return NULL Failure.
		(NULL Terminated Binary String) The Hardware Address of the interface. LAN Device 
		Specific
*/
TUint8* CPcCardControlEngine::GetInterfaceAddress()
{
	iConfig.SetMax();
	iCard.Config(iConfig);
	return ((TUint8*)iConfig.Ptr())+2; // The MAC address is located 2 bytes
					   // from the start of the buffer
}

#if !defined(__WINS__)
//

/** 
ethermac.dat reading - work around for mac address problem 
@internal component
*/
_LIT(KEtherMacFileName,"C:\\System\\Data\\EtherMac.dat");

/**
Parse the Machine Address from the File.
*/
void CPcCardControlEngine::ParseMACFromFileL()
{

	TBuf8<64> controlBuf;  // the size of this is defined in the driver as 64.
	TBuf8<12> macAddress = 0;
	RFile macFile;
	RFs fileSrv;
	
	User::LeaveIfError(fileSrv.Connect());
	User::LeaveIfError(macFile.Open(fileSrv,KEtherMacFileName,EFileRead));
	CleanupClosePushL(macFile);
	User::LeaveIfError(macFile.Read(macAddress,12));
	CleanupStack::PopAndDestroy(&macFile);
	fileSrv.Close();
	controlBuf.SetLength(8);	
	controlBuf[0] = KEthSpeed10BaseT;
	controlBuf[1] = KEthDuplexHalf;

	TBuf<20> validChars(_L("0123456789abcdef"));
	TUint8 value;
	TUint8 upper=0;
	TChar c;
	TInt pos; 
	iConfig.SetMax(); // MAC Address fix
	for(TInt i=0; i<6; i++)
	{
		c = macAddress[2*i];
		c.LowerCase();
		if((pos = validChars.Locate(c))==KErrNotFound)
		{
			pos = upper;
			break;
		}
		upper = (TUint8)pos;
		c = macAddress[(2*i)+1];
		c.LowerCase();
		if((pos = validChars.Locate(c))==KErrNotFound)
		{
			User::Leave(KErrNotFound);
		}
		value = (TUint8)pos;
		value = (TUint8)((upper<<4) | value);
		controlBuf.Append(value);
		iConfig[i+2]=value; // MAC Address fix 21/05/01
	}
	TRequestStatus status = 0;
	DPRINT((_L("ParseMACFromFileL(), iConfig MAC set to %x %x %x %x %x %x"),
		iConfig[2],iConfig[3],iConfig[4],iConfig[5],iConfig[6],iConfig[7]));
	User::LeaveIfError(iCard.SetMAC(controlBuf));

	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());
}
#endif

/**
Constructor. Generic Buffer class
Currently used for transmit buffers
*/
CIOBuffer::CIOBuffer() : iBufPtr(NULL,0)
{
}

/**
Destructor. Free the HBuf if there is one
*/
CIOBuffer::~CIOBuffer()
{
	FreeData();
}

/**
Creation where we new the HBuf.
@param aSize Length of the Buffer.
@return A pointer to CIOBuffer object.
*/
CIOBuffer* CIOBuffer::NewL(const TInt aSize)
{
	CIOBuffer * self = new (ELeave) CIOBuffer;
	CleanupStack::PushL(self);
	self->ConstructL(aSize);
	CleanupStack::Pop();
	return self;
}

/**
Construction where we new the HBuf
@param aSize Length of the Buffer.
*/
void CIOBuffer::ConstructL(const TInt aSize)
{
	iBuf = HBufC8::NewL(aSize);
	TPtr8 temp=iBuf->Des();
	iBufPtr.Set(temp);
}

/**
HBuf provided. 
@param aBuf The data to be assigned 
@return A pointer to CIOBuffer object.
*/
CIOBuffer* CIOBuffer::NewL(HBufC8* aBuf)
{
	CIOBuffer * self = new (ELeave) CIOBuffer;
	self->Assign(aBuf);
	return self;
}

/**
Offset 
*/
TInt CIOBuffer::LinkOffset()
{
	return _FOFF(CIOBuffer,iLink);	
}

/**
Assigns the data from buffer to pointer.

@param aBuffer The data to be assigned  
*/
void CIOBuffer::Assign(HBufC8* aBuffer)
{
	FreeData();
	iBuf = aBuffer;
	if(aBuffer)
		{
		TPtr8 temp=iBuf->Des();
		iBufPtr.Set(temp);
		}
}

/**
Frees the data.
*/
void CIOBuffer::FreeData()
{
	if(iBuf)
		{
		delete iBuf;
		iBuf = NULL;
		}
}

