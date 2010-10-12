// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements main test code for dnsproxy
//



/**
 @file 
*/

#include "t_dnsproxymain.h"
#include "t_dnsproxyconst.h"

#include <dns_qry.h>
#include <networking/dnd_err.h>
#include "es_enum.h"
#include  "dnsproxyupdateif.h"
#include "naptinterface.h"
/**
Purpose: Constructor of CT_DnsProxyMainTestWrapper class

@internalComponent
*/
CT_DnsProxyMainTestWrapper::CT_DnsProxyMainTestWrapper()
:	iObject(NULL)
	{
	}
/**
Purpose: Destructor of CT_DnsProxyMainTestWrapper class

@internalComponent
*/
CT_DnsProxyMainTestWrapper::~CT_DnsProxyMainTestWrapper()
	{
	//This is just an example about how to use create a new object
	delete iObject;
	iObject = NULL;

	if(iUseNapt)
	{
		iNaptSocket.Close();
		iUseNapt = 0;
	}
		
	iDnsProxySession.Close();
	iHr.Close();
	iConn2.Stop(); 	
	iConn2.Close();
	iSs.Close();
	}

/**
Purpose: Command fuction of CT_DnsProxyMainTestWrapper class

@internalComponent
*/
CT_DnsProxyMainTestWrapper* CT_DnsProxyMainTestWrapper::NewL()
	{
	CT_DnsProxyMainTestWrapper*	ret = new (ELeave) CT_DnsProxyMainTestWrapper();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}


/**
Purpose: Command fuction for a wrapper class

@internalComponent
*/
void CT_DnsProxyMainTestWrapper::ConstructL()
	{
	iObject	= new (ELeave) TInt;
	iUpdateDbFlag = 0;
	iQueryType = 0;
	iQueryResp = 0;
	iChangeIface=0;
	iUseNapt = 0;
	}


/**
Purpose: Command fuction for a wrapper class

@internalComponent
*/
TBool CT_DnsProxyMainTestWrapper::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	TBool ret = ETrue;

	// Print out the parameters for debugging
	INFO_PRINTF2( _L("<font size=2 color=990000><b>aCommand = %S</b></font>"), &aCommand );
	INFO_PRINTF2( _L("aSection = %S"), &aSection );
	INFO_PRINTF2( _L("aAsyncErrorIndex = %D"), aAsyncErrorIndex );

	if(KNew() == aCommand)
		{
		DoCmdNew(aSection);
		}
	else if(KTestGlobal() == aCommand)
		{
		DoCmdTestGlobal(aSection);
		}
	else if(KTestLocal() == aCommand)
		{
		DoCmdTestLocalL(aSection);
		}
	else if(KTestConnection() == aCommand)
		{
		DoCmdTestConnectionL(aSection);
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
void CT_DnsProxyMainTestWrapper::DoCmdNew(const TDesC& aSection)
	{
	TBuf<256> name16;
	TPtrC16 pname16 = name16;
	if (!GetStringFromConfig(aSection, KDomainName(), pname16))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter '%S'</font>"), &KDomainName());
		SetBlockResult(EFail);
		}

	TInt iap1,iap2,useiap;
	// Read iap info
	if (!GetIntFromConfig(aSection, KIAP1(), iap1))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KIAP1());
		SetBlockResult(EFail);
		}

	if (!GetIntFromConfig(aSection, KIAP2(), iap2))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KIAP2());
		SetBlockResult(EFail);
		}
	if (!GetIntFromConfig(aSection, KUSEIAP(), useiap))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KUSEIAP());
		SetBlockResult(EFail);
		}


	TBuf8<256> name8;
	name8.Copy(pname16);
	TInt err= KErrNone;
	//start the connection
	err = iSs.Connect();
	TCommDbConnPref prefs;


	prefs.SetIapId(iap2);
	err = iConn2.Open(iSs);
	err = iConn2.Start(prefs);

	if(err != KErrNone)
	    {
	    ERR_PRINTF2(_L("<font color=FF0000>Could not start default connection '%d'</font>"), err);
	    SetBlockResult(EFail);
	    return;
	    }
	// start dnsproxy
    err = iDnsProxySession.Connect();

    if(err != KErrNone)
	    {
	    ERR_PRINTF2(_L("<font color=FF0000>Could not start dnsproxy! '%d'</font>"), err);
	    SetBlockResult(EFail);
	    return;
	    }


	//
	TConnectionInfo configInfoDlink;
	configInfoDlink.iIapId = useiap;
    TConnectionInfoBuf configInfoDlinkBuf(configInfoDlink);


	TRequestStatus status1;

  	iDnsProxySession.ConfigureDnsProxyServer(configInfoDlinkBuf,status1);
  	User::WaitForRequest(status1);
      if(status1.Int() != KErrNone)
  	    {
  	    ERR_PRINTF2(_L("<font color=FF0000>Could not configure Downlink for dnsproxy! '%d'</font>"), status1.Int());
  	    SetBlockResult(EFail);
  	    return;
  	    }

	TConnectionInfo configInfoUlink;
	configInfoUlink.iIapId = 0;
    TConnectionInfoBuf configInfoUlinkBuf(configInfoUlink);

 	TRequestStatus status4;
	iDnsProxySession.ConfigureUplinkInfo(configInfoUlinkBuf,status4);
  	User::WaitForRequest(status4);
      if(status4.Int() != KErrNone)
  	    {
  	    ERR_PRINTF2(_L("<font color=FF0000>Could not configure Uplink for dnsproxy! '%d'</font>"), status4.Int());
  	    SetBlockResult(EFail);
  	    return;
  	    }

