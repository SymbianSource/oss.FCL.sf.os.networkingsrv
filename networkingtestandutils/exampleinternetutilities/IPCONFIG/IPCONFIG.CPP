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
// Started by MWT, August 1997
// 
//

#include <e32cons.h>
#include <in_sock.h>
#include <in_iface.h>
#include <nifman.h>


void ProgramL(CConsoleBase& aCons);
void PrintFeature(CConsoleBase& aCons, TUint& aMask, TUint aFeature, const TDesC& aDes);

GLDEF_C TInt E32Main()
    {
	__UHEAP_MARK;
	// Standard stuff
	CTrapCleanup* trap = CTrapCleanup::New();
	if(trap==NULL)
		return KErrNoMemory;

	CConsoleBase* cons=NULL;
	TRAPD(e, cons=Console::NewL(_L("IpConfig"),TSize(KConsFullScreen,KConsFullScreen));)
	if(e!=KErrNone)
		{
		delete trap;
		return e;
		}

	TRAPD(res, ProgramL(*cons);)
	if(res!=KErrNone)
		{
		cons->Printf(_L("Error %d\n"), res);
		cons->Printf(_L("Press any key to exit"));
		cons->Getch();
		}

	delete cons;
	delete trap;

	__UHEAP_MARKEND;
	return KErrNone;
    }

void PrintFeature(CConsoleBase& aCons, TUint& aMask, TUint aFeature, const TDesC& aDes)
	{
	if(aMask&aFeature)
		{
		aMask&=~aFeature;
		aCons.Printf(aDes);
		if(aMask)
			aCons.Printf(_L(","));
		}
	}

void ProgramL(CConsoleBase& aCons)
	{

//	User::LeaveIfError(Nifman::CheckIniConfig());

	TAutoClose<RSocketServ> ss;
	User::LeaveIfError(ss.iObj.Connect());
	ss.PushL();

	TAutoClose<RSocket> sock;
	User::LeaveIfError(sock.iObj.Open(ss.iObj, _L("udp")));
	sock.PushL();

	User::LeaveIfError(sock.iObj.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl));

	TProtocolDesc in;
	User::LeaveIfError(sock.iObj.Info(in));
	aCons.Printf(_L("EPOC32 IP Configuration TCPIP Version %d.%d.%d\n\n"), in.iVersion.iMajor, in.iVersion.iMinor, in.iVersion.iBuild);

	TPckgBuf<TSoInetInterfaceInfo> info, next;

	TInt res=sock.iObj.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info);
	if(res!=KErrNone)
		User::Leave(res);

	while(res==KErrNone)
		{

		res=sock.iObj.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, next);

		aCons.Printf(_L("   Interface name \"%S\"\n"), &info().iName);
		aCons.Printf(_L("    Interface tag \"%S\"\n"), &info().iTag);
		aCons.Printf(_L("            State "));
		switch (info().iState)
			{
		case EIfPending: 
			aCons.Printf(_L("pending\n"));
			break;
		case EIfUp: 
			aCons.Printf(_L("up\n"));
			break;
		case EIfBusy: 
			aCons.Printf(_L("busy\n")); 
			break;
		default:
			aCons.Printf(_L("down\n"));
			break;
			}

		aCons.Printf(_L("              Mtu %d\n"), info().iMtu);
        aCons.Printf(_L("     Speed Metric %d\n"), info().iSpeedMetric);
		aCons.Printf(_L("         Features "));

		TBool hardwareaddr = info().iFeatures&KIfHasHardwareAddr;

		PrintFeature(aCons, info().iFeatures, KIfIsLoopback, _L("loopback"));
		PrintFeature(aCons, info().iFeatures, KIfIsDialup, _L("dialup"));
		PrintFeature(aCons, info().iFeatures, KIfIsPointToPoint, _L("pointtopoint"));
		PrintFeature(aCons, info().iFeatures, KIfCanBroadcast, _L("canbroadcast"));
		PrintFeature(aCons, info().iFeatures, KIfCanMulticast, _L("canmulticast"));
		PrintFeature(aCons, info().iFeatures, KIfCanSetMTU, _L("cansetmtu"));
		PrintFeature(aCons, info().iFeatures, KIfHasHardwareAddr, _L("hardwareaddr"));
		PrintFeature(aCons, info().iFeatures, KIfCanSetHardwareAddr, _L("cansethardwareaddr"));
		aCons.Printf(_L("\n"));

		TName address;
		info().iAddress.Output(address);
		aCons.Printf(_L("          Address %S"), &address);
		if(info().iAddress.IsLinkLocal())
			aCons.Printf(_L("   Link local\n"));
		else if(info().iAddress.IsSiteLocal())
			aCons.Printf(_L("   Site local\n"));
		else
			aCons.Printf(_L("   Global\n"));

		info().iBrdAddr.Output(address);
		aCons.Printf(_L("Broadcast address %S\n"), &address);
		info().iDefGate.Output(address);
		aCons.Printf(_L("  Default gateway %S\n"), &address);
		info().iNameSer1.Output(address);
		aCons.Printf(_L("      Primary DNS %S\n"), &address);
		info().iNameSer2.Output(address);
		aCons.Printf(_L("    Secondary DNS %S\n"), &address);

		if(hardwareaddr)
			{
			aCons.Printf(_L(" Hardware address "));		
			TUint j;
			for(j=sizeof(SSockAddr) ; j<sizeof(SSockAddr)+6 ; ++j)
				{
				if(j<(TUint)info().iHwAddr.Length())
					aCons.Printf(_L("%02X"), info().iHwAddr[j]);
				else
					aCons.Printf(_L("??"));
				if(j<sizeof(SSockAddr)+5)
				   aCons.Printf(_L("-"));
				else
					aCons.Printf(_L("\n"));
				}
			}
		
		if(res==KErrNone)
			{
			info = next;
		    aCons.Printf(_L("\nPress any key for next\n"));
			}
		else
			aCons.Printf(_L("\nPress any key to exit\n"));
		
		aCons.Getch();
		}

	sock.Pop();
	ss.Pop();
	}
