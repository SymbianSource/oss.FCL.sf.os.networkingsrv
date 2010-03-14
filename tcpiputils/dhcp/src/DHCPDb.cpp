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
// Implements the DHCP database access
// 
//

/**
 @file
 @internalTechnology
*/

#include "DHCPDb.h"
#include "DHCPStateMachine.h"
#include <nifman.h>
#include <f32file.h>
#include <comms-infras/metabuffer.h>
#include <metadatabase.h>
#include <commsdattypesv1_1.h>
#include <es_prot.h>
#include "DhcpIP6Msg.h"
using namespace CommsDat;


void CDHCPDb::OpenIAPViewLC(CMDBSession*& aSession, CCDIAPRecord*& aIapRecord)
	{
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	aSession = CMDBSession::NewLC(KCDVersion1_2);
#else
	aSession = CMDBSession::NewLC(KCDVersion1_1);
#endif

	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	aSession->SetAttributeMask(ECDHidden | ECDPrivate);
	
	aIapRecord = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	CleanupStack::PushL(aIapRecord);

    aIapRecord->SetRecordId(iIapId);
        	
	aIapRecord->LoadL(*aSession);
	}
	



void CDHCPDb::GetAddressFamiliesL(RArray<int> & results)
	{
	CMDBSession* session;
	CCDIAPRecord* iap;
	// next call leaves session and iap object on cleanupstack
	OpenIAPViewLC(session, iap);

	CCDServiceRecordBase* service;
	InitialServiceLinkL(session, iap);
	service = static_cast<CCDServiceRecordBase *>(iap->iService.iLinkedRecord);
	TInt ignoreThis;
	CMDBField<TDesC>* cdbLoadStr = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameIfNetworks, ignoreThis));
	TPtrC readPtr(*cdbLoadStr);

	RSocketServ sockServ;
	User::LeaveIfError(sockServ.Connect());

	TLex lex(readPtr);
	while(!lex.Eos())
		{
		TPtrC token;
		lex.Mark();
		TChar ch;
		ch=lex.Peek();

		while(!lex.Eos() && (ch=lex.Peek())!=TChar(','))
			lex.Inc();
		
		token.Set(lex.MarkedToken());
		if(!token.Length())
			break;

		if(ch==TChar(','))
			lex.Inc();

		TServerProtocolDesc pinfo;
		User::LeaveIfError(sockServ.FindProtocol(token,pinfo));
		results.AppendL(pinfo.iAddrFamily);
		}
		
		
	CleanupStack::PopAndDestroy(iap);
	CleanupStack::PopAndDestroy(session);

	return;
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
/** Checks if DHCP server implementation should be enabled.
  * Currently this is decided depending on if the NapServiceEnabled field is set 	 
  * to true in PAN NAP Service extension table.
  * PAN NAP is the only client for DHCP server implementation in DHCP component.
  * 
  * @internalTechnology
  */
