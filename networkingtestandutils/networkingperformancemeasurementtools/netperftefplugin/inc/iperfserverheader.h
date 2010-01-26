// iperfserverheader.h
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
 * IPerf server header declaration.
 * In keeping with standard client/server communications architecture terminology,
 * server == receiver of data.
 *  This is also consistent with the remote PC based jperf/iperf implementation.
 *
 * @internalTechnology
 */

#ifndef IPERF_SERVER_H
#define IPERF_SERVER_H

#include "iperfudpheader.h"
#include <e32def.h>
#include <ss_std.h>
#include "inetaddrex.h"     // for TInetAddrEx
//#include "es_nifte.h"       // for Knifte8 logging tag


// iperf server protocol header
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
// |                    total received length                      |
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |       test duration (S)       |     test duration (microS)    |
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |   number of received errors   | number of received out of seq |
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// | number of received datagrams  |     received delay jitter     |
//   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |  received delay jitter (cont) |    
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class TIperfServerHeader : public TIperfUdpHeader
    {
public:
    inline void Init();         // init common values, deliberately not relying on constructor so that it can be explicitly invoked for pointer objects
    inline void hton();
    inline void ntoh() {hton();}; // network to host order conversion
    inline static TInt HeaderLength() {return sizeof(TIperfServerHeader);}    // 40 + 12 = 52

    // public member data - defined in same order as per the header (refer above)
    TUint32 iFlags;             // refer TIperfClientHeader::iFlags comments
    TUint64 iReceivedLength;    // total length of datagrams received in B, ie. including iperf overhead, but excluding underlying protocols
    TUint32 iDurationS;         // test duration, seconds component
    TUint32 iDurationMicroS;    // test duration, microseconds component
    TUint32 iReceivedErrors;    // number of datagrams received in error, eg. unsupported features, etc
    TUint32 iReceivedOutOfSeq;  // number of datagrams received out of received sequence
    TUint32 iReceivedDatagrams; // number of datagrams received in sequence(?), should match the highest sequence number received if no out of order
    TUint64 iReceivedDelayJitter;// calculated delay jitter of received data

    static const TUint KFinishFlag = 0x80000000;
    };

inline void TIperfServerHeader::Init()
    {
    TIperfUdpHeader::Init();
	iFlags = 0x80000000;
    }

// host to network order conversion
inline void TIperfServerHeader::hton() 
    {
    TInetAddrEx::hton((TUint32 *) this, sizeof(TIperfUdpHeader));

    // convert server fields - done individually since they are not all 32bit fields
    iFlags               = TInetAddrEx::hton(iFlags);
    iReceivedLength      = TInetAddrEx::hton(iReceivedLength);
    iDurationS           = TInetAddrEx::hton(iDurationS);
    iDurationMicroS      = TInetAddrEx::hton(iDurationMicroS);
    iReceivedErrors      = TInetAddrEx::hton(iReceivedErrors);
    iReceivedOutOfSeq    = TInetAddrEx::hton(iReceivedOutOfSeq);
    iReceivedDatagrams   = TInetAddrEx::hton(iReceivedDatagrams);
    iReceivedDelayJitter = TInetAddrEx::hton(iReceivedDelayJitter);
    }

#endif // IPERF_SERVER_H
