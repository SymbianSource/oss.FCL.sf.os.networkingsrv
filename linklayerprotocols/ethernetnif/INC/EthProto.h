// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Historical Note: this file was originally copied from networking/ether802/inc/ethinter.h.
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined(__ETHER802_H__)
#define __ETHER802_H__

#include <networking/pktdrv.h>
#include <comms-infras/nifif.h> //needed for cnififfactory.
#include <in_sock.h>

#include <comms-infras/commsdebugutility.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_flowbinders.h>
#include <networking/pktdrv_internal.h>

#ifdef __FLOG_ACTIVE
#define TCPDUMP_LOGGING // log all ethernet frame in pcap format
#endif // __FLOG_ACTIVE

class TLanLinkProvision;
class TLanIp4Provision;
class TLanIp6Provision;

const TInt KUidEthintNifmanInterface = 0x10000541;
const TInt KAuthenticationComplete = 5;
const TInt KEtherMaxFrame = 1492;
const TInt KEtherSpeedInKbps = 10000;
const TInt KProtocolAddressSize=4; //Address size of IP4
const TInt KNetworkAddressSize=6; //Address size for Ether MAC
const TInt KArpMacMulticastMask=0x7FFFFF;


enum TEtherHeaderType{EStandardEthernet, ELLCEthernet}; // Ethernet header types

/*******************************************************************************
* A quick intro to the Ethernet packet format:
*
* A plain frame looks like this:
*
* struct EtherFrame {
*   TUint64 iDestAddr, iSrcAddr;
*   TUint16 iType; // Denotes the Protocol Type == Service Access Point == SAP
*   TUint8  iData[1500]; // Payload.
* };
*
* iType denotes the type of the payload, 0x0800 denotes IP.
*                                        0x0806 denotes ARP.
*                                        >=Min, <=1500 denotes LLC.
* A frame may also contain an extra 'LLC' field which encapsulates the original
* payload in a 5 byte header. The LLC field is somewhat complex, but here we
* confine ourselves to the LLC-SNAP variant.
*
* struct EtherLLCFrame {
*   TUint64 iDestAddr, iSrcAddr;
*   TUint16 iLen; // Denotes the Len and must be <=1500 so as not to conflict
*                 // With iType interpretation.
*   TUint8 iDSAP=0xAA; // Which denotes an LLC-SNAP frame.
*   TUint8 iSSAP=0xAA; // Which denotes an LLC-SNAP frame.
*   TUint8 iCTRL=3;    // Which denotes an LLC-SNAP frame encapsulated by
*                      // Ethernet.
*   TUint24 OUI=iDstAddr&0xffffff; // 0 is also a valid value.
*   TUint16 iType; // The actual iType as described earlier.
*   TUint8  iData[1492]; // Payload.
* };
*
*******************************************************************************/

_LIT(ETHER802_CONFIG_FILENAME, "ether802.ini");
_LIT(KInterfaceSection,"interface_settings");
_LIT(KEthernetSection,"ethernet_settings");

const TUint32 KDefaultSpeedMetricSetting = 0;
const TUint32 KDefaultMtuSetting = 1500;
const TUint32 KMACByteLength = 6;
const TUint32 KDestAddrOffset = 8;

// For the purposes of Wants, we define the protocol code as a pair:
// EtherType, IPType... we don't know the link layer above is definitely
// using IP packets...
const TUint16 KIPFrameType = 0x800;
const TUint16 KIP6FrameType = 0x86DD;
const TUint16 KArpFrameType= 0x806;
const TUint8 KLLC_SNAP_DSAP = 0xAA;
const TUint8 KLLC_SNAP_SSAP = 0xAA;
const TUint8 KLLC_SNAP_CTRL = 3;

