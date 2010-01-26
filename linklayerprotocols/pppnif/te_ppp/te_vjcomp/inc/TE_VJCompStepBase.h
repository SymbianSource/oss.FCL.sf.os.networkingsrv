/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* This defines the TE_VJCompStepBase class which is the base class for all
* the VJComp test step classes
* 
*
*/



/**
 @file TE_VJCompStepBase.h
*/

#if (!defined __TE_VJCOMP_STEP_BASE_H__)
#define __TE_VJCOMP_STEP_BASE_H__

#include <test/testexecutestepbase.h>
#include "TE_VJComp.h"
#include "filepcap.h"

#include <e32std.h>
#include "VJ.H"
#include "VJIF.H"
#include "NCPIP.H"
#include "PPPCFG.H"
#ifdef SYMBIAN_ZERO_COPY_NETWORKING
#include <comms-infras/commsbufpondop.h>
#else
#include <es_mbman.h>
#endif // SYMBIAN_ZERO_COPY_NETWORKING

#include <cflog.h>

extern "C" {
	#include "VJRefComp.h"
	}

// Test Step Defaults
#define PACKET_SIZE		512
#define	MAX_PACKET_SIZE 2000
#define NUM_OF_PACKETS	20

_LIT(KHdrLengthString,"    Length = %d, Hdr len = %d");
_LIT(KChangeMaskString,"    Change Mask = 0x%x: ");
_LIT(KSrcDstAddrString,"	Src = %d.%d.%d.%d, Dst = %d.%d.%d.%d");
_LIT(KIDFragmentString,"	Id = 0x%04x, Fragment = %d %s%s%s");
_LIT(KConnectionString,"    Connection = 0x%x");
_LIT(KConnectionNoString,"    Connection number [0x%02x]");
_LIT(KChecksumString,"    Checksum = 0x%x");
_LIT(KTCPLengthString,"    Length = %d, Hdr len = %d");
_LIT(KPortString,"    Src port = %d, Dst port = %d");
_LIT(KSeqAckString,"    Seq = 0x%08x, Ack = 0x%08x");
_LIT(KFlagsString,"    Flags = 0x%04x (%s%s%s%s%s%s%s)");
_LIT(KWindowUrgentString,"    Window = %d, Urgent = %d");
_LIT(KChecksumString3,"    Chksum = 0x%04x (0x%04x) !!!");
_LIT(KCodeTextString,"    %s [0x%02x]");
_LIT(KTOSTTLChksumString,"    TOS = 0x%02x, TTL = %d, Chksum = 0x%04x");
_LIT(KReservedString, "    Reserved bits = 0x%04x");


class CTE_VJCompStepBase : public CTestStep
{
public:
	CTE_VJCompStepBase();
	~CTE_VJCompStepBase();
	void StartMBufL();
	void UnloadVJCompressor(void);
	void UnloadVJDeCompressor(void);
    void DumpCompressedPacket(TPtrC8 outputPtrC, TInt compRet);
    TBool CompareOutput(TPtrC8 aPtr1, TPtrC8 aPtr2);
    TBool TrimPacketBeforeCompression();
    TUint TrimPacketBeforeDecompression();
    TInt MapToVjCompRet (TUint refcompRet);
    virtual void ProcessPacketL()=0;
    TInt DumpIp(TPtrC8 aDes);
    void CompressDecompressL(TBool compareVJandRef);
    void IsItATossPacket(TBool vjdecompRet, TUint8 * refdecompOutput);


private:
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	const TText* IpProtocolToText(TUint aValue);
	TUint16 DecodeDelta(TUint8 * & aPtr);
	TInt16 DecodeSignedDelta(TUint8 * & aPtr);
	TInt DumpVjUncompTcp(TPtrC8 aDes);
    TInt DumpVjCompTcp(TPtrC8 aDes);
    TInt DumpTcp(TPtrC8 aDes, TUint32 aSrcA, TUint32 aDstA, TInt aLength);


protected:
    CPppNcpIp*         iPppNcpIp;
    CVJDeCompressorIf  *iVJDeCompressor;
    CVJCompressorIf    *iVJCompressor;
    CObjectCon         *iVJCompressorCon;
    CObjectCon         *iVJDeCompressorCon;
#ifdef SYMBIAN_ZERO_COPY_NETWORKING
	RCommsBufPondOp      iCommsBufPond;
#else
	CMBufManager       *iMBMngr;
#endif // SYMBIAN_ZERO_COPY_NETWORKING
	RFs                iFs;
	CFilePcap          *iFilepcap;
	TPcapRecord        *iRec;
	RMBufChain         iPacket;
	TBool              iDebug;
	struct slcompress  *iSlcomp;
	struct slcompress  *iComp;
    __CFLOG_DECLARATION_MEMBER;
};


#endif /* __TE_VJCOMP_STEP_BASE_H__ */

