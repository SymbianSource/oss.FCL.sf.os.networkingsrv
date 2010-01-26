// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// tcpip_ini.h - INI File literals
//

#ifndef __TCPIP_INI_H__
#define __TCPIP_INI_H__

/**
* @file tcpip_ini.h
* @ingroup tcpip_ini_parameters
* Define TCPIP.INI configuration parameters
* @internalComponent
*/

/**
* @defgroup tcpip_ini_parameters	TCPIP.INI configuration parameters
*
* @{
*/

//
// INI File literals
// *****************
//

/** The name of the ini-file */
_LIT(TCPIP_INI_DATA,                "tcpip.ini");

/**
* @name	Start Section
*
* When the protocol stack is loaded into memory, it can automatically start
* a number of system daemons. The DNS resolver daemon (dnd.exe) is an example
* of such system damoens. When the stack is unloaded, any running daemons are
* automatically killed.
*
* Each daemon has its own section in the TCPIP.INI file. The daemons that
* should be started up are listed in the [start] section under the daemons
* key, separated by commas. Each daemon specific section contains the
* initialization parameters for the daemon. Currently, the only supported
* parameter key is filename, which specifies the location of the executable.
* For example:
@verbatim
[start]
daemons= dnd,httpserver

[dnd]
filename= z:\system\programs\dnd.exe

[httpserver]
filename= d:\system\programs\httpserver.exe
@endverbatim
* @{
*/
/** The section name <tt>[start]</tt> */
_LIT(TCPIP_INI_START,               "start");
/** The daemons list. <tt>daemons= dnd,kmd</tt> */
_LIT(TCPIP_INI_DAEMONS,             "daemons");
/** The filename daemon. <tt>[dnd]<br>filename= dnd.exe</tt> */
_LIT(TCPIP_INI_FILENAME,            "filename");
/** @} */

