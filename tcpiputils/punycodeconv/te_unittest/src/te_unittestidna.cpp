// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @internalTechnology
*/

#include "te_unittestidnastepbase.h"
#include "te_unittestidna.h"

#include <es_sock.h> 
#include <in_sock.h> 
#include <commdbconnpref.h>
#include <networking/dnd_err.h>
#include <punycodeconverter.h>

CTestIdna01::CTestIdna01()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna01"));
	}

enum TVerdict CTestIdna01::doTestStepL()
	{

	INFO_PRINTF1(_L(" Testing GetByName(IDN) 	without IDN Enabled     "));
	INFO_PRINTF1(_L("****************************************************"));
	
	SetTestStepResult(EFail); // By default start the test case with failure.	
	
	RSocketServ pSocketServ;
	User::LeaveIfError(pSocketServ.Connect());
	RConnection myConnection;
	myConnection.Open(pSocketServ, KAfInet);

	TRequestStatus myStatus=KErrNone;
	myConnection.Start(myStatus);
	User::WaitForRequest(myStatus);

	_LIT(KTestName1,"räksmörgås.josefsson.org");
		
	TName myHostName = KTestName1();
	TNameEntry myResolvedName;
	RHostResolver hr;
	hr.Open(pSocketServ,KAfInet,KProtocolInetUdp);

	hr.GetByName(myHostName,myResolvedName,myStatus);
		
	User::WaitForRequest(myStatus);
	TInt err = myStatus.Int();
	
	if(err == KErrDndNameNotFound)
		{
		INFO_PRINTF2(_L(" GetByName(%S) 	without IDN Enabled   returned KErrDndNameNotFound "),&myHostName);
		SetTestStepResult(EPass);
		}
	
	hr.Close();
	myConnection.Close();
	pSocketServ.Close();

	return TestStepResult();

	}


CTestIdna02::CTestIdna02()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna02"));
	}

enum TVerdict CTestIdna02::doTestStepL()
	{

	INFO_PRINTF1(_L(" Testing GetByName(IDN) 	with IDN Enabled     "));
	INFO_PRINTF1(_L("****************************************************"));
	
	SetTestStepResult(EFail); // By default start the test case with failure.	
	
	RSocketServ pSocketServ;
	User::LeaveIfError(pSocketServ.Connect());
	RConnection myConnection;
	myConnection.Open(pSocketServ, KAfInet);

	TRequestStatus myStatus=KErrNone;
	myConnection.Start(myStatus);
	User::WaitForRequest(myStatus);

	_LIT(KTestName1,"räksmörgås.josefsson.org");
		
	TName myHostName = KTestName1();
	TNameEntry myResolvedName;
	RHostResolver hr;
	hr.Open(pSocketServ,KAfInet,KProtocolInetUdp);
	
	TBool enableIdn = ETrue;//enabling the option of IDN support
	TPckgC<TBool> pckgEnable(enableIdn);
	TInt setOptErr = hr.SetOpt(KSoDnsEnableIdn , KSolInetDns , pckgEnable);
	User::LeaveIfError(setOptErr);
	
	hr.GetByName(myHostName,myResolvedName,myStatus);
		
	User::WaitForRequest(myStatus);
	TInt err = myStatus.Int();
	
	if(err == KErrNone)
		{
		INFO_PRINTF2(_L(" GetByName(%S) with IDN Enabled   returned KErrNone "),&myHostName);
		SetTestStepResult(EPass);
		}
	
	hr.Close();
	myConnection.Close();
	pSocketServ.Close();

	return TestStepResult();

	}



CTestIdna03::CTestIdna03()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna03"));
	}

enum TVerdict CTestIdna03::doTestStepL()
	{

	INFO_PRINTF1(_L(" Testing GetByAddress(for an IDN) 	without IDN Enabled     "));
	INFO_PRINTF1(_L("*********************************************************"));
	
	SetTestStepResult(EFail); // By default start the test case with failure.	
	
	RSocketServ pSocketServ;
	User::LeaveIfError(pSocketServ.Connect());
	RConnection myConnection;
	myConnection.Open(pSocketServ, KAfInet);

	TRequestStatus myStatus=KErrNone;
	myConnection.Start(myStatus);
	User::WaitForRequest(myStatus);
	
	RHostResolver hr;
	hr.Open(pSocketServ,KAfInet,KProtocolInetUdp);
	
	TInetAddr inetAddr;
	inetAddr.Input(_L("83.241.177.38")); // this is the IPAddress of the domain räksmörgås.josefsson.org
										 // as of 06 Feb 2009. If this IP changes, this test case might fail
										 // but no harm on this.
		
	TNameEntry resultEntry;
	hr.GetByAddress(inetAddr,resultEntry,myStatus);
	User::WaitForRequest(myStatus);
	TInt err = myStatus.Int();
					
	if(err == KErrNone)
		{
		INFO_PRINTF2(_L(" GetByAddress 	with IDN disabled returned %d"),err);
		SetTestStepResult(EPass);
		}
	
	hr.Close();
	myConnection.Close();
	pSocketServ.Close();

	return TestStepResult();

	}



