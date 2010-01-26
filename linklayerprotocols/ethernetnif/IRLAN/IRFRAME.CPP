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
// Started by Mal, September 1997
// Irlan control frame construction and parsing 
// 
//

/**
 @file 
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <ir_sock.h>
#include "PKTDRV.H"
#include "ETHINTER.H"
#include "IRLAN.H"
#include "IRLANUTL.H"
#include "INTSOCK.H"
#include "IRLANBUF.H"
#include "IRLANDAT.H"

#ifdef __TRACEWIN__
  #include <log.h>
#else
  #define LOG(a)
#endif

#define IRLAN_GETPROVDINFO_NAME _L8("GetProviderInformation")
#define IRLAN_MEDIA_PARAMETERNAME _L8("MEDIA")
#define IRLAN_VERSION_PARAMETERNAME _L8("IRLAN_VER")
#define IRLAN_FILTERTYPE_PARAMETERNAME _L8("FILTER_TYPE")
#define IRLAN_FILTERMODE_PARAMETERNAME _L8("FILTER_MODE")
#define IRLAN_FILTEROPERATION_PARAMETERNAME _L8("FILTER_OPERATION")
#define IRLAN_FILTERENTRY_PARAMETERNAME _L8("FILTER_ENTRY")
#define IRLAN_MAXFRAME_PARAMETERNAME _L8("MAX_FRAME")
#define IRLAN_ACCESSTYPE_PARAMETERNAME _L8("ACCESS_TYPE")
#define IRLAN_DATACHANNEL_PARAMETERNAME _L8("DATA_CHAN")
#define IRLAN_CONARB_PARAMETERNAME _L8("CON_ARB")
#define IRLAN_RECONNKEY_PARAMETERNAME _L8("RECONNECT_KEY")

/** 
Send a GetProviderInformation to remote machine.
IRLAN COMMAND REQUEST FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value
@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::GetInfoCmd()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND GetInfoCmd\r\n")));
#endif
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KCommandFrameMinSize));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(0);		// GetInfoCmd has code of 0.
	TUint8 paramCount=0;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// MEDIA and IRLAN_VER
	SendIrlanControlFrame(cmd);
	return KErrNone;
}

//class TIrlanReplyFields
//	{
//public:
//	TBuf8<256> iParamName;
//	TUint8 iParamNameLen;
//	TBuf8<256> iParamValue;
//	TInt16 iParamValueLen;
//	};

/**
Send a GetProviderInfoReply to remote machine.
IRLAN PARAMETER RESPONSE FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value
@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::GetInfoReply()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND GetInfoReply\r\n")));
#endif
	TPtrC8 name;
	TPtrC8 value;
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KCommandFrameMinSize+1+5+2+5+1+9+2+2));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(0);		// GetInfoReply has code of 0.
	TUint8 paramCount=2;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// MEDIA and IRLAN_VER
	TInt offset=0;

	name.Set(IRLAN_MEDIA_PARAMETERNAME);
	TUint8 namelen=TUint8(name.Length());
	value.Set(_L8("802.3"));
	TUint8 valuelen=TUint8(value.Length());
	cmd.SetParameterNameLength(offset,namelen);
	cmd.SetParameterName(offset+1,name);
	offset+=namelen+1;
	cmd.SetParameterValueLength(offset,valuelen);
	cmd.SetParameterValue(offset+2,value);
	offset+=valuelen+2;

	name.Set(_L8("IRLAN_VERSION_PARAMETERNAME"));
	namelen=TUint8(name.Length());
	cmd.SetParameterNameLength(offset,namelen);
	cmd.SetParameterName(offset+1,name);
	offset+=namelen+1;
	cmd.SetParameterValueLength(offset,2);
	TBuf8<4> version;
	version.SetLength(2);
	version[0]=0x01;
	version[1]=0x01;
	cmd.SetParameterValue(offset+2,version);

	//CIrlanParameter *param=NULL;
	//TRAP(err,param=CIrlanParameter::NewL(name,0));
	//iIrlanParameterList.AddLast(*param);
	//offset+=nameLen+1;
	//TInt16 valueLen=LittleEndian::Get16(&iRecvBufPtr[offset]);
	//TPtr8 value(&iRecvBufPtr[offset+2],valueLen,valueLen);
	//TRAP(err,param->AddParameterValueL(value,EIrlanParameterString));
	//offset+=valueLen+2;
	
	SendIrlanResponseFrame(cmd);
	return KErrNone;
}

/**
Parse response from a GetProviderInformation to remote machine.
IRLAN PARAMETER RESPONSE FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value
*/
void CIrlanControlEngine::ParseInfoReply()
{
	TInt err;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	PARSE GetInfoReply\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iRecvBufPtr[0],iRecvBufPtr.Length()));
