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
// DHCPv6/v4 Authentication RFC 3118
// 
//

/**
 @file DHCPAuthentication.h
*/

#ifndef DHCPAUTHENTICATION_H
#define DHCPAUTHENTICATION_H

#include <e32base.h>
#include "DhcpIP6Msg.h"
//#include "DHCPIP4Msg.h"

/*
	A client MUST be configurable to discard unauthenticated messages,
   and SHOULD be configured by default to discard unauthenticated
   messages if the client has been configured with an authentication key
   or other authentication information.
*/
namespace DHCPv4
{
/* 0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Code      |    Length     |  Protocol     |   Algorithm   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     RDM       | Replay Detection (64 bits)                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Replay cont.                                                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Replay cont. |                                               |
   +-+-+-+-+-+-+-+-+                                               |
   |                                                               |
   |      Authentication Information(depends on the Protocol       |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
const TInt  KReqMaxRetry      = 3;  //     Max Request retry attempts
}//DHCPv4 namespace

namespace DHCPv6
{
/*   0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |          OPTION_AUTH          |          option-len           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |   protocol    |   algorithm   |      RDM      |               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               |
    |                                                               |
    |          replay detection (64 bits)           +-+-+-+-+-+-+-+-+
    |                                               |   auth-info   |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               |
    .                   authentication information                  .
    .                       (variable length)                       .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

      option-code                  OPTION_AUTH (11)

      option-len                   11 + length of authentication
                                   information field

      protocol                     The authentication protocol used in
                                   this authentication option

      algorithm                    The algorithm used in the
                                   authentication protocol

      RDM                          The replay detection method used in
                                   this authentication option

      Replay detection             The replay detection information for
                                   the RDM

      authentication information   The authentication information,
                                   as specified by the protocol and
                                   algorithm used in this authentication
                                   option
*/
const TInt KOptionAuthProtocolLength = 1; //(1 bytes)
const TInt KOptionAuthAlgorithmLength = 1; //(1 bytes)
const TInt KOptionAuthRDMLength = 1; //(1 bytes)
const TInt KOptionAuthRDMdataLength = 8; //(8 bytes)

class TInterfaceConfigInfo;
class CDHCPOptionAuthentication : public CDHCPOptionAny
/**
  * DHCP Authentication option
  *
  * @internalTechnology
  */
	{

public:
   CDHCPOptionAuthentication() :
      CDHCPOptionAny( &iProtocol ),
      iProtocol( &iAlgorithm, KOptionAuthProtocolLength ),
      iAlgorithm( &iRDM, KOptionAuthAlgorithmLength ),
      iRDM( &iRDMdata, KOptionAuthRDMLength ),
      iRDMdata( NULL, KOptionAuthRDMdataLength )
      {
      }

   static COptionNode* NewL();

   void CheckL( const TInterfaceConfigInfo& aInterfaceConfigInfo );
   void InitL( const TInterfaceConfigInfo& aInterfaceConfigInfo );

public:
   CConstItem  iProtocol;
   CConstItem  iAlgorithm;
   CConstItem  iRDM;
   CConstItem  iRDMdata;
   //trailing authentication data is in CDHCPOptionAny::iOptionData
	};

}//DHCPv6 namespace

#endif

