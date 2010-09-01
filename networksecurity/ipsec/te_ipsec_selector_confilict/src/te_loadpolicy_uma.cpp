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
* @File :- te_loadpolicy_uma.cpp Implements loading the policy 
*
*/


/**
 * @file ts_ipsec_polapi.cpp Implements main test code for IPsec
 */

#include "te_loadpolicy_uma.h"
#include "te_ipsecconst.h"
#include "te_selectorconflict.h"
#include <ES_SOCK.H> 
#include "lib_pfkey.h"
#include "pfkey_ext.h"
#include "pfkey_send.h"
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
outbound remote 10.225.208.86 255.255.255.255 protocol 17 = { trans_1(10.225.208.86) }\r\n\
inbound local 10.225.208.102 255.255.255.255 protocol 17  = { trans_1(10.225.208.86) }\r\n\
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
CT_LoadPolicyUMA::CT_LoadPolicyUMA()
	{
	}
/**
Purpose: Destructor of CT_LoadPolicyUMA class
@internalComponent
*/
CT_LoadPolicyUMA::~CT_LoadPolicyUMA()
	{
	delete iObject;
	iObject = NULL;
	}
	
/**
Purpose: Command fuction of CT_LoadPolicyUMA class
@internalComponent
*/
CT_LoadPolicyUMA* CT_LoadPolicyUMA::NewL()
	{
	CT_LoadPolicyUMA*	ret = new (ELeave) CT_LoadPolicyUMA();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}
		