/**
* @name Network layer parameters
* @{
*/
/** The section name <tt>[ip]</tt> */
_LIT(TCPIP_INI_IP,                  "ip");		    // [ip]
/**
* Default TTL (time to live) value for IP packets.
*
* Default: #KTcpipIni_Maxttl
*/
_LIT(TCPIP_INI_MAXTTL,              "maxttl");	            // maxttl= 69
/**
* Default TTL for link local destinations.(IPv4 and IPv6).
* If negative, then default is same as <tt>maxttl</tt>.
*
* - Default: #KTcpipIni_LinkLocalttl
* - Range: [-1..255]
*/
_LIT(TCPIP_INI_LINKLOCALTTL,		"linklocalttl");
/**
* Maximum number of simultaneous dial attempts via NifMan.
*
* - Default: #KTcpipIni_Maxdials
* - Range: >= 0
* @deprecated Not used
* @since 7.0
*/
_LIT(TCPIP_INI_MAXDIALS,            "maxdials");
/**
* Packet forwarding.
* - Default: #KTcpipIni_Forward
* - Range: 0 = disabled, > 0 = enabled, and the number of cached flows to use.
*/
_LIT(TCPIP_INI_FORWARD,             "forward");
/**
* Enable IPv4 link-local configuration.
* When set, the stack will autoconfigure interface for IPv4 linklocal
* address space (<tt>169.254.0.0/16</tt>). This happens in addition to
* any other configuration if present unless option 3 is specified. Possible
* values and their meanings:
*
* Default: #KTcpipIni_Ipv4Linklocal
*
* Valid values:
* - 0= No IPv4 link-local addresses,
* - 1= Always use IPv4 link-local addresses,
* - 2= Only use IPv4 link-local addresses when no global address has been configured,
* - 3= Use link-local address if no IPv4 address is read from Nif or configuration daemon (e.g., DHCP).
*/
_LIT(TCPIP_INI_IPV4LINKLOCAL,       "ipv4linklocal");
/**
* Probe addresses without route from all links.
* If set to 1, destination addresses for which no route can be found,
* are probed with ND (ARP for IPv4) on all interfaces within scope.
* This setting is ignored if disabled if the source address is an IPv4
* link local or else the link local would not function except for
* neighbours with routes already discovered and cached.  This is
* necessary for compliance with the ZEROCONF RFC.
*
* - Default: #KTcpipIni_ProbeAddress
* - Range: 0=disabled, 1=enabled
*/
_LIT(TCPIP_INI_PROBEADDRESS,		"probeaddress");
/**
* No id defence.
*
* Normally the node tries to keep assigned ID reserved for this node,
* regardless of the prefix part. Enabling this option, disables this
* feature (and allows other hosts use my id with different prefixes).
*
* - Default: #KTcpipIni_NoDefendId
* - Range: 0 = disabled (defend id), 1 = enabled (do not defend id).
*/
_LIT(TCPIP_INI_NODEFENDID,			"nodefendid");
/**
* No interface error to attaced flows.
*
* This is the default value for new sockets (flows).
* The default can be changed by a socket option
* #KSoNoInterfaceError.
*
* - Default: #KTcpipIni_Noiferror
* - Range: 0= pass, 1= don't pass errors to flow
*/
_LIT(TCPIP_INI_NOIFERROR,			"noiferror");
/**	
* Max time to wait for route or interface.
*
* - Default: #KTcpipIni_Maxholdtime
* - Range: [0..maxint]
*/
_LIT(TCPIP_INI_MAXHOLDTIME,			"maxholdtime");
/**
* Shutdown delay.
* How long to wait until to kill daemons after last user (SAP/NIF) exits.
*
* - Default: #KTcpipIni_ShutdownDelay seconds.
* - Range: [0..maxint]
*/
_LIT(TCPIP_INI_SHUTDOWN_DELAY,		"shutdown_delay");
/**
* Keep interface up.
*
* This is the default value for new sockets (flows).
* The default can be changed by a socket option
* #KSoKeepInterfaceUp.
*
* When enabled (1), attaching the flow to the interface
* increments the flow count on the interface. This
* count is part of the process that controls how
* long idle interfaces are kept up.
*
* When disabled (0), the flow is not counted.
*
* - Default: #KTcpipIni_KeepInterfaceUp
* - Range: 0 = disabled, 1 = enabled
*/
_LIT(TCPIP_INI_KEEP_INTERFACE_UP,	"keepinterfaceup");
/**
* Max Fragment assemblies.
*
* Max number of simultaneous incomplete fragment assembly (currently per
* protocol version: IPv4 and IPv6). This is a safeguard against denial of
* service (DOS) attacks. Prevents overusing the heap space for the
* assembly control blocks.
*
* - Default: #KTcpipIni_FragCount
* - Range: [0..maxint]
*/
_LIT(TCPIP_INI_FRAG_COUNT,			"frag_count");
/**
* Max Fragment buffering.
*
* Total amount buffer space that can be allocated (RMBufs) for incomplete
* assemblies (per protocol version). This is a safeguard against denial of
* service (DOS) attacks. Prevents overusing the RMBuf pool for the
* packet content.
*
* - Default: #KTcpipIni_FragTotal (bytes)
* - Range: [0..maxint]
*/
_LIT(TCPIP_INI_FRAG_TOTAL,			"frag_total");
/**
* Destination cache.
* - Default: 0 (disabled)
* - Range: 0 = disabled,
*          1 = enabled / one entry per address,
*          2 = enabled / one entry per network
*
* @see MDestinationCache for more information.
*/
_LIT(TCPIP_INI_DSTCACHE,			"dstcache");
/**
* Lifetime of dest. cache entry.
*
* - Default: 600 (secs)
*/
_LIT(TCPIP_INI_DST_LIFETIME,		"dst_lifetime");
/**
* Max total size of dest. cache.
*
* - Default: 2048 (bytes)
*/
_LIT(TCPIP_INI_DST_MAXSIZE,			"dst_maxsize");
/**
* Max transmitted ICMP errors or echo replies within ca. 8 second timeslot.
*
* - Default #KTcpipIni_IcmpLimit
*/
_LIT(TCPIP_INI_ICMP_LIMIT,			"icmp_limit");
/**
* Route Information option.
*
* The type code for RouteInformation has not been defined by the
* IANA. This option allows enabling the option by specifying some
* non-zero type, which identifies the option in Router Avertisement
* message. (Experimental)
*
* - Default: 0 (not enabled)
*/
_LIT(TCPIP_INI_RA_OPT_ROUTE,		"ra_opt_route");
/**
* Recursive DNS Server Option (RDNSS)
*
* The type code for RouteInformation has not been defined by the
* IANA. This option allows enabling the option by specifying some
* non-zero type, which identifies the option in Router Avertisement
* message. (Experimental)
*
* - Default: 0 (not enabled)
*/
_LIT(TCPIP_INI_RA_OPT_RDNSS,		"ra_opt_rdnss");
/** @} */

