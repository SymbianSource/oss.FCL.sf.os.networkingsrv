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
//

#include "Ctlsbrowsetoken.h"
#include "tlsprovider.h"

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif


//
//  CTlsBrowseToken
//


CTlsBrowseToken* CTlsBrowseToken::NewL()
	{ 
	CTlsBrowseToken* aPtrClnt = new (ELeave)CTlsBrowseToken();
	return(aPtrClnt);
	}


CTlsBrowseToken::CTlsBrowseToken():CActive(0)
	{
	if(CActiveScheduler::Current()) //Already installed?
		{
		CActiveScheduler::Add( this );
		}
	}

void CTlsBrowseToken::StartBrowsingL(RArray<CTokenTypesAndTokens>& aListAllTokensAndTypes,
									TRequestStatus& aStatus)
	{
	
	
	aStatus = KRequestPending;
	User::LeaveIfError(iFs.Connect());
	iOriginalRequestStatus = &aStatus;

	iListAllTokensAndTypes = &aListAllTokensAndTypes;
	TLSPROV_LOG(_L("Listing all types and tokens"))

	//The following functions fills the tokentype info in iTokenTypeInfo array.
	TLSPROV_LOG(_L("Browsing for Tokens..."))
	ListTokenTypesL();		
	}

void CTlsBrowseToken::ListTokenTypesL()
	{
	iCurrentTokentype= 0;  
		
	//Filter defining the expected interface
	RArray<TUid> Interface;
	CleanupClosePushL(Interface);	
	User::LeaveIfError(Interface.Append(TUid::Uid(KInterfaceTLSTokenProvider)));
	User::LeaveIfError(Interface.Append(TUid::Uid(KInterfaceTLSSession)));
	TCTFindTokenTypesByInterface filter(Interface.Array());
			
	//List all the tokentypes supported [eg. Software Token, WIM1, WIM2, etc]	
	CCTTokenTypeInfo::ListL(iTokenTypeInfo, filter);
	CleanupStack::Pop(&Interface);	// Interface
	Interface.Close();

	iTotalTokenTypeCount = iTokenTypeInfo.Count();
	if(!iTotalTokenTypeCount)
		{
		//Not possible..Software token always present
		User::Leave(KErrNoTokenTypes);
		}	
	BrowseTokenTypesL();
	}

TInt CTlsBrowseToken::TotalTypeCount()
	{
	return iTotalTokenTypeCount;
	}

/*
The following function gets a handle to a new token type. If any of the interfaces in opening 
a token fails and if no other tokens are present in the token type, this function will be then called 
to browse for the next token type if any.
*/
void CTlsBrowseToken::BrowseTokenTypesL()
	{
	//Now we try opening the token types here
	TInt Error = KErrNone;
	while( iCurrentTokentype  < iTotalTokenTypeCount)
		{
		if(iPtrTokenType)
			iPtrTokenType->Release();
	
		TRAP(Error,(iPtrTokenType = MCTTokenType::NewL(*iTokenTypeInfo[iCurrentTokentype], iFs)));
		
		if(Error != KErrNone)
			{
			//This Token type couldnt be opened..try the next one if present
         iPtrTokenType = NULL;
			if((iCurrentTokentype + 1) <= iTotalTokenTypeCount)
				{
				(iCurrentTokentype++);
				continue;
				}
			else
				User::Leave(Error); 
			}
		else
			{
			//List all those tokens in that type
			iCurrentState = EGetTokenList;
			iStatus = KRequestPending;
			iTokenList.Close();
			iPtrTokenType->List(iTokenList,iStatus);
			SetActive();
			return;
			}		
		}

	//Will only come here if every token has been browsed
	iCurrentTokentype= 0;
	User::RequestComplete(iOriginalRequestStatus, Error); 
	}


void CTlsBrowseToken::OnEGetTokenListL()
	{

	TLSPROV_LOG(_L("Obtaining the list of Tokens in a type"))
				
	if(!iStatus.Int()) //Token list for a particular Token type obtained successfully
		{
		iTotalTokensinType = iTokenList.Count();
		iCurrentToken = 0;
		if(iTotalTokensinType)
			{
			iStatus = KRequestPending;
			iCurrentState = EOpenToken;	
			iPtrTokenType->OpenToken((*iTokenList[0]), iTokenHandle, iStatus);
			SetActive();
			return;							
			}
		}	
	//Try for any other TokenTypes? If not present BrowseTokenTypesL() will complete
	iCurrentTokentype++;
	BrowseTokenTypesL();									
	}

void CTlsBrowseToken::OnEOpenTokenL()
	{		
	if(!iStatus.Int())
		{		
		iStatus = KRequestPending;
		iCurrentState = EGetProviderInterface;
		iTokenHandle->GetInterface(UidProv,iTokenInterface,iStatus);
		SetActive();
		return;
		}		
	
	//This token failed, try another one
	iCurrentTokentype++;
	BrowseTokenTypesL();
	}

