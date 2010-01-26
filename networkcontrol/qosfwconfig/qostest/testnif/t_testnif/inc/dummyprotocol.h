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
// Psydo protocol class. PPP's supposed to Bind it. As result 
// we can catch the end of PPP negotiation without using real protocols.
// 
//

#ifndef __DUMMYPROTOCOL_H__
#define __DUMMYPROTOCOL_H__

#include "es_prot.h"
#include "NIFIF.H"
#include <networking/umtsnifcontrolif.h>

const TUint KMaxPacketFilters	= 8;
const TInt KMAXCONTEXTBY_TSY =13 ;
const TInt KMAXTESTENTRY  =10; 

class TEventDummy 
{
public:
	MNifEvent  *iEvent;
};

// hold all the current parameter of a PDP 
class TPDPContextContent
{
public:
	TPDPContextContent();
	TContextParameters	iParam;
	TBool				iBlock;
	TBool			iPacketFilters[KMaxPacketFilters];  // array that set for each set TFT filter
};

// hold all the parameter of  a test case and will help us to evaluate if the  
// event wont come. 
class TTestEntry
{
public:
	
	TTestEntry(TContextParameters Param,TInt async,TInt ret);
		
	void SetTimeToLive(TInt time){iTimeToLive = time;}

	TContextParameters iParamRequest;		// param of the entry when control is call
	TInt		iEventReturnCode;	// event returened reason
	TInt		iAsyncOp;				// 0 when nom async op expected num i the Event waiting for 
	TInt        iAsyncEvent; 
	TInt		icontrolreturncode;
	TInt		iTimeToLive;			// max time we will wait for the event to come 
};



// A class to simulate the Qos and the tcp/ip stack 
// the Dummy will send control tothe TestNif , wait for event and check if recieved on time 
// and will issue  data send and check the data recieved ( cuase we loopback it at the TestNif.
// 
class CDummyProtocol:public CProtocolBase , public MNifEvent
{
public:
	virtual ~CDummyProtocol();
	static CDummyProtocol* NewL();

	virtual CServProviderBase* NewSAPL(TUint aProtocol);
	virtual CHostResolvProvdBase* NewHostResolverL();
	virtual CServiceResolvProvdBase* NewServiceResolverL();
	virtual CNetDBProvdBase* NewNetDatabaseL();
	CProtocolFamilyBase* ProtocolFamily();
	virtual void Close();
	virtual void Open();
	virtual void CloseNow();
	virtual void StartSending(CProtocolBase* aProtocol);
	virtual void InitL(TDesC &aTag);
	virtual void StartL();

	virtual void BindL(CProtocolBase* protocol, TUint id);
	virtual void BindToL(CProtocolBase* protocol);
	virtual TInt Send(RMBufChain& aPDU,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt Send(TDes8& aPDU,TSockAddr* to,TSockAddr* from=NULL,CProtocolBase* aSourceProtocol=NULL);
	virtual void Process(RMBufChain &,CProtocolBase* aSourceProtocol=NULL);
	virtual void Process(TDes8& aPDU,TSockAddr *from,TSockAddr *to=NULL,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt GetOption(TUint level,TUint name,TDes8& option,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt SetOption(TUint level,TUint name,const TDesC8& option,CProtocolBase* aSourceProtocol=NULL);
	virtual void Error(TInt anError,CProtocolBase* aSourceProtocol=NULL);
	void	SetiNifL(CNifIfBase* ncpAddr);  // register the TestNif Address and send primary control call
	TInt	Event(CProtocolBase* aProtocol, TUint aName, TDes8& aOption, TAny* aSource=0);
	TInt	CheckTestEntry(TInt timepass);
	TInt	FindEmptySlot(void);
	TInt 	FindPDPSlot(TInt contextId);
	TInt	FindTestEntry(TInt);
	TInt	FindPacketFilterId(TInt contextId);
	void	newtestentryL(const TContextParameters& Param,TInt async,TInt ret);
	
	#ifdef SYMBIAN_NETWORKING_UMTSR5 
	void	SetQosReqParameter(TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt
								   ,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt);
	#else
	void	SetQosReqParameter(TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt
								   ,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt,TInt);
	#endif 
	// SYMBIAN_NETWORKING_UMTSR5 

	void	newTFTFilters(void);
	void 	addTFTFilterParameter(TInt,TInt,TUint8 aAddr[6],TUint8 aAddrMask[6],TInt,TInt,TInt,TInt,TInt,TInt,TUint8,TInt);
	void    updateTFTFilter(TInt,TInt,TInt ,TUint8 aAddr[6],TUint8 aAddrMask[6],TInt,TInt,TInt,TInt,TInt,TInt,TUint8,TInt);
	void	deleteTFT(TInt,TInt id[8]);
	void	cleanDummy();

	//The next function are the one that will issue a control command to the TestNif
	TInt NewPdpContextL(TInt ExpRetCode);
	TInt DeleteL(TInt ExpRetCode,TInt8 aContextToDelete);
	TInt ActivateL(TInt ExpRetCode,TInt8);
	TInt SetQoSL(TInt ExpRetCode,TInt8 ContextIdToSet);
	TInt ModifyTftL(TInt ExpRetCode,TInt8 ContextIdToModify,TInt aTFTOperationCode);
	TInt ModifyActiveL(TInt ExpRetCode,TInt8 ContexIdToMActive);

// Pure virtual
	virtual void Identify(TServerProtocolDesc* aProtocolDesc)const;
private:
	CDummyProtocol();
	CNifIfBase*		iNif; // TestNif Address
	TBool			iSendavailable;	
	TInt			iNumofopencontext;
	
	TTFTInfo		iTft;
	TTFTInfo		iTftEmpty;
	CArrayFixFlat<TPDPContextContent> *icontext;		// we open one more then the max context avaliable 0 
													// 0 is the primary and the rest secondary 
	CArrayFixSeg<TTestEntry>	*iTestEntries;		// will hold all the evn that waiting to async or event came from TSY
	 

};

#endif //__DUMMYPROTOCOL_H__

