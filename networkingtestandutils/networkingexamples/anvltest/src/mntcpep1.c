/*
* Copyright (c) 2007 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


/* This file contains function definitions (wrappers) for linux-specifc
   calls made from mntcpapp.c. */

#if 0 // ###EPOC_MODIFIED

/* System Includes */
#include <stdarg.h>
#include "errno.h"
#include "fcntl.h"

#include "sys/types.h"
#include "sys/socket.h"
#include "net/if.h"
#include "netinet/in.h"
#include "netinet/tcp.h"

/* Local Includes */ 
#include "include/mntcpos.h"
#include "include/mntcpapp.h"

#else

/* System Includes */
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


/* Local Includes */ 
#include "mntcpos.h"
#include "mntcpapp.h"

#endif


#if 0 // ###EPOC_MODIFIED
#else

    #include "anvlmain.h"
    #include "anvlglob.h"

#endif



/* 
   Prototypes of system functions we use (from /usr/include/sys).
   They are included here so that we know what they are and if they 
   change.  
*/

//extern int   close(int sock);
//extern int   socket(int family, int type, int protocol);
//extern int   bind(int, const struct sockaddr *, int);
//extern int   connect(int sock, struct sockaddr *name, int namelen);
//extern int   setsockopt(int sock, int level, int optname, const char *optval, 
//                        int optlen);
//extern int   getsockopt(int sock, int level, int optname, char *optval, 
//                        int *optlen);
//extern int   getsockname(int sock, struct sockaddr *name, int *namelen);
//extern int   shutdown(int sock, int howto);
//extern int   accept(int sock, struct sockaddr *peer, int *addlen);
//extern int   listen(int sock, int backlog);
extern char  *inet_ntoa(struct in_addr inaddr);


//extern int shutdown(int s, int how);
extern void exit(int status);
extern int fprintf(FILE *stream, const char *format, ... );
extern size_t strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);
extern char *strcpy(char *dest, const char *src);
extern char *strcat(char *dest, const char *src);

//extern int socket(int domain, int type, int protocol);
extern unsigned long int inet_addr(const char *cp);
//extern int fcntl(int fd, int cmd, ...);
extern int atoi(const char *nptr);
extern  int atoi(const char *nptr);
extern char *strtok(char *s, const char *delim);
extern char *strchr (const char *, int);
extern int isdigit (int c);
extern void *memset(void *s, int c, size_t n);
extern  void perror(const char *s);
extern void *malloc(size_t size);
extern void free(void *ptr);

/* Wrapped Functions */

/*>
    
  void Exit(int code);
  
  DESCRIPTION: 
  This function terminates the calling process immediately.
  
  ARGS:
  code          process exit status
    
<*/
void 
Exit(int code)
{
  exit(code);
}

/*>
    
  (size_t) strLen = StrLen(const char *str);
  
  DESCRIPTION: 
  This function calculates the length of the string, not including the 
  terminating `\0' character.
  
  ARGS:
  str          string whose length will be calculated

  RETURNS:
  strLen       number of characters in str
    
<*/
size_t
StrLen(const char *str)
{
  return strlen(str);
}

/*>
    
  (int) numChar = Fprintf (FILE *fp, const char *format, ...);
  
  DESCRIPTION: 
  This function writes output to the given output stream
  
  ARGS:
  fp         file pointer
  format     the string format to output to the stream

  RETURNS:
  numChar    number of characters printed
    
<*/
#if 0 // ###EPOC_MODIFIED
int FPrintf(FILE *fp, const char *format, ...)
{
  va_list argPtr;
  int status;

  va_start(argPtr, format);
  status = vfprintf(fp, format, argPtr);
  va_end(argPtr);
  return status;
}

#else

int FPrintf(FILE *fp, const char *format)
{
    AnvlGlob *anvlglob;
    anvlglob = AnvlGetGlobals();
    if (anvlglob->pfile == NULL) {
        return -1;
    }
    fprintf(anvlglob->pfile, format);
    fprintf(anvlglob->pfile, "\n");
    fflush(anvlglob->pfile);
	(void)fp; // to avoid Code Warrior compiler warnings
    return 0;
    
}