CTestIdna04::CTestIdna04()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna04"));
	}

enum TVerdict CTestIdna04::doTestStepL()
	{

	INFO_PRINTF1(_L(" Testing GetByAddress(for an IDN) 	with IDN Enabled     "));
	INFO_PRINTF1(_L("*********************************************************"));
	
	SetTestStepResult(EFail); // By default start the test case with failure.	
	
	RSocketServ pSocketServ;
	User::LeaveIfError(pSocketServ.Connect());
	RConnection myConnection;
	myConnection.Open(pSocketServ, KAfInet);

	TRequestStatus myStatus=KErrNone;
	myConnection.Start(myStatus);
	User::WaitForRequest(myStatus);
	
	RHostResolver hr;
	hr.Open(pSocketServ,KAfInet,KProtocolInetUdp);

	TBool enableIdn = ETrue;//enabling the option of IDN support
	TPckgC<TBool> pckgEnable(enableIdn);
	TInt setOptErr = hr.SetOpt(KSoDnsEnableIdn , KSolInetDns , pckgEnable);
	User::LeaveIfError(setOptErr);
	
	TInetAddr inetAddr;
	//inetAddr.Input(_L("83.241.177.38")); // this is the IPAddress of the domain räksmörgås.josefsson.org
										 // as of 06 Feb 2009. If this IP changes, this test case might fail
										 // but no harm on this.
		
	//TNameEntry resultEntry;
	
	inetAddr.Input(_L("64.233.169.103")); 
	
	TNameRecord asdf;
	TPckgBuf<TNameRecord> resultEntry(asdf);
	
	hr.GetByAddress(inetAddr,resultEntry,myStatus);
	User::WaitForRequest(myStatus);
	TInt err = myStatus.Int();
					
	if(err == KErrNone)
		{
		INFO_PRINTF2(_L(" GetByAddress 	with IDN disabled returned %d"),err);
		SetTestStepResult(EPass);
		}
	
	hr.Close();
	myConnection.Close();
	pSocketServ.Close();

	return TestStepResult();

	}


CTestIdna05::CTestIdna05()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna05"));
	}

enum TVerdict CTestIdna05::doTestStepL()
	{
	INFO_PRINTF1(_L(" Testing GetByName(IDN in UTF-16) 	without IDN Enabled     "));
	INFO_PRINTF1(_L("***********************************************************"));

	SetTestStepResult(EFail); // By default start the test case with failure.
	
	RSocketServ pSocketServ;
	User::LeaveIfError(pSocketServ.Connect());
	RConnection myConnection;
	myConnection.Open(pSocketServ, KAfInet);

	TRequestStatus myStatus=KErrNone;
	myConnection.Start(myStatus);
	User::WaitForRequest(myStatus);

	RHostResolver hr;
	hr.Open(pSocketServ,KAfInet,KProtocolInetUdp);

	THostName utf16HostName;
	TInt surrogateInt = 55301 ; //0xD805
	utf16HostName.Copy((const unsigned short*)&surrogateInt);
	surrogateInt = 57173; // 0xDF55
	utf16HostName.Append((const unsigned short*)&surrogateInt, sizeof(TInt));
		
	TNameEntry myResolvedName;
	hr.GetByName(utf16HostName,myResolvedName,myStatus);
	User::WaitForRequest(myStatus);
	TInt err = myStatus.Int();

	if(err == KErrDndBadName)
		{
		INFO_PRINTF1(_L(" GetByName (IDN in UTF16) 	without IDN enabled returned KErrDndBadName"));
		SetTestStepResult(EPass);
		}

	hr.Close();
	myConnection.Close();
	pSocketServ.Close();

	return TestStepResult();

	}

CTestIdna06::CTestIdna06()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna06"));
	}

