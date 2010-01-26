// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This contains TE_VJCompStep class which is
// the base class for all the PPPComp test steps.
// 
//

/**
 @file TE_VJCompStepBase.cpp
*/
#include "TE_VJCompStepBase.h"
#include <test/testexecutelog.h>

extern "C" {
	#include <stdlib.h>
	}

#include "NCPIP.H"
#include <ss_pman.h>
//#include "pppdef.h"
// Constructor
CTE_VJCompStepBase::CTE_VJCompStepBase():iPppNcpIp(), iVJDeCompressor(NULL),
		iVJCompressor(NULL), iVJCompressorCon(NULL), iVJDeCompressorCon (NULL),
		iFs(), iFilepcap(NULL), iRec(NULL), iPacket(NULL), iDebug(EFalse),
		iSlcomp(NULL), iComp(NULL)
{
}

// Destructor
CTE_VJCompStepBase::~CTE_VJCompStepBase()
	{
	//
	// OK Spin around removing all the Compressor Config information`
	//

    UnloadVJCompressor();

	UnloadVJDeCompressor();

	//
	// Delete all the Containers
	//
	if(iVJCompressorCon)
		{
			for(TInt i=0 ; i<iVJCompressorCon->Count() ; ++i)
				CNifFactory::ControlledDelete((*iVJCompressorCon)[i]);
			iVJCompressorCon = NULL;
		}


	if(iVJDeCompressorCon)
		{
			for(TInt i=0 ; i<iVJDeCompressorCon->Count() ; ++i)
				CNifFactory::ControlledDelete((*iVJDeCompressorCon)[i]);
			iVJDeCompressorCon = NULL;
		}

	if (iPppNcpIp)
	{
		delete iPppNcpIp;
		iPppNcpIp = NULL;
	}

	delete iFilepcap;
}

TVerdict CTE_VJCompStepBase::doTestStepPreambleL()
{
	INFO_PRINTF1(_L("Test Step Preamble"));

	iPppNcpIp = new (ELeave) CPppNcpIp(NULL);
	CleanupStack::PushL(iPppNcpIp);
	iPppNcpIp->ConstructL();
	CleanupStack::Pop();
	iFilepcap = CFilePcap::NewLC();
	CleanupStack::Pop();

	SetTestStepResult(EPass);
	return TestStepResult();
}