/**
* @name TCP Parameters
* @{
*/
/** The section name <tt>[tcp]</tt> */
_LIT(TCPIP_INI_TCP,                 "tcp");
/**
* TCP maximum segment size.
* Upper limit is bounded by the network interface MTU.
*
* - Default: tcp_mss= 65535
* - Range: [0..65535]
*/
_LIT(TCPIP_INI_TCP_MSS,             "tcp_mss");
/**
* TCP receive buffer size.
* - Default: tcp_recv_buf= 8192
* - Range: [1024..65535]
*/
_LIT(TCPIP_INI_TCP_RECV_BUF,        "tcp_recv_buf");

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/**
* TCP receive maximum window size.
* - Default: tcp_recv_max_buf= 262144
* - Range: [131070-1073741823]
*/
_LIT(TCPIP_INI_TCP_RECV_MAX_WND,        "tcp_recv_max_wnd");
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

/**
* TCP send buffer size.
* - Default: tcp_send_buf= 8192
* - Range: [1024..65535]
*/
_LIT(TCPIP_INI_TCP_SEND_BUF,        "tcp_send_buf");
/**
* Minimum retransmission timeout (ms).
* - Default: tcp_min_rto= 1000
* - Range: 1000 ms
*/
_LIT(TCPIP_INI_TCP_MIN_RTO,         "tcp_min_rto");
/**
* Maximum retransmission timeout (ms).
* - Default: tcp_max_rto= 60000
* - Range: 60000 ms
*/
_LIT(TCPIP_INI_TCP_MAX_RTO,         "tcp_max_rto");
/**
* Initial retransmission timeout (ms).
* - Default: tcp_initial_rto= 3000
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_INITIAL_RTO,     "tcp_initial_rto");
/**
* SRTT smoothing.
*
* alpha = 1 /  tcp_srtt_smooth, as defined in RFC2988.
* - Default: tcp_srtt_smooth= 8
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_SRTT_SMOOTH,     "tcp_srtt_smooth");
/**
* MDEV smoothing.
* beta = 1 /  tcp_rttvar_smooth, as defined in RFC2988.
* - Default: tcp_rttvar_smooth= 4
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_RTTVAR_SMOOTH,   "tcp_rttvar_smooth");
/**
* G value in RTO calculation (RFC2988).
* Constant representing clock granularity.
* - Default: machine specific.
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_RTO_G,           "tcp_rto_g");
/**
* K value in RTO calculation (RFC2988).
* K constant, RTO = SRTT + max(K * RTTVAR, G)  (RFC2988)
* - Default: tcp_rto_k= 4
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_RTO_K,           "tcp_rto_k");
/**
* Maximum retransmit burst size
* - Default: tcp_max_burst= 2
* - Range: [1..]
*/
_LIT(TCPIP_INI_TCP_MAX_BURST,       "tcp_max_burst");
/**
* Delayed acknowledgement delay time (ms).
* - Default: tcp_ack_delay= 200
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_ACK_DELAY,       "tcp_ack_delay");
/**
* Number of retransmissions per connect() attempt.
* - Default: tcp_syn_retries= 5
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_SYN_RETRIES,     "tcp_syn_retries");
/**
* Number of retransmissions before black hole diagnostics.
* Maximum retries before MTU reduction.
* - Default: tcp_retries1= 3
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_RETRIES1,        "tcp_retries1");
/**
* Maximum number of retransmissions.
* - Default: tcp_retries2= 12
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_RETRIES2,        "tcp_retries2");
/**
* MSL2 timeout (ms).
* TIME-WAIT & FIN-WAIT timeout.
* - Default: tcp_msl2= 60000
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_MSL2,            "tcp_msl2");
/**
* Large initial window (RFC2414).
* Set initial cwnd according to RFC2414
* - Default: .tcp_rfc2414= 1
* - Range: 1=enable, 0=disable.
*/
_LIT(TCPIP_INI_TCP_RFC2414,         "tcp_rfc2414");
/**
* Initial congestion window size (segments) when RFC2414 is disabled.
* - Default: tcp_initial_cwnd= 2
* - Range: [1..]
*/
_LIT(TCPIP_INI_TCP_INITIAL_CWND,    "tcp_initial_cwnd");
/**
* TCP timestamps (RFC1323).
* - Default: tcp_timestamps= 1
* - Range: 1=enable, 0=disable
*/
_LIT(TCPIP_INI_TCP_TIMESTAMPS,      "tcp_timestamps");
/**
* Enable selective acknowledgements (RFC2018).
* - Default: tcp_sack= 1
* - Range: 1=enable, 0=disable
*/
_LIT(TCPIP_INI_TCP_SACK,            "tcp_sack");
/**
* Apply TIME-WAIT state also to local connections.
* - Default: tcp_local_timewait= 0
* - Range: 1=enable, 0=disable
*/
_LIT(TCPIP_INI_TCP_LOCAL_TIMEWAIT,  "tcp_local_timewait");
/**
* Typical degree of packet reordering in the network.
* - Default: tcp_reordering= 3
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_REORDERING,      "tcp_reordering");
/**
* Variant of the nagle algorithm.
* - Default: tcp_strict_nagle= 0
* - Range: 0 = modern, 1 = strict
*/
_LIT(TCPIP_INI_TCP_STRICT_NAGLE,    "tcp_strict_nagle");
/**
* Limited transmit window size in segments (RFC3042).
* - Default: tcp_ltx_window= 2
* - Range: 0=disable, [1..]
*/
_LIT(TCPIP_INI_TCP_LTX_WINDOW,      "tcp_ltx_window");
/**
* Zero window probe style.
* - Default: tcp_probe_style= 0
* - Range:
*		- 0 = probe with a single byte (standard)
*		- 1 = probe with a full segment (BSD)
*		- 2 = probe with an out-of-window acknowledgement (experimental)
*/
_LIT(TCPIP_INI_TCP_PROBE_STYLE,     "tcp_probe_style");
/**
* Acknowledge pushed segments immediately.
*
* - Default: ?
* - Range: ?
*/
_LIT(TCPIP_INI_TCP_PUSH_ACK,        "tcp_push_ack");
/**
* F-RTO, also disables timestamp spike detection.
*
* If enabled, the TCP sender tries to probe with new previously
* unsent data segments after an RTO whether the RTO was spurious.
* If the RTO is detected spurious, the sender continues by sending
* new data instead of retransmitting.
* - Default: tcp_frto= 0
* - Range: 1 = enable, 0 = disable
*/
_LIT(TCPIP_INI_TCP_FRTO,	    "tcp_frto");
/**
* Send DSACKs for out-of-order segments.
* If enabled, send DSACKs in acknowledgements when receiving
* duplicate segments (RFC 2883). The data sender behaviour is
* not modified.
* - Default: tcp_dsack= 1
* - Range: 1 = enable, 0 = disable
*/
_LIT(TCPIP_INI_TCP_DSACK,	    "tcp_dsack");
/**
* Interval between keepalive probes.
* Delay for sending a TCP Keep-Alive probe after the connection
* has become idle (seconds). This is also the interval between
* succesful, replied keepalive probes.
* - Default: tcp_keepalive_intv= 7200 (= 2h)
* - Range: [0..]
*/
_LIT(TCPIP_INI_TCP_KEEPALIVE_INTV,  "tcp_keepalive_intv");
/**
* Number of unreplied probes before quitting.
* Number of unreplied TCP Keep-Alive probes to send
* before the sender gives up and terminates the connection.
* - Default: tcp_num_keepalives= 8
* - Range: [1..]
*/
_LIT(TCPIP_INI_TCP_NUM_KEEPALIVES,  "tcp_num_keepalives");
/**
* Interval between keepalive rexmits 
* Interval between consecutive retransmissions of a Keep-Alive
* probe if there has been no response from the other end (seconds).
* - Default: tcp_keepalive_rxmt= 75
* - Range: [0..1800]
*/
_LIT(TCPIP_INI_TCP_KEEPALIVE_RXMT,  "tcp_keepalive_rxmt");

