// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file exists only for doxygen documentation //
// When the Socket Server starts, it reads the description of all available
// protocols from the ESK files. The <b>bindto</b> directives arrange the protocols
// into a static collection of acyclic graphs The TCP/IP and related protocols,
// including the protocol plugins, form one such graph:
// Upper Layer
// Protocols
// |     |bindto= ip6                     ________
// | tcp |------------->.                |        |
// |_____|               \           .-->|resolver|<- - - - > DND
// |     |bindto= ip6      \       /
// | udp |----------------> \     /bindto=resolver
// |     |bindto= ip6 |     |   | (3) |
// | ip  |----------->| ip6 |-->|hook |
// |     |                   /  /  ^    bindfrom=ip6
// |icmp |----------------->/  /   |
// |_____|bindto= ip6      /  /    |
// |     |bindto= ip6    /  /      |
// |icmp6|------------->/  /       |bindto= ip6
// /      | (2) |
// _____               /       |hook |
// | (1) |-->--------->/       /|_____|
// |hook |-->---------------->/
// |_____|bindto= ip6,hook2
// The protocol plugins (or <b>hooks</b>) are just additional protocol modules for the
// socket server. The stack provides protocol modules for TCP, UDP, ICMP and %ICMP6,
// but as far as %IP6 is concerned, they are also "protocol plugins". They have the
// same API to use as any other protocol plugin. The "resolver" is an internal
// "protocol" which provides the gateway to the DND (= Domain Name Daemon, an
// external DNS resolver implementation). The resolver module implements the
// RHostResolver services for the TCP/IP modules.
// The socket server loads and starts a protocol module only when it is needed.
// A protocol is needed when a socket is opened to it, or if another protocol
// that binds to it, is needed.
// For example, when an application opens a TCP socket, the following
// protocols are "needed" in this order (using above diagram as an example):
// -# resolver
// -# hook (3)
// -# ip6
// -# tcp
// Actually, the loading order of "resolver" and "hook (3)" may also be
// reversed. They are both leaves, and can be loaded in any order (or
// in parallel) relative to each other.
// The Socket Server uses it's internal view (based on ESK "bindto" directives)
// of the protocol net to load the leaves of the required graph before any
// internal nodes. If this is the only open socket, then no other protocols
// are loaded.
// If another (or same) application now opens an UDP socket, only the UDP protocol
// needs to be loaded.
// When the last socket to the TCP is closed, the TCP protocol will be unloaded. If
// also the UDP socket is closed, then all protocols are unloaded.
// Any protocol plugin (1-3 hook) can also provide the socket interface for the
// application. Then the socket server loads the plugin (and any dependant protocols,
// such as ip6) when a socket is opened. Note, that hook (3) has no dependents,
// a socket is opened to it will cause only hook (3) to be loaded.
// For example, if hook (2) is needed, the load order is
// -# resolver
// -# hook (3)
// -# ip6
// -# hook (2)
// If after above, hook (1) needed, then only that need to be loaded.
// As seen above, the socket server creates and loads the needed protocols
// in depth first order. A possible sequence of events is roughly as follows
// (for the above TCP example):
// -# resolver = NewProtocolL(...) of the TCP/IP CProtocolFamilyBase is
// called to create an instance the resolver.
// -# hook3 = NewProtocolL(...) of the hook CProtocolFamilyBase is called
// to create an instance of the hook3
// -# ip6 = The NewProtocolL(...) of the TCP/IP CProtocolFamilyBase is called to
// create an instance of the ip6.
// -# ip6->BindToL(resolver)
// -# ip6->BindToL(hook3)
// -# tcp = NewProtocolL(...) of the TCP/IP CProtocolFamilyBase is called to
// create an instance of the TCP.
// -# tcp->BindToL(ip6)
// -# resolver-StartL()
// -# hook3->StartL()
// -# ip6->StartL()
// -# tcp->StartL()
// -# sap = tcp->NewSAPL() to create the TCP/IP SAP
// In reality ordering may be different in details, but the main issue
// to note is: <tt>X->BindToL(Y)</tt> is not called until the subgraph
// rooted at Y has been fully created.
// The <tt>tcp->BindToL(ip6)</tt> only makes ip6 known to the tcp. The tcp
// can now send packets to the ip6 using the ip6->Send() function.
// However, the ip6 instance has no knowledge about the tcp, and there
// is nobody to receive packets of protocol 6 (= TCP).
// To receive the TCP packet, the TCP protocol must request them from the
// ip layer by registering itself as an upper layer protocol for 6. The
// TCP does this by calling <tt>ip6->BindL(tcp, 6)</tt>. This adds the TCP as
// the upper layer receiver of the packets for the protocol 6. There can be
// only one active upper layer protocol for each protocol number at any time.
// The ip6 accepts multiple upper layer binds for the same protocol
// number. The last protocol to bind the number will receive the packets
// for that protocol until it unbinds (at which point some other
// protocol will get the packets).
// When the last socket using the TCP is closed, the socket server unloads
// and deletes the TCP protocol instance. The destruction of the TCP <b>must
// cancel</b> the protocol 6 registration from the IP layer (or IP layer would
// be calling tcp->Process() function with deleted instance pointer).
// The TCP cancels the registration using <tt>ip6->Unbind(tcp, 6)</tt>.
// It could also use <tt>ip6->Unbind(tcp, 0)</tt>, which cancels all BindL
// that a protocol has done (mostly useful for hooks, that issue multiple
// BindL calls with different parameters).
// The Unbind function is not part of the CProtocolBase. The unbind
// is provide by CProtocolBaseUnbind, which is the base class of
// all protocols in the TCP/IP family, or even more, the common
// base class is CProtocolInet6Binder. This class gives the full
// access to the network layer via MNetworkService class.
// In addition to upper layer binds, a protocol module can install
// various other types of hooks to the inbound and outbound packet
// paths. These are discussed in detail in the following chapters.
// Once fully started, the stack is a network of objects passing packets or data
// between each other. Most of the complexities in the implementation are related
// to the starting of everything and coping with the dynamic changes in the environment
// (such as interfaces and routes going up and down).
// The inbound and outbound packet paths are very different.
// The inbound packet path has 3 different hook attachment points: before upper layer <b>(A)</b>,
// for a specific extension header or protocol before it gets passed to upper layer <b>(B)</b> and
// before any IP processing <b>(C)</b>. Additionally, if at any point of processing stack determines
// that the packet is not addressed to this node, the packet falls into the forwarding path. At
// that point <b>(F)</b>, the forwarding hooks get to see the packet. If none of them take the packet,
// and if forwarding is disabled, the packet is dropped.
// packet                       packet
// queued <-- single "RunL" --> queued <-- under NIF RunL -->
// |         IP6      |       |
// (6)     (5)    (4) |                  |       |
// App<=== SAP<== TCP<======.            |       |
// NewData       |  ^   \           |       |
// GetData       |  |    \     ARP  |       |
// |  |      \  |     |  (2)  |   (1)
// |  | .-<---* <====)|(======<== NIF
// |  | | (3) |       |   |   |    
// (A)  (B)  (F)    |  (C)
// -# <b>Driver</b>:
// The device driver (NIF) receives the packet and sends it to the
// appropriate protocol instance (MNetworkService::Process).
// -# <b>Post hook:</b>
// Optional inbound hooks <b>(C)</b> can be added between the IP layer and
// interface for additional processing. These hooks will see the raw
// packets as they come from the NIF (CProtocolPosthook::Process). After
// this step, the packet is queued for later processing in the stack.
// -# <b>IP + MIp6Hook::ApplyL</b>
// The IP layer receives the packet from the driver (after the post hook processing)
// If the packet is not an IPv4 ARP packet, it is assumed to be an IPv6 or IPv4 packet.
// The Next Header (Protocol) field determines the format of the next layer header.
// There may be zero or more extension headers before the actual upper layer
// header (for example TCP). <b>The header is recognized as an extension header
// only if there is an attached hook which implements this header.</b> The stack uses
// the hooks <b>(B)</b> to skip over extension headers, until a header which is not implemented
// by any hook is reached. That header is assumed to be an upper layer header and
// the packet is passed through the generic hooks <b>(A)</b>, and then, if none of the
// hooks reject it, to the appropriate upper layer protocol, if it exists. If upper
// layer protocol does not exist, the stack generates an ICMP message "protocol
// unreachable". The stack has built in set of simple default hooks for the basic
// IPv6 extension headers (destination, hop-by-hop options and routing header: see
// TInet6Options and TInet6HeaderRouting).
// -# <b>Upper layer:</b>
// The TCP (for example, derived from CProtocolBase) instance receives all packets
// for TCP protocol (= 6). The TCP header includes the source and destination ports,
// which together with the source and destination address uniquely determine the
// connection to which this packet belongs.
// If this connection (SAP) exists, the packet is passed to it for further processing.
// -# <b>SAP:</b>
// The Service Access Point (derived from CServProviderBase) represents the application
// socket within the protocol stack. The SAP for TCP takes care of the sequencing,
// error checking and retransmission requests in order to serve an error free stream
// of data to the application.
// -# <b>Socket:</b>
// The application uses the socket API (RSocket) to read data from the connection.
// If a hook attaches to multiple points <b>(A)</b>, <b>(B)</b> and <b>(F)</b>, then it
// will see the same packet multiple times. All calls occur within the same "run" and
// there will be no calls for some other packet between the calls (steps 3-5).
// However, the inbound hook <b>(C)</b> behaves differently. It is called directly from
// the Process() function, which is called from the NIF (steps 1-2).
// Each packet is processed individually, and only at SAP level, the packets belonging
// to the same stream are merged. Packets for the same stream may even enter the system
// from different interfaces or tunnels.
// UDP and ICMP protocols behave similarly, except that they process packet at time and
// don't need to keep extensive per socket state information in their SAP's.
// The inbound packet is passed between the protocols (Process) as a <b>reference</b>
// to a RMBufChain, which is a simple pointer to the chain of RMBuf objects.
// The first RMBuf of the chain (when in packed state) contains the RMBufRecvInfo.
// The outbound packet path has 2 different hook attachement points: for a flow (D) and
// before the NIF (E).
// |        IP6         |
// |                ARP--.
// - - | - - >  FLOW        | \
// (1)     (2)    (3) |     /(4)|    \ (5) |  (6)
// App ===>SAP ==>TCP ====>*    |     *======>NIF
// (D)     (E)
// -# <b>Socket:</b>
// The application uses a socket API (RSocket) to write data to the connection.
// -# <b>SAP, Flow:</b>
// The Service Access Point represents the application socket within the protocol
// stack. The SAP for TCP implements the TCP protocol over the IP. The SAP converts
// stream data from application into IP packets being exchanged between the TCP
// end points. Usually, a SAP allocates a flow (RFlowContext) for the outbound
// packets. When sending packet, the SAP attaches the flow context to each
// packet (the iFlow member of RMBufSendInfo). Normally, SAP would send a packet
// to the TCP protocol using the CProtocolBase::Send() function. However, if the
// protocol instance has nothing useful to do for outbound packet, the SAP can also
// send directly to the network layer using MNetworkService::Send.
// -# <b>Upper layer:</b>
// For example, TCP (derived from CProtocolBase). If most of the work for outbound
// direction is done within the SAP, the protocol instance does not have much
// to do here. If used, it's Send method just passes the packet to the IP layer
// (MNetworkService::Send).
// -# IP + MFlowHook::ApplyL
// First the IP layer adds the IP header (IPv4 or IPv6) on top of the upper
// layer header, and then calls all hooks attached to the flow. The extension
// headers, if any, are added by the flow hooks <b>(D)</b>.
// -# Posthook
// Optional (outbound) hooks (E) can be added between the IP and Interface for
// additional processing (CProtocolPosthook::Send). These hooks will see all
// raw packets (except IPv4 ARP) before they are passed to the network interface.
// -# Driver
// Driver receives the packet from the protocol stack (CNifIfBase::Send).
// Before a flow is usable, it must be properly connected:
// - Before the upper layer can build any outgoing packets, it must know what
// source address should be used. The source address is not usually known
// until the device is connected to the network. Thus, the upper layer
// needs a way to request a connection for a specific destination address
// to be set up (or a way to query what source address which will correspond
// the given destination address).
// - Additionally, to achieve optimal packetising, the upper layer needs to
// know the size of the packets it can use. Again, this is only known after
// the connection is established.
// The flow context is a solution to the above problems. The upper layer fills
// in the parameters of the flow (destination address, source and destination port,
// and optionally the source address) and opens the flow. At the open of the flow,
// the interface is selected based on the destination address. If such interface
// cannot yet be found, a interface activation process can be started and the flow
// is placed into a pending state to wait for the completion. When the activation,
// including the source address selection and other setup, is completed,
// the flows that were waiting for it, are released.
// As the flow now provides a "pre-computed path" from upper layer to the target
// interface, it can be further used in optimizing the packet processing. Thus,
// it has been made possible for the upper layer to attach the flow context to
// the outgoing packets. If the context is present, the IP layer uses it to
// find the interface. Otherwise, the ip layer has to open a temporary flow
// for every packet separately.
// Other packet processing, that doesn't change when the addresses and ports
// stay some, can be partially recomputed and attached to the flow. For example,
// the IPSEC security associations can be negotiated at the flow setup phase,
// and then attached to the flow for quick access. This type of packet
// processing can be performed by dynamically loaded modules, which are
// connected to the protocol stack. The programming interface for outbound
// flow processing is defined by the MFlowHook class.
// There are seven different ways of attaching an external protocol
// module to the IPv6/IPv4 stack using BindL:
// -#	Upper layer protocol (for example, implementing a new
// protocol on the same level as TCP or UDP).
// The incoming packets are received by CProtocolBase::Process .
// -#	Inbound packet sniffer before the upper layer: <b>(A)</b>.
// The incoming packets are shown to MIp6Hook::ApplyL .
// -#	Inbound packet processor (for example, implementing a new
// extension header): <b>(B)</b>.
// The incoming packets are shown to MIp6Hook::ApplyL
// -#	Inbound raw packet sniffer before the IP processing: (C).
// -#	Outbound packet processor (for example, implementing a
// new extension header): <b>(D)</b>
// The outbound packets are shown to MFlowHook::ApplyL .
// -#	Outbound raw packet sniffer before the NIF, post processing: <b>(E)</b>.
// The outbound packets are received by CProtocolBase::Send
// -#	Forwarding packet processor <b>(F)</b>.
// The forwarded packets are shown to MIp6Hook::ApplyL
// The id parameter of the BindL call determides which of the above
// bindinds occur.
// In the IPv6/IPv4 stack, the BindL interface is the principal
// mechanism, which allows different dynamically loaded packet
// processors to insert themselves into the IPv6/4 packet processing.
// The mechanism allows several different ways of attaching,
// depending on the value of the id parameter.
// The <b>ip6</b> maintains a data structure of all protocols that have
// issued a bind. To get removed from this data structure, the hooks must
// use the CProtocolBaseUnbind::Unbind() function.
// The basic id parameter refers the value of Protocol (IPv4,
// RFC 791) or Next Header (IPv6, RFC-2460) field in the IP packet
// header. The values are the same for both IPv4 and IPv6 and
// the range is from 0 to 255.
// When the id has a value in range 1-255,
// the protocol binds normally to receive all packets matching the
// specified protocol number to the protocols Process function.
// There can be only one such protocol at any time receiving the packets.
// If more than one protocol binds this way for the same number, the last
// one to bind will receive all packets. This condition lasts until
// the later one unbinds, after which the older bind is in effect again.
// Every protocol has an implicit binding to a NULL protocol,
// which receives the packet when there are no other upper layer
// bindings. The NULL protocol generates the appropriate ICMP message
// (protocol unreachable or parameter problem).
// When the id has a value in range 256-511 (= MIp6Hook::BindHookFor()),
// the protocol
// binds as a hook to get a peek at packet containing a header matching
// the specified protocol number (id-256). The primary use of hooks of
// this type is to enable dynamically configurable support for the
// IPv6 extension headers. The protocol stack includes the default
// handlers for the obligatory extension headers, such as Hop-by-Hop
// Options, Destination Options, Routing Header and Fragment Header.
// In some cases, the default implementation is a minimal verification
// of the header and skip.
// The ordering of the hooks can be defined by a control line
// (hook_inany) in the TCPIP.INI file.
// The same parameter controls the hook ordering for both this (B),
// and the following (A).
// When a protocol binds with id=512 (= MIp6Hook::BindHookAll() = 2*#KIp6Hook_ANY),
// it indicates
// interest to have a peek at every packet before they are passed to
// the upper layers, but after the protocol-specific hooks are run.
// For example, IP security (IPSEC) and Mobile IP (MIP) have such needs.
// The ordering of the hooks can be defined by a control line
// (hook_inany) in the TCPIP.INI file.
// When the id has a value in range 513-767 (= MIp6Hook::BindFlowHook(),
// the protocol binds as an outbound hook, with a priority value id - 512
// (1..255). The priority value determines the calling order of the hooks,
// The priority also affected a control line (hook_flow) in
// When a protocol binds with id=768 (= MIp6Hook::BindPostHook() = 3*#KIp6Hook_ANY),
// the protocols
// steps between the IP layer and device driver. It will receive all
// outbound packets to its Send function. Post processing hooks are
// chained together using the BindL mechanism and, unless the hook
// decides to drop the packet, it must call the Send method of the
// next hook. Much of the above logic is automaticly provided by
// the CProtocolPosthook base class. All posthooks should be
// derived from it. The stack will always provide a "terminator".
// The ordering of the post processing hooks can be
// defined by a control line (hook_outbound) in the TCPIP.INI file
// When a protocol binds with id=769 (= MIp6Hook::BindPreHook() = 3*#KIp6Hook_ANY+1),
// the protocol steps between the IP layer and device driver.
// It will receive all inbound packets to its Process
// function. Preprocessing hooks are chained together using the BindL
// mechanism and, unless the hook decides to drop the packet, it must
// call the Process method of the next. Much of the above logic is
// automaticly provided by the CProtocolPosthook base class. All prehooks
// should be derived from it. The stack will always provide
// a "terminator".
// The ordering of the pre-processing hooks can be
// defined by a control line (hook_inbound) in the TCPIP.INI file.
// When a protocol binds with id=770 (= MIp6Hook::BindFowardHook() = 3*#KIp6Hook_ANY+2),
// the protocols starts to see all packets which fall into forwarding
// path. These packets have a destination address which is not assigned
// to this node. This may also happen as a result of detunneling.
// The ordering of the forward hooks can be
// defined by a control line (hook_forward) in the TCPIP.INI file.
// All hooks have direct access to the packet and they can change
// the content as needed. However, the changes must fall into following
// three cases:
// -#	The packet was processed and possibly modified, but the
// modification didn't change the protocol or next header field.
// The process continues calling the next hook in line or
// passes the result to the upper layer protocol.
// -#	The packet was processed in such way that the protocol or
// next header field changed (for example, the hook handled the
// IPv6 extension header). The process is restarted using the
// new protocol or next header value (e.g., a new list is
// selected from the switch lists based on the new protocol value).
// -#	The hook owned the packet (either dropped it or passed it to
// some other instance that is outside the scope of the ip6
// processing). The process is aborted without referencing
// the packet content any more. (A complex "hook owning"
// example is the IPv6 fragment handling: the fragment hook
// will "own" the packets until a fully assembled packet is
// returned as a replacement for the original input fragment)
// When a protocol binds as an outbound hook, it must implement the
// MIp6Hook::OpenL function.
// -#	When an upper layer is connecting the flow to a destination
// address, all registered outbound hooks (CIp6Hook) are queried
// (OpenL) whether they have interest in this specific packet flow.
// If so, they return a hook instance (MFlowHook) which is attached
// to the flow and it will get further calls as the flows state
// progresses. (Open phase).
// -#	When a flow is connected to a specific interface and the
// source address is known, all attached hooks (MFlowHook) are
// called (ReadyL) in reverse order to determine if the flow is
// ready for the packets. Any such hook may delay the flows
// transition to the packet transfer state as needed (for example,
// the IPSEC hook may need to negotiate the keys, before actual
// packet traffic can be allowed). (Ready phase)
// -#	After all attached hooks (MFlowHook) have reported that they
// are ready, the actual packet flow from the upper layer can
// start. For each packet, each of the attached hooks is called
// (ApplyL) and they can modify the packet or cancel its progress
// at will. (Transfer phase)
// -#	When a flow is shut down, the Close method of each attached
// hook instance (MFlowHook) is called (Close) to allow it to
// release the allocated resources, if any. (Close phase).
// During its lifetime, the flow can make transitions between transfer
// (3) and ready (2) phases any number of times.
// The IP stack maintains two post hook lists: one for outbound packets
// and one for inbound packets. A post hook can be a member of either
// list or both. The establishment of the lists happens in three
// stages for each hook:
// -#	Hook finds the stack: BindL or BindToL is used.
// -#	Hook reports its wishes by zero or more BindL.
// -#	Stack chains the hooks into specific order using BindL calls.
// The CProtocolPosthook should be used as a base class for all hooks
// that needs to act as a post hook. The class handles the inbound
// and outbound chains by implementing the chaining BindL/Unbind
// calls from the stack. The chain is always terminated by an
// internally constructed termination hook (T)
// The class also detects the stack (ip6 instance) and notifies the
// derived class using NetworkAttached and NetworkDetached calls.
// The default Send and Process will pass the packet to the next
// hook in the chain (these can be called from the respective
// methods of the derived classes after they have done the hook
// specific actions on the packet).
// This mode of hooking is used by the Quality of Service and packet
// scheduling module, when installed. The post-processing hook
// needs only the basic CProtocolBase API (Send), but it must
// be aware of the "flow concept" in IPv6/IPv4 stack.
// Post-processing hook steps in between the IP layer and interface
// and it will receive all packets to its Send method. The IP layer
// will leave the flow information attached to the information block.
// If the hook does not pass the packet to the next hook, or does
// not use the Send method of the flow for a packet, it must itself
// release the packet. It must also take care of closing the attached
// flow handle (iFlow) in the information block.
// Inbound preprocessing hook steps in between the IP layer and interface,
// and it will receive all packets to its Process method. 
// Ip6 can be used as a "protocol launcher" to automatically activate any
// number of add on protocols whenever ip6 itself is active.
// IP6 implements a BindToL method, which accepts
// binds to any protocols and issues a BindL(ip6, #KProtocolInet6Ip) to it.
// The stack maintains a list of such bindings and
// upon destruction will call Unbind and Close for each of them.
// A protocol using this method of activation must at miminum support the
// Unbind method (it must be derived from CProtocolBaseUnbind).
// If the launched protocol installs hooks or does anything else with
// stack, then the CProtocolPosthook is the best base class to use.
// It handles the receiving end of the stack binding automaticly.
// The BindL call is the only way the bound protocol can learn
// about the IPv6 protocol instance.
// Additionally, ip6 recognizes a special interface protocol
// (derived from CProtocolInterfaceBase), which supports the GetBinderL
// function for creating instance of an interface. IP will install this
// interface to the interface manager.
// Some example code can be built into a real running code.
// This example can be built into a real protocol module <tt>docex1.prt</tt>
// using the MMP file <tt>docex1.mmp</tt> in the inhook6examples directory.
// The hook, when installed, inserts a strange extension header to <b>every</b>
// outgoing packet (both IPv4 and IPv6), and on input direction it helps the
// stack to recognize and skip this header.
// The CExampleHook is derived from CProtocolPosthook for the convenience
// (the purpose of the documentation, CIp6Hook would be sufficient). CProtocolPosthook
// provides the automatic network connection detection, which makes the example
// code simpler (no need to implement BindToL, BindL or Unbind).
// The default BindL or BindToL in CProtocolPosthook generate the call
// to NetworkAttached when the stack is connected. This function then
// attaches the hooks to the stack.
// Every protocol needs to provide a description of self in a form of
// TServerProtocolDesc, which for the CExampleHook is initialized as
// follows:
// The same description is needed for CProtocolBase::Identify() and
// CProtocolFamilyBase::ProtocolList(). Thus, the description filling
// function is usually implemented as a separate static function.
// Most of the values and flags can be initialized with zero or non-sensical
// "random" values, because this protocol does not implement the SAP (Service
// Access Point) interface. This means that an application cannot open a socket
// connecting to this protocol.
// The ESK file of this protocol module is:
// [sockman]
// protocols= docex1
// [docex1]
// filename= docex1.prt
// bindfrom= ip6
// index= 1
// The incoming ApplyL handler provides the skipping of the extension
// header for the stack. In the error path (aInfo.iIcmp != 0), it must
// check whether the parameter error applies this header, and if it does,
// the packet must be considered as processed. Otherwise, the function
// can just skip over the header and report DONE.
// Because the hook registered interest for outbound flows, the OpenL
// will be called for every flow, and the implemented OpenL attaches
// itself to each of them. Because this hook does not need any flow
// specific context, the MFlowHook class is mixed in directly into
// the hook protocol and the OpenL only incremements the reference count
// of the base protocol and returns itself as MFlowHook.
// This hook adds unconditionally a fixed length header, thus the iHdrSize
// can be updated already in OpenL.
// This hook does not have any need to block the flow connection
// process, and the ReadyL is coded always to return ready.
// The outgoing ApplyL handler is called for every outgoing packet (because
// hook is attached to every flow). The handler inserts the strange
// extension header directly below the IP header.
// The example hook can be dynamically changed by use of socket
// options: the protocol number and port number can be changed. When the protocol
// is changed, the hook need to change it's bindings to the stack (unbind the
// previous protocol and bind a new).
// When this hook is activated, it will quickly become apparent that it
// breaks almost all communication with other hosts that don't know about
// this extension header. This hook really affects <b>all</b> IP packets,
// including IPv6 neighbour discovery and forwarded packets.
// Some possible add-on excercise projects:
// so that this works.
// LLMNR traffic while at it).
// that stealthly passes firewalls or NAT boxes. You need to use
// TCP or UDP like header instead of TExtensionHeader.
//





