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
// engine.cpp - name resolver core engine
//

#include "dnd.hrh"
#include "engine.h"
#include "listener.h"
#include "dnd_ini.h"
#include "inet6log.h"
#include <es_ini.h>
#include <timeout.h>
#include "in6_version.h"

//

MDemonEngine *MDemonEngine::NewL(MDemonMain &aMain)
	/**
	* Create the instance of CDndEngine.
	* There can only be one of these.
	* @param aMain The DLL/EXE "main" interface
	* @return The Engine interface
	*/
	{
	return new (ELeave) CDndEngine(aMain);
	}

//
CDndEngine::~CDndEngine()
	{
	//
	// Ordering is important... Do not close iSS before
	// all sockets associated with it have been closed!
	//
	iHostsFile.Close();

	delete iListener;
	delete iConfig;
	delete iTimer;

	iSocket.Close();
	iFS.Close();
	iSS.Close();
	}


TInt CDndEngine::GetIniValue(const TDesC &aSection, const TDesC &aName, TInt aDefault, TInt aMin, TInt aMax)
	/**
	* Read an integer value from the INI file.
	*
	* The INI is a text file (currently named by #DND_INI_DATA).
	* The used sections are [resolver] and [llmnr]
	*
	* @param aSection The INI section name
	* @param aName The INI variable name
	* @param aDefault The value returned if variable name not found or is out of range
	* @param aMin The min of the accepted range
	* @param aMax The max of the accepted range
	*/
	{
	LOG(_LIT(KFormat, "\t[%S] %S = %d"));
	LOG(_LIT(KFormatInv, "\t[%S] %S = %d is invalid"));

	TInt value;
	if (!FindVar(aSection, aName, value))
		value = aDefault;
	else if (value < aMin || value > aMax)
		{
		LOG(Log::Printf(KFormatInv, &aSection, &aName, value));
		value = aDefault;
		}
	LOG(Log::Printf(KFormat, &aSection, &aName, value));
	return value;
	}

void CDndEngine::GetIniAddress(const TDesC &aSection, const TDesC &aName, const TDesC &aDefault, TIp6Addr &aAddr)
	/**
	* Read an address value from the INI file.
	*
	* The INI is a text file (currently named by #DND_INI_DATA).
	* The currently used sections are [resolver] and [llmnr].
	*
	* The default is used if INI file does not specify address or the specified
	* address is not syntactically correct. If the default is also bad, the
	* aAddr will be unspecified.
	*
	* @param aSection The INI section name
	* @param aName The INI variable name
	* @param aDefault The default value for the variable (string format)
	* @retval aAddr The address
	*/
	{
	TInetAddr addr;
	TPtrC str;

	if (!FindVar(aSection, aName, str))
		str.Set(aDefault);
	const TInt value = addr.Input(str);
	// If the parameter is bad, then use the aDefault, but show the
	// parameter value and result. (If aDefault is bad, Input is done
	// twice to it, both fail and address will be unspecified.)
	if (value != KErrNone)
		(void)addr.Input(aDefault);

	if (addr.Family() != KAfInet6)
		addr.ConvertToV4Mapped();
	aAddr = addr.Ip6Address();
	LOG(_LIT(KFormatAddr, "\t[%S] %S = %S [err=%d]"));
	LOG(Log::Printf(KFormatAddr, &aSection, &aName, &str, value));
	}

