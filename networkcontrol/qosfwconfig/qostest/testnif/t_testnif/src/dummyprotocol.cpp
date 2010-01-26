// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "dummyprotocol.h"
#include <in_iface.h>
#include "Tlog.h" 
#include "common.h"
#include <etelqos.h>




TTestEntry::TTestEntry(TContextParameters Param,TInt async,TInt ret)
{	
	iParamRequest = Param; 
	iEventReturnCode = 0x7e; 
	iAsyncOp = async;
	iAsyncEvent=0; 
	icontrolreturncode = ret; 
	iTimeToLive = 3000;
}

TPDPContextContent::TPDPContextContent()
{ 
	iBlock = EFalse;
	iParam.iContextInfo.iContextId=0;	
	for (TUint i=0; i < KMaxPacketFilters; i++)
		iPacketFilters[i] = ETrue;		
}


CDummyProtocol::CDummyProtocol()
{
} 


CDummyProtocol::~CDummyProtocol()
{
	delete icontext;		
	delete iTestEntries;	
}




CDummyProtocol* 
CDummyProtocol::NewL()
{
	return new(ELeave) CDummyProtocol();
}

CServProviderBase* 
CDummyProtocol::NewSAPL(TUint /*aProtocol*/)
{
	return NULL;
}
 CHostResolvProvdBase* 
CDummyProtocol::NewHostResolverL()
{
	 return NULL;
}

CServiceResolvProvdBase* 
CDummyProtocol::NewServiceResolverL()
{
	return NULL;
}

CNetDBProvdBase* 
CDummyProtocol::NewNetDatabaseL()
{
	return NULL;
}

CProtocolFamilyBase* 
CDummyProtocol::ProtocolFamily()
{
	return NULL;
}

void 
CDummyProtocol::Close()
{
}

void 
CDummyProtocol::Open()
{
}

void 
CDummyProtocol::CloseNow()
{
}

void 
CDummyProtocol::StartSending(CProtocolBase* /*aProtocol*/)
{
	iSendavailable = ETrue;
	TestLog.Printf(_L(" Startsendwas called can  send data \n"));
}

void 
CDummyProtocol::InitL(TDesC &/*aTag*/){}

void CDummyProtocol::StartL(){}

void 
CDummyProtocol::BindL(CProtocolBase* /* protocol*/, TUint /*id*/){}

void 
CDummyProtocol::BindToL(CProtocolBase* /*protocol*/){}

TInt 
CDummyProtocol::Send(RMBufChain& /*aPDU*/,CProtocolBase* /*aSourceProtocol=NULL*/){return KErrNone;}

TInt 
CDummyProtocol::Send(TDes8& /*aPDU*/,TSockAddr* /*to*/,TSockAddr* /*from=NULL*/,CProtocolBase* /*aSourceProtocol=NULL*/){return KErrNone;}

void 
CDummyProtocol::Process(RMBufChain &,CProtocolBase* /*aSourceProtocol=NULL*/){}

void 
CDummyProtocol::Process(TDes8& /*aPDU*/,TSockAddr * /*from*/,TSockAddr * /*to=NULL*/,CProtocolBase* /*aSourceProtocol=NULL*/){}

TInt 
CDummyProtocol::GetOption(TUint /*level*/,TUint /*name*/,TDes8& /*option*/,CProtocolBase* /*aSourceProtocol=NULL*/)
{
	return KErrNone;
}

TInt 
CDummyProtocol::SetOption(TUint /*level*/,TUint /*name*/,const TDesC8& /*option*/,CProtocolBase* /*aSourceProtocol=NULL*/)
{
	return KErrNone;
}

void 
CDummyProtocol::Error(TInt /*anError*/,CProtocolBase* /*aSourceProtocol=NULL*/){}

void 
CDummyProtocol::Identify(TServerProtocolDesc* /*aProtocolDesc*/)const{}


void	CDummyProtocol::cleanDummy()
{
	for (TInt j=0;j<10;j++)
		for (TUint i=1; i <= KMaxPacketFilters; i++)
			(*icontext)[j].iPacketFilters[i] = ETrue;
}



// the next two function handle the TPDPContextContent class array
TInt 
CDummyProtocol::FindEmptySlot(void)
{
TInt offset;	
	for(offset=1;offset<KMAXCONTEXTBY_TSY;offset++)
		if ((*icontext)[offset].iParam.iContextInfo.iContextId == 0)
			return offset;
	return offset;
}

TInt 
CDummyProtocol::FindPDPSlot(TInt contextId)
{
TInt offset;	
	for(offset=1;offset<KMAXCONTEXTBY_TSY;offset++)
		if ((*icontext)[offset].iParam.iContextInfo.iContextId == contextId)
			return offset;
	return offset;
}


TInt 
CDummyProtocol::FindTestEntry(TInt ContextId)
{
TInt offset;	
	for(offset=0;offset<iTestEntries->Count()/*KMAXTESTENTRY*/;offset++)
		if ((*iTestEntries)[offset].iParamRequest.iContextInfo.iContextId == ContextId)
			return offset;
	offset = KMAXTESTENTRY;
	return offset;
}

void CDummyProtocol::newtestentryL(const TContextParameters& param,TInt async,TInt ret)
{
	TTestEntry* TestE = new (ELeave) TTestEntry(param,async,ret);
	iTestEntries->AppendL(*TestE);
}



