// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// MNTCPAPP -- a Brief
// To perform TCP tests sometimes it is required to run a stub application 
// on the DUT. The job of this stub application is to generate the TCP user 
// calls as per the test requirements. It generates the following user 
// calls(as described in RFC 793) Open(Active/Passive), Send, Receive, Close 
// and Abort. To generate these calls by the stub application, ANVL needs to 
// send certain commands to the stub. This control  communication between ANVL 
// and stub is done through a UDP channel. At this point the stub application 
// behaves as a UDP server and ANVL acts as its client. On receiving different 
// control commands the from ANVL via the control UDP channel, the stub 
// application generates the above mentioned TCP calls. As a TCP application 
// the stub sometimes acts as a tcp client and some times as a TCP server, 
// depending on the type of Open call (active or passive) issued by it.
//



//#define USEFULLDUPLEXCLOSE // Run failed cases with this TAG activated (changes anvl stub RSocket API usage)

#define __DEBUG__

#if 0 // ###EPOC_MODIFIED
    #include <mntcpos.h>
    #include <mntcpapp.h>
#else

    #define PRINT_NL(a)  FPrintf(a, "") // print \n

    #include <stdlib.h> 
    #include <stdarg.h>
    
    #include <e32std.h>
    #include <es_sock.h>  
    #include <in_sock.h>
    #include <ext_hdr.h>
    #include <udp_hdr.h> 
    
    #include "anvlmain.h"
    #include "anvlglob.h"

    #include "mntcpos.h"
    #include "mntcpapp.h"

#endif


/* #define  __TCP_STDURG__ if this socket option is suported */

static int UDPRequestsLoop(void);

// static void UDPRequestsLoop(unsigned short localPort, int selectedIpVersion);

static void AppVersionGet(void);

/* Functions to generate TCP calls as mentioned in RFC 793 */
static int TCPListen(char *localPort);  /* Passive open */
static int TCPConnect(char *anvlAddr, char *anvlTCPPort);  /* active open */
static int TCPSend(char *data, int dataLen, char noDelay, char urg, char noPush);
static int TCPReceive(int expectLen);
static int TCPReceiveLimSz(int expectLen);
static int TCPReceiveOnce(int expectLen);
static int TCPClose(int statSend);
static int TCPCloseNoReset(void);
static int TCPAbort(void);

static int UDPSend(char *data, int dataLen, MNTCPAppSocketInfo_t sockInfo);

/* TCPSocketCreate is not a call described by RFC 793, it actually creates 
   a stream socket. */
static int TCPSocketCreate(void);

// static int StateSet(long int state);
static int StateSet(int state);

static void PendingErrorGet(ApplicationState_t *theAppState);
static int MSSSet(long int mssVal);
static int TTLSet(long int ttlVal);
static int SockRecvBuffSet(long int rcvbuflen);
static void IssueAccept(void);
static int OptUrgSet(void);
static int OptUrgInlineSet(void);


/* initialization/logging method */
static void NotifyStart(char *testNum);
static void NotifyEnd(char *testNum);

static void SendStatusToAnvl(char *msg);

static unsigned int GetCmdId(char *type, char *command);

static char *IntToASCII(long int num, char *numStr);

static unsigned short IPChecksum(unsigned char *data, unsigned long int len);
static unsigned long int IPChecksumIncr(unsigned char *data, unsigned long int len, unsigned long int total);
static void NetworkByteOrder(unsigned char *data, unsigned int count);

static unsigned int MsgPaddingAdd(char *msg);
static unsigned int MsgPaddingRemove(char *msg);

// #ifdef __DEBUG__
// static void DEBUGStateStrGet(MNTCPAppState_t stateIndx, char *stateStr);

// static void DEBUGMsgDecode(char *cmdMsg, char *extra);

// #endif

/*+++king added */
int ShutDnConn(ShutDownSock_t type);


#if 0 // ###EPOC_MODIFIED
/* Global data */
ApplicationState_t appState;

char tempRecv[MAX_MESG_SIZE + 1];  /* +1 to add NULL at the end */
#endif


/*>
    
  int main(int argc, char* argv[]);
  
  DESCRIPTION: 

  This function parses command line arguments and opens a UDP socket
  through which the TCP Stub Application will listen for requests to 
  perform TCP functions.
  
  ARGS:
  argc         - the number of arguments
  argv[]       - the list of arguments
  
  RETURNS:
  
  int          - C return value of main
   
<*/

int mntcpapp_main(unsigned short localPort, int selectedIpVersion) 
{
	
	// int			   argc;
	// char		   *argv[5];

	// char		   *arg0 = "mntpcapp";
	// char		   *arg1 = "-p";
	// char		   *arg2 = "10000";
	// char		   *arg3 = "-ipv6";
	// char		   *arg4 = NULL;

	int            status;
	unsigned short idx;
	
	int ret_loop_value = TRUE;

	const TInt KListenQSize = 32;

    	TInt error;
  
	AnvlGlob       *anvlglob;

	anvlglob = AnvlGetGlobals();

	if(ret_loop_value == TRUE)
	{ 

	RSocketServ iSocketServer;
	RSocket iSocket;				// Listen socket.
	RSocket* iConnection;			// Connection socket.
    	// #if EPOC_SDK >= 0x07010000 
    	RConnection iRConnection;     
    
    	TRequestStatus iStatus = KErrNone; // CActive data member
    

	// Make a connection to the Socket Server.
    	// error = iSocketServer.Connect(201);
	error = iSocketServer.Connect();	

    	error = iRConnection.Open(iSocketServer, KAfInet);
    	error = iRConnection.Start();
   
    	error = iSocket.Open(iSocketServer, KAfInet, KSockStream, KProtocolInetTcp);
	
	if (error == 0)
	{
		
	// Reuse the port
	iSocket.SetOpt(KSoReuseAddr, KProtocolInetIp, 1);

	// Bind the socket to the http port.
	
	TInetAddr httpPort(KInetAddrAny, 80);
	
	httpPort.SetFamily(0); // Listen both IPV4 and IPV6. (Dual-Stack).
	
	iSocket.Bind(httpPort);
	
	// Listen for incoming connections.
	iSocket.Listen(KListenQSize);
	
	iConnection = new RSocket;
   
	error = iConnection->Open(iSocketServer);

	if (error == 0)
	{
	
	iSocket.Accept(*iConnection,iStatus);
	
    if (selectedIpVersion == 4)
	{
    	
	MemSet(&anvlglob->appState, '\0', sizeof(anvlglob->appState));

	// if ((anvlglob->pfile = fopen("c:\\anvllog.txt", "wt")) == NULL) {
	//	return MNTCPAPP_APICALL_ERR;
	//  } 
  
  	/* Open a UDP socket (an Internet datagram socket) */
  
	if ((anvlglob->appState.udpSockInfo.connSock = 
		OSSockSocket(MNTCPAPP_SOCKTYPE_DGRAM)) == MNTCPAPP_API_RETCODE_NOTOK) {
		// FPrintf(stderr, "! Unable to create a UDP socket");
		PError("! reason");
		return MNTCPAPP_APICALL_ERR;
	}
	
	/* Bind a local address so that the client can send requests */
	if (localPort == 0) {
	
		anvlglob->appState.udpSockInfo.localPort = MNTCPAPP_UDP_PORT;
		// anvlglob->appState.udpSockInfo.localPort = 10000;	
	}
	else {
		anvlglob->appState.udpSockInfo.localPort = localPort;
	} 
	
	/* Retry UDP_SOCK_BIND_RETRIES_NUM times to bind the UDP socket */
	for (idx = 0, status = -1; status < 0 && idx < UDP_SOCK_BIND_RETRIES_NUM; 
		idx++) {
		status = OSSockBind(anvlglob->appState.udpSockInfo.connSock, 0, 
                        anvlglob->appState.udpSockInfo.localPort);
	}

	
	if (status == MNTCPAPP_API_RETCODE_NOTOK) {
		OSSockClose(anvlglob->appState.udpSockInfo.connSock);
	//	FPrintf(stdout, "Unable to bind the UDP socket to any local address");
		PError("! reason");
		return MNTCPAPP_APICALL_ERR;
	}
  
	}
  
  
   /* Wait for requests from ANVL and take action(s) accordingly */
   

   ret_loop_value = UDPRequestsLoop();
  
  
   /* CLOSE UDP SOCKETS */

   OSSockClose(anvlglob->appState.udpSockInfo.connSock);
   fclose(anvlglob->pfile);

  
   iConnection->Close();
   
   iSocket.Close();

   iRConnection.Stop();
	
   iRConnection.Close();
	
   iSocketServer.Close();
	
   delete iConnection;
  
  }
 }
}
	
 if(ret_loop_value == FALSE)
 {
 return TRUE;	
 }
 else
 {
  return FALSE;	
 }
}


/*-----------------------------------------------------------------------*/

/*
  NOTE:
  #1 After receiving a command through the UDP channel this function sends
  an ack. This ack is an ASCII string containing the length of the 
  received command
*/



/*>>
   static void UDPRequestsLoop(void)
  
  DESCRIPTION: 

  This function waits for UDP messages on the UDP socket, parses them, and then
  calls the appropriate TCP subroutine.  This function continues in this
  manner indefinitely.
  
 
  
<<*/

