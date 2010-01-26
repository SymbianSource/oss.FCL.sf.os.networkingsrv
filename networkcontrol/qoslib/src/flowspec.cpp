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
//

#include "qoslib.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

/* 
 * These methods are valid for Symbian OS 9.0 and onwards.
 */ 
/**
Default constructor.
 
@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
*/
EXPORT_C CQoSParameters::CQoSParameters()
    {
    iExtensionList.SetOffset(_FOFF(CExtensionBase, iLink));
    }

/**
Destructor.
  
Deletes all extensions.
 
@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
*/
EXPORT_C CQoSParameters::~CQoSParameters()
    {
    TQoSExtensionQueueIter iter(iExtensionList);
    CExtensionBase *extension;

    while ((extension = iter++) != NULL)
        {
        iExtensionList.Remove(*extension);
        delete extension;
        }
    }

/** 
Copies the parameters and extensions from aFlowSpec into this object.

Any existing extensions are deleted.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aFlowSpec Contains the CQoSParameters object that is copied into this 
object.
@leave If there is no memory available for extensions. 
*/
EXPORT_C void CQoSParameters::CopyL(CQoSParameters& aFlowSpec)
    {
    TQoSExtensionQueueIter iter_old(iExtensionList);
    TQoSExtensionQueueIter iter(aFlowSpec.Extensions());
    CExtensionBase *ext;

    // Delete old extensions
    while ((ext = iter_old++) != NULL)
        {
        iExtensionList.Remove(*ext);
        delete ext;
        }

    // Add new extensions
    while ((ext = iter++) != NULL)
        {
        AddExtensionL(*ext);
        }
    iQoS = aFlowSpec.iQoS;
    }

// Keep private copy of extensions
/**
Copies the extension into the list of extensions. 

If there already exists an extension with the same type, it is replaced.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aExtension The extension that is added to the list of extenstions.
@leave If there is no memory available for the extension.
@return KErrNone, always. 
*/
EXPORT_C TInt CQoSParameters::AddExtensionL(CExtensionBase& aExtension)
    {
    CExtensionBase *ext = FindExtension(aExtension.Type());
    if (ext)
        {
        ext->Copy(aExtension);
        }
    else
        {
        ext = aExtension.CreateL();
        ext->Copy(aExtension);
        iExtensionList.AddLast(*ext);
        }
    return KErrNone;
    }


/**
Removes the extension with the specified extension type from the list of 
extensions.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aType The type of extension to be removed from the list of extensions.
@return KErrNone, if the extension is found; otherwise, KErrNotFound. 
*/
EXPORT_C TInt CQoSParameters::RemoveExtension(TInt aType)
    {
    CExtensionBase *extension = FindExtension(aType);
    if (!extension)
        {
        return KErrNotFound;
        }
    iExtensionList.Remove(*extension);
    delete extension;
    return KErrNone;
    }

/**
Gets an extension with the specified extension type.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aType The type of extension that is to be searched for from the list 
of extensions.
@return A pointer to an extension if the extension is found, otherwise NULL. 
*/
EXPORT_C CExtensionBase* CQoSParameters::FindExtension(TInt aType)
    {
    TQoSExtensionQueueIter iter(iExtensionList);
    CExtensionBase *ext;

    while ((ext = iter++) != NULL) 
        {
        if (ext->Type() == aType)
            {
            return ext;
            }
        }
    return NULL;
    }

/**
Gets a list of extensions that have been added to this object.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return A list of extensions. 
*/
EXPORT_C TQoSExtensionQueue& CQoSParameters::Extensions()
    {
    return iExtensionList;
    }

/**
Gets the bandwidth for uplink direction. 
 
Bandwidth defines the requested transfer rate that the application requests.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current bandwidth for uplink direction (in bytes/second). 
*/
EXPORT_C TInt CQoSParameters::GetUplinkBandwidth() const
    {
    return iQoS.GetUplinkBandwidth();
    }

/**
Gets the maximum burst size for uplink direction. 
 
Maximum burst size defines the burst size that the application might send.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current maximum burst size for uplink direction (in bytes). 
*/
EXPORT_C TInt CQoSParameters::GetUpLinkMaximumBurstSize() const
    {
    return iQoS.GetUpLinkMaximumBurstSize();
    }

/**
Gets the maximum packet size for uplink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current maximum packet size for uplink direction (in bytes). 
*/
EXPORT_C TInt CQoSParameters::GetUpLinkMaximumPacketSize() const
    {
    return iQoS.GetUpLinkMaximumPacketSize();
    }

/**
Gets the average packet size for uplink direction. 
 
Average packet size is used when doing traffic policing and estimating 
effect of protocol headers.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current average packet size for uplink direction (in bytes). 
*/
EXPORT_C TInt CQoSParameters::GetUpLinkAveragePacketSize() const
    {
    return iQoS.GetUpLinkAveragePacketSize();
    }

