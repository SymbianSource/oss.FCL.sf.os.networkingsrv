/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* This file defines the generic QoS internal API interface.
* 
*
*/



/**
 @file qoslib_internal.h
 @internalTechnology
 @released

*/


#ifndef __QOSLIB_INTERNAL_H__
#define __QOSLIB_INTERNAL_H__
#include <networking/qosparameters.h>

/**
This class contains the generic QoS parameters and optional extensions.

*/
/* 
 * These methods are valid for Symbian OS 9.0 and onwards.
 */ 
class CQoSParameters : public CBase
{
public:
    /**
     * Constructor.
     */
    IMPORT_C CQoSParameters();

    /**
     * Destructor.
     */
    IMPORT_C ~CQoSParameters();

    /**
     * Sets the bandwidth for uplink direction. Bandwidth defines the requested transfer rate that the
     * application requests. Bandwidth must be > 0.
     *
     * @param aBandwidth Value (bytes/second) to which to set the bandwidth for uplink direction.
     */
    IMPORT_C void SetUplinkBandwidth(TInt aBandwidth);

    /**
     * Sets the maximum burst size for uplink direction. Maximum burst size defines the burst size that the
     * application might send. Maximum burst size must be > 0.
     *
     * @param aSize Value (bytes) to which to set the maximum burst size for uplink direction.
     */
    IMPORT_C void SetUpLinkMaximumBurstSize(TInt aSize);

    /**
     * Sets the maximum packet size for uplink direction.
     *
     * @param aMaxSize Value (bytes) to which to set the maximum packet size for uplink direction.
     */
    IMPORT_C void SetUpLinkMaximumPacketSize(TInt aMaxSize);

    /**
     * Sets the average packet size for uplink direction. Average packet size is used 
     * when doing traffic policing and estimating effect of protocol headers.
     * The average packet size must be <= maximum packet size.
     *
     * @param aSize Value (bytes) to which to set the average packet size for uplink direction.
     */
    IMPORT_C void SetUpLinkAveragePacketSize(TInt aSize);

    /**
     * Sets the requested delay for uplink direction.
     *
     * @param aDelay Value (ms) to which to set the delay for uplink direction.
     */
    IMPORT_C void SetUpLinkDelay(TInt aDelay);

    /**
     * Sets the priority for uplink direction. Priority can be used to prioritise between traffic 
     * flows inside the terminal.
     *
     * @param aPriority Value (0-7) to which to set the priority for uplink direction. 0 is the highest priority, 
     * 7 is lowest.
     */
    IMPORT_C void SetUpLinkPriority(TInt aPriority);

    /**
     * Returns the bandwidth for uplink direction. Bandwidth defines the requested transfer rate that the
     * application requests.
     *
     * @return Current bandwidth for uplink direction (bytes/second).
     */
    IMPORT_C TInt GetUplinkBandwidth() const;

    /**
     * Returns the maximum burst size for uplink direction. Maximum burst size defines the burst size that the
     * application might send.
     *
     * @return Current maximum burst size for uplink direction (bytes).
     */
    IMPORT_C TInt GetUpLinkMaximumBurstSize() const;

    /**
     * Returns the maximum packet size for uplink direction.
     *
     * @return Current maximum packet size for uplink direction (bytes).
     */
    IMPORT_C TInt GetUpLinkMaximumPacketSize() const;

    /**
     * Returns the average packet size for uplink direction. Average packet size is used 
     * when doing traffic policing and estimating effect of protocol headers.
     *
     * @return Current average packet size for uplink direction (bytes).
     */
    IMPORT_C TInt GetUpLinkAveragePacketSize() const;

    /**
     * Returns the delay for uplink direction.
     *
     * @return Currrent delay for uplink direction (ms).
     */
    IMPORT_C TInt GetUpLinkDelay() const;

    /**
     * Returns the priority for uplink direction. Priority can be used to prioritise between traffic 
     * flows inside the terminal.
     *
     * @return Current priority for uplink direction. 0 is the highest priority, 7 is lowest.
     */
    IMPORT_C TInt GetUpLinkPriority() const;

    /**
     * Sets the bandwidth for downlink direction. Bandwidth defines the requested transfer rate that the
     * application requests. Bandwidth must be > 0.
     *
     * @param aBandwidth Value (bytes/second) to which to set the bandwidth for downlink direction.
     */
    IMPORT_C void SetDownlinkBandwidth(TInt aBandwidth);

    /**
     * Sets the maximum burst size for downlink direction. Maximum burst size defines the burst size that the
     * application might send. Maximum burst size must be > 0.
     *
     * @param aSize Value (bytes) to which to set the maximum burst size for downlink direction.
     */
    IMPORT_C void SetDownLinkMaximumBurstSize(TInt aSize);

    /**
     * Sets the maximum packet size for downlink direction.
     *
     * @param aMaxSize Value (bytes) to which to set the maximum packet size for downlink direction.
     */
    IMPORT_C void SetDownLinkMaximumPacketSize(TInt aMaxSize);

    /**
     * Sets the average packet size for downlink direction. Average packet size is used as the 
     * smallest packet size when doing traffic policing and estimating effect of protocol headers.
     * The average packet size must be <= maximum packet size.
     *
     * @param aSize Value (bytes) to which to set the average packet size for downlink direction.
     */
    IMPORT_C void SetDownLinkAveragePacketSize(TInt aSize);

    /**
     * Sets the requested delay for downlink direction.
     *
     * @param aDelay Value (ms) to which to set the delay for downlink direction.
     */
    IMPORT_C void SetDownLinkDelay(TInt aDelay);

