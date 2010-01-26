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


#ifndef _USCL_QOS_INTERNAL_H__
#define _USCL_QOS_INTERNAL_H__

#include "uscl_qos.h"

// ******************
// FOR PACKET QOS
// ******************

class CPacketQoSInternalData : public CBase
    {
      friend class RPacketQoS;
    public:
      static CPacketQoSInternalData* NewL();
      ~CPacketQoSInternalData();

    private:
      CPacketQoSInternalData();
      void ConstructL() {}

    private:
    };

#endif
