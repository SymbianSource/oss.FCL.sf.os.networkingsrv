// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#if (!defined __TEST_STEP_QOS_H__)
#define __TEST_STEP_QOS_H__

#include <in_sock.h>
#include <qoslib.h>

#ifdef SYMBIAN_NETWORKING_UMTSR5
#include <umtsextn.h>
#else
#include <umtsapi.h>
#endif // SYMBIAN_NETWORKING_UMTSR5


#include <cs_subconparams.h>
#include <cs_subconevents.h>
#include <es_sock.h>

#ifdef SYMBIAN_NETWORKING_UMTSR5
#include "imsextn.h"
//#include "sblpextn.h"
#else
//#include "sblpapi.h"
#endif // SYMBIAN_NETWORKING_UMTSR5

#include "uscl_ip_subconparams.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

class CTestSuite;
class CTestSuiteQoS;


//
// Class for listening the qos events
//
class CQoSListener : public CActive
{
public:
    static CQoSListener* NewL();
    ~CQoSListener();

    void Start();

private:
    CQoSListener();
    void ConstructL();
    
    // from CActive
    void RunL();
    void DoCancel();
    
    //
    RTimer iTimer;
    TInt iInterval;
    TInt iTimeSpent, iMaxTimeSpent;
//  TInt iEventType;
};


//
// Class for listening the RSubConnection events
//
class CEventListener : public CActive
{
public:
    static CEventListener* NewL();
    ~CEventListener();

    void Start();

private:
    CEventListener();
    void ConstructL();
    
    // from CActive
    void RunL();
    void DoCancel();
    
public: 
    TNotificationEventBuf iSubConnNotifBuf;
};


//
// Class for listening the RSubConnection events
//
class CSubConnListener : public CActive
{
public:
    static CSubConnListener* NewL();
    ~CSubConnListener();

    void Start();

private:
    CSubConnListener();
    void ConstructL();
    
    // from CActive
    void RunL();
    void DoCancel();
    
    RTimer iTimer;
    TInt iInterval;
    TInt iTimeSpent, iMaxTimeSpent;

public: 
    TNotificationEventBuf iSubConnNotifBuf;
};


class CTestStepQoS : public CTestStep, public MQoSObserver
{
public:
    CTestStepQoS();
    ~CTestStepQoS();

    void Event(const CQoSEventBase& aEvent);
    void WaitForQoSEventL();
    void WaitForSubConnEventL();
    TInt SendData(RSocket& aSocket);
	TInt WakeupNifL();
	TInt NifAPIL(RSocket& aSocket, const TUint aCommand);
    TBool CheckTestFile(const TDesC& aFileName);
    TInt CreateTestFile(const TDesC& aName);
    TInt ClearTestFiles();
    TPtrC QoSEventToText(TInt aEvent);

    // fill the parameters with predefined values
    void SetQoSParamDefault(CQoSParameters& aParameters);
    void SetQoSParamSet1(CQoSParameters& aParameters);
    void SetQoSParamSet12(CQoSParameters& aParameters);
    void SetQoSParamSet2(CQoSParameters& aParameters);
    void SetQoSParamSet3(CQoSParameters& aParameters);
    // To create non-real time traffic class
    void SetQoSParamSetForNRT(CQoSParameters& aParameters);
    void SetQoSParamSetArbitrary(CQoSParameters& aParameters);

    void SetEsockParamSet1(CSubConQosGenericParamSet& aParameters);
    void SetEsockParamSet2(CSubConQosGenericParamSet& aParameters);
    
    // for umts r5 tests
    void SetEsockParamSetWithPriorityOne(CSubConQosGenericParamSet& aParameters);
    
    TInt CheckEvent(TNotificationEventBuf& aEvent, 
                    const TUint32 aExpectedEvent);
                    
	TInt CheckImsExtensionValueL(RSubConnection &aSubconn, TBool aImsFlagValue);
	TInt CheckUmtsR5ExtensionValuesL(RSubConnection &aSubconn, 
									TBool aSignallingIndicator, 
									RPacketQoS::TSourceStatisticsDescriptor aSourceStatisticsDescriptor);    

    void SetIPLinkR99HighDemand(CSubConQosIPLinkR99ParamSet& aParameters);
    void SetIPLinkR99HighDemandMin(CSubConQosIPLinkR99ParamSet& aParameters);
    void SetIPLinkR99HighDemandNegotiated(CSubConQosIPLinkR99ParamSet& 
         aParameters);
    void SetIPLinkR99Background(CSubConQosIPLinkR99ParamSet& aParameters);
    void SetIPLinkR99BackgroundMin(CSubConQosIPLinkR99ParamSet& aParameters);
    
    void SetUmtsTestPol1(TUmtsQoSParameters& aParameters);
    void SetUmtsTestMinPol1(TUmtsQoSParameters& aParameters);
    void SetUmtsQoSHighDemand(TUmtsQoSParameters& aParameters);
    void SetUmtsQoSHighDemandMin(TUmtsQoSParameters& aParameters);
    void SetUmtsQoSHighDemandNegotiated(TUmtsQoSParameters& aParameters);

    void SetSetUmtsQoSBestEffort2(TUmtsQoSParameters& aParameters);
    void SetSetUmtsQoSBestEffortMin2(TUmtsQoSParameters& aParameters);

	void SetUmtsQoSBackground(TUmtsQoSParameters& aParameters);
	void SetUmtsQoSBackgroundMin(TUmtsQoSParameters& aParameters);
	
	#ifdef SYMBIAN_NETWORKING_UMTSR5
	void SetUmtsR5QoSBackground(TUmtsR5QoSParameters& aParameters);
	void SetUmtsR5QoSBackgroundMin(TUmtsR5QoSParameters& aParameters);
	void SetUmtsR5QoSRequested1(CSubConQosR5ParamSet& aParameters);
	void SetUmtsR5QoSAcceptable1(CSubConQosR5ParamSet& aParameters);
	#endif // SYMBIAN_NETWORKING_UMTSR5
	
    // pointer to suite which owns this test 
    CTestSuiteQoS * iQoSSuite;

    //
    TInt iReceivedEvent;
    TInt iReason;
    CQoSParameters *iParameters;

    CQoSListener *iListener;
	CSubConnListener       *iSubConnListener;
	CEventListener         *iEventListener;
};


#endif /* __TEST_STEP_QOS_H__ */