/**
* Number of FIN retransmissions before socket is excluded from route flow count.
* After the FIN persistency limit is exceeded, link layer and TCP/IP stack is given permission
* to go down if no one else uses them.
* Value 0 means that socket is included in the interface flow count until it is really closed,
* i.e. in TIME_WAIT state.
* - Default: tcp_fin_persistency= 2
* - Range: [0..tcp_retries2]
*/
_LIT(TCPIP_INI_TCP_FIN_PERSISTENCY, "tcp_fin_persistency");

/**
Determines how to set congestion control parameters after spurious RTO.
1 = Eifel response (revert ssthresh and cwnd).
2 = Half congestion window and ssthresh.
3 = Set cwnd to 1 but revert ssthresh (Early versions of DCLOR).

- Default: tcp_spurious_rto_response= 1
- Range: [1..3]
*/
_LIT(TCPIP_INI_TCP_SPURIOUS_RTO_RESPONSE, "tcp_spurious_rto_response");

/**
* ECN.
* Toggles Explicit Congestion Notification [RFC3168] on and off.
* Two alternatives of ECN are available: when parameter value is 1
* the host uses ECN capable bit ECT(1).  When value is 2, the host
* uses ECN capable bit ECT(0). Of these two, value 2 is recommended,
* because some older network nodes may not recognize ECT(1) bit.
* - Default: tcp_ecn= 0
* - Range: 0= disabled, 1 & 2 = enabled)
*/
_LIT(TCPIP_INI_TCP_ECN,  	    "tcp_ecn");

