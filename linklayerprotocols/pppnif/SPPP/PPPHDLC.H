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
 @internalComponent 
*/

#if !defined(PPPHDLC_H)
#define PPPHDLC_H

#include <networking/bca.h>
#include <networking/bcafactory.h>
#include "PPPBASE.H"
#include "PPPConfig.h"

//=========================================
// JGG PPP CHANGE
#include <etelpckt.h>
//#include <ETelGprs.h> // don't link to ETelGPRS, just include for the data count structure
//=========================================

#ifndef MAX
 #define MAX(a, b) ((a > b) ? a : b)
#endif
// Length of longest typical encoded HDLC frame 
const TInt KPppHdlcSize    = 1503;
const TInt KPppHdlcFcsSize = 4;

// Create buffer at least size of mbuf + overhead, to ensure execution of escape sequence expansion code (refer CPppHdlcLink::DoSend)
const TInt KPppHdlcBufSize = 2 * (MAX(KPppHdlcSize, KMBufSmallSize) + KPppHdlcFcsSize + 1);
typedef TBuf8<KPppHdlcBufSize> TPppHdlcBuf;

// Framing chars
const TUint8 KPppHdlcFlagChar = 0x7e;
const TUint8 KPppHdlcEscChar = 0x7d; 
const TUint8 KPppHdlcEscBit = 0x20;

// HDLC address and control vales
const TUint8 KPppHdlcAddrByte = 0xff;
const TUint8 KPppHdlcCtrlUIByte = 0x03;
const TUint8 KPppHdlcPadChar = 0x01;

const TInt KPppHdlcSendChannel = 0;
const TInt KPppHdlcRecvChannel = 1;

const TInt KPppHdlcCommReadPriority = 150;
const TInt KPppHdlcCommWritePriority = 100;

// Flags use in FCS negotiation
const TUint KPppHdlcFcs0Flag = 0x1;
const TUint KPppHdlcFcs16Flag = 0x2;
const TUint KPppHdlcFcs32Flag = 0x4;

using namespace BasebandChannelAdaptation;

// BCA factory is hosted in BCA dll: this routine creates BCA factory instance.
typedef MBcaFactory* (*TNewBcaFactoryL)();

class CBcaWriter;
class CBcaReader;
class CBcaControl;