static int
UDPRequestsLoop(void)
{
 
  RTimer timer; 
  
  TRequestStatus delayStatus;
  
  int shutdown_tcpip_stack = FALSE;
 
  int noDelay;
    
  int urg;
  
  int noPush;
  
  char          *anvlAddr; 
  char          *anvlTCPPort; 
  
  int			TCPport;
  int			UDPport;

  char          *dutTCPPort;
  char          *data;
  char          *repeatDataBuff;
  char          *command;
  char          *type;

  #if 0 // ###EPOC_MODIFIED
  
  char          mesg[MAX_MESG_SIZE];
   
  char          mesgAck[MAX_MESG_SIZE];
  
  #else
  
  char          *mesg;

  char          *mesgAck;
  
  #endif
  
  char          buf[SMALL_BUF_LEN];
  char          tmpBuf[SMALL_BUF_LEN];
  long int      numBytes;
  long int      ackLen;
  long int      expectLen;
  long int      dataLen;
  long int      tmpLen;
  long int      tempState; 
  long int      mssVal;
  int           ttlVal;
  int           cmdId;
  unsigned long rcvBufVal;
  unsigned short checksum;
  char          *anvlUDPPort; 
  char          *dutUDPPort;
  MNTCPAppSocketInfo_t udpDataSockInfo;

  #if 0 // ###EPOC_MODIFIED
  #else
  
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();
 
  
  if ((mesg = ((char *)malloc(MAX_MESG_SIZE))) == NULL) {
 	    FPrintf(stderr, "malloc of mesg failed in UDPRequestsLoop");
      return -1;
    } 
   
 
   if ((mesgAck = ((char *)malloc(MAX_MESG_SIZE))) == NULL) {
      FPrintf(stderr, "malloc of mesgAck failed in UDPRequestsLoop");
      return -1;
     }
  
  #endif

//  for ( ; ; ) {  
  
while (shutdown_tcpip_stack == FALSE)
{

    numBytes = OSSockRecvFrom(anvlglob->appState.udpSockInfo.connSock, mesg, 
                              MAX_MESG_SIZE, &(anvlglob->appState.udpSockInfo));
  
    if (numBytes == MNTCPAPP_API_RETCODE_NOTOK) {
      // FPrintf(stderr, "! recvfrom error occurred");
      PError("! UDPRequestsLoop::reason");
      continue;
    }
    mesg[numBytes] = 0; /* NULL terminate the received UDP message */

    /* remove any padding */
    numBytes -= MsgPaddingRemove(mesg);

    
    /* 
       convert the integer (numBytes => number of received) to a string. This 
       string will be sent via the UDP channel as an ACK of the received UDP 
       command 
    */
    
    IntToASCII(numBytes, mesgAck); 
    ackLen = StrLen(mesgAck);

    type = StrTok(mesg, MNTCPAPP_CMD_DELIM);
    command = StrTok(NULL, MNTCPAPP_CMD_DELIM);
    cmdId = GetCmdId(type, command);

    /* send acknowledgement */
    /* first pad the ack if necessary */
    ackLen += MsgPaddingAdd(mesgAck);

    if (OSSockSendTo(anvlglob->appState.udpSockInfo.connSock, mesgAck, 
                     ackLen, anvlglob->appState.udpSockInfo) == 
                                     MNTCPAPP_API_RETCODE_NOTOK) {
    // FPrintf(stdout, "\n OSSockSendTo MNTCPAPP_API_RETCODE_NOTOK \n");      
    // FPrintf(stderr, "sendto error occurred");
     	PError("! UDPRequestsLoop::reason");
      continue;
    }

    switch (cmdId) {
	 
	case TCP_CMD_REBOOT:
	 
    		timer.CreateLocal();
  		timer.After(delayStatus,10000000);
  		User::WaitForRequest(delayStatus);
  		timer.Close();
  
 		free(mesg);
		free(mesgAck);
	
		shutdown_tcpip_stack = TRUE;

	break;

	case TCP_CMD_GET_VERSION:

        	 AppVersionGet();

	break;

    case TCP_CMD_CONNECT:
      anvlAddr = command;
      anvlTCPPort = StrTok(NULL, MNTCPAPP_CMD_DELIM);
      anvlglob->appState.tcpSockInfo.connSock = TCPConnect(anvlAddr, anvlTCPPort);
      break;

    case TCP_CMD_SEND:
      anvlAddr = command;
      anvlTCPPort = StrTok(NULL, MNTCPAPP_CMD_DELIM);
      data = StrTok(NULL, MNTCPAPP_CMD_DELIM);
      dataLen = ASCIIToInt(StrTok(NULL, MNTCPAPP_CMD_DELIM));
      noDelay = (StrCmp(StrTok(NULL, MNTCPAPP_CMD_DELIM), 
                        (const char *)MNTCPAPP_STR_NODELAY) ? 0 : 1);
      urg = (StrCmp(StrTok(NULL, MNTCPAPP_CMD_DELIM), 
                    (const char *)MNTCPAPP_STR_URG) ? 0 : 1);
	  noPush = (StrCmp(StrTok(NULL, MNTCPAPP_CMD_DELIM), 
					   (const char *)MNTCPAPP_STR_NOPUSH) ? 0 : 1);

	  if (anvlglob->appState.dutState == TCP_STATE_LISTEN) {

		/* In LISTEN we won't do a Connect so we have to supply the 
		   other side's address info directly */
		if (StrCmp(anvlAddr, "0") == 0) {
		
		  /*
			When anvl address is 0 we do not want to
			set this socket blocking.
		  */
		  anvlglob->appState.tcpSockInfo.anvlIPAddr = 0;
		  anvlglob->appState.tcpSockInfo.anvlPort = 0;
		} 
		else {
			anvlglob->appState.tcpSockInfo.anvlIPAddr = OSIPAddrStrToLong(anvlAddr);
		     
		    TCPport = ASCIIToInt((const char*)anvlTCPPort);
	        
			//anvlglob->appState.tcpSockInfo.anvlPort = ASCIIToInt(anvlTCPPort);
			anvlglob->appState.tcpSockInfo.anvlPort = (unsigned short)TCPport;

			if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
						  FCNTL_APPEND_NONBLOCKING) == 
			  MNTCPAPP_API_RETCODE_NOTOK) {
			// FPrintf(stderr, "! Flag setting error before connect\n");
			PError("! TCPConnect::reason");
		  }
		}
	  }

      if (!TCPSend(data, dataLen, (unsigned char)noDelay, (unsigned char)urg, (unsigned char)noPush)) {
      //  FPrintf(stdout, "Data sent");
      }
      break;
    

	 case TCP_CMD_SEND_REPEAT_DATA:
	  anvlAddr = command;
	  anvlTCPPort = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	  
	  data = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	  dataLen = ASCIIToInt(StrTok(NULL, MNTCPAPP_CMD_DELIM));

	 
	  repeatDataBuff = ((char *)Malloc(dataLen * sizeof(unsigned char)));
	  
	  
	  // (unsigned char *)Malloc(dataLen * sizeof(unsigned char));
	  
	  MemSet(repeatDataBuff, *data, dataLen);
	  
	  noDelay = (StrCmp(StrTok(NULL, MNTCPAPP_CMD_DELIM), 
						MNTCPAPP_STR_NODELAY) ? 0 : 1);
	  urg = (StrCmp(StrTok(NULL, MNTCPAPP_CMD_DELIM), 
					MNTCPAPP_STR_URG) ? 0 : 1);
	  noPush = (StrCmp(StrTok(NULL, MNTCPAPP_CMD_DELIM), 
					   MNTCPAPP_STR_NOPUSH) ? 0 : 1);
	  
	  if (!TCPSend(repeatDataBuff, dataLen, (unsigned char)noDelay, (unsigned char)urg, (unsigned char)noPush)) {
	  // 	FPrintf(stdout, "Data sent\n");
	  }

	  free(repeatDataBuff);

	  break;

    case TCP_CMD_RECEIVE:
      if (command) {
        expectLen = ASCIIToInt(command);
      }
      else {
        expectLen = MNTCPAPP_DEF_RECV_DATALEN;
      }
      if (TCPReceive(expectLen) > 0) {
      //  FPrintf(stdout, "Data received");
      }
      else {
      //  FPrintf(stderr, "! Did not received data");
      }
      break;
    
    case TCP_CMD_RECEIVE_LIMSZ:
      if (command) {
        expectLen = ASCIIToInt(command);
      }
      else {
        expectLen = MNTCPAPP_DEF_RECV_DATALEN;
      }
      if (TCPReceiveLimSz(expectLen) > 0) {
      //  FPrintf(stdout, "Data received");
      }
      else {
      //  FPrintf(stdout, "! Did not receive data");
      }
      break;
      
    case TCP_CMD_RECEIVE_RECEIVE_ONCE:
      if (command) {
        expectLen = ASCIIToInt(command);
      }
      else {
        expectLen = MNTCPAPP_DEF_RECV_DATALEN;
      }
      
      if (TCPReceiveOnce(expectLen) > 0) {
      //  FPrintf(stdout, "Data received");
      }
      else {
      //  FPrintf(stdout, "! Did not receive data");
      }
      break;

    case TCP_CMD_LISTEN:
      dutTCPPort = command;
      TCPListen(dutTCPPort);
      break;

      /*+++king renamed*/
    case TCP_CMD_CLOSE:
      #ifdef USEFULLDUPLEXCLOSE
      ShutDnConn(SHUT_WRITE);
      #else
      TCPClose(TRUE);
      #endif
      break;
    
      /*+++king renamed*/
    case TCP_CMD_CLOSE_NORST:
      TCPCloseNoReset();
      break;
    
    case TCP_CMD_SET_STATE:
      tempState = ASCIIToInt(StrTok(NULL, MNTCPAPP_CMD_DELIM));
      StateSet(tempState);
      break;
    
    case TCP_CMD_ASYNC_RECV:
      anvlglob->appState.recvMode = 1;
      break;
    
    case TCP_CMD_GET_SAMPLE_DATA:

	  /* Command to retrieve fixed data (through the UDP channel) which 
		 is already received through the TCP channel */

	  /* Add padding, if necessary */
	  tmpLen = StrLen(anvlglob->appState.rcvdBuff) + MsgPaddingAdd(anvlglob->appState.rcvdBuff);

	  if (OSSockSendTo(anvlglob->appState.udpSockInfo.connSock, anvlglob->appState.rcvdBuff, 
					   tmpLen, anvlglob->appState.udpSockInfo) ==
		  MNTCPAPP_API_RETCODE_NOTOK ) {
		// FPrintf(stderr, "Sendto failed\n");
		PError("! UDPRequestsLoop::reason");
	  }
	  break;
  

    case TCP_CMD_GET_DATA:
    
      /* Command to retrieve the data (through the UDP channel) which 
         is already received through the TCP channel */
      
      /* Add padding, if necessary */
    
      tmpLen = StrLen(anvlglob->appState.rcvdBuff) + MsgPaddingAdd(anvlglob->appState.rcvdBuff);
      
	  checksum = IPChecksum((unsigned char *)anvlglob->appState.rcvdBuff, tmpLen);
	  //checksum = IPChecksum(anvlglob->appState.rcvdBuff, tmpLen);
	  
	  IntToASCII(checksum, tmpBuf);

	  IntToASCII(tmpLen, buf); 

	  StrCat(buf, MNTCPAPP_CMD_DELIM);

	  StrCat(buf, tmpBuf);

	  StrCat(buf, MNTCPAPP_CMD_DELIM);

	  if (EFalse)
	  	{
		if (OSSockSendTo(anvlglob->appState.udpSockInfo.connSock, anvlglob->appState.rcvdBuff, 
                       tmpLen, anvlglob->appState.udpSockInfo) ==
                                          MNTCPAPP_API_RETCODE_NOTOK ) 
			{
	        PError("! UDPRequestsLoop::reason");
      		}
      	break;
	  	}
	  else
	  	{
		/* Add padding, if necessary */
	  	tmpLen = StrLen(buf) + MsgPaddingAdd(buf);

	  	if (OSSockSendTo(anvlglob->appState.udpSockInfo.connSock, buf, 
                       tmpLen, anvlglob->appState.udpSockInfo) ==
                                          MNTCPAPP_API_RETCODE_NOTOK ) 
        	{
			// FPrintf(stdout, " OSSockSendTo MNTCPAPP_API_RETCODE_NOTOK ");
      		//  FPrintf(stderr, "! Sendto failed");
        	PError("! UDPRequestsLoop::reason");
      		}
      	break;
	  	}

    case TCP_CMD_GET_CODE:
      
      /* Command to retrieve the last error code. */ 
      IntToASCII(anvlglob->appState.lastErrCode[anvlglob->appState.topCode-1], buf); 

      /* Add padding, if necessary */
      tmpLen = StrLen(buf) + MsgPaddingAdd(buf);
      
      if (OSSockSendTo(anvlglob->appState.udpSockInfo.connSock, buf, 
                       tmpLen, anvlglob->appState.udpSockInfo)  ==
                                          MNTCPAPP_API_RETCODE_NOTOK ) {
      //  FPrintf(stderr, "! Sendto failed");
        PError("! UDPRequestsLoop::reason");
      }
      else {
        --anvlglob->appState.topCode;
      }
      break;

    case TCP_CMD_GET_RETCODE:
      /* Command to retrieve the return code of the last function called. */ 
      IntToASCII(anvlglob->appState.returnCode, buf); 

      /* Add padding, if necessary */
      tmpLen = StrLen(buf) + MsgPaddingAdd(buf);
      
      if (OSSockSendTo(anvlglob->appState.udpSockInfo.connSock, buf, 
                        tmpLen, anvlglob->appState.udpSockInfo) == 
                                           MNTCPAPP_API_RETCODE_NOTOK) {
      //  FPrintf(stderr, "! Sendto failed");
        PError("! UDPRequestsLoop::reason");
      }
      break;
    
       
    case TCP_CMD_SOCK_CREATE:
      TCPSocketCreate();
      break;
    
    case TCP_CMD_ABORT:
    // tcp abortive close in symbian is shutdown(3) not linger close!
      #ifdef USEFULLDUPLEXCLOSE
      ShutDnConn(SHUT_ABORT);
      #else
      TCPAbort();
      #endif
      break;
    
    case TCP_CMD_PERROR:
      PendingErrorGet(&anvlglob->appState);
      break;
    
    case TCP_CMD_SET_MSS:
      if (command) {
        mssVal = ASCIIToInt(command);
      }
      else {
        mssVal = MNTCPAPP_DEF_MSS;
      }

      if (MSSSet(mssVal) == 0) {
      //  FPrintf(stdout, "! Could not set MSS");
      }
      break;
    
    case TCP_CMD_SET_TTL:
      if (command) {
        ttlVal = ASCIIToInt(command);
      }
      else {
        ttlVal = DEF_IP_TTL_VAL;
      }
      
      if (TTLSet(ttlVal) == 0) {
      //  FPrintf(stdout, "! Could not set TTL");
      }
      break;
      
    case TCP_CMD_SET_RCVBUF_LEN:
      if (command) {
        rcvBufVal = ASCIIToInt(command);
      }
      else {
        rcvBufVal = DEF_SOCK_RCVBUFF_LEN; 
      }
      
      if (SockRecvBuffSet(rcvBufVal) == 0) {
      //  FPrintf(stdout, "! Could not set sock recv buffer");
      }
      break;
    
    case TCP_CMD_NOTIFY_TEST_START:
      NotifyStart(command);
      break;
    
    case TCP_CMD_NOTIFY_TEST_END:
      NotifyEnd(command);
      break;
    
    case TCP_CMD_API_ACCEPT:
      IssueAccept();
      break;
            
    case TCP_CMD_OPT_STDURG:
      if (OptUrgSet() == 0) {
      //  FPrintf(stdout,"! Could not set socket option TCP_STDURG");
      }
      break;
            
    case TCP_CMD_OPT_URGINLINE:
      if (OptUrgInlineSet() == 0) {
      //  FPrintf(stdout,"! Could not set socket option SO_OOBINLINE");
      }
      break;

      /*+++king added for phase 4*/
    case TCP_CMD_SHUTDOWN_READ:
      ShutDnConn(SHUT_READ);
      break;
      
    case TCP_CMD_SHUTDOWN_WRITE:
      ShutDnConn(SHUT_WRITE);
      break;
      
    case TCP_CMD_SHUTDOWN_RD_WT:
      ShutDnConn(SHUT_RD_WT);
      break;

	case UDP_CMD_SEND:
	
	  // FPrintf(stdout, " UDP_CMD_SEND\n ");
	
	  anvlAddr = command;
	  anvlUDPPort = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	
	  // FPrintf_str(stdout, " UDP_CMD_SEND TO anvlUDPPort %s\n ",anvlUDPPort);
	  
	  dutUDPPort = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	 
	  // FPrintf_str(stdout, " UDP_CMD_SEND TO PORT dutUDPPort %s\n ",dutUDPPort);
	 
	  data = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	  dataLen = ASCIIToInt(StrTok(NULL, MNTCPAPP_CMD_DELIM));
	  MemSet(&udpDataSockInfo, '\0', sizeof(udpDataSockInfo));
  
	  /* initialize socket */
	  
	  udpDataSockInfo.connSock = -1;

	  udpDataSockInfo.anvlIPAddr = OSIPAddrStrToLong(anvlAddr); 
   
	  
	  // udpDataSockInfo.anvlPort = ASCIIToInt(anvlUDPPort);
	  // udpDataSockInfo.localPort = ASCIIToInt(dutUDPPort);
	  
	  UDPport = ASCIIToInt(anvlUDPPort);

	  // FPrintf_int(stdout, " UDP_CMD_SEND TO PORT UDPport %d\n ",UDPport);
	  
	  udpDataSockInfo.anvlPort = (unsigned short)UDPport;

	  UDPport = ASCIIToInt(dutUDPPort);

      // FPrintf_int_int(stdout," UDPport %d anvlPort %d /n", UDPport, udpDataSockInfo.anvlPort);
	  
	  udpDataSockInfo.localPort = (unsigned short)UDPport;

	  if (!UDPSend(data, dataLen, udpDataSockInfo)) {
	  // 	FPrintf(stdout, " ERROR UDP Data sent\n");
	  }
	  
	  break;

	 case UDP_CMD_SEND_REPEAT_DATA:
	  anvlAddr = command;
	  anvlUDPPort = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	  dutUDPPort = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	  data = StrTok(NULL, MNTCPAPP_CMD_DELIM);
	  dataLen = ASCIIToInt(StrTok(NULL, MNTCPAPP_CMD_DELIM));
	  
	  repeatDataBuff =  
		(char *)Malloc(dataLen * sizeof(unsigned char));
	  MemSet(repeatDataBuff, *data, dataLen);
	  MemSet(&udpDataSockInfo, '\0', sizeof(udpDataSockInfo));
  
	  /* initialize socket */
	  udpDataSockInfo.connSock = -1;
	  
	  udpDataSockInfo.anvlIPAddr = OSIPAddrStrToLong(anvlAddr); 
   
	  //udpDataSockInfo.anvlPort = ASCIIToInt(anvlUDPPort);
	  //udpDataSockInfo.localPort = ASCIIToInt(dutUDPPort);
	  
	  UDPport = ASCIIToInt(anvlUDPPort);

	  udpDataSockInfo.anvlPort = (unsigned short)UDPport;

	  UDPport = ASCIIToInt(dutUDPPort);

	  udpDataSockInfo.localPort = (unsigned short)UDPport;

	  if (!UDPSend(repeatDataBuff, dataLen, udpDataSockInfo)) {
		// FPrintf(stdout, "ERROR UDP Repeat Data sent\n");
	  }
	  break;

    default:
      // FPrintf_int(stdout, "Unknown Command :: %d", cmdId);
      break;
    } /* end switch */
    
    
  } /* infinite for */
    
  return shutdown_tcpip_stack;
} /* UDPRequestsLoop */