/*****************************************************************************************/
    if(!iUseNapt)
    	{
    	err = iNaptSocket.Open(iSs,_L("napt"));
 	    if(err != KErrNone)
    		{
      		ERR_PRINTF2(_L("<font color=FF0000>Load Napt failure! '%d'</font>"), err);
  	    	SetBlockResult(EFail);
  	    	return;
			}
		iUseNapt = ETrue;	

	_LIT(KInterfaceName,"wlan");
	_LIT(KInterfaceName2,"wlan");

   		RSocket sock;
   		sock.Open(iSs,KAfInet, KSockDatagram, KProtocolInetUdp);     	
   		TUint32 testaddr = GetInterfaceAddress(sock,KInterfaceName());
   		TUint32 ifindex = GetInterfaceIndex(sock, KInterfaceName2());
     	sock.Close();
     	     
    	TPckgBuf <TInterfaceLockInfo> info;    
		info().iPublicIap  = useiap;  
		info().iPrivateIap = useiap;  
		info().iIfIndex = ifindex;
    	TInetAddr::Cast	(info().iPrivateIp).SetV4MappedAddress(testaddr); 
    	TInetAddr::Cast(info().iPublicIp).SetV4MappedAddress(testaddr) ; 
    	info().iNetmaskLength = 8;
    	info().iUplinkAccess = ETrue; 
    
		err = iNaptSocket.SetOpt(KSoNaptSetup,KSolNapt,info);
		if(err != KErrNone)
    		{
      		ERR_PRINTF2(_L("<font color=FF0000>Napt Setup configuration failed! '%d'</font>"), err);
  	    	SetBlockResult(EFail);
  	    	return;	
  	    	}
 
    	}
       
/****************************************************************************************/    
  	TRequestStatus status2;
  	iDnsProxySession.UpdateDomainName(name8,status2);
  	User::WaitForRequest(status2);
	if(status2.Int() != KErrNone)
		{
		ERR_PRINTF2(_L("<font color=FF0000>Could not set local DomainName '%d'</font>"), status2.Int());
		SetBlockResult(EFail);
		return;
		}


	// start tdnd


    err = iHr.Open(iSs,KAfInet,KProtocolInetUdp);
 	if(err != KErrNone)
	    {
	    ERR_PRINTF2(_L("<font color=FF0000>could not start tDND '%d'</font>"), err);
	    SetBlockResult(EFail);
	    return;
	    }


	}
