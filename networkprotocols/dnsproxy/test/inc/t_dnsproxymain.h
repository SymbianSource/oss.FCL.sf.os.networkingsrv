// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// header file for main test code for dnsproxy server
//



/**
 @file 
*/

#if (!defined __TEF_DNSPROXY_MAIN_WRAPPER_H__)
#define __TEF_DNSPROXY_MAIN_WRAPPER_H__



#include <test/datawrapper.h>
#include "dnsproxyclient.h"
#include <es_sock.h>
class CT_DnsProxyMainTestWrapper : public CDataWrapper
	{
public:
	CT_DnsProxyMainTestWrapper();
	~CT_DnsProxyMainTestWrapper();
	
	static	CT_DnsProxyMainTestWrapper*	NewL();
	
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny*	GetObject() { return iObject; }
	inline virtual void	SetObjectL(TAny* aObject)
		{
		DestroyData();
		iObject	= static_cast<TInt*> (aObject);
		}

	inline virtual void	DisownObjectL()
		{
		iObject = NULL;
		}

	void DestroyData()
		{
		delete iObject;
		iObject=NULL;
		}

	inline virtual TCleanupOperation CleanupOperation()
		{
		return CleanupOperation;
		}
		
protected:
	void ConstructL();
	
private:
	RSocketServ iSs;
	RDNSClient iDnsProxySession;
	RHostResolver iHr;
	RSocket iNaptSocket;
	TInt iUseNapt;
	TBuf8<256> url8;
	TBuf<100> addr;
	TInt iUpdateDbFlag;
	TInt iQueryType;
	TInt iQueryResp;
	TInt iChangeIface;
	static void CleanupOperation(TAny* aAny)
		{
		TInt* number = static_cast<TInt*>(aAny);
		delete number;
		}
	
	inline void DoCmdNew(const TDesC& aEntry);
	void DoCmdTestGlobal(const TDesC& aSection);
	void DoCmdTestLocalL(const TDesC& aSection);
	void DoCmdTestConnectionL(const TDesC& aSection);
	void CleanDNDCache();
	TVerdict VerifyChangeInterface();
	TVerdict QueryAndValidate(const TDesC8& url,const TDesC& addr);
	TInt QueryAndValidate_a(const TDesC8& url,const TDesC& addr);
	TInt QueryAndValidate_prt();
	TInt QueryAndValidate_srv();
	TVerdict ReadCommonParameters(const TDesC& aSection);
	TUint32 GetInterfaceAddress(RSocket& sock,const TDesC& aFName);
	TUint32 GetInterfaceIndex(RSocket& sock,const TDesC& aFName);
				
protected:
	TInt*		  iObject;
	RConnection   iConn1;
	RConnection   iConn2;
	};


#endif // __TEF_DNSPROXY_MAIN_WRAPPER_H__

