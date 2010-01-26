// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// CProcess class definition
// 
//

#include "CProcess.h"
using namespace std;

// ctor
CProcess::CProcess(const string &aCmd, const string &aArgs, const string &aTitle)
    : iCmd(aCmd), iArgs(aArgs), iTitle(aTitle), iProcessHandle(NULL)
{ }

// dtor
CProcess::~CProcess()
{
	Stop();
}

// start a process
BOOL CProcess::Start()
{
	BOOL result = TRUE;

	// stop previous process instance
	Stop();

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

    STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb         = sizeof(si);
    if (iTitle.length())
        si.lpTitle = (char *) iTitle.c_str();
    string cmdLine = iCmd + " " + iArgs;
    if (!CreateProcess(NULL, (char *) cmdLine.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		printf("Failed to create process [%ld]\n", GetLastError());
		result = FALSE;
	}

	if (result)
	{
		iProcessHandle = pi.hProcess;
		pi.hProcess = NULL;
	}

	if (pi.hProcess)
		CloseHandle(pi.hProcess);
	if (pi.hThread)
		CloseHandle(pi.hThread);

	return result;
}

// stop process
void CProcess::Stop()
{
	if (!iProcessHandle || GetExitCode() != STILL_ACTIVE)
		return;

	if (!TerminateProcess(iProcessHandle, (UINT) -1))
	{
		printf("Failed to terminate process [%ld]\n", GetLastError());
	}

	CloseHandle(iProcessHandle);
	iProcessHandle = NULL;
}

DWORD CProcess::GetExitCode()
{
    DWORD result;
    if (!GetExitCodeProcess(iProcessHandle, &result))
	{
		printf("Failed to get exit code of process [%ld]\n", GetLastError());
		result = UINT(-1);
	}
    return result;
}
