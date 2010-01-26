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
//

#ifndef __PFQOS_H__
#define __PFQOS_H__

#include <e32std.h>

/** @internalTechnology */
const TUint KPfqosMsgV1     = 1;
/** @internalTechnology */
const TUint KPfqosMsgRev    = 200009L;


/** 
 * Message Types
 * 
 * @internalTechnology 
 */
enum TPfqosMessages
    {
    EPfqosReserved,        // Not used
    EPfqosUpdate,
    EPfqosAdd,
    EPfqosDelete,
    EPfqosGet,
    EPfqosFlush,
    EPfqosDump,            // Only for debugging purposes
    EPfqosEvent,
    EPfqosConfigure,
    EPfqosReject,
    EPfqosJoin,
    EPfqosLeave,
    EPfqosCreateChannel,
    EPfqosOpenExistingChannel,
    EPfqosDeleteChannel,
    EPfqosConfigChannel,
    EPfqosLoadFile,
    EPfqosUnloadFile
    };

/**
 * Extension header values
 * 
 * @internalTechnology 
 */
enum TPfqosExtensions
    {
    EPfqosExtReserved,        // Not used
    EPfqosExtSrcAddress,
    EPfqosExtDstAddress,
    EPfqosExtSelector,
    EPfqosExtFlowspec,
    EPfqosExtModulespec,
    EPfqosExtEvent,
    EPfqosExtConfigure,
    EPfqosExtConfigFile,
    EPfqosExtExtension,
    EPfqosExtChannel
    };

// Event types
/** @internalTechnology */
const TUint KPfqosEventReserved  = 0x0001;
/** @internalTechnology */
const TUint KPfqosEventFailure   = 0x0002;
/** @internalTechnology */
const TUint KPfqosEventConfirm   = 0x0004;
/** @internalTechnology */
const TUint KPfqosEventReceivers = 0x0008;
/** @internalTechnology */
const TUint KPfqosEventSenders   = 0x0010;
/** @internalTechnology */
const TUint KPfqosEventAdapt     = 0x0020;
/** @internalTechnology */
const TUint KPfqosEventJoin      = 0x0040;
/** @internalTechnology */
const TUint KPfqosEventLeave     = 0x0080;
/** @internalTechnology */
const TUint KPfqosEventAll = (KPfqosEventFailure | KPfqosEventConfirm | \
                              KPfqosEventReceivers | KPfqosEventReceivers | \
                              KPfqosEventAdapt | KPfqosEventJoin | KPfqosEventLeave);


/** 
 * Policy options
 * 
 * @internalTechnology 
 */
const TUint KPfqosOptionDynamic = 1; // Dynamic policy => removed when 
                                     // PFQOS SAP is closed


/**
 * Policy types
 * 
 * @internalTechnology 
 */
enum TQoSPolicyTypes
    {
    EPfqosFlowspecPolicy,
    EPfqosModulespecPolicy,
    EPfqosExtensionPolicy
    };

/** @internalTechnology */
enum TPfqosPriority
    {
    EPfqosDefaultPriority,
    EPfqosApplicationPriority,
    EPfqosOverridePriority
    };


/**
 * Base Message Header Format
 *
 * @internalTechnology 
 */
struct pfqos_msg
{
    TUint8  pfqos_msg_version;     /* PFQOS_MSG_V1                         */
    TUint8  pfqos_msg_type;        /* Message type: see PFQOS_xxx defines  */
    TInt    pfqos_msg_errno;       /* Error return value                   */
    TUint16 pfqos_msg_options;     /* Policy options                       */
    TUint16 pfqos_msg_len;         /* Total msg length in 64-bit words     */
    TUint32 pfqos_msg_seq;         /* Sequence number assigned by original */
                                   /*                              sender  */
    TUint32 pfqos_msg_pid;         /* Id of the user-process (Not used)    */
    TUint32 pfqos_msg_reserved;    /* Pad = 0 */
};

//
// Base header is followed by additional message fields (extensions), 
// all of which start with a length-type pair. This is a generic struct 
// used to decode the actual length and type of an extension, i.e. all 
// extensions begin with these exactly same fields.
//

//
// Additional Message Fields
//
/** @internalTechnology */
struct pfqos_ext
{
    TUint16 pfqos_ext_len;     /* In 64-bit words, inclusive */
    TUint16 pfqos_ext_type;    /* see PFQOS_EXT_xxx          */
};

/**
 * Address Extension
 *
 * @internalTechnology 
 */
struct pfqos_address
{
    TUint16    pfqos_address_len;
    TUint16    pfqos_ext_type; /* == PFQOS_EXT_ADDRESS_SRC, */
                               /*    PFQOS_EXT_ADDRESS_DST  */
    TUint16    pfqos_port_max; /* Maximum port number       */
                               /*  (needed for port ranges) */
    TUint16    reserved;       /* Padding = 0               */
};
/* Followed by some form of struct sockaddr */

/**
 * Selector Extension
 *
 * @internalTechnology 
 */
struct pfqos_selector
{
    TUint16 pfqos_selector_len;
    TUint16 pfqos_ext_type;    /* == PFQOS_EXT_SELECTOR           */
    TInt32  uid1;              /* Uid1                            */
    TInt32  uid2;              /* Uid2                            */
    TInt32  uid3;              /* Uid3                            */
    TUint32 iap_id;            /* IAP identifier                  */
    TUint32 policy_type;       /* Policy type                     */
    TUint32 priority;          /* Priority: default, application, */
                               /*                    or override  */
    TUint16 protocol;          /* Protocol number                 */
    TUint16 reserved;          /* Padding = 0                     */
    char    name[KMaxName];    /* Name                            */
};


/**
 * Event Extension
 *
 * @internalTechnology 
 */
