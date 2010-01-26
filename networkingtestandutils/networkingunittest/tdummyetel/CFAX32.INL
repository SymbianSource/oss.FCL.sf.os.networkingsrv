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

#ifndef __cfax32_INL
#define __cfax32_INL

/********************************************************************/

inline void CFaxTransfer::SetMode (TFaxMode aMode)
   {
   iMode = aMode ;
   }

inline void CFaxTransfer::SetPhoneNumber (TDesC8 & aNumber)
   {
   iPhoneNumber.Copy (aNumber);
   }

inline void CFaxTransfer::SetReceiveFileName (const TFileName & aName)
   {
   iReceiveFileName.Copy (aName);
   }

/********************************************************************/

#endif