/**
@internalComponent
*/
struct TEtherFrame
{
	TUint16 GetType() const { return BigEndian::Get16((TUint8*)&iTypeLen); }
	TUint16 iDestAddr[3];
	TUint16 iSrcAddr[3];
	TUint16 iTypeLen; // Denotes the Protocol Type <=> Service Access Point <=> SAP
	TUint8  iData[KDefaultMtuSetting]; // Payload.
};

const TUint KEtherHeaderSize=14;
const TUint KEtherLLCHeaderSize=KEtherHeaderSize+8;

/**
@internalComponent
*/
struct TEtherLLCFrame
{
	TUint16 GetType() const { return (TUint16)((iTypeHi<<8)|iTypeLo); }
	void SetType(TUint16 aType)
		{
		iTypeHi=(TUint8)(aType>>8); iTypeLo=(TUint8)(aType&0xff);
		}
	void SetDestAddr( TDesC8& aDest);
	void SetSrcAddr( TDesC8& aSrc);
	void SetOUI( TUint32 aOUI);
	TUint16 iDestAddr[3];
	TUint16 iSrcAddr[3];
	TUint16 iTypeLen;
	TUint8 iDSAP; // Which denotes an LLC-SNAP frame.
	TUint8 iSSAP; // Which denotes an LLC-SNAP frame.
	TUint8 iCtrl;    // Which denotes an LLC-SNAP frame encapsulated by
	// Ethernet.
	TUint8 OUI[3];
	TUint8 iTypeHi;
	TUint8 iTypeLo; // The actual iType as described earlier.
	TUint8 iData[1492]; // Payload.
};

class CLanxBearer;
typedef CArrayPtrFlat<CLanxBearer> CLanxBearerPtrArray;

class CEthLog;

/**
Main LAN Nif Control object creates lower layer objects :-
Packet Driver & EtherXXX
Calls back to protocol (iProtocol) and nifman (iNotifier in CNifIfBase)
Calls down to EtherXX (iMacLayer)
@internalComponent
*/
class CLANLinkCommon : public ESock::CSubConnectionFlowBase, public ESock::MFlowBinderControl
{
	friend class CPktDrvBase;
public:
	enum TAction
		{
		EReconnect,
		EDisconnect
		};
	IMPORT_C static CLANLinkCommon* NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
	IMPORT_C CLANLinkCommon(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
	IMPORT_C virtual ~CLANLinkCommon();
	IMPORT_C void ConstructL();

	// from MFlowBinderControl
	IMPORT_C virtual ESock::MLowerControl* GetControlL(const TDesC8& aProtocol);
	IMPORT_C virtual ESock::MLowerDataSender* BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	IMPORT_C virtual void Unbind(ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual ESock::CSubConnectionFlowBase* Flow()
    	{
    	return this;
    	}

	// from Messages::ANode
	IMPORT_C virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

	// Dispatch functions for messages received from SCPR.

	void StartFlowL();
	void CancelStartFlow();
	void StopFlow(TInt aError);
	void SubConnectionGoingDown();
	void SubConnectionError(TInt aError);
	void BindToL(Messages::TNodeId& aCommsBinder);

	void Destroy();

	// Utility functions for sending messages to SCPR.
	void PostProgressMessage(TInt aStage, TInt aError);
	void PostDataClientStartedMessage();
	void PostFlowDownMessage(TInt aError);
	void PostFlowGoingDownMessage(TInt aError, MNifIfNotify::TAction aAction);
	void MaybePostDataClientIdle();

	// CSubConnectionFlowBase
	MFlowBinderControl* DoGetBinderControlL()
    	{
    	return this;
    	}

	//
	// Upcalls from the packet driver
	//
	IMPORT_C void Process(RMBufChain& aPdu, TAny* aMAC=0);
	IMPORT_C void ResumeSending(); // Flow Control unblocked
	IMPORT_C void LinkLayerUp(); // Notify the Protocol that the line is up
	IMPORT_C void LinkLayerDown(TInt aError, TAction aAction); // Notify the Protocol that the line is down
	IMPORT_C void FoundMACAddrL(); // Added for interfaces that take a while to find their MAC addrs

	IMPORT_C TInt ReadInt(const TDesC& aField, TUint32& aValue);
    IMPORT_C TInt WriteInt(const TDesC& aField, TUint32 aValue);
    IMPORT_C TInt ReadDes(const TDesC& aField, TDes8& aValue);
    IMPORT_C TInt ReadDes(const TDesC& aField, TDes16& aValue);
    IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC8& aValue);
	IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC16& aValue);
	IMPORT_C TInt ReadBool(const TDesC& aField, TBool& aValue);
    IMPORT_C TInt WriteBool(const TDesC& aField, TBool aValue);
	IMPORT_C void IfProgress(TInt aStage, TInt aError);
	IMPORT_C void IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	IMPORT_C void NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);

	TUint Mtu() const;

	TUint SpeedMetric() const {return KEtherSpeedInKbps; }
	TEtherHeaderType EtherType() const { return iEtherType; }
	const TBuf8<KMACByteLength>& MacAddress() const;

	TInt FrameSend(RMBufChain& aPdu, TAny* aSource=0, TUint16 aType=KIPFrameType);

	void SetAllowedBearer(CLanxBearer* aBearer);
    TBool BearerIsActive(CLanxBearer* aBearer);

