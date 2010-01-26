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

/**
 @file lib_pfkey.inl
 @publishedPartner
 @released
*/
//	class TPfkeyAnyExt
inline TPfkeyAnyExt::TPfkeyAnyExt(const TDesC8& aDes)
		: TPtrC8(aDes) {}
	
inline TPfkeyAnyExt::TPfkeyAnyExt() {}

inline TUint16 TPfkeyAnyExt::ExtLen() const
	{ return ExtHdr().sadb_ext_len; }

inline TUint16 TPfkeyAnyExt::ExtType() const
	{ return ExtHdr().sadb_ext_type; }

inline const struct sadb_ext& TPfkeyAnyExt::ExtHdr() const
	{ return *(const sadb_ext *)iPtr; }

inline struct sadb_ext& TPfkeyAnyExt::ExtHdr()
	{ return *(sadb_ext *)iPtr; }
	
//	template class TPfkeyExt
template <class T>
inline const T& TPfkeyExt<T>::Ext() const
	{ return *(const T *)(this->iBuf); }

template <class T>
inline T& TPfkeyExt<T>::Ext()
	{ return *(T *)(this->iBuf); }

template <class T>
inline TPfkeyExt<T>::TPfkeyExt()
	{
	this->FillZ(sizeof(T));
	}

//	class TPfkeyMsgBase
inline const struct sadb_msg& TPfkeyMsgBase::MsgHdr() const 
	{ return *(const struct sadb_msg*)iBuf; }

inline struct sadb_msg& TPfkeyMsgBase::MsgHdr() 
	{ return *(struct sadb_msg*)iBuf; }

inline TPfkeyMsgBase::TPfkeyMsgBase() :TBuf8<KPfkeyMsgMaxLen>(sizeof(sadb_msg)) 
	{}

inline TPfkeyMsgBase::TPfkeyMsgBase(const TDesC8& aOther) :TBuf8<KPfkeyMsgMaxLen>(aOther)
	{}
