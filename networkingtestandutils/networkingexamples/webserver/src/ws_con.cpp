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
// ws_con.cpp - http server connection manager
// WebServer connection handles a connection between a client and the
// server.  It implements the HTTP protocol.
// Notation : We will refer the C++ methods as functions.  Methods are
// referred to HTTP methods (GET,HEAD,...)
//

#include <eikenv.h>

#include "ws_con.h"
//#include "ws_sec.h"


CWebServerCon::CWebServerCon() : CActive(CActive::EPriorityStandard)
{
}

CWebServerCon* CWebServerCon::NewL(CWebServerEng* aWebServerEng)
// Function which creates the WebServerCon object.
{
	CWebServerCon* self= new (ELeave) CWebServerCon;
	CleanupStack::PushL(self);
	self->ConstructL(aWebServerEng);
	CleanupStack::Pop();
	return self;
}

void CWebServerCon::StartConnection()
{
	iSocket->SetOpt(KSOBlockingIO,KSOLSocket); // Blocking sockets. Asyncronous behaviour garantizes concurrency.
	WaitingForRequest(); // The WebServer Protocol starts waiting a Request (Waiting a HTTP message)
	SetActive();		 // Request pending.
	iTimer->After(TimeOutTime); // Start the Timer. (Time out control)
}

void CWebServerCon::WebServerBusyL()
{
	iStatusText = _L("\n!! The WebServer has rejected a connection because it can not handle more.(HTTP BUSY ERROR Message) !!\n");
	NotifyStatus();
	iMessage.SetCurrentVersionL();
	iMessage.iConnection = _L("close").AllocL();
	SendErrorMessageL(SC_SERVICE_UNAVAILABLE);
	SetActive();
}

void CWebServerCon::SetObserver(MWebServerObserver* aObserver)
// Set the Observer for the object.
{
	iObserver = aObserver;
}


const TInt CWebServerCon::iOffset = _FOFF(CWebServerCon,iConLink);
// Function which must be implemented for the single-link list from the WebSeverEng.

void CWebServerCon::ConstructL(CWebServerEng* aWebServer)
// Sencond-phase contructor.
{	
	// Initialization of the socket;
	iSocket = aWebServer->iConnection;
	// Initialization of the enviroment variables.
	iServerEnv = aWebServer->iServerEnv; //redundant !!!
	iWebServer = aWebServer;
	//iObserver = aWebserver->iObserver;

	// Initialization of the Logging stuff
	iConId = aWebServer->iTotalNumberConnections;
	iMiscData = iServerEnv->iMiscData;
	iHttpReq = iServerEnv->iHttpReq;
	iHttpResp = iServerEnv->iHttpResp;

	// Initialization of the CGI stuff.
	iExists = FALSE;
	iURISize = 0;
	iTmpInput.Format(_L("tmpin%x.tmp"),(TUint)this);
	iTmpOutput.Format(_L("tmpout%x.tmp"),(TUint)this);
	
	// Initialization of the timer. (Time-Out timer)
	TimeOutTime = TTimeIntervalMicroSeconds32(KTimeOutTime);
	iTimer = CTimeOutTimer::NewL(EPriorityHigh,this);
	
	// Initialization of the file session;
	User::LeaveIfError(ifsSession.Connect());
	
	// Insert this AO in the current Active Scheduler.
	CActiveScheduler::Add(this);
}


CWebServerCon::~CWebServerCon()
{
	// If active then cancel the request.
	Cancel();

	// Close the socket
	iSocket->Close();

	// Delete temporal files (cgi)
	ifsSession.Delete(iTmpInput);
	ifsSession.Delete(iTmpOutput);
	ifsSession.Close();

	// Free memory.
	delete iResource;
	delete iSocket;
	delete iRawMessage;
    delete iPreviousData;
	delete iTimer;
	
}


//******************************************** CGI Functions ************************************************************

TInt CWebServerCon::ThreadFunction(TAny* aArg)
// Function which will take the control of the new thread.
{
	CExecCGI* cgi = (CExecCGI*) aArg;
	cgi->StartCGI();
	cgi->ExecuteCGI();
	//Think about the error
	cgi->CloseCGI();
	return KErrNone;
}

TInt CWebServerCon::SetCgiEnv()
{

	// Define TCgiEnv type and declare the member variable in the class.
	// Note:
	// e.g : /cgipath/cgi/enc_path_info (script name = /cgipath/cgi enc_path_info = /enc_path_info)
	// iCgiPath must be equal to /cgipath/.

	TInt pos,pos2,err = KErrNone;
	TEntry fileentry;
	TBuf<RESOURCE_SIZE> resource;
	TInetAddr address;

	// Obtain ScriptName
	iMessage.GetAbsPath(resource);

	// The path of the cgi can be outside of the root directory of the website.
	// In fact, it must be outside for security reasons.
	// resource.Insert(0,iServerEnv->iServerPath->Des());
	pos = resource.Find(iServerEnv->CgiPath());
	
	if (pos >= KErrNone)
	{
		TLex lex(resource);
		
		pos = iServerEnv->CgiPath().Length() + pos;

		lex.Mark();
		lex.Inc(pos);

		iCgiEnv.iScriptName = lex.MarkedToken();
		//err = ifsSession.Entry(iCgiEnv.iScriptName,fileentry);

		pos2 = resource.LocateReverse('\\');
		if (pos2 >= KErrNone)
		{
			iCgiEnv.iPathTranslated = resource.Mid(pos - 1, (pos2 - pos) + 2);
			iCgiEnv.iScriptName.Append(resource.Mid(pos2 + 1));
		}
		err = ifsSession.Entry(iCgiEnv.iScriptName,fileentry);

		// Note: ***** JUST DONE.
		// An improvement would be to allow subdirectories in the cgi_path.
		// The way to do it would be parse resource until
		// we find that the entry is a file nam instead of directory name.
		// Obtain path variables.

		if (err == KErrNone)
		{
			if (iCgiEnv.iPathTranslated.Length() > 0 )
			{
				iCgiEnv.iPathInfo = iCgiEnv.iPathTranslated;
				TLex lex2(iCgiEnv.iPathInfo);

				// Undo Change Slash.
				while (!lex2.Eos())
				{
					if (lex2.Peek() == '\\')
					{
						pos = lex2.Offset();
						iCgiEnv.iPathInfo.Replace(pos,1,_L("/"));
					}
					lex2.Inc();
				}
			}

			// Obtain the remote address.
			iSocket->RemoteName(address);
			TBuf<REMOTE_ADDRESS_SIZE> rmtaddr;
			address.Output(rmtaddr);
			//
			if (iMessage.iAuthorization !=0)
				iCgiEnv.iAuthType = iMessage.iAuthorization->Des();
			if (iMessage.iContentLength !=0)
				iCgiEnv.iContentLength = iMessage.iContentLength->Des();
		
			if (iMessage.iContentType !=0)
				iCgiEnv.iContentType = iMessage.iContentType->Des();
		
			iCgiEnv.iGatewayInterface = CGIVersion;
			iMessage.GetCGIQueryString(iCgiEnv.iQueryString);
			iCgiEnv.iRemoteAddr = rmtaddr;
			
			if (iMessage.iUserAgent !=0)
				iCgiEnv.iRemoteIdent = iMessage.iUserAgent->Des();
			if (iMessage.iMethod !=0)
				iCgiEnv.iRequestMethod= iMessage.iMethod->Des();
			if (iMessage.iHost != 0)
				iCgiEnv.iServerName = iMessage.iHost->Des();
			
			iCgiEnv.iServerPort.Num(iServerEnv->iPort);
			iCgiEnv.iServerProtocol = iMessage.iVersion->Des();
			iCgiEnv.iServerSoftware = SERVER_NAME;
		}
	}
	else
		err = pos;

	return err;
}

