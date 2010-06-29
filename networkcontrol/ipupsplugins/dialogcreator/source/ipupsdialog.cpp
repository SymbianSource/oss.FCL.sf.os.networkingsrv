// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <e32svr.h> // Included here, since removal of Platform headers from public headers[f32file.h] for TB92SDK 
#include "ipupsdialog.h"
#include <ecom/implementationproxy.h>
#include <apgcli.h>
#include <ups/promptrequest.h>
#include <swi/sisregistrypackage.h>
#include <swi/sisregistrysession.h>
#include <scs/nullstream.h>
#include <s32mem.h>
#include <u32hal.h>

#include "ipupsconst.h"

static const TUint KIpDialogCreatorImplementationId = 0x10285A7C;

CIpUpsDialog* CIpUpsDialog::CreateDialogCreatorL()
/**
Factory method that instantiates a new dialog creator ECOM plug-in.

@return A pointer to the new reference dialog creator object.
*/
   {
   CIpUpsDialog* self = new (ELeave)CIpUpsDialog();
   CleanupStack::PushL(self);
   self->ConstructL();
   CleanupStack::Pop(self);
   return self;
   }

const TImplementationProxy ImplementationTable[] = 
   {
   IMPLEMENTATION_PROXY_ENTRY(KIpDialogCreatorImplementationId, CIpUpsDialog::CreateDialogCreatorL)
   };

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
/**
Standard ECOM factory
*/
   {
   aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
   return ImplementationTable;
   }	
	

CIpUpsDialog::CIpUpsDialog() 
/**
Constructor
*/
   : CDialogCreator(), iPromptResult(),iPromptResultPckg(iPromptResult), iState(EIdle)
   {
   CActiveScheduler::Add(this);
   }
	
CIpUpsDialog::~CIpUpsDialog()
/**
Destructor
*/
   {
   Deque();
   iPromptDataDes.Close();
   delete iPromptData;
   iNotifier.Close();
   }

void CIpUpsDialog::ConstructL()
/**
Second phase constructor
*/
   {
   User::LeaveIfError(iNotifier.Connect());
   
   // setup the value for the notifier. Test or reference
#if (defined (__EABI__)  ||  defined (__GCCXML__))
   // this value is patched via the patchable constant mechanism
   iNotifierId = KNotifierImplementationId;
#else
   TUint notifierUidVal = 0;
   TInt retCode = UserSvr::HalFunction(EHalGroupEmulator, EEmulatorHalIntProperty,
		   (TAny*)"NETWORKING_UPS_NOTIFIERUID", &notifierUidVal);

   if (retCode == KErrNone)
	   {
	   iNotifierId = notifierUidVal;
	   }
   else
	   {
	   TUint startupModeVal = 0;
	   retCode = UserSvr::HalFunction(EHalGroupEmulator, EEmulatorHalIntProperty,
			   (TAny*)"startupmode", &startupModeVal);
	   
	   if(retCode == KErrNone && startupModeVal == 1)
		   iNotifierId = KTestNotifierImplementationId;
	   else
		   iNotifierId = KNotifierImplementationId;
	   }  
#endif   
   }

void CIpUpsDialog::DoCancel()
   {
   if (iState == EProcessResult)
      {
      iNotifier.CancelNotifier(TUid::Uid(iNotifierId));
      }
   
   if (iClientStatus)
      {
      User::RequestComplete(iClientStatus, KErrCancel);
      }
   }
	
TInt CIpUpsDialog::RunError(TInt aError)
   {
   if (iClientStatus)
      {
      User::RequestComplete(iClientStatus, aError);
      }
   return KErrNone;
   }

void CIpUpsDialog::RunL()
   {
   User::LeaveIfError(iStatus.Int());
   switch (iState)
      {
      case EPrepareDialog:
         DoPrepareDialogL();
         break;
      case EDisplayDialog:
	     DoDisplayDialogL();
         break;
      case EProcessResult:
         DoProcessResultL();
         break;
      default:
         ASSERT(EFalse);			
      }
   }
	
