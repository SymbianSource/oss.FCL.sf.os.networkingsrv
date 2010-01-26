// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <e32std.h>
#include <c32comm.h>
#include "common.h"
/**
*	Serial Listener Active Object - receive commands on the serial port
*/
class CTestMgr;

class CSerialListener : public CActive
{
public:
	static CSerialListener* NewL(RComm* aCommPort, CTestMgr* aController);
		~CSerialListener();
	void 
			Recv();
		TBuffer*  GetData();
private:
		void ConstructL();
		CSerialListener(RComm* aCommPort, CTestMgr* aController);

		void RunL();
		void DoCancel();
//attributes
		RComm* iCommPort;						// Handle to the comm port
		CTestMgr* iController;			// Link to the controller
		TBuffer	iDataBuffer;		//The received data buffer
		TBuffer	iLastBuffer;		// The last data received
};

inline TBuffer*  
CSerialListener::GetData()
{
	return &iLastBuffer;
}

/**
*	Serial Sender Active Object - send commands on the serial port
*/
class CSerialSender : public CActive
	{
	public:
		static CSerialSender* NewL(RComm* aCommPort, CTestMgr* aController);
		~CSerialSender();
		void 
			Send(TBuffer& aBuff);
	private:
		void ConstructL();
		CSerialSender(RComm* aCommPort, CTestMgr* aController);

		void RunL();
		void DoCancel();
//attributes
		RComm* iCommPort;					// Handle to the comm port
		CTestMgr* iController;		// Link to the controller
		TBuffer iDataBuffer;		// The data to send
	};

#endif //__SERIAL_H__