#endif
	TIrlanQueryResult result=TIrlanQueryResult(iRecvBufPtr[0]);
	// Need to check this command code.
	PrintResultCode(result);
	TInt paramCount=iRecvBufPtr[1];
	__ASSERT_DEBUG(paramCount==2,IrlanUtil::Fault(EIrlanInvalidParameterCount));
	TInt offset=2;
	for (TInt i=0;i<paramCount;i++)
		{// Go through parsing for each parameter in turn.
		TUint8 nameLen=iRecvBufPtr[offset];
		TPtr8 name(&iRecvBufPtr[offset+1],nameLen,nameLen);
//		__ASSERT_DEBUG(!name.MatchF(IRLAN_MEDIA_PARAMETERNAME),IrlanUtil::Fault(EIrlanInvalidParameter));
		CIrlanParameter *param=NULL;
		TRAP(err,param=CIrlanParameter::NewL(name,0));
		iIrlanParameterList.AddLast(*param);
		offset+=nameLen+1;
		TInt16 valueLen=LittleEndian::Get16(&iRecvBufPtr[offset]);
		TPtr8 value(&iRecvBufPtr[offset+2],valueLen,valueLen);
		TRAP(err,param->AddParameterValueL(value,EIrlanParameterString));
		offset+=valueLen+2;
		}
	iAwaitingResponse=EFalse;
}

/**
Send a GetMediaCharacteristics to remote machine.
IRLAN COMMAND REQUEST FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value
@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::GetMediaCmd()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND GetMediaCmd\r\n")));
#endif
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KMediaCommandFrameSize));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(1);		// GetMediaCharacteristics has code of 1.
	TUint8 paramCount=1;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// MEDIA only
	TInt offset=0;
	// we want the "MEDIA" parameter.
#ifdef __DEBUG__
	CIrlanParameter* param=LookUpParameter(IRLAN_MEDIA_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	__ASSERT_DEBUG(param->LookUpParameterValue(_L("802.3"))!=NULL,
	  IrlanUtil::Fault(EIrlanInvalidParameterValue));
#endif
	cmd.SetParameterNameLength(offset,5);
	cmd.SetParameterName(offset+1,IRLAN_MEDIA_PARAMETERNAME);
	offset+=5+1;
//	TUint16 valueLen=iProviderInfoParam[0].iValueLength;
	cmd.SetParameterValueLength(offset,5);
	cmd.SetParameterValue(offset+2,_L8("802.3"));
	SendIrlanControlFrame(cmd);
	return KErrNone;
}

/**
Send a GetProviderMediaReply to remote machine.
IRLAN PARAMETER RESPONSE FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value
@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::GetMediaReply()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND GetMediaReply\r\n")));
#endif
	TPtrC8 name;
	TPtrC8 value;
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KCommandFrameMinSize+1+5+2+5+1+9+2+2));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(0);		// GetInfoReply has code of 0.
	TUint8 paramCount=2;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// MEDIA and IRLAN_VER
	TInt offset=0;

	name.Set(IRLAN_MEDIA_PARAMETERNAME);
	TUint8 namelen=TUint8(name.Length());
	value.Set(_L8("802.3"));
	TUint8 valuelen=TUint8(value.Length());
	cmd.SetParameterNameLength(offset,namelen);
	cmd.SetParameterName(offset+1,name);
	offset+=namelen+1;
	cmd.SetParameterValueLength(offset,valuelen);
	cmd.SetParameterValue(offset+2,value);
	offset+=valuelen+2;

	name.Set(IRLAN_VERSION_PARAMETERNAME);
	namelen=TUint8(name.Length());
	cmd.SetParameterNameLength(offset,namelen);
	cmd.SetParameterName(offset+1,name);
	offset+=namelen+1;
	cmd.SetParameterValueLength(offset,2);
	TBuf8<4> version;
	version.SetLength(2);
	version[0]=0x01;
	version[1]=0x01;
	cmd.SetParameterValue(offset+2,version);

	//CIrlanParameter *param=NULL;
	//TRAP(err,param=CIrlanParameter::NewL(name,0));
	//iIrlanParameterList.AddLast(*param);
	//offset+=nameLen+1;
	//TInt16 valueLen=LittleEndian::Get16(&iRecvBufPtr[offset]);
	//TPtr8 value(&iRecvBufPtr[offset+2],valueLen,valueLen);
	//TRAP(err,param->AddParameterValueL(value,EIrlanParameterString));
	//offset+=valueLen+2;
	
	SendIrlanResponseFrame(cmd);
	return KErrNone;
}

/**
Parse a GetMediaCharacteristicsReply to Remote Machine.
IRLAN INFO CMD FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList
*/
TInt CIrlanControlEngine::ParseMediaReply()
{
	TInt err;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	PARSE GetMediaCharacteristicsReply\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iRecvBufPtr[0],iRecvBufPtr.Length()));
