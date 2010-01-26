// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation of dummyNif.
// 
//

/**
 @file
 @internalTechnology
*/

#include <comms-infras/nifif.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <nifmbuf.h>
#include <in_iface.h>
#include <comms-infras/commsdebugutility.h>
#include <connectprog.h>

#include <in_chk.h>
#include <in_sock.h>
#include <tcp_hdr.h>

#include "dummynifvar.h"
#include "dummynif.h"

/*
 * This sections defines a whole load of constants etc... not very exciting
 */

_LIT(KDummyIfLogFolder, "dummynif");
_LIT(KDummyIfLogFile, "dummynif.txt");
_LIT(KEndOfLine, "\n");

/*
 * The Link class
 */

CDummyIfLink::CDummyIfLink(CNifIfFactory& aFactory)
	: CNifIfLink(aFactory),
	  iFactory(&aFactory)
	{
	CDummyIfLog::Printf(_L("CDummyIfLink::CDummyIfLink()"));
	}

CDummyIfLink::~CDummyIfLink()
	{
	TimerDelete();
	}
	
CDummyIfFactory& CDummyIfLink::Factory()
	{
	return static_cast<CDummyIfFactory&>(*iFactory);
	}

void CDummyIfLink::Info(TNifIfInfo& aInfo) const
	{
	FillInInfo(aInfo, (TAny*) this);
	}

void CDummyIfLink::FillInInfo(TNifIfInfo& aInfo, TAny* aPtr)
	{
	aInfo.iProtocolSupported=0;
	aInfo.iVersion = TVersion(1,1,1);
	aInfo.iFlags = KNifIfIsBase | KNifIfIsLink | KNifIfUsesNotify | KNifIfCreatedByFactory;
    aInfo.iName = _L("dummyiflink");
	aInfo.iName.AppendFormat(_L("[0x%08x]"), aPtr);
	aInfo.iFlags |= KNifIfCreatesBinder;
	}

TInt CDummyIfLink::Send(RMBufChain& /*aPdu*/, TAny* /*aSource*/)
	{
	// Not used - just for satisfying pure virtual
	return KDummyNifSendOkay;
	}

TInt CDummyIfLink::Start()
	{
	// NOTE: according to the NAF docs the sequence should really be StartSending(), then LinkLayerUp() then Progress()
	// for DNS to work.  However, as this dummy NIF doesn't support DNS, the sequence is okay as it stands.
	// 
	CDummyIfLog::Write(_L("CDummyIfLink::Start()"));

	iNotify->IfProgress(KLinkLayerOpen, KErrNone);
	iNotify->LinkLayerUp();
	if (iNifIf4)
		iNifIf4->iProtocol->StartSending((CProtocolBase*)iNifIf4);
	if (iNifIf6)
		{
		// setup static DNS configuration if required
		iNifIf6->StaticDnsConfiguration();

		iNifIf6->iProtocol->StartSending((CProtocolBase*)iNifIf6);
		}
	return KErrNone;
	}

void CDummyIfLink::TimerComplete(TInt)
	{
    iNotify->LinkLayerDown(KErrTimedOut, MNifIfNotify::EDisconnect);
	}

void CDummyIfLink::AuthenticateComplete(TInt aResult)
	{
	iNotify->IfProgress(5, aResult);
	}

void CDummyIfLink::Stop(TInt aError, MNifIfNotify::TAction aAction)		
	{
	CDummyIfLog::Printf(_L("CDummyIfLink::Stop(aError %d, TAction %d)"), aError, aAction);
	iNotify->IfProgress(KLinkLayerClosed, aError);
	iNotify->LinkLayerDown(aError, aAction);
	}

void CDummyIfLink::BindL(TAny *aId)
	{
	CDummyIfLog::Printf(_L("CDummyIfLink::BindL(aId %x)"), aId);
	}