/*>>
    
  (int) socket = TCPConnect(char *anvlAddr, char *anvlTCPPort);
  
  DESCRIPTION: 

  This function issues a TCP active open call. If socket is not
  created then it creates the socket first and then issues a 
  nonblocking connect call on this socket. It tries to connect 
  to the address passed to this function as function arguments.
  
  ARGS:
  
  anvlAddr     - IP address of ANVL.
  anvlTCPPort  - TCP port number of ANVL.
  
  RETURNS:
  
  int          - the socket used for the connect call
    
<<*/

static int 
TCPConnect(char *anvlAddr, char *anvlTCPPort)
{

  int			TCPport;
  int           connResult;
  char          buf[MAX_STR];
  AnvlGlob      *anvlglob;

  
// #ifdef __DEBUG__
//   char             state[20];
//   PRINT_NL(stdout);
//   FPrintf(stdout, "--->In TCPConnect");
// #endif

  anvlglob = AnvlGetGlobals();
  
  TCPSocketCreate();    

  StrCpy(buf, MNTCPAPP_STR_SUCCESS);

  /* Connect to the server */
  if (StrCmp(anvlAddr, "0") == 0) {

    /*
      When anvl address is 0 we do not want to
      set this socket blocking.
    */
    anvlglob->appState.tcpSockInfo.anvlIPAddr = 0;
    anvlglob->appState.tcpSockInfo.anvlPort = 0;
  } 
  else {

    anvlglob->appState.tcpSockInfo.anvlIPAddr = OSIPAddrStrToLong(anvlAddr);
	// anvlglob->appState.tcpSockInfo.anvlPort = ASCIIToInt(anvlTCPPort);

     TCPport = ASCIIToInt((const char*)anvlTCPPort);
	        
     anvlglob->appState.tcpSockInfo.anvlPort = (unsigned short)TCPport;

    if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                    FCNTL_APPEND_NONBLOCKING) == MNTCPAPP_API_RETCODE_NOTOK) {
      // FPrintf(stderr, "! Flag setting error before connect");
      PError("! TCPConnect::reason");
    }
  }

  connResult = OSSockConnect(anvlglob->appState.tcpSockInfo.connSock, 
                             anvlglob->appState.tcpSockInfo.anvlIPAddr, 
                             anvlglob->appState.tcpSockInfo.anvlPort);
    
  if ((connResult == MNTCPAPP_API_RETCODE_NOTOK) && ConnInProgressCheck()) {
    anvlglob->appState.dutState = TCP_STATE_SYNSENT;
    // FPrintf(stdout, "TCP Connection is in progress");

// #ifdef __DEBUG__
//     DEBUGStateStrGet(anvlglob->appState.dutState, state);
//     FPrintf_str(stdout, "TCPConnect:DUT reached %s state", state);
// #endif
  }
  else if (connResult == MNTCPAPP_API_RETCODE_NOTOK) {
      
    // FPrintf(stdout, "=========================================================");
    //FPrintf(stdout, "***** ANVLTEST ConnInProgressCheck is not currently supported - error returned");
    // FPrintf(stdout, "***** ANVLTEST Connect: RETCODE_NOTOK and not ConnInProgress");
    // FPrintf(stdout, "=========================================================");

    anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo();

// #ifdef __DEBUG__
//    FPrintf_int(stdout, "Set last error code to %d", ErrNo());
// #endif

    // FPrintf(stderr, "! Connection error occurred");
    PError("! TCPConnect::reason");
    OSSockClose(anvlglob->appState.tcpSockInfo.connSock);
    anvlglob->appState.tcpSockInfo.connSock = 0;    
    anvlglob->appState.dutState = TCP_STATE_CLOSED;
    if (anvlglob->appState.tcpListenSock) {
      OSSockClose(anvlglob->appState.tcpListenSock);    
      anvlglob->appState.tcpListenSock = 0; 

// #ifdef __DEBUG__
//      FPrintf(stdout, "Closed TCP listening socket");
// #endif

      StrCpy(buf, MNTCPAPP_STR_FAIL);
    }
  }
  else {
    // FPrintf(stdout, "TCP Connection has been established");

    anvlglob->appState.dutState = TCP_STATE_ESTABLISHED;

// #ifdef __DEBUG__
//    DEBUGStateStrGet(anvlglob->appState.dutState, state);
//    FPrintf_str(stdout, "TCPConnect:DUT reached %s state", state);
//    FPrintf_int(stdout, "Set last error code to %d", ErrNo());
// #endif
  }

  SendStatusToAnvl(buf);

// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of TCPConnect");
//  PRINT_NL(stdout);
// #endif

  return(anvlglob->appState.tcpSockInfo.connSock);       
} 

/*>>
    
  (int) socket = TCPListen(char* localPort);
    
  DESCRIPTION: 

  This function issues a TCP passive open call. If socket is not
  created then it creates the socket first and then issues a 
  nonblocking bind and listen call on this socket. It listens
  on the port number passed as argument to this function.
  
  ARGS:

  localPort  - Port on which to listen.
  
  RETURNS:
  
  int        -  the socket used for the connect call
    
<<*/

static int 
TCPListen(char* localPort)
{
  char            buf[MAX_STR];
  AnvlGlob       *anvlglob;

  int	     	  port;


// #ifdef __DEBUG__
//  FPrintf(stdout, "---> In TCPListen");
//  PRINT_NL(stdout);
// #endif

  anvlglob = AnvlGetGlobals();
  #ifdef USEFULLDUPLEXCLOSE
  if (anvlglob->appState.tcpSockInfo.connSock != 0)
  	TCPClose(EFalse);
  #endif
  //if (!(anvlglob->appState.tcpSockInfo.connSock)) {
    if ((anvlglob->appState.tcpSockInfo.connSock = 
         OSSockSocket(MNTCPAPP_SOCKTYPE_STREAM)) == 
                                MNTCPAPP_API_RETCODE_NOTOK) {
      // FPrintf(stderr, "! Could not create TCP socket");
      PError("! reason");
      return MNTCPAPP_APICALL_ERR;
    }
  //}

  if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                  FCNTL_APPEND_NONBLOCKING) == MNTCPAPP_API_RETCODE_NOTOK) {
    // FPrintf(stderr, "! Flag setting error before listen");
    PError("! TCPListen::reason");
    StrCpy(buf, MNTCPAPP_STR_FAIL);
  }
  
  /* Bind it to the local address */
  
  // anvlglob->appState.tcpSockInfo.localPort = ASCIIToInt(localPort);

  	port = (int)localPort;
	port = ASCIIToInt(localPort);
	anvlglob->appState.tcpSockInfo.localPort = (unsigned short)port;

  if (OSSockBind(anvlglob->appState.tcpSockInfo.connSock, 0, 
                  anvlglob->appState.tcpSockInfo.localPort ) == 
                                         MNTCPAPP_API_RETCODE_NOTOK ) {
    // FPrintf(stderr, "! Could not bind the tcp socket to the local address");
    PError("! reason");
    anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo();

// #ifdef __DEBUG__
    // FPrintf_int(stdout, "Set last error code to %d", ErrNo());
    // FPrintf(stdout, "TCPListen:DUT reached TCP_STATE_CLOSED state");
// #endif
   
    OSSockClose(anvlglob->appState.tcpSockInfo.connSock);
    anvlglob->appState.tcpSockInfo.connSock = 0; 
    anvlglob->appState.dutState = TCP_STATE_CLOSED;
    StrCpy(buf, MNTCPAPP_STR_FAIL);
  }
  else if (OSSockListen(anvlglob->appState.tcpSockInfo.connSock, 
                        MNTCPAPP_NO_OF_CLIENT_TO_LISTEN) == 
                                            MNTCPAPP_API_RETCODE_NOTOK) {
    anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo();

// #ifdef __DEBUG__
    // FPrintf_int(stdout, "Set last error code to %d", ErrNo());
// #endif
    // FPrintf(stderr, "! Listen error occurred");
   
    PError("! reason");
    OSSockClose(anvlglob->appState.tcpSockInfo.connSock);
    anvlglob->appState.tcpSockInfo.connSock = 0; 
    StrCpy(buf, MNTCPAPP_STR_FAIL);
  }
  else {
    anvlglob->appState.dutState = TCP_STATE_LISTEN;
    StrCpy(buf, MNTCPAPP_STR_SUCCESS);
  }

  SendStatusToAnvl(buf);

