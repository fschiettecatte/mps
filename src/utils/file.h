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

    Module:     file.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    17 February 1994

    Purpose:    This is the header file for file.c. 

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_FILE_H)
#define UTL_FILE_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Memory mapping */

/* Solaris  */
#if defined(__sun__)
#define UTL_FILE_MEMORY_MAPPING_AVAILABLE
#endif    /* defined(__sun__) */


/* Linux */
#if defined(linux)
#define UTL_FILE_MEMORY_MAPPING_AVAILABLE
#endif    /* defined(linux) */


/* AIX */
#if defined(AIX)
#define UTL_FILE_MEMORY_MAPPING_AVAILABLE
#endif    /* defined(AIX) */


/* MacOS X */
#if defined(__APPLE__) && defined(__MACH__)
#define UTL_FILE_MEMORY_MAPPING_AVAILABLE
#endif    /* defined(__APPLE__) && defined(__MACH__) */


/* Pull in the memory mapping include file */
#if defined(UTL_FILE_MEMORY_MAPPING_AVAILABLE)
#include <sys/mman.h>
#endif    /* def UTL_FILE_MEMORY_MAPPING_AVAILABLE */


/*---------------------------------------------------------------------------*/


/* Set file length maximum */
#define UTL_FILE_LEN_MAX                            (LONG_MAX)
/* #define UTL_FILE_LEN_MAX                            ((1UL << (_FILE_OFFSET_BITS - 1)) - 1) */


/*---------------------------------------------------------------------------*/


/* Create the file path maximum, cap it at 1024 */
#if (PATH_MAX < 1024)
#define UTL_FILE_PATH_MAX                           (PATH_MAX)
#else
#define UTL_FILE_PATH_MAX                           (1024)
#endif    /* (PATH_MAX <= 1024) */


/* Create the file name maximum, cap it at 1024 */
#if (NAME_MAX < 1024)
#define UTL_FILE_NAME_MAX                           (NAME_MAX)
#else
#define UTL_FILE_NAME_MAX                           (1024)
#endif    /* (NAME_MAX <= 1024) */


/*---------------------------------------------------------------------------*/


/* Directory root - unix/linux for now */
#define UTL_FILE_DIRECTORY_ROOT_CHAR                '/'
#define UTL_FILE_DIRECTORY_ROOT_STRING              (unsigned char *)"/"
#define UTL_FILE_DIRECTORY_ROOT_WSTRING             L"/"

/* Directory separators - unix/linux for now */
#define UTL_FILE_DIRECTORY_SEPARATOR_CHAR           '/'
#define UTL_FILE_DIRECTORY_SEPARATOR_STRING         (unsigned char *)"/"
#define UTL_FILE_DIRECTORY_SEPARATOR_WSTRING        L"/"

/* Tilde - unix/linux for now */
#define UTL_FILE_TILDE_CHAR                         '~'
#define UTL_FILE_TILDE_STRING                       (unsigned char *)"~"
#define UTL_FILE_TILDE_WSTRING                      L"~"


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

/* File access and information */
boolean bUtlFileIsFile (unsigned char *pucPath);
boolean bUtlFileIsDirectory (unsigned char *pucPath);
boolean bUtlFileIsLink (unsigned char *pucPath);

boolean bUtlFilePathExists (unsigned char *pucPath);
boolean bUtlFilePathRead (unsigned char *pucPath);
boolean bUtlFilePathWrite (unsigned char *pucPath);
boolean bUtlFilePathExec (unsigned char *pucPath);

int iUtlFileGetFileAccessTimeT (FILE *pfFile, time_t *ptTimeT);
int iUtlFileGetFileModificationTimeT (FILE *pfFile, time_t *ptTimeT);
int iUtlFileGetFileStatusChangeTimeT (FILE *pfFile, time_t *ptTimeT);

int iUtlFileGetPathAccessTimeT (unsigned char *pucPath, time_t *ptTimeT);
int iUtlFileGetPathModificationTimeT (unsigned char *pucPath, time_t *ptTimeT);
int iUtlFileGetPathStatusChangeTimeT (unsigned char *pucPath, time_t *ptTimeT);

int iUtlFileGetLinkPathAccessTimeT (unsigned char *pucPath, time_t *ptTimeT);
int iUtlFileGetLinkPathModificationTimeT (unsigned char *pucPath, time_t *ptTimeT);
int iUtlFileGetLinkPathStatusChangeTimeT (unsigned char *pucPath, time_t *ptTimeT);

int iUtlFileSetPathUTime (unsigned char *pucPath);

int iUtlFileGetFileDescriptorLength (int iFile, off_t *pzFileLength);
int iUtlFileGetFileLength (FILE *pfFile, off_t *pzFileLength);
int iUtlFileGetFilePathLength (unsigned char *pucFilePath, off_t *pzFileLength);
int iUtlFileGetFileLineLength (FILE *pfFile, off_t *pzLineLength);
int iUtlFileGetFilePathLineLength (unsigned char *pucFilePath, off_t *pzLineLength);


/* Path properties */
boolean bUtlFileIsName (unsigned char *pucPath);
boolean bUtlFileIsPath (unsigned char *pucPath);
boolean bUtlFileIsPathAbsolute (unsigned char *pucPath);
boolean bUtlFileIsPathTilde (unsigned char *pucPath);


/* Path processors */
int iUtlFileCleanPath (unsigned char *pucPath);

int iUtlFileGetTruePath (unsigned char *pucPath, 
        unsigned char *pucTruePath, unsigned int uiTruePathLength);

int iUtlFileGetPathBase (unsigned char *pucPath, unsigned char **ppucPathBasePtr);

int iUtlFileGetPathDirectoryPath (unsigned char *pucPath, 
        unsigned char *pucDirectoryPath, unsigned int uiDirectoryPathLength);

int iUtlFileMergePaths (unsigned char *pucDirectoryPath, unsigned char *pucPath,
        unsigned char *pucFinalPath, unsigned int uiFinalPathLength);


/* Directory creation */
int iUtlFileCreateDirectoryPath (unsigned char *pucDirectoryPath, 
        unsigned char *pucDirectoryPathStartPtr, mode_t zMode);


/* Directory scanner */
int iUtlFileScanDirectory (unsigned char *pucDirectoryPath, int (*iSelectorFunction)(), 
        int (*iSorterFunction)(), unsigned char ***pppucDirectoryEntryList);

int iUtlFileFreeDirectoryEntryList (unsigned char **ppucDirectoryEntryList);


/* Memory mapping */
int iUtlFileMemoryMap (int iFile, off_t zOffset, size_t zSize, 
        int iProtection, void **ppvPtr);

int iUtlFileMemoryUnMap (void *pvPtr, size_t zSize);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_FILE_H) */


/*---------------------------------------------------------------------------*/
