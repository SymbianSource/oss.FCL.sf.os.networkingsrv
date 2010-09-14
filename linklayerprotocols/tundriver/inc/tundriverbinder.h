/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Header file for the Ip4 and Ip6 Binder Configuration.
* 
*
*/

/**
 @file tundriverbinder.h
 @internalTechnology
*/

#ifndef _TUNDRIVERBINDER_H
#define _TUNDRIVERBINDER_H

#include <ip4_hdr.h>
#include <ip6_hdr.h>
#include <udp_hdr.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_flowbinders.h>
#include <comms-infras/commsdebugutility.h>
#include <nifmbuf.h>
#include "es_protbinder.h"
#include "tundriverflow.h"

const TInt KMTU = 1500;

const TInt KSpeedMetric = 0;

const TUint KTUNDriverTos      = 192; // 0xC0; uses the UNUSED 7,8 MSB of Differentiated Services

class CTunDriverSubConnectionFlow;

class CTunDriverBinder : public CBase, public ESock::MLowerDataSender, public ESock::MLowerControl
/**
Common Binder for the tunnel driver binder. This binder will be inherited by IPv4 or IPv6 Binder classes.
*/
    {
public:
    virtual MLowerDataSender* Bind(ESock::MUpperDataReceiver& aUpperReceiver , ESock::MUpperControl& aUpperControl);
    virtual void Unbind (ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aUpperControl);
    virtual TBool MatchesUpperControl(ESock::MUpperControl* aUpperControl) const;
    virtual TInt Control(TUint, TUint, TDes8&);
    void SetPort(TUint aPort);
    TUint GetPort();
protected:
    CTunDriverBinder(CTunDriverSubConnectionFlow& aTunDriverSubConnectionFlow);
protected:
    __FLOG_DECLARATION_MEMBER;
private:
    CTunDriverSubConnectionFlow& iTunDriverSubConnectionFlow;
    TUint iPort;
    };

class CTunDriverBinder4 : public CTunDriverBinder
    {
public:
    static CTunDriverBinder4* NewL(CTunDriverSubConnectionFlow& aLink);
    ~CTunDriverBinder4();
    // from MLowerDataSender
    virtual TInt GetName(TDes& aName);
    virtual TInt GetConfig(TBinderConfig& aConfig);
    MLowerDataSender* Bind(ESock::MUpperDataReceiver& aUpperReceiver , ESock::MUpperControl& aUpperControl);
    void Unbind (ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aUpperControl);
    TBool MatchesUpperControl(ESock::MUpperControl* aUpperControl) const;
    ESock::MLowerDataSender::TSendResult Send(RMBufChain& aData);
    void StartSending();
private:
    CTunDriverBinder4(CTunDriverSubConnectionFlow& aLink);
    void ConstructL();
    void TunnelProcessing(RMBufPacket& aPacket, RMBufPacket& aRecv);
    inline CTunDriverSubConnectionFlow* Flow();
    virtual void DoSend();
    virtual void DoProcess();
    static TInt RecvCallBack(TAny* aCProtocol);
    static TInt SendCallBack(TAny* aCProtocol);
private:
    TInetAddr iLocalAddress;
    CTunDriverSubConnectionFlow& iTunDriverSubConnectionFlow;
    ESock::MUpperDataReceiver* iUpperReceiver;
    ESock::MUpperControl* iUpperControl;
    RMBufPktQ iSendQ;
    RMBufPktQ iRecvQ;
    CAsyncCallBack* iSendCallBack;
    CAsyncCallBack* iRecvCallBack;
    };

CTunDriverSubConnectionFlow* CTunDriverBinder4::Flow()
    {
    return &iTunDriverSubConnectionFlow;
    }

#ifdef IPV6SUPPORT
class CTunDriverBinder6 : public CTunDriverBinder
    {
public:
    static CTunDriverBinder6* NewL(CTunDriverSubConnectionFlow& aLink);
    ~CTunDriverBinder6();
    // from MLowerDataSender
    virtual TInt GetName(TDes& aName);
    virtual TInt GetConfig(TBinderConfig& aConfig);
    MLowerDataSender* Bind(ESock::MUpperDataReceiver& aUpperReceiver , ESock::MUpperControl& aUpperControl);
    void Unbind (ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aUpperControl);
    TBool MatchesUpperControl(ESock::MUpperControl* aUpperControl) const;
    ESock::MLowerDataSender::TSendResult Send(RMBufChain& aData);
    void StartSending();
private:
    CTunDriverBinder6(CTunDriverSubConnectionFlow& aLink);
    void ConstructL();
    void TunnelProcessing(RMBufPacket& aPacket, RMBufPacket& aRecv);
    inline CTunDriverSubConnectionFlow* Flow();
    virtual void DoSend();
    virtual void DoProcess();
    static TInt RecvCallBack(TAny* aCProtocol);
    static TInt SendCallBack(TAny* aCProtocol);
private:
    TInetAddr iLocalAddress;
    CTunDriverSubConnectionFlow& iTunDriverSubConnectionFlow;
    ESock::MUpperDataReceiver* iUpperReceiver;
    ESock::MUpperControl* iUpperControl;
    RMBufPktQ iSendQ;
    RMBufPktQ iRecvQ;
    CAsyncCallBack* iSendCallBack;
    CAsyncCallBack* iRecvCallBack;
    };
#endif 
#endif // _TUNDRIVERBINDER_H