    /**
     * Sets the priority for downlink direction. Priority can be used to prioritise between traffic 
     * flows inside the terminal.
     *
     * @param aPriority Value (0-7) to which to set the priority for downlink direction. 0 is the highest priority, 
     * 7 is lowest.
     */
    IMPORT_C void SetDownLinkPriority(TInt aPriority);

    /**
     * Returns the bandwidth for downlink direction. Bandwidth defines the requested transfer rate that the
     * application requests.
     *
     * @return Current bandwidth for downlink direction (bytes/second).
     */
    IMPORT_C TInt GetDownlinkBandwidth() const;

    /**
     * Returns the maximum burst size for downlink direction. Maximum burst size defines the burst size that the
     * application might send.
     *
     * @return Current maximum burst size for downlink direction (bytes).
     */
    IMPORT_C TInt GetDownLinkMaximumBurstSize() const;

    /**
     * Returns the maximum packet size for downlink direction.
     *
     * @return Current maximum packet size for downlink direction (bytes).
     */
    IMPORT_C TInt GetDownLinkMaximumPacketSize() const;

    /**
     * Returns the average packet size for downlink direction. Average packet size is used as the 
     * smallest packet size when doing traffic policing and estimating effect of protocol headers.
     *
     * @return Current average packet size for downlink direction (bytes).
     */
    IMPORT_C TInt GetDownLinkAveragePacketSize() const;

    /**
     * Returns the delay for downlink direction.
     *
     * @return Currrent delay for downlink direction (ms).
     */
    IMPORT_C TInt GetDownLinkDelay() const;

    /**
     * Returns the priority for downlink direction. Priority can be used to prioritise between traffic 
     * flows inside the terminal.
     *
     * @return Current priority for downlink direction. 0 is the highest priority, 7 is lowest.
     */
    IMPORT_C TInt GetDownLinkPriority() const;

    /**
     * Sets the name of the flowspec. This allows application to specify a user friendly name for a flowspec.
     *
     * @param aName The name for the flowspec. Maximum length for the name is KMaxName.
     * @return KErrNone if maximum length is <= KMaxName, otherwise KErrTooBig.
     */
    IMPORT_C void SetName(const TName& aName);

    /**
     * Sets the adaptation mode. If the application is willing to accept lower QoS than requested, 
     * it should set the adapt mode on. By default adapt mode is not set.
     *
     * @param aCanAdapt The value for adapt mode.
     */
    IMPORT_C void SetAdaptMode(TBool aCanAdapt);

    /**
     * Sets the header mode. If the application wants QoS Framework to include network and transport layer
     * header values in bitrate calcutaions or not. By default the mode is set to include the headers in
     * the calculations. Protocols such as RTSP or SIP, which already include the header sizes in their 
     * calculations will want to alter the mode so that header values are not included in calculations twice.
     *
     * @param aHeadersIncluded The value for the header mode.
     */
    IMPORT_C void SetHeaderMode(TBool aHeadersIncluded);


    /**
     * Returns the name of the flowspec.
     *
     * @return Current name of the flowspec.
     */
    IMPORT_C const TName& GetName() const;

    /**
     * Returns the adaptation mode.
     *
     * @return aCanAdapt Current value for adapt mode.
     */
    IMPORT_C TBool AdaptMode() const;

    /**
     * Returns the headers included mode.
     *
     * @return aHeadersIncluded Current value for header included mode.
     */
    IMPORT_C TBool GetHeaderMode() const;

    /**
     * Copies the parameters and extensions from aPolicy into this object.
     *
     * @param aPolicy Contains the CQoSParameters object that is copied into this object.
     * @exception Leaves if there is no memory available for extensions.
     */
    IMPORT_C void CopyL(CQoSParameters& aPolicy);

    /**
     * Copies the extension into the list of extensions. If there already exists an extension with the
     * same type, it is replaced.
     *
     * @param aExtension The extension that is added to the list of extenstions.
     * @exception Leaves if there is no memory available for the extension.
     */
    IMPORT_C TInt AddExtensionL(CExtensionBase& aExtension);

    /**
     * Removes the extension with extension type aType from the list of extensions.
     *
     * @param aType The type of extension that is removed from the list of extensions.
     * @return KErrNone if the extension is found, othewise KErrNotFound.
     */
    IMPORT_C TInt RemoveExtension(TInt aType);

    /**
     * Returns an extension with extension type aType.
     *
     * @param aType The type of extension that is searched from the list of extensions.
     * @return A pointer to an extension if the extension is found, othewise NULL.
     */
    IMPORT_C CExtensionBase* FindExtension(TInt aType);

    /**
     * Returns a list of extensions that have been added to this object.
     *
     * @return A list of extensions.
     */
    IMPORT_C TQoSExtensionQueue& Extensions();

    IMPORT_C TInt GetDownLinkDelayVariation() const; 
    
    IMPORT_C TInt GetUpLinkDelayVariation() const; 
    
    IMPORT_C void SetDownLinkDelayVariation(TInt aVariation);
    
    IMPORT_C void SetUpLinkDelayVariation(TInt aVariation);
private:
    /** Generic QoS parameters. */
    TQoSParameters iQoS;
    /** A list of extensions. */
    TQoSExtensionQueue iExtensionList;
    // Member variable iName was taken into use temporarily
    TName iName;
    friend class CQoSMan;
    friend class CPolicy;
    friend class CQoSRequestBase;
};



#endif
