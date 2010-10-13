// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// TftpEngine.CPP
// Started by AY, May 1997
// 
//

#include <tftpeng.h>

EXPORT_C CTftpEngine::~CTftpEngine()
    {
    Cancel();
    iSocket.Close();
    iSockServ.Close();
    iFs.Close();
    delete iTimer;
    }

CTftpEngine::CTftpEngine(const TInt aPriority, MTftpNotifier& aNotifier)
    : CActive(aPriority), iNotifier(aNotifier)
    {
    CActiveScheduler::Add(this);
    }

void CTftpEngine::ConstructL()
    {
	iTimer = CTftpTimer::NewL(10, *this); 
	iTimeOut = 30000000;
	iAddress.SetPort(5001);		//Arbitrary port
	User::LeaveIfError(iSockServ.Connect());
	User::LeaveIfError(iSocket.Open(iSockServ, KAfInet, KSockDatagram, KProtocolInetUdp));
	iSocket.Bind(iAddress);
	User::LeaveIfError(iFs.Connect());
    }

EXPORT_C void CTftpEngine::Get(const TDesC& aNetAddr, const TDesC& aName, const TDesC& aMode)
    {
        //send Get rqst. to given address, port 69 (TFTP standard)
	iAddress.SetPort(69);
	iAddress.Input(aNetAddr);
	if (iFile.Replace(iFs, aName, EFileWrite|EFileShareAny) != KErrNone)
		iNotifier.OpComplete(EFileOpenFailure, 0);
	else
        {
        iPacket.MakeRqst(ERRQ, aName, aMode);
        iSocket.SendTo(iPacket, iAddress, 0, iStatus);
        iState = ESending;
            //start timer for rqst.
        iTimer->After(iTimeOut);
        SetActive();
        }
    }

EXPORT_C void CTftpEngine::Put(const TDesC& aNetAddr, const TDesC& aName, const TDesC& aMode)
    {
    //send Put rqst. to given address, port 69 (TFTP standard)
	iAddress.SetPort(69);
	iAddress.Input(aNetAddr);
	if (iFile.Open(iFs, aName, EFileRead|EFileShareAny) != KErrNone)
		iNotifier.OpComplete(EFileOpenFailure, 0);
	else
		{
		iPacket.MakeRqst(EWRQ, aName, aMode);
		iSocket.SendTo(iPacket, iAddress, 0, iStatus);
		iState = ESending;
        //start timer for rqst.
		iTimer->After(iTimeOut);
		SetActive();
		}
    }

EXPORT_C void CTftpEngine::TimerExpired()
	{
	Cancel();
	iNotifier.OpComplete(ETimedOut, 0);
	}

EXPORT_C void CTftpEngine::SetTimeOut(const TInt aTimeOut)
	{
	iTimeOut = aTimeOut;	
	}	

void CTftpEngine::RunL()
    {
	//cancel Timer immediately to avoid unwanted timeouts
	iTimer->Cancel();

	if (iStatus!=KErrNone)
        //the send/recv completed with an error: Terminate
		{
		//NOTE:must check for other states too (e.g. ESendingLastData)
		if(iState==ESending) 
			iNotifier.OpComplete(ESendFail, 0);
		else if(iState==EReceiving) 
			iNotifier.OpComplete(ERecvFail, 0);
		else 
			iNotifier.OpComplete(EError, 0); //temp. cover for other states
		iState = EComplete;
		}

	else 
		{
		if(iState==ESendingLastAckn) 
            //transfer completed successfully: Notify
			{
			iState = EComplete;
			iNotifier.OpComplete(EOpComplete, 0);
			}

		else if(iState==ESending||iState==ESendingLastData)
            //send successful, collect reply
			{
			iSocket.RecvFrom(iPacket, iAddress, 0, iStatus);
			//is this the last packet to be received? 
			if(iState==ESendingLastData) iState = EReceivingLast;
			else iState = EReceiving;
			//start timer for timeout and set object active
			iTimer->After(iTimeOut);
			SetActive();
			}

		else if(iState==EReceiving||iState==EReceivingLast)
            //received packet, determine opcode and behave accordingly
			{
			switch(iPacket.OpCode())
				{
            case EDATA: //DATA packet
                if (!iPacket.CheckBlock()) break;
                else
                    //write data to file and make ackn packet
                    {
                    if(iFile.Write(iPacket.Mid(4))!=KErrNone)  //Error, bail out.
						{
						iFile.Close();
						iState = EComplete;
						iNotifier.OpComplete(EError, 0);
						break;
						}
                    iNotifier.ProgressNotification();
                    if (iPacket.Size()<516)
                        //this is the last packet  
                        {
                        iFile.Close();
                        iState = ESendingLastAckn;
                        }
                    else 
                        iState = ESending;
                    //now make the ackn. packet
                    iPacket.MakeAckn();
                    }
                break;

            case EACK: // ACKN. packet
                if (!iPacket.CheckBlock()) break;
                else if (iState == EReceiving)
                    {
                    iNotifier.ProgressNotification();
                    //more data to be sent
                    TBuf8<512> data;
                    if(iFile.Read(data)!=KErrNone)	//Error, bail out.
						{
						iFile.Close();
						iState = EComplete;
						iNotifier.OpComplete(EError, 0);
						break;
						}
                    if(iPacket.MakeData(data)<516) 
                        //this is the last data packet to be sent
                        {
                        iFile.Close();
                        iState=ESendingLastData;
                        }
                    else
                        iState=ESending;
                    }
                else
                    //this was the final ackn., no more data to be sent
                    {
                    iState = EComplete;
                    iNotifier.OpComplete(EOpComplete, 0);
                    }
                break;

            case EERR: //ERROR packet
                //ERROR: Terminate (implement later)
                iState = EComplete;
                iNotifier.OpComplete(EError, 0);
                break;

            default:
                //OpCode should never be anything other than 3, 4 
                //or 5 so something screwy happened: terminate
                iState = EComplete;
                iNotifier.OpComplete(EError, 0);
                break;
				}

			if(iState != EComplete)
				if (iState==EReceiving||iState==EReceivingLast)
                    //block number incorrect, repeat the receive from
					{
					iSocket.RecvFrom(iPacket, iAddress, 0, iStatus);
					iTimer->After(iTimeOut);
					SetActive();
					}
				else
                    //send the packet constructed in above SWITCH statement
					{
					iSocket.SendTo(iPacket, iAddress, 0, iStatus);
					iTimer->After(iTimeOut);
					SetActive();
					}
			}
		}	
	}