/************************************************************************************************/
TUint32 CT_DnsProxyMainTestWrapper::GetInterfaceAddress(RSocket& sock,const TDesC& aFName)
/**
 * Get interface gloabal Ip Address
 * @param sock - socket connection
 * @param aFName - interface name
 * @return Ip address
 */
{ 
	TName address;
	TBool isSiteLocal,isLinkLocal;
	TInt retVal = -1;
	
	TPckgBuf<TSoInetInterfaceInfo> info; 
	 
 	retVal = sock.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
 	
 	
    retVal = sock.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info);
    
            
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
		  
		  retVal = sock.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info);
	      }

	return info().iAddress.Address();
}

TUint32 CT_DnsProxyMainTestWrapper::GetInterfaceIndex(RSocket& sock,const TDesC& aFName)
/**
 * Get interface gloabal Ip Address
 * @param sock - socket connection
 * @param aFName - interface name
 * @return Ip address
 */
{ 
	TName address;
	//TBool isSiteLocal,isLinkLocal;
	TInt retVal = -1;
	
	TPckgBuf<TSoInetInterfaceInfo> info; 
	 
 	retVal = sock.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
 	
 	
    retVal = sock.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info);
    
            
   	while(retVal==KErrNone)
		 {
		 
		 TSoInetIfQuery ifquery;
    	 TPckg<TSoInetIfQuery> queryopt(ifquery);
    
    	 ifquery.iName = info().iName;
     	 retVal = sock.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, queryopt);
	 	 if (retVal == KErrNone)
	 	 	{
	 	   	TInt str = info().iName.Find(aFName);
		 	if(str != KErrNone) 
		 		retVal = sock.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info);
		 	else
		 		retVal = ifquery.iIndex;
	 	 	}
	      }

	return retVal;
}
/***********************************************************************************************/	
TVerdict CT_DnsProxyMainTestWrapper::ReadCommonParameters(const TDesC& aSection)
	{
	// read url from ini file
	TBuf<256> url;
	TPtrC16 purl = url;

	if (!GetStringFromConfig(aSection, KUrl(), purl))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter '%S'</font>"), &KUrl());
		return EFail;
		}

	url8.Copy(purl);

	// read  addr from ini file

	TPtrC16 paddr = addr;
	if (!GetStringFromConfig(aSection, KAddr(), paddr))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter '%S'</font>"), &KAddr());
		return EFail;
		}
	addr.Copy(paddr);

	// Read query type from ini file
	if (!GetIntFromConfig(aSection, KQueryType(), iQueryType))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KQueryType());
		return EFail;
		}

	// Read query Resp from ini file
	if (!GetIntFromConfig(aSection, KQueryResp(), iQueryResp))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KQueryResp());
		return EFail;
		}

	// Read closecon from ini file
	if (!GetIntFromConfig(aSection, KCHANGEIFACE(), iChangeIface))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KCHANGEIFACE());
		return EFail;
		}
	return EPass;

	}

void CT_DnsProxyMainTestWrapper::CleanDNDCache()
	{
		TPtrC8 purl8 = url8;
		 //Clean the dnd cache memory.
		INFO_PRINTF1(_L("<font color=9900000>Clean the dnd cache memory</font>"));

         TDnsQuery query(purl8, KDnsQTypeCacheClear);
         TPckgC<TDnsQuery> querybuf(query);
 		 TBuf8<1024> iResult;
         iResult.SetLength(iResult.MaxLength());
         iHr.Query(querybuf, iResult);

	}
TVerdict CT_DnsProxyMainTestWrapper::VerifyChangeInterface()
	{
	TBool val= EFalse;
	switch(iChangeIface)
		{
		case 1:
			{
			iConn1.Stop();
			val = ETrue;
			}
		break;
		case 2:
			{
				INFO_PRINTF1(_L("<font color=990000>About stop connection</font>"));	
			iConn2.Stop();
			TInt kerr = iConn2.Start();
				INFO_PRINTF2(_L("<font color=990000>Start ,return[%d]</font>"),kerr);
			val = ETrue;
			}
		break;
		}
	if(val)
		{
		TPtrC8 purl8 = url8;
		TPtrC16 paddr = addr;
		INFO_PRINTF1(_L("<font color=990000>query, after conn close</font>"));
		CleanDNDCache();
		return  QueryAndValidate(purl8,paddr);
		}
    return EPass;
	}

