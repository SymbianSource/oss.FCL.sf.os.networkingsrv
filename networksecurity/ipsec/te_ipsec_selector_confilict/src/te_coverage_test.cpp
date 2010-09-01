/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Symbian Foundation License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
** @File :- te_loadpolicy_bbd.cpp Implements loading the policy 
*/


/**
 * @file ts_ipsec_polapi.cpp Implements main test code for IPsec
 */

#include "te_coverage_test.h"
#include "te_ipsecconst.h"
#include "te_selectorconflict.h"
#include <ES_SOCK.H> 
#include "lib_pfkey.h"
#include "pfkey_ext.h"
#include "pfkey_send.h"
#include <commdbconnpref.h>
#include <ipsecpolapi.h>

#if defined(SYMBIAN_CRYPTO)
#include <cryptostrength.h>
#else
#include <cryptalg.h>
#endif

_LIT8( KMyPolicy,  
"SECURITY_FILE_VERSION: 3\r\n[INFO]\r\n\
IpSec Policy LOADING\r\n\
[POLICY]\r\n\
sa trans_1 = {\r\n\
esp\r\n\
encrypt_alg 12\r\n\
src_specific\r\n\
}\r\n\
outbound remote 10.225.208.183 255.255.255.255 protocol 17 = { trans_1(10.225.208.183) }\r\n\
inbound local 10.225.208.102 255.255.255.255 protocol 17  = { trans_1(10.225.208.183) }\r\n\
inbound = {}\r\n\
outbound = {}\r\n" );


_LIT8( KMyPolicy1,  
"SECURITY_FILE_VERSION: 3\r\n[INFO]\r\n\
IpSec Policy LOADING\r\n\
[POLICY]\r\n\
sa trans_2 = {\r\n\
esp\r\n\
encrypt_alg 12\r\n\
src_specific\r\n\
}\r\n\
outbound remote 10.225.208.49 255.255.255.255 protocol 17 = { trans_2(10.225.208.49) }\r\n\
inbound local 10.225.208.155 255.255.255.255 protocol 17  = { trans_2(10.225.208.49) }\r\n" );

_LIT8( KMyPolicy2,  
"SECURITY_FILE_VERSION: 3\r\n[INFO]\r\n\
IpSec Policy LOADING\r\n\
[POLICY]\r\n\
sa trans_3 = {\r\n\
esp\r\n\
encrypt_alg 12\r\n\
src_specific\r\n\
}\r\n\
outbound remote 192.168.1.1 255.255.255.255 protocol 17 = { trans_3() }\r\n\
inbound local 192.168.1.2 255.255.255.255 protocol 17  = { trans_3() }\r\n\
inbound = {}\r\n\
outbound = {}\r\n" );


/**
Purpose: Constructor of CT_IPSecIKEV2TestWrapper class
@internalComponent
*/
CT_CoverageTest::CT_CoverageTest()
	{
	}

/**
Purpose: Destructor of CT_CoverageTest class
@internalComponent
*/
CT_CoverageTest::~CT_CoverageTest()
	{
	delete iObject;
	iObject = NULL;
	}

/**
Purpose: Command fuction of CT_CoverageTest class
@internalComponent
*/
CT_CoverageTest* CT_CoverageTest::NewL()
	{
	CT_CoverageTest*	ret = new (ELeave) CT_CoverageTest();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}
	
	
/**
Purpose: Command fuction for a wrapper class
@internalComponent
*/
void CT_CoverageTest::ConstructL()
	{
    TInt errfound;
    
	iObject = new (ELeave) TInt;
	errfound = iDrpPolicyServer.Connect();
	if( errfound != KErrNone )
	    {
	    ERR_PRINTF2(_L("failed to connect RIpsecPolicyServ with error: %d"), errfound);
	    SetBlockResult(EFail);
	    return;
	    }
	errfound=iBypsPolicyServer.Connect();
	if( errfound != KErrNone )
	    {
	    ERR_PRINTF2(_L("failed to connect RIpsecPolicyServ with error: %d"), errfound);
	    SetBlockResult(EFail); 
	    return;
	    }

	iMyZoneInfoSet.iSelectorZone.iScope = KScopeNetwork;
	iMyZoneInfoSet.iSelectorZone.iId = 8;
	iMyZoneInfoSet.iEndPointZone.iScope = KScopeNetwork;
	iMyZoneInfoSet.iEndPointZone.iId = 7;
	        
	}