NONSHARABLE_CLASS(CPppHdlcLink) : public CPppLinkBase, public MPppOptionHandler
/**
PPP over serial line protocol (RFC 1662)
*/
	{
	enum TPppHdlcState
		{ EPppHdlcClosed, EPppHdlcConnecting, EPppHdlcOpen, EPppHdlcDisconnecting, EPppHdlcReconnecting };

public:
	CPppHdlcLink(CPppLcp* aLcp);
	~CPppHdlcLink();
	virtual void CreateL();
	// CPppLinkBase
	virtual TInt Send(RMBufChain& aPacket, TUint aPppId=KPppIdAsIs);
	virtual void OpenL();
	virtual void Close();
	virtual void StartL();
	virtual void Stop(TInt aReason, TBool aLinkDown=ETrue);	
	virtual void GetSendRecvSize(TInt& aMaxRecvSize, TInt& aMaxSendSize);
	virtual TInt SpeedMetric();
	
	//BCA related methods
	void LoadBcaL();
	
	TBool DoShutdownBcaGracefully() const;	
	void LinkDown(TInt aStatus);
	void LinkTerminationComplete();
	
	void SetOriginalSerialPortConfig(TCommConfig& aHdlcConfig);
	void GetSerialPortConfigForHdlc(TCommConfig& aHdlcConfig) const;
	TCommConfig OriginalSerialPortConfig() const;


	void CancelOperation();
	void StartPPP();
	TBool ShutDownPPP();
	void FreeBuf();
	void BcaReadComplete(TInt aStatus);
	void BcaWriteComplete(TInt aStatus); 
	#if defined (_DEBUG)
	inline CPppLog& Logger() const;
	#endif
	//======================================
	// JGG PPP CHANGE
	virtual void GetDataTransfer(RPacketContext::TDataVolume&);
	//virtual void GetDataTransfer(RGprsContext::TDataVolume&);
	//========================================
	
    inline TPtrC GetBCAStack() const;
    inline TPtrC GetBCAName() const;
    inline TPtrC GetPortName() const;
    inline TUint32 GetIAPid() const;
    inline TCommRole GetCommRole() const;
    inline TUint32 GetHandShaking() const;
    inline TPtrC8 GetExcessData() const;
    	
protected:
	 
	// MOptionExtender
	virtual void OptNegotiationStarted();
	virtual void OptNegotiationAborted();
	virtual void OptNegotiationComplete();
	virtual void OptFillinConfigRequestL(RPppOptionList& aRequestList);
	virtual TPppOptResponse OptCheckConfigRequest(RPppOption& aOption);
	virtual void OptApplyConfigRequest(RPppOption& aOption);
	virtual void OptRecvConfigAck(RPppOption& aOption);
	virtual void OptRecvConfigNak(RPppOption& aOption, RPppOptionList& aReqList);
	virtual void OptRecvConfigReject(RPppOption& aOption, RPppOptionList& aReqList);
private:
	void DoSend(TBool aRestart);
	void DoRecv();
	void DoBadRecv();
	TBool AppendRecvMbuf(TUint8*& mptr, TUint8*& mend);
	void PacketModeOnL();
	void PacketModeOff();
	void InitEscMap();
	void SetEscMapBit(TUint8 aChar);
	inline TBool IsEscapedChar(TUint8 aChar);
	inline TBool IsInRecvEscMap(TUint8 aChar);
	void EncodeChar(TUint8*& aPtr, TUint8 aChar);
	TBool DecodeChar(TUint8*& aPtr, TUint8*& aPtrEnd, TUint8 aChar);
	TBool UnescapeChar(TUint8*& ptr, TUint8*& end, TUint8*& mptr, TUint8*& mend);
	void DeferredApplyOptions();
	void ReadIniFileL();
	inline void CallBackOptRequestGranted(); // CSW
	void LogUserData(RMBufChain& aPacket, TInt aChannel);	
	void CreateHdlcHeader(TUint8*& aPtr, TBool aRestart, TUint16 aProt);
	TUint16 DecodeProtocolID(TUint8*& ptr) const;


    /** The original comm port configuration */
	TCommConfig iOrigConfig;
	
	/** HDLC connection state */
	TPppHdlcState iState;
	
	/** Internal configuration flags */
	TUint iFlags;
	
	/** Flags to apply once LCP has finished negotiation */
	TUint iPendingMask;

	/** Mask of flags to apply once LCP has finished negotiation */
	TUint iPendingFlags;
	
	/** Currently negotiated control character escape map for sending */
	TUint iPendingEscMap;
	
	RMBuf* iApplyConfigMarker;

	/** Send buffer low water mark below which flow is turned on */
	TInt iSendLoWat;

	/** Send buffer high water mark above which flow is turned off */
	TInt iSendHiWat;
	
	/** Number of buffers currently sending */
	TInt iSendNumBufs;
	
	/** Whether packet flow has been turned off due to exceeding the high water mark */
	TBool iSendFlowOn;
	
	/** Queue of packets to send */
	RMBufPktQ iSendQ;
	
	/** Current packet being sent */
	RMBufChain iSendPkt;
	
	/** Buffer holding bytes to send to comm port */
	TPppHdlcBuf iSendBuf;
	
	/** CRC-16 for current sending frame */
	TPppFcs16 iSendFcs16;

	/** CRC-16 for current receiving frame */
	TPppFcs16 iRecvFcs16;

#ifdef __LCP_EXTENSION_FCS_32
	/** CRC-32 for current sending frame */
	TPppFcs32 iSendFcs32;

	/** CRC-32 for current receiving frame */
	TPppFcs32 iRecvFcs32;
#endif	

    /** Control character escape map to use while sending */
	TUint32 iSendEscMap[8];
	
	/** Used for telling peer which characters < 0x20 to escape	ACCM negotiation */
	TUint32 iRecvEscMap;

	/** The escape map we would like to use */
	TUint32 iDesiredRecvEscMap;
	
	/** Queue of incoming PPP packets ready to be processed by LCP */
	RMBufPktQ iRecvQ;
	
	/** Current packet being received */
	RMBufQ iRecvPkt;
	
	/** Current MBuf of packet being received */
	RMBuf* iRecvMBuf;
	
	/** Buffer containing a raw PPP packet from a comm port */
	TPppHdlcBuf iRecvBuf;

    /** Number of IP data bytes sent */
	TUint32 iSentData;

    /** Number of IP data bytes received */
	TUint32 iRecvdData;
	//TBool iUpdate;
	
	/** BCA lives here*/
	TAutoClose<RLibrary> iBcaDll;

	/** The Baseband Channel Adaptor - the actual link provider */
   	MBca* iBca; 
   	   	
   	/** used to send data*/
	CBcaWriter* iBcaWriter;
	
	/** used to receive data*/
	CBcaReader* iBcaReader;
	
	/** used to control the BCA asynchronously*/
	CBcaControl* iBcaControl;
	
	/** IAP ID used to open CommDB*/
	TUint32 iIapId;
		
	/** PPP NIF down error code*/
	TInt iError;
	
	/** if PPP is down*/
	TBool iLinkDown;
	
#if defined (_DEBUG)
	/** Verbosity of log */
	TInt iLogLevel;
	
	/** File format of stored data dump */
	TInt iLogFormat;
	
	/** Type of data stored in log dump file */
	TInt iLogLinkFormat;
#endif
	};

inline TBool CPppHdlcLink::IsEscapedChar(TUint8 aChar)
/**
Determine if character should be escaped on transmit based on ACCM negotiation.

@param aChar Character

@return Whether character should be escaped
*/
	{
	return (iSendEscMap[aChar>>5] & (1<<(aChar&0x1f))) !=0;
	}

inline TBool CPppHdlcLink::IsInRecvEscMap(TUint8 aChar)
/**
Determine if character should have been escaped on receive based on ACCM negotiation.

@param aChar Character

@return Whether character should have been escaped
*/
	{
	return (iRecvEscMap < 0x20) && (iRecvEscMap & (1<<(aChar&0x1f)));
	}
	