// #ifdef __DEBUG__
//  FPrintf(stdout, "---> Moving out of TCPListen");
//  PRINT_NL(stdout);
// #endif
  
  return anvlglob->appState.tcpSockInfo.connSock;

}  

/*  TCPListen */

/*>
    
  (int) socket = TCPSocketCreate();
    
  DESCRIPTION: 

  This function creates a TCP socket (AN INTERNET STREAM SOCKET)
    
  RETURNS:

  int  -  the created socket
    
<*/

int 
TCPSocketCreate(void)
{
    AnvlGlob       *anvlglob;

// #ifdef __DEBUG__
//  char state[20];
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TCPSocketCreate");
// #endif

  anvlglob = AnvlGetGlobals();
  
  if (!(anvlglob->appState.tcpSockInfo.connSock)) {
    if ((anvlglob->appState.tcpSockInfo.connSock = 
         OSSockSocket(MNTCPAPP_SOCKTYPE_STREAM)) == 
                                    MNTCPAPP_API_RETCODE_NOTOK) {
      // FPrintf(stderr, "! Could not create TCP socket");
      PError("! reason");
      return MNTCPAPP_APICALL_ERR;
    }
  }

  anvlglob->appState.dutState = SOCKET_CREATED;

// #ifdef __DEBUG__
//  DEBUGStateStrGet(anvlglob->appState.dutState, state);
//  FPrintf_str(stdout, "TCPSocketCreate:DUT reached %s state", state);
//  FPrintf(stdout, "--->Moving out of TCPSocketCreate");
//  PRINT_NL(stdout);
// #endif
  
  return anvlglob->appState.tcpSockInfo.connSock;
}

/*>>
    
  (int) no_of_byte_sent = TCPSend(char *data, int dataLen, char noDelay, 
                                  char urg, noPush);
    
   DESCRIPTION: 

   This function is used to send data to anvl. The data to be send and the 
   length of the data is passed to this function. This function also
   enable / disable Naggel Algo and also can set urgent pointer.
   
   ARGS:
   
   data        -  Data to send.
   dataLen     -  Length of the data.
   noDelay     -  Flag to enable disable Naggel Algo.
   urg         -  Flag to mark this data as urgent.

    
   RETURNS:

   int         -  no of data bytes sent. In case of error this is
                  0 or negative.

<<*/

static int 
TCPSend(char *data, int dataLen, char noDelay, char urg, char noPush)
{
  int  val = 0;
  char buf[MAX_STR];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

// #ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TCPSend");
// #endif

  if (noPush) {
	/* we should only set NOPUSH if explicitly asked to do so */
	val = 1;
	if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
						 SOCKOPT_SET_NOPUSH, val, 0) == 
		MNTCPAPP_API_RETCODE_NOTOK) {
	    PError("! TCPSend::reason");
	}
  }
  
  if (noDelay) {
    val = 1;
  } 
  else {
    val = 0;
  }

  if (!noPush) {

	/* socket error occurs if NOPUSH and NODELAY both set on socket */
	if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
						 SOCKOPT_SET_NAGGLE_ALGO, val, 0) == 
		MNTCPAPP_API_RETCODE_NOTOK) {
	  // FPrintf(stderr, "! SOCKOPT_SET_NAGGLE_ALGO setsockopt error occurred\n");
	  PError("! TCPSend::reason");
	}
  }

  if (urg) {

    if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                    FCNTL_APPEND_NONBLOCKING) == MNTCPAPP_API_RETCODE_NOTOK ) 
	{
      // FPrintf(stderr, "! (urg) Flag setting error before send");
      PError("! TCPSend::reason");
    }
		
	// FPrintf(stderr, "Sending urgent data");
	
	if (anvlglob->appState.dutState != TCP_STATE_LISTEN) 
	{
		val = OSSockSend(anvlglob->appState.tcpSockInfo.connSock, data, dataLen, TRUE);
	} 
	else {
		val = OSSockSendTo(anvlglob->appState.tcpSockInfo.connSock, data, dataLen, 
					 anvlglob->appState.tcpSockInfo);
		}
	}	
	else {
		if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                    FCNTL_APPEND_NONBLOCKING) == MNTCPAPP_API_RETCODE_NOTOK) 
		{
			// FPrintf(stderr, "! Flag setting error before send");
			PError("! TCPSend::reason");
		}
		// FPrintf(stderr, "Sending normal data");

		if (anvlglob->appState.dutState != TCP_STATE_LISTEN) 
		{
			val = OSSockSend(anvlglob->appState.tcpSockInfo.connSock, data, dataLen, FALSE);
		}
		else {
			// This SendTo() should never be used with TCP sock.
			val = OSSockSendTo(anvlglob->appState.tcpSockInfo.connSock, data, dataLen,
					 anvlglob->appState.tcpSockInfo);
			}
	}

  /* +++king: BUG insert error code only if send fails */
  anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo(); 
  StrCpy(buf, MNTCPAPP_STR_SUCCESS);

// #ifdef __DEBUG__
//  if (val == MNTCPAPP_API_RETCODE_NOTOK) {
    // FPrintf_int(stdout, "Set last error code to %d", ErrNo());
    // PError("! TCPSend::reason");
//  }
// #endif

  SendStatusToAnvl(buf);


// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of TCPSend");
//  PRINT_NL(stdout);
// #endif
  
  return val;
}

/*>>
    
  (int) num_of_byte_received = TCPReceive(int expectLen);
    
  DESCRIPTION: 

  This function is used to receive data from anvl. The number of
  bytes to receive is passed as argument to this function. This 
  function calls recv (blocking) multiple time each time try to recv 
  MAX_MESG_SIZE bytes, till the accumulated received data size is 
  greater or equal to expectLen. After receiving the received data 
  is kept in the global structure appState.

    
  ARGS:

  expectLen        -  Num of bytes to read.

    
  RETURNS:

  int              -  num of data bytes received. In case of error this is
                      0 or negative.
    
<<*/

int 
TCPReceive(int expectLen)
{
  int  ret, numBytes = 0;
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();
     
// #ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TCPReceive");
// #endif

  if (expectLen > MAX_MESG_SIZE -1 ) {
  //   FPrintf(stderr, "! Expected data len exceeds buffer size.");
    return -1;
  }    

  if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                  FCNTL_APPEND_BLOCKING) == MNTCPAPP_API_RETCODE_NOTOK) {
//     FPrintf(stderr, "! Flag setting error before receive");
    PError("! TCPSend::reason");
    /* +++ even in case of error continue further */
  }

  anvlglob->tempRecv[0] = '\0';

  /* Receive bytes from ANVL upto expectLen or return -1 */
  do {

// #ifdef __DEBUG__
//    FPrintf_int(stdout, "Before a recv call to read %d bytes of data.",
//            MAX_MESG_SIZE);
// #endif

    ret = OSSockRecv(anvlglob->appState.tcpSockInfo.connSock, 
                     anvlglob->appState.rcvdBuff, MAX_MESG_SIZE);

    if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
      // FPrintf_int(stderr, "! Error %d encountered in receive", ErrNo());
      PError("! reason");
      anvlglob->appState.rcvdBuff[0] = '\0';
      anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo(); 

// #ifdef __DEBUG__
//      FPrintf_int(stdout, "Set last error code to %d", ErrNo());
//      FPrintf(stdout, "--->Moving out of TCPReceive");
//      PRINT_NL(stdout);
// #endif
   
      return -1;
    }
    else if (ret == 0) {
      // FPrintf(stdout, "! No data received");
      return -1;
    }
    
// #ifdef __DEBUG__
//    FPrintf_int(stdout, "After the recv call. Read %d bytes of data.", ret);
// #endif
   
    numBytes += ret;
    anvlglob->appState.rcvdBuff[ret] = '\0';
    if (numBytes < MAX_MESG_SIZE) {
      StrCat(anvlglob->tempRecv, anvlglob->appState.rcvdBuff);
    }
  } while (numBytes < expectLen);
    
  if (numBytes > expectLen) {
   // FPrintf_int(stderr, "! Data received is more than expected(%d bytes)",
   //         expectLen);

    if (numBytes > MAX_MESG_SIZE) {
     // FPrintf_int(stderr, "! Data received(%d bytes) is more than ", numBytes);
     // FPrintf_int(stderr, "available buffer(%d bytes)", MAX_MESG_SIZE);
     // FPrintf(stderr, "! ANVL will not be able to get correct data from "
     //         "DUT");
    
    }
  }

  if (numBytes <= MAX_MESG_SIZE) {
    StrCpy(anvlglob->appState.rcvdBuff, anvlglob->tempRecv); 
  }

// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of TCPReceive");
//  PRINT_NL(stdout);
// #endif

  return numBytes;
}

/*>> 
 
  int TCPReceiveLimSz(int expectLen)
 
  DESCRIPTION: 
  This function is used to read data of the exect size given by expectLen.
  It give recv calls repetedly till it reads expectLen bytes of data.


  The difference between TCPReceive and TCPReceiveLimSz is TCPReceive 
  tries to read data of size MAX_MESG_SIZE from the TCP receive buffer 
  till it reads data greater than or equal to expectLen. But this 
  routine reads data of size expectLen only from the TCP receive 
  buffer. Thus tcp input buffer gets freed by expectLen only.

  PARAMETERS:         
  expectLen   -   Length of data to read.
  
  RETURNS: 
  Size of the data read.
        
  NOTES:
  expectLen should be less than MAX_MESG_SIZE

<<*/ 

int 
TCPReceiveLimSz(int expectLen)
{
  int ret, numBytes = 0;
  int bytesToRead = 0;
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

// #ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TCPReceiveLimSz");
// #endif
    
  if (expectLen > MAX_MESG_SIZE-1) {
  //  FPrintf(stderr, "! Expected data len exceeds buffer size.");
    return -1;
  } 

  if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                  FCNTL_APPEND_BLOCKING) == MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stderr, "! Flag setting error before receive limit size");
    PError("! TCPSend::reason");
  }

  anvlglob->tempRecv[0] = '\0';
  bytesToRead = expectLen;
  
  /* Receive bytes from ANVL up to expectLen or return -1 */
  do {
  
// #ifdef __DEBUG__
//    FPrintf_int(stdout, "Before a recv call to read %d bytes of data.",
//            bytesToRead);
// #endif
    ret = OSSockRecv(anvlglob->appState.tcpSockInfo.connSock, anvlglob->appState.rcvdBuff, 
                     bytesToRead);

    if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
    //  FPrintf_int(stderr, "! Error %d encountered in receive", ErrNo());
      PError("! reason");
      anvlglob->appState.rcvdBuff[0] = '\0';
      anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo(); 

// #ifdef __DEBUG__
//      FPrintf_int(stdout, "Set last error code to %d", ErrNo());
// #endif
   
      return -1;
    }
    else if (ret == 0) {
    //  FPrintf(stdout, "! No data received");
      return -1;
    }

