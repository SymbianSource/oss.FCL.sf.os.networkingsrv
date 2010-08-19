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
//



/**
 @file
 @internalTechnology
*/

#ifndef __TE_DNSSUFFIXTESTWRAPPER_H__
#define __TE_DNSSUFFIXTESTWRAPPER_H__


#include <test/datawrapper.h>
#include <e32base.h>
#include <es_sock.h>
#include <in_sock.h>
#include <nifman.h>
#include <commdbconnpref.h>
#include <badesca.h>    // MDesCArray and CDesCArray

#include "CallBackHandler.h"

/**
Forward declaration
*/ 


/**
Class implements the CDataWrapper base class and provides the commands used by the scripts file
*/
class CDNSSuffixTestWrapper : public CDataWrapper,
                              public MCallBackHandler
                        
	{
public:
	CDNSSuffixTestWrapper();
	~CDNSSuffixTestWrapper();
	
	static	CDNSSuffixTestWrapper*	NewL();
	//This function is not used currently
	virtual TAny*	GetObject() { return this; }
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	
protected:
	void ConstructL();
	
public: // from MCallBackHandler
    void HandleCallBackL(TInt aError);
		
private:
    
    // Initial configuration
    void DoInitTestingL();    
    
    // Enumeration
    void DoEnumerateInterfacesL();    
    void GetAvaiableInterfacesL(MDesCArray* aInterfaceNamesArray);
    
    // Setting/Getting suffix list on/from interface.
    void DoSetAndGetSuffixListL();    
    TInt SetDNSSuffixListOnInterface(RSocketServ& aServ,RConnection& aConn,const TDesC& aInterfaceName,RInetSuffixList& aData);
    
    TInt GetDNSSuffixList(RSocketServ& aServ,const TDesC& aInterfaceName,RInetSuffixList& aData);
    
    
    // Host resolve   
    
    void DoResolveL();    
    void DoResolveHostWithoutDomainWithSuffixListSetL();
    void DoResolveHostWithoutDomainWithoutSuffixListL();
    
    void DNSSuffixSupportTC005L();
    void DNSSuffixSupportTC006L();
    void DNSSuffixSupportTC007L();
    void DNSSuffixSupportTC008L();
    void DNSSuffixSupportTC009L();
    void DNSSuffixSupportTC010L();
    void DNSSuffixSupportTC011L();
    
    
private: // utility or helper functions
    
    TInt StartConnections(RSocketServ& aSockServ, RConnection& aConn1, RConnection& aConn2);
    TInt StartConnection(RSocketServ& aSockServ, RConnection& aConn, TUint aIapId);
    void GetConnPrefL(TUint aIapId,TCommDbConnPref& aPref);
    
    TInt32 GetFirstIapId();
    TInt32 GetSecondIapId();
    
    TInt GetFirstInterfaceNameL(TDes& aIfaceName);
    TInt GetSecondInterfaceNameL(TDes& aIfaceName);    
    TInt GetInterfaceNameL(const TDesC& aCriteria,TDes& aIfaceName);
    
    TInt FillSuffixList(RInetSuffixList& aSuffixList,TDesC& aData);
    void CloseSocketSrv();
    TInt IsSuffixListEqual(const RInetSuffixList& aSuffixList1, const RInetSuffixList& aSuffixList2);
    
private:    
    RSocketServ        iSocketServ;
    CActiveSchedulerWait iWait;
    RInetSuffixList     iSuffixList;
    RInetSuffixList     iSuffixList2;
	};
	

#endif //__TE_DNSSUFFIXTESTWRAPPER_H__
