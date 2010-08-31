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
// Provides constants common across DHCP component
// 
//

/**
 @file DHCP_Std.h
*/

#ifndef __DHCP_STD_H__
#define __DHCP_STD_H__

#include <e32def.h>

#include <es_sock.h>
#include <es_enum.h>
#include <in_sock.h>
#include <cflog.h>
#include <es_sock_partner.h>

#ifdef _DEBUG
const TUid KMyPropertyCat = {0x101FD9C5};
const TUint32 KMyPropertyDestPortv4 = 67;
const TUint32 KMyPropertyDestPortv6 = 547;
#endif

__CFLOG_STMT(_LIT8(KLogSubSysDHCP, "DHCP");) // subsystem name

IMPORT_C TInt E32Main();

#ifdef EKA2
_LIT(KDHCPServerName,"!DHCPServ");
#else
_LIT(KDHCPServerName,"DHCPServ");
#endif

_LIT(KDHCPProcessName,"DhcpServ");

_LIT(KDHCPExeName, "DHCPexe.exe");
_LIT(KDHCPDLLName,"DHCPServ.dll");

//persistence TIDs for DHCPv4 & DHCPv6 (cooked-up numbers to be able distinguish
//what's been stored. Doesn't have to be system wide unique
const TInt KDHCPv6Persinstence = 0x01DFA341;
const TInt KDHCPv4Persinstence = 0x01DFA342;
const TInt KDHCPv6PersinstenceId = 1;
const TInt KDHCPv4PersinstenceId = 1;
const TInt KDHCPv4MaxRetryCount = 3;

const TInt KDHCPSrvMajorVersionNumber=1;
const TInt KDHCPSrvMinorVersionNumber=0;
const TInt KDHCPSrvBuildVersionNumber=0;

const TUint8	KDhcpParameterRequestListLen = 7;
const TUint16	KDhcpMaxMsgSizeIP4 = 576;

const TUint8	KIp4AddrByteLength = 4;
const TUint8	KIp4ChAddrMaxLength = 16;
const TUint8	KHwAddrOffset = 8;
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
const TUint 	KHwAddrLength = 6;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION

#ifdef _DEBUG
const TUint8	KDhcpWrongSrcPort = 34;
const TUint8	KDhcpWrongDestPort = 33;
#else
const TUint8	KDhcpSrcPort = 68;
const TUint8	KDhcpDestPort = 67;
#endif

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
// now that we are having both dhcp client and server implemantation within 
// DHCPServe.exe, we explicitly mentioned the client and server port to avoid confusion
const TUint8 	KDhcpClientPort = 68;
const TUint8	KDhcpServerPort = 67;

/** 6 hours, magic number which is set on the rational that
 anything bigger than this number would not be required as a mobile device might
 not be used as a DHCP server for more than 6hours continuously
 anything less than this lease time would mean more DHCP network traffic to reconfigure/renew
 when the lease time expires
*/
const TUint32 KDefaultLeaseTime = 21600;

// Constant used for setting custom lease time for the DHCP server 
// using RProperty::Define and RProperty::Set API's
// Only on Debug builds
#ifdef _DEBUG
const TUint32 KMyDefaultLeaseTime = 21600;
#endif

#endif // SYMBIAN_NETWORKING_DHCPSERVER


const TUint		KOneSecond = 1;
const TInt		KInfinity = -1;
const TUint		KHalfSecondInMicroSeconds = 500000;
const TUint		KZeroSecond = 0;
const TUint     KFailTimeOut = 10;
const TUint		KWaitForResponseTime = 512;	// in seconds
const TUint		KReallyLongLease = 0x7fffffff; // over 68 years - a virtual freehold

const TTimeIntervalSeconds KManualRenewTimeout = 2*KWaitForResponseTime;

//------------------------------------------------------------------------------------
//-- Debug heap control functions definition for the DHCP daemon.
//-- Access to these function is provided by RConnection Ioctl functionality.
//-- Note that this Ioctl level KDhcpMemDbgIoctl is not standard.
//------------------------------------------------------------------------------------

#ifdef  _DEBUG
#define __DHCP_HEAP_CHECK_CONTROL
#endif

