// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @publishedPartner
 @released 
*/
 
#if !defined(__PKTDRV_H__)
#define __PKTDRV_H__

#include <comms-infras/nifprvar.h>
#include <comms-infras/ss_nodemessages.h>

/**
Uid2 for all pkt drv
@publishedPartner
@released
@since v8.1 (internalComponent from v5.0u)
*/
const TInt KUidNifmanPktDrv			= 0x1000053F;

/**
@publishedPartner
@released
*/
_LIT(KPacketDriverFileExtension, ".drv");

/**
@publishedPartner
@released
*/
const TInt KIeee802AddrSize = 0x06;

/**
An IEEE802-assigned address
@note This should be compatible with TBTDevAddr
@publishedPartner
@released
@since v8.1
*/
class TIeee802Addr
{
public:
/*		
	// Commented out until we converge with Bluetooth definitions - these methods will then be defined			
	inline TBool operator==(const TIeee802Addr& aAddr) const;
	inline TBool operator!=(const TIeee802Addr& aAddr) const;
	inline const TUint8 &operator[](TInt aIndex) const;
	inline TUint8 &operator[](TInt aIndex);
	inline TPtr8 Des();  //< Access value as a bigendian descriptor
	inline const TPtrC8 Des() const;
	IMPORT_C TInt SetReadable(const TDesC& aSource);
	inline void GetReadable(TDes& aDest) const;
	IMPORT_C void GetReadable(TDes& aDest, const TDesC& aPrepend, const TDesC& aByteSeperator, const TDesC& aAppend) const;
*/
	IMPORT_C void Reset(); //< Zero fill
protected:
	IMPORT_C TIeee802Addr();
	IMPORT_C TIeee802Addr(const TInt64 &aInt); //< Construct from a TInt64
	IMPORT_C TIeee802Addr(const TDesC8 &aDes);  //< Construct from a buffer
private:
	TFixedArray<TUint8, KIeee802AddrSize> iAddr;
};
	
/**
An ethernet or IEEE802.3 address.
No special implementation at the moment
@publishedPartner
@released
@since v8.1
*/
class TEthernetAddr : public TIeee802Addr
{
	public:
	IMPORT_C TEthernetAddr();
	IMPORT_C TEthernetAddr(const TInt64 &aInt); //< Construct from a TInt64
	IMPORT_C TEthernetAddr(const TDesC8 &aDes);  //< Construct from a buffer
};

typedef TEthernetAddr THWAddr; // for SC with code that uses THWAddr (note: SetAddress() method missing, so not full SC)

/**
@publishedPartner
@released
@since v8.1 (internalComponent from v5.0u)
*/
class TPacketDriverStats
{
public:
	TUint8 iPacketsIn;
	TUint8 iPacketsOut;
	TUint8 iBytesIn;
	TUint8 iBytesOut;
	TUint8 iErrorsIn;
	TUint8 iErrorsOut;
	TUint8 iPacketsLost;
};
	
class CPktDrvBase;
class CLANLinkCommon;
class CEther802LLCBase;
/**
@publishedPartner
@released
@since v8.1 (internalComponent from v5.0u)
*/
class CPktDrvFactory : public CObject
{
public:
	virtual CPktDrvBase* NewDriverL(CLANLinkCommon* aParent)=0;
  	void Install( RLibrary& aLib );
	IMPORT_C virtual void Close();
	virtual TVersion Version() const=0;
public:
	RLibrary iLib;
};
	
class RMBufChain;

/**
@publishedPartner
@released
@since v8.1 (internalComponent from v5.0u)
*/
class CPktDrvBase : public CBase
{
public:
	enum TRxMode {RxOff, RxPhysicalAddr, RxBroadcast, 
   	RxSomeMulticast, RxAllMulticast, RxPromiscuous};
public: 
	IMPORT_C CPktDrvBase(CPktDrvFactory& aFactory);
	IMPORT_C ~CPktDrvBase();
	IMPORT_C void SetNameL(const TDesC& aName);
	virtual void ConstructL(CLANLinkCommon* aParent)=0;
	virtual TInt Send(RMBufChain& aPkt)=0;
	virtual TInt StartInterface()=0;
	virtual TInt StopInterface()=0;
	virtual TInt ResetInterface()=0;
	virtual TInt SetRxMode(TRxMode AMode)=0; 
	virtual TInt GetRxMode() const=0;
	virtual TInt SetInterfaceAddress(const THWAddr&)=0;
	virtual TUint8* GetInterfaceAddress()const=0;
	virtual TInt GetMulticastList(const THWAddr* aAddr, TInt& n) const=0;
	virtual TInt SetMulticastList(const THWAddr* aAddr, TInt n)=0;
	virtual TInt InterfacePowerUp()=0;
	virtual TInt InterfacePowerDown()=0;
	virtual TInt InterfaceSleep()=0;
	virtual TInt InterfaceResume()=0;
	virtual TInt Notification(enum TAgentToNifEventType aEvent, void* aInfo)=0;
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource=0)=0;

	// 
	const TPacketDriverStats& GetInterfaceStats() const { return iStats;}
	TInt SenseInterfacePower() const {return iPowerState;}
protected:
	HBufC* iName;
	CPktDrvFactory* iFactory;
	TInt iRefCount;
	CLANLinkCommon* iParent;
	enum TDriverResetState {EStart, EWarmStart, EInitialising, 
	Ready, ESleeping, ERunning, EHalted};
	enum TDriverPowerState {EPowerOn, EPowerOff, ESleep};
	TDriverResetState iResetState;
	TDriverPowerState iPowerState;
	TPacketDriverStats iStats;
	THWAddr iHWAddr;
	TRxMode iRxMode;
};

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/pktdrv_internal.h>
#endif

#endif




