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

#ifndef __QOSPARAMETERS_H__
#define __QOSPARAMETERS_H__


#include <networking/pfqos.h>

/** States the highest allowable priority for Interactive class traffic. */
const TInt KQoSHighestPriority = 0;
/** States the lowest allowable priority for Interactive class traffic. */
const TInt KQoSLowestPriority = 7;

class CExtension;

// QoS policy options
/** Flag to indicate that the application is able to adapt. */
const TUint KPfqosOptionCanAdapt        = 1; // Application is able to adapt
// Network and Transport layer hdrs are included in bitrate values.
const TUint KPfqosOptionHeadersIncluded = 2;    

class TQoSParameters
/** Holds information on the parameters that define Quality of Service (QoS).
 *
 * @internalTechnology
 */
    {
    public:
    IMPORT_C TQoSParameters();
    IMPORT_C TInt operator==(const TQoSParameters& aFlowSpec) const;
    IMPORT_C TQoSParameters &operator=(const TQoSParameters& aSpec);

    inline TInt GetUplinkBandwidth() const;
    inline TInt GetUpLinkMaximumBurstSize() const;
    inline TInt GetUpLinkMaximumPacketSize() const;
    inline TInt GetUpLinkAveragePacketSize() const;
    inline TInt GetUpLinkDelay() const;
    inline TInt GetUpLinkPriority() const;
    
    inline TInt GetDownlinkBandwidth() const;
    inline TInt GetDownLinkMaximumBurstSize() const;
    inline TInt GetDownLinkMaximumPacketSize() const;
    inline TInt GetDownLinkAveragePacketSize() const;
    inline TInt GetDownLinkDelay() const;
    inline TInt GetDownLinkPriority() const;
    
    IMPORT_C void SetUplinkBandwidth(TInt aBandwidth);
    IMPORT_C void SetUpLinkMaximumBurstSize(TInt aSize);
    IMPORT_C void SetUpLinkMaximumPacketSize(TInt aMaxSize);
    IMPORT_C void SetUpLinkAveragePacketSize(TInt aSize);
    IMPORT_C void SetUpLinkDelay(TInt aDelay);
    IMPORT_C void SetUpLinkPriority(TInt aPriority);
    
    IMPORT_C void SetDownlinkBandwidth(TInt aBandwidth);
    IMPORT_C void SetDownLinkMaximumBurstSize(TInt aSize);
    IMPORT_C void SetDownLinkMaximumPacketSize(TInt aMaxSize);
    IMPORT_C void SetDownLinkAveragePacketSize(TInt aSize);
    IMPORT_C void SetDownLinkDelay(TInt aDelay);
    IMPORT_C void SetDownLinkPriority(TInt aPriority);
    
    inline void SetFlags(TUint aFlags);
    inline TUint Flags() const;
    inline const TName& GetName() const;
    IMPORT_C TBool AdaptMode() const;
    IMPORT_C TBool GetHeaderMode() const;

    IMPORT_C TInt SetName(const TName& aName);
    IMPORT_C void SetAdaptMode(TBool aCanAdapt);

    IMPORT_C void SetHeaderMode(TBool aHeadersIncluded);

    IMPORT_C void Copy(CExtension& aExtension);

    private:
    /** The token rate for the uplink. */    
    TInt iUplinkBandwidth;
    /** The token bucket size for the uplink. */
    TInt iUpLinkMaximumBurstSize;
    /** The maximum packet size for the uplink. */
    TInt iUpLinkMaximumPacketSize;
    /** The size of the minimum policed unit for the uplink. */
    TInt iUpLinkAveragePacketSize;
    /** The delay (in milliseconds) for the uplink. */
    TInt iUpLinkDelay;
    /** The priority for the uplink. */
    TInt iUpLinkPriority;
    
    /** The token rate for the downlink. */
    TInt iDownlinkBandwidth;
    /** The token bucket size for the downlink. */
    TInt iDownLinkMaximumBurstSize;
    /** The maximum packet size for the downlink. */
    TInt iDownLinkMaximumPacketSize;
    /** The size of the minimum policed packet for the downlink. */
    TInt iDownLinkAveragePacketSize;
    /** The delay (in milliseconds) for the downlink. */
    TInt iDownLinkDelay;
    /** The priority for the downlink. */
    TInt iDownLinkPriority;
    
    /** The name. */
    TName    iName;
    /** Internal represention of the flags. */
    TUint    iFlags;
    };