void CIpUpsDialog::DoPrepareDialogL()
   {
   iPromptData = CPromptData::NewL();
	
   // Only one state at the moment but more should be
   // added for long running operators e.g. querying the SIS registry
   // or resolving the client entity.
   ResolveClientNameL(iRequest->ClientSid());
	
   // Get the vendor name for the client process
   ResolveVendorNameL(iRequest->ClientVid());
	
   // pass the destination information through.
   iPromptData->iDestination.Create(iRequest->Destination());
  
   // Pass any opaque data from the user to the notifier
   iPromptData->iOpaqueData.Create(iRequest->OpaqueData());

   // Server / Service localized names generated in notifier plug-in. 
   iPromptData->iServerSid = iRequest->ServerSid();
   iPromptData->iServiceId = iRequest->ServiceId();
	
   // Different dialog text is displayed depending on whether the client application
   // is signed.
   // N.B. Protected SID is assumed to be signed or included at ROM build.
   if (iRequest->IsClientSidProtected()) iPromptData->iFlags |= ETrustedClient;
	
   // Use the options specified by the policy
   iPromptData->iOptions = iPolicy->Options();
	
   // Add the descriptions of the fingerprints. This could be used
   // to allow the user to grant access to all destinations 
   // or a single destination.
   TInt count = iFingerprints->Count();
   for (TInt i = 0; i < count; ++i)
      {
      HBufC* description = (*iFingerprints)[i]->Description().AllocLC();
      iPromptData->iDescriptions.AppendL(description);
      CleanupStack::Pop(description);
      }
	
   User::RequestComplete(iClientStatus, KErrNone);		
   // DisplayDialog is invoked by the UPS, this just verifies 
   // that PrepareDialog was called first.
   iState = EDisplayDialog;
   }
	
void CIpUpsDialog::DoDisplayDialogL()
/**
Uses the notifier framework to display the dialog.
*/
   {
   // Externalize the prompt data to a descriptor
   RNullWriteStream ns;
   ns << *iPromptData;
   ns.CommitL();
   iPromptDataDes.CreateL(ns.BytesWritten());
   RDesWriteStream ws;	
   ws.Open(iPromptDataDes);
   ws << *iPromptData;
   ws.CommitL();	
   iNotifier.StartNotifierAndGetResponse(iStatus, TUid::Uid(iNotifierId),
                                         iPromptDataDes, iPromptResultPckg);
   SetActive();
   iState = EProcessResult;
   }
	
void CIpUpsDialog::DoProcessResultL()
/**
Processes the result returned by the notifier.
*/
   {			
   if(iPromptResult.iSelected == CPolicy::EAlways || iPromptResult.iSelected == CPolicy::ENever)
      {
      // The Always or Never option was selected so return the fingerprint 
      // for the new decision record.
      // 
      // In this implementation a copy of the original fingerprint is returned. However,
      // it is permitted to return a different fingerprint e.g. a modifier description.       
      if(iPromptResult.iDestination >= 0 && iPromptResult.iDestination < iFingerprints->Count())     
         {
         *iFingerprint = (*iFingerprints)[iPromptResult.iDestination];
         }
      else
         {
         ASSERT(EFalse); // should never happen, unless notifier is buggy.
         }
      }   

    // ensure the notifier has returned a valid option specified in policy file
   if(iPromptResult.iSelected & iPromptData->iOptions)
      {
      *iOptionSelected = iPromptResult.iSelected;
      }
   else
      {
      ASSERT(EFalse); 
      }
      
   iState = EIdle;
   User::RequestComplete(iClientStatus, KErrNone);	
   }

void CIpUpsDialog::ResolveVendorNameL(const TVendorId& aVid)
/**
Looks up the localized vendor name for the client process and writes
this to iPromptData.iVendorName.

Typically, this would be resolved from the SIS registry or a lookup table.

@param aVid	The vendor id of the client process.
*/
   {
   if (iPromptData->iVendorName.Length() != 0)
      {
      // already obtained vendor name from SIS registry
      return;
      }
		
   if (aVid.iId == 0x70000001)
      {
      _LIT(KSymbian, "Symbian Software Ltd");
      iPromptData->iVendorName.Create(KSymbian);
      }
   else 
      {
      _LIT(KUnknown, "Unknown vendor");
      iPromptData->iVendorName.Create(KUnknown);
      }
   }
	
