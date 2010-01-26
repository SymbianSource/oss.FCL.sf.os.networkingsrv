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

#include <ctransfer.h>
#include <in_sock.h>


const TInt KNbTransfers= 25;			// Number of transfers. Typically, it represents
						// the number maximum of socket transfer a listener can handle.
const TUint KQueueListenSize = 5;		// Number of waiting connection to be accepted


//
// class CListenerOptions //
//
/** Object from this class will contain option for a
 * listening connection
 */
class CListenerOptions : public CBase
	{
	public:
		static CListenerOptions* NewL(TPtrC8 aConfiguration);
		
		TInetAddr Addr();
		void SetAddress(const TInetAddr& aAddr);
		TUint Family();
		void SetFamily(TUint aFamily);
		TInt Port();
		void SetPort(TInt aPort);
		TName ProgramName();
		void SetProgramName(const TName& aName);
		TName ProgramOptions();
		void SetProgramOptions(const TName& aOptions);
		TUint Protocol();
		void SetProtocol(TUint aProtocol);
		TUint SockType();
		void SetSockType(TUint aSockType);

	protected:
		void ConstructL(TPtrC8 aConfiguration);
		
	private:
		TInetAddr iBindAddr;	// Bind address
		TName iProgramName;	// Program Name
		TName iProgramOptions;	// Program Arguments
		TUint iProtocol;	// Protocol (TCP, UDP, ...)
		TName iService;		// Internet service name
		TUint iSockType;	// Socket type (STREAM, DATAGRAM)
	};


//
// class CListener //
//
/** Each object of this class will represent a listening
 * connection.
 * It will provide the required socket and manage the transfer
 * to the appropriate program.
 *
 *
 * //
 * // Handle receiving connections //
 * //
 * Get ready to listen and accept connection (only in connected mode)
 * NewL():
 * 	1: open listening socket (in a real implementation you may want to look at options)
 * 	2: bind socket (in a real implementation you may want to look at options)
 * Accept():
 * 	3: wait and listen
 * RunL():
 * 	4: open blank socket
 * 	5: accept 
 */ 	
class CListener : public CActive
	{
	public:
		static CListener* NewL(	
				RSocketServ& aSockServ, 
				RConnection& aConnection, 
				CListenerOptions* aOptions);
		~CListener();
		void DoCancel();
		void AcceptL();
		CListenerOptions* Options();

	protected:
		CListener(RSocketServ& aSockServ, CListenerOptions* aOptions);
		void RunL();
		void ConstructL(RConnection& aConnection);
		void AddTransferL();
		void PurgeAllTransfers();

	private:
		RSocket iListenSocket;			// Listening socket as specified
							// in iOptions.
		RSocket iBlankSocket;			// Pointer to temporary socket
	       						// used in Accept.
		RSocketServ iSockServ;			// Reference to socket server.
		CListenerOptions* iOptions;		// Listeners options.
		RPointerArray<CTransfer> iTransfers;		// Stores all active transfers.
	};


// Typedef for easier maintenance
typedef RPointerArray<CListener> TListeners;