TBool CDHCPDb::CheckIfDHCPServerImplEnabledL()
	{
	CMDBSession* session;
	CCDIAPRecord* iap;
	// next call leaves session and iap object on cleanupstack
	OpenIAPViewLC(session, iap);

	CCDServiceRecordBase* service;
	InitialServiceLinkL(session, iap);
	service = static_cast<CCDServiceRecordBase *>(iap->iService.iLinkedRecord);
	TInt ignoreThis;
	CMDBField<TDesC>* cdbLanServiceExtn = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameServiceExtensionTableName, ignoreThis));
	CMDBField<TInt>* cdbLanServiceExtnRecId = static_cast<CMDBField<TInt>*>(service->GetFieldByNameL(KCDTypeNameServiceExtensionTableRecordId, ignoreThis));
	
	TInt recId = *cdbLanServiceExtnRecId;
	
	CCDPANServiceExtRecord* panServiceExtRecord = static_cast<CCDPANServiceExtRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdPANServiceExtRecord));
	CleanupStack::PushL(panServiceExtRecord);
	
	HBufC16* readBuffer = HBufC16::NewLC(KCommsDbSvrMaxFieldLength);
	TPtr16 readPtr = readBuffer->Des();
	
	readPtr.Copy(*cdbLanServiceExtn);
	iDHCPServerImpl = EFalse; // reset this, found it was set to some negative value by default though "this" is derived from CBase
	// DHCP server implementation at the moment is only used by PAN NAP imlpementation
	// Hence, check if the Lan service extension was PAN service extension 
	if(recId && ! readPtr.Compare(TPtrC(KCDTypeNamePANServiceExt))) 
		{
		panServiceExtRecord->SetRecordId(recId);
		panServiceExtRecord->LoadL(*session);
	
		// set the flag to indicate that this connection needs DHCP server implementation
		iDHCPServerImpl = panServiceExtRecord->iNapServiceEnabled;
		}

	CleanupStack::PopAndDestroy(readBuffer);
	CleanupStack::PopAndDestroy(panServiceExtRecord);	
	CleanupStack::PopAndDestroy(iap);
	CleanupStack::PopAndDestroy(session);
	
	return iDHCPServerImpl;
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
#ifdef SYMBIAN_DNS_PROXY	
void CDHCPDb::ReadHostNameL(CDHCPStateMachine& aDhcpStateMachine)
	{
	CMDBSession* session;
	CCDIAPRecord* iap;
	// next call leaves session and iap object on cleanupstack
	OpenIAPViewLC(session, iap);
	
	if(iap->iNetwork.iLinkedRecord == 0)
       {
       iap->iNetwork.iLinkedRecord= static_cast<CCDNetworkRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdNetworkRecord));       
       }
    iap->iNetwork.iLinkedRecord->SetRecordId(iap->iNetwork);      
 
    // load linked network record so we can get its id
    iap->iNetwork.iLinkedRecord->LoadL(*session); 
   
	CCDNetworkRecord* service;
    service = static_cast<CCDNetworkRecord *>(iap->iNetwork.iLinkedRecord);
    
	TInt ignoreThis;
	CMDBField<TDesC>* cdbHostName = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameHostName, ignoreThis));
	
	HBufC16* readHostName = HBufC16::NewLC(KMaxName);
	TPtr16 hostPtr = readHostName->Des();
	hostPtr.Copy(*cdbHostName);
	if(hostPtr.Length()>1)
		{ 
		TInt index = hostPtr.Locate('.');
		TPtrC16 host = hostPtr.Left(index);
		TPtrC16 domain = hostPtr.Right((hostPtr.Length()-1)-index);
	 
		aDhcpStateMachine.iProxyHostName.Copy(hostPtr);
		aDhcpStateMachine.iProxyDomainName.Copy(domain);
		}
	 
	CleanupStack::PopAndDestroy(readHostName);
	CleanupStack::PopAndDestroy(iap);
	CleanupStack::PopAndDestroy(session);
	}
#endif // SYMBIAN_DNS_PROXY	