void CTftpEngine::DoCancel()
//
// Cancel what ever is happening
//
    {
	if (iState == ESending||iState == ESendingLastData||
		iState == ESendingLastAckn) 
		iSocket.CancelSend();
	else if (iState == EReceiving||iState == EReceivingLast)
		iSocket.CancelRecv();
	//set state to complete. may replace with error code later
	iState = EComplete;
	iFile.Close();
	}

EXPORT_C CTftpEngine* CTftpEngine::NewL(const TInt aPriority, MTftpNotifier& aNotifier)
//
// Create and connect client with leaving
//
    {
    CTftpEngine *p = new (ELeave) CTftpEngine(aPriority, aNotifier);
    CleanupStack::PushL(p);
    p->ConstructL();
    CleanupStack::Pop(p);
    return p;
    }

//**********************************************
//
//	The Methods for the class CTftpTimer:
//
//**********************************************

CTftpTimer::CTftpTimer(const TInt aPriority, CTftpEngine& aEngine)
    : CTimer(aPriority), iEngine(aEngine)
//
// C++ C'tor
//
    {
    CActiveScheduler::Add(this);
    }

CTftpTimer::~CTftpTimer()
//
// C++ D'tor
//
    {
	Cancel();
    }

void CTftpTimer::ConstructL()
//
// E32 C'tor
//
    {
	CTimer::ConstructL();
    }

void CTftpTimer::RunL()
//
// Run method where all the action happens
//
    {
	iEngine.TimerExpired();
	}

CTftpTimer* CTftpTimer::NewL(const TInt aPriority, CTftpEngine& aEngine)
//
// Create and connect client with leaving
//
    {
    CTftpTimer *p = new (ELeave) CTftpTimer(aPriority, aEngine);
    CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop(p);
    return p;
    }

//**********************************************
//
//	The Methods for the class TTftpPacket:
//
//**********************************************

TTftpPacket::TTftpPacket()
    : TPtr8(iBuffer,2,516),iBlockNum(0)
    {
    }

TBool TTftpPacket::CheckBlock()
	{
	//if the block numbers do not match then return false
	if (BigEndian::Get16(&iBuffer[2])!=iBlockNum) 
		return EFalse;
	//otherwise, increment block num. for next packet and return true
	iBlockNum++;
	return ETrue;
	}

void TTftpPacket::MakeRqst(const TOpCode aOpc, const TDesC& aName, const TDesC& aMode)
	{
	TChar null = 0;
	//initialize block no. for this transfer
	if (aOpc == ERRQ) 
		iBlockNum = 1;
	else 
		iBlockNum = 0;
	//make the request packet
	SetLength(2);
	BigEndian::Put16(iBuffer, (TUint16)aOpc);
	Append(aName);
	Append(null);
	Append(aMode);
	Append(null);
	}

TInt TTftpPacket::OpCode()
	{
	//return OpCode from packet
	return (TInt)BigEndian::Get16(iBuffer);
	}

void TTftpPacket::MakeAckn()
	{
	//make an ackn. packet
	SetLength(4);
	BigEndian::Put16(iBuffer, (TUint16)4);
	}

TInt TTftpPacket::MakeData(const TDesC8& aData)
	{
	//make a data packet using data supplied
	SetLength(4);
	BigEndian::Put16(iBuffer, (TUint16)3);
	BigEndian::Put16(&iBuffer[2], (TUint16)iBlockNum);
	Append(aData);
	return Size();	
	}

void TTftpPacket::MakeError()
	{
	//not implemented 
	}