inline TUint32 CPppHdlcLink::GetIAPid() const
    {
    return iPppLcp->GetBCAProvision()->GetIAPid();
    }
    
inline TPtrC CPppHdlcLink::GetBCAStack() const
    {
    return iPppLcp->GetBCAProvision()->GetBCAStack();        
    }

inline TPtrC CPppHdlcLink::GetBCAName() const
    {
    return iPppLcp->GetBCAProvision()->GetBCAName();
    }

inline TPtrC CPppHdlcLink::GetPortName() const
    {
    return iPppLcp->GetBCAProvision()->GetPortName();        
    }

inline TCommRole CPppHdlcLink::GetCommRole() const
    {
    return iPppLcp->GetBCAProvision()->GetCommRole();
    }

inline TUint32 CPppHdlcLink::GetHandShaking() const
    {        
    return iPppLcp->GetBCAProvision()->GetHandShaking();
    }

inline TPtrC8 CPppHdlcLink::GetExcessData() const
    {        
    return iPppLcp->GetExcessData();
    }

/** Gets the PPP logger

@return the reference to the logger object.
*/
#if defined (_DEBUG)
inline CPppLog& CPppHdlcLink::Logger() const
	{
	ASSERT(iPppLcp);
	ASSERT(iPppLcp->iLogger);
	return *(iPppLcp->iLogger);	
	}
#endif
/**
Control of the BCA: translates the instructions from the higher level (i.e. HDLC) 
into sequences of Control actions on the BCA, and dispatches them. 

*/
NONSHARABLE_CLASS(CBcaControl) : public CActive
	{
public:
	CBcaControl(CPppHdlcLink& aUser, MBca& aBca);
	~CBcaControl();

	void StartStartupSequence();
	void StartShutdownSequence();
	inline TBool BcaIsOpen() const;
	
	// CActive.
	void RunL();
	void DoCancel();
	
private: 
	
	TBool IsShuttingDown() const;
	
	// N.B.
	// The order of these enums reflects the BCA lifecycle, and thus is absolutely critical!
	// Later, we'll be doing checks like if iControlStep < EShuttingDown
	// New steps must be inserted in an appropriate position. There is no danger of BC
	// break, because this enum is local to this class, obviously.
	 
	/** Current control step on BCA.*/
	enum TBcaControlStep
		{
		/** Not doing anything */
		ENone,
		
		//
		// Startup sequence
		// 
		
		/** Pseudo step - starting BCA sequence started */
		EStartingStartup,
		/** Setting IAP */
		ESettingIap,
		/** Provisioning the BCA stack */
		ESettingBcaStack,		
		/** Opening the BCA for use */
		EOpeningChannel,
		/** Enabling monitoring of serial control lines */
		EnablingLinkMonitoring,
		/** Getting current serial config */
		EGettingSerialConfig,
		/** Applying serial config settings appropriate for HDLC */
		ESettingSerialConfigForHdlc,
		/** Setting BCA internal buffers size */
		ESettingBufferSize,
		/** Purging the buffers */
		EResettingBuffers,
		
		//
		// Shutdown sequence
		//		
		
		/** Pseudo step - starting BCA shutdown sequence */
		EStartingShutdown,
		/** Returning the Comm port to its original state */
		ERestoringOrigSerialConfig,
		/** Shutting down the open channel, possibly in a graceful way */
		EShuttingDownChannel
		};
		
	/** Our user, the HDLC link */
	CPppHdlcLink& iUser;

	
	/** BCA */ 
	MBca& iBca;

	/** Control step we are currently executing */
	TBcaControlStep iControlStep;
	
	/** Is the BCA open? I.e. do we have to close it or not? */
	TBool iBcaIsOpen;
	
	/** Storage for retrieved serial port configuration */
	TCommConfig iSerialConfig;
	};


/**
Asynchronous BCA reader.

*/
NONSHARABLE_CLASS(CBcaReader) : public CActive
	{
public:
	CBcaReader(CPppHdlcLink& aUser, MBca& aBca, TInt aPriority);
	~CBcaReader();

	void Read(TDes8& aDes);
	void ReadReady();

	void RunL();
	void DoCancel();	

private:
	/** Our user - the HDLC link */ 
	CPppHdlcLink& iUser;
	
	/** The BCA  */
	MBca& iBca;
	};
	
/**
Asynchronus BCA writer.

*/
NONSHARABLE_CLASS(CBcaWriter) : public CActive
	{
public:
	CBcaWriter(CPppHdlcLink& aUser, MBca& aBca, TInt aPriority);
	~CBcaWriter();

	void Write(const TDesC8& aDes);

	void RunL();
	void DoCancel();	

private: 
	
	/** Our user - the HDLC link */
	CPppHdlcLink& iUser;
	/* The BCA */
	MBca& iBca;
	};
	

/**
Returns True if the BCA has been opened, i.e. if it has to be closed

@return ETrue if the BCA has been opened
*/	
inline TBool CBcaControl::BcaIsOpen() const
	{
	return iBcaIsOpen;
	}	

#endif
