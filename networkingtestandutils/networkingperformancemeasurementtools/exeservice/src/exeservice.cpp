// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// UCC service allowing TEF scripts on a Symbian OS device to run 
// binaries on the PC side. Launched with the allowed binaries in an argument,
// to prevent a rampant entity on the network invoking bad things.
// 
//

/**
 @file
 @internalTechnology
*/

#include "exeService.h"
#include <sys/stat.h>

using std::string;

string RemoveExtenstion(const string ext);

#define CONSOLE_NAME "Console - "
#define SERVICE_NAME "ExeService"

CService* Service() {return CExeService::Instance();}

// static data storage
CExeService* CExeService::iInstance = NULL;

CExeService* CExeService::Instance()
{
	if (iInstance == NULL)
	{
		iInstance = new CExeService();
		iInstance->iSetup = true; // needs setup
	}
	return iInstance;
}

CExeService::~CExeService()
{
	iInstance = NULL;
}

bool CExeService::Setup()
{
	if (!iSetup)
		return true;

	for (int i = 1; i < __argc; i++)
	{
		// process only --[arg] (ignore others)
		if (strncmp(__argv[i], "--", 2) != 0)
			continue;

		if (_stricmp(__argv[i], "--outputdir") == 0)
		{
			iOutputDir = __argv[++i];
			if (iOutputDir[iOutputDir.size() - 1] == '\\')
			{
				std::string::iterator it = iOutputDir.end();
				iOutputDir.erase(--it);
			}

			struct _stat s;
			if (_stat(iOutputDir.c_str(), &s) || !(s.st_mode & _S_IFDIR))
			{
				printf("Target output directory is not accessible\n");
				return false;
			}

			iOutputDir += "\\";
		}
		else if (_stricmp(__argv[i], "--proclist") == 0)
		{
			char *str = __argv[++i];
			char *end;

			while (*str)
			{
				end = strchr(str, ',');
				std::string procname;
				if (end)
				{
					procname.assign(str, end - str);
					str = end + 1;
				}
				else
				{
					procname = str;
					str += procname.length();
				}

				if (!EraseExtension(procname))
				{
					printf("\"%s\" extension is not allowed\n", procname.c_str());
					return false;
				}
				iProcList.push_back(procname);
			}
		}
		else
			printf("Unknown argument \"%s\" ignored\n", __argv[i]);
	}

	if (iProcList.size() == 0)
	{
		printf("At least one allowed process name has to be specified.\nStopped\n");
		return false;
	}

	// setup completed
	iSetup = false;
	return true;
}

bool CExeService::EraseExtension(std::string &aFileName)
{
	size_t pos = aFileName.rfind(".");
	if (pos == string::npos)
		return true;
	if (_stricmp(aFileName.c_str() + pos, ".exe") != 0)
		return false;
	aFileName.erase(pos);
	return true;
}

// entry point for received commands (RPC)
// - Possible fututre enhancement:
//    add multi-threaded command handling support here --> such that long running operations don't block commands from other sessions
int CExeService::RunCommand(const CCall &aCall)
{
	int result = NOERROR;
	printf("\nreceived command (RPC)...\n");
	aCall.Dump();

	// handle call
	int callId;
	if (aCall.CallID(callId))
	{
		switch (callId)
		{
		case 1: // "Start"
		{
			// get & check args
			string cmd, args;
			if (!aCall.Get("cmd", cmd) || !aCall.Get("args", args))
			{
				printf("missing or invalid parameters; \"cmd\", \"args\"");
				return ERR_MISSING_PARAMETER;
			}

			// check cmd extension
			if (!EraseExtension(cmd))
			{
				printf("binary extension is not allowed");
				return ERR_INVALID_CALL;
			}

			// check if given process is allowed to start
			int i;
			bool allowed = false;
			for (i = 0; i < iProcList.size(); ++i)
			{
				if (_stricmp(cmd.c_str(), iProcList[i].c_str()) == 0)
				{
					allowed = true;
					break;
				}
			}
			if (!allowed)
			{
				printf("binary '%s' not allowed; restart exeservice with '%s' as an argument.", cmd.c_str(), cmd.c_str());
				return ERR_INVALID_CALL;
			}

			// here are optional arguments
			bool isDefault = false;
			aCall.Get("isDefault", isDefault);
			std::string output;
			aCall.Get("output", output);

			// update command line if capturing is enabled
			if (output.length() > 0)
			{
				// accept only file name
				if (output.find("\\") != string::npos || output.find("/") != string::npos)
				{
					printf("only file name (without path) is accepted as output");
					return ERR_INVALIDARG;
				}

				args = std::string("/c \"") + cmd + std::string(" ") + args + std::string(" > ") + iOutputDir + output + std::string("\"");
				cmd = "cmd";
			}
        
			// issue cmd - construct & start operations have been combined into 1 client side request (for sake of simplicity)
			CJobProcess *process = new CJobProcess(cmd, args, CONSOLE_NAME SERVICE_NAME);
			if (!process)
				return ERR_CANNOT_CREATE_NEW_INSTANCE;

			if (!iProcesses.Add(*process, isDefault))
			{
				delete process;
				return ERR_INVALIDSTATE;
			}

			if (!process->Start())
				return ERR_CANNOT_CREATE_NEW_INSTANCE;

			result = (int) process;
			break;
		}
		case 2: // "Stop"
		{
			// issue cmd - stop & destruct operations have been combined into 1 client side request (for sake of simplicity)
			if (!iProcesses.Remove(aCall))
				return ERR_INVALIDSTATE;
			break;
		} 
		default:
			printf("callId=%d is unsupported", callId);
			result = ERR_INVALID_CALL;
		}
	}
	else
	{
		printf("can't get callId");
		result = ERR_INVALID_CALL;
	}

	return result;
}

