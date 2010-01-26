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

#ifndef PCAP_SERVICE_H
#define PCAP_SERVICE_H

#include "CService.h"
#include "CInstanceContainer.h"
#include "CProcess.h"
#include <vector>

class CPcapService : public CService
	{
	public:
		virtual ~CPcapService();

	public:
		virtual bool Setup();
		static CPcapService* Instance();
	private:
		int RunCommand(const CCall& aCall);
        
        CInstanceContainer<CProcess> iProcesses;
		std::string iDefaultDevice;
		std::string iWiresharkPath;

		static CPcapService* iInstance;
	};

#endif // PCAP_SERVICE_H