// #ifdef __DEBUG__
//    FPrintf_int(stdout, "After the recv call. Read %d bytes of data.", ret);
// #endif
   
    numBytes += ret;
    anvlglob->appState.rcvdBuff[ret] = '\0';
    StrCat(anvlglob->tempRecv, anvlglob->appState.rcvdBuff);
        
    if (numBytes < expectLen) {
      bytesToRead = expectLen - numBytes;
    }
  } while (numBytes < expectLen);
    
  if (numBytes != expectLen) {
    /* expectLen is always greater than numBytes as we have come out
       of the while loop */
   // FPrintf(stderr, "! Data received is more than expected");
    StrCpy(anvlglob->appState.rcvdBuff, anvlglob->tempRecv); 
    return -1;
  }

  StrCpy(anvlglob->appState.rcvdBuff, anvlglob->tempRecv); 

// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of TCPReceiveLimSz");
//  PRINT_NL(stdout);
// #endif

  return numBytes;
}

/*>> 
 
  int TCPReceiveOnce(int expectLen)
 
  DESCRIPTION: 
  It gives recv calls only once to read expectLen bytes of data.

  PARAMETERS:         
  expectLen   - Length of data to read.
        
  RETURNS: 
  1           - On success
  <=0         - On error
        
  NOTES:
  expectLen should be less than MAX_MESG_SIZE

<<*/
 
int 
TCPReceiveOnce(int expectLen)
{
  int ret;
  int retCode = 1;
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();
    
// #ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TCPReceiveOnce");
// #endif
    
  if (expectLen > MAX_MESG_SIZE-1) {
//  FPrintf(stderr, "! TCPReceiveOnce:Expected data len exceeds buffer "
//            "size.");
    retCode = -1;
  } 
  else if (anvlglob->appState.tcpSockInfo.connSock == 0) {
  //  FPrintf(stderr, "! TCPReceiveOnce:TCP connected socket not available.");
    retCode = -1;
  } 

  if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                  FCNTL_APPEND_BLOCKING) == MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stderr, "! Flag setting error before receive once");
    PError("! TCPReceiveOnce::reason");
    /* +++ continue further if if there is error */
  }

  anvlglob->tempRecv[0] = '\0';

  /* Receive bytes from ANVL up to expectLen or return -1 */

// #ifdef __DEBUG__
//  FPrintf_int(stdout, "Before a recv call to read %d bytes of data.",
//          expectLen);
// #endif
   
    ret = OSSockRecv(anvlglob->appState.tcpSockInfo.connSock, 
                                    anvlglob->appState.rcvdBuff, expectLen);
    if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
    //    FPrintf_int(stderr, "! Error %d encountered in receive", ErrNo());
        PError("! reason");
        anvlglob->appState.rcvdBuff[0] = '\0';
        anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo(); 

// #ifdef __DEBUG__
//        FPrintf_int(stdout, "Set last error code to %d", ErrNo());
// #endif
        retCode = MNTCPAPP_APICALL_ERR;
    }
    else if (ret == 0) {
    //  FPrintf(stdout, "! No data received");
      retCode = MNTCPAPP_APICALL_ERR;
    }
    else {
      anvlglob->appState.rcvdBuff[ret] = '\0';
      StrCat(anvlglob->tempRecv, anvlglob->appState.rcvdBuff);
      StrCpy(anvlglob->appState.rcvdBuff, anvlglob->tempRecv); 
      if (ret != expectLen) {
      //  FPrintf_int(stderr, "! Data received (%d bytes) is more or less ", ret);
      //  FPrintf_int(stderr, "than expected(%d bytes)", expectLen);
        StrCpy(anvlglob->appState.rcvdBuff, anvlglob->tempRecv); 
        retCode = ret;
      }
    }

// #ifdef __DEBUG__
//    FPrintf_int(stdout, "After the recv call. Read %d bytes of data.",
//            ret);
//    FPrintf(stdout, "--->Moving out of TCPReceiveOnce");
//    PRINT_NL(stdout);
// #endif
   
    return retCode;
}  /* TCPReceiveOnce */

/*>
 
  int TCPClose(int StatSend)
 
  DESCRIPTION: 
  This function issues the close call on the connection socket.

  ARGS:         
  statSend
        
  RETURNS: 
  1           - On success
  >=0         - On error
        
<*/ 
int 
TCPClose(int statSend)
{
  int  ret;
  char buf[MAX_STR];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();


// #ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TCPClose");
// #endif

#if 0 // ###EPOC_MODIFIED
#else
  ret = MNTCPAPP_API_RETCODE_OK;
#endif
  
  if (anvlglob->appState.tcpSockInfo.connSock) {
   // FPrintf(stdout, "TCP connection is closing");
    
    /*
      +++ we have only one reference to this connected socket.
      So calling close will send the FIN immediately
    */
    ret = OSSockClose(anvlglob->appState.tcpSockInfo.connSock);
    if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
      anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo();

// #ifdef __DEBUG__
//      FPrintf_int(stdout, "Set last error code to %d", ErrNo());
//          PError("! Reason :");
// #endif
    }
    anvlglob->appState.tcpSockInfo.connSock = 0;
  } 
  else {
  //   FPrintf(stdout, "TCP connection has already closed down");
  }
  
  if (anvlglob->appState.tcpListenSock) {
    OSSockClose(anvlglob->appState.tcpListenSock);
    anvlglob->appState.tcpListenSock = 0;

// #ifdef __DEBUG__
//    FPrintf(stdout, "Closed TCP listening socket");
// #endif
   
    }

  
  /* send a reply to ANVL */
  if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
    StrCpy(buf, MNTCPAPP_STR_FAIL);
  }
  else {
    StrCpy(buf, MNTCPAPP_STR_SUCCESS);
  }
    
  if (statSend) {
    SendStatusToAnvl(buf);
  }
  else {
   // FPrintf(stdout, "No status to send");    
  }

// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of TCPClose");
//  PRINT_NL(stdout);
// #endif

  return(anvlglob->appState.lastErrCode[anvlglob->appState.topCode - 1]);
}   /* TCPClose */

/*>
 
  int TCPCloseNoReset(void)
 
  DESCRIPTION: 
  This function issues a close call but does not reset appState.tcpSock to 0;

  ARGS:         
  statSend
        
  RETURNS: 
  1           - On success
  >=0         - On error
        
<*/
 
int 
TCPCloseNoReset(void)
{
  int  ret;
  char buf[MAX_STR];
  AnvlGlob       *anvlglob;

// #ifdef __DEBUG__
//  char state[20];
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TCPCloseNoReset");
// #endif

  anvlglob = AnvlGetGlobals();
  
#if 0 // ###EPOC_MODIFIED
#else
  ret = MNTCPAPP_API_RETCODE_OK;
#endif
  
  if (anvlglob->appState.tcpSockInfo.connSock) {
   // FPrintf(stdout, "TCP connection is closing");
        
    ret = OSSockClose(anvlglob->appState.tcpSockInfo.connSock);
    if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
      anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo();

// #ifdef __DEBUG__
//      FPrintf_int(stdout, "Set last error code to %d", ErrNo());
//          PError("! reason");
// #endif
   
    }
  } 
  else {
   // FPrintf(stdout, "TCP connection has already closed down");
  }

  if (anvlglob->appState.tcpListenSock) {
    OSSockClose(anvlglob->appState.tcpListenSock);
    anvlglob->appState.tcpListenSock = 0;

// #ifdef __DEBUG__
//    FPrintf(stdout, "Closed TCP listening socket");
// #endif
  }

  if ((anvlglob->appState.dutState == TCP_STATE_ESTABLISHED) || 
      (anvlglob->appState.dutState == TCP_STATE_SYNRCVD)) {
    anvlglob->appState.dutState = TCP_STATE_FINWAIT1;

// #ifdef __DEBUG__
//    DEBUGStateStrGet(anvlglob->appState.dutState, state);
//    FPrintf_str(stdout, "TCPCloseNoReset:DUT reached %s state", state);
// #endif
 
  }
  else if (anvlglob->appState.dutState == TCP_STATE_CLOSEWAIT) {
    anvlglob->appState.dutState = TCP_STATE_LASTACK;

// #ifdef __DEBUG__
//    DEBUGStateStrGet(anvlglob->appState.dutState, state);
//    FPrintf_str(stdout, "TCPCloseNoReset:DUT reached %s state", state);
// #endif
    }
  else if ((anvlglob->appState.dutState == TCP_STATE_SYNSENT) || 
           (anvlglob->appState.dutState == TCP_STATE_LISTEN)) {
    anvlglob->appState.dutState = TCP_STATE_CLOSED;

// #ifdef __DEBUG__
//    DEBUGStateStrGet(anvlglob->appState.dutState, state);
//    FPrintf_str(stdout, "TCPCloseNoReset:DUT reached %s state", state);
// #endif
  
  }
        

  /* send a reply to ANVL */
  if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
    StrCpy(buf, MNTCPAPP_STR_FAIL);
  }
  else {
    StrCpy(buf, MNTCPAPP_STR_SUCCESS);
  }
  
  SendStatusToAnvl(buf);

// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of TCPCloseNoReset");
//  PRINT_NL(stdout);
// #endif
  
  return(anvlglob->appState.lastErrCode[anvlglob->appState.topCode - 1]);
}            /* TCPCloseNoReset */

/*>
 
  int TCPAbort(void);
 
  DESCRIPTION: 
  This function issues the abort call on the connection socket.

  RETURNS: 
  1           - On success
  >=0         - On error
        
<*/
 
int 
TCPAbort(void)
{

  long int ret = 0;
       
  AnvlGlob       *anvlglob;
  
  anvlglob = AnvlGetGlobals();

  if (!(anvlglob->appState.tcpSockInfo.connSock)) {
    // FPrintf(stderr, "! Socket is not present. Abort call not issued");
    if (anvlglob->appState.tcpListenSock) {
      OSSockClose(anvlglob->appState.tcpListenSock);
      anvlglob->appState.tcpListenSock = 0;
    }
    return -1;
  }

  if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
                       SOCKOPT_SET_LINGER_TIME, LINGER_ON, 0) ==
                                               MNTCPAPP_API_RETCODE_NOTOK) {
    // FPrintf(stderr, "! setsockopt error occurred");
    PError("! reason");
  }

  ret = OSSockClose(anvlglob->appState.tcpSockInfo.connSock); 
  
  if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
      anvlglob->appState.returnCode = MNTCPAPP_API_RETCODE_NOTOK;
      anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo();

//    FPrintf(stderr, "! Aborting error occurred");
  
      PError("! reason");

      return MNTCPAPP_APICALL_ERR;
  }
  
  anvlglob->appState.returnCode = MNTCPAPP_API_RETCODE_OK;
    
  anvlglob->appState.tcpSockInfo.connSock = 0;
  if (anvlglob->appState.tcpListenSock) {
    OSSockClose(anvlglob->appState.tcpListenSock);
    anvlglob->appState.tcpListenSock = 0;

// #ifdef __DEBUG__
//    FPrintf(stdout, "Closed TCP listening socket");
// #endif
  }
  
  anvlglob->appState.dutState = TCP_STATE_CLOSED;

// #ifdef __DEBUG__
//  DEBUGStateStrGet(anvlglob->appState.dutState, state);
//  FPrintf_str(stdout, "DUT reached %s state", state);
//  FPrintf(stdout, "--->Moving out of TCPAbort");
//  PRINT_NL(stdout);
// #endif

/*
 RETURNS: 
  0           - On success
  < 0         - On error
  
 */
 
return 0 ;

}

/*>>
    
  (int) no_of_byte_sent = UDPSend(char *data, int dataLen,
                                  MNTCPAppSocketInfo_t sockInfo);
    
   DESCRIPTION: 

   This function is used to send udp data to anvl. The data to be send and the 
   length of the data is passed to this function. 
   
   ARGS:
   
   data        -  Data to send.
   dataLen     -  Length of the data.
   sockInfo    -  MNTCPAppSocketInfo_t structure
    
   RETURNS:

   int         -  no of data bytes sent. In case of error this is
                0 or negative.

<<*/

