// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @file policies.h
// This header file contain the classes that handle qos policies
// @internalTechnology
// @released
//

#ifndef __POLICIES_H__
#define __POLICIES_H__
	
#include "pfqosparser.h"

/**
 * Modulespec
 *
 * @internalTechnology 
 */
class CModuleSpec : public CBase
	{
public:
	static CModuleSpec* NewL(const TDesC& aFileName, const TDesC& aModuleName);
	static CModuleSpec* NewL(const TPfqosModule& aModule);
	~CModuleSpec();

	inline TUint ProtocolId() const;
	inline void SetProtocolId(TUint aProtocolId);
	inline TUint32 Flags() const;
	inline void SetFlags(TUint32 aFlags);
	inline const TDesC& FileName() const;
	inline const TDesC& Name() const;
	inline CExtension* PolicyData();

	TDblQueLink iDLink;

private:
	CModuleSpec();
	void ContructL(const TPfqosModule& aModule);
	void ContructL(const TDesC& aFileName, const TDesC& aModuleName);

private:
	TName		iName;			// Module name
	TFileName	iFileName;		// Filename of DLL module
	TUint32		iFlags;			// Module flags
	TUint		iProtocolId;	// Needed for NewProtocolL()
	CExtension*	iExtension;		// Config data
	};

// Inline methods
/** @internalTechnology */
inline TUint CModuleSpec::ProtocolId() const
	{ return iProtocolId; };

/** @internalTechnology */
inline void CModuleSpec::SetProtocolId(TUint aProtocolId)
	{ iProtocolId = aProtocolId; }

/** @internalTechnology */
inline TUint32 CModuleSpec::Flags() const
	{ return iFlags; };

/** @internalTechnology */
inline void CModuleSpec::SetFlags(TUint32 aFlags)
	{ iFlags = aFlags; };

/** @internalTechnology */
inline const TDesC& CModuleSpec::FileName() const
	{ return iFileName; };

/** @internalTechnology */
inline const TDesC& CModuleSpec::Name() const
	{ return iName; };

/** @internalTechnology */
inline CExtension* CModuleSpec::PolicyData()
	{ return iExtension; };


/**
 * Modulespec policy
 *
 * @internalTechnology 
 */
class CModuleSelector : public CSelectorBase
	{
public:
	CModuleSelector(TPfqosMessage& aMsg);
	CModuleSelector(CSelectorBase& aSel);
	~CModuleSelector();

	void AddModuleSpec(CModuleSpec& aModule);
	void RemoveModuleSpec(CModuleSpec* aModule);
	CModuleSpec* FindModuleSpec(TUint aProtocolId);
	inline TDblQue<CModuleSpec>& GetModuleList();

protected:
	TDblQue<CModuleSpec> iModules;
	};

// Inline methods
/** @internalTechnology */
inline TDblQue<CModuleSpec>& CModuleSelector::GetModuleList()
	{ return iModules; };


/**
 * Flowspec policy
 *
 * @internalTechnology 
 */
class CPolicySelector : public CSelectorBase
	{
public:
	CPolicySelector(TPfqosMessage& aMsg);
	CPolicySelector(CSelectorBase& aSel);
	~CPolicySelector();
	void SetQoSParameters(const TQoSParameters& aSpec) { iQoS = aSpec; };
	void SetQoSParameters(const TPfqosFlowSpec& aSpec);
	inline const TQoSParameters& QoSParameters() const;

private:
	TQoSParameters iQoS;
	};

// Inline methods
/** @internalTechnology */
inline const TQoSParameters& CPolicySelector::QoSParameters() const
	{ return iQoS; };

	
// Internal help function for logging only (defining this
// always should not cause a problem in release build, becuase
// it's neiter defined nor used).
extern void DumpSelector(const CSelectorBase& aSelector);


#endif
