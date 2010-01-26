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
// [Defaults]
// defaults= another_config_file.txt
// [SectionName]
// variable= value
// variable2= value2
// variable= value3
// [SectionName2]
// variable= value
// endscript
// A configuration file is made up of a number of "sections", each of which can contain a number of "items" (name, value combination).
// "Sections" must have a name and be surrounded by square backets, e.g.:
// [SectionName]
// Each "item" consists of consists of a name, followed by an equals sign, followed by blank space, followed by the value to assign to that variable.
// The value can be of any length, contain whitespace and span multiple lines. The value ends when the next item or section is found. E.g:
// Simple Item:
// variable= value
// Two items on one line:
// variable= value variable2= value2
// Multi-line item:
// variable= This variable
// spans multiple
// lines
// To specify default values for all sections, add a section at the start of the config file called [Defaults], e.g.:
// [Defaults]
// sc= +447785016005
// To read default values from another config file, add an item with name "defaults" and value is the name of the file. E.g.:
// defaults= another_config_file.txt
// 
//

/**
 @file testscripts.h Defines classes for reading a configuration file
 @note Configuration File Format:
 @note Explanation:
 @note Parsing stops at End-Of-File or if the tag "endscript" (without the quotes) appears in the file.
 @note A section may take some default values from another section or config file
*/


#ifndef __TEST_SCRIPTS_H__
#define __TEST_SCRIPTS_H__

#include <e32std.h>
#include <e32base.h>

class CTestConfigSection;
class CTestConfigItem;
class RFs;
class TParse;
class RFile;

_LIT(KScriptPanic, "TEST-SCRIPT");
_LIT(KScriptPathSep,"\\");
_LIT8(KScriptSectionStart, "[");
_LIT8(KScriptSectionStart8, "[");
_LIT8(KScriptSectionEnd8, "]");
_LIT8(KScriptCRLF, "\r\n");
_LIT8(KScriptCRLF8, "\r\n");
_LIT8(KScriptLF, "\n");
_LIT8(KScriptCR, "\r");
_LIT8(KScriptItemEnd, "=");
_LIT8(KScriptItemEnd8, "=");
_LIT8(KScriptSpace8, " ");
_LIT8(KScriptDefaults, "Defaults");
_LIT8(KScriptDefault1, "Def");
_LIT8(KScriptDefault2, "Default");

class CTestConfig : public CBase
	{
	public:
		IMPORT_C static CTestConfig* NewLC(RFs& aFs, const TDesC& aComponent, const TDesC& aScriptFile);
		IMPORT_C static CTestConfig* NewLC(RFs& aFs, const TDesC& aComponent);
		IMPORT_C ~CTestConfig();

		IMPORT_C const TDesC8& ItemValue(const TDesC8& aSection, const TDesC8& aItem, const TDesC8& aDefault) const;
		IMPORT_C TInt ItemValue(const TDesC8& aSection, const TDesC8& aItem, const TInt aDefault) const;
		
		IMPORT_C void ReadScriptL(const TDesC& aScript);

		inline const RPointerArray<CTestConfigSection>& Sections() const;
		inline RPointerArray<CTestConfigSection>& Sections();

		IMPORT_C const CTestConfigSection* Section(const TDesC8& aSectionName) const; //return NULL if section not found
		inline const CTestConfigSection& operator[](TInt aIndex) const {return *iSections[aIndex];}

		IMPORT_C static TInt CountElements(const TDesC8& aInput, TChar aDelimiter);
		IMPORT_C static TInt GetElement(const TDesC8& aInput, TChar aDelimiter, TInt aIndex, TInt& aOutput);
		IMPORT_C static TInt GetElement(const TDesC8& aInput, TChar aDelimiter, TInt aIndex, TPtrC8& aOutput);
		IMPORT_C static TPtrC8 Trim(const TDesC8& aInput);

		IMPORT_C static HBufC8* ReplaceLC(const TDesC8& aOld, const TDesC8& aNew, const TDesC8& aOldString);
		IMPORT_C static TInt ResolveFile(RFs& aFs, const TDesC& aComponent, const TDesC& aFileName, TParse& aParseOut);

		IMPORT_C void WriteFileL(const TDesC& aFileName);
		TBool operator==(const CTestConfig& aFile) const;

	protected:

		CTestConfig(RFs& aFs);
		void ConstructL(const TDesC& aComponent);

		TPtrC8 ParseValue(const TDesC8& aText, const TLex8& aInput, TInt aCurrentItemStart) const;
		void ParseAndSetItemValueL(const TDesC8& aText, const TLex8& aInput, TInt aCurrentItemStart, CTestConfigItem*& arCurrentItem);
		void FoundNewItemL(const TDesC8& aText, TLex8& arInput, TInt& arCurrentItemStart, CTestConfigSection& aSection, CTestConfigItem*& arCurrentItem);
		void CopyInDefaultsL(CTestConfigSection& aSection, const TDesC& aDefaultFile);

		HBufC8* ReadFileL(const TDesC& aFile) const;

		TBool IsDefaultSection(const TDesC8& aSectionName) const;
		static TInt GetNextElement(TLex8& aInput, TChar aDelimiter, TPtrC8& aOutput);

	protected:

		RFs& iFs;
		HBufC* iComponent;
		RPointerArray<CTestConfigSection> iSections;
	};

