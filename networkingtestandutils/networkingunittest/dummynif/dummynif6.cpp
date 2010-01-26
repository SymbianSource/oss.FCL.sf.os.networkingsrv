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
//

#include <e32hal.h>	// UserHal::MachineInfo()
#include <comms-infras/nifif.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <in_iface.h>
#include <comms-infras/commsdebugutility.h>
#include <connectprog.h>

#include <in_chk.h>
#include <in_sock.h>
#include <in6_if.h>	// KSoIface*, KIf*, TSoInet6IfConfig
#include "dummynifvar.h"
#include "dummynif.h"


/*
 * The IPv6 interface binder class
 */

CDummyIf6::CDummyIf6(CDummyIfLink& aLink)
	: CNifIfBase(aLink)
	{
	CDummyIfLog::Printf(_L("CDummyIf6::CDummyIf6()"));

	iLink = &aLink;

	//
	// Use the 64 bit id of MARM machines as our interface id
	//
	TMachineInfoV1Buf machineInfo;

	UserHal::MachineInfo(machineInfo);
	iLocalIfId.SetAddr(machineInfo().iMachineUniqueId);
	iLocalIfId.SetUniversalBit(0);

	//
	// In WINS environment the id is zero which is no-no
	//
	if (iLocalIfId.IsZero())
		iLocalIfId.SetAddrRandomNZ();

	iIfName.Format(_L("dummynif6[0x%08x]"), this);
	}

void CDummyIf6::BindL(TAny *aId)
	{
	CDummyIfLog::Printf(_L("CDummyIf6::BindL(aId %x)"), aId);
	if(iProtocol)
		User::Leave(KErrAlreadyExists);
	iProtocol = (CProtocolBase*)aId;	
	}

TInt CDummyIf6::Send(RMBufChain& aPdu, TAny*)
	{
	Recv(aPdu);
	return 1;
	}

TInt CDummyIf6::State()
    {
    return EIfUp;
    }

void CDummyIf6::UpdateHeaders(TInet6HeaderIP* aIp6, TInet6HeaderUDP* /*aUdp*/)
{
	// swap over the destination and source addresses
	TIp6Addr temp;
	temp = aIp6->SrcAddr();
	aIp6->SetSrcAddr(aIp6->DstAddr());
	aIp6->SetDstAddr(temp);
}