TVerdict CTE_VJCompStepBase::doTestStepL()
{


    if (TestStepResult() != EPass)
	{
	    return TestStepResult();
	}

	// Construct and install the active scheduler
	CActiveScheduler* stepSched=NULL;
	stepSched=new(ELeave) CActiveScheduler;

	// Push onto the cleanup stack
	CleanupStack::PushL(stepSched);

	//Install as the active scheduler
	CActiveScheduler::Install(stepSched);

	// open the binary file, instanciate the compressor and feed the compressor with the binary and look at the output.
	TInt err;

	//-------------- substep 1 --------------------
	INFO_PRINTF1(_L("  01 connect to file server:"));
	// connect to file system and read prev, source and dest file names and open them
	err = iFs.Connect();
	if (err != KErrNone)
	{
		INFO_PRINTF2(_L("Could not connect to file server (error %d)."), err);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(stepSched);
		return TestStepResult();
	}

	//-------------- substep 2 --------------------
	// open libpcap file
	INFO_PRINTF1(_L("  02 read file name from ini file"));

	TPtrC ptrPcapFileName;
	err = GetStringFromConfig(ConfigSection(), _L("pcapfile"), ptrPcapFileName);
	if (!err)
		{
		INFO_PRINTF2(_L("Could not find pcap file name in ini file(error %d)."), err);
		iFs.Close();
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(stepSched);
		return TestStepResult();
		}

	INFO_PRINTF2(_L("  02 Open pcap file: %S"), &ptrPcapFileName);

	err = iFilepcap->Open(ptrPcapFileName);
	if (err != KErrNone)
	{
	    INFO_PRINTF2(_L("Could not open pcap file (error %d)."), err);
		SetTestStepResult(EFail);
		iFs.Close();
		CleanupStack::PopAndDestroy(stepSched);
		return TestStepResult();
	}

	// read max slots from ini file
	INFO_PRINTF1(_L("  02 read max slots from ini file"));

	TInt aMaxSlots;

	err = GetIntFromConfig(ConfigSection(), _L("max_slots"), aMaxSlots);
	if (!err)
		{
		INFO_PRINTF1(_L("Could not find max slots in ini file.Use default value 15 instead."));
		aMaxSlots = 15;
		}

	// Abort test if the specified max slots value is invalid
	if (aMaxSlots < 2 || aMaxSlots > 255)
	{
	    INFO_PRINTF2(_L("The specified max slot value %d is invalid. Max slots must be between 2 and 255."), aMaxSlots);
	   	SetTestStepResult(EFail);
		iFs.Close();
		CleanupStack::PopAndDestroy(stepSched);
		return TestStepResult();
	}

	INFO_PRINTF2(_L("Max number of slots is %d"), aMaxSlots );

	// read debug mode from ini file
	INFO_PRINTF1(_L("  02 read debug mode from ini file"));

    err = GetBoolFromConfig(ConfigSection(), _L("debug"), iDebug);
	if (!err)
		{
		INFO_PRINTF1(_L("Could not find debug mode in ini file. Use default value"));
		}

	if (iDebug)
	{
	    INFO_PRINTF1(_L("Debug mode is ON"));
	}
	else
	{
	    INFO_PRINTF1(_L("Debug mode is OFF"));
	}

    //-------------- substep 3 --------------------
	INFO_PRINTF1(_L("  03 Load VJCompressor:"));
	//load VJ compressor
	__ASSERT_ALWAYS(iPppNcpIp,User::Panic(_L("TE_VJCompStep::DummyCPppNcpIp not instantiated"),KErrGeneral));


	TBool aCompressConnId = EFalse;
	iVJCompressor = iPppNcpIp->LoadVJCompressorL(aMaxSlots, aCompressConnId);


	INFO_PRINTF1(_L("  03 Load VJDecompressor:"));
	iVJDeCompressor = iPppNcpIp->LoadVJDeCompressorL(aMaxSlots);
	INFO_PRINTF1(_L("  03 LoadVJDeCompressorL: finished"));
	__UHEAP_MARK;

	// initialize the state machines in reference implementation
	INFO_PRINTF1 (_L(" 03 Initialize Reference Compressor state machine: "));
	iSlcomp = (struct slcompress *) new struct slcompress;
	sl_compress_init(iSlcomp);

	INFO_PRINTF1 (_L(" 03 Initialize Reference DeCompressor state machine: "));
	iComp = (struct slcompress *) new struct slcompress;
	sl_compress_init(iComp);

	__CFLOG_CREATEL;
	__UHEAP_MARK;

	StartMBufL();

	//-------------- substep 4 --------------------
	INFO_PRINTF1(_L("  04 read records in libpcap file"));
	iRec = new (ELeave) TPcapRecord;

	TUint count = 0;


	for(;;)
	{
	    INFO_PRINTF1(_L("  Allocate KMaxPacketSize-bytes long RMBufChain:"));

		iPacket.Free();

		TRAPD(allocErr,iPacket.AllocL(TPcapRecord::KMaxPacketSize));
		if (allocErr!= KErrNone)
		{
			INFO_PRINTF1(_L("Error: Couldn't allocate RMBuf:"));
			iPacket.Free();
#ifdef SYMBIAN_ZERO_COPY_NETWORKING			
			iCommsBufPond.Close();
#else
			delete iMBMngr; 
#endif // SYMBIAN_ZERO_COPY_NETWORKING			
			UnloadVJCompressor();
			UnloadVJDeCompressor();
			iFs.Close();
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(stepSched);
			return TestStepResult();
		}

	    err = iFilepcap->ReadRecord(*iRec);
	    if (err != KErrNone)
	    {
	        INFO_PRINTF2(_L("Could not read record (error %d)."), err);
			iPacket.Free();
#ifdef SYMBIAN_ZERO_COPY_NETWORKING			
			iCommsBufPond.Close();
#else
			delete iMBMngr; 
#endif // SYMBIAN_ZERO_COPY_NETWORKING			
			UnloadVJCompressor();
			UnloadVJDeCompressor();
			iFs.Close();
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(stepSched);
	        return TestStepResult();
	    }

	    if (iRec->Length() == 0)
	    {
	        INFO_PRINTF1(_L("Reach the end of libpcap file. No more packets to be read."));
	        break;
	    }

	    INFO_PRINTF2(_L("#################### PACKET %d ####################"), ++count);
	    INFO_PRINTF2(_L("  05 record length is : %d "), iRec->Data().Length());

	    ProcessPacketL();
	}



	//-------------- substep 12 --------------------
	INFO_PRINTF1(_L("  12 Free chain. Clean up stack:"));
	iPacket.Free();
	INFO_PRINTF1(_L("After freeing aPacket"));

	delete iRec;
    delete iComp;
    delete iSlcomp;
#ifdef SYMBIAN_ZERO_COPY_NETWORKING			
			iCommsBufPond.Close();
#else
			delete iMBMngr; 
#endif // SYMBIAN_ZERO_COPY_NETWORKING			

    __CFLOG_DELETE;
	
	__UHEAP_MARKEND;

	UnloadVJDeCompressor();

	INFO_PRINTF1(_L("After UnloadVJDeCompressor"));

	UnloadVJCompressor();
	INFO_PRINTF1(_L("After UnloadVJCompressor"));


	iFs.Close();
	INFO_PRINTF1(_L("after iFs.Close"));

	CleanupStack::PopAndDestroy(stepSched);
	INFO_PRINTF1(_L(" After popanddestroy stepSched"));

	return TestStepResult();

}

TBool CTE_VJCompStepBase::TrimPacketBeforeCompression()
{
    TBool err = EFalse;
    TUint packetType;
    TUint firstByte;
    TUint secondByte;

    // Check if the file type and trim packet accordingly
	switch (iFilepcap->LinkType())
	{
		case EPcapLinkTypePpp:
		// Type 9 file is received, need to strip Protocol ID and CRC data

		    firstByte = iRec->Data().Ptr()[0];
		    secondByte = iRec->Data().Ptr()[1];

	        if ((firstByte & 0x01) == 0)
	        {
	            // if the lowest bit is 0, the protocol id occupies 2 bytes
	            // trim out the protocol id (first 2 bytes) and CRC (final 2 bytes)
	            packetType = (firstByte << 8) | secondByte;
	            iRec->SetData (iRec->Data().Mid(2, iRec->Data().Length() - 4));
	        }
	        else
	        {
	            // the protocol id occupies 1 byte
	            // trim out the protocol id (the first byte) and CRC (final 2 bytes)
	            packetType = firstByte;
	            iRec->SetData (iRec->Data().Mid(1, iRec->Data().Length() - 3 ));
	        }

	        if (packetType != KPppIdIp)
	            err = EFalse;
	        break;


	     case EPcapLinkTypeIp:
	     // Type 12 file is accepted. No trimming is needed
	         break;
	     default:
	     // All other file types including EPcapLinkTypeEthernet is not acceted, abort test
	         INFO_PRINTF2(_L("File type %d is unsupported. Abort test"), iFilepcap->LinkType());
	         err = ETrue;
	         break;
	 }

	if (err == EFalse)
	{
	    if (iDebug)
	    {
	       // Dump original packet
	       INFO_PRINTF1(_L("    06 Original packet before compression:"));
 	       TPtrC8 origPtrC (iRec->Data());
	       DumpIp(origPtrC);
           INFO_PRINTF2(_L(" ===Size of original packet before compression: %d==="), iRec->Data().Length());
        }
     }

     return err;
}