TInt CT_DnsProxyMainTestWrapper::QueryAndValidate_a(const TDesC8& url,const TDesC& addr)
	{
	// query via dnd
    TDnsQueryBuf  qbuf;
    TDnsRespABuf  rspbuf;
    TInt err = KErrNone;
    qbuf().SetType(iQueryType);

    qbuf().SetData(url);
    
    err =iHr.Query(qbuf,rspbuf);
	INFO_PRINTF2(_L("<font color=FF0000>atype Query returned '%d'</font>"), err);    
	if(err != KErrNone)return err;

    //-- check response
    TInetAddr expInetAddr;
    expInetAddr.Input(addr);
    const TDnsRespA& dnsRespA = rspbuf();
    if(!dnsRespA.HostAddress().CmpAddr(expInetAddr))
	  	{
	  	ERR_PRINTF1(_L("<font color=FF0000>Host address does not match </font>"));
	  	return KErrGeneral;
	  	}
	return KErrNone;

	}

TInt  CT_DnsProxyMainTestWrapper::QueryAndValidate_prt()
   {
	TInt err = KErrNone;
	TDnsQueryBuf      dnsQryBuf;
	TDnsRespPTRBuf    dnsRespPtrBuf;

	dnsQryBuf().SetType(KDnsRRTypePTR);


	_LIT8(KTDNSProxyNode5,"14.109.116.66.in-addr.arpa");
	TBufC8<50>buf4(KTDNSProxyNode5);
	dnsQryBuf().SetData(buf4);

	//-- make an PTR query.
	//-- The query result shall be forged by pdummy.prt
	err = iHr.Query(dnsQryBuf, dnsRespPtrBuf);

	INFO_PRINTF2(_L("<font color=FF0000>ptr Query returned '%d'</font>"), err);
    return err;
     }

 TInt CT_DnsProxyMainTestWrapper::QueryAndValidate_srv()
     {
     TInt err = KErrNone;

     TDnsQueryBuf      dnsQryBuf;
     TDnsRespSRVBuf    dnsRespSrvBuf;

     //-- test SRV query. see also RFC2782
     _LIT8(KSRVqrySample, "_sip._tls.nokia.com");
     dnsQryBuf().SetType(KDnsRRTypeSRV);
     dnsQryBuf().SetData(KSRVqrySample);

     err = iHr.Query(dnsQryBuf, dnsRespSrvBuf);
	 INFO_PRINTF2(_L("<font color=FF0000>srv Query returned '%d'</font>"), err);
     const TDnsRespSRV& dnsResp=dnsRespSrvBuf();
     return err;
     }

TVerdict CT_DnsProxyMainTestWrapper::QueryAndValidate(const TDesC8& url,const TDesC& addr)
	{
	TInt err = KErrNone;
	if(iQueryType == KDnsRRTypePTR )
		{
		err = QueryAndValidate_prt();
		}

	else if(iQueryType == KDnsRRTypeSRV )
		{
		err = QueryAndValidate_srv();
		}

	else
		{
		err = QueryAndValidate_a(url,addr);
		}
	TVerdict result = EPass;	
	if(err != iQueryResp) result = EFail;	
	
	return result;
	}
void CT_DnsProxyMainTestWrapper::DoCmdTestGlobal(const TDesC& aSection)
	{
	TVerdict result = EFail;
	result = ReadCommonParameters(aSection);
	if (result == EPass)
		{
		TPtrC8 purl8 = url8;
		TPtrC16 paddr = addr;
		result  = QueryAndValidate(purl8,paddr);
		}

    //End
    SetBlockResult(result);
    return;
	}

