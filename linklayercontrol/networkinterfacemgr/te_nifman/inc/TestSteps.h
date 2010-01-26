/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* OpenCloseBogus.h: interface for the COpenCloseBogus class.
* 
*
*/



/**
 @file teststeps.h
*/

#if !defined(_OPENCLOSEBOGUS_H_)
#define _OPENCLOSEBOGUS_H_

#include <testexecutestepbase.h>

class COpenCloseBogus : public CTestStep  
	{
public:
	virtual TVerdict doTestStepL();
	COpenCloseBogus();
	virtual ~COpenCloseBogus();
	};


class CSocketServerShutdown : public CTestStep  
	{
public:
	virtual TVerdict doTestStepL();
	CSocketServerShutdown();
	virtual ~CSocketServerShutdown();

	};


class CStartStopInterfaces : public CTestStep  
	{
public:
	virtual TVerdict  doTestStepL();
	CStartStopInterfaces();
	virtual ~CStartStopInterfaces();
	};

class CProgressNotification : public CTestStep  
	{
public:
	virtual TVerdict  doTestStepL();
	CProgressNotification();
	virtual ~CProgressNotification();
	};

class CConnectReconnect: public CTestStep  
	{
public:
	virtual TVerdict  doTestStepL();
	CConnectReconnect();
	virtual ~CConnectReconnect();
	};

class CTest8 : public CTestStep  
	{
public:
	virtual TVerdict  doTestStepL();
	CTest8();
	virtual ~CTest8();
	};

class CBinderLayerDown : public CTestStep  
	{
public:
	virtual TVerdict  doTestStepL();
	CBinderLayerDown();
	virtual ~CBinderLayerDown();
	};

class CTest5 : public CTestStep  
	{
public:
	virtual TVerdict  doTestStepL();
	CTest5();
	virtual ~CTest5();
	};

class COpenClosePSD : public CTestStep  
	{
public:
	virtual TVerdict doTestStepL();
	COpenClosePSD();
	virtual ~COpenClosePSD();
	};
#endif

