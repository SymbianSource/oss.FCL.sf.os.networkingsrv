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
// llmnrconf.cpp - LLMNR (linklocal multicast name resolver) configuration
// file parser module
//

#ifdef LLMNR_ENABLED
#include <f32file.h>	// RFs
#include <in_sock.h>	// TInetAddr
#include <es_ini.h>
#include "dnd_ini.h"
#include "engine.h"
#include "llmnrresponder.h"	// this
#include "inet6log.h"

CLlmnrConf::CLlmnrConf(CDndEngine &aControl)
: iControl(aControl)
	{
	}

CLlmnrConf::~CLlmnrConf()
	{
	delete iHostList;
	}

void CLlmnrConf::ConstructL()
	{
	iLlmnrEntries = iControl.GetIniValue(DND_INI_LLMNR, LLMNR_INI_ENTRIES, 0, 0, KLlmnrMaxEnabled);
	iNotifyTime = iControl.GetIniValue(DND_INI_LLMNR, LLMNR_INI_NOTIFYTIME, KLlmnrIni_NotifyTime, 1, KMaxTInt);
	iRescans = iControl.GetIniValue(DND_INI_LLMNR, LLMNR_INI_RESCANS, KLlmnrIni_Rescans, 1, 255);
	iTTL = iControl.GetIniValue(DND_INI_LLMNR, LLMNR_INI_TTL, KLlmnrIni_Ttl, 1, KMaxTInt);
	}

void CLlmnrConf::GetHostNamesL()
	{
	delete iHostList;
	iHostList = NULL;
	// Aways allocate the array "skeleton" to keep things simple.
	iHostList = new (ELeave) CArrayFixFlat<THostNameEntry>(iLlmnrEntries < 1 ? 1 : iLlmnrEntries);

	for (TUint i = 1; i <= iLlmnrEntries; ++i)
		{
		THostNameEntry entry;

		// Borrow he iName field of the entry
		entry.iName = LLMNR_INI_ENTRY;
		entry.iName.AppendNum(i);
		TPtrC entrystr;
		if(!iControl.FindVar(DND_INI_LLMNR, entry.iName, entrystr))
			{
			// This log message should probably be logged in release also
			LOG(Log::Printf(_L("DND LMNR: %S is missing [%S] %S%d\r\n"), &DND_INI_DATA, &DND_INI_LLMNR, &LLMNR_INI_ENTRY, (TInt)i));
			continue;
			}

		TLex wordLex(entrystr);

		// Get hostname
		wordLex.Mark();
		while (!wordLex.Eos() && wordLex.Peek() != ',')
			wordLex.Inc();

		TInt err = 0;
		for (;;)
			{
			const TInt N = wordLex.MarkedToken().Length();
			if (N <= 0 || N > entry.iName.MaxLength())
				{
				err = 1;
				break;	// invalid hostname (either not given or too long)
				}
			entry.iName = wordLex.MarkedToken();

			// By default, entry is valid for ...
			entry.iVersion = EIPany;		// ...both IPv4 and IPv6
			entry.iIfName.SetLength(0);		// ...any interface
			if(wordLex.Eos())
				break;

			// Get interface name
			wordLex.Inc();
			if(wordLex.Peek() != ',') // not ",," after hostname
				{
				wordLex.Mark();
				while (!wordLex.Eos() && wordLex.Peek() != ',')
					wordLex.Inc();
				if (wordLex.MarkedToken().Length() > entry.iIfName.MaxLength())
					{
					err = 2;
					break;
					}
				entry.iIfName = wordLex.MarkedToken();
				}

			if(wordLex.Eos())
				break;

			// Get ip-version
			wordLex.Inc();
			if(wordLex.Eos()) // Eos after ','
				break;

			TInt n;
			if((wordLex.Val(n) != KErrNone) || (n != 0 && n != 4 && n != 6))
				err = 1;
			else
				entry.iVersion = (TIpVer)n;
			break;
			}
		if (err)
			{
			LOG(Log::Printf(_L("DND LMNR: %S has invalid [%S] %S%d= %S [ignored]\r\n"), &DND_INI_DATA, &DND_INI_LLMNR, &LLMNR_INI_ENTRY, (TInt)i, &entrystr));
			}
		else
			iHostList->AppendL(entry);
		}
	}

#endif
