/*****************************************************************************
*       Copyright (C) 1993-2011, FS Consulting LLC. All rights reserved      *
*                                                                            *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.                   *
*                                                                            *
*  This software is the proprietary and confidential property                *
*  of FS Consulting LLC.                                                     *
*****************************************************************************/


/*

    Module:     posix.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    Header file for posix.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_POSIX_H)
#define UTL_POSIX_H


/*---------------------------------------------------------------------------*/


/*
** Check the generic defines, these should be set in Makefile:
**
**    http://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
**
*/

/* Enable gnu source */
#if !defined(_GNU_SOURCE)
#error "_GNU_SOURCE was not defined"
#endif    /* !defined(_GNU_SOURCE) */


/* Enable reentrant functions */
#if !defined(_REENTRANT)
#error "_REENTRANT was not defined"
#endif    /* !defined(_REENTRANT) */


/*---------------------------------------------------------------------------*/


/*
** Set up architecture specific defines
*/

/* Solaris  */
#if defined(__sun__)
#endif    /* defined(__sun__) */


/* Linux */
#if defined(linux)
#endif    /* defined(linux) */


/* Mac OS X */
#if defined(__APPLE__) && defined(__MACH__)
#endif /* defined(__APPLE__) && defined(__MACH__) */


/*---------------------------------------------------------------------------*/


/*
** C includes (POSIX)
*/

/* #include <assert.h> */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <grp.h>
#include <limits.h>
#include <math.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <locale.h>
#include <iconv.h>
#include <wchar.h>
#include <wctype.h>

#if defined(__sun__)
#include <widec.h>
#endif    /* defined(__sun__) */

/* For sockets support */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

/* For signal support */
#include <signal.h>

#if defined(TRE_REGEX_ENABLE)
#include <tre/regex.h>
#else
#include <regex.h>
#endif /* TRE_REGEX_ENABLE */


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/* Check for the __DATE__ define */
#if !defined(__DATE__)
#define __DATE__            "Undefined __DATE__"
#endif    /* defined(__DATE__) */

/* Check for the __TIME__ define */
#if !defined(__TIME__)
#define __TIME__            "Undefined __TIME__"
#endif    /* defined(__TIME__) */


/* Check for the __FILE__ define */
#if !defined(__FILE__)
#define __FILE__            "Undefined __FILE__"
#endif    /* defined(__FILE__) */

/* Check for the __LINE__ define */
#if !defined(__LINE__)
#define __LINE__            (-1)
#endif    /* defined(__LINE__) */


/*---------------------------------------------------------------------------*/


/* Set the number of bits in a long, get it from the operating system if it is
** available, otherwise we work it out (we can't use sizeof() because it 
** sometimes implemented as a function)
*/
#if !defined(LONG_BIT)
#if defined(LONGBITS)
#define LONG_BIT            (LONGBITS)
#else
#if (LONG_MAX == ((1UL << 31) - 1))
#define LONG_BIT            (32)
#else
#define LONG_BIT            (64)
#endif    /* (LONG_MAX == ((1UL << 31) - 1)) */
#endif    /* defined(LONGBITS) */
#endif    /* !defined(LONG_BIT) */


/*---------------------------------------------------------------------------*/


/* Set the floating point limits if they are not set, which they really should be (in linux for example) */

#if !defined(FLT_MAX) && defined(MAXFLOAT)
#define FLT_MAX             (MAXFLOAT)
#endif    /* !defined(FLT_MAX) && defined(MAXFLOAT) */

#if !defined(FLT_MIN) && defined(MINFLOAT)
#define FLT_MIN             (MINLOAT)
#endif    /* !defined(FLT_MIN) && defined(MINFLOAT) */

#if !defined(DBL_MAX) && defined(MAXDOUBLE)
#define DBL_MAX             (MAXDOUBLE)
#endif    /* !defined(DBL_MAX) && defined(MAXDOUBLE) */

#if !defined(DBL_MIN) && defined(MINDOUBLE)
#define DBL_MIN             (MINDOUBLE)
#endif    /* !defined(DBL_MIN) && defined(MINDOUBLE) */