TBool CDHCPDb::ReadL( CDHCPStateMachine& aDhcpStateMachine, Meta::SMetaData& aPersistent )
/**
  * Read the relevant CommDB entries.
  *
  * Primarily we are looking at the service tables
  * and reading ip address, subnet mask, gateway, 
  * dns address from server fields.  Reading these
  * fields give us connection specific configuration
  * for how to proceed with dhcp negotions...e.g if
  * ipAddress field has a value, then this is the static
  * ip address used in an inform message broadcast across
  * the network...or if dns address from server is true, then
  * dhcp will set the dns servers into the stack once address
  * acquisition is over.
  *
  * @internalTechnology
  */
	{
	TBool addrFromServer=EFalse;
	HBufC16* readBuffer;

	CMDBSession* session;
	CCDIAPRecord* iap;

	// next call leaves session and iap object on cleanupstack
	OpenIAPViewLC(session, iap);

	CCDServiceRecordBase* service;
	InitialServiceLinkL(session, iap);
	service = static_cast<CCDServiceRecordBase *>(iap->iService.iLinkedRecord);
	
	readBuffer = HBufC16::NewLC(KCommsDbSvrMaxFieldLength);
	TPtr16 readPtr = readBuffer->Des();

	TInt ignoreThis;

	CMDBField<TBool>* cdbLoadBool = static_cast<CMDBField<TBool>*>(service->GetFieldByNameL(KCDTypeNameIpAddrFromServer, ignoreThis));
	addrFromServer = *cdbLoadBool;
	
	CMDBField<TDesC>* cdbLoadStr = 0;


	if (addrFromServer)
		{//don't really want to leave
#ifdef DHCP_PERSISTENTDATA_INFILE
		TRAPD(err,ReadPersistentL( aDhcpStateMachine.iClientId, aPersistent ));
#else
		//assumes flat memory and byte addressing mode
		HBufC8* persist = HBufC8::NewLC(KCommsDbSvrMaxFieldLength * 2 * sizeof(TUint16));
		TPtr8 des = persist->Des();

		cdbLoadStr = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameIpAddrLeaseValidFrom, ignoreThis));
		des.Copy(*cdbLoadStr);
		cdbLoadStr = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameIpAddrLeaseValidTo, ignoreThis));
		des.Append(*cdbLoadStr);

		TPtrC8 ptr(des);

		if ( aPersistent.GetTypeId().Check( ptr ) == KErrNone )
			{//whatever we read is assumed ok, since TID matches and if not the confirm or reboot will fail
			aPersistent.Load( ptr );
			}
		CleanupStack::PopAndDestroy(persist);
#endif
		}
	else
		{
// DHCP shouldn't look at these values.. if they're static, it's down to the driver below to set them, we'll pick that up later.
		}



	cdbLoadBool = static_cast<CMDBField<TBool>*>(service->GetFieldByNameL(iIpDNSAddressFromServer, ignoreThis));
	aDhcpStateMachine.iNameServerAddressesFromServer = (*cdbLoadBool);
	if( ! aDhcpStateMachine.iNameServerAddressesFromServer )
		{
#ifdef SYMBIAN_DNS_PROXY		
		CMDBField<TDesC>* cdbLoadDns = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameIpNameServer1, ignoreThis));
		aDhcpStateMachine.iProxyDnsSrvAddr.Input(*cdbLoadDns);
#endif		
		}	

	// note the handling of cleanup here...as view and buffer swap positions on the cleanup
	// stack early in the function, forcing this pop and destroy, and the else...
	CleanupStack::PopAndDestroy(readBuffer);	
	CleanupStack::PopAndDestroy(iap);
	CleanupStack::PopAndDestroy(session);
	return addrFromServer;
	}

/* static */
void CDHCPDb::VerifyFieldReadSuccessL(TInetAddr& addr, const TDesC& input, TInt field)
	{
	TInt err;
	(void)field; // for UREL mode
	
	if(input.Length() == 0)
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb: Can't find field %d - using null value"),field));
		TInetAddr nullAddr;  //creates unspecif. (null) address
		addr = nullAddr;
		}
	else if( (err=addr.Input(input)) != KErrNone)
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb: Error %d: Can't read field %d as an IP address!"),err,field));
		User::Leave(err);
		}
	}

