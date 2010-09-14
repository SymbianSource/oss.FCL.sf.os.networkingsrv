// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "te_dnssuffixtestwrapper.h"
#include "DNSSuffixHostResolver.h"


#include <e32base.h>
#include <in_sock.h>
#include <es_sock.h>
#include <commdbconnpref.h>
#include <commdb.h>

/*
 * TODO: 1. Try to test with end to end real scenario. Means resolving over real tunnel.
 * 
 * 
 */
#define KIPAddrLen 39

// Used for handle the multiple hosts in same test cases. @See TC006
#define KMaxHosts 8

// Commands
_LIT(KInitDNSSuffixTesting, "InitDNSSuffixTesting");
_LIT(KEnumerateInterface,   "EnumerateInterface");
_LIT(KSetAndGetSuffixList,  "SetAndGetSuffixList");
_LIT(KDoResolve,            "DoResolve");
_LIT(KDoResolveHostWithoutDomainWithSuffix,     "DoResolveHostWithoutDomainWithSuffix");
_LIT(KDoResolveHostWithoutDomainWithoutSuffix,  "DoResolveHostWithoutDomainWithoutSuffix");

_LIT(KDNSSuffixSupportTC005,    "DNSSuffixSupportTC005");
_LIT(KDNSSuffixSupportTC006,    "DNSSuffixSupportTC006");
_LIT(KDNSSuffixSupportTC007,    "DNSSuffixSupportTC007");
_LIT(KDNSSuffixSupportTC008,    "DNSSuffixSupportTC008");
_LIT(KDNSSuffixSupportTC009,    "DNSSuffixSupportTC009");
_LIT(KDNSSuffixSupportTC010,    "DNSSuffixSupportTC010");
_LIT(KDNSSuffixSupportTC011,    "DNSSuffixSupportTC011");


//Test code
_LIT(KCommand, 			    "aCommand = %S");
_LIT(KSection, 			    "aSection = %S");
_LIT(KAsyncErrorIndex, 	    "aAsyncErrorIndex = %D");

_LIT(KKeyDNSSuffixTestSuffixList,   "suffixlist");
_LIT(KKeyDNSSuffixTestSuffixList1,   "suffixlist1");
_LIT(KKeyDNSSuffixTestSuffixList2,   "suffixlist2");
_LIT(KKeyDNSSuffixTestSuffixList3,   "suffixlist3");

_LIT(KKeyDNSSuffixTestHostname,     "hostname");

_LIT(KSectionDNSSuffixTest001,      "dnssuffix_test001");
_LIT(KSectionDNSSuffixTest002,      "dnssuffix_test002");
_LIT(KSectionDNSSuffixTest003,      "dnssuffix_test003");
_LIT(KSectionDNSSuffixTest004,      "dnssuffix_test004");
_LIT(KSectionDNSSuffixTest005,      "dnssuffix_test005");

_LIT(KSectionDNSSuffixTest006,          "dnssuffix_test006");
_LIT(KKeyDNSSuffixTest006NoOfHostname,  "noofhosts");
_LIT(KKeyDNSSuffixTest006Hostname,      "hostname%D");

_LIT(KSectionDNSSuffixTest007,              "dnssuffix_test007");
_LIT(KKeyDNSSuffixTest007ValidHostname,     "validhostname");
_LIT(KKeyDNSSuffixTest007InValidHostname,   "invalidhostname");

_LIT(KSectionDNSSuffixTest008,      "dnssuffix_test008");
_LIT(KKeyDNSSuffixTestHostname1,    "hostname1");
_LIT(KKeyDNSSuffixTestHostname2,    "hostname2");

_LIT(KSectionDNSSuffixTest009,      "dnssuffix_test009");

_LIT(KSectionDNSSuffixTest010,      "dnssuffix_test010");

_LIT(KSectionDNSSuffixTest011,      "dnssuffix_test011");

_LIT(KSectionDNSSuffixFirstInterface, "dnssuffix_first_interface");
_LIT(KSectionDNSSuffixSecondInterface, "dnssuffix_second_interface");
_LIT(KKeyDNSSuffixIAPId,"iapid");
_LIT(KKeyInterfaceName,                "interfacename");

/*
 * @author: Santosh K Patil
 * TODO: Needs to verify the interfaces used to resolve the hosts in each test case.
 * It is required to verify wheather test executed as desired or not. 
 * In other words to confirm the new solution is working as expected we need to get 
 * and verify the inteface name used to resolve the host.
 * @See Test spec for more details.
 * 
 * TODO: Check all config sections
 */


/**
Constructor.

@internalTechnology
 */
CDNSSuffixTestWrapper::CDNSSuffixTestWrapper()
    {
    }

/**
Destructor.

@internalTechnology
 */
CDNSSuffixTestWrapper::~CDNSSuffixTestWrapper()
    {  
    
    }

/**
Function to instantiate TestWrapper.
@return Returns constructed TestWrapper instance pointer
@internalTechnology
 */
CDNSSuffixTestWrapper* CDNSSuffixTestWrapper::NewL()
    {
    CDNSSuffixTestWrapper*	ret = new (ELeave) CDNSSuffixTestWrapper();
    CleanupStack::PushL(ret);
    ret->ConstructL();
    CleanupStack::Pop(ret);
    return ret;	
    }

/**
Second level constructor, constructs TestWrapper instance.
@internalTechnology
 */
void CDNSSuffixTestWrapper::ConstructL()
    {    
    }


/*
 * @HandleCallBackL Called by the CDNSSuffixHostResolver::RunL
 * 
 */
void CDNSSuffixTestWrapper::HandleCallBackL(TInt aError)
    {
    INFO_PRINTF1(_L("HandleCallBackL - Entry"));    
    INFO_PRINTF2(_L("Error code: %D"),aError);
    
    if (KErrNone != aError)
        SetError(KErrNone);
    else
        SetError(KErrGeneral);
    
    if (iWait.IsStarted())
        {
        INFO_PRINTF1(_L("Stoping waiter"));
        iWait.AsyncStop();
        }
    INFO_PRINTF1(_L("HandleCallBackL - Exit"));
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
TBool CDNSSuffixTestWrapper::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
    {
    TBool ret = ETrue;

    // Print out the parameters for debugging
    INFO_PRINTF2( KCommand, &aCommand );
    INFO_PRINTF2( KSection, &aSection );
    INFO_PRINTF2( KAsyncErrorIndex, aAsyncErrorIndex );
    
    if(KInitDNSSuffixTesting() == aCommand)
        {
        DoInitTestingL();
        }
    else if(KEnumerateInterface() == aCommand)
        {
        DoEnumerateInterfacesL();
        }
    else if (KSetAndGetSuffixList() == aCommand)
        {
        DoSetAndGetSuffixListL();
        }
    else if( KDoResolve() == aCommand)
        {
        DoResolveL();
        }
    else if( KDoResolveHostWithoutDomainWithSuffix() == aCommand)
        {
        DoResolveHostWithoutDomainWithSuffixListSetL();
        }    
    else if( KDoResolveHostWithoutDomainWithoutSuffix() == aCommand)
        {
        DoResolveHostWithoutDomainWithoutSuffixListL();
        }
    else if( KDNSSuffixSupportTC005() == aCommand)
        {
        DNSSuffixSupportTC005L();
        }
    else if( KDNSSuffixSupportTC006() == aCommand)
        {
        DNSSuffixSupportTC006L();
        }
    else if( KDNSSuffixSupportTC007() == aCommand)
        {
        DNSSuffixSupportTC007L();
        }
    else if( KDNSSuffixSupportTC008() == aCommand)
        {
        DNSSuffixSupportTC008L();
        }
    else if( KDNSSuffixSupportTC009() == aCommand)
        {
        DNSSuffixSupportTC009L();
        }
    else if( KDNSSuffixSupportTC010() == aCommand)
       {
       DNSSuffixSupportTC010L();
       }
    else if( KDNSSuffixSupportTC011() == aCommand)
       {
       DNSSuffixSupportTC011L();
       }    
    else
        {        
        ret = EFalse;
        User::LeaveIfError(KErrNone); // just to suppress LeaveScan warning
        }
    
    return ret;
    }


TInt CDNSSuffixTestWrapper::StartConnections(RSocketServ& aSockServ, RConnection& aConn1, RConnection& aConn2)
    {
    INFO_PRINTF1(_L("StartConnections - Entry"));
    
    TUint iapId = GetFirstIapId();
    
    if ((TInt)iapId == KErrNotFound)
        return KErrNotFound;
    
    TInt err = StartConnection(aSockServ,aConn1,iapId);
        
    if (err != KErrNone)
        {
        ERR_PRINTF1(_L("Failed to start first connection"));
        SetError(err);
        return err;
        }
        
    INFO_PRINTF2(_L("First connection started successfully - IAP: %D"),iapId);        
        
    iapId = GetSecondIapId();
             
    if ((TInt)iapId == KErrNotFound)
        {       
        aConn1.Close();
        return KErrNotFound;
        }
        
    err = StartConnection(iSocketServ,aConn2,iapId);
        
    if (err != KErrNone)
        {
        ERR_PRINTF1(_L("Failed to start second connection"));
        aConn1.Close();
        return err;
        }
        
    INFO_PRINTF2(_L("Second connection started successfully - IAP: %D"),iapId);        
            
    INFO_PRINTF1(_L("StartConnections - Exit"));
    return KErrNone;
    }



TInt CDNSSuffixTestWrapper::StartConnection(RSocketServ& aSockServ, RConnection& aConn, TUint aIapId)
    {
    INFO_PRINTF1(_L("StartConnection - Entry"));
    
    TInt err(KErrNone);
    
    TCommDbConnPref connPref;
    INFO_PRINTF1(_L("Getting conn preference"));
    
    TRAP(err, GetConnPrefL(aIapId,connPref));
    
    if (KErrNone != err)
        {
        ERR_PRINTF1(_L("Failed to read conn preference"));
        return err;
        }
    
    INFO_PRINTF1(_L("Got conn preference"));
    
    if((err = aConn.Open(aSockServ, KAfInet)) != KErrNone )
        {
        ERR_PRINTF2(_L("Failed to open Connection: %D"),err);
        return err;
        }
        
    INFO_PRINTF1(_L("Connection opened"));
    
    INFO_PRINTF1(_L("Starting Connection"));
    err = aConn.Start(connPref);
    
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed starting Connection: %D"),err);
        aConn.Close();
        return err;
        }
    
    INFO_PRINTF1(_L("Connection Started"));
    
    INFO_PRINTF1(_L("StartConnection - Exit"));
    
    return KErrNone;    
    }

   