private:
	//
	// Configuration methods
	TBool ReadMACSettings();
	void ReadEthintSettingsL();
	TInt EtherFrame(RMBufChain &aChain, TUint16 aType=KIPFrameType);
	TBool CheckMac(TDes8& aMacAddr); // checks that the packet driver has returned a valid MAC address,
	//New.
	TInt GetProtocolType(RMBufChain& aPdu, TUint16& aEtherType, TUint8*& aPayloadPtr, TUint& aEtherHeaderSize) const;
	//
	// Driver managment
	void LoadPacketDriverL();

	// Search CLanxBearer array by various keys (used in bind/unbind/provisioning)
	CLanxBearer* FindBearerByProtocolName(const TDesC8& aProtocol, TInt& aIndex) const;
	CLanxBearer* FindBearerByUpperControl(const ESock::MUpperControl* aUpperControl, TInt& aIndex) const;

	// Provisioning
	void ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData);
	void ProvisionConfigL();
	void ProvisionBearerConfigL(const TDesC8& aName);
	const TLanLinkProvision& LinkProvision();

protected:
	// Creates packet driver methods
	CPktDrvFactory* DoCreateDriverFactoryL(TUid aUid2,const TDesC& aFilename);
	CPacketDriverOwner* iPktDrvOwner; // We create this but do not own it

	// This class creates the packet driver and binds it to the mac layer
	CPktDrvBase*		iPktDrv;

	//MAC Address
	TBuf8<KMACByteLength> iMacAddr;
	//indicates if iMacAddr contains a valid Mac address
	TBool iValidMacAddr;

	TEtherHeaderType	iEtherType;

	// Bearer List:
	CLanxBearerPtrArray  *iBearers;
	CLanxBearer			 *iOnlyThisBearer;

	#ifdef TCPDUMP_LOGGING
	private:
		CEthLog* iLogger;
	#endif // TCPDUMP_LOGGING

	// Provisioning related
	const TLanLinkProvision* iLanLinkProvision;		// link provisioning information from SCPR
	const TLanIp4Provision* iLanIp4Provision;		// ip4 provisioning information from SCPR
	const TLanIp6Provision* iLanIp6Provision;		// ip6 provisioning information from SCPR
	TBool iStopRequested;

	TInt iSavedError;								// saved errors from processing TProvisionConfig message

	enum TMeshMashineFlowState
      	{
      	EStopped,
      	EStarting,
      	EStarted,
      	EStopping,
      	};
	TMeshMashineFlowState iMMState;
};

inline const TBuf8<KMACByteLength>& CLANLinkCommon::MacAddress() const
{
	return iMacAddr;
}



#endif
