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
*/


/**
 * @file ts_ipsec_polapi.cpp Implements main test code for IPsec
 */

#include "te_loadpolicy_bdd.h"
#include "te_ipsecconst.h"
#include "te_selectorconflict.h"
#include "te_ipsecconst.h"
#include <ES_SOCK.H> 
#include "lib_pfkey.h"
#include "pfkey_ext.h"
#include "pfkey_send.h"
#include "ipsecpolapi.h"
#include <commdbconnpref.h>


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
remote 10.225.208.183 255.255.255.255 = { trans_1(10.225.208.183) }\r\n" );


_LIT8( KMyPolicy1,  
"SECURITY_FILE_VERSION: 3\r\n[INFO]\r\n\
IpSec Policy LOADING\r\n\
[POLICY]\r\n\
sa trans_2 = {\r\n\
esp\r\n\
encrypt_alg 12\r\n\
src_specific\r\n\
}\r\n\
outbound remote 10.225.208.99 255.255.255.0 protocol 17 = { trans_2(10.225.208.99) }\r\n\
inbound local 10.225.208.155 255.255.255.0 protocol 17  = { trans_2(10.225.208.99) }\r\n\
inbound = {}\r\n\
outbound = {}\r\n" );

_LIT8( KMyPolicy2,  
"SECURITY_FILE_VERSION: 3\r\n[INFO]\r\n\
IpSec Policy LOADING\r\n\
[POLICY]\r\n\
sa trans_1 = {\r\n\
esp\r\n\
encrypt_alg 12\r\n\
src_specific\r\n\
}\r\n\
remote 10.225.208.1 255.255.255.255 = { trans_1(10.225.208.1) }\r\n" );



/**
Purpose: Constructor of CT_IPSecIKEV2TestWrapper class
@internalComponent
*/
CT_LoadPolicyBDD::CT_LoadPolicyBDD()
	{
	}


/**
Purpose: Destructor of CT_LoadPolicyBDD class
@internalComponent
*/
CT_LoadPolicyBDD::~CT_LoadPolicyBDD()
	{
	delete iObject;
	iObject = NULL;
	}


/**
Purpose: Command fuction of CT_LoadPolicyBDD class
@internalComponent
*/
CT_LoadPolicyBDD* CT_LoadPolicyBDD::NewL()
	{
	CT_LoadPolicyBDD*	ret = new (ELeave) CT_LoadPolicyBDD();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}
	
	
/**
Purpose: Command fuction for a wrapper class
@internalComponent
*/
void CT_LoadPolicyBDD::ConstructL()
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
TBool CT_LoadPolicyBDD::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
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
	else if(KTestLoadDropPolicy() == aCommand)
		 {
	     DoLoadDropModePolicy(aSection);
		 }
	else if (KLoadBypasspolicy()== aCommand)
	     {
	     DoLoadBypassModePolicy(aSection);
	     }
	else if (KLoadNewDroppolicy()== aCommand)
	     {
	     DoLoadNewDropModePolicy(aSection);
	     }
	else if (KUnloadDropPolicy() == aCommand)
	     {
	     DoUnloadDropPolicy(aSection);
	     }
	else if (KUnloadBypassPolicy() == aCommand)
	     {
	     DoUnloadBypassPolicy(aSection);
	     }
	else if (KUnloadNewDropPolicy() == aCommand)
	     {
	     DoUnloadNewDropModePolicy(aSection);
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
void CT_LoadPolicyBDD::DoCmdNewL(const TDesC& aSection)
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


void CT_LoadPolicyBDD::DoLoadDropModePolicy(const TDesC& /*aSection*/)//this will load a drop mode policy
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
        HBufC8 *policyData = HBufC8::NewLC( KMyPolicy().Length() + 256); // Allow size for IP spec.
        TPtr8 policyDataPtr(policyData->Des());
        policyDataPtr.Append(KMyPolicy);
        
        TRequestStatus status;
        iDrpPolicyServer.LoadPolicy( *policyData, iDrpPolicyHandle, status, iMyZoneInfoSet);
        User::WaitForRequest(status);
        
        err = status.Int();
        if( err == KErrNone)
            {
            iDrpPolicyServer.ActivatePolicy( iDrpPolicyHandle(), status );
            User::WaitForRequest(status);
            User::LeaveIfError(status.Int());   
            SetBlockResult(EPass);
            }
        else
            {
            SetError(status.Int());
            ERR_PRINTF2(_L("DoLoadDropModePolicy():- LoadPolicy failed with error: %d"), err);
            SetBlockResult(EFail);
            }
        CleanupStack::PopAndDestroy(policyData);
        pSADB.Close();
        pSocketServ.Close();    

    }

