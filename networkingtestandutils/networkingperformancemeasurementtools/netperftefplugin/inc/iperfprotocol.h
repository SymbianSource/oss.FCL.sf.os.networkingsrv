// iperfprotocol.h
//
// Portions Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
//

/*
 * Copyright (c) 1999-2007, The Board of Trustees of the University of Illinois
 * All Rights Reserved.
 * 
 * Iperf performance test
 * Mark Gates
 * Ajay Tirumala
 * Jim Ferguson
 * Jon Dugan
 * Feng Qin
 * Kevin Gibbs
 * John Estabrook
 * National Laboratory for Applied Network Research 
 * National Center for Supercomputing Applications 
 * University of Illinois at Urbana-Champaign 
 * http://www.ncsa.uiuc.edu
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software (Iperf) and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimers in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * - Neither the names of the University of Illinois, NCSA, nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   Software without specific prior written permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * CONTIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */

/**
 * @file
 * 
 * IPerf protocol. Talks the same language as remote PC-based iperf to compare
 *  sequence numbers, delays, metrics etc. 
 *
 * @internalTechnology
 */

#ifndef __IPERF_PROTOCOL_H
#define __IPERF_PROTOCOL_H

#include "iperfudpheader.h"
#include "iperfclientheader.h"
#include "iperfserverheader.h"

enum TSessionType
    {
    ENone = 0,              // session has not been initialised yet
    EClient,                // synonymous with upload test - also consistent with iperf nomenclature
    EServer,                // synonymous with download test - also consistent with iperf nomenclature
    };

enum EDisassembleResult
    {
    // ETypeAccepted...
    EValidInSeq = 0,        // valid, in sequence (if layer does not support sequencing, then it is assumed to be in sequence anyhow)
    EValidOutOfSeq,         // valid, but out of sequence (layer must support sequence numbering)

    // ETypeIgnored...
    EValidNotAddressee,     // valid, but not addressed to us
    EValidDuplicate,        // valid, but duplicate (layer must support sequence numbering)
    EInvalid,               // invalid, badly formatted protocol
    EUnsupported,           // validity indeterminate, valid enough to detect unsupported feature(s) but unable to validate after that

    // ETypeLost...
    ELost,                  // validity n/a, a message (of indeterminate length) has been lost (layer must support sequence numbering) 

    ENumDisassembleResults, // number of disassembly results
    };


class CIperfProtocol
	{
public:	

	static TInt AssembleIperfHeader(TDes8& aPacket, TInt aClientListeningPort, TInt aThroughputInKbps);
	static TInt UpdateIperfHeader(TDes8& aPacket,TIperfUdpHeader::TMsg aMsg, TInt aSeqNum);


	static TInt DisassembleIperf(TDes8 &aPacket, TSessionType aType,
						TIperfUdpHeader::TMsg &aMsg, TInt& aSeqNum);
	static EDisassembleResult DisassembleIperf(TIperfClientHeader &aClient, TInt aLength);
	static EDisassembleResult DisassembleIperf(TIperfServerHeader &aClient);

	};



#endif //ifndef __IPERF_PROTOCOL_H