class CTestConfigSection : public CBase
	{
	friend class CTestConfig;

	public:
		IMPORT_C static CTestConfigSection* NewLC(const TDesC8& aSectionName);
		IMPORT_C static CTestConfigSection* NewLC(const TDesC8& aSectionName, CTestConfigSection& aDefaults);
		IMPORT_C ~CTestConfigSection();
		
		inline const TDesC8& SectionName() const;

		IMPORT_C const CTestConfigItem* Item(const TDesC8& aItem) const; //return NULL if the item does not exist
		IMPORT_C const CTestConfigItem* Item(const TDesC8& aItem,TInt aIndex) const; //return NULL if the item does not exist
		IMPORT_C TInt ItemCount(const TDesC8& aItem) const;
//		inline CTestConfigItem& ItemL(const TDesC8& aItem); //leaves with KErrNotFound if the item does not exist

		IMPORT_C const TDesC8& ItemValue(const TDesC8& aItem, const TDesC8& aDefault) const;
		IMPORT_C TInt ItemValue(const TDesC8& aItem, TInt aDefault) const;

		IMPORT_C CTestConfigItem& AddItemL(const TDesC8& aItem, const TDesC8& aValue);
		IMPORT_C void DeleteItemsL(const TDesC8& aItem);

		inline const RPointerArray<CTestConfigItem>& Items() const {return iItems;}
		inline RPointerArray<CTestConfigItem>& Items() {return iItems;}

		inline const CTestConfigItem& operator[](TInt aIndex) const  {return *iItems[aIndex];}

		inline void SetDefaultsL(const CTestConfigSection& aDefaults);
		inline CTestConfigSection* Defaults() const {return iDefaults;}

		IMPORT_C CTestConfigSection* CopyLC() const;

		void WriteL(RFile& aFile) const;
		TBool operator==(const CTestConfigSection& aFile) const;

	private:
		void ConstructL(const TDesC8& aSectionName);
		CTestConfigSection();
		RPointerArray<CTestConfigItem> iItems;
		HBufC8* iSectionName;
//		CTestConfigItem* iLastItem;
		CTestConfigSection* iDefaults;
	};

class CTestConfigItem : public CBase
	{
	friend class CTestConfigSection;
	friend class CTestConfig;

	public:
		IMPORT_C static CTestConfigItem* NewLC(CTestConfigSection& aParent, const TDesC8& aItem, const TDesC8& aValue);
		inline CTestConfigItem* CopyLC() const;

		IMPORT_C ~CTestConfigItem();
		inline const TDesC8& Item() const;
		inline const TDesC8& Value() const;

		void WriteL(RFile& aFile) const;
		TBool operator==(const CTestConfigItem& aItem) const {return Item() == aItem.Item() && Value() == aItem.Value();}

	public:

		CTestConfigSection& iParent;
		
	private:
		CTestConfigItem(CTestConfigSection& aParent);
		void ConstructL(const TDesC8& aItem, const TDesC8& aValue);
		HBufC8* iItem;
		HBufC8* iValue;
	};

#include "testconfigfile.inl"

#endif