CNifIfBase* CDummyIfLink::GetBinderL(const TDesC& aName)
{
	CDummyIfLog::Printf(_L("CDummyIfLink::GetBinderL(%S)"), &aName);

	_LIT(KDescIp6, "ip6");
	if (aName.CompareF(KDescIp6) == 0)
	{
		iNifIf6 = new(ELeave) CDummyIf6(*this);
		return iNifIf6;
	}
	else
	{	// ip4
		iNifIf4 = new(ELeave) CDummyIf4(*this);
		return iNifIf4;
	}
}

TInt CDummyIfLink::Notification(TAgentToNifEventType aEvent,void* aInfo)
	{
	if (aEvent!=EAgentToNifEventTypeDisableTimers)
		return KErrUnknown;
	if (aInfo!=NULL)
		return KErrUnknown;

	return KErrNone;
	}

void CDummyIfLink::Restart(CNifIfBase*)
	{}

/**
Sets the cached value of Nifman Idle timeout to the specified value.

@param aTimeoutToSet the idle timeout to update. One of:
	LastSessionClosedTimeout
	LastSocketClosedTimeout
	LastSocketActivityTimeout
@param aTimeoutValueBuf a package buffer containing the new value for the timeout specified by aTimeoutToSet
@return KErrNone on success, or a system wide error code. 
*/
TInt CDummyIfLink::SetNifmanIdleTimeout(const TDesC& aTimeoutToSet, const TDes8& aTimeoutValueBuf)
	{
    ASSERT(aTimeoutToSet.Compare(TPtrC(LAST_SESSION_CLOSED_TIMEOUT))  == 0 ||
		   aTimeoutToSet.Compare(TPtrC(LAST_SOCKET_CLOSED_TIMEOUT))   == 0 ||
		   aTimeoutToSet.Compare(TPtrC(LAST_SOCKET_ACTIVITY_TIMEOUT)) == 0);
	
	if(!aTimeoutValueBuf.Ptr()) // NULL pointer provided. 
		{
		CDummyIfLog::Printf(_L("CDummyIfLink::SetNifmanIdleTimeout timeoutToSet[%S]: NULL pointer provided where timeout value expected. Returning KErrAgument[-6]"), &aTimeoutToSet);
		return KErrArgument;
		}	
	//
	// Extract the argument value and set the appropriate timeout.	
	//
	const TInt KNewTimeoutValue = 
	  *(
	   reinterpret_cast<const TInt*>(aTimeoutValueBuf.Ptr())
	   );
	TInt setErr = iNotify->WriteInt(aTimeoutToSet, KNewTimeoutValue);
	CDummyIfLog::Printf(_L("CDummyIfLink::SetNifmanIdleTimeout [%S] to [%d]. WriteInt Error [%d]"), &aTimeoutToSet, KNewTimeoutValue, setErr);
	return setErr;
	}

/**
Controls the Interface.
Capability to change LastSocketActivityTimeout on the fly (in CDMA Mobile IP)

@param aLevel option level. Must be KCOLInterface.
@param aName  option name. One of:
	KTestSoDummyNifSetLastSessionClosedTimeout  - updates the LastSessionClosed timeout
	KTestSoDummyNifSetLastSocketClosedTimeout   - updates the LastSocketClosed timeout
	KTestSoDummyNifSetLastSocketActivityTimeout - updates the LastSocketActivity timeout
@param aOption package buffer containing the TInt value to set the timeout to.
@return KErrNone on success, or a system-wide error code. Common errors are:
	KErrNotSupported if the provided option level is not of the type KCOLInterface
	KErrNotSupported if the provided option name is not supported (not implemented)
	KErrArgument if the argument is unacceptable (e.g. aOption carries NULL)
*/
TInt CDummyIfLink::Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* /* aSource */)
	{
	TInt colErr(KErrNotSupported);	
	
	if(KCOLInterface == aLevel) // The only level supported.
		{	
		switch(aName)
			{		
			// Support for testing updating of Nifman Idle timeouts via CNifAgentRef::WriteInt
			// When called, the specified timeout is set to the provided value		
			case KTestSoDummyNifSetLastSessionClosedTimeout:
				colErr = SetNifmanIdleTimeout(TPtrC(LAST_SESSION_CLOSED_TIMEOUT),  aOption);
				break;	
			case KTestSoDummyNifSetLastSocketClosedTimeout:
				colErr = SetNifmanIdleTimeout(TPtrC(LAST_SOCKET_CLOSED_TIMEOUT),   aOption);	
				break;
			case KTestSoDummyNifSetLastSocketActivityTimeout: 
				colErr = SetNifmanIdleTimeout(TPtrC(LAST_SOCKET_ACTIVITY_TIMEOUT), aOption);
				break;
			default:
				;			
			}
		}
	// Log Option Name in a readable form by removing User Read bit. 
	const TInt KOptionName  = aName & ~KConnReadUserDataBit;		
	CDummyIfLog::Printf(_L("CDummyIfLink::Control Level [%d], Name [%d], Raw Name [%d]. Error [%d]"), aLevel, KOptionName, aName, colErr);
	return colErr;	
	}