void CWebServerCon::ExecCGIL()
{
	TInt err;
	
	iMessage.GetCGICommandArgumentsL(comarg);	
	err = SetCgiEnv();
	if (err == KErrNone)
	{
		err = library.Load(iCgiEnv.iScriptName);
		if (err == KErrNone)
		{
			if (library.Type()[1] == KCExecCGIUid)
			{	
				// Two phase construction.
				TLibraryFunction entry = library.Lookup(1);
				cgi = (CExecCGI*) entry();

				CleanupStack::PushL(cgi);
				cgi->ConstructL(iTmpInput,iTmpOutput,comarg,iCgiEnv);
				CleanupStack::Pop();

				// Create & Resume the thread which executes the CGI.
				
				TBuf<THREADNAME_SIZE> threadname;
				threadname.Format(_L("CGI%x"),(TUint)cgi);
				thread.Create(threadname,
					(TThreadFunction)ThreadFunction,
					KDefaultStackSize,
					KMinHeapSize,
					KHeapSize,				
					cgi,
					EOwnerThread);
				thread.Logon(iStatus);
				//thread.SetPriority(EPriorityMuchLess);
				thread.Resume();
				


				iConState = EProcessingCGI;
			}
			else
				// Find a more suitbale error !!!
				SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
		}
		else if (err == KErrNotFound)
			SendErrorMessageL(SC_NOT_FOUND);
		else
			SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);

	}
	else if (err == KErrNotFound)
		SendErrorMessageL(SC_NOT_FOUND);
	else
		SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);

}

//*************************************************************************************************************************

void CWebServerCon::SendErrorMessageL(TUint aErrorType)
// Functions which sends the HTTP error messages.
{
	TInt err;	

	iMessage.CleanMessage();
	iMessage.SetStatusLineL(aErrorType);
	iMessage.SetServerL();
	iMessage.SetDateL();

	if ((aErrorType > SC_INFORMATIONAL_MAX &&  aErrorType != SC_NO_CONTENT && aErrorType != SC_NOT_MODIFIED && aErrorType != SC_MOVED_TEMPORARILY)
		&& (iMessage.GetMethod() != EHead) && iMessage.iVersion->Compare(_L("HTTP/0.9")) != 0)
	{// The RFC sets that all informational codes and the 204 & 304 code MUST NOT send an entity-body.
		iMessage.iContentType = HTML_CONTENT_TYPE.AllocL();
		
		TBuf<RESOURCE_SIZE> filename = iWebServer->iServerEnv->ErrorPath();
		filename.AppendFormat(_L("\\error%d.htm"),aErrorType);

		err = iFile.Open(ifsSession,filename,EFileRead|EFileShareAny|EFileStream);
		
		if (err == KErrNone)
		{
			TInt size;
			err = iFile.Size(size);
			iMessage.SetContentLengthL(size);
			iConState = ESendingGet; //IPV6??
			iMessage.MakeRawMessage(iData);
		}
		else
		{
			// Set the length of the Error message to be send.
			switch (aErrorType)
			{
			case SC_CREATED:
				iMessage.SetContentLengthL(SC_CREATED_MES.Length());
				break;
			case SC_NOT_FOUND:
				iMessage.SetContentLengthL(SC_NOT_FOUND_MES.Length());
				break;
			case SC_NOT_ACCEPTABLE:
				iMessage.SetContentLengthL(SC_NOT_ACCEPTABLE_MES.Length());
				break;
			case SC_CONFLICT:
				iMessage.SetContentLengthL(SC_CONFLICT_MES.Length());
				break;
			case SC_BAD_REQUEST:
				iMessage.SetContentLengthL(SC_BAD_REQUEST_MES.Length());
				break;
			case SC_LENGTH_REQUIERED:
				iMessage.SetContentLengthL(SC_LENGTH_REQUIERED_MES.Length());
				break;
			case SC_PRECONDITION_FAILED:
				iMessage.SetContentLengthL(SC_PRECONDITION_FAILED_MES.Length());
				break;
			case SC_RANGE_NOT_SATISFIABLE:
				iMessage.SetContentLengthL(SC_RANGE_NOT_SATISFIABLE_MES.Length());
				break;
			case SC_REQUEST_URI_TOO_LONG:
				iMessage.SetContentLengthL(SC_REQUEST_URI_TOO_LONG_MES.Length());
				break;
			case SC_INTERNAL_SERVER_ERROR:
				iMessage.SetContentLengthL(SC_INTERNAL_SERVER_ERROR_MES.Length());
				break;
			case SC_REQUEST_TIMEOUT:
				iMessage.SetContentLengthL(SC_REQUEST_TIMEOUT_MES.Length());
				break;
			case SC_NOT_IMPLEMENTED:
				iMessage.SetContentLengthL(SC_NOT_IMPLEMENTED_MES.Length());
				break;
			case SC_UNAUTHORIZED:
				iMessage.SetContentLengthL(SC_UNAUTHORIZED_MES.Length());
				break;
			case SC_SERVICE_UNAVAILABLE:
				iMessage.iRetryAfter = RETRY_AFTER_TIME.AllocL();
				iMessage.SetContentLengthL(SC_SERVICE_UNAVAILABLE_MES.Length());
				break;
			case SC_HTTP_VERSION_NOT_SUPPORTED:
				iMessage.SetContentLengthL(SC_HTTP_VERSION_NOT_SUPPORTED_MES.Length());
				break;
			default:
				iMessage.SetContentLengthL(SC_UNEXPECTED_MES.Length());
				break;
			}
			
			// Make the Header Message
			iMessage.MakeRawMessage(iData);

			// Set the error message to be send. (Body message)
			
			switch (aErrorType)
			{
			case SC_CREATED:
				iData.Append(SC_CREATED_MES);
				break;
			case SC_NOT_FOUND:
				iData.Append(SC_NOT_FOUND_MES);
				break;
			case SC_NOT_ACCEPTABLE:
				iData.Append(SC_NOT_ACCEPTABLE_MES);
				break;
			case SC_CONFLICT:
				iData.Append(SC_CONFLICT_MES);
				break;
			case SC_BAD_REQUEST:
				iData.Append(SC_BAD_REQUEST_MES);
				break;
			case SC_LENGTH_REQUIERED:
				iData.Append(SC_LENGTH_REQUIERED_MES);
				break;
			case SC_PRECONDITION_FAILED:
				iData.Append(SC_PRECONDITION_FAILED_MES);
				break;
			case SC_RANGE_NOT_SATISFIABLE:
				iData.Append(SC_RANGE_NOT_SATISFIABLE_MES);
				break;
			case SC_REQUEST_URI_TOO_LONG:
				iData.Append(SC_REQUEST_URI_TOO_LONG_MES);
				break;
			case SC_INTERNAL_SERVER_ERROR:
				iData.Append(SC_INTERNAL_SERVER_ERROR_MES);
				break;
			case SC_REQUEST_TIMEOUT:
				iData.Append(SC_REQUEST_TIMEOUT_MES);
				break;
			case SC_NOT_IMPLEMENTED:
				iData.Append(SC_NOT_IMPLEMENTED_MES);
				break;
			case SC_UNAUTHORIZED:
				iData.Append(SC_UNAUTHORIZED_MES);
				break;
			case SC_SERVICE_UNAVAILABLE:
				iData.Append(SC_SERVICE_UNAVAILABLE_MES);
				break;
			case SC_HTTP_VERSION_NOT_SUPPORTED:
				iData.Append(SC_HTTP_VERSION_NOT_SUPPORTED_MES);
				break;
			default:
				iData.Append(SC_UNEXPECTED_MES);
				break;
			}
			
		iConState = ESendingError;
/*		if (iMessage.CloseConnection())
			iConState = EFinnished;
		else
			iConState = EWaitingRequest;*/
		}
	}
	else
	{
		iMessage.MakeRawMessage(iData);
		iConState = ESendingError;
/*
		if (iMessage.CloseConnection())
			iConState = EFinnished;
		else
			iConState = EWaitingRequest;
*/	}

	iSocket->Send(iData,0,iStatus,iLen);
}