void CDNSSuffixTestWrapper::GetConnPrefL(TUint aIapId,TCommDbConnPref& aPref)
    {
    INFO_PRINTF1(_L("GetConnPrefL - Entry"));
    
    INFO_PRINTF1(_L("Instantiating CCommsDatabase"));
    CCommsDatabase* commsDb = CCommsDatabase::NewL();
    CleanupStack::PushL(commsDb);
    
    INFO_PRINTF1(_L("Calling OpenConnectionPrefTableLC"));
    CCommsDbConnectionPrefTableView* commsDbView = commsDb->OpenConnectionPrefTableLC();
    
    INFO_PRINTF1(_L("Going to first record"));
    User::LeaveIfError(commsDbView->GotoFirstRecord());
    
    TInt isFound(KErrNotFound);
    do
        {
        CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref pref;
        INFO_PRINTF1(_L("Reading conn preferecne."));
        commsDbView->ReadConnectionPreferenceL(pref);
        
        if (pref.iBearer.iIapId == aIapId)
            {
            INFO_PRINTF1(_L("Match found"));
            aPref.SetIapId(aIapId);
            aPref.SetBearerSet(pref.iBearer.iBearerSet);
            aPref.SetDirection(pref.iDirection);
            aPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
            isFound = KErrNone;
            break;
            }
        } while(!commsDbView->GotoNextRecord());    
    
    INFO_PRINTF1(_L("Cleanup resources"));
    CleanupStack::PopAndDestroy(2, commsDb); // commsDbView, commsDb
    
    User::LeaveIfError(isFound);
    
    INFO_PRINTF1(_L("GetConnPrefL - Exit"));    
    }



TInt32 CDNSSuffixTestWrapper::GetFirstIapId()
    {
    INFO_PRINTF1(_L("GetFirstIapId - Entry"));
    
    TInt iapId(KErrNotFound);
    
    if (!GetIntFromConfig(KSectionDNSSuffixFirstInterface,KKeyDNSSuffixIAPId,iapId))
         {
         ERR_PRINTF1(_L("Failed to read the First IAP id"));
         }
      else
         {
         INFO_PRINTF2(_L("Got the IAP: %D"),iapId);
         }
    
    INFO_PRINTF1(_L("GetFirstIapId - Exit"));
    return iapId;
    }


TInt32 CDNSSuffixTestWrapper::GetSecondIapId()
   {
   INFO_PRINTF1(_L("GetSecondIapId - Entry"));
   
   TInt iapId(KErrNotFound);
   
   if (!GetIntFromConfig(KSectionDNSSuffixSecondInterface,KKeyDNSSuffixIAPId,iapId))
        {
        ERR_PRINTF1(_L("Failed to read second IAP id"));
        }
     else
        {
        INFO_PRINTF2(_L("Got the IAP: %D"),iapId);
        }
   
   INFO_PRINTF1(_L("GetSecondIapId - Exit"));
   return iapId;
   }
 

TInt CDNSSuffixTestWrapper::GetFirstInterfaceNameL(TDes& aIfaceName)
    {
    INFO_PRINTF1(_L("GetFirstInterfaceNameL - Entry"));
    TInt err(KErrNone);    
    
    TPtrC interfaceName;
    if (!GetStringFromConfig(KSectionDNSSuffixFirstInterface,KKeyInterfaceName,interfaceName))
        {
        ERR_PRINTF1(_L("Failed to read the interface name from config"));
        err = KErrNotFound;
        SetError(err);
        return err;
        }
        
    err = GetInterfaceNameL(interfaceName,aIfaceName);
    
    INFO_PRINTF1(_L("GetFirstInterfaceNameL - Exit"));
    return err;
    }
    

TInt CDNSSuffixTestWrapper::GetSecondInterfaceNameL(TDes& aIfaceName)
    {    
    INFO_PRINTF1(_L("GetSecondInterfaceNameL - Entry"));
    TInt err(KErrNone);    
    
    TPtrC interfaceName;
    if (!GetStringFromConfig(KSectionDNSSuffixSecondInterface,KKeyInterfaceName,interfaceName))
        {
        ERR_PRINTF1(_L("Failed to read the interface name from config"));
        err = KErrNotFound;
        SetError(err);
        return err;
        }    
    
    err = GetInterfaceNameL(interfaceName,aIfaceName);
    
    INFO_PRINTF1(_L("GetSecondInterfaceNameL - Exit"));
    return err;    
    }

    
TInt CDNSSuffixTestWrapper::GetInterfaceNameL(const TDesC& aCriteria,TDes& aIfaceName)
    {
    INFO_PRINTF1(_L("GetInterfaceNameL - Entry"));

    TInt err = KErrNone;
    CDesCArray* interfaceNameArray = new(ELeave) CDesCArrayFlat(4);
    CleanupStack::PushL(interfaceNameArray);
    TRAP(err,GetAvaiableInterfacesL(interfaceNameArray));
    
    if (KErrNone != err)
        {       
        ERR_PRINTF2(_L("Failed to enumeate the interfaces with error: %D"),err);
        return err;
        }
    
    INFO_PRINTF2(_L("No of interfaces: %D"),interfaceNameArray->Count());
    
    if (interfaceNameArray->Count() <= 0)
       {
       INFO_PRINTF1(_L("No interfaces available:"));
       CleanupStack::PopAndDestroy(interfaceNameArray);
       err = KErrNotFound;
       SetError(err);
       return err;
       }
    
    TBool isInterfaceFound(EFalse);
    TInt index = -1;
   
    for (TInt a=0; a<interfaceNameArray->Count();a++)
       {
       HBufC* interfaceNameBuf = (*interfaceNameArray)[a].AllocL();
       
       TInt pos = interfaceNameBuf->FindF(aCriteria);
       
       delete interfaceNameBuf;
       
       if ( KErrNotFound != pos)
           {
           isInterfaceFound = ETrue;
           index = a;
           break;
           }
       }
    
    if (!isInterfaceFound)
        {
        ERR_PRINTF1(_L("Interface not found"));
        CleanupStack::PopAndDestroy(interfaceNameArray);
        err = KErrNotFound;
        SetError(err);
        return err;
        }    
    
    aIfaceName.Copy((*interfaceNameArray)[index]);
    
    CleanupStack::PopAndDestroy(interfaceNameArray);
    
    TPtr tempName((TUint16*)aIfaceName.Ptr(),aIfaceName.Length(),aIfaceName.Length());
    INFO_PRINTF2(_L("Found Interface and name is: %S"),&tempName);
    
    INFO_PRINTF1(_L("GetInterfaceNameL - Exit"));
    return KErrNone;
    }

/**
 * DoInitTestingL. Configures the initial connections. 
 * Function to start the connection to configure the named server on one interface. 
 */
void CDNSSuffixTestWrapper::DoInitTestingL()
    {
    INFO_PRINTF1(_L("********************************************"));
    INFO_PRINTF1(_L("DoInitTestingL"));
    INFO_PRINTF1(_L("********************************************"));

    /*@Note, We need to configure the named servers on vTunnel and ethernet interfaces.
     * So, Configuring them by starting connections on both interfaces. 
     * 
     */
    TInt err = KErrNone;
    
    if((err = iSocketServ.Connect()) == KErrNone )
        INFO_PRINTF1(_L("Socket Server Opened."));
    else
        {
        ERR_PRINTF2(_L("Error Opening Socket Server: %d"), err);
        SetError(err);
        return;
        }
    
    ERR_PRINTF2(_L("Error Opening Socket Server: %d"), err);
    
    RConnection firstConn; 
    RConnection vTunConn; // Usef for virtual Tunnel interface
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed to start the Connections: %D"),err);
        CloseSocketSrv();
        SetError(err);
        return;
        }
    INFO_PRINTF1(_L("Started the connections successfully"));
    
    INFO_PRINTF1(_L("Close the connections"));
   
    if (vTunConn.SubSessionHandle())
        vTunConn.Close();

    if (firstConn.SubSessionHandle())
        firstConn.Close();
    
    SetError(err);      
    CloseSocketSrv();    
    }