/**
 @internalComponent
 @page plugin_guide	Writing TCP/IP Protocol Plugins
 @verbatim
 @endverbatim
 @note
 @section plugin_loading	Plugin loading and binding
 @note
 @note
 @note
 @section packet_flows Packet flows
 @subsection inbound_flows	Inbound packet path
 @verbatim
 @endverbatim
 See also @ref nif_inbound_packets
 @subsection outbound_flows	Outbound packet path
 @verbatim
 @endverbatim
 See also @ref nif_outbound_packets
 @section attach_hooks	Attaching protocol modules to the stack
 @subsection bindl_interface	ip6 BindL(protocol, id) interface
 @subsubsection bind_upper_protocol	Upper layer protocol implementation
 @subsubsection bind_inbound_hook (B) Implementation of a new header.
 (@ref tcpip_ini_parameters
 @note
 @subsubsection bind_inbound_all (A) Sniffing all packets for upper layer
 (@ref tcpip_ini_parameters
 @subsubsection bind_outbound_flow (D) Outbound flow bind
 when they attached to an outbound flow (@ref outbound_path
 the TCPIP.INI file (@ref tcpip_ini_parameters
 @subsubsection bind_posthook (E) Outbound post processing
 (@ref tcpip_ini_parameters
 @subsubsection bind_prehoook (C) Inbound pre processing
 (@ref tcpip_ini_parameters
 @subsubsection bind_foward_hook (F) Looking at forwarded packets
 (@ref tcpip_ini_parameters
 @subsection inbound_path	Inbound packet path (MIp6Hook)
 @subsection outbound_path	Outbound flow hook (MFlowHook)
 @subsection	post_hooks	Hooks between IP layer and network interface
 @subsubsection post_hooks_outbound Outbound post processing hook
 @subsubsection posth_hooks_inbound Inbound preprocessing hook
 @subsection ip6_bindtol	ip6 as a protocol launcher
 @page doc_examples	Example projects for the documentation
 @section doc_example_1	Implementation of a strange extension header
 @subsection doc_example_1_1	The test environment
 @dontinclude mip6hook.cpp
 @skip class TExtensionHeader
 @until //-
 @skip class CHookExample
 @until //-
 @skip ::NetworkAttachedL(
 @until //-
 @skip _LIT(KMyName
 @until //-
 @verbatim
 @endverbatim
 @subsection doc_example_1_2	The packet processing
 @dontinclude mip6hook.cpp
 @skip ::ApplyL(
 @until //-
 @skip ::OpenL
 @until //-
 @skip ::ReadyL
 @until //-
 @skip ::ApplyL
 @until //-
 @subsection doc_example_1_3	Dynamically changing the hook
 @skip ::SetFlowOption
 @until //-
 @subsection doc_example_1_4	Discussion
 @li	change OpenL to return NULL for IPv6 Neighbor Discovery traffic,
 @li	change OpenL to return NULL for DNS traffic (and don't forget
 @li	change the header protocol to UDP or TCP, and implement something
*/
