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
// This file contains header for OS-specific functions called from
// mntcpapp.c  The function bodies are in a different file for each
// OS implemented.  (For example mntcplnx.c for Linux) 
//




#ifndef __MNTCPOS_H__
#define __MNTCPOS_H__

#include <stdio.h>

#define __MNTCPAPP_LINUX__

#ifdef __MNTCPAPP_LINUX__
    #define MNTCPAPP_OS_SOCKET      int
    #define FALSE                      0

    #ifndef __cplusplus
    #ifndef TRUE
        #define TRUE                       !(FALSE)
    #endif
    #endif

    typedef int boolean;
#endif

/* Dclare socket for Win NT */
#ifdef __MNTCPAPP_WINNT__
    #include <winsock2.h>
    #define MNTCPAPP_OS_SOCKET      SOCKET
#endif

#define MNTCPAPP_SOCKTYPE_STREAM   1       /* type for Socket */
#define MNTCPAPP_SOCKTYPE_DGRAM    2       /* type for Socket */ 

#define LINGER_OFF                 0      /* linger on close if data present */
#define LINGER_ON                  1

            
struct MNTCPAppSocketInfo_s {
  /* all are in host byte order */
  unsigned long int anvlIPAddr;
  unsigned short anvlPort;
  unsigned long int localIPAddr;
  unsigned short localPort;

  int connSock;
};
typedef struct MNTCPAppSocketInfo_s MNTCPAppSocketInfo_t;

enum FCntl_e {
  FCNTL_ILLEGAL = 0,

  /* these commands overwrite the existing flags */
  FCNTL_SET_NONBLOCKING,
  FCNTL_SET_BLOCKING,

  /* these commands append to the existing flags */
  FCNTL_APPEND_NONBLOCKING,
  FCNTL_APPEND_BLOCKING
};
typedef enum FCntl_e FCntl_t;

enum SockOpt_e {
  SOCKOPT_ILLEGAL = 0,

  /* The following are of type SOL_SOCKET */
  SOCKOPT_SET_LINGER_TIME,
  SOCKOPT_SET_OOB_DATA_INLINE,
  SOCKOPT_SET_RECV_BUF_SIZE,
  SOCKOPT_SET_SEND_BUF_SIZE,

  /* The followings are of type IPPROTO_TCP */
  SOCKOPT_SET_NAGGLE_ALGO,
  SOCKOPT_SET_MSS,
  SOCKOPT_SET_STDURG,
  SOCKOPT_SET_NOPUSH,

  /* The followings are of type IPPROTO_IP */
  SOCKOPT_SET_TTL,
  SOCKOPT_GET_ERROR,
  SOCKOPT_GET_RECV_BUF,
  SOCKOPT_GET_STDURG,
  SOCKOPT_GET_OOB_DATA_INLINE,

  SOCKOPT_SET_MD5,
  SOCKOPT_GET_MD5
};
typedef enum SockOpt_e SockOpt_t;

#ifdef __cplusplus
extern "C" {
#endif

void PErrorCalled(void); // added in EPOC

int OSSockClose(MNTCPAPP_OS_SOCKET sock);
int OSSockBind(MNTCPAPP_OS_SOCKET sock, unsigned long int srcAddr, 
                      unsigned short srcPort);
MNTCPAPP_OS_SOCKET OSSockSocket(unsigned short sType);
int OSSockSendTo(MNTCPAPP_OS_SOCKET sock, const void *msg, int magLen,
                        MNTCPAppSocketInfo_t dst);
int OSSockRecvFrom(MNTCPAPP_OS_SOCKET sock, void *buff, int bufflen,
                          MNTCPAppSocketInfo_t *from);
int OSSockConnect(MNTCPAPP_OS_SOCKET sock, unsigned long int dstIPAddr, 
                         unsigned short dstPort);
MNTCPAPP_OS_SOCKET OSSockAccept(MNTCPAPP_OS_SOCKET sock, 
                                         MNTCPAppSocketInfo_t *nameInfo);
int OSSockFcntl(MNTCPAPP_OS_SOCKET sock, unsigned short cmd);
int OSSockListen(MNTCPAPP_OS_SOCKET sock, int backlog);
int OSSockSend(MNTCPAPP_OS_SOCKET sock, char *buff, unsigned int nbytes,
                      boolean urgent);
int OSSockRecv(MNTCPAPP_OS_SOCKET sock, char *buff, unsigned int nbytes);
int OSSockSetSockOpt(MNTCPAPP_OS_SOCKET sock, SockOpt_t opt, int value1, int value2);
int OSSockGetSockOpt(MNTCPAPP_OS_SOCKET sock, SockOpt_t opt, int *value);
int OSSockShutDn(int sock, int type);

size_t StrLen(const char *str);
int StrCmp(const char *str1, const char *str2);
char *StrCpy(char *dst, const char *src);
char *StrCat(char *dest, const char *src);
char *StrTok(char *str, const char *delim);
char *StrChr(const char *str, char c);

void Exit(int code);

//extern int FPrintf(FILE *fp, const char *format, ...);
int FPrintf(FILE *fp, const char *format);
int FPrintf_int(FILE *fp, const char *format, int par1);
int FPrintf_str(FILE *fp, const char *format, char *par1);
int FPrintf_int_int(FILE *fp, const char *format, int par1, int par2);
int FPrintf_str_str(FILE *fp, const char *format, char *par1, char *par2);
int FPrintf_str_int(FILE *fp, const char *format, char *par1, int par2);

int SPrintf(char *str, const char *format, ...);

int IsDigit(int c);
void *MemSet(void *buff, int value, size_t length);
void PError(const char *usrDesc);
unsigned long OSIPAddrStrToLong(char *ipAddr);
int ErrNo(void);
boolean ConnInProgressCheck(void);
int ASCIIToInt(const char *ascii);

void *Malloc(unsigned int size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif


#endif /*! __MNTCPOS_H__ */