/**
 * @DoEnumerateInterfacesL. Enumerates the all available interfaces.
 */
void CDNSSuffixTestWrapper::DoEnumerateInterfacesL()
    {
    INFO_PRINTF1(_L("********************************************"));
    INFO_PRINTF1(_L("DoEnumerateInterfacesL"));
    INFO_PRINTF1(_L("********************************************"));
    
    CDesCArray* interfaceNameArray = new(ELeave) CDesCArrayFlat(4);
    CleanupStack::PushL(interfaceNameArray);
    TRAPD(err,GetAvaiableInterfacesL(interfaceNameArray));
   
    if (KErrNone == err)
        {
        // Print the interface names        
        INFO_PRINTF2(_L("Found %D interfaces"),interfaceNameArray->Count());
        
        for(TInt a=0;a<interfaceNameArray->Count();a++)
            {
            TPtrC interfaceName = (*interfaceNameArray)[a];
            INFO_PRINTF2(_L("Interface: %S"),&interfaceName);
            }
        }
    else
        {
        ERR_PRINTF2(_L("Failed to enumeate the interfaces with error: %D"),err);
        }
    
    CleanupStack::PopAndDestroy(interfaceNameArray);
    
    SetError(err);
    }

/*
 * GetAvaiableInterfacesL. Function to get the names of all available names of  all interfaces.
 * @arg aInterfaceNamesArray (out param), Array of interface names owned by caller. 
 */
void CDNSSuffixTestWrapper::GetAvaiableInterfacesL(MDesCArray* aInterfaceNamesArray)
    {
    INFO_PRINTF1(_L("GetAvaiableInterfacesL - Entry"));
    
    CDesCArray* interfacesArray = static_cast<CDesCArray*>(aInterfaceNamesArray);
    interfacesArray->Reset();
    
    TInt err = KErrNone;
    RSocketServ socketSrv;
    
    if ( (err = socketSrv.Connect()) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Socet server error: %D"),err);
        SetError(err);
        return;
        }
    INFO_PRINTF1( _L("Connected to Socket server"));
    
    INFO_PRINTF1(_L("Opening socket..."));
    
    RSocket socket;
    err = socket.Open(socketSrv,KAfInet, KSockDatagram, KProtocolInetUdp);
    
    if (KErrNone != err)
        {
        INFO_PRINTF2(_L("Failed Opening socket with error: %D"),err);
        SetError(err);
        socketSrv.Close();
        return;
        }
    
    INFO_PRINTF1(_L("Socket opened"));
    
    err = socket.SetOpt(KSoInetEnumInterfaces,KSolInetIfCtrl);
  
    if (KErrNone != err)
        {
        INFO_PRINTF2(_L("Failed enumerating interfaces with error: %D"),err);
        SetError(err);
        socket.Close();
        socketSrv.Close();
        return;
        }
    
    TSoInetInterfaceInfo info;
    TPckg<TSoInetInterfaceInfo> opt(info);
    
    while(socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
        {
       // INFO_PRINTF2(_L("Interface Name %S"),&opt().iName);
        interfacesArray->AppendL(opt().iName);
        }
    
    INFO_PRINTF1(_L("Finished"));
    
    INFO_PRINTF1(_L("Closing connections"));
    socket.Close();
    socketSrv.Close();
    SetError(KErrNone);
    INFO_PRINTF1(_L("GetAvaiableInterfacesL - Exit"));
    }


/**
Function test the setting of suffix list and getting suffix list on interface.
*/
void CDNSSuffixTestWrapper::DoSetAndGetSuffixListL()
    {
    INFO_PRINTF1(_L("************************************************************"));
    INFO_PRINTF1(_L("DoSetAndGetSuffixListL - DNS_Suffix_Support_TC001"));
    INFO_PRINTF1(_L("************************************************************"));
        
    TInt err(KErrNone);
    
    TName interfaceName;
    
    TRAP(err, err = GetFirstInterfaceNameL(interfaceName));
        
    if (err != KErrNone )
        {
        ERR_PRINTF2(_L("Failed to get the interface error: %D"),err);
        SetError(err);
        return;
        }
    
    TPtrC suffixList;
    iSuffixList.Reset();
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest001,KKeyDNSSuffixTestSuffixList,suffixList))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Suffix list: %S"),&suffixList);
        FillSuffixList(iSuffixList,suffixList);
        }
            
    err = iSocketServ.Connect();
           
    if(KErrNone != err)
       {
       ERR_PRINTF1(_L("Failed to connect socket Server"));
       SetError(err);
       return;
       }
    
    INFO_PRINTF1(_L("Connected to socket Server"));
    
    TUint iapId = GetFirstIapId();
    
    if (KErrNotFound == (TInt)iapId)
       {
       ERR_PRINTF2(_L("Failed to get the iap id: %D"),err);
       SetError(err);
       CloseSocketSrv();
       return;
       }
    
    RConnection firstConn; 
    err = StartConnection(iSocketServ,firstConn,iapId);
    
    if (KErrNone != err)
      {
      ERR_PRINTF2(_L("Failed to start the Connections: %D"),err);
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    INFO_PRINTF1(_L("Started the connections successfully"));
    
    
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,interfaceName,iSuffixList);
    
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed Setting the suffix list with errror : %D"),err);
        SetError(err);  
        firstConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF1(_L("Getting the suffix list on interface:"));
    
    iSuffixList2.Reset();
    
    err = GetDNSSuffixList(iSocketServ,interfaceName,iSuffixList2);
        
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed to get the suffix list with errror : %D"),err);
        }
    
    
    if (err == KErrNone)
        {        
        if ( (err = IsSuffixListEqual(iSuffixList,iSuffixList2)) == KErrNone)
            {
            INFO_PRINTF1(_L("Suffix list are same and Test is successfull"));
            }
        else
            {
            ERR_PRINTF1(_L("Suffix list are not same and Test failed"));
            }
        }
    
    SetError(err);
    
    firstConn.Close();
    CloseSocketSrv();
    
    INFO_PRINTF1(_L("Set and Get Suffix list Test - Finished"));
    }



TInt CDNSSuffixTestWrapper::SetDNSSuffixListOnInterface(RSocketServ& aServ,RConnection& /*aConn*/,const TDesC& aInterfaceName,RInetSuffixList& aData)
    {
    INFO_PRINTF1(_L("SetDNSSuffixListOnInterfaceL - Entry"));
    
    INFO_PRINTF1(_L("Opening socket..."));    
       
    RSocket socket;
    TInt err = socket.Open(aServ,KAfInet, KSockDatagram, KProtocolInetUdp);    
    
    if (KErrNone != err)
        {
        INFO_PRINTF2(_L("Failed Opening socket with error: %D"),err);
        SetError(err);
        return err;
        }
  
    INFO_PRINTF1(_L("Socket opened"));
    
    err = socket.SetOpt(KSoInetEnumInterfaces,KSolInetIfCtrl);
    
    if (KErrNone != err)
        {
        INFO_PRINTF2(_L("Failed enumerating interfaces with error: %D"),err);
        SetError(err);        
        socket.Close();
        return err;
        }
    
	TSoInetInterfaceInfo getOpt;
    TPckg<TSoInetInterfaceInfo> opt_out(getOpt);
    
    TBool isSuffixListSet(EFalse);
    
    while(socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt_out)== KErrNone)        
        {    
        
        INFO_PRINTF2(_L("Interface Name: %S"),&opt_out().iName);        
        INFO_PRINTF2(_L("Name Server: %D"),!opt_out().iNameSer1.IsUnspecified());
        
        TBuf<KIPAddrLen> ipBuf;
        opt_out().iAddress.Output(ipBuf);
        INFO_PRINTF2(_L("IP Address: %S"),&ipBuf);

        TBuf<KIPAddrLen> nameserBuf;
        opt_out().iNameSer1.Output(nameserBuf);
        INFO_PRINTF2(_L("Name Server IP: %S"),&nameserBuf);

        if( (opt_out().iName.Length()>0) /*&& !(opt_out().iNameSer1.IsUnspecified())*/ && (aInterfaceName.CompareF(opt_out().iName)==0) )       
            {
            INFO_PRINTF2(_L("Setting suffix list on Interface: %S"),&opt_out().iName);
            INFO_PRINTF2(_L("MTU on the interface is : %d"), opt_out().iMtu);

            TSoInetInterfaceInfoExtnDnsSuffix setOpt;
            (TSoInetInterfaceInfo &)setOpt = opt_out();
            TPckg<TSoInetInterfaceInfoExtnDnsSuffix> opt_in(setOpt);
            /*opt_in().iName = opt_out().iName;
            opt_in().iAddress = opt_out().iAddress;
            opt_in().iNetMask = opt_out().iNetMask;*/
            opt_in().iDoId = ETrue;
            opt_in().iDelete = EFalse;
            opt_in().iDomainSuffix.DeleteAll();
            
            err = socket.SetOpt(KSoInetConfigInterface,KSolInetIfCtrl,opt_in);
            
            if (err != KErrNone)
                {
                ERR_PRINTF2(_L("Failed to reset suffix list on the interface: %D"),err);
                break;
                }
            else
	            INFO_PRINTF1(_L("Interface domain suffix list reset successful"));
            	
            if (aData.Count() == 0) // We plan to set empty record, so only reset is sufficient
                isSuffixListSet = ETrue;
            
            for(TInt a=0;a<aData.Count();a++)
                {
                opt_in().iDomainSuffix.Copy(aData[a]);
                err = socket.SetOpt(KSoInetConfigInterface,KSolInetIfCtrl,opt_in);
                if (err != KErrNone)
                	{
                	ERR_PRINTF2(_L("Failed to set suffix list: %D"),err);
                	break;
            		}
                else
                	{
	                INFO_PRINTF1(_L("Suffix list added successfully"));
                	isSuffixListSet = ETrue;
            		}
                }
            break;
            }
        }
    
    if (!isSuffixListSet)
        {
        err = KErrNotFound;
        ERR_PRINTF2(_L("Failed to set suffix list with error: %D"),err);       
        }
        
    SetError(err);
    
    INFO_PRINTF1(_L("Closing socket"));
    
    socket.Close();
    INFO_PRINTF1(_L("SetDNSSuffixListOnInterfaceL - Exit"));
    return err;
    }



