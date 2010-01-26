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
// This test step feeds IP/TCP packets into both the VJCompressor 
// (unit under test) and the Reference Compressor (provided in RFC 1144).
// The output of the VJCompressor is compared against the output of 
// the Reference Compressor to verify its corectness
// 
//

/**
 @file VJCompressTest.cpp
*/

#include "VJTest.h"

/**
This is a virtual function.  
This virtual function feeds IP/TCP packets into both the VJCompressor 
(unit under test) and the Reference Compressor (provided in RFC 1144).
The output of the VJCompressor is compared against the output of 
the Reference Compressor to verify its corectness
*/

void CVJCompressTest::ProcessPacketL()
{	
	
	__UHEAP_MARK;
	
	// Check File type and trim packet accordingly
	
	TBool trimErr = TrimPacketBeforeCompression();
	if (trimErr)
	{
	    // File is of unsupported type. Abort test. 
	    SetTestStepResult(EFail);
	    return;
	}
	    
	   
	// Create packet buffer to be sent to reference compressor
	// This is to prevent compromising the test results if the compressor/decompressor alters the original data
	HBufC8* refPacketBuffer = iRec->Data().AllocLC();   
	     
    //-------------- substep 7 -------------------- 
	// compress using our VJCompressor
	INFO_PRINTF1(_L("  07 Compress the packet using VJCompressor:"));
    
    // Create packet buffer to be sent to VJcompressor
    // This is to prevent compromising the test results if the compressor/decompressor alters the original data
	HBufC8* vjPacketBuffer = iRec->Data().AllocLC();
	iPacket.CopyIn(*vjPacketBuffer, 0);  
	iPacket.TrimEnd(vjPacketBuffer->Length());
		 
	// Create info packet
 	RMBufPacket pkt;
    pkt.NewInfoL();
	pkt.Pack();
	
	// Put info packet in front of this packet 
	iPacket.Prepend(pkt);
	// DEBUG
	//INFO_PRINTF2(_L(" ***Size of info before compression: %d***"), (*(iPacket.First())).Length()); 
	// DEBUG
	//INFO_PRINTF2(_L(" ***Size of TCP/IP packet before compression (after padding info): %d***"), (*(iPacket.Last())).Length()); 
	TInt vjcompRet = iVJCompressor->VJCompressFrame(iPacket);
	     
	
	//-------------- substep 8 -------------------- 
	// Display the result of the VJ compression
	INFO_PRINTF1(_L("  08 Display VJ compressed packets:"));
	
	// TPtr8 object is created because CopyOut() takes a TPtr8 object as a parameter
	TPtr8 vjOutputPtr (vjPacketBuffer->Des());
	
	// Remove info packet 
	iPacket.Remove();
 	
	iPacket.CopyOut(vjOutputPtr);	//the compressed data should be smaller     
      
    // TPtrC object is needed because Dump function takes a TPtrC obj as parameter
    TPtrC8 vjOutputPtrC (vjOutputPtr);  
    
    // Print each field in the VJ compressed packet
    DumpCompressedPacket(vjOutputPtrC, vjcompRet);
    
       
    //-------------- substep 9 -------------------
    // compress using our reference Compressor
	INFO_PRINTF1(_L("  09 Compress the packet using reference compressor:"));
  
    struct mbuf mbufObj; 
    mbufObj.m_len = refPacketBuffer->Length();
    mbufObj.m_off = const_cast<TUint8 *>(refPacketBuffer->Ptr());
       
    struct mbuf* mbufObjPtr = &mbufObj;
       
    TUint refcompRet = sl_compress_tcp(mbufObjPtr, (struct ip *)(const_cast<TUint8*>(refPacketBuffer->Ptr())), iSlcomp);
       
       
    //-------------- substep 10 -------------------- 
	// display the result of the Reference compressor 
	INFO_PRINTF1(_L("  10 Display REF compressed packet:"));
	TPtrC8 refOutputPtrC ((TUint8 *)mbufObjPtr->m_off, mbufObjPtr->m_len); 
	    
	DumpCompressedPacket (refOutputPtrC, refcompRet);
	
	     
 	//--------------substep 11----------------------
 	//Compare the results of the reference compressor output and our VJ compressor output   
 	
 	INFO_PRINTF1(_L("  11 Compare the result of the compression by VJCompressor to Reference compressor:"));
 	INFO_PRINTF2(_L(" Size of VJ comp output: %d"), vjOutputPtrC.Length()); 
 	INFO_PRINTF2(_L(" Size of Reference comp output: %d"), refOutputPtrC.Length()); 
 	
 	if (!CompareOutput(vjOutputPtrC, refOutputPtrC))
 	{
 	    SetTestStepResult(EFail);
 	}
		
	CleanupStack::PopAndDestroy(vjPacketBuffer);
   	CleanupStack::PopAndDestroy(refPacketBuffer); 
   	vjPacketBuffer = NULL;
   	refPacketBuffer = NULL;  
   	
   	__UHEAP_MARKEND;
	
} // VJCompressTest