//-- IOCTL level for heap debug control functions
const TUint KDhcpMemDbgIoctl    = KConnWriteUserDataBit | 0x1000;

//-- debug heap control functions numbers

const TUint KDHCP_DbgMarkHeap   = 1; //-- start heap mark.
const TUint KDHCP_DbgCheckHeap  = 2; //-- check heap, required a paremeter - number of expected allocated cells.
const TUint KDHCP_DbgMarkEnd    = 3; //-- mark the end of heap checking, required a paremeter - number of expected allocated cells.
const TUint KDHCP_DbgFailNext   = 4; //-- simulate heap alloc failure. Requires a parameter - failure rate. if param <0 resets.
const TUint KDHCP_DbgFlags      = 5; //-- set debug flags.

//-- debug flags used with KDHCP_DbgFlags
const TUint KDHCP_FailDiscover         	 = 1;  	//-- force init task to fail (state machine returns -KErrTimeOut)
const TUint KDHCP_FailRenew            	 = 2;  	//-- force renew task to fail (state machine returns -KErrTimeOut)
const TUint KDHCP_FailRebind           	 = 4;  	//-- force rebind task to fail (state machine returns -KErrTimeOut)
const TUint KDHCP_SetShortLease        	 = 8;  	//-- set short lease as defined in KDHCP_ShortLeaseSec
const TUint KDHCP_SetShortRetryTimeOut 	 = 16; 	//-- set short lease as defined in KDHCP_ShortRetryTimeOutSec
const TUint KDHCP_ForceDiscovery       	 = 32; 	//-- ignore lease time in commDB.
const TUint KDHCP_Dad 					 = 64;	// simulate a duplicate ip address has been found
const TUint KDHCP_ShortLeaseRenewTime  	 = 20;	//
const TUint KDHCP_ShortLeaseRebindTime   = 70;	// must be greater than timeout for test 5_6, so we know that renew worked
const TUint KDHCP_ShortLeaseLeaseTime 	 = 110;	//
const TUint KDHCP_ShortRetryTimeOut    	 = 7;	//
const TUint KDHCP_RequestIP4BroadcastOffer = 128;	//-- override setting unicast bit in offers


//-- buffer for storing function number
typedef TPckgBuf<TInt> TDhcpMemDbgParamBuf;


//-- IOCTL level for per-interface debug functions.
//    specific numbers must have KConnWriteUserDataBit/KConnReadUserDataBit set as appropriate
const TUint KDhcpInterfaceDbgIoctl    = 0x2000;






class TDhcpRnd
/**
  * Little random number generator class
  *
  * @internalTechnology
  */
        {
public:
        TDhcpRnd();
        TInt Rnd(TInt aMin, TInt aMax);
        TUint32 Xid() const;
        void SetXid(TUint32 aXid);
    void Init();
    void Init6();
private:
        TInt64 iSeed;
        TUint32 iXid;
        };

inline void TDhcpRnd::SetXid(TUint32 aXid)
/**
  * Sets transaction id
  *
  * @internalTechnology
  *
  */
        {
        iXid = aXid;
        }

inline TUint32 TDhcpRnd::Xid() const
/**
  * Returns transaction id
  *
  * @internalTechnology
  *
  */
        {
        return iXid;
        }

inline void TDhcpRnd::Init6()
/**
  * Initialises class to a random value
  *
  * @internalTechnology
  */
   {
        SetXid(static_cast<TUint32>(Rnd(0,0xFFFFFF)));
   }

inline void TDhcpRnd::Init()
/**
  * Initialises class to a random value
  *
  * @internalTechnology
  */
   {
        SetXid(static_cast<TUint32>(Rnd(0,KMaxTInt)));
   }

/**
Byte-to byte copying one object to another. Using Mem::Copy allows to avoid system crash on target platform
if the destination address isn't aligned to machine word boundary (e.g if it is a middle of some buffer).
Also performs a kind of type-check.

@param  apDst   destination address
@param  apSrc   pointer to the source object
@return pointer to the destination object 
*/
template<class T>
inline T* ObjectByteCopy(T* apDst, const T* apSrc)
{
    return reinterpret_cast<T*>( Mem::Copy(apDst, apSrc ,sizeof(T)));
}




#endif // __DHCP_STD_H__