/**
 * GetDNSSuffixList. Gets the suffix list set on an interface.
 * 
 * @arg  aServ, session handle to a socket server. 
 * @arg  aData (in/out param) Data having the suffix list. Packed buffer of type TInetSuffixList
 *
 */
TInt CDNSSuffixTestWrapper::GetDNSSuffixList(RSocketServ& aServ,const TDesC& aInterfaceName,RInetSuffixList& aData)
    {
    INFO_PRINTF1(_L("GetDNSSuffixList - Entry"));
    
    INFO_PRINTF1(_L("Opening socket..."));
    RSocket socket;
    TInt err = socket.Open(aServ,KAfInet, KSockDatagram, KProtocolInetUdp);
    
    if (KErrNone != err)
       {
       INFO_PRINTF2(_L("Failed Opening socket with error: %D"),err);
       SetError(err);
       return err;
       }
    
    INFO_PRINTF1(_L("Socket opened"));
    
    err = socket.SetOpt(KSoInetEnumInterfaces,KSolInetIfCtrl);
    
    if (KErrNone != err)
       {
       INFO_PRINTF2(_L("Failed enumerating interfaces with error: %D"),err);
       SetError(err);
       socket.Close();
       return err;
       }

	TSoInetInterfaceInfo getOpt;
    TPckg<TSoInetInterfaceInfo> opt(getOpt);
       
    while(socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (aInterfaceName.CompareF(opt().iName) == KErrNone)
			{
			INFO_PRINTF2(_L("Reading suffix list from interface: %S"),&opt().iName);     

			TBuf<KIPAddrLen> tempIp;
			opt().iAddress.Output(tempIp);
			INFO_PRINTF2(_L("IP: %S"),&tempIp);

			TInetSuffix domainSuffix;
			TPckg<TInetSuffix> dom_opt(domainSuffix);
			if (err = socket.SetOpt(KSoInetEnumDomainSuffix, KSolInetIfCtrl) != KErrNone)
				return err; 
			while( socket.GetOpt(KSoInetNextDomainSuffix, KSolInetIfCtrl, dom_opt) == KErrNone)
				{
                aData.AppendL(dom_opt().iSuffixName);
				}
			break;
			}
		}
    
    INFO_PRINTF1(_L("Closing socket"));
    socket.Close();

    INFO_PRINTF1(_L("GetDNSSuffixList - Exit"));
    return KErrNone;
    }



/**
 * FillSuffixListL, Function to fill the suffix names in the TInetSuffixList object. 
 * @arg aSuffixList, out param, on return has the suffix list set in it.
 * @arg aData, has the list of domain suffix names in the form of abc.xyz.com;efg.xyz.com
 */
TInt CDNSSuffixTestWrapper::FillSuffixList(RInetSuffixList& aSuffixList,TDesC& aData)
    {
    INFO_PRINTF1(_L("FillSuffixList - Entry"));
    
    INFO_PRINTF2(_L("Processing Suffix list: %S"),&aData);
    
    TPtr data((TUint16*)aData.Ptr(),aData.Length(),aData.Length());
    data.TrimAll();
    
    TInt pos = KErrNone;
    while( (pos = data.Find(_L(";"))) != KErrNotFound)
        {
        TPtr suffixItem((TUint16*)data.Left(pos).Ptr(),pos,pos);
        INFO_PRINTF3(_L("Processing %D suffix name: %S"),/*sListBuf()*/iSuffixList.Count()+1,&suffixItem);
        
        TSuffixName tmpBuf;
        tmpBuf.Copy(suffixItem);
        aSuffixList.AppendL(tmpBuf);
        data.Copy(data.Mid(pos+1));
        }
    data.TrimAll();
    INFO_PRINTF3(_L("Processing %D suffix name: %S"),/*sListBuf()*/iSuffixList.Count()+1,&data);
    INFO_PRINTF2(_L("Data Len: %D"),data.Length());
    
    TSuffixName tmpBuf;
    tmpBuf.Copy(data);
    aSuffixList.AppendL(tmpBuf);
    
    INFO_PRINTF1(_L("FillSuffixList - Exit"));
    
    return aSuffixList.Count() == 0 ? KErrNotFound : KErrNone;
    }


void CDNSSuffixTestWrapper::CloseSocketSrv()
    {
    INFO_PRINTF1(_L("Closing socket server"));
    
    if (iSocketServ.Handle())
        {
        iSocketServ.Close();
        INFO_PRINTF1(_L("Closed Sock Srv successfully"));
        }
    else
        {
        INFO_PRINTF1(_L("Inavild Sock Srv handle"));
        }
    }

/* 
 * IsSuffixListEqual, Utility function, compares the two suffix list structures. 
 * @arg aSuffixList1 First structure holding the suffix list
 * @arg aSuffixList2 second structure holding the suffix list
 * 
 * @return KErrNone if both suffix list strucures same otherwise KErrArgument.  
 * 
 */
TInt CDNSSuffixTestWrapper::IsSuffixListEqual(const RInetSuffixList& aSuffixList1, const RInetSuffixList& aSuffixList2)
    {
    INFO_PRINTF1(_L("IsSuffixListEqual - Entry"));
    
    if (aSuffixList1.Count() != aSuffixList2.Count())
        {
        return KErrArgument;
        }
    
    for (TInt a=0; a<aSuffixList1.Count();a++)
        {
        if (aSuffixList1[a].CompareF(aSuffixList2[a]) != KErrNone)
            {
            return KErrArgument;
            }
        }
    INFO_PRINTF1(_L("IsSuffixListEqual - Exit"));
    return KErrNone;
    }


/*
 * Prerequisite: Two interfaces required to be up and running (virtual Tunnel and ethernet).
 * 
 * DoResolveL. Resolves the domain name
 * 
 * 
 */
void CDNSSuffixTestWrapper::DoResolveL()
    {
    INFO_PRINTF1(_L("********************************************"));
    INFO_PRINTF1(_L("DoResolveL  - DNS_Suffix_Support_TC002"));
    INFO_PRINTF1(_L("********************************************"));
    
    TPtrC hostname;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest002,KKeyDNSSuffixTestHostname,hostname))
        {
        ERR_PRINTF1(_L("Failed to read the host name value"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name: %S"),&hostname);       
        }

	TInt err = KErrNone;
    TName interfaceName;
    
    TRAP(err, err = GetFirstInterfaceNameL(interfaceName));
        
    if (err != KErrNone )
        {
        ERR_PRINTF2(_L("Failed to get the interface error: %D"),err);
        SetError(err);
        return;
        }
        
    TPtrC suffixList;
    iSuffixList.Reset();
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest002,KKeyDNSSuffixTestSuffixList,suffixList))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Suffix list: %S"),&suffixList);
        FillSuffixList(iSuffixList,suffixList);
        }

    if ( (err = iSocketServ.Connect()) != KErrNone)
        {
        ERR_PRINTF1(_L("Failed Connecting to socket server"));
        SetError(err);
        return;
        }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    /*
     * @Note: Start more than one interface. 
     * Here starting vitual tunnel nif interface as a default. @see ethernet ced.xml file in which I have made 
     * vTun as default in pref table.
     */ 
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
   
    if (err != KErrNone)
        {
        ERR_PRINTF1(_L("Failed to start connections"));
        CloseSocketSrv();
        SetError(err);
        return;
        }
    
    INFO_PRINTF1(_L("Connections started successfully"));
   
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,interfaceName,iSuffixList);
    
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed Setting the suffix list with errror : %D"),err);
        SetError(err);  
        firstConn.Close();
        CloseSocketSrv();
        return;
        }

    // Open implicit resolver.
    RHostResolver resolver;
    
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);        
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    INFO_PRINTF2(_L("Requesting to resolve hostname: %S"),&hostname);
    
    if ( (err = resolver.GetByName(hostname,nameEntry)) != KErrNone)
      {
      ERR_PRINTF3(_L("Failed host resolution for %S with error: %D"),&hostname,err);       
      }
    else
      {
      TInetAddr& netAddress = TInetAddr::Cast(nameEntry().iAddr);
      
      if (netAddress.IsUnspecified())
          {
          ERR_PRINTF1(_L("Invalid host address"));
          err = KErrArgument;
          }
      else
          {
          TBuf<KIPAddrLen> address;
          netAddress.Output(address);
          INFO_PRINTF1(_L("Got valid IP address:"));
          INFO_PRINTF2(_L("Host name: %S"),&nameEntry().iName);
          INFO_PRINTF2(_L("Host address: %S"),&address);
          }
      }
    
    INFO_PRINTF1(_L("Closing host resolver"));
    resolver.Close();

    SetError(err);
    
    INFO_PRINTF1(_L("Closing connections and socket serv session"));
    firstConn.Close();
    vTunConn.Close();
    CloseSocketSrv();           
    }



