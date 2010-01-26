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
//

#include <e32std.h>

#include "anvlsock.h"
#include "anvlglob.h"

#include <in6_opt.h> 

void CAnvlSockMain::InitL(void)
    {

    User::LeaveIfError(iSocketServer.Connect());

    iOwnServer.ConstructL(1, FALSE);

    }

void CAnvlSockMain::Close(void)
    {
    iOwnServer.Close();
    }

RSocketServ &CAnvlSockMain::SocketServer(void)
    {
    return iSocketServer;
    }

void CAnvlSockMain::PrintInt(TInt number)
    {
    iOwnServer.PrintInt(number);
    }

void CAnvlSockMain::PrintStr(char *str)
    {
    iOwnServer.PrintStr(str);
    }

void CAnvlSockMain::PrintfStr(char *format, char *str)
    {
    iOwnServer.PrintfStr(format, str);
    }

void CAnvlSockMain::PrintfInt(char *format, TInt number)
    {
    iOwnServer.PrintfInt(format, number);
    }

    
CAnvlSockMain *GetSockMain(void)
    {
    AnvlGlob *anvlglob;
    anvlglob = (AnvlGlob*)User::LeaveIfNull(AnvlGetGlobalsPlusPlus());
    return (CAnvlSockMain*)anvlglob->anvl_main_blk;
    }


void PErrorCalled(void)
    {
    CAnvlSockMain *sockMain = GetSockMain();
    sockMain->PrintStr("-PError called");
    }
    
void PrintErrorSocketIsZero(char *str)
    {
    CAnvlSockMain *sockMain = GetSockMain();
    FPrintf(stdout, "=========================================================");
    FPrintf_str(stdout, "***** ANVLTEST error: socket is zero in %s", str);
    FPrintf(stdout, "=========================================================");
    sockMain->PrintfStr("-socket is 0 in %s", str);
    }
    
void CAnvlSock::PrintErrorSetSockOpt(char *str)
    {
    CAnvlSockMain *sockMain = GetSockMain();
    FPrintf(stdout, "=========================================================");
    FPrintf_str(stdout, "***** ANVLTEST error: %s SetSockOpt option not supported under EPOC", str);
    FPrintf(stdout, "=========================================================");
    sockMain->PrintfStr("-%s SetSockOpt not supported", str);
    }
    
void CAnvlSock::PrintErrorGetSockOpt(char *str)
    {
    CAnvlSockMain *sockMain = GetSockMain();
    FPrintf(stdout, "=========================================================");
    FPrintf_str(stdout, "***** ANVLTEST error: %s GetSockOpt option not supported under EPOC", str);
    FPrintf(stdout, "=========================================================");
    sockMain->PrintfStr("-%s GetSockOpt not supported", str);
    }
    
void CAnvlSock::AllocCheckSockValue(void)
    {
    FPrintf_int(stdout, "ANVLTEST socket created: %x", (TUint)this);
    iCheckSockValue = ANVLSOCK_CHECK_SOCK_VALUE_IN_USE;
    }
    
TBool CAnvlSock::ReleaseCheckSockValue(void)
    {
    CAnvlSockMain *sockMain = GetSockMain();
    if (iCheckSockValue == ANVLSOCK_CHECK_SOCK_VALUE_IN_USE)
        {
        iCheckSockValue = ANVLSOCK_CHECK_SOCK_VALUE_RELEASED;
        FPrintf_int(stdout, "ANVLTEST socket closed: %x", (TUint)this);
        return TRUE;
        }
    else
        {
        if (iCheckSockValue == ANVLSOCK_CHECK_SOCK_VALUE_RELEASED)
            {
            FPrintf(stdout, "=========================================================");
            FPrintf_int(stdout, "***** ANVLTEST socket close error: already closed: %x", (TUint)this);
            FPrintf(stdout, "=========================================================");
            sockMain->PrintfInt("-socket already closed: %x", (TUint)this);
            }
        else
            {
            FPrintf(stdout, "=========================================================");
            FPrintf_int(stdout, "***** ANVLTEST socket close error: invalid socket: %x", (TUint)this);
            FPrintf(stdout, "=========================================================");
            sockMain->PrintfInt("-socket already closed or invalid: %x", (TUint)this);
            }
        return FALSE;
        }
    }
    
