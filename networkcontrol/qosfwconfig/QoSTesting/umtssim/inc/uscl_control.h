// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef _USCL_CONTROL_H__
#define _USCL_CONTROL_H__

class RControl : public RSubSessionBase
    {
    public:
      IMPORT_C RControl();
      IMPORT_C ~RControl();
      IMPORT_C TInt Open(RUmtsSimServ &aServer);
      IMPORT_C void Close();

    public:
      // order notifications on all server activity
      IMPORT_C void NotifyAll(TRequestStatus& aStatus, TDes& aMsg) const;

      // Force server reload configuration from file (note that reconfiguring events cancel any
      // triggered events that are waiting for timer). Note that if reconfiguration is asked
      // for both sections and either of them fails, then request fails and resulting state is
      // unspecified mix of new, old and cleared configurations.
      enum TCFlag {ECFlagReqMan = 1, ECFlagEventCtrl = 2};
      IMPORT_C TInt ReconfigureSimulator(TInt aFlag = ECFlagReqMan | ECFlagEventCtrl) const;
      IMPORT_C TInt ReconfigureSimulator(const TDesC& aFilename,
                                         TInt aFlag = ECFlagReqMan | ECFlagEventCtrl) const;

      // configure single request handler
      IMPORT_C TInt ConfigureRequestHandler(const TDesC& aCfgString) const;
      // configure single event (note: cannot override existing id)
      IMPORT_C TInt ConfigureEvent(const TDesC& aCfgString) const;

      IMPORT_C void CancelAsyncRequest(TInt aReqToCancel) const;

      // Added because of migration to Client/Server V2 API
      // TRequestStatus iSRStatus;
    };

#endif