static int 
UDPSend(char *data, int dataLen, MNTCPAppSocketInfo_t sockInfo)
{
  int  val = 0;
  int  status;
  unsigned short idx;
  char buf[MAX_STR];

  AnvlGlob       *anvlglob;
  anvlglob = AnvlGetGlobals();

//#ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "\n--->In UDPSend \n");
//#endif
 
  /* Close the socket if there is already a valid socket descriptor */
  if (sockInfo.connSock >= 0){
    OSSockClose(sockInfo.connSock);
  }

  if ((sockInfo.connSock = OSSockSocket(MNTCPAPP_SOCKTYPE_DGRAM)) 
	  == MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stderr, "! Unable to create a UDP socket\n");
    PError("! reason");
    return MNTCPAPP_APICALL_ERR;
  }

  /* Retry UDP_SOCK_BIND_RETRIES_NUM times to bind the UDP socket */
  for (idx = 0, status = -1; status < 0 && idx < UDP_SOCK_BIND_RETRIES_NUM; 
	   idx++) {
	  status = OSSockBind(sockInfo.connSock, 0, sockInfo.localPort); 
  }

  if (status == MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stdout, "! Unable to bind the UDP socket to any local port\n");
    PError("! reason");
  }
  
  /* Send UDP data */
  if ((val = OSSockSendTo(sockInfo.connSock, data, 
						  dataLen, sockInfo)) == 
	  MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stderr, "! sendto error occurred\n");
    PError("! UDPSend::reason");
  }
  
  anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo(); 
  StrCpy(buf, MNTCPAPP_STR_SUCCESS);
  
// #ifdef __DEBUG__
//  if (val == MNTCPAPP_API_RETCODE_NOTOK) {
//	FPrintf_int(stdout, "Set last error code to %d\n", ErrNo());
//	PError("! UDPSend::reason");
//  }
// #endif

  SendStatusToAnvl(buf);
  
//#ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of UDPSend\n\n");
//  PRINT_NL(stdout);
//#endif
  
  OSSockClose(sockInfo.connSock);
  return val;
}


/*>>
  
  char *IntToASCII(long int num, char *numStr)
  
  REQUIRES: 
 
  DESCRIPTION: 
  This function converts an integer to a string. 
  
  ARGS: 
  num       -   The integer to convert
  numStr    -   String to hold the number
    
  RETURNS: 
  The converted string

<<*/

static char  
*IntToASCII(long int num, char *numStr)
{
  char         tempStr[100];
  long int     count = 0;
  long int	   temp;

  while (num) {

	//  tempStr[count++] = num % 10 + '0';
	temp = num % 10 + '0';
	tempStr[count++] = (char)temp;
    num = num / 10;
  }
 
  while (count) {
    numStr[num++] = tempStr[--count];
  } 
    
  numStr[num] = '\0';
    
  return numStr;
}  /* IntToASCII */


/*>

  (ubyte2)sum = IPChecksum(unsigned char *data, unsigned long int len);

  REQUIRES:

  DESCRIPTION:
  Computes the IP checksum for a buffer of data.
  
  ARGS:
  data pointer to the data to compute the checksum on
  len  how many bytes to compute it for

  RETURNS:
  sum the 16 bit checksum

  SIDE EFFECTS:

<*/

static unsigned short
IPChecksum(unsigned char *data, unsigned long int len)
{
  unsigned long int total;

  total = IPChecksumIncr(data, len, 0);

  /* One's complement */
  total = (~total) & 0xffff;

  return (unsigned short)total;
}

/*>

  (unsigned long int)sum = IPChecksumIncr(unsigned char *data, 
                                          unsigned long int len, 
                                          unsigned long int total);

  REQUIRES:

  DESCRIPTION:
  Increments the IP checksum for a buffer of data.
  
  ARGS:
  data  pointer to the data to compute the checksum on
  len   how many bytes to compute it for
  total current checksum total -- will be incremented

  RETURNS:
  total incremented checksum total

  SIDE EFFECTS:

<*/

static unsigned long int
IPChecksumIncr(unsigned char *data, unsigned long int len, 
			   unsigned long int total)
{
  unsigned short sum2;
  unsigned char  remainder[2] = {0, 0};
  unsigned long int sum = 0;
  boolean swap = FALSE;
  
  /*
	The algorithm has a few subtleties:

	(a) we add aligned short words, handling leading or trailing bytes
	    specially.  In particular, if there is a leading unaligned byte
	    then this introduces a byte swap that we have to undo at the end.

	(b) summing is done in host byte order, and the resulting sum is then
	    converted to network byte order.

	See RFC1071 for details.
  */

  /* Align to the first short boundary, remember to swap if we do */
  if ((len > 0) && ((long int)data & 1)) {
	remainder[1] = *data++;
	sum += *((unsigned short*)remainder);
	len--;
	swap = 1;
  }

  /* Add up all the full short words. */
  for (; len > 1; len -= 2, data += 2) {
	sum += *(unsigned short*)data;
  }

  /* Add any trailing byte */
  if (len > 0) {
	remainder[0] = *data;
	remainder[1] = 0;
	sum += *((unsigned short*)remainder);
  }
  
  /* Wrap the overflow around */
  while(sum > 0xffff){
    sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
  }

  /* swap bytes if we had an odd byte at the front */
  if (swap) {
	sum = ((sum << 8) & 0xff00) | ((sum >> 8) & 0x00ff);
  }
  
  /* convert to network byte order and add into total */
  sum2 = (unsigned short)sum;
  NetworkByteOrder((unsigned char*)&sum2, sizeof(unsigned short));

  total += sum2;
  while (total > 0xffff) {
	total = (total & 0xffff) + ((total >> 16) & 0xffff);
  }
  
  return total;
}

static void 
NetworkByteOrder(unsigned char *data, unsigned int count)
{
  unsigned char tmp;
  unsigned long int i;

  /* Swap the first half of the bytes with the last half */
  for(i = 0; i < count/2; i++){
    tmp = *(data + i);
    *(data + i) = *(data + count - i - 1);
    *(data + count - i - 1) = tmp;
  }
}

/*>>
  
  void PendingErrorGet(ApplicationState_t* appState )
    
  REQUIRES: 
  Tcp connected socket must be available in ApplicationState. It generates
  a FATAL error message if this socket is not existing.
  It also generate a fatal error message if getsockopt fails.

  DESCRIPTION: 
  This function converts an integer to a string. 
    
  ARGS: 
  appState        -   The application state data structure
  
  RETURNS: 

<<*/

static void 
PendingErrorGet(ApplicationState_t *theAppState)
{
  int                val;
  // int                valLen;
  char               buf[SMALL_BUF_LEN];
  
  // AnvlGlob       *anvlglob;

  // anvlglob = AnvlGetGlobals();

  if (theAppState->tcpSockInfo.connSock == 0) {
  //  FPrintf(stdout, "!FATAL ERROR: Try to retrive pending error without "
  //          "connected socket");
  }
    
  // valLen = sizeof(val);

  if (OSSockGetSockOpt(theAppState->tcpSockInfo.connSock, 
                       SOCKOPT_GET_ERROR, &val) == 
                                 MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stderr, "! FATAL ERROR: getsockopt error occurred");
    PError("! reason");
  }

  IntToASCII(val, buf); 

  SendStatusToAnvl(buf);


}

/*>>
 
  static int MSSSet(long int mssVal);
 
  DESCRIPTION: 
  This function sets the Maximum Segment Size on the TCP connection socket.

  RETURNS: 
  Upon failure return 0 
  otherwise value of MSS
       
<<*/ 

static int 
MSSSet(long int mssVal)
{
  long int val = mssVal;
  char     buf[SMALL_BUF_LEN];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

// #ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In MSSSet");
// #endif

  if (mssVal == 0) {
  //  FPrintf(stderr, "! MSS value given is 0");
    return 0;
  }

  if (anvlglob->appState.tcpSockInfo.connSock == 0) {
  //  FPrintf(stderr, "! TCP socket not created");
    return 0;
  }

  if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, SOCKOPT_SET_MSS, 
                       val, 0) == MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stderr, "! setsockopt error occurred");
    PError("! reason");
  }
  /* Send the value of MSS to ANVL */

  IntToASCII(val, buf); 

// #ifdef __DEBUG__
//  FPrintf_str(stdout, "Set socket MSS = %s", buf);
// #endif

  SendStatusToAnvl(buf);
  
// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of MSSSet");
//  PRINT_NL(stdout);
// #endif
  
  return mssVal;
}

/*>>
 
  static int TTLSet(long int ttlVal);
 
  DESCRIPTION: 
  This function sets the Time To Live value on the TCP connection socket.

  RETURNS: 
  Upon failure return 0 
  otherwise value of TTL
       
<<*/
 
static int 
TTLSet(long int ttlVal)
{
  int  val = ttlVal;
  char buf[SMALL_BUF_LEN];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

// #ifdef __DEBUG__
//  PRINT_NL(stdout);
//  FPrintf(stdout, "--->In TTLSet");
// #endif

  if (ttlVal == 0) {
  //  FPrintf(stderr, "! TTLSet:TTL value given is 0");
    return 0;
  }

  if (anvlglob->appState.tcpSockInfo.connSock == 0) {
  //  FPrintf(stderr, "! TTLSet: TCP socket not created");
    return 0;
  }
  
  if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, SOCKOPT_SET_TTL, 
                       val, 0) == MNTCPAPP_API_RETCODE_NOTOK) {
  //  FPrintf(stderr, "! setsockopt error occurred");
    PError("! reason");
  }
    
  /* Send the value of TTL to ANVL */
  IntToASCII(val, buf); 

// #ifdef __DEBUG__
//  FPrintf_str(stdout, "Set socket TTL = %s", buf);
// #endif

  SendStatusToAnvl(buf);

// #ifdef __DEBUG__
//  FPrintf(stdout, "--->Moving out of TTLSet");
//  PRINT_NL(stdout);
// #endif
  
  return ttlVal;
}

// #ifdef __DEBUG__

/*>>
 
  static void DEBUGMsgDecode(char* cmdMsg, char* extra);
 
  DESCRIPTION: 
  This function is used for debugging only. It prints the command received 
  from ANVL is descriptive format.

  ARGS:
  cmdMsg
  extra
       
<<*/ 


/*>>
 
  static void DEBUGStateStrGet(MNTCPAppState_t stateIndx, char *stateStr)
 
  DESCRIPTION: 
  This function is used for debugging only. It translates a TCP state
  into the appropriate string.

  ARGS:
  stateIndx
  stateStr
       
<<*/
 
// static void 
// DEBUGStateStrGet(MNTCPAppState_t stateIndx, char *stateStr)
// {

//   switch (stateIndx){
//   case TCP_STATE_CLOSED:
//     StrCpy(stateStr, "TCP_STATE_CLOSED");
//     break;
    
//   case TCP_STATE_LISTEN:
//     StrCpy(stateStr, "TCP_STATE_LISTEN");
//     break;

//   case TCP_STATE_SYNSENT:
//     StrCpy(stateStr, "TCP_STATE_SYNSENT");    
//     break;

//   case TCP_STATE_SYNRCVD:
//     StrCpy(stateStr, "TCP_STATE_SYNRCVD");    
//     break;
    
//   case TCP_STATE_ESTABLISHED:
//     StrCpy(stateStr, "TCP_STATE_ESTABLISHED");
//     break;

//   case TCP_STATE_FINWAIT1:
//     StrCpy(stateStr, "TCP_STATE_FINWAIT1");  
//     break;
    
//   case TCP_STATE_FINWAIT2:
//     StrCpy(stateStr, "TCP_STATE_FINWAIT2");  
//     break;

//   case TCP_STATE_CLOSING:
//     StrCpy(stateStr, "TCP_STATE_CLOSING");    
//     break;

//   case TCP_STATE_CLOSEWAIT:
//     StrCpy(stateStr, "TCP_STATE_CLOSEWAIT");  
//     break;

//   case TCP_STATE_LASTACK:
//     StrCpy(stateStr, "TCP_STATE_LASTACK");    
//     break;
    
//   case TCP_STATE_TIMEWAIT:
//     StrCpy(stateStr, "TCP_STATE_TIMEWAIT");
//     break;

//   case SOCKET_CREATED:
//     StrCpy(stateStr, "SOCKET_CREATED");
//     break;