TBool CAnvlSock::TestCheckSockValueOK(void)
    {
    CAnvlSockMain *sockMain = GetSockMain();
    if (iCheckSockValue == ANVLSOCK_CHECK_SOCK_VALUE_IN_USE)
        return TRUE;
    else
        {
        if (iCheckSockValue == ANVLSOCK_CHECK_SOCK_VALUE_RELEASED)
            {
            FPrintf(stdout, "=========================================================");
            FPrintf_int(stdout, "*****ANVLTEST error: invalid socket (already closed): %x", (TUint)this);
            FPrintf(stdout, "=========================================================");
            sockMain->PrintfInt("-invalid socket: %x", (TUint)this);
            }
        else
            {
            FPrintf(stdout, "=========================================================");
            FPrintf_int(stdout, "*****ANVLTEST error: invalid socket: %x", (TUint)this);
            FPrintf(stdout, "=========================================================");
            sockMain->PrintfInt("-invalid socket: %x", (TUint)this);
            }
        return FALSE;
        }
    }
    
CAnvlSock *CAnvlSock::OpenBlankAnvlSocketL(void)
    {
    TInt        ret;
    CAnvlSock   *sock;

    sock = new (ELeave) CAnvlSock;
    sock->SetSockMain();

    ret = sock->iSock.Open(sock->iSockMain->SocketServer());

    if (ret != KErrNone)
        {
        //User::Leave(KErrGeneral);
        delete sock;
        return NULL;
        }
    else
        {
        return sock;
        }
    }

void CAnvlSock::SetSockMain(void)
    {
    AnvlGlob *anvlglob;
    anvlglob = (AnvlGlob*)User::LeaveIfNull(AnvlGetGlobalsPlusPlus());
    iSockMain = (CAnvlSockMain*)anvlglob->anvl_main_blk;
    AllocCheckSockValue();
    }


void CAnvlSock::CheckL(TInt aValue, TInt aCheckValue, TInt /*aLineNumber*/)
    {
    if (aValue != aCheckValue)
        {
        User::Leave(KErrGeneral);
        }
    }

/*>
    
  (int) closed = OSSockClose(int sock);
  
  DESCRIPTION: 
  This function closes a socket decriptor.

  ARGS:
  sock              the socket descriptor to be closed

  RETURNS
  closed            zero on success, or -1 if an error occurred. 
  
<*/
int 
OSSockClose(int sock)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockClose");
        return MNTCPAPP_API_RETCODE_NOTOK;    
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
    return ((CAnvlSock*)sock)->OSSockClose();
    }

int CAnvlSock::OSSockClose(void)
    {
    if (!ReleaseCheckSockValue())
        return MNTCPAPP_API_RETCODE_NOTOK;    
    iSock.Close();
    delete this;
    return MNTCPAPP_API_RETCODE_OK;    
    }

/*>
    
  (int) bound = OSSockBind(int sock, unsigned long int srcAddr, 
                           unsigned short srcPort);
  
  DESCRIPTION: 
  This function assigns a name to a socket decriptor.

  ARGS:
  sock              the socket descriptor to be bound
  srcAddr           local address 
  srcPort           local port

  RETURNS
  bound             zero on success, or -1 if an error occurred. 
  
<*/
int 
OSSockBind(int sock, unsigned long int srcAddr, unsigned short srcPort)   
    {
    int i, status;

    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockBind");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }

    /* Retry TCP_SOCK_BIND_RETRIES_NUM times to bind the TCP socket */
    for (i = 0, status = MNTCPAPP_API_RETCODE_NOTOK;
         status == MNTCPAPP_API_RETCODE_NOTOK && i < TCP_SOCK_BIND_RETRIES_NUM; i++)
        {
        status = ((CAnvlSock*)sock)->OSSockBind(srcAddr, srcPort);   
        }

    return status;
    }

