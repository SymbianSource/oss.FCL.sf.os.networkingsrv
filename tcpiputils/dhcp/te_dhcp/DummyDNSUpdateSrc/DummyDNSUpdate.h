/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* The Dummy DNS Update plug-in interface
* 
*
*/



/**
 @file DummyDNSUpdate.h
*/

#ifndef __DUMMYDNSUPDATE_H__
#define __DUMMYDNSUPDATE_H__

#include <e32base.h>
#include "DNSUpdateIf.h"

class CDummyDnsUpdate : public CDnsUpdateIf
{

public:
   // Wraps ECom object instantitation
   static CDummyDnsUpdate* NewL();
   // Wraps ECom object destruction 
   virtual ~CDummyDnsUpdate();


   virtual void Update( TDesC& aInterfaceName, TDesC8* aHostName, TDesC8* aDomainName );

private:
   void ConstructL();
};

#endif