void CT_DnsProxyMainTestWrapper::DoCmdTestLocalL(const TDesC& aSection)
	{
	TVerdict result = EFail;
	result = ReadCommonParameters(aSection);
		if (result == EFail)
		{
		SetBlockResult(EFail);
		return;
		}

	TPtrC8 purl8 = url8;
	TPtrC16 paddr = addr;

	// Read updatedb flag
	if (!GetIntFromConfig(aSection, KUpdateDBFlag(), iUpdateDbFlag))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter1 %S</font>"), &KUpdateDBFlag());
		SetBlockResult(EFail);
		return;
		}
	if(iUpdateDbFlag == 1)
		{

		 //Add the entry into DB.
		INFO_PRINTF1(_L("<font color=990000>Add    Entry into  dnsproxy DB</font>"));

		TUid uid = {0x200215F5};
		CDNSProxyUpdateIf* plugin = CDNSProxyUpdateIf::NewL(uid);
		TRequestStatus status3;
		plugin->AddDbEntry(url8, addr,  status3);
		User::WaitForRequest(status3);

        if(status3.Int() != KErrNone)
    	    {
    	    ERR_PRINTF2(_L("<font color=FF0000>Could not Add to  dnsproxy DB '%d'</font>"), status3.Int());
    	    SetBlockResult(EFail);
    	   // return;
    	    }
    	CleanDNDCache();    
		TVerdict result = QueryAndValidate(purl8,paddr);
		if(result == EFail )
    	    {
    	    SetBlockResult(EFail);
    	  //  return;
    	    }
		CleanDNDCache();
	     //query once again.
		INFO_PRINTF1(_L("<font color=990000>second query</font>"));
		result = QueryAndValidate(purl8,paddr);
		if(result == EFail )
    	    {
    	    SetBlockResult(EFail);
    	   // return;
    	    }

  	    VerifyChangeInterface();

		INFO_PRINTF1(_L("<font color=990000>Remove Entry from  dnsproxy DB</font>"));

		TRequestStatus status5;
		plugin->RemoveDbEntry(addr,status5);
		User::WaitForRequest(status5);

        if(status5.Int() != KErrNone)
    	    {
    	    ERR_PRINTF2(_L("<font color=FF0000>Could not Remove from  dnsproxy DB '%d'</font>"), status5.Int());
    	    SetBlockResult(EFail);
    	   // return;
    	    }

	    delete plugin;
	    REComSession::FinalClose();
		}
	else
		{
		 TDnsQueryBuf  qbuf;
		TDnsRespABuf  rspbuf;
		qbuf().SetType(KDnsRRTypeA);

		qbuf().SetData(purl8);
		TInt err =iHr.Query(qbuf,rspbuf);
		
		}
	TDnsQueryBuf  qbuf;	
	TDnsRespABuf  rspbuf;
	TInt err = KErrNone;
	qbuf().SetType(KDnsRRTypeA);		 	
	_LIT8(KURLA, "ccc.locala");  		
	TPtrC8 purl8a(KURLA);
	qbuf().SetData(purl8a);
	err =iHr.Query(qbuf,rspbuf);

	if(err != KErrDndNameNotFound)
		{
		ERR_PRINTF2(_L("<font color=FF0000>responces for ccc.locala is not KErrDndNameNotFound, but  '%d'</font>"), err);
		SetBlockResult(EFail);
		return ;
	    }
	
	_LIT8(KURLB, "bcc.local");
	TPtrC8 purl8b(KURLB);
	qbuf().SetData(purl8b);
	err =iHr.Query(qbuf,rspbuf);
	
	if(err != KErrDndNameNotFound)
		{
		ERR_PRINTF2(_L("<font color=FF0000>Query responces is not KErrDndNameNotFound, but  '%d'</font>"), err);
		SetBlockResult(EFail);
		return ;
	    }

	//End
	SetBlockResult(EPass);
	return;
	}


