// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \file mmlist.h
// Multimode ETel API v1.0
// Header file for phone list classes
// 
//

#ifndef _MMLIST_H_
#define _MMLIST_H_

#include "ETELMM.H"
#include <e32base.h>

class CMobilePhoneListBase : public CBase
/**
class CMobilePhoneListBase mmlist.h "INC/mmlist.h"
brief Is a base class within a thin-template implementation
Implements the methods for "read-only" access to a list class
CMobilePhoneListBase inherits from CBase
@internalComponent
*/
	{
public:
	~CMobilePhoneListBase();
	
	IMPORT_C TInt Enumerate() const;
	IMPORT_C TInt MaxNumberEntries() const;
	IMPORT_C void SetMaxNumberEntries(TInt aMax);

	IMPORT_C void StoreL(TDes8& aDes);
	IMPORT_C void RestoreL(const TDesC8& aBuf);
	IMPORT_C CBufBase* StoreLC();

	IMPORT_C void InternalizeL(RReadStream& aStream);
	IMPORT_C void ExternalizeL(RWriteStream& aStream) const;

	enum 
		{
		KMaxEntriesNotSet = - 1
		};

	enum
		{
		EBadRange,
		EListIndexOutOfRange,
		EListMaxNumberReached
		};

protected:

	CMobilePhoneListBase(TInt aLength, TInt aGranularity);

	IMPORT_C const TAny* GetEntryL(TInt aIndex) const;
	IMPORT_C void AddEntryL(const TAny* aEntry);

	virtual void InternalizeEntryL(TAny* aPtr,RReadStream& aStream) const=0;
	virtual void ExternalizeEntryL(const TAny* aPtr,RWriteStream& aStream) const=0;

protected:
	CArrayFixFlat<TAny> iList;
	TInt iMaxNumber;
	};

class CMobilePhoneEditableListBase : public CMobilePhoneListBase
/**
class CMobilePhoneEditableListBase mmlist.h "INC/mmlist.h"
brief Is a base class within a thin-template implementation
Implements the methods for "write" access to a list class
CMobilePhoneEditableListBase inherits from CMobilePhoneListBase
@internalComponent
*/
{
public:
	~CMobilePhoneEditableListBase();

	IMPORT_C void DeleteEntryL(TInt aIndex);
protected:
	IMPORT_C void InsertEntryL(TInt aIndex, const TAny* aEntry);

	CMobilePhoneEditableListBase(TInt aLength, TInt aGranularity);
};

template<class T>
class CMobilePhoneReadOnlyList : public CMobilePhoneListBase
/**
class CMobilePhoneReadOnlyList mmlist.h "INC/mmlist.h"
brief Is a template class within a thin-template implementation
Provides the type-safe wrappers for "read-only" access to a list class
CMobilePhoneReadOnlyList inherits from CMobilePhoneListBase
@internalComponent
*/
{
protected:
	inline CMobilePhoneReadOnlyList();
public:
	inline void AddEntryL(const T& aEntry);
	inline const T& GetEntryL(TInt aIndex) const; 

private:
	void InternalizeEntryL(TAny* aPtr,RReadStream& aStream) const
	{aStream >> *(new(aPtr) T);} // use in place new to construct T
	void ExternalizeEntryL(const TAny* aPtr,RWriteStream& aStream) const
		{aStream << *static_cast<const T*>(aPtr);}
};

template<class T>
class CMobilePhoneEditableList : public CMobilePhoneEditableListBase
/**
class CMobilePhoneEditableList mmlist.h "INC/mmlist.h"
brief Is a template class within a thin-template implementation
Provides the type-safe wrappers for "read-write" access to a list class
CMobilePhoneEditableList inherits from CMobilePhoneEditableListBase
@internalComponent
*/
{
protected:
	inline CMobilePhoneEditableList();
public:
	inline void AddEntryL(const T& aEntry);
	inline const T& GetEntryL(TInt aIndex) const; 
	inline void InsertEntryL(TInt aIndex, const T& aEntry);
	inline void ChangeEntryL(TInt aIndex, const T& aEntry);

private:
	void InternalizeEntryL(TAny* aPtr,RReadStream& aStream) const
	{aStream >> *(new(aPtr) T);} // use in place new to construct T
	void ExternalizeEntryL(const TAny* aPtr,RWriteStream& aStream) const
		{aStream << *static_cast<const T*>(aPtr);}
};

