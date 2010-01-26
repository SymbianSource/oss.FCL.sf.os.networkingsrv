
// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// qoslib.h - This file defines the generic QoS API interface.
//

#ifndef __QOSLIB_H__
#define __QOSLIB_H__

/**
 * @file qoslib.h 
 * 
 * QoS API - This file defines the generic QoS API interface.
 */

#include <e32std.h>
#include <in_sock.h>

#include <networking/qos_extension.h>
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

class CQoSParameters;
class CPolicy;

/**
 * Selector for QoS policies. The variables defined in TQoSSelector are used to find QoS policies
 * for traffic flows. Not all variables have to be set, the application can specify the variables that it wants to.
 * Note: TQoSSelector stores IPv4 addresses in "IPv4-mapped IPv6 address" format.
 *
 * @publishedPartner
 * @released
 */
class TQoSSelector
{
public:
    /**
     * Constructor. Initialises all variables to unspecified values.
     */
    IMPORT_C TQoSSelector();

    /**
     * Compares if the selectors are equal.
     *
     * @param aSelector TQoSSelector object that is compared with this object.
     * @return ETrue if selectors are equal to this object, otherwise EFalse.
     */
    IMPORT_C TInt operator==(const TQoSSelector& aSelector) const;

    /**
     * Compares if the selector matches aSocket. 
     *
     * @param aSocket RSocket that is compared with this object.
     * @return ETrue if selector created from aSocket is equal to this object, otherwise EFalse.
     */
    IMPORT_C TBool Match(RSocket& aSocket) const;

    /**
     * Sets the addresses for selector.
     *
     * @param aSrcAddr Source address. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @param aSrcAddrMask Source address mask. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @param aDstAddr Destination address. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @param aDstAddrMask Destination address mask. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @param aProtocol Protocol id.
     * @param aSrcPortMax Maximum source port. Must have value <= 65535 (0 is used as uspecified value for a port).
     * @param aDstPortMax Maximum destination port. Must have value <= 65535 (0 is used as uspecified value for a port).
     * @return KErrNone if parameters have valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetAddr(const TInetAddr& aSrcAddr, const TInetAddr& aSrcAddrMask, const TInetAddr& aDstAddr, const TInetAddr& aDstAddrMask, TUint aProtocol, TUint aSrcPortMax, TUint aDstPortMax);

    /**
     * Sets the addresses for selector. RSocket is used to fetch addresses and ports for a selector. The resulting
     * selector will match only for one socket. NOTE: RSocket::Connect() must be called before calling this method.
     *
     * @param aSocket RSocket object that is used to set the selector variables. Note: RSocket must be connected
     * @return KErrNone if parameters have valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetAddr(RSocket& aSocket);

    /**
     * Sets the source address for selector.
     *
     * @param aAddr Source address. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @return KErrNone if parameters have valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetSrc(const TInetAddr& aAddr);

    /**
     * Sets the destination address for selector.
     *
     * @param aAddr Source address. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @return KErrNone if parameters have valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetDst(const TInetAddr& aAddr);

    /**
     * Returns the current source address.
     *
     * @return Source address.
     */
    IMPORT_C TInetAddr GetSrc() const;

    /**
     * Returns the current destination address.
     *
     * @return Destination address.
     */
    IMPORT_C TInetAddr GetDst() const;

    /**
     * Sets the source address mask.
     *
     * @param aMask Source address mask. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @return KErrNone if parameters have valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetSrcMask(const TInetAddr& aMask);

    /**
     * Returns the current source address mask.
     *
     * @return Source address mask.
     */
    IMPORT_C TInetAddr GetSrcMask() const;

    /**
     * Sets the destination address mask.
     *
     * @param aMask Destination address mask. Port must have value <= 65535 (0 is used as uspecified value for a port).
     * @return KErrNone if parameters have valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetDstMask(const TInetAddr& aMask);

    /**
     * Returns the current destination address mask.
     *
     * @return Destination address mask.
     */
    IMPORT_C TInetAddr GetDstMask() const;