//   default:
//     StrCpy(stateStr, "INVALID");
//     break;

//   }
//   return;
// }
//  #endif

/*>>
 
  static void NotifyStart(char* testNum);
 
  DESCRIPTION: 
  This function prints out a string to stdout indicating the beginning
  of a TCP test.

  ARGS:
  testNum      NULL terminated string containing the number of the current 
               test being run.
       
<<*/

static void 
NotifyStart(char* testNum) 
{
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();
  
  // FPrintf(stdout, "=========================================");
  FPrintf_str(stdout, "             START TEST %s", testNum);
  // FPrintf(stdout, "========================================="); 
  // PRINT_NL(stdout);
  
  MemSet(anvlglob->appState.lastErrCode, 0, MAX_ERR_COUNT * sizeof(int));
  anvlglob->appState.topCode = 0;
}

/*>>
 
  static void NotifyEnd(char* testNum);
 
  DESCRIPTION: 
  This function prints out a string to stdout indicating the completion
  of a TCP test.

  ARGS:
  testNum      NULL terminated string containing the number of the current 
               test being run.
       
<<*/
static void 
NotifyEnd(char* testNum) 
{
  // PRINT_NL(stdout);
  // PRINT_NL(stdout);
  // FPrintf(stdout, "=========================================");
   FPrintf_str(stdout, "              END TEST %s", testNum);
  // FPrintf(stdout, "========================================="); 
  // PRINT_NL(stdout);
}

/*>>
 
  static void IssueAccept(void);
 
  DESCRIPTION: 
  This function issues an accept call on the TCP connection socket and then 
  closes the TCP listening socket.

<<*/

static void
IssueAccept(void)
{
  int  temp;
  char buf[MAX_STR];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();
  
// #ifdef __DEBUG__
//   FPrintf(stdout, "--->In IssueAccept");
//   PRINT_NL(stdout);
// #endif

  if (OSSockFcntl(anvlglob->appState.tcpSockInfo.connSock, 
                  FCNTL_APPEND_BLOCKING) == MNTCPAPP_API_RETCODE_NOTOK) {
   //  FPrintf(stderr, "! Flag setting error before accept");
    PError("! IssueAccept::reason");
  }

  temp = OSSockAccept(anvlglob->appState.tcpSockInfo.connSock, &anvlglob->appState.tcpSockInfo);

  if (temp == MNTCPAPP_API_RETCODE_NOTOK) 
 	{
	//FPrintf(stderr, "! Could not issue accept");
	PError("! reason");
	StrCpy(buf, MNTCPAPP_STR_FAIL);
	}
  else 
	{
    StrCpy(buf, MNTCPAPP_STR_SUCCESS);
    // FPrintf(stdout, "Issued accept call successfully");

	// #ifdef __DEBUG__
	// FPrintf(stdout, "Created TCP connected socket");
	// #endif
   
    anvlglob->appState.tcpListenSock = anvlglob->appState.tcpSockInfo.connSock;
    anvlglob->appState.tcpSockInfo.connSock = temp;
	// Update DUT state.
	anvlglob->appState.dutState = TCP_STATE_ESTABLISHED;

    /* close the listening socket */ 
    if (anvlglob->appState.tcpListenSock) 
    	{
      	OSSockClose(anvlglob->appState.tcpListenSock);
      	anvlglob->appState.tcpListenSock = 0;

		// #ifdef __DEBUG__
		//       FPrintf(stdout, "Closed TCP listening socket");
		// #endif
    	}
	}

	/* send status to ANVL */
	SendStatusToAnvl(buf);

  
	// #ifdef __DEBUG__
	//   FPrintf(stdout, "--->Moving out of IssueAccept");
	//   PRINT_NL(stdout);
	// #endif
  	return;
}

/*>>
 
  static unsigned int GetCmdId(char *type, char *command)
 
  DESCRIPTION:
  This function is used to get the command id from command string
  
  ARGS:
  type             
  command

<<*/
static unsigned int 
GetCmdId(char *type, char *command)
{
  unsigned int retVal = 0;

  if (StrCmp(type, CMD_GET_VERSION_STR) == 0) {
	retVal = TCP_CMD_GET_VERSION;
  }
  else if (StrCmp(type, CMD_CONNECT_STR) == 0) {
    retVal = TCP_CMD_CONNECT;
  }
  else if (StrCmp(type, CMD_SEND_STR) == 0) {
    retVal = TCP_CMD_SEND;
  }
  else if (StrCmp(type, CMD_SEND_REPEAT_DATA_STR) == 0) {
	retVal = TCP_CMD_SEND_REPEAT_DATA;
  }
  else if (StrCmp(type, CMD_RECEIVE_STR) == 0) {
    retVal = TCP_CMD_RECEIVE;
  }
  else if (StrCmp(type, CMD_RECEIVE_LIMSZ_STR) == 0) {
    retVal = TCP_CMD_RECEIVE_LIMSZ;
  }
  else if (StrCmp(type, CMD_RECEIVE_RECEIVE_ONCE_STR) == 0) {
    retVal = TCP_CMD_RECEIVE_RECEIVE_ONCE;
  }
  else if (StrCmp(type, CMD_LISTEN_STR) == 0) { 
    retVal = TCP_CMD_LISTEN;
  }
  else if (StrCmp(type, CMD_CLOSE_STR) == 0) { 
    /*+++king renamed*/
    retVal = TCP_CMD_CLOSE;
  }
  else if (StrCmp(type, CMD_RECEIVE_CLOSE_NORST_STR) == 0) { 
    /*+++king renamed*/
    retVal = TCP_CMD_CLOSE_NORST;
  }
  else if (StrCmp(type, CMD_INFORMATION_STR) == 0) {
    
    if (StrCmp(command, "STATE") == 0) {
      retVal = TCP_CMD_SET_STATE;
    }
    else if (StrCmp(command, "ASYNC_RECV") == 0) {
      retVal = TCP_CMD_ASYNC_RECV;
    }
	else if (StrCmp(command, "GET_SAMPLE_DATA") == 0) {
	  retVal = TCP_CMD_GET_SAMPLE_DATA;
	}
    else if (StrCmp(command, "GETDATA") == 0) {
      retVal = TCP_CMD_GET_DATA;
    }
    else if (StrCmp(command, "GETCODE") == 0) {
      retVal = TCP_CMD_GET_CODE;
    }
    else if (StrCmp(command, "GETRETCODE") == 0) {
      retVal = TCP_CMD_GET_RETCODE;
    }
  } 
  else if (StrCmp(type, CMD_REBOOT_STR) == 0) {
    retVal = TCP_CMD_REBOOT;
  }
  else if (StrCmp(type, CMD_SOCK_CREATE_STR) == 0) { 
    retVal = TCP_CMD_SOCK_CREATE;
  }
  else if (StrCmp(type, CMD_ABORT_STR) == 0) { 
    retVal = TCP_CMD_ABORT;
  }
  else if (StrCmp(type, CMD_PERROR_STR) == 0) { 
    retVal = TCP_CMD_PERROR;
  }
  else if (StrCmp(type, CMD_SETMSS_STR) == 0) {
    retVal = TCP_CMD_SET_MSS;
  }
  else if (StrCmp(type, CMD_SETTTL_STR) == 0) {
    retVal = TCP_CMD_SET_TTL;
  }
  else if (StrCmp(type, CMD_SET_SOCK_RCVBUF_LEN_STR) == 0) {
    retVal = TCP_CMD_SET_RCVBUF_LEN;
  }
  else if (StrCmp(type, CMD_NOTIFY_TESTSTART_STR) == 0) {
    retVal = TCP_CMD_NOTIFY_TEST_START;
    }
  else if (StrCmp(type, CMD_NOTIFY_TESTEND_STR) == 0) {
    retVal = TCP_CMD_NOTIFY_TEST_END;
  }
  else if (StrCmp(type, CMD_API_ACCEPT_STR) == 0) {
    retVal = TCP_CMD_API_ACCEPT;
  }
  else if (StrCmp(type, CMD_URG_CORRECT_STR) == 0) {
    retVal = TCP_CMD_OPT_STDURG;
  }
  else if (StrCmp(type, CMD_URG_INLINE_STR) == 0) {
    retVal = TCP_CMD_OPT_URGINLINE;
  }
  else if (StrCmp(type, CMD_SHUTDOWN_READ_STR) == 0) {
  
    retVal = TCP_CMD_SHUTDOWN_READ;
  }
  else if (StrCmp(type, CMD_SHUTDOWN_WRITE_STR) == 0) {
    retVal = TCP_CMD_SHUTDOWN_WRITE;
  }
  else if (StrCmp(type, CMD_SHUTDOWN_RD_WT_STR) == 0) {
    retVal =   TCP_CMD_SHUTDOWN_RD_WT;
  }
  else if (StrCmp(type, CMD_SEND_UDP_STR) == 0) {
    retVal = UDP_CMD_SEND;
  }
  else if (StrCmp(type, CMD_SEND_UDP_REPEAT_DATA_STR) == 0) {
    retVal = UDP_CMD_SEND_REPEAT_DATA;
  }
  else {
    retVal = MNTCPAPP_CMD_INVALID;
  }
  return retVal;
}

/*>>
 
  static int OptUrgSet(void);
 
  DESCRIPTION:
  This function is used to set the TCP socket up to use the Urgent Pointer 
  value correction defined in RFC 1122 
  
  RETURNS
  0 to indicate error

<<*/
static int 
OptUrgSet(void)
{
  long int setVal = 0;
  char     buf[MAX_STR];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

// #ifdef __DEBUG__
//   PRINT_NL(stdout);
//   FPrintf(stdout, "--->In OptUrgSet");
// #endif

  if (anvlglob->appState.tcpSockInfo.connSock == 0){
//     FPrintf(stderr, "! TCP socket not created");

// #ifdef __DEBUG__
//     FPrintf(stdout, "--->Moving out of OptUrgSet");
//     PRINT_NL(stdout);
// #endif

    return 0;
  }

#ifdef __TCP_STDURG__
  valLen = sizeof(setVal);

  if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, SOCKOPT_SET_STDURG, 
                       val, 0) == MNTCPAPP_API_RETCODE_NOTOK) {
   //  FPrintf(stderr, "! setsockopt error occurred");
    PError("! reason");
  }
  else if (OSSockGetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
                            SOCKOPT_GET_STDURG, &setVal) == 
                                        MNTCPAPP_API_RETCODE_NOTOK) {
  //   FPrintf(stderr, "! FATAL ERROR: getsockopt error occurred");
    PError("! reason");
  }
#else
  setVal = 0;
 //  FPrintf(stderr, "! OptUrgSet: socket option TCP_STDURG not ");
 //  FPrintf(stderr, "suported by DUT");
#endif

  /* get the socket option and send a reply to ANVL */
  if (setVal == 0) {
#ifdef __TCP_STDURG__
    // FPrintf(stderr, "! OptUrgSet: could not set TCP_STDURG");
#endif
    StrCpy(buf, MNTCPAPP_STR_FAIL);
  }
  else {
    // FPrintf(stdout, "Set TCP_STDURG");
    StrCpy(buf, MNTCPAPP_STR_SUCCESS);
  }

  SendStatusToAnvl(buf);
  
// #ifdef __DEBUG__
//   FPrintf(stdout, "--->Moving out of OptUrgSet");
//   PRINT_NL(stdout);
// #endif

  return setVal;
}

