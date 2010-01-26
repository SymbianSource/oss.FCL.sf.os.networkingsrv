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
// The DHCPv6 States header file
// 
//

/**
 @file DHCPIP6States.h
*/

#ifndef __DHCPIP6STATES_H__
#define __DHCPIP6STATES_H__

#include "DHCPStates.h"
#include "DHCPIP6StateMachine.h"

#ifdef SYMBIAN_ESOCK_V3
#include <networking/ipeventtypes.h>
#include <comms-infras/netsignalevent.h>

namespace NetSubscribe
{
class CNetSubscribe;
}

class CDHCPIP6ListenToNeighbor : public SDhcpSignal, public CDHCPState, public MExpireTimer
	{
public:	
	CDHCPIP6ListenToNeighbor(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPState(aDHCPIPv6),
		iEvent( this, SignalHandlerFn ),
		iErr( KErrNone ),
		iMFlag( 0 ), //default act as if we haven't got it
		iOFlag( 0 ) //default act as if we haven't got it
		{
		}
	~CDHCPIP6ListenToNeighbor();
	
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	virtual void TimerExpired();
	void BecomeIdle();
	virtual void Cancel();
	void CompleteClientAndStartInform();

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}

protected:
	static void SignalHandlerFn( TAny* aThis, const Meta::SMetaData* aData );
	
	NetSubscribe::TEvent iEvent;
	TInt iErr; //to avoid infinite loop
	TInt iMFlag;
	TInt iOFlag;
	};
#endif

class CDHCPIP6Solicit : public CDHCPAddressAcquisition
	{
public:
	CDHCPIP6Solicit(CDHCPIP6StateMachine& aDHCPIPv6,TInt aTimeOut = 0) :
		CDHCPAddressAcquisition(aDHCPIPv6), iUserDefinedTimeout( aTimeOut )
		{
		}
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
private:
	TInt iUserDefinedTimeout;		
	};

class CDHCPIP6Select : public CDHCPSelect, public MMSListener
	{		
public:
	CDHCPIP6Select(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPSelect(aDHCPIPv6),
		iBestServerPreference(-1)
		{		
		}
	~CDHCPIP6Select();
   //from CDHCPState->CActiveEvent
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	//from MMSListener
	virtual TInt MSReportError(TInt aError);
	void SetMaxRetryCount(TInt aMaxRetryCount);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
protected:
   	TBool iDone;
	TInt iMaxRetryCount;	//This used in MSReportError so we can control
							//how many more times to try after an error.
	TInt iBestServerPreference;
	RBuf8 iSelectedMessage;
	};

class CDHCPIP6InformRequest : public CDHCPInformationConfig
	{
public:
	CDHCPIP6InformRequest(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPInformationConfig(aDHCPIPv6)
		{
		}

   virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
	};

class CDHCPIP6Request : public CDHCPRequest
	{
public:
	CDHCPIP6Request(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPRequest(aDHCPIPv6)
		{
		}

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
	};

class CDHCPIP6Confirm : public CDHCPRebootConfirm
	{
public:
	CDHCPIP6Confirm(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPRebootConfirm(aDHCPIPv6)
		{
		}
   virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
	};

#if 0
class CDHCPIP6WaitForDAD : public CDHCPWaitForDADIPNotifier
	/** the class waits for DAD to finish and than takes next address from IA, set it and waits for DAD again until
	all addresses have been verified and marked as valid/invalid. 
	@see CDHCPIP6Control::TaskCompleteL
	@internalComponent
	*/
	{
public:
	CDHCPIP6WaitForDAD(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPWaitForDADBind(aDHCPIPv6),
		iAddressIndex( 0 ) //the first address (index 0) is set when reply msg's been received
		{
		}

	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
		
protected:
	TInt iAddressIndex;
	};
#else
class CDHCPIP6WaitForDAD : public CDHCPWaitForDADBind
	/** the class waits for DAD to finish and than takes next address from IA, set it and waits for DAD again until
	all addresses have been verified and marked as valid/invalid. 
	@see CDHCPIP6Control::TaskCompleteL
	@internalComponent
	*/
	{
public:
	CDHCPIP6WaitForDAD(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPWaitForDADBind(aDHCPIPv6),
		iAddressIndex( 0 ) //the first address (index 0) is set when reply msg has been received
		{
		}

	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	virtual void TimerExpired();

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
		
protected:
	TInt iAddressIndex;
	};
#endif

class CDHCPIP6Renew : public CDHCPRenew
	{
public:
	CDHCPIP6Renew(CDHCPIP6StateMachine& aDHCPIPv6) : CDHCPRenew(aDHCPIPv6),iUserDefinedTimeout(0)
		{
		}

	CDHCPIP6Renew(CDHCPIP6StateMachine& aDHCPIPv6,TInt& aTimeOut) : CDHCPRenew(aDHCPIPv6),iUserDefinedTimeout(aTimeOut)
		{
		}

		
	//   ACK received => continue with next state (Bound)
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
private:
	TInt iUserDefinedTimeout;		
	};

class CDHCPIP6Rebind : public CDHCPRebind
	{
public:
	CDHCPIP6Rebind(CDHCPIP6StateMachine& aDHCPIPv6) : CDHCPRebind(aDHCPIPv6),iUserDefinedTimeout(0)
		{		
		}
		
	CDHCPIP6Rebind(CDHCPIP6StateMachine& aDHCPIPv6,TInt& aTimeOut) : CDHCPRebind(aDHCPIPv6),iUserDefinedTimeout(aTimeOut)
		{
		}
	//   ACK received => continue with next state (Bound)
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
private:
	TInt iUserDefinedTimeout;		
	};

class CDHCPIP6Reconfigure : public CDHCPRequest
	{
public:
	CDHCPIP6Reconfigure(CDHCPIP6StateMachine& aDHCPIPv6) : CDHCPRequest(aDHCPIPv6)
		{
		}
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	virtual CDHCPState* ProcessAckNakL(TRequestStatus* aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
	};

class CDHCPIP6Release : public CDHCPRelease
	{
public:
	CDHCPIP6Release(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPRelease(aDHCPIPv6)
		{
		}
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
	};

class CDHCPIP6Decline : public CDHCPDecline
	{
public:
	CDHCPIP6Decline(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPDecline(aDHCPIPv6)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
	};

class CDHCPIP6ReplyNoBinding : public CDHCPRequest
	{
public:
	CDHCPIP6ReplyNoBinding(CDHCPIP6StateMachine& aDHCPIPv6) :
		CDHCPRequest(aDHCPIPv6)
		{
		}
	virtual CDHCPState* ProcessAckNakL(TRequestStatus* aStatus);

	CDHCPIP6StateMachine& DHCPIPv6()
		{
		return static_cast<CDHCPIP6StateMachine&>(*iStateMachine);
		}
	};

#endif