void CDNSSuffixTestWrapper::DoResolveHostWithoutDomainWithSuffixListSetL()
    {
    INFO_PRINTF1(_L("******************************************************"));
    INFO_PRINTF1(_L("DoResolveHostWithoutDomainL with Domain suffix Set"));
    INFO_PRINTF1(_L(" - DNS_Suffix_Support_TC003"));
    INFO_PRINTF1(_L("******************************************************"));
    
    TPtrC hostname;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest003,KKeyDNSSuffixTestHostname,hostname))
        {
        ERR_PRINTF1(_L("Failed to read the host name value"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name: %S"),&hostname);       
        }
    
    TInt err = KErrNone;
    
    if ( (err = iSocketServ.Connect()) != KErrNone)
        {
        ERR_PRINTF1(_L("Failed Connecting to socket server"));
        SetError(err);
        return;
        }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
        
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
       {
       ERR_PRINTF1(_L("Failed to start connections"));
       CloseSocketSrv();
       SetError(err);
       return;
       }

   INFO_PRINTF1(_L("Connections started successfully"));
       
    // Open explicit resolver with ethernet
    RHostResolver resolver;
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp,firstConn)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    if ( (err = resolver.GetByName(hostname,nameEntry)) != KErrNone)
      {
      ERR_PRINTF3(_L("Failed host resolution for %S with error: %D"),&hostname,err);
      INFO_PRINTF1(_L("Test success"));
      SetError(err);
      }
    else
      {
      TInetAddr address = nameEntry().iAddr;
      
      if (address.IsUnspecified())
          {
          ERR_PRINTF1(_L("Invalid host address"));
          err = KErrArgument;
          }
      else
          {
          TBuf<KIPAddrLen> ipAddr;
          address.Output(ipAddr);
          INFO_PRINTF1(_L("Got valid IP address:"));
          INFO_PRINTF2(_L("Host name: %S"),&nameEntry().iName);
          INFO_PRINTF2(_L("Host address: %S"),&ipAddr);
          }
      SetError(err);
      }
    
    INFO_PRINTF1(_L("Closing host resolver"));
    resolver.Close();
    firstConn.Close();
    vTunConn.Close();   
    CloseSocketSrv();           
    }


void CDNSSuffixTestWrapper::DoResolveHostWithoutDomainWithoutSuffixListL()
    {
    INFO_PRINTF1(_L("******************************************************"));
    INFO_PRINTF1(_L("DoResolveHostWithoutDomainL  With no Suffix list"));
    INFO_PRINTF1(_L("DNS_Suffix_Support_TC004"));
    INFO_PRINTF1(_L("******************************************************"));
    
    TPtrC hostname;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest004,KKeyDNSSuffixTestHostname,hostname))
        {
        ERR_PRINTF1(_L("Failed to read the host name value"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name: %S"),&hostname);       
        }
    
    TInt err(KErrNone);
    // Note: Remove the suffix list set on Ethernet interface.
  
    TName interfaceName;
    err = GetFirstInterfaceNameL(interfaceName);   
   
    if ( (err = iSocketServ.Connect()) != KErrNone)
        {
        ERR_PRINTF1(_L("Failed Connecting to socket server"));
        SetError(err);
        return;
        }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
        
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn;
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
      {
      ERR_PRINTF1(_L("Failed to start connections"));
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    /*
    * @Note: Remove the suffix list (which we set in TC001) set on the interface by just setting empty list on it.
    */
    INFO_PRINTF1(_L("Setting empty suffix list on interface:"));
    iSuffixList.Reset();
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,interfaceName,iSuffixList);
    
    if (err != KErrNone)
        {
        ERR_PRINTF2(_L("Failed to remove the suffix list set on interface with error: %D"),err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF1(_L("Removed suffix list from interface:"));
    
    // Open explicit resolver with ethernet
    RHostResolver resolver;
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp,firstConn)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    if ( (err = resolver.GetByName(hostname,nameEntry)) != KErrNone)
      {
      ERR_PRINTF3(_L("Failed host resolution for %S with error: %D"),&hostname,err);
      INFO_PRINTF1(_L("Test success"));
      SetError(EPass);
      }
    else
      {
      TInetAddr address = nameEntry().iAddr;
      
      if (address.IsUnspecified())
          {
          ERR_PRINTF1(_L("Invalid host address"));
          err = KErrArgument;
          }
      else
          {
          TBuf<KIPAddrLen> ipAddr;
          address.Output(ipAddr);
          INFO_PRINTF1(_L("Got valid IP address:"));
          INFO_PRINTF2(_L("Host name: %S"),&nameEntry().iName);
          INFO_PRINTF2(_L("Host address: %S"),&ipAddr);
          }
      SetError(EFail);
      }
    
    INFO_PRINTF1(_L("Closing host resolver"));
    resolver.Close();
    firstConn.Close();
    vTunConn.Close();   
    CloseSocketSrv();      
    INFO_PRINTF1(_L("DoResolveHostWithoutDomainWithoutSuffixListL - Exit"));
    }



void CDNSSuffixTestWrapper::DNSSuffixSupportTC005L()
    {
    INFO_PRINTF1(_L("****************************************************"));
    INFO_PRINTF1(_L("DNSSuffixSupportTC005L - DNS_Suffix_Support_TC005"));
    INFO_PRINTF1(_L("****************************************************"));
    
    TPtrC hostname;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest005,KKeyDNSSuffixTestHostname,hostname))
        {
        ERR_PRINTF1(_L("Failed to read the host name value"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name: %S"),&hostname);       
        }
    
    TInt err(KErrNone);
        
    TFileName interfaceName;
    
    TRAP(err, err = GetFirstInterfaceNameL(interfaceName));
    
    if (err != KErrNone )
        {
        ERR_PRINTF2(_L("Failed to get the interface error: %D"),err);
        SetError(err);
        return;
        }
    
    TPtrC suffixList;    
    iSuffixList.Reset();
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest005,KKeyDNSSuffixTestSuffixList,suffixList))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Suffix list: %S"),&suffixList);
        FillSuffixList(iSuffixList,suffixList);
        }
     
    if ( (err = iSocketServ.Connect()) != KErrNone)
        {
        ERR_PRINTF1(_L("Failed Connecting to socket server"));
        SetError(err);
        return;
        }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
       {
       ERR_PRINTF1(_L("Failed to start connections"));
       CloseSocketSrv();
       SetError(err);
       return;
       }
    
    INFO_PRINTF1(_L("Connections started successfully"));
    
    INFO_PRINTF2(_L("Setting suffix list on : %S"),&interfaceName);
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,interfaceName,iSuffixList);    
        
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed Setting the suffix list with errror : %D"),err);
        SetError(err);    
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF1(_L("Suffix list set successfully"));
    
    INFO_PRINTF1(_L("Creating explicit host resolver"));
    
    // Open explicit resolver
    RHostResolver resolver;
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp,firstConn)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    if ( (err = resolver.GetByName(hostname,nameEntry)) != KErrNone)
        {
        INFO_PRINTF3(_L("Host resolution for %S with error: %D"),&hostname,err);
        INFO_PRINTF1(_L("Test success"));
        err = KErrNone;
        }    
    else
        {
        err = KErrArgument;
        ERR_PRINTF1(_L("Test failed: Expected no resolution, but resolved"));
        }
    
    SetError(err);
    INFO_PRINTF1(_L("Closing host resolver, connections and Sock Srv session"));
    resolver.Close();
    firstConn.Close();
    vTunConn.Close();    
    CloseSocketSrv();
    INFO_PRINTF1(_L("CDNSSuffixTestWrapper::DNSSuffixSupportTC005L - Exit"));
    }