void CWebServerCon::GetHttpMessageL()
// This functions processes the data from the socket. Which can be processed by the Message functions.
{
    TInt pos = 0;

	HBufC8* aux;
	
	if (iPreviousData == 0)
	{
		iPreviousData = iCurrentData.AllocL();
	}
	else
	{
		aux = HBufC8::NewL(iPreviousData->Length() + iCurrentData.Length());
		aux->Des().Copy(iPreviousData->Des());
		aux->Des().Append(iCurrentData);
		delete iPreviousData;
		iPreviousData = aux;
	}

	if ((pos = iPreviousData->Find(_L8("\r\n\r\n"))) >= KErrNone)
	{

		delete iRawMessage;
		iRawMessage = iPreviousData->Left(pos + 4).AllocL();
		if (pos + 4 < iPreviousData->Length())
		{	
			aux = iPreviousData->Mid(pos + 4).AllocL();
			delete iPreviousData;
			iPreviousData = aux;
		}
		else
		{
			delete iPreviousData;
			iPreviousData= 0;
		}
		iConState = EReceivedRequest;
	}
	else
	{
		iConState = EReceivingRequest;
	}

}

void CWebServerCon::SendGetResponseL()
// Functions which handles a GET & HEAD response message.
{
	TInt err,size;
	TTime date;
	TInt64 ETag;
	TBuf<MAX_HEADER_LEN> contenttype;
	TParse parse;

	err=iFile.Modified(date);
	if ( err == KErrNone)
	{
		ETag = date.Int64();
		if (iMessage.IfNoneMatch(ETag)) //Note: If there is If-None-Match field, and none value matches then
		{								// if there is If-Modified-Since field it is ignored (is deleted in iMessage)
			if (iMessage.IfModSince(date) || iMessage.ExistsIfRange())
			{
				err = iFile.Size(size);	
				if (err == KErrNone)
				{
					parse.Set(iResource->Des(),NULL,NULL);
					
					if (parse.ExtPresent())
					{
						err = iWebServer->iHttpTypes.FindContentType(parse.Ext().Mid(1),contenttype);
					}
					else
					{
						err = KErrNotFound;
					}

					if (err >= KErrNotFound)
					{
						if (err == KErrNotFound)
							contenttype = DEFAULT_CONTENT_TYPE;

						err = iMessage.CheckAccept(contenttype);
						if (err == KErrNone)
						{
							if (iMessage.IfMatch(ETag) && iMessage.IfUnModSince(date))
							{

								if (iMessage.CheckRange(size) == KErrNone)
								{
									iMessage.CleanMessage();
									if (iMessage.ExistsRange() && iMessage.IfRange(ETag,date))
									{
										iMessage.SetStatusLineL(SC_PARTIAL_CONTENT);
										if (iMessage.IsMultiRange())
										{
											TTime aux;
											aux.UniversalTime();
#ifdef I64LOW
											iBoundary.Format(_L("%x%x"),I64LOW(aux.Int64()),I64HIGH(aux.Int64()));
#else
											iBoundary.Format(_L("%x%x"),aux.Int64().Low(),aux.Int64().High());
#endif
											contenttype.Format(_L("multipart/byteranges; boundary=\"%S\""),&iBoundary);			
											iConState = ERangeSent;
										}
										else
										{
											iCurrentPosition = 0; iLastPosition = size - 1;
											iMessage.SetRangeValuesL(iCurrentPosition,iLastPosition);
											if (iCurrentPosition < 0) iCurrentPosition = size + iCurrentPosition;
											if (iLastPosition < iCurrentPosition) iLastPosition = size - 1;
											iMessage.SetContentLengthL((iLastPosition - iCurrentPosition) + 1);
											iMessage.SetContentRangeL(iCurrentPosition,iLastPosition,size);
											iConState = ESendingRange;
										}
									}
									else
									{
										iMessage.SetStatusLineL(SC_OK);
										iMessage.SetContentLengthL(size);
										iConState=ESendingGet; //IPV6
									}
									iMessage.SetServerL();
									iMessage.SetDateL();
									iMessage.iContentType = contenttype.AllocL();
									iMessage.SetLastModifiedL(date);
									iMessage.SetETagL(ETag);
									iMessage.MakeRawMessage(iData);

									iSocket->Send(iData,0,iStatus,iLen);
								}
								else
									SendErrorMessageL(SC_RANGE_NOT_SATISFIABLE);
							}
							else
								SendErrorMessageL(SC_PRECONDITION_FAILED);
						}
						else if (err == KErrNotFound)
							SendErrorMessageL(SC_NOT_ACCEPTABLE);
						else
							SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
					}
					else
						SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
				}
				else
					SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
			}
			else
				SendErrorMessageL(SC_NOT_MODIFIED);
		}
		else
			SendErrorMessageL(SC_NOT_MODIFIED);
	}
	else
		SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
}


