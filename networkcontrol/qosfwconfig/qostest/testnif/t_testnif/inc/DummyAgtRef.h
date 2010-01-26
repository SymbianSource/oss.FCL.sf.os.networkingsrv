// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __DUMMYAGTREF_H__
#define __DUMMYAGTREF_H__

#include "NIFAGT.H"
#include "NIFIF.H"
#include "es_ini.h"

//Nasty classes to sneak into protected sections
class CCheekyNifIfBase:public CNifIfBase
{
public:
	void SetNotify(MNifIfNotify* aNotify){iNotify=aNotify;}
};
class CCheekyNifIfFactory : public CNifIfFactory
{
public:
	CNifIfBase*
		CreateInterfaceL(const TDesC& aName){ return this->NewInterfaceL(aName);}
};
class CCheekyNifAgentBase: public CNifAgentBase
{
public:
	void SetNotify(MNifAgentNotify* aNotify){iNotify=aNotify;}
};
class CCheekyNifAgentFactory : public CNifAgentFactory
{
public:
	CNifAgentBase*
		CreateAgentL(const TDesC& aName){ return this->NewAgentL(aName);}
};
class CTestMgr;
class CDummyProtocol;

class CDummyNifAgentRef : public CBase, public MNifAgentNotify, public MNifIfNotify
{
public:
	static CDummyNifAgentRef* NewL(const TDesC& aName,CTestMgr& aTheMgr);
	virtual ~CDummyNifAgentRef();
	void StartL(CNifAgentFactory* aFactory);

	// MNifIfNotify overrides
    virtual void LinkLayerDown(TInt aReason, TAction aAction);
	virtual void LinkLayerUp();
    virtual void NegotiationFailed(CNifIfBase* aIf, TInt aReason);
    virtual TInt Authenticate(TDes& aUsername, TDes& aPassword);
    virtual void CancelAuthenticate();
	virtual TInt GetExcessData(TDes8& aBuffer);
    virtual void IfProgress(TInt aStage, TInt aError);
    virtual void IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	virtual void OpenRoute();
	virtual void CloseRoute();
	virtual TInt Notification(TNifToAgentEventType aEvent, void* aInfo);
	virtual void BinderLayerDown(CNifIfBase* aBinderIf, TInt aReason, TAction aAction);
	virtual TInt PacketActivity(TDataTransferDirection aDirection, TUint aBytes, TBool aResetTimer);
	virtual void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume);
	virtual void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume);
	virtual void NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);

	//
	// MNifAgentNotify overrides
	virtual void ConnectComplete(TInt aStatus);
	virtual void ReconnectComplete(TInt aStatus);
	virtual void AuthenticateComplete(TInt aStatus);
	virtual void ServiceStarted();
	virtual void ServiceClosed();
	virtual void DisconnectComplete();
	virtual void AgentProgress(TInt aStage, TInt aError);
	virtual	void AgentProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo);
	virtual TInt IncomingConnectionReceived();
	virtual void AgentEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0);

    TName Name() const;
	void SetInterface(CNifIfLink* alink);
	void SetNifIfBase(CNifIfBase* aBase);
	CDummyProtocol* GetDummyProtocol(void);

protected:
	virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage);
    virtual TInt DoWriteInt( const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage);
    virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage);
    virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage);
    virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage);
    virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage);
	virtual TInt DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage);
    virtual TInt DoWriteBool( const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage);

private:
	//
	CDummyNifAgentRef(const TDesC& aName,CTestMgr& aTheMgr);
	void ConstructL();
	void ServiceStartedL();

	TDesC iName;
	CNifIfLink*		iInterface;
	CNifIfBase*		iInterfaceBound;
	CTestMgr&		iTheMgr;

	CNifAgentBase*	iAgent;
	CESockIniData* ipIniFileReader;
};



#endif //__DUMMYAGTREF_H__