int CAnvlSock::OSSockBind(unsigned long int srcAddr, unsigned short srcPort)   
    {
    TUint32 addrPar;
    TInt    ret;

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    if (srcAddr)
        addrPar = srcAddr;
    else
        addrPar = KInetAddrAny;

    TInetAddr addr(addrPar, srcPort);

    ret = iSock.Bind(addr);

    if (ret == KErrNone)
        {     
        return MNTCPAPP_API_RETCODE_OK;
        }
    else
        {
        FPrintf_int(stdout, "socket Bind failed: %d", ret);
        return MNTCPAPP_API_RETCODE_NOTOK;
        }
    }

/*>
    
  (int) sockFD = OSSockSocket(unsigned short sType);
  
  DESCRIPTION: 
  This function creates an endpoint for communication and returns 
  a socket decriptor.

  ARGS:
  sType         the type of socket to create   

  RETURNS
  sockFD        the socket descriptor for created endpoint
                or -1 upon error
  
<*/

MNTCPAPP_OS_SOCKET   
OSSockSocket(unsigned short sType)
    {
    CAnvlSock   *sock;
    sock = new (ELeave) CAnvlSock;
    sock->SetSockMain();
    return sock->OSSockSocketL(sType);
    }

MNTCPAPP_OS_SOCKET   
CAnvlSock::OSSockSocketL(unsigned short sType)
    {
    TInt ret = 0;
    
    switch (sType)
        {
        case MNTCPAPP_SOCKTYPE_STREAM:
         
            ret = iSock.Open(iSockMain->SocketServer(), KAfInet, KSockStream, KProtocolInetTcp);
            break;
    
        case MNTCPAPP_SOCKTYPE_DGRAM:
         
            ret = iSock.Open(iSockMain->SocketServer(),KAfInet,KSockDatagram, KProtocolInetUdp);
            break;
    
        default:
        	
            // FPrintf(stdout, "Invalid Socket type requested");
            ret  = -1;
            break;
        }

    if (ret != KErrNone)
        {
        return MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        return (MNTCPAPP_OS_SOCKET)this;
        }
    }


/*>
    
  (int) numBytes = OSSockSendTo(int sock, const void *msg, int msgLen, 
                                MNTCPAppSocketInfo_t dst);
  
  DESCRIPTION: 
  This function is used to transmit a message to another socket.

  ARGS:
  sock             local socket descriptor
  msg              message to be sent
  msgLen           length of message
  dst              information on destination socket

  RETURNS
  
<*/
int 
OSSockSendTo(int sock, const void *msg, int msgLen, MNTCPAppSocketInfo_t dst) 
    { 
         
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockSendTo");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }

    return ((CAnvlSock*)sock)->OSSockSendTo(msg, msgLen, dst);
    } 

int CAnvlSock::OSSockSendTo(const void *msg, int msgLen, MNTCPAppSocketInfo_t dst) 
    {
    int                 retCode = 0;
    TInetAddr          addr(dst.anvlIPAddr, dst.anvlPort);
    TSockXfrLength      bytesSent;
    TPtrC8              buffer((const TUint8*)msg, msgLen);

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    
    
    iSock.SendTo(buffer, addr, 0, iReqStatus, bytesSent);

    User::WaitForRequest(iReqStatus);

    if (iReqStatus.Int() != KErrNone)
        {
        	
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        if (bytesSent.Length() == 0)
            {
            retCode = MNTCPAPP_API_RETCODE_NOTOK;
            }
        else
            {
            retCode = MNTCPAPP_API_RETCODE_OK;
            }
        }

    return retCode;
    } 


