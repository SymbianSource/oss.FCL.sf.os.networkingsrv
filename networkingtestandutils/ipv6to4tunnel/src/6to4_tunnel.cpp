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
// Name        : 6to4_tunnel.cpp
// Part of     : 6to4 plugin / 6to4.prt
// Implements 6to4 automatic and configured tunnels, see
// RFC 3056 & RFC 2893
// Version     : 0.2
//




// INCLUDE FILES
#include "6to4_tunnel.h"

// EXTERNAL DATA STRUCTURES
// EXTERNAL FUNCTION PROTOTYPES  
// CONSTANTS
// MACROS
// LOCAL CONSTANTS AND MACROS
// MODULE DATA STRUCTURES
// LOCAL FUNCTION PROTOTYPES
// FORWARD DECLARATIONS

// ============================= LOCAL FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// TInetAddress6Only
// A simple help class which overrides the Input method and
// guarantees that the result is always KAfInet6 on valid
// input.
// ----------------------------------------------------------------------------
class TInetAddr6Only : public TInetAddr
	{
	public:
	TInt Input(const TDesC &aBuf);
	};

TInt TInetAddr6Only::Input(const TDesC &aBuf)
	{
	const TInt ret = TInetAddr::Input(aBuf);
	if (ret == KErrNone && Family() == KAfInet)
		ConvertToV4Mapped();
	return ret;
	}

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CTunnel::CTunnel
// C++ default constructor can NOT contain any code, that
// might leave.
// ----------------------------------------------------------------------------
//
CTunnel::CTunnel ()
	{
	}

// Destructor
CTunnel::~CTunnel ()
	{
	delete iName;
	}

// ----------------------------------------------------------------------------
// CTunnel::SetNameL
// Sets a name for the tunnel.
// ----------------------------------------------------------------------------
//
void CTunnel::SetNameL (const TDesC & aName)
	{
	delete iName;
	iName = NULL;
	iName = HBufC::NewMaxL (aName.Length ());
	*iName = aName;
	}

// ----------------------------------------------------------------------------
// TTunnelParser::TTunnelParser
// C++ default constructor can NOT contain any code, that
// might leave.
// ----------------------------------------------------------------------------
//
TTunnelParser::TTunnelParser () : iToken(), iTokenVal(0), iTunnelQueue()
	{
	iTunnelQueue.SetOffset (_FOFF (CTunnel, iDLink));
	}

// Destructor
TTunnelParser::~TTunnelParser ()
	{
	CTunnel *tunnel = NULL;
	TTunnelQueueIter iter (iTunnelQueue);

	while ((tunnel = iter++) != NULL)
		{
		tunnel->iDLink.Deque();
		delete tunnel;
		}
	}

