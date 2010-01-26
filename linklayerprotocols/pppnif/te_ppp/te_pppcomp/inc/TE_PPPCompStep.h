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
// This defines the TE_PPPCompStep class which is the base class for all
// the PPPComp test step classes
// 
//

/**
 @file
*/

#if (!defined __TE_PPPCOMP_STEP_H__)
#define __TE_PPPCOMP_STEP_H__

#include <test/testexecutestepbase.h>
#include "TE_PPPComp.h"

#include <e32std.h>
#include "PPPCOMP.H"
#include "PPPCFG.H"
#include "pppCcp.h"
#ifdef SYMBIAN_ZERO_COPY_NETWORKING
#include <comms-infras/commsbufpondop.h>
#else
#include <es_mbman.h>
#endif // SYMBIAN_ZERO_COPY_NETWORKING
#include <cflog.h>

// Test Step Defaults
#define PACKET_SIZE		512
#define	MAX_PACKET_SIZE 2000
#define NUM_OF_PACKETS	20

const TInt KPppCompressionChunkMinimumSize = 100;
const TInt KPppCompressionChunkMaximumSize = 5*KPppDefaultFrameSize;


class CTE_PPPCompStep : public CTestStep
{
public:
	CTE_PPPCompStep();
	virtual ~CTE_PPPCompStep();
	void StartMBufL();
	void GetCompressionTypeFromConfig();
	void UnloadCompressor(void);
	void UnloadDeCompressor(void);
	void StoreCompressedDataIntoNewFile(const TPtrC& aDestFileNamePtr, TPtr8& aSourcePtr );
	TBool IsDeCompressedBufferIdenticalToReference(RMBufQ& aBufferQ,
			TPtr8& aReference, TPtrC& aDestFileNamePtr);

private:
	TVerdict doTestStepPreambleL();
	TBool ReadCompressorInfo(CPppCompConfig* aData, TUint aNumberOfTurns);

protected:
	CPppCcp*		iPppCcp;
	TSglQue<CPppCompConfig> iCompressorConfig;
	CObjectCon*			iCompressorCon;
	CObjectCon*			iDeCompressorCon;
	CPppCompressor* iCompressor;
	CPppDeCompressor* iDeCompressor;
#ifdef SYMBIAN_ZERO_COPY_NETWORKING	
	RCommsBufPondOp	iCommsBufPond;
#else
	CMBufManager*	iMBMngr;
#endif // SYMBIAN_ZERO_COPY_NETWORKING	
	RFs              iFs;
	TPtrC iPtrCompType;
	CPppCompConfig	iPppCompConfig;
    __CFLOG_DECLARATION_MEMBER;
};

class CPPPCompressTest1 : public CTE_PPPCompStep
{
public:
	CPPPCompressTest1();
	virtual ~CPPPCompressTest1();
	virtual TVerdict doTestStepL();
};


class CPPPDeCompressTest1 : public CTE_PPPCompStep
{
public:
	CPPPDeCompressTest1();
	virtual ~CPPPDeCompressTest1();
	virtual TVerdict doTestStepL();
};

class CPPPCompressDecompressTest1 : public CTE_PPPCompStep
{
public:
	CPPPCompressDecompressTest1();
	virtual ~CPPPCompressDecompressTest1();
	virtual TVerdict doTestStepL();
};


_LIT(KPPPCompTest,"PPPCompressTest1");
_LIT(KPPPDecompTest,"PPPDecompressTest1");
_LIT(KPPPCompDecompTest,"PPPCompressDecompressTest1");

#endif /* __TE_PPPCOMP_STEP_H__ */
