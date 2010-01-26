// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains the implementation of all the commands, which is used by the script file.
//



/**
 @file
 @internalTechnology
*/

#include "te_punycodeconvertertestwrapper.h"
#include <e32base.h>
#include <utf.h>
#include <in_sock.h>
#include <es_sock.h>
#include <commdbconnpref.h>
#include <networking/dnd_err.h>
#include <punycodeconverter.h>

// Commands
_LIT(KOpenResolver,    	"OpenResolver");
_LIT(KCloseResolver,	"CloseResolver");
_LIT(KEnableIdnSupport,	"EnableIdnSupport");
_LIT(KDisableIdnSupport,"DisableIdnSupport");
_LIT(KResolveName,      "ResolveName");	
_LIT(KResolveAddress,   "ResolveAddress");
_LIT(KPunyCodeToIdn,   	"PunyCodeToIdn");
_LIT(KIdnToPunyCode,   	"IdnToPunyCode");
_LIT(KCapTest,   		"CapTest");
_LIT(KSurrogatePair,	"SurrogatePair");

// Config file
_LIT(KInputFromConfig,	"InputFromConfig");
_LIT(KInput,   			"Input");
_LIT(KExpect,   		"Expect");
_LIT(KPunyCodeSign,   	"xn--");
_LIT(KInputHighByte,		"InputH"); // High order byte
_LIT(KInputLowByte,		"InputL"); // Low order byte
_LIT(KNameDefault, 		"default");
_LIT(KDestAddr, 		"DestAddr");

//Test code
_LIT(KCommand, 			"aCommand = %S");
_LIT(KSection, 			"aSection = %S");
_LIT(KAsyncErrorIndex, 	"aAsyncErrorIndex = %D");

//Test code error
_LIT(KInvldHostRslverState, "Host resolver state is invalid");
_LIT(KHostResolverOpenFail, "RHostResolver Open() failed with error: %d");
_LIT(KSockServConnFail, "SockServ Connect() failed with error: %d");
_LIT(KHostRslvOrSockServFail, "HostResolver ResolveName() failed with error: %d or SockServ error: %d ");
_LIT(KConversionFail, "Conversion failed with error: %d");
_LIT(KMatchFail,"Match failed, with error : %d");
_LIT(KNotAPunyCodeName, "Received Domain Name is not an encoded Punycode name");
_LIT(KInetInputFail, "TInetAddr Input failed with error: %d");

//Test code Information
_LIT(KHostRslvReturn, "HostResolver returned: %d");

//DND name length.
const TInt KNameLength=0xff;

/**
Type used to pass value to the punycode converter library

@internalTechnology
*/
typedef TBuf<KNameLength> TInputName;

/**
Constructor.

@internalTechnology
*/
CPunycodeConverterTestWrapper::CPunycodeConverterTestWrapper()
	{
	}

/**
Destructor.

@internalTechnology
*/
CPunycodeConverterTestWrapper::~CPunycodeConverterTestWrapper()
	{
	if(iHostResolver)
		{
		delete iHostResolver;
		}
	if(iSockServ)
		{
		delete iSockServ;
		}
	}

/**
Function to instantiate TestWrapper.

@return Returns constructed TestWrapper instance pointer

@internalTechnology
*/
CPunycodeConverterTestWrapper* CPunycodeConverterTestWrapper::NewL()
	{
	CPunycodeConverterTestWrapper*	ret = new (ELeave) CPunycodeConverterTestWrapper();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;	
	}

/**
Second level constructor, constructs TestWrapper instance.

@internalTechnology
*/
void CPunycodeConverterTestWrapper::ConstructL()
	{
	iHostResolver = new (ELeave) RHostResolver;
	iSockServ = new (ELeave) RSocketServ;
	}

