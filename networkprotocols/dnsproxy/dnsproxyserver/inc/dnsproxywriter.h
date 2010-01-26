//dnsproxywriter.h
/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* dnsproxywriter.h - dns proxy writer class
* @file
* @internalTechnology
*
*/




#ifndef __H_DNS_PROXY_WRITER_H__
#define __H_DNS_PROXY_WRITER_H__

// Forward declaration for CDnsProxyWriter class
class CDnsProxyListener;
class TQueryContext;

class CDnsProxyWriter:public CActive
/**
 *CDnsProxyWriter class is an active object and is responsible for sending the query
 *to the querying host address. After sending the response packet it 
 *deletes the query context and checks if there is any other query pending processing.
 *
 *@internalTechnology
**/
{
friend class CDnsProxyListener;
	
public:
	static CDnsProxyWriter* NewL(CDnsProxyListener& aListener);
	void WriteTo(TQueryContext* queryContext);
	~CDnsProxyWriter();
		
protected:
	CDnsProxyWriter(CDnsProxyListener& aListener);
	void ConstructL();
	void RunL();
	void DoCancel();
	TInt RunError(TInt aErr);
	
private:
	CDnsProxyListener& iListener;
	
	TSockAddr addr;
	
};
#endif/*__H_DNS_PROXY_WRITER_H__ */