#endif
	TIrlanQueryResult result=TIrlanQueryResult(iRecvBufPtr[0]);
	// Need to check this command code.
	PrintResultCode(result);
	TInt paramCount=iRecvBufPtr[1];
	if (paramCount<2 || paramCount>9)
		return EIrlanInvalidParameterCount;
	TInt offset=2;
	CIrlanParameter *param=NULL;
	for (TInt i=0;i<paramCount;i++)
		{// Go through parsing for each parameter in turn.
		TUint8 nameLen=iRecvBufPtr[offset];
		TPtr8 name(&iRecvBufPtr[offset+1],nameLen,nameLen);
//		__ASSERT_DEBUG(!name.MatchF(IRLAN_MEDIA_PARAMETERNAME),IrlanUtil::Fault(EIrlanInvalidParameter));
		TRAP(err,param=CIrlanParameter::NewL(name,1));
		if (param)
			iIrlanParameterList.AddLast(*param);  // New Parameter.
		else
			param=LookUpParameter(name);
		offset+=nameLen+1;
		TInt16 valueLen=LittleEndian::Get16(&iRecvBufPtr[offset]);
		TPtr8 value(&iRecvBufPtr[offset+2],valueLen,valueLen);
		TRAP(err,param->AddParameterValueL(value,EIrlanParameterString));
		offset+=valueLen+2;
		}
	param=LookUpParameter(IRLAN_ACCESSTYPE_PARAMETERNAME);
	if (param == NULL) 
		return EIrlanInvalidParameter;
	CIrlanParameterValue *value=param->LookUpParameterValue();
	if (value == NULL) 
		return EIrlanInvalidParameterValue;
	if (value->Match(_L8("DIRECT")))	// We have a simple Access Point IrLAN node
		iAccessType=EAccessPoint;
	else if (value->Match(_L8("PEER")))	// We are connecting to a Peer IrLAN device.
		iAccessType=EPeerToPeer;
	else if (value->Match(_L8("HOSTED")))	// We are connecting via IrLAN to a hosted computer.
		iAccessType=EHosted;
	else
		return EIrlanInvalidIrlanMode;
	iAwaitingResponse=EFalse;
	return KErrNone;
}

/**
Send a OpenDataCmd to remote machine.
IRLAN COMMAND REQUEST FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value

@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::OpenDataCmd()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND OpenDataCmd\r\n")));
#endif
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KOpenDataCommandFrameSize));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(2);		// OpenDataChannel has code of 1.
	TUint8 paramCount=2;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// MEDIA and ACCESS_TYPE
	TInt offset=0;
	// we want the "MEDIA" parameter.
	CIrlanParameter* param=LookUpParameter(IRLAN_MEDIA_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	CIrlanParameterValue* value=param->LookUpParameterValue();
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,5);
	cmd.SetParameterName(offset+1,IRLAN_MEDIA_PARAMETERNAME);
	offset+=5+1;
	TPtr8 ptr(NULL,0);
	value->GetValue(ptr);
	__ASSERT_DEBUG(TUint16(ptr.Length())==value->GetValueLength(),IrlanUtil::Fault(EIrlanInvalidParameter));
	cmd.SetParameterValueLength(offset,value->GetValueLength());
	cmd.SetParameterValue(offset+2,ptr);
	offset+=ptr.Length()+2;
	// now we want "ACCESS_TYPE" parameter.
	param=LookUpParameter(IRLAN_ACCESSTYPE_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	value=param->LookUpParameterValue();
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,11);
	cmd.SetParameterName(offset+1,IRLAN_ACCESSTYPE_PARAMETERNAME);
	offset+=11+1;
	value->GetValue(ptr);
	cmd.SetParameterValueLength(offset,value->GetValueLength());
	cmd.SetParameterValue(offset+2,ptr);

	SendIrlanControlFrame(cmd);
	return KErrNone;
}

/**
Parse a OpenDataReply to Remote machine.
IRLAN INFO CMD FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList
*/
void CIrlanControlEngine::ParseOpenDataReply()
{
	TInt err;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	PARSE OpenDataReply\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iRecvBufPtr[0],iRecvBufPtr.Length()));
