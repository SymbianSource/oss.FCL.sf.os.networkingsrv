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
// This contains CTE_PPPCompStep class which is
// the base class for all the PPPComp test steps.
// 
//

/**
 @file TE_PPPCompStep.cpp
*/
#include "TE_PPPCompStep.h"
#include <test/testexecutelog.h>

#include "pppCcp.h"
#include <ss_pman.h>

/* Legal PPP ID to use for compressed frames */
static const TUint16 KTestPppId = 1;

#define VERBOSELOG(s)
//#define VERBOSELOG(s) s

#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

// Constructor
CTE_PPPCompStep::CTE_PPPCompStep():iPppCcp(NULL),iCompressorConfig(),
		iCompressorCon(NULL),iDeCompressorCon(NULL),iCompressor(NULL),
		iDeCompressor(NULL), iFs(),iPtrCompType(),iPppCompConfig()
{
}

// Destructor
CTE_PPPCompStep::~CTE_PPPCompStep()
	{
	//
	// OK Spin around removing all the Compressor Config information`
	//
	CPppCompConfig* CompressorData;
	CObjectCon* con;
	TInt i;

	TSglQueIter<CPppCompConfig> Iterator(iCompressorConfig);
	Iterator.SetToFirst();

	CompressorData = Iterator++;
	while(CompressorData)
	{
		delete CompressorData;
		CompressorData = Iterator++;
	}

	if (iCompressor)
		{
		iCompressor->Close();
		iCompressor = NULL;
		}

	if (iDeCompressor)
		{
		iDeCompressor->Close();	//pb in closing decompressor?
		iDeCompressor = NULL;
		}

	//
	// Delete all the Containers
	//
	if(iCompressorCon)
		{
		con = iCompressorCon;

		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
			iCompressorCon = NULL;
		}


	if(iDeCompressorCon)
		{
		CObjectCon* con;
		con = iDeCompressorCon;
		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
		iDeCompressorCon = NULL;
		}

	if (iPppCcp)
		{
		delete iPppCcp;
		iPppCcp = NULL;
		}

    __CFLOG_DELETE;
	}

TVerdict CTE_PPPCompStep::doTestStepPreambleL()
{
	INFO_PRINTF1(_L("Test Step Preamble"));
	/*
	 * Load the config from the .ini file
	 */
	iPppCcp = CPppCcp::NewL(NULL);
	SetTestStepResult(EPass);
	return TestStepResult();
}

static const TInt KMBuf_MaxAvail = 393216;
static const TInt KMBuf_MBufSize = 128;
static const TInt KMBuf_MBufSizeBig = 1600;
static const TInt KMBuf_InitialAllocation = 128;
static const TInt KMBuf_MinGrowth = 64;
static const TInt KMBuf_GrowthThreshold = 40;

