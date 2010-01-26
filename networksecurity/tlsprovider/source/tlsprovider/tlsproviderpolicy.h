// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This class wraps up the tlsprovider policy.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __TLSPROVIDERPOLICY_H__
#define __TLSPROVIDERPOLICY_H__

#include <e32base.h>

_LIT(KTlsProviderPolicyFile, "z:\\resource\\tlsproviderpolicy.ini");
_LIT8(KClientAuthenticationDialogEnabled, "ClientAuthDlgEnabled");
_LIT8(KPKICriticalExtn_Pattern, "PKICriticalExtn_*");

/**
 * This class represents some settings that control the behaviour
 * of tlsprovider. The settings are stored in a resource
 * files and are retrieved at construction time.
 */
class CTlsProviderPolicy : public CBase
	{
public:

	/**
	 * This function returns an instance of CTlsProviderPolicy.
	 * It reads the settings from the resource file and initializes
	 * the instance accordingly.
	 */
	static CTlsProviderPolicy* NewL();
	
	~CTlsProviderPolicy();	
	
	/**
	 * This function checks if the tls client authentication dialog 
	 * is enabled or not
	 * @retval ETrue Tls client authentication dialog is enabled.
	 * @retval EFalse Tls client authentication dialog is disabled.
	 */
	TBool ClientAuthenticationDialogEnabled() const;
	
	TInt PKICriticalExtensions();
	RPointerArray<TDesC>& GetPKICriticalExtensions();
		
private:	

	CTlsProviderPolicy();
	
	/**
	 * The second-phase constructor. It reads the settings from
	 * the resource file.
	 */
	void ConstructL();

	/**
	Retrieves the next line of text from a buffer. Blank lines are skipped.

	@param aBuffer The buffer to parse.
	@param aPos    The position to start reading from. This IN/OUT parameter
	               should be initialised to zero on the first call.
    @param aLine   An out parameter that will be set to point to the next line
                   of text IF found.
    @return        Whether a line of text was successfully read.
	*/
	TBool ReadLineL(const TDesC8& aBuffer, TInt& aPos, TPtrC8& aLine) const; 
	 
private:

	/**
	 * This is set to ETrue if the client authentication is enabled
	 * It is set to EFalse if the client authentication is disabled
	 */
	TBool iClientAuthenticationEnabled;
	
	RPointerArray<TDesC> iCriticalExtn;
	};

#endif // __TLSPROVIDERPOLICY_H__