/**
Gets the delay for uplink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Currrent delay for uplink direction (in milliseconds). 
*/
EXPORT_C TInt CQoSParameters::GetUpLinkDelay() const
    {
    return iQoS.GetUpLinkDelay();
    }

/**
Gets the priority for uplink direction. 
 
Priority can be used to prioritise between traffic flows inside the terminal.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current priority for uplink direction: 0 is the highest priority, 
7 is lowest. 
*/
EXPORT_C TInt CQoSParameters::GetUpLinkPriority() const
    {
    return iQoS.GetUpLinkPriority();
    }

/**
Gets the bandwidth for downlink direction. 
 
Bandwidth defines the requested transfer rate that the application requests.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current bandwidth for downlink direction (in bytes/second). 
*/
EXPORT_C TInt CQoSParameters::GetDownlinkBandwidth() const
    {
    return iQoS.GetDownlinkBandwidth();
    }

/**
Gets the maximum burst size for downlink direction. 
 
Maximum burst size defines the burst size that the application might send.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current maximum burst size for downlink direction (in bytes). 
*/
EXPORT_C TInt CQoSParameters::GetDownLinkMaximumBurstSize() const
    {
    return iQoS.GetDownLinkMaximumBurstSize();
    }

/**
Gets the maximum packet size for downlink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current maximum packet size for downlink direction (in bytes). 
*/
EXPORT_C TInt CQoSParameters::GetDownLinkMaximumPacketSize() const
    {
    return iQoS.GetDownLinkMaximumPacketSize();
    }

/**
Gets the average packet size for downlink direction. 
 
Average packet size is used when doing traffic policing and estimating 
effect of protocol headers.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current average packet size for downlink direction (in bytes). 
*/
EXPORT_C TInt CQoSParameters::GetDownLinkAveragePacketSize() const
    {
    return iQoS.GetDownLinkAveragePacketSize();
    }

/**
Gets the delay for downlink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Currrent delay for downlink direction (in milliseconds). 
*/
EXPORT_C TInt CQoSParameters::GetDownLinkDelay() const
    {
    return iQoS.GetDownLinkDelay();
    }

/**
Gets the priority for downlink direction. 
 
Priority can be used to prioritise between traffic flows inside the terminal.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current priority for downlink direction: 0 is the highest priority, 
7 is lowest. 
*/
EXPORT_C TInt CQoSParameters::GetDownLinkPriority() const
    {
    return iQoS.GetDownLinkPriority();
    }

/**
Sets the bandwidth for uplink direction. 
 
Bandwidth defines the requested transfer rate that the application requests.
Bandwidth must be > 0.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aBandwidth Value (in bytes/second) to which to set the bandwidth for 
uplink direction. 
*/
EXPORT_C void CQoSParameters::SetUplinkBandwidth(TInt aBandwidth)
    {
    iQoS.SetUplinkBandwidth(aBandwidth);
    }

/**
Sets the maximum burst size for uplink direction. 
 
Maximum burst size defines the burst size that the application might send. 
Maximum burst size must be > 0.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aSize Value (in bytes) to which to set the maximum burst size for 
uplink direction. 
*/
EXPORT_C void CQoSParameters::SetUpLinkMaximumBurstSize(TInt aSize)
    {
    iQoS.SetUpLinkMaximumBurstSize(aSize);
    }

/**
Sets the maximum packet size for uplink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aMaxSize Value (in bytes) to which to set the maximum packet size for 
uplink direction. 
*/
EXPORT_C void CQoSParameters::SetUpLinkMaximumPacketSize(TInt aMaxSize)
    {
    iQoS.SetUpLinkMaximumPacketSize(aMaxSize);
    }

/**
Sets the average packet size for uplink direction. 
 
Average packet size is used when doing traffic policing and estimating 
effect of protocol headers. The average packet size must be <= maximum 
packet size.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aSize Value (in bytes) to which to set the average packet size 
for uplink direction. 
*/
EXPORT_C void CQoSParameters::SetUpLinkAveragePacketSize(TInt aSize)
    {
    iQoS.SetUpLinkAveragePacketSize(aSize);
    }

/**
Sets the requested delay for uplink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aDelay Value (in milliseconds) to which to set the delay for uplink 
direction. 
*/
EXPORT_C void CQoSParameters::SetUpLinkDelay(TInt aDelay)
    {
    iQoS.SetUpLinkDelay(aDelay);
    }

/**
Sets the priority for uplink direction. 
 
Priority can be used to prioritise between traffic flows inside the terminal.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aPriority Value (0-7) to which to set the priority for uplink 
direction: 0 is the highest priority, 7 is lowest. 
*/
EXPORT_C void CQoSParameters::SetUpLinkPriority(TInt aPriority)
    {
    iQoS.SetUpLinkPriority(aPriority);
    }