int FPrintf_int(FILE *fp, const char *format, int par1)
{
    AnvlGlob *anvlglob;
    anvlglob = AnvlGetGlobals();
    if (anvlglob->pfile == NULL) {
        return -1;
    }
    fprintf(anvlglob->pfile, format, par1);
    fprintf(anvlglob->pfile, "\n");
    fflush(anvlglob->pfile);
	(void)fp; // to avoid Code Warrior compiler warnings
    return 0;
    
}

int FPrintf_str(FILE *fp, const char *format, char *par1)
{
    AnvlGlob *anvlglob;
    anvlglob = AnvlGetGlobals();
    if (anvlglob->pfile == NULL) {
        return -1;
    }
    fprintf(anvlglob->pfile, format, par1);
    fprintf(anvlglob->pfile, "\n");
    fflush(anvlglob->pfile);
	(void)fp; // to avoid Code Warrior compiler warnings
    return 0;
    
}

int FPrintf_int_int(FILE *fp, const char *format, int par1, int par2)
{
    AnvlGlob *anvlglob;
    anvlglob = AnvlGetGlobals();
    if (anvlglob->pfile == NULL) {
        return -1;
    }
    fprintf(anvlglob->pfile, format, par1, par2);
    fprintf(anvlglob->pfile, "\n");
    fflush(anvlglob->pfile);
	(void)fp; // to avoid Code Warrior compiler warnings
    return 0;
    
}

int FPrintf_str_str(FILE *fp, const char *format, char *par1, char *par2)
{
    AnvlGlob *anvlglob;
    anvlglob = AnvlGetGlobals();
    if (anvlglob->pfile == NULL) {
        return -1;
    }
    fprintf(anvlglob->pfile, format, par1, par2);
    fprintf(anvlglob->pfile, "\n");
    fflush(anvlglob->pfile);
	(void)fp; // to avoid Code Warrior compiler warnings
    return 0;
    
}

int FPrintf_str_int(FILE *fp, const char *format, char *par1, int par2)
{
    AnvlGlob *anvlglob;
    anvlglob = AnvlGetGlobals();
    if (anvlglob->pfile == NULL) {
        return -1;
    }
    fprintf(anvlglob->pfile, format, par1, par2);
    fprintf(anvlglob->pfile, "\n");
    fflush(anvlglob->pfile);
	(void)fp; // to avoid Code Warrior compiler warnings
    return 0;
    
}
#endif


/*>
    
  (int) compare =  StrCmp(const char *str1, const char* str2);
  
  DESCRIPTION: 
  This function compares the two strings str1 and str2. It returns an integer 
  less than, equal to, or greater than zero if str1 is found, respectively, to 
  be less than, to match, or be greater than str2.
  
  ARGS:
  str1          strings whose values are to be compared
  str2

  RETURNS:
  compare       integer less than, equal to, or greater than zero
  
<*/
int
StrCmp(const char *str1, const char* str2)
{
  return strcmp(str1,str2);
}

/*>
    
  (char *) dstPtr = StrCpy(char *dst, const char* src);
  
  DESCRIPTION: 
  This function copies the string pointed to be src (including the terminating
  `\0' character) to the array pointed to by dest.
  
  ARGS:
  dst           string whose value will be copied to
  src           string whose value will be copied from

  RETURNS:
  dstPtr        pointer to dest string after copy
  
<*/
char *
StrCpy(char *dst, const char* src)
{
  return strcpy(dst, src);
}


/*>
    
  (char *) dstPtr =  StrCat(char *dest, const char *src);
  
  DESCRIPTION: 
  This function appends the src string to the dest string, overwriting the 
  `\0' character at the end of dest, and then adds a terminating `\0' character.
  
  ARGS:
  dest          string whose value will be appended
  src           string whose value will be used to append to src
  
  RETURNS:
  dstPtr        result string after append

<*/
char *
StrCat(char *dest, const char *src)
{
  return strcat(dest, src);
}

char *
StrChr(const char *str, char c)
{
  return strchr(str, c);
}

/*>
    
  (int) isDigit = IsDigit(int c);
  
  DESCRIPTION: 
  This function checks whether c is a digit (0 through 9).

  ARGS:
  c             value to be checked

  RETURNS
  isDigit       nonzero if c is a digit
                otherwise zero
  
<*/
int 
IsDigit(int c)
{ 
  return isdigit(c); 
}