void CWebServerCon::SendRangeURI()
{
	iSize = Min(BUFFER_SIZE,(iLastPosition - iCurrentPosition) + 1);
	iFile.Seek(ESeekStart, iCurrentPosition);
	iFile.Read(iData,iSize);
	iSocket->Send(iData,0,iStatus,iLen);
	iCurrentPosition = iCurrentPosition + iSize;

	if (iCurrentPosition >= iLastPosition) iConState = ERangeSent;
}


void CWebServerCon::SendRangeHeadersL()
{
	TInt size;
	TParse parse;
	TInt err;
	TBuf<MAX_HEADER_LEN> contenttype;

	iFile.Size(size);
	
	parse.Set(iResource->Des(),NULL,NULL);
	if (parse.ExtPresent())
	{
		// Mid(1) because we don't want the dot '.'
		err = iWebServer->iHttpTypes.FindContentType(parse.Ext().Mid(1),contenttype);
	}
	else
	{
		err = KErrNotFound;
	}

	iCurrentPosition = 0; iLastPosition = size - 1;
	iMessage.SetRangeValuesL(iCurrentPosition,iLastPosition);

	if (iCurrentPosition < 0) iCurrentPosition = size + iCurrentPosition;
	if (iLastPosition < iCurrentPosition) iLastPosition = size - 1;
	if (err == KErrNotFound) contenttype = DEFAULT_CONTENT_TYPE;
//	if (iCurrentPosition < 0) iCurrentPosition = size + iCurrentPosition;	// Suffix-Range.
																			// e.g.: size 2000
																			// Range: -1 = Range: 1999 - 2000
//	if (iLastPosition == -1) iLastPosition = size; // That is when we have a range "500-" for example.

	iData.AppendFormat(_L8("--%S\r\n"),&iBoundary);
	iData.AppendFormat(_L8("Content-Range: bytes %d - %d/%d\r\n"),iCurrentPosition,iLastPosition,size);
	iData.AppendFormat(_L8("Content-Type: %S\r\n\r\n"),&contenttype);

	iSocket->Send(iData,0,iStatus,iLen);
	iConState = ESendingRange;

}

void CWebServerCon::RangeSentL()
{
	if (iMessage.MoreRanges())
	{
		iData =_L8("\r\n");
		SendRangeHeadersL();
	}
	else
	{
		if (iBoundary.Length() > 0)
		{
			iData.Format(_L8("\r\n--%S--\r\n"),&iBoundary);
			iSocket->Send(iData,0,iStatus,iLen);
			iBoundary = _L("");
			iConState = ERangeSent;
		}
		else
		{
			if (iMessage.CloseConnection())
				{
					iConState = EFinnished;
				}
			else
				WaitingForRequest();
		}
	}
}

void CWebServerCon::SendURI()
{
	TInt err = 0;
	
	err = iFile.Read(iData);
	if (err == KErrNone)
	{
		if (iData.Length() > 0) // If it is not the end of file.
		{
			iSocket->Send(iData,0,iStatus,iLen);
			iConState = ESendingGet;
		
		}
		else
		{	// MessageBody has been all send
			iFile.Close();
			
			if (iMessage.CloseConnection()) // Is a Connectionless request ?
			{
				iConState = EFinnished;
			}
			else
			{
				WaitingForRequest();
			}	
			
		}
	}
	else
	{
		iFile.Close();
		iConState = EFinnished;
		//ShuttingDownConnection(RSocket::EImmediate);
	}
}



void CWebServerCon::PutURIL()
{
	if (iConState == EPuttingURI)
	{
		iURISize += iCurrentData.Length();
		iFile.Write(iCurrentData);
	}
	
	if (iURISize < iURISizeMax) //If we are expecting more data then we listen again.
	{
		iConState = EPuttingURI;
		iLen = iCurrentData.MaxLength();
		iSocket->RecvOneOrMore(iCurrentData,0,iStatus,iLen);
		// iTimer->After(TimeOutTime);
		// Then I have to change the cancel function. I should close the file and delete it.
	}
	else
	{	
		if (iExists)
			SendErrorMessageL(SC_NO_CONTENT);
		else
			SendErrorMessageL(SC_CREATED);

		iExists  = FALSE;
		iURISize = 0;
		iFile.Close();
	}
}


// ************************************** POST METHOD FUNCTIONS **********************************************************
//*************************************************************************************************************************

void CWebServerCon::GetPostBodyL()
{
	if (iConState == EGettingPostBody)
	{
		iURISize += iCurrentData.Length();
		iFile.Write(iCurrentData);
	}
	if (iURISize < iURISizeMax) //If we are expecting more data then we listen again.
	{
		iConState = EGettingPostBody;
		iLen = iCurrentData.MaxLength();
		iSocket->RecvOneOrMore(iCurrentData,0,iStatus,iLen);		
	}
	else
	{	
		iURISize = 0;
		iFile.Close();
		ExecCGIL();
	}
}

void CWebServerCon::ProcessCGIHeaderL(HBufC* aBuffer)
{

	if (aBuffer->Find(_L("Content-Type:")) == 0 )
		iMessage.SetHeaderLineL(aBuffer);
	else if (aBuffer->Find(_L("Location:")) == 0)
	{
		iMessage.SetHeaderLineL(aBuffer);
		iMessage.SetStatusLineL(SC_MOVED_TEMPORARILY);
	}
	else if (aBuffer->Find(_L("Status:")) == 0)
	{
		iMessage.iStatusCode = aBuffer->Mid(8,3).AllocL();
		iMessage.iReasonPhrase  = aBuffer->Mid(12).AllocL();
	}
	else
		iMessage.SetHeaderLineL(aBuffer); // If the header is unknow then skip it.
}