/**
Sets the bandwidth for downlink direction. 
 
Bandwidth defines the requested transfer rate that the application requests.
Bandwidth must be > 0.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aBandwidth Value (in bytes/second) to which to set the bandwidth for 
downlink direction. 
*/
EXPORT_C void CQoSParameters::SetDownlinkBandwidth(TInt aBandwidth)
    {
    iQoS.SetDownlinkBandwidth(aBandwidth);
    }

/**
Sets the maximum burst size for downlink direction. 
 
Maximum burst size defines the burst size that the application might send. 
Maximum burst size must be > 0.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aSize Value (in bytes) to which to set the maximum burst size for 
downlink direction. 
*/
EXPORT_C void CQoSParameters::SetDownLinkMaximumBurstSize(TInt aSize)
    {
    iQoS.SetDownLinkMaximumBurstSize(aSize);
    }

/**
Sets the maximum packet size for downlink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aMaxSize Value (in bytes) to which to set the maximum packet size for 
downlink direction. 
*/
EXPORT_C void CQoSParameters::SetDownLinkMaximumPacketSize(TInt aMaxSize)
    {
    iQoS.SetDownLinkMaximumPacketSize(aMaxSize);
    }

/**
Sets the average packet size for downlink direction. 
 
Average packet size is used when doing traffic policing and estimating 
effect of protocol headers. The average packet size must be <= maximum 
packet size.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aSize Value (in bytes) to which to set the average packet size 
for downlink direction. 
*/
EXPORT_C void CQoSParameters::SetDownLinkAveragePacketSize(TInt aSize)
    {
    iQoS.SetDownLinkAveragePacketSize(aSize);
    }

/**
Sets the requested delay for downlink direction.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aDelay Value (in milliseconds) to which to set the delay for downlink 
direction. 
*/
EXPORT_C void CQoSParameters::SetDownLinkDelay(TInt aDelay)
    {
    iQoS.SetDownLinkDelay(aDelay);
    }

/**
Sets the priority for downlink direction. 
 
Priority can be used to prioritise between traffic flows inside the terminal.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aPriority Value (0-7) to which to set the priority for downlink 
direction: 0 is the highest priority, 7 is lowest. 
*/
EXPORT_C void CQoSParameters::SetDownLinkPriority(TInt aPriority)
    {
    iQoS.SetDownLinkPriority(aPriority);
    }

/**
Gets the name of the flowspec.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current name of the flowspec. 
*/
EXPORT_C const TName& CQoSParameters::GetName() const
    {
    return iQoS.GetName();
    }

/**
Gets the adaptation mode.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current value for adapt mode. 
*/
EXPORT_C TBool CQoSParameters::AdaptMode() const
    {
    return iQoS.AdaptMode();
    }

/**
Returns the headers included mode.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@return Current value for header included mode. 
*/
EXPORT_C TBool CQoSParameters::GetHeaderMode() const
    {
    return iQoS.GetHeaderMode();
    }

/** 
Sets the name of the flowspec. 
 
This allows application to specify a user friendly name for a flowspec.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aName The name for the flowspec. Maximum length for the name is 
KMaxName.
*/
EXPORT_C void CQoSParameters::SetName(const TName& aName)
    {
    // Member variable iName was taken into use temporarily
    iName.Copy(aName);
    iQoS.SetName(aName);
    }

/**
Sets the adaptation mode. 
 
If the application is willing to accept lower QoS than requested, 
it should set the adapt mode on. By default adapt mode is not set.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aCanAdapt The value for adapt mode. 
*/
EXPORT_C void CQoSParameters::SetAdaptMode(TBool aCanAdapt)
    {
    iQoS.SetAdaptMode(aCanAdapt);
    }

/**
Sets the header mode. If the application wants QoS Framework to include 
network and transport layer header values in bitrate calculations or not. 
By default the mode is set to include the headers in the calculations. 
Protocols such as RTSP or SIP, which already include the header sizes in 
their calculations will want to alter the mode so that header values are 
not included in calculations twice.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
@param aHeadersIncluded The value for header mode.
*/
EXPORT_C void CQoSParameters::SetHeaderMode(TBool aHeadersIncluded)
    {
    iQoS.SetHeaderMode(aHeadersIncluded);
    }

EXPORT_C TInt CQoSParameters::GetDownLinkDelayVariation() const
    {
    return KErrNotSupported;
    }


EXPORT_C TInt CQoSParameters::GetUpLinkDelayVariation() const
    {
    return KErrNotSupported;
    }


EXPORT_C void CQoSParameters::SetDownLinkDelayVariation(TInt /* aVariation */)
    {
    // This method is not yet supported
    }


EXPORT_C void CQoSParameters::SetUpLinkDelayVariation(TInt /* aVariation */)
    {
    // This method is not yet supported
    }