/*
 * The IPv4 interface binder class
 */

CDummyIf4::CDummyIf4(CDummyIfLink& aLink)
	: CNifIfBase(aLink)
	{
	CDummyIfLog::Printf(_L("CDummyIf4::CDummyIf4()"));

	iLink = &aLink;
	iLink->Factory().SetDripReceiver(TCallBack(DrainNextDrips, this));

	// generate my local ip address (ip4) - vals potentially will be overwritten by any derived classes
	iLocalAddressBase = KDummyNifLocalAddressBase; // also used later in control method
	iLocalAddress = iLocalAddressBase + ((TUint32)this)%255;

	iIfName.Format(_L("dummynif[0x%08x]"), this);
	}
	
CDummyIf4::~CDummyIf4()
	{
	iLink->Factory().SetDripReceiver(TCallBack());
	}

void CDummyIf4::BindL(TAny *aId)
	{
	CDummyIfLog::Printf(_L("CDummyIf4::BindL(aId %x)"), aId);
	if(iProtocol)
		User::Leave(KErrAlreadyExists);
	iProtocol = (CProtocolBase*)aId;	
	}

TInt CDummyIf4::Send(RMBufChain& aPdu, TAny*)
	{
	Recv(aPdu);
	return 1;
	}

TInt CDummyIf4::State()
    {
    return EIfUp;
    }

void CDummyIf4::UpdateHeaders(TInet6HeaderIP4* aIp4, TInet6HeaderUDP* aUdp)
/**
Update the IPv4 and UDP headers to allow the packet to be looped back.
*/
{
	// swap over the destination and source addresses
	TUint32 temp;
	temp = aIp4->SrcAddr();
	aIp4->SetSrcAddr(aIp4->DstAddr());
	aIp4->SetDstAddr(temp);

	// we've changed the ip hdr so need to recalculate the ip hdr checksum
	aIp4->SetChecksum(0); 
	aIp4->SetChecksum(TChecksum::ComplementedFold(TChecksum::Calculate((TUint16*)aIp4, aIp4->HeaderLength())));

	// also want to set the udp checksum to zero cos it will be wrong now that we have 
	// changed the ip hdr - just set to zero and it is ignored
	aUdp->SetChecksum(0);

}