void CWebServerCon::ParseHeaderCGIL()
{

	TBuf<256> buffer;
	TInt err;
	TFileText TxtFile;
	TInt pos = 0;
	TInt size = 0;


	TxtFile.Set(iFile);
	TxtFile.Seek(ESeekStart);
	

	iMessage.CleanMessage();

	err=TxtFile.Read(buffer);
	pos = buffer.Length() + 1; // What happen when the file is empty !!?

	while ((err == KErrNone) && (buffer.Length() > 0)) // && (!(buffer.Compare(_L("\r\n")) == 0)) )
	// Note: Think about the KErrTooBig. May be I should try to read all the data and then set the field instead of
	// returning an error
	{
		HBufC* aux = buffer.AllocL();
		ProcessCGIHeaderL(aux);
		delete aux;
		err=TxtFile.Read(buffer);
		pos += buffer.Length() + 1;
	}
	pos += 2;

	if ((err == KErrEof) || (err == KErrNone)) // Think about the KErrEof case. We shouldn't try to open & seek the file.
	{

		iFile.Close();

		if ((err=iFile.Open(ifsSession,iTmpOutput,EFileStream)) == KErrNone)
		{
				err=iFile.Seek(ESeekStart,pos);
		}
		if (err == KErrNone)
		{
				if (iMessage.iStatusCode == 0)
					iMessage.SetStatusLineL(SC_OK);
				if (iMessage.iContentLength == 0)
				{
					iFile.Size(size);
					size = size - pos;
				}
				iMessage.SetContentLengthL(size);
				iMessage.SetServerL();
				iMessage.SetDateL();	
				iMessage.MakeRawMessage(iData);
				if (iMessage.GetMethod() == EHead)
				{
					iFile.Close();
					iConState = ECloseConnection;
/*
					if (iMessage.CloseConnection())
						iConState = EFinnished;
					else
						iConState = EWaitingRequest;
 */				}
				else
				{
					iConState=ESendingGet; //IPV6???
				}
				iSocket->Send(iData,0,iStatus,iLen);
		}
	
	}

	if (err != KErrNone)
		SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);

}



void CWebServerCon::SendCGIOutputL()
{
	TInt err;

	thread.Close();
	delete cgi;
	library.Close();

	TParse parse;
	parse.Set(iCgiEnv.iScriptName,NULL,NULL);
	if (parse.Name().Find(_L("nph-")) == 0) // Is a non-parse header CGI ?	
	{
		err =iFile.Open(ifsSession,iTmpOutput,EFileStream);
		if (err == KErrNone)
		{
			if (iMessage.GetMethod() == EHead)
			{ //Check this code. What is it going to happen when the HTTP Message is larger than iData size ?
				iFile.Read(iData);
				TInt pos;
				pos = iData.Find(_L8("\r\n\r\n"));
				if (pos >= KErrNone)
				{
					iData = iData.Left(pos + 4);
					iConState = ECloseConnection;
/*	
					if (iMessage.CloseConnection())
						iConState = EFinnished;
					else
						iConState = EWaitingRequest;
					datalen = iData.Length();*/
					iSocket->Send(iData,0,iStatus,iLen);
				}
				else
					err = KErrArgument;
				iFile.Close();
			}
			else
			{
				//iConState = ESendingGet;
				SendURI();
			}
		}
	}
	else
	{
		err = iFile.Open(ifsSession,iTmpOutput,EFileStreamText|EFileShareExclusive|EFileRead);
		if (err == KErrNone)
			ParseHeaderCGIL();
	}
	if (err == KErrNotFound) // If there is no file , there is no output
						 // and the default behaviour is to send a Non Content Response
		SendErrorMessageL(SC_NO_CONTENT);
	else if (err != KErrNone)
		SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);

}

//***** Chunked Encoding functions: This kind of encoding will be used when the comunication with the CGI will be through a "pipe"
// instead of a file.
/*
void CWebServerCon::SendCGIOutputL()
{
	delete cgi;
	library.Close();

	TParse parse;
	parse.Set(iCgiEnv.iScriptName,NULL,NULL);
	if (parse.Name().Find(_L("nph-")) == 0) // Is a non-parse header CGI ?	
	{
		
		
	}
	else
	{
		err = iFile.Open(ifsSession,iTmpOutput,EFileStreamText|EFileShareExclusive|EFileRead);
		if (err == KErrNone)
			ParseHeaderCGIL();
	}
	if (err == KErrNotFound) // If there is no file , there is no output
							 // and the default behaviour is to send a Non Content Response
		SendErrorMessageL(SC_NO_CONTENT);
	else if (err != KErrNone)
		SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);


}
*/
//**** End Chuncked Encoding.

//*************************************************************************************************************************

TInt CWebServerCon::BackupResource()
{
	TBuf<256> newresource;
	TBuf8<BACKUP_MESSAGE_SIZE> message;
	TInt err=KErrNone;
	TInt pos = 0;
	RFile log;

	// A little trick to get an unique filename. ;)
	err = log.Temp(ifsSession,iWebServer->iServerEnv->BackupPath(),newresource,EFileWrite);
	if(err == KErrNone)
	{
		log.Close();
		// Move the resource.
		TPtrC resource =iResource->Des();
		err = ifsSession.Replace(resource,newresource);	
		if (err == KErrNone)
		{	// Update the log file. (This is used to undo changes by the WebMaster if it is needed.
			err = log.Open(ifsSession,LOG_FILENAME,EFileStreamText|EFileWrite|EFileShareAny);
			if (err == KErrNotFound)
				err = log.Create(ifsSession,LOG_FILENAME,EFileStreamText|EFileWrite|EFileShareAny);
			else
				err = log.Seek(ESeekEnd,pos);
			if (err == KErrNone)
			{				
				TPtrC host = iMessage.iHost->Des();
				message.Format(_L8("\r\nHost:%S File %S moved to %S\r\n"),&(host),&(resource),&newresource);
				log.Write(message);
				log.Write(iRawMessage->Des());
				log.Write(_L8("\r\n***\r\n"));
				log.Close();
			}
		}
	}
	return err;
}

