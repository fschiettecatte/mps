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

    Module:     cwrappers.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    7 July 1997

    Purpose:    This is the header file for cwrappers.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_CWRAPPERS_H)
#define UTL_CWRAPPERS_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/
#include "utils.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Enable the cwrappers */
/* #define UTL_CWRAPPERS_ENABLE */

/* Enable the cwrappers with additional debug code */
#define UTL_CWRAPPERS_ENABLE_DEBUG


/* We automatically turn on UTL_CWRAPPERS_ENABLE_DEBUG if DEBUG mode is set */
#if defined(DEBUG)
#define UTL_CWRAPPERS_ENABLE_DEBUG
#undef UTL_CWRAPPERS_ENABLE
#endif    /* defined(DEBUG) */


/* Turn off UTL_CWRAPPERS_ENABLE_DEBUG if OPTIMIZED mode is set */
#if defined(OPTIMIZED)
#undef UTL_CWRAPPERS_ENABLE_DEBUG
#define UTL_CWRAPPERS_ENABLE
#endif    /* defined(OPTIMIZED) */


/* Check that both defined are no enabled */ 
#if defined(UTL_CWRAPPERS_ENABLE) && defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#error "Cannot enable both UTL_CWRAPPERS_ENABLE and UTL_CWRAPPERS_ENABLE_DEBUG"
#endif


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/


/* You dont want to call those, you really want to call the defines
** below as they present a much nicer interface.
**
** Function names which begin with 'fs_' are always called unless UTL_CWRAPPERS_DISABLE
** is defined as they report errors. The philosophy of these functions is
** to provide a degree of safety when things go wrong, as well as a log of what 
** went wrong.
**
** If UTL_CWRAPPERS_ENABLE_DEBUG is defined, the calling file and line number is passed to 
** the 'fs_' function for inclusion in the log.
*/