void CIpUpsDialog::ResolveClientNameL(const TSecureId& aSid)
/**
Generates a human readable name for the client process. In order of 
preference the following data is returned

- The AppArc caption name.
- The localized package name that owns this SID.
- A value from a lookup table.
- The filename for the client process executable.

@param aSid	The secure id of the client process.
*/
   {
   TBool found = EFalse;
	
   // Although the client name from AppArc takes precedance the SIS
   // registry is always invoked in order to retrieve the vendor name
   found |= ResolveClientNameFromSisRegistryL(aSid);
   found |= ResolveClientNameFromAppArcL(aSid);
			
   // A lookup that maps secure-ids to application names could
   // be used here.

   // Fall back to the filename of the client process
   // The original thread may have exited so the process handle is used instead.
   // because the client-side object e.g. RSocket may be shared between threads.

   // If the process has exited then it's o.k. to leave.
   if (! found)
      {			
      RProcess clientProcess;
      User::LeaveIfError(clientProcess.Open(iRequest->ClientProcessId()));
      CleanupClosePushL(clientProcess);
      iPromptData->iClientName.Create(clientProcess.FileName());		
      CleanupStack::PopAndDestroy(&clientProcess); 
      }
   }

TBool CIpUpsDialog::ResolveClientNameFromAppArcL(const TSecureId& aSid)
/**
Gets the caption name for the application from AppArc (if available).

@param	aSid	The secure id of the client process.
@return		ETrue if a match was found in apparc; otherwise, EFalse is returned.
*/
   {
   TBool found(EFalse);
	
   RApaLsSession apa;
   CleanupClosePushL(apa);	
   TInt err = apa.Connect();
   if (err == KErrNone)
      {		
      TApaAppInfo* info = new(ELeave) TApaAppInfo();
      CleanupStack::PushL(info);
		
      err = apa.GetAppInfo(*info, TUid::Uid(aSid));
      
      if (err == KErrNone)
         {
         iPromptData->iClientName.Close();
         iPromptData->iClientName.Create(info->iCaption);
         found = ETrue;
	     }
      else if (err != KErrNotFound)
	     {
	     User::Leave(err);
	     }	
      
      CleanupStack::PopAndDestroy(info);
      }
   else if (err != KErrNotFound)
      {
      // If the connection to apparc failed with KErrNotFound
      // then the error is ignored becase we assume the dialog
      // creator was invoked from text-shell
      User::Leave(err);
      }
   
   CleanupStack::PopAndDestroy(&apa);
   return found;
   }

TBool CIpUpsDialog::ResolveClientNameFromSisRegistryL(const TSecureId& aSid)
/**
Retrieves the client and vendor information from the SIS registry.
@param aSid		The secure-id of the client application to lookup in the registry.
@return			ETrue, if the lookup was successful; otherwise, EFalse is returned.
*/
   {
   TBool found(EFalse);
   Swi::RSisRegistrySession r;
   User::LeaveIfError(r.Connect());
   CleanupClosePushL(r);
	
   Swi::CSisRegistryPackage* p(0);
   TRAPD(err, p = r.SidToPackageL(aSid));
   if (err == KErrNone)
      {
      iPromptData->iClientName.Create(p->Name());
      iPromptData->iVendorName.Create(p->Vendor());
      found = ETrue;
      delete p;
      }
   
   CleanupStack::PopAndDestroy(&r);
   return found;
   }

// From CDialogCreator
void CIpUpsDialog::PrepareDialog(const UserPromptService::CPromptRequest& aRequest, 
                                 const CPolicy& aPolicy,			
                                 const RPointerArray<CFingerprint>& aFingerprints, 
                                 const CClientEntity* aClientEntity, 
                                 const TAny* aEvalPrivateData, 
                                 TRequestStatus& aStatus)
   {
   aStatus = KRequestPending;
   iClientStatus = &aStatus;
	
   iRequest = &aRequest;
   iPolicy = &aPolicy;
   iFingerprints = &aFingerprints;
   iEvalPrivateData = aEvalPrivateData;
   (void) aClientEntity;

   // Kick off dialog creator state machine
   iState = EPrepareDialog;
   iStatus = KRequestPending;
   TRequestStatus* status = &iStatus;
   SetActive();
   User::RequestComplete(status, KErrNone);
   }
	
void CIpUpsDialog::DisplayDialog(CPolicy::TOptions& aOptions, 
                                 const CFingerprint*& aFingerprint,
                                 TUint& aEvaluatorInfo,
                                 TRequestStatus& aStatus)
   {	
   aStatus = KRequestPending;
   iClientStatus = &aStatus;
	
   iOptionSelected = &aOptions;
   iFingerprint = &aFingerprint;
   aFingerprint = 0;
   iEvaluatorInfo = &aEvaluatorInfo;
   iClientStatus = &aStatus;
	
   // Start state machine
   ASSERT(iState == EDisplayDialog); // PrepareDialog should have been called first
   iStatus = KRequestPending;
   TRequestStatus* status = &iStatus;
   SetActive();
   User::RequestComplete(status, KErrNone);
   }

