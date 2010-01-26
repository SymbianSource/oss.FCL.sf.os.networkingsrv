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
// process.h
// CProcess class decleration
// 
//

#ifndef PROCESS_H
#define PROCESS_H

#include <windows.h>
#include <string>

class CProcess
{
public:
	CProcess(const std::string &aCmd, const std::string &aArgs = "", const std::string &aTitle = "");
	~CProcess();

	BOOL  Start();
	void  Stop();
	BOOL  IsRunning();
	DWORD GetExitCode();

private:
	CProcess operator=(CProcess &) {};

private:
	const std::string iCmd, iArgs, iTitle;
	HANDLE iProcessHandle;
};


#endif // PROCESS_H