// look for free TFT filter Id 
TInt
CDummyProtocol::FindPacketFilterId(TInt contextId)
{
	for (TUint i=1; i <= KMaxPacketFilters; i++)
		if ((*icontext)[contextId].iPacketFilters[i])
		{
			(*icontext)[contextId].iPacketFilters[i] = EFalse;
			return i ;
		}
	return KErrNotFound;
} 

// the init proccess of the Test Nif we need to register the Dummy and set it to on 
// and we  then init the dummy variable that hold data about Nif PDP 
void 
CDummyProtocol::SetiNifL(CNifIfBase* ncp)
{
	
	// register  the ncp address
	iNif = ncp;
	// send the registerEvent and activate it .
	TPckgBuf<TEventDummy> opt;
	opt().iEvent = (MNifEvent*)this ;
	iNif->Control(KSOLInterface, KRegisterEventHandler, opt);	//check the return code 
	TestLog.Printf(_L("Registring Event Handler in the TestNif\n"));
	iNif->Control(KSOLInterface, KContextSetEvents, opt);		//check the return code but we must pass 
	TestLog.Printf(_L("Set the event to On \n"));				//this other wise we cannot test anything

	iTestEntries = new(ELeave) CArrayFixSeg<TTestEntry>(KMAXTESTENTRY);	// creat the TestEntry DB to handle all the test command 
																// we will issue.
	TPDPContextContent empty;
	icontext = new(ELeave) CArrayFixFlat<TPDPContextContent>(KMAXCONTEXTBY_TSY);  //  we wont have more context then the TSY max.
	for (TInt i=0;i<KMAXCONTEXTBY_TSY;i++)	
	{
		//(*icontext)[i].iParam.iContextInfo.iContextId = i+1;
		icontext->AppendL(empty);			//   INIT WITH EMPTY PARAMETER 
	}
}



