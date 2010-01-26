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
// This test step feeds precompressed IP/TCP packets into both 
// the VJDeCompressor (unit under test) and the Referece Decompressor 
// (provided in RFC 1144).
// The output of the VJDeCompressor is compared against the output of 
// the Reference Decompressor to verify its corectness
// 
//

/**
 @file VJDecompressTypeErrorTest.cpp
*/
 
#include "VJTest.h"

// Reference decompressor needs this much space before the compressed
// packet so it can store the TCP/IP header
static const TUint KRefPacketHeaderSpace = 128;

void CVJDecompressTypeErrorTest::ProcessPacketL()
/**
This is a virtual function. 
This function feeds precompressed IP/TCP packets into both 
the VJDeCompressor (unit under test) and the Referece Decompressor 
(provided in RFC 1144).
The output of the VJDeCompressor is compared against the output of 
the Reference Decompressor to verify its corectness 
*/

{	
		TUint packetType = TrimPacketBeforeDecompression();
		
		if (packetType == 911)
		{
		    // Packet type is unsupported. Abort test.
		    SetTestStepResult(EFail);
		    return;
		}
			
		// Create packet buffer to be sent to reference decompressor
		// This is to prevent compromising the test results if the compressor/decompressor alters the original data
	    HBufC8* refPacketBuffer = HBufC8::NewMaxLC(TPcapRecord::KMaxPacketSize+KRefPacketHeaderSpace);
	    // Reserve some space at the head of the HBufC8 for the uncompressed header
	    // The reference decompressor expects the packet has enough space to prepend 120 byes of resconstructed header 
	    TPtr8 refInputPtr(const_cast<TUint8*>(refPacketBuffer->Ptr())+KRefPacketHeaderSpace, refPacketBuffer->Length()-KRefPacketHeaderSpace);
	    refInputPtr = iRec->Data();
	    
	    TUint8 *refdecompOutput = NULL;
	    TInt refPacketLen = refInputPtr.Length();
	  
	    // Create packet buffer to be sent to VJDecompressor
	    // This is to prevent compromising the test results if the compressor/decompressor alters the original data
	    // The size of the HBuf object must have a maximum length that is big enough to hold the decompressed data
	    HBufC8* vjPacketBuffer = HBufC8::NewLC(TPcapRecord::KMaxPacketSize);
	    TPtr8 vjOutputPtr (vjPacketBuffer->Des());
	    vjOutputPtr = iRec->Data();
	    		  
	    iPacket.CopyIn(*vjPacketBuffer, 0); 
	    iPacket.TrimEnd(vjPacketBuffer->Length());
		 
	   
	    // Create info packet
 	    RMBufPacket pkt;
	    RMBufPktInfo* info = 0;
        info = pkt.NewInfoL();  
       	info->iLength = vjPacketBuffer->Length();
        pkt.Pack();
        iPacket.Prepend(pkt);
	    		
	    // Put info packet in front of this packet 
	    TUint vjdecompRet;
	    
	     //-------------- substep 7 -------------------- 
	    // decompress the packet using VJDeCompressor and Reference decompressor 
	    INFO_PRINTF1(_L("  07, 08 Decompress the packet using VJDeCompressor and Reference Decompressor:"));
	    
	    switch (packetType)
	    {
	        case KPppIdIp:
	        // the packet is TYPE_IP
	             // decompress TYPE_IP packet using our reference decompressor
	            INFO_PRINTF1(_L("  07 Decompress TYPE_IP packet using reference decompressor:"));
      			refdecompOutput = sl_uncompress_tcp(const_cast<TUint8*>(refInputPtr.Ptr()), &refPacketLen, (TUint)TYPE_IP, iComp);
	    
	            // VJ Decompressor function is not called because the packet is TYPE_IP
	            INFO_PRINTF1(_L("  08 TYPE_IP packet is not modified by VJDecompressor."));
	            vjdecompRet = ETrue;
	    		break;
  		    case KPppIdVjCompTcp:
	        // the packet is COMPRESSED_TCP
	            // decompress using our reference decompressor
	            INFO_PRINTF1(_L("  07 Decompress COMPRESSED_TCP packet using reference decompressor:"));
      			refdecompOutput = sl_uncompress_tcp(const_cast<TUint8*>(refInputPtr.Ptr()), &refPacketLen, (TUint)TYPE_COMPRESSED_TCP, iComp);
	        
                //decompress using our VJ Decompressor
	            INFO_PRINTF1(_L("  08 Decompress COMPRESSED_TCP packet using VJDeCompressor:"));
	            vjdecompRet = iVJDeCompressor->DecompVJComp(iPacket);
	            INFO_PRINTF2(_L(" 08 COMPRESSED_TCP packet is decompressed: %d"), vjdecompRet);
	   	  		break;
	        case KPppIdVjUncompTcp:
	        // the packet is UNCOMPRESSED_TCP
	            // decompress using our reference Compressor
	            INFO_PRINTF1(_L("  07 Decompress UNCOMPRESSED_TCP packet using reference decompressor:"));
      			refdecompOutput = sl_uncompress_tcp(const_cast<TUint8*>(refInputPtr.Ptr()), &refPacketLen, (TUint)TYPE_UNCOMPRESSED_TCP, iComp);
	        	
	        //decompress using our VJ Decompressor
	            INFO_PRINTF1(_L("  08 Decompress UNCOMPRESSED_TCP packet using VJDeCompressor:"));
	            vjdecompRet = iVJDeCompressor->DecompVJUncomp(iPacket);
	            INFO_PRINTF2(_L(" 08 UNCOMPRESSED_TCP packet is decompressed: %d"), vjdecompRet);
	        	break;
	        default: 
	        //the packet is TYPE_ERROR
	            // set 'toss' flag and discard COMPRESSED_TCP packet until one with C bit set
	            // or an UNCOMPRESSED_TCP packet arrives
	             // decompress using our reference decompressor
	            INFO_PRINTF1(_L("  07 Decompress TYPE_ERROR packet using reference decompressor:"));
      			refdecompOutput = sl_uncompress_tcp(const_cast<TUint8*>(refInputPtr.Ptr()), &refPacketLen, (TUint)TYPE_ERROR, iComp);
	     		//decompress using our VJ Decompressor
	            INFO_PRINTF1(_L("  08 Handle TYPE_ERROR packet using VJDeCompressor:"));
	            iVJDeCompressor->CRCError(); 
	            vjdecompRet = EFalse;
	    		break;
	    }    
	    
	     if ((vjdecompRet) && refdecompOutput != 0)
	     {
	         //--------------substep 09----------------------
 	         // Display VJ decompressed packet
	         INFO_PRINTF1(_L("  09 The VJ decompressed packet:"));
	     
	         // The decompressed data should be larger than the compressed data
	         // So we need to expand the size of the pointer in order to hold the decompressed data
	         vjOutputPtr.SetMax();
	         
	         // Remove info packet 
	         iPacket.Remove();
	         iPacket.CopyOut(vjOutputPtr);	
 			 
	         INFO_PRINTF2(_L("VJ decompressed packet len: %d"), vjPacketBuffer->Length()); 
	         // TPtrC8 object is created because DumpIp() takes a TPtrC8 object as a parameter
	         TPtrC8 vjOutputPtrC (vjOutputPtr);
	         DumpIp(vjOutputPtrC);
	     
	         //--------------substep 10----------------------
 	         // Display REF decompressed packet
	         INFO_PRINTF1(_L("10 The REF decompressed packet is:")); 
	         TPtrC8 refOutputPtrC(refdecompOutput, refPacketLen);
	         DumpIp(refOutputPtrC);
	         
 	         //--------------substep 11----------------------
 	         //Compare the results of the reference compressor output and our VJ compressor output
 	     
 	         INFO_PRINTF1(_L("  11 Compare the result of the compression by VJCompressor to Reference compressor:"));
 	         INFO_PRINTF2(_L(" Size of VJ comp output: %d"), vjOutputPtrC.Length()); 
 		     INFO_PRINTF2(_L(" Size of Reference comp output: %d"), refOutputPtrC.Length()); 
 	     
 	    	 if (!CompareOutput(vjOutputPtrC, refOutputPtrC))
 		     {
 	              INFO_PRINTF1(_L("The results are not equal"));
 	              
 	              SetTestStepResult(EFail);   
 	         }
 	     }
 	     else
 	     {
 	           IsItATossPacket(vjdecompRet, refdecompOutput);
 	     }
 	     
 	     
 	     CleanupStack::PopAndDestroy(vjPacketBuffer);
 	        
		 CleanupStack::PopAndDestroy(refPacketBuffer);
 	   
		 vjPacketBuffer = NULL;
		 refPacketBuffer = NULL;
	
} // VJDecompressTypeErrorTest
