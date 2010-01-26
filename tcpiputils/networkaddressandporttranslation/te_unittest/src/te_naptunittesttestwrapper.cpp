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

#include "te_naptunittesttestwrapper.h"
#include <e32base.h>
#include <in_sock.h>
#include <es_sock.h>
#include <commdbconnpref.h>
#include "naptinterface.h"

// Commands
_LIT(KBringUpIfs, "BringUpIfs");
_LIT(KOpenSockets, "OpenSockets");
_LIT(KCloseSockets, "CloseSockets");
_LIT(KCloseIfs, "CloseIfs");
_LIT(KConfigureNapt, "ConfigureNapt");
_LIT(KConfigureNaptCustom, "ConfigureNaptCustom");
_LIT(KGetOpt, "GetOpt");
_LIT(KOpenNaptSocket,"OpenNaptSocket");
_LIT(KOpenPublicSocket, "OpenPublicSocket");
_LIT(KConnectSocketServ, "ConnectSocketServ");
_LIT(KCloseSocketServ, "CloseSocketServ");
_LIT(KConfigureNaptWithInvalidBuffer,"ConfigureNaptWithInvalidBuffer");
_LIT(KGetOptWithInvalidBuffer, "GetOptWithInvalidBuffer");

//INI file
_LIT(KPublicIap, "PublicIap");
_LIT(KPrivateIap, "PrivateIap");
_LIT(KPublicAddr, "PublicAddr");
_LIT(KPrivateAddr, "PrivateAddr");
_LIT(KUplinkAccess, "UplinkAccess");
_LIT(KOptionName, "OptionName");
_LIT(KGetOptOptionName, "GetOptOptionName");
_LIT(KOptionLevel, "OptionLevel");

//Test code
_LIT(KCommand, 			"aCommand = %S");
_LIT(KSection, 			"aSection = %S");
_LIT(KAsyncErrorIndex, 	"aAsyncErrorIndex = %D");

//Test code error
_LIT(KPublicConnectionError, "Public RConnection error : %d");
_LIT(KPrivConnectionError, "Private RConnection error : %d");
_LIT(KPublicSockError, "Public RSocket error: %d");
_LIT(KPrivateSockError, "Private RSocket error: %d");
_LIT(KSocketServError, "RSockServ error: %d");
_LIT(KIniError, "INI file Read error: %d");


/**
Macro to check and register errors

@internalTechnology
*/
#define REGISTER_ERROR(aErrorCategory, aError)\
    iErrorCode = aError;\
    if(aError != KErrNone)\
    {\
        iErrorBit |= aErrorCategory;\
        return;\
    }

/**
Macro to check and return errors

@internalTechnology
*/
#define CHECK_AND_RETURN(aError)\
    if(aError != KErrNone)\
    {\
        return aError;\
    }

/**
Funtion to report the observed errors

@internalTechnology
*/
void CNaptUnitTestTestWrapper::ReportError()
{
    TInt index = 0;
    TInt forMatching = 1;
    for(; index < KNoOfErrors; ++index)
    {
    switch(iErrorBit & forMatching)
        {
        case KSockServError:
            ERR_PRINTF2(KSocketServError, iErrorCode);
            SetError(iErrorCode);
            break;
        case KPublicConnError:
            ERR_PRINTF2(KPublicConnectionError, iErrorCode);
            SetError(iErrorCode);
            break;
        case KPrivateConnError:
            ERR_PRINTF2(KPrivConnectionError, iErrorCode);
            SetError(iErrorCode);
            break;
        case KPublicSocketError:
            ERR_PRINTF2(KPublicSockError, iErrorCode);
            SetError(iErrorCode);
            break;
        case KPrivateSocketError:
            ERR_PRINTF2(KPrivateSockError, iErrorCode);
            SetError(iErrorCode);
            break;
        case KIniReadError:
            ERR_PRINTF2(KIniError,iErrorCode);
            SetError(iErrorCode);
            break;
        default:
            break;
        }
    forMatching <<= 1;
    }
}

/**
Constructor.

@internalTechnology
*/
CNaptUnitTestTestWrapper::CNaptUnitTestTestWrapper()
	{
	}

/**
Destructor.

@internalTechnology
*/
CNaptUnitTestTestWrapper::~CNaptUnitTestTestWrapper()
	{
	delete iSockServ;
	delete iPrivateConn;
	delete iPublicConn;
    delete iPrivateSocket;
    delete iPublicSocket;
    delete iPrivateIp;
	}

/**
Function to instantiate TestWrapper.

@return Returns constructed TestWrapper instance pointer

@internalTechnology
*/
CNaptUnitTestTestWrapper* CNaptUnitTestTestWrapper::NewL()
	{
	CNaptUnitTestTestWrapper* naptUnitTestTestWrapper = new (ELeave) CNaptUnitTestTestWrapper();
	CleanupStack::PushL(naptUnitTestTestWrapper);
	naptUnitTestTestWrapper->ConstructL();
	CleanupStack::Pop();
	return naptUnitTestTestWrapper;
	}

