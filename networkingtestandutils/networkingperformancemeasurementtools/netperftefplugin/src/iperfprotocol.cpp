// iperfprotocol.cpp
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

#include "iperfprotocol.h"
#include "iperfclientheader.h"
#include "iperfserverheader.h"


TInt CIperfProtocol::DisassembleIperf(TDes8 &aPacket, TSessionType aType,
						TIperfUdpHeader::TMsg &aMsg, TInt& aSeqNum)
    {
    EDisassembleResult result = EValidInSeq;

	TIperfServerHeader* iperf = (TIperfServerHeader*) aPacket.Ptr();

    // validate specifics - because protocol doesn't specify msg type, we assume it from the context of the active session type
    TInt len = 0;
    if (!result)
        {
        if (aType == EClient)
            {
            aMsg = TIperfUdpHeader::EFinishResponse;
            len = TIperfServerHeader::HeaderLength();
            if (!result && aPacket.Length() < len)
                result = EInvalid;         
            if (!result)
                result = DisassembleIperf(*(TIperfServerHeader *) iperf);
            
            // since this is a client session (ie. uploading), download sequence number not yet set... init here for sake of consitency/clarity
            aSeqNum = iperf->iSequenceNum;
            }
        else if (aType == EServer)
            {
            aMsg = TIperfUdpHeader::EData;  // assume it is a data msg, after sequence number checking may revise to a finish request
            len = TIperfClientHeader::HeaderLength();
            if (!result && aPacket.Length() < len)
                result = EInvalid;
            if (!result)
                {
                result = DisassembleIperf(*(TIperfClientHeader *) iperf, aPacket.Length());
                }
            }
        else
            {
            // ENone - we are receiving iperf messages w/o an active session
            result = EUnsupported;
            }
        }
    
#if 0
    // validate sequence number
    // - only for server sessions (ie. download)
    // - can't (easily) skip validation (ie. as an optimisation option) since sequence number is used to detect end of session request
    TUint numLostDatagrams = 0;
    if (!result && aType == EServer)
        {
        __FLOG_VA((_L8(" type=%d rxSeq=%d nextSeq=%d"), iSession.iType, iperf->iSequenceNum, iSession.iDownload.iIperfNextSeqNum));

        if (iperf->iSequenceNum < 0)
            {
            // scenario #1 - datagram with sequence number indicating end of session, still need to check for lost datagrams

            // negate sequence number the first time we receive a negative seq number
            if (iSession.iDownload.iIperfNextSeqNum >= 0)
                iSession.iDownload.iIperfNextSeqNum = -iSession.iDownload.iIperfNextSeqNum;   
            
            // known iperf PC defect (refer [2]), whereby the negative sequence number is not always <= -(last seq. number + 1)
    	    // - following is a workaround that silently absorbs this defect, whereby we force the sequence number to be in sequence
            if (iperf->iSequenceNum > iSession.iDownload.iIperfNextSeqNum)
                iperf->iSequenceNum = iSession.iDownload.iIperfNextSeqNum;
            
            // check & handle lost/oos datagrams
            if (iperf->iSequenceNum < iSession.iDownload.iIperfNextSeqNum)
                {
                numLostDatagrams = iSession.iDownload.iIperfNextSeqNum - iperf->iSequenceNum ;
                __FLOG_VA((_L8(" lost iperf detected, rxSeq=%d nextSeq=%d numLost=%d"), iperf->iSequenceNum, iSession.iDownload.iIperfNextSeqNum, numLostDatagrams));

                result = ELost;
                } 
            else if (iperf->iSequenceNum == iSession.iDownload.iIperfNextSeqNum)
                result = EValidInSeq;               // expected sequence number, thus no loss
            else
                CMisc::Panic(EBadSequenceNumber);   // negative sequence numbers are static, thus can't have a stale sequence number

            // negative sequence numbers do not increment/decrement
            // - this is most unfortunate design oversight since we are thus unable to detect lost finish request messages :(
            iSession.iDownload.iIperfNextSeqNum = iperf->iSequenceNum;   
            aMsg = TIperfUdpHeader::EFinishRequest;
            }
        else if (iperf->iSequenceNum == iSession.iDownload.iIperfNextSeqNum)
            {
            // scenario #2 - datagram with in sequence number; 'normal' data
            iSession.iDownload.iIperfNextSeqNum = iperf->iSequenceNum + 1;
            result = EValidInSeq;
            }
        else if (iperf->iSequenceNum > iSession.iDownload.iIperfNextSeqNum)
            {
            // scenario #3 - datagram with future sequence number; frame(s) lost
            // proposal; refer [1]
            // - add sequence number to a lost datagram array, thus allowing subsequent out of sequence handling (ie. stale sequence numbers)
            numLostDatagrams = iperf->iSequenceNum - iSession.iDownload.iIperfNextSeqNum;
            __FLOG_VA((_L8(" lost iperf detected, rxSeq=%d nextSeq=%d numLost=%d"), iperf->iSequenceNum, iSession.iDownload.iIperfNextSeqNum, numLostDatagrams));

            iSession.iDownload.iIperfNextSeqNum = iperf->iSequenceNum + 1;
            result = ELost;
            }
        else if (iperf->iSequenceNum < iSession.iDownload.iIperfNextSeqNum)
            {
            // scenario #4 - datagram with stale sequence number; either duplicate or an out of order sequence number
            // proposal; refer [1]
            // - check lost datagram array... either;
            //   a) if exists, then remove, subtract from lost metrics, add to out of sequence metrics
            //   b) if !exists, add to duplicate metrics
            result = EValidOutOfSeq;
            }
        }
    
    // update session metrics
    // - for lost datagrams, explicitly invoked twice to account for implication that we have received a valid in sequence
    if (result == ELost)
       {
        UpdateSessionMetrics(ETrue, KProtocolIperf, result, 0, 0, numLostDatagrams);  // the lost length is indeterminate
        result = EValidInSeq;
        }
    result = (EDisassembleResult) UpdateSessionMetrics(ETrue, protocol, result, info->iLength, len);
#endif
    return result;
    }

