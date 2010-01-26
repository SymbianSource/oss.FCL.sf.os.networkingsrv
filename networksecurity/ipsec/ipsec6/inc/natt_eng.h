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
// natt_eng.h - Nokia VPN specific UDP encapsulating
// //    natt_eng.h - Nokia VPN specific UDP encapsulating of ESP packet
// //    This class takes care both of Nokia VPN specific and IETF specified
// //    UDP encapsulating ESP UDP capsulating means the following tunnel mode ESP packet
// //    encapsulation. 
// //    | IPV4 HDR  | UDP |  ESP    |  IP HDR  !    DATA    !  EPS   !  ESP  !
// //    |   New     | Hdr |  Header | Original !            ! Trailer!  AUTH !
// //                                |<---------- encrypted --------->| 
// //                      |<------------- authenticated ------------>|
// //    where UDP port number is configured via PFKEY API generic extension.
// //    Generic extension is used in PFKEY API Update and Add primitives
// //    This kind of encapsulation is required for handling IPSEC traffic
// //    in networks with NAT devices.
//




/**
 @internalComponent
*/
#ifndef __NATT_ENG_H__
#define __NATT_ENG_H__

#include "sa_spec.h"
#include "pfkeyext.h"

#define NON_ESP_MARKER       0x0
#define NON_ESP_MARKER_LTH   0x4     
const TInt KNATTraversalPort = 9872;
/**-----------------------------------------------------------------------------------------
 *
 * Class CNatTraversal
 * This class contains all parameters needed to handle ESP UDP encapsulation (= NAT Traversal)
 * The class instance is created (when necessary) due PFKEY Update (Add) primitives.
 *
 *-----------------------------------------------------------------------------------------*/
 
class CNatTraversal : public CBase
{
private:
      CNatTraversal(TBool aNatTraversalIETF)
      {   iUDPPort    = KNATTraversalPort;
          iUDPSrcPort = KNATTraversalPort;
          iNatTraversalIETF = aNatTraversalIETF;
      }
      TBool GetParameters(TDes8& aParams)
      {
          TPfkeyGenExtension NatExtension(aParams);
          if ( !NatExtension.CheckExtensionType(ESP_UDP_ENCAPSULATION_EXT) )
             return EFalse;
          
          TPtr8 Port((TUint8*)&iUDPPort, sizeof(TUint16));
          NatExtension.GetParameterData(UDP_ENCAPSULATION_PORT, Port);
          
          TPtr8 KeepAlive((TUint8*)&iKeepAliveTimeout, sizeof(TUint16));
          NatExtension.GetParameterData(NAT_KEEPALIVE_TIMEOUT, KeepAlive);
          
          TPtr8 DestIPAddr((TUint8*)&iDestIPAddr, sizeof(TInetAddr));
          NatExtension.GetParameterData(DESTINATION_ADDRESS, DestIPAddr); 

          return ETrue;
      }   
    
public:
      static CNatTraversal* New(TUint32 aFlags, const TPFkeyPrivExt& aGenericExt)
      {
          CNatTraversal*  NatTraversal = NULL;
          TBool NokiaNat = (aFlags & SADB_SAFLAGS_NAT_T);
          
          if ( NokiaNat || aGenericExt.iExt ) {
             NatTraversal = new CNatTraversal(!NokiaNat);
             if ( NatTraversal && aGenericExt.iExt ) {
                if ( NatTraversal->GetParameters((TDes8&)aGenericExt.iData) ) {
                   NatTraversal->iUDPSrcPort    = NatTraversal->iUDPPort;   
                   NatTraversal->iUseDestIPAddr = !NatTraversal->iDestIPAddr.IsUnspecified();
                   if ( NatTraversal->iUseDestIPAddr ) {
                      NatTraversal->iUDPPort = (TUint16)NatTraversal->iDestIPAddr.Port(); 
                   }   
                }
                else {
                   delete NatTraversal;
                   NatTraversal = NULL;
                }   
             }
          }   
               
          return NatTraversal;                  
      }
      inline TBool   NatTraversalIETF() { return iNatTraversalIETF; }     
      inline TUint16 GetDestPort() { return iUDPPort; }
      inline TUint16 GetSrcPort() { return iUDPSrcPort; }     
      inline TBool   UseDestIPAddr() { return iUseDestIPAddr; }
      inline TInetAddr* GetDestIPAddr() { return &iDestIPAddr; }         

     ~CNatTraversal() {}
     
private:      
      TBool     iNatTraversalIETF;     // TRUE = IETF UDP ESP encapsulation, FALSE = Nokia NAT Traversal
      TBool     iUseDestIPAddr;        // TRUE = Use destination IP address = The peer is behind a NAT device 
      TUint16   iUDPPort;
      TUint16   iUDPSrcPort;      
      TUint16   iKeepAliveTimeout;
      TInetAddr iDestIPAddr;
	};  



//
//  TIpsecNATT
//
class TIpsecNATT
    {
public:
    TIpsecNATT(MAssociationManager *aProtocol) : iManager(aProtocol) {}
    TInt ApplyL(CNatTraversal* aNatTraversal, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
    TInt ApplyL(CSecurityAssoc* &aSa, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo);
    TInt Overhead() const;
private:
    MAssociationManager *iManager;
    
    };

#endif