/**
Function to instantiate and intialize the required class members.

@internalTechnology
*/
void CNaptUnitTestTestWrapper::ConstructL()
    {
    iSockServ = new (ELeave) RSocketServ;
    iPrivateConn = new (ELeave) RConnection;
    iPublicConn = new (ELeave) RConnection;
    iPrivateSocket = new (ELeave) RSocket;
    iPublicSocket = new (ELeave) RSocket;
    iPrivateIp = new (ELeave) TInetAddr;
    iErrorBit = 0;
    }

/**
 * Get interface Ip Address
 * @param aSock - socket connection
 * @param aFName - interface name
 * @param aAddr - To save Ip address
 * @return Error Code
 *
 * @internalTechnology
 */
TInt CNaptUnitTestTestWrapper::GetInterfaceAddress(RSocket* aSock, const TDesC& aFName, TUint32& aAddr)
    { 
	TName address;
	TBool isSiteLocal,isLinkLocal;
	TInt retVal = -1;
	
	TPckgBuf<TSoInetInterfaceInfo> info; 
	 
	retVal = aSock->SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
	CHECK_AND_RETURN(retVal);
 	
	retVal = aSock->GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info);
	CHECK_AND_RETURN(retVal);

   	while(retVal==KErrNone)
		 {
	 	 info().iAddress.Output(address);
				
		 isSiteLocal = info().iAddress.IsSiteLocal();	
		 isLinkLocal = info().iAddress.IsLinkLocal();
	
		 TInt str = info().iName.Find(aFName);
		 if((str == KErrNone) && (isSiteLocal == EFalse) && (isLinkLocal == EFalse))
		    {
	         INFO_PRINTF2(_L("Interface address obtained: %S."),&address);
		     break;
			}
		  retVal = aSock->GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info);
	      }
    
   	CHECK_AND_RETURN(retVal);
   	aAddr = info().iAddress.Address();
	return retVal;
    }


/**
 * Get interface index
 * @param aSock - socket connection
 * @param aFName - interface name
 * @param aAddr - to save Interface index
 * @return errocode 
 *
 * @internalTechnology
 */
TInt CNaptUnitTestTestWrapper::GetInterfaceIndex(RSocket* aSock, const TDesC& aFName, TUint32& aIndex)
    {
    TPckgBuf<TSoInetInterfaceInfo> interfaceInfo; 
    TPckgBuf<TSoInetIfQuery> queryInfo;
    
    TName address;
    TInt retVal = KErrNotFound;

    retVal = aSock->SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
    CHECK_AND_RETURN(retVal);
    
    retVal = aSock->GetOpt(KSoInetNextInterface, KSolInetIfCtrl, interfaceInfo);
    CHECK_AND_RETURN(retVal);
            
    while(retVal == KErrNone)
         {
         TInt str = interfaceInfo().iName.Find(aFName);
         if(str == KErrNone)
            {
            queryInfo().iName = interfaceInfo().iName;
            break;
            }
          retVal = aSock->GetOpt(KSoInetNextInterface, KSolInetIfCtrl, interfaceInfo);
          }
    CHECK_AND_RETURN(retVal);
    
    retVal = aSock->GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, queryInfo);
    
    CHECK_AND_RETURN(retVal);
    aIndex = queryInfo().iIndex;
    return retVal;
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
TBool CNaptUnitTestTestWrapper::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	TBool ret = ETrue;
	
	// Print out the parameters for debugging
	INFO_PRINTF2( KCommand, &aCommand );
	INFO_PRINTF2( KSection, &aSection );
	INFO_PRINTF2( KAsyncErrorIndex, aAsyncErrorIndex );
    if(iErrorBit == KErrNone)
    {
        if(KBringUpIfs() == aCommand)
            {
            DoBringUpIfs(aSection);
            }
        else if(KConnectSocketServ() == aCommand)
            {
            DoConnectSocketServ();
            }
        else if(KConfigureNapt() == aCommand)
            {
            DoConfigureNapt();
            }
        else if(KCloseIfs() == aCommand)
            {
            DoCloseIfs();
            }
        else if(KConfigureNaptCustom() == aCommand)
            {
            DoConfigureNaptCustom(aSection);
            }
        else if(KOpenSockets() == aCommand)
            {
            DoOpenSockets();
            }
        else if(KCloseSockets() == aCommand)
            {
            DoCloseSockets();
            }
        else if(KGetOpt() == aCommand)
            {
            DoGetOpt(aSection);
            }
        else if(KOpenNaptSocket() == aCommand)
            {
            DoOpenNaptSocket();
            }
        else if(KOpenPublicSocket() == aCommand)
            {
            DoOpenPublicSocket();
            }
        else if(KCloseSocketServ() == aCommand)
            {
            DoCloseSocketServ();
            }
        else if(KConfigureNaptWithInvalidBuffer() == aCommand)
            {
            DoConfigureNaptWithInvalidBuffer();
            }
        else if(KGetOptWithInvalidBuffer() == aCommand)
            {
            DoGetOptWithInvalidBuffer();
            }
        else
            {
            ret = EFalse;
            User::LeaveIfError(KErrNone); // just to suppress LeaveScan warning
            }
        }
    else
        {
        ret = EFalse;
        }
    ReportError();
	return ret;
	}