TUint CTE_VJCompStepBase::TrimPacketBeforeDecompression()
{
	TUint packetType;
	// Check if the file type and trim packet accordingly
	if (iFilepcap->LinkType() != EPcapLinkTypePpp)
	{
	  // File is EPcapLinkTypeIp (Type 12), EPcapLinkTypeEthernet (Type 1) or other types

	  INFO_PRINTF2(_L("File type %d is unsupported"), iFilepcap->LinkType());
	  packetType = 911;	// assume error packet
	  return packetType;
	}

    // Type_9 PPP file is received. Need to strip the Protocol ID and CRC data
    TUint8 firstByte = iRec->Data().Ptr()[0];
	TUint8 secondByte = iRec->Data().Ptr()[1];

	if ((firstByte & 0x01) == 0)
	{
	    // if the lowest bit is 0, the protocol id occupies 2 bytes
	    packetType = (firstByte << 8) | secondByte;
	    // trim out the protocol id (first 2 bytes) and CRC (final 2 bytes)
	    iRec->SetData (iRec->Data().Mid(2, iRec->Data().Length() - 4));
	}
	else
	{
	    // the protocol id occupies 1 byte
	    packetType = firstByte;
	    // trim out the protocol id (the first byte) and CRC (final 2 bytes)
	    iRec->SetData (iRec->Data().Mid(1, iRec->Data().Length() - 3));
	}

	// Dump original packet
	INFO_PRINTF1(_L("  06 Compressed packet before decompression:"));
	TPtrC8 origPtrC (iRec->Data());
	DumpVjCompTcp(origPtrC);

	return packetType;
}

