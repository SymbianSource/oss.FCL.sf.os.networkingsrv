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

#include <es_ini.h>
#include <in_iface.h>
#include "PPPLRD.H"
#include "PPPLOG.H"
#include "PPPLCP.H"
#include "PPPBASE.H"

//
CPppLrd::CPppLrd(CPppLcp* aPppLcp):iPppLcp(aPppLcp)
	{}
	
CPppLrd::~CPppLrd()
	{
	TimerCancel();
	TimerDelete();
	}
	
CPppLrd* CPppLrd::NewL(CPppLcp* aPppLcp)
/**
Create a new object instance.

@return Pointer to new object
*/
	{
	CPppLrd* self=new (ELeave)CPppLrd(aPppLcp);
	CleanupStack::PushL(self);
	self->ConstructL();
    CleanupStack::Pop();
	return self;
	}
	
void CPppLrd::ConstructL()
/**
Initialize the object and start the timer.
*/
	{
	iShiftRegister = KShiftRegisterWidth;
	iBitCount = KSampleWidth;
	TimerConstructL(KPppLrdTimerPriority);
	// Initialise with default value
	iPppLrdEnabled	= KPppLrdDefaultEnabledMode; 
	iLrdPeriod		= KPppLrdDefaultTimerPeriod; 
	// Override default with values from INI file
	ReadIniFileL();
	StartTimer();
	}

void CPppLrd::ReadIniFileL()
/**
Read the LRD section of the ppp.ini file.
*/
	{
	CESockIniData* ini = NULL;
	TRAPD(res,
		if (iPppLcp->PppLinkMode() == CPppLcpConfig::EPppLinkIsServer)
			{
			ini = CESockIniData::NewL(PPP_SERVER_INI_FILE);
			}
		else
			{
			ini = CESockIniData::NewL(PPP_INI_FILE);
			}
		)
	if(res!=KErrNone)
		{
		if(res==KErrNotFound)
			return;
		User::Leave(res);
		}
	CleanupStack::PushL(ini);
	TInt	entry = 0;
	// Read enable switch
	if (ini->FindVar(LRDSECTIONNAME,LRDENTRYNAME_ENABLE, entry))
		{
		if (entry == 0) 
			iPppLrdEnabled = EFalse;
		else
			iPppLrdEnabled = ETrue;
		}
	// Read period value
	if (ini->FindVar(LRDSECTIONNAME,LRDENTRYNAME_PERIOD, entry))
		{
		if((entry >= KPppLrdTimerPeriodMin) && (entry <=KPppLrdTimerPeriodMax))
			iLrdPeriod = entry;
		}

	CleanupStack::PopAndDestroy();
	}

void CPppLrd::StartTimer()
/**
Start the timer.
*/
	{
	if(iPppLrdEnabled)
		{
		iLastEchoAck = ETrue;
		TimerAfter(iLrdPeriod*1000000);
		}
	}

void CPppLrd::StopTimer()
/**
Stop the timer.
*/
	{
	TimerCancel();
	}

void CPppLrd::RecvEchoReply(TUint8 aIdentifier)
/**
Upcall from LCP when an Echo Reply packet is received.

@param aIdentifier Packet identifier
*/
	{
	// Check this reply matches the last request
	iLastEchoAck = (iLastEchoAck || (iEchoReqFrameIdentifier == aIdentifier));
	LOG( iPppLcp->iLogger->Printf(_L("CPppLrd::RecvEchoReply "));)
	if(iLastEchoAck)
		{LOG( iPppLcp->iLogger->Printf(_L("TRUE\n"));)}
	else
		{LOG( iPppLcp->iLogger->Printf(_L("FALSE\n"));)}
	}
	
void CPppLrd::TimerComplete(TInt /*aStatus*/)
/**
Checks how many Echo Reply packets we've received and shuts down the link
if we've missed too many.  If we're still OK, send another Echo Request.
Called by MTimer on timer expiry.

@param aStatus Aynchronous request completion status (ignored)
*/
	{
	// Shift Register
	iShiftRegister <<=1;
	// Update Bit counter
	iBitCount -= (iShiftRegister & KBitOverflow ? 1:0);
	// Update Register with last answer
	iShiftRegister |=(iLastEchoAck?1:0);
	// Update Bit counter
	iBitCount += (iLastEchoAck?1:0);
	LOG( iPppLcp->iLogger->Printf(_L("CPppLrd::TimerComplete \n")); )
	//  Check the number of missed echo answers
	if(iBitCount == (KSampleWidth-KMaxNFailure))
		{
		// too many missed replies - we're dead
		// signal that to the PPP stack
		// with code below? - Check that 1st
		// Changed Error Code to reflect faulty transmission chain
		iPppLcp->Stop(KErrIfLRDBadLine,MNifIfNotify::EDisconnect);
		LOG( iPppLcp->iLogger->Printf(_L("Lrd detected a link FAILURE\nDropping the connection\n")); )
		return;
		}
	// Otherwise if we're opened carry on sending echo request
	if(iPppLcp->FsmIsThisLayerOpen())
		iEchoReqFrameIdentifier = iPppLcp->SendEchoRequest();
	else 
		iEchoReqFrameIdentifier = 0;
	if(iEchoReqFrameIdentifier)
		iLastEchoAck = EFalse;
	else
		// if for some reason sending the echo req failed
		// disable decrementing the counter
		// We still trigger the timer and retry later on
		iLastEchoAck = ETrue;
	TimerAfter(iLrdPeriod*1000000);
	}

void CPppLrd::EnableOrDisableTimer(TBool aEnable)
/**
Leave enabled or disable the echo check by starting or stopping the timer.
If the timer is stopped, it will not be restarted by this method.

@param aEnable EFalse to stop the timer
*/
	{
	if (!aEnable)
		{
		StopTimer();
		iPppLrdEnabled=EFalse;
		}
	else
		{
		iPppLrdEnabled=ETrue;	
		// TODO: do we need to reset anything here?
		}
	}