/**
 * Function to read data from ini file.
 * @param aSection - section to read
 * @return error or KErrNone
 *
 * @internalTechnology
 */
TInt CNaptUnitTestTestWrapper::ReadIniFile(const TTEFSectionName& aSection)
{
    if(!GetIntFromConfig(aSection, KPublicIap, iPublicIap))
        return KErrNotFound;
    if(!GetIntFromConfig(aSection, KPrivateIap, iPrivateIap))
        return KErrNotFound;
    
    TPtrC ptrToReadIp(0,NULL);
    if(!GetStringFromConfig(aSection, KPrivateAddr, ptrToReadIp))
        return KErrNotFound;
    iPrivateIp->Input(ptrToReadIp); 
    
    if(!GetIntFromConfig(aSection, KUplinkAccess, iUplinkAccess))
        return KErrNotFound;

    return KErrNone;
}


/**
 * Function to connect to the SocketServer.
 * Registers error in case
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoConnectSocketServ()
{
    TInt err = KErrNotFound;
    err = iSockServ->Connect();
    REGISTER_ERROR(KSockServError, err);
}

/**
 * Function to close SocketServer.
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoCloseSocketServ()
{
    iSockServ->Close();
}

/**
 * Function to start the Private(Ethernet) and Public(IPCP::COMM) Interface.
 * Registers error in case
 * @param aSection - section to read
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoBringUpIfs(const TTEFSectionName& aSection)
{
    TInt err = KErrNotFound;
    err = ReadIniFile(aSection);
    REGISTER_ERROR(KIniReadError, err);
    
    DoConnectSocketServ();
    if(iErrorBit != KErrNone)
        {
        return;
        }
   
    err = iPublicConn->Open(*iSockServ);
    REGISTER_ERROR(KPublicConnError, err);
    
    err = iPrivateConn->Open(*iSockServ);
    REGISTER_ERROR(KPrivateConnError, err);
    
    TCommDbConnPref prefs;
    prefs.SetIapId(iPublicIap);
    err = iPublicConn->Start(prefs);
    REGISTER_ERROR(KPublicConnError, err);
    
    prefs.SetIapId(iPrivateIap);
    err = iPrivateConn->Start(prefs);
    REGISTER_ERROR(KPrivateConnError, err);
}

/**
 * Function to stop the Private(Ethernet) and Public(IPCP::COMM) Interface.
 * Registers error in case
 * @param aSection - section to read
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoCloseIfs()
{
    TInt err = KErrNotFound;
    err = iPublicConn->Stop();
    REGISTER_ERROR(KPublicConnError, err);
    iPublicConn->Close();
    
    err = iPrivateConn->Stop();
    REGISTER_ERROR(KPrivateConnError, err);
    iPrivateConn->Close();
    DoCloseSocketServ();
    return;
}

/**
 * Function to open the the Private(Ethernet) and Public(IPCP::COMM) Sockets.
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoOpenSockets()
{
    DoOpenNaptSocket();
    DoOpenPublicSocket();
}

/**
 * Function to open the socket and to load the NAPT protocol.
 * Registers error in case
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoOpenNaptSocket()
{
    TInt err = KErrNotFound;
    err = iPrivateSocket->Open(*iSockServ, _L("napt"));
    REGISTER_ERROR(KPrivateSocketError, err)
}

/**
 * Function to open the public socket.
 * Registers error in case
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoOpenPublicSocket()
{
    TInt err = KErrNotFound;
    err = iPublicSocket->Open(*iSockServ, KAfInet, KSockStream, KProtocolInetTcp, *iPublicConn);
    REGISTER_ERROR(KPublicSocketError, err);
}

/**
 * Function to close the the Private(Ethernet) and Public(IPCP::COMM) Sockets.
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoCloseSockets()
{
    if(iPrivateSocket)
    {
        iPrivateSocket->Close();
    }
    if(iPublicSocket)
    {
        iPublicSocket->Close();
    }
}

/**
 * Function to Configure NAPT with valid parameters
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoConfigureNapt()
{
    TInt err = KErrNotFound;
    const TInt netMaskLength   = 8;
    _LIT(KInterfaceName,"ipcp::comm");
    _LIT(KInterfaceName1,"eth");
    
    TUint32 publicAddr = 0;
    err = GetInterfaceAddress(iPublicSocket, KInterfaceName(), publicAddr);
    REGISTER_ERROR(KPublicSocketError, err);
    
    TPckgBuf <TInterfaceLockInfo> info;
    info().iPublicIap  = iPublicIap;  
    info().iPrivateIap = iPrivateIap;  
    TInetAddr::Cast	(info().iPrivateIp).SetV4MappedAddress(iPrivateIp->Address()); 
    TInetAddr::Cast(info().iPublicIp).SetV4MappedAddress(publicAddr) ; 
    info().iNetmaskLength = netMaskLength; 
    err = GetInterfaceIndex(iPublicSocket, KInterfaceName1(), info().iIfIndex);
    REGISTER_ERROR(KPrivateSocketError, err);
    
    #ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION	
    info().iUplinkAccess = iUplinkAccess;
    #endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
    
    err = iPrivateSocket->SetOpt(KSoNaptSetup, KSolNapt, info);
    REGISTER_ERROR(KPrivateSocketError, err);
}

/**
 * Function to Configure NAPT with customized parameters
 * @param aSection - section to read the custom parameters
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoConfigureNaptCustom(const TTEFSectionName& aSection)
{
    TInt err = KErrNotFound;
    const TInt netMaskLength   = 8;
    const TInt interfaceIndex = 4;
    
    TInt publicIap = 0;
    if(!GetIntFromConfig(aSection, KPublicIap, publicIap))
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    
    TInt privateIap = 0;
    if(!GetIntFromConfig(aSection, KPrivateIap, privateIap))
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    
    TPtrC publicAddrPtr(0, NULL);
    TInetAddr publicAddr;
    if(!GetStringFromConfig(aSection, KPublicAddr, publicAddrPtr))
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    
    TPtrC privateAddrPtr(0, NULL);
    TInetAddr privateAddr;
    if(!GetStringFromConfig(aSection, KPrivateAddr, privateAddrPtr))		
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    
    TBool uplinkAccess = 0;
    if(!GetIntFromConfig(aSection, KUplinkAccess, uplinkAccess))		
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    
    TInt optionName = KSoNaptSetup;
    if(!GetIntFromConfig(aSection, KOptionName, optionName))
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    
    TInt optionLevel = KSoNaptSetup;
    if(!GetIntFromConfig(aSection, KOptionLevel, optionLevel))
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    
    publicAddr.Input(publicAddrPtr);
    privateAddr.Input(privateAddrPtr);
    
    TPckgBuf <TInterfaceLockInfo> info;
    info().iPublicIap  = publicIap;  
    info().iPrivateIap = privateIap;  
    TInetAddr::Cast (info().iPrivateIp).SetV4MappedAddress(privateAddr.Address()); 
    TInetAddr::Cast(info().iPublicIp).SetV4MappedAddress(publicAddr.Address()) ; 
    info().iNetmaskLength = netMaskLength; 
    info().iIfIndex = interfaceIndex;

    #ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION 
    info().iUplinkAccess = uplinkAccess;
    #endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
    
    err = iPrivateSocket->SetOpt(optionName, optionLevel, info);
    SetError(err);
}

/**
 * Function to Configure NAPT with invalid buffer
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoConfigureNaptWithInvalidBuffer()
{
    TInt err = KErrNotFound;
    TPckgBuf <TUplinkInfo> info;
    err = iPrivateSocket->SetOpt(KSoNaptSetup, KSolNapt, info);
    SetError(err);
}

/**
 * Function to get the SAP configuration associated with NAPT Socket
 * @param aSection - section to read the custom parameters
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoGetOpt(const TTEFSectionName& aSection)
{
    TPckgBuf <TUplinkInfo> info;
    TInt privateIap = 0;
    if(!GetIntFromConfig(aSection, KPrivateIap, privateIap))
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    info().iPrivateIap = privateIap;
    TInt getOptOptionName = KErrNone;

    if(!GetIntFromConfig(aSection, KGetOptOptionName, getOptOptionName))
    {
        REGISTER_ERROR(KIniReadError, KErrNotFound);
    }
    TInt err = iPrivateSocket->GetOpt(getOptOptionName, KSolNapt, info);
    SetError(err);
}

/**
 * Function to get the SAP configuration associated with NAPT Socket by passing invalid Buffer.
 * @param aSection - section to read the custom parameters
 *
 * @internalTechnology
 */
void CNaptUnitTestTestWrapper::DoGetOptWithInvalidBuffer()
{
    TPckgBuf <TInterfaceLockInfo> info;
    TInt err = iPrivateSocket->GetOpt(KSoNaptUplink, KSolNapt, info);
    SetError(err);
}