TBool CTE_VJCompStepBase::CompareOutput(TPtrC8 aPtr1, TPtrC8 aPtr2)
{
    TInt result = aPtr1.Compare(aPtr2);

    if (result != 0)
    {
       INFO_PRINTF1(_L(" !!!!!!!!!!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
       INFO_PRINTF2(_L(" Results are not equal: %d"), result);
       return EFalse;
    }
    else
    {
       INFO_PRINTF2(_L(" Results are equal: %d"), result);
       return ETrue;
    }


}

TInt CTE_VJCompStepBase::MapToVjCompRet (TUint refcompRet)
{
    TInt mappedCompRet;

    switch (refcompRet)
    {
        case TYPE_COMPRESSED_TCP:
	        mappedCompRet = KPppIdVjCompTcp;
	    	break;
	    case TYPE_UNCOMPRESSED_TCP:
	        mappedCompRet = KPppIdVjUncompTcp;
	    	break;
	    case TYPE_IP:
	    	mappedCompRet = KPppIdIp;
        	break;
        default:
        	mappedCompRet = refcompRet;
        	break;
    }

    return mappedCompRet;
}


void CTE_VJCompStepBase::DumpCompressedPacket(TPtrC8 outputPtrC, TInt compRet)
{
    // Do not display if not in debug mode
	if (!iDebug)
        return;

    switch(compRet)
	{
	    // Compressed packets returned by Reference Compressor
	    case TYPE_COMPRESSED_TCP:
	        INFO_PRINTF2(_L(" 09 Ref Compressor produced returned packet type: %x TYPE_COMPRESSED_TCP"), compRet);
	    	DumpVjCompTcp(outputPtrC);
	    	break;
	    case TYPE_UNCOMPRESSED_TCP:
	        INFO_PRINTF2(_L(" 09 Ref Compressor produced returned packet type: %x TYPE_UNCOMPRESSED_TCP"), compRet);
	    	DumpVjUncompTcp(outputPtrC);
	    	break;
	    case TYPE_IP:
	    	INFO_PRINTF2(_L(" 09 Ref Compressor produced returned packet type: %x TYPE_IP"), compRet);
        	DumpIp(outputPtrC);
        	break;
        case TYPE_ERROR:
        	INFO_PRINTF2(_L(" 09 Ref Compressor produced returned packet type: %x TYPE_ERROR"), compRet);
        	break;

	    // Compressed packets returned by VJ Compressor
	    case KPppIdVjCompTcp:
	        INFO_PRINTF2(_L(" 07 VJCompressor produced returned packet type: %x TYPE_COMPRESSED_TCP"), compRet);
	    	DumpVjCompTcp(outputPtrC);
	    	break;
	    case KPppIdVjUncompTcp:
	        INFO_PRINTF2(_L(" 07 VJCompressor produced returned packet type: %x TYPE_UNCOMPRESSED_TCP"), compRet);
	    	DumpVjUncompTcp(outputPtrC);
	    	break;
	    case KPppIdIp:
	    	INFO_PRINTF2(_L(" 07 VJCompressor produced returned packet type: %x TYPE_IP"), compRet);
	    	DumpIp(outputPtrC);
	    	break;
	    default:
	    	INFO_PRINTF2(_L(" 07 VJCompressor produced returned packet type: %x TYPE_ERROR"), compRet);
	    	break;
	}

}

static const TInt KMBuf_MaxAvail = 393216;
static const TInt KMBuf_MBufSize = 128;
static const TInt KMBuf_MBufSizeBig = 1600;
static const TInt KMBuf_InitialAllocation = 128;
static const TInt KMBuf_MinGrowth = 64;
static const TInt KMBuf_GrowthThreshold = 40;

void CTE_VJCompStepBase::StartMBufL()
/**
Initialize the MBuf Manager
*/
{
#ifdef SYMBIAN_ZERO_COPY_NETWORKING
    // Initialize the comms buf pond
    RArray<TCommsBufPoolCreateInfo> createInfos;
    TCommsBufPoolCreateInfo createInfo;
    createInfo.iBufSize = KMBuf_MBufSize;
    createInfo.iInitialBufs = KMBuf_InitialAllocation;
    createInfo.iGrowByBufs = KMBuf_MinGrowth;
    createInfo.iMinFreeBufs = KMBuf_GrowthThreshold;
    createInfo.iCeiling = (KMBuf_MaxAvail/2) / KMBuf_MBufSize;
    createInfos.AppendL(createInfo);
    TCommsBufPoolCreateInfo createInfo2;
    createInfo2.iBufSize = KMBuf_MBufSizeBig;
    createInfo2.iInitialBufs = KMBuf_InitialAllocation;
    createInfo2.iGrowByBufs = KMBuf_MinGrowth;
    createInfo2.iMinFreeBufs = KMBuf_GrowthThreshold;
    createInfo2.iCeiling = (KMBuf_MaxAvail/2) / KMBuf_MBufSize;     
    createInfos.AppendL(createInfo2);
	User::LeaveIfError(iCommsBufPond.Open(createInfos));   
	createInfos.Close();
#else
	// Initialize the MBuf manager
    MMBufSizeAllocator *mBufSizeAllocator;
	iMBMngr = CMBufManager::NewL(KMBuf_MaxAvail, mBufSizeAllocator);

	if (iMBMngr && mBufSizeAllocator)
        {
		// configure the mbuf size allocation info
	    mBufSizeAllocator->AddL(KMBuf_MBufSize,    KMBuf_InitialAllocation, KMBuf_MinGrowth, KMBuf_GrowthThreshold);
	    mBufSizeAllocator->AddL(KMBuf_MBufSizeBig, KMBuf_InitialAllocation, KMBuf_MinGrowth, KMBuf_GrowthThreshold);
        }

#endif // SYMBIAN_ZERO_COPY_NETWORKING	
 }

void CTE_VJCompStepBase::UnloadVJCompressor(void)
/**
To unload the VJ Compressor
*/
{
	if (iVJCompressor)
		{
		iVJCompressor->Close();
		iVJCompressor = NULL;
		}

}

void CTE_VJCompStepBase::UnloadVJDeCompressor(void)
/**
To unload the VJ Decompressor
*/
{
	if (iVJDeCompressor)
		{
		iVJDeCompressor->Close();
		iVJDeCompressor = NULL;
		}
}

const TText* CTE_VJCompStepBase::IpProtocolToText(TUint aValue)
/**
To change the IP Protocol type from an TUint value into text

@param aValue integer value of the IP protocol type
*/
	{

	// IP Protocols
	switch (aValue)
		{
	case 1:
		return _S("ICMP");
	case 58:
		return _S("ICMPv6");
	case 17:
		return _S("UDP");
	case 6:
		return _S("TCP");

	default:
		return _S("Unknown");
		}
	}

TUint16 CTE_VJCompStepBase::DecodeDelta(TUint8*& aPtr)
/**
To get the unsigned delta value of a compressed field

@param aPtr a pointer to the compressed field
*/

	{

	TUint16 wordValue;
	TUint8	byteValue = *aPtr++;
	if (byteValue != 0)
		return (TUint16)byteValue;
	else
		{
		/*
		*	Zero is an extender
		*/
		wordValue = BigEndian::Get16(aPtr);
		aPtr += 2;
		return wordValue;
		}
	}

TInt16 CTE_VJCompStepBase::DecodeSignedDelta(TUint8*& aPtr)
/**
To get the signed delta value of a compressed field

@param aPtr a pointer to the compressed field
*/
	{

	TInt16 wordValue;
	TUint8 byteValue = *aPtr++;
	if (byteValue != 0)
		return (TInt16)byteValue;
	else
		{
		/*
		*	Zero is an extender
		*/
		wordValue = BigEndian::Get16(aPtr);
		aPtr += 2;
		return wordValue;
		}
	}

TInt CTE_VJCompStepBase::DumpVjUncompTcp(TPtrC8 aDes)

/**
Dump a Van Jacobson uncompressed TCP/IP packet.

@param aDes pointer to the VJ uncompressed header
*/
{
	INFO_PRINTF1(_L("~~~~~~~~~~ DUMP VJ UNCOMPRESSED TCP/IP PACKET ~~~~~~~~~~"));

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 c = *ptr++;
//	TUint ver = c >> 4;
	TUint hlen = (c & 0xf) << 2;
    TUint8 tos = *ptr++;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 id = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 frag = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TBool zf = (frag & 0x8000);
	TBool df = (frag & 0x4000);
	TBool mf = (frag & 0x2000);
	frag = (TUint16)((frag & 0x1fff)<<3);
    TUint8 ttl = *ptr++;
	TUint8 proto = *ptr++;
	TUint16 chksum = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint32 srca = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 dsta = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
//	TBool opts = (hlen>20);

	Logger().WriteFormat(KHdrLengthString, len, hlen);
	Logger().WriteFormat(KSrcDstAddrString, (srca&0xff000000)>>24,(srca&0x00ff0000)>>16,(srca&0x0000ff00)>>8,(srca&0x000000ff),
							  (dsta&0xff000000)>>24,(dsta&0x00ff0000)>>16,(dsta&0x0000ff00)>>8,(dsta&0x000000ff));
	Logger().WriteFormat(KIDFragmentString, id, frag, df?_S("<DF>"):_S(""), mf?_S("<MF>"):_S(""), zf?_S("<Z>"):_S(""));
	Logger().WriteFormat(KTOSTTLChksumString, tos, ttl, chksum);
	Logger().WriteFormat(KConnectionNoString, proto);

	if (hlen>20)
		ptr += (hlen-20);

	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;
	aDes.Set(ptr, tlen);
	return n+DumpTcp(aDes, srca, dsta, tlen);
	}


TInt CTE_VJCompStepBase::DumpVjCompTcp(TPtrC8 aDes)
/**
Dump a Van Jacobson compressed TCP/IP packet.

@param aDes pointer to the VJ compressed header
*/
	{

	INFO_PRINTF1(_L("~~~~~~~~~~ DUMP VJ COMPRESSED TCP/IP PACKET ~~~~~~~~~~"));

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint16	change = *ptr++;
	TUint8	urgent=0;
	TInt16	window=0;
	TUint16	ack=0;
	TUint16	sequence=0;
	TBuf<50> changeMaskBuf;
	TUint8	connection=0;

	changeMaskBuf.Append(KChangeMaskString);

	if (change & KVjCompMaskConn)
		{
		_LIT(string7,"C");
		changeMaskBuf.Append(string7);
		connection = *ptr++;
		}

	TUint16 checksum = BigEndian::Get16(ptr);
	ptr += 2;

	if (change & KVjCompMaskPush)
		{
		_LIT(string8,"P");
		changeMaskBuf.Append(string8);
		}


	//	Don't reorder SpecialI && SpecialD, they are like this
	//	as SpecialD is the SWAU bits and SpecialI is SWU



	if ((change & KVjCompMaskSpecials) == KVjCompMaskSpecialD)
		{
		_LIT(string1,"	Special D");
		Logger().Write(string1);
		}
	else if ((change & KVjCompMaskSpecials) == KVjCompMaskSpecialI)
		{
		_LIT(string2,"	Special I");
		Logger().Write(string2);
		}
	else
		{
		if (change & KVjCompMaskUrgent)
			{
			_LIT(string3,"U");
			changeMaskBuf.Append(string3);
			urgent = *ptr++;
			}

		if (change & KVjCompMaskWindow )
			{
			window = (TInt16)DecodeSignedDelta(ptr);
			_LIT(string4,"W");
			changeMaskBuf.Append(string4);
			}

		if (change & KVjCompMaskAck )
			{
			ack = DecodeDelta(ptr);
			_LIT(string5,"A");
			changeMaskBuf.Append(string5);
			}

		if (change & KVjCompMaskSeq)
			{
			sequence = DecodeDelta(ptr);
			_LIT(string6,"S");
			changeMaskBuf.Append(string6);
			}
		}



    // the displayed value is the IP ID delta sent in the compressed packet
    // if a IP ID delta value is not sent in the compressed packet,
    // this means the IP ID will increase by 1 by default,
    //but it'll not be displayed in this dump function
	TUint16	ipId=0;
	if (change & KVjCompMaskIp)
		{
		ipId = DecodeDelta(ptr);
		_LIT(string9,"I");
		changeMaskBuf.Append(string9);
		}

	Logger().WriteFormat(TRefByValue<const TDesC>(changeMaskBuf), change);

	Logger().WriteFormat(KChecksumString, checksum);

	if (change & KVjCompMaskConn)
		{
		Logger().WriteFormat(KConnectionString,connection);
		}

	if	(urgent)
		{
		_LIT(string10,"	Urgent Delta = 0x%x");
		Logger().WriteFormat(string10,urgent);
		}

	if (window)
		{
		_LIT(string11,"	Window Delta = %d");
		Logger().WriteFormat(string11,window);
		}

	if (ack)
		{
		_LIT(string12,"	Ack Delta = 0x%x");
		Logger().WriteFormat(string12,ack);
		}

	if (sequence)
		{
		_LIT(string13,"	Sequence Delta = 0x%x");
		Logger().WriteFormat(string13,sequence);
		}

	if (ipId)
		{
		_LIT(string14,"	IPId = 0x%x");
		Logger().WriteFormat(string14,ipId);
		}

	return 1;
	}


TInt CTE_VJCompStepBase::DumpTcp(TPtrC8 aDes, TUint32 aSrcA, TUint32 aDstA, TInt aLength)

/**
Dump a TCP header.

@param aDes pointer to the TCP header
@param aSrcA source address in associated IP header
@param aDstA destination address in associated IP header
@param aLength length of the TCP header
*/
	{

	INFO_PRINTF1(_L("~~~~~~~~~~ DUMP TCP PACKET ~~~~~~~~~~"));

	TInt n = Min(aLength, aDes.Length());
	TInt len = n;

	TUint8* ptr = (TUint8*)aDes.Ptr();
	TUint8 osum0 = ptr[16];
	ptr[16] = 0;
	TUint8 osum1 = ptr[17];
	ptr[17] = 0;

	TUint32 sum = 0;
	sum += (aSrcA >> 16);
	sum += (aSrcA & 0xffff);
	sum += (aDstA >> 16);
	sum += (aDstA & 0xffff);
	sum += 6;
	sum += n;
	while (n>1)
		{
		sum += (ptr[0]<<8);
		sum += (ptr[1]);
		ptr += 2;
		n -= 2;
		}
	if (n>0)
		sum += (ptr[0]<<8);
	while (sum>0xffff)
		{
		sum = (sum & 0xffff) + (sum>>16);
		}
	sum = ~sum & 0xffff;
	ptr = (TUint8*)aDes.Ptr();
	ptr[16] = osum0;
	ptr[17] = osum1;

	TUint srcp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint dstp = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr +=2;
	TUint32 seqnum = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 acknum = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;

    TUint16 hlenflag = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));

	TUint8 c = *ptr;
	TUint hlen = (c>>4)<<2;
    TUint reserved = (hlenflag & 0x0f00);
    TUint flag = (hlenflag & 0x003f);
    TBool ecnf = (hlenflag & 0x00c0);
	TBool urgf = (hlenflag & 0x0020);
	TBool ackf = (hlenflag & 0x0010);
	TBool pshf = (hlenflag & 0x0008);
	TBool rstf = (hlenflag & 0x0004);
	TBool synf = (hlenflag & 0x0002);
	TBool finf = (hlenflag & 0x0001);
    ptr+=2;
	TUint window = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	TUint chksum = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;
	TUint urgptr = ByteOrder::Swap16((TUint16)(*ptr | (*(ptr+1)<<8)));
	ptr+=2;

	Logger().WriteFormat(KTCPLengthString, len, hlen);
	Logger().WriteFormat(KPortString, srcp, dstp);
	Logger().WriteFormat(KSeqAckString, seqnum, acknum);
	Logger().WriteFormat(KReservedString, reserved);
	Logger().WriteFormat(KFlagsString, flag, ecnf?_S("ECN,"):_S(""),urgf?_S("URG,"):_S(""), ackf?_S("ACK,"):_S(""), pshf?_S("PSH,"):_S(""),
		                      rstf?_S("RST,"):_S(""), synf?_S("SYN,"):_S(""), finf?_S("FIN"):_S(""));
	Logger().WriteFormat(KWindowUrgentString, window, urgptr);
	if (chksum != sum)
		Logger().WriteFormat(KChecksumString3, chksum, sum);

	if (hlen>20)
		{
		_LIT(string2,"	TCP Options %d bytes");
		Logger().WriteFormat(string2, hlen-20);
		TInt h, i, opt, optlen=0;
		h = hlen-20;
		for (i=0; i<h; i+=optlen)
			{
			opt = ptr[i];
			if (opt == 0) // KTcpOptEol
				break;
			if (opt == 1) // KTcpOptNop
				optlen = 1;
			else
				{
				if (i+1 >= h)
					break;
				optlen = ptr[i+1];
				if (optlen < 2)
					optlen = 2;
				}

			switch (opt)
				{
			case 1:
					{
					_LIT(string3,"	    NOP");
					Logger().Write(string3);
					}
				break;
			case 2:
					{
					_LIT(string4,"	    Max Seg Size = %d");
					Logger().WriteFormat(string4, BigEndian::Get16(ptr+i+2));
					}
				break;
			default:
					{
					_LIT(string5,"	    Unknown [0x%02x]");
					Logger().WriteFormat(string5, opt);
					}
				break;
				}
			}
		}

	ptr += (hlen-20);
	TInt n1 = (TInt)aDes.Ptr()-(TInt)ptr;
	aDes.Set(ptr, aDes.Length()-n1);
	return n1;
	}


