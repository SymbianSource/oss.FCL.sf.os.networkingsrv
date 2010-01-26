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
// clistener.h
// 
//

#include <e32base.h>
#include <es_sock.h>

const TUint KWaitTime = 300000;			// Time before cancelling a socket transfer.
const TUint KSlotParameter = 8;			// This is the slot used to transfer the socket.

class CTransfer;


//
// class CTimerTransfer //
//
/** Objects from this class will be used in order
 * to prevent some CTransfer object waiting forever
 * on a Rendezvous. 
 */
class CTransferTimer : public CTimer
	{
	public:
		static CTransferTimer* NewL(CTransfer* aTransfer);
		
	protected:
		CTransferTimer(CTransfer* aTransfer);
		void ConstructL();
		void RunL();
		void DoCancel();

	private:
		CTransfer* iTransfer;
	};


//
// class CTransfer //
//
/** Object from this class will manage transfer from an accepted connection
 * to appropriate program
 *
 * //
 * // Transfer socket to appropriate program //
 * //
 * We have accepted a connection, we pass it
 * to appropriate program.
 * Transfer():
 *	1: Create a new process
 *	2: Get socket name
 *	3: Pass socket name as a parameter for newly
 * 	   created process
 * 	4: Rendezvous newly created process
 * 	5: Resume newly created process
 * 	6: Wait for rendezvous
 * RunL():
 * 	7: Verify socket transfer result
 */ 	
class CTransfer : public CActive
	{
	public:
		static CTransfer* NewL(RSocket& aSocket, const TName& aProgram, const TName& aProgramOptions);
		~CTransfer();

		void TransferL();
		void DoCancel();
		void Stop();
	
	protected:
		CTransfer(RSocket& aSocket, const TName& aProgram, const TName& aProgramOptions);
		void RunL();
		void ConstructL();

	private: 
		TName iProgram;		// Program to transfer to.
		TName iProgOptions;	// Program options.
		RProcess iProcess;	// Used to create new process.
		RSocket& iSocket;	// Socket to transfer.
		CTransferTimer* iTimer;	// Timer in order to be cancelled 
					// if we do not manage to create
					// a new process.
	};