// Inline methods
inline TInt TQoSParameters::GetUplinkBandwidth() const
    /** Gets the current token rate for the uplink.
    * 
    * @return    The current token rate for the uplink. */
    { return iUplinkBandwidth; };

inline TInt TQoSParameters::GetUpLinkMaximumBurstSize() const
    /** Gets the current size of the token bucket, that is the maximum number
    * of tokens that the transfer can support.
    * 
    * @return    The current size of the token bucket. */
    { return iUpLinkMaximumBurstSize; };

inline TInt TQoSParameters::GetUpLinkMaximumPacketSize() const
    /** Gets the maximum uplink single packet size.
    * 
    * @return    The maximum uplink single packet size. */
    { return iUpLinkMaximumPacketSize; };

inline TInt TQoSParameters::GetUpLinkAveragePacketSize() const
    /** Gets the size of the minimum policed unit which is the smallest packet
    * size for traffic policing.
    * 
    * @return    The size of the minimum policed unit. */
    { return iUpLinkAveragePacketSize; };

inline TInt TQoSParameters::GetUpLinkDelay() const
    /** Gets the delay for uplink direction (in milliseconds).
    * 
    * @return    The delay for uplink direction (in milliseconds). */
    { return iUpLinkDelay; };

inline TInt TQoSParameters::GetUpLinkPriority() const
    /** Gets the uplink priority.
    * 
    * @return    The uplink priority. */
    { return iUpLinkPriority; };

inline TInt TQoSParameters::GetDownlinkBandwidth() const
    /** Gets the token rate for downlink direction. 
    * 
    * Token rate defines the requested transfer rate that the application 
    * requests.
    * 
    * @return    The downlink token rate in bytes/second. */
    { return iDownlinkBandwidth; };

inline TInt TQoSParameters::GetDownLinkMaximumBurstSize() const
    /** Gets the token bucket size for downlink direction. 
    * 
    * Token bucket size defines the burst size that the application may send.
    * 
    * @return The current token bucket size for downlink direction (bytes). */
    { return iDownLinkMaximumBurstSize; };

inline TInt TQoSParameters::GetDownLinkMaximumPacketSize() const
    /** Gets the maximum packet size for downlink direction.
    * @return    The maximum packet size for downlink direction in bytes. */
    { return iDownLinkMaximumPacketSize; };

inline TInt TQoSParameters::GetDownLinkAveragePacketSize() const
    /** The minimum policed unit for downlink direction. 
    * 
    * Minimum policed unit is used as the smallest packet size when doing 
    * traffic policing and estimating effect of protocol headers. The minimum
    * policed unit must be <= maximum packet size.
    * 
    * @return    The minimum policed unit for downlink direction in bytes. */
    { return iDownLinkAveragePacketSize; };

inline TInt TQoSParameters::GetDownLinkDelay() const
    /** Gets the requested delay for downlink direction.
    * 
    * @return    The delay for downlink direction (in milliseconds). */
    { return iDownLinkDelay; };

inline TInt TQoSParameters::GetDownLinkPriority() const
    /** Gets the priority for downlink direction. 
    * Priority can be used to prioritise between traffic flows inside the 
    * terminal.
    *
    * @return The priority for downlink direction, the value 0 represents the 
    *         highest priority. */
    { return iDownLinkPriority; };

inline void TQoSParameters::SetFlags(TUint aFlags)
    /** Set the flags. 
    * 
    * Currently the only flag supported is KPfqosOptionCanAdapt that indicates 
    * whether the aplication can adapt.
    * 
    * @param aFlags    The flag values to set. */
    { iFlags = aFlags; };

inline TUint TQoSParameters::Flags() const
    /** Gets the current flags.
    * 
    * @return    The current flags. */
    { return iFlags; };

inline const TName& TQoSParameters::GetName() const
    /** Gets a reference to the name.
    * 
    * @return    The reference to the name. */
    { return iName; };

#endif