/**
Purpose: Command fuction for a wrapper class
@internalComponent
*/
TBool CT_CoverageTest::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	TBool ret = ETrue;

	// Print out the parameters for debugging
	INFO_PRINTF2( _L("<font size=2 color=990000><b>aCommand = %S</b></font>"), &aCommand );
	INFO_PRINTF2( _L("aSection = %S"), &aSection );
	INFO_PRINTF2( _L("aAsyncErrorIndex = %D"), aAsyncErrorIndex );

	if(KNewCMD() == aCommand)
		{
		DoCmdNewL(aSection);
		}
	else if(KTestLoadBypassPolicy() == aCommand)
		{
	    DoLoadBypassModePolicy(aSection);
		}
	else if (KTestLoadDropModePolicy() == aCommand)
	    {
        DoLoadDropModePolicy(aSection);
	    }
	else if (KLoadNewBypassPolicy() == aCommand)
        {
        DoLoadNewBypassModePolicy(aSection);
        }
	else if (KUnloadDropPolicy() == aCommand)
        {
        DoUnloadDropPolicy(aSection);
        }
	else if (KUnloadBypassPolicy() == aCommand)
        {
        DoUnloadBypassPolicy(aSection);
        }
	else if (KUnloadNewBypassPolicy() == aCommand)
	    {
	    DoUnloadNewBypassPolicy(aSection);
	    }	
	else if (KCloseConnection() == aCommand)
	    {
	    DoCmdClose(aSection);
	    }
	else
		{
		ret = EFalse;
		}

	return ret;
	}


/**
Purpose: To create a new object of the CTEFTest type through the API.

Ini file options:
	iniData - The data from the ini file at the section provided.

@internalComponent
@param  aSection Current ini file command section
*/
void CT_CoverageTest::DoCmdNewL(const TDesC& aSection)
	{
	TInt objectValue = 0;
	TInt iapid =0;
	if (!GetIntFromConfig(aSection, KObjectValue(), objectValue))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KObjectValue());
		SetBlockResult(EFail);
		}
	else
	    {
	    *iObject = objectValue;	    
	    
        if(!GetIntFromConfig(aSection, KIapid(), iapid))
            {
            ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KObjectValue());
            SetBlockResult(EFail);
            }
        else
            {
            }
	    }
	}

/**
 * Bypass policy loading for ONB scripts
 */