void CDNSSuffixTestWrapper::DNSSuffixSupportTC006L()
    {
    INFO_PRINTF1(_L("****************************************************"));
    INFO_PRINTF1(_L("DNSSuffixSupportTC006L - DNS_Suffix_Support_TC006"));
    INFO_PRINTF1(_L("****************************************************"));
    
    TInt numberOfHosts(0);
    
    if (!GetIntFromConfig(KSectionDNSSuffixTest006,KKeyDNSSuffixTest006NoOfHostname,numberOfHosts))
        {
        ERR_PRINTF1(_L("Failed to read the no of hosts from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("No of hosts: %D"),numberOfHosts);       
        }       

    TPtrC hostnames[KMaxHosts];
    
    for (TInt b=0;b<numberOfHosts;b++)
        {
        TFileName keyName;
        keyName.Format(KKeyDNSSuffixTest006Hostname,b+1);
        
        if (!GetStringFromConfig(KSectionDNSSuffixTest006,keyName,hostnames[b]))
            {
            ERR_PRINTF1(_L("Failed to read the 1st host name value"));    
            SetError(KErrNotFound);
            return;
            }
        else
            {
            INFO_PRINTF3(_L("Host name%D: %S"),b,&hostnames[b]);       
            }        
        }
        
    TInt err(KErrNone);
        
    TName interfaceName;
    
    TRAP(err, err = GetFirstInterfaceNameL(interfaceName));
    
    if (err != KErrNone )
        {
        ERR_PRINTF2(_L("Failed to get the interface error: %D"),err);
        SetError(err);
        return;
        }
    
    TPtrC suffixList;
    iSuffixList.Reset();
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest006,KKeyDNSSuffixTestSuffixList,suffixList))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Suffix list: %S"),&suffixList);
        FillSuffixList(iSuffixList,suffixList);
        }
     
    if ( (err = iSocketServ.Connect()) != KErrNone)
        {
        ERR_PRINTF1(_L("Failed Connecting to socket server"));
        SetError(err);
        return;
        }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
      {
      ERR_PRINTF1(_L("Failed to start connections"));
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    INFO_PRINTF1(_L("Connections started successfully"));
    
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,interfaceName,iSuffixList);
       
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed Setting the suffix list with errror : %D"),err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }

    INFO_PRINTF1(_L("Set suffix list on interface successfully"));
    
    RHostResolver resolver;
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp,firstConn)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to Host resolver successfully"));
               
    TNameEntry nameEntry[KMaxHosts];
    
    // Resolve all three hosts
    for (TInt a=0;a<numberOfHosts;a++)
        {
        INFO_PRINTF2(_L("Resolving host: %S"),&hostnames[a]);
        
        if ( (err = resolver.GetByName(hostnames[a],nameEntry[a])) != KErrNone)
            {
            INFO_PRINTF3(_L("Host resolution for %S failed with error: %D"),&hostnames[a],err);
            INFO_PRINTF1(_L("Test Failed"));
            SetError(err);
            break;
            }
        }
    
    if (err == KErrNone)
        {
        for (TInt a=0;a<numberOfHosts;a++)
            {
            TInetAddr address = nameEntry[a]().iAddr;
            
            if (address.IsUnspecified())
                {
                ERR_PRINTF1(_L("Invalid host address"));
                err = KErrArgument;
                }
            else
                {
                TBuf<KIPAddrLen> ipAddr;
                address.Output(ipAddr);
                INFO_PRINTF1(_L("Got valid IP address:"));
                INFO_PRINTF2(_L("Host name: %S"),&nameEntry[a]().iName);
                INFO_PRINTF2(_L("Host address: %S"),&ipAddr);
                }            
            }       
        }
    
    SetError(err);
    INFO_PRINTF1(_L("Closing host resolver, connections and Sock Srv session"));
    resolver.Close();
    firstConn.Close();
    vTunConn.Close();    
    CloseSocketSrv();
    INFO_PRINTF1(_L("CDNSSuffixTestWrapper::DNSSuffixSupportTC006L - Exit"));
    }


/*
 * TODO: Needs to find the way to change the ranking IAP's programatically. 
 * Note: Before running this test case make ethernet iap as default
 * by changing its ranking to one(1). 
 * @ See te_dnssuffixcedv2.xml
 * @See te_dnssuffixsuite.script for below line
 * ced -i z:\testdata\configs\te_dnssuffixcedv2.xml 
 *
 */
void CDNSSuffixTestWrapper::DNSSuffixSupportTC007L()
    {
    INFO_PRINTF1(_L("**********************************************************************"));
    INFO_PRINTF1(_L("DNS_Suffix_Test007L - DNS_Suffix_Support_007"));
    INFO_PRINTF1(_L("**********************************************************************"));
    
    TPtrC validHostname;
    TPtrC inValidHostname;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest007,KKeyDNSSuffixTest007ValidHostname,validHostname))
        {
        ERR_PRINTF1(_L("Failed to read the valid host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Valid Host name: %S"),&validHostname);       
        }
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest007,KKeyDNSSuffixTest007InValidHostname,inValidHostname))
        {
        ERR_PRINTF1(_L("Failed to read the invalid host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Invalid Host name: %S"),&inValidHostname);       
        }   
   
    // @Santosh K Patil
    // Note: Remove the suffix lists from the interfaces set by previous test cases, if any.    
    // Note: start multiple interfaces
    
    TFileName interfaceName;
    TInt err = KErrNone;
    err = GetFirstInterfaceNameL(interfaceName);
    
    /*
     * @Note: Remove the suffix list set on the interface (which we set in earlier test cases) 
     *  by just setting empty list on it.
     */
    INFO_PRINTF1(_L("Setting empty suffix list on interface:"));
    iSuffixList.Reset();
    
    if ( (err = iSocketServ.Connect()) != KErrNone)
       {
       ERR_PRINTF1(_L("Failed Connecting to socket server"));
       SetError(err);
       return;
       }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
      {
      ERR_PRINTF1(_L("Failed to start connections"));
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    INFO_PRINTF1(_L("Connections started successfully"));
    
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,interfaceName,iSuffixList);
           
    if (KErrNone != err)
        {
        ERR_PRINTF2(_L("Failed Setting the suffix list with errror : %D"),err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF1(_L("Removed suffix list from interface:"));

    INFO_PRINTF1(_L("Opening implicit resolver"));
    // Open implicit resolver.
    RHostResolver resolver;
    
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);        
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    INFO_PRINTF2(_L("Requesting to resolve valid hostname: %S"),&validHostname);
    
    if ( (err = resolver.GetByName(validHostname,nameEntry)) != KErrNone)
      {
      ERR_PRINTF3(_L("Host resolution failed for %S with error: %D"),&validHostname,err);       
      }
    else
      {
      TInetAddr& netAddress = TInetAddr::Cast(nameEntry().iAddr);
      
      if (netAddress.IsUnspecified())
          {
          ERR_PRINTF1(_L("Invalid host address"));
          err = KErrArgument;
          }
      else
          {
          TBuf<KIPAddrLen> address;
          netAddress.Output(address);
          INFO_PRINTF1(_L("Got valid IP address:"));
          INFO_PRINTF2(_L("Host name: %S"),&nameEntry().iName);
          INFO_PRINTF2(_L("Host address: %S"),&address);
          }
      }    
    
    if (err == KErrNone)
        {
        TNameEntry nameEntry2;
        
        INFO_PRINTF2(_L("Requesting to resolve invalid hostname: %S"),&inValidHostname);
        
        if ( (err = resolver.GetByName(inValidHostname,nameEntry2)) != KErrNone)
          {
          INFO_PRINTF3(_L("Host resolution failed for %S with error: %D"),&inValidHostname,err);
          INFO_PRINTF1(_L("Test scenario passed"));
          err = KErrNone;
          }
        else
          {
          err = KErrArgument;
          INFO_PRINTF1(_L("Error: Seems host is resolvable"));
          }        
        }
    
    SetError(err);
    
    INFO_PRINTF1(_L("Closing host resolver"));
    resolver.Close();    
    
    INFO_PRINTF1(_L("Closing connections and socket serv session"));
    firstConn.Close();
    vTunConn.Close();
    CloseSocketSrv();           
    INFO_PRINTF1(_L("CDNSSuffixTestWrapper::DNSSuffixSupportTC007L - Exit"));
    }