/* Enhanced memory allocation functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
void        *fs_malloc (size_t zSize, char *pcFile, size_t zLine);
void        *fs_calloc (size_t zNum, size_t zSize, char *pcFile, size_t zLine);
void        *fs_realloc (void *vPtr, size_t zSize, char *pcFile, size_t zLine);
void        fs_free (void *vPtr, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
void        *fs_malloc (size_t zSize);
void        *fs_calloc (size_t zNum, size_t zSize);
void        *fs_realloc (void *vPtr, size_t zSize);
void        fs_free (void *vPtr);
#endif


/* Enhanced memory handling functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int     fs_memcmp (void *pvPtr1, void *pvPtr2, size_t zCount, char *pcFile, size_t zLine);
void    *fs_memcpy (void *pvPtr1, void *pvPtr2, size_t zCount, char *pcFile, size_t zLine);
void    *fs_memmove (void *pvPtr1, void *pvPtr2, size_t zCount, char *pcFile, size_t zLine);
void    *fs_memset (void *pvPtr, int iChar, size_t zCount, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int     fs_memcmp (void *pvPtr1, void *pvPtr2, size_t zCount);
void    *fs_memcpy (void *pvPtr1, void *pvPtr2, size_t zCount);
void    *fs_memmove (void *pvPtr1, void *pvPtr2, size_t zCount);
void    *fs_memset (void *pvPtr, int iChar, size_t zCount);
#endif


/* Enhanced memory handling functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_wmemcmp (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount, char *pcFile, size_t zLine);
wchar_t     *fs_wmemcpy (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount, char *pcFile, size_t zLine);
wchar_t     *fs_wmemmove (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount, char *pcFile, size_t zLine);
wchar_t     *fs_wmemset (wchar_t *pwcStr, wchar_t wcChar, size_t zCount, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_wmemcmp (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
wchar_t     *fs_wmemcpy (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
wchar_t     *fs_wmemmove (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
wchar_t     *fs_wmemset (wchar_t *pwcStr, wchar_t wcChar, size_t zCount);
#endif



/* Enhanced file access functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
FILE        *fs_fopen (char *pcFilePath, char *pcMode, char *pcFile, size_t zLine);
FILE        *fs_fdopen (int iFileDescriptor, char *pcMode, char *pcFile, size_t zLine);
FILE        *fs_freopen (char *pcFilePath, char *pcMode, FILE *fpFile, char *pcFile, size_t zLine);
int         fs_fclose (FILE *pfFile, char *pcFile, size_t zLine);
size_t      fs_fread (void *pvPtr, size_t zSize, size_t zCount, FILE *pfFile, char *pcFile, size_t zLine);
size_t      fs_fwrite (void *pvPtr, size_t zSize, size_t zCount, FILE *pfFile, char *pcFile, size_t zLine);
char        *fs_fgets (char *pcStr, int iCount, FILE *pfFile, char *pcFile, size_t zLine);
int         fs_fputs (char *pcStr, FILE *pfFile, char *pcFile, size_t zLine);
int         fs_fflush (FILE *pfFile, char *pcFile, size_t zLine);
int         fs_fseek (FILE *pfFile, off_t zOffset, int iWhereFrom, char *pcFile, size_t zLine);
off_t       fs_ftell (FILE *pfFile, char *pcFile, size_t zLine);
int         fs_feof (FILE *pfFile, char *pcFile, size_t zLine);
int         fs_ferror (FILE *pfFile, char *pcFile, size_t zLine);
void        fs_clearerr (FILE *pfFile, char *pcFile, size_t zLine);
int         fs_truncate (char *pcFilePath, off_t zLen, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
FILE        *fs_fopen (char *pcFilePath, char *pcMode);
FILE        *fs_fdopen (int iFileDescriptor, char *pcMode);
FILE        *fs_freopen (char *pcFilePath, char *pcMode, FILE *fpFile);
int         fs_fclose (FILE *pfFile);
size_t      fs_fread (void *pvPtr, size_t zSize, size_t zCount, FILE *pfFile);
size_t      fs_fwrite (void *pvPtr, size_t zSize, size_t zCount, FILE *pfFile);
char        *fs_fgets (char *pcStr, int iCount, FILE *pfFile);
int         fs_fputs (char *pcStr, FILE *pfFile);
int         fs_fflush (FILE *pfFile);
int         fs_fseek (FILE *pfFile, off_t zOffset, int iWhereFrom);
off_t       fs_ftell (FILE *pfFile);
int         fs_feof (FILE *pfFile);
int         fs_ferror (FILE *pfFile);
void        fs_clearerr (FILE *pfFile);
int         fs_truncate (char *pcFilePath, off_t zLen);
#endif



/* Enhanced low level file I/O functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_open (char *pcFilePath, int iFlags, int iMode, char *pcFile, size_t zLine);
int         fs_close (int iFile, char *pcFile, size_t zLine);
ssize_t     fs_read (int iFile, char *pcStr, size_t zCount, char *pcFile, size_t zLine);
ssize_t     fs_write (int iFile, char *pcStr, size_t zCount, char *pcFile, size_t zLine);
off_t       fs_lseek (int iFile, off_t zOffset, int iWhereFrom, char *pcFile, size_t zLine);
int         fs_ioctl (int iFile, int iRequest, void *pvArgs, char *pcFile, size_t zLine);
int         fs_poll (struct pollfd *pPollFd, nfds_t zNfds, int iTimeOut, char *pcFile, size_t zLine);
int         fs_pipe (int *piFile, char *pcFile, size_t zLine);
int         fs_dup (int iFile, char *pcFile, size_t zLine);
int         fs_dup2 (int iFile1, int iFile2, char *pcFile, size_t zLine);
int         fs_ftruncate (int iFile, off_t zLen, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_open (char *pcFilePath, int iFlags, int iMode);
int         fs_close (int iFile);
ssize_t     fs_read (int iFile, char *pcStr, size_t zCount);
ssize_t     fs_write (int iFile, char *pcStr, size_t zCount);
off_t       fs_lseek (int iFile, off_t zOffset, int iWhereFrom);
int         fs_ioctl (int iFile, int iRequest, void *pvArgs);
int         fs_poll (struct pollfd *pPollFd, nfds_t zNfds, int iTimeOut);
int         fs_pipe (int *piFile);
int         fs_dup (int iFile);
int         fs_dup2 (int iFile1, int iFile2);
int         fs_ftruncate (int iFile, off_t zLen);
#endif



/* Enhanced low level socket I/O functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_socket (int iDomain, int iType, int iProtocol, char *pcFile, size_t zLine);
int         fs_bind (int iSocket, struct sockaddr *psaSocketAddress, socklen_t zSocketAddressLength, char *pcFile, size_t zLine);
int         fs_connect (int iSocket, struct sockaddr *psaSocketAddress, socklen_t zSocketAddressLength, char *pcFile, size_t zLine);
int         fs_accept (int iSocket, struct sockaddr *psaSocketAddress, socklen_t *pzSocketAddressLength, char *pcFile, size_t zLine);
int         fs_listen (int iSocket, int iBacklog, char *pcFile, size_t zLine);
int         fs_setsockopt (int iSocket, int iLevel, int iOptionName, void *pvOptionValue, socklen_t zOptionLength, char *pcFile, size_t zLine);
int         fs_getsockopt (int iSocket, int iLevel, int iOptionName, void *pvOptionValue, socklen_t *pzOptionLength, char *pcFile, size_t zLine);
ssize_t     fs_recv (int iSocket, char *pcStr, int iCount, int iFlag, char *pcFile, size_t zLine);
ssize_t     fs_send (int iSocket, char *pcStr, int iCount, int iFlag, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_socket (int iDomain, int iType, int iProtocol);
int         fs_bind (int iSocket, struct sockaddr *psaSocketAddress, socklen_t zSocketAddressLength);
int         fs_connect(int iSocket, struct sockaddr *psaSocketAddress, socklen_t zSocketAddressLength);
int         fs_accept (int iSocket, struct sockaddr *psaSocketAddress, socklen_t *pzSocketAddressLength);
int         fs_listen (int iSocket, int iBacklog);
int         fs_setsockopt (int iSocket, int iLevel, int iOptionName, void *pvOptionValue, socklen_t zOptionLength);
int         fs_getsockopt (int iSocket, int iLevel, int iOptionName, void *pvOptionValue, socklen_t *pzOptionLength);
ssize_t     fs_recv (int iSocket, char *pcStr, int iCount, int iFlag);
ssize_t     fs_send (int iSocket, char *pcStr, int iCount, int iFlag);
#endif



/* Enhanced file manipulation functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_remove (char *pcFilePath, char *pcFile, size_t zLine);
int         fs_rename (char *pcOldFilePath, char *pcNewFilePath, char *pcFile, size_t zLine);
FILE        *fs_tmpfile (char *pcFile, size_t zLine);
char        *fs_tmpnam (char *pcFilePath, char *pcFile, size_t zLine);
int         fs_stat (char *pcFilePath, struct stat *pstStat, char *pcFile, size_t zLine);
int         fs_lstat (char *pcFilePath, struct stat *pstStat, char *pcFile, size_t zLine);
int         fs_fstat (int iFile, struct stat *pstStat, char *pcFile, size_t zLine);
int         fs_access (char *pcFilePath, int iAccessMode, char *pcFile, size_t zLine);
int         fs_chmod (char *pcFilePath, mode_t zMode, char *pcFile, size_t zLine);
int         fs_chown (char *pcFilePath, uid_t zUserID, gid_t zGroupID, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_remove (char *pcFilePath);
int         fs_rename (char *pcOldFilePath, char *pcNewFilePath);
FILE        *fs_tmpfile (void);
char        *fs_tmpnam (char *pcFilePath);
int         fs_stat (char *pcFilePath, struct stat *pstStat);
int         fs_lstat (char *pcFilePath, struct stat *pstStat);
int         fs_fstat (int iFile, struct stat *pstStat);
int         fs_access (char *pcFilePath, int iAccessMode);
int         fs_chmod (char *pcFilePath, mode_t zMode);
int         fs_chown (char *pcFilePath, uid_t zUserID, gid_t zGroupID);
#endif


/* Enhanced directory manipulation functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
char        *fs_getcwd (char *pcDirectory, size_t zCount, char *pcFile, size_t zLine);
int         fs_mkdir (char *pcDirectory, mode_t zMode, char *pcFile, size_t zLine);
int         fs_rmdir (char *pcDirectory, char *pcFile, size_t zLine);
int         fs_chdir (char *pcDirectory, char *pcFile, size_t zLine);
DIR         *fs_opendir (char *pcDirectory, char *pcFile, size_t zLine);
struct dirent   *fs_readdir (DIR *pdDirectory, char *pcFile, size_t zLine);
void        fs_rewinddir (DIR *pdDirectory, char *pcFile, size_t zLine);
int         fs_closedir (DIR *pdDirectory, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
char        *fs_getcwd (char *pcDirectory, size_t zCount);
int         fs_mkdir (char *pcDirectory, mode_t zMode);
int         fs_rmdir (char *pcDirectory);
int         fs_chdir (char *pcDirectory);
DIR         *fs_opendir (char *pcDirectory);
struct dirent   *fs_readdir (DIR *pdDirectory);
void        fs_rewinddir (DIR *pdDirectory);
int         fs_closedir (DIR *pdDirectory);
#endif


/* Enhanced string handling functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
size_t      fs_strlen (char *pcStr, char *pcFile, size_t zLine);
char        *fs_strcpy (char *pcStr1, char *pcStr2, char *pcFile, size_t zLine);
char        *fs_strncpy (char *pcStr1, char *pcStr2, size_t zCount, char *pcFile, size_t zLine);
char        *fs_strcat (char *pcStr1, char *pcStr2, char *pcFile, size_t zLine);
char        *fs_strncat (char *pcStr1, char *pcStr2, size_t zCount, char *pcFile, size_t zLine);
int         fs_strcmp (char *pcStr1, char *pcStr2, char *pcFile, size_t zLine);
int         fs_strncmp (char *pcStr1, char *pcStr2, size_t zCount, char *pcFile, size_t zLine);
int         fs_strcoll (char *pcStr1, char *pcStr2, char *pcFile, size_t zLine);
int         fs_strcasecmp (char *pcStr1, char *pcStr2, char *pcFile, size_t zLine);
int         fs_strncasecmp (char *pcStr1, char *pcStr2, size_t zCount, char *pcFile, size_t zLine);
char        *fs_strchr (char *pcStr, char cChar, char *pcFile, size_t zLine);
char        *fs_strrchr (char *pcStr, char cChar, char *pcFile, size_t zLine);
char        *fs_strstr (char *pcStr1, char *pcStr2, char *pcFile, size_t zLine);
char        *fs_strtok_r (char *pcStr1, char *pcStr2, char **ppcStr3, char *pcFile, size_t zLine);
char        *fs_strpbrk (char *pcStr1, char *pcStr2, char *pcFile, size_t zLine);
char        *fs_strdup (char *pcStr, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
size_t      fs_strlen (char *pcStr);
char        *fs_strcpy (char *pcStr1, char *pcStr2);
char        *fs_strncpy (char *pcStr1, char *pcStr2, size_t zCount);
char        *fs_strcat (char *pcStr1, char *pcStr2);
char        *fs_strncat (char *pcStr1, char *pcStr2, size_t zCount);
int         fs_strcmp (char *pcStr1, char *pcStr2);
int         fs_strncmp (char *pcStr1, char *pcStr2, size_t zCount);
int         fs_strcoll (char *pcStr1, char *pcStr2);
int         fs_strcasecmp (char *pcStr1, char *pcStr2);
int         fs_strncasecmp (char *pcStr1, char *pcStr2, size_t zCount);
char        *fs_strchr (char *pcStr, char cChar);
char        *fs_strrchr (char *pcStr, char cChar);
char        *fs_strstr (char *pcStr1, char *pcStr2);
char        *fs_strtok_r (char *pcStr1, char *pcStr2, char **ppcStr3);
char        *fs_strpbrk (char *pcStr1, char *pcStr2);
char        *fs_strdup (char *pcStr);
#endif



/* Enhanced wide character string handling functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
size_t      fs_wcslen (wchar_t *pwcStr, char *pcFile, size_t zLine);
wchar_t     *fs_wcscpy (wchar_t *pwcStr1, wchar_t *pwcStr2, char *pcFile, size_t zLine);
wchar_t     *fs_wcsncpy (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount, char *pcFile, size_t zLine);
wchar_t     *fs_wcscat (wchar_t *pwcStr1, wchar_t *pwcStr2, char *pcFile, size_t zLine);
wchar_t     *fs_wcsncat (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount, char *pcFile, size_t zLine);
int         fs_wcscmp (wchar_t *pwcStr1, wchar_t *pwcStr2, char *pcFile, size_t zLine);
int         fs_wcsncmp (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount, char *pcFile, size_t zLine);
int         fs_wcscoll (wchar_t *pwcStr1, wchar_t *pwcStr2, char *pcFile, size_t zLine);
int         fs_wcscasecmp (wchar_t *pwcStr1, wchar_t *pwcStr2, char *pcFile, size_t zLine);
int         fs_wcsncasecmp (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount, char *pcFile, size_t zLine);
wchar_t     *fs_wcschr (wchar_t *pwcStr, wchar_t wcChar, char *pcFile, size_t zLine);
wchar_t     *fs_wcsrchr (wchar_t *pwcStr, wchar_t wcChar, char *pcFile, size_t zLine);
wchar_t     *fs_wcsstr (wchar_t *pwcStr1, wchar_t *pwcStr2, char *pcFile, size_t zLine);
wchar_t     *fs_wcstok (wchar_t *pwcStr1, wchar_t *pwcStr2, wchar_t **ppwcStr3, char *pcFile, size_t zLine);
wchar_t     *fs_wcspbrk (wchar_t *pwcStr1, wchar_t *pwcStr2, char *pcFile, size_t zLine);
wchar_t     *fs_wcsdup (wchar_t *pwcStr, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
size_t      fs_wcslen (wchar_t *pwcStr);
wchar_t     *fs_wcscpy (wchar_t *pwcStr1, wchar_t *pwcStr2);
wchar_t     *fs_wcsncpy (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
wchar_t     *fs_wcscat (wchar_t *pwcStr1, wchar_t *pwcStr2);
wchar_t     *fs_wcsncat (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
int         fs_wcscmp (wchar_t *pwcStr1, wchar_t *pwcStr2);
int         fs_wcsncmp (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
int         fs_wcscoll (wchar_t *pwcStr1, wchar_t *pwcStr2);
int         fs_wcscasecmp (wchar_t *pwcStr1, wchar_t *pwcStr2);
int         fs_wcsncasecmp (wchar_t *pwcStr1, wchar_t *pwcStr2, size_t zCount);
wchar_t     *fs_wcschr (wchar_t *pwcStr, wchar_t cChar);
wchar_t     *fs_wcsrchr (wchar_t *pwcStr, wchar_t cChar);
wchar_t     *fs_wcsstr (wchar_t *pwcStr1, wchar_t *pwcStr2);
wchar_t     *fs_wcstok (wchar_t *pwcStr1, wchar_t *pwcStr2, wchar_t **ppwcStr3);
wchar_t     *fs_wcspbrk (wchar_t *pwcStr1, wchar_t *pwcStr2);
wchar_t     *fs_wcsdup (wchar_t *pwcStr);
#endif



/* Enhanced numeric conversion handling functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
long        fs_strtol (char *pcStr, char **ppcEndStr, int iBase, char *pcFile, size_t zLine);
float       fs_strtof (char *pcStr, char **ppcEndStr, char *pcFile, size_t zLine);
double      fs_strtod (char *pcStr, char **ppcStr, char *pcFile, size_t zLine);
long        fs_wcstol (wchar_t *pwcStr, wchar_t **pwcEndStr, int iBase, char *pcFile, size_t zLine);
float       fs_wcstof (wchar_t *pwcStr, wchar_t **pwcEndStr, char *pcFile, size_t zLine);
double      fs_wcstod (wchar_t *pwcStr, wchar_t **pwcEndStr, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
long        fs_strtol (char *pcStr, char **ppcEndStr, int iBase);
float       fs_strtof (char *pcStr, char **ppcEndStr);
double      fs_strtod (char *pcStr, char **ppcEndStr);
long        fs_wcstol (wchar_t *pwcStr, wchar_t **pwcEndStr, int iBase);
float       fs_wcstof (wchar_t *pwcStr, wchar_t **pwcEndStr);
double     fs_wcstod (wchar_t *pwcStr, wchar_t **pwcEndStr);
#endif



/* Enhanced time functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
time_t      fs_time (time_t *ptTime, char *pcFile, size_t zLine);
double      fs_difftime (time_t tEndTime, time_t tStartTime, char *pcFile, size_t zLine);
int         fs_utime (char *pcFilePath, struct utimbuf *putmTimeBuffer, char *pcFile, size_t zLine);
int         fs_sleep (int iSeconds, char *pcFile, size_t zLine);
int         fs_usleep (int iMicroSeconds, char *pcFile, size_t zLine);
struct tm   *fs_localtime_r (time_t *ptTime, struct tm *ptmTime, char *pcFile, size_t zLine);
struct tm   *fs_gmtime_r (time_t *ptTime, struct tm *ptmTime, char *pcFile, size_t zLine);
time_t      fs_mktime (struct tm *ptmTime, char *pcFile, size_t zLine);
size_t      fs_strftime (char *pcStr, size_t zSize, char *pcFormat, struct tm *ptmTime, char *pcFile, size_t zLine);
char        *fs_strptime (char *pcStr, char *pcFormat, struct tm *ptmTime, char *pcFile, size_t zLine);
int         fs_gettimeofday(struct timeval *tp, struct timezone *tzp, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
time_t      fs_time (time_t *ptTime);
double      fs_difftime (time_t tEndTime, time_t tStartTime);
int         fs_utime (char *pcFilePath, struct utimbuf *putmTimeBuffer);
int         fs_sleep (int iSeconds);
int         fs_usleep (int iMicroSeconds);
struct tm   *fs_localtime_r (time_t *ptTime, struct tm *ptmTime);
struct tm   *fs_gmtime_r (time_t *ptTime, struct tm *ptmTime);
time_t      fs_mktime (struct tm *ptmTime);
size_t      fs_strftime (char *pcStr, size_t zSize, char *pcFormat, struct tm *ptmTime);
char        *fs_strptime (char *pcStr, char *pcFormat, struct tm *ptmTime);
int         fs_gettimeofday(struct timeval *ptvTimeVal, struct timezone *ptzTimeZone);
#endif



/* Enhanced process/UID functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_setuid (uid_t zUserID, char *pcFile, size_t zLine);
int         fs_setgid (gid_t zGroupID, char *pcFile, size_t zLine);
int         fs_setpgid (pid_t zProcessID, pid_t zProcessGroupID, char *pcFile, size_t zLine);
int         fs_getpgid (pid_t zProcessID, char *pcFile, size_t zLine);
int         fs_getpwnam_r (char *pcName, struct passwd *ppwPasswd, char *pucBuf, size_t zBufLen, struct passwd **pppwPasswd, char *pcFile, size_t zLine);
int         fs_getpwuid_r (uid_t zUserID, struct passwd *ppwPasswd, char *pucBuf, size_t zBufLen, struct passwd **pppwPasswd, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_setuid (uid_t zUserID);
int         fs_setgid (gid_t zGroupID);
int         fs_setpgid (pid_t zProcessID, pid_t zProcessGroupID);
int         fs_getpgid (pid_t zProcessID);
int         fs_getpwnam_r (char *pcName, struct passwd *ppwPasswd, char *pucBuf, size_t zBufLen, struct passwd **pppwPasswd);
int         fs_getpwuid_r (uid_t zUserID, struct passwd *ppwPasswd, char *pucBuf, size_t zBufLen, struct passwd **pppwPasswd);
#endif



/* Enhanced system functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
char        *fs_getenv (char *pcName, char *pcFile, size_t zLine);
int         fs_putenv (char *pcString, char *pcFile, size_t zLine);
pid_t       fs_fork (char *pcFile, size_t zLine);
int         fs_kill (pid_t zProcessID, int iSignal, char *pcFile, size_t zLine);
int         fs_wait (int *piStatus, char *pcFile, size_t zLine);
int         fs_waitpid (pid_t zProcessID, int *piStatus, int iOptions, char *pcFile, size_t zLine);
int         fs_system (char *pcCommand, char *pcFile, size_t zLine);
int         fs_sigaction(int iSignal, struct sigaction *psaSigAction, struct sigaction *psaSigActionOld, char *pcFile, size_t zLine);
int         fs_setitimer (int iTimer, struct itimerval *pIValNew, struct itimerval *pIValOld, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
char        *fs_getenv (char *pcName);
int         fs_putenv (char *pcString);
pid_t       fs_fork (void);
int         fs_kill (pid_t zProcessID, int iSignal);
int         fs_wait (int *piStatus);
int         fs_waitpid (pid_t zProcessID, int *piStatus, int iOptions);
int         fs_system (char *pcCommand);
int         fs_sigaction(int iSignal, struct sigaction *psaSigAction, struct sigaction *psaSigActionOld);
int         fs_setitimer (int iTimer, struct itimerval *pIValNew, struct itimerval *pIValOld);
#endif



/* Enhanced sort functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
void        fs_qsort (void *pvBase, size_t zCount, size_t zSize, int (*iCompFunction)(), char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
void        fs_qsort (void *pvBase, size_t zCount, size_t zSize, int (*iCompFunction)());
#endif



/* Enhanced memory mapping functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
void        *fs_mmap (void *pvAddress, size_t zLen, int iProt, int iFlags, int iFile, off_t zOffset, char *pcFile, size_t zLine);
int         fs_munmap (void *pvAddress, size_t zLen, char *pcFile, size_t zLine);
int         fs_mlock (void *pvAddress, size_t zLen, char *pcFile, size_t zLine);
int         fs_munlock (void *pvAddress, size_t zLen, char *pcFile, size_t zLine);
int         fs_mlockall (int iFlags, char *pcFile, size_t zLine);
int         fs_munlockall (char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
void        *fs_mmap (void *pvAddress, size_t zLen, int iProt, int iFlags, int iFile, off_t zOffset);
int         fs_munmap (void *pvAddress, size_t zLen);
int         fs_mlock (void *pvAddress, size_t zLen);
int         fs_munlock (void *pvAddress, size_t zLen);
int         fs_mlockall (int iFlags);
int     fs_munlockall (void);
#endif



/* Enhanced shared memory functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_shm_open (char *pcName, int iFlags, mode_t zMode, char *pcFile, size_t zLine);
int         fs_shm_unlink (char *pcName, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_shm_open (char *pcName, int iFlags, mode_t zMode);
int         fs_shm_unlink (char *pcName);
#endif



/* Enhanced semaphores functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
sem_t       *fs_sem_create (char *pcName, int iFlags, mode_t zMode, unsigned int iValue, char *pcFile, size_t zLine);
sem_t       *fs_sem_open (char *pcName, int iFlags, char *pcFile, size_t zLine);
int         fs_sem_wait (sem_t *psSemaphore, char *pcFile, size_t zLine);
int         fs_sem_post (sem_t *psSemaphore, char *pcFile, size_t zLine);
int         fs_sem_getvalue (sem_t *psSemaphore, int *piValue, char *pcFile, size_t zLine);
int         fs_sem_close (sem_t *psSemaphore, char *pcFile, size_t zLine);
int         fs_sem_unlink (char *pcName, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
sem_t       *fs_sem_create (char *pcName, int iFlags, mode_t zMode, unsigned int iValue);
sem_t       *fs_sem_open(char *pcName, int iFlags);
int         fs_sem_wait (sem_t *psSemaphore);
int         fs_sem_post (sem_t *psSemaphore);
int         fs_sem_getvalue (sem_t *psSemaphore, int *piValue);
int         fs_sem_close (sem_t *psSemaphore);
int         fs_sem_unlink (char *pcName);
#endif



/* Enhanced regex functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_regcomp (regex_t *prRegex, char *pcRegex, int iflags, char *pcFile, size_t zLine);
int         fs_regexec (regex_t *prRegex, char *pcString, size_t zNMatch, regmatch_t prmRMatch[], int iFlags, char *pcFile, size_t zLine);
void        fs_regfree (regex_t *prRegex, char *pcFile, size_t zLine);
#if defined(TRE_REGEX_ENABLE)
int         fs_regwcomp (regex_t *prRegex, wchar_t *pwcRegex, int iflags, char *pcFile, size_t zLine);
int         fs_regwexec (regex_t *prRegex, wchar_t *pwcString, size_t zNMatch, regmatch_t prmRMatch[], int iFlags, char *pcFile, size_t zLine);
#endif    /* TRE_REGEX_ENABLE */
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_regcomp (regex_t *prRegex, char *pcRegex, int iflags);
int         fs_regexec (regex_t *prRegex, char *pcString, size_t zNMatch, regmatch_t prmRMatch[], int iFlags);
void        fs_regfree (regex_t *prRegex);
#if defined(TRE_REGEX_ENABLE)
int         fs_regwcomp (regex_t *prRegex, wchar_t *pwcRegex, int iflags);
int         fs_regwexec (regex_t *prRegex, wchar_t *pwcString, size_t zNMatch, regmatch_t prmRMatch[]);
#endif    /* TRE_REGEX_ENABLE */
#endif



