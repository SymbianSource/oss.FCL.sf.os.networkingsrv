/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/


/**
 * @file ts_ipsec_crypto.h header file for main test code for IPsec
 */

#if (!defined __TEF_IPSEC_IKEV2_TEST_WRAPPER_H__)
#define __TEF_IPSEC_IKEV2_TEST_WRAPPER_H__

#include <datawrapper.h>

class CT_IPSecIKEv2TestWrapper : public CDataWrapper
	{
public:
	CT_IPSecIKEv2TestWrapper();
	~CT_IPSecIKEv2TestWrapper();
	
	static	CT_IPSecIKEv2TestWrapper*	NewL();
	
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
	static void CleanupOperation(TAny* aAny)
		{
		TInt* number = static_cast<TInt*>(aAny);
		delete number;
		}
	
	inline void DoCmdNewL(const TDesC& aEntry);
	void DoCmdTestIKEv2(const TDesC& aSection);
	void DoCmdNegativeTestIKEv2(const TDesC& aSection);
	void TestIKEv2WithEchoL(TInt aPort, TPtrC16 aIpDAddr);
				
protected:
	TInt*						iObject;
	};


#endif // __TEF_IPSEC_IKEV2_TEST_WRAPPER_H__