/*>>
 
  static int OptUrgInlineSet(void);
 
  DESCRIPTION:
  
  RETURNS:
  0 to indicate error

<<*/
static int 
OptUrgInlineSet(void)
{
  int val = 1;
  int setVal = 0;
  // int setValSize;
  char buf[MAX_STR];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

  // setValSize = sizeof(setVal);

  if (anvlglob->appState.tcpSockInfo.connSock == 0){
  //   FPrintf(stderr, "! TCP socket not created");


    return 0;
  }

  if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
                       SOCKOPT_SET_OOB_DATA_INLINE, val, 0) == 
                                          MNTCPAPP_API_RETCODE_NOTOK) {
  //   FPrintf(stderr, "! setsockopt error occurred");
    PError("! reason");
  }
  else if (OSSockGetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
                            SOCKOPT_GET_OOB_DATA_INLINE, &setVal) == 
                                            MNTCPAPP_API_RETCODE_NOTOK) {
    // FPrintf(stderr, "! FATAL ERROR: getsockopt error occurred");
    PError("! reason");
  }
  else {
    /* get the socket option and send a reply to ANVL */

    if (setVal == 0){
    //   FPrintf(stderr, "! Could not set SO_OOBINLINE");
      StrCpy(buf, MNTCPAPP_STR_FAIL);
    }
    else {
    //   FPrintf(stdout, "Set SO_OOBINLINE");
      StrCpy(buf, MNTCPAPP_STR_SUCCESS);
    }
  }

  SendStatusToAnvl(buf);

// #ifdef __DEBUG__
//   FPrintf(stdout, "--->Moving out of OptUrgInlineSet");
//   PRINT_NL(stdout);
// #endif

  return setVal;
}

/*>>
 
  static int SockRecvBuffSet(long int rcvbuflen);
 
  DESCRIPTION:
  This function sets the receive buffer length of the TCP connection
  socket.  

  RETURNS:
  0 to indicate error

<<*/

static int 
SockRecvBuffSet(long int rcvbuflen)
{
  int  val = 0;
  // int  valLen;
  char buf[SMALL_BUF_LEN];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

  if (rcvbuflen == 0) {
  //   FPrintf(stderr, "! SockRecvBuffSet: buffer value given is 0");
    return 0;
  }

  if (anvlglob->appState.tcpSockInfo.connSock == 0) {
  //   FPrintf(stderr, "! SockRecvBuffSet: TCP socket not created");
    return 0;
  }

  val = rcvbuflen;

  if (OSSockSetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
                       SOCKOPT_SET_RECV_BUF_SIZE, val, 0) ==
                                            MNTCPAPP_API_RETCODE_NOTOK) {
  //   FPrintf(stderr, "! setsockopt error occurred");
    PError("! reason");
  }

  // valLen = sizeof(val);

  /* Get the value of buffer to check if has been set with proper value */
  if (OSSockGetSockOpt(anvlglob->appState.tcpSockInfo.connSock, 
                       SOCKOPT_GET_RECV_BUF, &val) == 
                                     MNTCPAPP_API_RETCODE_NOTOK) {
  //     FPrintf(stderr, "! FATAL ERROR: getsockopt error occurred");
      PError("! reason");
  }
  if (val != rcvbuflen) {
  //   FPrintf_int(stderr, "! SockRecvBuffSet: Unable to set socket recv buffer "
  //           "of %ld bytes", rcvbuflen);
  }

  /* Send the value of buffer to ANVL */
  IntToASCII(val, buf); 


  SendStatusToAnvl(buf);

// #ifdef __DEBUG__
//   FPrintf_str(stdout, "Set socket receive buffer size = %s", buf);
//   FPrintf(stdout, "--->Moving out of SockRecvBuffSet");
//   PRINT_NL(stdout);
// #endif

  return val;
}

/*>>
 
  static void SendStatusToAnvl(char *msg);
 
  DESCRIPTION:
  This function sends a status message over the UDP socket.  

  ARGS:
  msg      should be null terminated string 

<<*/

static void 
SendStatusToAnvl(char *msg)
{
  char msgCopy[MAX_STR];
  long int msgLen;
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();
  

  StrCpy(msgCopy, msg);
  /* add padding, if necessary */
  msgLen = StrLen(msgCopy) + MsgPaddingAdd(msgCopy);

  if (OSSockSendTo(anvlglob->appState.udpSockInfo.connSock, msgCopy, 
                   msgLen, anvlglob->appState.udpSockInfo) == 
                                      MNTCPAPP_API_RETCODE_NOTOK ) {
  //   FPrintf(stderr, "! Sendto failed");
    PError("! SendStatusToAnvl::reason");
  }
  
// #ifdef __DEBUG__
//   else {
  //  FPrintf_str(stdout, "Sending [%s] status to ANVL", msg); 
//   }
// #endif
  
  return;
}

/*>>
 
  static int StateSet(long int state);
 
  DESCRIPTION:
  This function prints out the current TCP state to stdout.  

  ARGS:
  state      the current TCP state

<<*/

// static int 
// StateSet(long int state)
// {

static int 
StateSet(int state)
{
//   char stateStr[MAX_STR];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();

  /* +++king add some range checking on the value of state */

// #ifdef __DEBUG__
//   FPrintf(stdout, "--->In Set State");
//   PRINT_NL(stdout);
// #endif
  
  if (state == TCP_STATE_CLOSED) {

// #ifdef __DEBUG__
//     FPrintf(stdout, "Calling TCPClose");
// #endif
   
    TCPClose(FALSE);
  }

  anvlglob->appState.dutState = (MNTCPAppState_t)state;
  
// #ifdef __DEBUG__
//   DEBUGStateStrGet(anvlglob->appState.dutState, stateStr);
//   FPrintf_str(stdout, "DUT reached %s state", stateStr);
// #endif
  
  SendStatusToAnvl(MNTCPAPP_STR_SUCCESS);

// #ifdef __DEBUG__
//   FPrintf(stdout, "--->Moving out of Set State");
//   PRINT_NL(stdout);
// #endif

  return 1;
}

/*>
 
  int ShutDnConn(ShutDownSock_t type)
 
  DESCRIPTION: 
  This function issues the shutdown call on the connection socket.
  It can shutdown read/write od both end

  ARGS:         
  statSend
        
  RETURNS: 
  MNTCPAPP_API_RETCODE_OK            - On success
  MNTCPAPP_API_RETCODE_NOTOK         - If API fail
  ERR_INVALID_ARG                    - Invalide type
        
<*/ 
int 
ShutDnConn(ShutDownSock_t type)
{
  int  ret;
  char buf[MAX_STR];
  AnvlGlob       *anvlglob;

  anvlglob = AnvlGetGlobals();


// #ifdef __DEBUG__
//     PRINT_NL(stdout);
//     FPrintf(stdout, "--->In ShutDnConn");
// #endif

#if 0 // ###EPOC_MODIFIED
#else
    ret = MNTCPAPP_API_RETCODE_OK;
#endif
    
    if (anvlglob->appState.tcpSockInfo.connSock) 
    {
    
        switch (type) {
        
        case   SHUT_READ:
            // FPrintf(stdout, "Closing read end of the connection");
            break;
        
        case SHUT_WRITE:
            // FPrintf(stdout, "Closing write end of the connection");
            break;
        
        case SHUT_RD_WT:
            // FPrintf(stdout, "Closing read and write end of the connection");
            break;

#ifdef USEFULLDUPLEXCLOSE
        case SHUT_ABORT:
        	break;
#endif
        default:
            // FPrintf_int(stdout, "! Invalid shutdown type %d", type);
            // FPrintf(stdout, "! Shutdown call not issued");
            return ERR_INVALID_ARG;
        }


        anvlglob->appState.returnCode = 
              OSSockShutDn(anvlglob->appState.tcpSockInfo.connSock, type);

        ret = anvlglob->appState.returnCode;

        if (anvlglob->appState.returnCode == MNTCPAPP_API_RETCODE_NOTOK) {
            anvlglob->appState.lastErrCode[anvlglob->appState.topCode++] = ErrNo();

// #ifdef __DEBUG__
//                 FPrintf_int(stdout, "Set last error code to %d", ErrNo());
// #endif
//              FPrintf(stdout, "! Shutdown failed");
        }
        else {
            // FPrintf(stdout, "Shutdown successfully");
        }
        /*+++king       appState.tcpSockInfo.connSock = 0; */
    } 
    else {
        // FPrintf(stdout, "TCP connection has already closed down");
    }
  
    if (anvlglob->appState.tcpListenSock) {
        OSSockClose(anvlglob->appState.tcpListenSock);
        anvlglob->appState.tcpListenSock = 0;

// #ifdef __DEBUG__
//         FPrintf(stdout, "Closed TCP listening socket");
// #endif
   
    }

  
    /* send a reply to ANVL */
    if (ret == MNTCPAPP_API_RETCODE_NOTOK) {
        StrCpy(buf, MNTCPAPP_STR_FAIL);
    }
    else {
        StrCpy(buf, MNTCPAPP_STR_SUCCESS);
    }
    
    SendStatusToAnvl(buf);

// #ifdef __DEBUG__
//   FPrintf(stdout, "--->Moving out of ShutDnConn");
//   PRINT_NL(stdout);
// #endif
    
    return anvlglob->appState.returnCode;
}   /* ShutDnConn */

/*>>
 
  static unsigned int MsgPaddingAdd(char *msg);
 
  DESCRIPTION: 
  This function adds padding to the msg so that the length of the data
  is big enough to prevent the message from getting stuck in buffers
  for being "too small". See note for MNTCPAPP_UDP_MIN_DATA_LEN. If
  the msg is bigger than the minimum size, then no padding is added.

  ARGS:         
  msg      pointer to string buffer containing message to be padded
        
  RETURNS: 
  paddingLen    number of bytes of padding added to end of msg
        
<<*/
 
static unsigned int
MsgPaddingAdd(char *msg)
{
  char paddingLen = 0, *paddingPtr, i;
  long int msgLen = 0;
  long int pLen;

  msgLen = StrLen(msg);
  if (msgLen < MNTCPAPP_UDP_MIN_DATA_LEN) {
    /* calculate number of bytes of padding to add */
    
	  //paddingLen = MNTCPAPP_UDP_MIN_DATA_LEN - msgLen;
	  pLen = MNTCPAPP_UDP_MIN_DATA_LEN - msgLen;
   
	  paddingPtr = msg + msgLen;

    /* set length of message to minimum data length */
    msgLen = MNTCPAPP_UDP_MIN_DATA_LEN;
    
	//for (i = 0; i < paddingLen; i++) {
    
	for (i = 0; i < pLen; i++) {
		*(paddingPtr++) = MNTCPAPP_PADDING_CHAR;
    }
    /* null-terminate new string */
    *paddingPtr = '\0';
  }
  else {
    /* data is already the correct size, so no need to pad */
  }
  return paddingLen;
}

/*>>
 
  static unsigned int MsgPaddingRemove(char *msg);
 
  DESCRIPTION: 
  This function removes any padding from a msg. Note that
  MNTCPAPP_PADDING_CHAR should not be used within a message.

  ARGS:         
  msg      pointer to string containing message
        
  RETURNS: 
  paddingLen    number of bytes of padding removed from message
        
<<*/ 

static unsigned int
MsgPaddingRemove(char *msg)
{
  //char paddingLen = 0, *paddingPtr;
  char *paddingPtr;
  long int msgLen = 0;
  unsigned int pLen = 0;

  msgLen = StrLen(msg);
  
  paddingPtr = StrChr(msg, MNTCPAPP_PADDING_CHAR);
  
  
  if (paddingPtr) {
    /* +++dhwong: or do I need a bigger variable? */
  
	//paddingLen = msg + msgLen - paddingPtr;
      pLen = msg + msgLen - paddingPtr;
	/* chop off padding by null terminating string before it */
    *paddingPtr = '\0';
  }
  else {
    /* no padding was found */
  }

  // return paddingLen;
  
  return pLen;
  
}


/*>>
 
  static void AppVersionGet(void);
 
  DESCRIPTION: 
  This function notifies ANVL of Stub Application version.

<<*/

static void
AppVersionGet(void)
{
  char          buf[MAX_STR];
  unsigned char versionMajor = MNTCPAPP_VERSION_MAJOR;
  unsigned char versionMinor = MNTCPAPP_VERSION_MINOR;

// #ifdef __DEBUG__
// 	PRINT_NL(stdout);
// 	FPrintf(stdout, "--->In AppVersionGet\n\n");
// #endif

  /* Send version stamp to ANVL */
  SPrintf(buf, "%d#%d#", versionMajor, versionMinor); 

  SendStatusToAnvl(buf);
    
// #ifdef __DEBUG__
// 	FPrintf(stdout, "--->Moving out of AppVersionGet\n\n");
//   	PRINT_NL(stdout);
// #endif
  
  return;
}

