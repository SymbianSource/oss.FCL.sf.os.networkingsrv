// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ws_eng.cpp - http server engine
//

#include "ws_eng.h"
//#include "ws_sec.h"

#if EPOC_SDK >= 0x06000000
#include "uniload.h"

class TLineBuffer
	{
public:
	TLineBuffer(const TDesC &aBuffer);
	TPtrC ReadLine();
	inline TBool EOB() const { return iRemainder == 0; }
private:
	TInt iRemainder;
	TPtrC iTail;
	};

TLineBuffer::TLineBuffer(const TDesC &aBuffer) : iRemainder(aBuffer.Length()), iTail(aBuffer)
	{
	}

TPtrC TLineBuffer::ReadLine()
	{
	// Class nvariant: iRemainder == iTail.Length()
	ASSERT(iRemainder == iTail.Length());

	TInt end = iTail.Locate('\n');
	if (end < 0)
		{
		// Buffer ends without ending '\n', treat as line
		end = iTail.Length();
		iRemainder = 0;
		}
	else
		{
		// 0 <= end < iTail.Length() => iTail.Length() > iRemainder >= 0
		// (remainder is always decreased at least by 1, skipping '\n',
		// and will never become negative, because end < remainder)
		iRemainder -= end + 1;		
		// Ignore CR before LF, if present
		if (end > 0 && iTail[end-1] == '\r')
			--end;
		}
	const TPtrC line(iTail.Left(end));
	iTail.Set(iTail.Right(iRemainder));
	return line;
	}
#endif

//*************************************** CWebServerEng implementation**********************************************

// Construtors

CWebServerEng::CWebServerEng() : CActive(CActive::EPriorityStandard), iConList(CWebServerCon::iOffset)
// Check the priority. (Initially seems to be ok!)
{
}

