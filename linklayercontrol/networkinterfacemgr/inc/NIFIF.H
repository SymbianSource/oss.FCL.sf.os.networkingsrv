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
// Network interface (nif) APIs.
// This file contains all the APIs required to implement a NIF for Symbian OS.
// 
//

/**
 @file NIFIF.H
 @publishedPartner
 @released
*/

#if !defined(__NIFIF_H__)
#define __NIFIF_H__

#define SYMBIAN_NETWORKING_UPS

#include <nifman.h>
#include <comms-infras/nifprvar.h>
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifif_internal.h>
#endif


/**
 The direction of data transfer
 Currently used by network layers making the PacketActivity() call to specify
 which direction the data was transferred in

 Flags for use with the TNifIfInfo structure
 @defgroup TNifIfInfoflags Flags for TNifIfInfo
 @publishedPartner
 @released
 @since v5.0
 */
//@{
const TUint KNifIfIsBase           = 0x00000001;	//< The object is CNifIfBase-derived (one per protocol, per nif)
const TUint KNifIfIsLink           = 0x00000002;	//< The object is CNifIfLink-derived (one per nif)
const TUint KNifIfUsesNotify       = 0x00000004;	//< The object uses the MNifIfNotify interface
const TUint KNifIfCreatedByFactory = 0x00000008;	//< The object was created by a call to CNifIfFactory
const TUint KNifIfCreatedByLink    = 0x00000010;	//< The object was created by the link layer of the nif (and is usually a binder to a layer 3 protocol)
const TUint KNifIfCreatedAlone     = 0x00000020;	//< The object was created using another mechanism
													//< @note This is typically used for local loopback interfaces, where the interface is created by a means other than the nif factory object
const TUint KNifIfCreatesBinder    = 0x00000040;	//< The object creates other objects to bind to a layer 3 protocol
//@}


enum TDataTransferDirection
/**
 * The direction of data transfer
 * @par
 * Currently used by network layers making the PacketActivity() call to specify which direction the data was transferred in
 *
 * @publishedPartner
 * @released
 * @since v7.0s
 */
	{
	/** Sometime idle timers need to be reset without a packet arriving -
	using this value allows packet counts to be maintained correctly */
	ENoPacket,
	EIncoming,
	/** Leave a gap in case it is necessary to distinguish different sorts of incoming data in the future */
	EOutgoing = 128
	};


// Interface classes

class TNifIfInfo
/**
 * Structure used by a nif to pass information about this object in the nif to the caller.
 * @note Accessed via the CNifIfBase::Info() call, and the CNifIfLink::Info() call, since it is a derived class
 *
 * @publishedPartner
 * @released
 * @since v5.0
 */
	{
public:
	TUint iProtocolSupported;	//< The protocol supported by this object
								//< @note Typically KProtocolInetIp or Inet6Ip for IP binders, K<protocol name>IdEsock for link layer protocols
	TUint iFlags;				//< Flags indicating the class type, reason for creation of this object, interfaces that it uses
								//< @see TNifIfInfoflags
	TVersion iVersion;			//< The version of this object
	TName iName;				//< The "friendly" (for user display, logging) name of the nif
	};

class CNifIfBase;
class MNifIfNotify;
class CNifIfFactory : public CNifFactory
/**
 * The factory object used to create new instances of a nif.
 * A nif is packaged in a dll - nifman will call this class to create new instances of a nif.
 *
 * @publishedPartner
 * @released
 * @since v5.0
 */
	{
friend class CNifMan;
protected:
	/**
	 * Create a new instance of the nif contained within this DLL
	 * @param aName A protocol name supported by the nif
	 * @returns A CNifIfBase-derived object of the requested type
	 * @note The class returned should actually be a CNifIfLink-derived object
	 */
	virtual CNifIfBase *NewInterfaceL(const TDesC& aName)=0;

	/**
	 * Return information about modules within this nif
	 * @param aInfo On return, contains information about a module within the nif
	 * @param aIndex An index into CNifIfBase/CNifIfLink-derived objects in the nif, starting from 0.  It is up to the nif how it assigns indices to objects.
	 * @returns KErrNone if successful; otherwise, one of the standard Symbian OS error codes
	 */
	virtual TInt Info(TNifIfInfo& aInfo, TInt aIndex) const=0;

	IMPORT_C virtual CNifIfBase *NewInterfaceL(const TDesC& aName, MNifIfNotify* aNotify);
	};

