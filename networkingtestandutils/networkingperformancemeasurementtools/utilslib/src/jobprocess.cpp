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


#include "CJobProcess.h"
using namespace std;


#if (_WIN32_WINNT < 0x0500)

// Visual C++ 6.0 doesn't contain constants and function declarations for JobObject.
// It is part of SDK update. To remove this dependency, here are all required
// definitions for VC 6.0

extern "C" {

#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE          0x00002000
#define JobObjectExtendedLimitInformation			((_JOBOBJECTINFOCLASS) 9)

WINBASEAPI
HANDLE
WINAPI
CreateJobObjectA(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCSTR lpName
    );
WINBASEAPI
HANDLE
WINAPI
CreateJobObjectW(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCWSTR lpName
    );
#ifdef UNICODE
#define CreateJobObject  CreateJobObjectW
#else
#define CreateJobObject  CreateJobObjectA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
AssignProcessToJobObject(
    HANDLE hJob,
    HANDLE hProcess
    );

WINBASEAPI
BOOL
WINAPI
SetInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength
    );

WINBASEAPI
BOOL
WINAPI
TerminateJobObject(
    HANDLE hJob,
    UINT uExitCode
    );

typedef struct _IO_COUNTERS {
    ULONGLONG  ReadOperationCount;
    ULONGLONG  WriteOperationCount;
    ULONGLONG  OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS;
typedef IO_COUNTERS *PIO_COUNTERS;

typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
    IO_COUNTERS IoInfo;
    SIZE_T ProcessMemoryLimit;
    SIZE_T JobMemoryLimit;
    SIZE_T PeakProcessMemoryUsed;
    SIZE_T PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION, *PJOBOBJECT_EXTENDED_LIMIT_INFORMATION;

} // extern "C"

#endif // _WIN32_WINNT < 0x0500

//

// ctor
CJobProcess::CJobProcess(const string &aCmd, const string &aArgs, const string &aTitle)
	: iCmd(aCmd), iArgs(aArgs), iTitle(aTitle), iProcessHandle(NULL), iJobHandle(NULL)
{
}

// dtor
CJobProcess::~CJobProcess()
{
	Stop();
}

// start a process
BOOL CJobProcess::Start()
{
	BOOL result = TRUE;

	// stop previous process instance
	Stop();

	// create job
	iJobHandle = CreateJobObject(NULL, NULL);
	if (!iJobHandle)
	{
        printf("Failed to create job object [%ld]\n", GetLastError());
		return FALSE;
	}
	// causes all processes associated with the job to terminate when the last handle to the job is closed
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	SetInformationJobObject(iJobHandle, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	// create process
    STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb         = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_MINIMIZE;
    if (iTitle.length())
        si.lpTitle = (char *) iTitle.c_str();
    string cmdLine = iCmd + " " + iArgs;
    if (!CreateProcess(NULL, (char *) cmdLine.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, &si, &pi))
	{
		printf("Failed to create process [%ld]\n", GetLastError());
		result = FALSE;
	}

	if (result)
	{
		// process is now started
		iProcessHandle = pi.hProcess;
		pi.hProcess = NULL;

		// add the process to the job object
		if (!AssignProcessToJobObject(iJobHandle, iProcessHandle))
		{
			printf("Failed to assign process to job [%ld]\n", GetLastError());
		}
		// start our suspended process
		ResumeThread(pi.hThread);
	}

	if (pi.hProcess)
		CloseHandle(pi.hProcess);
	if (pi.hThread)
		CloseHandle(pi.hThread);
	return result;
}

// stop process
void CJobProcess::Stop()
{
    UINT result = NOERROR;
    if (iProcessHandle)
    {
        result = GetExitCode();
		if (result == STILL_ACTIVE)
		{
	        if (!TerminateJobObject(iJobHandle, STATUS_USER_APC))
		        printf("Failed to terminate job [%ld]\n", GetLastError());
		}
    }

	if (iProcessHandle)
	{
	    CloseHandle(iProcessHandle);
		iProcessHandle = NULL;
	}
	if (iJobHandle)
	{
		CloseHandle(iJobHandle);
		iJobHandle = NULL;
	}
}

// retrieve process's exit result
DWORD CJobProcess::GetExitCode()
{
    DWORD result;
    if (!GetExitCodeProcess(iProcessHandle, &result))
	{
		printf("Failed to get exit code of process [%ld]\n", GetLastError());
		result = UINT(-1);
	}
    return result;
}