void CDummyIf6::Recv(RMBufChain& aPdu)
	{

	TInt res;

	// this received data has already been looped back...
	// get the ip header from the RMBufChain
	TInet6HeaderIP* ip6 = (TInet6HeaderIP*) aPdu.First()->Next()->Ptr();
	TInet6HeaderUDP* udp = NULL;

	if ((TUint)ip6->NextHeader() == KProtocolInetUdp)
		{
		// get the udp header as well - assume only udp traffic here
		udp = (TInet6HeaderUDP*) ip6->EndPtr();

		CDummyIfLog::Printf(_L("CDummyIf6::Recv(...): UDP length %d, src port %d, dst port %d"),
			udp->Length(), udp->SrcPort(), udp->DstPort());

		// depending on the contents, pass it on up thru the stack 
		// or maybe do something else

		// use the destination port number to decide whether or not the payload is a command
		TUint dstPort = udp->DstPort();
		if (KDummyNifCmdPort == dstPort)
			{	
			// let's use the first payload byte as the command byte
			switch (*(udp->EndPtr()))
				{
			case KForceDisconnect:
				CDummyIfLog::Printf(_L("KForceDisconnect command"));
				// do some action
				iNotify->IfProgress(KLinkLayerClosed, KErrCommsLineFail);
				iNotify->LinkLayerDown(KErrCommsLineFail, MNifIfNotify::EDisconnect);
				// no return code so all we can do is respond with what we got
				UpdateHeaders(ip6, udp);
				iProtocol->Process(aPdu, (CProtocolBase*)this);
				break;

			case KForceReconnect:
				CDummyIfLog::Printf(_L("KForceReconnect command"));
				// do some action
				iNotify->IfProgress(KLinkLayerClosed, KErrCommsLineFail);
				iNotify->LinkLayerDown(KErrCommsLineFail, MNifIfNotify::EReconnect);
				// no return code so all we can do is respond with what we got
				UpdateHeaders(ip6, udp);
				iProtocol->Process(aPdu, (CProtocolBase*)this);
				break;

			case KSendNotification:
				CDummyIfLog::Printf(_L("KSendNotification command"));
				res = iNotify->Notification(ENifToAgentEventTypeQueryIsDialIn);
				//let's write the result in the next byte of the reply
				if (res == KErrNotSupported)
					udp->EndPtr()[1] = (unsigned char) KErrNone;
				else
					udp->EndPtr()[1] = (unsigned char) KErrGeneral; // this will lose it's sign :-(
				
				UpdateHeaders(ip6, udp);
				iProtocol->Process(aPdu, (CProtocolBase*)this);
				break;

			default:
				CDummyIfLog::Printf(_L("Unknown command - ignoring it"));
				break;
				// unknown command, just ignore this packet???
				}
			return;
			}

		}
	else
		{
		CDummyIfLog::Printf(_L("CDummyIf6::Recv(...): IPv6 length %d, next header %d"),
			ip6->PayloadLength(), ip6->NextHeader());
		}

	// just echo the packet back to the original sender

	// update the headers (addresses, checksums etc).  If "udp" is non-NULL, then
	// the UDP ports will be updated as well.
	UpdateHeaders(ip6, udp);
	// now process it (pass up the stack)
	iProtocol->Process(aPdu, (CProtocolBase*)this);		
}

void CDummyIf6::Info(TNifIfInfo& aInfo) const
	{
	aInfo.iVersion = TVersion(1,1,1);
	aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfCreatedByLink;
	aInfo.iName.Copy(iIfName);
	aInfo.iProtocolSupported = 0;
	}

TInt CDummyIf6::Control(TUint aLevel, TUint aName, TDes8& aOption, TAny* /* aSource */)
	{

	if (aLevel==KSOLInterface)
		{
		switch (aName)
			{
		case KSoIfInfo6:
			{
			CDummyIfLog::Printf(_L("CDummyIf6::Control(KSOLInterface, KSoIfInfo6, ...)"));
			__ASSERT_DEBUG((TUint)aOption.MaxLength() >= sizeof (TSoIfInfo6), User::Panic(_L("Dummy6"), 0));

			if ((TUint)aOption.MaxLength() < sizeof (TSoIfInfo6))
				return KErrArgument;

			TSoIfInfo6& opt = *(TSoIfInfo6*)aOption.Ptr();
			opt.iFeatures = KIfCanBroadcast | KIfCanMulticast;
			opt.iName.Copy(iIfName);
			opt.iMtu = 1500;
			opt.iRMtu = opt.iMtu;
			opt.iSpeedMetric = 0;

			return KErrNone;
			}

		case KSoIfHardwareAddr:
			return KErrNotSupported;

		case KSoIfConfig:
			{
			CDummyIfLog::Printf(_L("CDummyIf6::Control(KSOLInterface, KSoIfConfig, ...)"));
			__ASSERT_DEBUG((TUint)aOption.MaxLength() >= sizeof (TSoInet6IfConfig), User::Panic(_L("Dummy6"), 0));

			if ((TUint)aOption.MaxLength() < sizeof (TSoInet6IfConfig))
				return KErrArgument;

			TSoInet6IfConfig& opt = *(TSoInet6IfConfig*)aOption.Ptr();
			if (opt.iFamily != KAfInet6)
				return KErrNotSupported;

			TEui64Addr* ifId = (TEui64Addr*)&opt.iLocalId;

			ifId->Init();
			ifId->SetAddress(iLocalIfId);

			ifId = (TEui64Addr*)&opt.iRemoteId;
			ifId->Init();
			ifId->SetAddress(iRemoteIfId);

			// Setup static DNS address if required

			if (!iPrimaryDns.IsUnspecified())
				{
				opt.iNameSer1.SetAddress(iPrimaryDns);
				if (!iSecondaryDns.IsUnspecified())
					opt.iNameSer2.SetAddress(iSecondaryDns);
				}

			opt.idPaddingBits = 0;
			return KErrNone;
			}

		case KSoIfGetConnectionInfo:
			{
			CDummyIfLog::Printf(_L("CDummyIf6::Control(KSOLInterface, KSoIfGetConnectionInfo, ...)"));
			TSoIfConnectionInfo& opt = *(TSoIfConnectionInfo*)aOption.Ptr();
			TInt err = KErrNone;
			TBuf<2*KCommsDbSvrMaxColumnNameLength+1> fieldName;
			_LIT(KSlashChar, "\\");

			fieldName.Copy(TPtrC(IAP));
			fieldName.Append(KSlashChar);
			fieldName.Append(TPtrC(COMMDB_ID));
			if ((err = iNotify->ReadInt(fieldName, opt.iIAPId)) != KErrNone)
				return err;

			fieldName.Copy(TPtrC(IAP));
			fieldName.Append(KSlashChar);
			fieldName.Append(TPtrC(IAP_NETWORK));
			if ((err = iNotify->ReadInt(fieldName, opt.iNetworkId)) != KErrNone)
				return err;

			return KErrNone;
			}
		default:
			CDummyIfLog::Printf(_L("CDummyIf6::Control(KSOLInterface, aName %x, ...)"), aName);
			break;
			}
		}
	else
		{
		CDummyIfLog::Printf(_L("CDummyIf6::Control(aLevel %x, aName %x, ...)"), aLevel, aName);
		}
	return KErrNotSupported;
	}