void CDndEngine::ConstructL()
	/**
	* Construct the engine.
	*
	* Open file and socket server sessions, read configuration parameters
	* from [resolver] and [lmnr] sections, create the timeout
	* manager (MTimeoutManager) and listener (CDndListener) instances.
	*/
	{
	LOG(Log::Printf(_L("--- DND starting, version: [%S] ---"), &KInet6Version));
	//
	// Start Socket Reader activity
	//
	CheckResultL(_L("Active Scheduler"), CActiveScheduler::Current() == NULL);
	CheckResultL(_L("Connect to File server"), iFS.Connect());
	CheckResultL(_L("Connect to Socket server"), iSS.Connect());
	CheckResultL(_L("Opening socket for Get/SetOpt"), iSocket.Open(iSS,  KAfInet, KSockDatagram, KProtocolInetUdp));
	// The utility socket should be pretty much "invisible", so disable some things...
	(void)iSocket.SetOpt(KSoUserSocket, KSolInetIp, 0);
	(void)iSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0);
	// Failing to open hosts file is not a fatal error.
	(void)CheckResult(_L("Opening hosts file"), iHostsFile.Open());

	//
	// Setup Configuration Parameters
	//
	iParams.iRetries = GetIniValue(DND_INI_RESOLVER, DND_INI_RETRIES, KDndIni_Retries, 1, 255);
	iParams.iMinTime = GetIniValue(DND_INI_RESOLVER, DND_INI_MINTIME, KDndIni_MinTime, 1, KMaxTInt);
	iParams.iMaxTime = GetIniValue(DND_INI_RESOLVER, DND_INI_MAXTIME, KDndIni_MaxTime, 1, KMaxTInt);
	iParams.iSetupTime = GetIniValue(DND_INI_RESOLVER, DND_INI_SETUPTIME, KDndIni_SetupTime, 1, KMaxTInt);
	iParams.iSetupPoll = GetIniValue(DND_INI_RESOLVER, DND_INI_SETUPPOLL, KDndIni_SetupPoll, 1, KMaxTInt);
	iParams.iSprayMode = GetIniValue(DND_INI_RESOLVER, DND_INI_SPRAYMODE);
	iParams.iSkipNotFound = GetIniValue(DND_INI_RESOLVER, DND_INI_SKIPNOTFOUND);
	iParams.iQueryHack = GetIniValue(DND_INI_RESOLVER, DND_INI_QUERYHACK);
	iParams.iFlushOnConfig = GetIniValue(DND_INI_RESOLVER, DND_INI_FLUSHONCONFIG);
	iParams.iQueryOrder = GetIniValue(DND_INI_RESOLVER, DND_INI_QUERYORDER, KDndIni_QueryOrder, 0, 5);
#ifdef _LOG
		{
		if (iParams.iQueryOrder & KQueryOrder_OneQuery)
			{
			_LIT(a, "A");
			_LIT(aaaa, "AAAA");
			Log::Printf(_L("\t\t= (only %S)"),
				(iParams.iQueryOrder & KQueryOrder_PreferA) ? &a : &aaaa);
			}
		else
			{
			_LIT(sequentially, "sequentially");
			_LIT(parallel, "in parallel");
			_LIT(aaaa_a, "AAAA, A");
			_LIT(a_aaaa, "A, AAAA");
			Log::Printf(_L("\t\t= (%S %S)"),
				(iParams.iQueryOrder & KQueryOrder_PreferA) ? &a_aaaa : &aaaa_a,
				(iParams.iQueryOrder & KQueryOrder_Parallel) ? &parallel : &sequentially);
			}
		}
#endif
	iParams.iEDNS0 = GetIniValue(DND_INI_RESOLVER, DND_INI_EDNS0, KDndIni_EDNS0, KDnsMinHeader, 65000);

#ifdef LLMNR_ENABLED
	iParams.iLlmnrLlOnly = GetIniValue(DND_INI_LLMNR, LLMNR_INI_LLONLY, KLlmnrIni_LlOnly);
	iParams.iLlmnrPort = GetIniValue(DND_INI_LLMNR, LLMNR_INI_PORT, KLlmnrIni_Port, 0, 65535);
	iParams.iLlmnrRetries = GetIniValue(DND_INI_LLMNR, LLMNR_INI_RETRIES, KLlmnrIni_Retries, 1, 255);
	iParams.iLlmnrMinTime = GetIniValue(DND_INI_LLMNR, LLMNR_INI_MINTIME, KLlmnrIni_MinTime, 0, KMaxTInt);
	iParams.iLlmnrMaxTime = GetIniValue(DND_INI_LLMNR, LLMNR_INI_MAXTIME, KLlmnrIni_MaxTime, 1, KMaxTInt);
	iParams.iLlmnrHoplimit = GetIniValue(DND_INI_LLMNR, LLMNR_INI_HOPLIMIT, KLlmnrIni_Hoplimit, -1, 255);

	// Note: both addresses are stored as IPv6 (TIp6Addr)
	GetIniAddress(DND_INI_LLMNR, LLMNR_INI_IPV4ADDR, KLlmnrIni_Ipv4Addr, iParams.iLlmnrIpv4);
	GetIniAddress(DND_INI_LLMNR, LLMNR_INI_IPV6ADDR, KLlmnrIni_Ipv6Addr, iParams.iLlmnrIpv6);
#endif

	// Setup the timeout manager
	iTimer = TimeoutFactory::NewL();
	// Setup the "real" DNS resolver main
	iListener = MDndListener::NewL(*this);
	UnloadConfigurationFile();
	}