#endif
	TIrlanQueryResult result=TIrlanQueryResult(iRecvBufPtr[0]);
	// Need to check this command code.
	PrintResultCode(result);
	TInt paramCount=iRecvBufPtr[1];
	__ASSERT_DEBUG(paramCount>=2 && paramCount<=3,IrlanUtil::Fault(EIrlanInvalidParameterCount));
	TInt offset=2;
	for (TInt i=0;i<paramCount;i++)
		{// Go through parsing for each parameter in turn.
		TUint8 nameLen=iRecvBufPtr[offset];
		TPtr8 name(&iRecvBufPtr[offset+1],nameLen,nameLen);
		CIrlanParameter *param=NULL;
		TRAP(err,param=CIrlanParameter::NewL(name,2));
		iIrlanParameterList.AddLast(*param);  // New Parameter.
		offset+=nameLen+1;
		TInt16 valueLen=LittleEndian::Get16(&iRecvBufPtr[offset]);
		TPtr8 value(&iRecvBufPtr[offset+2],valueLen,valueLen);
		if (param->Match(IRLAN_DATACHANNEL_PARAMETERNAME))
			{
			TRAP(err,param->AddParameterValueL(value,EIrlanParameterByte));
			iIrlanDataPortNum=iRecvBufPtr[offset+2];
			}
		else if (param->Match(IRLAN_CONARB_PARAMETERNAME))
			{
			TRAP(err,param->AddParameterValueL(value,EIrlanParameterInteger));
			}
		else
			{
			__ASSERT_DEBUG(param->Match(IRLAN_RECONNKEY_PARAMETERNAME),
			  IrlanUtil::Fault(EIrlanInvalidParameter));
			TRAP(err,param->AddParameterValueL(value,EIrlanParameterString));
			}
		offset+=valueLen+2;
		}
	iAwaitingResponse=EFalse;
}

/**
Send a GetFilterConfig to remote machine.
IRLAN COMMAND REQUEST FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value

@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::GetDirectedFilterConfigCmd()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND GetDirectedFilterConfigCmd\r\n")));
#endif
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KFilterCommandMinFrameSize+26));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(5);		// GetFilterConfig has code of 5
	TUint8 paramCount=3;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// DATA_CHAN,FILTER_TYPE and FILTER_OPERATION
	TInt offset=0;
	// we want the "DATA_CHAN" parameter.
	CIrlanParameter* param=LookUpParameter(IRLAN_DATACHANNEL_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	CIrlanParameterValue* value=param->LookUpParameterValue();
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,9);
	cmd.SetParameterName(offset+1,IRLAN_DATACHANNEL_PARAMETERNAME);
	offset+=9+1;
	TPtr8 ptr(NULL,0);
	value->GetValue(ptr);
	__ASSERT_DEBUG(TUint16(ptr.Length())==value->GetValueLength(),IrlanUtil::Fault(EIrlanInvalidParameter));
	cmd.SetParameterValueLength(offset,value->GetValueLength());
	cmd.SetParameterValue(offset+2,ptr);
	offset+=ptr.Length()+2;
	// now add the "FILTER_TYPE" parameter.
	param=LookUpParameter(IRLAN_FILTERTYPE_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	value=param->LookUpParameterValue(_L8("DIRECTED"));
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,11);
	cmd.SetParameterName(offset+1,IRLAN_FILTERTYPE_PARAMETERNAME);
	offset+=11+1;
	cmd.SetParameterValueLength(offset,8);
	cmd.SetParameterValue(offset+2,_L8("DIRECTED"));
	offset+=8+2;
	// finally add the "FILTER_OPERATION" parameter
	cmd.SetParameterNameLength(offset,16);
	cmd.SetParameterName(offset+1,IRLAN_FILTEROPERATION_PARAMETERNAME);
	offset+=16+1;
	cmd.SetParameterValueLength(offset,7);
	cmd.SetParameterValue(offset+2,_L8("DYNAMIC"));
//	offset+=ptr.Length()+2;
	SendIrlanControlFrame(cmd);
	return KErrNone;
}

/**
Parse a ParseDirectedFilterConfigReply to remote machine.
IRLAN INFO CMD FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList
*/
void CIrlanControlEngine::ParseDirectedFilterConfigReply()
{
	TInt err;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	PARSE ParseDirectedFilterConfigReply\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iRecvBufPtr[0],iRecvBufPtr.Length()));