/*>
    
  void *MemSet(void *buff, int value, size_t length);
  
  DESCRIPTION: 
  This function fills the first length bytes of the memory area pointed to
  by buff with the constant value.

  ARGS:
  buff                   buffer to fill
  value                  value to repeat into buffer
  length                 number of bytes to fill in
  
<*/
void *
MemSet(void *buff, int value, size_t length)
{
  return (void *)memset(buff, value, length);
}

/*>
    
  void PError(const char *usrDesc);
  
  DESCRIPTION: 
  This function produces a message on the standard error output, describing the
  last error that occured.

  ARGS:
  usrDesc            description prepended to the error message
  
<*/
#if 0 // ###EPOC_MODIFIED
void 
PError(const char *usrDesc)
{
  perror(usrDesc);  
}
#else
void 
PError(const char *usrDesc)
{
  char buf[200];
  char text[] =":PERROR";
  unsigned lth;
  AnvlGlob *anvlglob;

  PErrorCalled();

  anvlglob = AnvlGetGlobals();
  if (anvlglob->pfile == NULL) {
        return;
  }
  lth = strlen(usrDesc);
  if ((lth+sizeof(text)+5) > sizeof(buf)) {
      fprintf(anvlglob->pfile, usrDesc);
      fprintf(anvlglob->pfile, "\n");
      fflush(anvlglob->pfile);
  }
  else {
      strcpy(buf, usrDesc);
      strcat(buf, text);
      lth = strlen(buf);
      fprintf(anvlglob->pfile, buf);
      fprintf(anvlglob->pfile, "\n");
      fflush(anvlglob->pfile);
  }
}
#endif
  

/*>
  (int) errorNum = ErrNo(void);

  DESCRIPTION:
  This function is used to retrieve the error code from the last error.

  RETURNS:
  errorNum         error code number

<*/
int 
ErrNo(void)
{
  return errno;
}

/*>

  (boolean) value = ASCIIToInt(const char *ascii);

  DESCRIPTION:
  This function converts the string pointed to by ascii to an integer.

  ARGS:
  ascii        the ASCII character to convert to integer

  RETURNS:
  value        the converted value
  
<*/
int 
ASCIIToInt(const char *ascii)
{
  return atoi(ascii);
}

/*>

  (char *) token = StrTok(char *str, const char *delim);

  DESCRIPTION:
  This function creates a nonempty string of characters up to, but not
  including a delimiter string.  This function is used to parse str
  into these nonempty strings called tokens.

  ARGS:
  str          string to be parsed
  delim        delimiter string

  RETURNS:
  token        pointer to next token
               NULL if no more tokens exist
  
<*/
char *
StrTok(char *str, const char *delim)
{
  return strtok(str, delim);
}
/*>
    
  (unsigned long) addr = OSIPAddrStrToLong(char *ipAddr);

  DESCRIPTION: 
  This function converts an IP address to host byte order.

  ARGS:
  ipAddr             IP address to be put in host byte order

  RETURNS
  addr               IP address in host byte order
  
<*/
unsigned long OSIPAddrStrToLong(char *ipAddr)
{
  unsigned long addr;
  if (!ipAddr) {
    FPrintf(stdout, "NULL address given programming error");
    Exit(-1);
  }
    
  addr = inet_addr(ipAddr);

  return ntohl(addr); 
}


/*>
    
  (int) numChar = SPrintf (char *str, const char *format, ...);
  
  DESCRIPTION: 
  This function writes output to the given string
  
  ARGS:
  fp         file pointer
  format     the string format to output to the string

  RETURNS:
  numChar    number of characters printed
    
<*/
int
SPrintf(char *str, const char *format, ...)
{
  va_list argPtr;
  int status;
  
  va_start(argPtr, format);
  status = vsprintf(str, format, argPtr);
  va_end(argPtr);
  return status;
}

/*>

  (void *) ptr = Malloc(unsigned int size);

  DESCRIPTION:
  This function returns a pointer to memory allocated by the OS.

  ARGS:
  size         size of memory block to be returned

  RETURNS:
  ptr          pointer to newly allocated memory

<*/
void *
Malloc(unsigned int size)
{
  return malloc(size);
}


/*>

  (void) Free(void *ptr);

  DESCRIPTION:
  This function returns a pointer to memory allocated by the OS.

  ARGS:
  ptr          pointer to memory to be freed

  RETURNS:
  NONE

<*/

// void
// Free(void *ptr)
// {
//  return (void *)free(ptr);
// }
