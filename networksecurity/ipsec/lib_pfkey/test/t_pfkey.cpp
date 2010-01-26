// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32test.h>

#include "lib_pfkey.h"
#include "pfkey_ext.h"
#include "pfkey_send.h"

LOCAL_D RTest test(_L("T_pfkey"));
_LIT(KMyPanicDescriptor, "Comparison failed");


void do_pfkey_base(TPfkeyMsgBase& aMsg)
	{
//	test.Printf(_L("\n********Testing Creating base*********\n"));
	sadb_msg &msg = aMsg.MsgHdr();

	msg.sadb_msg_version = PF_KEY_V2;
    msg.sadb_msg_type = SADB_GETSPI;
    msg.sadb_msg_satype = SADB_SATYPE_ESP;
    msg.sadb_msg_seq = 1;
    msg.sadb_msg_pid = 1;
    msg.sadb_msg_errno = 0;
    msg.sadb_msg_reserved = 0;

//	test.Printf(_L("Length of Pfkkey msg is %d\n"), sizeof(TPfkeyMsgBase));
	}

void add_SA_ext(TPfkeySendMsg& aMsg, TInt one = 10, TInt two = 20, TInt three = 30)
	{
//	test.Printf(_L("Sizeof() SA_ext is %d\n"), sizeof(TPfkeyExtSA));
	aMsg.Add(Int2Type<SADB_EXT_SA>(), one, (TUint8)two, (TUint8)three);
	const TInetAddr addr(INET_ADDR(192,168,10,3), 80);
	aMsg.Add(Int2Type<SADB_EXT_ADDRESS_SRC>(), addr, (TUint8)two, (TUint8)three);
	aMsg.Finalize();
	}

void decode_ext(TPfkeyAnyExt& aExt)
	{
	__ASSERT_ALWAYS((aExt.ExtType() == SADB_EXT_SA) || (aExt.ExtType() == SADB_EXT_ADDRESS_SRC),
		User::Panic(KMyPanicDescriptor, KErrGeneral));
	}

void test_rMsg(TPfkeyRecvMsg& aMsg)
	{	
	TInt count = 0;
	const sadb_msg &msg = aMsg.MsgHdr();
	__ASSERT_ALWAYS(msg.sadb_msg_satype == SADB_SATYPE_ESP, User::Panic(KMyPanicDescriptor, KErrGeneral));
	__ASSERT_ALWAYS(msg.sadb_msg_errno == 0, User::Panic(KMyPanicDescriptor, KErrGeneral));
	__ASSERT_ALWAYS(msg.sadb_msg_version == PF_KEY_V2, User::Panic(KMyPanicDescriptor, KErrGeneral));

	TPfkeyAnyExt* ext2 = new(ELeave) TPfkeyAnyExt;
	while(aMsg.NextExtension(*ext2) == KErrNone)
		{
		count++;
		decode_ext(*ext2);
		}
	__ASSERT_ALWAYS(count==2, User::Panic(KMyPanicDescriptor, count));

	delete ext2;
	}

void test_sendL()
	{
	__UHEAP_MARK;

	TPfkeySendMsg* msg1 = new(ELeave) TPfkeySendMsg();
	do_pfkey_base(*msg1);

	TPfkeySendMsg msg2(SADB_GETSPI, SADB_SATYPE_ESP, 1, 1);
	TInt err = msg2.Compare(*msg1);
	__ASSERT_ALWAYS(err>=0, User::Panic(KMyPanicDescriptor, err));

	msg2.Reset(SADB_GETSPI, SADB_SATYPE_ESP, 1, 1);
	err = msg2.Compare(*msg1);
	__ASSERT_ALWAYS(err>=0, User::Panic(KMyPanicDescriptor, err));

	add_SA_ext(*msg1);
	add_SA_ext(msg2);
	err = msg2.Compare(*msg1);
	__ASSERT_ALWAYS(err>=0, User::Panic(KMyPanicDescriptor, err));

	TPfkeyRecvMsg rMsg1 = msg2;
	TPfkeyRecvMsg* rMsg2 = new(ELeave) TPfkeyRecvMsg(*msg1);
	
	test_rMsg(rMsg1);
	test_rMsg(*rMsg2);

	delete(msg1);
	delete(rMsg2);
	__UHEAP_MARKEND;
	}