void CWebServerCon::PutMethodL()
// This function handles the Put Mehotd. This version doesn't have security issues.But it must be implemented
{
	TInt err = KErrNone;
	TEntry entry;
	

	iExists = FALSE;
	iURISizeMax = 0;		
	
	//	err = iMessage.CheckAuthorization();
	if (iWebServer->iServerEnv->iPutMethod)
	{
		if (err == KErrNone)
		{
			SetResourceL();			
			err = ifsSession.Entry(iResource->Des(),entry);
			iExists = (err != KErrNotFound);
			if ((err == KErrNone) || (err == KErrNotFound))
			{
				
				if (iMessage.IfUnModSince(entry.iModified) && iMessage.IfMatch(entry.iModified.Int64()))
				{
					 iURISizeMax = iMessage.GetContentLength();
					 if (iURISizeMax >= 0)
					 {
						err=BackupResource();
						if ((err == KErrNone) || (err == KErrNotFound))
						{
							err = iFile.Replace(ifsSession,iResource->Des(),EFileStream);
							if (err == KErrNone)
							{
								if (iPreviousData !=0) // If has been send data with the HTTP Message we write it down.
								{
									iURISize += iPreviousData->Des().Length();
									iFile.Write(iPreviousData->Des());
								}
								PutURIL();
							}
							else  // Unexpeted error trying to open the file to write.
								SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
						}
						else //Unexpected error trying to backup the file.
							SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
					 }
					else if (iURISizeMax == KErrNotFound) // The content-length field has not been initialised.
						SendErrorMessageL(SC_LENGTH_REQUIERED);
					else //The data in the content length is not in expected format
						SendErrorMessageL(SC_BAD_REQUEST);
				}
				else // The URI has been modified after that date. OR The entintiy tag don't match
					SendErrorMessageL(SC_PRECONDITION_FAILED);
			}
			else if (err == KErrPathNotFound) // The path doesn't exist
					SendErrorMessageL(SC_NOT_FOUND);
			else // Unexpected error.
				SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
		}
		else if (err == KErrAccessDenied) // The Authorization field is missing or wrong.
			SendErrorMessageL(SC_UNAUTHORIZED);
		else // Just in case.
			SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
	}
	else
		SendErrorMessageL(SC_NOT_IMPLEMENTED);// Method Not Allowed !!?
}

void CWebServerCon::PostMethodL()
{
	iExists = FALSE;
	iURISizeMax = 0;		

	 iURISizeMax = iMessage.GetContentLength();
	 if (iURISizeMax > 0)
	 {
		
		if (iFile.Replace(ifsSession,iTmpInput,EFileStream) == KErrNone)
		{
			if (iPreviousData != 0)// If it has been send data with the HTTP Message we write it down.
			{
				iURISize += iPreviousData->Des().Length();
				iFile.Write(iPreviousData->Des());
				// I should delete the iPreviousData. Its data has just been consumed.
				delete iPreviousData;
				iPreviousData = 0;
			}
			GetPostBodyL();
		}
		else
			SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
	 }		
	 else // The URI has been modified after that date.
		SendErrorMessageL(SC_PRECONDITION_FAILED);

}

void CWebServerCon::OptionsMethodL()
// This funciton handles the Option method.
{

	TBuf<RESOURCE_SIZE> resource;
	TEntry fileentry;
	TInt err = KErrNone;
	iMessage.CleanMessage();
	
	if (iMessage.GetURI().CompareF(_L("*")) == 0)
	{
		iMessage.SetAllowL(EAll);
		iMessage.SetStatusLineL(SC_OK);
		iMessage.SetServerL();
		iMessage.SetDateL();
		iMessage.iContentType = HTTP_CONTENT_TYPE.AllocL();
		iMessage.SetContentLengthL(0);
		//iMessage.SetAcceptRanges();
		//iMessage.SetAcceptEncoding();				
		iMessage.MakeRawMessage(iData);
		iSocket->Send(iData,0,iStatus,iLen);
	}
	else
	{
		if (!iMessage.IsQuery())
		{
				// Obtain ScriptName
			SetResourceL();	
			resource = iResource->Des();
			if (resource.FindF(iServerEnv->CgiPath()) == 0)
			{
				// This code is a cut and paste from SetCgiEnv -> I should do a function.
				
				TLex lex(resource);
				TInt pos = 0,pos2=0;
				pos = iServerEnv->CgiPath().Length();
				lex.Mark();
				lex.Inc(pos);
				TBuf<RESOURCE_SIZE> iScriptName;
				iScriptName = lex.MarkedToken();
			 	pos2 = resource.LocateReverse('\\');
				if (pos2 >= KErrNone)
				{
					iScriptName.Append(resource.Mid(pos2 + 1));
				}
				err = ifsSession.Entry(iScriptName,fileentry);
				if(err == KErrNone)
				{
					if (fileentry.IsArchive())
						iMessage.iAllow = _L("POST").AllocL();
					else
						err = KErrNotFound;
				}

			}
			else
			{
				err = ifsSession.Entry(iResource->Des(),fileentry);
				if (err == KErrNone)
					if (fileentry.IsArchive())
						iMessage.iAllow = _L("GET, HEAD").AllocL();
					else
						err = KErrNotFound;
				
			}

			if (err == KErrNone)
			{

				iMessage.SetStatusLineL(SC_OK);
				iMessage.SetServerL();
				iMessage.SetDateL();
				iMessage.iContentType = HTTP_CONTENT_TYPE.AllocL();
				iMessage.SetContentLengthL(0);
				iMessage.MakeRawMessage(iData);
				iSocket->Send(iData,0,iStatus,iLen);
			}
			else
				SendErrorMessageL(SC_NOT_FOUND);
		}
		else
			SendErrorMessageL(SC_BAD_REQUEST);
	}
}

void CWebServerCon::SendTraceBody()
// This function sends the message body of a TRACE response.
{

	if (iRawMessage == 0)
	{
		iConState = ECloseConnection;

		if (iMessage.CloseConnection()) // Is a Connectionless request ?
			iConState = EFinnished;
		else
			WaitingForRequest();
	}
	else
	{

		if (iRawMessage->Length() > BUFFER_SIZE)
		{
			iData.Copy(iRawMessage->Des().Left(BUFFER_SIZE));
			*iRawMessage = iRawMessage->Des().Mid(BUFFER_SIZE);
		}
		else
		{
			iData.Copy(iRawMessage->Des());
			delete iRawMessage;
			iRawMessage = 0;
		}
		iConState = ESendingTrace;
		iSocket->Send(iData,0,iStatus,iLen);
	}

}

void CWebServerCon::TraceMethodL()
// This function implements the TRACE Method.
{
	ASSERT(iRawMessage != NULL);

	iMessage.CleanMessage();
	iMessage.SetStatusLineL(SC_OK);
	iMessage.SetServerL();
	iMessage.SetDateL();
	iMessage.iContentType = HTTP_CONTENT_TYPE.AllocL();
	iMessage.SetContentLengthL(iRawMessage->Length());

	iMessage.MakeRawMessage(iData);

	iConState = ESendingTrace;
	iSocket->Send(iData,0,iStatus,iLen);
	
}

