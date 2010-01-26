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
// jobprocess.h
// CProcess class decleration
// 
//

#ifndef JOBPROCESS_H
#define JOBPROCESS_H

#include <windows.h>
#include <string>

// manage a process (aka executable)
// - all methods (except dtor) are capable of throwing
class CJobProcess
{
public:
	CJobProcess(const std::string &aCmd, const std::string &aArgs = "", const std::string &aTitle = "");
	~CJobProcess();

	BOOL Start();
	void Stop();
	DWORD GetExitCode();

private:
	CJobProcess operator=(CJobProcess &) {};

private:
	const std::string iCmd, iArgs, iTitle;
	HANDLE iJobHandle;
	HANDLE iProcessHandle;
};


#endif // JOBPROCESS_H
