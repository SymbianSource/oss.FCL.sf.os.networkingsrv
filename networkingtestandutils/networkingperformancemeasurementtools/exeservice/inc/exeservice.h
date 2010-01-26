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

#ifndef EXE_SERVICE_H
#define EXE_SERVICE_H

#include "CService.h"
#include "CInstanceContainer.h"
#include "CJobProcess.h"

class CExeService : public CService
{
public:
	virtual ~CExeService();

private:
	int RunCommand(const CCall& aCall);

public:
	static CExeService* Instance();
    bool Setup();
	bool EraseExtension(std::string &aFileName);

private:
    CInstanceContainer<CJobProcess> iProcesses;
	std::string iOutputDir;
	std::vector<std::string> iProcList;
	static CExeService* iInstance;
	bool iSetup;
};

#endif // EXE_SERVICE_H