TInt CDummyIf6::Notification(TAgentToNifEventType /*aEvent*/, void * /*aInfo*/)
	{
	return KErrNone;
	}

void CDummyIf6::StaticDnsConfiguration()
	{
	TBool requestDynamicDNSAddress;
	// See whether we are configured to request a dynamic DNS address.
	if (iNotify->ReadBool(TPtrC(SERVICE_IP6_DNS_ADDR_FROM_SERVER), requestDynamicDNSAddress) != KErrNone)
		{
		// Default behaviour
		requestDynamicDNSAddress = ETrue;
		}

	if(!requestDynamicDNSAddress)
		{
		// Setup static DNS addresses from CommDb
		CDummyIfLog::Printf(_L("CDummyIf6: Configuring static DNS addresses"));
		PresetAddr(iPrimaryDns, TPtrC(SERVICE_IP6_NAME_SERVER1));
		PresetAddr(iSecondaryDns, TPtrC(SERVICE_IP6_NAME_SERVER2));
		}
	else
		{
		// Ensure that static DNS addresses are set as unspecified,
		// so they are not used in Control(KSoIfConfig).
		iPrimaryDns = KInet6AddrNone;
		iSecondaryDns = KInet6AddrNone;
		}
	}

TInt CDummyIf6::PresetAddr(TIp6Addr& aAddr, const TDesC& aVarName)
/**
 * Preset IP adress in aAddr with value from CommDB. The field name
 * is given in aVarName. Examples are "IpAddr" and "IpNameServer1"
 *
 * @param aAddr    IP address to be set
 * @param aVarName name of CommDB field
 * @return KErrNone if success, global error code otherwise
 */
	{
	if (!aAddr.IsUnspecified())
		return KErrNone;
	
	TBuf<KCommsDbSvrMaxFieldLength> name;

	(void)iNotify->ReadDes(aVarName, name); // ignore return value
	TInetAddr ip6Addr;

	TInt ret = ip6Addr.Input(name);
	if (ret == KErrNone)
		{
		aAddr = ip6Addr.Ip6Address();
		}
	return ret;
	}
