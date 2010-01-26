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
// ws_cgi.h - CGI library main header
// CExecCGI ver 1.0
// Abstract class for executing CGIs.
// Last Modification 8 - 2 - 2000
//



/**
 @internalComponent
*/

#ifndef __WS_CGI_H
#define __WS_CGI_H

#include <e32base.h>
#include <e32std.h>
#include <f32file.h> // Provisional

#define MAX_CGI_BUFFER 256
#define MAX_ARGV 256
#define CGIVersion _L("CGI/1.1")

const TUint KCExecCGIValue = 0x10000885;
const TUid KCExecCGIUid = { KCExecCGIValue };


class TCgiEnv //May be it should be implemnted in the cgi header !
{
public:
	void Set(const TCgiEnv& aCgiEnv);
public:
	TBuf<MAX_CGI_BUFFER> iAuthType;
	TBuf<MAX_CGI_BUFFER> iContentLength;
	TBuf<MAX_CGI_BUFFER> iContentType;
	TBuf<MAX_CGI_BUFFER> iGatewayInterface;
	TBuf<MAX_CGI_BUFFER> iPathInfo;
	TBuf<MAX_CGI_BUFFER> iPathTranslated;
	TBuf<MAX_CGI_BUFFER> iQueryString;
	TBuf<MAX_CGI_BUFFER> iRemoteAddr;
	TBuf<MAX_CGI_BUFFER> iRemoteHost;
	TBuf<MAX_CGI_BUFFER> iRemoteIdent;
	TBuf<MAX_CGI_BUFFER> iRemoteUser;
	TBuf<MAX_CGI_BUFFER> iRequestMethod;
	TBuf<MAX_CGI_BUFFER> iScriptName;
	TBuf<MAX_CGI_BUFFER> iServerName;
	TBuf<MAX_CGI_BUFFER> iServerPort;
	TBuf<MAX_CGI_BUFFER> iServerProtocol;
	TBuf<MAX_CGI_BUFFER> iServerSoftware;

};



class CCommandArg: public CBase
{
public:
	IMPORT_C CCommandArg();
	IMPORT_C ~CCommandArg();
	void SetL(const CCommandArg& aComArg);
public:
	HBufC* iArgv[MAX_ARGV]; 
	TInt iArgc;
};

class CExecCGI: public CBase
{
public:
	// For the WebServer
	IMPORT_C void ConstructL(const TFileName& aFileIn, const TFileName& aFileOut, const CCommandArg& aComArg, const TCgiEnv& aEnv); //Provisional
	IMPORT_C void StartCGI(); // Provisional
	IMPORT_C void CloseCGI(); // Provisional
public:
	virtual void ExecuteCGI() = 0;
protected:// For the CGI.
	IMPORT_C TInt Read(TDes8& aBuffer) const;
	IMPORT_C TInt Write(const TDesC8& aBuffer);
protected:
	CCommandArg iComArg;
	TCgiEnv iEnv;
	// Provisional.
	TFileName iFileIn;
	TFileName iFileOut;
	RFs iFileSession;
	RFile iFileInput;
	RFile iFileOutput;
};


#endif