/********************************************************************/
//
// CMobilePhoneReadOnlyList<T>
// Implementation of the inline wrapper methods
//
/********************************************************************/

/**
@internalComponent
*/

template <class T>
inline CMobilePhoneReadOnlyList<T>::CMobilePhoneReadOnlyList()
	: CMobilePhoneListBase(sizeof(T), 1)
	{}

template <class T>
inline const T& CMobilePhoneReadOnlyList<T>::GetEntryL(TInt aIndex) const
	{return(*((const T*)CMobilePhoneListBase::GetEntryL(aIndex)));}

template <class T>
inline void CMobilePhoneReadOnlyList<T>::AddEntryL(const T& aEntry)
	{CMobilePhoneListBase::AddEntryL(&aEntry);}

/********************************************************************/
//
// CMobilePhoneEditableList<T>
// Implementation of the inline wrapper methods
//
/********************************************************************/

/**
@internalComponent
*/
template <class T>
inline CMobilePhoneEditableList<T>::CMobilePhoneEditableList()
	: CMobilePhoneEditableListBase(sizeof(T), 1)
	{}

template <class T>
inline const T& CMobilePhoneEditableList<T>::GetEntryL(TInt aIndex) const
	{return(*((const T*)CMobilePhoneListBase::GetEntryL(aIndex)));}

template <class T>
inline void CMobilePhoneEditableList<T>::AddEntryL(const T& aEntry)
	{CMobilePhoneListBase::AddEntryL(&aEntry);}

template <class T>
inline void CMobilePhoneEditableList<T>::InsertEntryL(TInt aIndex, const T& aEntry)
	{CMobilePhoneEditableListBase::InsertEntryL(aIndex, &aEntry);}

template <class T>
inline void CMobilePhoneEditableList<T>::ChangeEntryL(TInt aIndex, const T& aEntry)
	{*((T*)CMobilePhoneListBase::GetEntryL(aIndex))=aEntry;}


