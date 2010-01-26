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

#include "PPPCFG.H"

CPppCompConfig::CPppCompConfig()
	{
	}

CPppCompConfig::~CPppCompConfig()
	{
	delete iName;
	delete iOptions;
	}

TUint8 CPppCompConfig::ID()
	{
	return iID;
	}

TPtrC8 CPppCompConfig::Options()
	{
	return iOptions->Des();
	}

TInt CPppCompConfig::OptionsLength()
	{
	return iOptions->Length();
	}

void CPppCompConfig::AddOptionsL(TPtrC aOptions)
//
// Read the options field from the file into the Config Class
//
	{
	TInt Length;

	//
	// This Length is in characters, each byte is two characters
	//
	Length = aOptions.Length();
	Length /= 2;

	iOptions = HBufC8::NewMaxL(Length);

	TPtr8	Temp = iOptions->Des();


#ifdef UNICODE
	TUint16* Ptr;
	Ptr = CONST_CAST(TUint16*, (aOptions.Ptr()));
#else
	TUint8* Ptr;
	Ptr = CONST_CAST(TUint8*, (aOptions.Ptr()));
#endif

	TInt i;
	TInt Byte=0;
	TInt Byte1=0;
	Temp.Zero();
	for (i=0;i<Length;i++)
		{
		//
		// Read two characters and convert them into a byte
		//
		Byte = *(Ptr+(i*2));
		Byte &= 0xFF;
		Byte -= 0x30;
		Byte <<= 4;
		Byte1 = *(Ptr+(i*2)+1);
		Byte1 &= 0xFF;
		Byte1 -= 0x30;
		Byte |= Byte1;

		Temp.Append((TUint8)Byte);
		}
	}

void CPppCompConfig::AddNameL(TPtrC aName)
	{

	iName = HBufC::NewMaxL(aName.Length());
	TPtr temp = iName->Des();
	temp.Copy((aName.Ptr()), (aName.Length()));
	}

TPtrC CPppCompConfig::Name()
	{
	return *iName;
	}


CPppCompConfig* CPppCompConfig::NewL()
	{
	CPppCompConfig * pppComp = new (ELeave) CPppCompConfig();

	return pppComp;
	}

void CPppCompConfig::AddID(TUint8 aID)
	{
	iID = aID;
	}

