/*****************************************************************************
*        Copyright (C) 1993-2011, FS Consulting LLC. All rights reserved     *
*                                                                            *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.                   *
*                                                                            *
*  This software is the proprietary and confidential property                *
*  of FS Consulting LLC.                                                     *
*****************************************************************************/


/*

    Module:     machenv.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    17 July 1994

    Purpose:    This module is a simple program which gets the 
                machine configuration and prints it out. It is 
                designed to help cope with portability problems
                could arise.

*/


/*---------------------------------------------------------------------------*/


/*
** C includes (POSIX)
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <grp.h>
#include <limits.h>
#include <locale.h>
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
#include <wchar.h>


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define SHORT_CODE      (0)
#define INT_CODE        (1)
#define LONG_CODE       (2)
#define CHAR_CODE       (3)


#define ERRNO_MAX       (1000)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

struct testShort {
    char cChar;
    unsigned short usShort;
};

struct testLong {
    char cChar;
    unsigned long ulLong;
};

struct testInt {
    char cChar;
    unsigned int uiInt;
};


/*---------------------------------------------------------------------------*/


/*
** Function prototypes
*/

static void vDumpSymbols (void);

static void vDumpSizes (void);

static void vDumpSysconf (void);

static void vDumpErrors (void);

static void vDumpLimits (void);

static void vDumpMaximumFileDescriptors (void);


/*---------------------------------------------------------------------------*/


