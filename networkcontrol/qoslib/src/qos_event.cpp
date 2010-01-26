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

#include "qoslib.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

// CQoSEventBase
/**
Constructor; sets the QoS event type.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aEventType QoS event type (values defined in enum TQoSEvent).
*/
EXPORT_C CQoSEventBase::CQoSEventBase(TInt aEventType) 
	: iEventType(aEventType)
	{
	}

/**
Gets the QoS event type.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return QoS event type (values defined in enum TQoSEvent).
*/
EXPORT_C TInt CQoSEventBase::EventType() const
	{
	return iEventType;
	}

/**
Constructor; sets the QoS parameters. 
 
The class takes a reference to the CQoSParameters argument so the caller must
keep that argument in scope whilst this class is being used.
 
@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aParameters QoS parameters. 
*/
EXPORT_C CQoSConfirmEvent::CQoSConfirmEvent(CQoSParameters& aParameters) 
	: CQoSEventBase(EQoSEventConfirm)
	{
	iParameters = &aParameters;
	}

// CQoSFailureEvent
/**
Constructor. Sets the QoS parameters and reason code.

The class takes a reference to the CQoSParameters argument, so the caller 
must keep that argument in scope whilst this class is being used.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aParameters QoS parameters.
@param aReason Reason code specifies the success or failure of the request.
*/
EXPORT_C CQoSFailureEvent::CQoSFailureEvent(CQoSParameters& aParameters, 
											TInt aReason) 
	: CQoSEventBase(EQoSEventFailure), iReason(aReason)
	{
	iParameters = &aParameters;
	}

/**
Gets the QoS parameters.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return QoS parameters.
*/
//lint -e{1763} thinks this indirectly modifies class -- wants return to be const?
EXPORT_C CQoSParameters* CQoSConfirmEvent::Parameters() const
	{
	return iParameters;
	}

/**
Gets the QoS parameters.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return QoS parameters.
*/
//lint -e{1763} thinks this indirectly modifies class -- wants return to be const?
EXPORT_C CQoSParameters* CQoSFailureEvent::Parameters() const
	{
	return iParameters;
	}

/**
Gets the reason code. 

Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSFailureEvent::Reason() const
	{
	return iReason;
	}

// CQoSAdaptEvent
/**
Constructor.

Sets the QoS parameters and reason code.

The class takes a reference to the CQoSParameters argument, so the caller 
must keep that argument in scope whilst this class is being used.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aParameters QoS parameters.
@param aReason Reason code specifies the success or failure of the request.
*/
EXPORT_C CQoSAdaptEvent::CQoSAdaptEvent(CQoSParameters& aParameters, 
										TInt aReason) 
	: CQoSEventBase(EQoSEventAdapt), iReason(aReason)
	{
	iParameters = &aParameters;
	}

/**
Gets the QoS parameters.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return QoS parameters.
*/
//lint -e{1763} thinks this indirectly modifies class -- wants return to be const?
EXPORT_C CQoSParameters* CQoSAdaptEvent::Parameters() const
	{
	return iParameters;
	}

/**
Gets the reason code. 
 
Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSAdaptEvent::Reason() const
	{
	return iReason;
	}

// CQoSChannelEvent
/**
Constructor.

Sets the QoS parameters and reason code.

The class takes a reference to the CQoSParameters argument so the caller 
must keep that argument in scope whilst this class is being used.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aParameters QoS parameters.
@param aReason Reason code specifies the success or failure of the request.
*/
EXPORT_C CQoSChannelEvent::CQoSChannelEvent(CQoSParameters* aParameters, 
											TInt aReason) 
	: CQoSEventBase(EQoSEventChannel), iReason(aReason)
	{
	iParameters = aParameters;
	}

/**
Gets the QoS parameters.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return QoS parameters.
*/
//lint -e{1763} thinks this indirectly modifies class -- wants return to be const?
EXPORT_C CQoSParameters* CQoSChannelEvent::Parameters() const
	{
	return iParameters;
	}

/**
Gets the reason code. 

Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code: KErrNone, if an existing channel was found; otherwise, 
an error code.
*/
EXPORT_C TInt CQoSChannelEvent::Reason() const
	{
	return iReason;
	}


// CQoSJoinEvent
/**
Constructor.
 
Sets the selector and reason code. Selector specifies the socket 
that was attached to the QoS channel.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aSelector Selector specifies the socket that was attached to the QoS 
channel.
@param aReason Reason code specifies the success or failure of the request.
*/
EXPORT_C CQoSJoinEvent::CQoSJoinEvent(const TQoSSelector& aSelector, 
									  TInt aReason) 
	: CQoSEventBase(EQoSEventJoin), iReason(aReason)
	{
	iSelector = aSelector;
	}