EDisassembleResult CIperfProtocol::DisassembleIperf(TIperfClientHeader &aIperf, TInt aLength)
    {
//    __FLOG_VA((_L8("CProtocol::DisassembleIperf(TIperfClientHeader), rxLen=%d"), aLength));

    EDisassembleResult result = EValidInSeq;
    aIperf.ntoh();

    // validate client fields only - ie. common udp header is validated elsewhere
    // - following fields are deliberately not validated;
    //   a) desired throughput - refer proposal note below
    //   b) test duration - substituted with more accurate local timers instead
    //   c) client listening port - this is a stupid concept whereby server (us) can connect back to client that is not supported but always set
    
    // proposal; refer [1]
    // - report client's intended upstream desired throughput to SAP so it can be used by to report efficiency w.r.t. desired throughput
    //   (ie. in addition to maximum capacity)
    // - will be useful metric in determing whether we are capable of accepting increased bit rate from the remote client
    if (!result && aIperf.iDatagramLen != aLength)
        result = EInvalid;
    else if (!result && (aIperf.iFlags != 0 || aIperf.iNumConnections != 1))
        result = EUnsupported;
    return result;
    }

// disassemble iperf server msg
EDisassembleResult CIperfProtocol::DisassembleIperf(TIperfServerHeader &aIperf)
    {
//    __FLOG_VA((_L8("CProtocol::DisassembleIperf(TIperfServerHeader)")));

    EDisassembleResult result = EValidInSeq;
    aIperf.ntoh();

    // validate server fields only - ie. common udp header is validated elsewhere
    // - following fields are deliberately not validated;
    //   a) desired throughput - refer proposal below
    //   b) test duration - substituted with more accurate local timers instead
    //   c) client listening port - this is a stupid concept whereby server (us) can connect back to client that is not supported but always set
    if (!result && (aIperf.iFlags != TIperfServerHeader::KFinishFlag))
        result = EUnsupported;
    // proposal; refer [1]
    // - report client's intended upstream desired throughput to SAP so it can be used by to report efficiency w.r.t. desired throughput
    //   (ie. in addition to maximum capacity)
    // - will be useful metric in determing whether we are capable of accepting increased bit rate from the remote client
#if 0
rob - maybe store these metrics for the ReportL step but nothing more fancy than that 
    // merge upload session metrics
    // - due to the intrinsic nature of sent/uploaded data, our generated upload session metrics are not able to detect if a datagram was
    //   actually received in sequence, ouf of sequence, lost, etc or even sent!. fortunately, this information is available in the 
    //   iperf server response message, thus we merge this data into our upload metrics here
    // - we are only interested in a subset of the server supplied metrics, as some of these have either already been generated/accounted
    //   for internally or can be deduced (eg. received = sent - lost)
    // - length fields are NOT updated;
    //   a) server response message only indicates the total received length, and not individual length totals for each of the counts,
    //      thus the length values can not be updated.  in theory, we could make a best guess based on mtu length (only known by SAP), but
    //      deliberate decision not to do this; "best not to guess"
    //   b) non-valid datagrams are very visible - marked up as noise, lost, etc.
    //   c) higher tier protocol lengths would need to be updated too - indeterminate as we don't know how big the datagram was
    //   d) iperf server response excludes metrics for the finish request - unable to fudge the length
    //   e) pointless, since the test needs to be re-run (eg. at slower speed) - until no (or negligible) invalid or missing datagrams are sent
    // - metrics for iperf's lower tier protocols are deliberately not updated because;
    //   a) with the exception of lost datagrams, the protocol had been correctly processed by all of the lower tiers
    //   b) lost datagrams - indeterminate what protocol tier the datagram had reached... "best not to guess"
    // - following (potentially) lengthy calculations does not need performance optimisation since the time sensitive metrics have 
    //   already ceased (during the finish request message)
    if (iSession.iMetricsEnabled)
        { 
        // update metrics 
        iSession.iUpload.iAverageDelayJitterUs = aIperf.iReceivedDelayJitter;
        UpdateSessionMetrics(EFalse, KProtocolIperf, EValidOutOfSeq, 0, aIperf.iReceivedOutOfSeq *TIperfServerHeader::HeaderLength(), aIperf.iReceivedOutOfSeq);
        UpdateSessionMetrics(EFalse, KProtocolIperf, EInvalid, 0, aIperf.iReceivedErrors * TIperfServerHeader::HeaderLength(), aIperf.iReceivedErrors);

        // lost metrics - unfortunately;
        // a) not reported by remote server, but easily calculated
        // b) known iperf defect (refer [2]), whereby remote iperf server pretends to of received initial missing datagrams (detected by 
        //    sequence number), even when it hasn't!
        TInt totalLen, protocolLen, datagramCount;
        GetSessionMetrics(EFalse, KProtocolIperf, EValidInSeq, totalLen, protocolLen, datagramCount);
        TUint lostCount = datagramCount - aIperf.iReceivedDatagrams - 1;    // -1 to account for the finish request
        UpdateSessionMetrics(EFalse, KProtocolIperf, ELost, 0, lostCount * TIperfServerHeader::HeaderLength(), lostCount);

        // revise "in sequence" datagrams
        // - count adjustment is accurate
        // - length adjustment may not be accurate, since only the lost length is (indirectly) supplied by the server response message (refer notes above)
        TInt nonInSeqCount = aIperf.iReceivedOutOfSeq + aIperf.iReceivedErrors + lostCount;
        UpdateSessionMetrics(EFalse, KProtocolIperf, EValidInSeq, 0, -nonInSeqCount * TIperfServerHeader::HeaderLength(), -nonInSeqCount);

        // mark metrics as dirty if we know there is a possibility that the metrics are not 100% accurate - ie. the length fields
        if (nonInSeqCount != 0)
            iSession.iUpload.iMetricsDirty = ETrue;
        }
#endif
    return result;
    }





















