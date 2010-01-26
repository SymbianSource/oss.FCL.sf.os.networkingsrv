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

#include "rasdef.h"

TInt Util::GetCommaSepToken(TLex& aLex, TPtrC& aPtr)
	{
	aPtr.Set(NULL,0);
	
	TChar ch;
	while(!aLex.Eos() && (ch = aLex.Peek(), ch==','))
		aLex.Inc();

	if(aLex.Eos())
		return KErrNone;

	aLex.Mark();		
	while (!aLex.Eos() && (ch = aLex.Peek(), ch!=','))
		aLex.Inc();
	aPtr.Set(aLex.MarkedToken());
	return KErrNone;
	}

TInt Util::GetCommHandshake(const TDesC& aDes, TUint& aHandshake)
	{
	TLex lex(aDes);	
	TPtrC des;
	TUint hs = 0;	
	for (;;)
		{
		TInt ret = GetCommaSepToken(lex, des);
		if (ret!=KErrNone)
			return ret;
		if (des.Length()>0)
			{
         _LIT(KXonXoff, "XON/XOFF");
         _LIT(KRtsCts,  "RTS/CTS");
         _LIT(KDtrDsr,  "DTR/DSR");
         _LIT(KDcd,  "DCD");

			if (!des.CompareF(KXonXoff))
				hs |= (KConfigObeyXoff | KConfigSendXoff);
			else if (!des.CompareF(KRtsCts))
				hs |= (KConfigObeyCTS);
			else if (!des.CompareF(KDtrDsr))
				hs |= (KConfigObeyDSR | KConfigFailDSR);
			else if (!des.CompareF(KDcd))
				hs |= (KConfigObeyDCD);
			else
				return KErrArgument;
			}
		else
			break;
		}
	aHandshake = hs;
	return KErrNone;
	}

TInt Util::GetCommFormat(const TDesC& aDes, TDataBits& aDataBits, TStopBits& aStopBits, TParity& aParity)
	{
	switch (aDes[0])
		{
	case '5':
		aDataBits = EData5;
		break;
	case '6':
		aDataBits = EData6;
		break;
	case '7':
		aDataBits = EData7;
		break;
	case '8':
		aDataBits = EData8;
		break;
	default:
		return KErrArgument;
		}
	switch (aDes[1])
		{
	case 'N':
		aParity = EParityNone;
		break;
	case 'E':
		aParity = EParityEven;
		break;
	case 'O':
		aParity = EParityOdd;
		break;
	case 'M':
		aParity = EParityMark;
		break;
	case 'S':
		aParity = EParitySpace;
		break;
	default:
		return KErrArgument;
		}
	switch (aDes[2])
		{
	case '1':
		aStopBits = EStop1;
		break;
	case '2':
		aStopBits = EStop2;
		break;
	default:
		return KErrArgument;
		}
	return KErrNone;
	}


TInt Util::GetCommSpeed(TInt aSpeed, TBps& aBps)
	{
	switch (aSpeed)
		{
	case 50:
		aBps = EBps50;
		break;
	case 75:
		aBps = EBps75;
		break;
	case 110:
		aBps = EBps110;
		break;
	case 134:
		aBps = EBps134;
		break;
	case 150:
		aBps = EBps150;
		break;
	case 300:
		aBps = EBps300;
		break;
	case 600:
		aBps = EBps600;
		break;
	case 1200:
		aBps = EBps1200;
		break;
	case 1800:
		aBps = EBps1800;
		break;
	case 2000:
		aBps = EBps2000;
		break;
	case 2400:
		aBps = EBps2400;
		break;
	case 3600:
		aBps = EBps3600;
		break;
	case 4800:
		aBps = EBps4800;
		break;
	case 7200:
		aBps = EBps7200;
		break;
	case 9600:
		aBps = EBps9600;
		break;
	case 19200:
		aBps = EBps19200;
		break;
	case 38400:
		aBps = EBps38400;
		break;
	case 57600:
		aBps = EBps57600;
		break;
	case 115200:
		aBps = EBps115200;
		break;
	default:
		return KErrArgument;
		}
	return KErrNone;
	}

#if defined (__WINS__)
_LIT(PDD_NAME,"ECDRV");
_LIT(LDD_NAME,"ECOMM");
#else
_LIT(PDD_NAME,"EUART1");
_LIT(PDD2_NAME,"EUART2");
_LIT(LDD_NAME,"ECOMM");
#endif

void Util::CommInit()
	{

	User::LoadPhysicalDevice(PDD_NAME);
#ifdef PDD2_NAME
	User::LoadPhysicalDevice(PDD2_NAME);
#endif
	User::LoadLogicalDevice(LDD_NAME);
 	// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 	// it needs a different CommDB
 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    StartC32WithCMISuppressions(KPhbkSyncCMI);
	}
