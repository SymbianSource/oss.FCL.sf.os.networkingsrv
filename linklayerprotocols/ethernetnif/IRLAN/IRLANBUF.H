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
// IrLAN mbuf function header.
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined(__IRLANBUF_H__)
#define __IRLANBUF_H__

/***********************************************************/
// FORMAT OF AN IRLAN CONTROL PACKET IS AS FOLLOWS:
// 1	   +		1		+      1022
// command + parametercount + parameterlist
// 
// FORMAT OF PARAMETER LIST IS AS FOLLOWS:
// 1			   +   <=255   +	   2	   +  (<=1016)
// paramnamelength + paramname + paramvaluelen + paramvalue
/***********************************************************/

const TInt EIrlanSenderPriority=10;			//< Constant Specifies Sender Priority Need to be less than IrDA ao's		  
const TInt EIrlanReceiverPriority=20;		//< Constant Specifies Receiver Priority.
const TInt EIrlanControlPriority=20;		//< Constant Specifies Control Priority.
const TUint KIrlanBufSize=1540;				//< Constant Specifies Buffer Size.
const TUint KParameterFrameMaxSize=1024;	//< Constant Specifies Parameter Max Frame Size.
const TUint KParameterNameMaxSize=255;		//< Constant Specifies Paramter Max Name Size.
const TUint KParameterValueMaxSize=1016;	//< Constant Specifies Paramter Max value Size.
const TUint KCommandCodeFieldOffset=0;		//< Constant Specifies command code field offset
const TUint KParameterCountFieldOffset=1;	//< Constant Specifies parameter count field offset
const TUint KParameterListFieldOffset=2;	//< Constant Specifies parameter list field offset.
const TUint KCommandFrameMinSize=2;			//< ie no params,Constant Specifies command min frame size.
const TUint KMediaCommandFrameSize=15;   //< Constant Specifies Media command frame size, ie 2+ 1+5+2+5
/**
Constant Specifies open data command frame size.
ie 2+ 1+5+2+5 + 1+11+2+6
*/
const TUint KOpenDataCommandFrameSize=35; 
/**
Constant Specifies filter command min frame size.
ie 2+ 1+9+2+1 + 1+11+2+8 + need to add!
*/
const TUint KFilterCommandMinFrameSize=37;  
//const TUint KGetInfoRespLength=28;	 // ie 2+ 1+5+2+4 +1+9+2+2
const TUint8 KIrlanHomeControlPort=7;		//< Constant specifies Home control port.
const TUint8 KIrlanHomeDataPort=8;			//< Constant specifies Home data port.
const TUint KFlagReadQueued         = 0x01; //< Constant specifies Read queued.
const TUint KFlagWriteQueued	    = 0x02; //< Constant specifies write queued.

/**
IrLAN specific mbuf functionality
@internalComponent
*/
class RIrlanControlMBufChain : public RMBufChain
{
public:
	void SetCommandCode(TUint8 aCode);
	TUint8 GetCommandCode();
	void SetResultCode(TUint8 aCode);
	TUint8 GetResultCode();
	void SetParameterCount(TUint8 aCode);
	TUint8 GetParameterCount();

	void SetParameterNameLength(TInt aOffset,TUint8 aLength);
	TUint8 GetParameterNameLength(TInt aOffset);
	void SetParameterName(TInt aOffset,const TDesC8& aName);
	void GetParameterName(TInt aOffset,TInt aLength,TDes8& aName);
	TUint16 GetParameterValueLength(TInt aOffset);
	void SetParameterValueLength(TInt aOffset,TUint16 aCount);
	void SetParameterValue(TInt aOffset,const TDesC8& aValue);
	void GetParameterValue(TInt aOffset,TInt aLength,TDes8& aValue);
private:
};

#endif
