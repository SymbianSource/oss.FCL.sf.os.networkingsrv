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
// info.pro - protocol analyzer development log
//

-	Problems with CActive receiver: Only crashes when using socket on iStatus
	else works fine (Waiting explicitly why?)


-	How to open Network (see RNif, Nif,...)
	
	Done, but now the net is automatically open when starting the program without 
	using the Netdial code (Why???)




- Refine Speed and Redrawings (Rotor Redraw)

- Check UDP and TCP. Choose port.

- If press cancel when opening network is not open anymore and it crashes


- TInt err=xnetdial.NetworkActive(active);	//Fails WHY???


- In_addr.cpp used provisionally.

- Add full decoding in Both ICMP header monitoring functions

- IPv6 Extension headers monitoring individual monitoring


//COMMENTS
- When output destaddr in IPv6 it returns '::' when is ::0 correct?
- ESP doesn't let you continue sniffing because next header is encapsulated
- Classes TInet6Options && TInet6HeaderRouting better in a .h than a ip6_rth.cpp ip6_doh.cpp
- TInet6Options -> TInet6HeaderOptions



- Beware net/host order in IPv4. Ask markku or mika	(ALWAYS HOST)
- add extra monitoring (ex: ICMP differents types)
- Font size and types ?