void CT_CoverageTest::DoLoadBypassModePolicy(const TDesC& /*aSection*/) 
    {
    
    TInt errfound=iBypsPolicyServer.Connect();
    if( errfound != KErrNone )
         {
         ERR_PRINTF2(_L("failed to connect RIpsecPolicyServ with error: %d"), errfound);
         SetBlockResult(EFail);
         return;
         }
     /***
     * this code is to check the UMA setopt function
     * if we call with 0 then it is considered as UMA policy
     */
    TInt sur = 0x2002E241;
    _LIT8(KFormatStr,"%D");
    TBuf8<50> sidbuf;
    sidbuf.Format(KFormatStr,sur);
    TLex8 lex(sidbuf);    
    TInt intval;
    TInt err = lex.Val(intval);
    
    //this test case is to cove all the conditions.
    //in this case all the parmeters are proper
    TInt error = iBypsPolicyServer.SetOpt(KOptionNameSid, KOptionLevelDefault, sidbuf);        
    if ( KErrNone != error )
        {
        ERR_PRINTF2(_L("DoLoadUMAPolicy->LoadPolicy ERROR  in setOpt to the policy server %d"), error);
        SetError(error);
        SetBlockResult(EFail);
        return ;
        }
        

    //in this case second parm is correct
    error = iBypsPolicyServer.SetOpt(0 , KOptionLevelDefault, sidbuf);        
    if ( KErrArgument != error )
        {
        ERR_PRINTF2(_L("DoLoadUMAPolicy->LoadPolicy ERROR  in setOpt to the policy server %d"), error);
        SetError(error);
        SetBlockResult(EFail);
        return ;
        }

    //in this case second parm is correct
    error = iBypsPolicyServer.SetOpt(KOptionNameSid , 0, sidbuf);        
    if ( KErrArgument != error )
        {
        ERR_PRINTF2(_L("DoLoadUMAPolicy->LoadPolicy ERROR  in setOpt to the policy server %d"), error);
        SetError(error);
        SetBlockResult(EFail);
        return ;
        }
    
       RSocketServ pSocketServ;
       /** Handle to SADB socket */
       RSADB pSADB;
       err = pSocketServ.Connect();
       if ( err != KErrNone)
            {
            ERR_PRINTF2(_L("DoLoadDropModePolicy():- Failed to connect  RSocketServ with error: %d"), err);
            SetBlockResult(EFail);   
            }   
       err = pSADB.Open(pSocketServ);
       if ( err != KErrNone)
            {
            ERR_PRINTF2(_L("DoLoadDropModePolicy():- Failed to open RSADB with error: %d"), err);
            SetBlockResult(EFail);  
            }                                   
                  
       HBufC8 *policyData = HBufC8::NewLC( KMyPolicy().Length() + 256); // Allow size for IP spec.
       TPtr8 policyDataPtr(policyData->Des());
       policyDataPtr.Append(KMyPolicy);
       ///
        CMDBSession *cmdbSession = CMDBSession::NewL(CMDBSession::LatestVersion());            
        CleanupStack::PushL(cmdbSession);
        CCDIAPRecord *iapRecord = (CCDIAPRecord*)CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord);
        CleanupStack::PushL(iapRecord);
        iapRecord->SetRecordId(12);
        iapRecord->LoadL(*cmdbSession);
        iapRecord->iAppSid = 0x2002E241;
        iapRecord->ModifyL(*cmdbSession);
        CleanupStack::PopAndDestroy(iapRecord);
        CleanupStack::PopAndDestroy(cmdbSession);
       
       //
       TRequestStatus status;
       iBypsPolicyServer.LoadPolicy( *policyData, iBypsPolicyHandle, status, iMyZoneInfoSet);
       User::WaitForRequest(status);
       TInt err1 = status.Int();
       if(err1 == KErrNone)
           {
           iBypsPolicyServer.ActivatePolicy(iBypsPolicyHandle(), status );
           User::WaitForRequest(status);
           User::LeaveIfError(status.Int());   
           SetBlockResult(EPass);
           }
       else
           {
           SetError(status.Int());
           ERR_PRINTF2(_L("DoLoadDropModePolicy->LoadPolicy failed with error: %d"),err1);
           SetBlockResult(EFail);
           }
       CleanupStack::PopAndDestroy(policyData);    
       pSADB.Close();
       pSocketServ.Close();    
    }

/*
*Execute the test for closing the connection.
*/
void CT_CoverageTest::DoLoadDropModePolicy(const TDesC& /*aSection*/)
    {       
    RSocketServ pSocketServ;
    /** Handle to SADB socket */
    RSADB pSADB;
    TInt err;
            
    err = pSocketServ.Connect();
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadDropModePolicy():- Failed to connect  RSocketServ with error: %d"), err);
         SetBlockResult(EFail);   
         }   
    err = pSADB.Open(pSocketServ);
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadDropModePolicy():- Failed to open RSADB with error: %d"), err);
         SetBlockResult(EFail);  
         }                                   
               
    HBufC8 *policyData = HBufC8::NewLC( KMyPolicy1().Length() + 256); // Allow size for IP spec.
    TPtr8 policyDataPtr(policyData->Des());
    policyDataPtr.Append(KMyPolicy1);
    ///
     CMDBSession *cmdbSession = CMDBSession::NewL(CMDBSession::LatestVersion());            
     CleanupStack::PushL(cmdbSession);
     CCDIAPRecord *iapRecord = (CCDIAPRecord*)CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord);
     CleanupStack::PushL(iapRecord);
     iapRecord->SetRecordId(12);
     iapRecord->LoadL(*cmdbSession);
     iapRecord->iAppSid = 0;
     iapRecord->ModifyL(*cmdbSession);
     CleanupStack::PopAndDestroy(iapRecord);
     CleanupStack::PopAndDestroy(cmdbSession);    
    ///
    //storing the SID information to the policy server which
    //is used to identify the UMA policy in ipsec policy manager handler.
    TInt error=0 ;
    //pPolicyServer.SetOpt(KOptionNameSid, KOptionLevelDefault, sidbuf);
    //ERR_PRINTF2(_L("DoLoadDropModePolicy->LoadPolicy ERROR  in setOpt to the policy server %s"), sidBuffer);
    RDebug::Printf("\n see the value \n");
    if ( KErrNone != error )
    {
    ERR_PRINTF2(_L("DoLoadDropModePolicy->LoadPolicy ERROR  in setOpt to the policy server %d"), error);
    }
    
    TRequestStatus status;
    iDrpPolicyServer.LoadPolicy( *policyData, iDrpPolicyHandle, status, iMyZoneInfoSet);
    User::WaitForRequest(status);
    TInt err1 = status.Int();
    if(err1 == KErrNone)
        {
        iDrpPolicyServer.ActivatePolicy(iDrpPolicyHandle(), status );
        User::WaitForRequest(status);
        User::LeaveIfError(status.Int());   
        SetBlockResult(EPass);
        }
    else
        {
        SetError(status.Int());
        ERR_PRINTF2(_L("DoLoadDropModePolicy->LoadPolicy failed with error: %d"),err1);
        SetBlockResult(EFail);
        }
    CleanupStack::PopAndDestroy(policyData);    
    pSADB.Close();
    pSocketServ.Close();     
    }