#endif
	TIrlanQueryResult result=TIrlanQueryResult(iRecvBufPtr[0]);
	// Need to check this command code.
	PrintResultCode(result);
	TInt paramCount=iRecvBufPtr[1];
	__ASSERT_DEBUG(paramCount>=0 && paramCount<=9,IrlanUtil::Fault(EIrlanInvalidParameterCount));
	TInt offset=2;
	for (TInt i=0;i<paramCount;i++)
		{// Go through parsing for each parameter in turn.
		TUint8 nameLen=iRecvBufPtr[offset];
		TPtr8 name(&iRecvBufPtr[offset+1],nameLen,nameLen);
		CIrlanParameter *param=NULL;
		TRAP(err,param=CIrlanParameter::NewL(name,2));
		iIrlanParameterList.AddLast(*param);  // New Parameter.
		offset+=nameLen+1;
		TInt16 valueLen=LittleEndian::Get16(&iRecvBufPtr[offset]);
		TPtr8 value(&iRecvBufPtr[offset+2],valueLen,valueLen);
		if (param->Match(IRLAN_FILTERENTRY_PARAMETERNAME))
			{
			TRAP(err,param->AddParameterValueL(value,EIrlanParameterOctetSeq));
			TPtr8 dest(&iHWAddr[0],6,6);//ethint removes the first three bytes of 
										//the mac address so add 3 bytes of padding	
			dest.Copy(value);
            iNotify->FoundMACAddr();
			}
		offset+=valueLen+2;
		}
	iAwaitingResponse=EFalse;
}

/**
Send a GetFilterConfig to remote machine.
IRLAN COMMAND REQUEST FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value
@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::SetDirectedFilterOperationCmd()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND SetDirectedFilterOperationCmd\r\n")));
#endif
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KFilterCommandMinFrameSize+20));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(5);		// GetFilterConfig has code of 5
	TUint8 paramCount=3;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// DATA_CHAN,FILTER_TYPE and FILTER_MODE
	TInt offset=0;
	// we want the "DATA_CHAN" parameter.
	CIrlanParameter* param=LookUpParameter(IRLAN_DATACHANNEL_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	CIrlanParameterValue* value=param->LookUpParameterValue();
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,9);
	cmd.SetParameterName(offset+1,IRLAN_DATACHANNEL_PARAMETERNAME);
	offset+=9+1;
	TPtr8 ptr(NULL,0);
	value->GetValue(ptr);
	__ASSERT_DEBUG(TUint16(ptr.Length())==value->GetValueLength(),IrlanUtil::Fault(EIrlanInvalidParameter));
	cmd.SetParameterValueLength(offset,value->GetValueLength());
	cmd.SetParameterValue(offset+2,ptr);
	offset+=ptr.Length()+2;
	// now add the "FILTER_TYPE" parameter.
	param=LookUpParameter(IRLAN_FILTERTYPE_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	value=param->LookUpParameterValue(_L8("DIRECTED"));
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,11);
	cmd.SetParameterName(offset+1,IRLAN_FILTERTYPE_PARAMETERNAME);
	offset+=11+1;
	cmd.SetParameterValueLength(offset,8);
	cmd.SetParameterValue(offset+2,_L8("DIRECTED"));
	offset+=8+2;
	// finally add the "FILTER_MODE" parameter
	cmd.SetParameterNameLength(offset,11);
	cmd.SetParameterName(offset+1,IRLAN_FILTERMODE_PARAMETERNAME);
	offset+=11+1;
	cmd.SetParameterValueLength(offset,6);
	cmd.SetParameterValue(offset+2,_L8("FILTER"));
//	offset+=ptr.Length()+2;
	SendIrlanControlFrame(cmd);
	return KErrNone;
}

/**
Parse a ParseDirectedFilterOperationReply to remote machine.
IRLAN INFO CMD FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList
*/
void CIrlanControlEngine::ParseDirectedFilterOperationReply()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	PARSE ParseDirectedFilterOperationReply\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iRecvBufPtr[0],iRecvBufPtr.Length()));
#endif
	TIrlanQueryResult result=TIrlanQueryResult(iRecvBufPtr[0]);
	// Need to check this command code.
	PrintResultCode(result);
	__ASSERT_DEBUG(!iRecvBufPtr[1],IrlanUtil::Fault(EIrlanInvalidParameterCount));
	iAwaitingResponse=EFalse;

}