TInt 
CDummyProtocol::Event(CProtocolBase* aProtocol, TUint aName, TDes8& aOption, TAny* /*aSource*/)
{
TInt offset;
CProtocolBase*	tmpProtocol=aProtocol;
	
			
	const TContextParameters& opt = *(TContextParameters*)aOption.Ptr();
	switch (aName)
	{
		// First id the group of event that TSY trigger them so we dont have testentryt for them and we 
		// just update the DB 
		case KContextDeleteEvent:
			{  // this event we can get only if TSY trigger it.
				FindTestEntry(opt.iContextInfo.iContextId);
				offset= FindPDPSlot(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{// yes it exist
						//iTestEntries->Delete(offsetTest);
						//iNumofopencontext--;
						(*icontext)[offset].iParam.iContextInfo.iContextId = 0;
						TestLog.Printf(_L("   TestNif Event-delete PDP:: PDP type %d Context Id : %d \n")
								,(opt.iContextType),(opt.iContextInfo.iContextId));
					
					}
				else// no we got deleter to context that not exist 
					{
						TestLog.Printf(_L("   TestNif Event Delete -for context that not exist!!!! \n"));
					}
			}
			return KErrNone;

		case KContextParametersChangeEvent:
			{// this one will come only from TSY sowe just updating DB
				offset= FindPDPSlot(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{// yes it exist
						iNumofopencontext--;
						(*icontext)[offset].iParam = opt ;
						TestLog.Printf(_L("   TestNif Event- change Parameter  PDP type %d Context Id : %d \n")
									,(opt.iContextType),(opt.iContextInfo.iContextId));
					}
				else// no we got change param  to context that not exist 
					{
						TestLog.Printf(_L("   TestNif Event-Parameter Change Event for context that not exist!!!! \n"));
					}
			}
			return KErrNone;


		case KContextUnblockedEvent:
			{  // this event we can get only if TSY trigger it.
				
				// first find if exist
				// print report , put testentyry(no much use), and delete from icontext 
				offset= FindPDPSlot(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{// yes it exist
						TestLog.Printf(_L("   TestNif Event-Unblock :: PDP type %d Context Id : %d and it was block <> unblock : %d  \n")
								,(opt.iContextType),(opt.iContextInfo.iContextId),(*icontext)[offset].iBlock);
						(*icontext)[offset].iBlock = EFalse;
					}
				else// no we got deleter to context that not exist 
					{
						TestLog.Printf(_L("   TestNif Event-Unblock Event for context that not exist!!!! \n"));
					}
			}
			return KErrNone;

		case KContextBlockedEvent:
			{  // this event we can get only if TSY trigger it.
				// first find if exist
				// print report , put testentyry(no much use), and delete from icontext 
				offset= FindPDPSlot(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{// yes it exist
						TestLog.Printf(_L("   TestNif Event-Block ::PDP type %d Context Id : %d and it was block<>unblock: %d  \n")
								,(opt.iContextType),(opt.iContextInfo.iContextId),(*icontext)[offset].iBlock);
						(*icontext)[offset].iBlock = ETrue;
					}
				else// no we got deleter to context that not exist 
					{
						TestLog.Printf(_L("   TestNif Event-Block Event for context that not exist!!!! \n"));
					}
				
			}
			return KErrNone;

		// From here is the event that are respons to call control 
		// if it arrive after we already erase the testentry from the array then 
		// we need to ignor them ( mark that there was a problem )

		case KContextQoSSetEvent:
			{
				// find the match entry in the testentry and put the correct data.
				offset = FindTestEntry(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{
						iTestEntries->Delete(offset);
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event-Qos Set Event with NO CONTROL call  \n"));	
						return KErrNone;
					}
				// if ok then put into the icontext if it err then the chekfunction will take care for that.
				if (opt.iReasonCode == KErrNone)
					{
						offset = FindPDPSlot(opt.iContextInfo.iContextId);
						(*icontext)[offset].iParam = opt;
						TestLog.Printf(_L("   TestNif Event-QosSet OK ::PDP for Id  : %d status : %d  \n")
								,opt.iContextInfo.iContextId,opt.iContextInfo.iStatus );
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event-Qos Set Event for ID %d recieved ERROR  %d  \n"),opt.iContextInfo.iContextId,opt.iReasonCode );
					}
			}
			return KErrNone;

		case KContextTFTModifiedEvent:
			{
				// find the match entry in the testentry and put the correct data.
				offset = FindTestEntry(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{
						iTestEntries->Delete(offset);
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event-TFT Modify Event with NO CONTROL call  \n"));	
						return KErrNone;
					}
				// if ok then put into the icontext   if it err then the chekfunction will take care for that.
				if (opt.iReasonCode == KErrNone)
					{
						offset = FindPDPSlot(opt.iContextInfo.iContextId);
						(*icontext)[offset].iParam = opt;
						TestLog.Printf(_L("   TestNif Event-TFT Modify %d  for Id  : %d status : %d  \n")
									,opt.iTFTOperationCode,opt.iContextInfo.iContextId,opt.iContextInfo.iStatus );
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event-TFT Modify Id recieved ERROR  %d  \n"),opt.iContextInfo.iContextId,opt.iReasonCode );
					}
			}
			return KErrNone;

		case KContextActivateEvent:
			{
				// find the match entry in the testentry and put the correct data.
				offset = FindTestEntry(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{
						iTestEntries->Delete(offset);
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event-Activate Event with NO CONTROL call  \n"));	
						return KErrNone;
					}
				// if ok then update icontext 
				if (opt.iReasonCode == KErrNone)
					{
						offset = FindPDPSlot(opt.iContextInfo.iContextId);
						(*icontext)[offset].iParam = opt;
						TestLog.Printf(_L("   TestNif Event-Activate  for Id  : %d status : %d   resasonCode %d \n")
						,opt.iContextInfo.iContextId,opt.iContextInfo.iStatus,opt.iReasonCode );
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event- Activate Id : %d  ERROR  %d  \n"),opt.iContextInfo.iContextId,opt.iReasonCode );
					}
			}
			return KErrNone;

		case KContextModifyActiveEvent:
			{
				offset = FindTestEntry(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{
						iTestEntries->Delete(offset);
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event-ModifyActive Event with NO CONTROL call \n"));	
						return KErrNone;
					}
				// if ok then put into the icontext   if it err then the chekfunction will take care for that.
				if (opt.iReasonCode == KErrNone)
					{
						offset = FindPDPSlot(opt.iContextInfo.iContextId);
						(*icontext)[offset].iParam = opt;
						TestLog.Printf(_L("   TestNif Event-ModifyActive:: PDP Id  : %d status : %d  \n")
									,opt.iContextInfo.iContextId,opt.iContextInfo.iStatus );
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event- Modifyactive ERROR  %d  \n"),opt.iReasonCode );
					}
			}
			return KErrNone;

		case KNetworkStatusEvent:
			{
				TestLog.Printf(_L("   TestNif Event-Network was drop and test nif notify the Qos about that !!!! \n") );
			}
			return KErrNone;



		case KSecondaryContextCreated:
			{
				// find the match entry in the testentry and put the correct data.
				offset = FindTestEntry(opt.iContextInfo.iContextId);
				if (offset<KMAXCONTEXTBY_TSY)
					{
						iTestEntries->Delete(offset);
					}
				else 
					{
						TestLog.Printf(_L("   TestNif Event-Secondary creat with NO CONTROL call  \n"));	
						return KErrNone;
					}
				// if ok then update icontext
				if (opt.iReasonCode == KErrNone)
					{
						offset = FindEmptySlot();
						(*icontext)[offset].iParam = opt;
						TestLog.Printf(_L("   TestNif Event-Secondary Creat for Id  : %d status : %d   resasonCode %d \n")
						,opt.iContextInfo.iContextId,opt.iContextInfo.iStatus,opt.iReasonCode );
						iNumofopencontext++;
					}
				else // we got error code 
					{
						TestLog.Printf(_L("   TestNif Event-Secondary create Event Recieved with ERROR %d\n"),opt.iReasonCode);
					}
				
			}
			return KErrNone;



		// the only event that in init process so we dont have testentry for him
		case KPrimaryContextCreated:
			{
				TestLog.Printf(_L("   TestNif Event-Primary PDP created \n"));
				// check if no prob  must successed 
				iNumofopencontext++;
				(*icontext)[1].iParam = opt;			// set offset 1 to be the primary 
			}
			if (tmpProtocol ) 	TestLog.Printf(_L("Perotocol Check \n"));
			return KErrNone;

		default:
			TestLog.Printf(_L("   TestNif Event- got event which we DONOT support!!!! \n"));
			break;
	}

	return KErrNotSupported;
}



// next functions will issue a control command to the 
// TestNif and put entry in the TestEntry DB 
//

// Issue the creation of secondary PDP context
TInt CDummyProtocol::NewPdpContextL(TInt ExpRetCode)
{
	TPckgBuf<TContextParameters> aOpt;
	aOpt().iContextType = ESecondaryContext;
	
	TInt ret = iNif->Control(KSOLInterface, KContextCreate, aOpt);
	
	TestLog.Printf(_L("Request To Create Secondary PDP:: Id Alocated : %d  resasonCode %d  ret code %d\n")
						,aOpt().iContextInfo.iContextId,aOpt().iReasonCode,ret );

	
	// put it in the TestEntry array.
	// check if expretcode is what we got 
	
	if (aOpt().iReasonCode == KErrNone )
	{
		newtestentryL(aOpt(),KSecondaryContextCreated,aOpt().iReasonCode);
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && !ExpRetCode)
	{
		TestLog.Printf(_L("Request To Create Secondary PDP::We expected to pass but it fail \n"));
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && ExpRetCode)
	{
		TestLog.Printf(_L("Request To Create Secondary PDP::We expected to fail and so it was \n"));
		return KErrNone;
	}
	else  return 1;//KErrNone; need to return errcode 
}


// delete Pdp context
TInt CDummyProtocol::DeleteL(TInt ExpRetCode,TInt8 aContextToDelete)
{
	TPckgBuf<TContextParameters> aOpt;
	aOpt().iContextInfo.iContextId = aContextToDelete;
	
	TInt ret = iNif->Control(KSOLInterface, KContextDelete, aOpt);

	TestLog.Printf(_L("Request to Delete PDP:: Id deleted : %d -- resasonCode %d  ret code %d\n")
						,(aOpt().iContextInfo.iContextId),(aOpt().iReasonCode),ret);


	if (aOpt().iReasonCode == KErrNone )
	{
		iNumofopencontext--;
		//newtestentryL(aOpt(),KContextDeleteEvent,aOpt().iReasonCode);
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && !ExpRetCode)
	{
		TestLog.Printf(_L("Request to Delete PDP::We expected to pass but it fail \n"));
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && ExpRetCode)
	{
		TestLog.Printf(_L("Request to Delete PDP::We expected to fail and so it was \n"));
		return KErrNone;
	}
	else  return 1;//KErrNone; need to return errcode 
}



// Activate context
TInt CDummyProtocol::ActivateL(TInt ExpRetCode,TInt8 ContextIdToActivate)
{

	TInt offset= FindPDPSlot(ContextIdToActivate);	// find Context in DB may not exist if we fake 
	if (offset==KMAXCONTEXTBY_TSY){TestLog.Printf(_L("We got Activate Request to non exist Context Id!!!! \n"));}				
		
	TPckgBuf<TContextParameters> aOpt;
	aOpt().iContextInfo.iContextId = ContextIdToActivate;
	TInt ret = iNif->Control(KSOLInterface, KContextActivate, aOpt);
		
	TestLog.Printf(_L("Request to Activate PDP ::PDP Id to activate : %d -- resasonCode %d ret code %d \n")
						,(aOpt().iContextInfo.iContextId),(aOpt().iReasonCode),ret);

	if (aOpt().iReasonCode == KErrNone )
	{
		newtestentryL(aOpt(),KContextActivateEvent,aOpt().iReasonCode);
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && !ExpRetCode)
	{
		TestLog.Printf(_L("Request to Activate PDP ::We expected to pass but it fail \n"));
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && ExpRetCode)
	{
		TestLog.Printf(_L("Request to Activate PDP ::We expected to fail and so it was \n"));
		return KErrNone;
	}
	else  return 1;//KErrNone; need to return errcode 

}




// Modify active Pdp context
TInt CDummyProtocol::ModifyActiveL(TInt ExpRetCode,TInt8 ContexIdToMActive)
{

	TInt offset= FindPDPSlot(ContexIdToMActive);	// find Context in DB may not exist if we fake 
	if (offset==KMAXCONTEXTBY_TSY){TestLog.Printf(_L("We got Modify Activate Request to non exist Context Id!!!! \n"));}				
		
	TPckgBuf<TContextParameters> aOpt;
	aOpt().iContextInfo.iContextId = ContexIdToMActive;

	TInt ret = iNif->Control(KSOLInterface, KContextModifyActive, aOpt);

	TestLog.Printf(_L("Request to Modify Activate PDP:: Id to activate : %d -- resasonCode %d ret code %d \n")
						,(aOpt().iContextInfo.iContextId),(aOpt().iReasonCode),ret);

	if (aOpt().iReasonCode == KErrNone )
	{
		newtestentryL(aOpt(),0,0);
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && !ExpRetCode)
	{
		TestLog.Printf(_L("Request to Modify Activate PDP::We expected to pass but it fail \n"));
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && ExpRetCode)
	{
		TestLog.Printf(_L("Request to Modify Activate PDP::We expected to fail and so it was \n"));
		return KErrNone;
	}
	else  return 1;//KErrNone; need to return errcode 

}



// clean the TFTInfo for new filter to be send
void CDummyProtocol::newTFTFilters(void)
{
	iTft.Set(iTftEmpty);
}


void  
CDummyProtocol::addTFTFilterParameter
		(TInt acontextId, TInt aEvaluationPrecedenceInde, TUint8 aAddr[6],TUint8 aAddrMask[6],
			TInt aProtocol,	TInt aMinRemotePort, TInt aMaxRemotePort, TInt aMinDestPort,
			TInt aMaxDestPort, TInt aIPSec, TUint8 aiTOSorTrafficClass, TInt aFlowLevel)
{
RPacketContext::TPacketFilterV2 aFilter;
TInt i;

	iTft.SetToFirst();
	
	aFilter.iId =								FindPacketFilterId(acontextId);
	aFilter.iEvaluationPrecedenceIndex =		aEvaluationPrecedenceInde;
	// need to check if 16 is only for IP6
	for ( i = 0; i < 6; i++)
		aFilter.iSrcAddr[i] =					aAddr[i];
	for (i = 0; i < 6; i++)
		aFilter.iSrcAddrSubnetMask[i] =			aAddrMask[i];
	
	aFilter.iProtocolNumberOrNextHeader =		aProtocol;
	aFilter.iSrcPortMin =						aMinRemotePort;
	aFilter.iSrcPortMax =						aMaxRemotePort;
	aFilter.iDestPortMin =						aMinDestPort;
	aFilter.iDestPortMax =						aMaxDestPort;
	aFilter.iIPSecSPI =							aIPSec;
	aFilter.iTOSorTrafficClass =				aiTOSorTrafficClass;
	aFilter.iFlowLabel =						aFlowLevel;	


	iTft.AddPacketFilter(aFilter);
}


void
CDummyProtocol::updateTFTFilter
			(TInt contextId,TInt filterId, TInt aEvaluationPrecedenceInde,
			TUint8 aAddr[6],TUint8 aAddrMask[6], TInt aProtocol,
			TInt aMinRemotePort, TInt aMaxRemotePort, TInt aMinDestPort,
			TInt aMaxDestPort, TInt aIPSec, TUint8 aiTOSorTrafficClass, TInt aFlowLevel)
{
RPacketContext::TPacketFilterV2 aFilter;
TTFTInfo aTFT;
TInt i;

	aFilter.iId = filterId;
	(*icontext)[contextId].iParam.iContextConfig.GetTFTInfo(aTFT); 
	iTft.GetPacketFilter(aFilter);
	// chane the filter 
	iTft.SetToFirst();
	for ( i = 0; i < 4; i++)
		aFilter.iSrcAddr[i] =					aAddr[i];
	for (i = 0; i < 4; i++)
		aFilter.iSrcAddrSubnetMask[i] =			aAddrMask[i];
	if (aEvaluationPrecedenceInde!=(-1)) aFilter.iEvaluationPrecedenceIndex =		aEvaluationPrecedenceInde;
	if (aProtocol!=(-1))				aFilter.iProtocolNumberOrNextHeader =		aProtocol;
	if (aMinRemotePort!=(-1))			aFilter.iSrcPortMin =						aMinRemotePort;
	if (aMaxRemotePort!=(-1))			aFilter.iSrcPortMax =						aMaxRemotePort;
	if (aMinDestPort!=(-1))				aFilter.iDestPortMin =						aMinDestPort;
	if (aMaxDestPort!=(-1))				aFilter.iDestPortMax =						aMaxDestPort;
	if (aIPSec!=(-1))					aFilter.iIPSecSPI =							aIPSec;
	if (aiTOSorTrafficClass!=(0xff))	aFilter.iTOSorTrafficClass =				aiTOSorTrafficClass;
	if (aFlowLevel!=(-1))				aFilter.iFlowLabel =						aFlowLevel;	
	
	
	iTft.AddPacketFilter(aFilter);
}


void	CDummyProtocol::deleteTFT(TInt,TInt id[8])
{
RPacketContext::TPacketFilterV2 aFilter;
	iTft.SetToFirst();
	for (TInt i=0;i<8;i++)
	{
		if (id[i] >0)
		{
			aFilter.iId = id[i];
			iTft.AddPacketFilter(aFilter);
		}
	}

}
	

TInt CDummyProtocol::ModifyTftL(TInt ExpRetCode,TInt8 ContextIdToModify ,TInt aTFTOperationCode)
{

	TInt offset= FindPDPSlot(ContextIdToModify);	// find Context in DB may not exist if we fake 
	if (offset==KMAXCONTEXTBY_TSY){TestLog.Printf(_L("We got TFT Modify Request to non exist Context Id!!!!! \n"));}				
	
	
	TContextParameters aParameters;
	aParameters.iContextConfig.SetTFTInfo(iTft);
	aParameters.iContextInfo.iContextId = ContextIdToModify;
	aParameters.iContextType = (offset) ? ESecondaryContext:EPrimaryContext;
	aParameters.iTFTOperationCode = aTFTOperationCode;
	TPckg<TContextParameters> aOpt(aParameters);

	TInt ret = iNif->Control(KSOLInterface, KContextTFTModify, aOpt);
	
	TestLog.Printf(_L("Request to TFT Modify :: Id to set : %d -- resasonCode %d  ret code %d \n")
						,(aOpt().iContextInfo.iContextId),(aOpt().iReasonCode),ret);

	if (aOpt().iReasonCode == KErrNone )
	{
		newtestentryL(aOpt(),KContextQoSSetEvent,aOpt().iReasonCode);		
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && !ExpRetCode)
	{
		TestLog.Printf(_L("Request to TFT Modify ::We expected to pass but it fail \n"));
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && ExpRetCode)
	{
		TestLog.Printf(_L("Request to TFT Modify ::We expected to fail and so it was \n"));
		return KErrNone;
	}
	return KErrGeneral;//KErrNone; need to return errcode 
}


#ifdef SYMBIAN_NETWORKING_UMTSR5 
void 
CDummyProtocol::SetQosReqParameter
	(	TInt ContextIdToSet ,
		TInt aQosReqTrafficClass ,
		TInt aQosReqTrafficClassMin ,
		TInt aQosReqDeliveryOrder ,
		TInt aQosReqDeliveryOrderMin ,
		TInt aQosReqDeliveryErr,
		TInt aQosReqDeliveryErrMin,
		TInt aQosReqMaxSDUSize,
		TInt aQosReqMaxSDUSizeMin,
		TInt aQosReqUpBitRate,
		TInt aQosReqDownBitRate ,
		TInt aQosReqUpBitRateMin,
		TInt aQosReqDownBitRateMin ,
		TInt aQosReqBitErrorRatio,
		TInt aQosReqBitErrorRatioMin,
		TInt aQosReqSDUErrorRatio,
		TInt aQosReqSDUErrorRatioMin,
		TInt aQosReqTrafficHandlingPriority,
		TInt aQosReqTrafficHandlingPriorityMin,
		TInt aQosReqTransferDelay,
		TInt aQosReqTransferDelayMin,
		TInt aQosReqGuaranteedUpRate,
		TInt aQosReqGuaranteedDownRate,
		TInt aQosReqGuaranteedUpRateMin,
		TInt aQosReqGuaranteedDownRateMin,
		TInt aQosReqSourceStatisticsDescriptor,
		TInt aQosReqSignallingIndication
)
{
	TInt offset= FindPDPSlot(ContextIdToSet);	// find Context in DB may not exist if we fake 
	if (offset==KMAXCONTEXTBY_TSY){TestLog.Printf(_L("We got Request to new Qos parameter to non exist Context Id!!!! \n"));}				

		
			RPacketQoS::TQoSR5Requested reqR5qos;
			reqR5qos.iReqTrafficClass                 = (RPacketQoS::TTrafficClass)(aQosReqTrafficClass );
			reqR5qos.iMinTrafficClass					= (RPacketQoS::TTrafficClass)(aQosReqTrafficClassMin );	
			reqR5qos.iReqDeliveryOrderReqd            = (RPacketQoS::TDeliveryOrder)(aQosReqDeliveryOrder );
			reqR5qos.iMinDeliveryOrderReqd			= (RPacketQoS::TDeliveryOrder)(aQosReqDeliveryOrderMin );
			reqR5qos.iReqDeliverErroneousSDU          = (RPacketQoS::TErroneousSDUDelivery)(aQosReqDeliveryErr );
			reqR5qos.iMinDeliverErroneousSDU			= (RPacketQoS::TErroneousSDUDelivery)(aQosReqDeliveryErrMin );	
			reqR5qos.iReqMaxSDUSize                   = (aQosReqMaxSDUSize );
			reqR5qos.iMinAcceptableMaxSDUSize			= (aQosReqMaxSDUSizeMin );
			reqR5qos.iReqMaxRate.iUplinkRate          = (aQosReqUpBitRate );
			reqR5qos.iMinAcceptableMaxRate.iUplinkRate= (aQosReqUpBitRateMin );	
			reqR5qos.iReqMaxRate.iDownlinkRate        = (aQosReqDownBitRate );
			reqR5qos.iMinAcceptableMaxRate.iDownlinkRate = (aQosReqDownBitRateMin );
			reqR5qos.iReqBER                          = (RPacketQoS::TBitErrorRatio)(aQosReqBitErrorRatio );
			reqR5qos.iMaxBER							= (RPacketQoS::TBitErrorRatio)(aQosReqBitErrorRatioMin );
			reqR5qos.iReqSDUErrorRatio                = (RPacketQoS::TSDUErrorRatio)(aQosReqSDUErrorRatio );
			reqR5qos.iMaxSDUErrorRatio				= (RPacketQoS::TSDUErrorRatio)(aQosReqSDUErrorRatioMin );
			reqR5qos.iReqTrafficHandlingPriority      = (RPacketQoS::TTrafficHandlingPriority)(aQosReqTrafficHandlingPriority );
			reqR5qos.iMinTrafficHandlingPriority		= (RPacketQoS::TTrafficHandlingPriority)(aQosReqTrafficHandlingPriorityMin );	
			reqR5qos.iReqTransferDelay                = (aQosReqTransferDelay );
			reqR5qos.iMaxTransferDelay				= (aQosReqTransferDelayMin );
			reqR5qos.iReqGuaranteedRate.iUplinkRate   = (aQosReqGuaranteedUpRate );
			reqR5qos.iReqGuaranteedRate.iDownlinkRate = (aQosReqGuaranteedDownRate );
			reqR5qos.iMinGuaranteedRate.iUplinkRate	= (aQosReqGuaranteedUpRateMin );
			reqR5qos.iMinGuaranteedRate.iDownlinkRate	= (aQosReqGuaranteedDownRateMin );
		
		
			
			reqR5qos.iSourceStatisticsDescriptor    = (RPacketQoS::TSourceStatisticsDescriptor)(aQosReqSourceStatisticsDescriptor );
			reqR5qos.iSignallingIndication			= (aQosReqSignallingIndication );
		

		
//		TTrafficClass				iReqTrafficClass;			//< Requested traffic class	
//		TTrafficClass				iMinTrafficClass;			//< Minimum acceptable traffic class
//		TDeliveryOrder				iReqDeliveryOrderReqd;		//< Requested value for sequential SDU delivery
//		TDeliveryOrder				iReqDeliveryOrderReqd;		//< Minimum acceptable value for sequential SDU delivery
//		TErroneousSDUDelivery		iReqDeliverErroneousSDU;	//< Requested value for erroneous SDU delivery
//		TErroneousSDUDelivery		iMinDeliverErroneousSDU;	//< Minimum acceptable value for erroneous SDU delivery
//		TInt						iReqMaxSDUSize;				//< Request maximum SDU size
//		TInt						iMinAcceptableMaxSDUSize;	//< Minimum acceptable SDU size
//		TBitRate					iReqMaxRate;				//< Requested maximum bit rates on uplink and downlink
//		TBitRate					iMinAcceptableMaxRate;		//< Minimum acceptable bit rates on uplink and downlink
//		TBitErrorRatio				iReqBER;					//< Requested target BER
//		TBitErrorRatio				iMaxBER;					//< Maximum acceptable target BER
//		TSDUErrorRatio				iReqSDUErrorRatio;			//< Requested target SDU error ratio
//		TSDUErrorRatio				iMaxSDUErrorRatio;			//< Maximum acceptable target SDU error ratio
//		TTrafficHandlingPriority	iReqTrafficHandlingPriority;//< Requested traffic handling priority
//		TTrafficHandlingPriority	iMinTrafficHandlingPriority;//< Minimum acceptable traffic handling priority
//		TInt						iReqTransferDelay;			//< Requested transfer delay (in milliseconds)
//		TInt						iMaxTransferDelay;			//< Maximum acceptable  transfer delay (in milliseconds)
//		TBitRate					iReqGuaranteedRate;			//< Requested guaranteed bit rates on uplink and downlink
//		TBitRate					iMinGuaranteedRate;			//< Minimum acceptable guaranteed bit rates on uplink and downlink

		
		
			(*icontext)[ContextIdToSet].iParam.iContextConfig.SetUMTSQoSReq(reqR5qos);
		
}
#else
void 
CDummyProtocol::SetQosReqParameter
	(	TInt ContextIdToSet ,
		TInt aQosReqTrafficClass ,
		TInt aQosReqTrafficClassMin ,
		TInt aQosReqDeliveryOrder ,
		TInt aQosReqDeliveryOrderMin ,
		TInt aQosReqDeliveryErr,
		TInt aQosReqDeliveryErrMin,
		TInt aQosReqMaxSDUSize,
		TInt aQosReqMaxSDUSizeMin,
		TInt aQosReqUpBitRate,
		TInt aQosReqDownBitRate ,
		TInt aQosReqUpBitRateMin,
		TInt aQosReqDownBitRateMin ,
		TInt aQosReqBitErrorRatio,
		TInt aQosReqBitErrorRatioMin,
		TInt aQosReqSDUErrorRatio,
		TInt aQosReqSDUErrorRatioMin,
		TInt aQosReqTrafficHandlingPriority,
		TInt aQosReqTrafficHandlingPriorityMin,
		TInt aQosReqTransferDelay,
		TInt aQosReqTransferDelayMin,
		TInt aQosReqGuaranteedUpRate,
		TInt aQosReqGuaranteedDownRate,
		TInt aQosReqGuaranteedUpRateMin,
		TInt aQosReqGuaranteedDownRateMin
)
{
	TInt offset= FindPDPSlot(ContextIdToSet);	// find Context in DB may not exist if we fake 
	if (offset==KMAXCONTEXTBY_TSY){TestLog.Printf(_L("We got Request to new Qos parameter to non exist Context Id!!!! \n"));}				

		
			RPacketQoS::TQoSR99_R4Requested reqqos;
			reqqos.iReqTrafficClass                 = (RPacketQoS::TTrafficClass)(aQosReqTrafficClass );
			reqqos.iMinTrafficClass					= (RPacketQoS::TTrafficClass)(aQosReqTrafficClassMin );	
			reqqos.iReqDeliveryOrderReqd            = (RPacketQoS::TDeliveryOrder)(aQosReqDeliveryOrder );
			reqqos.iMinDeliveryOrderReqd			= (RPacketQoS::TDeliveryOrder)(aQosReqDeliveryOrderMin );
			reqqos.iReqDeliverErroneousSDU          = (RPacketQoS::TErroneousSDUDelivery)(aQosReqDeliveryErr );
			reqqos.iMinDeliverErroneousSDU			= (RPacketQoS::TErroneousSDUDelivery)(aQosReqDeliveryErrMin );	
			reqqos.iReqMaxSDUSize                   = (aQosReqMaxSDUSize );
			reqqos.iMinAcceptableMaxSDUSize			= (aQosReqMaxSDUSizeMin );
			reqqos.iReqMaxRate.iUplinkRate          = (aQosReqUpBitRate );
			reqqos.iMinAcceptableMaxRate.iUplinkRate= (aQosReqUpBitRateMin );	
			reqqos.iReqMaxRate.iDownlinkRate        = (aQosReqDownBitRate );
			reqqos.iMinAcceptableMaxRate.iDownlinkRate = (aQosReqDownBitRateMin );
			reqqos.iReqBER                          = (RPacketQoS::TBitErrorRatio)(aQosReqBitErrorRatio );
			reqqos.iMaxBER							= (RPacketQoS::TBitErrorRatio)(aQosReqBitErrorRatioMin );
			reqqos.iReqSDUErrorRatio                = (RPacketQoS::TSDUErrorRatio)(aQosReqSDUErrorRatio );
			reqqos.iMaxSDUErrorRatio				= (RPacketQoS::TSDUErrorRatio)(aQosReqSDUErrorRatioMin );
			reqqos.iReqTrafficHandlingPriority      = (RPacketQoS::TTrafficHandlingPriority)(aQosReqTrafficHandlingPriority );
			reqqos.iMinTrafficHandlingPriority		= (RPacketQoS::TTrafficHandlingPriority)(aQosReqTrafficHandlingPriorityMin );	
			reqqos.iReqTransferDelay                = (aQosReqTransferDelay );
			reqqos.iMaxTransferDelay				= (aQosReqTransferDelayMin );
			reqqos.iReqGuaranteedRate.iUplinkRate   = (aQosReqGuaranteedUpRate );
			reqqos.iReqGuaranteedRate.iDownlinkRate = (aQosReqGuaranteedDownRate );
			reqqos.iMinGuaranteedRate.iUplinkRate	= (aQosReqGuaranteedUpRateMin );
			reqqos.iMinGuaranteedRate.iDownlinkRate	= (aQosReqGuaranteedDownRateMin );
		
		
		
//		TTrafficClass				iReqTrafficClass;			//< Requested traffic class	
//		TTrafficClass				iMinTrafficClass;			//< Minimum acceptable traffic class
//		TDeliveryOrder				iReqDeliveryOrderReqd;		//< Requested value for sequential SDU delivery
//		TDeliveryOrder				iReqDeliveryOrderReqd;		//< Minimum acceptable value for sequential SDU delivery
//		TErroneousSDUDelivery		iReqDeliverErroneousSDU;	//< Requested value for erroneous SDU delivery
//		TErroneousSDUDelivery		iMinDeliverErroneousSDU;	//< Minimum acceptable value for erroneous SDU delivery
//		TInt						iReqMaxSDUSize;				//< Request maximum SDU size
//		TInt						iMinAcceptableMaxSDUSize;	//< Minimum acceptable SDU size
//		TBitRate					iReqMaxRate;				//< Requested maximum bit rates on uplink and downlink
//		TBitRate					iMinAcceptableMaxRate;		//< Minimum acceptable bit rates on uplink and downlink
//		TBitErrorRatio				iReqBER;					//< Requested target BER
//		TBitErrorRatio				iMaxBER;					//< Maximum acceptable target BER
//		TSDUErrorRatio				iReqSDUErrorRatio;			//< Requested target SDU error ratio
//		TSDUErrorRatio				iMaxSDUErrorRatio;			//< Maximum acceptable target SDU error ratio
//		TTrafficHandlingPriority	iReqTrafficHandlingPriority;//< Requested traffic handling priority
//		TTrafficHandlingPriority	iMinTrafficHandlingPriority;//< Minimum acceptable traffic handling priority
//		TInt						iReqTransferDelay;			//< Requested transfer delay (in milliseconds)
//		TInt						iMaxTransferDelay;			//< Maximum acceptable  transfer delay (in milliseconds)
//		TBitRate					iReqGuaranteedRate;			//< Requested guaranteed bit rates on uplink and downlink
//		TBitRate					iMinGuaranteedRate;			//< Minimum acceptable guaranteed bit rates on uplink and downlink

		
		
			(*icontext)[ContextIdToSet].iParam.iContextConfig.SetUMTSQoSReq(reqqos);
		
}

#endif 
// SYMBIAN_NETWORKING_UMTSR5 






//
// Set QoS parameters for a Pdp context.
// Note: Change is not signaled to network until Activate/ModifyActive is called.
//
TInt CDummyProtocol::SetQoSL(TInt ExpRetCode,TInt8 ContextIdToSet)
{
	TInt offset= FindPDPSlot(ContextIdToSet);	// find Context in DB may not exist if we fake 
	if (offset==KMAXCONTEXTBY_TSY){offset=10;TestLog.Printf(_L("We got SetQos Request to non exist Context Id!!!! \n"));}				

	TPckgBuf<TContextParameters> aOpt;
	aOpt().iContextInfo.iContextId = ContextIdToSet;
	aOpt().iContextConfig = (*icontext)[offset].iParam.iContextConfig;
	aOpt().iContextType = (offset-1) ? ESecondaryContext:EPrimaryContext;

	TInt ret = iNif->Control(KSOLInterface, KContextQoSSet, aOpt);

	TestLog.Printf(_L("Request to SetQos PDP:: Id to set Qos : %d -- resasonCode %d ret code %d \n")
						,(aOpt().iContextInfo.iContextId),(aOpt().iReasonCode),ret);


	
	if (aOpt().iReasonCode == KErrNone )
	{
		newtestentryL(aOpt(),KContextQoSSetEvent,aOpt().iReasonCode);
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && !ExpRetCode)
	{
		TestLog.Printf(_L("Request to SetQos PDP::We expected to pass but it fail \n"));
		return KErrNone;
	}
	else if ( (aOpt().iReasonCode!= KErrNone) && ExpRetCode)
	{
		TestLog.Printf(_L("Request to SetQos PDP::We expected to fail and so it was \n"));
		return KErrNone;
	}
	else  return 1;//KErrNone; need to return errcode 
}




// function that call each second to see if any async event happand 
TInt
CDummyProtocol::CheckTestEntry(TInt timepass)
{
// go over the testentry array and chck if time finish 
// event happand >> chek error code  
// if not just decrese the timeToLive 
// if timer finish  and no event occure then >>> problem 
//
	TInt offset=0;// we only have one entry in this program 
	if (iTestEntries->Count()==0) return KErrNone;
	//numofntry = iTestEntries->Count() ;
		if ((*iTestEntries)[offset].iTimeToLive > 0)
		{// no event yet time is still ok
			(*iTestEntries)[offset].iTimeToLive -=timepass;
			return KErrNone;
		}
		else 
		{// no event and no time left to wait.
			TestLog.Printf(_L("No event recieved on call control \n"));
			iTestEntries->Delete(offset);
			return KErrGeneral;//EntryFailToCokmplete;
		}
}