/**
TCP Option alignment toggle.
If set to 1, TCP options are aligned on 32-bit boundaries using the NOP option.
- Default: tcp_alignopt= 0
- Range: 0, 1
*/
_LIT(TCPIP_INI_TCP_ALIGNOPT,	"tcp_alignopt");

/**
* TCP Window scaling option.
* Controls TCP window scaling option [RFC 1323]. -1 means window scaling
* is disabled. 0 means that window scaling capability is advertised in TCP
* handshake and the advertised window scale factor is determined based on
* the available receive window. Values 1 to 6 mean that window scaling is
* advertised on TCP handshake, but the given scale factor is always used
* in advertisement regardless of the receive window size.
*
* - Default: tcp_winscale= 0
* - Range: [-1..6]
*/
_LIT(TCPIP_INI_TCP_WINSCALE,	    "tcp_winscale");
/** @} */

/**
* @name UDP parameters
* @{
*/
/** The section name <tt>[udp]</tt>. */
_LIT(TCPIP_INI_UDP,                 "udp");
/**
* Causes UDP send to block on interface setup.
* Block UDP send while waiting for network interface setup.
* - Default: udp_wait_nif= 1
* - Range: 1 = enable, 0=disable (1 is conformant with old Symbian IPv4 stack)
*/
_LIT(TCPIP_INI_UDP_WAIT_NIF,        "udp_wait_nif");
/**
* Maximum inbound bytes queued by UDP.
* UDP receive queue size (octets, including header). Received packets will be
* discarded if receive queue is full. The queue will always hold at least one
* packet, regardles of packet size.
* - Default: udp_recv_buf= 8192
* - Range: [0..655535]
*/
_LIT(TCPIP_INI_UDP_RECV_BUF,        "udp_recv_buf");
/** @} */

/**
* @name Generic network group options.
* For sections [ip6], [ip], [icmp] and [icmp6].
* The section name is the protocol name..
* @{
*/
/**
* Maximum inbound bytes queued.
*
* Maximum inbound bytes queued by ip6, ip, icmp or icmp6
* socket (SAP).
*
* The test at arrival of new packet goes as follows:
*  - if already queued packets exceed the limit, packet is dropped
*  - if already queued packets do not exceed the limit, packet is
*    queued (even if doing so puts the queue over the limit)
*
* From above, some useful special cases follow:
*
* -	Zero limits queue to single packet at time. Incoming
*	packets are dropped until application reads the queued
*	packet.
*
* -	Negative value, nothing is queued and no data can be
*	received via this socket (SAP).
*
* Default value: 8192
*/
_LIT(TCPIP_INI_RECV_BUF,	"recv_buf");
/** @} */

/**
* @name Hostname section
*
* @{
*/
/** The section name <tt>[host]</tt> */
_LIT(TCPIP_INI_HOST,			"host");
/**
* The host name.
* The default value returned by RHostResolver::GetHostName(),
* if no SetHostName() has been issued.
*
* - Default:  <tt>hostname= localhost</tt>.
*/
_LIT(TCPIP_INI_HOSTNAME,		"hostname");
/** @} */