/**
Send a GetFilterConfig to remote machine.
IRLAN COMMAND REQUEST FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList

  ParamEntry :  1 	+	up to 255  +   2	 +  up to 1016
		    NameLen	      Name		 ValLen		  Value
@return KErrNone if Success otherwise any failure code.
*/
TInt CIrlanControlEngine::SetBroadcastFilterOperationCmd()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	SEND SetBroadcastFilterOperationCmd\r\n")));
#endif
	RIrlanControlMBufChain cmd;
	TRAPD(err,cmd.AllocL(KFilterCommandMinFrameSize+21));
	if (err!=KErrNone)
		return err;
	cmd.SetCommandCode(5);		// GetFilterConfig has code of 5
	TUint8 paramCount=3;		// Cannot have >256
	cmd.SetParameterCount(paramCount);	// DATA_CHAN,FILTER_TYPE and FILTER_MODE
	TInt offset=0;
	// we want the "DATA_CHAN" parameter.
	CIrlanParameter* param=LookUpParameter(IRLAN_DATACHANNEL_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	CIrlanParameterValue* value=param->LookUpParameterValue();
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,9);
	cmd.SetParameterName(offset+1,IRLAN_DATACHANNEL_PARAMETERNAME);
	offset+=9+1;
	TPtr8 ptr(NULL,0);
	value->GetValue(ptr);
	__ASSERT_DEBUG(TUint16(ptr.Length())==value->GetValueLength(),IrlanUtil::Fault(EIrlanInvalidParameter));
	cmd.SetParameterValueLength(offset,value->GetValueLength());
	cmd.SetParameterValue(offset+2,ptr);
	offset+=ptr.Length()+2;
	// now add the "FILTER_TYPE" parameter.
	param=LookUpParameter(IRLAN_FILTERTYPE_PARAMETERNAME);
	__ASSERT_DEBUG(param,IrlanUtil::Fault(EIrlanInvalidParameter));
	value=param->LookUpParameterValue(_L8("DIRECTED"));
	__ASSERT_DEBUG(value,IrlanUtil::Fault(EIrlanInvalidParameterValue));
	cmd.SetParameterNameLength(offset,11);
	cmd.SetParameterName(offset+1,IRLAN_FILTERTYPE_PARAMETERNAME);
	offset+=11+1;
	cmd.SetParameterValueLength(offset,9);
	cmd.SetParameterValue(offset+2,_L8("BROADCAST"));
	offset+=9+2;
	// finally add the "FILTER_MODE" parameter
	cmd.SetParameterNameLength(offset,11);
	cmd.SetParameterName(offset+1,IRLAN_FILTERMODE_PARAMETERNAME);
	offset+=11+1;
	cmd.SetParameterValueLength(offset,6);
	cmd.SetParameterValue(offset+2,_L8("FILTER"));
//	offset+=ptr.Length()+2;
	SendIrlanControlFrame(cmd);
	return KErrNone;
}

/**
Parse a ParseBroadcastFilterOperationReply to remote machine.
IRLAN INFO CMD FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList
*/
void CIrlanControlEngine::ParseBroadcastFilterOperationReply()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	PARSE ParseBroadcastFilterOperationReply\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iRecvBufPtr[0],iRecvBufPtr.Length()));
#endif
	TIrlanQueryResult result=TIrlanQueryResult(iRecvBufPtr[0]);
	// Need to check this command code.
	PrintResultCode(result);
	__ASSERT_DEBUG(!iRecvBufPtr[1],IrlanUtil::Fault(EIrlanInvalidParameterCount));
	iAwaitingResponse=EFalse;
}

/**
Parse a GetInfoCmd.
@internalComponent
IRLAN INFO CMD FORMAT:     
				1		  1		 +   up to 1020 
			   Code	 ParamCount	      ParamList
*/
void CIrlanControlEngine::ParseControlCommand()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	PARSE GetInfoCmd\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iRecvBufPtr[0],iRecvBufPtr.Length()));
#endif
	TIrlanCmdNumber cmd=TIrlanCmdNumber(iRecvBufPtr[0]);
	// Need to check this command code.
	switch (cmd)
		{
	case EGetProviderInfo:
		GetInfoReply();
		break;
	case EGetMediaCharacteristics:
		// first should be parsing the frame to check "MEDIA" field is "802.3"
		GetMediaReply();
		break;
	case EOpenDataChannel:
		//OpenDataReply();
		iState=E_Open;
		break;
	default:
		break;
		}
}
