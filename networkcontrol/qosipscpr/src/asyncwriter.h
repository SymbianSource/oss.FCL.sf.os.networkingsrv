// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ASYNCWRITER_H_
#define ASYNCWRITER_H_

#include <networking/qoslib.h>
#include <e32base.h>
#include "pfqoslib.h"
#include "pfqos_stream.h"
#include "qos_msg.h"

NONSHARABLE_CLASS(CAsyncWriter) : public CActive
	{
	public:
		static CAsyncWriter* NewL(CQoSMsgWriter* aWriter);
		void Send(CQoSMsg* aMessage);
	protected:
		CAsyncWriter(CQoSMsgWriter* aWriter);
		void ConstructL();
		void RunL();
		void DoCancel();
	private:
		CQoSMsgWriter* iWriter;
		CQoSMsg* iMessage;
	};

#endif //ASYNCWRITER_H_

