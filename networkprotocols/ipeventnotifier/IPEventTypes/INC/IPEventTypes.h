// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file IPEventTypes.h
 @publishedPartner
 @released
*/


#ifndef __IPEVENTTYPES_H__
#define __IPEVENTTYPES_H__

#include <e32math.h>
#include <comms-infras/metadata.h>
#include <networking/ipeventtypesids.h>
#include <in_sock.h>


namespace IPEvent
{


#define IP_EVENT_DECL( _class, _uid, _type ) \
public: \
	static inline _class* NewL(void) \
		{ STypeId typeId = { (_uid ) , (_type) }; \
		  return static_cast< _class* >(SMetaDataECom::NewInstanceL(typeId)); } \
	static inline _class* LoadL(TPtrC8& aDes) \
		{ return static_cast< _class* >(SMetaDataECom::LoadL(aDes)); } \
	static TUid GetUid(void) { return TUid::Uid(_uid) ; } \
protected: \
	DATA_VTABLE


class CIPEventType : public Meta::SMetaDataECom
	{
public:
	static SMetaDataECom* InstantiateL(TAny* aParams);
	TYPEID_TABLE
	};

class CMFlagReceived : public CIPEventType
	{
public:
	IP_EVENT_DECL( CMFlagReceived , KEventImplementationUid, EMFlagReceived )

public: // data accessors
	inline TBool GetMFlag() const;
	inline void SetMFlag(TBool aFlag);
	inline TBool GetOFlag() const;
	inline void SetOFlag(TBool aFlag);

private:
	TBool iMFlag;
	TBool iOFlag;	//If any more attributes are added please pack everything in more effectively
	};


class CIPReady : public CIPEventType
	{
	IP_EVENT_DECL( CIPReady , KEventImplementationUid, EIPReady )

public: // data accessors
	inline const TInetAddr& GetIPAddress() const;
	inline void SetIPAddress(const TInetAddr& aAddr);

	inline TBool GetAddressValid() const;
	inline void  SetAddressValid(TBool aAddrValid);

private:
	TInetAddr iIPAddress;
	TBool     iAddressValid;
	};



class CLinklocalAddressKnown : public CIPEventType
	{
	IP_EVENT_DECL( CLinklocalAddressKnown , KEventImplementationUid, ELinklocalAddressKnown )

public: // data accessors
	inline const TInetAddr & GetIPAddress() const;
	inline void SetIPAddress(const TInetAddr& aAddr);

private:
	TInetAddr iIPAddress;
	};




#include <networking/ipeventtypes.inl>

}


#endif // __IPEVENTTYPES_H__



