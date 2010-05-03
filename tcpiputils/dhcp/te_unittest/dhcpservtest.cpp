// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 
#include <e32std.h>
#include <e32base.h>
#include <e32cons.h>
#include <es_sock.h>
#include <in_sock.h>
#include <commdbconnpref.h>
#include<c32comm.h>
#include "es_enum.h" 
#include <nifman.h>
#include <naptinterface.h>
#include "dhcpstatesdebug.h"

/**
 * MAC address length
 */
  const TInt KMacAddrLength = 6;
 
/** 
 * Variables used to configure the DHCP server.
 *
 * @internalTechnology
 */
TText8 KReset[KMacAddrLength]= {0x00,0x00,0x00,0x00,0x00,0x00};
TText8 KAny[KMacAddrLength]= {0xff,0xff,0xff,0xff,0xff,0xff};
//This value has to be changed by user and recompile this code. The value is the MAC address of 
//the machine for which DHCP server has to provide the IP.
TText8 KCurrent[KMacAddrLength]= {0x04,0x13,0x21,0xca,0x78,0xde};

/**
  * Console pointer to display the help and application response.
  *
  * @internalTechnology
  */
CConsoleBase *gConsole;

/** 
 * Function to load the DHCP server.
 *
 * @internalTechnology
 * @param socketServer - client side session of Socket server
 * @param aDhcpServConnection - RConnection to load and configure DHCP server.
 * @return - returns if there are any error, otherwise KErrNone.
 */
TInt LaunchDhcpServ(RSocketServ& socketServer, RConnection& aDhcpServConnection)
	{
	
	TInt err(KErrNone);
	//Open a subsession with the socket server for the RConnection
	err = aDhcpServConnection.Open(socketServer);
	if(err != KErrNone) return err;
	
	TCommDbConnPref prefs;
	prefs.SetIapId(12); //IAP 12 is used to launch the DHCP server.
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

	TRequestStatus status;
	aDhcpServConnection.Start(prefs, status); //Loads the DHCP server, if the provided configuration is perfect.
	User::WaitForRequest(status);
	gConsole->Printf(_L("aDhcpServConnection. Start Status %d\n"), status.Int());
	if(status.Int() != KErrNone) return status.Int();
	return KErrNone;
	}

/** 
 * Function to configure DHCP with the received aHwAddr.
 *
 * @internalTechnology
 * @param aDhcpServConnection - RConnection to load and configure DHCP server.
 * @param aHwAddr - Hardware address.
 * @return - returns if there are any error, otherwise KErrNone.
 */
TInt ProvisionDhcpServ(RConnection& aDhcpServConnection, TText8* aHwAddr)
	{
	TRequestStatus status;
	TBuf8<KMacAddrLength> address = TPtrC8(&aHwAddr[0],KMacAddrLength);
	aDhcpServConnection.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address);
	User::WaitForRequest(status);
	return status.Int();
	}

/** 
 * Function to stop DHCP server.
 *
 * @internalTechnology
 * @param aDhcpServConnection - RConnection associated with DHCP server.
 */
void StopInterfacesL(RConnection& aConnection)
	{
	User::LeaveIfError(aConnection.Stop());
	aConnection.Close();
	}

/** 
 * Function to stop DHCP server.
 *
 * @internalTechnology
 * @param aDhcpServConnection - RConnection associated with DHCP server.
 */
TInt TestAddressProvisioningL(CConsoleBase* gConsole)
	{
#ifdef _DEBUG	
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 118));
#endif
	
	RSocketServ socketServer;
    TInt err(KErrNone);
    err = socketServer.Connect();
	if (err != KErrNone) return err;
	
	RConnection myDhcpServConnection;
    err = LaunchDhcpServ(socketServer, myDhcpServConnection);
	if (err != KErrNone) return err;
	TBool done = EFalse;
	while (!done)
		{
		// Get user command from menu
		gConsole->ClearScreen();
		_LIT(KSelectOption, " Select an option \n\n");
		gConsole->Printf(KSelectOption);
		_LIT(KPossibleSelectionsText, " 1 KAny \n 2 KRest \n 3 Provision \n 4 NoProvisioning \n 5 Quit \n");
		gConsole->Printf(KPossibleSelectionsText);
		TKeyCode key = gConsole->Getch();
		switch (key)
			{
		case '1': 
			err = ProvisionDhcpServ(myDhcpServConnection, KAny);
			if (err != KErrNone) return err;
			gConsole->Printf(_L("DHCP server is in ANY mode.\n"));
			break;
		case '2':
			err = ProvisionDhcpServ(myDhcpServConnection, KReset);
			if (err != KErrNone) return err;
			gConsole->Printf(_L("DHCP server is in RESET mode.\n"));
			break;
		case '3': 
			err = ProvisionDhcpServ(myDhcpServConnection, KCurrent);
			if (err != KErrNone) return err;
			gConsole->Printf(_L("DHCP server is in SPECIFIC mode.\n"));
			break;
		case '4':
			err = ProvisionDhcpServ(myDhcpServConnection, KReset);
			if (err != KErrNone) return err;
			gConsole->Printf(_L("DHCP server is in RESET mode.\n"));
			break;
		case '5':
		done = ETrue;
			break;
		default:
			break;
			}
		gConsole->Printf(_L("Press any key to next.\n"));
		gConsole->Getch();
		} // while
	
    StopInterfacesL(myDhcpServConnection);
    gConsole->Printf(_L("Basic DHCP testing finished .....\n"));	
	return KErrNone;
	}

/** 
 * Function to triggers the test cases.
 *
 * @internalTechnology
 * @return - Returns error if any, otherwise KErrNone.
 */
TInt TestConsoleL()
	{
	gConsole = Console::NewL(_L("PREQ1872 DHCP Unit Test Console"), TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(gConsole);
	TInt err = TestAddressProvisioningL(gConsole);
	CleanupStack::PopAndDestroy(gConsole);
	return err;
	}

/** 
 * Main function
 *
 * @internalTechnology
 * @return - Returns 0.
 */
EXPORT_C GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup * trap = CTrapCleanup::New();
	TRAPD(err,TestConsoleL());
	delete trap;
	__UHEAP_MARKEND;
	return err;
	}
