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
// TestStepSelfTest.h
// This contains CTestStepSelfTest which is the base class for all 
// the Psd Agx suite test steps
// 
//

#if (!defined __SELF_TEST_TESTSTEP_H__)
#define __SELF_TEST_TESTSTEP_H__

#include "TestStep.h"

#include "TS_QoSSuite.h"
#include "CMultipleArray.h"

_LIT16(Socket,"Socket");
_LIT16(Channel,"QoSChannel");


class CTS_QoSSuite;
class CTS_QoSStep : public CTestStep
{
public:
	CTS_QoSStep();
	~CTS_QoSStep();

	TBool GetIpAddressFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TInetAddr &anAddr);
	void SetupConnectionPrefrencesL(TUint aKProtocolInet,  TInt aQoSChannelNum, TInt aSocketNum);
	void SetupQoSChannelParametersL(const TDesC &qoSParameterRefrence, TInt i);

	// Nokia's Wait for QoS Request Implementation
	void WaitForQoSEvent();

	// pointer to suite which owns this test 
	CTS_QoSSuite * iQoSSuite;

	TInt iReceivedEvent;

private:

};

// CQoSListener is activeObject which implements the loop to check
// whether I have received the event

//
// Class for listening the qos events
//
class CQoSListener : public CActive
{
public:
	static CQoSListener* NewL(CTS_QoSStep*);
	~CQoSListener();
	void Start();

private:
	CQoSListener();
	void ConstructL(CTS_QoSStep*);
	
	// from CActive
	void RunL();
	void DoCancel();
	
	RTimer iTimer;
	TInt iInterval;
	TInt iTimeSpent, iMaxTimeSpent;
	// Pointer to CTS_QoSStep - All test steps inherit this class (QoSSocketSection, QoSOperationSection, so on....)
	CTS_QoSStep * iTestStepQoS;
};

#endif /* __SELF_TEST_TESTSTEP_H__ */
