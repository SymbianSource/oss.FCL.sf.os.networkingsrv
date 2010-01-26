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

/**
 @file tipsec_inter.cpp IPSec interactive tests
*/

#include "ipsecapi.h"
#include <e32test.h>


_LIT(KPolicyName, "PolicyName");
_LIT(KPolicyFile, "PolicyFile");



_LIT(KDefaultPolicy, "hurr-pppdialin-1_0");

RTest t(_L("tipsec inter"));

CIPSecAPI* api;



void IpsecActivate(void)
/**
 * activate IPSec with given policy
 */
	{
	TPolicyID policyID;
	policyID.iPolicyID.Copy(KDefaultPolicy);
	const TInt ret = api->ActivateIPSec(policyID);
	t.Printf(_L("activate policy [ret=%d]\n"), ret);

	const TBool active = api->IsIPSecActive();
	t.Printf(_L("is active? [active=%d]\n"), active);
	}


void IpsecDeactivate(void)
/**
 * activate IPSec with given policy
 */
	{
	TPolicyID policyID;
	policyID.iPolicyID.Copy(KDefaultPolicy);
	const TInt ret = api->DeactivateIpsec();
	t.Printf(_L("deactivate policy [ret=%d]\n"), ret);

	const TBool active = api->IsIPSecActive();
	t.Printf(_L("is active? [active=%d]\n"), active);
	}


void IpsecStatus(void)
/**
 * get IPSec status
 */
	{
	_LIT(KActive, "active");
	_LIT(KInactive, "not active");
		
	t.Printf(_L("\nStatus:\n"));
	const TBool active = api->IsIPSecActive();
	t.Printf(_L("  IPSec is %S\n"), active ? &KActive : &KInactive );
	const TInt count = api->PolicyCount();
	t.Printf(_L("  PolicyCount = %d\n"), count );
	}


void IpsecImport(void)
/**
 * import IPSec policy files
 */
	{
	_LIT(KInstallDir, "c:\\system\\data\\security\\install");
	const TInt ret = api->ImportPolicy(KInstallDir);
	t.Printf(_L("Installing policy files from %S [ret=%d]\n"), &KInstallDir, ret );
	}


void MainL(void)
	{
	api = CIPSecAPI::NewL();
	CleanupStack::PushL(api);

	FOREVER
		{
		t.Printf(_L("menu:\n"));
		t.Printf(_L("  0 deactivate ipsec\n"));
		t.Printf(_L("  1 activate ipsec\n"));
		t.Printf(_L("  i import policy files\n"));
		t.Printf(_L("  s show ipsec status\n"));
		t.Printf(_L("  q quit (also try Ctrl-C)\n"));

		TInt key = t.Getch();
		t.Printf(_L("%c"), key);

		switch(key)
			{
			case '0':
				IpsecDeactivate();
				break;
			case '1':
				IpsecActivate();
				break;
			case 'i':
			case 'I':
				IpsecImport();
				break;
			case 's':
			case 'S':
				IpsecStatus();
				break;
			case 'q':
			case 'Q':
			case 0x03: // Ctrl-C
				User::Leave(KErrNone);
			default:
				break;
			}
		}
	}


#ifdef __DLL__
EXPORT_C TInt WinsMain(void)
#else
TInt E32Main(void)
#endif
/**
 * main function
 */
    {
	CTrapCleanup* cleanup = CTrapCleanup::New();

	__UHEAP_MARK;

	t.SetLogged(EFalse);    // to avoid garbage on Com port
	t.Title();
	t.Start(_L("Starting tests..."));
	TRAPD(err, MainL());
	if (err!=KErrNone)
		t.Printf(_L("ERROR: Leave %d\n"),err);

    t.End();
    t.Close();

    __UHEAP_MARKEND;

    delete cleanup;
    return KErrNone;
    }

#ifndef EKA2
#ifdef __DLL__
TInt E32Dll(enum TDllReason)
	{
	return KErrNone;
	}
#endif // __DLL__
#endif // EKA2