CWebServerEng* CWebServerEng::NewL()
{

	CWebServerEng* self = new (ELeave) CWebServerEng;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

void CWebServerEng::SetObserver(MWebServerObserver* aObserver)
{
	iObserver = aObserver;

}

void CWebServerEng::ConstructL()
{
	iWebServerState = ENotStarted;
	iServerEnv = new (ELeave) CWebServerEnv;
	iServerEnv->ConstructL();
	// We add the WebServer (An Active Object) to the scheduler.
	CActiveScheduler::Add(this);
	
}

CWebServerEng::~CWebServerEng()
{

	// Cancel actual request, if any.
	Cancel(); // It is not necessary if Reset() has been previously called.
	delete iServerEnv;
}

/*
void CWebServerEng::ReadConfigFileL()
{
	// Read Config File.
	TRAP(err,iServerEnv->ReadConfigFileL(CFG_FILENAME));
	if ((err == KErrNotFound) || (err == KErrPathNotFound))
	{
		iObserver->ShowWarningL(R_CONFIG_FILE_MISSING_WAR);
		iServerEnv->CheckAndSetDefaultL();
	}
	else
	{
		User::LeaveIfError(err);
	}
}

void CWebServerEng::ReadHttpTypesFileL()
{
	// Read Http types files
	TRAP(err,iHttpTypes.ReadHttpTypesFileL(HTTP_TYPES_FILENAME));
	if ((err == KErrNotFound) || (err == KErrPathNotFound))
	{
		iObserver->ShowWarningL(R_TYPES_FILE_MISSING_WAR);
		iHttpTypes.SetDefaultHttpTypesL();
	}
	else
	{
		User::LeaveIfError(err);
	}
}

*/
void CWebServerEng::StartWebServerL()
// This function starts the WebServer.
{
	
	TInt err;
/*
// *** TEST of the Security Database.
	CWebServerSecDB SecDb; // ----> Must be menber of the object.

//TRAP(err,SecDb.OpenSecDBL(_L("WebServer")));
//if (err == KErrNotFound)
//{
	SecDb.CreateSecDB(_L("webserver.db"));
	SecDb.OpenSecDBL(_L("webserver.db"));
	SecDb.InsertUserL(_L("Litos"),_L("Groo"),_L("GrooWorld"));
	SecDb.InsertUserL(_L("Rufferto"),_L("Groo"),_L("RuffertoWorld"));
	TInt ruffertotwice = SecDb.InsertUserL(_L("Rufferto"),_L("Groonan"),_L("RuffertoWorld"));
	SecDb.InsertRealmL(_L("GrooWorld"),_L("Basic"));
	SecDb.InsertResourceDirL(_L("\\ws\\laetitia\\"),_L("GrooWorld"),SEC_GET);
	SecDb.CloseSecDB();
	SecDb.OpenSecDBL(_L("webserver.db"));
	TBool boolean1 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EGet);
	TBool boolean2 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EPut);
	TBool boolean3 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EDelete);
	TBool boolean4 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EOptions);
	TBool boolean5 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EPost);
	TBool boolean6 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EGet);
	TBool boolean7 = SecDb.GrantAccessL(_L("Litos5"),_L("Groo"),_L("\\ws\\laetitia\\"),EPost);
	TBool boolean8 = SecDb.GrantAccessL(_L("Litos"),_L("Groonan"),_L("\\ws\\laetitia\\"),EPost);
	TBool boolean9 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\lala\\laetitia\\"),EPost);
	TBool boolean10 = SecDb.GrantAccessL(_L("Rufferto"),_L("Groo"),_L("\\ws\\laetitia\\"),EGet);
	TInt deluser = SecDb.DeleteUserL(_L("Litos"),_L("RuffertoWorld"));
	SecDb.DeleteUserL(_L("Litos"),_L("GrooWorld"));
	TInt dellit = SecDb.DeleteUserL(_L("Litos"),_L("GrooWorld"));
	TBool boolean11 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EGet);
	SecDb.InsertUserL(_L("Litos"),_L("Groo"),_L("GrooWorld"));
	SecDb.DeleteRealm(_L("GrooWorld"));
	TBool boolean12 = SecDb.GrantAccessL(_L("Litos"),_L("Groo"),_L("\\ws\\laetitia\\"),EGet);
	SecDb.CloseSecDB();
	
//}


*/

//***********************************

#ifdef IPV4_ORIG

	iStatusText = _L("************* IPV4 STACK ****************\n\n");
#else

	iStatusText=_L("*************** DUAL STACK IPV4/IPV6 ****************\n\n");
#endif
	NotifyStatus();

	// Setting up the dial connection.
	iStatusText=_L("Setting up net connection.\n");
	NotifyStatus();

	// Make a connection to the Socket Server.
	iStatusText=_L("Connecting to the Socket Server.\n");
	NotifyStatus();
	err = iSocketServer.Connect(201);
	User::LeaveIfError(err);

#if EPOC_SDK >= 0x07010000
    User::LeaveIfError(iRConnection.Open(iSocketServer));
    err = iRConnection.Start();

    if (err != KErrAlreadyExists)
        User::LeaveIfError(err);

#elif EPOC_SDK >= 0x06010000

    User::LeaveIfError(iGenericAgent.Open());
    err = iGenericAgent.StartOutgoing();
    err = iGenericAgent.DisableTimers();

    if (err != KErrAlreadyExists)
        User::LeaveIfError(err);
#else
    User::LeaveIfError(iNetdial.Open());
    
	err = iNetdial.DisableTimers();
	if (err != KErrAlreadyExists)
		User::LeaveIfError(err);

	err = iNetdial.StartDialOut();

	if (err != KErrAlreadyExists)
		User::LeaveIfError(err);
#endif // EPOC_SDK >= 0x06010000

	// Open a socket to listen on.
	iStatusText=_L("Opening the Socket.\n");
	NotifyStatus();

	err = iSocket.Open(iSocketServer, KAfInet, KSockStream, KProtocolInetTcp);
	User::LeaveIfError(err);

	// Reuse the port
	iSocket.SetOpt(KSoReuseAddr, KProtocolInetIp, 1);

	// Bind the socket to the http port.
	iStatusText=_L("Binding the Socket.\n");
	NotifyStatus();

	TInetAddr httpPort(KInetAddrAny, iServerEnv->iPort);
	httpPort.SetFamily(0); // Listen both IPV4 and IPV6. (Dual-Stack).
	err = iSocket.Bind(httpPort);
	User::LeaveIfError(err);

	// Listen for incoming connections.
	iStatusText=_L("Listening through the Socket.\n");
	NotifyStatus();
	err = iSocket.Listen(100);
	User::LeaveIfError(err);

	// The Socket for the "real" connection.(between the client and the WebServer)
	iStatusText=_L("Waiting for a Connection.\n");
	NotifyStatus();
	
	CreateSocketAndConnectionL();
	
	iWebServerState = EWaitingForConnections;
	iSocket.Accept(*iConnection,iStatus);
	iCurrentNumberConnections = 0;
	iTotalNumberConnections = 0;
	SetActive();
}


void CWebServerEng::DestroyConnectionL(CWebServerCon* aWebServerCon)
{
	iConList.Remove(*aWebServerCon);
	delete aWebServerCon;
	iCurrentNumberConnections--;	
	iObserver->UpdateNumberConL();

	if (iWebServerState == EErrorRecovery)
	{
		TRAPD(err, CreateSocketAndConnectionL());
				if (err >= KErrNone)
				{
					iWebServerState = EWaitingForConnections;
					iSocket.Accept(*iConnection,iStatus);
					SetActive();
				}
				else
				{
					iCon = 0;
					iConnection = 0;
				}
	}
}

void CWebServerEng::CreateSocketAndConnectionL()
{
	iConnection = new (ELeave) RSocket;
	CleanupStack::PushL(iConnection);
	User::LeaveIfError(iConnection->Open(iSocketServer));
	iCon = CWebServerCon::NewL(this);
	CleanupStack::Pop();
}

void CWebServerEng::RunL()
{
	if (iStatus==KErrNone)
	{
			switch(iWebServerState)
			{
			case EWaitingForConnections:
				ASSERT(iCon != NULL);

				iCon->SetObserver(iObserver);
				if (iCurrentNumberConnections > MAX_NUMBER_CONNECTIONS) // This must be dynamic.(E.g: Only 10% memory free then WebserverBusy)
				{
					iCon->WebServerBusyL();
				}
				else
				{
					iCon->StartConnection();
				}
				iConList.AddLast(*iCon);
				iCurrentNumberConnections++;
				iTotalNumberConnections++;
				iObserver->UpdateNumberConL();
	

				TRAPD(err, CreateSocketAndConnectionL());
				if (err >= KErrNone)
				{
					ASSERT(iConnection != NULL);
					iWebServerState = EWaitingForConnections;		
					iSocket.Accept(*iConnection,iStatus);
					SetActive();
				}
				else
				{
					iCon = 0;
					delete iConnection;
					iConnection = 0;
					iWebServerState = EErrorRecovery;
				}
				break;
			case EShuttingDownServer:
				delete this;
				break;
			case EErrorRecovery:
				User::Panic(_L("BAD STATE IN THE STATE MACHINE"),2);
				break;
			default: //Something is going VERY, VERY wrong
				User::Panic(_L("UnKnown State in the WebServer."),2);
				break;
			}
		
	}
	else if (iStatus == KErrWouldBlock)
	{
		ASSERT(iConnection != NULL);
		iStatusText.Format(_L("\r\nRECOVERING ACCEPT !!!!!\r\n"),iCurrentNumberConnections);
		NotifyStatus();

		iSocket.Accept(*iConnection,iStatus);
		SetActive();	
	}
	else if(iWebServerState == EShuttingDownServer)
	{
		delete this;
	}
	else
	{		
		if (iWebServerState == EWaitingForConnections)
			iSocket.CancelAccept();
		
		iWebServerState = EShuttingDownServer;
		iSocket.Shutdown(RSocket::ENormal,iStatus);
		SetActive();
	}
		
}


void CWebServerEng::DoCancel()
{
	// Now we shutdown all connections.
	iStatusText.Format(_L("Number of lost connections: %d\n"),iCurrentNumberConnections);
	NotifyStatus();
	
	iStatusText.Format(_L("Number of connections: %d\n"),iTotalNumberConnections);
	NotifyStatus();

	// Deleting all the connections.
	TSglQueIter<CWebServerCon> ConIter(iConList);
	CWebServerCon* Connection;
	ConIter.SetToFirst();
	while ((Connection = ConIter++) != NULL)
	{
		Connection->Cancel();
		delete Connection;
	}

	// Actualize the lits of connections.
	iConList.Reset(); // This line was missing before, I think this was the reason of Marku's Panic.
	

	// Cancel the Accpet in the listen socket.
	iSocket.CancelAccept();


	// Delete the next conenction.
	// Note: As a collateral effect the iConnection is deleted when iCon is deleted !!
	// I don't like too much this (:() but I leave it because it works.
	delete iCon;
	iCon = 0;


	iSocket.Close();

#if EPOC_SDK >= 0x07010000
	iRConnection.Close();
#elif EPOC_SDK >= 0x06010000
	iGenericAgent.Close();
#else
	iNetdial.Stop();
	iNetdial.Close();
#endif
	iSocketServer.Close();

	iWebServerState = ENotStarted;
	iStatusText=_L("Closing WebServer.\n");
	NotifyStatus();
}

void CWebServerEng::Reset()
{
	// This function is need due to Error recovery.
	// If we are in the EErrorRecovery state, the webserver engine is not active, but it can still have connections.
	// Therefore, they must be destroyed. A simple Cancel() doesn't work because the DoCancel() function is not called
	// when the Active Object is not active.

	if (iWebServerState == EErrorRecovery)
	{
		DoCancel();
	}
	else
	{
		Cancel();
	}
}

void CWebServerEng::NotifyStatus()
{
	iObserver->Write(iStatusText);
}

//*******************************************************************************************************************

//********************************************* CWebServerEnv Implementation ****************************************
CWebServerEnv::CWebServerEnv()
{
}

void CWebServerEnv::ConstructL()
{
	iBackupPath = DEFAULT_BACKUP_PATH.AllocL();
	iCgiPath = DEFAULT_CGI_PATH.AllocL();	
	iDefaultResource = DEFAULT_DEFAULT_RESOURCE.AllocL();
	iDeleteMethod = FALSE;
	iPutMethod = FALSE;
	iMiscData = TRUE;
	iHttpResp = TRUE;
	iHttpReq = TRUE;
	iErrorPath = DEFAULT_ERROR_PATH.AllocL();
	iPort = DEFAULT_PORT;
	iServerPath = DEFAULT_SERVER_PATH.AllocL();
	iHistorySize = DEFAULT_HISTORY_SIZE;
}

CWebServerEnv::~CWebServerEnv()
{
	delete iBackupPath;
	delete iErrorPath;
	delete iCgiPath;
	delete iServerPath;
	delete iDefaultResource;
}

void CWebServerEnv::ParseConfigLineL(const RFs& aFs, const TDesC& aLine)
// Function which parse the config file and set the configuration
{

	TLex lex(aLine);
	TPtrC value;
	TParse parse;

	lex.SkipSpaceAndMark();

	// SkipCharacters until we find a '='
	while ((!lex.Eos()) && (lex.Peek() != '='))
		lex.Inc();
	

	TPtrC variable = lex.MarkedToken();
	
	
	if (!lex.Eos())
	{
		//Skip the '=' character.
		lex.Inc();
		lex.SkipSpaceAndMark();
		lex.SkipCharacters();
		value.Set(lex.MarkedToken());
	}
	else
		value.Set(_L(""));

	if (variable.Compare(_L("SERVER_PATH")) == 0)
	{
		User::LeaveIfError(parse.Set(value,NULL,NULL));
		User::LeaveIfError(aFs.Parse(value,parse));
		delete iServerPath;
		iServerPath = value.AllocL();
	
	}
	else if (variable.Compare(_L("CGI_PATH")) == 0)
	{
		User::LeaveIfError(parse.Set(value,NULL,NULL));	
		User::LeaveIfError(aFs.Parse(value,parse));
		delete iCgiPath;
		iCgiPath = value.AllocL();
	}
	else if (variable.Compare(_L("ERROR_PATH")) == 0)
	{
		User::LeaveIfError(parse.Set(value,NULL,NULL));	
		User::LeaveIfError(aFs.Parse(value,parse));
		delete iErrorPath;
		iErrorPath = value.AllocL();
	}
	else if (variable.Compare(_L("BACKUP_PATH")) == 0)
	{
		User::LeaveIfError(parse.Set(value,NULL,NULL));	
		User::LeaveIfError(aFs.Parse(value,parse));
		delete iBackupPath;
		iBackupPath = value.AllocL();
	}
	else if (variable.Compare(_L("DEFAULT_RESOURCE")) == 0)
	{
		delete iDefaultResource;
		iDefaultResource = value.AllocL();
	}
	else if (variable.Compare(_L("PORT")) == 0)
	{
		lex.UnGetToMark();
		User::LeaveIfError(lex.Val(iPort));
	}
	else if (variable.Compare(_L("DELETE_METHOD")) == 0)
	{
		if (value.CompareF(_L("Yes")) == 0)
			iDeleteMethod = TRUE;
		else
			iDeleteMethod = FALSE;
	}
	else if (variable.Compare(_L("PUT_METHOD")) == 0)
	{
		if (value.CompareF(_L("Yes")) == 0)
			iPutMethod = TRUE;
		else
			iPutMethod = FALSE;
	}
	else if (variable.Compare(_L("HTTP_REQ")) == 0)
	{
		if (value.CompareF(_L("Yes")) == 0)
			iHttpReq = TRUE;
		else
			iHttpReq= FALSE;
	}
	else if (variable.Compare(_L("HTTP_RESP")) == 0)
	{
		if (value.CompareF(_L("Yes")) == 0)
			iHttpResp = TRUE;
		else
			iHttpResp= FALSE;
	}
	else if (variable.Compare(_L("MISC_DATA")) == 0)
	{
		if (value.CompareF(_L("Yes")) == 0)
			iMiscData = TRUE;
		else
			iMiscData = FALSE;
	}
	
	else
	{
		User::Leave(KErrCorrupt);
	}

}

void CWebServerEnv::SetDefaultResourceL(const TDesC& aDefaultResource)
{
	delete iDefaultResource;
	iDefaultResource = aDefaultResource.AllocL();
}

void CWebServerEnv::SetServerPathL(const TDesC& aServerPath)
{
	delete iServerPath;
	iServerPath = aServerPath.AllocL();
}

void CWebServerEnv::SetCgiPathL(const TDesC& aCgiPath)
{
	delete iCgiPath;
	iCgiPath = aCgiPath.AllocL();
}
void CWebServerEnv::SetErrorPathL(const TDesC& aErrorPath)
{
	delete iErrorPath;
	iErrorPath = aErrorPath.AllocL();
}
void CWebServerEnv::SetBackupPathL(const TDesC& aBackupPath)
{
	delete iBackupPath;
	iBackupPath = aBackupPath.AllocL();
}


void CWebServerEnv::CheckAndSetDefaultL()
// This function initializes the environment variables which has not been initialized.
{
	if (iPort <= 0) iPort = DEFAULT_PORT;
	if (iServerPath == 0) iServerPath = DEFAULT_SERVER_PATH.AllocL();
	if (iCgiPath == 0)	  iCgiPath = DEFAULT_CGI_PATH.AllocL();	
	if (iDefaultResource == 0) iDefaultResource = DEFAULT_DEFAULT_RESOURCE.AllocL();
	if (iErrorPath == 0) iErrorPath = DEFAULT_ERROR_PATH.AllocL();
	if (iBackupPath == 0) iBackupPath = DEFAULT_BACKUP_PATH.AllocL();
	
}

void CWebServerEnv::ReadConfigFileL(const TDesC& aConfigFileName)
//Function which reads and parse the confing file
{

	RFs fs;
#if EPOC_SDK >= 0x06000000
	User::LeaveIfError(fs.Connect());
	HBufC *config = NULL;
	// Need to TRAP leaves, because must close the fs (right).
	// Thus, no point in using the CleanupStack for config either.
	//
	TRAPD(err, config = UnicodeLoad::LoadL(fs, aConfigFileName));
	if (err == KErrNone && config != NULL)
		{
		TLineBuffer buffer(config->Des());
		while (!buffer.EOB())
			{
			TPtrC line = buffer.ReadLine();
			if (line.Length() > 0)
				{
				TRAP(err, ParseConfigLineL(fs, line));
				if (err != KErrNone)
					break;
				}
			}
		}
	delete config;
	fs.Close();
	User::LeaveIfError(err);
#else
	RFile cfg;
	TFileText cfgtxt;
	TBuf<256> buffer;

	User::LeaveIfError(fs.Connect());
	User::LeaveIfError(cfg.Open(fs,aConfigFileName,EFileStreamText));
	
	cfgtxt.Set(cfg);
	User::LeaveIfError(cfgtxt.Seek(ESeekStart));
	cfgtxt.Read(buffer);
	while (buffer.Length() > 0)
	{
		ParseConfigLineL(fs,buffer);
		cfgtxt.Read(buffer);
	}
	
	cfg.Close();

	fs.Close();
#endif
	CheckAndSetDefaultL();

}

void CWebServerEnv::SaveConfigFileL(const TDesC& aConfigFileName)
{
	RFs fs;
	RFile cfg;

	TBuf8<256> buffer;

	User::LeaveIfError(fs.Connect());
	User::LeaveIfError(cfg.Replace(fs,aConfigFileName,EFileStreamText|EFileWrite));

	// Write config file in "narrow" ASCII, regardless of the build variant!
	// (config loader will automaticly handle conversions, if any required)

#ifdef _UNICODE
	HBufC8* aux = HBufC8::NewL(256);
	aux->Des().Copy(iServerPath->Des());

	buffer.Format(_L8("SERVER_PATH=%S\n"), aux);
	cfg.Write(buffer);

	aux->Des().Copy(iCgiPath->Des());
	buffer.Format(_L8("CGI_PATH=%S\n"), aux);
	cfg.Write(buffer);

	aux->Des().Copy(iErrorPath->Des());	
	buffer.Format(_L8("ERROR_PATH=%S\n"), aux);
	cfg.Write(buffer);

	aux->Des().Copy(iBackupPath->Des());
	buffer.Format(_L8("BACKUP_PATH=%S\n"), aux);
	cfg.Write(buffer);

	aux->Des().Copy(iDefaultResource->Des());
	buffer.Format(_L8("DEFAULT_RESOURCE=%S\n"), aux);
	cfg.Write(buffer);

	delete aux;
	
#else
	buffer.Format(_L8("SERVER_PATH=%S\n"), iServerPath);
	cfg.Write(buffer);
	buffer.Format(_L8("CGI_PATH=%S\n"), iCgiPath);
	cfg.Write(buffer);
	buffer.Format(_L8("ERROR_PATH=%S\n"), iErrorPath);
	cfg.Write(buffer);
	buffer.Format(_L8("BACKUP_PATH=%S\n"), iBackupPath);
	cfg.Write(buffer);
	buffer.Format(_L8("DEFAULT_RESOURCE=%S\n"), iDefaultResource);
	cfg.Write(buffer);
#endif
	buffer.Format(_L8("PORT=%d\n"),iPort);
	cfg.Write(buffer);

	const TBuf8<3> yes(_L8("Yes"));
	const TBuf8<3> no(_L8("No"));

	buffer.Format(_L8("DELETE_METHOD=%S\n"), iDeleteMethod ? &yes : &no);
	cfg.Write(buffer);
	buffer.Format(_L8("PUT_METHOD=%S\n"), iPutMethod ? &yes : &no);
	cfg.Write(buffer);
	buffer.Format(_L8("HTTP_REQ=%S\n"), iHttpReq ? &yes : &no);
	cfg.Write(buffer);
	buffer.Format(_L8("HTTP_RESP=%S\n"), iHttpResp ? &yes : &no);
	cfg.Write(buffer);
	buffer.Format(_L8("MISC_DATA=%S\n"), iMiscData ? &yes : &no);
	cfg.Write(buffer);

	cfg.Close();
	
	fs.Close();
	

}
//*******************************************************************************************************************

//********************************** CHttpTypes Implementation *******************************************************

// Constructor
CHttpTypes::CHttpTypes()
{
}

// Destructor
CHttpTypes::~CHttpTypes()
{
	delete iHttpTypesArray;
}


void CHttpTypes::ParseTypeLineL(const TDesC& aLine)
{
	TLex lex(aLine);
	TBuf<256> buffer;

	lex.SkipSpaceAndMark();
	lex.SkipCharacters();
	buffer = lex.MarkedToken();

	if (buffer.Length() != 0)
	{
		
		lex.SkipSpaceAndMark();
		lex.SkipCharacters();
		buffer = lex.MarkedToken();

		if (buffer.Length() != 0)
		{
			lex.SkipSpace();
			if (lex.Eos())
			{
				buffer = aLine;
				buffer.Trim();
				iHttpTypesArray->AppendL(buffer);
			}
		}
	}
}

TInt CHttpTypes::FindContentType(const TDesC& aExtension, TDes& aContentType) const
{
	TInt err = KErrNotFound;
	TInt len = iHttpTypesArray->Count();
 	TBuf<9> extension = aExtension;

	extension.Append('\t'); // KColumnListSeparator = '\t'
	
	for (TInt index = 0; ((index < len) && (err == KErrNotFound)); index++)
	{
		if ((*iHttpTypesArray)[index].Find(extension) == 0)
		{
			aContentType = (*iHttpTypesArray)[index].Mid(extension.Length());
			err = KErrNone;
		}

	}

	return err;
}


void CHttpTypes::SetDefaultHttpTypesL()
{
	if (iHttpTypesArray == 0)
	{
		iHttpTypesArray  = new (ELeave) CDesCArrayFlat(HTTP_TYPES_ARRAY_GRANURALITY);
	}

	ParseTypeLineL(_L("html\ttext/html"));
	ParseTypeLineL(_L("htm \ttext/html"));
	ParseTypeLineL(_L("txt\ttext/plain"));
	ParseTypeLineL(_L("gif\timage/gif"));
	ParseTypeLineL(_L("jpg\timage/jpeg"));
	ParseTypeLineL(_L("js\tapplication/octet-stream"));
	ParseTypeLineL(_L("ps\tapplication/postscript"));
	
	iHttpTypesArray->Sort();
}

void CHttpTypes::ReadHttpTypesFileL(const TDesC& aFileName)
{
	RFs fs;

#if EPOC_SDK >= 0x06000000
	User::LeaveIfError(fs.Connect());
	iHttpTypesArray = new (ELeave) CDesCArrayFlat(HTTP_TYPES_ARRAY_GRANURALITY);

	HBufC *config = NULL;
	// Need to TRAP leaves, because must close the fs (right).
	// Thus, no point in using the CleanupStack for config either.
	//
	TRAPD(err, config = UnicodeLoad::LoadL(fs, aFileName));
	if (err == KErrNone && config != NULL)
		{
		TLineBuffer buffer(config->Des());
		while (!buffer.EOB())
			{
			TPtrC line = buffer.ReadLine();
			if (line.Length() > 0)
				{
				TRAP(err, ParseTypeLineL(line));
				if (err != KErrNone)
					break;
				}
			}
		}
	delete config;
	User::LeaveIfError(err);

#else
	RFile cfg;
	TFileText cfgtxt;
	TBuf<256> buffer;

	User::LeaveIfError(fs.Connect());
	User::LeaveIfError(cfg.Open(fs,aFileName,EFileStreamText));

	iHttpTypesArray = new (ELeave) CDesCArrayFlat(HTTP_TYPES_ARRAY_GRANURALITY);
	
	cfgtxt.Set(cfg);
	User::LeaveIfError(cfgtxt.Seek(ESeekStart));
	
	cfgtxt.Read(buffer);
	while (buffer.Length() > 0)
	{
		ParseTypeLineL(buffer);
		cfgtxt.Read(buffer);
	}
	
	cfg.Close();
#endif
	iHttpTypesArray->Sort();

	fs.Close();
}


void CHttpTypes::SetHttpTypesArray(CDesCArrayFlat* aHttpTypesArray)
{
	delete iHttpTypesArray;
	iHttpTypesArray = aHttpTypesArray;
}


void CHttpTypes::SaveHttpTypesFileL(const TDesC& aFileName) const
{
	RFs fs;
	RFile cfg;
	TBuf8<256> buffer;
	TInt len;

	User::LeaveIfError(fs.Connect());
	User::LeaveIfError(cfg.Replace(fs,aFileName,EFileStreamText|EFileWrite));

	len = iHttpTypesArray->Count();

	for (TInt index = 0; index < len; index++)
	{
		// *NOTE* With wide build, Copy will do 16 -> 8 truncation
		// which should not cause problems, as type strings must be
		// plain ASCII anyway -- msa
		buffer.Copy((*iHttpTypesArray)[index]);
		buffer.Append('\n');
		cfg.Write(buffer);
	}

	cfg.Close();
	fs.Close();
		
}