void CT_CoverageTest::DoLoadNewBypassModePolicy(const TDesC& /*aSection*/)
    {       
    RSocketServ pSocketServ;
    /** Handle to SADB socket */
    RSADB pSADB;
    TInt err;
            
    err = pSocketServ.Connect();
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadNewBypassModePolicy():- Failed to connect  RSocketServ with error: %d"), err);
         SetBlockResult(EFail);  
         }   
    err = pSADB.Open(pSocketServ);
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadNewBypassModePolicy():- Failed to open RSADB with error: %d"), err);
         SetBlockResult(EFail);   
         }                                   
                    
    HBufC8 *policyData = HBufC8::NewLC( KMyPolicy2().Length() + 256); // Allow size for IP spec.
    TPtr8 policyDataPtr(policyData->Des());
    policyDataPtr.Append(KMyPolicy2);
    

    TRequestStatus status;
    iBypsPolicyServer.LoadPolicy( *policyData, iNewBypsPolicyHandle, status, iMyZoneInfoSet);
    User::WaitForRequest(status);
    
    err = status.Int();
    if( err == KErrNone)
        {
        iBypsPolicyServer.ActivatePolicy( iNewBypsPolicyHandle(), status );
        User::WaitForRequest(status);
        User::LeaveIfError(status.Int());   
        SetBlockResult(EPass);
        }
    else
        {
        SetError(status.Int());
        ERR_PRINTF2(_L("DoLoadNewBypassPolicy():- LoadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
    CleanupStack::PopAndDestroy(policyData);
    pSADB.Close();
    pSocketServ.Close();     
    }


/*
*Execute the test to unload the policy.
*/
void CT_CoverageTest::DoUnloadDropPolicy(const TDesC& /*aSection*/)
    {      
    TRequestStatus status;
    iDrpPolicyServer.UnloadPolicy(iDrpPolicyHandle(),status);
    User::WaitForRequest(status);
    TInt err = status.Int();
    if( err != KErrNone)
        {
        SetError(err);
        ERR_PRINTF2(_L("DoUnloadDropPolicy():- UnloadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
    }

/*
*Execute the test to unload the policy.
*/
void CT_CoverageTest::DoUnloadNewBypassPolicy(const TDesC& /*aSection*/)
    {      
    TRequestStatus status;    
    iBypsPolicyServer.UnloadPolicy(iNewBypsPolicyHandle(),status);
    User::WaitForRequest(status);
    TInt err = status.Int();
    if( err != KErrNone)
        {
        SetError(err);
        ERR_PRINTF2(_L("DoUnloadNewBypassPolicy():- UnloadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
    }

/*
*Execute the test to unload the policy.
*/
void CT_CoverageTest::DoUnloadBypassPolicy(const TDesC& /*aSection*/)
    {      
    TRequestStatus status;    
    iBypsPolicyServer.UnloadPolicy(iBypsPolicyHandle(),status);
    User::WaitForRequest(status);
    TInt err = status.Int();
    if( err != KErrNone)
        {
        SetError(err);
        ERR_PRINTF2(_L("DoUnloadBypassPolicy():- UnloadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
    }


/*
*Execute the test for closing the connection.
*/
void CT_CoverageTest::DoCmdClose(const TDesC& /*aSection*/)
    {      
    SetBlockResult(EPass);
    }
    