void CDHCPDb::WriteL( CDHCPStateMachine& aDhcpStateMachine, const Meta::SMetaData& aPersistent )
/**
  * Writes acquired settings into commDB.
  *
  * Stores the time the lease is valid from and to
  * and the configured IP address
  *
  * @internalTechnology
  */
	{
#ifdef DHCP_PERSISTENTDATA_INFILE
	WritePersistentL( aDhcpStateMachine.iClientId, aPersistent );
#else
	(void)aDhcpStateMachine;
	HBufC16* buffer;

	HBufC8* persist = HBufC8::NewLC(aPersistent.Length());
	TPtr8 des = persist->Des();
	aPersistent.Store( des );
#ifdef _DEBUG
	if ( (TUint)des.Length() > 2 * KCommsDbSvrMaxFieldLength * sizeof(TUint16) )
		{
	   __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb::WriteL - persistent data length 3 * KDefaultTextColumnWidth %d"), des.Length()));
		}
#endif

	CMDBSession* session;
	CCDIAPRecord* iap;
	// next call leaves session and iap object on cleanupstack
	OpenIAPViewLC(session, iap);

	CCDServiceRecordBase* service;
	InitialServiceLinkL(session, iap);
	service = static_cast<CCDServiceRecordBase *>(iap->iService.iLinkedRecord);

	buffer = HBufC16::NewLC(KCommsDbSvrMaxFieldLength);
	
	TPtr16 readWritePtr = buffer->Des();

		{//assumes flat memory and byte addressing mode
		TInt nStored = readWritePtr.MaxLength() * sizeof(TUint16);
		TPtr8 writePtr8( (TUint8*)readWritePtr.Ptr(), readWritePtr.MaxLength() * sizeof(TUint16));
		writePtr8.Copy( des.Mid( 0, Min(nStored, des.Length() )) );
		readWritePtr.SetLength( writePtr8.Length() / sizeof(TUint16) );
		// write the first part of the persistent data
		TInt ignoreThis;
		CMDBField<TDesC>* cdbLoadStr = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameIpAddrLeaseValidFrom, ignoreThis));
		cdbLoadStr->SetMaxLengthL(readWritePtr.Length());
		(*cdbLoadStr) = readWritePtr;
		if ( 	nStored < des.Length() )
			{
			writePtr8.Copy( des.Mid( nStored, Min(des.Length(), KCommsDbSvrMaxFieldLength * sizeof(TUint16)) ) );
			readWritePtr.SetLength( writePtr8.Length() / sizeof(TUint16) );
			// write the second part of the persistent data
			cdbLoadStr = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameIpAddrLeaseValidTo, ignoreThis));
			cdbLoadStr->SetMaxLengthL(readWritePtr.Length());
			(*cdbLoadStr) = readWritePtr;
			}
		service->ModifyL(*session);
		}

	CleanupStack::PopAndDestroy(buffer);
	CleanupStack::PopAndDestroy(iap);
	CleanupStack::PopAndDestroy(session);
	CleanupStack::PopAndDestroy(persist);
#endif // DHCP_PERSISTENTDATA_INFILE
	}

#ifdef DHCP_PERSISTENTDATA_INFILE
void CDHCPDb::OpenFileLC( const TDesC8& aClientId, TAutoClose<RFs>& aFs, TAutoClose<RFile>& aFile, TBool aCreateNew )
	{
	User::LeaveIfError( aFs.iObj.Connect() );
	aFs.PushL();
	TInt result;
	if ( aCreateNew )
		{
		result = aFs.iObj.CreatePrivatePath (EDriveC);
		if ((result != KErrNone) && (result != KErrAlreadyExists)) 
			{
			User::Leave( result );
			}
		}
	Elements::TRBuf privatePath;
	privatePath.CreateL( 100 );
	CleanupClosePushL(privatePath);
	aFs.iObj.PrivatePath(privatePath);
	TInt toAdd = aClientId.Length() * 2 + 2 + 4;
	TInt space = privatePath.MaxLength() - privatePath.Length();
	if ( space < toAdd )
		{
		privatePath.ReAllocL(privatePath.Length() + toAdd);
		}

	// Get the correct system drive letter in a buffer.	
	TChar sysDriveLetter = RFs::GetSystemDriveChar();
	TBuf<2> sysDriveLetterBuf;
	sysDriveLetterBuf.Fill( sysDriveLetter, 1 );
	sysDriveLetterBuf.Append( _L( ":" ) );

 	privatePath.Insert (0, sysDriveLetterBuf);
	for (TInt n = 0; n < aClientId.Length(); n++)
		{
		privatePath.AppendNumFixedWidthUC( aClientId[n],EHex,2 );
		}
	privatePath.Append( _L(".dat") );
	result = aFile.iObj.Open(aFs.iObj,privatePath,EFileStream|EFileWrite);
	if ( result != KErrNone && aCreateNew )
		{
		result = aFile.iObj.Create (aFs.iObj, privatePath, EFileWrite);
		}
	User::LeaveIfError( result );
	CleanupStack::PopAndDestroy();
	aFile.PushL();
	}