class CMobilePhoneNetworkList : public CMobilePhoneReadOnlyList<RMobilePhone::TMobilePhoneNetworkInfoV1>
/**
class CMobilePhoneNetworkList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-only" list of detected networks retrieved from the phone
CMobilePhoneNetworkList inherits from CMobilePhoneReadOnlyList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneNetworkList* NewL();
	IMPORT_C ~CMobilePhoneNetworkList ();
protected:
	CMobilePhoneNetworkList();
private:
	void ConstructL();
};


class CMobilePhoneCFList : public CMobilePhoneReadOnlyList<RMobilePhone::TMobilePhoneCFInfoEntryV1>
/**
class CMobilePhoneCFList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-only" list of Call Forwarding status entries retrieved from the phone
CMobilePhoneCFList inherits from CMobilePhoneReadOnlyList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneCFList* NewL();
	IMPORT_C ~CMobilePhoneCFList ();
protected:
	CMobilePhoneCFList();
private:
	void ConstructL();
};


class CMobilePhoneCBList : public CMobilePhoneReadOnlyList<RMobilePhone::TMobilePhoneCBInfoEntryV1>
/**
class CMobilePhoneCBList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-only" list of Call Barring status entries retrieved from the phone
CMobilePhoneCBList inherits from CMobilePhoneReadOnlyList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneCBList* NewL();
	IMPORT_C ~CMobilePhoneCBList();
protected:
	CMobilePhoneCBList();
private:
	void ConstructL();
};


class CMobilePhoneCWList : public CMobilePhoneReadOnlyList<RMobilePhone::TMobilePhoneCWInfoEntryV1>
/**
class CMobilePhoneCWList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-only" list of Call Waiting status entries retrieved from the phone
CMobilePhoneCWList inherits from CMobilePhoneReadOnlyList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneCWList* NewL();
	IMPORT_C ~CMobilePhoneCWList();
protected:
	CMobilePhoneCWList();
private:
	void ConstructL();
};

class CMobilePhoneCcbsList : public CMobilePhoneReadOnlyList<RMobilePhone::TMobilePhoneCCBSEntryV1>
/**
class CMobilePhoneCcbsList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-only" list of CCBS request entries retrieved from the phone
CMobilePhoneCcbsList inherits from CMobilePhoneReadOnlyList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneCcbsList* NewL();
	IMPORT_C ~CMobilePhoneCcbsList();
protected:
	CMobilePhoneCcbsList();
private:
	void ConstructL();
};


class CMobilePhoneGsmSmsList : public CMobilePhoneReadOnlyList<RMobileSmsStore::TMobileGsmSmsEntryV1>
/**
class CMobilePhoneGsmSmsList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-only" list of SMS messages retrieved from the phone
CMobilePhoneGsmSmsList inherits from CMobilePhoneReadOnlyList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneGsmSmsList* NewL();
	IMPORT_C ~CMobilePhoneGsmSmsList ();
protected:
	CMobilePhoneGsmSmsList();
private:
	void ConstructL();
};

class CMobilePhoneSmspList : public CMobilePhoneEditableList<RMobileSmsMessaging::TMobileSmspEntryV1>
/**
class CMobilePhoneSmspList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-write" list of SMS parameter entries retrieved from the phone
CMobilePhoneSmspList inherits from CMobilePhoneEditableList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneSmspList* NewL();
	IMPORT_C ~CMobilePhoneSmspList ();
protected:
	CMobilePhoneSmspList();
private:
	void ConstructL();
};

class CMobilePhoneNamList : public CMobilePhoneEditableList<RMobileNamStore::TMobileNamEntryV1>
/**
class CMobilePhoneNamList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-write" list of NAM entries retrieved from the phone
CMobilePhoneNamList inherits from CMobilePhoneEditableList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneNamList* NewL();
	IMPORT_C ~CMobilePhoneNamList();
protected:
	CMobilePhoneNamList();
private:
	void ConstructL();
};

class CMobilePhoneBroadcastIdList : public CMobilePhoneEditableList<RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1>
/**
class CMobilePhoneBroadcastIdList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-write" list of Broadcast ID entries retrieved from the phone
CMobilePhoneBroadcastIdList inherits from CMobilePhoneEditableList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneBroadcastIdList* NewL();
	IMPORT_C ~CMobilePhoneBroadcastIdList ();
	IMPORT_C void AddRangeEntryL(const RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1& aStart, const RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1& aEnd);

protected:
	CMobilePhoneBroadcastIdList();
private:
	void ConstructL();
};

class CMobilePhoneONList : public CMobilePhoneEditableList<RMobileONStore::TMobileONEntryV1>
/**
class CMobilePhoneONList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-write" list of Own Numbers retrieved from the phone
CMobilePhoneONList inherits from CMobilePhoneEditableList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneONList* NewL();
	IMPORT_C ~CMobilePhoneONList();
protected:
	CMobilePhoneONList();
private:
	void ConstructL();
};

class CMobilePhoneENList : public CMobilePhoneReadOnlyList<RMobileENStore::TMobileENEntryV1>
/**
class CMobilePhoneENList mmlist.h "INC/mmlist.h"
brief Is an instantiation of the list thin-template
Used to hold the "read-only" list of Emergency Numbers retrieved from the phone
CMobilePhoneENList inherits from CMobilePhoneReadOnlyList
@internalComponent
*/
{
public:
	IMPORT_C static CMobilePhoneENList* NewL();
	IMPORT_C ~CMobilePhoneENList();
protected:
	CMobilePhoneENList();
private:
	void ConstructL();
};

#endif
