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
// ExampleTest.cpp
// This file contains an example Test step implementation 
// This demonstrates the various functions provided
// by the CTestStep base class which are available within
// a test step 
// 
//


#if (!defined __QOSOPERATIONSECTION_H__)
#define __QOSOPERATIONSECTION_H__

#include "TS_QoSStep.h"
#include "TS_QoSSuite.h"
#include "TS_QoSSocketSection.h"

_LIT16(QoSChannel,"QoSChannel");
_LIT16(QoS,"QoS");

/* QoS TestCase 2_0 Class Definition
 */
class CTS_QoSOperationSection2_0 : public CTS_QoSStep
{
public:
	CTS_QoSOperationSection2_0();
	~CTS_QoSOperationSection2_0();
	virtual enum TVerdict doTestStepL( void );

private:
	TInt i;

};

/* QoS TestCase 2_1 Class Definition
 */
class CTS_QoSOperationSection2_1 : public CTS_QoSStep
{
public:
	CTS_QoSOperationSection2_1();
	~CTS_QoSOperationSection2_1();
	virtual enum TVerdict doTestStepL( void );

private:
	TInt i2;

};

/* QoS TestCase 2_2 Class Definition
 */
class CTS_QoSOperationSection2_2 : public CTS_QoSStep
{
public:
	CTS_QoSOperationSection2_2();
	~CTS_QoSOperationSection2_2();
	virtual enum TVerdict doTestStepL( void );

};

/* QoS TestCase 2_3 Class Definition
 */
class CTS_QoSOperationSection2_3 : public CTS_QoSStep, public MQoSObserver
{
public:
	CTS_QoSOperationSection2_3();
	~CTS_QoSOperationSection2_3();
	virtual enum TVerdict doTestStepL( void );
	virtual void Event(const CQoSEventBase& aEvent);

private:
	CTS_CEsockSendAndRecvData* sendAndRecv;

	TInt reason, i;
};

/* QoS TestCase 2_4 Class Definition
 */
class CTS_QoSOperationSection2_4 : public CTS_QoSStep, public MQoSObserver
{
public:
	CTS_QoSOperationSection2_4();
	~CTS_QoSOperationSection2_4();
	virtual enum TVerdict doTestStepL( void );
	virtual void Event(const CQoSEventBase& aEvent);

private:
	TInt reason, i, i2;
};

/* QoS TestCase 2_5 Class Definition
 */
class CTS_QoSOperationSection2_5 : public CTS_QoSStep, public MQoSObserver
{
public:
	CTS_QoSOperationSection2_5();
	~CTS_QoSOperationSection2_5();
	virtual enum TVerdict doTestStepL( void );
	virtual void Event(const CQoSEventBase& aEvent);

private:
	TInt reason, i, i2;
};

/* QoS TestCase 2_6 Class Definition
 */
class CTS_QoSOperationSection2_6 : public CTS_QoSStep
{
public:
	CTS_QoSOperationSection2_6();
	~CTS_QoSOperationSection2_6();
	virtual enum TVerdict doTestStepL( void );

};

/* QoS TestCase 2_7 Class Definition
 */
class CTS_QoSOperationSection2_7 : public CTS_QoSStep, public MQoSObserver
{
public:
	CTS_QoSOperationSection2_7();
	~CTS_QoSOperationSection2_7();
	virtual enum TVerdict doTestStepL( void );
	virtual void Event(const CQoSEventBase& aEvent);

private:
	CTS_CEsockSendAndRecvData* sendAndRecv;
	TInt reason, i, i2;
};

/* QoS TestCase 2_8 Class Definition
 */
class CTS_QoSOperationSection2_8 : public CTS_QoSStep
{
public:
	CTS_QoSOperationSection2_8();
	~CTS_QoSOperationSection2_8();
	virtual enum TVerdict doTestStepL( void );

};

/* QoS TestCase 2_9 Class Definition
 */
class CTS_QoSOperationSection2_9 : public CTS_QoSStep
{
public:
	CTS_QoSOperationSection2_9();
	~CTS_QoSOperationSection2_9();
	virtual enum TVerdict doTestStepL( void );

};

/* QoS TestCase 2_10 Class Definition
 */

class CTS_QoSOperationSection2_10 : public CTS_QoSStep, public MQoSObserver
{
public:
	CTS_QoSOperationSection2_10();
	~CTS_QoSOperationSection2_10();
	virtual enum TVerdict doTestStepL( void );
	virtual void Event(const CQoSEventBase& aEvent);

private:
	TInt i, i2, reason;

};

/* QoS TestCase 2_11 Class Definition
 */

class CTS_QoSOperationSection2_11 : public CTS_QoSStep, public MQoSObserver
{
public:
	CTS_QoSOperationSection2_11();
	~CTS_QoSOperationSection2_11();
	virtual enum TVerdict doTestStepL( void );
	virtual void Event(const CQoSEventBase& aEvent);

private:
	TInt i, i2, reason;

};

/* QoS TestCase 2_11 Class Definition
 */

class CTS_QoSOperationSection2_12 : public CTS_QoSStep, public MQoSObserver
{
public:
	CTS_QoSOperationSection2_12();
	~CTS_QoSOperationSection2_12();
	virtual enum TVerdict doTestStepL( void );
	virtual void Event(const CQoSEventBase& aEvent);

private:
	TInt i, reason;

};


#endif //(__QOSOPERATIONSECTION_H__)
