
Netperf Packet Generator Protocol
=================================


** Overview **

NetperfPacketGen is something like NifTE, but under Freeway architecture.
It sends out UDP traffic without using the IP stack, so performance characteristics of different bearers can be compared without the IP stack processing overhead coming into consideration.


** How it works: **

NetperfPacketGen sits between the IP proto and link layers, so it can be as close to the bearer as possible whilst remaining bearer independent.

Further explanation: As the functionality required (IP packet generation) is a feature which doesn't depend on the implementation of
the link layer (e.g. RNDIS, ethernet, GPRS, dialup via modem, wifi etc.etc.), it belongs in a layer above the link layer.
As it also doesn't depend on the implementation of the IP stack (today we only have 1 but there may be other implementations later),
it belongs in a layer below the IP proto layer. Hence an inserted layer. This can be dropped into any connection stack as a
generic mechanism by which we can compare the performance of different bearers (taking the IP stack out of the equation). 

So it is configured in the same way as the delaymeter protocol- using a special access point record in commsdat
(so must be started using a SNAP preference). It reuses IP proto MCPR but should probably use something more generic.

NetperfPacketGen passes through inbound/outbound traffic unaltered (so existing IP-based setup things like DHCP, ARP etc can still work).

The packet generator itself is enabled by providing it the following information via a bespoke pub/sub interface: [lies! at the moment values are hardcoded]
	TInt iDestinationUdpPortNumber;
	TInt iDuration_s;
[	TInt iRequestedRate_kbps; ]

After this, the first (iperf formatted) packet must be sent from the client side (e.g. Netperf TEF library).

Once NetperfPacketGen detects this packet (by its destination UDP Port number matching iDestinationUdpPortNumber),
it then repeatedly:
	- clones this packet
	- increments sequence number and corrects checksum
	- sends it
[	- waits til the next packet send time ]

.. until the test duration has elapsed.


A remote iperf instance receives the generated packets to provide verification that the requested rate was achieved, and report any loss.