void CDNSSuffixTestWrapper::DNSSuffixSupportTC008L()
    {
    INFO_PRINTF1(_L("**********************************************************************"));
    INFO_PRINTF1(_L("DNS_Suffix_Test008L - DNS_Suffix_Support_008"));
    INFO_PRINTF1(_L("**********************************************************************"));
    
    TPtrC hostname1;
    TPtrC hostname2;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest008,KKeyDNSSuffixTestHostname1,hostname1))
        {
        ERR_PRINTF1(_L("Failed to read the 1 host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name1: %S"),&hostname1);       
        }
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest008,KKeyDNSSuffixTestHostname2,hostname2))
        {
        ERR_PRINTF1(_L("Failed to read the 2 host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name2: %S"),&hostname2);       
        }   
   
    // @Santosh K Patil
    // @note: Set suffix lists on both interfaces (eth and vtun)    
    // @note: start multiple interfaces
    
    TName ethernetInterface;
    TName vTunInterface;
    TInt err = KErrNone;
    err = GetFirstInterfaceNameL(ethernetInterface);
    
    if (KErrNone != err)
        { 
        ERR_PRINTF1(_L("Failed get the name of the interface (ethernet)"));    
        SetError(KErrNotFound);
        return;
        }
    
    err = GetSecondInterfaceNameL(vTunInterface);
    
    if (KErrNone != err)
        { 
        ERR_PRINTF1(_L("Failed get the name of the interface (vTun)"));    
        SetError(KErrNotFound);
        return;
        }
   
    INFO_PRINTF1(_L("Setting suffix list on interface:"));
    
    iSuffixList2.Reset();
    
    TPtrC suffixList;
    if (!GetStringFromConfig(KSectionDNSSuffixTest008,KKeyDNSSuffixTestSuffixList,suffixList))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Suffix list: %S"),&suffixList);
        FillSuffixList(iSuffixList2,suffixList);
        }   
    
    if ( (err = iSocketServ.Connect()) != KErrNone)
       {
       ERR_PRINTF1(_L("Failed Connecting to socket server"));
       SetError(err);
       return;
       }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
      {
      ERR_PRINTF1(_L("Failed to start connections"));
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    INFO_PRINTF1(_L("Connections started successfully"));
    
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,ethernetInterface,iSuffixList2);
             
    if (KErrNone != err)
        {
        ERR_PRINTF3(_L("Failed Setting the suffix list on %S with errror : %D"),&ethernetInterface,err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF2(_L("Set suffix list on %S successfully"),&ethernetInterface);
    
    err = SetDNSSuffixListOnInterface(iSocketServ,vTunConn,vTunInterface,iSuffixList2);
               
    if (KErrNone != err)
        {
        ERR_PRINTF3(_L("Failed Setting the suffix list on %S with errror : %D"),&vTunInterface,err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF2(_L("Set suffix list on %S successfully"),&vTunInterface);        
    
    INFO_PRINTF1(_L("Opening implicit resolver"));
    // Open implicit resolver.
    RHostResolver resolver;
    INFO_PRINTF1(_L("Creating implicit resolver"));
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);        
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to implicit Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    INFO_PRINTF2(_L("Requesting to resolve hostname: %S"),&hostname1);
    
    if ( (err = resolver.GetByName(hostname1,nameEntry)) != KErrNone)
      {
      ERR_PRINTF3(_L("Failed to resolve the host %S with error: %D"),&hostname1,err);       
      }
    else
      {
      TInetAddr& netAddress = TInetAddr::Cast(nameEntry().iAddr);
      
      if (netAddress.IsUnspecified())
          {
          ERR_PRINTF1(_L("Invalid host address"));
          err = KErrArgument;
          }
      else
          {
          TBuf<KIPAddrLen> address;
          netAddress.Output(address);
          INFO_PRINTF1(_L("Got valid IP address:"));
          INFO_PRINTF2(_L("Host name: %S"),&nameEntry().iName);
          INFO_PRINTF2(_L("Host address: %S"),&address);
          }
      }    
    
    INFO_PRINTF1(_L("Closing implicit host resolver"));
    resolver.Close(); 
        
    if (err == KErrNone)
        {
        // Open explicit resolver.
        RHostResolver resolver2;
        INFO_PRINTF1(_L("Creating explicit resolver"));
        if ( (err = resolver2.Open(iSocketServ,KAfInet,KProtocolInetUdp,vTunConn)) != KErrNone)
            {
            ERR_PRINTF2(_L("Failed Connecting to explicit Host resolver with error: %D"),err);
            firstConn.Close();
            vTunConn.Close();
            CloseSocketSrv();
            SetError(err);        
            return;
            }       
        
        INFO_PRINTF1(_L("Connected to explicit Host resolver successfully"));
        
        
        TNameEntry nameEntry2;        
        INFO_PRINTF2(_L("Requesting to resolve hostname: %S"),&hostname2);
        
        if ( (err = resolver2.GetByName(hostname2,nameEntry2)) != KErrNone)
          {
          INFO_PRINTF3(_L("Host resolution failed for %S with error: %D"),&hostname2,err);
          INFO_PRINTF1(_L("Test scenario passed"));
          err = KErrNone;
          }
        else
          {
          err = KErrArgument;
          INFO_PRINTF1(_L("Error: Seems host is resolvable"));
          }      
        
        INFO_PRINTF1(_L("Closing explicit host resolver"));
        resolver2.Close();    
        }
    
    SetError(err);
    
    INFO_PRINTF1(_L("Closing connections and socket serv session"));
    firstConn.Close();
    vTunConn.Close();
    CloseSocketSrv();           
    INFO_PRINTF1(_L("CDNSSuffixTestWrapper::DNSSuffixSupportTC008L - Exit"));
    }


// TODO: Cross check the correct ness of the implementation.
void CDNSSuffixTestWrapper::DNSSuffixSupportTC009L()
    {
    INFO_PRINTF1(_L("**********************************************************************"));
    INFO_PRINTF1(_L("DNSSuffixSupportTC009L - DNS_Suffix_Support_009"));
    INFO_PRINTF1(_L("**********************************************************************"));
    
    TPtrC hostname1;
    TPtrC hostname2;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest009,KKeyDNSSuffixTestHostname1,hostname1))
        {
        ERR_PRINTF1(_L("Failed to read the 1 host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name1: %S"),&hostname1);       
        }
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest009,KKeyDNSSuffixTestHostname2,hostname2))
        {
        ERR_PRINTF1(_L("Failed to read the 2 host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name2: %S"),&hostname2);       
        }
    
    /*
     * @Santosh K Patil        
     * @note: start multiple interfaces
     * Set suffix lists on vTunnel interface    
     * Open explicit connection on other interface
     */
    
    TFileName vTunInterface;
    TInt err = GetSecondInterfaceNameL(vTunInterface);
    
    if (KErrNone != err)
        { 
        ERR_PRINTF1(_L("Failed get the name of the interface (vTun)"));    
        SetError(KErrNotFound);
        return;
        }    
   
    INFO_PRINTF1(_L("Setting suffix list on vTun interface:"));
    
    iSuffixList.Reset();
    
    TPtrC suffixList;
    if (!GetStringFromConfig(KSectionDNSSuffixTest009,KKeyDNSSuffixTestSuffixList,suffixList))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Suffix list: %S"),&suffixList);
        FillSuffixList(iSuffixList,suffixList);
        }
    
    if ( (err = iSocketServ.Connect()) != KErrNone)
       {
       ERR_PRINTF1(_L("Failed Connecting to socket server"));
       SetError(err);
       return;
       }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
      {
      ERR_PRINTF1(_L("Failed to start connections"));
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    INFO_PRINTF1(_L("Connections started successfully"));
    
    err = SetDNSSuffixListOnInterface(iSocketServ,vTunConn,vTunInterface,iSuffixList);
                   
    if (KErrNone != err)
        {
        ERR_PRINTF3(_L("Failed Setting the suffix list on %S with errror : %D"),&vTunInterface,err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF2(_L("Set suffix list on %S successfully"),&vTunInterface);  

    INFO_PRINTF1(_L("Opening explicit resolver"));
    // Open implicit resolver.
    RHostResolver resolver;
    INFO_PRINTF1(_L("Creating explicit resolver"));
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp,firstConn)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);        
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to explicit Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    INFO_PRINTF2(_L("Requesting to resolve hostname: %S"),&hostname1);
    
    if ( (err = resolver.GetByName(hostname1,nameEntry)) != KErrNone)
      {
      ERR_PRINTF3(_L("Failed to resolve the host %S with error: %D"),&hostname1,err);       
      }
    else
      {
      TInetAddr& netAddress = TInetAddr::Cast(nameEntry().iAddr);
      
      if (netAddress.IsUnspecified())
          {
          ERR_PRINTF1(_L("Invalid host address"));
          err = KErrArgument;
          }
      else
          {
          TBuf<KIPAddrLen> address;
          netAddress.Output(address);
          INFO_PRINTF1(_L("Got valid IP address:"));
          INFO_PRINTF2(_L("Host name: %S"),&nameEntry().iName);
          INFO_PRINTF2(_L("Host address: %S"),&address);
          }
      }
        
    if (err == KErrNone)
        { 
        TNameEntry nameEntry2;        
        INFO_PRINTF2(_L("Requesting to resolve hostname: %S"),&hostname2);
        
        if ( (err = resolver.GetByName(hostname2,nameEntry2)) != KErrNone)
          {
          INFO_PRINTF3(_L("Host resolution failed for %S with error: %D"),&hostname2,err);
          INFO_PRINTF1(_L("Test scenario passed"));
          err = KErrNone;
          }
        else
          {
          // TODO: We have to change this once we come up with end to end setup for VTun err = KErrArgument;
          INFO_PRINTF1(_L("We assumed that host is resolved through explicit connection and hence test is pass."));
          INFO_PRINTF1(_L("We have to change this once we come up with end to end setup for VTun."));
          err = KErrNone;
         // INFO_PRINTF1(_L("Error: Seems host is resolvable"));
          } 
        }   
    
    SetError(err);

    INFO_PRINTF1(_L("Closing host resolver"));
    resolver.Close(); 
    
    INFO_PRINTF1(_L("Closing connections and socket serv session"));
    firstConn.Close();
    vTunConn.Close();
    CloseSocketSrv();           
    INFO_PRINTF1(_L("CDNSSuffixTestWrapper::DNSSuffixSupportTC009L - Exit"));
    }


