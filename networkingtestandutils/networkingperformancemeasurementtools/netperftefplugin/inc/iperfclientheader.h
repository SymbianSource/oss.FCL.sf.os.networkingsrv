// iperfclientheader.h
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
 * IPerf client header declaration.
 * Contains sequence numbers, delays, metrics etc. 
 *
 * @internalTechnology
 */

#ifndef IPERF_CLIENT_H
#define IPERF_CLIENT_H

#include "iperfudpheader.h"
#include <e32def.h>
#include <ss_std.h>
#include "inetaddrex.h"     // for TInetAddrEx


// iperf client protocol header
// - for simplicity (& because all elements are int32) the header has been modelled as a structure that is intended to be overlayed atop
//   of the protocol msg, ie. instead of the 'typical' array of chars (eg. as used by tcpip6)
// - if this class is overlaid atop of existing data (eg. via a pointer to this class), then you will need to ensure the data is aligned as 
//   req'd by the target platform/cpu
// - format;
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           iperf udp                           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |    iperf udp (cont)           |            flags              |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |     number of connections     |     client listening port     |
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |       datagram length         |      desired throughput       |
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |        test duration          |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class TIperfClientHeader : public TIperfUdpHeader
    {
public:
    inline void Init();                // init common values, deliberately not relying on constructor so that it can be explicitly invoked for pointer objects
    inline void hton() {TInetAddrEx::hton((TUint32 *) this, sizeof(TIperfClientHeader));};  // host to network order conversion
    inline void ntoh() {TInetAddrEx::ntoh((TUint32 *) this, sizeof(TIperfClientHeader));};  // network to host order conversion
    inline static TInt HeaderLength() {return sizeof(TIperfClientHeader);}    // 24 + 12 = 36

    // public member data - defined in same order as per the header (refer above)
    TUint32 iFlags;             // Iperf code & documentation does not adequately describe the purpose of these flags, the comments from the Iperf code
                                // are copied verbatim below with the hope that they will make more sense in the future!...
                                // "flags is a bitmap for different options the most significant bits are for determining
                                //  which information is available. So 1.7 uses 0x80000000 and the next time information is added
                                //  the 1.7 bit will be set and 0x40000000 will be set signifying additional information. If no 
                                //  information bits are set then the header is ignored. The lowest order diferentiates between dualtest and
                                //  tradeoff modes, wheither the speaker needs to start immediately or after the audience finishes."
    TUint32 iNumConnections;    // number of active socket connections, this is ambiguously referred to as threads by Iperf and thus renamed in the code here
    TUint32 iClientListeningPort;// server-->client = 0, client-->server = specifies listening port for server to connect back to
                                // yuk! this is very bad as a server should *never* attempt a connection to a client, eg. won't get through nat router
    TUint32 iDatagramLen;       // length of datagram (ie. including iperf header). don't exceed MSS! eg. udp over ethernet; mss = 1500 - 20 - 8 = 1472
    TUint32 iThroughput;        // desired throughput (ie. @ link layer or physical layer?) in b/s
    TInt32  iDuration;          // duration of test (in seconds), represented as a negative number (2's complement)
    };

inline void TIperfClientHeader::Init()
    {
    TIperfUdpHeader::Init();
	iFlags = 0;
	iNumConnections = 1;
    }

#endif // IPERF_CLIENT_H
