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
// the VJDeCompressor (unit under test) and the Referece Decompressor 
// (provided in RFC 1144).
// The output of the VJDeCompressor is compared against the output of 
// the Reference Decompressor to verify its corectness.
// This test step is helpful especially if NULL packets are expected from the 
// decompressors. 
// 
//

/**
 @file VJandRefDecompressTest.cpp
*/
#include "VJTest.h"


void CVJandRefDecompressTest::ProcessPacketL()
/** 
This is a virtual function.
This function calls CompressDecompressL() which feeds IP/TCP packets 
into Reference Compressor to produce compressed IP/TCP packets. 
The compressed packets are then fed into the VJDeCompressor (unit under test) 
and the Referece Decompressor (provided in RFC 1144).
The output of the VJDeCompressor is compared against the output of 
the Reference Decompressor to verify its corectness.
This test step is helpful especially if NULL packets are expected from the 
decompressors. 
*/

{	
	// Compare the VJ decompressed output with the original packet

	TBool compareVJandRef = ETrue;
	
	CompressDecompressL(compareVJandRef);
 		 

} // VJandRefDecompressTest

