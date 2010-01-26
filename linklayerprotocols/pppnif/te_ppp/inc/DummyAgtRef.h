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
// Defines the interface to a "dummy" Nifman Agent Ref.
// 
//

/**
 @file
*/

#ifndef __DUMMYAGTREF_H__
#define __DUMMYAGTREF_H__

#include "nifagt.h"
#include "nifif.h"
#include "es_ini.h"
#include <testexecutelog.h>
#include <testexecutestepbase.h>
#include "common.h"

class CTestMgr;
class CDummyProtocol;
class CESockIniData;
class CCommonData;

class CDummyNifAgentRef : public CBase,
								 public MNifAgentNotify,
								 public MNifIfNotify
{
public:
	static CDummyNifAgentRef* NewL(const TDesC& aName,CTestMgr& aTheMgr, CTestExecuteLogger& aLogger);
	~CDummyNifAgentRef();
	//
	bool CreateIniReader();
	void DestroyIniReader();
	//
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
    virtual TName Name() const;
	virtual void Close();

	void Stop();

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
	/**
	CNifAgentRef implements a state machine. 
	Dummy NifAgentRef implements a simplified version of that state machine, sufficient for	testing with ANVL.  
	This enum should be extended as necessary.	*/
	enum TState
		{
		/** Link layer is up: NIF is Up. */
		ELinkUp,
		/** Link layer is in the process of being stopped: NIF is negotiating termination */
		EStopping,
		/** Link layer is down: NIF has terminated */
		EStopped	
		};
private:
	//
	CDummyNifAgentRef(const TDesC& aName,CTestMgr& aTheMgr, CTestExecuteLogger& aLogger);
	void ConstructL();
	void ServiceStartedL();
	//attributes
	TDesC iName;
	CNifIfLink* iInterface;
	CNifIfBase* iInterfaceBound;
	CTestMgr& iTheMgr;
	CDummyProtocol* ipDummyPrt; //psydo protocol to bind with the ppp.
	CESockIniData* ipIniFileReader;
	CTestExecuteLogger iLogger;
	CCommonData iData;
	/** The state of the AgentRef (Reflects the state of the NIF associated with the AgentRef. */
	TState iState;
};

//
//inlines
inline bool
CDummyNifAgentRef::CreateIniReader()
{
		//if the it's imposible to open ppp.ini -> will use hardcoded info
	TRAPD(result,ipIniFileReader=CESockIniData::NewL(iData.KPppIniFullPath));
	if (result != KErrNone)
	{
		ipIniFileReader=0;
	}
	
	return ipIniFileReader != 0 ;
}

inline void
CDummyNifAgentRef::DestroyIniReader()
{
		delete ipIniFileReader;
}

#endif //__DUMMYAGTREF_H__