/* Enhanced thread functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
int         fs_pthread_create (pthread_t *ptThread, pthread_attr_t *pThreadAttributes, void *(*pvFunction)(void *), void *pvParameter, char *pcFile, size_t zLine);
int         fs_pthread_join (pthread_t tThread, void **ppvReturnValue, char *pcFile, size_t zLine);
int         fs_pthread_detach (pthread_t tThread, char *pcFile, size_t zLine);
int         fs_pthread_cancel (pthread_t tThread, char *pcFile, size_t zLine);
int         fs_pthread_mutex_lock (pthread_mutex_t *ptmThreadMutex, char *pcFile, size_t zLine);
int         fs_pthread_mutex_unlock (pthread_mutex_t *ptmThreadMutex, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
int         fs_pthread_create (pthread_t *ptThread, pthread_attr_t *pThreadAttributes, void *(*pvFunction)(void *), void *pvParameter);
int         fs_pthread_join (pthread_t tThread, void **ppvReturnValue);
int         fs_pthread_detach (pthread_t tThread);
int         fs_pthread_cancel (pthread_t tThread);
int         fs_pthread_mutex_lock (pthread_mutex_t *ptmThreadMutex);
int         fs_pthread_mutex_unlock (pthread_mutex_t *ptmThreadMutex);
#endif



/* Enhanced localization functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
char        *fs_setlocale (int iCategory, char *pcLocale, char *pcFile, size_t zLine);
iconv_t     fs_iconv_open (char *pcToCode, char *pcFromCode, char *pcFile, size_t zLine);
size_t      fs_iconv(iconv_t zDescriptor, char **ppcSource, size_t *pzSourceLen, char **ppcDestination, size_t *pzDestinationLen, char *pcFile, size_t zLine);
int         fs_iconv_close (iconv_t zDescriptor, char *pcFile, size_t zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
char        *fs_setlocale (int iCategory, char *pcLocale);
iconv_t     fs_iconv_open (char *pcToCode, char *pcFromCode);
size_t      fs_iconv(iconv_t zDescriptor, char **ppcSource, size_t *pzSourceLen, char **ppcDestination, size_t *pzDestinationLen);
int         fs_iconv_close (iconv_t zDescriptor);
#endif



/*---------------------------------------------------------------------------*/