    /**
     * Sets the Internet access point identifier. 0 is used as unspecified value.
     *
     * @param aIapId Value to which to set the IapId.
     */
    IMPORT_C void SetIapId(TInt aIapId);

    /**
     * Returns the current Internet access point identifier.
     *
     * @return Internet access point identifier.
     */
    IMPORT_C TInt IapId() const;

    /**
     * Sets the protocol identifier. 0 is used as unspecified value.
     *
     * @param aProtocol Value to which to set the protocol id.
     */
    IMPORT_C void SetProtocol(TUint aProtocol);

    /**
     * Returns the current protocol identifier.
     *
     * @return Protocol identifier.
     */
    IMPORT_C TUint Protocol() const;

    /**
     * Sets the maximum source port. Port must have value <= 65535 (0 is used as unspecified value).
     *
     * @param aMaxPort Value to which to set maximum source port.
     * @return KErrNone if aMaxPort has valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetMaxPortSrc(TUint aMaxPort);

    /**
     * Sets the maximum destination port. Port must have value <= 65535 (0 is used as unspecified value).
     *
     * @param aMaxPort Value to which to set maximum destination port.
     * @return KErrNone if aMaxPort has valid values, otherwise KErrArgument.
     */
    IMPORT_C TInt SetMaxPortDst(TUint aMaxPort);

    /**
     * Returns the maximum source port.
     *
     * @return Maximum source port.
     */
    IMPORT_C TUint MaxPortSrc() const;

    /**
     * Returns the maximum destination port.
     *
     * @return Maximum destination port.
     */
    IMPORT_C TUint MaxPortDst() const;

private:
    /** The source address. */
    TInetAddr   iSrc;
    /** The destination address. */
    TInetAddr   iDst;
    /** The source address mask. */
    TInetAddr   iSrcMask;
    /** The destination address mask. */
    TInetAddr   iDstMask;
    /** The protocol ID. */
    TUint       iProtocol;
    /** The maximum source port. */
    TUint       iSrcPortMax;
    /** The maximum destination port. */
    TUint       iDstPortMax;
    /** The Internet access point identifier. */
    TUint       iIapId;
};

/**
* @name QoS capabilities. 
* 
* These are used to notify the application as to the type of QoS support provided.
* @publishedPartner
* @released
*/
//@{
/** End-to-end signalling is used, e.g. RSVP. */
const TUint KQoSSignaled            = 0x0001;
/** Signalling is used along some part of end-to-end path. */
const TUint KQoSPartiallySignaled   = 0x0002;
/** No signalling is used, e.g. DiffServ. */
const TUint KQoSProvisioned         = 0x0004;
//@}

/**
 * QoS event types
 * @publishedPartner
 * @released
 */
enum TQoSEvent
    {
    /** QoS request failed. */
    EQoSEventFailure=1,
    /** QoS request was successful. */
    EQoSEventConfirm,
    /** QoS parameters have changed. */
    EQoSEventAdapt,
    /** QoS channel is opened. */
    EQoSEventChannel,
    /** A socket is attached to a QoS channel. */
    EQoSEventJoin,
    /** A socket is detached from a QoS channel. */
    EQoSEventLeave,
    /** QoS policy was added to QoS policy database. */
    EQoSEventAddPolicy,
    /** QoS policy was searched from the QoS policy database. */
    EQoSEventGetPolicy,
    /** QoS policy was deleted. */
    EQoSEventDeletePolicy,
    /** QoS policy file was loaded. */
    EQoSEventLoadPolicyFile,
    /** QoS policy file was unloaded. */
    EQoSEventUnloadPolicyFile
    };

/**
 * Default mask for QoS events: all events are notified to the application.
 * @publishedPartner
 * @released
 */
