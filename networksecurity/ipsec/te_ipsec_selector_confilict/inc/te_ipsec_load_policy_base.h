// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Connection Test Step Header
// 
//

/**
 @file 
*/
#ifndef __TEF_IPSEC_LOAD_POLICY_BASE__
#define __TEF_IPSEC_LOAD_POLICY_BASE__


//system includes here
#include<DataWrapper.h>

//local includes here
//---------------------------------------------------------------------------
class CT_IPSec_Load_Policy_Base : public CDataWrapper
{
public:
    virtual ~CT_IPSec_Load_Policy_Base() 
    {    
    delete iObject;
    iObject = NULL;
    }
    
    CT_IPSec_Load_Policy_Base(): iObject(NULL)
    {    
    
    }
    
    virtual TAny* GetObject() { return iObject; }
    
    virtual void SetObjectL(TAny* aObject)
        {
        DestroyData();
        iObject = static_cast<TInt*> (aObject);
        }
    
    virtual void    DisownObjectL()
        {
        iObject = NULL;
        }
    
    void DestroyData()
        {
        delete iObject;
        iObject=NULL;
        }

protected:

	virtual void DoDelay(const TDesC& /*aSection*/) {};
	virtual TBool DoCommandL(const TTEFFunction& /* aCommand */, const TTEFSectionName& /*aSection*/, const TInt /*aAsyncErrorIndex*/){return ETrue;}
	
	/*
	*Execute the test (test case code).
	*/
	virtual void DoCmdTestConnection(const TDesC& /*aSection*/){};

	/*
	*Execute the test for sending data(test case code).
	*/
	virtual void DoCmdSendPacket(const TDesC& /*aSection*/){};
	
protected :	
	TInt* iObject;
	
public :	
	virtual void StopWaiting(){};
};

#endif//__TEF_IPSEC_LOAD_POLICY_BASE__