/*
** Defines, use these guys
*/


/* Macros for enhanced memory allocation functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_malloc(s)                         fs_malloc((s), (__FILE__), (__LINE__))
#define s_calloc(n, s)                      fs_calloc((n),(s), (__FILE__), (__LINE__))
#define s_realloc(p, s)                     fs_realloc((p),(s), (__FILE__), (__LINE__))
#define s_free(p)                           { if ((p) != NULL ) { fs_free((p), (__FILE__), (__LINE__)); (p) = NULL; } }
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_malloc(s)                         fs_malloc((s))
#define s_calloc(c, s)                      fs_calloc((c), (s))
#define s_realloc(p, s)                     fs_realloc((p), (s))
#define s_free(p)                           { if ( (p) != NULL ) { free((p)); (p) = NULL; } }
#else
/* #define s_malloc(s)                      malloc((s)) */
#define s_malloc(s)                         calloc(1, (s))
#define s_calloc(c, s)                      calloc((c), (s))
#define s_realloc(p, s)                     realloc((p), (s))
#define s_free(p)                           { if ( (p) != NULL ) { free((p)); (p) = NULL; } }
#endif



/* Macros for enhanced memory handling functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_memcmp(p1, p2, n)                 fs_memcmp((p1), (p2), (n), (__FILE__), (__LINE__))
#define s_memcpy(p1, p2, n)                 fs_memcpy((p1), (p2), (n), (__FILE__), (__LINE__))
#define s_memmove(p1, p2, n)                fs_memmove((p1), (p2), (n), (__FILE__), (__LINE__))
#define s_memset(p, c, n)                   fs_memset((p), (c), (n), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_memcmp(p1, p2, n)                 fs_memcmp((p1), (p2), (n))
#define s_memcpy(p1, p2, n)                 fs_memcpy((p1), (p2), (n))
#define s_memmove(p1, p2, n)                fs_memmove((p1), (p2), (n))
#define s_memset(p, c, n)                   fs_memset((p), (c), (n))
#else
#define s_memcmp(p1, p2, n)                 memcmp((p1), (p2), (n))
#define s_memcpy(p1, p2, n)                 memcpy((p1), (p2), (n))
#define s_memmove(p1, p2, n)                memmove((p1), (p2), (n))
#define s_memset(p, c, n)                   memset((p), (c), (n))
#endif


/* Macros for enhanced memory handling functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_wmemcmp(s1, s2, n)                fs_wmemcmp((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_wmemcpy(s1, s2, n)                fs_wmemcpy((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_wmemmove(s1, s2, n)               fs_wmemmove((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_wmemset(s, c, n)                  fs_wmemset((s), (c), (n), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_wmemcmp(s1, s2, n)                fs_wmemcmp((s1), (s2), (n))
#define s_wmemcpy(s1, s2, n)                fs_wmemcpy((s1), (s2), (n))
#define s_wmemmove(s1, s2, n)               fs_wmemmove((s1), (s2), (n))
#define s_wmemset(s, c, n)                  fs_wmemset((s), (c), (n))
#else
#define s_wmemcmp(s1, s2, n)                wmemcmp((s1), (s2), (n))
#define s_wmemcpy(s1, s2, n)                wmemcpy((s1), (s2), (n))
#define s_wmemmove(s1, s2, n)               wmemmove((s1), (s2), (n))
#define s_wmemset(s, c, n)                  wmemset((s), (c), (n))
#endif




/* Macros for enhanced file access functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_fopen(n, m)                       fs_fopen((n), (m), (__FILE__), (__LINE__))
#define s_fdopen(d, m)                      fs_fdopen((d), (m), (__FILE__), (__LINE__))
#define s_freopen(n, m, f)                  fs_freopen((n), (m), (f), (__FILE__), (__LINE__))
#define s_fclose(f)                         { fs_fclose(((FILE*)f), (__FILE__), (__LINE__)); (f) = NULL; }
#define s_fread(p, s, n, f)                 fs_fread((p), (s), (n), (f), (__FILE__), (__LINE__))
#define s_fwrite(p, s, n, f)                fs_fwrite((p), (s), (n), (f), (__FILE__), (__LINE__))
#define s_fgets(s, n, f)                    fs_fgets((s), (n), (f), (__FILE__), (__LINE__))
#define s_fputs(s, f)                       fs_fputs((s), (f), (__FILE__), (__LINE__))
#define s_fflush(f)                         fs_fflush((f), (__FILE__), (__LINE__))
#define s_fseek(f, o, w)                    fs_fseek((f), (o), (w), (__FILE__), (__LINE__))
#define s_ftell(f)                          fs_ftell((f), (__FILE__), (__LINE__))
#define s_feof(f)                           fs_feof((f), (__FILE__), (__LINE__))
#define s_ferror(f)                         fs_ferror((f), (__FILE__), (__LINE__))
#define s_clearerr(f)                       fs_clearerr((f), (__FILE__), (__LINE__))
#define s_truncate(f, l)                    fs_truncate((f), (l), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_fopen(n, m)                       fs_fopen((n), (m))
#define s_fdopen(d, m)                      fs_fdopen((d), (m))
#define s_freopen(n, m, f)                  fs_freopen((n), (m), (f))
#define s_fclose(f)                         { fs_fclose((FILE*)f); (f) = NULL; }
#define s_fread(p, s, n, f)                 fs_fread((p), (s), (n), (f))
#define s_fwrite(p, s, n, f)                fs_fwrite((p), (s), (n), (f))
#define s_fgets(s, n, f)                    fs_fgets((s), (n), (f))
#define s_fputs(s, f)                       fs_fputs((s), (f))
#define s_fflush(f)                         fs_fflush((f))
#define s_fseek(f, o, w)                    fs_fseek((f), (o), (w))
#define s_ftell(f)                          fs_ftell((f))
#define s_feof(f)                           fs_feof((f))
#define s_ferror(f)                         fs_ferror((f))
#define s_clearerr(f)                       fs_clearerr((f))
#define s_truncate(f, l)                    fs_truncate((f), (l))
#else
#define s_fopen(n, m)                       fopen((n), (m))
#define s_fdopen(d, m)                      fdopen((d), (m))
#define s_freopen(n, m, f)                  freopen((n), (m), (f))
#define s_fclose(f)                         { if ( (f) != NULL ) { fclose((FILE *)(f)); (f) = NULL; } }
#define s_fread(p, s, n, f)                 fread((p), (s), (n), (f))
#define s_fwrite(p, s, n, f)                fwrite((p), (s), (n), (f))
#define s_fgets(s, n, f)                    fgets((s), (n), (f))
#define s_fputs(s, f)                       fgets((s), (f))
#define s_fflush(f)                         fflush((f))
#define s_fseek(f, o, w)                    fseek((f), (o), (w))
#define s_ftell(f)                          ftell((f))
#define s_feof(f)                           feof((f))
#define s_ferror(f)                         ferror((f))
#define s_clearerr(f)                       clearerr((f))
#define s_truncate(f, l)                    truncate((f), (l))
#endif




/* Macros enhanced low level file I/O functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_open(p, f, m)                     fs_open((p), (f), (m), (__FILE__), (__LINE__))
#define s_close(d)                          fs_close((d), (__FILE__), (__LINE__))
#define s_read(f, p, n)                     fs_read((f), (p), (n), (__FILE__), (__LINE__))
#define s_write(f, p, n)                    fs_write((f), (p), (n), (__FILE__), (__LINE__))
#define s_lseek(f, o, w)                    fs_lseek((f), (o), (w), (__FILE__), (__LINE__))
#define s_ioctl(f, r, a)                    fs_ioctl((f), (r), (a), (__FILE__), (__LINE__))
#define s_poll(p, n, t)                     fs_poll((p), (n), (t), (__FILE__), (__LINE__))
#define s_pipe(f)                           fs_pipe((f), (__FILE__), (__LINE__))
#define s_dup(f)                            fs_dup((f), (__FILE__), (__LINE__))
#define s_dup2(f1, f2)                      fs_dup2((f1), (f2), (__FILE__), (__LINE__))
#define s_ftruncate(f, l)                   fs_ftruncate((f), (l), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_open(p, f, m)                     fs_open((p), (f), (m))
#define s_close(d)                          fs_close((d))
#define s_read(f, p, n)                     fs_read((f), (p), (n))
#define s_write(f, p, n)                    fs_write((f), (p), (n))
#define s_lseek(f, o, w)                    fs_lseek((f), (o), (w))
#define s_ioctl(f, r, a)                    fs_ioctl((f), (r), (a))
#define s_poll(p, n, t)                     fs_poll((p), (n), (t))
#define s_pipe(f)                           fs_pipe((f))
#define s_dup(f)                            fs_dup((f))
#define s_dup2(f1, f2)                      fs_dup2((f1), (f2))
#define s_ftruncate(f, l)                   fs_ftruncate((f), (l))
#else
#define s_open(p, f, m)                     open((p), (f), (m))
#define s_close(d)                          close((d))
#define s_read(f, p, n)                     read((f), (p), (n))
#define s_write(f, p, n)                    write((f), (p), (n))
#define s_lseek(f, o, w)                    lseek((f), (o), (w))
#define s_ioctl(f, r, a)                    ioctl((f), (r), (a))
#define s_poll(p, n, t)                     poll((p), (n), (t))
#define s_pipe(f)                           pipe((f))
#define s_dup(f)                            dup((f))
#define s_dup2(f1, f2)                      dup2((f1), (f2))
#define s_ftruncate(f, l)                   ftruncate((f), (l))
#endif



/* Macros for enhanced low level socket I/O functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_socket(d, t, p)                   fs_socket((d), (t), (p), (__FILE__), (__LINE__))
#define s_bind(s, a, l)                     fs_bind((s), (a), (l), (__FILE__), (__LINE__))
#define s_connect(s, a, l)                  fs_connect((s), (a), (l), (__FILE__), (__LINE__))
#define s_accept(s, a, l)                   fs_accept((s), (a), (l), (__FILE__), (__LINE__))
#define s_listen(s, b)                      fs_listen((s), (b), (__FILE__), (__LINE__))
#define s_setsockopt(s, l, n, v, z)         fs_setsockopt((s), (l), (n), (v), (z), (__FILE__), (__LINE__))
#define s_getsockopt(s, l, n, v, z)         fs_getsockopt((s), (l), (n), (v), (z), (__FILE__), (__LINE__))
#define s_recv(s, p, n, f)                  fs_recv((s), (p), (n), (f), (__FILE__), (__LINE__))
#define s_send(s, p, n, f)                  fs_send((s), (p), (n), (f), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_socket(d, t, p)                   fs_socket((d), (t), (p))
#define s_bind(s, a, l)                     fs_bind((s), (a), (l))
#define s_connect(s, a, l)                  fs_connect((s), (a), (l))
#define s_accept(s, a, l)                   fs_accept((s), (a), (l))
#define s_listen(s, b)                      fs_listen((s), (b))
#define s_setsockopt(s, l, n, v, z)         fs_setsockopt((s), (l), (n), (v), (z))
#define s_getsockopt(s, l, n, v, z)         fs_getsockopt((s), (l), (n), (v), (z))
#define s_recv(s, p, n, f)                  fs_recv((s), (p), (n), (f))
#define s_send(s, p, n, f)                  fs_send((s), (p), (n), (f))
#else
#define s_socket(d, t, p)                   socket((d), (t), (p))
#define s_bind(s, a, l)                     bind((s), (a), (l))
#define s_connect(s, a, l)                  connect((s), (a), (l))
#define s_accept(s, a, l)                   accept((s), (a), (l))
#define s_listen(s, b)                      listen((s), (b))
#define s_setsockopt(s, l, n, v, z)         setsockopt((s), (l), (n), (v), (z)))
#define s_getsockopt(s, l, n, v, z)         getsockopt((s), (l), (n), (v), (z)))
#define s_recv(s, p, n, f)                  recv((s), (p), (n), (f))
#define s_send(s, p, n, f)                  send((s), (p), (n), (f))
#endif




/* Macros for enhanced file manipulation functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_remove(n)                         fs_remove((n), (__FILE__), (__LINE__))
#define s_rename(o, n)                      fs_rename((o), (n), (__FILE__), (__LINE__))
#define s_tmpfile()                         fs_tmpfile((__FILE__), (__LINE__))
#define s_tmpnam(n)                         fs_tmpnam((n), (__FILE__), (__LINE__))
#define s_stat(n, s)                        fs_stat((n), (s), (__FILE__), (__LINE__))
#define s_lstat(n, s)                       fs_lstat((n), (s), (__FILE__), (__LINE__))
#define s_fstat(f, s)                       fs_fstat((f), (s), (__FILE__), (__LINE__))
#define s_access(f, m)                      fs_access((f), (m), (__FILE__), (__LINE__))
#define s_chmod(f, m)                       fs_chmod((f), (m), (__FILE__), (__LINE__))
#define s_chown(f, u, g)                    fs_chown((f), (u), (g), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_remove(n)                         fs_remove((n))
#define s_rename(o, n)                      fs_rename((o), (n))
#define s_tmpfile()                         fs_tmpfile()
#define s_tmpnam(n)                         fs_tmpnam((n))
#define s_stat(n, s)                        fs_stat((n), (s))
#define s_lstat(n, s)                       fs_lstat((n), (s))
#define s_fstat(f, s)                       fs_fstat((f), (s))
#define s_access(f, m)                      fs_access((f), (m))
#define s_chmod(f, m)                       fs_chmod((f), (m))
#define s_chown(f, u, g)                    fs_chown((f), (u), (g))
#else
#define s_remove(n)                         remove((n))
#define s_rename(o, n)                      rename((o), (n))
#define s_tmpfile()                         tmpfile()
#define s_tmpnam(n)                         tmpnam((n))
#define s_stat(n, s)                        stat((n), (s))
#define s_lstat(n, s)                       lstat((n), (s))
#define s_fstat(f, s)                       fstat((f), (s))
#define s_access(f, m)                      access((f), (m))
#define s_chmod(f, m)                       chmod((f), (m))
#define s_chown(f, u, g)                    chown((f), (u), (g))
#endif



/* Macros for enhanced directory manipulation functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_getcwd(p, n)                      fs_getcwd((p), (n), (__FILE__), (__LINE__))
#define s_mkdir(d, m)                       fs_mkdir((d), (m), (__FILE__), (__LINE__))
#define s_rmdir(d)                          fs_rmdir((d), (__FILE__), (__LINE__))
#define s_chdir(d)                          fs_chdir((d), (__FILE__), (__LINE__))
#define s_opendir(d)                        fs_opendir((d), (__FILE__), (__LINE__))
#define s_readdir(d)                        fs_readdir((d), (__FILE__), (__LINE__))
#define s_rewinddir(d)                      fs_rewinddir((d), (__FILE__), (__LINE__))
#define s_closedir(d)                       fs_closedir((d), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_getcwd(p,n)                       fs_getcwd((p),(n))
#define s_mkdir(d, m)                       fs_mkdir((d), (m))
#define s_rmdir(d)                          fs_rmdir((d))
#define s_chdir(d)                          fs_chdir((d))
#define s_opendir(d)                        fs_opendir((d))
#define s_readdir(d)                        fs_readdir((d))
#define s_rewinddir(d)                      fs_rewinddir((d))
#define s_closedir(d)                       fs_closedir((d))
#else
#define s_getcwd(p,n)                       getcwd((p),(n))
#define s_mkdir(d, m)                       mkdir((d), (m))
#define s_rmdir(d)                          rmdir((d))
#define s_chdir(d)                          chdir((d))
#define s_opendir(d)                        opendir((d))
#define s_readdir(d)                        readdir((d))
#define s_rewinddir(d)                      rewinddir((d))
#define s_closedir(d)                       closedir((d))
#endif



/* Macros for enhanced string functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_strlen(s)                         fs_strlen((s), (__FILE__), (__LINE__))
#define s_strcpy(s1, s2)                    fs_strcpy((s1), (s2), (__FILE__), (__LINE__))
#define s_strncpy(s1, s2, n)                fs_strncpy((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_strcat(s1, s2)                    fs_strcat((s1), (s2), (__FILE__), (__LINE__))
#define s_strncat(s1, s2, n)                fs_strncat((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_strcmp(s1, s2)                    fs_strcmp((s1), (s2), (__FILE__), (__LINE__))
#define s_strncmp(s1, s2, n)                fs_strncmp((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_strcoll(s1, s2)                   fs_strcoll((s1), (s2), (__FILE__), (__LINE__))
#define s_strcasecmp(s1, s2)                fs_strcasecmp((s1), (s2), (__FILE__), (__LINE__))
#define s_strncasecmp(s1, s2, n)            fs_strncasecmp((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_strchr(s, c)                      fs_strchr((s), (c), (__FILE__), (__LINE__))
#define s_strrchr(s, c)                     fs_strrchr((s), (c), (__FILE__), (__LINE__))
#define s_strstr(s1, s2)                    fs_strstr((s1), (s2), (__FILE__), (__LINE__))
#define s_strtok_r(s1, s2, s3)              fs_strtok_r((s1), (s2), (s3), (__FILE__), (__LINE__))
#define s_strpbrk(s1, s2)                   fs_strpbrk((s1), (s2), (__FILE__), (__LINE__))
#define s_strdup(s)                         fs_strdup((s), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_strlen(s)                            fs_strlen((s))
#define s_strcpy(s1, s2)                    fs_strcpy((s1), (s2))
#define s_strncpy(s1, s2, n)                fs_strncpy((s1), (s2), (n))
#define s_strcat(s1, s2)                    fs_strcat((s1), (s2))
#define s_strncat(s1, s2, n)                fs_strncat((s1), (s2), (n))
#define s_strcmp(s1, s2)                    fs_strcmp((s1), (s2))
#define s_strncmp(s1, s2, n)                fs_strncmp((s1), (s2), (n))
#define s_strcoll(s1, s2)                   fs_strcoll((s1), (s2))
#define s_strcasecmp(s1, s2)                fs_strcasecmp((s1), (s2))
#define s_strncasecmp(s1, s2, n)            fs_strncasecmp((s1), (s2), (n))
#define s_strchr(s, c)                      fs_strchr((s), (c))
#define s_strrchr(s, c)                     fs_strrchr((s), (c))
#define s_strstr(s1, s2)                    fs_strstr((s1), (s2))
#define s_strtok_r(s1, s2, s3)              fs_strtok_r((s1), (s2), (s3))
#define s_strpbrk(s1, s2)                   fs_strpbrk((s1), (s2))
#define s_strdup(s)                         fs_strdup((s))
#else
#define s_strlen(s)                         strlen((s))
#define s_strcpy(s1, s2)                    strcpy((s1), (s2))
#define s_strncpy(s1, s2, n)                strncpy((s1), (s2), (n))
#define s_strcat(s1, s2)                    strcat((s1), (s2))
#define s_strncat(s1, s2, n)                strncat((s1), (s2), (n))
#define s_strcmp(s1, s2)                    strcmp((s1), (s2))
#define s_strncmp(s1, s2, n)                strncmp((s1), (s2), (n))
#define s_strcoll(s1, s2)                   strcoll((s1), (s2))
#define s_strcasecmp(s1, s2)                strcasecmp((s1), (s2))
#define s_strncasecmp(s1, s2, n)            strncasecmp((s1), (s2), (n))
#define s_strchr(s, c)                      strchr((s), (c))
#define s_strrchr(s, c)                     strrchr((s), (c))
#define s_strstr(s1, s2)                    strstr((s1), (s2))
#define s_strtok_r(s1, s2, s3)              strtok_r((s1), (s2), (s3))
#define s_strpbrk(s1, s2)                   strpbrk((s1), (s2))
#define s_strdup(s)                         strdup((s))
#endif




/* Macros for enhanced string functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_wcslen(s)                         fs_wcslen((s), (__FILE__), (__LINE__))
#define s_wcscpy(s1, s2)                    fs_wcscpy((s1), (s2), (__FILE__), (__LINE__))
#define s_wcsncpy(s1, s2, n)                fs_wcsncpy((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_wcscat(s1, s2)                    fs_wcscat((s1), (s2), (__FILE__), (__LINE__))
#define s_wcsncat(s1, s2, n)                fs_wcsncat((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_wcscmp(s1, s2)                    fs_wcscmp((s1), (s2), (__FILE__), (__LINE__))
#define s_wcsncmp(s1, s2, n)                fs_wcsncmp((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_wcscoll(s1, s2)                   fs_wcscoll((s1), (s2), (__FILE__), (__LINE__))
#define s_wcscasecmp(s1, s2)                fs_wcscasecmp((s1), (s2), (__FILE__), (__LINE__))
#define s_wcsncasecmp(s1, s2, n)            fs_wcsncasecmp((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_wcschr(s, c)                      fs_wcschr((s), (c), (__FILE__), (__LINE__))
#define s_wcsrchr(s, c)                     fs_wcsrchr((s), (c), (__FILE__), (__LINE__))
#define s_wcsstr(s1, s2)                    fs_wcsstr((s1), (s2), (__FILE__), (__LINE__))
#define s_wcstok(s1, s2, s3)                fs_wcstok((s1), (s2), (s3), (__FILE__), (__LINE__))
#define s_wcspbrk(s1, s2)                   fs_wcspbrk((s1), (s2), (__FILE__), (__LINE__))
#define s_wcsdup(s)                         fs_wcsdup((s), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_wcslen(s)                         fs_wcslen((s))
#define s_wcscpy(s1, s2)                    fs_wcscpy((s1), (s2))
#define s_wcsncpy(s1, s2, n)                fs_wcsncpy((s1), (s2), (n))
#define s_wcscat(s1, s2)                    fs_wcscat((s1), (s2))
#define s_wcsncat(s1, s2, n)                fs_wcsncat((s1), (s2), (n))
#define s_wcscmp(s1, s2)                    fs_wcscmp((s1), (s2))
#define s_wcsncmp(s1, s2, n)                fs_wcsncmp((s1), (s2), (n))
#define s_wcscoll(s1, s2)                   fs_wcscoll((s1), (s2))
#define s_wcscasecmp(s1, s2)                fs_wcscasecmp((s1), (s2))
#define s_wcsncasecmp(s1, s2, n)            fs_wcsncasecmp((s1), (s2), (n))
#define s_wcschr(s, c)                      fs_wcschr((s), (c))
#define s_wcsrchr(s, c)                     fs_wcsrchr((s), (c))
#define s_wcsstr(s1, s2)                    fs_wcsstr((s1), (s2))
#define s_wcstok(s1, s2, s3)                fs_wcstok((s1), (s2), (s3))
#define s_wcspbrk(s1, s2)                   fs_wcspbrk((s1), (s2))
#define s_wcsdup(s)                         fs_wcsdup((s))
#else
#define s_wcslen(s)                         wcslen((s))
#define s_wcscpy(s1, s2)                    wcscpy((s1), (s2))
#define s_wcsncpy(s1, s2, n)                wcsncpy((s1), (s2), (n))
#define s_wcscat(s1, s2)                    wcscat((s1), (s2))
#define s_wcsncat(s1, s2, n)                wcsncat((s1), (s2), (n))
#define s_wcscmp(s1, s2)                    wcscmp((s1), (s2))
#define s_wcsncmp(s1, s2, n)                wcsncmp((s1), (s2), (n))
#define s_wcscoll(s1, s2)                   wcscoll((s1), (s2))
#define s_wcscasecmp(s1, s2)                wcscasecmp((s1), (s2))
#define s_wcsncasecmp(s1, s2, n)            wcsncasecmp((s1), (s2), (n))
#define s_wcschr(s, c)                      wcschr((s), (c))
#define s_wcsrchr(s, c)                     wcsrchr((s), (c))
#define s_wcsstr(s1, s2)                    wcsstr((s1), (s2))
#define s_wcstok(s1, s2, s3)                wcstok((s1), (s2), (s3))
#define s_wcspbrk(s1, s2)                   wcspbrk((s1), (s2))
#define s_wcsdup(s)                         wcsdup((s))
#endif




/* Macros for enhanced number convertion functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_strtol(s, e, b)                   fs_strtol((s), (e), (b), (__FILE__), (__LINE__))
#define s_strtof(s, e)                      fs_strtof((s), (e), (__FILE__), (__LINE__))
#define s_strtod(s, e)                      fs_strtod((s), (e), (__FILE__), (__LINE__))
#define s_wcstol(s, e, b)                   fs_wcstol((s), (e), (b), (__FILE__), (__LINE__))
#define s_wcstof(s, e)                      fs_wcstof((s), (e), (__FILE__), (__LINE__))
#define s_wcstod(s, e)                      fs_wcstod((s), (e), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_strtol(s, e, b)                   fs_strtol((s), (e), (b))
#define s_strtof(s, e)                      fs_strtof((s), (e))
#define s_strtod(s, e)                      fs_strtod((s), (e))
#define s_wcstol(s, e, b)                   fs_wcstol((s), (e), (b))
#define s_wcstof(s, e)                      fs_wcstof((s), (e))
#define s_wcstod(s, e)                      fs_wcstod((s), (e))
#else
#define s_strtol(s, e, b)                   strtol((s), (e), (b))
#define s_strtof(s, e)                      strtof((s), (e))
#define s_strtod(s, e)                      strtod((s), (e))
#define s_wcstol(s, e, b)                   wcstol((s), (e), (b))
#define s_wcstof(s, e)                      wcstof((s), (e))
#define s_wcstod(s, e)                      wcstod((s), (e))
#endif




/* Macros for enhanced time functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_time(t)                           fs_time((t), (__FILE__), (__LINE__))
#define s_difftime(e, s)                    fs_difftime((e), (s), (__FILE__), (__LINE__))
#define s_utime(f, t)                       fs_utime((f), (t), (__FILE__), (__LINE__))
#define s_sleep(s)                          fs_sleep((s), (__FILE__), (__LINE__))
#define s_usleep(s)                         fs_usleep((s), (__FILE__), (__LINE__))
#define s_localtime_r(t, r)                 fs_localtime_r((t), (r), (__FILE__), (__LINE__))
#define s_gmtime_r(t, r)                    fs_gmtime_r((t), (r), (__FILE__), (__LINE__))
#define s_mktime(t)                         fs_mktime((t), (__FILE__), (__LINE__))
#define s_strftime(s, l, f, t)              fs_strftime((s), (l), (f), (t), (__FILE__), (__LINE__))
#define s_strptime(s, f, t)                 fs_strptime((s), (f), (t), (__FILE__), (__LINE__))
#define s_gettimeofday(t, z)                fs_gettimeofday((t), (z), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_time(t)                           fs_time((t))
#define s_difftime(e, s)                    fs_difftime((e), (s))
#define s_utime(f, t)                       fs_utime((f), (t))
#define s_sleep(s)                          fs_sleep((s))
#define s_usleep(s)                         fs_usleep((s))
#define s_localtime_r(t, r)                 fs_localtime_r((t), (r))
#define s_gmtime_r(t, r)                    fs_gmtime_r((t), (r))
#define s_mktime(t)                         fs_mktime((t))
#define s_strftime(s, l, f, t)              fs_strftime((s), (l), (f), (t))
#define s_strptime(s, f, t)                 fs_strptime((s), (f), (t))
#define s_gettimeofday(t, z)                fs_gettimeofday((t), (z))
#else
#define s_time(t)                           time((t))
#define s_difftime(e, s)                    difftime((e), (s))
#define s_utime(f, t)                       utime((f), (t))
#define s_sleep(s)                          sleep((s))
#define s_usleep(s)                         usleep((s))
#define s_localtime_r(t, r)                 localtime_r((t), (r))
#define s_gmtime_r(t, r)                    gmtime_r((t), (r))
#define s_mktime(t)                         mktime((t))
#define s_strftime(s, l, f, t)              strftime((s), (l), (f), (t))
#define s_strptime(s, f, t)                 strptime((s), (f), (t))
#define s_gettimeofday(t, z)                gettimeofday((t), (z))
#endif


/* Macros for enhanced process/UID functions */
#if defined(UTL_CWRAPPERS_ENABLE) || defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_getuid()                          getuid()
#define s_geteuid()                         geteuid()
#define s_getgid()                          getgid()
#define s_getegid()                         getegid()
#define s_getpid()                          getpid()
#define s_getppid()                         getppid()
#define s_getpgrp()                         getpgrp()
#define s_setpgrp()                         setpgrp()
#endif

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_setuid(u)                         fs_setuid((u), (__FILE__), (__LINE__))
#define s_setgid(g)                         fs_setgid((g), (__FILE__), (__LINE__))
#define s_setpgid(p, g)                     fs_setpgid((p), (g), (__FILE__), (__LINE__))
#define s_getpgid(p)                        fs_getpgid((p), (__FILE__), (__LINE__))
#define s_getpwnam_r(n, p, b, l, r)         fs_getpwnam_r((n), (p), (b), (l), (r), (__FILE__), (__LINE__))
#define s_getpwuid_r(u, p, b, l, r)         fs_getpwuid_r((u), (p), (b), (l), (r), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_setuid(u)                         fs_setuid((u))
#define s_setgid(g)                         fs_setgid((g))
#define s_setpgid(p, g)                     fs_setpgid((p), (g))
#define s_getpgid(p)                        fs_getpgid((p))
#define s_getpwnam_r(n, p, b, l, r)         fs_getpwnam_r((n), (p), (b), (l), (r))
#define s_getpwuid_r(u, p, b, l, r)         fs_getpwuid_r((u), (p), (b), (l), (r))
#else
#define s_setuid(u)                            setuid((u))
#define s_setgid(g)                         setgid((g))
#define s_setpgid(p, g)                     setpgid((p), (g))
#define s_getpgid(p)                        getpgid((p))
#define s_getpwnam_r(n, p, b, l, r)         getpwnam_r((n), (p), (b), (l), (r))
#define s_getpwuid_r(u, p, b, l, r)         getpwuid_r((u), (p), (b), (l), (r))
#endif