void CT_DnsProxyMainTestWrapper::DoCmdTestConnectionL(const TDesC& aSection)
	{
	TBuf<256> name16;
	TPtrC16 pname16 = name16;


	TInt iap1,iap2,useiap;
	// Read iap info
	if (!GetIntFromConfig(aSection, KIAP1(), iap1))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KIAP1());
		SetBlockResult(EFail);
		}

	if (!GetIntFromConfig(aSection, KIAP2(), iap2))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KIAP2());
		SetBlockResult(EFail);
		}
		
	if (!GetIntFromConfig(aSection, KUSEIAP(), useiap))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KUSEIAP());
		SetBlockResult(EFail);
		}
    		
    // Read expectation from ini file
	if (!GetIntFromConfig(aSection, KExpectation(), iQueryResp))
		{
		ERR_PRINTF2(_L("<font color=FF0000>No parameter %S</font>"), &KExpectation());
		SetBlockResult(EFail);
		}
		
	
	TBuf8<256> name8;
	name8.Copy(pname16);
	TInt err= KErrNone;
	//start the connection
	err = iSs.Connect();
	TCommDbConnPref prefs;
    

	prefs.SetIapId(iap2);
	err = iConn2.Open(iSs);
	
	err = iConn2.Start(prefs);

    
	if(err != KErrNone)
	    {
	    ERR_PRINTF2(_L("<font color=FF0000>Could not start default connection '%d'</font>"), err);
	    SetBlockResult(EFail);
	    return;
	    }
    
    err = iHr.Open(iSs,KAfInet,KProtocolInetUdp);
 	if(err != KErrNone)
	    {
	    ERR_PRINTF2(_L("<font color=FF0000>could not start tDND '%d'</font>"), err);
	    SetBlockResult(EFail);
	    return;
	    }	    
	// start dnsproxy
    err = iDnsProxySession.Connect();

    if(err != KErrNone)
	    {
	    ERR_PRINTF2(_L("<font color=FF0000>Could not start dnsproxy! '%d'</font>"), err);
	    SetBlockResult(EFail);
	    return;
	    }
    
    if(!iUseNapt)
    	{
    	err = iNaptSocket.Open(iSs,_L("napt"));
 	    if(err != KErrNone)
    		{
      		ERR_PRINTF2(_L("<font color=FF0000>Load Napt failure! '%d'</font>"), err);
  	    	SetBlockResult(EFail);
  	    	return;
			}
		iUseNapt = ETrue;	

	_LIT(KInterfaceName,"wlan");
	_LIT(KInterfaceName2,"wlan");

   		RSocket sock;
   		sock.Open(iSs,KAfInet, KSockDatagram, KProtocolInetUdp);     	
   		TUint32 testaddr = GetInterfaceAddress(sock,KInterfaceName());
   		TUint32 ifindex = GetInterfaceIndex(sock, KInterfaceName2());
     	sock.Close();
     	     
    	TPckgBuf <TInterfaceLockInfo> info;    
		info().iPublicIap  = useiap;  
		info().iPrivateIap = useiap;  
		info().iIfIndex = ifindex;
    	TInetAddr::Cast	(info().iPrivateIp).SetV4MappedAddress(testaddr); 
    	TInetAddr::Cast(info().iPublicIp).SetV4MappedAddress(testaddr) ; 
    	info().iNetmaskLength = 8;
    	info().iUplinkAccess = ETrue; 
    
		err = iNaptSocket.SetOpt(KSoNaptSetup,KSolNapt,info);
		if(err != KErrNone)
    		{
      		ERR_PRINTF2(_L("<font color=FF0000>Napt Setup configuration failed! '%d'</font>"), err);
  	    	SetBlockResult(EFail);
  	    	return;	
  	    	}
 
    	}

	//
	TConnectionInfo configInfoDlink;
	configInfoDlink.iIapId = useiap;
    TConnectionInfoBuf configInfoDlinkBuf(configInfoDlink);

    

	TRequestStatus status1;

  	iDnsProxySession.ConfigureDnsProxyServer(configInfoDlinkBuf,status1);
  	User::WaitForRequest(status1);
      if(status1.Int() != KErrNone)
  	    {
  	    ERR_PRINTF2(_L("<font color=FF0000>ConfigureDnsProxyServer returned'%d'</font>"), status1.Int());
  	    SetBlockResult(EPass);
  	    return;
  	    }
  	    
  	

	TDnsQueryBuf  qbuf;	
	TDnsRespABuf  rspbuf;

	qbuf().SetType(KDnsRRTypeA);		 	
	_LIT8(KURLA, "www.google.com");  		
	TPtrC8 purl8a(KURLA);
	qbuf().SetData(purl8a);
	err =iHr.Query(qbuf,rspbuf);
	if(err != KErrDndNameNotFound)
		{
		ERR_PRINTF2(_L("<font color=FF0000>responces for ccc.locala is not KErrDndNameNotFound, but  '%d'</font>"), err);
		SetBlockResult(EPass);
  	    return;
	  
		}
  	    
	}
	