/**
Gets the selector that was attached to the QoS channel.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Selector that was attached to the QoS channel.
*/
EXPORT_C const TQoSSelector& CQoSJoinEvent::Selector() const
	{
	return iSelector;
	}

/**
Gets the reason code. 

Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSJoinEvent::Reason() const
	{
	return iReason;
	}

// CQoSLeaveEvent
/**
Constructor. 

Sets the selector and reason code. Selector specifies the socket 
that was detached from the QoS channel.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aSelector Selector specifies the socket that was detached from the QoS
channel.
@param aReason Reason code specifies the success or failure of the request.
*/
EXPORT_C CQoSLeaveEvent::CQoSLeaveEvent(const TQoSSelector& aSelector, 
										TInt aReason) 
	: CQoSEventBase(EQoSEventLeave), iReason(aReason)
	{
	iSelector = aSelector;
	}

/**
Gets the selector that was detached from the QoS channel.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Selector that was detached from the QoS channel.
*/
EXPORT_C const TQoSSelector& CQoSLeaveEvent::Selector() const
	{
	return iSelector;
	}

/**
Gets the reason code. 

Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSLeaveEvent::Reason() const
	{
	return iReason;
	}

// CQoSLoadEvent
/**
Constructor.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aEvent Sets the event type (EQoSEventLoadPolicyFile or 
EQoSEventUnloadPolicyFile).
@param aReason Reason code.
@param aFileName The name of the policy file.
*/
EXPORT_C CQoSLoadEvent::CQoSLoadEvent(const TQoSEvent& aEvent, TInt aReason, 
									  const TDesC& aFileName) 
	: CQoSEventBase(aEvent), iReason(aReason), iFileName(aFileName)
	{
	}

/**
Gets the name of the policy file. 

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return The name of the policy file.
*/
EXPORT_C const TFileName& CQoSLoadEvent::FileName() const
	{
	return iFileName;
	}

/**
Gets the reason code. 
 
Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSLoadEvent::Reason() const
	{
	return iReason;
	}


// CQoSAddEvent
/**
Constructor.

Sets the QoS parameters and reason code.

The class takes a reference to the CQoSParameters argument so the caller 
must keep that argument in scope whilst this class is being used.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aParameters QoS parameters.
@param aReason Reason code specifies the success or failure of the request.
*/
EXPORT_C CQoSAddEvent::CQoSAddEvent(CQoSParameters* aParameters, 
									TInt aReason) 
	: CQoSEventBase(EQoSEventAddPolicy), iReason(aReason)
	{
	iParameters = aParameters;
	}

/**
Gets the QoS parameters.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return QoS parameters.
*/
//lint -e{1763} thinks this indirectly modifies class -- wants return to be const?
EXPORT_C CQoSParameters* CQoSAddEvent::Parameters() const
	{
	return iParameters;
	}

/**
Gets the reason code. 

Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSAddEvent::Reason() const
	{
	return iReason;
	}


// CQoSGetEvent
/**
Constructor.
 
Sets the QoS parameters and reason code.

The class takes a reference to the CQoSParameters argument so the caller 
must keep that argument in scope whilst this class is being used.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aParameters QoS parameters.
@param aReason Reason code specifies the success or failure of the request.
*/
EXPORT_C CQoSGetEvent::CQoSGetEvent(CQoSParameters* aParameters, 
									TInt aReason) 
	: CQoSEventBase(EQoSEventGetPolicy), iReason(aReason)
	{
	iParameters = aParameters;
	}

/**
Gets the QoS parameters.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return QoS parameters.
*/
//lint -e{1763} thinks this indirectly modifies class -- wants return to be const?
EXPORT_C CQoSParameters* CQoSGetEvent::Parameters() const
	{
	return iParameters;
	}

/**
Gets the reason code. 
 
Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSGetEvent::Reason() const
	{
	return iReason;
	}

// CQoSDeleteEvent
/**
Constructor; sets the reason code indicating the success or failure of the 
request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aReason Reason code.
*/
EXPORT_C CQoSDeleteEvent::CQoSDeleteEvent(TInt aReason) :
  CQoSEventBase(EQoSEventDeletePolicy), iReason(aReason)
	{
	}


/**
Gets the reason code. 

Reason code specifies the success or failure of the request.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Reason code.
*/
EXPORT_C TInt CQoSDeleteEvent::Reason() const
	{
	return iReason;
	}