void CDndEngine::HandleCommandL(TInt aCommand)
	{
	switch(aCommand)
		{
		case EDndDump:
			if (iListener)
				iListener->HandleCommandL(aCommand);
			else
				iMain.Write(_L("Listener not active.\n"));
			break;

		default:
			// No own commands, pass the buck to other modules
			if (iListener)
				iListener->HandleCommandL(aCommand);
		}
	}

// CDndEngine::CheckResult
// ***********************
TInt CDndEngine::CheckResult(const TDesC &aText, TInt aResult)
	/**
	* If the result parameter is not KErrNone, output the
	* error message (with error number).
	*
	* @param aText	message to be output in case of error
	* @param aResult to be tested
	* @return aResult
	*/
	{
	_LIT(KFormat, "%S Error=%d");

	if (aResult != KErrNone)
		ShowTextf(KFormat, &aText, aResult);
	return aResult;
	}

// CDndEngine::CheckResultL
// ************************
void CDndEngine::CheckResultL(const TDesC &aText, TInt aResult)
	/**
	* Output success or fail message, and Leave if the code is not
	* KErrNone.
	*
	* @param aText	message to be output in case of error
	* @param aResult to be tested
	*/
	{
	if (CheckResult(aText, aResult) != KErrNone)
		User::Leave(aResult);
	}

// CDndEngine::ShowText
// ********************
void CDndEngine::ShowText(const TDesC &aText)
	/**
	* Write a text message.
	*
	* @param aText	is the message
	*/
	{
	iMain.Write(aText);
	}

// CDndEngine::ShowTextf
// *********************
void CDndEngine::ShowTextf(TRefByValue<const TDesC> aFmt, ...)
	/**
	* Write a message using format string.
	*
	* @param aFmt	is the format string
	*/
	{
	//coverity[var_decl];
	VA_LIST list;
	VA_START(list,aFmt);
	//coverity[uninit_use_in_call];
	iMain.WriteList(aFmt, list);
	}

// CDndEngine::LoadConfigurationFile
// *********************************
TBool CDndEngine::LoadConfigurationFile()
	/**
	* Load the configuration file.
	* Load the DND configuration file (INI file) into memory (iConfig),
	* if not already loaded. Only try loading once, if attempt
	* fails.
	* The CDndEngine::FindVar functions call this implicitly. There is
	* no need to call this explicitly.
	*
	* @return
	* @li	TRUE, if configuration availabe (iConfig != NULL)
	* @li	FALSE, if configuration file is not available
	*/
	{
	if (iConfig)
		return TRUE;	// Already loaded!
	// if iConfigErr != 0 (KErrNone), then an attempt for
	// loading configuration has been made and failed,
	// assume it never will succeed and avoid further
	// attemps on each FindVar...
	if (iConfigErr)
		return FALSE;
	LOG(Log::Printf(_L("CDndEngine::LoadConfigurationFile(): %S"), &DND_INI_DATA));
	TRAP(iConfigErr, iConfig = CESockIniData::NewL(DND_INI_DATA));
	return (iConfig != NULL);
	}

// CDndEngine::UnloadConfigurationFile
// ***********************************
void CDndEngine::UnloadConfigurationFile()
	/**
	* Free the configuration data.
	* The configuration information (INI file in iConfig) can be unloaded, if it is
	* assumed that no further need for configuration is required.
	*/
	{
	delete iConfig;
	iConfig = NULL;
	iConfigErr = 0;
	}

//
// Access to the configuration file
TBool CDndEngine::FindVar(const TDesC &aSection, const TDesC &aVarName, TPtrC &aResult)
	/**
	* @param aSection	the section name in the configution
	* @param aVarName	the variable name within section
	* @retval aResult	variable value, if found
	* @return
	* @li TRUE, if variable found
	* @li FALSE, if not found
	*/
	{
	if (LoadConfigurationFile())
		{
		ASSERT(iConfig);	// <-- lint gag
		return iConfig->FindVar(aSection, aVarName, aResult);
		}
	return FALSE;
	}

TBool CDndEngine::FindVar(const TDesC &aSection, const TDesC &aVarName, TInt &aResult)
	/**
	* @param aSection	the section name in the configution
	* @param aVarName	the variable name within section
	* @retval aResult	variable value, if found
	* @return
	* @li TRUE, if variable found
	* @li FALSE, if not found
	*/
	{
	if (LoadConfigurationFile())
		{
		ASSERT(iConfig);	// <-- lint gag
		return iConfig->FindVar(aSection, aVarName, aResult);
		}
	return FALSE;
	}