TInt CIperfProtocol::AssembleIperfHeader(TDes8& aPacket, TInt aClientListeningPort, TInt aThroughputInKbps)
	{
	TIperfClientHeader* iperf = (TIperfClientHeader*) aPacket.Ptr();
	iperf->Init();

    iperf->iClientListeningPort = aClientListeningPort; // a stupid concept to allow server to connect to client, but set just in case (assigned to local port)
    iperf->iDatagramLen = aPacket.Length();	// segment size (ideally MSS), eg. 802.3 (minimum udp & ip framing) = 1472
    iperf->iThroughput = aThroughputInKbps; // required throughput (B/s), not currently supported (link capacity used instead)
    iperf->iDuration = 0;                   // set to zero since this test will terminate based on transferred size & not time elapsed
    iperf->hton();
    
    return KErrNone;
	}

TInt CIperfProtocol::UpdateIperfHeader(TDes8& aPacket,TIperfUdpHeader::TMsg aMsg, TInt aSeqNum)
	{
	TIperfClientHeader* iperf = (TIperfClientHeader*) aPacket.Ptr();

	if (aMsg == TIperfUdpHeader::EFinishRequest && aSeqNum >= 0)
		{
		aSeqNum = -aSeqNum;
		}

	iperf->iSequenceNum = TInetAddrEx::hton((TUint32)aSeqNum);

	iperf->SetTime();

	return KErrNone;
	}