/*>
    
  (int) numBytes = OSSockRecvFrom(int sock, void *buff, int bufflen, 
                                  MNTCPAppSocketInfo_t *from);
  
  DESCRIPTION: 
  This function is used to receive incoming data from another socket.

  ARGS:
  sock             local socket descriptor
  buff             buffer to fill with incoming data
  buffLen          length of the buffer
  from             information on source socket sending us data

  RETURNS
  numBytes         number of bytes read or -1 upon error
  
<*/
int 
OSSockRecvFrom(int sock, void *buff, int bufflen, MNTCPAppSocketInfo_t *from) 
    { 
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockRecvFrom");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }

    return ((CAnvlSock*)sock)->OSSockRecvFrom(buff, bufflen, from);
    } 

int CAnvlSock::OSSockRecvFrom(void *buff, int bufflen, MNTCPAppSocketInfo_t *from) 
    {
    int             retCode;
    TInetAddr      addr;
    TPtr8           buffer((TUint8*)buff, bufflen, bufflen);

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    iSock.RecvFrom(buffer, addr, 0, iReqStatus);

    User::WaitForRequest(iReqStatus);

    if (iReqStatus.Int() != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = buffer.Length();
        from->anvlIPAddr = addr.Address();
        from->anvlPort = (unsigned short)addr.Port();
        
        }

    return retCode;
    } 

/*>
    
  (int) connected = OSSockConnect(int sock, unsigned long int dstIPAddr, 
                                  unsigned short dstPort);

  DESCRIPTION: 
  This function creates a connection between a local and remote socket.

  ARGS:
  sock             local socket descriptor
  dstIPAddr        IP Address of remote socket
  dstPort          port value of remote socket

  RETURNS
  connected        MNTCPAPP_API_RETCODE_OK             upon success
                   MNTCPAPP_API_RETCODE_NOTOK          upon error
  
<*/
int 
OSSockConnect(int sock, unsigned long int dstIPAddr, unsigned short dstPort)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockConnect");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
    return ((CAnvlSock*)sock)->OSSockConnect(dstIPAddr, dstPort);
    }

int CAnvlSock::OSSockConnect(unsigned long int dstIPAddr, unsigned short dstPort)
    {
    int                 retCode;
    TInetAddr          addr(dstIPAddr, dstPort);
    AnvlGlob            *anvlglob;

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    iSock.Connect(addr, iReqStatus);

    User::WaitForRequest(iReqStatus);

    anvlglob = (AnvlGlob*)User::LeaveIfNull(AnvlGetGlobalsPlusPlus());
    anvlglob->sock_connect_last_status = iReqStatus.Int();
    
    if (iReqStatus.Int() != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = MNTCPAPP_API_RETCODE_OK;
        }
    return retCode;
    }


/*>
    
  (int) acceptedSock = int OSSockAccept(int sock, 
                                        MNTCPAppSocketInfo_t *nameInfo);

  DESCRIPTION: 
  This function accepts a connection request that comes in on the
  listening socket and creates a new socket desciptor for the accepted
  request. 

  ARGS:
  sock             socket descriptor of listening socket
  nameInfo         name information for accepted socket

  RETURNS
  acceptedSock     socket descriptor of accepted socket
                   MNTCPAPP_API_RETCODE_NOTOK upon error
  
<*/
MNTCPAPP_OS_SOCKET
OSSockAccept(MNTCPAPP_OS_SOCKET sock, MNTCPAppSocketInfo_t *nameInfo)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockAccept");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
    return ((CAnvlSock*)sock)->OSSockAccept(nameInfo);
    }