TInt CTE_VJCompStepBase::DumpIp(TPtrC8 aDes)

/**
Dump a IP and a TCP header if the protocol type is TCP
Otherwise, dump only the IP header

@param aDes pointer to the IP header
*/
	{

    // Do not display if not in debug mode
	    if (!iDebug)
            return 0;

	INFO_PRINTF1(_L("~~~~~~~~~~ DUMP IP PACKET ~~~~~~~~~~"));

	const TUint8* ptr = aDes.Ptr();
	TUint8 c = *ptr++;
//	TUint ver = c >> 4;
	TUint hlen = (c & 0xf) << 2;
	TUint8 tos = *ptr;
	ptr++;
	TUint16 len = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 id = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint16 frag = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TBool zf = (frag & 0x8000);
	TBool df = (frag & 0x4000);
	TBool mf = (frag & 0x2000);
	frag = (TUint16)((frag & 0x1fff)<<3);

	TUint8 ttl = *ptr;
	ptr++;
	TUint8 proto = *ptr++;
	TUint16 chksum = ByteOrder::Swap16((TUint16)(*ptr | *(ptr+1)<<8));
	ptr+=2;
	TUint32 srca = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
	TUint32 dsta = ByteOrder::Swap32(*ptr | (*(ptr+1)<<8) | (*(ptr+2)<<16) | (*(ptr+3)<<24));
	ptr+=4;
//	TBool opts = (hlen>20);

	Logger().WriteFormat(KHdrLengthString, len, hlen);
	Logger().WriteFormat(KSrcDstAddrString,(srca&0xff000000)>>24,(srca&0x00ff0000)>>16,(srca&0x0000ff00)>>8,(srca&0x000000ff),
							 (dsta&0xff000000)>>24,(dsta&0x00ff0000)>>16,(dsta&0x0000ff00)>>8,(dsta&0x000000ff));
	Logger().WriteFormat(KIDFragmentString, id, frag, df?_S("<DF>"):_S(""), mf?_S("<MF>"):_S(""), zf?_S("<Z>"):_S(""));
	Logger().WriteFormat(KTOSTTLChksumString, tos, ttl, chksum);
	Logger().WriteFormat(KCodeTextString, IpProtocolToText(proto), proto);

	if (hlen>20)
		ptr += (hlen-20);

	TInt n = (TInt)ptr-(TInt)aDes.Ptr();
	TInt tlen = aDes.Length()-n;

	if (tlen >= 0)
	{
		aDes.Set(ptr, tlen);

		switch (proto)
		{
		case 1:
			//return n+DumpIcmp(aDes, tlen);
		    return n;
		case 6:
	     	return n+DumpTcp(aDes, srca, dsta, tlen);
	     	//return n+DumpTcp(payload, srca, dsta, tlen);
		case 17:
	    	// return n+DumpUdp(aDes,srca,dsta, tlen);
		    return n;
		default:
			return n;
		}

	}
	else
	{
	    return n;
	}
}