void CTlsBrowseToken::OnEGetProviderInterfaceL()
	{
	if(!iStatus.Int()) 
		{
		TLSPROV_LOG(_L("Provider Interface for this token obtained successfully"))
				
      CTokenTypesAndTokens* tempObj = new CTokenTypesAndTokens;
      if ( !tempObj || iListAllTokensAndTypes->Append(*tempObj) != KErrNone )
         {
   		delete tempObj;
   		if (iTokenHandle)
   		    {
         iTokenHandle->Release();
   		    }
         iTokenHandle = NULL;
         User::Leave( KErrNoMemory );
         }
  		delete tempObj;
      CTokenTypesAndTokens& Tokens = (*iListAllTokensAndTypes)[iListAllTokensAndTypes->Count() - 1];
      Tokens.iTokenInfo = new CTokenInfo;
      if ( !Tokens.iTokenInfo  )
         {
          if (iTokenHandle)
              {
         iTokenHandle->Release();
              }
         iTokenHandle = NULL;
         User::Leave( KErrNoMemory );
         }

      iTokenHandle = NULL; //owned by iProviderInterface
		Tokens.iProviderInterface = static_cast<MTLSTokenProvider*>(iTokenInterface);
      iTokenInterface = NULL;
   
					
		iCurrentState = EGetCiphers;
		iStatus = KRequestPending;
		
		Tokens.iProviderInterface->CryptoCapabilities(
			Tokens.iTokenInfo->iSupportedProtocols,
			Tokens.iTokenInfo->aKeyExchAlgs,
			Tokens.iTokenInfo->aSignatureExchAlgs,
			iStatus);		
		SetActive();
		return;

		}
	
	iCurrentTokentype++;
	BrowseTokenTypesL();
	}


void CTlsBrowseToken::RunL()
	{
	if(iStatus.Int() == KErrNoMemory)
		{
		User::RequestComplete(iOriginalRequestStatus, iStatus.Int()); 
		iCurrentState = ENullState;
		return;
		}

	switch(iCurrentState)
		{
	case EGetTokenList:
		OnEGetTokenListL();
		break;
	case EOpenToken:
		OnEOpenTokenL();
		break;
	case EGetProviderInterface:
		{
		OnEGetProviderInterfaceL();
		}
		break;
	case EGetCiphers:
		{		

      CTokenTypesAndTokens& Tokens = (*iListAllTokensAndTypes)[iListAllTokensAndTypes->Count() - 1];
		Tokens.iTotalTokenCount = iTotalTokensinType;

		const TUid KSoftwareTokenType = {0x101FE20D};
		TLSPROV_LOG2(_L("loaded ECOM TLS tokentype plugin with uid = %08x"), ((Tokens.iProviderInterface->Token()).TokenType()).Type())
		if(((Tokens.iProviderInterface->Token()).TokenType()).Type() == KSoftwareTokenType)
			Tokens.iSoftwareToken = ETrue;
		else
			Tokens.iSoftwareToken = EFalse;

		
		iCurrentTokentype++;
		BrowseTokenTypesL();
		}
		break;
	case EGetSessionInterface:
		{
		TLSPROV_LOG(_L("Session Interface for this token obtained successfully"))
		*iSessionInterface = static_cast<MTLSSession*>(iTokenInterface);
      iTokenInterface = NULL;
		User::RequestComplete(iOriginalRequestStatus, iStatus.Int()); 
		}
		break;
	default:
		break;
		}

	}


void CTlsBrowseToken::GetSessionInterface(MTLSTokenProvider* aProviderInterface,
										  MTLSSession*& aSessionInterface,
										  TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;	
	iSessionInterface = &aSessionInterface;
	iCurrentState = EGetSessionInterface;
	iOriginalRequestStatus = &aStatus;
	iTokenInterface = 0;
	iStatus = KRequestPending;
	iTokenProvider = aProviderInterface;
	(aProviderInterface->Token()).GetInterface(UidSess,iTokenInterface,iStatus);
	SetActive();
	return;
	}

TInt CTlsBrowseToken::RunError(TInt aError)
	{
	User::RequestComplete(iOriginalRequestStatus, aError); 
	return KErrNone;
	}

void CTlsBrowseToken::CancelRequest()
	{
	Cancel();
	return;
	}
void CTlsBrowseToken::DoCancel()
	{
	switch (iCurrentState)
		{
	case EGetTokenList:
        {
	    if (iPtrTokenType)
		iPtrTokenType->CancelList();
        }
		break;
		
	case EOpenToken:
	    {
	    if (iPtrTokenType)
		iPtrTokenType->CancelOpenToken();
	    }
		break;
		
	case EGetProviderInterface:
	    {
	    if (iTokenHandle)
		iTokenHandle->CancelGetInterface();
	    }
		break;
	
	case EGetSessionInterface:
	    {
	    if (iTokenProvider)
		(iTokenProvider->Token()).CancelGetInterface();
	    }
		break;		

	case EGetCiphers:
	    if (iTokenInterface)
	        {
		MTLSTokenProvider* provider = static_cast<MTLSTokenProvider*>(iTokenInterface);
		provider->CancelCryptoCapabilities();
	        }
		break;

		}
	iCurrentState=ENullState;
	User::RequestComplete(iOriginalRequestStatus, KErrCancel);
	return;
	}

CTlsBrowseToken::~CTlsBrowseToken()
	{
	if(iPtrTokenType)
		{
		iPtrTokenType->Release();
		}
		
	iTokenTypeInfo.Close();
	iTokenList.Close();
	
	if ( iTokenInterface )
		{
		iTokenInterface->Release();
		}
		
	if(iTokenHandle)
		{
		iTokenHandle->Release();
		}
		
	iFs.Close();
	
	return;
	}
