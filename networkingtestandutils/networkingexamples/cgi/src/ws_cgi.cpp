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
// ws_cgi.cpp - CGI library main module
//

#include "ws_cgi.h"

// Entry point.
#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
{
	return (KErrNone);
}
#endif


//******************************************* CCommandArg Functions ***********************************************
EXPORT_C CCommandArg::CCommandArg()
{
	iArgc = 0;
}

EXPORT_C CCommandArg::~CCommandArg()
{
	for (TInt i=0; i<iArgc;i++)
	{
		delete iArgv[i];
	}
	iArgc = 0;

}

void CCommandArg::SetL(const CCommandArg& aComArg)
{
	iArgc = aComArg.iArgc;

	for (TInt i=0; i<iArgc;i++)
	{
		iArgv[i] = aComArg.iArgv[i]->Des().AllocL();
	}
}

//******************************************* TCgiEnv Functions ***********************************************
void TCgiEnv::Set(const TCgiEnv& aCgiEnv)
{
	iAuthType.Copy(aCgiEnv.iAuthType);
	iContentLength.Copy(aCgiEnv.iContentLength);
	iContentType.Copy(aCgiEnv.iContentType);
	iGatewayInterface.Copy(aCgiEnv.iGatewayInterface);
	iPathInfo.Copy(aCgiEnv.iPathInfo);
	iPathTranslated.Copy(aCgiEnv.iPathTranslated);
	iQueryString.Copy(aCgiEnv.iQueryString);
	iRemoteAddr.Copy(aCgiEnv.iRemoteAddr);
	iRemoteHost.Copy(aCgiEnv.iRemoteHost);
	iRemoteIdent.Copy(aCgiEnv.iRemoteIdent);
	iRemoteUser.Copy(aCgiEnv.iRemoteUser);
	iRequestMethod.Copy(aCgiEnv.iRequestMethod);
	iScriptName.Copy(aCgiEnv.iScriptName);
	iServerName.Copy(aCgiEnv.iServerName);
	iServerPort.Copy(aCgiEnv.iServerPort);
	iServerProtocol.Copy(aCgiEnv.iServerProtocol);
	iServerSoftware.Copy(aCgiEnv.iServerSoftware);
}

//************************************************  CExecCGI functions **********************************************
EXPORT_C void CExecCGI::ConstructL(const TFileName& aFileIn, const TFileName& aFileOut, const CCommandArg& aComArg, const TCgiEnv& aEnv)
{
	iFileIn = aFileIn;
	iFileOut = aFileOut;
	iComArg.SetL(aComArg);
	iEnv.Set(aEnv);
}

EXPORT_C void CExecCGI::StartCGI()
{
	iFileSession.Connect();
	iFileOutput.Replace(iFileSession,iFileOut,EFileStream);
	iFileInput.Open(iFileSession,iFileIn,EFileStream);
}

EXPORT_C TInt CExecCGI::Read(TDes8& aBuffer) const
{
	return iFileInput.Read(aBuffer);
}

EXPORT_C TInt CExecCGI::Write(const TDesC8& aBuffer)
{
	return iFileOutput.Write(aBuffer);
}

EXPORT_C void CExecCGI::CloseCGI()
{
	iFileInput.Close();
	iFileOutput.Close();
	iFileSession.Close();
}