/**
Purpose: Command fuction for a wrapper class
@internalComponent
*/
void CT_LoadPolicyUMA::ConstructL()
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
TBool CT_LoadPolicyUMA::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
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
	else if(KLOadUMAPolicy() == aCommand)
		{
	    DoLoadUMAPolicy(aSection);
		}
	else if (KTestLoadDropModePolicy() == aCommand)
	    {
        DoLoadDropModePolicy(aSection);
	    }
	else if (KLoadUMAByPassPolicy() == aCommand)
        {
        DoLoadBypassModePolicy(aSection);
        }
	else if (KUnloadDropPolicy() == aCommand)
        {
        DoUnloadDropPolicy(aSection);
        }
	else if (KUnloadBypassPolicy() == aCommand)
        {
        DoUnloadBypassPolicy(aSection);
        }
	else if (KUnloadUMAPolicy() == aCommand)
	    {
	    DoUnloadUMAPolicy(aSection);
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
void CT_LoadPolicyUMA::DoCmdNewL(const TDesC& aSection)
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
 * UMA policy loading for ONB scripts
 */
void CT_LoadPolicyUMA::DoLoadUMAPolicy(const TDesC& /*aSection*/) 
    {
    RSocketServ pSocketServ;
    /** Handle to SADB socket */
    RSADB pSADB; 
    TInt err;
        
    err = pSocketServ.Connect();
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadUMAPolicy():- Failed to connect  RSocketServ with error: %d"), err);
         SetBlockResult(EFail);  
         }   
    err = pSADB.Open(pSocketServ);
    if ( err != KErrNone)
         {
         ERR_PRINTF2(_L("DoLoadUMAPolicy():- Failed to open RSADB with error: %d"), err);
         SetBlockResult(EFail);   
         }                                   
          
    /***
     * this code is to check the UMA setopt function
     * if we call with 0 then it is considered as UMA policy
     */
    TInt sur = 0;
    _LIT8(KFormatStr,"%D");
    TBuf8<50> sidbuf;
    sidbuf.Format(KFormatStr,sur);
    TLex8 lex(sidbuf);    
    TInt intval;
    err = lex.Val(intval);
    
    //storing the SID information to the policy server which
    //is used to identify the UMA policy in ipsec policy manager handler.
    TInt error = iBypsPolicyServer.SetOpt(KOptionNameSid, KOptionLevelDefault, sidbuf);        
    if ( KErrNone != error )
    {
    ERR_PRINTF2(_L("DoLoadUMAPolicy->LoadPolicy ERROR  in setOpt to the policy server %d"), error);
    SetError(error);
    SetBlockResult(EFail);
    return ;
    }
    
    HBufC8 *policyData = HBufC8::NewLC( KMyPolicy().Length() + 256); // Allow size for IP spec.
    TPtr8 policyDataPtr(policyData->Des());
    policyDataPtr.Append(KMyPolicy);
    TRequestStatus status;      
    iBypsPolicyServer.LoadPolicy( *policyData, iUMAPolicyHandle, status, iMyZoneInfoSet);
    User::WaitForRequest(status);
    err = status.Int();
    if( err == KErrNone)
        {      
        iBypsPolicyServer.ActivatePolicy( iUMAPolicyHandle(), status );
        User::WaitForRequest(status);
        User::LeaveIfError(status.Int());   
        SetBlockResult(EPass);
        }
    else
        {
        SetError(status.Int());
        ERR_PRINTF2(_L("DoLoadUMAPolicy():- LoadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
    CleanupStack::PopAndDestroy(policyData);
    pSADB.Close();
    pSocketServ.Close();    
    }

/*
*Execute the test for loading the drop mode policy.
*/
void CT_LoadPolicyUMA::DoLoadDropModePolicy(const TDesC& /*aSection*/)
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
     

    /***
     * this code is to check the UMA setopt function
     * if we call with 0 then it is considered as UMA policy
     * in this case we are passing some other value
     */
    TInt sur = 0x100;
    _LIT8(KFormatStr,"%D");
    TBuf8<50> sidbuf;
    sidbuf.Format(KFormatStr,sur);
        /*  if you want to check the value un comment this part of the code
        TLex8 lex(sidbuf);    
        TInt intval;
        TInt err = lex.Val(intval);
        */
    
    //storing the SID information to the policy server which
    //is used to identify the UMA policy in ipsec policy manager handler.
    TInt error = iBypsPolicyServer.SetOpt(KOptionNameSid, KOptionLevelDefault, sidbuf);        
    if ( KErrNone != error )
    {
    ERR_PRINTF2(_L("DoLoadDropModePolicy->LoadPolicy ERROR  in setOpt to the policy server %d"), error);
    SetError(error);
    SetBlockResult(EFail);
    return ;
    }
    HBufC8 *policyData = HBufC8::NewLC( KMyPolicy1().Length() + 256); // Allow size for IP spec.
    TPtr8 policyDataPtr(policyData->Des());
    policyDataPtr.Append(KMyPolicy1);

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


void CT_LoadPolicyUMA::DoLoadBypassModePolicy(const TDesC& /*aSection*/)
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
        
    /***
     * this code is to check the UMA setopt function
     * if we call with 0 then it is considered as UMA policy
     * in this case we are passing some other value
     * 
     * this is done becoze we don't have any value present in comms database or it could be a big bug
     */
    TInt sur = 0x100;
    _LIT8(KFormatStr,"%D");
    TBuf8<50> sidbuf;
    sidbuf.Format(KFormatStr,sur);
    TLex8 lex(sidbuf);    
    TInt intval;
    err = lex.Val(intval);

    //storing the SID information to the policy server which
    //is used to identify the UMA policy in ipsec policy manager handler.
    TInt error = iBypsPolicyServer.SetOpt(KOptionNameSid, KOptionLevelDefault, sidbuf);        
    if ( KErrNone != error )
    {
    ERR_PRINTF2(_L("DoLoadBypassModePolicy->LoadPolicy ERROR  in setOpt to the policy server %d"), error);
    SetError(error);
    SetBlockResult(EFail);
    return ;
    }   
    HBufC8 *policyData = HBufC8::NewLC( KMyPolicy2().Length() + 256); // Allow size for IP spec.
    TPtr8 policyDataPtr(policyData->Des());
    policyDataPtr.Append(KMyPolicy2);
    TRequestStatus status;
    iBypsPolicyServer.LoadPolicy( *policyData, iBypsPolicyHandle, status, iMyZoneInfoSet);
    User::WaitForRequest(status);
    err = status.Int();
    if( err == KErrNone)
        {
        iBypsPolicyServer.ActivatePolicy( iBypsPolicyHandle(), status );
        User::WaitForRequest(status);
        User::LeaveIfError(status.Int());   
        SetBlockResult(EPass);
        }
    else
        {
        SetError(status.Int());
        ERR_PRINTF2(_L("DoLoadBypassModePolicy():- LoadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
    CleanupStack::PopAndDestroy(policyData);
    pSADB.Close();
    pSocketServ.Close();     
    }

/*
*Execute the test to unload the policy.
*/
void CT_LoadPolicyUMA::DoUnloadDropPolicy(const TDesC& /*aSection*/)
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
void CT_LoadPolicyUMA::DoUnloadUMAPolicy(const TDesC& /*aSection*/)
    {      
    TRequestStatus status;    
    iBypsPolicyServer.UnloadPolicy(iUMAPolicyHandle(),status);
    User::WaitForRequest(status);
    TInt err = status.Int();
    if( err != KErrNone)
        {
        SetError(err);
        ERR_PRINTF2(_L("DoUnloadUMAPolicy():- UnloadPolicy failed with error: %d"), err);
        SetBlockResult(EFail);
        }
    }
/*
*Execute the test for unload the policy.
*/
void CT_LoadPolicyUMA::DoUnloadBypassPolicy(const TDesC& /*aSection*/)
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
void CT_LoadPolicyUMA::DoCmdClose(const TDesC& /*aSection*/)
    {      
    SetBlockResult(EPass);
    }
    