void test_CheckSadb_AddL()
	{
	__UHEAP_MARK;
	
	TPfkeySendMsg sendMessage(SADB_ADD,SADB_SATYPE_ESP,4,6);
	TInt one=10;
	TInt two=2;
	TInt three=3;
	sendMessage.Add(Int2Type<SADB_EXT_SA>(),one, (TUint8)two, (TUint8)three);

	const TText8* encKey8 = (TText8*)"567812341234567812341234";
	TPtrC8 encKey(encKey8);
	sendMessage.Add(Int2Type<SADB_EXT_KEY_ENCRYPT>(),encKey);
	const TText8* authKey8 = (TText8*)"567812341234567812341234";
	TPtrC8 authKey(authKey8);

	sendMessage.Add(Int2Type<SADB_EXT_KEY_AUTH>(),authKey);
	
	const TInetAddr srcAddr(INET_ADDR(10,192,192,150), 2280);
	sendMessage.Add(Int2Type<SADB_EXT_ADDRESS_SRC>(), srcAddr, (TUint8)two);
	const TInetAddr destnAddr(INET_ADDR(10,192,192,150), 2380);
	sendMessage.Add(Int2Type<SADB_EXT_ADDRESS_DST>(), destnAddr, (TUint8)two);
	
			/* Second SA addition */
	
	
	TPfkeySendMsg sendMessage1(SADB_ADD,SADB_SATYPE_ESP,1,1);
	sendMessage1.Add(Int2Type<SADB_EXT_SA>(),one, (TUint8)two, (TUint8)three);

	const TText8* encKey18 = (TText8*)"567812341234567812341234";
	TPtrC8 encKey1(encKey18);
	sendMessage1.Add(Int2Type<SADB_EXT_KEY_ENCRYPT>(),encKey1);

	sendMessage1.Add(Int2Type<SADB_EXT_KEY_AUTH>(),authKey);
	
	const TInetAddr srcAddr1(INET_ADDR(10,192,192,151), 2080);
	sendMessage1.Add(Int2Type<SADB_EXT_ADDRESS_SRC>(), srcAddr1, (TUint8)two);
	const TInetAddr destnAddr1(INET_ADDR(10,192,192,151), 2180);
	sendMessage1.Add(Int2Type<SADB_EXT_ADDRESS_DST>(), destnAddr1, (TUint8)two);
	
	

	RSocketServ iSS;
	User::LeaveIfError(iSS.Connect());
	RSADB iSadb;
	iSadb.Open(iSS);
	
	TRequestStatus aStatus;
	iSadb.FinalizeAndSend(sendMessage,aStatus);
	User::WaitForRequest(aStatus);
	User::LeaveIfError(aStatus.Int());
	
	iSadb.FinalizeAndSend(sendMessage1,aStatus);
	User::WaitForRequest(aStatus);
	User::LeaveIfError(aStatus.Int());

	TPfkeyRecvMsg recvMsg;
	iSadb.ReadRequest(recvMsg,aStatus);
	User::WaitForRequest(aStatus);
	User::LeaveIfError(aStatus.Int());
	
	if(recvMsg.MsgHdr().sadb_msg_type != SADB_ADD)
		{
		User::Leave(KErrGeneral);
		}

	sadb_msg sMsg = recvMsg.MsgHdr(); // just for verifying all fields
	TInt mesErrNo = recvMsg.MsgHdr().sadb_msg_errno;
	TInt remBytes = recvMsg.BytesUnparsed();
	
	TPfkeyRecvMsg recvMsg1;
	iSadb.ReadRequest(recvMsg1,aStatus);
	User::WaitForRequest(aStatus);
	User::LeaveIfError(aStatus.Int());
	
	if(recvMsg1.MsgHdr().sadb_msg_type != SADB_ADD)
		{
		User::Leave(KErrGeneral);
		}
	
	sMsg = recvMsg1.MsgHdr();

	mesErrNo = recvMsg1.MsgHdr().sadb_msg_errno;
	
	if(mesErrNo != KErrNone)
		{
		User::Leave(KErrGeneral);
		}
	
	iSadb.Close();
	__UHEAP_MARKEND;
	}
	

GLDEF_C TInt E32Main()
    {
	test.Printf(_L("\n********Testing TPfkeysendMsg*********\n"));
	test_sendL();
	test_CheckSadb_AddL();
	test.Getch();
	return KErrNone;
	}
