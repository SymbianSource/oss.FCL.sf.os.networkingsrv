// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#if !defined(__PCKTPTR_H__)
#define __PCKTPTR_H__

#if !defined(__ETELPCKT_H__)
#include "etelpckt.h"
#endif

class CEtelPacketPtrHolder : public CBase
/**
@internalComponent
*/
	{
public:
	static CEtelPacketPtrHolder* NewL(const TInt aSizeOfPtrArray);
	virtual ~CEtelPacketPtrHolder();

	// The ptr holder slot numbers used by RPacketService asynchronous requests
	enum TPacketPtrHolderSlots
	{
		ESlotPacketStatus=0,
		ESlotRegStatus,
		ESlotGetNtwkReg,
		ESlotProtocolType,
		ESlotPpdAddress,
		ESlotNrcaApn,
		ESlotDynamicCaps,
		ESlotEnumerateCount,
		ESlotEnumerateMaxCount,
		ESlotContextInfoIndex,
		ESlotContextInfo,
		ESlotSetMsClass,
		ESlotNtfMsClass,
		ESlotCurrentMsClass,
		ESlotMaxMsClass,
		ESlotSetPrefBearer,
		EMaxNumPacketPtrSlots
	};

	// The ptr holder slot numbers used by RPacketContext asynchronous requests
	enum TPacketContextPtrHolderSlots
	{
		ESlotDataPort=0,
		ESlotContextStatus,
		ESlotDataVolume,
		ESlotGranularity,
		ESlotGetSpeed,
		ESlotNotifySpeed,
		EMaxNumPacketContextPtrSlots
	};

	// The ptr holder slot numbers used by RPacketQoS asynchronous requests
	enum TPacketQoSPtrHolderSlots
		{
		EMaxNumPacketQoSPtrSlots
		};

public:
	template <typename T> inline TPtr8& Set(TInt aSlot,T& aObject)
		{
		TPtr8& ptr=Ptr(aSlot);
		ptr.Set(reinterpret_cast<TText8*>(&aObject),sizeof(T),sizeof(T));
		return ptr;
		}
protected:
	virtual void ConstructL(const TInt aSizeOfPtrArray);	
	CEtelPacketPtrHolder();
private:
	TPtr8& Ptr(const TInt aIndex);
protected:
	RArray<TPtr8> iPtrArray;
	};

class CPacketQoSPtrHolder : public CEtelPacketPtrHolder
/**
CPacketQoSPtrHolder
@internalComponent
*/
{
public:
	static CPacketQoSPtrHolder* NewL(const TInt aSizeOfPtrArray);
	~CPacketQoSPtrHolder();
protected:
	CPacketQoSPtrHolder();
public:
	// currently there are no asynch. requests in RPacketQoS that pass argument by value
};

class CPacketContextPtrHolder : public CEtelPacketPtrHolder
/**
CPacketContextPtrHolder
@internalComponent
*/
{
public:
	static CPacketContextPtrHolder* NewL(const TInt aSizeOfPtrArray);
	~CPacketContextPtrHolder();
protected:
	CPacketContextPtrHolder();
public:
	RPacketContext::TNotifyDataTransferredRequest iNotifyDataTransferRequest; // SLOT_GRANULARITY (used in RPacketContext::NotifyDataTransferred)
	};

class CPacketPtrHolder : public CEtelPacketPtrHolder
/**
CPacketPtrHolder
@internalComponent
*/
	{
public:
	static CPacketPtrHolder* NewL(const TInt aSizeOfPtrArray);
	~CPacketPtrHolder();
protected:
	CPacketPtrHolder();
public:
	TInt iGetContextInfoIndex;	// SLOT_CONTEXT_INFO_INDEX (used in RPacketService::GetContextInfo() async. version)
	RPacketService::TMSClass iMSClass; // SLOT_SET_MS_CLASS (used in RPacketService::SetMSClass() async. version)
	RPacketService::TPreferredBearer iPrefBearer; // SLOT_SET_PREF_BEARER (used in RPacketService::SetPreferredBearer() async. version)
	};	

#endif