const TUint KQoSEventAll = EQoSEventFailure | EQoSEventConfirm | EQoSEventAdapt | EQoSEventChannel | EQoSEventJoin | EQoSEventLeave | EQoSEventAddPolicy | EQoSEventGetPolicy | EQoSEventDeletePolicy | EQoSEventLoadPolicyFile | EQoSEventUnloadPolicyFile;

/**
 * Base class for QoS events.
 *
 * @publishedPartner
 * @released
 */
class CQoSEventBase : public CBase
{
public:
    /**
     * Constructor. Sets the QoS event type.
     *
     * @param aEventType QoS event type (values defined in enum TQoSEvent).
     */
    IMPORT_C CQoSEventBase(TInt aEventType);

    /**
     * Returns the QoS event type.
     *
     * @return QoS event type (values defined in enum TQoSEvent).
     */
    IMPORT_C TInt EventType() const;

private:
    /** The QoS event type. */
    TInt iEventType;
};

/**
 * Class for EQoSEventConfirm event whereby a QoS request was successful
 * 
 * @publishedPartner
 * @released
 */
class CQoSConfirmEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the QoS parameters.
     *
     * @param aParameters QoS parameters.
     */
    IMPORT_C CQoSConfirmEvent(CQoSParameters& aParameters);

    /**
     * Returns the QoS parameters.
     *
     * @return QoS parameters.
     */
    IMPORT_C CQoSParameters* Parameters() const;

private:
    /** The address of the CQoSParameters parameter supplied during construction. */
    CQoSParameters* iParameters; 
};

/**
 * Class for EQoSEventFailure event whereby a QoS request failed
 *
 * @publishedPartner
 * @released
 */
class CQoSFailureEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the QoS parameters and reason code.
     *
     * @param aParameters QoS parameters.
     * @param aReason Reason code specifies the success or failure of the request.
     */
    IMPORT_C CQoSFailureEvent(CQoSParameters& aParameters, TInt aReason);

    /**
     * Returns the QoS parameters.
     *
     * @return QoS parameters.
     */
    IMPORT_C CQoSParameters* Parameters() const;

    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The address of the CQoSParameters parameter supplied during construction. */
    CQoSParameters* iParameters;
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventAdapt event whereby the QoS parameters have changed
 *
 * @publishedPartner
 * @released
 */
class CQoSAdaptEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the QoS parameters and reason code.
     *
     * @param aParameters QoS parameters.
     * @param aReason Reason code specifies the success or failure of the request.
     */
    IMPORT_C CQoSAdaptEvent(CQoSParameters& aParameters, TInt aReason);

    /**
     * Returns the QoS parameters.
     *
     * @return QoS parameters.
     */
    IMPORT_C CQoSParameters* Parameters() const;

    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The address of the CQoSParameters parameter supplied during construction. */
    CQoSParameters* iParameters;
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventChannel event whereby a QoS channel is opened
 *
 * @publishedPartner
 * @released
 */
class CQoSChannelEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the QoS parameters and reason code.
     *
     * @param aParameters QoS parameters.
     * @param aReason Reason code specifies the success or failure of the request.
     */
    IMPORT_C CQoSChannelEvent(CQoSParameters* aParameters, TInt aReason);

    /**
     * Returns the QoS parameters.
     *
     * @return QoS parameters.
     */
    IMPORT_C CQoSParameters* Parameters() const;

    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code. KErrNone if an existing channel was found, otherwise an error code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The address of the CQoSParameters parameter supplied during construction. */
    CQoSParameters* iParameters;
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventJoin event whereby a socket is attached to a QoS channel
 *
 * @publishedPartner
 * @released
 */
class CQoSJoinEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the selector and reason code. Selector specifies the socket 
     * that was attached to the QoS channel.
     *
     * @param aSelector Selector specifies the socket that was attached to the QoS channel.
     * @param aReason Reason code specifies the success or failure of the request.
     */
    IMPORT_C CQoSJoinEvent(const TQoSSelector& aSelector, TInt aReason);

    /**
     * Returns the selector that was attached to the QoS channel.
     *
     * @return Selector that was attached to the QoS channel.
     */
    IMPORT_C const TQoSSelector& Selector() const;
    
    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The selector that specifies the socket that was attached to the QoS channel. */
    TQoSSelector iSelector;
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventLeave event whereby a socket is detached from a QoS channel
 *
 * @publishedPartner
 * @released
 */
class CQoSLeaveEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the selector and reason code. Selector specifies the socket 
     * that was detached from the QoS channel.
     *
     * @param aSelector Selector specifies the socket that was detached from the QoS channel.
     * @param aReason Reason code specifies the success or failure of the request.
     */
    IMPORT_C CQoSLeaveEvent(const TQoSSelector& aSelector, TInt aReason);

    /**
     * Returns the selector that was detached from the QoS channel.
     *
     * @return Selector that was detached from the QoS channel.
     */
    IMPORT_C const TQoSSelector& Selector() const;

    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The QoS channel selector. */
    TQoSSelector iSelector;
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventAddPolicy event whereby a QoS policy was added to QoS policy database
 *
 * @publishedPartner
 * @released
 */
class CQoSAddEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the QoS parameters and reason code.
     *
     * @param aParameters QoS parameters.
     * @param aReason Reason code specifies the success or failure of the request.
     */
    IMPORT_C CQoSAddEvent(CQoSParameters* aParameters, TInt aReason);

    /**
     * Returns the QoS parameters.
     *
     * @return QoS parameters.
     */
    IMPORT_C CQoSParameters* Parameters() const;

    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The address of the CQoSParameters parameter supplied during construction. */
    CQoSParameters* iParameters;
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventGetPolicy event whereby a QoS policy was searched from the 
 * QoS policy database
 *
 * @publishedPartner
 * @released
 */
class CQoSGetEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the QoS parameters and reason code.
     *
     * @param aParameters QoS parameters.
     * @param aReason Reason code specifies the success or failure of the request.
     */
    IMPORT_C CQoSGetEvent(CQoSParameters* aParameters, TInt aReason);

    /**
     * Returns the QoS parameters.
     *
     * @return QoS parameters.
     */
    IMPORT_C CQoSParameters* Parameters() const;
    
    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The address of the CQoSParameters parameter supplied during construction. */
    CQoSParameters* iParameters;
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventDeletePolicy event whereby a QoS policy was deleted
 *
 * @publishedPartner
 * @released
 */
class CQoSDeleteEvent : public CQoSEventBase
{
public:
    /**
     * Constructor. Sets the reason code indicating the success or failure of the request.
     *
     * @param aReason Reason code.
     */
    IMPORT_C CQoSDeleteEvent(TInt aReason);

    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The reason code specifies the success or failure of the request. */
    TInt iReason;
};

/**
 * Class for EQoSEventLoadPolicyFile and EQoSEventUnloadPolicyFile events whereby 
 * policy files are loaded or unloaded
 *
 * @publishedPartner
 * @released
 */
class CQoSLoadEvent : public CQoSEventBase
{
public:
    /**
     * Constructor.
     *
     * @param aEvent Sets the event type (EQoSEventLoadPolicyFile or EQoSEventUnloadPolicyFile).
     * @param aReason Reason code.
     * @param aFileName The name of the policy file.
     */
    IMPORT_C CQoSLoadEvent(const TQoSEvent& aEvent, TInt aReason, const TDesC& aFileName);

    /**
     * Returns the name of the policy file. 
     *
     * @return The name of the policy file.
     */
    IMPORT_C const TFileName& FileName() const;

