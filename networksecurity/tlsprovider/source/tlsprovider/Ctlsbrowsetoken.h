// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file does the implementation for encryption , decryption, mac computation and PRFL
// Security component: TLS Provider. 
// 
//

/**
 @file 
 @internalTechnology
*/


#ifndef __TLSBROWSETOKEN_H__
#define __TLSBROWSETOKEN_H__


#include <e32std.h>
#include <e32base.h>

#include "tlstypedef.h"
#include "tlsprovider_log.h"

#include <ct/rmpointerarray.h>
#include "tlsprovtokeninterfaces.h"
class CTokenTypesAndTokens;
class CTlsBrowseToken : public CActive
	{
public:	
	
	static CTlsBrowseToken* NewL();

	void StartBrowsingL(RArray<CTokenTypesAndTokens>& aListAllTokensAndTypes,
						TRequestStatus& aStatus);
	void GetSessionInterface(MTLSTokenProvider* aProviderInterface,
							 MTLSSession*& aSessionInterface,
							 TRequestStatus& aStatus);
	TInt TotalTypeCount();
	void CancelRequest();

	~CTlsBrowseToken();
private:

	enum TStateLists { ENullState,EOpenToken,EGetTokenList,EGetProviderInterface,
					   EGetInterface,EGetSessionInterface,EReturnResult,EGetCiphers};

	//Active
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);

	void BrowseTokenTypesL();
	void ListTokenTypesL();	
	void OnEGetTokenListL();
	void OnEOpenTokenL();
	void OnEGetProviderInterfaceL();

	CTlsBrowseToken();

private:
	TInt iTotalTokenTypeCount;
	TInt iTotalTokenCount;
	TInt iCurrentTokentype;
	RCPointerArray<CCTTokenTypeInfo> iTokenTypeInfo;
	RArray<CTokenTypesAndTokens>* iListAllTokensAndTypes;

	MCTTokenType* iPtrTokenType;
	MTLSSession** iSessionInterface;
	MCTToken* iTokenHandle;
	MCTTokenInterface* iTokenInterface; 
	RCPointerArray<HBufC> iTokenList;
	RFs iFs;
	//state members
	TStateLists iCurrentState;
	TRequestStatus* iOriginalRequestStatus;
	//Helpers
	TInt iCurrentToken;
	TInt iTotalTokensinType;

	MTLSTokenProvider* iTokenProvider; // not owned
};

#endif