void CT_LoadPolicyBDD::DoLoadBypassModePolicy(const TDesC& /*aSection*/)
    {       
    RSocketServ pSocketServ;
    /** Handle to SADB socket */
    RSADB pSADB;
    TInt err;
    
    err = pSocketServ.Connect();
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadBypassModePolicy():- Failed to connect  RSocketServ with error: %d"), err);
         SetBlockResult(EFail);   
         }   
    err = pSADB.Open(pSocketServ);
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadBypassModePolicy():- Failed to open RSADB with error: %d"), err);
         SetBlockResult(EFail);  
         }                                   
    
    HBufC8 *policyData = HBufC8::NewLC( KMyPolicy1().Length() + 256); // Allow size for IP spec.
    TPtr8 policyDataPtr(policyData->Des());
    policyDataPtr.Append(KMyPolicy1);
    
    TRequestStatus status;
    iBypsPolicyServer.LoadPolicy( *policyData,iBypsPolicyHandle, status, iMyZoneInfoSet);
    User::WaitForRequest(status);
    
    if(status.Int() == KErrNone)
        {
        iBypsPolicyServer.ActivatePolicy( iBypsPolicyHandle(), status );
        User::WaitForRequest(status);
        User::LeaveIfError(status.Int());   
        SetBlockResult(EPass);
        }
    else
        {
        SetError(status.Int());
        ERR_PRINTF2(_L("DoLoadBypassModePolicy():- LoadPolicy failed with error: %d"), status.Int());
        SetBlockResult(EFail);
        }
    CleanupStack::PopAndDestroy(policyData);    
    pSADB.Close();
    pSocketServ.Close();     
    }




void CT_LoadPolicyBDD::DoLoadNewDropModePolicy(const TDesC& /*aSection*/)
    {       
    RSocketServ pSocketServ;
    /** Handle to SADB socket */ 
    RSADB pSADB;         
    TInt err;
        
    err = pSocketServ.Connect();
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadNewDropModePolicy():- Failed to connect  RSocketServ with error: %d"), err);
         SetBlockResult(EFail);   
         }   
    err = pSADB.Open(pSocketServ);
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadNewDropModePolicy():- Failed to open RSADB with error: %d"), err);
         SetBlockResult(EFail);   
         }                                   
            
    HBufC8 *policyData = HBufC8::NewLC( KMyPolicy2().Length() + 256); // Allow size for IP spec.
    TPtr8 policyDataPtr(policyData->Des());
    policyDataPtr.Append(KMyPolicy2);
    

    TRequestStatus status;   
    iDrpPolicyServer.LoadPolicy( *policyData, iNewDrpPolicyHandle, status, iMyZoneInfoSet);
    User::WaitForRequest(status);
    
    if(status.Int() == KErrNone)
        {
        iDrpPolicyServer.ActivatePolicy( iNewDrpPolicyHandle(), status );
        User::WaitForRequest(status);
        User::LeaveIfError(status.Int());   
        SetBlockResult(EPass);
        }
    else
        {
        SetError(status.Int());
        ERR_PRINTF2(_L("DoLoadNewDropModePolicy():- LoadPolicy failed with error: %d"), status.Int());
        SetBlockResult(EFail);
        }
    CleanupStack::PopAndDestroy(policyData);    
    pSADB.Close();
    pSocketServ.Close();     
    }


/*
*Execute the test to unload the policy.
*/
void CT_LoadPolicyBDD::DoUnloadDropPolicy(const TDesC& /*aSection*/)
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
void CT_LoadPolicyBDD::DoUnloadBypassPolicy(const TDesC& /*aSection*/)
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
*Execute the test to unload the policy.
*/
void CT_LoadPolicyBDD::DoUnloadNewDropModePolicy(const TDesC& /*aSection*/)
    {  

    TRequestStatus status;
    iDrpPolicyServer.UnloadPolicy(iNewDrpPolicyHandle(),status);
    User::WaitForRequest(status);
    TInt err = status.Int();
    if( err != KErrNone)
        {
        SetError(err);
        ERR_PRINTF2(_L("DoUnloadNewDropPolicy():- UnloadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
   
    }


/*
*Execute the test for closing the connection.
*/
void CT_LoadPolicyBDD::DoCmdClose(const TDesC& /*aSection*/)
    {       
    SetBlockResult(EPass);
    }
    