MNTCPAPP_OS_SOCKET CAnvlSock::OSSockAccept(MNTCPAppSocketInfo_t *nameInfo)
    {
    CAnvlSock           *blankAnvlSocket;
    int                 retCode;

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    if ((blankAnvlSocket = CAnvlSock::OpenBlankAnvlSocketL()) == NULL)
        {
        return MNTCPAPP_API_RETCODE_NOTOK;
        }

    iSock.Accept(blankAnvlSocket->iSock, iReqStatus);

    User::WaitForRequest(iReqStatus);

    if (iReqStatus.Int() != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        blankAnvlSocket->OSSockClose();
        }
    else
        {
        retCode = (MNTCPAPP_OS_SOCKET)blankAnvlSocket;
        TInetAddr      addr;
        blankAnvlSocket->iSock.RemoteName(addr);

        nameInfo->anvlIPAddr = addr.Address();
        nameInfo->anvlPort = (unsigned short)addr.Port();
        }
    return retCode;
    }



/*>
   
  (int) retVal = OSSockFcntl(int sock, unsigned short cmd);

  DESCRIPTION: 
  This function performs the operations on a socket file descriptor.
  The following operations are supported.  In addition some operations
  are supported in combination with others.

  O_NONBLOCK,     ~O_NONBLOCK
  FASYNC,         ~FASYNC
  MSG_OOB

  ARGS:
  sock             local socket file descriptor
  cmd              the operation to perform on file descriptor

  RETURNS
  retVal           MNTCPAPP_API_RETCODE_NOTOK upon error
                   MNTCPAPP_API_RETCODE_OK    on success

<*/
int
OSSockFcntl(int sock, unsigned short cmd)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockFcntl");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
    return ((CAnvlSock*)sock)->OSSockFcntl(cmd);
    }

int CAnvlSock::OSSockFcntl(unsigned short cmd)
    {
    TInt                 retCode;
 
    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    switch (cmd)
        {
        case FCNTL_SET_NONBLOCKING:
            retCode = iSock.SetOpt(KSONonBlockingIO, KSOLSocket, 0);
            break;

        case FCNTL_SET_BLOCKING:
            retCode = iSock.SetOpt(KSOBlockingIO, KSOLSocket, 0);
            break;
    
        case FCNTL_APPEND_NONBLOCKING:
            retCode = iSock.SetOpt(KSONonBlockingIO, KSOLSocket, 0);
            break;

        case FCNTL_APPEND_BLOCKING:
            retCode = iSock.SetOpt(KSOBlockingIO, KSOLSocket, 0);
            break;

        default:
            FPrintf(stderr, "! Invalid Fcntl command");
            return MNTCPAPP_API_RETCODE_NOTOK;
        }
  
    if (retCode != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = MNTCPAPP_API_RETCODE_OK;
        }
    return retCode;
    }

/*>
    
  (int) success = OSSockListen(int sock, int backlog);

  DESCRIPTION: 
  This function is used to indicate a willingness to accept incoming 
  connections.

  ARGS:
  sock             local socket descriptor
  backlog          limit on queue for incoming connections

  RETURNS
  success          MNTCPAPP_API_RETCODE_OK upon success
                   MNTCPAPP_API_RETCODE_NOTOK upon error
  
<*/
int
OSSockListen(int sock, int backlog)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockListen");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
 
    return ((CAnvlSock*)sock)->OSSockListen(backlog);
    }

int CAnvlSock::OSSockListen(int backlog)
    {
    TInt                 retCode;

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    retCode = iSock.Listen(backlog);

    if (retCode != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = MNTCPAPP_API_RETCODE_OK;
        }
    return retCode;
    }

