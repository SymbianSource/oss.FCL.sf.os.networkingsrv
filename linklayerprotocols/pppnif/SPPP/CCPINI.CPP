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

#include "CCPINI.H"
#include "PPPCFG.H"

CPppIniData* CPppIniData::NewL()
	{
	CPppIniData* p=new(ELeave) CPppIniData;
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop();
	return p;
	}

CPppIniData::~CPppIniData()
	{
	}

CPppIniData::CPppIniData()
: iSearchPtr(NULL,0)
	{
	}

void CPppIniData::ConstructL()
	{
	CESockIniData::ConstructL(CCP_INI_DATA);
	iSearchPtr.Set(iPtr);
	}

TBool CPppIniData::ReadCompressorInfo(CPppCompConfig* aData, TUint aNumberOfTurns)
//
// Read the filename data into the config structure,
// 
//
	{
	TBool	retCode = EFalse;
	TBuf<3> section;
	//
	// Add Section Name
	//
	TPtrC dllName;
	section.Append(aNumberOfTurns+0x30);
	
	if ( FindVar(section, _L("dll") ,dllName))
		{
		TRAPD(ret,aData->AddNameL(dllName))
		if (ret == KErrNone)
			{
			TInt	temp;
			if ( FindVar(section, _L("id") ,temp))
				{
				aData->AddID((TUint8)temp);
				if ( FindVar(section, _L("optlen"),temp))
					{
					if (temp)
						{
						//
						// We have to sort out the options
						//
						TPtrC options;
						if ( FindVar(section, _L("opts"),options))
							{
							TRAPD(ret,aData->AddOptionsL(options))
							if (ret == KErrNone)
								{
								retCode = ETrue;
								}
							}
						}
					else
						{
						// Added because of predictor that has no "opts"
						TPtrC Options(_L("0"));
						// Ignore Error code, when '0 opts' are passed to 
						// AddOptionsL nothing is added to the config class.
						TRAP_IGNORE(aData->AddOptionsL(Options))
						retCode = ETrue;
						}
					}
				}
			}
		}

	return retCode;
	}
