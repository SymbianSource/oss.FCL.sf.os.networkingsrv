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
// Contents : for saving/restoring persistent fax settings
// 
//

#ifndef __FAXSET_H
#define __FAXSET_H

#include <commdb.h>
#include <cdblen.h> 
#include <cdbcols.h>

/********************************************************************/

class TFaxSettings
/**
@internalComponent
*/
   {
 public:
   IMPORT_C void ExternalizeL (RWriteStream &) const;
   IMPORT_C void InternalizeL (RReadStream &);
   IMPORT_C TFaxSettings& operator=(const TFaxSettings& aFaxSettings);
   TBuf8 < 20 > iFaxId ;
   TInt iMaxSpeed;
   TInt iMinSpeed;
   TFaxResolution iPreferredResolution;
   TFaxCompression iPreferredCompression;
   TInt iPreferredECM;
   TInt iVerbose;
   TInt iFaxOnDemandDelay ;
   TFaxClass iFaxClass;
   };

/********************************************************************/

class CFaxSettings : public CBase
/**
now the CFaxSettings class
@internalComponent
*/
   {
public:
   static CFaxSettings * NewL ();
   static CFaxSettings * NewLC ();
   ~CFaxSettings ();
   void ValidateAndSetClassL (TFaxSettings *);

protected:
   void ConstructL ();
 protected:
   RFs iFileSession;
   CCommsDatabase* iDatabase;
   };

/********************************************************************/

#endif

