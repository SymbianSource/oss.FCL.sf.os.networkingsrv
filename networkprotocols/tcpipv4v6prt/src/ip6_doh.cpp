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
// ip6_doh.cpp - hooks for IPv6 protocol message options
// Default IPv6 Options Hooks (Hop-by-Hop and Destination Options)
//

#include "ip6_doh.h"
#include <in_pkt.h>
#include <ext_hdr.h>
#include <icmp6_hdr.h>
#include "addr46.h"
#include "inet6log.h"
//
// *NOTE*
//		With the autumn 2000 hook api changes, there is no reason
//		allocate two different objects for DOP and HBH. The same
//		object could handle both and bind for both. This is
//		because aInfo.iProtocol can now be used to differentiate
//		the two, if needed! -- msa [for future cleanup..]
//
// CDestinationOptionsHook::ConstructL
//
void CDestinationOptionsHook::ConstructL()
	{
	iProtocol->BindL((CProtocolBase *)this, BindHookFor(KProtocolInet6DestinationOptions));
	}
//
// CHopOptionsHook::ConstructL
//
void CHopOptionsHook::ConstructL()
	{
	iProtocol->BindL((CProtocolBase *)this, BindHookFor(KProtocolInet6HopOptions));
	}

//
//	CDefaultOptionsHook::ApplyL
//	***************************
//	The default Options processing (code shared by Destination Options and
//	Hop-by-Hop Options).
//
TInt CDefaultOptionsHook::ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
	{
	if (aInfo.iVersion == 4)
		{
		//
		// Do not process HBH or DOP for IPv4. Returning PASS
		// will cause the main loop eventually to treat this
		// as unknown header, if nothing else handles it.
		//
		return KIp6Hook_PASS;
		}
	for (;;)	// .. just for exits.. not a loop..
		{
		TInet6Packet<TInet6Options> oh(aPacket, aInfo.iOffset);
		if (oh.iHdr == NULL)
			break;	// Bad packet, not enough for even the basic header
		const TInt hdrlen = aInfo.CheckL(oh.iHdr->HeaderLength());
		const TInt next_header = oh.iHdr->NextHeader();

		if (aInfo.iIcmp == 0)
			{
			//
			// Normal Option Header Handling
			//
			TInt count = hdrlen - TInet6Options::O_Options;			// Count of bytes in actual options.
			TInt start = aInfo.iOffset + TInet6Options::O_Options;	// Offset of the first
			//
			// Possibly heavy overhead, brute RMBuf Goto application
			// (but should work always)
			//
			while (count > 0)
				{
				RMBuf *p;
				TInt offset, len;
				TUint8 *ptr, type;

				if (!aPacket.Goto(start, p, offset, len))
					goto drop_packet;
				ptr = p->Buffer() + offset;
				switch ((type = *ptr++))
					{
				case 0:	// Pad1
					count--;
					start++;
					break;
#if 0
				case KDstOptionHomeAddress:
					if (aInfo.iProtocol == STATIC_CAST(TInt, KProtocolInet6DestinationOptions) &&
						// ..just check that there is enough stuff in the buffer
						// for the IPv6 address, so that the following CopyOut
						// does not panic. If not, just fall through. Should
						// do some better validity checking someday -- msa
						STATIC_CAST(TUint, count) >= sizeof(TIp6Addr)+2)
						{
						// This is shared between DOP and HBH, but this option is only
						// accepted in the DestinationOptions. The home address value
						// is simply copied to the aInfo.iDstAddr!
						//
						TIp6Addr home;
						TPtr8 ptr(home.u.iAddr8, sizeof(home.u.iAddr8), sizeof(home.u.iAddr8));

						aPacket.CopyOut(ptr, start+2);
						TInetAddr::Cast(aInfo.iSrcAddr).SetAddress(home);
#ifdef _LOG
							{
							TBuf<70> tmp_src;
							TInetAddr::Cast(aInfo.iSrcAddr).OutputWithScope(tmp_src);
							Log::Printf(_L("CDefaultOptionsHook::ApplyL: HOMEADDRESS src=[%S]\r\n"), &tmp_src);
							}
#endif
						aInfo.iFlags &= ~KIpAddressVerified;		// Request Address Check!
						goto skip_option;
						}
#endif
					// If not handled, fall unknown option handling!
				default:
					//
					// Unknown options
					//
					// 11 - send ICMP only if not Multicast dst
					// 10 - send ICMP
					// 01 - drop without ICMP
					// 00 - skip and continue
					//
					// (use HookValue with keyid=(protocol | (type << 8)) to check if someone else
					// already handled/implemented this option!)
					//
					if ((type & 0xC0) != 0 && aPacket.HookValue(aInfo.iProtocol | (type << 8)) == 0)
						{
						type &= 0xC0;
						if (type == 0xC0)
							{
							if (TIp46Addr::Cast(TInetAddr::Cast(aInfo.iDstAddr).Ip6Address()).IsMulticast())
								goto drop_packet;
							}
						else if (type != 0x80)
							goto drop_packet;
						//
						// Either 11 without multicast or 10, send ICMP6 Parameter Promblem,
						//
						iProtocol->Icmp6Send(aPacket, KInet6ICMP_ParameterProblem, 2, start, 1 /* Allow MC destination */);
						return -1;
						}
					//
					// 00 - Skip Over
					//
					// ** FALL THROUGH TO THE PadN handling, skip option **
				case 1:	// PadN
#if 0
skip_option:
#endif
					--len;
					while (len == 0)	// Should loop only once, but 'while' just in
										// case someone wants RMBuf with zero length...
						{
						p = p->Next();
						if (p == NULL)
							goto drop_packet;
						len = p->Length();
						ptr = p->Ptr();
						}
					count -= 2 + *ptr;
					start += 2 + *ptr;
					break;
					}
				}
			// Fall to "header processed"!
			}
		else if (aInfo.iIcmp == KProtocolInet6Icmp)
			{
			//
			// ICMP6 Problem report for a packet
			//
			const TInt offset = aInfo.iOffset - aInfo.iOffsetIp;	// Relative offset within problem packet

			if (aInfo.iType == KInet6ICMP_ParameterProblem &&	// A parameter problem...
				offset <= (TInt)aInfo.iParameter &&			// after start of this header?
				offset + hdrlen > (TInt)aInfo.iParameter)		// and before end of this header?
				break;		// Drop!
			// Fall to "header processed"!
			}
		else
			{
			//
			// ICMP v4 problem report (or something else)
			//
			// * just drop it for now*
			//
			break;
			}
		//
		// Gets Here only if the header is "processed" successfully
		// Remove options header, pass error processing to the next header
		//
		aInfo.iPrevNextHdr = (TUint16)aInfo.iOffset;	// Dest. Opt next header is at +0
		aInfo.iProtocol = next_header;
		aInfo.iOffset += hdrlen;
		return KIp6Hook_DONE;
		}
	//
	// All breakouts from the above "loop" cause the packet to be dropped
	//
drop_packet:
	aPacket.Free();
	return -1;
	}