// ----------------------------------------------------------------------------
// TTunnelParser::ParseL
// Parses the tunnel configuration file and creates tunnel objects accordingly.
//
// File syntax example. 
// Keywords: tunnels, tunnel, endpoint_address, virtual_if_address,
//           route_address, route_prefix_length, STRING, INTEGER.
// #
// # Write comments like this to the beginning of the file
// #
//
// tunnels = {
//	 tunnel_1,
//	 tunnel_2,
//	 tunnel_3
//	}
//
// tunnel tunnel_1 = {
//	 STRING endpoint_address = 192.168.0.3,
//	 STRING virtual_if_address = 2000:abcd::1,
//	 STRING route_address = 2000:abcd::,
//	 INTEGER route_prefix_length = 32
// }
//
// tunnel tunnel_2 = {
// 	 STRING endpoint_address = 192.168.0.3,
//   STRING virtual_if_address = 2000:1111::1,	
//	 STRING route_address = 2000:1111::,
//	 INTEGER route_prefix_length = 32
// }
//
// tunnel tunnel_3 = {
//	 STRING endpoint_address = 192.168.0.5,
//   STRING virtual_if_address = 2000:2222::1,
//	 STRING route_address = 2000:2222::,
// 	 INTEGER route_prefix_length = 32
// }
// ----------------------------------------------------------------------------
//
TInt TTunnelParser::ParseL (const TDesC & aTunnelData)
	{

	Assign (aTunnelData);

	// Parse the list of tunnels first and create and add them to the list.
	TTokenType token;

	while ((token = NextToken()) == ETokenTypeComment)
		{
				// Intentionally left blank
		}
	if (token == ETokenTypeString)
		{
		if (iToken.Compare (_L ("tunnels")) != 0)
			{
			return KErrGeneral;
			}
		}
	else
		{
		return token == ETokenTypeEof ? KErrNone : KErrGeneral;
		}

	if ((token = NextToken ()) != ETokenTypeEqual)
		{
		return KErrGeneral;
		}

	if ((token = NextToken ()) != ETokenTypeBraceLeft)
		{
		return KErrGeneral;
		}

	do
		{
		token = NextToken ();
		if (token == ETokenTypeBraceRight)
			{
			// Tunnels done
			break;
			}

		if (token == ETokenTypeString)
			{
			CTunnel *tunnel = new (ELeave) CTunnel;

			tunnel->SetNameL (iToken);

			iTunnelQueue.AddLast (*tunnel);
			}
		else
			{
			return KErrGeneral;
			}
		}
	while ((token = NextToken ()) == ETokenTypeComma);

	if (token != ETokenTypeBraceRight)
		{
		return KErrGeneral;
		}

	// Parse individual tunnels. Search for the list for a match and if
	// found, add parameters there.
	while ((token = NextToken ()) == ETokenTypeString)
		{
		if (iToken.Compare (_L ("tunnel")) != 0)
			{
			return KErrGeneral;
			}

		TInetAddr6Only address;
		CTunnel *tunnel = NULL;

		if ((token = NextToken ()) == ETokenTypeString)
			{
			TTunnelQueueIter iter (iTunnelQueue);

			while ((tunnel = iter++) != NULL)
				{
				if (tunnel->Name().Compare (iToken) == 0)
					{
					// found.
					break;
					}
				}
			if (tunnel == NULL)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeEqual)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeBraceLeft)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("STRING")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("endpoint_address")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeEqual)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			// Parse endpoint address
			if (address.Input (iToken) != KErrNone)
				{
				return KErrGeneral;
				}
			tunnel->EndpointAddr().SetAddress (address.Ip6Address(), address.Scope());
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeComma)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("STRING")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("virtual_if_address")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeEqual)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			// Parse inner virtual address
			if (address.Input (iToken) != KErrNone)
				{
				return KErrGeneral;
				}
			tunnel->VirtualIfAddr().SetAddress(address.Ip6Address(), address.Scope());
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeComma)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("STRING")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("route_address")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeEqual)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			// Parse route prefix as "address"
			if (address.Input (iToken) != KErrNone)
				{
				return KErrGeneral;
				}
			tunnel->RouteAddr() = address.Ip6Address();
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeComma)
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("INTEGER")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) == ETokenTypeString)
			{
			if (iToken.Compare (_L ("route_prefix_length")) != 0)
				{
				return KErrGeneral;
				}
			}
		else
			{
			return KErrGeneral;
			}

		if ((token = NextToken ()) != ETokenTypeEqual)
			{
			return KErrGeneral;
			}

		if (Val (iTokenVal) != KErrNone)
			return KErrGeneral;
		tunnel->SetRoutePrefixLength(iTokenVal);

		if ((token = NextToken ()) != ETokenTypeBraceRight)
			{
			return KErrGeneral;
			}
		}

	// Scan through the list and make sure all tunnels have been configured.
	TTunnelQueueIter iter (iTunnelQueue);
	CTunnel *tunnel = NULL;

	while ((tunnel = iter++) != NULL)
		{
		if (tunnel->EndpointAddr().IsUnspecified ())
			{
			// not configured.
			return KErrGeneral;
			}
		}

	return KErrNone;
	}

// ----------------------------------------------------------------------------
// TTunnelParser::NextToken
// Returns the next token type in the input stream. iToken gets the value of 
// the (string) token.
// ----------------------------------------------------------------------------
//
TTunnelParser::TTokenType TTunnelParser::NextToken ()
	{
	TChar ch;
	TTokenType val;

	SkipSpaceAndMark ();
	if (Eos ())
		val = ETokenTypeEof;
	else
		{
		ch = Get ();
		switch (ch)
			{
			case '{':
				val = ETokenTypeBraceLeft;
				break;
			case '}':
				val = ETokenTypeBraceRight;
				break;
			case '(':
				val = ETokenTypeParLeft;
				break;
			case ')':
				val = ETokenTypeParRight;
				break;
			case '=':
				val = ETokenTypeEqual;
				break;
			case ',':
				val = ETokenTypeComma;
				break;
				case '#':
						val = ETokenTypeComment;
						while (!Eos())
						{
								ch = Get();
								if (ch == '\n' || ch == '\r')
										break;
						}
						break;
			default:
				// Integers, ip addresses, etc. are mapped to strings.
				val = ETokenTypeString;
				while (!Eos ())
					{
					ch = Peek ();
					if (ch == '{' || ch == '}' ||
						ch == '(' || ch == ')' ||
						ch == '=' || ch == '#' ||
						ch == ',' || ch.IsSpace ())
						break;
					Inc ();
					}
			}
		}

	iToken.Set (MarkedToken ());
	SkipSpaceAndMark ();

	return val;
	}

// ========================== OTHER EXPORTED FUNCTIONS ========================

//  End of File  
