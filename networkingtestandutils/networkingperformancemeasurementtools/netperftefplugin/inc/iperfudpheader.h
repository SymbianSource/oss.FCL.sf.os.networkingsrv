// iperfudpheader.h
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
 * IPerf UDP header declaration.
 *
 * @internalTechnology
 */

#ifndef IPERF_UDP_H
#define IPERF_UDP_H

#include <e32def.h>
#include <ss_std.h>


const TUint KProtocolIperf = 0xace;     // arbitrary protocol id, since iPerf isn't an officially recognized OSI protocol (ie. no unique id)
// iperf protocol header
// - for simplicity (& because most elements are int32) the header has been modelled as a structure instead of the 'typical' array of chars
// - if this class is overlaid atop of existing data (eg. via a pointer to this class), then you will need to ensure the data is aligned as 
//   req'd by the target platform/cpu
// - format;
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |     sequence number (udp)     |     time in seconds (udp)     |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |    time microseconds (udp)    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+rcmdrr
class TIperfUdpHeader
    {
public:
    inline void Init() {};  // init common values, deliberately not relying on constructor so that it can be explicitly invoked for pointer objects
    //inline void hton() {TInetAddrEx::hton((TUint *) this, sizeof(TIperfUdpHeader));};  // host to network order conversion
    //inline void ntoh() {TInetAddrEx::ntoh((TUint *) this, sizeof(TIperfUdpHeader));};  // network to host order conversion

    enum TMsg               // iperf message type
	    {
        EUnknown,           // unknown msg type
        EData,              // client msg; normal data send msg
	    EFinishRequest,     // client msg; request to finish, ie. negative sequence number
        EFinishResponse,    // server msg; reply to a prior request to finish
	    };

    // public member data - defined in same order as per the header (refer above)
    TInt32  iSequenceNum;   // udp only; datagram sequence number, aka "id". -ve number indicates client finish request
    TUint32 iTimeS;         // udp only; seconds from 1970 (time_t) and also GMT (which alas is not specified by time_t, but is typically implied)
    TUint32 iTimeMicroS;    // udp only; microseconds

    void SetTime();         // set current time
    inline static TInt HeaderLength() {return sizeof(TIperfUdpHeader);}   // 12
    };

#ifdef __FLOG_ACTIVE
    _LIT8(KLoggingMinorTagIperf,  "iperf");
#endif

#endif // IPERF_UDP_H

