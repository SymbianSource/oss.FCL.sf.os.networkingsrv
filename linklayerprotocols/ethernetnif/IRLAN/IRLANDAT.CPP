// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Irlan parameter data model code 
// 
//

/**
 @file
*/

#include <e32base.h>
#include "IRLANUTL.H"
#include "IRLANDAT.H"

/**
current IrLAN version 1.1
@internalComponent
*/

/*
CIrlanRegister::CIrlanRegister()
	{
	__DECLARE_NAME(_S("CIrlanRegister"));
    iParameterList.SetOffset(_FOFF(CIrlanCommand,iLink));
	iUnique=1;
	}

CIrlanRegister::~CIrlanRegister()
	{
	CIrlanCommand *cl;
	TDblQueIter<CIrlanCommand> i(iParameterList);

	while (cl=i++,cl!=NULL)
		{
		cl->iLink.Deque();	   
		delete cl;
		}
	}

CIrlanRegister *CIrlanRegister::NewL()
// Invoked to create a new instance of the IAS register object
	{
	CIrlanRegister *reg=new (ELeave) CIrlanRegister();
	reg->InitL();
	return reg;
	}

void CIrlanRegister::InitL()
// register the compulsory device classes
	{
	TIASDatabaseEntry ent;
	SetHostName(PROTEA_DEFAULT_NICKNAME);
	ent.SetClassName(IRDA_IAS_DEVICE_CLASSNAME);
	ent.SetAttributeName(IRDA_IAS_DEVICENAME_ATTR);
	ent.SetToCharString(iHostName);
	RegisterEntryL(ent);

	ent.SetClassName(IRDA_IAS_DEVICE_CLASSNAME);
	ent.SetAttributeName(IRDA_IAS_LMPSUPPORT_ATTR);
	TBuf8<3> data;
	data.SetLength(3);
	data[0]=KIrLMPVersionMask;
	data[1]=KIASSupportMask;
	data[2]=KIrLMPSupportMask;
	ent.SetToOctetSeq(data);
	RegisterEntryL(ent);
	return;
	}

void CIrlanRegister::RegisterEntryL(const TDesC8& aBuffer)
// Add a new command to the data base
	{
	TIASDatabaseEntry ent;
	ent.Copy(aBuffer);
	CIASClass * cls=LookUpCommand(ent().iClassName);
	if (cls==NULL)
		{
		TInt uniqueID=0; // "Device" is always id 0 - autoincrement for others.
		if (ent().iClassName!=IRDA_IAS_DEVICE_CLASSNAME)		
			uniqueID=iUnique++;
		cls=CIASClass::NewL(ent().iClassName,uniqueID);
		iIASClassList.AddLast(*cls);
		}
	cls->AddAttributeL(ent().iAttributeName,ent().iData);
	}

TInt CIrlanRegister::UnregisterEntry(const TDesC8& aBuffer)
//
// Remove a whole class from the database.
//
	{
	TIrlanDatabaseEntry ent;
	ent.Copy(aBuffer);
	CIASClass * cls=LookUpClass(ent().iClassName);
	if (cls==NULL)
		return KErrNotFound;
	return cls->RemoveAttribute(ent().iAttributeName);
	}

TIASQueryResult CIASRegister::Lookup(const TDesC8& aClassName,
  const TDesC8& anAttributeName,TUint16& anObjectId,TIASResponse& aResponse)
	{
	CIASClass* cls=LookUpClass(aClassName);
	if (cls!=NULL)
		{
		anObjectId=cls->LookUpObjectID();

		CIASAttribute* attr=cls->LookUpAttribute(anAttributeName);
		if (attr!=NULL)
			attr->GetAttributeData(aResponse);
		else
			return EIASNoSuchAttribute;
		}
	else
		return EIASNoSuchClass;

	return EIASSuccess;
	}

CIASClass* CIASRegister::LookUpClass(const TDesC8& aName)
//
// Go through the list of classes and see if any of them match
// on names with aName.
//
	{
	CIASClass *cl;
	TDblQueIter<CIASClass> i(iIASClassList);

	while (cl=i++,cl!=NULL)
		{
		if (cl->Match(aName))
			return cl;
		}
	return NULL;
	}

void CIASRegister::SetHostName(const TDesC8 &aName)
	{
	iHostName.Copy(aName);
	}

void CIASRegister::GetHostName(TDes8 &aName)
	{
	aName.Copy(iHostName);
	}

 */

//*******************************************************************

/**
Constructor.
*/
CIrlanParameter::CIrlanParameter()
{
	__DECLARE_NAME(_S("CIrlanParameter"));
}

/**
Destructor.
*/
CIrlanParameter::~CIrlanParameter()
{
	CIrlanParameterValue *val;
	TDblQueIter<CIrlanParameterValue> i(iIrlanParameterValueList);

	delete iParamName;
	while (val=i++,val!=NULL)
		{
		val->iLink.Deque();	
		delete val;
		}
}

/**
Invoked to create a new instance of an irlan parameter object.
@param aName parameter name.
@param aID   parameter ID.
@return A pointer to CIrlanParameter object.
*/
CIrlanParameter *CIrlanParameter::NewL(const TDesC8& aName,const TUint8 aID)
{
	CIrlanParameter *cl=new (ELeave) CIrlanParameter();
	CleanupStack::PushL(cl);
	cl->InitL(aName,aID);
	CleanupStack::Pop();
	return cl;
}