/*>
  (int) retVal = OSSockSetSockOpt(int sock, SockOpt_t opt, int value1, 
                                  int value2);

  DESCRIPTION:
  This function is used to set the following socket options:

  SOL_SOCKET, SO_LINGER,    (int, int)
  SOL_SOCKET, SO_OOBINLINE  (int)
  SOL_SOCKET, SO_RCVBUF     (int)          done

  IPPROTO_TCP, TCP_NODELAY  (int)          done
  IPPROTO_TCP, TCP_MAXSEG   (int)
  IPPROTO_TCP, TCP_STDURG   (int)

  IPPROTO_IP, IP_TTL        (int)

  ARGS:
  sock             local socket descriptor
  opt              socket option to be set
  value1           value to set the option to
  value2           used for SO_LINGER 

  RETURNS:
  retVal          MNTCPAPP_API_RETCODE_OK    upon success
                  MNTCPAPP_API_RETCODE_NOTOK upon failure
<*/
int 
OSSockSetSockOpt(int sock, SockOpt_t opt, int value1, int value2)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockSetSockOpt");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
    return ((CAnvlSock*)sock)->OSSockSetSockOpt(opt, value1, value2);
    }

int CAnvlSock::OSSockSetSockOpt(SockOpt_t opt, int value1, int value2)
    {
    TInt                 retCode;
        
    TPckgBuf<TSoTcpLingerOpt> ling;
    		
    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    switch (opt)
        {
      
        case SOCKOPT_SET_LINGER_TIME:
           		
			ling().iOnOff = value1;
			
			ling().iLinger =  value2;
		
			retCode = iSock.SetOpt(KSoTcpLinger, KSolInetTcp, ling);
					 
            break;
            
        case SOCKOPT_SET_OOB_DATA_INLINE:
            retCode = iSock.SetOpt(KSoTcpOobInline, KSolInetTcp, value1);
            break;
            
        case SOCKOPT_SET_RECV_BUF_SIZE:
            {
            TUint   bufSize = (TUint)value1;
            TPckgBuf<TUint> optBufSizeOld;
            TPckgBuf<TUint> optBufSize(bufSize);

            if (iSock.GetOpt(KSORecvBuf, KSOLSocket, optBufSizeOld) != KErrNone)
                {
                FPrintf(stdout, "! Could not get current buffer size");
                PError("!OSSockSetSockOpt : reason");
                return MNTCPAPP_API_RETCODE_NOTOK; 
                }
            FPrintf_int_int(stdout, "recv buffer size was %d, changing to %d",
                    optBufSizeOld(), optBufSize());
            
            retCode = iSock.SetOpt(KSORecvBuf, KSOLSocket, optBufSize);
            }
            break;
            
        case SOCKOPT_SET_SEND_BUF_SIZE:
            {
            TUint   bufSize = (TUint)value1;
            TPckgBuf<TUint> optBufSizeOld;
            TPckgBuf<TUint> optBufSize(bufSize);

            if (iSock.GetOpt(KSOSendBuf, KSOLSocket, optBufSizeOld) != KErrNone)
                {
                FPrintf(stdout, "! Could not get current buffer size");
                PError("!OSSockSetSockOpt : reason");
                return MNTCPAPP_API_RETCODE_NOTOK; 
                }
            FPrintf_int_int(stdout, "recv buffer size was %d, changing to %d",
                    optBufSizeOld(), optBufSize());
            
            retCode = iSock.SetOpt(KSOSendBuf, KSOLSocket, optBufSize);
            }
            break;
            
        case SOCKOPT_SET_NAGGLE_ALGO:
            retCode = iSock.SetOpt(KSoTcpNoDelay, KSolInetTcp, value1);
            break;
            
        case SOCKOPT_SET_MSS:
            retCode = iSock.SetOpt(KSoTcpMaxSegSize, KSolInetTcp, value1);
            break;
            
#ifdef __TCP_STDURG__
        case SOCKOPT_SET_STDURG:
            PrintErrorSetSockOpt("STDURG");
            return MNTCPAPP_API_RETCODE_NOTOK;
            break;
#endif
		
		case SOCKOPT_SET_NOPUSH:
				
			retCode = iSock.SetOpt(KSoTcpCork, KSolInetTcp, value1);
		
			break;


        case SOCKOPT_SET_TTL:
            retCode = iSock.SetOpt(KSoIpTTL, KSolInetIp, value1);
            break;
            
        case SOCKOPT_SET_MD5:
            PrintErrorSetSockOpt("MD5");
            return MNTCPAPP_API_RETCODE_NOTOK;
            // break;
            
        default:
            PrintErrorSetSockOpt("INVALID");
            return MNTCPAPP_API_RETCODE_NOTOK;
            // break;
        }
    
    
    if (retCode != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = MNTCPAPP_API_RETCODE_OK;
        }
    return retCode;
    }