class MNifIfNotify
/**
 * Interface from the nif and the protocol layers to Nifman
 * @publishedPartner
 * @released
 */
	{
public:
	enum TAction
	{
		EReconnect,
		ECallBack,
		EDisconnect,
		ENoAction
	};

public:
	/**
	 * Notification from the nif that the link layer has gone down.
	 * @param aReason A Symbian OS error code indicating the reason for the link closing down
	 * @param aAction The action that should be taken as a result of link layer down being signalled
	 */
    virtual void LinkLayerDown(TInt aReason, TAction aAction)=0;

	/**
	 * Notification from the nif that the link layer is up
	 */
	virtual void LinkLayerUp()=0;

	/**
	 * Indicate that link negotiation failed on this nif to allow nifman to take appropriate action
	 * @param aIf The address of the CNifIfBase/Link object for which negotiation failed
	 * @param aReason An error code denoting the reason for failure
	 */
    virtual void NegotiationFailed(CNifIfBase* aIf, TInt aReason)=0;

	/**
	 * Request to the agent to return the data required for authentication
	 * @param aUsername On return, contains the username to be used for authentication
	 * @param aPassword On return, contains the password to be used for authentication
	 * @note The return values in aUsername and aPassword should not be considered valid until CNifIfLink::AuthenticateComplete() has been called
	 */
    virtual TInt Authenticate(TDes& aUsername, TDes& aPassword)=0;

	/**
	 * Request to the agent to cancel a pending authentication
	 */
    virtual void CancelAuthenticate()=0;

	/**
	 * Fetch any excess data from the agent that it received during connection setup
	 * @param aBuffer On return, the buffer contains the excess data from the agent
	 * @returns KErrNone, if successful; otherwise, one of the standard Symbian OS error codes
	 */
	virtual TInt GetExcessData(TDes8& aBuffer)=0;
	/**
	 * Upcall from the nif indicating that a new progress state has been reached on this connection
	 * @pre The stage value is within the range assigned to nifs i.e. KMinNifProgress <= aStage <= KMaxNifProgress
	 * @param aStage The progress stage that has been reached
	 * @param aError Any errors that have occured
	 * @since v5.0
	 */
	virtual void IfProgress(TInt aStage, TInt aError)=0;

	/**
	 * Upcall from the nif indicating that a new progress state has been reached on this subconnection
	 * @pre The stage value is within the range assigned to nifs i.e. KMinNifProgress <= aStage <= KMaxNifProgress
	 * @param aSubConnectionUniqueId The subconnection to which this notification refers
	 * @param aStage The progress stage that has been reached
	 * @param aError Any errors that have occured
	 * @since v7.0s
	 */
	virtual void IfProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError)=0;

	/**
	 * Increment the count of users of this nif
	 */
	virtual void OpenRoute()=0;

	/**
	 * Decrement the count of users of this nif
	 */
	virtual void CloseRoute()=0;

	/**
	 * Notification of an event from the nif
	 * @param aEvent The type of event that occured
	 * @param aInfo Any data associated with the event
 	 * @returns KErrNone if successful, otherwise one of the system-wide error codes
	 * @note SC with previous releases broken in v6.1 when first argument changed type
	 */
	virtual TInt Notification(TNifToAgentEventType aEvent, void * aInfo = NULL)=0;

	/**
	 * Notification from the nif that a binder layer has gone down
	 * @param aBinderIf An identifier for the binder layer that went down
	 * @param aReason The reason for the binder layer failure
	 * @param aAction The action to be taken as a result, eg. disconnect, reconnect
	 * @since v7.0s
	 */
	virtual void BinderLayerDown(CNifIfBase* aBinderIf, TInt aReason, TAction aAction)=0;

	/**
 	 * Indication from network layer that a packet has been transmitted or received
	 *
 	 * @param aDirection Whether the packet was transmitted or received
 	 * @param aBytes The size of the packet
 	 * @param aResetTimer Whether the idle timer should be reset
 	 * @returns KErrNone if successful, otherwise one of the system-wide error codes
 	 * @since v7.0s
 	 */
 	virtual TInt PacketActivity(TDataTransferDirection aDirection, TUint aBytes, TBool aResetTimer)=0;

	/**
	 * Indication that the sent bytes threshold for a subconnection has been met or exceeded in the nif
	 *
	 * @param aSubConnectionUniqueId The subconnection to which this notification refers
	 * @param aUplinkVolume The total number of bytes sent by this subconnection
 	 * @since v7.0s
	 */
	virtual void NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume)=0;

	/**
	 * Indication that the received bytes threshold for a subconnection has been met or exceeded in the nif
	 *
	 * @param aSubConnectionUniqueId The subconnection to which this notification refers
	 * @param aDownlinkVolume The total number of bytes sent by this subconnection
 	 * @since v7.0s
	 */
	virtual void NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume)=0;

	/**
	 * Indication that some parameter relating to a subconnection has changed in the nif eg. subconnection closed, opened, QoS changed
	 *
	 * @param aEventType The type of event which has occured - used to direct the event notification to the appropriate component(s)
	 * @param aEvent The event which has occured
 	 * @param aEventData Any arguments or additional data to do with the event; the interpretation of this depends on the event
 	 * @param aSource An appropriate identifier of the source of the event - this will be used when one receiver is receiving events from many producers
 	 * @since v7.0s
	 * @todo In the future, migrate GUQoS event handler and agent/nif notifications to this generic event handler - Nifman can then act as an event distributor
	 */
	virtual void NifEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=0)=0;



	/**
	 * Read an integer field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt ReadInt( const TDesC& aField, TUint32& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadInt( const TDesC& aField, TUint32& aValue );
	/**
	 * Write an integer field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt WriteInt( const TDesC& aField, TUint32 aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteInt( const TDesC& aField, TUint32 aValue );

	/**
	 * Read a 8-bit descriptor field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt ReadDes( const TDesC& aField, TDes8& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadDes( const TDesC& aField, TDes8& aValue );

	/**
	 * Write an 8-bit descriptor field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt WriteDes( const TDesC& aField, const TDesC8& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteDes( const TDesC& aField, const TDesC8& aValue );

	/**
	 * Read a 16-bit descriptor field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt ReadDes( const TDesC& aField, TDes16& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadDes( const TDesC& aField, TDes16& aValue );

	/**
	 * Write an 16-bit descriptor field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt WriteDes( const TDesC& aField, const TDesC16& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteDes( const TDesC& aField, const TDesC16& aValue );

	/**
	 * Read an boolean field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt ReadBool( const TDesC& aField, TBool& aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt ReadBool( const TDesC& aField, TBool& aValue );

	/**
	 * Write an boolean field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	IMPORT_C TInt WriteBool( const TDesC& aField, TBool aValue, const RMessagePtr2* aMessage );
	IMPORT_C TInt WriteBool( const TDesC& aField, TBool aValue );

protected:
	/**
	 * Read an integer field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	virtual TInt DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage)=0;

	/**
	 * Write an integer field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
    virtual TInt DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage)=0;

	/**
	 * Read a 8-bit descriptor field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
    virtual TInt DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage)=0;

	/**
	 * Read a 16-bit descriptor field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
    virtual TInt DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage)=0;

	/**
	 * Write an 8-bit descriptor field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
    virtual TInt DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage)=0;

	/**
	 * Write an 16-bit descriptor field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	virtual TInt DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage)=0;

	/**
	 * Read an boolean field from the connection settings provider
	 * @param aField The name of the field to read
	 * @param aValue On return, contains the value of the field read
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
	virtual TInt DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage)=0;

	/**
	 * Write an boolean field to the connection settings provider
	 * @param aField The name of the field to which to write
	 * @param aValue The value to be written to the field
	 * @param aMessage for capability checking
	 * @returns KErrNone, if successful; otherwise one of the standard Symbian OS error codes
	 */
    virtual TInt DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage)=0;


	};

