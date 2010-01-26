// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Agent APIs
// 
//

#if !defined(__NIFAGT_H__)
#define __NIFAGT_H__

#ifndef __NIFMAN_H__
#include <nifman.h>
#endif

#ifndef __NIFPRVAR_H__
#include <comms-infras/nifprvar.h>
#endif

#include <comms-infras/connectionsettings.h>
#include <cdbstore.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#include <nifman_internal.h>
#endif

// Agent classes

class CNifAgentBase;
class CNifAgentRef;
class CNifSession;

class CNifAgentRefN1;
class CNifAgentFactory : public CNifFactory
	{
friend class CNifAgentRef;
friend class CNifAgentRefN1;
protected:
	virtual CNifAgentBase *NewAgentL(const TDesC& aName) = 0;
	virtual TInt Info(TNifAgentInfo& aInfo, TInt aIndex) const = 0;
	};

class MNifAgentNotify
/**
 * Interface from the agent to Nifman
 * @internalComponent
 */
	{
public:
	virtual void ConnectComplete(TInt aStatus) = 0;
	virtual void ReconnectComplete(TInt aStatus) = 0;
	virtual void AuthenticateComplete(TInt aStatus) = 0;
	virtual void ServiceStarted() = 0;
	virtual void ServiceClosed() = 0;
	virtual void DisconnectComplete() = 0;
	virtual void AgentProgress(TInt aStage, TInt aError) = 0;

	/**
	 * Progress notification from an agent for a subconnection
	 * @note This function will not be called until agents support starting subconnections
	 * @param aSubConnectionUniqueId The subconnection to which this notification refers
	 * @param aStage The progress stage that has been reached
	 * @param aError Any errors that have occured
	 */
	virtual void AgentProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError) = 0;

	virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo = NULL) = 0;
	virtual TInt IncomingConnectionReceived() = 0;		/* needs to be called by the agent when it gets 
	an incoming call so that agent can wait for an incoming and do an outgoing at the 
	same time, i.e. it waits for the incoming without NifMan's knowledge. */

	/**
	 * Indication that some parameter relating to a subconnection has changed in the agent eg. new subconnection opened, QoS changed
	 * @param aSubConnectionUniqueId The subconnection to which this notification refers
	 * @param aEvent The event which has occured
	 */
	virtual void AgentEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0)=0;
	};

class MAgentSessionNotify
/**
 * Interface from agents to CNifSession
 * @internalComponent
 * @note This used to be the interface used by both CNifAgentRef and the agent to communicate with CNifSession
 * @note It has now been altered so only the agent uses it, for ServiceChangeNotification(); ProgressNotification() has now been moved to the MNifSessionNotify interface
 * @note Agents should not have been calling ProgressNotification() anyway, they should call MNifAgentNotify::AgentProgress()
 */
	{
public:
	virtual void ServiceChangeNotification(TUint32 aId, const TDesC& aType) = 0;
	};

class MNifAgentExtendedManagementInterface
/**
 * Interface for extended RConnection functionality
 * Intended for derivation by agents wishing to support extended connection management functionality
 * @see Corresponding MNifIfExtendedManagementInterface for nifs wishing to support extended connection management functionality
 * @internalComponent
 * @note Augments CNifAgentBase, which provides basic connection management functionality
 * @note To support this interface, clients must accept a Control(KCOLAgent, KCOGetAgentEMIPtr, TPckg<MNifIfExtendedManagementInterface*>) and return a pointer to a class that implements this interface
 * @note Until the APIs are correctly defined to support agents that start subconnetions, this class should also be derived in the nif
 */
	{
public:
	/**
	 * Find out the type of this connection (eg. CSD, GPRS, CDMA, Ethernet, some more specific subtype)
	 * @param aConnectionType On return, contains the type of the connection
	 * @returns KErrNone, or one of the system-wide error codes
	 */
	virtual TInt GetInterfaceType(TConnectionType& aConnectionType)=0;

	/**
	 * Discover how many subconnections exist on an interface
	 * @note All interfaces must support at least one subconnection
	 * @param aCount On return, contains the number of subconnections that currently exist on this interface
	 * @returns KErrNone, or one of the system-wide error codes
	 */
	virtual TInt EnumerateSubConnections(TUint& aCount)=0;

	/**
	 * Get information about a subconnection with no prior knowledge of the unique id of any subconnetion
	 * @note aSubConnectionInfo should contain a class derived from TSubConnectionInfo; the class should be suitable for type of interface being queried, however it is up to the interface to check this.
	 * @note The type of the class can be determined by checking the iConnectionType member
	 * @note Unique IDs for subconnections are on a per-nif basis.  It is the responsibility of the nif or agent to generate these IDs and to ensure they are unique across the connection
	 * @param aIndex A number between one and the total number of subconnections returned by EnumerateSubConnections()
	 * @param aSubConnectionInfo Should contain a class derived from TSubConnectionInfo and suitable for the type of the interface being queried; on return, contains the class with all its members filled in
	 * @returns KErrNone, or one of the system-wide error codes; in particular KErrArgument if the class type is unsuitable
	 */
	virtual TInt GetSubConnectionInfo(TUint aIndex, TDes8& aSubConnectionInfo)=0;
  
	/**
	 * Get information about the subconnection with the supplied unique ID
	 * @note aSubConnectionInfo should contain a class derived from TSubConnectionInfo; the class should be suitable for type of interface being queried, however it is up to the interface to check this.
	 * @note The type of the class can be determined by checking the iConnectionType member
	 * @param aSubConnectionInfo Should contain a class derived from TSubConnectionInfo and suitable for the type of the interface being queried, and with the iSubConnectionUniqueId member set to the appropriate unique ID
	 * @returns KErrNone, or one of the system-wide error codes; in particular KErrArgument if the class type is unsuitable
	 */
	virtual TInt GetSubConnectionInfo(TDes8& aSubConnectionInfo)=0;
	};


