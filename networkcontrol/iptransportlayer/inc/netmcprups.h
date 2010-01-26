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
 @file
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_NETMCPRUPS_H
#define SYMBIAN_NETMCPRUPS_H

#define SYMBIAN_NETWORKING_UPS
#ifdef SYMBIAN_NETWORKING_UPS

#include "netmcpr.h"


namespace NetUps
	{
	class CNetUps;
	}

//

NONSHARABLE_CLASS(CUpsNetworkMetaConnectionProvider) : public CNetworkMetaConnectionProvider
/**
CUpsNetworkMetaConnectionProvider

UPS specific support in NetMCpr base class.
*/
    {
public:
	static CUpsNetworkMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory,
	                                               const ESock::TProviderInfo& aProviderInfo);

public:
    struct TUpsClientHandleRefCount
    	{
    	TUpsClientHandleRefCount(const Messages::TNodeId& aCommsId, TInt32 aCount) : iCommsId(aCommsId), iCount(aCount) {}
    	
		Messages::TNodeId iCommsId;
    	TInt32			  iCount;
    	};

public:

	// from CMetaConnectionProviderBase
	void ShowAccessPointRecordL(CommsDat::CMDBSession* aSession, CommsDat::CCDAccessPointRecord* aApRec);

	// @TODO PREQ1116 - revisit (see TUpsProcessProviderStatusChange)
	inline TBool ProviderStatusDown() const;
	inline void SetProviderStatusDown(TBool aStatus);

	inline TBool UpsDisabled() const;
	inline void SetUpsDisabled(TBool aUpsDisabled);
	
	inline const TDesC& ApName() const;				// retrieve Access Point name
	inline void SetApNameL(const TDesC& aApName);	// store Access Point name
	inline void FreeApName();						// free Access Point Name storage

	inline NetUps::CNetUps* NetUps();
	inline void SetNetUps(NetUps::CNetUps* aNetUps);

	inline TBool UpsControlClientPresent();
	inline  void SetUpsControlClientPresent();
	
	void CloseNetUps();

	TBool FindUpsClientHandle(const Messages::TNodeId& aCommsId, TInt32& aIndex, TInt32& aCount);
	void  IncrementUpsClientHandle(const Messages::TNodeId& aCommsId);
	void  DecrementUpsClientHandle(const Messages::TNodeId& aCommsId, TBool& aAllHandlesDeleted);
	void  AddUpsClientCommsIdL(const Messages::TNodeId& aCommsId);
	
protected:
    CUpsNetworkMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory,
                                   const ESock::TProviderInfo& aProviderInfo,
                                   const MeshMachine::TNodeActivityMap& aActivityMap);
    void ConstructL();

	virtual ~CUpsNetworkMetaConnectionProvider();

private:
	RBuf	iApName;					// Access Point record name
	NetUps::CNetUps* iNetUps;
	TInt	iNetUpsRefCount;
	TUint	iProviderStatusDown:1;	    // Set if TProviderStatusChange "down" has been received
									    // @TODO PREQ1116 - hack - revisit (see TUpsProcessProviderStatusChange)
	TUint	iUpsDisabled:1;			    // Set if UPS has been disabled ("short circuited")
	TUint 	iUpsControlClientPresent:1; // Indicates that the there are multiple UPS clients associated with this Node.

    RPointerArray<TUpsClientHandleRefCount> iUpsClientHandleRefCount;
    };

#include "netmcprups.inl"

#endif //SYMBIAN_NETWORKING_UPS

#endif //SYMBIAN_NETMCPRUPS_H