/**
We don't initialise attributes here.  Must subsequently invoke.
Add the newly created object to an parameter list.
@param aName The parameter name.
@param aCode The Command Code.
*/
void CIrlanParameter::InitL(const TDesC8& aName,TUint8 aCode)
{
	iParamName=HBufC8::NewL(aName.Length());
	TPtr8 temp=iParamName->Des();
	temp=aName;

	iCommandCode=aCode;
    iIrlanParameterValueList.SetOffset(_FOFF(CIrlanParameterValue,iLink));
}

/**
Adds the parameter value to List.
@param aName parameter name.
@param aType parameter type.
*/
void CIrlanParameter::AddParameterValueL(const TDesC8& aName,const TIrlanParameterType& aType)
{
	if (LookUpParameterValue(aName)!=NULL)
		User::Leave(KErrAlreadyExists);
	CIrlanParameterValue *val=CIrlanParameterValue::NewL(aName,aType);
	iIrlanParameterValueList.AddFirst(*val);
}

/**
Searchs a parameter in the parameter List.
@return A pointer to CIrlanParameterValue class
*/
CIrlanParameterValue* CIrlanParameter::LookUpParameterValue()
{
	CIrlanParameterValue *val=NULL;
	CIrlanParameterValue *ret=NULL;
	TDblQueIter<CIrlanParameterValue> i(iIrlanParameterValueList);

	TInt count=0;
	while (val=i++,val!=NULL)
		{
		ret=val;
		count++;
		}
	__ASSERT_DEBUG(count<=1,IrlanUtil::Fault(EIrlanInvalidParameter));
	return ret;
}

/**
Searchs a parameter in the parameter List depending on the value.
@param aValue A parameter value to be searched.
@return A pointer to CIrlanParameterValue class
*/
CIrlanParameterValue* CIrlanParameter::LookUpParameterValue(const TDesC8& aValue)
{
	CIrlanParameterValue *val;
	TDblQueIter<CIrlanParameterValue> i(iIrlanParameterValueList);

	while (val=i++,val!=NULL)
		{
		if (val->Match(aValue))
			return val;
		}
	return NULL;
}

/**
Remove the paramter value from the parameter list.
@param aValue A parameter value to be removed.
@return KErrNone if success else KErrNotFound.
*/
TInt CIrlanParameter::RemoveParameterValue(const TDesC8& aValue)
{
	CIrlanParameterValue *val=LookUpParameterValue(aValue);
	if (val==NULL)
		return KErrNotFound;
	val->iLink.Deque();
	delete val;
	if (iIrlanParameterValueList.IsEmpty())
		{
		iLink.Deque();
		delete this;
		}
	return KErrNone;
}

/**
Searches this descriptor's data for a match with the match pattern supplied in the specified 
descriptor.
@param aName A 8 bit non modifable descriptor containing the match pattern
@return If a match is found, the offset within this descriptor's data where the match first 
		occurs. 
*/
TBool CIrlanParameter::Match(const TDesC8& aName)const
{
	return (iParamName->Compare(aName)==0);
}

/**
Get the command code.
@return The command code.
*/
TUint8 CIrlanParameter::GetCommandCode()
{
	return iCommandCode;
}

//*******************************************************************

/**
Constructor.
*/
CIrlanParameterValue::CIrlanParameterValue()
{
	__DECLARE_NAME(_S("CIrlanParameter"));
}

/**
Destructor.
*/
CIrlanParameterValue::~CIrlanParameterValue()
{
	delete iParamValue;
}

/**
Invoked to create a new instance of an irlan parameter object.
@param aValue parameter value.
@param aType  parameter Type.
@return A pointer to CIrlanParameterValue object.
*/
CIrlanParameterValue* CIrlanParameterValue::NewL(const TDesC8& aValue,const TIrlanParameterType& aType)
{
	CIrlanParameterValue *val=new (ELeave) CIrlanParameterValue();
	CleanupStack::PushL(val);
	val->InitL(aValue,aType);
	CleanupStack::Pop();
	return val;
}

/**
Add the newly created object to an parameter list.
@param aValue parameter value.
@param aType  parameter Type.
*/
void CIrlanParameterValue::InitL(const TDesC8& aValue,const TIrlanParameterType& aType)
{
	iParamValue=HBufC8::NewL(aValue.Length());
	TPtr8 temp=iParamValue->Des();
	temp=aValue;
	
	iParamValueLength=TUint16(aValue.Length());
	iParamValueType=aType;
}

/**
Searches this descriptor's data for a match with the match pattern supplied in the specified 
descriptor.
@param aName A 8 bit non modifable descriptor containing the match pattern
@return If a match is found, the offset within this descriptor's data where the match first 
		occurs. 
*/
TBool CIrlanParameterValue::Match(const TDesC8& aName) const
{
	return (iParamValue->Compare(aName)==0);
}

/**
Gets a 8-bit integer value from the buffer.
@param aDes On completion, the 8-bit value from the buffer
*/
void CIrlanParameterValue::GetValue(TPtr8& aDes)
{
	TPtr8 val=iParamValue->Des();
	aDes.Set(val);
}

/**
Gets the length of the Parameter value.
@return The 16-bit parameter value.
*/
TUint16 CIrlanParameterValue::GetValueLength()
{
	return iParamValueLength;
}