/**
* @name Hook ordering options
*
* The ordering of outbound flowhooks, outbound and inbound posthooks
* '*' indicates "position" of all hooks that don't match the specific
* listed names.
*
* @note
*	The options are only examined when a protocol binds to the stack.
*	The control affects the bind only if the name matches exactly
*	(case sensitive). The options can include protocol names that are
*	not installed in the system (there is no harm, as it doesn't match
*	any protocol).
* @{
*/
/** The section name <tt>[hook]</tt> */
_LIT(TCPIP_INI_HOOK,			"hook");
/**
* Controls ordering of the outbound flow hooks.
* For example, <tt>hook_flow= ipsec,mip6,*,foobar</tt> says that <tt>ipsec</tt>
* hook will be run first, then <tt>mip6</tt>, and <tt>foobar</tt> will be the last.
* All other hooks (matched by "*" are run between <tt>mip6</tt> and <tt>foobar</tt>.
* If the "*" is missing, it is implicitly assumed to be at the end of the string. 
*/
_LIT(TCPIP_INI_HOOK_FLOW,		"hook_flow");
/**
* Controls ordering of the inbound protocol hooks.
* For example, <tt>hook_any= ipsec,*,foobar</tt> says that <tt>ipsec</tt>
* hook will be run first, and <tt>foobar</tt> will be the last. This can
* be useful, if <tt>foobar</tt> is a protocol that wants to handle only
* packets that have been verified by the IPSEC.
*/
_LIT(TCPIP_INI_HOOK_INANY,		"hook_inany");
/**
* Controls the ordering of outbound post-processing hooks.
* For example, <tt>hook_outbound= *,probe,qos</tt> says that last two
* post-processors before NIF are <tt>probe</tt> and <tt>qos</tt> in this order.
* Any other post-processor will be run before them. If the "*" is missing,
* it is implicitly assumed to be at the end of the string.
*/
_LIT(TCPIP_INI_HOOK_OUTBOUND,	"hook_outbound");
/**
* Controls the ordering of inbound pre-processing hooks.
* For example, <tt>hook_inbound= probe</tt> says that packet from the NIF
* is given first to <tt>probe</tt>, and to other possible pre-processors after it.
* If the "*" is missing, it is implicitly assumed to be at the end of the string.
*/
_LIT(TCPIP_INI_HOOK_INBOUND,	"hook_inbound");
/**
* Controls the ordering of packet forwarding hooks.
* Usage is similar to the above examples.
*/
_LIT(TCPIP_INI_HOOK_FORWARD,	"hook_forward");
/** @} */

/**
* @name Compile time default values
*
* Many TCPIP.INI file parameters will have a hard coded default,
* if the value is not specified in the ini file. Some of the
* default constants are defined below.
*
* The name of the constant is generated directly from the string
* literal name according to the following template
* <tt>
*    TCPIP_INI_PARAMETER_NAME => KTcpipIni_ParameterName
* </tt>
* @{
*/
/** <tt>frag_count= 10</tt> */
const TInt KTcpipIni_FragCount = 10;	// [0..maxint]
/** <tt>frag_total= 30000</tt> */
const TInt KTcpipIni_FragTotal = 30000;	// [0..maxint]
/** <tt>maxttl= 69</tt> */
const TInt KTcpipIni_Maxttl = 69;		// [1..255]
/** <tt>linklocalttl= 1</tt> */
const TInt KTcpipIni_LinkLocalttl = 1;	// [-1,,255]
/** <tt>maxdials= 1</tt> */
const TInt KTcpipIni_Maxdials = 1;		// [0..maxint]
/** <tt>forward= 0</tt> */
const TInt KTcpipIni_Forward = 0;		// [0..1]
/** <tt>ipv4linklocal= 0</tt> */
const TInt KTcpipIni_Ipv4Linklocal = 0;	// [0..1]
/** <tt>probeaddress= 0</tt> */
const TInt KTcpipIni_ProbeAddress = 0;	// [0..1]
/** <tt>nodefendid= 0</tt> */
const TInt KTcpipIni_NoDefendId = 0;	// [0..1]
/** <tt>noiferror= 0</tt> */
const TInt KTcpipIni_Noiferror = 0;		// [0..1]
/** <tt>maxholdtime= 60</tt> */
const TInt KTcpipIni_Maxholdtime = 60;	// [0..maxint]
/** <tt>icmp_limit= 10</tt> */
const TInt KTcpipIni_IcmpLimit = 30;	// [0..maxint]

/** <tt>timeoutpriority= 12</tt>.
* Hardcoded for now. No tcpip.ini parameter yet. This
* defines the CActive priority used by the interface
* manager for it's internal CTimeoutManager.
*/
const TInt KTcpipIni_TimeoutPriority = 12;
/** <tt>keepinterfaceup= 1</tt> */
const TInt KTcpipIni_KeepInterfaceUp = 1;	// [0..1]
/** <tt>shutdowndelay= 10</tt> */
const TInt KTcpipIni_ShutdownDelay = 10;	// [0..maxint] (0=no delay)
/** <tt>hostname= localhost</tt> */
_LIT(KTcpipIni_Hostname, "localhost");	// default host name
/** @} */

/** @} */

#endif