void CWebServerCon::DeleteMethodL()
// This funtion handles the DELETE Method.
{
	TInt err = KErrNone;
	TEntry entry;
	
	if (iWebServer->iServerEnv->iDeleteMethod)
	{
		//err = iMessage.CheckAuthoritation()
		if (err == KErrNone)
		{
			SetResourceL();
			err = ifsSession.Entry(iResource->Des(),entry);
			if (err == KErrNone)
			{
				if (entry.IsArchive())
				{
					if (iMessage.IfUnModSince(entry.iModified))
					{
						err = BackupResource();
						if (err == KErrNone)  // This message must be send according to the RFC.
							SendErrorMessageL(SC_NO_CONTENT);
						else //  Something has gone wrong.
							SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
					}
					else // The URI has been modified after that date.
						SendErrorMessageL(SC_PRECONDITION_FAILED);
				}
				else
					SendErrorMessageL(SC_NOT_FOUND); //The action is forbidden, but we don't want to give this information.So
											         // We send a not found error.(following RFC 2616)
			}
			else if ((err == KErrPathNotFound) || (err == KErrNotFound))// The path or file  doesn't exist
					SendErrorMessageL(SC_NOT_FOUND);
			else // Unexpected error.
				SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
		}
		else if (err == KErrAccessDenied) // The Authorization field is missing or wrong.
			SendErrorMessageL(SC_UNAUTHORIZED);
		else // Just in case.
			SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
	}
	else
		SendErrorMessageL(SC_NOT_IMPLEMENTED);// Method Not Allowed !!?
}

void CWebServerCon::SetResourceL()
{
	TBuf<RESOURCE_SIZE> resource;

	// Get the resource and append the Server directory
	iMessage.GetAbsPath(resource);
	if (iMessage.DefaultResource())
		resource.Append(iWebServer->iServerEnv->DefaultResource());
	
	delete iResource;
	iResource = HBufC::NewL(iWebServer->iServerEnv->ServerPath().Size() + resource.Size());
	*iResource = iWebServer->iServerEnv->ServerPath();
	iResource->Des().Append(resource);	
}


void CWebServerCon::GetMethodL()
// This funtion handles the GET Method.
{
	TInt err = KErrNone;;

	if (iMessage.IsQuery())
	{
		ExecCGIL();
	}
	else
	{
		SetResourceL();
	// It is the job of the WebMaster to protect cgi putting them outside of the WebSite tree !!!!
	//	if (iResource->Des().FindF(iWebServer->iServerEnv->iCgiPath->Des()) != 0)
	//	{
#if 0
			TBuf<RESOURCE_SIZE> test = iResource->Des(); // TEST LINE !!!!!!!!!!!!!!!!!1
#endif
			err = iFile.Open(ifsSession,iResource->Des(),EFileRead|EFileShareAny|EFileStream);
			if (err == KErrNone)
				SendGetResponseL();
			else if ((err == KErrPathNotFound) || (err == KErrNotFound) || (err == KErrAccessDenied))
				SendErrorMessageL(SC_NOT_FOUND);
			else if (err == KErrBadName)
				SendErrorMessageL(SC_BAD_REQUEST);
			else // Something has gone wrong
				SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);

			
	//	}
	//	else // Forbidden action.
	//		SendErrorMessageL(SC_NOT_FOUND);	
	}

}

void CWebServerCon::ProcessingRequestL()
// Reads the incoming data and process the request.
{
	TInt err;
	TMethodType method = EHead;

	GetHttpMessageL();
	if (iConState == EReceivedRequest) //We have received a HTTP message. Now we process it.
	{
	
		// Initilialize iMessage.
		iMessage.CleanFullMessage();
		iMessage.SetMessageL(iRawMessage);
		
		//****** Show Log. information. ******
			
		ShowHttpRequest();

		//****** End Show Log Information ****

		err = iMessage.CheckRequestLine();
		if (err == KErrNone) err = iMessage.CheckHost();
		if (err == KErrNone)
		{
			method = iMessage.GetMethod();
			switch(method)
			{
			case EGet:
			case EHead:
				GetMethodL();
				if (method == EHead && iConState != EProcessingCGI)
				{
					iFile.Close();
					iConState = ECloseConnection;
/*
					if (iMessage.CloseConnection())
						iConState = EFinnished;
					else
						iConState = EWaitingRequest;
*/				}
				break;
			case ETrace:
				TraceMethodL();
				break;
			case EOptions:
				OptionsMethodL();
				break;
			case EPut:
				PutMethodL();
				break;
			case EDelete:
				DeleteMethodL();
				break;
			case EPost:
				PostMethodL();
				break;
			default: // This condition must not be satisfied. But just in case...
				SendErrorMessageL(SC_NOT_IMPLEMENTED);
				break;
			}
			
		}		
		else if (err == KErrNotSupported)
			SendErrorMessageL(SC_HTTP_VERSION_NOT_SUPPORTED);
		else if (err == KErrArgument) // The URI's name is larger than 256.
			SendErrorMessageL(SC_REQUEST_URI_TOO_LONG);
		else if ((err == KErrBadName) || (err == KErrNotFound))
			SendErrorMessageL(SC_BAD_REQUEST);
		else		
			SendErrorMessageL(SC_INTERNAL_SERVER_ERROR);
		
		if (method != EPut)	
			ShowHttpResponse();
	}
	else // It is a big Http header message. :)
		WaitingForRequest();
	
}

void CWebServerCon::ReSendData() // Code for non-blocking sockets.
{

//		SetPriority(CActive::EPriorityStandard -1); // We low th priority to ...	
		iData = iData.Mid(iLen());
		iSocket->Send(iData,0,iStatus,iLen);

}

void CWebServerCon::WaitingForRequest()
// This functions wait for incoming messages.
{
	iConState=EReceivingRequest;
	iLen = iCurrentData.MaxLength();
	iSocket->RecvOneOrMore(iCurrentData,0,iStatus,iLen);

}

void CWebServerCon::ShuttingDownConnection(RSocket::TShutdown aHow)
// This functions shutdowns the socket.
{
	iConState = EFinnished;
	iSocket->Shutdown(aHow,iStatus);
}

void CWebServerCon::ConnectionTimeOutL()
// Function called when the timeout is triggered. Cancel the ongoing request and send a TimeOut message as specified
// in the RFC 2616. It starts again the timer. If the error message can not be send then the connections is shut down.
{
	Cancel();
	SignalRunL(KErrTimeOut);
}

void CWebServerCon::SignalRunL(TInt aError)
// This functions signals the RunL() function with the error aError.
{
	TRequestStatus* p=&iStatus;
	SetActive();
	User::RequestComplete(p,aError);
}

void CWebServerCon::ShowMiscData()
{
	TInetAddr address;
	iSocket->RemoteName(address);
	TBuf<REMOTE_ADDRESS_SIZE> rmtaddr;
	address.Output(rmtaddr);
	iStatusText.Format(_L("\nOrigin Address : %S Connection Number: %d\n"),&rmtaddr,iConId);
	NotifyStatus();
}