void CTE_PPPCompStep::StartMBufL()
{
	__CFLOG_CREATEL;
#ifdef SYMBIAN_ZERO_COPY_NETWORKING	
    // Initialize the commsbuf pond
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

void CTE_PPPCompStep::GetCompressionTypeFromConfig()
{

	TInt bRet = GetStringFromConfig(ConfigSection(), _L("comptype"), iPtrCompType);
	if (!bRet)
		SetTestStepResult(EFail);

	if      (iPtrCompType==_L("predcomp" ))
		{
	 	INFO_PRINTF1(_L("     ------------ predcomp ---------------:"));
		}
	else if (iPtrCompType==_L("mscomp"))
		{
	 	INFO_PRINTF1(_L("     ------------ mscomp --------------:"));
		}
	else if (iPtrCompType==_L("staccomp"))
		{
	 	INFO_PRINTF1(_L("     ------------ staccomp --------------:"));
		}

}

void CTE_PPPCompStep::UnloadCompressor(void)
{
	if (iCompressor)
		{
		iCompressor->Close();
		iCompressor = NULL;
		}
}

void CTE_PPPCompStep::UnloadDeCompressor(void)
{
	if (iDeCompressor)
		{
		iDeCompressor->Close();
		iDeCompressor = NULL;
		}
}

void CTE_PPPCompStep::StoreCompressedDataIntoNewFile(const TPtrC& aDestFileNamePtr, TPtr8& aSourcePtr )
	{
	//-------------- substep 9 --------------------
	// store the result of the compression because it differs from the reference
	INFO_PRINTF1(_L("Error: The content is not the same"));
	//store the result of the compression in a file
	// create the file to store the compressed data
	RFile compressedFile;
	TFileName compressedFileName(aDestFileNamePtr);
	compressedFileName.Append(_L("."));
	compressedFileName.Append(iPtrCompType);
	TInt err = compressedFile.Create(iFs, compressedFileName, EFileWrite | EFileShareAny);
	if (err == KErrAlreadyExists)
		{
		err = compressedFile.Replace(iFs, compressedFileName, EFileWrite | EFileShareAny);
		}
	else if (err!=KErrNone) // file does not exist - create it
		{
		INFO_PRINTF2(_L("Could not create compressed file (error %d)."), err);
		iFs.Close();
		//aChain.Free();
		UnloadCompressor();
		iFs.Close();
		SetTestStepResult(EFail);
		}

	err = compressedFile.Write(aSourcePtr);
	compressedFile.Close();
	//aChain.Free();
	UnloadCompressor();
	iFs.Close();
	SetTestStepResult(EFail);
	}


TBool CTE_PPPCompStep::IsDeCompressedBufferIdenticalToReference(RMBufQ& aBufferQ, TPtr8& aReference, TPtrC& aDestFileNamePtr)
	{
	// Decompress OK so...
	// extract the protocol information - this can be either one or two bytes, but
	// the last byte must be odd, and the first byte (if present) must be even
	RMBuf* m = aBufferQ.First();
	TUint8* p = m->Ptr();
	TUint prot = *p++;
	if(!(prot&1))
		prot = (prot<<8)|*p++;
	m->AdjustStart(p - m->Ptr());

	TInt		offset = 0;
	RMBuf*		temp = NULL;
	TBool isIdenticalToReference = ETrue;

	// Make sure the protocol ID came through OK
	if (prot != KTestPppId)
		{
		INFO_PRINTF1(_L("Error: The protocol ID is not the same"));
		isIdenticalToReference = EFalse;
		}

	// Compare each MBuf in the chain with the corresponding slice
	// of the reference descriptor
	while (	isIdenticalToReference &&
			(offset < aReference.Length()) &&
			((temp = aBufferQ.Remove()) != NULL)
			)
		{
		TInt bufLength = temp->Length();

		if (temp->Length() > (aReference.Length() - offset))
			{
			INFO_PRINTF4(_L("Warning: The length of the decompressed MBuf (%d) is not the same as the original (%d) at offset %d"),
				temp->Length(), (aReference.Length() - offset), offset);
			isIdenticalToReference  = EFalse;
			// futilely keep going
			bufLength = (aReference.Length() - offset);
			}
		TPtrC8 remainingDataPtr(aReference.Mid(offset, bufLength));
		TPtrC8 currentBufferPtr(temp->Ptr(), bufLength);
		if (remainingDataPtr != currentBufferPtr )	// comparing strings here
			{
			INFO_PRINTF1(_L("Error: The content is not the same"));
			isIdenticalToReference  = EFalse;
			}
		else
			{
			offset += temp->Length();
			temp->Free();
			temp = NULL;
			}
		}
	if ((offset != aReference.Length()) || temp)
		{
		INFO_PRINTF3(_L("Error: The content is not the same length (%d != %d)"), offset, aReference.Length());
		isIdenticalToReference  = EFalse;
		}

	if (!isIdenticalToReference && temp )
		{
		//store the result of the compression in a file
		// create the file to store the compressed data
		RFile decompressedFile;
		TFileName decompressedFileName(aDestFileNamePtr);
		decompressedFileName.Append(_L(".de"));
		decompressedFileName.Append(iPtrCompType);
		TInt err = decompressedFile.Create(iFs, decompressedFileName, EFileWrite | EFileShareAny);
		if (err == KErrAlreadyExists)
			{
			err = decompressedFile.Replace(iFs, decompressedFileName, EFileWrite | EFileShareAny);
			}
		else if (err!=KErrNone) // error in file creation
			{
			INFO_PRINTF2(_L("Could not create compressed file (error %d)."), err);
			iFs.Close();
			SetTestStepResult(EFail);
			}

		err = decompressedFile.Write(aReference.Left(offset));
		TPtrC8 currentBufferPtr(temp->Ptr(), temp->Length());
		err = decompressedFile.Write(currentBufferPtr);
		while ((temp = aBufferQ.Remove()) != NULL)
			{
			TPtrC8 currentBufferPtr(temp->Ptr(), temp->Length());
			err = decompressedFile.Write(currentBufferPtr);
			temp->Free();
			temp = NULL;
			}
		decompressedFile.Close();
		}
	//make sure the buffQ has been freed
	aBufferQ.Free();
	return isIdenticalToReference ;
	}

CPPPCompressTest1::CPPPCompressTest1() : CTE_PPPCompStep()
{
	SetTestStepName(KPPPCompTest);
}

CPPPCompressTest1::~CPPPCompressTest1()
{
}


// TODO: All the various error cases in here that call SetTestStepResult(EFail)
// should clean up and return TestStepResult() to prevent the test from
// continuing with bad data.
TVerdict CPPPCompressTest1::doTestStepL()
{
	if (TestStepResult() != EPass)
	{
	    return TestStepResult();
	}

	// Construct and install the active schedule
	CActiveScheduler* stepSched=NULL;
	stepSched=new(ELeave) CActiveScheduler;

	// Push onto the cleanup stack
	CleanupStack::PushL(stepSched);

	//Install as the active scheduler
	CActiveScheduler::Install(stepSched);

	// open the binary file, instantiate the compressor and feed the compressor with the binary and look at the output.
	TInt err;
	//-------------- substep 1 --------------------
	INFO_PRINTF1(_L("  01 Read from script which test to run:"));
	GetCompressionTypeFromConfig();

	//-------------- substep 2 --------------------
	INFO_PRINTF1(_L("  02 connect to file server:"));
	// connect to file system and read source and dest file names and open them
	TInt bRet = iFs.Connect();
	if (bRet != KErrNone)
	{
		INFO_PRINTF2(_L("Could not connect to file server (error %d)."), bRet);
		SetTestStepResult(EFail);
	}

	//-------------- substep 3 --------------------
	// open source file
	TPtrC ptrSourceFileName;
	bRet = GetStringFromConfig(ConfigSection(), _L("sourcefile"), ptrSourceFileName);
	if (!bRet)
		{
		INFO_PRINTF2(_L("Could not find source file name in ini file(error %d)."), bRet);
		SetTestStepResult(EFail);
		}
	RFile sourceFile;
	TFileName sourceFileName(ptrSourceFileName);
	INFO_PRINTF2(_L("  03 read file name and open source file:%S"), &sourceFileName);
	err = sourceFile.Open(iFs, sourceFileName, EFileRead|EFileShareAny);
	if (err==KErrNotFound) // file does not exist - create it
		{
		INFO_PRINTF2(_L("Could not open Source file (error %d)."), bRet);
		SetTestStepResult(EFail);
		}
	TInt sourceFileSize;
	err = sourceFile.Size(sourceFileSize);
	//-------------- substep 3 --------------------
	// double check the size here
	HBufC8 *sourceBuffer = HBufC8::NewLC(sourceFileSize);
	TPtr8 sourcePtr = sourceBuffer->Des();
	err = sourceFile.Read(sourcePtr, sourceFileSize);
	sourceFile.Close();

	StartMBufL();

	//-------------- substep 4 --------------------
	INFO_PRINTF1(_L("  04 Allocate filesize-bytes long RMBufChain:"));
	RMBufChain aChain;
	TRAPD(allocErr,aChain.AllocL(sourceFileSize));
	if (allocErr!= KErrNone)
		{
		INFO_PRINTF1(_L("Error: Couldn't allocate RMBuf:"));
		aChain.Free();
		iFs.Close();
		SetTestStepResult(EFail);
		}
	aChain.CopyIn(*sourceBuffer, 0);

	//-------------- substep 4b --------------------
	// get options
	TPtrC ptrOptions;
	bRet = GetStringFromConfig(ConfigSection(), _L("opts"), ptrOptions);
	// If opts isn't available, use no options

	INFO_PRINTF2(_L("  04 options is:%S"), &ptrOptions);

	//-------------- substep 5 --------------------
	INFO_PRINTF1(_L("  05 Load Compressor:"));
	//load compressor according to ini file
	__ASSERT_ALWAYS(iPppCcp,User::Panic(_L("CTE_PPPCompStep::DummyCPppCcp not instantiated"),KErrGeneral));

	iPppCompConfig.AddNameL(iPtrCompType);
	iPppCompConfig.AddOptionsL(ptrOptions);
	TRAP( err, iCompressor = iPppCcp->LoadCompressorL(iPppCompConfig, sourceFileSize));
	TESTEL(KErrNone == err, err);

	// compress
	TPppCompressReturnValue compRet = iCompressor->Compress(aChain, KTestPppId);
	TESTEL (EPppCompressedOK ==compRet, compRet);

	//-------------- substep 6 --------------------
	// copy the result of the compression
	INFO_PRINTF1(_L("  06 Copy the result of the compression to sourceBuffer:"));
	aChain.CopyOut(sourcePtr);	//reuse the source buffer, the compressed data should be smaller

	//-------------- substep 7 --------------------
	// copy the result of the compression
	INFO_PRINTF1(_L("  07 open the reference file and compare with the result of the compression:"));
	// compare the compressed data with the reference file
	//get the name of the destination file from ini

	TPtrC ptrDestFileName;
	bRet = GetStringFromConfig(ConfigSection(), _L("destfile"), ptrDestFileName);
	if (!bRet)
		{
		aChain.Free();
		UnloadCompressor();
		iFs.Close();
		SetTestStepResult(EFail);
		}
	RFile destFile;
	TFileName destFileName(ptrDestFileName);
	INFO_PRINTF2(_L("  07 open dest file:%S"), &destFileName);
	err = destFile.Open(iFs, destFileName, EFileRead|EFileShareAny);
	TInt referenceCompareResult = KErrPathNotFound;
	if (err==KErrNone) // file could be opened properly
		{
		//-------------- substep 8 --------------------
		INFO_PRINTF1(_L("  08 Compare the contents of the compressed chain and the content of the reference file:"));
		TInt destFileSize;
		err = destFile.Size(destFileSize);
		// double check the size here
		HBufC8 *destBuffer = HBufC8::NewLC(destFileSize);
		TPtr8 destPtr(destBuffer->Des());
		err = destFile.Read(destPtr, destFileSize);
		destFile.Close();
		// compare to the reference
		referenceCompareResult = sourcePtr.Compare(destPtr);
		CleanupStack::PopAndDestroy(destBuffer);
		}
	else
		{
		INFO_PRINTF2(_L("Could not find dest file (error %d)."), err);
		// let it produce the compressed file that we can use later as a source for the decompression
		}

	//-------------- substep 9 --------------------
	//check if necessary to store the result of the compression because it differs from the reference
	if(referenceCompareResult)
		{
		StoreCompressedDataIntoNewFile( ptrDestFileName, sourcePtr);
		}

	//-------------- substep 10 --------------------
	INFO_PRINTF1(_L("  10 Free chain. Clean up stack:"));
	aChain.Free();
#ifdef SYMBIAN_ZERO_COPY_NETWORKING	
	iCommsBufPond.Close();
#else
	delete iMBMngr;
#endif 	// SYMBIAN_ZERO_COPY_NETWORKING	
	CleanupStack::PopAndDestroy(sourceBuffer);
	UnloadCompressor();
	iFs.Close();

	return TestStepResult();
} // PPPCompressTest1

CPPPDeCompressTest1::CPPPDeCompressTest1() : CTE_PPPCompStep()
{
	SetTestStepName(KPPPDecompTest);
}

CPPPDeCompressTest1::~CPPPDeCompressTest1()
{
}

// TODO: All the various error cases in here that call SetTestStepResult(EFail)
// should clean up and return TestStepResult() to prevent the test from
// continuing with bad data.
TVerdict CPPPDeCompressTest1::doTestStepL()
{
	if (TestStepResult() != EPass)
	{
	    return TestStepResult();
	}

	// Construct and install the active schedule
	CActiveScheduler* stepSched=NULL;
	stepSched=new(ELeave) CActiveScheduler;

	// Push onto the cleanup stack
	CleanupStack::PushL(stepSched);

	//Install as the active scheduler
	CActiveScheduler::Install(stepSched);

	// open the binary file, instantiate the compressor and feed the compressor with the binary and look at the output.
	TInt err;

	//-------------- substep 1 --------------------
	INFO_PRINTF1(_L("  01 Read from script which test to run:"));
	GetCompressionTypeFromConfig();

	//-------------- substep 2 --------------------
	INFO_PRINTF1(_L("  02 connect to file server:"));
	// connect to file system and read source and dest file names and open them;
	TInt bRet = iFs.Connect();
	if (bRet != KErrNone)
	{
		INFO_PRINTF2(_L("Could not connect to file server (error %d)."), bRet);
		SetTestStepResult(EFail);
	}

	//-------------- substep 3 --------------------
	TPtrC ptrSourceFileName;
	bRet = GetStringFromConfig(ConfigSection(), _L("sourcefile"), ptrSourceFileName);
	if (!bRet)
		{
		INFO_PRINTF2(_L("Could not find source file name in ini file(error %d)."), bRet);
		SetTestStepResult(EFail);
		}
	RFile sourceFile;
	TFileName sourceFileName(ptrSourceFileName);
	INFO_PRINTF2(_L("  03 read file name and open source file:%S"), &sourceFileName);
	err = sourceFile.Open(iFs, sourceFileName, EFileRead|EFileShareAny);
	if (err==KErrNotFound) // file does not exist - create it
		{
		INFO_PRINTF2(_L("Could not open Source file (error %d)."), bRet);
		SetTestStepResult(EFail);
		}

	TInt sourceFileSize;
	err = sourceFile.Size(sourceFileSize);
	// double check the size here
	HBufC8 *sourceBuffer = HBufC8::NewLC(sourceFileSize);
	TPtr8 sourcePtr = sourceBuffer->Des();
	err = sourceFile.Read(sourcePtr, sourceFileSize);
	sourceFile.Close();

	StartMBufL();

	//-------------- substep 4 --------------------
	INFO_PRINTF1(_L("  04 Allocate filesize-bytes long RMBufChain:"));
	RMBufQ aBufQ;
	RMBufChain aChain;
	TRAPD(allocErr,aChain.AllocL(sourceFileSize));
	if (allocErr!= KErrNone)
		{
		INFO_PRINTF1(_L("Error: Couldn't allocate RMBuf:"));
		iFs.Close();
		SetTestStepResult(EFail);
		}
	aChain.CopyIn(*sourceBuffer, 0);
	CleanupStack::PopAndDestroy(sourceBuffer);
	sourceBuffer=NULL;
	aBufQ.Assign(aChain);	// aBufQ takes the ownership of the chain
	//-------------- substep 4b --------------------
	// get options
	TPtrC ptrOptions;
	bRet = GetStringFromConfig(ConfigSection(), _L("opts"), ptrOptions);
	// If opts isn't available, use no options

	INFO_PRINTF2(_L("  04 options is:%S"), &ptrOptions);

	//-------------- substep 5 --------------------
	INFO_PRINTF1(_L("  05 Load Compressor:"));
	//load compressor according to ini file
	__ASSERT_ALWAYS(iPppCcp,User::Panic(_L("CTE_PPPCompStep::DummyCPppCcp not instantiated"),KErrGeneral));
	iPppCompConfig.AddNameL(iPtrCompType);
	iPppCompConfig.AddOptionsL(ptrOptions);
	TRAP( err, iDeCompressor = iPppCcp->LoadDeCompressorL(iPppCompConfig, sourceFileSize));
	TESTEL(KErrNone == err, err);

	// decompress
	TBool decompRet = iDeCompressor->Decompress(aBufQ);
	UnloadDeCompressor();
	if (!decompRet)
		{
		INFO_PRINTF2(_L("Error in decompressing"), decompRet);
		aBufQ.Free();
		iFs.Close();
		SetTestStepResult(EFail);
		}


	//-------------- substep 6 --------------------
	// open the reference file to check the decompression
	INFO_PRINTF1(_L("  06 open the reference file and compare with the result of the compression:"));
	// compare the decompressed data with the reference file
	//get the name of the destination file from ini
	TPtrC ptrDestFileName;
	bRet = GetStringFromConfig(ConfigSection(), _L("destfile"), ptrDestFileName);
	if (!bRet)
		{
		INFO_PRINTF2(_L("Could not find dest file name in ini file (error %d)."), bRet);
		aBufQ.Free();
		iFs.Close();
		SetTestStepResult(EFail);
		}
	RFile destFile;
	TFileName destFileName(ptrDestFileName);
	INFO_PRINTF2(_L("  06 open dest file:%S"), &destFileName);
	err = destFile.Open(iFs, destFileName, EFileRead|EFileShareAny);
	if (err==KErrNotFound) // file does not exist - create it
		{
		INFO_PRINTF2(_L("Could not find dest file (error %d)."), bRet);
		aBufQ.Free();
		iFs.Close();
		SetTestStepResult(EFail);
		}

	TInt destFileSize;
	err = destFile.Size(destFileSize);
	// double check the size here
	HBufC8 *destBuffer = HBufC8::NewLC(destFileSize);
	TPtr8 destPtr = destBuffer->Des();
	err = destFile.Read(destPtr, destFileSize);
	destFile.Close();

	//-------------- substep 7 --------------------
	INFO_PRINTF1(_L("  07 Compare the contents of the decompressed chain and the content of the reference file:"));
	if(!IsDeCompressedBufferIdenticalToReference(aBufQ, destPtr, ptrDestFileName))
		{
		INFO_PRINTF1(_L("Error: The content is not the same"));
		//store the result of the compression in a file
		// create the file to store the compressed data
		aBufQ.Free();
		iFs.Close();
		SetTestStepResult(EFail);
		}

	// compare to the reference

	//-------------- substep 8 --------------------
	INFO_PRINTF1(_L("  08 Free chain. Clean up stack:"));
	aBufQ.Free();
	
#ifdef SYMBIAN_ZERO_COPY_NETWORKING	
	iCommsBufPond.Close();
#else
	delete iMBMngr;
#endif 	// SYMBIAN_ZERO_COPY_NETWORKING	

	CleanupStack::PopAndDestroy(destBuffer);
	iFs.Close();

	return TestStepResult();

}


CPPPCompressDecompressTest1::CPPPCompressDecompressTest1() : CTE_PPPCompStep()
{
	SetTestStepName(KPPPCompDecompTest);
}

CPPPCompressDecompressTest1::~CPPPCompressDecompressTest1()
{
}

TVerdict CPPPCompressDecompressTest1::doTestStepL()
{
	if (TestStepResult() != EPass)
	{
	    return TestStepResult();
	}

	// Construct and install the active schedule
	CActiveScheduler* stepSched=NULL;
	stepSched=new(ELeave) CActiveScheduler;

	// Push onto the cleanup stack
	CleanupStack::PushL(stepSched);

	//Install as the active scheduler
	CActiveScheduler::Install(stepSched);

	// open the binary file, instantiate the compressor and feed the compressor with the binary and look at the output.
	TInt err;
	//-------------- substep 1 --------------------
	INFO_PRINTF1(_L("  01 Read from script which test to run:"));
	GetCompressionTypeFromConfig();

	//-------------- substep 2 --------------------
	INFO_PRINTF1(_L("  02 connect to file server:"));
	// connect to file system and read source and dest file names and open them
	TInt bRet = iFs.Connect();
	if (bRet != KErrNone)
	{
		INFO_PRINTF2(_L("Could not connect to file server (error %d)."), bRet);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(stepSched);
	    return TestStepResult();
	}

	//-------------- substep 3 --------------------
	// open source file
	TPtrC ptrSourceFileName;
	bRet = GetStringFromConfig(ConfigSection(), _L("sourcefile"), ptrSourceFileName);
	if (!bRet)
		{
		INFO_PRINTF2(_L("Could not find source file name in ini file(error %d)."), bRet);
		iFs.Close();
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(stepSched);
	    return TestStepResult();
		}
	RFile sourceFile;
	TFileName sourceFileName(ptrSourceFileName);
	INFO_PRINTF2(_L("  03 read file name and open source file:%S"), &sourceFileName);
	err = sourceFile.Open(iFs, sourceFileName, EFileRead|EFileShareAny);
	if (err==KErrNotFound) // file does not exist - create it
		{
		INFO_PRINTF2(_L("Could not open Source file (error %d)."), bRet);
		iFs.Close();
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(stepSched);
	    return TestStepResult();
		}
	TInt sourceFileSize;
	err = sourceFile.Size(sourceFileSize);
	//-------------- substep 3b --------------------
	// get destination file name
	TPtrC ptrDestFileName;
	bRet = GetStringFromConfig(ConfigSection(), _L("destfile"), ptrDestFileName);
	if (!bRet)
		{
		iFs.Close();
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(stepSched);
	    return TestStepResult();
		}

	TFileName destFileName(ptrDestFileName);
	INFO_PRINTF2(_L("  03 dest file is:%S"), &destFileName);
	//  dont open the file, it will be used just to store compressed date
	//	err = destFile.Open(iFs, destFileName, EFileRead|EFileShareAny);

	//-------------- substep 3c --------------------
	// get options
	TPtrC ptrOptions;
	bRet = GetStringFromConfig(ConfigSection(), _L("opts"), ptrOptions);
	// If opts isn't available, use no options

	INFO_PRINTF2(_L("  03 options is:%S"), &ptrOptions);

	//-------------- substep 4 --------------------
	INFO_PRINTF1(_L("  04 Load Compressor and decompressor:"));
	//load compressor according to ini file
	__ASSERT_ALWAYS(iPppCcp,User::Panic(_L("CTE_PPPCompStep::DummyCPppCcp not instantiated"),KErrGeneral));
	iPppCompConfig.AddNameL(iPtrCompType);
	iPppCompConfig.AddOptionsL(ptrOptions);

	TRAP( err, iCompressor = iPppCcp->LoadCompressorL(iPppCompConfig, KPppCompressionChunkMaximumSize));
	TESTEL(KErrNone == err, err);

	TRAP( err, iDeCompressor = iPppCcp->LoadDeCompressorL(iPppCompConfig, KPppCompressionChunkMaximumSize));
	TESTEL(KErrNone == err, err);

	//-------------- substep 5 --------------------
	INFO_PRINTF1(_L("  05 Allocate KPppCompressionChunkMaximumSize-bytes long RMBufChain:"));
	// read some chunks of the source file and compress them one by one.
	// the size of the chunks is incremented up to KPppCompressionChunkMaximumSize

	StartMBufL();
	RMBufChain aChain;
	TRAPD(allocErr,aChain.AllocL(KPppCompressionChunkMaximumSize));
	if (allocErr!= KErrNone)
		{
		INFO_PRINTF1(_L("Error: Couldn't allocate RMBuf:"));
		aChain.Free();
		iFs.Close();
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(stepSched);
	    return TestStepResult();
		}
	RMBufQ aBufQ;

	// we allocate just the maximum size of a chunk
	HBufC8 *sourceBuffer = HBufC8::NewLC(KPppCompressionChunkMaximumSize);
	TPtr8 sourcePtr = sourceBuffer->Des();

	//beginning of the loop
	TInt chunkSize = 0;
	TInt offset = 0;
	TUint totalCompressed = 0;
	TUint totalUncompressed = 0;
	do
		{
		// find the right size of chunk, if the chunk is too small, the compression will expand and the decompress will fail
		TPppCompressReturnValue compRet = EPppCompressedNotOK;
		do
			{
			// free the old chain and create a new one, in case this is the
			// second or subsequent trip through this loop.
			aChain.Free();
			aChain.AllocL(KPppCompressionChunkMaximumSize);
			// increment chunkSize
			chunkSize+=KPppCompressionChunkMinimumSize;
			chunkSize = min(chunkSize, KPppCompressionChunkMaximumSize);
			// check that we don't reach the end of the file
			chunkSize = min(chunkSize, (sourceFileSize-offset));
			sourcePtr = sourceBuffer->Des();
			err = sourceFile.Read(sourcePtr, chunkSize);
			sourcePtr.SetLength(chunkSize);
			// check err
			//aChain.CopyIn(*sourceBuffer, 0);
			aChain.CopyIn(sourcePtr, 0);
			aChain.TrimEnd(chunkSize);
			//-------------- substep 6 --------------------
			// compress the chunk
			compRet = iCompressor->Compress(aChain, KTestPppId);
			if (compRet == EPppCompressedFrameExpanded)
				{
				VERBOSELOG( INFO_PRINTF4(_L("     Compression expanded from %d (%d); leaving at %d bytes"), chunkSize, chunkSize+2, aChain.Length()); )
				// Reset the compressor after an expansion. We are assuming this will only
				// happen at the beginning of the search for the chunkSize, so the decompressor
				// will not have been called yet
				(void) iCompressor->ResetCompressor(chunkSize, aChain);
				}
			else
				{
				VERBOSELOG( INFO_PRINTF5(_L("     Compressed from %d (%d) to %d bytes (%d%% compression)"),
					chunkSize, chunkSize+2, aChain.Length(), 100 - (100 * aChain.Length() / (chunkSize+2))); )
				}
			}
		while (	(compRet ==  EPppCompressedFrameExpanded)
				&& (chunkSize <KPppCompressionChunkMaximumSize)
				&& (chunkSize < (sourceFileSize-offset)));
		VERBOSELOG( INFO_PRINTF3(_L("     Testing chunk size %d bytes at offset %d"), chunkSize, offset); )
		// now we can validate the chunk size and use it for decompression
		offset+=chunkSize;
		totalUncompressed += chunkSize + 2;
		totalCompressed += aChain.Length();

		if (EPppCompressedNotOK == compRet)
			{
			INFO_PRINTF1(_L("06 Error in compressing"));
			aBufQ.Free();
			aChain.Free();
			UnloadCompressor();
			UnloadDeCompressor();
			iFs.Close();
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(sourceBuffer);
			CleanupStack::PopAndDestroy(stepSched);
		    return TestStepResult();
			}
		if (EPppCompressedFrameExpanded == compRet)
			{
			INFO_PRINTF1(_L("06 Compressing expands, test aborted"));
			aBufQ.Free();
			aChain.Free();
			UnloadCompressor();
			UnloadDeCompressor();
			iFs.Close();
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(sourceBuffer);
			CleanupStack::PopAndDestroy(stepSched);
		    return TestStepResult();
			}

		// now chain contains the compressed data

		//-------------- substep 7 --------------------
		// decompress that same chunk
		aBufQ.Assign(aChain);	// aBufQ takes the ownership of the chain
		TBool decompRet = iDeCompressor->Decompress(aBufQ);
		if (!decompRet)
			{
			INFO_PRINTF2(_L("07 Error in decompressing"), decompRet);
			aBufQ.Free();
			UnloadCompressor();
			UnloadDeCompressor();
			iFs.Close();
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(sourceBuffer);
			CleanupStack::PopAndDestroy(stepSched);
		    return TestStepResult();
			}
		//-------------- substep 8 --------------------
		// compare decompression and original
		if(!IsDeCompressedBufferIdenticalToReference(aBufQ, sourcePtr, ptrDestFileName))
			{
			INFO_PRINTF1(_L("08 Error: The content is not the same"));
			//store the result of the compression in a file
			// create the file to store the compressed data


			aBufQ.Free();
			UnloadCompressor();
			UnloadDeCompressor();
			iFs.Close();
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(sourceBuffer);
			CleanupStack::PopAndDestroy(stepSched);
		    return TestStepResult();
			}
		else
			{
			//INFO_PRINTF1(_L("08 buffer identical"));
			// When BuffQ has been freed, Chain has been freed too, so need to reallocate .
			aChain.AllocL(KPppCompressionChunkMaximumSize);
			}
		}
		while (offset < sourceFileSize);

	INFO_PRINTF2(_L("     Compression ratio: %d%%"), 100 - (100 * totalCompressed / totalUncompressed));
	INFO_PRINTF1(_L("  09 Test  << PASS >>:"));
	//end of the loop

	//-------------- substep 10 --------------------
	INFO_PRINTF1(_L("  10 Free chain. Clean up stack:"));
	CleanupStack::PopAndDestroy(sourceBuffer);
	sourceFile.Close();
	aBufQ.Free();
	aChain.Free();
	UnloadCompressor();
	UnloadDeCompressor();
#ifdef SYMBIAN_ZERO_COPY_NETWORKING	
	iCommsBufPond.Close();
#else
	delete iMBMngr;
#endif 	// SYMBIAN_ZERO_COPY_NETWORKING	

	iFs.Close();
	CleanupStack::PopAndDestroy(stepSched);

	return TestStepResult();

} // PPPCompressDecompressTest1