/**
Function to map the input command to respective function.

@return - True Upon successfull command to Function name mapping otherwise False
@param aCommand Function name has to be called
@param aSection INI file paramenter section name
@param aAsyncErrorIndex Error index
@see Refer the script file COMMAND section.

@internalTechnology
*/
TBool CPunycodeConverterTestWrapper::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	TBool ret = ETrue;
	
	// Print out the parameters for debugging
	INFO_PRINTF2( KCommand, &aCommand );
	INFO_PRINTF2( KSection, &aSection );
	INFO_PRINTF2( KAsyncErrorIndex, aAsyncErrorIndex );
	
	if(KResolveName() == aCommand)
		{
		DoResolveName(aSection);
		}
	else if(KResolveAddress() == aCommand)
		{
		DoResolveAddress(aSection);
		}
	else if (KPunyCodeToIdn() == aCommand)
		{
		DoPunyCodeToIdn(aSection);
		}
	else if (KIdnToPunyCode() == aCommand)
		{
		DoIdnToPunyCode(aSection);
		}
	else if (KCapTest() == aCommand)
		{
		DoCapTest(aSection);
		}
	else if (KSurrogatePair() == aCommand)
		{
		DoSurrogatePair(aSection);
		}
	else if (KOpenResolver() == aCommand)
		{
		DoOpenResolver();
		}
	else if (KCloseResolver() == aCommand)
		{
		DoCloseResolver();
		}
	else if (KEnableIdnSupport() == aCommand)
		{
		DoEnableIdnSupport();
		}
	else if (KDisableIdnSupport() == aCommand)
		{
		DoDisableIdnSupport();
		}
	else
		{
		ret = EFalse;
		User::LeaveIfError(KErrNone); // just to suppress LeaveScan warning
		}
	return ret;
	}

