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

    Module:     file.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    17 February 1994

    Purpose:    This file contains a lot of useful C functions.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Externals
*/
extern int    errno;


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.file"


/* Enable this to check mmapping bounds */
/* #define FILE_ENABLE_MMAP_BOUND_CHECKING */

/* Enable this to enable mmapping usage advice */
/* #define FILE_ENABLE_MMAP_USAGE_ADVICE */


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

#if defined(UTL_FILE_MEMORY_MAPPING_AVAILABLE)
int iUtlFilePageSizeGlobal = -1;
#endif    /* defined(UTL_FILE_MEMORY_MAPPING_AVAILABLE) */


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iDirectorySorterAlphaAsc (unsigned char **ppucDirectoryEntry1, 
        unsigned char **ppucDirectoryEntry2);

static int iDirectorySorterAlphaDesc (unsigned char **ppucDirectoryEntry1, 
        unsigned char **ppucDirectoryEntry2);


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFileIsFile()

    Purpose:    To see if the passed path is really a file

    Parameters: pucPath     a path

    Globals:    none

    Returns:    true if it is a file, false otherwise

*/
boolean bUtlFileIsFile
(
    unsigned char *pucPath
)
{

    struct stat     psStatBuffer;


    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFileIsFile'."); 
        return (false);
    }


    if ( s_stat(pucPath, &psStatBuffer) == -1 ) {
        return (false);
    }

    if ( S_ISREG(psStatBuffer.st_mode) != false ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFileIsDirectory()

    Purpose:    To see if the passed path is really a directory

    Parameters: pucPath     a path

    Globals:    none

    Returns:    true if it is a directory, false otherwise

*/
boolean bUtlFileIsDirectory
(
    unsigned char *pucPath
)
{

    struct stat psStatBuffer;


    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFileIsDirectory'."); 
        return (false);
    }


    if ( s_stat(pucPath, &psStatBuffer) == -1 ) {
         return (false);
    }

    if ( S_ISDIR(psStatBuffer.st_mode) != false ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFileIsLink()

    Purpose:    To see if the passed path is really a link

    Parameters: pucPath     a path

    Globals:    none

    Returns:    true if it is a link, false otherwise

*/
boolean bUtlFileIsLink
(
    unsigned char *pucPath
)
{

    struct stat     psStatBuffer;


    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFileIsLink'."); 
        return (false);
    }


    if ( s_stat(pucPath, &psStatBuffer) == -1 ) {
         return (false);
    }

    if ( S_ISLNK(psStatBuffer.st_mode) != false ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFilePathExists()

    Purpose:    See if a given path exists

    Parameters: pucPath     a path

    Globals:    none

    Returns:    true if the path exists, false otherwise

*/
boolean bUtlFilePathExists
(
    unsigned char *pucPath
)
{

    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFilePathExists'."); 
        return (false);
    }


    if ( s_access(pucPath, F_OK) == 0 ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFilePathRead()

    Purpose:    See if a given path exists and can be read

    Parameters: pucPath     a path

    Globals:    none

    Returns:    true if the path exists and can be read, false otherwise

*/
boolean bUtlFilePathRead
(
    unsigned char *pucPath
)
{

    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFilePathRead'."); 
        return (false);
    }


    if ( s_access(pucPath, R_OK) == 0 ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFilePathWrite()

    Purpose:    See if a given path exists and can be written to

    Parameters: pucPath     a path

    Globals:    none

    Returns:    true if the path exists and can be written to, false otherwise

*/
boolean bUtlFilePathWrite
(
    unsigned char *pucPath
)
{

    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFilePathWrite'."); 
        return (false);
    }


    if ( s_access(pucPath, W_OK) == 0 ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFilePathExec()

    Purpose:    See if a given path exists and is executable

    Parameters: pucPath     a path

    Globals:    none

    Returns:    true if the path and is executable, false otherwise

*/
boolean bUtlFilePathExec
(
    unsigned char *pucPath
)
{

    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFilePathExec'."); 
        return (false);
    }


    if ( s_access(pucPath, X_OK) == 0 ) {
        return (true);
    }


    return(false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFileAccessTimeT()

    Purpose:    Get the last access time on a file

    Parameters: pfFile      FILE pointer
                ptTimeT     return pointer for the last access time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetFileAccessTimeT
(
    FILE *pfFile,
    time_t *ptTimeT
)
{

    struct stat psStatBuffer;


    /* Check parameters */
    if ( pfFile == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pfFile' parameter passed to 'iUtlFileGetFileAccessTimeT'."); 
        return (UTL_FileInvalidFile);
    }
    
    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetFileAccessTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_fstat(fileno(pfFile), &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_atime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFileModificationTimeT()

    Purpose:    Get the last modification time on a file

    Parameters: pfFile      FILE pointer
                ptTimeT     return pointer for the last modification time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetFileModificationTimeT
(
    FILE *pfFile,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( pfFile == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pfFile' parameter passed to 'iUtlFileGetFileModificationTimeT'."); 
        return (UTL_FileInvalidFile);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetFileModificationTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_fstat(fileno(pfFile), &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_mtime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFileStatusChangeTimeT()

    Purpose:    Get the last status change time on a file

    Parameters: pfFile      FILE pointer
                ptTimeT     return pointer for the last status change time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetFileStatusChangeTimeT
(
    FILE *pfFile,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( pfFile == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pfFile' parameter passed to 'iUtlFileGetFileStatusChangeTimeT'."); 
        return (UTL_FileInvalidFile);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetFileStatusChangeTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_fstat(fileno(pfFile), &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_ctime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetPathAccessTimeT()

    Purpose:    Get the last access time on a path

    Parameters: pucPath     the path
                ptTimeT     return pointer for the last access time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetPathAccessTimeT
(
    unsigned char *pucPath,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetPathAccessTimeT'."); 
        return (UTL_FileInvalidPath);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetPathAccessTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_stat(pucPath, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_atime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetPathModificationTimeT()

    Purpose:    Get the last modification time on a path

    Parameters: pucPath     the path
                ptTimeT     return pointer for the last modification time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetPathModificationTimeT
(
    unsigned char *pucPath,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetPathModificationTimeT'."); 
        return (UTL_FileInvalidPath);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetPathModificationTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_stat(pucPath, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_mtime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetPathStatusChangeTimeT()

    Purpose:    Get the last status change time on a path

    Parameters: pucPath     the path
                ptTimeT     return pointer for the last status change time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetPathStatusChangeTimeT
(
    unsigned char *pucPath,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetPathStatusChangeTimeT'."); 
        return (UTL_FileInvalidPath);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetPathStatusChangeTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_stat(pucPath, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_ctime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetLinkPathAccessTimeT()

    Purpose:    Get the last access time on a link path

    Parameters: pucPath     the path
                ptTimeT     return pointer for the last access time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetLinkPathAccessTimeT
(
    unsigned char *pucPath,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetLinkPathAccessTimeT'."); 
        return (UTL_FileInvalidPath);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetLinkPathAccessTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_lstat(pucPath, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_atime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetLinkPathModificationTimeT()

    Purpose:    Get the last modification time on a link path

    Parameters: pucPath     the path
                ptTimeT     return pointer for the last modification time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetLinkPathModificationTimeT
(
    unsigned char *pucPath,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetLinkPathModificationTimeT'."); 
        return (UTL_FileInvalidPath);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetLinkPathModificationTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_lstat(pucPath, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_mtime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetLinkPathStatusChangeTimeT()

    Purpose:    Get the last status change time on a link path

    Parameters: pucPath     the path
                ptTimeT     return pointer for the last status change time

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetLinkPathStatusChangeTimeT
(
    unsigned char *pucPath,
    time_t *ptTimeT
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetLinkPathStatusChangeTimeT'."); 
        return (UTL_FileInvalidPath);
    }

    if ( ptTimeT == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTimeT' parameter passed to 'iUtlFileGetLinkPathStatusChangeTimeT'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_lstat(pucPath, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return parameter */
    *ptTimeT = (time_t)psStatBuffer.st_ctime;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileSetPathUTime()

    Purpose:    Update the last access and mod date for this path.

    Parameters: pucPath     a file path

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileSetPathUTime
(
    unsigned char *pucPath
)
{

    /* Check paramters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileSetPathUTime'."); 
        return (UTL_FileInvalidPath);
    }


    /* Set the utime */
    if ( s_utime(pucPath, NULL) != 0 ) {
        return (UTL_FileUTimeFailed);
    }

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFileDescriptorLength()

    Purpose:    Get the length of a file, leaves the current position
                where it is.

    Parameters: iFile           File descriptor
                pzFileLength    return pointer for the file length

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetFileDescriptorLength
(
    int iFile,
    off_t *pzFileLength
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( iFile < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iFile' parameter passed to 'iUtlFileGetFileDescriptorLength'."); 
        return (UTL_FileInvalidFileDescriptor);
    }
    
    if ( pzFileLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzFileLength' parameter passed to 'iUtlFileGetFileDescriptorLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_fstat(iFile, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return pointer */
    *pzFileLength = psStatBuffer.st_size;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFileLength()

    Purpose:    Get the length of a file, leaves the current position
                where it is.

    Parameters: pfFile          FILE pointer
                pzFileLength    return pointer for the file length

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetFileLength
(
    FILE *pfFile,
    off_t *pzFileLength
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( pfFile == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pfFile' parameter passed to 'iUtlFileGetFileLength'."); 
        return (UTL_FileInvalidFile);
    }
    
    if ( pzFileLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzFileLength' parameter passed to 'iUtlFileGetFileLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Stat the file */
    if ( s_fstat(fileno(pfFile), &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return pointer */
    *pzFileLength = psStatBuffer.st_size;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFilePathLength()

    Purpose:    Get the length of a file

    Parameters: pucFilePath     file path
                pzFileLength    return pointer for the file length

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlFileGetFilePathLength
(
    unsigned char *pucFilePath,
    off_t *pzFileLength
)
{

    struct stat     psStatBuffer;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucFilePath' parameter passed to 'iUtlFileGetFilePathLength'."); 
        return (UTL_FileInvalidPath);
    }

    if ( pzFileLength == NULL ) {
        return (UTL_ReturnParameterError);
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzFileLength' parameter passed to 'iUtlFileGetFilePathLength'."); 
    }


    /* Stat the file */
    if ( s_stat(pucFilePath, &psStatBuffer) != 0 ) {
        return (UTL_FileStatFailed);
    }

    /* Set the return pointer */
    *pzFileLength = psStatBuffer.st_size;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFileLineLength()

    Purpose:    Returns the number of lines in a file, this only really
                works on a text file where the lines are less than UTL_FILE_PATH_MAX
                in length

    Parameters: pfFile          FILE pointer
                pzLineLength    return pointer for the line length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileGetFileLineLength
(
    FILE *pfFile,
    off_t *pzLineLength
)
{

    off_t           zLineLength = 0;
    unsigned char   pucLine[UTL_FILE_PATH_MAX + 1] = {'\0'};


    /* Check parameters */
    if ( pfFile == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pfFile' parameter passed to 'iUtlFileGetFileLineLength'."); 
        return (UTL_FileInvalidFile);
    }
    
    if ( pzLineLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzLineLength' parameter passed to 'iUtlFileGetFileLineLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Seek to the start of the file */
    if ( s_fseek(pfFile, 0, SEEK_SET) != 0 ) {
        return (UTL_FileSeekFailed);
    }

    /* Loop reading lines, we assume that they are less than UTL_FILE_PATH_MAX in length */
    while ( s_fgets(pucLine, UTL_FILE_PATH_MAX, pfFile) != NULL ) {
        zLineLength++;
    }

    /* Set the return pointer */
    *pzLineLength = zLineLength;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetFilePathLineLength()

    Purpose:    Returns the number of lines in a file, this only really
                works on a text file

    Parameters: pucFilePath     file path
                pzLineLength    return pointer for the line length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileGetFilePathLineLength
(
    unsigned char *pucFilePath,
    off_t *pzLineLength
)
{

    int     iError = UTL_NoError;
    FILE    *pfFile = NULL;
    


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucFilePath' parameter passed to 'iUtlFileGetFilePathLineLength'."); 
        return (UTL_FileInvalidPath);
    }
    
    if ( pzLineLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzLineLength' parameter passed to 'iUtlFileGetFilePathLineLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Open the file */
    if ( (pfFile = s_fopen(pucFilePath, "r")) == NULL ) {
        return (UTL_FileOpenFailed);
    }

    /* Get the file line length */
    iError = iUtlFileGetFileLineLength(pfFile, pzLineLength);
    
    /* Close the file */
    s_fclose(pfFile);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFileIsName

    Purpose:    Returns true if the path is a name

    Parameters: pucPath     the path

    Globals:    none

    Returns:    true of the path is a name

*/
boolean bUtlFileIsName
(
    unsigned char *pucPath
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFileIsName'."); 
        return (false);
    }


    /* Return true if the path is a name */
    return ((s_strchr(pucPath, UTL_FILE_DIRECTORY_SEPARATOR_CHAR) == NULL) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFileIsPath

    Purpose:    Returns true if the path is a path, rather than just a name

    Parameters: pucPath     the path

    Globals:    none

    Returns:    true if the path is a path

*/
boolean bUtlFileIsPath
(
    unsigned char *pucPath
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFileIsPath'."); 
        return (false);
    }


    /* Return true if the path is a path */
    return ((s_strchr(pucPath, UTL_FILE_DIRECTORY_SEPARATOR_CHAR) != NULL) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFileIsPathAbsolute

    Purpose:    Returns true if the path is an absolute one

    Parameters: pucPath     the path

    Globals:    none

    Returns:    true if the path is an absolute one

*/
boolean bUtlFileIsPathAbsolute
(
    unsigned char *pucPath
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFileIsPathAbsolute'."); 
        return (false);
    }


    /* Return true if the path is absolute, ie '/home/francois/mps' as opposed to 'francois/mps' */
    return ((pucPath[0] == UTL_FILE_DIRECTORY_SEPARATOR_CHAR) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlFileIsPathTilde

    Purpose:    Returns true if the path is a tilde one

    Parameters: pucPath     the path

    Globals:    none

    Returns:    true if the path is a tilde one

*/
boolean bUtlFileIsPathTilde
(
    unsigned char *pucPath
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'bUtlFileIsPathTilde'."); 
        return (false);
    }


    /* Return UTL_NoError if the path is tilde, ie '~francois/mps' or '~/mps' */
    return ((pucPath[0] == UTL_FILE_TILDE_CHAR) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileCleanPath

    Purpose:    This takes out the '/../' and the '/./' from the path by modifying 
                the argument and returning it.

    Parameters: pucPath     the path

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileCleanPath
(
    unsigned char *pucPath
)
{

    unsigned char   *pucPathPtr = NULL;
    unsigned int    uiPathLength = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileCleanPath'."); 
        return (UTL_FileInvalidPath);
    }


    /* Look for '/../' and shorten the path accordingly */
    while ( (pucPathPtr = (unsigned char *)s_strstr(pucPath, "/../")) != NULL ) {

        unsigned char   *pucPathStartPtr = NULL;
        boolean         bPathModified = false;

        for ( pucPathStartPtr = pucPathPtr - 1; pucPathStartPtr >= pucPath; pucPathStartPtr-- ) {
            
            if ( *pucPathStartPtr == UTL_FILE_DIRECTORY_SEPARATOR_CHAR ) {
                
                /* Then we found the beginning of the path */
                
                /* Set the point we copy from */
                pucPathPtr = pucPathPtr + s_strlen("/../") - 1;
                
                /* Copy while there are bytes to copy */
                for ( ; *pucPathPtr != '\0'; pucPathPtr++, pucPathStartPtr++ ) {
                    *pucPathStartPtr = *pucPathPtr;
                }
                
                /* NULL terminate the string */
                *pucPathStartPtr = '\0';

                /* Set the flag telling us that the path was modified and break out */
                bPathModified = true;
                break;
            }
        }

        /* There is a chance that the path will not be modified, so we 
        ** use a flag to break out here otherwise we will loop forever
        */
        if ( bPathModified == false ) {
            break;
        }
    }


    /* Look for '/./' and replace with UTL_FILE_DIRECTORY_SEPARATOR_CHAR */
    while ( (pucPathPtr = (unsigned char *)s_strstr(pucPath, "/./")) != NULL ) {
        s_strnncpy(pucPathPtr, pucPathPtr + s_strlen("/./") - 1, UTL_FILE_PATH_MAX + 1 - ((pucPathPtr - pucPath) + s_strlen("/./") - 1));
    }


    /* Eliminate trailing slash */
    if ( (uiPathLength = s_strlen(pucPath)) > 0 ) {
        if ( pucPath[uiPathLength - 1] == UTL_FILE_DIRECTORY_SEPARATOR_CHAR ) {
            pucPath[uiPathLength - 1] = '\0';
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetTruePath()

    Purpose:    This function gets the true path  of a path and places 
                it in pucTruePath.

                Note that the path IS cleaned/normalized.

    Parameters: pucPath             the original path
                pucTruePath         return pointer for the true path
                uiTruePathLength    length of the return pointer for the true path

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileGetTruePath
(
    unsigned char *pucPath,
    unsigned char *pucTruePath,
    unsigned int uiTruePathLength
)
{

    int     iError = UTL_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetTruePath'."); 
        return (UTL_FileInvalidPath);
    }
    
    if ( pucTruePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucTruePath' parameter passed to 'iUtlFileGetTruePath'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiTruePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTruePathLength' parameter passed to 'iUtlFileGetTruePath'."); 
        return (UTL_ReturnParameterError);
    }


    /* Clean the path first - this normalizes it too */
    if ( (iError = iUtlFileCleanPath(pucPath)) != UTL_NoError ) {
        return (iError);
    }


    /* Expand the tilde if the path is a tilde, this will create an absolute path */
    if ( bUtlFileIsPathTilde(pucPath) == true ) {
    
        unsigned char   pucUserName[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char   *pucHomeDirectory = NULL;
        unsigned char   *pucPathPtr = NULL;
        
        struct passwd   pwPasswd;
        unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};
        struct passwd   *ppwPasswdPtr = NULL;


        /* Extract the user name */
        if ( (pucPathPtr = s_strchr(pucPath, UTL_FILE_DIRECTORY_SEPARATOR_CHAR)) != NULL ) {
        
            /* Get the user name length */
            unsigned int uiUserNameLength = pucPathPtr - pucPath - 1;

            /* Limit the user name length so we don't blow our buffer */
            uiUserNameLength = UTL_MACROS_MIN(uiUserNameLength, UTL_FILE_PATH_MAX);
            
            /* Extract the user name */
            if ( uiUserNameLength > 0 ) {
                s_strnncpy(pucUserName, pucPath + 1, uiUserNameLength + 1);
            }
        }
        /* The path is the user name, minus the tilde of course */ 
        else {
            s_strnncpy(pucUserName, pucPath + 1, UTL_FILE_PATH_MAX + 1);
        }
            

        /* Get the password entry from the user name */
        if ( bUtlStringsIsStringNULL(pucUserName) == false ) {
            if ( s_getpwnam_r(pucUserName, &pwPasswd, pucBuffer, UTL_FILE_PATH_MAX + 1, &ppwPasswdPtr) == -1 ) {
                return (UTL_FileInvalidPath);
            }
        }
        /* Get the password entry from the user ID */
        else {
            if ( s_getpwuid_r(s_getuid(), &pwPasswd, pucBuffer, UTL_FILE_PATH_MAX + 1, &ppwPasswdPtr) == -1 ) {
                return (UTL_FileInvalidPath);
            }
        }
        
        /* Check that we got a home directory */
        if ( ppwPasswdPtr == NULL ) {
            return (UTL_FileInvalidPath);
        }

        /* Set the home directory pointer */
        pucHomeDirectory = ppwPasswdPtr->pw_dir;


        /* Merge the paths */
        if ( (iError = iUtlFileMergePaths(pucHomeDirectory, (pucPathPtr != NULL) ? pucPathPtr + 1 : (unsigned char *)"", pucTruePath, uiTruePathLength)) != UTL_NoError ) {
            return (iError);
        }

        /* And return */
        return (UTL_NoError);
    }


    /* Merge the path to the current directory if the path is not absolute */
    if ( bUtlFileIsPathAbsolute(pucPath) == false ) {

        unsigned char pucDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};

        /* Get the current directory and clean/normalize it */
        s_getcwd(pucDirectoryPath, UTL_FILE_PATH_MAX);
        if ( (iError = iUtlFileCleanPath(pucDirectoryPath)) != UTL_NoError ) {
            return (iError);
        }

        /* Merge the paths */
        if ( (iError = iUtlFileMergePaths(pucDirectoryPath, pucPath, pucTruePath, uiTruePathLength)) != UTL_NoError ) {
            return (iError);
        }

        /* And return */
        return (UTL_NoError);
    }


    /* Copy the path to the true path */
    s_strnncpy(pucTruePath, pucPath, uiTruePathLength);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetPathBase()

    Purpose:    Returns a pointer to the base part of the path.

                path                    base

                /mnt/mps/bin    ->      bin

                /mnt/mps/bin/   ->      bin/

                bin             ->      bin

                Note that the path IS NOT cleaned/normalized.

    Parameters: pucPath         path
                ppucPathBase    return pointer for the path base

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileGetPathBase
(
    unsigned char *pucPath,
    unsigned char **ppucPathBasePtr
)
{

    unsigned char   *pucPathPtr = pucPath;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetPathBase'."); 
        return (UTL_FileInvalidPath);
    }

    if ( ppucPathBasePtr == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucPathBasePtr' parameter passed to 'iUtlFileGetPathBase'."); 
        return (UTL_ReturnParameterError);
    }


    /* Find the path base */
    if ( (pucPathPtr = (unsigned char *)s_strrchr(pucPath, UTL_FILE_DIRECTORY_SEPARATOR_CHAR)) != NULL ) {
         pucPathPtr++;
    }


    /* Set the return pointer */
    *ppucPathBasePtr = (pucPathPtr != NULL) ? pucPathPtr : pucPath;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileGetPathDirectoryPath

    Purpose:    This function returns the directory path for a path.
                For example, 
                
                path                    directory path

                /mnt/mps/bin    ->      /mnt/mps

                /mnt/mps/bin/   ->      /mnt/mps

                bin             ->      ./

                /               ->      /

                Note that the path IS NOT cleaned/normalized.

    Parameters: pucPath                 path
                pucDirectoryPath        return pointer for the directory path
                uiDirectoryPathLength   length of the return pointer for the directory path

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileGetPathDirectoryPath
(
    unsigned char *pucPath,
    unsigned char *pucDirectoryPath,
    unsigned int uiDirectoryPathLength
)
{

    unsigned char    *pucPathPtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileGetPathDirectoryPath'."); 
        return (UTL_FileInvalidPath);
    }
    
    if ( pucDirectoryPath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDirectoryPath' parameter passed to 'iUtlFileGetPathDirectoryPath'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiDirectoryPathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDirectoryPathLength' parameter passed to 'iUtlFileGetPathDirectoryPath'."); 
        return (UTL_ReturnParameterError);
    }



    /* Search for the UTL_FILE_DIRECTORY_SEPARATOR_CHAR, if it is at the end of the string, 
    ** we back up to the previous UTL_FILE_DIRECTORY_SEPARATOR_CHAR and use that
    */
    if ( (pucPathPtr = (unsigned char *)s_strrchr(pucPath, UTL_FILE_DIRECTORY_SEPARATOR_CHAR)) != NULL ) {
        
        if ( *(pucPathPtr + 1) == '\0' ) {
            
            boolean     bFoundIt = false;
            
            while ( pucPathPtr > pucPath ) {
                
                pucPathPtr--;
            
                if ( *pucPathPtr == UTL_FILE_DIRECTORY_SEPARATOR_CHAR ) {
                    bFoundIt = true;
                    break;
                }
            }
            
            if ( bFoundIt == false ) {
                pucPathPtr = NULL;
            }
        }
    }


    /* Copy that part of the base if we found it, otherwise we return './' */
    if ( pucPathPtr != NULL ) {
        s_strnncpy(pucDirectoryPath, pucPath, uiDirectoryPathLength);
        pucDirectoryPath[pucPathPtr - pucPath] = '\0';
    }
    else {
        s_strnncpy(pucDirectoryPath, "./", uiDirectoryPathLength);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileMergePaths()

    Purpose:    This function merges a directory path and a path
                into a single path. If the path is complete,
                then no merge takes place, for example:

                directory path              path                    final path

                '/mnt'              +       'mps/bin'       ->      '/mnt/mps/bin'

                '/mnt'              +       '/mps/bin'      ->      '/mps/bin'

                NULL                +       'mps/bin'       ->      'mps/bin'

                NULL                +       '/mps/bin'      ->      '/mps/bin'

                Note that the path IS cleaned/normalized.

    Parameters: pucDirectoryPath    the directory path (optional)
                pucPath             the path
                pucFinalPath        pointer for the final path
                uiFinalPathLength   length of the pointer for the final path

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileMergePaths
(
    unsigned char *pucDirectoryPath,
    unsigned char *pucPath,
    unsigned char *pucFinalPath,
    unsigned int uiFinalPathLength
)
{

    int     iError = UTL_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPath' parameter passed to 'iUtlFileMergePaths'."); 
        return (UTL_FileInvalidPath);
    }
    
    if ( pucFinalPath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFinalPath' parameter passed to 'iUtlFileMergePaths'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiFinalPathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFinalPathLength' parameter passed to 'iUtlFileMergePaths'."); 
        return (UTL_ReturnParameterError);
    }


    /* Clean/normalize the paths */
    if ( (iError = iUtlFileCleanPath(pucPath)) != UTL_NoError ) {
        return (iError);
    }
    
    if ( bUtlStringsIsStringNULL(pucDirectoryPath) == false ) {
        if ( (iError = iUtlFileCleanPath(pucDirectoryPath)) != UTL_NoError ) {
            return (iError);
        }
    }


    /* Merge the paths is the path is not absolute */
    if ( bUtlFileIsPathAbsolute(pucPath) == false ) {
    
        /* So we merge the path name to the directory path if it was specified, otherwise just copy the path */
        if ( bUtlStringsIsStringNULL(pucDirectoryPath) == false ) {

            /* Copy the directory path to the final path */
            s_strnncpy(pucFinalPath, pucDirectoryPath, uiFinalPathLength);
    
            /* Append a UTL_FILE_DIRECTORY_SEPARATOR_CHAR to the final path if needed */
            if ( pucDirectoryPath[s_strlen(pucDirectoryPath) - 1] != UTL_FILE_DIRECTORY_SEPARATOR_CHAR ) {
                s_strnncat(pucFinalPath, UTL_FILE_DIRECTORY_SEPARATOR_STRING, uiFinalPathLength - 1, uiFinalPathLength);
            }
    
            /* Append the path to the final path */
            s_strnncat(pucFinalPath, pucPath, uiFinalPathLength - 1, uiFinalPathLength);
        }
        else {
            /* Copy the path to the final path */
            s_strnncpy(pucFinalPath, pucPath, uiFinalPathLength);
        }
    }
    /* The path is already absolute */
    else {
        /* Copy the path to the final path */
        s_strnncpy(pucFinalPath, pucPath, uiFinalPathLength);
    }


    /* Clean the final path */
    if ( (iError = iUtlFileCleanPath(pucFinalPath)) != UTL_NoError ) {
        return (iError);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileCreateDirectoryPath()

    Purpose:    This function creates a directory path.

    Parameters: pucDirectoryPath            the directory path
                pucDirectoryPathStartPtr    the directory path start pointer (optional)
                zMode                       mode

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileCreateDirectoryPath
(
    unsigned char *pucDirectoryPath,
    unsigned char *pucDirectoryPathStartPtr,
    mode_t zMode
)
{

    /* Set the directory path pointer */
    unsigned char   *pucDirectoryPathPtr = (pucDirectoryPathStartPtr != NULL) ? pucDirectoryPathStartPtr : pucDirectoryPath + 1;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucDirectoryPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDirectoryPath' parameter passed to 'iUtlFileCreateDirectoryPath'."); 
        return (UTL_FileInvalidPath);
    }
    
    
    /* Loop forever, we control the look from within */
    while ( true ) {
    
        /* Get a pointer to the next directory separator, could return a null */
        pucDirectoryPathPtr = s_strchr(pucDirectoryPathPtr, UTL_FILE_DIRECTORY_SEPARATOR_CHAR);
    
        /* Null terminate the partial directory */
        if ( pucDirectoryPathPtr != NULL ) {
            *pucDirectoryPathPtr = '\0';
        }

        /* Create the directory if it does not already exist */
        if ( bUtlFileIsDirectory(pucDirectoryPath) == false ) {
            if ( s_mkdir(pucDirectoryPath, zMode) != 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the directory: '%s'.", pucDirectoryPath);
                return (UTL_FileDirectoryCreateFailed);
            }
        }
        
        /* Break out if we reached the end of the directory path */
        if ( pucDirectoryPathPtr == NULL ) {
            break;
        }

        /* Otherwise restore the directory separator and increment */
        *pucDirectoryPathPtr = UTL_FILE_DIRECTORY_SEPARATOR_CHAR;
        pucDirectoryPathPtr++;
        
    }
    

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileScanDirectory()

    Purpose:    Scan a directory for files based on a selector function. This
                skips system directories.
                
                The default is to select all entries and to sort alphabetically. 
                
                If there are not matches, a UTL_NoError will be returned and 
                the return pointer will be Null.

    Parameters: pucDirectoryPath            directory path
                iSelectorFunction           pointer to a selector function (optional)
                iSorterFunction             pointer to a sorter function (optional)
                pppucDirectoryEntryList     return pointer for the directory entry list

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileScanDirectory
(
    unsigned char *pucDirectoryPath,
    int (*iSelectorFunction)(),
    int (*iSorterFunction)(),
    unsigned char ***pppucDirectoryEntryList
)
{

    int             iError = UTL_NoError;
    struct dirent   *pDirectoryEntry = NULL;
    DIR             *pDirectory = NULL;
    unsigned char   **ppucDirectoryEntryListPtr = NULL;
    unsigned char   **ppucDirectoryEntryList = NULL;
    int             iEntryLength = 0;
    int             iEntryCapacity = 0;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucDirectoryPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDirectoryPath' parameter passed to 'iUtlFileScanDirectory'."); 
        return (UTL_FileInvalidDirectoryPath);
    }

    if ( pppucDirectoryEntryList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pppucDirectoryEntryList' parameter passed to 'iUtlFileScanDirectory'."); 
        return (UTL_ReturnParameterError);
    }


    /* Open the directory */
    if ( (pDirectory = s_opendir(pucDirectoryPath)) == NULL ) {
        return (UTL_FileOpenDirectoryFailed);
    }


    /* Read entries in the directory */
    for ( iEntryLength = 0; (pDirectoryEntry = s_readdir(pDirectory)); ) {

        /* Skip system directories */
        if ( (s_strcmp(pDirectoryEntry->d_name, ".") == 0) || (s_strcmp(pDirectoryEntry->d_name, "..") == 0) || (s_strcmp(pDirectoryEntry->d_name, "lost+found") == 0) ) {
            continue;
        }

        /* Skip this if there is a selector and we fail the selection */
        if ( (iSelectorFunction != NULL) && ((*iSelectorFunction)(pDirectoryEntry->d_name) == 0) ) {
            continue;
        }


        /* Extend the list of needed */
        if ( (++iEntryLength) >= iEntryCapacity ) {

            iEntryCapacity += 20;

            /* Extend the pointer */
            if ( (ppucDirectoryEntryListPtr = (unsigned char **)s_realloc((unsigned char **)ppucDirectoryEntryList, (size_t)(iEntryCapacity * sizeof(unsigned char *)))) == NULL ) {
                iError = UTL_MemError;
                goto bailFromiUtlFileScanDirectory;
            }

            /* Hand over the pointer */
            ppucDirectoryEntryList = ppucDirectoryEntryListPtr;
        }


        /* Duplicate the name */
        if ( (ppucDirectoryEntryList[iEntryLength - 1] = (unsigned char *)s_strdup(pDirectoryEntry->d_name)) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiUtlFileScanDirectory;
        }
    }


    /* Null terminate the list and sort it */
    if ( iEntryLength > 0 ) {
        
        /* NULL terminate the list */
        ppucDirectoryEntryList[iEntryLength] = NULL;

        /* Sort the list */
        s_qsort(ppucDirectoryEntryList, iEntryLength, sizeof(unsigned char *), 
                (iSorterFunction != NULL) ? (int (*)())iSorterFunction : (int (*)())iDirectorySorterAlphaAsc);
    }
    

    
    /* Bail label */
    bailFromiUtlFileScanDirectory:
    

    /* Close the directory */
    if  ( pDirectory != NULL ) {
        s_closedir(pDirectory);
    }

    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *pppucDirectoryEntryList = ppucDirectoryEntryList;
    }
    else {
    
        /* Free resources */
        iUtlFileFreeDirectoryEntryList(ppucDirectoryEntryList);
        ppucDirectoryEntryList = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileFreeDirectoryEntryList()

    Purpose:    Free the directory entry list allocated by iUtlFileScanDirectory

    Parameters: ppucDirectoryEntryList      pointer to a NULL terminated directory name list

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileFreeDirectoryEntryList
(
    unsigned char **ppucDirectoryEntryList
)
{

    unsigned int    uiI = 0;


    /* Check parameters */
    if ( ppucDirectoryEntryList == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'ppucDirectoryEntryList' parameter passed to 'iUtlFileFreeDirectoryEntryList'."); 
        return (UTL_FileInvalidDirectoryEntryList);
    }


    /* Free the items in the directory entry list */
    for ( uiI = 0; ppucDirectoryEntryList[uiI] != NULL; uiI++ ) {
        s_free(ppucDirectoryEntryList[uiI]);
    }

    /* Free the directory entry list */
    s_free(ppucDirectoryEntryList);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileMemoryMap()

    Purpose:    Create a memory mapping of a file

    Parameters: iFile           file number
                zOffset         offset into the file to map from
                zSize           length of the data to map from the file
                iProtection     protection
                ppvPtr          return pointer for the mapping

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileMemoryMap
(
    int iFile,
    off_t zOffset,
    size_t zSize,
    int iProtection,
    void **ppvPtr
)
{

    unsigned char   *pucPtr = NULL;
    int             iIndent = 0;
    void            *pvAddress = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlFileMemoryMap - zSize: %ld, zOffset: %ld, fileno: %d.", zSize, zOffset, iFile); */

    /* Check the parameters */
    if ( iFile < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iFile' parameter passed to 'iUtlFileMemoryMap'."); 
        return (UTL_FileInvalidFileDescriptor);
    }

    if ( zOffset < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'zOffset' parameter passed to 'iUtlFileMemoryMap'."); 
        return (UTL_FileInvalidOffset);
    }

    if ( zSize <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'zSize' parameter passed to 'iUtlFileMemoryMap'."); 
        return (UTL_FileInvalidSize);
    }

    if ( ppvPtr == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ppvPtr' parameter passed to 'iUtlFileMemoryMap'."); 
        return (UTL_ReturnParameterError);
    }


#if defined(FILE_ENABLE_MMAP_BOUND_CHECKING)
    /* Check that the mapping is valid, we do this because some operating systems will allow us to map
    ** space beyond the  end of the file and seg fault when we try to read the  (non-exitant) content
    */
    {
        struct stat     psStatBuffer;
        off_t           zFileLen = 0;

        /* Fstat the file */
        if ( s_fstat(iFile, &psStatBuffer) != 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "iUtlFileMemoryMap failed to fstat the file descriptor: %d.", iFile);
            return (UTL_FileStatFailed);
        }
    
        /* Get the file length */
        zFileLen = psStatBuffer.st_size;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, iUtlFileMemoryMap - "zFileLen: %ld, fileno: %d.", zFileLen, iFile); */
    
        /* Check that we are not mapping in beyond the end of the file */
        if ( (zOffset + zSize) > zFileLen ) {
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlFileMemoryMap tried to map in: %ld bytes, at offset: %ld, but the file length was only: %ld bytes, fileno: %d.", zSize, zOffset, zFileLen, iFile); */
            return (UTL_FileInvalidMapping);
        }
    }
#endif    /* defined(FILE_ENABLE_MMAP_BOUND_CHECKING) */


    /* Store the page size if this is the first call */
    if ( iUtlFilePageSizeGlobal == -1 ) {
        iUtlFilePageSizeGlobal = getpagesize();
    }


    /* Calculate the indent */
    iIndent = zOffset % iUtlFilePageSizeGlobal;


    /* Do the mapping */
    pvAddress = s_mmap(0, zSize + iIndent, iProtection, MAP_SHARED, iFile, zOffset - iIndent);
    
    if ( (pvAddress == MAP_FAILED) || (pvAddress == NULL) ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "iUtlFileMemoryMap failed to map in: %ld bytes, at offset: %ld, fileno: %d.", zSize, zOffset, iFile); */
        return (UTL_FileInvalidMapping);
    }


#if defined(FILE_ENABLE_MMAP_USAGE_ADVICE)
    /* Advise on use */
    madvise(pvAddress, zSize + iIndent, MADV_NORMAL);
#endif    /* defined(FILE_ENABLE_MMAP_USAGE_ADVICE) */


    /* Cast the pointer */
    pucPtr = (unsigned char *)pvAddress;

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlFileMemoryMap mapped in: %ld bytes, at offset: %ld, at address: %ld.", zSize + iIndent, zOffset, (long)pucPtr); */

    /* Increment the pointer so that we point at the real data */
    pucPtr += iIndent;

    /* Set the return pointer */
    *ppvPtr = (void *)pucPtr;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlFileMemoryUnMap()

    Purpose:    Release a memory mapping of a file, this function will use munmap()
                if it is available, otherwise we simulate it with free()

    Parameters: pvPtr   pointer to map
                zSize   size of the data to unmap

    Globals:    none

    Returns:    UTL error code

*/
int iUtlFileMemoryUnMap
(
    void *pvPtr,
    size_t zSize
)
{

    unsigned char   *pucPtr = (unsigned char *)pvPtr;
    int             iIndent = 0;
    off_t           zOffset = 0;


    /* Check the parameters */
    if ( pvPtr == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvPtr' parameter passed to 'iUtlFileMemoryUnMap'."); 
        return (UTL_FileInvalidMapping);
    }

    if ( zSize <= 0 ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Invalid 'zSize' parameter passed to 'iUtlFileMemoryUnMap'."); 
        return (UTL_FileInvalidSize);
    }


    /* Calculate the indent and reset the pointer to its real origin */
    zOffset = (size_t)pucPtr;
    iIndent = zOffset % iUtlFilePageSizeGlobal;
    pucPtr -= iIndent;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlFileMemoryUnMap releasing: %lu bytes at: %lu.",  (unsigned long)(zSize + iIndent),  (unsigned long)pucPtr); */


    /* Release the mapping */
    s_munmap(pucPtr, (zSize + iIndent));


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iDirectorySorterAlphaAsc()

    Purpose:    Sorts the directory entries in ascending alphabetical order 

    Parameters: ppucDirectoryEntry1     directory entry
                ppucDirectoryEntry1     directory entry

    Globals:    none

    Returns:    the strcmp() value of the name comparison

*/
static int iDirectorySorterAlphaAsc
(
    unsigned char **ppucDirectoryEntry1,
    unsigned char **ppucDirectoryEntry2
)
{

    ASSERT(ppucDirectoryEntry1 != NULL);
    ASSERT(*ppucDirectoryEntry1 != NULL);
    ASSERT(ppucDirectoryEntry2 != NULL);
    ASSERT(*ppucDirectoryEntry2 != NULL);


    return (s_strcmp(*ppucDirectoryEntry1, *ppucDirectoryEntry2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDirectorySorterAlphaDesc()

    Purpose:    Sorts the directory entries in descending alphabetical order 

    Parameters: ppucDirectoryEntry1     directory entry
                ppucDirectoryEntry1     directory entry

    Globals:    none

    Returns:    the reverse strcmp() value of the name comparison

*/
static int iDirectorySorterAlphaDesc
(
    unsigned char **ppucDirectoryEntry1,
    unsigned char **ppucDirectoryEntry2
)
{
    
    int     iStatus = 0;

    
    ASSERT(ppucDirectoryEntry1 != NULL);
    ASSERT(*ppucDirectoryEntry1 != NULL);
    ASSERT(*ppucDirectoryEntry2 != NULL);
    ASSERT(ppucDirectoryEntry2 != NULL);
    
    
    iStatus = s_strcmp(*ppucDirectoryEntry1, *ppucDirectoryEntry2);
    
    if ( iStatus < 0 ) {
        iStatus = 1;
    }
    else if ( iStatus > 0 ) {
        iStatus = -1;
    }

    
    return (iStatus);

}


/*---------------------------------------------------------------------------*/
