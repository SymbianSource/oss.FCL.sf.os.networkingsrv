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
// Started by Mal, August 1997
// IrLAN mbuf functions
// 
//

/**
 @file
 @internalComponent 
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include "IRLANUTL.H"
#include "IRLANBUF.H"

//#define __TRACEWIN__
#ifdef __TRACEWIN__
  #include <log.h>
#else
  #define LOG(a)
#endif

/**
Sets the command code.
@param aCode The Command Code.
*/
void RIrlanControlMBufChain::SetCommandCode(TUint8 aCode)
{
	*(this->First()->Ptr()+KCommandCodeFieldOffset)=aCode;
}

/**
Get the command code.
@return The command code.
*/
TUint8 RIrlanControlMBufChain::GetCommandCode()
{
	return *(this->First()->Ptr()+KCommandCodeFieldOffset);
}

/**
Sets the command code result.
@param aCode The Command Code.
*/
void RIrlanControlMBufChain::SetResultCode(TUint8 aCode)
{
	*(this->First()->Ptr()+KCommandCodeFieldOffset)=aCode;
}

/**
Gets the Command code result.
@param aCode The Command Code.
*/
TUint8 RIrlanControlMBufChain::GetResultCode()
{
	return *(this->First()->Ptr()+KCommandCodeFieldOffset);
}

/**
Sets the parameter count.
@param aCount A parameter count.
*/
void RIrlanControlMBufChain::SetParameterCount(TUint8 aCount)
{
	*(this->First()->Ptr()+KParameterCountFieldOffset)=aCount;
}

/**
Gets the parameter count.
@return The parameter count.
*/
TUint8 RIrlanControlMBufChain::GetParameterCount()
{
	return *(this->First()->Ptr()+KParameterCountFieldOffset);
}

/**
OK to use TUint8 for name length - it can only be up to 255 in size.
Sets the length of the Parameter name.
@param aOffset offset.
@param aLength Length of the parameter name.
*/
void RIrlanControlMBufChain::SetParameterNameLength(TInt aOffset,TUint8 aLength)
{
	*(this->First()->Ptr()+KParameterListFieldOffset+aOffset)=aLength;
}

/**
OK to use TUint8 for name length - it can only be up to 255 in size.
Gets the length of the parameter name.
@param aOffset offset.
@return The name length.
*/
TUint8 RIrlanControlMBufChain::GetParameterNameLength(TInt aOffset)
{
	return *(this->First()->Ptr()+KParameterListFieldOffset+aOffset);
}

/**
Sets the Parameter name.
@param aOffset offset.
@param aName The parameter name.
*/
void RIrlanControlMBufChain::SetParameterName(TInt aOffset,const TDesC8& aName)
{
	__ASSERT_DEBUG((TUint)aName.Length()<=KParameterNameMaxSize,IrlanUtil::Panic(EIrlanParameterNameTooBig));
	TUint offset=aOffset;
	TPtr8 temp((this->First()->Ptr()+KParameterListFieldOffset),
		offset,KParameterFrameMaxSize-offset);
	temp.Append(aName);
}

/**
Gets the Parameter name.
@param aOffset offset.
@param aLength The length of the parameter name.
@param aName The parameter name.
*/
void RIrlanControlMBufChain::GetParameterName(TInt aOffset,TInt aLength,TDes8& aName)
{
	__ASSERT_DEBUG((TUint)aLength<=KParameterNameMaxSize,IrlanUtil::Panic(EIrlanParameterNameTooBig));
	TUint offset=aOffset;
	TPtr8 name((this->First()->Ptr()+offset),aLength,KParameterNameMaxSize);
	aName=name;
}

/**
Gets the length of the Parameter value.
@param aOffset offset.
@return The lenght of the value.
*/
TUint16 RIrlanControlMBufChain::GetParameterValueLength(TInt aOffset)
{
//	TUint16 val;
//	TPtr8 p((TUint8 *)&val,2,4);
//	CopyOut(p,aOffset);
//	return val;
	return LittleEndian::Get16(this->First()->Ptr()+KParameterListFieldOffset+aOffset);
}

/**
Sets the length of the Parameter value.
@param aOffset offset.
@param aValue  The value of the parameter.
*/
void RIrlanControlMBufChain::SetParameterValueLength(TInt aOffset,TUint16 aValue)
{
//	TPtr8 val((TUint8 *)&aValue,2,4);
//	CopyIn(val,aOffset);
	LittleEndian::Put16(this->First()->Ptr()+aOffset+KParameterListFieldOffset,aValue);
}

/**
Sets the Parameter value.
@param aOffset offset.
@param aValue  The value of the parameter.
*/
void RIrlanControlMBufChain::SetParameterValue(TInt aOffset,const TDesC8& aValue)
{
	__ASSERT_DEBUG((TUint)aValue.Length()<=KParameterValueMaxSize,IrlanUtil::Panic(EIrlanParameterValueTooBig));
	TUint offset=aOffset;
	TPtr8 temp((this->First()->Ptr()+KParameterListFieldOffset),
		offset,KParameterFrameMaxSize-offset);
	temp.Append(aValue);
}

/**
Gets the Parameter value.
@param aOffset offset.
@param aValue  The value of the parameter.
*/
void RIrlanControlMBufChain::GetParameterValue(TInt aOffset,TInt aLength,TDes8& aValue)
{
	__ASSERT_DEBUG((TUint)aLength<=KParameterValueMaxSize,IrlanUtil::Panic(EIrlanParameterValueTooBig));
	TUint offset=aOffset;
	TPtr8 value((this->First()->Ptr()+offset),aLength,KParameterValueMaxSize);
	aValue=value;
}
