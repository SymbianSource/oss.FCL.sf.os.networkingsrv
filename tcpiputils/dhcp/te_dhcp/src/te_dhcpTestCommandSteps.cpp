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
// (test steps which set the mode of subsequent test case calls.
// this gives more power to the test scripts and reduces need
// for adding 1 test step in C++ per test case)
// 
//

/**
 @file te_dhcpTestCommandSteps.cpp
*/
#include "te_dhcpTestCommandSteps.h"
#include <test/testexecutelog.h>
#include <comms-infras/startprocess.h>
#include <f32file.h>

#include "Te_TestDaemonClient.h"

/**
* purely exists to prevent the base doTestStepPreambleL running
*  (which bombs out if parameters haven't yet been set... (by these test commands!)
*/
TVerdict CDhcpTestCommandBase::doTestStepPreambleL()
{
    return EPass;
}

TVerdict CDhcpTestCommandSetAddressMode::doTestStepL()
/**
* @return - sets address mode of subsequent test cases
*/
	{
	SetTestStepResult(EFail);

    TBool bRes;
    TBool bKeyFound = GetBoolFromConfig(ConfigSection(),_L("UsingIPv6"), bRes);
    if(!bKeyFound)
    {
        ERR_PRINTF1(_L("error accessing ini file!"));
        User::Leave(KErrNotFound);
    }

    Set_UsingIPv6L(bRes);

	INFO_PRINTF2(_L("UsingIPv6 set to %d"),UsingIPv6L());

	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestCommandSetIAPToUse::doTestStepL()
/**
* @return - sets IAP to use in subsequent test cases
*/
	{
	SetTestStepResult(EFail);

    TInt bRes;
    TBool bKeyFound = GetIntFromConfig(ConfigSection(),_L("IAPToUse"), bRes);
    if(!bKeyFound)
    {
        ERR_PRINTF1(_L("error accessing ini file!"));
        User::Leave(KErrNotFound);
    }

    SetIAPToUseL(bRes);

	INFO_PRINTF2(_L("IAP to use set to %d"),IAPToUseL());

	// Check to see if the IAP record should be made private.
    bRes = 0;
    GetIntFromConfig(ConfigSection(),_L("MakeIAPPrivate"), bRes);
	if( bRes )
		{
		INFO_PRINTF2(_L("Making IAP %d private"),IAPToUseL());
		
		MakeIAPPrivateL(bRes);
		}

	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestCommandSetDebugFlags::doTestStepL()
/**
* @return - sets debug flags to use in subsequent test cases
*/
	{
	SetTestStepResult(EFail);

    TInt flags;
    TBool bKeyFound = GetIntFromConfig(ConfigSection(),_L("DebugFlags"), flags);
    if(!bKeyFound)
    {
        ERR_PRINTF1(_L("error accessing DebugFlags field in ini file!"));
        User::Leave(KErrNotFound);
    }

	//Start DHCP server.
	RStartProcess processStart;
	TInt err = processStart.Start(KDHCPProcessName);
	TESTEL( err == KErrNone || err ==KErrAlreadyExists, err);

	//Connect to the server. We will use this connection to issue heap debug commands
	RTestDaemonClient debugSession;
	debugSession.CreateSession();
	CleanupClosePushL(debugSession);

	//Set debug flags
	INFO_PRINTF1(_L("Setting debug option for short lease time and timeout..."));
	TRequestStatus stat;
    TDhcpMemDbgParamBuf dhcpMemDbgParamBuf;
	dhcpMemDbgParamBuf() = flags;
	debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());

	//Close DHCP debug session
	CleanupStack::PopAndDestroy(&debugSession);

	INFO_PRINTF2(_L("DHCP server debug flags set to %d"),IAPToUseL());

	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestCommandSetIPv4LinkLocal::doTestStepL()
/**
* @return - sets the IPv4 link local IP option of subsequent test cases
*/
	{
	SetTestStepResult(EFail);

    TInt bRes;
    TBool bKeyFound = GetIntFromConfig(ConfigSection(),_L("IPv4LinkLocal"), bRes);
    if(!bKeyFound)
    	{
        ERR_PRINTF1(_L("error accessing ini file!"));
        User::Leave(KErrNotFound);
    	}
    	
		{
		// Open the .ini file.
		TAutoClose<RFs> fs;
		User::LeaveIfError( fs.iObj.Connect() );
		fs.PushL();
		TAutoClose<RFile> file;
		User::LeaveIfError( file.iObj.Open( fs.iObj, TCPIP_INI_PATH, EFileWrite | EFileShareExclusive ) );
		file.PushL();

		// Read the contents of the file.
		TInt size;
		User::LeaveIfError( file.iObj.Size( size ) );
		TAutoClose<RBuf8> buf;
		buf.iObj.CreateL( size );
		buf.PushL();
		User::LeaveIfError( file.iObj.Read( buf.iObj ) );

		// Edit the .ini file - the change will not take effect until the TCP/IP stack
		// is reloaded...
		TBuf8<128> tempStr;
		TBuf8<1> linkLocalValue;
		linkLocalValue.AppendNum( bRes );
		TInt secPos = buf.iObj.Find( TCPIP_INI_IP );
		if( secPos == KErrNotFound )
			{
			tempStr.Append( _L8( "\r\n" ) );
			tempStr.Append( TCPIP_INI_IP );
			tempStr.Append( TCPIP_INI_IPV4LINKLOCAL );
			tempStr.Append( linkLocalValue );
			tempStr.Append( _L8( "\r\n" ) );
			
			buf.iObj.ReAllocL( buf.iObj.Length() + tempStr.Length() );
			
			// There is no [IP] section so add it and the ipv4linklocal value.
			buf.iObj.Append( tempStr );
			}
		else
			{
			TInt valPos = buf.iObj.Find( TCPIP_INI_IPV4LINKLOCAL );
			
			// There is an [IP] section so check to see if the ipv4linklocal value is already present.
			if( valPos == KErrNotFound )
				{
				tempStr.Append( TCPIP_INI_IPV4LINKLOCAL );
				tempStr.Append( linkLocalValue );
				tempStr.Append( _L8( "\r\n" ) );
			
				buf.iObj.ReAllocL( buf.iObj.Length() + tempStr.Length() );

				// The value is not present so insert it in the [IP] section.
				buf.iObj.Insert( secPos + ( (const TDes8 &)TCPIP_INI_IP ).Length(), tempStr );
				}
			else
				{
				// The value is present so overwrite it with our own.
				buf.iObj[valPos + ( (const TDes8 &)TCPIP_INI_IPV4LINKLOCAL ).Length()] = linkLocalValue[0];
				}
			}
		
		// Write the new file contents.
		TInt pos = 0;
		User::LeaveIfError( file.iObj.Seek( ESeekStart, pos ) );
		User::LeaveIfError( file.iObj.Write( buf.iObj ) );
		
		buf.Pop();
		file.Pop();
		fs.Pop();
		}
	
	INFO_PRINTF2(_L("tcpip.ini ipv4linklocal option set to %d"),bRes);

	SetTestStepResult(EPass);
	return TestStepResult();
	}
