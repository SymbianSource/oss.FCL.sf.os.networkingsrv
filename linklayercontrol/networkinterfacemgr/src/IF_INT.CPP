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
 @file IF_INT.CPP
*/

#include <comms-infras/nifif.h>
#include "NI_STD.H"
#include <in_sock.h>


//
// CNifIfFactory default implementations
//

EXPORT_C CNifIfBase* CNifIfFactory::NewInterfaceL(const TDesC& aName, MNifIfNotify* /*aNotify*/)
/**
 * Create a new instance of the nif contained within this DLL, passing in the interface to nifman for nifs that need the MNifIfNotify pointer for early initialisation (eg: for CommDb access).
 * @note This should only be used as a temporary method of stacking nifs, where the commdb access is used to find out which additional nif to load
 * @param aName The name of the nif instance to create
 * @since v7.0s
 */
	{
	return NewInterfaceL(aName);	// By default, call the standard NewInterfaceL() virtual
	}


//
// CNifIfBase default implementations
//

EXPORT_C CNifIfBase::CNifIfBase(CNifIfFactory& aFactory)
/**
Constructor
 */
	: iFactory(&aFactory)
	{

	iFactory->Open();
	}

EXPORT_C CNifIfBase::CNifIfBase(CNifIfLink& aLink)
/**
Constructor
 */
	: iFactory(aLink.iFactory), iNotify(aLink.iNotify)
	{

	iFactory->Open();
	}

EXPORT_C CNifIfBase::CNifIfBase()
/**
Constructor
 */
	{

	}

EXPORT_C CNifIfBase::~CNifIfBase()
/**
Destructor
*/
	{

	if(iFactory)
	    iFactory->Close();
	}

EXPORT_C void CNifIfBase::Open()
/**
Increment reference count
*/
	{
	++iRefCount;
	}


EXPORT_C void CNifIfBase::Close()
/**
Decrement reference count
*/
	{
	
	--iRefCount;
	__ASSERT_DEBUG(iRefCount>=0, Panic(ENifManPanic_NegativeCloseCount));
	if(iRefCount<=0)
		delete this;
	}

EXPORT_C void CNifIfBase::Cleanup(TAny* aIf)
/**
Clean up an interface
*/
	{

	((CNifIfBase*)aIf)->Close();
	}


EXPORT_C void CNifIfBase::BindL(TAny* /*aId*/)
/**
 * Bind to the protocol layer (store the pointer to the CProtocolBase object)
 * @param aId A pointer to the protocol layer (should be cast to a CProtocolBase object)
 */
	{

	}

EXPORT_C TInt CNifIfBase::Control(TUint /*aLevel*/, TUint /*aName*/, TDes8& /*aOption*/, TAny* /*aSource*/)
/**
 * Control function for additional control of the nif
 * @param aLevel The intended level for this control option
 * @param aName The name of the control option
 * @param aOption Any data associated with this control option, contained within a TPckg(Buf)
 * @param aSource If provided, an identifier for the source of the control option; by default, zero
 * @return KErrNone if successful; otherwise one of the standard Symbian OS error codes
 */
	{
	return KErrNotSupported;
	}

EXPORT_C TInt CNifIfBase::State()
/**
 * Return the current state of the interface
 * @return A TIfStatus indicating the current state of the interface
 * @note Return interface as being up for BC
 * @since v2.0?
 */
	{
	// Panic as each nif should implement this method, however, for BC reasons this cannot be made pure virtual
	__ASSERT_DEBUG(ETrue, Panic(ENifManPanic_IncorrectNifOperation));
	return TInt(EIfUp);
	}


//
// CNifIfLink default implementations
//

EXPORT_C CNifIfLink::CNifIfLink(CNifIfFactory& aFactory)
/**
 *
 */
	: CNifIfBase(aFactory)
/**
Constructor
*/
	{
	}

EXPORT_C void CNifIfLink::AuthenticateComplete(TInt /*aResult*/)
/**
 * Indicate that the data returned by MNifIfNotify::Authenticate() is now valid
 * @note This function is unused because [*** to do ***]
 * @param aResult The error code, if any, from fetching the authentication data
 */
	{
	}

EXPORT_C void CNifIfLink::Restart(CNifIfBase* /*aIf*/)
/**
 * Do nothing, as nifs do not have to support binder layer restart functionality
 * @note Obviously if a nif does support binder layer restart, then it should overload this method
 * @param aIf Pointer to the CNifIfBase binder layer to restart
 */
	{
	}




