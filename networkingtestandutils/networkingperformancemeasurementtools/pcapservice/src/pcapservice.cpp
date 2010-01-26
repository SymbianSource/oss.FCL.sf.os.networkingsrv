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
// UCC service to launch wireshark on remote PC.
// This allows controlled packet capture during each send receive test
// run by Netperf. 
// 
//

/**
 @file
 @internalTechnology
*/

#include <string>
#include <stdio.h>
#include "pcapService.h"

using std::string;

#define CONSOLE_NAME "Console - "
#define SERVICE_NAME "PcapService"
#define PCAPSVC_CFG   "PcapService.cfg"

CService* Service() {return CPcapService::Instance();}

// static data storage
CPcapService* CPcapService::iInstance = NULL;

/*static*/ CPcapService* CPcapService::Instance()
{
	if (iInstance==NULL)
	{
		iInstance = new CPcapService;
	}
	return iInstance;
}

CPcapService::~CPcapService()
{
	iInstance = NULL;
}

bool CPcapService::Setup()
{
	// quick config file read..
	FILE* cfgin = fopen(PCAPSVC_CFG,"r");
	if (!cfgin)
	{
		printf("Error reading config file PcapService.cfg\n");
		return false;
	}

	char line[200];
	while (!feof(cfgin))
	{
		*line = '\0';
		fgets(line, 200, cfgin);
		char* eolCh = line + strlen(line);
		while (*--eolCh == '\r' || *eolCh == '\n') // strip end of line chars
			*eolCh='\0';

		if (line == strstr(line,"Device="))
		{
			iDefaultDevice = line+7;
			printf("Device: [%s]\n",iDefaultDevice.c_str());
		}
		else if (line == strstr(line, "WiresharkPath="))
		{
			iWiresharkPath = line+14;
			printf("Wireshark Path: [%s]\n", iWiresharkPath.c_str());
		}
		else
		{
			printf("Config line ignored: %s\n", line);
		}
	}

	// check read configuration
	if (iDefaultDevice.empty() || iWiresharkPath.empty())
	{
		printf("Not all required parameters found in config file PcapService.cfg\n");
		return false;
	}

	fclose(cfgin);
	return true;
}


// entry point for received commands
int CPcapService::RunCommand(const CCall &aCall)
{
	int result = NOERROR;
	aCall.Dump();

	// handle call
	int callId;
	if (aCall.CallID(callId))
	{
		switch (callId)
		{
		case 1: // "StartServer"
		{
			// get & check args
			string outputFileName = "out.pcap";
			if (!aCall.Get("outputFileName", outputFileName))
			{
				printf("missing or invalid parameters; \"cmd\", \"args\"");
				return ERR_MISSING_PARAMETER;
			}

			string Device = iDefaultDevice;
			aCall.Get("device", Device);

			// issue cmd
			char args[256];
			sprintf(args, "-i \"%s\" -w \"%s\" -F libpcap",  Device.c_str(), outputFileName.c_str());
			puts(args);
			CProcess *process = new CProcess(iWiresharkPath, args, "");
			if (!process)
				return ERR_CANNOT_CREATE_NEW_INSTANCE;

			bool isDefault = true;
			if (!iProcesses.Add(*process, isDefault))
				return ERR_INVALIDSTATE;

			if (!process->Start())
				return ERR_CANNOT_CREATE_NEW_INSTANCE;

			result = (int) process;
			break;
		}
		case 2: // "Stop"
		{
			// issue cmd - stop & destruct operations have been combined into 1 client side request (for sake of simplicity)
		   // iProcesses.Remove(aCall);
			iProcesses.Remove(false); // remove default
			break;
		} 
		default:
			result = ERR_INVALID_CALL;
		}
	}
	else
		result = ERR_INVALID_CALL;

	return result;
}

