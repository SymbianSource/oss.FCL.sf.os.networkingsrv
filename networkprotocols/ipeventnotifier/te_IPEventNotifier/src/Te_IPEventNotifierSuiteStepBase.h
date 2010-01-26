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
//

/**
 @file
 @internalComponent
*/

#if (!defined __TE_IPEVENTNOTIFIER_STEP_BASE__)
#define __TE_IPEVENTNOTIFIER_STEP_BASE__
#include <test/testexecutestepbase.h>
#include <c32root.h>
#include <e32cmn.h>

#include <comms-infras/idquerynetmsg.h>
#include <comms-infras/netsubscribe.h>
#include <es_sock.h>
#include <comms-infras/asynchevent.h>

namespace NetSubscribe
{
class TEvent;
}

class CFactoryChannel : public CBase
	{
public:
	~CFactoryChannel();
	CFactoryChannel()
		{
		}
	void SendMessageL( NetMessages::CMessage& aMsg );
	
protected:
	TCFModuleName iModule;
	Elements::TRBuf8 iBuf;
	RRootServ iC32Root;
	};

class CDhcpSignal : public CFactoryChannel
	{
public:
	~CDhcpSignal();
	CDhcpSignal()
		{
		}
	void SubscribeL( const TName& aInterfaceName, TInt aEventId, NetSubscribe::TEvent& aEvent );
	void UnsubscribeL( NetSubscribe::TEvent& aEvent );

protected:
	NetSubscribe::CNetSubscribe* iNetSubscribe;
	NetMessages::CTypeIdQuery* iQuery;
	};


class CTE_IPEventNotifierSuiteStepBase;

class CTimeout : public CTimer
	{
protected:
	CTimeout(CTE_IPEventNotifierSuiteStepBase* aTestStep);
	
public:
	static CTimeout* NewLC(CTE_IPEventNotifierSuiteStepBase* aTestStep);
	virtual void ConstructL();
	virtual void RunL();
private:
	CTE_IPEventNotifierSuiteStepBase* iTestStep;
	};




class TSoInet6InterfaceInfo;

/****************************************************************************
  The reason to have a new step base is that it is very much possible
  that the all individual test steps have project related common variables 
  and members 
  and this is the place to define these common variable and members.
  
****************************************************************************/
class CTE_IPEventNotifierSuiteStepBase : public CTestStep
	{
public:
	virtual ~CTE_IPEventNotifierSuiteStepBase();
	CTE_IPEventNotifierSuiteStepBase();
	virtual TVerdict doTestStepPreambleL(); 
	virtual TVerdict doTestStepPostambleL();

	void StartConnectionL(TInt aIAP);
	const TName GetFullInterfaceNameL(const TDesC& aSubName);
	void GetInterfaceInfoL(const TDesC& aName, TPckgBuf<TSoInet6InterfaceInfo>& aInfo);

protected:
	HBufC8*		iReadData;
	HBufC8*		iWriteData;
	
	RSocketServ* iSocketServ;
	RConnection* iRConnection;
	RSocket* iSocket;
	
	CActiveScheduler* iActSched;
	};






#endif
