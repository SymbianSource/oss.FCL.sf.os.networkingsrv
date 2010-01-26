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
// DHCPControl.h
// The DHCP Database access header file
// 
//

/**
 @file DHCPControl.h
*/

#ifndef DHCPDB_H
#define DHCPDB_H

#include <e32base.h>
#include <es_enum.h>
#include <comms-infras/metadata.h>

#include <metadatabase.h>
#include <commsdattypesv1_1.h>

//uncomment this line to store DHCP persistent data in a file named according to the client ID
//#define DHCP_PERSISTENTDATA_INFILE

class CDHCPStateMachine;
class TInetAddr;
#ifdef DHCP_PERSISTENTDATA_INFILE
class RFile;
class RFs;
#endif
class CDHCPDb : public CBase
/**
  * Base DHCP database access control class
  * 
  *
  * @internalTechnology
  *
  */
	{
public:
	CDHCPDb( TUint32 aIapId ) :
		iIapId( aIapId ),
		iIpDNSAddressFromServer(0,0),
		iNameServer1(0,0),
		iNameServer2(0,0)
		{
		}
	CDHCPDb( const TPtrC& aIpDNSAddressFromServer, const TPtrC& aNameServer1, const TPtrC& aNameServer2 ) :
		iIpDNSAddressFromServer( aIpDNSAddressFromServer ),
		iNameServer1( aNameServer1 ),
		iNameServer2( aNameServer2 )
		{
		}
	TBool ReadL( CDHCPStateMachine& aDhcpStateMachine, Meta::SMetaData& aPersistent );
   void WriteL( CDHCPStateMachine& aDhcpStateMachine, const Meta::SMetaData& aPersistent );

	// returns an array of address family enums e.g. KAfInet6, KAfInet
	void GetAddressFamiliesL(RArray<int> & results);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER   	
	TBool CheckIfDHCPServerImplEnabledL();
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
#ifdef SYMBIAN_DNS_PROXY   	
	void ReadHostNameL(CDHCPStateMachine& aDhcpStateMachine);
#endif //SYMBIAN_DNS_PROXY

public:
	TUint32 iIapId;
	TTime iLeaseExpiresAt;  //for IP4 it's the interface's IP address expiration time
                           //for IP6 it's the interface's IA expiration time
#ifdef SYMBIAN_NETWORKING_DHCPSERVER   
	// set to indicate that the DHCP server implementation is needed
	TBool iDHCPServerImpl;
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
                       

protected:

	void OpenIAPViewLC(CommsDat::CMDBSession*& aSession, CommsDat::CCDIAPRecord*& aIapRecord);

	static void VerifyFieldReadSuccessL(TInetAddr& addr, const TDesC& input, TInt field);

#ifdef DHCP_PERSISTENTDATA_INFILE
	void OpenFileLC( const TDesC8& aClientId, TAutoClose<RFs>& aFs, TAutoClose<RFile>& aFile, TBool aCreateNew );
	void WritePersistentL( const TDesC8& aClientId, const Meta::SMetaData& aPersistent );
	void ReadPersistentL( const TDesC8& aClientId, Meta::SMetaData& aPersistent );
#endif

private:
	void InitialServiceLinkL( CommsDat::CMDBSession* aDbSession, CommsDat::CCDIAPRecord* aIAPSetting );

protected:
   //names of the fields in CommDb
   TPtrC iIpDNSAddressFromServer;
   TPtrC iNameServer1;
   TPtrC iNameServer2;
	};

#endif