// TODO: If I set suffix list name of length with max size then client panic's.
void CDNSSuffixTestWrapper::DNSSuffixSupportTC010L()
    {
    INFO_PRINTF1(_L("**********************************************************************"));
    INFO_PRINTF1(_L("DNSSuffixSupportTC010L - DNS_Suffix_Support_010"));
    INFO_PRINTF1(_L("**********************************************************************"));
    
    TPtrC hostname;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest010,KKeyDNSSuffixTestHostname,hostname))
        {
        ERR_PRINTF1(_L("Failed to read the host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name1: %S"),&hostname);       
        }
    
    // @Santosh K Patil     
    // @note: start multiple interfaces
    // @note: Set long suffix lists on both interface  
    // @note: Create explicit resolver and set the suffixlist with max size.
    
    TFileName ethernetInterface;
    TInt err = KErrNone;
    err = GetFirstInterfaceNameL(ethernetInterface);
    
    if (KErrNone != err)
        { 
        ERR_PRINTF1(_L("Failed get the name of the interface (ethernet)"));    
        SetError(KErrNotFound);
        return;
        }
  
    INFO_PRINTF1(_L("Setting suffix list on interface:"));
    iSuffixList.Reset();
    
    
    // @Ganesh - Constructing suffix list for FillSuffixList with 3 separate suffix name entries in ini
    // @Ganesh - as the names are too long for the parser to parse and add it in one GetStringFromConfig call 
    TPtrC suffixName;
    TBuf<1024> suffixList;
    if (!GetStringFromConfig(KSectionDNSSuffixTest010,KKeyDNSSuffixTestSuffixList1,suffixName))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF1(_L("Suffix name:"));
        INFO_PRINTF2(_L("%S"),&suffixName);
        suffixList.Append(suffixName);
        suffixList.Append(_L(";"));
        }

    if (!GetStringFromConfig(KSectionDNSSuffixTest010,KKeyDNSSuffixTestSuffixList2,suffixName))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF1(_L("Suffix name:"));
        INFO_PRINTF2(_L("%S"),&suffixName);
        suffixList.Append(suffixName);
        suffixList.Append(_L(";"));
        }
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest010,KKeyDNSSuffixTestSuffixList3,suffixName))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF1(_L("Suffix name:"));
        INFO_PRINTF2(_L("%S"),&suffixName);
        suffixList.Append(suffixName);
        }
    FillSuffixList(iSuffixList,suffixList);
    
    if ( (err = iSocketServ.Connect()) != KErrNone)
       {
       ERR_PRINTF1(_L("Failed Connecting to socket server"));
       SetError(err);
       return;
       }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
      {
      ERR_PRINTF1(_L("Failed to start connections"));
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    INFO_PRINTF1(_L("Connections started successfully"));
    
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,ethernetInterface,iSuffixList);
                  
    if (KErrNone != err)
        {
        ERR_PRINTF3(_L("Failed Setting the suffix list on %S with errror : %D"),&ethernetInterface,err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF2(_L("Set suffix list on %S successfully"),&ethernetInterface);  
    
    INFO_PRINTF1(_L("Opening explicit resolver"));
    
    RHostResolver resolver;
    INFO_PRINTF1(_L("Creating explicit resolver"));
    if ( (err = resolver.Open(iSocketServ,KAfInet,KProtocolInetUdp,firstConn)) != KErrNone)
        {
        ERR_PRINTF2(_L("Failed Connecting to Host resolver with error: %D"),err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        SetError(err);        
        return;
        }       
    
    INFO_PRINTF1(_L("Connected to explicit Host resolver successfully"));
               
    TNameEntry nameEntry;
    
    INFO_PRINTF2(_L("Requesting to resolve hostname: %S"),&hostname);
    
    if ( (err = resolver.GetByName(hostname,nameEntry)) != KErrNone)
      {
      INFO_PRINTF3(_L("Failed to resolve the host %S with error: %D"),&hostname,err);   
      INFO_PRINTF1(_L("Test Pass"));
      err = KErrNone;
      }
    else
      {
      err = KErrArgument;
      INFO_PRINTF1(_L("Host seems to be resolvable"));
      }    
    
    SetError(err);
    
    INFO_PRINTF1(_L("Closing host resolver"));
    resolver.Close();  
    
    INFO_PRINTF1(_L("Closing connections and socket serv session"));
    firstConn.Close();
    vTunConn.Close();
    CloseSocketSrv();           
    INFO_PRINTF1(_L("CDNSSuffixTestWrapper::DNSSuffixSupportTC010L - Exit"));
    }


/*
 * @Test: Connection dissconnection test
 * 
 * Make vTun as default interface
 * Set suffix on ethernet
 * Open implicit resolver (assumed that it is opened on vTun)
 * Request for name resolution (Async one)
 * Stop the connection and observe the behaviour.(Expected, resolution should fail gracefully)
 * With appropriate error code.
 * 
 */
void CDNSSuffixTestWrapper::DNSSuffixSupportTC011L()
    {
    INFO_PRINTF1(_L("**********************************************************************"));
    INFO_PRINTF1(_L("DNSSuffixSupportTC011L - DNS_Suffix_Support_011"));
    INFO_PRINTF1(_L("**********************************************************************"));
    
    TPtrC hostname;
    
    if (!GetStringFromConfig(KSectionDNSSuffixTest011,KKeyDNSSuffixTestHostname,hostname))
        {
        ERR_PRINTF1(_L("Failed to read the host name from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF2(_L("Host name1: %S"),&hostname);       
        }
    
    TFileName ethernetInterface;
    TInt err = KErrNone;
    err = GetFirstInterfaceNameL(ethernetInterface);
    
    if (KErrNone != err)
        { 
        ERR_PRINTF1(_L("Failed get the name of the interface (ethernet)"));    
        SetError(KErrNotFound);
        return;
        }
  
    INFO_PRINTF1(_L("Setting suffix list on interface:"));
    iSuffixList.Reset();
        
    
    TPtrC suffixList;
    if (!GetStringFromConfig(KSectionDNSSuffixTest011,KKeyDNSSuffixTestSuffixList,suffixList))
        {
        ERR_PRINTF1(_L("Failed to read suffix lists from config"));    
        SetError(KErrNotFound);
        return;
        }
    else
        {
        INFO_PRINTF1(_L("Suffix list:"));
        INFO_PRINTF2(_L("%S"),&suffixList);
        FillSuffixList(iSuffixList,suffixList);
        }   
    
    if ( (err = iSocketServ.Connect()) != KErrNone)
       {
       ERR_PRINTF1(_L("Failed Connecting to socket server"));
       SetError(err);
       return;
       }
    
    INFO_PRINTF1(_L("Connected to socket server successfully"));
    
    RConnection vTunConn; // used for vTunnel
    RConnection firstConn; 
    
    err = StartConnections(iSocketServ,firstConn, vTunConn);
    
    if (err != KErrNone)
      {
      ERR_PRINTF1(_L("Failed to start connections"));
      CloseSocketSrv();
      SetError(err);
      return;
      }
    
    INFO_PRINTF1(_L("Connections started successfully"));
    
    err = SetDNSSuffixListOnInterface(iSocketServ,firstConn,ethernetInterface,iSuffixList);
                     
    if (KErrNone != err)
        {
        ERR_PRINTF3(_L("Failed Setting the suffix list on %S with errror : %D"),&ethernetInterface,err);
        SetError(err);
        firstConn.Close();
        vTunConn.Close();
        CloseSocketSrv();
        return;
        }
    
    INFO_PRINTF2(_L("Set suffix list on %S successfully"),&ethernetInterface);  

    
    // Open explcit resolver. using CDNSSuffixHostResolver    
    CDNSSuffixHostResolver* resolver = CDNSSuffixHostResolver::NewLC(*this,iSocketServ,firstConn);
    
    INFO_PRINTF2(_L("Request to resolve host(async): %S"),&hostname);
    err = resolver->ResolveL(hostname);
    
    INFO_PRINTF1(_L("Stoping the connection"));
    
    err = firstConn.Stop();
    
    // Note: we have to stop the wait loop in HandleCallBackL
    iWait.Start();
    
    CleanupStack::PopAndDestroy();
    
    INFO_PRINTF1(_L("Closing connections and socket serv session"));
    firstConn.Close();
    vTunConn.Close();
    CloseSocketSrv();           
    INFO_PRINTF1(_L("CDNSSuffixTestWrapper::DNSSuffixSupportTC011L - Exit"));
    }


// End of file
