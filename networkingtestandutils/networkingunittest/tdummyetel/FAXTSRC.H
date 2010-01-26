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
// Contents :  for sending faxes from fax stores
// 
//

#ifndef __FAXTSRC_H
#define __FAXTSRC_H

/**
@internalComponent
*/
enum TFaxPreferredCompression
	{
	Prefer1D, Prefer2D 
	};


/********************************************************************/

class TFaxEntry
/**
@internalComponent
*/
   {
 public:
   void InternalizeL (RReadStream &);
   void ExternalizeL (RWriteStream &) const;

   TInt iNameLength;
   TFileName iFaxPageStore;
   TInt iStartPage;
   TInt iPageCount;
   };

/********************************************************************/

class CFaxTransferSource : public CBase
/**
@internalComponent
*/
   {
 public:
   static CFaxTransferSource * NewL ();
   static CFaxTransferSource * NewLC ();
    ~CFaxTransferSource ();

 protected:
   void ConstructL ();
   
 public:
   IMPORT_C void AddSourceL (const TFileName &, TFaxPreferredCompression);
   IMPORT_C void AddSourceL (const TFileName &, TInt, TFaxPreferredCompression);
   IMPORT_C void AddSourceL (const TFileName &, TInt, TInt, TFaxPreferredCompression);
   IMPORT_C void RemoveAllSources ();

   void GetNextPageReadyL ();

   void OpenFaxInL (const TDesC & aFileName);
   void PagePrepare ();
   void WriteFaxDataL (const TDesC8 & aBuffer);
   void WritePageParmsL (TFaxBufSenderId & aSenderId);
   void CloseFaxInStore ();

   TInt OpenFaxOutL (const TDesC & aFileName); // returns the number of pages in the file
   TInt NextPageFind ();
   void ReadPageParmsL ();
   void ReadFaxData (TDes8 & aBuffer);
   void PreviousPageFindL ();
   void CloseFaxStore ();

   TInt iLines;                 // length of image in lines
   TInt iResolu;                // resolution of image
   TInt iCompression;           // type of compression
   TInt iBitOrder;              // little or big endian ordering of bits in a byte
   TInt iPage;                  // page number of image in fax

   TStreamId iOurStreamID;
   CBufSeg * iSources;
   RBufReadStream iInstream;
   RBufWriteStream iOutstream;

   CWriteFaxFile * iWriteFaxFile;
   CReadFaxFile * iReadFaxFile;

   TInt iFaxListEntries;
   TInt iSavedFaxListEntries;
   TInt iFaxPages;
   TInt iPageInCurrentFile;
	
   TFaxPreferredCompression  iOurPreferredCompression;
   TFaxEntry iOurFaxEntry;
   };

#endif