/*>
    
  (int) numBytes = OSSockSend(int sock, char *buff, unsigned int nBytes,
                              boolean urgent);

  DESCRIPTION: 
  This function transmits a message to another socket.

  ARGS:
  sock              local socket descriptor
  buff              buffer containing message to be sent
  nBytes            length of message
  urgent            whether or not message should be sent as 
                    out of band data
  RETURNS
  numBytes          number of characters sent( > 0), otherwise
                    MNTCPAPP_API_RETCODE_NOTOK upon error
  
<*/


int   
OSSockSend(int sock, char *buff, unsigned int nBytes, boolean urgent)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockSend");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }

    return ((CAnvlSock*)sock)->OSSockSend(buff, nBytes, urgent);
    }

int CAnvlSock::OSSockSend(char *buff, unsigned int nBytes, boolean urgent)
    {
    int                 retCode = 0;
    TSockXfrLength      bytesSent;
    TPtrC8              buffer((const TUint8*)buff, nBytes);

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    if (urgent)
        {
        iSock.Send(buffer, KSockWriteUrgent, iReqStatus, bytesSent);
        }
    else
        {
        iSock.Send(buffer, 0, iReqStatus, bytesSent);
        }

    User::WaitForRequest(iReqStatus);

    if (iReqStatus.Int() != KErrNone)
        {
		
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        
        }
    else
        {
        if (bytesSent.Length() == 0)
            {
            retCode = MNTCPAPP_API_RETCODE_NOTOK;
            }
        else
            {
            retCode = bytesSent.Length();
            }
        }

    return retCode;
    }


/*>
    
  (int) numBytes = int OSSockRecv(int sock, char *buff, unsigned int nbytes);

  DESCRIPTION: 
  This function receives a message from a connected socket.

  ARGS:
  sock              local socket descriptor
  buff              buffer where message data should be filled in
  nBytes            length of buffer

  RETURNS
  numBytes          number of characters read, otherwise
                   MNTCPAPP_API_RETCODE_NOTOK upon error
  
<*/
int   
OSSockRecv(int sock, char *buff, unsigned int nBytes)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockRecv");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
        
    return ((CAnvlSock*)sock)->OSSockRecv(buff, nBytes);
    }

int CAnvlSock::OSSockRecv(char *buff, unsigned int nBytes)
    {
    TSockXfrLength  bytesRcvd;
    int             retCode;
    TPtr8           buffer((TUint8*)buff, nBytes, nBytes);
    
    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    //iSock.Recv(buffer, 0, iReqStatus);
    iSock.RecvOneOrMore(buffer, 0, iReqStatus, bytesRcvd);

    User::WaitForRequest(iReqStatus);

    if (iReqStatus.Int() != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = buffer.Length();	
        }

    return retCode;
    }

/*>
  (int) retVal = OSSockGetSockOpt(int sock, SockOpt_t opt, int *value);

  DESCRIPTION:
  This function is used to retrieve the following socket options:

  SOL_SOCKET, SO_ERROR
  SOL_SOCKET, SO_OOBINLINE
  SOL_SOCKET, SO_RCVBUF

  IPPROTO_TCP, TCP_STDURG

  ARGS:
  sock             local socket descriptor
  opt              socket option whose value will be retrieved
  value            location to store returned value

  RETURNS:
  retVal           MNTCPAPP_API_RETCODE_OK     upon success
                   MNTCPAPP_API_RETCODE_NOTOK  upon error

<*/
int 
OSSockGetSockOpt(int sock, SockOpt_t opt, int *value)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockGetSockOpt");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
    return ((CAnvlSock*)sock)->OSSockGetSockOpt(opt, value);
    }