/**
Determine if the packet is a tossed packet
Set test step result to EFalse if it is not a tossed packet

@param vjdecompRet Returned boolean value of DecompVJUncomp() or DecompVJComp
@param refdecompOutput Returned decompressed packet from Reference Decompressor
*/


void CTE_VJCompStepBase::IsItATossPacket(TBool vjdecompRet, TUint8 * refdecompOutput)
{
     if (!vjdecompRet)
 	 {
 	     INFO_PRINTF1(_L("Packet was not successfully processed by VJ Decompressor."));
 	 }

 	 if (refdecompOutput == 0)
 	 {
 	     INFO_PRINTF1(_L("Packet was not successfully processed by Ref Decompressor."));
 	 }

 	 if ((!vjdecompRet) && refdecompOutput == 0)
 	 {
 	      // Both VJ Decompressor and Ref Decompressor did not process the packet successfully.
 	      // Assume that toss flag is set.
 	      // No comparison should be done
 	      // No need to set Test Step Result because it is still EPass at this point
 	      INFO_PRINTF1(_L("Toss flag is set and packet is discarded."));
 	 }
 	 else
 	 {
 	      // Only one of VJ Decompressor or Ref Decompressor could not process the packet successfully
 	      // Their results don't agree, so set test step to fail
 	      // No comparison should be done
 	      SetTestStepResult(EFail);
 	 }
}