void CDummyIf4::Recv(RMBufChain& aPdu)
	{

	TInt res;
	TBool drop = EFalse;
	
	// this received data has already been looped back...
	// get the ip header from the RMBufChain
	TInet6HeaderIP4* ip4 = (TInet6HeaderIP4*) aPdu.First()->Next()->Ptr();
    if(ip4->Protocol() == KProtocolInetUdp)
    	{
		// get the udp header as well - assume only udp traffic here
		TInet6HeaderUDP* udp = (TInet6HeaderUDP*) ip4->EndPtr();
	
		TUint8* subConn = udp->EndPtr()+1;
	
		CDummyIfLog::Printf(_L("CDummyIf4::Recv(...): UDP length %d, src port %d, dst port %d"),
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
				UpdateHeaders(ip4, udp);
				break;

			case KForceReconnect:
				CDummyIfLog::Printf(_L("KForceReconnect command"));
				// do some action
				iNotify->IfProgress(KLinkLayerClosed, KErrCommsLineFail);
				iNotify->LinkLayerDown(KErrCommsLineFail, MNifIfNotify::EReconnect);
				// no return code so all we can do is respond with what we got
				UpdateHeaders(ip4, udp);
				break;

			case KSendNotification:
				CDummyIfLog::Printf(_L("KSendNotification command"));
				res = iNotify->Notification(ENifToAgentEventTypeQueryIsDialIn);
				//let's write the result in the next byte of the reply
				if (res == KErrNotSupported)
					udp->EndPtr()[1] = (unsigned char) KErrNone;
				else
					udp->EndPtr()[1] = (unsigned char) KErrGeneral; // this will lose it's sign :-(
				
				UpdateHeaders(ip4, udp);
				break;
			case KForceFinishedSelection:
				CDummyIfLog::Printf(_L("KForceFinishedSelection command"));
				// force subConn into KFinishedSelection State
				iNotify->IfProgress(*subConn, KFinishedSelection, KErrNone);

				UpdateHeaders(ip4, udp);
				break;
			default:
				CDummyIfLog::Printf(_L("Unknown command - ignoring it"));
				drop = ETrue;
				// unknown command, just ignore this packet???
			}
		}
		else
			{	// just echo the packet back to the original sender
			
			// update the headers (addresses, checksums etc)
			UpdateHeaders(ip4, udp);
			// now process it (pass up the stack)
			}
    	}
    else if(ip4->Protocol() == KProtocolInetTcp)
    	{
		// swap over the destination and source addresses
		TUint32 origSrc = ip4->SrcAddr();
		TUint32 origDst = ip4->DstAddr();
		ip4->SetSrcAddr(origDst);
		ip4->SetDstAddr(origSrc);
		ip4->SetChecksum(0); 
		ip4->SetChecksum(TChecksum::ComplementedFold(TChecksum::Calculate((TUint16*)ip4, ip4->HeaderLength())));
    	}
    else
    	{
		CDummyIfLog::Printf(_L("CDummyIf4::Recv(...): IPv4 length %d, protocol %d [passing through]"), ip4->TotalLength(), ip4->Protocol());
    	}
    if(drop)
    	{
    	aPdu.Free();
    	}
    else
    	{
    	if(KDelaySlots > 0)
    		{
	    	iLink->Factory().AddDrip(aPdu.First());
	    	aPdu.Init();
    		}
    	else
    		{
			iProtocol->Process(aPdu, (CProtocolBase*)this);
    		}
    	}
	}
	
TInt CDummyIf4::DrainNextDrips(TAny* aSelf)
	{
	CDummyIf4* self = (CDummyIf4*) aSelf;
	RMBuf* next;
	while((next = self->iLink->Factory().GetDrip()) != NULL)
		{
		RMBufChain pdu(next);
		self->iProtocol->Process(pdu, (CProtocolBase*)self);
		}
	return 0;
	}
	

void CDummyIf4::Info(TNifIfInfo& aInfo) const
	{
	aInfo.iVersion = TVersion(1,1,1);
	aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfCreatedByLink;
	aInfo.iName.Copy(iIfName);
	aInfo.iProtocolSupported = 0;
	}

