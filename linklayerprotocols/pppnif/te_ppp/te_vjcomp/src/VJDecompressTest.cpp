// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This test step feeds IP/TCP packets into Reference Compressor to 
// produce compressed IP/TCP packets. The compressed packets are then fed into 
// the VJDeCompressor (unit under test).
// The output of the VJDeCompressor is compared against the original packet 
// before compression to verify its corectness.
// 
//

/**
 @file VJDecompressTest.cpp
*/

#include "VJTest.h"

// creates a connection and destroys it again
void CVJDecompressTest::ProcessPacketL()
/**
This is a virtual function that calls CompressDecompressL() which 
feeds IP/TCP packets into Reference Compressor to produce compressed 
IP/TCP packets. The compressed packets are then fed into 
the VJDeCompressor (unit under test).
The output of the VJDeCompressor is compared against the original packet 
before compression to verify its corectness.
*/
 
{	
    // Compare VJ and orig packet instead
    TBool compareVJandRef = EFalse;
    
	CompressDecompressL(compareVJandRef);
	
} // VJDecompressTest