/**
   Compress a packet.
   Decompress the same chunk using VJ DeCompressor and Reference Decompressor
   Compare the results of VJ Decompressor to original packet or
   Compare the results of the VJ Decompressor with that of the Reference Decompressor

   @param compareVJandRef says whether to compare VJ and Ref outputs or VJ and orig output
*/


void CTE_VJCompStepBase::CompressDecompressL(TBool compareVJandRef)
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

    // Create source buffer which holds the original packet
    // This is to prevent compromising the test results if the compressor/decompressor alters the original data
	HBufC8 *sourceBuffer = iRec->Data().AllocLC();
	INFO_PRINTF2(_L(" ===Size of sourceBuffer before compression: %d==="), sourceBuffer->Length());

	// Create a pointer to the original data to be used for comparison later on
	TPtrC8 origPtrC((TUint8 *)iRec->Data().Ptr(), iRec->Data().Length());

	// Dump original packet
	INFO_PRINTF1(_L("  06 Original packet before compression:"));
	DumpIp(origPtrC);

    //-------------- substep 07 -------------------
    // compress using our reference Compressor
	INFO_PRINTF1(_L("  07 Compress the packet using reference compressor:"));

    struct mbuf mbufObj;
    mbufObj.m_len = sourceBuffer->Length();
    mbufObj.m_off = const_cast<TUint8 *> (sourceBuffer->Ptr());

    struct mbuf* mbufObjPtr = &mbufObj;

    TUint refcompRet = sl_compress_tcp(mbufObjPtr, (struct ip *)(const_cast<TUint8 *>(sourceBuffer->Ptr())), iSlcomp);


	TPtrC8 refCompPacketPtrC ((TUint8 *)mbufObjPtr->m_off, mbufObjPtr->m_len);
	INFO_PRINTF2(_L(" ***Size of REF compressed packet: %d***"), mbufObjPtr->m_len);

    // Print information of the reference compressed packet
    DumpCompressedPacket(refCompPacketPtrC, refcompRet);

    //--------------substep 08----------------------
	// Decompress the same chunk using Reference Decompressor and VJ Decompressor

	TInt refPacketLen = mbufObjPtr->m_len;

	// Create packet to be sent to Reference decompressor
	TUint8 *refdecompOutput = NULL;

	// Create packet buffer to be sent to VJ decompressor
	// This is to prevent compromising the test results if the compressor/decompressor alters the original data
	// The size of the HBuf object must have a maximum length that is big enough to hold the decompressed data
	HBufC8* vjCompBuffer = HBufC8::NewLC(TPcapRecord::KMaxPacketSize);
	TPtr8 vjOutputPtr (vjCompBuffer->Des());
	vjOutputPtr = refCompPacketPtrC;

	iPacket.CopyIn(*vjCompBuffer,0);
	iPacket.TrimEnd(vjCompBuffer->Length());

	// Create info packet
 	RMBufPacket pkt;
    pkt.NewInfoL();
	pkt.Pack();

	// Put info packet in front of this packet
	iPacket.Prepend(pkt);

	TUint vjdecompRet;

	// decompress the packet using VJCompressor and Reference decompressor
	INFO_PRINTF1(_L("  08 Decompress the packet using VJDeCompressor and Reference Decompressor:"));

	//Packet type returned by reference compressor is converted to packet type used in VJ
    TInt mappedCompRet = MapToVjCompRet (refcompRet);

	switch (mappedCompRet)
	{
	   	 case KPppIdIp:
	   	 // the packet is TYPE_IP
	    	// decompress TYPE_IP packet using our reference decompressor
	    	INFO_PRINTF1(_L("  07 Decompress TYPE_IP packet using reference decompressor:"));
      		refdecompOutput = sl_uncompress_tcp(mbufObjPtr->m_off,&refPacketLen, (TUint)TYPE_IP, iComp);

	        // VJ Decompressor function is not called because the packet is TYPE_IP
	        INFO_PRINTF1(_L("  TYPE_IP packet is not modified by VJDecompressor."));
	        vjdecompRet = ETrue;
	    	break;
  		 case KPppIdVjCompTcp:
	     // the packet is COMPRESSED_TCP
            // decompress using our reference decompressor
	        INFO_PRINTF1(_L("  07 Decompress COMPRESSED_TCP packet using reference decompressor:"));
      		refdecompOutput = sl_uncompress_tcp(mbufObjPtr->m_off, &refPacketLen, (TUint)TYPE_COMPRESSED_TCP, iComp);

            //decompress using our VJ Decompressor
	        INFO_PRINTF1(_L("  08 Decompress COMPRESSED_TCP packet using VJDeCompressor:"));
	        vjdecompRet = iVJDeCompressor->DecompVJComp(iPacket);
	        INFO_PRINTF2(_L(" 08 COMPRESSED_TCP packet is decompressed: %d"), vjdecompRet);
	   	  	break;
	     case KPppIdVjUncompTcp:
	     // the packet is UNCOMPRESSED_TCP
	        // decompress using our reference Compressor
	        INFO_PRINTF1(_L("  07 Decompress UNCOMPRESSED_TCP packet using reference decompressor:"));
      		refdecompOutput = sl_uncompress_tcp(mbufObjPtr->m_off, &refPacketLen, (TUint)TYPE_UNCOMPRESSED_TCP, iComp);

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
      		refdecompOutput = sl_uncompress_tcp(mbufObjPtr->m_off,&refPacketLen, (TUint)TYPE_ERROR, iComp);

	     	//decompress using our VJ Decompressor
	        INFO_PRINTF1(_L("  08 Decompress ERROR packet using VJDeCompressor:"));
	        iVJDeCompressor->CRCError();
	        vjdecompRet = EFalse;
	        break;
	    }

	     if ((vjdecompRet) && refdecompOutput != 0)
	     {

	         //--------------substep 09----------------------
 	         //Display the decompressed packets
 	         INFO_PRINTF1(_L("  09 The VJ Decompressed Packet is:"));


	          // The decompressed data should be larger than the compressed data
	         // So we need to expand the size of the pointer in order to hold the decompressed data
	         vjOutputPtr.SetMax();

	         // Remove info packet
	         iPacket.Remove();

	         iPacket.CopyOut(vjOutputPtr);

	         // TPtrC8 is created because DumpIp() only takes a TPtrC8 as a parameter
	         TPtrC8 vjOutputPtrC (vjOutputPtr);
	         DumpIp(vjOutputPtrC);

	         INFO_PRINTF2(_L(" ***Size of VJ decompressed packet: %d***"), vjOutputPtr.Length());

             //--------------substep 10----------------------
 	         // Copy the result of the Reference decompressor to file
	         INFO_PRINTF1(_L("  10 The REF Decompressed Packet is:"));

	         // content of refdecompOutput is copied into refOutputPtr to be returned to caller
	         TPtrC8 refOutputPtrC(refdecompOutput, refPacketLen);
	         DumpIp(refOutputPtrC);
	         INFO_PRINTF2(_L(" ***Size of REF decompressed packet: %d***"), refPacketLen);

	         if (compareVJandRef)
	         {
	             //--------------substep 11----------------------
 	             //Compare the results of the reference decompressor output and our VJ decompressor output
 	             INFO_PRINTF1(_L("  11 Compare the result produced by the Ref decompressor and the VJ decompressor:"));
 	             INFO_PRINTF2(_L(" Size of VJ decomp output: %d"), vjOutputPtrC.Length());
 		         INFO_PRINTF2(_L(" Size of Reference decomp output: %d"), refOutputPtrC.Length());


 		         if (!CompareOutput(refOutputPtrC, vjOutputPtrC))
 	 	         {
 	     	        SetTestStepResult(EFail);
 		         }
 		     }
 		     else
 		     {

 		        //--------------substep 11----------------------
 	            //Compare the results of the VJ decompressor output and the original packet
 	            INFO_PRINTF1(_L("  12 Compare the result of the VJ decompression and the original:"));
 	            INFO_PRINTF2(_L(" Size of VJ decomp output: %d"), vjOutputPtrC.Length());
 		        INFO_PRINTF2(_L(" Size of original output: %d"), origPtrC.Length());

 	            if (!CompareOutput(vjOutputPtrC, origPtrC))
 	 	        {
 	     	         SetTestStepResult(EFail);

 	     	         //--------------substep 12----------------------
 	                 //Compare the results of the reference decompressor output and the original packet
 	                 //Only display if debug mode is on so that we can verify if output of VJ decompressor
 	                 //is the same as output of reference decompressor
 	                 // This information is useful when VJ decompressed output is different from the original packet
 	                 if (iDebug)
 	                 {
 	                     INFO_PRINTF1(_L("  11 Compare the result of the ref decompression with the original:"));
 	                     INFO_PRINTF2(_L(" Size of Reference decomp output: %d"), refOutputPtrC.Length());
 		                 INFO_PRINTF2(_L(" Size of original output: %d"), origPtrC.Length());

 	                     if (CompareOutput(refOutputPtrC, origPtrC))
 	                     {
 	                          INFO_PRINTF1(_L("The result of the Ref decompressor and the original packet are the same."));

 	                          if (!CompareOutput(refOutputPtrC, vjOutputPtrC))
 	                          {
 	                             INFO_PRINTF1(_L("But the result of the Ref decompressor and the VJ are NOT the same."));
 					             INFO_PRINTF1(_L("There is something wrong with the VJ Decompressor!!!"));
 	                          }

 	                     }
 	                     else
 	                     {
 	                          INFO_PRINTF1(_L("The result of the Ref decompressor and the original packet are not the same."));
 	                          if (CompareOutput(refOutputPtrC, vjOutputPtrC))
 	                          {
 	                              INFO_PRINTF1(_L("But the result of the Ref decompressor and VJ decompressor are the same."));
 	                              INFO_PRINTF1(_L("Something wrong with the test packet data?"));
 	                          }
 	                          else
 	                          {
 	                              INFO_PRINTF1(_L("The Ref decompressor result, VJ decompressor result and the original packet are all different!!!"));
 	                          }

 	                     }

 		             }

	             }
	         }
	     }
	     else
	     {

	         IsItATossPacket(vjdecompRet, refdecompOutput);

	     }
	     // Cleanup
	     CleanupStack::PopAndDestroy(vjCompBuffer);
		 CleanupStack::PopAndDestroy(sourceBuffer);
		 vjCompBuffer = NULL;
		 sourceBuffer = NULL;

		 __UHEAP_MARKEND;
}
