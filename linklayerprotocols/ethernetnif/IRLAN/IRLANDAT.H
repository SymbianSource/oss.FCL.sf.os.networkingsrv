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
// IrLAN data model header file
// Written by Mal September 1997
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined (__IRLANDAT_H__)
#define __IRLANDAT_H__

#include <ir_sock.h>

#define TINYTP_PROTOCOL			_L("IrTinyTP")  //< Defines Protocol Type.
#define IRLAN_CLASSNAME			_L8("IrLAN")	//< Defines LAN Classname
#define TINYTP_ATTRNAME			_L8("IrDA:TinyTP:LsapSel") //< Defines Attribute Name.

//################# CLASS DEFINITIONS #############################

enum TIrlanCmdNumber
{

	EGetProviderInfo,			//< Specifies the Provider info. 
	EGetMediaCharacteristics,	//< Specifies Media Characteristics. 
	EOpenDataChannel			//< Specifies Open data channel. 
};

// IrLAN register classes - the information model.  An instance of this
// is initialised upon creation of the IrLAN driver and it holds info
// regarding all the required command parameters.

enum TIrlanQueryResult 
{
	EIrlanSuccess,					//< Specifies Success
	EIrlanInsufficientResources,	//< Specifies Insufficient Resources.
	EIrlanInvalidFormat,			//< Specifies Invalid format
	EIrlanCommand,					//< Specifies command.
	EIrlanParameter,				//< Specifies parameter
	EIrlanValue,					//< Specifies parameter value
	EIrlanNotOpen,					//< Specifies not open
	EIrlanAuthentication,			//< Specifies Authentication
	EIrlanPassword,					//< Specifies Password
	EIrlanProtocol,					//< Specifies Protocol
	EIrlanAsynch=255				//< Specifies Asynchronous.
};

/*
class CIrlanCommand;
class CIrlanParameter;
class CIrlanRegister : public CBase
	{
public:
	CIrlanRegister();
	~CIrlanRegister();
	static CIrlanRegister *NewL();
	void RegisterEntryL(const TDesC8& aBuffer);		// Called from the CNetDatabaseServProvd
	TInt UnregisterEntry(const TDesC8& aBuffer);	// Called from the CNetDatabaseServProvd
	TIrlanQueryResult Lookup(const TUint aCode,
	  const TDesC8& aParameter,TDesC8 &aValue);
	void SetHostName(const TDesC8 &aName);
	void GetHostName(TDes8 &aName);
private:
	CIrlanCommand *LookUpCommandCode(const TUint aCode);
   	void InitL();				// Registers the compulsory IAS entries.
private:
	TDblQue<CIrlanCommand> iCommandList;
	};

// eg. command GetProviderInformation with code 0 has two parameters 
class CIrlanCommand : public CBase
	{
public:
	CIrlanCommand();
	~CIrlanCommand();
	static CIrlanCommand *NewL(const TDesC8& aName,TUint aID);
	CIrlanParameter* LookUpParameter(const TDesC8& aName);
	void AddParameterL(const TDesC8& aName,const TDesC8& aValue);
	TBool Match(const TDesC8& aName)const;
	TUint GetCommandCode();
	TInt RemoveParameter(const TDesC& aParameterName);
private:
	void InitL(const TDesC8& aName,TUint aID);
public:
	TDblQueLink iLink;
private:
	HBufC *iParameterName;
	TUint iCommandCode;
	TDblQue<CIrlanParameter> iIrlanParameterList;
	};
*/

/**
@internalComponent
*/
enum TIrlanParameterType 
{
	EIrlanParameterString,			//< Specifies parameter string.
	EIrlanParameterOctetSeq,		//< Specifies parameter octet sequence.
	EIrlanParameterInteger,			//< Specifies parameter integer.
	EIrlanParameterByte				//< Specifies parameter byte.
};

class CIrlanParameterValue;
/**
Provides Irlan parameter data model fucntionality
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanParameter) : public CBase
{								 
public:
	CIrlanParameter();
	~CIrlanParameter();
	static CIrlanParameter *NewL(const TDesC8& aName,const TUint8 aID);
	void AddParameterValueL(const TDesC8& aName,const TIrlanParameterType& aType);
	TInt RemoveParameterValue(const TDesC8& aValue);
	/** For single param value cases. */
	CIrlanParameterValue* LookUpParameterValue();   
	CIrlanParameterValue* LookUpParameterValue(const TDesC8& aName);
	TBool Match(const TDesC8& aName)const;
	TUint8 GetCommandCode();
	TDblQueLink iLink;
private:
	void InitL(const TDesC8& aName,TUint8 aCode);
private:
	HBufC8* iParamName;
	TUint8 iCommandCode;
	TDblQue<CIrlanParameterValue> iIrlanParameterValueList;
};

/**
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanParameterValue) : public CBase
{
public:
	CIrlanParameterValue();
	~CIrlanParameterValue();
	static CIrlanParameterValue *NewL(const TDesC8& aValue,const TIrlanParameterType& aType);
	TBool Match(const TDesC8& aValue) const;
	void GetValue(TPtr8& aDes);
	TUint16 GetValueLength();
private:
	void InitL(const TDesC8& aValue,const TIrlanParameterType& aType);
public:
	TDblQueLink iLink;
private:
	HBufC8* iParamValue;
	TUint16 iParamValueLength;
	TIrlanParameterType iParamValueType;
};

#endif