class CNifAgentBase : public CBase
/** @internalComponent */
	{
friend class CNifAgentRef;
friend class CNifAgentRefN1;
public:
	IMPORT_C CNifAgentBase();
	IMPORT_C virtual TInt Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption);

	virtual void Info(TNifAgentInfo& aInfo) const = 0;
	virtual void Connect(TAgentConnectType aType) = 0;
	virtual void Connect(TAgentConnectType aType, CStoreableOverrideSettings* aOverrideSettings) = 0;
	virtual void CancelConnect() = 0;
	virtual void Reconnect() = 0;
	virtual void CancelReconnect() = 0;
	virtual void Authenticate(TDes& aUsername, TDes& aPassword) = 0;
	virtual void CancelAuthenticate() = 0;
	virtual void Disconnect(TInt aReason) = 0;
	virtual TInt GetExcessData(TDes8& aBuffer) = 0;
	virtual TInt Notification(TNifToAgentEventType aEvent, TAny* aInfo) = 0;
	virtual TInt IncomingConnectionReceived() = 0;
	virtual void GetLastError(TInt& aError) = 0;
	virtual TBool IsActive() const = 0;
	virtual TBool IsReconnect() const = 0;
	virtual void SetConnectionSettingsL(const TConnectionSettings& aSettings) = 0;
	virtual TConnectionSettings& ConnectionSettingsL() = 0;
	virtual void SetOverridesL(CStoreableOverrideSettings* aOverrideSettings) = 0;
	virtual CStoreableOverrideSettings* OverridesL() = 0;

	virtual void RequestNotificationOfServiceChangeL(MAgentSessionNotify* aSession) = 0;
	virtual void CancelRequestNotificationOfServiceChange(MAgentSessionNotify* aSession) = 0;

	IMPORT_C virtual TInt Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, const RProcess& aProcess);
	IMPORT_C TInt ReadInt(const TDesC& aField, TUint32& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadInt(const TDesC& aField, TUint32& aValue );
	IMPORT_C TInt WriteInt(const TDesC& aField, TUint32 aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteInt(const TDesC& aField, TUint32 aValue );
	IMPORT_C TInt ReadDes(const TDesC& aField, TDes8& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadDes(const TDesC& aField, TDes8& aValue );
	IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC8& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC8& aValue );
	IMPORT_C TInt ReadDes(const TDesC& aField, TDes16& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadDes(const TDesC& aField, TDes16& aValue );
	IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC16& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC16& aValue );
	IMPORT_C TInt ReadBool(const TDesC& aField, TBool& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadBool(const TDesC& aField, TBool& aValue );
	IMPORT_C TInt WriteBool(const TDesC& aField, TBool aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteBool(const TDesC& aField, TBool aValue );
	IMPORT_C HBufC* ReadLongDesLC(const TDesC& aField, const RMessagePtr2* aMessage );
	IMPORT_C HBufC* ReadLongDesLC(const TDesC& aField );

	IMPORT_C TInt CheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage );
	IMPORT_C TInt CheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage );



protected:
	virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoWriteInt(const TDesC& aField, TUint32 aValue, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoReadBool(const TDesC& aField, TBool& aValue, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoWriteBool(const TDesC& aField, TBool aValue, const RMessagePtr2* aMessage ) = 0;
	virtual HBufC* DoReadLongDesLC(const TDesC& aField, const RMessagePtr2* aMessage ) = 0;

	virtual TInt DoCheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage );
	virtual TInt DoCheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage );
	private:
	TBool CheckControlPolicy(TUint aLevel, TUint aOption, const RProcess& aProcess);


protected:
	MNifAgentNotify* iNotify;
	};

#endif