class CNifAgentRef;
class CNifIfLink;
class CNifIfBase : public CBase
/**
 * Base class for nif binder layers.
 * Classes derived from CNifIfBase act as a binder layer to the layer 3 protocol, and send and receive a specified protocol between the network layer and the link layer
 * @note There should be one CNifIfBase object per protocol, per nif instance
 *
 * @publishedPartner
 * @released
 * @since v5.0
 */
	{
friend class CNifMan;
public:
	IMPORT_C CNifIfBase(CNifIfFactory& aFactory);
	IMPORT_C CNifIfBase();
	IMPORT_C ~CNifIfBase();
	IMPORT_C static void Cleanup(TAny* aIf);
	IMPORT_C virtual void Open();
	IMPORT_C virtual void Close();
	IMPORT_C virtual void BindL(TAny *aId);
	IMPORT_C virtual TInt State();
	IMPORT_C virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource=0);

	/**
	 * Retrieve information about an network interface
	 * @param aInfo On return, contains information about the nif
	 */
	virtual void Info(TNifIfInfo& aInfo) const =0;

	/**
	 * Send data through this interface
	 * @param aPdu A packet contained within a RMBufChain
	 * @param aSource A pointer to the object that is sending this data
	 * @returns -ve number	Reserved for future use
	 *			0			Stop sending
	 *			+ve number	Continue sending
	 */
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0)=0;

	/**
	 * Notification of an event from the agent
	 * @param aEvent The type of event that occured
	 * @param aInfo Any data associated with the event
 	 * @returns KErrNone if successful, otherwise one of the system-wide error codes
	 * @note SC with previous releases broken in v6.1 when first argument changed type
	 * @since
	 */
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo)=0;

	inline MNifIfNotify* Notify() const;

