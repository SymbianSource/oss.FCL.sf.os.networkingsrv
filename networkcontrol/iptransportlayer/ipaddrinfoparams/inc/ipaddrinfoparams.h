// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
* Copyright (c) Symbian Software Ltd. 2007.  All rights reserved.
*
*	@file ip_subconparams.h
*	Header file for the IP Address Info SubConnection Parameters
*/

#ifndef __IPADDRINFOPARAMS_H__
#define __IPADDRINFOPARAMS_H__

#include <es_sock.h>
#include <comms-infras/metadata.h>
#include <comms-infras/metatype.h>
#include <comms-infras/es_parameterfamily.h>

/** Provides Implementation of IP Address Info Parameters

@publishedPartner
@released since 9.5
*/
class CSubConIPAddressInfoParamSet : public CSubConGenericParameterSet
	{
	public:
		struct TSubConIPAddressInfo
		/** IP address Info structure

		@publishedPartner
		@released since 9.5
		*/
			{
			enum EState
				{
				ENone,
				EAdd,
				ERemove
				};

			inline TSubConIPAddressInfo(TSockAddr &iCliSrcAddr, TSockAddr &aCliDstAddr, TInt aProtocolId, EState aState);
			
			inline TBool Compare(TSubConIPAddressInfo aInfo);
			
			TSockAddr   iCliSrcAddr;
			TSockAddr   iCliDstAddr;
			TInt        iProtocolId;

			const EState	iState;
			};

		enum 
			{
			EUid = 0x102822D5,
			ETypeId = 1
			};
		
		enum EOperationCode
			{
			ENone,
			EDelete
			};
	
	public:
		inline static CSubConIPAddressInfoParamSet* NewL(RParameterFamily& aFamily, RParameterFamily::TParameterSetType aType);
		inline static CSubConIPAddressInfoParamSet* NewL();

		inline CSubConIPAddressInfoParamSet();
		inline ~CSubConIPAddressInfoParamSet();
		
		inline void AddParamInfo(TSubConIPAddressInfo aParam);
		inline void RemoveParamInfo(TUint aIndex);
		inline TInt DeleteParams();
		
		inline TSubConIPAddressInfo& GetParamInfoL(TUint aIndex);
		inline TUint GetParamNum();
		
		inline void SetOperationCode(EOperationCode aOpCode);
		inline EOperationCode GetOperationCode();

	protected:

		DATA_VTABLE

	protected:
		RArray<TSubConIPAddressInfo> iParams;
		EOperationCode iOpCode;
	};

#include <networking/ipaddrinfoparams.inl>

#endif
// __IPADDRINFOPARAMS_H__