#ifdef _DEBUG
const TInt KMaxPersistenDataLength = 512; //an arbitrarry nember to have some means to check the max store length
#endif

void CDHCPDb::WritePersistentL( const TDesC8& aClientId, const Meta::SMetaData& aPersistent )
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb::WritePersistentL")));
	TAutoClose<RFs> fs;
	TAutoClose<RFile> file;
	OpenFileLC( aClientId, fs, file, ETrue );

	HBufC8* persist = HBufC8::NewLC(aPersistent.Length());
	TPtr8 des = persist->Des();
#ifdef _DEBUG
	if ( des.MaxLength() > KMaxPersistenDataLength )
		{
	   __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb::WriteL - persistent data suspiciously long %d"), des.MaxLength()));
		}
#endif
	aPersistent.Store( des );
	file.iObj.Write( des );
	CleanupStack::PopAndDestroy(persist);
	file.Pop();
	fs.Pop();
	}

void CDHCPDb::ReadPersistentL( const TDesC8& aClientId, Meta::SMetaData& aPersistent )
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb::ReadPersistentL")));
	TAutoClose<RFs> fs;
	TAutoClose<RFile> file;
	OpenFileLC( aClientId, fs, file, EFalse );
	TInt data_size;
	User::LeaveIfError(file.iObj.Size(data_size));
#ifdef _DEBUG
	if ( data_size > KMaxPersistenDataLength )
		{
	   __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb::WriteL - persistent data suspiciously long %d"), data_size));
		}
#endif

	HBufC8* persist = HBufC8::NewLC(data_size);
	TPtr8 des = persist->Des();
	file.iObj.Read(des);
	TPtrC8 ptr( des );
	if ( aPersistent.GetTypeId().Check( ptr ) == KErrNone )
		{//whatever we read is assumed ok, since TID matches and if not the confirm or reboot will fail
		aPersistent.Load( ptr );
		}
	CleanupStack::PopAndDestroy(persist);
	file.Pop();
	fs.Pop();
	}
#endif

void CDHCPDb::InitialServiceLinkL(CMDBSession* aDbSession, CCDIAPRecord* aIapRecord)
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb::InitialServiceLinkL()")));

	// This will be made more simple quite soon when commsdat supports linked records
	//
	if (aIapRecord->iService.iLinkedRecord == 0)
		{
		const TDesC& servType = aIapRecord->iServiceType;

			{
			// Convert the service type to ASCII.
			TBuf8<KMaxName> tempBuf;
			tempBuf.Copy( servType );

			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPDb::InitialServiceLinkL() service type = \"%S\""), &tempBuf));
			}

		if (servType.CompareF(TPtrC(KCDTypeNameLANService))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDLANServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANServiceRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameDialOutISP))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDDialOutISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialOutISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameDialInISP))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDDialInISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialInISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameVPNService))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDVPNServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVPNServiceRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameOutgoingWCDMA))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDOutgoingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdOutgoingGprsRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameIncomingWCDMA))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDIncomingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIncomingGprsRecord));
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CCDHCPDb::InitialServiceLinkL() Invalid Service Type!!!")));
			User::Leave(KErrBadName);	
			}
		
		aIapRecord->iService.iLinkedRecord->SetRecordId(aIapRecord->iService);
		}

	aIapRecord->iService.iLinkedRecord->LoadL(*aDbSession);
	}