protected:
	IMPORT_C CNifIfBase(CNifIfLink& aLink);
	TInt iRefCount;
	CNifIfFactory* iFactory;
	MNifIfNotify* iNotify;
	};

class CNifIfLink : public CNifIfBase
/**
 * Interface between nifman and the nif
 * This interface is used by nifman to perform operations on the link layer of the nif - the "reverse" operations (from nif to nifman) are performed on the MNifIfNotify interface
 * @note There should be one CNifIfLink object per nif instance
 * @note Typically the link layer object is CNifIfLink-derived
 *
 * @publishedPartner
 * @released
 * @since v5.0
 */
 	{
public:
	IMPORT_C CNifIfLink(CNifIfFactory& aFactory);

	IMPORT_C virtual void AuthenticateComplete(TInt aResult);

	/**
	 *
	 */
	virtual TInt Start()=0;

	/**
	 *
	 */
	virtual void Stop(TInt aReason, MNifIfNotify::TAction aAction)=0;

	/**
	 *
	 */
	virtual CNifIfBase* GetBinderL(const TDesC& aName)=0;

	IMPORT_C virtual void Restart(CNifIfBase* aIf);
	};

/**
 * Used to request MNifIfUser-derived object from layer 3 protocol
 * @sa TNifIfUser data structure
 *
 * @publishedPartner
 * @released
 * @since v5.0
 */
const TUint KNifOptGetNifIfUser = 1|KSocketInternalOptionBit;

/**
 * Connection option name for getting the pointer to the nifs implementation of the MNifIfExtendedManagementInterface
 *
 * @publishedPartner
 * @released
 * @since v7.0s
 */
const TUint KCOGetNifEMIPtr = 0x01 | KConnInternalOptionBit;

/**
 * Connection option name for getting the pointer to the agents implementation of the MNifAgentExtendedManagementInterface
 * @note This is currently defined here until a proper API for agents to start and hand off subconnections can be defined
 *
 * @publishedPartner
 * @released
 * @since v7.0s
 */
const TUint KCOGetAgentEMIPtr = 0x02 | KConnInternalOptionBit;

/**
 * @defgroup nifagtdlluids UIDs for agent and nif DLLs
 *
 * @publishedPartner
 * @released
 * @since v5.0
 */
//@{
const TInt KUidNifmanAgent     = 0x10000388;			//< 2nd UID for agent DLLs
														//< @deprecated Use Unicode version instead
const TInt KUidUnicodeNifmanAgent = 0x10003d39;			//< 2nd UID for agent DLLs
const TInt KUidNifmanInterface = 0x10000389;			//< 2nd UID for nif DLLs
														//< @deprecated Use Unicode version instead
const TInt KUidUnicodeNifmanInterface = 0x10003d3a;		//< 2nd UID for nif DLLs
//const TInt KUidEsockIniCategory  = 0x1000047A;		//< @removed in v6.0
//const TInt KUidUnicodeEsockIniCategory  = 0x10003d3b;	//< @removed in v6.0
//@}


class TSoIfConnectionInfo
/**
 * Class for communicating information about the connection selection to the esock service provider.
 * @note It is defined here in Nifman in order to be opaque to Esock.
 * @publishedPartner
 * @released
 * @since v7.0s
 */
	{
public:
	inline TSoIfConnectionInfo();
	inline TBool operator==(const TSoIfConnectionInfo& aInfo) const;
public:
	TUint32		iIAPId;			//< ID of selected IAP
	TUint32		iNetworkId;		//< ID of Network to which IAP belongs
	};


// Inlines

inline MNifIfNotify* CNifIfBase::Notify() const
/**
 * Accessor method for the iNotify pointer to the MNifIfNotify interface
 * @returns A pointer to the MNifIfNotify interface
 */
	{ return iNotify; }

inline TSoIfConnectionInfo::TSoIfConnectionInfo()
: iIAPId(0), iNetworkId(0)
	{}

inline TBool TSoIfConnectionInfo::operator==(const TSoIfConnectionInfo& aInfo) const
	{ return (iIAPId == aInfo.iIAPId && iNetworkId == aInfo.iNetworkId); };


#endif