int CAnvlSock::OSSockGetSockOpt(SockOpt_t opt, int *value)
    {
    TInt                 retCode;

    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    switch (opt)
        {
        case SOCKOPT_GET_ERROR:
            retCode = iSock.GetOpt(KSOSelectLastError, KSOLSocket, *value);
            break;
  
        case SOCKOPT_GET_RECV_BUF:
            {
            TPckgBuf<TUint> optBufSize;
            if ((retCode = iSock.GetOpt(KSORecvBuf, KSOLSocket, optBufSize)) != KErrNone)
                {
                *value = optBufSize();
                }
            }
            break;
  
        case SOCKOPT_GET_OOB_DATA_INLINE:
            retCode = iSock.GetOpt(KSoTcpOobInline, KSolInetTcp, *value);
            break;
  
#ifdef __TCP_STDURG__
        case SOCKOPT_GET_STDURG:
            PrintErrorGetSockOpt("STDURG");
            return MNTCPAPP_API_RETCODE_NOTOK;
            break;
#endif

        case SOCKOPT_GET_MD5:
            PrintErrorGetSockOpt("MD5");
            return MNTCPAPP_API_RETCODE_NOTOK;
            // break;

        default:
            PrintErrorGetSockOpt("INVALID");
            return MNTCPAPP_API_RETCODE_NOTOK;
            // break;
        }
    
    if (retCode != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = MNTCPAPP_API_RETCODE_OK;
        }
    return retCode;
    }

/*---------------------------------------------------------------------------*/
/*+++king added*/
int
OSSockShutDn(int sock, int type)
    {
    if (sock == 0)
        {
        PrintErrorSocketIsZero("OSSockShutDn");
        return MNTCPAPP_API_RETCODE_NOTOK;
        //User::Leave(KErrGeneral);
        //Exit(1);
        }
    return ((CAnvlSock*)sock)->OSSockShutDn(type);
    }

int CAnvlSock::OSSockShutDn(int type)
    {
    int                 retCode;
    RSocket::TShutdown  how;
    
    if (!TestCheckSockValueOK())
        return MNTCPAPP_API_RETCODE_NOTOK;    

    switch (type)
        {
        case  SHUT_READ:
            how = RSocket::EStopInput;
            break;
    
        case SHUT_WRITE:
            how = RSocket::EStopOutput;
            break;
    
        case SHUT_RD_WT:
            how = RSocket::ENormal;
            break;
#ifdef USEFULLDUPLEXCLOSE
        case SHUT_ABORT:
            how = RSocket::EImmediate;
        	break;
#endif            
        default:
            how = RSocket::EImmediate;
            break;
        }
  
    iSock.Shutdown(how, iReqStatus);

    User::WaitForRequest(iReqStatus);

    if (iReqStatus.Int() != KErrNone)
        {
        retCode = MNTCPAPP_API_RETCODE_NOTOK;
        }
    else
        {
        retCode = MNTCPAPP_API_RETCODE_OK;
        }
    return retCode;
    }

/*>

  (boolean) inProgress = ConnInProgressCheck(void);

  DESCRIPTION:
  This function determines whether or not the connection is currently 
  in progress or not.

  RETURNS:
  inProgress          TRUE if connection is in progress
                      FALSE otherwise
<*/
boolean ConnInProgressCheck(void)
    {
#if 0 // ###EPOC_MODIFIED
    return (errno == EINPROGRESS) ? TRUE : FALSE;
#else
    AnvlGlob *anvlglob;
    anvlglob = (AnvlGlob*)User::LeaveIfNull(AnvlGetGlobalsPlusPlus());
    if (anvlglob->sock_connect_last_status == KErrWouldBlock)
        {
        return TRUE;
        }
    else
        {
        return FALSE;
        }
    //return FALSE;
#endif  
    }