/*---------------------------------------------------------------------------*/


/* Set NAME_MAX */
#if !defined(NAME_MAX)
#if defined(_POSIX_NAME_MAX)
#define NAME_MAX            (_POSIX_NAME_MAX)
#else
#define NAME_MAX            (14)
#endif    /* defined(_POSIX_NAME_MAX) */
#endif    /* defined(NAME_MAX) */


/* Set PATH_MAX */
#if !defined(PATH_MAX)
#if defined(_POSIX_PATH_MAX)
#define PATH_MAX            (_POSIX_PATH_MAX)
#else
#define PATH_MAX            (256)
#endif    /* defined(_POSIX_PATH_MAX) */
#endif    /* defined(PATH_MAX) */


/*---------------------------------------------------------------------------*/


/* Set PIPE_BUF */
#if !defined(PIPE_BUF)
#if defined(_POSIX_PIPE_BUF)
#define PIPE_BUF            (_POSIX_PIPE_BUF)
#else
#define PIPE_BUF            (512)
#endif    /* defined(_POSIX_PIPE_BUF) */
#endif    /* defined(PIPE_BUF) */


/* Set BUFSIZ */
#if !defined(BUFSIZ)
#define BUFSIZ              (256)
#endif    /* !defined(BUFSIZ)) */


/*----------------------------------------------------------------------*/


/* Maximum number of open file descriptors - minimum of 256 */
#if !defined(OPEN_MAX) || (OPEN_MAX < 256)
#undef OPEN_MAX
#define OPEN_MAX            (256)
#endif    /* !defined(OPEN_MAX) || (OPEN_MAX < 256) */


/* Maximum number of open file streams - minimum of 256 */
#if !defined(FOPEN_MAX) || (FOPEN_MAX < 256)
#undef FOPEN_MAX
#define FOPEN_MAX           (256)
#endif    /* !defined(FOPEN_MAX) || (FOPEN_MAX < 256) */


/* Maximum number of open streams - minimum of 256 */
#if !defined(STREAM_MAX) || (STREAM_MAX < 8)
#undef STREAM_MAX
#define STREAM_MAX          (8)
#endif    /* !defined(STREAM_MAX) || (STREAM_MAX < 8) */


/*---------------------------------------------------------------------------*/


/* File seeking constants */

#if !defined(SEEK_SET)
#define SEEK_SET            (0)
#endif    /* !defined(SEEK_SET) */

#if !defined(SEEK_CUR)
#define SEEK_CUR            (1)
#endif    /* !defined(SEEK_CUR) */

#if !defined(SEEK_END)
#define SEEK_END            (2)
#endif    /* !defined(SEEK_END) */

#if !defined(EOF)
#define EOF                 (-1)
#endif    /* !defined(EOF) */


/*---------------------------------------------------------------------------*/


/* Standard file descriptor constants */

#if !defined(STDIN_FILENO)
#define STDIN_FILENO        (0)
#endif    /* !defined(STDIN_FILENO) */

#if !defined(STDOUT_FILENO)
#define STDOUT_FILENO       (1)
#endif    /* !defined(STDOUT_FILENO) */

#if !defined(STDERR_FILENO)
#define STDERR_FILENO       (2)
#endif    /* !defined(STDERR_FILENO) */


/*---------------------------------------------------------------------------*/


/* Temp file location constants */

#if !defined(P_tmpdir)
#define P_tmpdir            "/tmp"
#endif    /* !defined(P_tmpdir) */


/*---------------------------------------------------------------------------*/


/* File access constants */

#if !defined(R_OK)
#define R_OK                (4)
#endif    /* !defined(R_OK) */

#if !defined(W_OK)
#define W_OK                (2)
#endif    /* !defined(W_OK) */

#if !defined(X_OK)
#define X_OK                (1)
#endif    /* !defined(X_OK) */

#if !defined(F_OK)
#define F_OK                (0)
#endif    /* !defined(F_OK) */