/*

    Function:   main()

    Purpose:    Call all the other functions.

    Parameters: argc    number of arguments
                argv    arguments
                env     null terminated list of environment variables

    Globals:    none

    Returns:    int

*/
int main(argc, argv, env)
int argc;
char *argv[];
char *env[];
{


    printf("Content-type: text/plain\n\n");

    {
        unsigned long ulI = 0;

        for ( ulI = 0; ulI < argc; ulI++ ) {
            printf("argv[%ld] = \"%s\"\n", ulI, argv[ulI]);
        }

        for ( ulI = 0; (env != NULL) && (env[ulI] != NULL); ulI++ ) {
            printf("env[%ld] = \"%s\"\n", ulI, env[ulI]);
        }

        printf("\n\n");
    }



    /* Dump all the symbols */
    vDumpSymbols();

    /* Dump all sizes */
    vDumpSizes();

    /* Dump all Sysconf */
    vDumpSysconf();

    /* Dump all the error strings */
    vDumpErrors();

    /* Dump the limits */
    vDumpLimits();

    /* Dump the maximum number of file descriptors */
    vDumpMaximumFileDescriptors();

    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vDumpSymbols

    Purpose:    Dump out symbols

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vDumpSymbols()
{

    printf("Symbols:\n\n");


#if !defined(LONG_BIT)
    printf("LONG_BIT undefined\n");
#else
    printf("LONG_BIT defined: %d\n", (int)LONG_BIT);
#endif


#if !defined(LONGBITS)
    printf("LONGBITS undefined\n");
#else
    printf("LONGBITS defined: %d\n", (int)LONGBITS);
#endif


#if !defined(ULLONG_MAX)
    printf("ULLONG_MAX undefined\n");
#else
    printf("ULLONG_MAX defined\n");
#endif


#if !defined(ULLONG_MIN)
    printf("ULLONG_MIN undefined\n");
#else
    printf("ULLONG_MIN defined\n");
#endif


#if !defined(LLONG_MAX)
    printf("LLONG_MAX undefined\n");
#else
    printf("LLONG_MAX defined\n");
#endif


#if !defined(LLONG_MIN)
    printf("LLONG_MIN undefined\n");
#else
    printf("LLONG_MIN defined\n");
#endif


#if !defined(ULONG_MAX)
    printf("ULONG_MAX undefined\n");
#else
    printf("ULONG_MAX defined\n");
#endif


#if !defined(ULONG_MIN)
    printf("ULONG_MIN undefined\n");
#else
    printf("ULONG_MIN defined\n");
#endif


#if !defined(LLONG_MAX)
    printf("LONG_MAX undefined\n");
#else
    printf("LONG_MAX defined\n");
#endif


#if !defined(LONG_MIN)
    printf("LONG_MIN undefined\n");
#else
    printf("LONG_MIN defined\n");
#endif


#if !defined(FLT_MAX)
    printf("FLT_MAX undefined\n");
#else
    printf("FLT_MAX defined\n");
#endif


#if !defined(FLT_MIN)
    printf("FLT_MIN undefined\n");
#else
    printf("FLT_MIN defined\n");
#endif


#if !defined(MAXFLOAT)
    printf("MAXFLOAT undefined\n");
#else
    printf("MAXFLOAT defined\n");
#endif


#if !defined(MINFLOAT)
    printf("MINFLOAT undefined\n");
#else
    printf("MINFLOAT defined\n");
#endif


#if !defined(DBL_MAX)
    printf("DBL_MAX undefined\n");
#else
    printf("DBL_MAX defined\n");
#endif


#if !defined(DBL_MIN)
    printf("DBL_MIN undefined\n");
#else
    printf("DBL_MIN defined\n");
#endif


#if !defined(MAXDOUBLE)
    printf("MAXDOUBLE undefined\n");
#else
    printf("MAXDOUBLE defined\n");
#endif


#if !defined(MINDOUBLE)
    printf("MINDOUBLE undefined\n");
#else
    printf("MINDOUBLE defined\n");
#endif


#if !defined(PATH_MAX)
    printf("PATH_MAX undefined\n");
#else
    printf("PATH_MAX defined: %d\n", (int)PATH_MAX);
#endif


#if !defined(PIPE_BUF)
    printf("PIPE_BUF undefined\n");
#else
    printf("PIPE_BUF defined: %d\n", (int)PIPE_BUF);
#endif


#if !defined(BUFSIZ)
    printf("BUFSIZ undefined\n");
#else
    printf("BUFSIZ defined: %d\n", (int)BUFSIZ);
#endif


#if !defined(FD_SETSIZE)
    printf("FD_SETSIZE undefined\n");
#else
    printf("FD_SETSIZE defined: %d\n", (int)FD_SETSIZE);
#endif


#if !defined(OPEN_MAX)
    printf("OPEN_MAX undefined\n");
#else
    printf("OPEN_MAX defined: %d\n", (int)OPEN_MAX);
#endif


#if !defined(FOPEN_MAX)
    printf("FOPEN_MAX undefined\n");
#else
    printf("FOPEN_MAX defined: %d\n", (int)FOPEN_MAX);
#endif


#if !defined(_POSIX_LINK_MAX)
    printf("_POSIX_LINK_MAX undefined\n");
#else
    printf("_POSIX_LINK_MAX defined: %d\n", (int)_POSIX_LINK_MAX);
#endif


#if !defined(_POSIX_MAX_CANON)
    printf("_POSIX_MAX_CANON undefined\n");
#else
    printf("_POSIX_MAX_CANON defined: %d\n", (int)_POSIX_MAX_CANON);
#endif


#if !defined(_POSIX_MAX_INPUT)
    printf("_POSIX_MAX_INPUT undefined\n");
#else
    printf("_POSIX_MAX_INPUT defined: %d\n", (int)_POSIX_MAX_INPUT);
#endif


#if !defined(_POSIX_NAME_MAX)
    printf("_POSIX_NAME_MAX undefined\n");
#else
    printf("_POSIX_NAME_MAX defined: %d\n", (int)_POSIX_NAME_MAX);
#endif


#if !defined(_POSIX_PATH_MAX)
    printf("_POSIX_PATH_MAX undefined\n");
#else
    printf("_POSIX_PATH_MAX defined: %d\n", (int)_POSIX_PATH_MAX);
#endif


#if !defined(_POSIX_PIPE_BUF)
    printf("_POSIX_PIPE_BUF undefined\n");
#else
    printf("_POSIX_PIPE_BUF defined: %d\n", (int)_POSIX_PIPE_BUF);
#endif


#if !defined(_POSIX_CHOWN_RESTRICTED)
    printf("_POSIX_CHOWN_RESTRICTED undefined\n");
#else
    printf("_POSIX_CHOWN_RESTRICTED defined: %d\n", (int)_POSIX_CHOWN_RESTRICTED);
#endif


#if !defined(_POSIX_NO_TRUNC)
    printf("_POSIX_NO_TRUNC undefined\n");
#else
    printf("_POSIX_NO_TRUNC defined: %d\n", (int)_POSIX_NO_TRUNC);
#endif


#if !defined(STREAM_MAX)
    printf("STREAM_MAX undefined\n");
#else
    printf("STREAM_MAX defined: %d\n", (int)STREAM_MAX);
#endif


#if !defined(SEEK_SET)
    printf("SEEK_SET undefined\n");
#else
    printf("SEEK_SET defined: %d\n", (int)SEEK_SET);
#endif


#if !defined(SEEK_CUR)
    printf("SEEK_CUR undefined\n");
#else
    printf("SEEK_CUR defined: %d\n", (int)SEEK_CUR);
#endif


#if !defined(SEEK_END)
    printf("SEEK_END undefined\n");
#else
    printf("SEEK_END defined: %d\n", (int)SEEK_END);
#endif


#if !defined(EOF)
    printf("EOF undefined\n");
#else
    printf("EOF defined: %d\n", (int)EOF);
#endif


#if !defined(STDIN_FILENO)
    printf("STDIN_FILENO undefined\n");
#else
    printf("STDIN_FILENO defined: %d\n", (int)STDIN_FILENO);
#endif


#if !defined(STDOUT_FILENO)
    printf("STDOUT_FILENO undefined\n");
#else
    printf("STDOUT_FILENO defined: %d\n", (int)STDOUT_FILENO);
#endif


#if !defined(STDERR_FILENO)
    printf("STDERR_FILENO undefined\n");
#else
    printf("STDERR_FILENO defined: %d\n", (int)STDERR_FILENO);
#endif


#if !defined(P_tmpdir)
    printf("P_tmpdir undefined\n");
#else
    printf("P_tmpdir defined: '%s'\n", P_tmpdir);
#endif


#if !defined(R_OK)
    printf("R_OK undefined\n");
#else
    printf("R_OK defined: %d\n", (int)R_OK);
#endif


#if !defined(W_OK)
    printf("W_OK undefined\n");
#else
    printf("W_OK defined: %d\n", (int)W_OK);
#endif


#if !defined(X_OK)
    printf("X_OK undefined\n");
#else
    printf("X_OK defined: %d\n", (int)X_OK);
#endif


#if !defined(F_OK)
    printf("F_OK undefined\n");
#else
    printf("F_OK defined: %d\n", (int)F_OK);
#endif


#if !defined(NULL)
    printf("NULL undefined\n");
#else
    printf("NULL defined: %d\n", (int)NULL);
#endif


#if !defined(EXIT_SUCCESS)
    printf("EXIT_SUCCESS undefined\n");
#else
    printf("EXIT_SUCCESS defined: %d\n", (int)EXIT_SUCCESS);
#endif


#if !defined(EXIT_FAILURE)
    printf("EXIT_FAILURE undefined\n");
#else
    printf("EXIT_FAILURE defined: %d\n", (int)EXIT_FAILURE);
#endif


#if !defined(MAXHOSTNAMELEN)
    printf("MAXHOSTNAMELEN undefined\n");
#else
    printf("MAXHOSTNAMELEN defined: %d\n", (int)MAXHOSTNAMELEN);
#endif


#if !defined(_LARGEFILE_SOURCE)
    printf("_LARGEFILE_SOURCE undefined\n");
#else
    printf("_LARGEFILE_SOURCE defined: %d\n", (int)_LARGEFILE_SOURCE);
#endif


#if !defined(_FILE_OFFSET_BITS)
    printf("_FILE_OFFSET_BITS undefined\n");
#else
    printf("_FILE_OFFSET_BITS defined: %d\n", (int)_FILE_OFFSET_BITS);
#endif


#if !defined(__BIG_ENDIAN)
    printf("__BIG_ENDIAN undefined\n");
#else
    printf("__BIG_ENDIAN defined: %d\n", (int)__BIG_ENDIAN);
#endif


#if !defined(__LITTLE_ENDIAN)
    printf("__LITTLE_ENDIAN undefined\n");
#else
    printf("__LITTLE_ENDIAN defined: %d\n", (int)__LITTLE_ENDIAN);
#endif


#if !defined(__BYTE_ORDER)
    printf("__BYTE_ORDER undefined\n");
#else
    printf("__BYTE_ORDER defined: %d\n", (int)__BYTE_ORDER);
#endif


#if !defined(__FUNCTION__)
    if ( __FUNCTION__ != NULL ) {
        printf("__FUNCTION__ undefined but set to: %s\n", __FUNCTION__);
    }
    else {
        printf("__FUNCTION__ undefined\n");
    }
#else
    printf("__FUNCTION__ defined: %s\n", __FUNCTION__);
#endif


#if !defined(__func__)
    if ( __func__ != NULL ) {
        printf("__func__ undefined but set to: %s\n", __func__);
    }
    else {
        printf("__func__ undefined\n");
    }
#else
    printf("__func__ defined: %s\n", __func__);
#endif


#if !defined(__PRETTY_FUNCTION__)
    if ( __PRETTY_FUNCTION__ != NULL ) {
        printf("__PRETTY_FUNCTION__ undefined but set to: %s\n", __PRETTY_FUNCTION__);
    }
    else {
        printf("__PRETTY_FUNCTION__ undefined\n");
    }
#else
    printf("__PRETTY_FUNCTION__ defined: %s\n", __PRETTY_FUNCTION__);
#endif


#if !defined(__FILE__)
    printf("__FILE__ undefined\n");
#else
    printf("__FILE__ defined: %s\n", __FILE__);
#endif


#if !defined(__LINE__)
    printf("__LINE__ undefined\n");
#else
    printf("__LINE__ defined: %d\n", (int)__LINE__);
#endif


#if !defined(__DATE__)
    printf("__DATE__ undefined\n");
#else
    printf("__DATE__ defined: %s\n", __DATE__);
#endif


#if !defined(__TIME__)
    printf("__TIME__ undefined\n");
#else
    printf("__TIME__ defined: %s\n", __TIME__);
#endif


#if !defined(__STDC_ISO_10646__)
    printf("__STDC_ISO_10646__ undefined\n");
#else
    printf("__STDC_ISO_10646__ defined\n");
#endif


#if !defined(_GNU_SOURCE)
    printf("_GNU_SOURCE undefined\n");
#else
    printf("_GNU_SOURCE defined\n");
#endif


#if !defined(_REENTRANT)
    printf("_REENTRANT undefined\n");
#else
    printf("_REENTRANT defined\n");
#endif


#if !defined(PTHREAD_STACK_MIN)
    printf("PTHREAD_STACK_MIN undefined\n");
#else
    printf("PTHREAD_STACK_MIN defined: %d\n", PTHREAD_STACK_MIN);
#endif


#if !defined(true)
    printf("true undefined\n");
#else
    printf("true defined: %d\n", (int)true);
#endif


#if !defined(false)
    printf("false undefined\n");
#else
    printf("false defined: %ld\n", (int)false);
#endif


#if !defined(boolean)
    printf("boolean type undefined\n");
#endif

    printf("\n\n");

    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vDumpSizes

    Purpose:    Dump out sizes and default signs of various C datatypes as well 
                at the byte order and alignment of the cpu

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vDumpSizes()
{

    struct testShort        tsStruct;
    struct testLong         tlStruct;
    struct testInt          tiStruct;

    int                    iFourByte = (int)0;
    int                    iTwoByte = (int)0;
    int                    iOneByte = (int)0;

    unsigned long           ulLong = (unsigned long)0;
    unsigned char           *pucPtr = (unsigned long)0;

    long                    lLong = (long)ULONG_MAX;
#if defined(ULLONG_MAX)
    long long               llLongLong = (long long)ULLONG_MAX;
#elif defined(ULONG_LONG_MAX)
    long long               llLongLong = (long long)ULONG_LONG_MAX;
#else
    long long               llLongLong = (long long)0;
#endif
    int                     iInt = (int)UINT_MAX;
    short                   sShort = (short)USHRT_MAX;
    char                    cChar = (char)UCHAR_MAX;
    wchar_t                 wcChar = (wchar_t)UINT_MAX;

#if defined(FLT_MAX)
    float                   fFloat = (float)FLT_MAX;
#elif defined(MAXFLOAT)
    float                   fFloat = (float)MAXFLOAT;
#else
    float                   fFloat = (float)0;
#endif

#if defined(DBL_MAX)
    double                  dDouble = (double)DBL_MAX;
#elif defined(MAXDOUBLE)
    double                  dDouble = (double)MAXDOUBLE;
#else
    double                  dDouble = (double)0;
#endif

    unsigned long           lTest = (long)ULONG_MAX;

    time_t                  zTimeT = (time_t)(lTest);
    size_t                  zSizeT = (size_t)(lTest);
    off_t                   zOffT = (off_t)(lTest);
#if !defined(linux)
    fpos_t                  zFposT = (fpos_t)(lTest);
#endif
    pid_t                   zPidT = (pid_t)(lTest);
    uid_t                   zUidT = (uid_t)(lTest);
    gid_t                   zGidT = (gid_t)(lTest);
    mode_t                  zModeT = (mode_t)(lTest);
    caddr_t                 zCaddrT = (caddr_t)(lTest);
    daddr_t                 zDaddrT = (daddr_t)(lTest);
    ssize_t                 zSsizeT = (ssize_t)(lTest);
    
    unsigned char           *pucCharPtr = (unsigned char *)(lTest);
    void                    *pvVoidPtr = (unsigned char *)(lTest);


    printf("Type sizes:\n\n");
    printf("char        %ld byte%s %s\n", (long)sizeof(char), (sizeof(char) == 1) ? " " : "s", ((cChar < 0) ? "signed" : "unsigned")); 
    printf("wchar_t     %ld byte%s %s\n", (long)sizeof(wchar_t), (sizeof(wchar_t) == 1) ? " " : "s", ((wcChar < 0) ? "signed" : "unsigned")); 
    printf("short       %ld byte%s %s\n", (long)sizeof(short), (sizeof(short) == 1) ? " " : "s", ((sShort < 0) ? "signed" : "unsigned")); 
    printf("int         %ld byte%s %s\n", (long)sizeof(int), (sizeof(int) == 1) ? " " : "s", ((iInt < 0) ? "signed" : "unsigned")); 
    printf("long        %ld byte%s %s\n", (long)sizeof(long), (sizeof(long) == 1) ? " " : "s", ((lLong < 0) ? "signed" : "unsigned")); 
    printf("long long   %ld byte%s %s\n", (long)sizeof(long long), (sizeof(long long) == 1) ? " " : "s", ((llLongLong < 0) ? "signed" : "unsigned")); 
    printf("float       %ld byte%s %s\n", (long)sizeof(float), (sizeof(float) == 1) ? " " : "s", ((fFloat < 0) ? "signed" : "unsigned")); 
    printf("double      %ld byte%s %s\n", (long)sizeof(double), (sizeof(double) == 1) ? " " : "s", ((dDouble < 0) ? "signed" : "unsigned")); 
    printf("time_t      %ld byte%s %s\n", (long)sizeof(time_t), (sizeof(time_t) == 1) ? " " : "s", ((zTimeT < 0) ? "signed" : "unsigned")); 
    printf("size_t      %ld byte%s %s\n", (long)sizeof(size_t), (sizeof(size_t) == 1) ? " " : "s", ((zSizeT < 0) ? "signed" : "unsigned")); 
    printf("off_t       %ld byte%s %s\n", (long)sizeof(off_t), (sizeof(off_t) == 1) ? " " : "s", ((zOffT < 0) ? "signed" : "unsigned")); 
#if !defined(linux)
    printf("fpos_t      %ld byte%s %s\n", (long)sizeof(fpos_t), (sizeof(fpos_t) == 1) ? " " : "s", ((zFposT < 0) ? "signed" : "unsigned")); 
#else
    printf("fpos_t      (structure in linux)\n"); 
#endif
    printf("pid_t       %ld byte%s %s\n", (long)sizeof(pid_t), (sizeof(pid_t) == 1) ? " " : "s", ((zPidT < 0) ? "signed" : "unsigned")); 
    printf("uid_t       %ld byte%s %s\n", (long)sizeof(uid_t), (sizeof(uid_t) == 1) ? " " : "s", ((zUidT < 0) ? "signed" : "unsigned")); 
    printf("gid_t       %ld byte%s %s\n", (long)sizeof(gid_t), (sizeof(gid_t) == 1) ? " " : "s", ((zGidT < 0) ? "signed" : "unsigned")); 
    printf("mode_t      %ld byte%s %s\n", (long)sizeof(mode_t), (sizeof(mode_t) == 1) ? " " : "s", ((zModeT < 0) ? "signed" : "unsigned")); 
    printf("caddr_t     %ld byte%s %s\n", (long)sizeof(caddr_t), (sizeof(caddr_t) == 1) ? " " : "s", ((zCaddrT < 0) ? "signed" : "unsigned")); 
    printf("daddr_t     %ld byte%s %s\n", (long)sizeof(daddr_t), (sizeof(daddr_t) == 1) ? " " : "s", ((zDaddrT < 0) ? "signed" : "unsigned")); 
    printf("ssize_t     %ld byte%s %s\n", (long)sizeof(ssize_t), (sizeof(ssize_t) == 1) ? " " : "s", ((zSsizeT < 0) ? "signed" : "unsigned")); 
    printf("char*       %ld byte%s %s\n", (long)sizeof(char *), (sizeof(char *) == 1) ? " " : "s", ((pucCharPtr < 0) ? "signed" : "unsigned")); 
    printf("void*       %ld byte%s %s\n", (long)sizeof(char *), (sizeof(char *) == 1) ? " " : "s", ((pvVoidPtr < 0) ? "signed" : "unsigned")); 

    printf("\n\n");


    ulLong = 0xdeadbeef;
    pucPtr = (unsigned char*)&ulLong;


    printf("Byte order: ");

    if (*pucPtr == 0xde) {
        printf("Big Endian\n");
    }
    else {
        if (*pucPtr == 0xef) {
            printf("Little Endian\n");
        }
        else {
            printf("Error: can't find out byte order\n");
            return;
        }
    }
    printf("\n\n");


    printf("Byte alignment:\n\n");

    if (sizeof(char) == 1) {
        iOneByte = CHAR_CODE;
    }

    if (sizeof(int) == 2) {
        iTwoByte = INT_CODE;
    }
    else {
        if (sizeof(short) == 2) {
            iTwoByte = SHORT_CODE;
        }
    }

    if (sizeof(int) == 4) {
        iFourByte = INT_CODE;
    }
    else {
        if (sizeof(long) == 4) {
            iFourByte = LONG_CODE;
        }
    }


    if (iTwoByte == SHORT_CODE) {
        printf("Two byte      %ld\n", (long)(&tsStruct.usShort)-(long)(&tsStruct.cChar)); 
    }
    else {
        if (iTwoByte == INT_CODE) {
            printf("Two byte      %ld\n", (long)(&tiStruct.uiInt)-(long)(&tiStruct.cChar));
        }
    }

    if (iFourByte == INT_CODE) {
        printf("Four byte      %ld\n", (long)(&tiStruct.uiInt)-(long)(&tiStruct.cChar)); 
    }
    else {
        if (iFourByte == LONG_CODE) {
            printf("Four byte      %ld\n", (long)(&tlStruct.ulLong)-(long)(&tlStruct.cChar));
        }
    }
    printf("\n\n");


    return;
}


/*---------------------------------------------------------------------------*/


/*

    Function:   vDumpSysconf

    Purpose:    Dump out sysconf constants

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vDumpSysconf()
{


    printf("Sysconf:\n\n");


#if !defined(_SC_ARG_MAX)
    printf("_SC_ARG_MAX undefined\n");
#else
    printf("_SC_ARG_MAX defined: %ld\n", sysconf(_SC_ARG_MAX));
#endif


#if !defined(_SC_CHILD_MAX)
    printf("_SC_CHILD_MAX undefined\n");
#else
    printf("_SC_CHILD_MAX defined: %ld\n", sysconf(_SC_CHILD_MAX));
#endif


#if !defined(_SC_HOST_NAME_MAX)
    printf("_SC_HOST_NAME_MAX undefined\n");
#else
    printf("_SC_HOST_NAME_MAX defined: %ld\n", sysconf(_SC_HOST_NAME_MAX));
#endif


#if !defined(_SC_LOGIN_NAME_MAX)
    printf("_SC_LOGIN_NAME_MAX undefined\n");
#else
    printf("_SC_LOGIN_NAME_MAX defined: %ld\n", sysconf(_SC_LOGIN_NAME_MAX));
#endif


#if !defined(_SC_CLK_TCK)
    printf("_SC_CLK_TCK undefined\n");
#else
    printf("_SC_CLK_TCK defined: %ld\n", sysconf(_SC_CLK_TCK));
#endif


#if !defined(_SC_OPEN_MAX)
    printf("_SC_OPEN_MAX undefined\n");
#else
    printf("_SC_OPEN_MAX defined: %ld\n", sysconf(_SC_OPEN_MAX));
#endif


#if !defined(_SC_PAGESIZE)
    printf("_SC_PAGESIZE undefined\n");
#else
    printf("_SC_PAGESIZE defined: %ld\n", sysconf(_SC_PAGESIZE));
#endif


#if !defined(_SC_RE_DUP_MAX)
    printf("_SC_RE_DUP_MAX undefined\n");
#else
    printf("_SC_RE_DUP_MAX defined: %ld\n", sysconf(_SC_RE_DUP_MAX));
#endif


#if !defined(_SC_STREAM_MAX)
    printf("_SC_STREAM_MAX undefined\n");
#else
    printf("_SC_STREAM_MAX defined: %ld\n", sysconf(_SC_STREAM_MAX));
#endif


#if !defined(_SC_TTY_NAME_MAX)
    printf("_SC_TTY_NAME_MAX undefined\n");
#else
    printf("_SC_TTY_NAME_MAX defined: %ld\n", sysconf(_SC_TTY_NAME_MAX));
#endif


#if !defined(_SC_TZNAME_MAX)
    printf("_SC_TZNAME_MAX undefined\n");
#else
    printf("_SC_TZNAME_MAX defined: %ld\n", sysconf(_SC_TZNAME_MAX));
#endif


#if !defined(_SC_VERSION)
    printf("_SC_VERSION undefined\n");
#else
    printf("_SC_VERSION defined: %ld\n", sysconf(_SC_VERSION));
#endif


#if !defined(_SC_PHYS_PAGES)
    printf("_SC_PHYS_PAGES undefined\n");
#else
    printf("_SC_PHYS_PAGES defined: %ld\n", sysconf(_SC_PHYS_PAGES));
#endif


#if !defined(_SC_AVPHYS_PAGES)
    printf("_SC_AVPHYS_PAGES undefined\n");
#else
    printf("_SC_AVPHYS_PAGES defined: %ld\n", sysconf(_SC_AVPHYS_PAGES));
#endif


#if !defined(_SC_NPROCESSORS_CONF)
    printf("_SC_NPROCESSORS_CONF undefined\n");
#else
    printf("_SC_NPROCESSORS_CONF defined: %ld\n", sysconf(_SC_NPROCESSORS_CONF));
#endif


#if !defined(_SC_NPROCESSORS_ONLN)
    printf("_SC_NPROCESSORS_ONLN undefined\n");
#else
    printf("_SC_NPROCESSORS_ONLN defined: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
#endif


    printf("\n\n");

    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vDumpErrors

    Purpose:    Dump out errors

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vDumpErrors()
{

    unsigned int    uiI = 0;

    printf("Errors:\n\n");


    /* Loop printing all the errors */
    for ( uiI = 0; uiI < ERRNO_MAX; uiI++ ) {
        
        if ( strncmp(strerror(uiI), "Unknown error", strlen("Unknown error")) != 0 ) {
            printf("%3d - %s\n", uiI, strerror(uiI));
        }
    }


    printf("\n\n");

    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vDumpLimits

    Purpose:    Dump the limits imposed on the process

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vDumpLimits()
{


#if defined(RLIMIT_CORE) || defined(RLIMIT_CPU) || defined(RLIMIT_DATA) || defined(RLIMIT_FSIZE) || \
    defined(RLIMIT_NOFILE) || defined(RLIMIT_STACK) || defined(RLIMIT_VMEM) || defined(RLIMIT_AS) || \
    defined(RLIMIT_RSS) || defined(RLIMIT_NPROC) || defined(RLIMIT_MEMLOCK)

    struct rlimit rlp;

    printf("Resource:                    Current:             Maximum:\n\n");   

#if defined(RLIMIT_CORE)
    getrlimit(RLIMIT_CORE, &rlp);
    printf("RLIMIT_CORE:     %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_CPU)
    getrlimit(RLIMIT_CPU, &rlp);
    printf("RLIMIT_CPU:      %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_DATA)
    getrlimit(RLIMIT_DATA, &rlp);
    printf("RLIMIT_DATA:     %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_FSIZE)
    getrlimit(RLIMIT_FSIZE, &rlp);
    printf("RLIMIT_FSIZE:    %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_NOFILE)
    getrlimit(RLIMIT_NOFILE, &rlp);
    printf("RLIMIT_NOFILE:   %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_STACK)
    getrlimit(RLIMIT_STACK, &rlp);
    printf("RLIMIT_STACK:    %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_VMEM)
    getrlimit(RLIMIT_VMEM, &rlp);
    printf("RLIMIT_VMEM:     %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_AS)
    getrlimit(RLIMIT_AS, &rlp);
    printf("RLIMIT_AS:       %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_RSS)
    getrlimit(RLIMIT_RSS, &rlp);
    printf("RLIMIT_RSS:      %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_NPROC)
    getrlimit(RLIMIT_NPROC, &rlp);
    printf("RLIMIT_NPROC:    %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif

#if defined(RLIMIT_MEMLOCK)
    getrlimit(RLIMIT_MEMLOCK, &rlp);
    printf("RLIMIT_MEMLOCK:  %20lu %20lu\n", (unsigned long)rlp.rlim_cur, (unsigned long)rlp.rlim_max); 
#endif


#else

    printf("Resource limits are not supported.");

#endif

    printf("\n\n");

    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vDumpMaximumFileDescriptors

    Purpose:    Dump out the maximum number of files descriptors
                we can open at any one time. 

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vDumpMaximumFileDescriptors()
{


    int     iDescriptor = -1;
    int     iDescriptorCount = 0;
    FILE    *pfFile = NULL;



    /* Check that we can open the test file */
    if ( (pfFile = fopen("/dev/null", "r")) == NULL ) {
        printf("Error - failed to open test file.\n");
        return;
    }
    fclose(pfFile);



    /* Loop opening streams */
    while ( 1 == 1 ) {
        if ( (pfFile = fopen("/dev/null", "r")) == NULL ) {
            break;
        }

        iDescriptorCount++;
    }

    printf("Maximum number of file streams: %d, ", iDescriptorCount + 3);

#if defined(STREAM_MAX)
    printf("(STREAM_MAX = %d).\n\n", STREAM_MAX);
#else
    printf("(WARNING - STREAM_MAX is undefined).\n\n");
#endif



    /* Loop opening descriptors */
    while ( 1 == 1 ) {
        if ( (iDescriptor = open("/dev/null", O_RDWR)) == -1 ) {
            break;
        }

        iDescriptorCount++;
    }

    printf("Maximum number of file descriptors: %d, ", iDescriptorCount + 3);

#if defined(FOPEN_MAX)
    printf("(FOPEN_MAX = %d).\n\n", FOPEN_MAX);
#else
    printf("(WARNING - FOPEN_MAX is undefined).\n\n");
#endif



    return;

}


/*---------------------------------------------------------------------------*/