struct pfqos_event
{
    TUint16 pfqos_event_len;
    TUint16 pfqos_ext_type;    /* == PFQOS_EXT_EVENT             */
    TUint16 event_type;        /* Event type                     */
    TUint16 event_value;       /* Event value:                   */
};                             /* return value for a QoS request */


/**
 * Flow specification
 *
 * @internalTechnology 
 */
struct pfqos_flowspec
{
    TUint16     pfqos_flowspec_len;
    TUint16     pfqos_ext_type;        /* PFQOS_EXT_FLOWSPEC */
                
    TUint32     uplink_bandwidth;
    TUint32     uplink_maximum_burst_size;
    TUint32     uplink_maximum_packet_size;
    TUint32     uplink_average_packet_size;
    TUint32     uplink_delay;
                
    TUint32     downlink_bandwidth;
    TUint32     downlink_maximum_burst_size;
    TUint32     downlink_maximum_packet_size;
    TUint32     downlink_average_packet_size;
    TUint32     downlink_delay;
                
    TUint32     flags;
    TUint16     uplink_priority;
    TUint16     downlink_priority;
    TUint8      reserved;           /* Padding = 0           */
    TName       name;               /* Name of the flowspec  */
};


/** @internalTechnology */
struct pfqos_channel
{
    TUint16 pfqos_channel_len;
    TUint16 pfqos_ext_type;    /* == PFQOS_EXT_EVENT     */
    TInt     channel_id;       /* QoS Channel Identifier */
};


/**
 * Module Specification
 *
 * @internalTechnology 
 */
struct pfqos_modulespec
{
    TUint16 pfqos_modulespec_len;
    TUint16 pfqos_ext_type;        /* PFQOS_EXT_MODULESPEC            */
    TUint32 protocol_id;           /* Protocol id needed to load dlls */
    TUint32 flags;                 /* Flags                           */
    TUint32 reserved;              /* Padding = 0                     */
    char    name[KMaxName];        /* Name of the module              */
    char    path[KMaxFileName];    /* Filename of the dll module      */
};
/* may be followed by module specific extensions */

/**
 * policy_config_file is used to pass a config filename to a QoS module. 
 * Maybe not needed.
 * 
 * @internalTechnology 
 */
struct pfqos_config_file
{
    TUint16 pfqos_config_file_len;
    TUint16 pfqos_ext_type;            /* PFQOS_EXT_CONFIG_FILE */
    TUint32 reserved;                  /* Padding = 0           */
    char    filename[KMaxFileName];    /* Config filename       */
};


/**
 * Configure message
 *
 * @internalTechnology 
 */
struct pfqos_configure
{
    TUint16 pfqos_configure_len;
    TUint16 pfqos_ext_type;        /* PFQOS_EXT_CONFIGURE             */
    TUint16 protocol_id;           /* Protocol id needed to load dlls */
    TUint16 reserved;              /* Padding = 0                     */
};

/**
 * pfqos_configure is followed by modulespecific configure data block.
 *
 * @internalTechnology 
 */
struct pfqos_extension
{
    TUint16 pfqos_ext_len;             /* In 64-bit words, inclusive */
    TUint16 pfqos_ext_type;            /* see PFQOS_EXT_xxx          */
    TUint32 pfqos_extension_type;      /* Extension identifier       */
    char    extension_name[KMaxName];  /* Extension block name       */
};

//
// Extension policy types
//
/** @internalTechnology */
// For UMTS extension
const TUint KPfqosExtensionUmts     = 5;
const TUint KPfqosR5ExtensionUmts   = 9;
/** @internalComponent */
const TUint KPfqosExtensionChannel  = 6;

#ifdef SYMBIAN_NETWORKING_UMTSR5
// For SBLP extension
const TUint KPfqosExtensionSBLP     = 7;
// for IMS extension
const TUint KPfqosExtensionIMS      = 8;
#else
// for SBLP
const TUint KPfqosExtensionIMS      = 7;
#endif // SYMBIAN_NETWORKING_UMTSR5


//
// Data types
//
/** @internalTechnology */
const TInt KPfqosTypeInteger    = 1;
/** @internalTechnology */
const TInt KPfqosTypeReal       = 2;
/** @internalTechnology */
const TInt KPfqosTypeString     = 3;

// Maximum length of variable name
/** @internalTechnology */
const TUint KPfqosMaxName       = 64;


/** @internalTechnology */
struct pfqos_configblock
{
    char       id[KPfqosMaxName];
    TUint32    reserved;
    TUint16    type;
    TUint16    len;
    TUint32    padding;
};

/** @internalTechnology */
struct pfqos_configblock_int
{
    char       id[KPfqosMaxName];
    TUint32    reserved;
    TUint16    type;
    TUint16    len;
    TInt32     value;
    TUint32    padding;
};

/** @internalTechnology */
struct pfqos_configblock_real
{
    char       id[KPfqosMaxName];
    TUint32    reserved;
    TUint16    type;
    TUint16    len;
    float      value;
    TUint32    padding;
};

// Maximum length of fully qualified domain name
/** @internalTechnology */
const TUint KMaxFQDNLength = 256;
  
/** @internalTechnology */
struct pfqos_configblock_mat
{
    char                    id[KPfqosMaxName];
    TUint16                 type;
    TUint16                 len;
    TBuf8<KMaxFQDNLength>   auth_token;
    TUint16                 flow_id_count;
    TUint16                 padding;
};

/** @internalTechnology */
struct pfqos_configblock_flow_id
{
    char       id[KPfqosMaxName];
    TUint32    reserved;
    TUint16    type;
    TUint16    len;
    TUint16    media_comp_number;
    TUint16    ip_flow_number;
    TUint32    padding;
};

#endif

