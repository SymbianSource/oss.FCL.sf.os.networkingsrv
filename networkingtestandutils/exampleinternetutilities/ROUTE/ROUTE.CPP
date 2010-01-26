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
// Started by AY, August 1997
// 
//

#include <e32cons.h>
#include <in_sock.h>
#include <nifman.h>


void ProgramL(CConsoleBase& aCons);

GLDEF_C TInt E32Main()
    {
	__UHEAP_MARK;
	// Standard stuff
	CTrapCleanup* trap = CTrapCleanup::New();
	if(trap==NULL)
		return KErrNoMemory;

	CConsoleBase* cons=NULL;
	TRAPD(e, cons=Console::NewL(_L("Route"),TSize(KConsFullScreen,KConsFullScreen));)
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

void ProgramL(CConsoleBase& aCons)
	{

//	User::LeaveIfError(Nifman::CheckIniConfig());

	TAutoClose<RSocketServ> ss;
	User::LeaveIfError(ss.iObj.Connect());
	ss.PushL();

	TAutoClose<RSocket> sock;
	User::LeaveIfError(sock.iObj.Open(ss.iObj, _L("udp")));
	sock.PushL();

	User::LeaveIfError(sock.iObj.SetOpt(KSoInetEnumRoutes, KSolInetRtCtrl));

	TProtocolDesc in;
	User::LeaveIfError(sock.iObj.Info(in));
	aCons.Printf(_L("EPOC32 Route TCPIP Version %d.%d.%d\n\n"), in.iVersion.iMajor, in.iVersion.iMinor, in.iVersion.iBuild);

	TPckgBuf<TSoInetRouteInfo> info, next;

	TInt res=sock.iObj.GetOpt(KSoInetNextRoute, KSolInetRtCtrl, info);
	if(res!=KErrNone)
		User::Leave(res);

	while(res==KErrNone)
		{

		res=sock.iObj.GetOpt(KSoInetNextRoute, KSolInetRtCtrl, next);

		TName address;
		info().iDstAddr.Output(address);
		aCons.Printf(_L("Network Address   %S\n"), &address);
		if(info().iDstAddr.Family() == KAfInet)
			{
			info().iNetMask.Output(address);
			aCons.Printf(_L("Netmask           %S\n"), &address);
			}
		info().iGateway.Output(address);
		aCons.Printf(_L("Gateway Address   %S\n"), &address);
		info().iIfAddr.Output(address);
		aCons.Printf(_L("Interface         %S\n"), &address);
        aCons.Printf(_L("Metric            %d\n\n"), info().iMetric);

		
		
		if(res==KErrNone)
			{
			info = next;
		    aCons.Printf(_L("\nPress any key for next\n\n"));
			}
		else
			aCons.Printf(_L("\nPress any key to exit\n"));
		
		aCons.Getch();
		}

	sock.Pop();
	ss.Pop();
	}