enum TVerdict CTestIdna06::doTestStepL()
	{

	INFO_PRINTF1(_L(" Testing IDN to Punycode Conversion functionality   "));
	INFO_PRINTF1(_L("****************************************************"));
	
	SetTestStepResult(EFail); // By default start the test case with failure.
	
	TPunyCodeDndName punyCodeName;
	_LIT(KIDNName1,"räksmörgås.josefsson.org");
	TName myHostName = KIDNName1();
					
	TInt err = punyCodeName.IdnToPunycode(myHostName);
	if(err != KErrNone)
		{
		User::LeaveIfError(KErrNone); // just to suppress the LeaveScan warning
		INFO_PRINTF1(_L("Conversion of IDN to punycode is NOT successful "));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	
	_LIT8(KPunyCodeName,"xn--rksmrgs-5wao1o.josefsson.org");
	
	if( punyCodeName.Compare(KPunyCodeName()) == 0)
		{
		INFO_PRINTF1(_L(" Conversion of IDN to punycode is successful"));
		SetTestStepResult(EPass);
		}
	
	return TestStepResult();

	}

CTestIdna07::CTestIdna07()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna07"));
	}

enum TVerdict CTestIdna07::doTestStepL()
	{

	INFO_PRINTF1(_L(" Testing Punycode to IDN Conversion functionality   "));
	INFO_PRINTF1(_L("****************************************************"));
	
	SetTestStepResult(EFail); // By default start the test case with failure.	
	
	_LIT8(KPunyCodeName,"xn--rksmrgs-5wao1o.josefsson.org");
	TPunyCodeDndName punyCodeName;
	punyCodeName.Copy(KPunyCodeName());
	
	_LIT(KIDNName1,"räksmörgås.josefsson.org");
	
	TName myHostName ;
	TInt start =0;
	
	TInt err=punyCodeName.PunycodeToIdn(myHostName,start);
	if(err != KErrNone)
		{
		User::LeaveIfError(KErrNone); // just to suppress the LeaveScan warning
		INFO_PRINTF1(_L("Conversion of Punycode to IDN is NOT successful "));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	if( myHostName.Compare(KIDNName1()) == 0)
		{
		INFO_PRINTF1(_L(" Conversion of Punycode to IDN is successful"));
		SetTestStepResult(EPass);
		}
	
	return TestStepResult();
	}

CTestIdna08::CTestIdna08()
/** Each test step initialises it's own name
*/
	{
	// store the name of this test case
	// this is the name that is used by the script file
	SetTestStepName(_L("TestIdna08"));
	}

enum TVerdict CTestIdna08::doTestStepL()
	{

	INFO_PRINTF1(_L(" Testing the Loading of the new Library "));
	INFO_PRINTF1(_L("****************************************"));
	
	SetTestStepResult(EFail); // By default start the test case with failure.	
	
	RLibrary testLibrary;
	TInt err = testLibrary.Load(_L("punycodeconverter.dll"));
	
	if(err == KErrNone)
		{
		INFO_PRINTF1(_L(" Loading the punycodeconverter library is successful"));
		SetTestStepResult(EPass);
		}
	else 
		{	
		INFO_PRINTF1(_L(" Loading the punycodeconverter library is NOT successful"));
		User::LeaveIfError(KErrNone);  // just to suppress the LeaveScan warning
		}
	
	INFO_PRINTF1(_L("Negative Testing of SetOpt		 "));
	INFO_PRINTF1(_L("****************************************"));		

	RSocketServ pSocketServ;
	User::LeaveIfError(pSocketServ.Connect());
	RConnection myConnection;
	myConnection.Open(pSocketServ, KAfInet);

	TRequestStatus myStatus=KErrNone;
	myConnection.Start(myStatus);
	User::WaitForRequest(myStatus);

	RHostResolver hr;
	hr.Open(pSocketServ,KAfInet,KProtocolInetUdp);
	
	TBool enableIdn = ETrue;//enabling the option of IDN support
	TPckgC<TBool> pckgEnable(enableIdn);
	TInt setOptErr = hr.SetOpt(KSoInetConfigInterface , KSolInetDns , pckgEnable);
	if(setOptErr != KErrNone)
		{
		INFO_PRINTF1(_L(" Negative Testing of the Setopt successful "));
		SetTestStepResult(EPass);
		}
	
	setOptErr = hr.SetOpt(KSoDnsEnableIdn , KSolInetDns , pckgEnable);
	User::LeaveIfError(setOptErr);

	enableIdn = EFalse;
	TPckgC<TBool> pckgDisable(enableIdn);
	setOptErr = hr.SetOpt(KSoDnsEnableIdn , KSolInetDns , pckgDisable);
	User::LeaveIfError(setOptErr);
	
	hr.Close();
	myConnection.Close();
	pSocketServ.Close();
	return TestStepResult();

	}