    /**
     * Returns the reason code. Reason code specifies the success or failure of the request.
     *
     * @return Reason code.
     */
    IMPORT_C TInt Reason() const;

private:
    /** The reason code that specifies the success or failure of the request. */
    TInt iReason;
    /** The name of the QoS policy file. */
    TFileName iFileName;
};

/**
 * An interface to catch QoS events.
 *
 * @publishedPartner
 * @released
 */
class MQoSObserver
{
public:
    /**
    This method is called by the QoS API when a QoS event occurs.
    
    @publishedPartner
    @released
    @capability NetworkServices Restrict QoS operations in similar way as 
    normal socket operations.
    @param aQoSEvent QoS event class.
    */ 
    virtual void Event(const CQoSEventBase& aQoSEvent)=0;
};

/**
 * An interface to manage QoS policies and QoS policy files.  RQoSPolicy::Open must always be called before 
 * any other methods in the RQoSPolicy can be called. Note that only one request can be pending at once 
 * (QoS event should be received before issuing next request).
 *
 * @publishedPartner
 * @released
 */
class RQoSPolicy
{
public:
    IMPORT_C RQoSPolicy();
    IMPORT_C ~RQoSPolicy();

    /**
     * RQoSPolicy::Open must always be called before any other method can be used.
     * Specifies the selector for a QoS policy.
     *
     * @param aSelector Selector for the QoS policy.
     * @return KErrNone if everything is ok, != KErrNone is the QoS channel cannot be opened.
     */
    IMPORT_C TInt Open(const TQoSSelector& aSelector);

    /**
     * Sets the QoS parameters for the QoS policy. CQoSAddEvent event is received
     * asynchronously to indicate the success of failure of the request.
     *
     * @param aPolicy QoS parameters.
     * @return KErrNone if request was issued, != KErrNone if there was an error. 
     */
    IMPORT_C TInt SetQoS(CQoSParameters& aPolicy);

    /**
     * Gets the QoS policy from QoS policy database. CQoSGetEvent event is received
     * asynchronously to indicate the success of failure of the request.
     *
     * @return KErrNone if request was issued, != KErrNone if there was an error. 
     */
    IMPORT_C TInt GetQoS();

    /**
     * Deletes the QoS policy.
     * @return KErrNone if request was issued, != KErrNone if there was an error (for example, 
     * RQoSPolicy::Open was not called).
     */
    IMPORT_C TInt Close();

    /**
     * Registers an event observer to catch QoS events.
     *
     * @param aObserver Event observer.
     * @param aMask An event mask. An application can specify a set of QoS events that it wants to receive.
     * By default all events are notified to the application.
     * @return KErrNone if request was issued, != KErrNone if there was an error. 
     */
    IMPORT_C TInt NotifyEvent(MQoSObserver& aObserver, TUint aMask=KQoSEventAll);

    /**
     * Deregisters an event observer to catch QoS events.
     *
     * @param aObserver Event observer.
     * @return KErrNone if request was issued, != KErrNone if there was an error. 
     */
    IMPORT_C TInt CancelNotifyEvent(MQoSObserver& aObserver);

    /**
     * Loads a QoS policy file into the QoS policy database. EQoSEventLoadPolicyFile event is received
     * asynchronously to indicate the success of failure of the request.
     *
     * @param aName Name of the QoS policy file to be loaded.
     * @return KErrNone if request was issued, != KErrNone if there was an error. 
     */
    IMPORT_C TInt LoadPolicyFile(const TDesC& aName);

    /**
     * Unloads a QoS policy file from the QoS policy database. EQoSEventUnloadPolicyFile event is received
     * asynchronously to indicate the success of failure of the request.
     *
     * @param aName Name of the QoS policy file to be unloaded.
     * @return KErrNone if request was issued, != KErrNone if there was an error. 
     */
    IMPORT_C TInt UnloadPolicyFile(const TDesC& aName);

private:
    /** A dynamically allocated QoS policy. */
    CPolicy* iPolicy;// //< Used internally by qoslib
};

#endif
