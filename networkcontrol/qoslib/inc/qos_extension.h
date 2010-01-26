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

#ifndef __QOS_EXTENSION_H__
#define __QOS_EXTENSION_H__

/**
 * @file qos_extension.h 
 * This file defines the base class for QoS extensions that can be added to CQoSParameters.
 */

#include <e32std.h>
#include <in_sock.h>
//#include <networking/qosparameters.h>


/**
 * Base class for extension policies.
 *
 * @publishedPartner
 * @released
 */
class CExtensionBase : public CBase
{
public:
    IMPORT_C CExtensionBase();
    IMPORT_C ~CExtensionBase();
    
    /** 
    Extension must implement this function to allow qoslib to create 
    extensions.
     
    @publishedPartner
    @released
    @capability NetworkServices Restrict QoS operations in similar way as 
    normal socket operations.
    @leave If no memory is available.
    @return A pointer to extension object. 
    */
    virtual CExtensionBase* CreateL()=0;

    /** 
    Gets the type of the extension. 
     
    @publishedPartner
    @released
    @capability NetworkServices Restrict QoS operations in similar way as 
    normal socket operations.
    @return Extension type. 
    */
    inline TInt Type() const { return iType; }

    /** 
    Gets the extension data in a descriptor.
     
    The extension must implement this function to allow qoslib to fetch the 
    extension data.  
     
    @publishedPartner
    @released
    @capability NetworkServices Restrict QoS operations in similar way as 
    normal socket operations.
    @return Extension data. 
    */
    virtual TDesC8& Data()=0;

    /**
    Parses the extension data received as a parameter.
     
    The extension must implement this function to allow qoslib give the 
    extension data to the extension object.  
    
    @publishedPartner
    @released
    @capability NetworkServices Restrict QoS operations in similar way as 
    normal socket operations.
    @param aData Extension data.
    @return KErrNone, if aData contains a valid extension; otherwise, error 
    code. 
    */
    virtual TInt ParseMessage(const TDesC8& aData)=0;

    /**
    Copies aExtension into this object.
     
    The extension must implement this function to allow copying of 
    extensions.  
     
    @publishedPartner
    @released
    @capability NetworkServices Restrict QoS operations in similar way as 
    normal socket operations.
    @param aExtension Extension data.
    @return KErrNone, if copying succeeded; otherwise, error code. 
    */
    virtual TInt Copy(const CExtensionBase& aExtension)=0;


public:
    TSglQueLink iLink; //iLink needs to be accessible for setting offset!

protected:
    /** The extension type. */
    TInt iType;
    /** Buffer for extension data. */
    HBufC8* iData; 
    /** Pointer to next item in an extension queue. */
};

/** A typedef for a queue of extensions. */
typedef TSglQue<CExtensionBase> TQoSExtensionQueue;
/** A typedef for an iterator to a queue of extensions. */
typedef TSglQueIter<CExtensionBase> TQoSExtensionQueueIter;

#endif