/* Macros for enhanced system call functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_getenv(n)                         fs_getenv((n), (__FILE__), (__LINE__))
#define s_putenv(s)                         fs_putenv((s), (__FILE__), (__LINE__))
#define s_fork()                            fs_fork((__FILE__), (__LINE__))
#define s_kill(p, s)                        fs_kill((p), (s), (__FILE__), (__LINE__))
#define s_wait(s)                           fs_wait((s), (__FILE__), (__LINE__))
#define s_waitpid(p, s, o)                  fs_waitpid((p), (s), (o), (__FILE__), (__LINE__))
#define s_system(c)                         fs_system((c), (__FILE__), (__LINE__))
#define s_sigaction(s, n, o)                fs_sigaction((s), (n), (o), (__FILE__), (__LINE__))
#define s_setitimer(t, n, o)                fs_setitimer((t), (n), (o), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_getenv(n)                         fs_getenv((n))
#define s_putenv(s)                         fs_putenv((s))
#define s_fork()                            fs_fork()
#define s_kill(p, s)                        fs_kill((p), (s))
#define s_wait(s)                           fs_wait((s))
#define s_waitpid(p, s, o)                  fs_waitpid((p), (s), (o))
#define s_system(c)                         fs_system((c))
#define s_sigaction(s, n, o)                fs_sigaction((s), (n), (o))
#define s_setitimer(t, n, o)                fs_setitimer((t), (n), (o))
#else
#define s_getenv(n)                         getenv((n))
#define s_putenv(s)                         putenv((s))
#define s_fork()                            fork()
#define s_kill(p, s)                        kill((p), (s))
#define s_wait(s)                           wait((s))
#define s_waitpid(p, s, o)                  waitpid((p), (s), (o))
#define s_system(c)                         system((c))
#define s_sigaction(s, n, o)                sigaction((s), (n), (o))
#define s_setitimer(t, n, o)                setitimer((t), (n), (o))
#endif


/* Macros for enhanced sort function */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_qsort(b, c, s, f)                 fs_qsort((b), (c), (s), (f), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_qsort(b, c, s, f)                 fs_qsort((b), (c), (s), (f))
#else
#define s_qsort(b, c, s, f)                 qsort((b), (c), (s), (f))
#endif


/* Macros for enhanced memory mapping */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_mmap(a, l, p, fl, fd, o)          fs_mmap((a), (l), (p), (fl), (fd), (o), (__FILE__), (__LINE__))
#define s_munmap(a, l)                      { fs_munmap((a), (l), (__FILE__), (__LINE__)); (a) = NULL; }
#define s_mlock(a, l)                       fs_mlock((a), (l), (__FILE__), (__LINE__))
#define s_munlock(a, l)                     fs_munlock((a), (l), (__FILE__), (__LINE__))
#define s_mlockall(f)                       fs_mlockall((f), (__FILE__), (__LINE__))
#define s_munlockall()                      fs_munlockall((__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_mmap(a, l, p, fl, fd, o)          fs_mmap((a), (l), (p), (fl), (fd), (o))
#define s_munmap(a, l)                      { fs_munmap((a), (l)); (a) = NULL; }
#define s_mlock(a, l)                       fs_mlock((a), (l))
#define s_munlock(a, l)                     fs_munlock((a), (l))
#define s_mlockall(f)                       fs_mlockall((f))
#define s_munlockall()                      fs_munlockall()
#else
#define s_mmap(a, l, p, fl, fd, o)          mmap((a), (l), (p), (fl), (fd), (o))
#define s_munmap(a, l)                      { if ( (a) != NULL ) { munmap((a), (l)); (a) = NULL; } }
#define s_mlock(a, l)                       mlock((a), (l))
#define s_munlock(a, l)                     munlock((a), (l))
#define s_mlockall(f)                       mlockall((f))
#define s_munlockall()                      munlockall()
#endif


/* Macros for enhanced shared memory functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_shm_open(n, f, m)                 fs_shm_open((n), (f), (m), (__FILE__), (__LINE__))
#define s_shm_unlink(n)                     fs_shm_unlink((n), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_shm_open(n, f, m)                 fs_shm_open((n), (f), (m))
#define s_shm_unlink(n)                     fs_shm_unlink((n))
#else
#define s_shm_open(n, f, m)                 shm_open((n), (f), (m))
#define s_shm_unlink(n)                     shm_unlink((n))
#endif


/* Macros for enhanced semaphore functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_sem_create(n, f, m, v)            fs_sem_create((n), (f), (m), (v), (__FILE__), (__LINE__))
#define s_sem_open(n, f)                    fs_sem_open((n), (f), (__FILE__), (__LINE__))
#define s_sem_wait(s)                       fs_sem_wait((s), (__FILE__), (__LINE__))
#define s_sem_post(s)                       fs_sem_post((s), (__FILE__), (__LINE__))
#define s_sem_getvalue(s, v)                fs_sem_getvalue((s), (v), (__FILE__), (__LINE__))
#define s_sem_close(s)                      fs_sem_close((s), (__FILE__), (__LINE__))
#define s_sem_unlink(n)                     fs_sem_unlink((n), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_sem_create(n, f, m, v)            fs_sem_create((n), (f), (m), (v))
#define s_sem_open(n, f)                    fs_sem_open((n), (f))
#define s_sem_wait(s)                       fs_sem_wait((s))
#define s_sem_post(s)                       fs_sem_post((s))
#define s_sem_getvalue(s, v)                fs_sem_getvalue((s), (v))
#define s_sem_close(s)                      fs_sem_close((s))
#define s_sem_unlink(n)                     fs_sem_unlink((n))
#else
#define s_sem_create(n, f, m, v)            sem_open((n), (f), (m), (v))
#define s_sem_open(n, f)                    sem_open((n), (f))
#define s_sem_wait(s)                       sem_wait((s))
#define s_sem_post(s)                       sem_post((s))
#define s_sem_getvalue(s, v)                sem_getvalue((s), (v))
#define s_sem_close(s)                      sem_close((s))
#define s_sem_unlink(n)                     sem_unlink((n))
#endif


/* Macros for enhanced regex functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_regcomp(r, e, f)                  fs_regcomp((r), (e), (f), (__FILE__), (__LINE__))
#define s_regexec(r, s, n, p, e)            fs_regexec((r), (s), (n), (p), (e), (__FILE__), (__LINE__))
#define s_regfree(r)                        fs_regfree((r), (__FILE__), (__LINE__))
#if defined(TRE_REGEX_ENABLE)
#define s_regwcomp(r, e, f)                 fs_regcomp((r), (e), (f), (__FILE__), (__LINE__))
#define s_regwexec(r, s, n, p, e)           fs_regexec((r), (s), (n), (p), (e), (__FILE__), (__LINE__))
#endif    /* TRE_REGEX_ENABLE */
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_regcomp(r, e, f)                  fs_regcomp((r), (e), (f))
#define s_regexec(r, s, n, p, e)            fs_regexec((r), (s), (n), (p), (e))
#define s_regfree(r)                        fs_regfree((r))
#if defined(TRE_REGEX_ENABLE)
#define s_regwcomp(r, e, f)                 fs_regcomp((r), (e), (f))
#define s_regwexec(r, s, n, p, e)           fs_regexec((r), (s), (n), (p), (e))
#endif    /* TRE_REGEX_ENABLE */
#else
#define s_regcomp(r, e, f)                  regcomp((r), (e), (f))
#define s_regexec(r, s, n, p, e)            regexec((r), (s), (n), (p), (e))
#define s_regfree(r)                        regfree((r))
#if defined(TRE_REGEX_ENABLE)
#define s_regwcomp(r, e, f)                 regcomp((r), (e), (f))
#define s_regwexec(r, s, n, p, e)           regexec((r), (s), (n), (p), (e))
#endif    /* TRE_REGEX_ENABLE */
#endif


/* Macros for enhanced pthreads functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_pthread_create(t, a, f, p)        fs_pthread_create((t), (a), (f), (p), (__FILE__), (__LINE__))
#define s_pthread_join(t, r)                fs_pthread_join((t), (r), (__FILE__), (__LINE__))
#define s_pthread_detach(t)                 fs_pthread_detach((t), (__FILE__), (__LINE__))
#define s_pthread_cancel(t)                 fs_pthread_cancel((t), (__FILE__), (__LINE__))
#define s_pthread_self()                    pthread_self()
#define s_pthread_exit(r)                   pthread_exit((r))
#define s_pthread_mutex_lock(l)             fs_pthread_mutex_lock((l), (__FILE__), (__LINE__))
#define s_pthread_mutex_unlock(l)           fs_pthread_mutex_unlock((l), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_pthread_create(t, a, f, p)        fs_pthread_create((t), (a), (f), (p))
#define s_pthread_join(t, r)                fs_pthread_join((t), (r))
#define s_pthread_detach(t)                 fs_pthread_detach((t))
#define s_pthread_cancel(t)                 fs_pthread_cancel((t))
#define s_pthread_self()                    pthread_self()
#define s_pthread_exit(r)                   pthread_exit((r))
#define s_pthread_mutex_lock(l)             fs_pthread_mutex_lock((l))
#define s_pthread_mutex_unlock(l)           fs_pthread_mutex_unlock((l))
#else
#define s_pthread_create(t, a, f, p)        pthread_create((t), (a), (f), (p))
#define s_pthread_join(t, r)                pthread_join((t), (r))
#define s_pthread_detach(t)                 pthread_detach((t))
#define s_pthread_cancel(t)                 pthread_cancel((t))
#define s_pthread_self()                    pthread_self()
#define s_pthread_exit(r)                   pthread_exit((r))
#define s_pthread_mutex_lock(l)             pthread_mutex_lock((l))
#define s_pthread_mutex_unlock(l)           pthread_mutex_unlock((l))
#endif



/* Macros for enhanced localization functions */
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
#define s_setlocale(c, l)                   fs_setlocale((c), (l), (__FILE__), (__LINE__))
#define s_iconv_open(t, f)                  fs_iconv_open((t), (f), (__FILE__), (__LINE__))
#define s_iconv(c, f, fl, t, tl)            fs_iconv((c), (f), (fl), (t), (tl), (__FILE__), (__LINE__))
#define s_iconv_close(c)                    fs_iconv_close((c), (__FILE__), (__LINE__))
#elif defined(UTL_CWRAPPERS_ENABLE)
#define s_setlocale(c, l)                   fs_setlocale((c), (l))
#define s_iconv_open(t, f)                  fs_iconv_open((t), (f))
#define s_iconv(c, f, fl, t, tl)            fs_iconv((c), (f), (fl), (t), (tl))
#define s_iconv_close(c)                    fs_iconv_close((c))
#else
#define s_setlocale(c, l)                   setlocale((c), (l))
#define s_iconv_open(t, f)                  iconv_open((t), (f))
#define s_iconv(c, f, fl, t, tl)            iconv((c), (f), (fl), (t), (tl))
#define s_iconv_close(c)                    iconv_close((c))
#endif



/* Macros for enhanced exit functions - these do nothing at this point */
#define s_exit(s)                           exit((s))
#define s_abort()                           abort()


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_CWRAPPERS_H) */


/*---------------------------------------------------------------------------*/