/**
Function to enable IDN(International Domain Name) support for DNS.

@pre RHostResolver instance created and opened
@post Any IDN shall be resolved using RHostResolver API's

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoEnableIdnSupport()
	{
	TBool enable = ETrue;
	TPckg<TBool> pckgEnable(enable);
	if(iHostResolver)
		{
		SetError(iHostResolver->SetOpt(KSoDnsEnableIdn, KSolInetDns, pckgEnable));
		}
	else
		{
		ERR_PRINTF1(KInvldHostRslverState);
		SetError(KErrNotFound);
		}
	}

/**
Function to disable IDN(International Domain Name) support for DNS.

@pre RHostResolver instance created and opened.
@post Any IDN shall not be supported RHostResolver.
@see Sets error if the RHostResolver instance is not created. 

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoDisableIdnSupport()
	{
	TBool enable = EFalse;
	TPckg<TBool> pckgEnable(enable);
	if(iHostResolver)
		{
		SetError(iHostResolver->SetOpt(KSoDnsEnableIdn, KSolInetDns, pckgEnable));
		}
	else
		{
		ERR_PRINTF1(KInvldHostRslverState);
		SetError(KErrNotFound);
		}
	}

/**
Function to open the resolver and to start a sub session using RSockServ.

@post Any DN(Domain Name) shall be resolved using RHostResolver API's. 
@see Sets error if the RHostResolver or RSockeServ error occurs. 

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoOpenResolver()
	{
	
	iSockServError = iSockServ->Connect();
	if(iSockServError != KErrNone)
		{
		ERR_PRINTF2(KSockServConnFail, iSockServError);
		SetError(iSockServError);
		return;
		}
	iHostResolverError = iHostResolver->Open(*iSockServ, KAfInet, KProtocolInetUdp);
	if (iHostResolverError != KErrNone) 
		{
		ERR_PRINTF2(KHostResolverOpenFail, iHostResolverError);
		SetError(iHostResolverError);
		}
	}

/**
Function to close RHostResolver and RSockeServ.

@see RHostResolver and RSockeServ are opened by DoOpenResolver

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoCloseResolver()
	{
	iHostResolver->Close();
	iSockServ->Close();
	}


/**
Function to resolve DNs.

@param  aSection Current ini file command section
@see Sets error on upon config file read error or any RHostResolver error occurs.

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoResolveName(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("****************************************************"));
	INFO_PRINTF1(_L("Test International Domain Name resolve Functionality"));
	INFO_PRINTF1(_L("****************************************************"));
				
	TPtrC ptrToReadFromConfig(KNameDefault);
	TBool inputFromConfig;
	TBool returnValue = GetBoolFromConfig(aSection, KInputFromConfig, inputFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading InputFromConfig"));
		SetError(KErrUnknown);
		return;
		}
	// Get destination name from config file
	if(inputFromConfig)
		{
		returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
		if (!returnValue)
			{
			ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));
			SetError(KErrUnknown);
			return;
			}
		}
	else
		{
		//Input is set by Previous test case
		ptrToReadFromConfig.Set(iNextTestCaseInput);
		}
	
	// Get the Host name, which is read from config file
	TInputName	hostName;
	hostName.Copy(ptrToReadFromConfig.Ptr(), ptrToReadFromConfig.Length());	
	
	TNameEntry result;
	if(iHostResolverError == KErrNone)
		{
		iHostResolverError = iHostResolver->GetByName(hostName, result);
		}
	else
		{
		ERR_PRINTF3(KHostRslvOrSockServFail, iHostResolverError, iSockServError);
		SetError((iHostResolverError) ? iHostResolverError : iSockServError);
		}
	INFO_PRINTF2(KHostRslvReturn, iHostResolverError);
	
	SetError(iHostResolverError);
	}

/**
Function to resolve IP address.

@param  aSection Current ini file command section
@see Sets error on upon config file read error or any RHostResolver error occurs.

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoResolveAddress(const TDesC& aSection)
	{
	INFO_PRINTF1(_L("*********************************"));
	INFO_PRINTF1(_L("Test Get by address Functionality"));
	INFO_PRINTF1(_L("*********************************"));
				
	TPtrC ptrToReadFromConfig(KNameDefault);
	
	// Get destination address from config file	 		
	TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));	 
		SetError(KErrUnknown);
		return;
		}
	
	// Create address
	TInetAddr ipAddr;
	TInt err = ipAddr.Input(ptrToReadFromConfig);	
	if(err != KErrNone)
		{
		INFO_PRINTF2(KInetInputFail, err);
		SetError(err);
		return;
		}
	
	// Resolve address
	TNameEntry dnsResult;
	if(iHostResolverError == KErrNone)
		{
		iHostResolverError = iHostResolver->GetByAddress(ipAddr, dnsResult);
		}
	else
		{
		ERR_PRINTF3(KHostRslvOrSockServFail, iHostResolverError, iSockServError);
		SetError((iHostResolverError) ? iHostResolverError : iHostResolverError);
		}
	INFO_PRINTF2(KHostRslvReturn, iHostResolverError);
	
	SetError(iHostResolverError);
	}

/**
Function to convert Punycode to IDN and compare with the manually converted IDN which shall be
fetched from INI file.

@param  aSection Current ini file command section
@see Sets error on upon config file read error or any TPunyCodeDndName error occurs.

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoPunyCodeToIdn(const TDesC& aSection)
	{
	TPtrC ptrToReadFromConfig(KNameDefault);
	TInt err = KErrNone;
	// Get the input data
	TBool returnValue = GetStringFromConfig(aSection, KInput, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading Input"));
		SetError(KErrUnknown);
		return;
		}
	TName inputName;
	inputName.Copy(ptrToReadFromConfig.Ptr(), ptrToReadFromConfig.Length());
	
	//Punycode to IDN
	INFO_PRINTF1(_L("*********************"));
	INFO_PRINTF1(_L("Test Punycode to IDN "));
	INFO_PRINTF1(_L("*********************"));
	TInt index = inputName.Find(KPunyCodeSign);
	if(index == KErrNotFound )
		{
		ERR_PRINTF1(KNotAPunyCodeName);
		SetError(index);
		return;
		}
	
	TPunyCodeDndName punyCodeDndName;
	TBuf16<128> testPtrToRead;
	testPtrToRead = ptrToReadFromConfig;
	
	//Write the 8 bit Punycode into punyCodeDndName
	CnvUtfConverter::ConvertFromUnicodeToUtf8(punyCodeDndName, testPtrToRead);
	
	err = punyCodeDndName.PunycodeToIdn(inputName,index);
	iNextTestCaseInput = inputName;

	if (err != KErrNone)
		{
		ERR_PRINTF2(KConversionFail, err);
		SetError(err);
		return;
		}

	//Get the expected value from ini file
	returnValue = GetStringFromConfig(aSection, KExpect, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading Expect"));
		SetError(KErrUnknown);
		return;
		}

	TPunyCodeDndName expectedName8;
	//Convert the expected result to 8 bit
	CnvUtfConverter::ConvertFromUnicodeToUtf8(expectedName8, ptrToReadFromConfig);
	
	//Puny code to IDN
	TBuf16<128> inputNamePtr;
	inputNamePtr = inputName;
	TPunyCodeDndName inputName8;
	//Convert the input IDN to 8 bit
	CnvUtfConverter::ConvertFromUnicodeToUtf8(inputName8, inputNamePtr);
	err = inputName8.Match(expectedName8);
	if(err != KErrNone)
		{
		INFO_PRINTF2(KMatchFail, err);
		}
	SetError(err);
	}

/**
Function to convert IDN to punycode and compare with the manually converted punycode which shall be
fetched from INI file.

@param  aSection Current ini file command section
@see Sets error on upon config file read error or any TPunyCodeDndName error occurs.

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoIdnToPunyCode(const TDesC& aSection)
	{
	TPtrC ptrToReadFromConfig(KNameDefault);
	TInt err = KErrNone;
	// Get the input data
	TBool returnValue = GetStringFromConfig(aSection, KInput, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading Input"));
		SetError(KErrUnknown);
		return;
		}
	TName inputName;
	inputName.Copy(ptrToReadFromConfig.Ptr(), ptrToReadFromConfig.Length());
	
	INFO_PRINTF1(_L("**********************************"));
	INFO_PRINTF1(_L("Test IDN to Punycode Functionality"));
	INFO_PRINTF1(_L("**********************************"));
	TPunyCodeDndName punyCodeDndName;
	err = punyCodeDndName.IdnToPunycode(inputName);
	
	//Write the converted punycode into config file, which is input for next test case
	CnvUtfConverter::ConvertToUnicodeFromUtf8(iNextTestCaseInput, punyCodeDndName);

	if (err != KErrNone)
		{
		ERR_PRINTF2(KConversionFail, err);
		SetError(err);
		return;
		}
	
	//Get the expected value from ini file
	returnValue = GetStringFromConfig(aSection, KExpect, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading Expect"));
		SetError(KErrUnknown);
		return;
		}
	TBuf16<128> testPtrToRead;
	testPtrToRead = ptrToReadFromConfig;
	TPunyCodeDndName expectedName8;
	//Convert the expected result to 8 bit
	CnvUtfConverter::ConvertFromUnicodeToUtf8(expectedName8, testPtrToRead);
	
	//IDN to Puny code
	err = punyCodeDndName.Match(expectedName8);
	
	if(err < KErrNone)
		{ //Match not found
		SetError(err);
		}
	else
		{ //Match found
		SetError(KErrNone);
		}
	}

/**
Function to test capapbility of Punycode Converter

@param  aSection Current ini file command section
@see Sets error on upon config file read error or any RHostResolver error occurs.

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoCapTest(const TDesC& aSection)
	{
	
	INFO_PRINTF1(_L("**********************************"));
	INFO_PRINTF1(_L("PunyCode converter Capability Test"));
	INFO_PRINTF1(_L("**********************************"));
	
	TPtrC ptrToReadFromConfig(KNameDefault);
	TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));
		SetError(KErrUnknown);
		return;
		}
	TInputName	hostName;
	TNameEntry result;
	hostName.Copy(ptrToReadFromConfig.Ptr(), ptrToReadFromConfig.Length());
	
	TInt err = iHostResolver->SetHostName(hostName);
	
	INFO_PRINTF2(KHostRslvReturn, err);
	
	if(RProcess().HasCapability(ECapabilityNetworkControl))
		{
		if(err == KErrPermissionDenied)
			{
			SetBlockResult(EFail);
			return;
			}
		}
	SetError(err);
	}

/**
Function to test Surrogate pair support.

Surrogate Pair: If the encoded representation of IDN character does not fit within 16
bits, it is represented as pair of 16 bits.  
Currently this support is not available in the RHostResolver.

@param  aSection Current ini file command section
@see Sets error on upon config file read error or any RHostResolver error occurs.

@internalTechnology
*/
void CPunycodeConverterTestWrapper::DoSurrogatePair(const TDesC& aSection)
	{
	
	INFO_PRINTF1(_L("*******************"));
	INFO_PRINTF1(_L("Surrogate pair Test"));
	INFO_PRINTF1(_L("*******************"));

	TInputName	hostName;
	
	TInt ptrToReadFromConfig = 0;
	//Read input high value
	TBool returnValue = GetIntFromConfig(aSection, KInputHighByte, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading InputH"));
		SetError(KErrUnknown);
		return;
		}
	
	hostName.Copy((const unsigned short*)&ptrToReadFromConfig);
	//Read input low value
	returnValue = GetIntFromConfig(aSection, KInputLowByte, ptrToReadFromConfig);
	if (!returnValue)
		{
		INFO_PRINTF1(_L("Reading config file failed, while reading InputL"));
		SetError(KErrUnknown);
		return;
		}
	
	TNameEntry result;
	hostName.Append((const unsigned short*)&ptrToReadFromConfig, sizeof(TInt));
	TInt err = iHostResolver->GetByName(hostName, result);
	INFO_PRINTF2(KHostRslvReturn, err);
	
	SetError(err);
	}
