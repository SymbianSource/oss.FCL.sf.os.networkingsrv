// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __TUN_H__
#define __TUN_H__

#include <posthook.h>
#include <nifif.h>
#include "tunfamily.h"
#include "tun.pan"

const TUint KProtocolTUN    = 145; // number that is unassigned by IANA
const TUint KTunTos         = 192; // 0xC0; uses the UNUSED 7,8 MSB of Differentiated Services  

class CProtocolTun;
class CTunFlowInfo;
class CNifIfBase;

class CSapTun : public CServProviderBase
/** 
 *  Class Derived from CServProviderBase. 
 *  Inherit virtual functions from CServProviderBase.This Class is socket support for this 
 *  Hook (protocol).Protocol or Hook could be manipulated by the Socket using socket options.
 *  This class will be used to take downlink information from the private client.The SAP will 
 *  called for all the Socket opened by the socket. SAP is 1 to 1 with Socket.
 *  we are maintaining a singleton pattern for this SAP
 **/
    {

public:
    friend class CProtocolTun;

protected:
    CSapTun();
    ~CSapTun();
    
public:
    //static CSapTun* GetInstanceL();
    void Ioctl(TUint /*level*/,TUint /* name*/,TDes8* /* anOption*/);
    void Start();
    void Shutdown(TCloseType option);
    void LocalName(TSockAddr& anAddr) const; 
    TInt SetLocalName(TSockAddr& anAddr); 
    void RemName(TSockAddr& anAddr) const;
    TInt SetRemName(TSockAddr& anAddr);
    TInt GetOption(TUint /*level*/,TUint /*name*/,TDes8& /*anOption*/)const;    
    void CancelIoctl(TUint /*aLevel*/,TUint /*aName*/);
    TInt SetOption(TUint aLevel,TUint aName,const TDesC8& anOption); 
    void ActiveOpen();
    void ActiveOpen(const TDesC8& /*aConnectionData*/);
    TInt PassiveOpen(TUint /*aQueSize*/);
    TInt PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/);
    void Shutdown(TCloseType /*option*/,const TDesC8& /*aDisconnectionData*/);
    void AutoBind();
    TInt SecurityCheck(MProvdSecurityChecker* aChecker);

private:
    //static CSapTun* iInstance;
    CProtocolTun* iProtocol;
    CTunFlowInfo* iFlowInfo;
    };  //End Class CProtocolSap

//CSapTun* CSapTun::iInstance = NULL;

class CProtocolTun : public CProtocolPosthook
    {
    friend class CSapTun;
public:  // Constructors and destructor

    /**
     * Constructor.
     */

    CProtocolTun ();

    /* 
     * Two Phase Construction
     */
    static CProtocolTun* NewL();

    /**
     * Destructor.
     */
    virtual ~CProtocolTun ();

public: // Functions from base classes

    /**
     * From CProtocolBase  
     * Used as second phase of Two Phase construction
     * @param aTag Name
     * @return void
     */
    void ConstructL ();

    /**
     * From CProtocolBase  
     * @since 
     * @param aDesc Descriptor to be filled by this method.
     * @return void
     */
    virtual void Identify (TServerProtocolDesc * aDesc) const;

    /**
     * Static method that can be invoked from the Protocol Family 
     * @since 
     * @param aDesc Descriptor to be filled by this method.
     * @return void
     */
    static void Identify (TServerProtocolDesc & aDesc);

    /**
     * From CProtocolPostHook Handles attach to the stack
     * @since
     * @return void
     */
    void NetworkAttachedL();

    /**
     * From CProtocolPostHook Handles detach from the stack
     * @since
     * @return void
     */
    void NetworkDetached();

    /**
     * From MIp6Hook Handles received packets. 
     * @since 
     * @param aPacket Packet received.
     * @param aInfo Info structure.
     * @return void
     */       
    virtual TInt ApplyL (RMBufHookPacket & /* aPacket */ ,RMBufRecvInfo & /* aInfo */ );

    /**
     * From MIp6Hook Handles outbound opening. 
     * @since 
     * @param aHead Flow-specific precomputed data.
     * @param aFlow Flow.
     * @return void
     */       
    virtual MFlowHook *OpenL (TPacketHead & aHead, CFlowContext * aFlow);

    /**
     * NewSAPL function to create a CSapTun instance
     * and use for setting up the LocalAddress Information
     * @param aProtocol ignored in our case.
     * @return instance of the CServProviderBase instance CSapFullTunnel
     */
    CServProviderBase* NewSAPL(TUint aProtocol);

    /**
     *  Cancels the Sap creation
     *  @param the Sap instance that has to be removed 
     */ 
  //  void CancelSap(CSapTun *aSap);

    /**
     *  Sets the NifBase instances to be used for Control functionality
     *  @param the NifBase instance that will be retained.
     */
    void SetCNifBase(CNifIfBase* aNifBase)
        {
        iNifBase = aNifBase;
        }
    
    void SetAppPortNum(TUint aPort)
        {
        iAppPortNum = aPort;
        iPortSet = ETrue;
        }
    
    TBool IsPortSet()
        {
        return iPortSet;
        }


private:
    TUint iAppPortNum;  // Tunnel port configured.
    TBool iPortSet;     //Bool to check whether the port is set/not.
    RPointerArray<CSapTun> iSapList;  // List of SAP instances created.
    CNifIfBase* iNifBase;    // CNifBase* instance
    CTunFlowInfo* iFlowinfo;   // instance of the Flow Hook
    CSapTun* iSapInstance; // instance of sap

    };

/**
 *  TUN Driver flow hook
 *  Outbound Flow hook and performs actions on the packets
 *  destined for the Virtual Tunnel Nif.
 */
class CTunFlowInfo : public CBase, public MFlowHook
    {
    friend class CProtocolTun;
    friend class CSapTun;

public: // Constructors & destructors

    /**
     * Constructor.
     */

    CTunFlowInfo();

    /**
     * Copy Constructor.
     */

    CTunFlowInfo(CTunFlowInfo* aInstance )
    :iAppPortNum(aInstance -> iAppPortNum)
        {}

    /**
     * Destructor.
     */

    ~CTunFlowInfo();


public: // Functions from base classes

    /**
     * From MFlowHook Open and Close implements reference counting system.
     * @since
     * @param
     * @return void
     */
    void Open ();

    /**
     * From MFlowHook Informs stack if the hook is ready. Can update address
     * information.
     * @since
     * @param aHead Address information of the flow.
     * @return TInt 0 => ready, >0 => pending, <0 => error.
     */
    TInt ReadyL (TPacketHead & aHead);

    /**
     * Called by IP when processing outbound packet.
     * @since
     * @param aPacket Complete packet
     * @param aInfo Information block associated with the packet
     * @return TInt 0 => processed, <0 => discarded, >0 => restart processing
     */
    TInt ApplyL (RMBufSendPacket & aPacket, RMBufSendInfo & aInfo);

    /**
     * From MFlowHook Open and Close implements reference counting system.
     * @since
     * @param
     * @return void
     */
    void Close ();
    
    void SetAppPortNum(TUint aPort)
        {
        iAppPortNum = aPort;
        }
    
private:    // Data

    TInt iRef; // Reference count
    TUint iAppPortNum; // Tunnel port configured.

    };


#endif //__TUN_H__