void CWebServerCon::ShowHttpRequest()
{
	if (iHttpReq)
	{
		if (iMiscData) ShowMiscData();
		if (iRawMessage != 0)
		{
			TInt size = Min(1024,iRawMessage->Length());
			iStatusText.Copy(iRawMessage->Des().Left(size));
		}
		else
			iStatusText = _L("\n*********************** NO HTTP MESSAGE !!!!!!!!!!!!!!!!!!!!!!*****************\n");
		NotifyStatus();
	}
}

void CWebServerCon::ShowHttpResponse()
{
	if (iHttpResp)
	{
		TBuf8<1024> buffer; // Needed for unicode.
		iMessage.MakeRawMessage(buffer);
		iStatusText.Copy(buffer);// Needed for unicode.
		NotifyStatus();
	}
}


void CWebServerCon::NotifyStatus()
{
	iObserver->Write(iStatusText);
}


void CWebServerCon::DoCancel()
// DoCancel function which must be implemented in all Active Objects.
{
	iTimer->Cancel();

	switch (iConState)
	{
	case ESendingRange:
	case ERangeSent:
	case ESendingGet:
		iFile.Close();
		//lint -fallthrough
	case EWaitingRequest:
	case ESendingTrace:
		iSocket->CancelSend();
		break;
	case EPuttingURI:
		iSocket->CancelRecv();
		iFile.Close();
		ifsSession.Delete(iMessage.GetURI());
		break;
	case EReceivingRequest:
		iSocket->CancelRecv();
		break;
	case EGettingPostBody:
		iSocket->CancelRecv();
		break;
	case EProcessingCGI:
		if (thread.LogonCancel(iStatus) == KErrNone)	
				thread.Kill(KErrCancel);

		thread.Close();
		delete cgi;
		library.Close();
		break;
	default:
		iSocket->CancelAll();
		break;
	}

}

void CWebServerCon::RunL()
{


	TInt error = KErrNone;
	iTimer->Cancel();

	
	SetPriority(CActive::EPriorityStandard - 2);
	if (iStatus == KErrNone)
	{
		switch(iConState)
		{
		case EWaitingRequest:
			WaitingForRequest();
			break;
		case EReceivingRequest:
			TRAP(error,ProcessingRequestL());
			break;
		case ESendingGet:
			if (iLen() != iData.Length()) // Code for non-blocking sockets.
			{
				ReSendData();
			}
			else
			{
				SendURI();
			}
			break;
		case ESendingError:
			if (iLen() != iData.Length()) // Code for non-blocking sockets.
			{
				ReSendData();
			}
			else
			{
				if (iMessage.CloseConnection()) // Is a Connectionless request ?
					iConState = EFinnished;
				else
					WaitingForRequest();	
			}
			break;
		// States needed for ranging request.
		case ESendingRange:
			if (iLen() != iData.Length())  // Code for non-blocking sockets.
				ReSendData();
			else
				SendRangeURI();
			break;
		case ERangeSent:
			if (iLen() != iData.Length())  // Code for non-blocking sockets.
				ReSendData();
			else
				RangeSentL();
			break;
		// End. (Range states)
		case EPuttingURI:
			TRAP(error,PutURIL());
 			break;
		case EGettingPostBody:
			GetPostBodyL();
			break;
		case EProcessingCGI:
			TRAP(error,SendCGIOutputL());
			break;
		case ESendingTrace:
			SendTraceBody();
			break;
		case ECloseConnection: // Should the connection be closed ?
			if (iMessage.CloseConnection()) // Is a Connectionless request ?
				iConState = EFinnished;
			else
				WaitingForRequest();
			break;
		case EShuttingDownCon:
				iConState = EFinnished;
			break;
		case EFinnished:
			break;
		default: // just in case.
			iConState = EFinnished;
			break;
		}

	}
	else if (iStatus == KErrWouldBlock) // Code for non-blocking sockets.
	{
		
			switch (iConState)
			{
			case ESendingGet:
				ReSendData();
				break;
			case ESendingRange:
			case ERangeSent:
				ReSendData();
				break;
			case ESendingTrace:
				ReSendData();
				break;
			case EPuttingURI:
			case EWaitingRequest:
			case EReceivingRequest:
			case EGettingPostBody:
				iLen = iCurrentData.MaxLength();
				iSocket->RecvOneOrMore(iCurrentData,0,iStatus,iLen);
				break;
		
			case EProcessingCGI:
				// Finnish it !!
				
				break;
			case EFinnished:
				break;
			default: // just in case.
				// Finnish it !!
				
				break;
			}
		


	}
	else
	{


		if (iStatus == KErrTimeOut)
		{
			switch (iConState)
			{
				case EReceivingRequest:
				case EGettingPostBody:
				case EProcessingCGI:
					TRAP(error,SendErrorMessageL(SC_REQUEST_TIMEOUT));
					break;
				case EFinnished:
					break;
				default:
					iConState = EFinnished;
					//ShuttingDownConnection(RSocket::EImmediate); // Abotive close.
					break;
			}
			if (iMiscData)
			{
				ShowMiscData();
				iStatusText = _L("***** TIME OUT TRIGGERED *****");
				NotifyStatus();
			}
		}
		else
		{
			switch (iConState)
			{
			case ESendingGet:
				iSocket->CancelSend();
				iFile.Close();
				iConState = EFinnished;
				break;
			case ESendingRange:
			case ERangeSent:
				iFile.Close();
				//lint -fallthrough // similar case in DoCancel so this is probably intentional fall-through

			case EWaitingRequest:
			case ESendingTrace:
				iSocket->CancelSend();
				break;
			
			case EPuttingURI:
				iSocket->CancelRecv();
				iFile.Close();
				ifsSession.Delete(iMessage.GetURI());
				break;
			
			case EReceivingRequest:
			case EGettingPostBody:
				iSocket->CancelRecv();
				break;
		
			case EProcessingCGI:
				thread.Close();
				delete cgi;
				library.Close();
				break;
			case EFinnished:
				break;
			default: // just in case.
				iSocket->CancelAll();
				break;
			}
		
			if (iMiscData)
			{
				TBuf<512> errormsg;
				ShowMiscData();
				CEikonEnv::Static()->GetErrorText(errormsg,iStatus.Int());
				iStatusText.Format(_L("***** ERROR IN THE CONNECTION (%S) (Error Code: %d) *****"),&errormsg,iStatus.Int());
				NotifyStatus();
			}

			iConState = EFinnished;
		}					

	}
	
	if (iConState != EFinnished)
	{
		SetActive();
		iTimer->After(TimeOutTime);
	}
	else
	{
		if (iMiscData)
		{
			ShowMiscData();
			iStatusText.Format(_L("***************** CONNECTION CLOSED !! ********************\n\n"));
			NotifyStatus();
		}			
		iWebServer->DestroyConnectionL(this);
	}
}