/*---------------------------------------------------------------------------*/


/* NULL contant */

#if !defined(NULL)
#define NULL                (0)
#endif    /* !defined(NULL) */


/*---------------------------------------------------------------------------*/


/* Error constant */

#if !defined(EOK)
#define EOK                 (0)
#endif    /* !defined(EOK) */


/*---------------------------------------------------------------------------*/


/* Exit contants */

#if !defined(EXIT_FAILURE)
#define EXIT_FAILURE        (1)
#endif    /* !defined(EXIT_FAILURE) */

#if !defined(EXIT_SUCCESS)
#define EXIT_SUCCESS        (0)
#endif    /* !defined(EXIT_SUCCESS) */




/*---------------------------------------------------------------------------*/


/* Size specific types if we want them */

#if defined(NOTUSED)

#if !defined(uint64_t)
typedef unsigned long       uint64_t;
#endif    /* !defined(uint64_t) */

#if !defined(int64_t)
typedef signed long         int64_t;
#endif    /* !defined(int64_t) */

#if !defined(uint32_t)
typedef unsigned int        uint32_t;
#endif    /* !defined(uint32_t) */

#if !defined(int32_t)
typedef signed int          int32_t;
#endif    /* !defined(int32_t) */

#if !defined(uint16_t)
typedef unsigned short      uint16_t;
#endif    /* !defined(uint16_t) */

#if !defined(int16_t)
typedef signed short        int16_t;
#endif    /* !defined(int16_t) */

#if !defined(uint8_t)
typedef unsigned char       uint8_t;
#endif    /* !defined(uint8_t) */

#if !defined(int8_t)
typedef signed char         int8_t;
#endif    /* !defined(int8_t) */

#endif    /* #if defined(NOTUSED) */


/*---------------------------------------------------------------------------*/


/* Platform restriction */


/* We can only compile on 64 bit platforms */
#if (LONG_BIT != 64)
#error "MPS can only be compiled on 64 bit platforms"
#endif    /* (LONG_BIT != 64) */


/*---------------------------------------------------------------------------*/


/* Set up anything else which has not been supplied by the includes */

/* Solaris  */
#if defined(__sun__)

/* We need this for sockets i/o (FIONBIO) */
#include <sys/filio.h>

#define UTL_POSIX_ENABLE_WCSDUP
#define UTL_POSIX_ENABLE_WCSCASECMP
#define UTL_POSIX_ENABLE_WCSNCASECMP

/* With acc or cc on Solaris 2.x */
#if defined(__STDC__) && !defined(__GNUC__)

/* Sun's acc compiler does not seem to like the use of floats in
** functions declarations, so we remap it to a double 
*/
#define float               double                        

#endif    /* defined(__STDC__) && !defined(__GNUC__) */

#endif    /* defined(__sun__) */



/* Linux */
#if defined(linux)
#endif    /* defined(linux) */



/* MacOS X */
#if defined(__APPLE__) && defined(__MACH__)

#define UTL_POSIX_ENABLE_WCSDUP
#define UTL_POSIX_ENABLE_WCSCASECMP
#define UTL_POSIX_ENABLE_WCSNCASECMP

#endif    /* defined(__APPLE__) && defined(__MACH__) */



/* AIX */
#if defined(AIX)
#endif    /* defined(AIX) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

#if defined(UTL_POSIX_ENABLE_WCSDUP)
wchar_t     *wcsdup (wchar_t *pwcStr);
#endif    /* defined(UTL_POSIX_ENABLE_WCSDUP) */

#if defined(UTL_POSIX_ENABLE_WCSCASECMP)
int         wcscasecmp (wchar_t *pwcStr1, wchar_t *pwcStr2);
#endif    /* defined(UTL_POSIX_ENABLE_WCSCASECMP) */

#if defined(UTL_POSIX_ENABLE_WCSNCASECMP)
int         wcsncasecmp (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
#endif    /* defined(UTL_POSIX_ENABLE_WCSNCASECMP) */


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_POSIX_H) */


/*---------------------------------------------------------------------------*/
