/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* This defines the TS_RConnectionStep class which is the base class for all 
* the RConnection multihoming test step classes
* 
*
*/



/**
 @file TS_DummyPPPStep.h
*/
 
#if (!defined __TS_DUMMYPPPSTEP_H__)
#define __TS_DUMMYPPPSTEP_H__

#include <e32std.h>
#include <cdbstore.h>
#include "es_sock.h"
#include "in_sock.h"
#include <es_enum.h>
#include "commdbconnpref.h"
#include <networking/teststep.h>
#include "TS_DummyPPPSuite.h"

// Test Step Defaults
#define PACKET_SIZE		512
#define	MAX_PACKET_SIZE 2000		
#define NUM_OF_PACKETS	20
#define UDP_TOLERANCE	5

/* A class used to pass around timeout values (obviously) */
class TTimeoutValues
	{
public:
	TUint32	iShortTimeout;
	TUint32	iMediumTimeout;
	TUint32	iLongTimeout;
	};

class TS_DummyPPPStep : public CTestStep
{
public:
	TS_DummyPPPStep();
	virtual ~TS_DummyPPPStep();
	void copyFileL (const TDesC& anOld,const TDesC& aNew);


private:
	enum TVerdict doTestStepPreambleL();
	TInt ReadIniFile(void);
	
protected:					
	
	// to all of these methods pass in a pointer to objects you have created on the stack...
	// lots of them will also create temporary automatics but don't worry about that

	// socket server specific stuff
	TInt OpenSocketServer(RSocketServ& ss);
	void CloseSocketServer(RSocketServ& ss);

	// connection specific stuff
	TInt OpenConnection(RConnection& conn, RSocketServ& ss);
	TInt OpenConnection(RConnection& conn, RSocketServ& ss, TName& name);
	TInt StartConnection(RConnection& conn);
	TInt StartConnectionSynchronous(RConnection& conn);
	void StartConnectionAsynchronous(RConnection& conn, TRequestStatus& status);
	TInt StartConnectionWithOverrides(RConnection& conn, TInt iap);
	TInt StartConnectionWithOverridesSynchronous(RConnection& conn, TInt iap);
	void StartConnectionWithOverridesAsynchronous(RConnection& conn, TCommDbConnPref& aPrefs, TInt iap, TRequestStatus& status);
	void CloseConnection(RConnection& conn);
	TInt StopConnection(RConnection& conn);
	TInt EnumerateConnections(RConnection& conn, TUint& num);
	TInt GetTimeoutValues(RConnection& conn, TTimeoutValues& timeouts);
	TInt AttachNormal(RConnection& conn, const TDesC8& info);
	TInt AttachMonitor(RConnection& conn, const TDesC8& info);
	void ProgressNotification(RConnection& conn, TRequestStatus& status, TNifProgressBuf& progress, TUint aSelectedProgress);
	void ProgressNotification(RConnection& conn, TRequestStatus& status, TNifProgressBuf& progress);
	TInt Progress(RConnection& conn, TNifProgress& progress);
	TInt LastProgressError(RConnection& conn, TNifProgress& progress);
	void CancelProgressNotification(RConnection& conn);

	// finding out about connections
	// this uses the RConnection::getConnectionInfo method
	TInt GetConnectionInfo(RConnection& conn, TUint index, TDes8& aConnectionInfo);
	// this just reads from comm db
	TInt GetInfo(RConnection& conn, TPckgBuf<TConnectionInfo>& info);

	// socket specific stuff
	TInt OpenUdpSocketL(RSocket& sock, RSocketServ& ss);
	TInt OpenUdpSocketExplicitL(RSocket& sock, RSocketServ& ss, RConnection& conn);
	TInt TestUdpDataPath(RSocket& sock, TSockAddr& dest);
	void DestroyUdpSocket(RSocket& sock);

	// general helper methods
	TInt NumberOfInterfacesL(RSocketServ& ss);
	TInt TimeUntilRequestComplete(TRequestStatus& status, TTimeIntervalSeconds& timeElapsed);
	TInt RequestInterfaceDownL(RConnection& conn, RSocketServ& ss);
	TInt WaitForAllInterfacesToCloseL(RSocketServ& ss);

protected:
	// this is the config data read from the .ini file in ReadIniFile().
	// IAP numbers used when creating connections with overrides
	TInt iDummyNifIap;
	TInt iDummyNifLongTimeoutIap;
	TInt iNtRasIap;
	TInt iBadNtRasIap;
	TInt iMissingNifIap;
	
	// timeout values (for dummy nif)
	TInt iShortTimeout;
	TInt iMediumTimeout;
	TInt iLongTimeout;
	// the address to send TCP packets to (and to connect to with TCP sockets)
	TInetAddr iTcpSendAddr;
	// the echo port
	TInt iEchoPortNum;
	
	// the address to send UDP packets to when using Dummy Nif (doesn't come from the config file)
	TInetAddr iDummyNifSendAddr;
	
};


// define files that have to be copied prior to starting the test
// where they have to be copied from and to depends on platform
// - ideally the file names would be read from an .ini file, but as there are only two...
_LIT(KAgentSrcPath, "z:\\testdata\\configs\\agentdialog.ini");
_LIT(KAgentDestPath, "c:\\private\\101f7989\\esock\\agentdialog.ini");
_LIT(KPppSrcPath,  "z:\\private\\101f7989\\esock\\ts_dummyppp_overall_ppp.ini");
_LIT(KPppDestPath, "c:\\private\\101f7989\\esock\\ppp.ini");
_LIT(KPppTempPath, "c:\\private\\101f7989\\esock\\ts_dummyppp_overall_ppp_bak");



///////////////////////////////////////////////
// Test Copy Test
NONSHARABLE_CLASS (CDummyPPPPreCopy) : public TS_DummyPPPStep
{
public:
	CDummyPPPPreCopy();
	virtual ~CDummyPPPPreCopy();

	virtual enum TVerdict doTestStepL( void );
};

///////////////////////////////////////////////
// Test Post Delete
NONSHARABLE_CLASS (CDummyPPPPostDelete) : public TS_DummyPPPStep
{
public:
	CDummyPPPPostDelete();
	virtual ~CDummyPPPPostDelete();

	virtual enum TVerdict doTestStepL( void );
	void deleteFileL (const TDesC& aFileName);
};


#endif /* __TS_DUMMYPPPSTEP_H__ */