TInt CDummyIf4::Control(TUint aLevel, TUint aName, TDes8& aOption, TAny* /* aSource */)
	{
	CDummyIfLog::Printf(_L("CDummyIf::Control(aLevel %x, aName %x, ...)"), aLevel, aName);

	if (aLevel==KSOLInterface)
		{
		switch (aName)
			{
		case KSoIfInfo:
			{
			TSoIfInfo& opt = *(TSoIfInfo*)aOption.Ptr();

			_LIT(KName, "MyName");
			opt.iName.Copy(KName);
			opt.iFeatures = KIfCanBroadcast | KIfCanMulticast;
			opt.iMtu = 1500;
			opt.iSpeedMetric = 0;
			return KErrNone;
			}

		case KSoIfHardwareAddr:
			return KErrNotSupported;

		case KSoIfConfig:
			{
			TSoInetIfConfig& opt = *(TSoInetIfConfig*)aOption.Ptr();
			if (opt.iFamily!=KAfInet)
				return KErrNotSupported;

			TUint32 address;
			const TInt KPort = 65;

			opt.iConfig.iAddress.SetAddress(iLocalAddress);
			opt.iConfig.iAddress.SetPort(KPort);

			// network mask
			opt.iConfig.iNetMask.Input(KNetworkMask);
			opt.iConfig.iNetMask.SetPort(KPort);

			// broadcast address
			address = iLocalAddressBase + KBroadcastAddressSuffix;
			opt.iConfig.iBrdAddr.SetAddress(address);
			opt.iConfig.iBrdAddr.SetPort(KPort);

			// default gateway
			address = iLocalAddressBase + KDefaultGatewayAddressSuffix;
			opt.iConfig.iDefGate.SetAddress(address);
			opt.iConfig.iDefGate.SetPort(KPort);

			// primary DNS, just make same as default gateway
			opt.iConfig.iNameSer1.SetAddress(address);
			opt.iConfig.iNameSer1.SetPort(KPort);

			// secondary DNS
			address = iLocalAddressBase + KSecondaryDnsAddressSuffix;
			opt.iConfig.iNameSer2.SetAddress(address);
			opt.iConfig.iNameSer2.SetPort(KPort);

			return KErrNone;
			}

		case KSoIfCompareAddr:
			if(((TInetAddr&)aOption).Address()!=iLocalAddress)
				return KErrBadName;
			return KErrNone;

		case KSoIfGetConnectionInfo:
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
		}
	return KErrNotSupported;
	}

TInt CDummyIf4::Notification(TAgentToNifEventType /*aEvent*/, void * /*aInfo*/)
	{
	return KErrNone;
	}

void CDummyIfLog::Write(const TDesC& aDes)
//
// Write aText to the log
//
	{

	RFileLogger::Write(KDummyIfLogFolder(), KDummyIfLogFile(), EFileLoggingModeAppend, aDes);
	}

void CDummyIfLog::Printf(TRefByValue<const TDesC> aFmt,...)
//
// Write a mulitple argument list to the log, trapping and ignoring any leave
//
	{

	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KDummyIfLogFolder(), KDummyIfLogFile(), EFileLoggingModeAppend, aFmt, list);
	}

void CDummyIfLog::HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen, TInt aWidth)
	{

	TBuf<0x100> buf;
	TInt i = 0;
	const TText* p = aHeader;
	while (aLen>0)
		{
		TInt n = aLen>aWidth ? aWidth : aLen;
		if (p!=NULL)
			{
			_LIT(string1,"%s%04x : ");
			buf.AppendFormat(string1, p, i);
			}
		TInt j;
		_LIT(string2,"%02x ");
		for (j=0; j<n; j++)
			buf.AppendFormat(string2, aPtr[i+j]);
		_LIT(string3,"   ");
		while (j++<KHexDumpWidth)
			buf.Append(string3);
		_LIT(string4," ");
		buf.Append(string4);
		_LIT(string5,"%c");
		for (j=0; j<n; j++)
			buf.AppendFormat(string5, aPtr[i+j]<32 || aPtr[i+j]>126 ? '.' : aPtr[i+j]);
		buf.Append(KEndOfLine);
		Write(buf);
		buf.SetLength(0);
		aLen -= n;
		i += n;
		p = aMargin;
		}
	}


