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
//

/**
 @file
*/

#include "IF_DEF.H"
#include "NI_STD.H"
#include "Ni_Log.h"
#include "NIFPRVAR.H"
#include <in_sock.h>
#include <es_prot.h>
#include <agenterrors.h>
#include "NIFConfigurationControl.h"
#include <comms-infras/es_config.h>
#include <es_panic.h>

#ifdef _DEBUG
#include <in_iface.h>
#endif

using namespace ESock;
#include <comms-infras/ss_subconnprov.h>
#include <ss_glob.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_subconnflow.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <networking/cfbearers.h>
#endif //SYMBIAN_ADAPTIVE_TCP_WINDOW_RECEIVE


// Security policies
_LIT_SECURITY_POLICY_C1(NifAgentPolicyNetworkControl, ECapabilityNetworkControl);

// Diagnostic string identifying this module when calling security policy checking methods

const char * const NifAgentPolicyDiagnostic = "CNifAgentBase";

/**
Constructor
*/
EXPORT_C CNifAgentBase::CNifAgentBase()
	{
	}


/**
This version of the Control() function is now deprecated.
Use the version of CNifAgentBase::Control() that takes four parameters
*/
EXPORT_C TInt CNifAgentBase::Control(TUint, TUint, TDes8&)
	{
	return KErrNotSupported;
	}

/**
Control() with capability checking
*/
EXPORT_C TInt CNifAgentBase::Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, const RProcess& aProcess)
	{
	TBool result( CheckControlPolicy(aOptionLevel, aOptionName, aProcess) );
	
	//if we got Permission (ETrue i.e. TRUE i.e. 1) then return call to Control(), else return KErrPermissionDenied
	return ( result? Control(aOptionLevel, aOptionName, aOption) : KErrPermissionDenied );
	}


/**
Read an integer
@param aField   The field name
@param aValue   Returned value
@param aMessage For capability checking
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadInt( aField, aValue, aMessage );
	}


/**
Read an integer
@param aField   The field name
@param aValue   Returned value
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadInt(const TDesC& aField, TUint32& aValue )
	{
	return ReadInt( aField, aValue, NULL );
	}


/**
Write an integer
@param aField   The field name
@param aValue   The value
@param aMessage For capability checking
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteInt( aField, aValue, aMessage );
	}


/**
Write an integer
@param aField   The field name
@param aValue   The value
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteInt(const TDesC& aField, TUint32 aValue )
	{
	return WriteInt( aField, aValue, NULL );
	}


/**
Read a descriptor
@param aField   The field name
@param aValue   Returned value
@param aMessage For capability checking
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadDes( aField, aValue, aMessage );
	}


/**
Read a descriptor
@param aField   The field name
@param aValue   Returned value
@param aMessage For capability checking
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes8& aValue )
	{
	return ReadDes( aField, aValue, NULL );
	}


/**
Write a descriptor
@param aField   The field name
@param aValue   value to write
@param aMessage For capability checking
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteDes( aField, aValue, aMessage );
	}


/**
Write a descriptor
@param aField   The field name
@param aValue   value to write
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC8& aValue )
	{
	return WriteDes( aField, aValue, NULL );
	}


/**
Read a descriptor
@param aField   The field name
@param aValue   Returned value
@param aMessage For capability checking
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadDes( aField, aValue, aMessage );
	}


/**
Read a descriptor
@param aField   The field name
@param aValue   Returned value
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadDes(const TDesC& aField, TDes16& aValue )
	{
	return ReadDes( aField, aValue, NULL );
	}


/**
Write a descriptor
@param aField   The field name
@param aValue   value to write
@param aMessage For capability checking
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteDes( aField, aValue, aMessage );
	}


/**
Write a descriptor
@param aField   The field name
@param aValue   value to write
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteDes(const TDesC& aField, const TDesC16& aValue )
	{
	return WriteDes( aField, aValue, NULL );
	}


/**
Read a boolean value
@param aField Field name
@param aValue Returned value
@param aMessage For capability checking
@return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage)
	{
	return DoReadBool( aField, aValue, aMessage );
	}


/**
Read a boolean value
@param aField Field name
@param aValue Returned value
@return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::ReadBool(const TDesC& aField, TBool& aValue )
	{
	return ReadBool( aField, aValue, NULL );
	}


/**
Write a boolean value
@param aField Field name
@param aValue Value to write
@param aMessage for capability checking
@return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage)
	{
	return DoWriteBool( aField, aValue, aMessage );
	}


/**
Write a boolean value
@param aField Field name
@param aValue Value to write
@return one of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::WriteBool(const TDesC& aField, TBool aValue )
	{
	return WriteBool( aField, aValue, NULL );
	}


/**
Read a long descriptor value
@param aField Field name
@param aMessage For capability checking
@return HBufC containing value
*/
EXPORT_C HBufC* CNifAgentBase::ReadLongDesLC(const TDesC& aField,const RMessagePtr2* aMessage)
	{
	return DoReadLongDesLC( aField, aMessage );
	}


/**
Read a long descriptor value
@param aField Field name
@return HBufC containing value
*/
EXPORT_C HBufC* CNifAgentBase::ReadLongDesLC(const TDesC& aField )
	{
	return ReadLongDesLC( aField, NULL );
	}


/**
Ensure that a client has the required capabilites to read a field
@param aField  Field Name
@param aMessage Contains client capabilites to validate
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::CheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	if( aMessage )
		{
		return DoCheckReadCapability( aField, aMessage );
		}
	else
		{
		return KErrNone;
		}
	}


/**
Ensure that a client has the required capabilites to write a field
@param aField  Field Name
@param aMessage Contains client capabilites to validate
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::CheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	if( aMessage )
		{
		return DoCheckWriteCapability( aField, aMessage );
		}
	else
		{
		return KErrNone;
		}
	}


/**
Ensure that a client has the required capabilites to read a field
@param aField  Field Name
@param aMessage Contains client capabilites to validate
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::DoCheckReadCapability( const TDesC& /*aField*/, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNone;
	}


/**
Ensure that a client has the required capabilites to write a field
@param aField  Field Name
@param aMessage Contains client capabilites to validate
@return One of the system-wide error codes
*/
EXPORT_C TInt CNifAgentBase::DoCheckWriteCapability( const TDesC& /*aField*/, const RMessagePtr2* /*aMessage*/ )
	{
	return KErrNone;
	}

	
/**
Check that a client has the required capabilties
@param aOptionLevel 
@param aOptionName 
@param aOption
@param aMessage 
@returns KErrNone, if the client has the required capabilites, otherwise one of the standard Symbian OS error codes
*/
TBool CNifAgentBase::CheckControlPolicy(TUint aLevel, TUint /*aOption*/, const RProcess& aProcess)
	{
	TBool result;
	
	switch (aLevel)
		{
	case KCOLAgent:
		result = NifAgentPolicyNetworkControl.CheckPolicy(aProcess, NifAgentPolicyDiagnostic);
		break;
		
	default:
		result = EFalse;
		break;
		}
	
	return result;
	}

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
Retrieve the default bearer information to the Agent CPR
@return default bearer information to the Agent CPR.
*/
EXPORT_C TUint32 CNifAgentBase::GetBearerInfo() const
	{
	//Return the default value, if the agent
	//has not overridden this function
	return  KDefaultBearer;
	}
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW


