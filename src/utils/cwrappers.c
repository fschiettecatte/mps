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

    Module:     cwrappers.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 January 1994

    Purpose:    This file contains wrappers and debug version of most
                of the posix and standard C functions.

                Function names which begin with 'fs_' are always called
                as they report errors (via errno). The philosophy of
                these functions is to provide a log when things go
                wrong and a degree of safety when they are improperly called.

                Function names which begin with 's_' are only called when
                UTL_CWRAPPERS_ENABLE_DEBUG is enabled since they dont perform any runtime
                checking.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/


/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.cwrappers"



/* Enable this to report the freeing of a NULL pointer */
/* #define UTL_CWRAPPERS_ENABLE_NULL_POINTER_FREE_LOGGING */

/* Enable this to report the closing of a NULL file pointer */
/* #define UTL_CWRAPPERS_ENABLE_NULL_FILE_POINTER_CLOSE_LOGGING */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Error string length for strerror_r() */
#define UTL_CWRAPPERS_ERROR_STRING_LEN          (128)


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* This whole file is commented out if neither of the two defines below were activated */
#if defined(UTL_CWRAPPERS_ENABLE) || defined(UTL_CWRAPPERS_ENABLE_DEBUG)


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   fs_malloc()

    Purpose:    Allocate a pointer, does safety checks,
                clears the memory area allocated.

    Parameters: zSize       The size of the pointer
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the newly allocated memory area, 
                or NULL on error.

*/
void *fs_malloc
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    size_t zSize,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    size_t zSize
#endif
)
{

    void    *pvPtr = NULL;


    if ( zSize < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_malloc() tried to allocate: %lu bytes, file: '%s', line: %lu.", zSize, pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Allocate the pointer using calloc to clear it */
    pvPtr = (void*)calloc((size_t)1, (size_t)zSize);


    /* Allocation failed */
    if ( pvPtr == NULL ) {

         int            iError = errno;
         unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_malloc() failed to allocate: %lu bytes, errno: %d, strerror: '%s', file: '%s', line: %lu",
                zSize, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_malloc() failed to allocate: %lu bytes, errno: %d, strerror: '%s'",
                zSize, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pvPtr);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_calloc()

    Purpose:    Allocate a pointer, does safety checks,
                clears the memory area allocated.

    Parameters: zNum        Number of records to allocate
                zSize       The size of each record
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the newly allocated memory area, 
                or NULL on error.

*/
void *fs_calloc
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    size_t zNum,
    size_t zSize,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    size_t zNum,
    size_t zSize
#endif
)
{

    void    *pvPtr = NULL;


    if ( zNum < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_calloc() tried to allocate: %lu elements, file: '%s', line: %lu.", zNum, pcFile, zLine);
#endif
        return (NULL);
    }

    if ( zSize <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_calloc() tried to allocate elements of: %lu bytes, file: '%s', line: %lu.", zNum, pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Allocate the pointer */
    pvPtr = (void*)calloc((size_t)zNum, (size_t)zSize);


    /* Allocation failed */
    if ( pvPtr == NULL ) {

         int            iError = errno;
         unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_calloc() failed to allocate: %lu elements of: %lu bytes in length, errno: %d, strerror: '%s', file: '%s', line: %lu",
                zNum, zSize, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_calloc() failed to allocate: %lu elements of: %lu bytes in length, errno: %d, strerror: '%s'",
                zNum, zSize, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pvPtr);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_realloc()

    Purpose:    Rellocate a pointer, does safety checks. Note that we dont know how
                big our original pointer was so we dont null out the new memory area.

    Parameters: pvPtr       The current pointer
                zSize       The new size of the pointer
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the newly reallocated memory area, or the 
                original pointer on error.

*/
void *fs_realloc
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr,
    size_t zSize,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr,
    size_t zSize
#endif
)
{

    void    *pvNewPtr = NULL;


    if ( zSize <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_realloc() tried to reallocate to: %lu bytes, file: '%s', line: %lu.", zSize, pcFile, zLine);
#endif
        return (NULL);
    }


    /* This is really a malloc */
    if ( pvPtr != NULL ) {        

        /* Reset the error */
        errno = EOK;

        /* Reallocate the pointer */
        pvNewPtr = (void*)realloc(pvPtr, zSize);

        /* Reallocation failed */
        if ( pvNewPtr == NULL ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

            /* Reset the pointer */
            pvNewPtr = pvPtr;

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogPanic(UTL_LOG_CONTEXT, "fs_realloc() failed to reallocate: %lu bytes, errno: %d, strerror: '%s', file: '%s', line: %lu",
                    zSize, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogPanic(UTL_LOG_CONTEXT, "fs_realloc() failed to reallocate: %lu bytes, errno: %d, strerror: '%s'",
                    zSize, iError, pucBuffer);
#endif

             errno = iError;
        }
    }
    else {

        /* Reset the error */
        errno = EOK;

        /* Allocate the pointer using calloc to clear it */
        pvNewPtr = (void*)calloc((size_t)1, (size_t)zSize);

        /* Allocation failed */
        if ( pvNewPtr == NULL ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogPanic(UTL_LOG_CONTEXT, "fs_realloc failed to allocate: %lu bytes, errno: %d, strerror: '%s', file: '%s', line: %lu",
                    zSize, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogPanic(UTL_LOG_CONTEXT, "fs_realloc failed to allocate: %lu bytes, errno: %d, strerror: '%s'",
                    zSize, iError, pucBuffer);
#endif

             errno = iError;
         }
    }


    return (pvNewPtr);
}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_free()

    Purpose:    Free a pointer, does safety checks. Note that this function is
                sometimes passed as a pointer to a doList function, in this case
                we get garbage passed in pcFile and zLine.

    Parameters: pvPtr       The pointer to be freed
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    void

*/
void fs_free
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr
#endif
)
{

    if ( pvPtr != NULL ) {
        free(pvPtr);
    }
#if defined(UTL_CWRAPPERS_ENABLE_NULL_POINTER_FREE_LOGGING)
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    else {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_free() tried to free a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
    }
#endif
#endif    /* defined(UTL_CWRAPPERS_ENABLE_NULL_POINTER_FREE_LOGGING) */


    return; 

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_memcmp()

    Purpose:    Compare zCount bytes of pvPtr2 to pvPtr1.

    Parameters: pvPtr1      pointer.
                pvPtr2      pointer.
                zCount      length to compare.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_memcmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr1,
    void *pvPtr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr1,
    void *pvPtr2,
    size_t zCount
#endif
)
{

    /* Check the length to compare */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_memcmp() tried to compare an extent of length: %ld bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }
    else if ( zCount > 0 ) {

        /* Check the pvPtr1 pointer */    
        if ( pvPtr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memcmp() found a NULL destination pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }

        /* Check the pvPtr2 pointer */    
        if ( pvPtr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memcmp() found a NULL source pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
        
        /* Handle special cases gracefully */
        if ( (pvPtr1 == NULL) && (pvPtr2 == NULL) ) {
            return (0);
        }
        else if ( (pvPtr1 != NULL) && (pvPtr2 == NULL) ) {
            return (1);
        }
        else if ( (pvPtr1 == NULL) && (pvPtr2 != NULL) ) {
            return (-1);
        }
    }


    /* Do the comparison */
    return (memcmp(pvPtr1, pvPtr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_memcpy()

    Purpose:    Copy zCount bytes from pvPtr2 to pvPtr1.

    Parameters: pvPtr1      pointer.
                pvPtr2      pointer.
                zCount      length to copy.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to pvPtr1, NULL on error

*/
void *fs_memcpy
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr1,
    void *pvPtr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr1,
    void *pvPtr2,
    size_t zCount
#endif
)
{

    /* Check the length to copy */    
    if ( zCount <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memcpy() tried to copy: %lu bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }
    else if ( zCount > 0 ) {

        /* Check the pvPtr1 pointer */    
        if ( pvPtr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memcpy() found a NULL destination pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }

        /* Check the pvPtr2 pointer */    
        if ( pvPtr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memcpy() found a NULL source pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }

        /* Check for overlapping blocks */
        if ( ((unsigned char *)pvPtr1 >= (unsigned char *)pvPtr2) &&
            ((unsigned char *)pvPtr1 < (unsigned char *)((unsigned char *)pvPtr2 + zCount)) ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memcpy() tried to copy overlaping blocks, destination: %lu, source: %lu, bytes: %lu, file: '%s', line: %lu.", 
                    (unsigned long)pvPtr1, (unsigned long)pvPtr2, zCount, pcFile, zLine);
#endif
            return (NULL);
        }
        
        if ( ((unsigned char *)pvPtr2 >= (unsigned char *)pvPtr1) &&
            ((unsigned char *)pvPtr2 < (unsigned char *)((unsigned char *)pvPtr1 + zCount)) ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memcpy() tried to copy overlaping blocks, destination: %lu, source: %lu, bytes: %lu, file: '%s', line: %lu.", 
                    (unsigned long)pvPtr1, (unsigned long)pvPtr2, zCount, pcFile, zLine);
#endif
            return (NULL);
        }
    }


    return ((void *)memcpy(pvPtr1, pvPtr2, zCount));
}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_memmove()

    Purpose:    Move zCount bytes from pvPtr2 to pvPtr1.

    Parameters: pvPtr1      pointer.
                pvPtr2      pointer.
                zCount      length to move.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to pvPtr1, NULL on error

*/
void *fs_memmove
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr1,
    void *pvPtr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr1,
    void *pvPtr2,
    size_t zCount
#endif
)
{

    /* Check the length to move */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memmove() tried to move: %ld bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }
    else if ( zCount > 0 ) {

        /* Check the p1 pointer */    
        if ( pvPtr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memmove() found a NULL destination pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }

        /* Check the p2 pointer */    
        if ( pvPtr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memmove() found a NULL source pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }
    }


    return ((void *)memmove(pvPtr1, pvPtr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_memset()

    Purpose:    Set zCount bytes to iChar from pvPtr.

    Parameters: pvPtr       pointer.
                iChar       character.
                zCount      length.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to pvPtr

*/
void *fs_memset
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr,
    int iChar,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr,
    int iChar,
    size_t zCount
#endif
)
{

    /* Check the length to set */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memset() tried to set: %lu bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }
    else if ( zCount > 0 ) {

        /* Check the p pointer */    
        if ( pvPtr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_memset() found a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }
    }


    return ((void *)memset(pvPtr, iChar, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wmemcmp()

    Purpose:    Compare zCount wide characters of pwcStr2 to pwcStr1.

    Parameters: pwcStr1     pointer.
                pwcStr2     pointer.
                zCount      length to compare.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_wmemcmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to compare */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wmemcmp() tried to compare an extent of length: %ld bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }
    else if ( zCount > 0 ) {

        /* Check the pwcStr1 pointer */    
        if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemcmp() found a NULL destination pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }

        /* Check the pwcStr2 pointer */    
        if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemcmp() found a NULL source pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
        
        /* Handle special cases gracefully */
        if ( (pwcStr1 == NULL) && (pwcStr2 == NULL) ) {
            return (0);
        }
        else if ( (pwcStr1 != NULL) && (pwcStr2 == NULL) ) {
            return (1);
        }
        else if ( (pwcStr1 == NULL) && (pwcStr2 != NULL) ) {
            return (-1);
        }
    }


    /* Do the comparison */
    return (wmemcmp(pwcStr1, pwcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wmemcpy()

    Purpose:    Copy zCount wide characters from pwcStr2 to pwcStr1.

    Parameters: pwcStr1     pointer.
                pwcStr2     pointer.
                zCount      length to copy.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to pwcStr1, NULL on error

*/
wchar_t *fs_wmemcpy
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to copy */    
    if ( zCount <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemcpy() tried to copy: %lu bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }
    else if ( zCount > 0 ) {

        /* Check the pwcStr1 pointer */    
        if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemcpy() found a NULL destination pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }

        /* Check the pwcStr2 pointer */    
        if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemcpy() found a NULL source pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }

        /* Check for overlapping blocks */
        if ( (pwcStr1 >= pwcStr2) && (pwcStr1 <= (pwcStr2 + zCount)) ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemcpy() tried to copy overlaping blocks, destination: %lu, source: %lu, bytes: %lu, file: '%s', line: %lu.", 
                    (unsigned long)pwcStr1, (unsigned long)pwcStr2, zCount, pcFile, zLine);
#endif
            return (NULL);
        }
        
        if ( (pwcStr2 >= pwcStr1) && (pwcStr2 <= (pwcStr1 + zCount)) ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemcpy() tried to copy overlaping blocks, destination: %lu, source: %lu, bytes: %lu, file: '%s', line: %lu.", 
                    (unsigned long)pwcStr1, (unsigned long)pwcStr2, zCount, pcFile, zLine);
#endif
            return (NULL);
        }
    }


    return (wmemcpy(pwcStr1, pwcStr2, zCount));
}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wmemmove()

    Purpose:    Move zCount wide characters from pwcStr2 to pwcStr1.

    Parameters: pwcStr1     pointer.
                pwcStr2     pointer.
                zCount      length to move.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to pwcStr1, NULL on error

*/
wchar_t *fs_wmemmove
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to move */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemmove() tried to move: %ld bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }
    else if ( zCount > 0 ) {

        /* Check the p1 pointer */    
        if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemmove() found a NULL destination pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }

        /* Check the p2 pointer */    
        if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemmove() found a NULL source pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }
    }


    return (wmemmove(pwcStr1, pwcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wmemset()

    Purpose:    Set zCount wide characters to iChar from pvPtr.

    Parameters: pwcStr      pointer.
                wcChar      character.
                zCount      length.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to pvPtr

*/
wchar_t *fs_wmemset
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    wchar_t wcChar,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr,
    wchar_t wcChar,
    size_t zCount
#endif
)
{

    /* Check the length to set */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemset() tried to set: %lu bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }
    else if ( zCount > 0 ) {

        /* Check the p pointer */    
        if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wmemset() found a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
            return (NULL);
        }
    }


    return (wmemset(pwcStr, wcChar, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fopen()

    Purpose:    Opens a file.

    Parameters: pcFilePath      file path
                pcMode          the mode to open the file with
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    A pointer to the FILE structure if the open was successful,
                NULL otherwise.

*/
FILE *fs_fopen
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    char *pcMode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    char *pcMode
#endif
)
{

    FILE    *pfFile = NULL;


    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fopen() tried to open with a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    if ( bUtlStringsIsStringNULL(pcMode) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fopen() tried to open file: '%s', with a NULL mode, file: '%s', line: %lu.", pcFilePath, pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Open the file */
    pfFile = fopen(pcFilePath, pcMode);


    /* Open failed */
    if ( pfFile == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fopen() failed to open file: '%s', with mode: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pcFilePath, pcMode, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fopen() failed to open file: '%s', with mode: '%s', errno: %d, strerror: '%s'.",
                pcFilePath, pcMode, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pfFile);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fdopen()

    Purpose:    Opens a file descriptor.

    Parameters: iFileDescriptor     the file descriptor
                pcMode              the mode to open the file with
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    A pointer to the FILE structure if the open was successful,
                NULL otherwise.

*/
FILE *fs_fdopen
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int  iFileDescriptor,
    char *pcMode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int  iFileDescriptor,
    char *pcMode
#endif
)
{

    FILE    *pfFile = NULL;


    /* Check the file descriptor */
    if ( iFileDescriptor < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fdopen() tried to open with an invalid descriptor: %d, file: '%s', line: %lu.", iFileDescriptor, pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the mode */
    if ( bUtlStringsIsStringNULL(pcMode) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fdopen() tried to open with a NULL mode, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Open the file */
    pfFile = fdopen(iFileDescriptor, pcMode);


    /* Open failed */
    if ( pfFile == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fdopen() failed to open descriptor: %d, with mode: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iFileDescriptor, pcMode, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fdopen() failed to open descriptor: %d, with mode: '%s', errno: %d, strerror: '%s'.",
                iFileDescriptor, pcMode, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pfFile);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_freopen()

    Purpose:    Reopens a file.

    Parameters: pcFilePath      file path
                pcMode          the mode to open the file with
                pfFile          pointer to the stream to substitute for
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    A pointer to the FILE structure if the reopen was successful,
                NULL otherwise.

*/
FILE *fs_freopen
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    char *pcMode,
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    char *pcMode,
    FILE *pfFile
#endif
)
{

    FILE    *pfNewFile = NULL;


    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "d_freopen() tried to open with a NULL file path, file: '%s', line: %lu.", 
                pcFile, zLine);
#endif
        return (NULL);
    }

    if ( bUtlStringsIsStringNULL(pcMode) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_freopen() tried to open file: '%s', with a NULL mode, file: '%s', line: %lu.", 
                pcFilePath, pcFile, zLine);
#endif
        return (NULL);
    }

    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_freopen() tried to open file: '%s', with mode: '%s', from a NULL file pointer, file: '%s', line: %lu.", 
                pcFilePath, pcMode, pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Reopen the file */
    pfNewFile = freopen(pcFilePath, pcMode, pfFile);    


    /* Reopen failed */
    if ( pfNewFile == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_freopen() failed to reopen file: '%s', with mode: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pcFilePath, pcMode, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_freopen() failed to reopen file: '%s', with mode: '%s', errno: %d, strerror: '%s'].",
                pcFilePath, pcMode, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pfNewFile);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fclose()

    Purpose:    Close an open file.

    Parameters: pfFile      FILE pointer
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 if the file was successfully closed, non-0 otherwise

*/
int fs_fclose
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    FILE *pfFile
#endif
)
{

    int     iStatus = 0;


    /* Check the passed file pointer */
    if ( pfFile == NULL ) { 
#if defined(UTL_CWRAPPERS_ENABLE_NULL_FILE_POINTER_CLOSE_LOGGING)
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fclose() found a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
#endif    /* defined(UTL_CWRAPPERS_ENABLE_NULL_FILE_POINTER_CLOSE_LOGGING) */
        return (EOF);
    }


    /* Close the file */
    iStatus = fclose(pfFile);


    if ( iStatus != 0 ) {

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fclose() failed to close file, ferror(): %d, file: '%s', line: %lu.", ferror(pfFile), pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fclose() failed to close file, ferror(): %d.", ferror(pfFile));
#endif

    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fread()

    Purpose:    Read a number of records from the stream.

    Parameters: pvPtr       pointer to read to.
                zSize       size of record to read.
                zCount      number of records to read.
                pfFile      stream to read from.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of records read, this will equal n if 
                the read was successful
*/
size_t fs_fread
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr,
    size_t zSize,
    size_t zCount,
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr,
    size_t zSize,
    size_t zCount,
    FILE *pfFile
#endif
)
{

    size_t      zLocalCount = (size_t)0;


    /* Check the pointer */    
    if ( pvPtr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fread() tried to read to a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }

    /* Check the record size */    
    if ( zSize < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fread() tried to read elements of: %lu bytes, file: '%s', line: %lu.", zSize, pcFile, zLine);
#endif
        return (0);
    }

    /* Check the record count */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fread() tried to read: %lu elements, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }

    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fread() tried to read from a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Read the data */
    zLocalCount = fread(pvPtr, zSize, zCount, pfFile);


    /* Read failed and we have not hit the end of the stream */    
    if ( (zLocalCount != zCount) && (feof(pfFile) == 0) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fread() tried to read: %lu elements of: %lu bytes, and only read: %lu elements, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                zCount, zSize, zLocalCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fread() tried to read: %lu elements of: %lu bytes, and only read: %lu elements, errno: %d, strerror: '%s'.", 
                zCount, zSize, zLocalCount, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (zLocalCount);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fwrite()

    Purpose:    Write a number of records to the stream.

    Parameters: pvPtr       pointer to write from.
                zSize       size of record to write.
                zCount      number of records to write.
                pfFile      stream to write to.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of records written, this will equal n if 
                the write was successful
*/
size_t fs_fwrite
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvPtr,
    size_t zSize,
    size_t zCount,
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvPtr,
    size_t zSize,
    size_t zCount,
    FILE *pfFile
#endif
)
{

    size_t      zLocalCount = (size_t)0;


    /* Check the pointer */    
    if ( pvPtr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fwrite() tried to write from a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }

    /* Check the record size */    
    if ( zSize < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fwrite() tried to write elements of: %lu bytes, file: '%s', line: %lu.", zSize, pcFile, zLine);
#endif
        return (0);
    }

    /* Check the record count */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fwrite() tried to write: %lu elements, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }

    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fwrite() tried to write to a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Write the data */
    zLocalCount = fwrite(pvPtr, zSize, zCount, pfFile);


    /* Write failed and we have not hit the end of the stream */    
    if ( (zLocalCount != zCount) && (feof(pfFile) == 0) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fwrite() tried to write: %lu elements of: %lu bytes, and only wrote: %lu elements, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                zCount, zSize, zLocalCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fwrite() tried to write: %lu elements of: %lu bytes, and only wrote: %lu elements, errno: %d, strerror: '%s'.", 
                zCount, zSize, zLocalCount, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (zLocalCount);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fgets()

    Purpose:    Read up to n characters from the stream
                (or until we hit a new line in the byte stream).

    Parameters: pcStr       string pointer to read into.
                iCount      maximum number of bytes to read.
                pfFile      stream to read from.
                pcFile      __FILE__
                zLine       __LINE__


    Globals:    none

    Returns:    NULL if the read failed
*/
char *fs_fgets
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    int iCount,
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    int iCount,
    FILE *pfFile
#endif
)
{

    char    *pcPtr = NULL;


    /* Check the string pointer */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fgets() tried to read to a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the max number of characters */    
    if ( iCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fgets() set the max number of characters to read to: %d bytes, file: '%s', line: %lu.",
                iCount, pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fgets() tried to read from a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Read the string */
    pcPtr = fgets(pcStr, iCount, pfFile);


    /* Read failed and we have not hit the end of the stream */    
    if ( (pcPtr == NULL) && (feof(pfFile) == 0) ) {

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fgets() failed to read string, ferror(): %d, file: '%s', line: %lu.", ferror(pfFile), pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fgets() failed to read string, ferror(): %d.", ferror(pfFile));
#endif

    }


    return (pcPtr);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fputs()

    Purpose:    Write the string to the stream.

    Parameters: pcStr       string pointer to write from.
                pfFile      stream to write to.
                pcFile      __FILE__
                zLine       __LINE__


    Globals:    none

    Returns:    Non-negative number on success, EOF on error
*/
int fs_fputs
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    FILE *pfFile
#endif
)
{

    int     iStatus = 0;


    /* Check the string pointer */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fputs() tried to write from a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fputs() tried to write to a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Write the string */
    iStatus = fputs(pcStr, pfFile);


    /* Write failed if we got an EOF */    
    if ( iStatus == EOF ) {

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fputs() failed to write string, ferror(): %d, file: '%s', line: %lu.", ferror(pfFile), pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fputs() failed to write string, ferror(): %d.", ferror(pfFile));
#endif

    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fflush()

    Purpose:    Flush the stream.

    Parameters: pfFile      The stream to flush.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 indicates that no error occured,
                EOF is returned if the flush failed
*/
int fs_fflush
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    FILE *pfFile
#endif
)
{

    int     iStatus = 0;


    /* Flush the file, pfFile may be NULL which is why we dont check for it */
    iStatus = fflush(pfFile);


    /* Flush failed */
    if ( iStatus != 0 ) {

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fflush() failed, ferror(): %d, file: '%s', line: %lu.", ferror(pfFile), pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fflush() failed, ferror(): %d.", ferror(pfFile));
#endif

    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fseek()

    Purpose:    Seek to a position in a file.

    Parameters: pfFile          FILE pointer
                zOffset         the offset to seek to
                iWhereFrom      the origin to seek from (defined in posix.h/stdio.h)
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, non-0 otherwise

*/
int fs_fseek
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    FILE* pfFile,
    off_t zOffset,
    int iWhereFrom,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    FILE* pfFile,
    off_t zOffset,
    int iWhereFrom
#endif
)
{

    int     iStatus = 0;


    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fseek() tried to seek on a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Check the offset */    
    if ( (iWhereFrom == SEEK_SET) && (zOffset < 0) ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_fseek() tried to perform a negative seek, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Do the seek - map to fseeko() */ 
    iStatus = fseeko(pfFile, zOffset, iWhereFrom);
/*     iStatus = fseek(pfFile, zOffset, iWhereFrom); */


    /* Seek failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fseek() failed, offset: %ld, whereFrom: %d, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                zOffset, iWhereFrom, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fseek() failed, offset: %ld, whereFrom: %d, errno: %d, strerror: '%s'.", 
                zOffset, iWhereFrom, iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_ftell()

    Purpose:    Returns the current position in the file.

    Parameters: pfFile      FILE pointer
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    -1 if an error occurs, positive value otherwise.

*/
off_t fs_ftell
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    FILE *pfFile
#endif
)
{

    off_t   zStatus = 0;


    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_ftell() tried to tell on a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Do the tell - map to ftello() */
    zStatus = ftello(pfFile);
/*     zStatus = (off_t)ftell(pfFile); */


    /* Tell failed */
    if ( zStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_ftell() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_ftell() failed, errno: %d, strerror: '%s'.", iError, pucBuffer);
#endif

          errno = iError;
    }

    
    return (zStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_feof()

    Purpose:    Checks whether we have hit an end of file on this stream.

    Parameters: pfFile      The stream to check.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    non-0 value if we are positioned at the end of the file,
                0 otherwise
*/
int fs_feof
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    FILE* pfFile
#endif
)
{

    int     iStatus = 0;


    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_feof() tried to check on a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Check feof() */ 
    iStatus = feof(pfFile);


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_ferror()

    Purpose:    Return the current error on this stream.

    Parameters: pfFile      The stream to check.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 indicates that no error has occured, non-0 otherwise
*/
int fs_ferror
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    FILE* pfFile
#endif
)
{

    int     iStatus = 0;


    /* Check the stream pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_ferror() tried to check on a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Check ferror() */ 
    iStatus = ferror(pfFile);


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_clearerr()

    Purpose:    Clears the current error on this stream.

    Parameters: pfFile      The stream to check.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    void

*/
void fs_clearerr
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    FILE *pfFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    FILE* pfFile
#endif
)
{

    /* Check the file pointer */    
    if ( pfFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_clearerr() tried to clear errors on a NULL file pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return;
    }

    /* Check clearerr() */ 
    clearerr(pfFile);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_truncate()

    Purpose:    Truncate this file.

    Parameters: pcFilePath      file path
                zLen            The new length
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 indicates that no error has occured, -1 otherwise

*/
int fs_truncate
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    off_t zLen,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    off_t zLen
#endif
)
{

    int     iStatus = 0;


    /* Check the file pointer */    
    if ( pcFilePath == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_truncate() tried to truncate a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the length */    
    if ( zLen < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_truncate() tried to truncate to a negative length, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Truncate the file */ 
    iStatus =  truncate(pcFilePath, zLen);

    /* Truncate failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_truncate() failed, errno: %d, strerror: '%s', file: '%s', line: %lu, iStatus: %ld.", iError, pucBuffer, pcFile, zLine, iStatus);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_truncate() failed, errno: %d, strerror: '%s'.", iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_open()

    Purpose:    Opens a file.

    Parameters: pcFilePath      file path
                iFlags          flags
                iMode           modes
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    A valid file descriptor if the open was successful,
                -1 otherwise.

*/
int fs_open
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    int iFlags,
    int iMode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    int iFlags,
    int iMode
#endif
)
{

    int     iFile = 0;


    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_open() tried to open with a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Open the file */
    iFile = open(pcFilePath, iFlags, iMode);


    /* Open failed */
    if ( iFile == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_open() failed to open file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_open() failed to open file: '%s', errno: %d, strerror: '%s'.",
                pcFilePath, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iFile);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_close()

    Purpose:    Close a file.

    Parameters: iFile       file descriptor
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on failure.

*/
int fs_close
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile
#endif
)
{

    int     iStatus = 0;


    /* Check the file descriptor */
    if ( iFile < 0 ) { 
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_close() tried to close an invalid file descriptor: %d, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Close the file */
    iStatus = close(iFile);


    /* Close failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_close() failed to close file, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
/*         iUtlLogError(UTL_LOG_CONTEXT, "fs_close() failed to close file, errno: %d, strerror: '%s'.", */
/*                 iError, pucBuffer); */
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_read()

    Purpose:    Read from a file descriptor/pipe/socket.

    Parameters: iFile       file descriptor.
                pcStr       pointer to read to.
                zCount      number of bytes to read.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of bytes actually read
*/
ssize_t fs_read
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    char *pcStr,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile,
    char *pcStr,
    size_t zCount
#endif
)
{

    ssize_t     zLocalCount = (ssize_t)0;


    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_read() tried to read from an invalid file descriptor: %d, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the pointer */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_read() tried to read to a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the number of bytes */    
    if ( zCount < (size_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fread() tried to read: %lu bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Read the data */
    zLocalCount = read(iFile, pcStr, zCount);


    /* Read failed */
    if ( zLocalCount == -1 ) {

        if ( (errno != 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_read() tried to read: %ld bytes, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                (long)zCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_read() tried to read: %ld bytes, errno: %d, strerror: '%s'.", 
                (long)zCount, iError, pucBuffer);
#endif

            errno = iError;
        }
    }
    else if ( zLocalCount != zCount ) {

        if ( errno != 0 ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);
    
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_read() tried to read: %ld bytes, and only read: %ld bytes, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    (long)zCount, (long)zLocalCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_read() tried to read: %ld bytes and only read: %ld bytes, errno: %d, strerror: '%s'.", 
                    (long)zCount, (long)zLocalCount, iError, pucBuffer);
#endif

             errno = iError;
        }
    }


    return (zLocalCount);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_write()

    Purpose:    Write to a file descriptor/pipe/socket.

    Parameters: iFile       file descriptor.
                pcStr       pointer to write from.
                zCount      number of bytes to write.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of bytes actually written
*/
ssize_t fs_write
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    char *pcStr,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile,
    char *pcStr,
    size_t zCount
#endif
)
{

    ssize_t     zLocalCount = (ssize_t)0;


    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_write() tried to write to an invalid file descriptor: %d, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the pointer */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_write() tried to write from a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the number of bytes */    
    if ( zCount < (size_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_write() tried to write: %lu bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Write the data */
    zLocalCount = write(iFile, pcStr, zCount);


    /* Write failed */
    if ( zLocalCount == -1 ) {

        if ( (errno != 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_write() failed to write: %ld bytes, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    zCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_write() failed to write: %ld bytes, errno: %d, strerror: '%s'.", 
                    zCount, iError, pucBuffer);
#endif

            errno = iError;
        }
    }
    else if ( zLocalCount != zCount ) {

        if ( errno != 0 ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_write() tried to write: %lu bytes, and only wrote: %ld bytes, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    zCount, zLocalCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_write() tried to write: %lu bytes and only wrote: %ld bytes, errno: %d, strerror: '%s'.", 
                    zCount, zLocalCount, iError, pucBuffer);
#endif

             errno = iError;
         }
    }

    return (zLocalCount);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_lseek()

    Purpose:    Seek to a position in a file.

    Parameters: iFile           file descriptor.
                zOffset         the offset to seek to
                iWhereFrom      the origin to seek from (defined in futils.h/stdio.h)
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, non-0 otherwise

*/
off_t fs_lseek
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    off_t zOffset,
    int iWhereFrom,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile,
    off_t zOffset,
    int iWhereFrom
#endif
)
{

    off_t   zStatus = (off_t)0;


    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_lseek() tried to seek on an invalid file descriptor: %d, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Do the seek */ 
    zStatus = lseek(iFile, zOffset, iWhereFrom);


    /* Seek failed */
    if ( zStatus < (off_t)0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_lseek() failed, offset: %ld, whereFrom: %d, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                zOffset, iWhereFrom, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_lseek() failed, offset: %ld, whereFrom: %d, errno: %d, strerror: '%s'.",
                zOffset, iWhereFrom, iError, pucBuffer);
#endif

          errno = iError;
    }


    return (zStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_ioctl()

    Purpose:    Set file/socket descriptor options.

    Parameters: iFile       file descriptor.
                iRequest    request.
                pvArgs      request args
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of bytes actually sent
*/
int fs_ioctl
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    int iRequest,
    void *pvArgs,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile,
    int iRequest,
    void *pvArgs
#endif
)
{

    int     iStatus = 0;


    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_ioctl() tried to set an invalid file/socket descriptor: %d, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Do the ioctl */
    iStatus = ioctl(iFile, iRequest, pvArgs);


    /* Ioctl failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_ioctl() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_ioctl() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_poll()

    Purpose:    synchronous I/O multiplexing

    Parameters: pPollFd     poll structure.
                zNfds       number of entries in the poll structure.
                iTimeOut    timeout
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of bytes actually sent
*/
int fs_poll
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    struct pollfd *pPollFd, 
    nfds_t zNfds, 
    int iTimeOut,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    struct pollfd *pPollFd, 
    nfds_t zNfds, 
    int iTimeOut
#endif
)
{


    int     iStatus = 0;


    /* Check the poll structure */    
    if ( pPollFd == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_poll() was not passed any pollfd structures to check/set, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check that we were passed at least one fd_set struture */    
    if ( zNfds <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_poll() tried to select an invalid number of file descriptors: %d, file: '%s', line: %lu.", (int)zNfds, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Do the poll */
    iStatus = poll(pPollFd , zNfds, iTimeOut);


    /* Poll failed */
    if ( (iStatus == -1) && (errno != EINTR) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_poll() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_poll() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_pipe()

    Purpose:    Set up an IPC pipe.

    Parameters: piFile      file descriptor.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error
*/
int fs_pipe
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int *piFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int *piFile
#endif
)
{

    int     iStatus = 0;


    /* Check the file descriptor */    
    if ( piFile == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pipe() tried to set up a pipe with NULL file descriptors, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Set up the pipe */
    iStatus = pipe(piFile);


    /* Set up failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pipe() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pipe() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_dup()

    Purpose:    Duplicate a file descriptor.

    Parameters: iFile       file descriptor.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error
*/
int fs_dup
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile
#endif
)
{

    int     iStatus = 0;


    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_dup() tried to duplicate an invalid file descriptor: %d, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Duplicate the file descriptor */
    iStatus = dup(iFile);


    /* Duplication failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_dup() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_dup() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_dup2()

    Purpose:    Duplicate a file descriptor.

    Parameters: iFile1      file descriptor.
                iFile2      file descriptor.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error
*/
int fs_dup2
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile1,
    int iFile2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile1,
    int iFile2
#endif
)
{

    int     iStatus = 0;


    /* Check the file descriptor */    
    if ( iFile1 < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_dup2() tried to duplicate an invalid file descriptor 1: %d, file: '%s', line: %lu.", iFile1, pcFile, zLine);
#endif
        return (-1);
    }

    if ( iFile2 < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_dup2() tried to duplicate an invalid file descriptor 2: %d, file: '%s', line: %lu.", iFile2, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Duplicate the file descriptor */
    iStatus = dup2(iFile1, iFile2);


    /* Duplication failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_dup2() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_dup2() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_ftruncate()

    Purpose:    Truncate this file descriptor.

    Parameters: iFile       The file descriptor to truncate
                zLen        The new length
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 indicates that no error has occured, -1 otherwise

*/
int fs_ftruncate
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    off_t zLen,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile,
    off_t zLen
#endif
)
{

    int     iStatus = 0;


    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_ftruncate() tried to truncate a negative file descriptor, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the length */    
    if ( zLen < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_ftruncate() tried to truncate to a negative length, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Truncate the file */ 
    iStatus =  ftruncate(iFile, zLen);

    /* Truncate failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_ftruncate() failed, errno: %d, strerror: '%s', file: '%s', line: %lu, iStatus: %ld.", 
                iError, pucBuffer, pcFile, zLine, iStatus);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_ftruncate() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_socket()

    Purpose:    Open a socket.

    Parameters: iDomain     domain
                iType       type
                iProtocol   protocol
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the socket descriptor, or -1 on error

*/
int fs_socket
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iDomain,
    int iType,
    int iProtocol,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iDomain,
    int iType,
    int iProtocol
#endif
)
{

    int     iSocket = -1;


    /* Check the domain */    
    if ( iDomain < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_socket() tried to open with an invalid domain: %d, file: '%s', line: %lu.", iDomain, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the type */    
    if ( iType < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_socket() tried to open with an invalid type: %d, file: '%s', line: %lu.", iType, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the protocol */    
    if ( iProtocol < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_socket() tried to open with an invalid protocol: %d, file: '%s', line: %lu.", iProtocol, pcFile, zLine);
#endif
        return (-1);
    }


    /* Open the socket */
    iSocket = socket(iDomain, iType, iProtocol);


    /* Open failed */
    if ( iSocket == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_socket() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_socket() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iSocket);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_bind()

    Purpose:    Bind to a socket.

    Parameters: iSocket                 socket
                psaSocketAddress        socket address
                zSocketAddressLength    socket address length
                pcFile                  __FILE__
                zLine                   __LINE__
    
    Globals:    none

    Returns:    0 on success, or -1 on error

*/
int fs_bind
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    struct sockaddr *psaSocketAddress,
    socklen_t zSocketAddressLength,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    struct sockaddr *psaSocketAddress,
    socklen_t zSocketAddressLength
#endif
)
{

    int     iStatus = 0;


    /* Check the socket */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_bind() tried to bind with an invalid socket: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the socket address */    
    if ( psaSocketAddress == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_bind() tried to bind with a NULL socket address, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the socket address length */    
    if ( zSocketAddressLength < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_bind() tried to bind with a invalid socket address length: %d, file: '%s', line: %lu.", zSocketAddressLength, pcFile, zLine);
#endif
        return (-1);
    }


    /* Bind to the socket */
    iStatus = bind(iSocket, psaSocketAddress, zSocketAddressLength);


    /* Bind failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_bind() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_bind() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_connect()

    Purpose:    Connect to a socket.

    Parameters: iSocket                 socket
                psaSocketAddress        socket address
                zSocketAddressLength    socket address length
                pcFile                  __FILE__
                zLine                   __LINE__

    Globals:    none

    Returns:    0 on success, or -1 on error

*/
int fs_connect
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    struct sockaddr *psaSocketAddress,
    socklen_t zSocketAddressLength,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    struct sockaddr *psaSocketAddress,
    socklen_t zSocketAddressLength
#endif
)
{

    int     iStatus = 0;


    /* Check the socket */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_connect() tried to connect with an invalid socket: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the socket address */    
    if ( psaSocketAddress == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_connect() tried to connect with a NULL socket address, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the socket address length */    
    if ( zSocketAddressLength < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_connect() tried to connect with a invalid socket address length: %d, file: '%s', line: %lu.", zSocketAddressLength, pcFile, zLine);
#endif
        return (-1);
    }


    /* Connect to a socket */
    iStatus = connect(iSocket, psaSocketAddress, zSocketAddressLength);


    /* Connect failed */
    if ( (iStatus == -1) && (errno != EINPROGRESS) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_connect() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_connect() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_accept()

    Purpose:    Accept a connection on a socket.

    Parameters: iSocket                 socket
                psaSocketAddress        socket address
                pzSocketAddressLength   socket address length
                pcFile                  __FILE__
                zLine                   __LINE__

    Globals:    none

    Returns:    0 on success, or -1 on error

*/
int fs_accept
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    struct sockaddr *psaSocketAddress,
    socklen_t *pzSocketAddressLength,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    struct sockaddr *psaSocketAddress,
    socklen_t *pzSocketAddressLength
#endif
)
{

    int     iStatus = 0;


    /* Check the socket */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_accept() tried to accept with an invalid socket: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the socket address length if a socket address was specified */    
    if ( (psaSocketAddress != NULL) && (pzSocketAddressLength == NULL) ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_accept() tried to accept with a NULL socket address length, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Accept a connection on the socket */
    iStatus = accept(iSocket, psaSocketAddress, pzSocketAddressLength);


    /* Accept failed */
    if ( (iStatus == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_accept() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_accept() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_listen()

    Purpose:    Listen for connection on a socket.

    Parameters: iSocket     socket
                iBacklog    backlog
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, or -1 on error

*/
int fs_listen
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    int iBacklog,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    int iBacklog
#endif
)
{

    int     iStatus = 0;


    /* Check the socket */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_listen() tried to listen with an invalid socket: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the backlog */    
    if ( iBacklog < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_listen() tried to listen with an invalid backlog: %d, file: '%s', line: %lu.", iBacklog, pcFile, zLine);
#endif
        return (-1);
    }


    /* Listen for connection on the socket */
    iStatus = listen(iSocket, iBacklog);


    /* Listen failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_listen() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_listen() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_setsockopt()

    Purpose:    Set socket option.

    Parameters: iSocket         socket
                iLevel          level
                iOptionName     option name
                pvOptionValue   option value
                zOptionLength   option length
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, or -1 on error

*/
int fs_setsockopt
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    int iLevel,
    int iOptionName,
    void *pvOptionValue,
    socklen_t zOptionLength,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    int iLevel,
    int iOptionName,
    void *pvOptionValue,
    socklen_t zOptionLength
#endif
)
{

    int     iStatus = 0;


    /* Check the socket */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setsockopt() tried to set with an invalid socket: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the level */    
    if ( iLevel < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setsockopt() tried to set with an invalid level: %d, file: '%s', line: %lu.", iLevel, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the option name */    
    if ( iOptionName < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setsockopt() tried to set with an invalid option name: %d, file: '%s', line: %lu.", iOptionName, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the option value */    
    if ( pvOptionValue == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setsockopt() tried to set with a NULL option value, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the option length */    
    if ( zOptionLength < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setsockopt() tried to set with an invalid option length: %d, file: '%s', line: %lu.", zOptionLength, pcFile, zLine);
#endif
        return (-1);
    }


    /* Set the socket option */
    iStatus = setsockopt(iSocket, iLevel, iOptionName, pvOptionValue, zOptionLength);


    /* Accept failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setsockopt() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setsockopt() failed, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

          errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_getsockopt()

    Purpose:    Get socket option.

    Parameters: iSocket             socket
                iLevel              level
                iOptionName         option name
                pvOptionValue       option value
                pzOptionLength      option length
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 on success, or -1 on error

*/
int fs_getsockopt
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    int iLevel,
    int iOptionName,
    void *pvOptionValue,
    socklen_t *pzOptionLength,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    int iLevel,
    int iOptionName,
    void *pvOptionValue,
    socklen_t *pzOptionLength
#endif
)
{

    int     iStatus = 0;


    /* Check the socket */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getsockopt() tried to get with an invalid socket: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the level */    
    if ( iLevel < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getsockopt() tried to get with an invalid level: %d, file: '%s', line: %lu.", iLevel, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the option name */    
    if ( iOptionName < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getsockopt() tried to get with an invalid option name: %d, file: '%s', line: %lu.", iOptionName, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the option value */    
    if ( pvOptionValue == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getsockopt() tried to get with a NULL option value, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the option length */    
    if ( pzOptionLength == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getsockopt() tried to get with a NULL option length: %d, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Get the socket option */
    iStatus = getsockopt(iSocket, iLevel, iOptionName, pvOptionValue, pzOptionLength);


    /* Accept failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getsockopt() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.", iError, 
                pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getsockopt() failed, errno: %d, strerror: '%s'.", iError, 
                pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_recv()

    Purpose:    Receive stuff from a socket.

    Parameters: iSocket     socket number.
                pcStr       pointer to read to.
                iCount      number of bytes to read.
                iFlags      flags.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of bytes actually read

*/
ssize_t fs_recv
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    char *pcStr,
    int iCount,
    int iFlags,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    char *pcStr,
    int iCount,
    int iFlags
#endif
)
{

    ssize_t     zLocalCount = (ssize_t)0;


    /* Check the socket descriptor */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_recv() tried to read from an invalid socket descriptor: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the pointer */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_recv() tried to read to a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the number of bytes */    
    if ( iCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_recv() tried to read: %d bytes, file: '%s', line: %lu.", iCount, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Read the data */
    zLocalCount = recv(iSocket, pcStr, iCount, iFlags);


    /* Read failed */
    if ( zLocalCount == -1 ) {

        if ( (errno != 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_recv() failed to read: %d bytes, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    iCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_recv() failed to read: %d bytes, errno: %d, strerror: '%s'.", 
                    iCount, iError, pucBuffer);
#endif

            errno = iError;
        }
    }
    else if ( zLocalCount != iCount ) {

        if ( errno != 0 ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);
    
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_recv() tried to read: %d bytes, and only read: %ld, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    iCount, zLocalCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_recv() tried to read: %d bytes and only read: %ld, errno: %d, strerror: '%s'.", 
                    iCount, zLocalCount, iError, pucBuffer);
#endif

             errno = iError;
        }
    }


    return (zLocalCount);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_send()

    Purpose:    Send stuff down a socket.

    Parameters: iSocket     socket number.
                pcStr       pointer to read from.
                iCount      number of bytes to send.
                iFlags      flags.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the number of bytes actually sent

*/
ssize_t fs_send
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSocket,
    char *pcStr,
    int iCount,
    int iFlags,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSocket,
    char *pcStr,
    int iCount,
    int iFlags
#endif
)
{

    ssize_t     zLocalCount = (ssize_t)0;


    /* Check the socket descriptor */    
    if ( iSocket < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_send() tried to write to an invalid socket descriptor: %d, file: '%s', line: %lu.", iSocket, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the pointer */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_send() tried to write from a NULL pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the number of bytes */    
    if ( iCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_send() tried to write: %d bytes, file: '%s', line: %lu.", iCount, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Send the data */
    zLocalCount = send(iSocket, pcStr, iCount, iFlags);


    /* Write failed */
    if ( zLocalCount == -1 ) {

        if ( (errno != 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_send() failed to send: %d bytes, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    iCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_send() failed to send: %d bytes, errno: %d, strerror: '%s'.", 
                    iCount, iError, pucBuffer);
#endif

            errno = iError;
        }
    }
    else if ( zLocalCount != iCount ) {

        if ( errno != 0 ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_send() tried to send: %d bytes, and only sent: %d, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    iCount, zLocalCount, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_send() tried to send: %d bytes and only sent: %d, errno: %d, strerror: '%s'.", 
                    iCount, zLocalCount, iError, pucBuffer);
#endif

             errno = iError;
         }
    }


    return (zLocalCount);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_remove()

    Purpose:    Delete a file.

    Parameters: pcFilePath      file path
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, non-0 otherwise.

*/
int fs_remove
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath
#endif
)
{

    int     iStatus = 0;


    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_remove() tried to remove a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Remove the file */    
    iStatus = remove(pcFilePath);


    /* Remove failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_remove() failed to remove file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_remove() failed to remove file: '%s', errno: %d, strerror: '%s'.", 
                pcFilePath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_rename()

    Purpose:    Rename a file.

    Parameters: pcOldFilePath   the name of the file to rename
                pcNewFilePath   the new name for the file
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, non-0 otherwise.

*/
int fs_rename
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcOldFilePath,
    char *pcNewFilePath,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcOldFilePath,
    char *pcNewFilePath
#endif
)
{

    int     iStatus = 0;


    if ( bUtlStringsIsStringNULL(pcOldFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rename() tried to rename from a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    if ( bUtlStringsIsStringNULL(pcNewFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rename() tried to rename file: '%s', to a NULL file path, file: '%s', line: %lu.", pcOldFilePath, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Rename the file */
    iStatus = rename(pcOldFilePath, pcNewFilePath);


    /* Rename failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rename() failed to rename file from: '%s', to: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcOldFilePath, pcNewFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rename() failed to rename file from: '%s', to: '%s', errno: %d, strerror: '%s'.", 
                pcOldFilePath, pcNewFilePath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_tmpfile()

    Purpose:    Return a pointer to an open temporary file

    Parameters: pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the FILE structure of the open file, NULL 
                otherwise

*/
FILE *fs_tmpfile
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
#endif
)
{

    FILE    *pfFile = NULL;


    /* Reset the error */
    errno = EOK;


    /* Create the temporary file */
    pfFile = (FILE *)tmpfile();


    /* Create failed */
    if ( pfFile == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_tmpfile() failed to create a temporary file, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_tmpfile() failed to create a temporary file, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (pfFile);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_tmpnam()

    Purpose:    returns a valid temporary name for a file

    Parameters: pcFilePath      pointer to a valid string to store the file path
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    A pointer to a valid temporary name, otherwise NULL

*/
char *fs_tmpnam
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath
#endif
)
{

    char    *pcResult = NULL;


    /* Reset the error */
    errno = EOK;


    /* Create the temporary file path */
    pcResult = (char *)tmpnam(pcFilePath);


    /* Create failed */
    if ( pcResult == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_tmpnam() failed to provide a temporary file path, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_tmpnam() failed to provide a temporary file path, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (pcResult);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_stat()

    Purpose:    Gets the status of a file.

    Parameters: pcFilePath      file path
                pstStat         stat structure to fill in
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_stat
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    struct stat *pstStat,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    struct stat *pstStat
#endif
)
{

    int     iStatus = 0;


    /* Check the file path */
    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_stat() tried to stat a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the return pointer to the stat structure */
    if ( pstStat == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_stat() tried to stat file: '%s', into a NULL stat structure, file: '%s', line: %lu.", pcFilePath, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Stat the file */
    iStatus = stat(pcFilePath, pstStat);


    /* Stat failed */
    if ( (iStatus != 0) && (errno != ENOENT) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

        if ( iError != 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_stat() failed to stat file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                    pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_stat() failed to stat file: '%s', errno: %d, strerror: '%s'.", 
                    pcFilePath, iError, pucBuffer);
#endif
        }

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_lstat()

    Purpose:    Gets the status of a link.

    Parameters: pcFilePath      file path
                pstStat         stat structure to fill in
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_lstat
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    struct stat *pstStat,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    struct stat *pstStat
#endif
)
{

    int     iStatus = 0;


    /* Check the file path */
    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_lstat() tried to stat a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the return pointer to the stat structure */
    if ( pstStat == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_lstat() tried to stat file: '%s', into a NULL stat structure, file: '%s', line: %lu.", pcFilePath, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Stat the file */
    iStatus = lstat(pcFilePath, pstStat);


    /* Stat failed */
    if ( (iStatus != 0) && (errno != ENOENT) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_lstat() failed to stat file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_lstat() failed to stat file: '%s', errno: %d, strerror: '%s'.", 
                pcFilePath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fstat()

    Purpose:    Gets the status of a file descriptor.

    Parameters: iFile       file descriptor
                pstStat     stat structure to fill in
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_fstat
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFile,
    struct stat *pstStat,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFile,
    struct stat *pstStat
#endif
)
{

    int     iStatus = 0;


    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fstat() tried to write to an invalid file descriptor: %d, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the return pointer to the stat structure */
    if ( pstStat == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fstat() tried to stat file descriptor: %d into a NULL stat structure, file: '%s', line: %lu.", iFile, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Stat the file descriptor */
    iStatus = fstat(iFile, pstStat);


    /* Stat failed */
    if ( (iStatus != 0) && (errno != ENOENT) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fstat() failed to stat file: %d, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iFile, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fstat() failed to stat file: %d, errno: %d, strerror: '%s'.", 
                iFile, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_access()

    Purpose:    Gets the status of a link.

    Parameters: pcFilePath      file path
                iAccessMode     access mode
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_access
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    int iAccessMode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    int iAccessMode
#endif
)
{

    int     iStatus = 0;


    /* Check the file path */
    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_access() to access a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Access the file */
    iStatus = access(pcFilePath, iAccessMode);


    /* Access failed */
    if ( (iStatus != 0) && (errno != ENOENT) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_access() failed to access file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_access() failed to access file: '%s', errno: %d, strerror: '%s'.", 
                pcFilePath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_chmod()

    Purpose:    Modify the file permissions

    Parameters: pcFilePath      file path
                zMode           permissions mode
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_chmod
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    mode_t zMode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    mode_t zMode
#endif
)
{

    int     iStatus = 0;


    /* Check the file path */
    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chmod() tried to modify a file's permissions with a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Modify the permissions */
    iStatus = chmod(pcFilePath, zMode);


    /* Modify failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chmod() failed to modify the permissions on file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chmod() failed to modify the permissions on file: '%s', errno: %d, strerror: '%s'.", 
                pcFilePath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_chown()

    Purpose:    Modify the file ownership

    Parameters: pcFilePath      file path
                zUserID         user ID
                zGroupID        group ID
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_chown
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    uid_t zUserID,
    gid_t zGroupID,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    uid_t zUserID,
    gid_t zGroupID
#endif
)
{

    int     iStatus = 0;


    /* Check the file path */
    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chown() tried to modify a file's permissions with a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Modify the ownership */
    iStatus = chown(pcFilePath, zUserID, zGroupID);


    /* Modify failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chown() failed to modify the permissions on file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chown() failed to modify the permissions on file: '%s', errno: %d, strerror: '%s'.", 
                pcFilePath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_getcwd()

    Purpose:    Gets the current working directory pathname.

    Parameters: pcPathname      pathname
                zCount          length of pathname
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    non-NULL if successful, NULL on failure.

*/
char *fs_getcwd
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcPathname,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcPathname,
    size_t zCount
#endif
)
{

    char    *pcResult = NULL;


    /* Check the number of bytes */    
    if ( (pcPathname != NULL) && (zCount < 0) ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getcwd() got a directory pointer of: %lu bytes, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Get the working pathname */    
    pcResult = (char *)getcwd(pcPathname, zCount);


    /* Get failed */
    if ( pcResult == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getcwd() failed to get the current directory, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getcwd() failed to get the current directory, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (pcResult);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_mkdir()

    Purpose:    Create a new directory specifed by the path

    Parameters: pcDirectoryPath     pointer to a directory path
                zMode               permissions mode
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_mkdir
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcDirectoryPath,
    mode_t zMode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcDirectoryPath,
    mode_t zMode
#endif
)
{

    int     iStatus = 0;

    /* Check the directory path */
    if ( bUtlStringsIsStringNULL(pcDirectoryPath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mkdir() tried to create a directory with a NULL directory path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Create the directory */
    iStatus = mkdir(pcDirectoryPath, zMode);


    /* Create failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mkdir() failed to create directory: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcDirectoryPath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mkdir() failed to create directory: '%s', errno: %d, strerror: '%s'.", 
                pcDirectoryPath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_rmdir()

    Purpose:    Remove the directory specifed by the path

    Parameters: pcDirectoryPath     pointer to a directory path
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_rmdir
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcDirectoryPath,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcDirectoryPath
#endif
)
{


    int     iStatus = 0;


    /* Check the directory path */
    if ( bUtlStringsIsStringNULL(pcDirectoryPath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rmdir() tried to remove a directory with a NULL directory path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Remove the directory */
    iStatus = rmdir(pcDirectoryPath);


    /* Remove failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rmdir() failed to remove directory: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcDirectoryPath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rmdir() failed to remove directory: '%s', errno: %d, strerror: '%s'.", 
                pcDirectoryPath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_chdir()

    Purpose:    Change the current directory the that specified in the directory path

    Parameters: pcDirectoryPath     pointer to a directory path
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_chdir
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcDirectoryPath,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcDirectoryPath
#endif
)
{


    int     iStatus = 0;


    /* Check the directory path */
    if ( bUtlStringsIsStringNULL(pcDirectoryPath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chdir() tried to change directory with a NULL directory path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Change the directory */
    iStatus = chdir(pcDirectoryPath);


    /* Change failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chdir() failed to change to directory: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcDirectoryPath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_chdir() failed to change to directory: '%s', errno: %d, strerror: '%s'.", 
                pcDirectoryPath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_opendir()

    Purpose:    Open the directory to read all the entries

    Parameters: pcDirectoryPath     pointer to a directory path
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    a pointer to a DIR structure if successful, NULL on error

*/
DIR *fs_opendir
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcDirectoryPath,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcDirectoryPath
#endif
)
{

    DIR     *pdDirectory = NULL;


    /* Check the directory structure */
    if ( bUtlStringsIsStringNULL(pcDirectoryPath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_opendir() tried to open a directory with a NULL directory path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Open the directory */
    pdDirectory = opendir(pcDirectoryPath);


    /* Open failed */
    if ( pdDirectory == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_opendir() failed to open the directory: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                pcDirectoryPath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_opendir() failed to open the directory: '%s', errno: %d, strerror: '%s'.", 
                pcDirectoryPath, iError, pucBuffer);
#endif

        errno = iError;
    }


    return (pdDirectory);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_readdir()

    Purpose:    Read the next entry in the directory structure

    Parameters: pdDirectory     pointer to a DIR structure
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    a pointer to a DIR structure if successful, NULL on error

*/
struct dirent *fs_readdir
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    DIR    *pdDirectory,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    DIR    *pdDirectory
#endif
)
{

    struct dirent   *pdeDirectoryEntry = NULL;


    /* Check the directory structure */
    if ( pdDirectory == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_readdir() tried to read a directory entry in a NULL directory structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Read the directory entry */
    pdeDirectoryEntry = readdir(pdDirectory);


    /* Read failed */
    if ( pdeDirectoryEntry == NULL ) {

        if ( errno != 0 ) {

            int            iError = errno;
            unsigned char  pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};
    
            /* Get the error string */
            strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

            /* Dont report the "no such file or directory" error as it is how we detect the last directory entry */
            if ( iError != ENOENT ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
                iUtlLogError(UTL_LOG_CONTEXT, "fs_readdir() failed to read the directory entry, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                        iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
                iUtlLogError(UTL_LOG_CONTEXT, "fs_readdir() failed to read the directory entry, errno: %d, strerror: '%s'.", 
                        iError, pucBuffer);
#endif
            }

            errno = iError;
        }
    }


    return (pdeDirectoryEntry);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_rewinddir()

    Purpose:    Rewind the directory entry pointer in the directory structure

    Parameters: pdDirectory     pointer to a DIR structure
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    void

*/
void fs_rewinddir
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    DIR    *pdDirectory,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    DIR    *pdDirectory
#endif
)
{

    /* Check the directory structure */
    if ( pdDirectory == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_rewinddir() tried to rewind a NULL directory structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return;
    }


    /* Rewind the directory entry */
    rewinddir(pdDirectory);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_closedir()

    Purpose:    Close the directory structure

    Parameters: pdDirectory     pointer to a DIR structure
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
int fs_closedir
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    DIR    *pdDirectory,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    DIR    *pdDirectory
#endif
)
{

    int     iStatus = 0;

    if ( pdDirectory == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_closedir() tried to close a NULL directory structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Read the directory entry */
    iStatus = closedir(pdDirectory);


    /* Read failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_closedir() failed to close the directory structure, errno: %d, strerror: '%s', file: '%s', line: %lu.", 
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_closedir() failed to close the directory structure, errno: %d, strerror: '%s'.", 
                iError, pucBuffer);
#endif

        errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strlen()

    Purpose:    Return the length of pcPtr.

    Parameters: pcStr       The string to measure.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The length of the string, -1 on error

*/
size_t fs_strlen
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr
#endif
)
{

    /* Check the source string */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strlen() tried to get the length of a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    return (strlen(pcStr));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strcpy()

    Purpose:    Copy pcStr2 to pcStr1.

    Parameters: pcStr1      destination string pointer.
                pcStr2      source string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string, NULL on error

*/
char *fs_strcpy
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcpy() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcpy() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strcpy(pcStr1, pcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strncpy()

    Purpose:    Copy n characters from pcStrs2 to pcStr1.

    Parameters: pcStr1      destination string pointer.
                pcStr2      source string pointer.
                zCount      length to copy.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string, NULL on error

*/
char *fs_strncpy
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to copy */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncpy() tried to copy a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the destination pointer */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncpy() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncpy() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strncpy(pcStr1, pcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strcat()

    Purpose:    Cat pcStr2 to pcStr1.

    Parameters: pcStr1      destination string pointer.
                pcStr2      source string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string

*/
char *fs_strcat
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcat() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcat() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strcat(pcStr1, pcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strncat()

    Purpose:    Cat n characters from pcStr2 to pcStr1.

    Parameters: pcStr1      destination string pointer.
                pcStr2      source string pointer.
                zCount      length to cat.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string

*/
char *fs_strncat
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to cat */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncat() tried to concatenate a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
    }

    /* Check the destination pointer */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncat() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncat() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strncat(pcStr1, pcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strcmp()

    Purpose:    Compare pcStr1 to pcStr2.

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_strcmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Check the source pointer */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Handle special cases gracefully */
    if ( (pcStr1 == NULL) && (pcStr2 == NULL) ) {
        return (0);
    }
    else if ( (pcStr1 != NULL) && (pcStr2 == NULL) ) {
        return (1);
    }
    else if ( (pcStr1 == NULL) && (pcStr2 != NULL) ) {
        return (-1);
    }


    return (strcmp(pcStr1, pcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strncmp()

    Purpose:    Compare n characters of pcStr2 to pcStr1.

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                zCount      length to compare.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_strncmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to compare */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strncmp() tried to compare a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }
    else if ( zCount > 0 ) {

        /* Check the destination pointer */    
        if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Check the source pointer */    
        if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Handle special cases gracefully */
        if ( (pcStr1 == NULL) && (pcStr2 == NULL) ) {
            return (0);
        }
        else if ( (pcStr1 != NULL) && (pcStr2 == NULL) ) {
            return (1);
        }
        else if ( (pcStr1 == NULL) && (pcStr2 != NULL) ) {
            return (-1);
        }
    }


    return (strncmp(pcStr1, pcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strcoll()

    Purpose:    Collate pcStr2 to pcStr1.

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the collation

*/
int fs_strcoll
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcoll() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Check the source pointer */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcoll() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Handle special cases gracefully */
    if ( (pcStr1 == NULL) && (pcStr2 == NULL) ) {
        return (0);
    }
    else if ( (pcStr1 != NULL) && (pcStr2 == NULL) ) {
        return (1);
    }
    else if ( (pcStr1 == NULL) && (pcStr2 != NULL) ) {
        return (-1);
    }


    return (strcoll(pcStr1, pcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strcasecmp()

    Purpose:    Compare pcStr1 to pcStr2 (case insensitive).

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_strcasecmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcasecmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Check the source pointer */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strcasecmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Handle special cases gracefully */
    if ( (pcStr1 == NULL) && (pcStr2 == NULL) ) {
        return (0);
    }
    else if ( (pcStr1 != NULL) && (pcStr2 == NULL) ) {
        return (1);
    }
    else if ( (pcStr1 == NULL) && (pcStr2 != NULL) ) {
        return (-1);
    }


    return (strcasecmp(pcStr1, pcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strncasecmp()

    Purpose:    Compare n characters of pcStr2 to pcStr1 (case insensitive).

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                zCount      length to compare.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_strncasecmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to compare */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strncasecmp() tried to compare a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }
    else if ( zCount > 0 ) {

    /* Check the destination pointer */    
        if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncasecmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Check the source pointer */    
        if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strncasecmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Handle special cases gracefully */
        if ( (pcStr1 == NULL) && (pcStr2 == NULL) ) {
            return (0);
        }
        else if ( (pcStr1 != NULL) && (pcStr2 == NULL) ) {
            return (1);
        }
        else if ( (pcStr1 == NULL) && (pcStr2 != NULL) ) {
            return (-1);
        }
    }


    return (strncasecmp(pcStr1, pcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strchr()

    Purpose:    Check for the presence of cChar in pcStr, starts from the left.

    Parameters: pcStr       string pointer.
                cChar       character.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to cChar if found, NULL otherwise

*/
char *fs_strchr
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char cChar,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    char cChar
#endif
)
{

    /* Check the source string */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strchr() tried to check in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strchr(pcStr, cChar));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strrchr()

    Purpose:    Check for the presence of cChar in pcStr, starts from the right.

    Parameters: pcStr       string pointer.
                cChar       character.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to cChar if found, NULL otherwise
*/
char *fs_strrchr
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char cChar,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    char cChar
#endif
)
{

    /* Check the source string */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strrchr() tried to check in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strrchr(pcStr, cChar));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strstr()

    Purpose:    Check for the presence of pcStr2 in pcStr1, starts from the right.

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to pcStr2 if found, NULL otherwise

*/
char *fs_strstr
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2
#endif
)
{

    /* Check the pcStr1 string */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strstr() tried to search in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the pcStr2 string */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strstr() tried to search for a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strstr(pcStr1, pcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strtok_r()

    Purpose:    Strtok

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                ppcStr3     string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the next token

*/
char *fs_strtok_r
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char **ppcStr3,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2,
    char **ppcStr3
#endif
)
{

    /* Check the source string */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strtok_r() tried to look for a NULL tokens pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (strtok_r(pcStr1, pcStr2, ppcStr3));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strpbrk()

    Purpose:    Strpbrk

    Parameters: pcStr1      string pointer.
                pcStr2      string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    Non null on success

*/
char *fs_strpbrk
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr1,
    char *pcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr1,
    char *pcStr2
#endif
)
{

    /* Check the pcStr1 string */    
    if ( pcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strpbrk() tried to search in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }

    /* Check the pcStr2 string */    
    if ( pcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strpbrk() tried to search for a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    return (strpbrk(pcStr1, pcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strdup()

    Purpose:    Return a duplicate of pcStr.

    Parameters: pcStr       string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to a duplicate of pcStr

*/
char *fs_strdup
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr
#endif
)
{

    char    *pcStrCopy = NULL;


    /* Check the source string */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strdup() tried to duplicate a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Duplicate the string */
    pcStrCopy = strdup(pcStr);


    /* Duplication failed */
    if ( pcStrCopy == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strdup() failed to duplicate string, errno: %d, strerror: '%s', file: '%s', line: %lu",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strdup() failed to duplicate string, errno: %d, strerror: '%s'",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pcStrCopy);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcslen()

    Purpose:    Return the length of pcPtr.

    Parameters: pwcStr      The string to measure.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The length of the string, -1 on error

*/
size_t fs_wcslen
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr
#endif
)
{

    /* Check the source string */    
    if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcslen() tried to get the length of a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    return (wcslen(pwcStr));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcscpy()

    Purpose:    Copy pwcStr2 to pwcStr1.

    Parameters: pwcStr1     destination string pointer.
                pwcStr2     source string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string, NULL on error

*/
wchar_t *fs_wcscpy
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscpy() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscpy() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcscpy(pwcStr1, pwcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsncpy()

    Purpose:    Copy n characters from pwcStrs2 to pwcStr1.

    Parameters: pwcStr1     destination string pointer.
                pwcStr2     source string pointer.
                zCount      length to copy.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string, NULL on error

*/
wchar_t *fs_wcsncpy
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to copy */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncpy() tried to copy a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the destination pointer */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncpy() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncpy() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcsncpy(pwcStr1, pwcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcscat()

    Purpose:    Cat pwcStr2 to pwcStr1.

    Parameters: pwcStr1     destination string pointer.
                pwcStr2     source string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string

*/
wchar_t *fs_wcscat
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscat() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscat() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcscat(pwcStr1, pwcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsncat()

    Purpose:    Cat n characters from pwcStr2 to pwcStr1.

    Parameters: pwcStr1     destination string pointer.
                pwcStr2     source string pointer.
                zCount      length to cat.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the destination string

*/
wchar_t *fs_wcsncat
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to cat */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncat() tried to concatenate a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
    }

    /* Check the destination pointer */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncat() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the source pointer */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncat() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcsncat(pwcStr1, pwcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcscmp()

    Purpose:    Compare pwcStr1 to pwcStr2.

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_wcscmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Check the source pointer */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Handle special cases gracefully */
    if ( (pwcStr1 == NULL) && (pwcStr2 == NULL) ) {
        return (0);
    }
    else if ( (pwcStr1 != NULL) && (pwcStr2 == NULL) ) {
        return (1);
    }
    else if ( (pwcStr1 == NULL) && (pwcStr2 != NULL) ) {
        return (-1);
    }


    return (wcscmp(pwcStr1, pwcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsncmp()

    Purpose:    Compare n characters of pwcStr2 to pwcStr1.

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                zCount      length to compare.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_wcsncmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to compare */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcsncmp() tried to compare a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }
    else if ( zCount > 0 ) {

        /* Check the destination pointer */    
        if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Check the source pointer */    
        if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Handle special cases gracefully */
        if ( (pwcStr1 == NULL) && (pwcStr2 == NULL) ) {
            return (0);
        }
        else if ( (pwcStr1 != NULL) && (pwcStr2 == NULL) ) {
            return (1);
        }
        else if ( (pwcStr1 == NULL) && (pwcStr2 != NULL) ) {
            return (-1);
        }
    }


    return (wcsncmp(pwcStr1, pwcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcscoll()

    Purpose:    Collate pwcStr2 to pwcStr1.

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the collation

*/
int fs_wcscoll
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscoll() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Check the source pointer */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscoll() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Handle special cases gracefully */
    if ( (pwcStr1 == NULL) && (pwcStr2 == NULL) ) {
        return (0);
    }
    else if ( (pwcStr1 != NULL) && (pwcStr2 == NULL) ) {
        return (1);
    }
    else if ( (pwcStr1 == NULL) && (pwcStr2 != NULL) ) {
        return (-1);
    }


    return (wcscoll(pwcStr1, pwcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcscasecmp()

    Purpose:    Compare pwcStr1 to pwcStr2 (case insensitive).

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_wcscasecmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
#endif
)
{

    /* Check the destination pointer */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscasecmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Check the source pointer */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcscasecmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }

    /* Handle special cases gracefully */
    if ( (pwcStr1 == NULL) && (pwcStr2 == NULL) ) {
        return (0);
    }
    else if ( (pwcStr1 != NULL) && (pwcStr2 == NULL) ) {
        return (1);
    }
    else if ( (pwcStr1 == NULL) && (pwcStr2 != NULL) ) {
        return (-1);
    }


    return (wcscasecmp(pwcStr1, pwcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsncasecmp()

    Purpose:    Compare n characters of pwcStr2 to pwcStr1 (case insensitive).

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                zCount      length to compare.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The value of the comparison

*/
int fs_wcsncasecmp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
#endif
)
{

    /* Check the length to compare */    
    if ( zCount < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcsncasecmp() tried to compare a string of length: %lu characters, file: '%s', line: %lu.", zCount, pcFile, zLine);
#endif
        return (0);
    }
    else if ( zCount > 0 ) {

    /* Check the destination pointer */    
        if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncasecmp() found a NULL destination string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Check the source pointer */    
        if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsncasecmp() found a NULL source string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        }
    
        /* Handle special cases gracefully */
        if ( (pwcStr1 == NULL) && (pwcStr2 == NULL) ) {
            return (0);
        }
        else if ( (pwcStr1 != NULL) && (pwcStr2 == NULL) ) {
            return (1);
        }
        else if ( (pwcStr1 == NULL) && (pwcStr2 != NULL) ) {
            return (-1);
        }
    }


    return (wcsncasecmp(pwcStr1, pwcStr2, zCount));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcschr()

    Purpose:    Check for the presence of wcChar in pwcStr, starts from the left.

    Parameters: pwcStr      string pointer.
                wcChar      character.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to wcChar if found, NULL otherwise

*/
wchar_t *fs_wcschr
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    wchar_t wcChar,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr,
    wchar_t wcChar
#endif
)
{

    /* Check the source string */    
    if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcschr() tried to check in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcschr(pwcStr, wcChar));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsrchr()

    Purpose:    Check for the presence of wcChar in pwcStr, starts from the right.

    Parameters: pwcStr      string pointer.
                wcChar      character.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to cChar if found, NULL otherwise

*/
wchar_t *fs_wcsrchr
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    wchar_t wcChar,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr,
    wchar_t wcChar
#endif
)
{

    /* Check the source string */    
    if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcsrchr() tried to check in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcsrchr(pwcStr, wcChar));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsstr()

    Purpose:    Check for the presence of pwcStr2 in pwcStr1, starts from the right.

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to pwcStr2 if found, NULL otherwise

*/
wchar_t *fs_wcsstr
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
#endif
)
{

    /* Check the pwcStr1 string */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcsstr() tried to search in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the pwcStr2 string */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcsstr() tried to search for a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcsstr(pwcStr1, pwcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcstok()

    Purpose:    Strtok

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                ppwcStr3    string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to the next token

*/
wchar_t *fs_wcstok
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    wchar_t **ppwcStr3,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    wchar_t **ppwcStr3
#endif
)
{

    /* Check the source string */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcstok() tried to look for a NULL tokens pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    return (wcstok(pwcStr1, pwcStr2, ppwcStr3));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcspbrk()

    Purpose:    Strpbrk

    Parameters: pwcStr1     string pointer.
                pwcStr2     string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    Non null on success

*/
wchar_t *fs_wcspbrk
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
#endif
)
{

    /* Check the pwcStr1 string */    
    if ( pwcStr1 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcspbrk() tried to search in a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }

    /* Check the pwcStr2 string */    
    if ( pwcStr2 == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcspbrk() tried to search for a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    return (wcspbrk(pwcStr1, pwcStr2));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsdup()

    Purpose:    Return a duplicate of pwcStr.

    Parameters: pwcStr      string pointer.
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    A pointer to a duplicate of pwcStr

*/
wchar_t *fs_wcsdup
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr
#endif
)
{

    wchar_t     *pwcStrCopy = NULL;


    /* Check the source string */    
    if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsdup() tried to duplicate a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Duplicate the string */
    pwcStrCopy = wcsdup(pwcStr);


    /* Duplication failed */
    if ( pwcStrCopy == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsdup() failed to duplicate string, errno: %d, strerror: '%s', file: '%s', line: %lu",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsdup() failed to duplicate string, errno: %d, strerror: '%s'",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pwcStrCopy);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strtol()

    Purpose:    Convert pcStr to a number.

    Parameters: pcStr       string pointer
                ppcEndStr   end string pointer
                iBase       base
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The converted number, 0 on errorr

*/
long fs_strtol
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char **ppcEndStr,
    int iBase,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    char **ppcEndStr,
    int iBase
#endif
)
{

    long    lNumber = 0;


    /* Check the source string */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strtol() tried to convert a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }
/* iUtlLogInfo(UTL_LOG_CONTEXT, "fs_strtol('%s').", pcStr); */


    /* Reset the error */
    errno = EOK;


    /* Convert the string */
    lNumber = strtol(pcStr, ppcEndStr, iBase);


    /* Conversion failed */
    if ( errno != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strtol() failed to convert string: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pcStr, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strtol() failed to convert string: '%s', errno: %d, strerror: '%s'.",
                pcStr, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (lNumber);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strtof()

    Purpose:    Convert pcStr to a number.

    Parameters: pcStr       string pointer
                ppcEndStr   end string pointer
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The converted number, 0 on errorr

*/
float fs_strtof
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char **ppcEndStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    char **ppcEndStr
#endif
)
{

    float   fNumber = 0;


    /* Check the source string */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strtof() tried to convert a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


/* Solaris does not have strtof() so we need to use strtod() */
#if defined(__sun__)
    /* Convert the string */
    fNumber = (float)strtod(pcStr, ppcEndStr);
#else
    /* Convert the string */
    fNumber = strtof(pcStr, ppcEndStr);
#endif    /* defined(__sun__) */


    /* Conversion failed */
    if ( errno != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strtof() failed to convert string: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pcStr, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strtof() failed to convert string: '%s', errno: %d, strerror: '%s'.",
                pcStr, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (fNumber);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strtod()

    Purpose:    Convert pcStr to a number.

    Parameters: pcStr       string pointer
                ppcEndStr   end string pointer
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    The converted number, 0 on errorr

*/
double fs_strtod
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char **ppcEndStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    char **ppcEndStr
#endif
)
{

    double  dNumber = 0;


    /* Check the source string */    
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strtod() tried to convert a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Convert the string */
    dNumber = strtod(pcStr, ppcEndStr);


    /* Conversion failed */
    if ( errno != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strtod() failed to convert string: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pcStr, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strtod() failed to convert string: '%s', errno: %d, strerror: '%s'.",
                pcStr, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (dNumber);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcstol()

    Purpose:    Convert pwcStr to a number.

    Parameters: pwcStr          string pointer
                ppwcEndStr      end string pointer
                iBase           base
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    The converted number, 0 on errorr

*/
long fs_wcstol
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    wchar_t **ppwcEndStr,
    int iBase,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr,
    wchar_t **ppwcEndStr,
    int iBase
#endif
)
{

    long    lNumber = 0;


    /* Check the source string */    
    if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcstol() tried to convert a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Convert the string */
    lNumber = wcstol(pwcStr, ppwcEndStr, iBase);


    /* Conversion failed */
    if ( errno != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcstol() failed to convert string: '%ls', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pwcStr, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcstol() failed to convert string: '%ls', errno: %d, strerror: '%s'.",
                pwcStr, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (lNumber);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcstof()

    Purpose:    Convert pwcStr to a number.

    Parameters: pwcStr          string pointer
                ppwcEndStr      end string pointer
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    The converted number, 0 on errorr

*/
float fs_wcstof
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    wchar_t **ppwcEndStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr,
    wchar_t **ppwcEndStr
#endif
)
{

    float   fNumber = 0;


    /* Check the source string */    
    if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcstof() tried to convert a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


/* Solaris does not have wcstof() so we need to use wcstod() */
#if defined(__sun__)
    /* Convert the string */
    fNumber = (float)wcstod(pwcStr, ppwcEndStr);
#else
    /* Convert the string */
    fNumber = wcstof(pwcStr, ppwcEndStr);
#endif    /* defined(__sun__) */


    /* Conversion failed */
    if ( errno != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcstof() failed to convert string: '%ls', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pwcStr, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcstof() failed to convert string: '%ls', errno: %d, strerror: '%s'.",
                pwcStr, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (fNumber);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcstod()

    Purpose:    Convert pwcStr to a number.

    Parameters: pwcStr          string pointer
                ppwcEndStr      end string pointer
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    The converted number, 0 on errorr

*/
double fs_wcstod
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    wchar_t *pwcStr,
    wchar_t **ppwcEndStr,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    wchar_t *pwcStr,
    wchar_t **ppwcEndStr
#endif
)
{

    double  dNumber = 0;


    /* Check the source string */    
    if ( pwcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcstod() tried to convert a NULL string pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Convert the string */
    dNumber = wcstod(pwcStr, ppwcEndStr);


    /* Conversion failed */
    if ( errno != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcstod() failed to convert string: '%ls', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pwcStr, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wcstod() failed to convert string: '%ls', errno: %d, strerror: '%s'.",
                pwcStr, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (dNumber);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_time()

    Purpose:    Get the current time.

    Parameters: ptTime      return pointer for the current time (optional)
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    the current time, -1 otherwise
*/
time_t fs_time
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    time_t *ptTime,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    time_t *ptTime
#endif
)
{

    time_t      tTime = (time_t)0;


    /* Reset the error */
    errno = EOK;


    /* Get the time */
    tTime = time(ptTime);


    /* Check the time */    
    if ( tTime == (time_t)-1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_time() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_time() failed, errno: %d, strerror: '%s'].",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (tTime);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_difftime()

    Purpose:    Returns the difference between two times
                expressed in seconds.

    Parameters: tEndTime        Ending time
                tStartTime      Starting time
                pcFile          __FILE__
                zLine           __LINE__
    
    Globals:    none

    Returns:    the time difference
*/
double fs_difftime
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    time_t tEndTime,
    time_t tStartTime,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    time_t tEndTime,
    time_t tStartTime
#endif
)
{

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    /* Check the start and end times */    
    if ( tStartTime < (time_t)0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_difftime() got a start time of: %lu, file: '%s', line: %lu.", (unsigned long)tStartTime, pcFile, zLine);
    }
    if ( tEndTime < (time_t)0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_difftime() got an end time of: %lu, file: '%s', line: %lu.", (unsigned long)tEndTime, pcFile, zLine);
    }
#endif


    return (difftime(tEndTime, tStartTime));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_utime()

    Purpose:    Set the time on a file.

    Parameters: pcFilePath          file path
                putmTimeBuffer      time buffer (optional)
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_utime
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFilePath,
    struct utimbuf *putmTimeBuffer,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcFilePath,
    struct utimbuf *putmTimeBuffer
#endif
)
{

    int     iStatus = 0;


    if ( bUtlStringsIsStringNULL(pcFilePath) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_utime() tried to set the time on a NULL file path, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Set the time */
    iStatus = utime(pcFilePath, putmTimeBuffer);


    /* Set failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_utime() failed to set the time on file: '%s', errno: %d, strerror: '%s', file: '%s', line: %lu.",
                pcFilePath, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_utime() failed to set the time on file: '%s', errno: %d, strerror: '%s'.",
                pcFilePath, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sleep()

    Purpose:    Sleep for a number of seconds.

    Parameters: iSeconds    number of seconds to sleep
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 or the number of seconds to sleep if interrupted
*/
int fs_sleep
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSeconds,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSeconds
#endif
)
{

    int     iSecondsLeftToSleep = 0;


    /* Check the number of seconds */
    if ( iSeconds < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sleep() was asked to sleep for: %d seconds, file: '%s', line: %lu.",
                iSeconds, pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Sleep */
    iSecondsLeftToSleep = sleep(iSeconds);


    /* Check the number of seconds */
    if ( iSecondsLeftToSleep > 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sleep() failed to sleep for: %d seconds, %d seconds still to sleep, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iSeconds, iSecondsLeftToSleep, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sleep() failed to sleep for: %d seconds, %d seconds still to sleep, errno: %d, strerror: '%s'.",
                iSeconds, iSecondsLeftToSleep, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iSecondsLeftToSleep);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_usleep()

    Purpose:    Sleep for a number of microseconds.

    Parameters: iMicroSeconds   number of microseconds to sleep
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error
*/
int fs_usleep
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iMicroSeconds,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iMicroSeconds
#endif
)
{

    int     iStatus = 0;


    /* Check the number of micro seconds */
    if ( iMicroSeconds < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_usleep() was asked to sleep for: %d microseconds, file: '%s', line: %lu.",
                iMicroSeconds, pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Sleep */
    iStatus = usleep(iMicroSeconds);


    /* Check the number of seconds */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sleep() failed to sleep for: %d microseconds still to sleep, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iMicroSeconds, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sleep() failed to sleep for: %d microseconds still to sleep, errno: %d, strerror: '%s'.",
                iMicroSeconds, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_localtime_r()

    Purpose:    Convert the time into a local tm structure

    Parameters: ptTime      time
                ptmTime     time structure
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
struct tm *fs_localtime_r
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    time_t *ptTime,
    struct tm *ptmTime,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    time_t *ptTime,
    struct tm *ptmTime
#endif
)
{

    struct tm   *ptmResult = NULL;


    /* Check the time */
    if ( ptTime == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_localtime_r() tried to convert a NULL time, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the time */
    if ( *ptTime < (time_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_localtime_r() tried to convert an invalid time: %lu, file: '%s', line: %lu.", (unsigned long)(*ptTime), pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the time structure */
    if ( ptmTime == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_localtime_r() tried to convert a time to a NULL time structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Convert the time */
    ptmResult = localtime_r(ptTime, ptmTime);


    /* Convert failed */
    if ( ptmResult == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_localtime_r() failed to convert the time: %lu, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                (unsigned long)(*ptTime), iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_localtime_r() failed to convert the time: %lu, errno: %d, strerror: '%s'.",
                (unsigned long)(*ptTime), iError, pucBuffer);
#endif

         errno = iError;
    }


    return (ptmResult);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_gmtime_r()

    Purpose:    Convert the time into gmt tm structure

    Parameters: ptTime      time
                ptmTime     time structure
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
struct tm *fs_gmtime_r
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    time_t *ptTime,
    struct tm *ptmTime,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    time_t *ptTime,
    struct tm *ptmTime
#endif
)
{

    struct tm   *ptmResult = NULL;


    /* Check the time */
    if ( ptTime == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_gmtime_r() tried to convert a NULL time, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the time */
    if ( *ptTime < (time_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_gmtime_r() tried to convert an invalid time: %lu, file: '%s', line: %lu.", (unsigned long)(*ptTime), pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the time structure */
    if ( ptmTime == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_gmtime_r() tried to convert a time to a NULL time structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Convert the time */
    ptmResult = gmtime_r(ptTime, ptmTime);


    /* Convert failed */
    if ( ptmResult == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_gmtime_r() failed to convert the time: %lu, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                (unsigned long)(*ptTime), iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_gmtime_r() failed to convert the time: %lu, errno: %d, strerror: '%s'.",
                (unsigned long)(*ptTime), iError, pucBuffer);
#endif

         errno = iError;
    }


    return (ptmResult);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_mktime()

    Purpose:    Convert the tm structure into time

    Parameters: ptmTime     tm structure
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
time_t fs_mktime
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    struct tm *ptmTime,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    struct tm *ptmTime
#endif
)
{

    time_t      tTime = (time_t)0;


    /* Check the tm structure */
    if ( ptmTime == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_mktime() tried to convert a NULL tm structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Convert the time */
    tTime = mktime(ptmTime);


    /* Convert failed */
    if ( tTime == (time_t)-1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mktime() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mktime() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (tTime);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strftime()

    Purpose:    Convert the tm structure into printable time

    Parameters: pcStr       string to write to
                zSize       length of the string
                pcFormat    time format
                ptmTime     tm structure
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
size_t fs_strftime
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    size_t zSize,
    char *pcFormat,
    struct tm *ptmTime,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    size_t zSize,
    char *pcFormat,
    struct tm *ptmTime
#endif
)
{

    size_t      zNewSize = (size_t)0;


    /* Check the string */
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strftime() tried to write to a NULL string, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }

    /* Check the size of the string */
    if ( zSize <= (size_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strftime() tried to write to a string of: %lu bytes, file: '%s', line: %lu.", zSize, pcFile, zLine);
#endif
        return (0);
    }

    /* Check the format */
    if ( pcFormat == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strftime() tried to convert with a NULL format, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }

    /* Check the tm structure */
    if ( ptmTime == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strftime() tried to convert a NULL tm structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (0);
    }


    /* Reset the error */
    errno = EOK;


    /* Convert the time */
    zNewSize = strftime(pcStr, zSize, pcFormat, ptmTime);


    /* Convert failed */
    if ( zNewSize == (size_t)0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strftime() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strftime() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (zNewSize);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strptime()

    Purpose:    Convert a date/time into a tm structure

    Parameters: pcStr       string to write to
                pcFormat    time format
                ptmTime     tm structure
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
char *fs_strptime
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcStr,
    char *pcFormat,
    struct tm *ptmTime,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcStr,
    char *pcFormat,
    struct tm *ptmTime
#endif
)
{

    char    *pcResultPtr = NULL;


    /* Check the string */
    if ( pcStr == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strptime() tried to write to a NULL string, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the format */
    if ( pcFormat == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strptime() tried to convert with a NULL format, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }

    /* Check the tm structure */
    if ( ptmTime == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strptime() tried to convert a NULL tm structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Reset the error */
    errno = EOK;


    /* Parse the date */
    pcResultPtr = strptime(pcStr, pcFormat, ptmTime);


#if defined(NOTUSED)
    /* Parse failed */
    if ( pcResultPtr == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strftime() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_strftime() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }
#endif    /* defined(NOTUSED) */


    return (pcResultPtr);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_gettimeofday()

    Purpose:    Get the time of day

    Parameters: ptvTimeVal      pointer to a timeval structure
                ptzTimeZone     pointer to a timezone structure
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_gettimeofday
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    struct timeval *ptvTimeVal,
    struct timezone *ptzTimeZone,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    struct timeval *ptvTimeVal,
    struct timezone *ptzTimeZone
#endif
)
{

    int     iStatus = 0;


    /* Check the timeval */
    if ( ptvTimeVal == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_gettimeofday() was passed a NULL timeval structure pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Get the time of day */
    iStatus = gettimeofday(ptvTimeVal, ptzTimeZone);


    /* Convert failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_gettimeofday() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_gettimeofday() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_setuid()

    Purpose:    Set the user ID of the current process

    Parameters: zUserID     user ID
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_setuid
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    uid_t zUserID,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    uid_t zUserID
#endif
)
{

    int     iStatus = 0;


    /* Check the user ID */
    if ( zUserID < (uid_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_setuid() tried to set the user ID to: %d, file: '%s', line: %lu.", (int)zUserID, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Set the user ID */
    iStatus = setuid(zUserID);


    /* Set failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setuid() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setuid() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_setgid()

    Purpose:    Set the group id of the current process

    Parameters: zGroupID    group ID
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_setgid
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    gid_t zGroupID,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    gid_t zGroupID
#endif
)
{

    int     iStatus = 0;


    /* Check the group ID */
    if ( zGroupID < (gid_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_setgid() tried to set the group ID to: %d, file: '%s', line: %lu.", (int)zGroupID, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Set the group ID */
    iStatus = setgid(zGroupID);


    /* Set failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setgid() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setgid() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_setpgid()

    Purpose:    Set the process group ID of the specified process

    Parameters: zProcessID          process ID
                zProcessGroupID     process group ID
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_setpgid
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pid_t zProcessID,
    pid_t zProcessGroupID,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pid_t zProcessID,
    pid_t zProcessGroupID
#endif
)
{

    int     iStatus = 0;


    /* Check the process ID */
    if ( zProcessID < (pid_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_setpgid() tried to set the process ID: %d, file: '%s', line: %lu.", (int)zProcessID, pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the group process ID */
    if ( zProcessGroupID < (pid_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_setpgid() tried to set the group process ID: %d, file: '%s', line: %lu.", (int)zProcessGroupID, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Set the process group ID */
    iStatus = setpgid(zProcessID, zProcessGroupID);


    /* Set failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setpgid() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setpgid() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_getpgid()

    Purpose:    Get the process group ID for the specified process

    Parameters: zProcessID      process ID
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_getpgid
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pid_t zProcessID,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pid_t zProcessID
#endif
)
{

    pid_t       zProcessGroupID = 0;


    /* Check the process ID */
    if ( zProcessID < (pid_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_getpgid() tried to get the process group ID for process ID: %d, file: '%s', line: %lu.", (int)zProcessID, pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Get the process group ID */
    zProcessGroupID = getpgid(zProcessID);


    /* Get failed */
    if ( zProcessGroupID == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpgid() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpgid() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (zProcessGroupID);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_getpwnam()

    Purpose:    Get the password structure for a login name.

    Parameters: pcName          login name
                ppwPasswd       buffer for the password structure
                pucBuf          buffer for the password structure fields
                zBufLen         length of the above buffer
                pppwPasswd      return pointer for the password structure
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, error number otherwise
*/
int fs_getpwnam_r
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcName,
    struct passwd *ppwPasswd, 
    char *pucBuf, 
    size_t zBufLen, 
    struct passwd **pppwPasswd,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcName,
    struct passwd *ppwPasswd, 
    char *pucBuf, 
    size_t zBufLen, 
    struct passwd **pppwPasswd
#endif
)
{

    int     iStatus = 0;


    /* Check the name */
    if ( bUtlStringsIsStringNULL(pcName) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwnam_r() tried to get a password structure with a NULL name, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the password structure */
    if ( ppwPasswd == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwnam_r() tried to get a password structure with a NULL password structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the buffer */
    if ( pucBuf == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwnam_r() tried to get a password structure with a NULL buffer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the buffer lenth */
    if ( zBufLen <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwnam_r() tried to get a password structure with an invalid buffer length, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the return password structure */
    if ( pppwPasswd == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwnam_r() tried to get a password structure with a NULL return password structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }
    
    
    /* Reset the error */
    errno = EOK;


    /* Get the password structure */
    iStatus = getpwnam_r(pcName, ppwPasswd, pucBuf, zBufLen, pppwPasswd);


    /* Get failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwnam_r() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwnam_r() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_getpwuid()

    Purpose:    Get the password entry for a user ID

    Parameters: zUserID         user ID
                ppwPasswd       buffer for the password structure
                pucBuf          buffer for the password structure fields
                zBufLen         length of the above buffer
                pppwPasswd      return pointer for the password structure
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, error number otherwise
*/
int fs_getpwuid_r
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    uid_t zUserID,
    struct passwd *ppwPasswd, 
    char *pucBuf, 
    size_t zBufLen, 
    struct passwd **pppwPasswd,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    uid_t zUserID,
    struct passwd *ppwPasswd, 
    char *pucBuf, 
    size_t zBufLen, 
    struct passwd **pppwPasswd
#endif
)
{

    int     iStatus = 0;


    /* Check the uid */
    if ( zUserID < (uid_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwuid_r() tried to get a password structure with an invalid uid: %d, file: '%s', line: %lu.", 
                (int)zUserID, pcFile, zLine);
#endif
        return (-1);
    }


    /* Check the password structure */
    if ( ppwPasswd == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwuid_r() tried to get a password structure with a NULL password structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the buffer */
    if ( pucBuf == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwuid_r() tried to get a password structure with a NULL buffer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the buffer lenth */
    if ( zBufLen <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwuid_r() tried to get a password structure with an invalid buffer length, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }

    /* Check the return password structure */
    if ( pppwPasswd == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwuid_r() tried to get a password structure with a NULL return password structure, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }
    
    
    /* Reset the error */
    errno = EOK;


    /* Get the password structure */
    iStatus = getpwuid_r(zUserID, ppwPasswd, pucBuf, zBufLen, pppwPasswd);


    /* Get failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwuid_r() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getpwuid_r() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_getenv()

    Purpose:    Get the value of an environment variable.

    Parameters: pcName      name of the environment variable
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    a pointer to the environment variable on success, NULL on error
*/
char *fs_getenv
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcName,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcName
#endif
)
{

    /* Check the name */
    if ( bUtlStringsIsStringNULL(pcName) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_getenv() tried to get an environment variable with a NULL name, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (NULL);
    }


    /* Get the environment variable */
    return (getenv(pcName));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_putenv()

    Purpose:    Put the value of an environment variable.

    Parameters: pcString    name/value of the environment variable
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, non-0 otherwise
*/
int fs_putenv
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcString,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcString
#endif
)
{

    /* Check the string */
    if ( bUtlStringsIsStringNULL(pcString) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_putenv() tried to put an environment variable with a NULL string, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Put the environment variable */
    return (putenv(pcString));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_fork()

    Purpose:    Fork off a new process.

    Parameters: pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 to the child process and the pid of the new process to the
                parent, -1 otherwise
*/
pid_t fs_fork
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
#endif
)
{

    int     iStatus = 0;


    /* Reset the error */
    errno = EOK;


    /* Do the fork */
    iStatus = fork();


    /* Fork failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fork() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_fork() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_kill()

    Purpose:    Send a signal to a process.

    Parameters: lPid        process id
                iSignal     signal number
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    -1 on error
*/
int fs_kill
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pid_t zProcessID,
    int iSignal,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pid_t zProcessID,
    int iSignal
#endif
)
{

    int     iStatus = 0;


    /* Check the signal ID */    
    if ( iSignal < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_kill() tried to kill the process ID: %d with signal: %d, file: '%s', line: %lu.",
                (int)zProcessID, iSignal, pcFile, zLine);
#endif
    }


    /* Reset the error */
    errno = EOK;


    /* Do the kill */
    iStatus = kill(zProcessID, iSignal);


    /* Check the result for real signals only */    
    if ( (iStatus == -1) && (iSignal > 0) ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_kill() failed, process ID: %d, signal: %d, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                (int)zProcessID, iSignal, iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_kill() failed, process ID: %d, signal: %d, errno: %d, strerror: '%s'.",
                (int)zProcessID, iSignal, iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wait()

    Purpose:    Wait for a child process.

    Parameters: piStatus    return pointer for the status
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    process ID of the terminated child process, -1 on error

*/
int fs_wait
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int *piStatus,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int *piStatus
#endif
)
{

    int     iStatus = 0;


    /* Check the status return pointer */
    if ( piStatus == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wait() was passed a NULL status return pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Wait */
    iStatus = wait(piStatus);


    /* Wait failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wait() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_wait() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_waitpid()

    Purpose:    Wait for a child process.

    Parameters: zProcessID      process id
                piStatus        return pointer for the status
                iOptions        options
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    -1 on error
*/
int fs_waitpid
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pid_t zProcessID,
    int *piStatus,
    int iOptions,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pid_t zProcessID,
    int *piStatus,
    int iOptions
#endif
)
{

    int     iStatus = 0;


    /* Check the status return pointer */
    if ( piStatus == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_waitpid() was passed a NULL status return pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Wait */
    iStatus = waitpid(zProcessID, piStatus, iOptions);


    /* Wait failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);
            
        /* Dont report the "No child processes" error */
        if ( iError != ECHILD ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_waitpid() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                    iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_waitpid() failed, errno: %d, strerror: '%s'.",
                    iError, pucBuffer);
#endif
        }

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_system()

    Purpose:    Execute a system call.

    Parameters: pcCommand   command to execute
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    -1 on error
*/
int fs_system
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcCommand,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcCommand
#endif
)
{

    int     iStatus = 0;


    /* Check the command */    
    if ( bUtlStringsIsStringNULL(pcCommand) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_system() tried to execute a null command, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }


    /* Reset the error */
    errno = EOK;


    /* System call */
    iStatus = system(pcCommand);


    /* System call failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_system() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_system() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sigaction()

    Purpose:    Installs a signal action

    Parameters: iSignal             signal number
                psaSigAction        new action
                psaSigActionOld     old action
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    -1 on error
*/
int fs_sigaction
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iSignal,
    struct sigaction *psaSigAction,
    struct sigaction *psaSigActionOld,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iSignal,
    struct sigaction *psaSigAction,
    struct sigaction *psaSigActionOld
#endif
)
{

    int     iStatus = 0;


    /* Check the signal */    
    if ( iSignal <= 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_sigaction() tried install an action for an invalid signal, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }


    /* Check the signal action */    
    if ( psaSigAction == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_sigaction() tried install an NULL action, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }


    /* Reset the error */
    errno = EOK;


    /* Install the signal action */
    iStatus = sigaction(iSignal, psaSigAction, psaSigActionOld);


    /* System call failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sigaction() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sigaction() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_setitimer()

    Purpose:    Set the timer

    Parameters: iTimer      which timer to use
                pIValNew    new value
                pIValOld    old value
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_setitimer
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iTimer,
    struct itimerval *pIValNew,
    struct itimerval *pIValOld,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iTimer,
    struct itimerval *pIValNew,
    struct itimerval *pIValOld
#endif
)
{

    int     iStatus = 0;


    /* Check the timer structure pointer */    
    if (pIValNew == NULL) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_setitimer() found NULL pIValNew pointer, file: '%s', line: %lu.", pcFile, zLine);
#endif
    }


    /* Reset the error */
    errno = EOK;


    /* Set the timer */
    iStatus = setitimer(iTimer, pIValNew, pIValOld);


    /* Check the result */    
    if ( iStatus < 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setitimer() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setitimer() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_qsort()

    Purpose:    Sort function

    Parameters: pvBase          base pointer
                zCount          number of elements
                zSize           size of elements
                iCompFunction   compare function
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    void

*/
void fs_qsort
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvBase,
    size_t zCount,
    size_t zSize,
    int (*iCompFunction)(),
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvBase,
    size_t zCount,
    size_t zSize,
    int (*iCompFunction)()
#endif
)
{

    /* Check the base pointer */    
    if ( pvBase == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort a NULL array, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort a NULL array.");
#endif
        return;
    }

    /* Check the number of elements */
    if ( zCount <= (size_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort an array of: %lu elements, file: '%s', line: %lu.", zCount, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort an array of: %lu elements.", zCount);
#endif
        return;
    }

    /* Check the element size */
    if ( zSize <= (size_t)0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort an array of elements of size: %lu, file: '%s', line: %lu.", zSize, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort an array of elements of size: %lu.", zSize);
#endif
        return;
    }

    /* Check comparison function */    
    if ( iCompFunction == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort with a NULL comparison function, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "d_qsort() tried to sort with a NULL comparison function.");
#endif
        return;
    }


    /* Sort */
    qsort(pvBase, zCount, zSize, iCompFunction);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_mmap()

    Purpose:    Map a file into memory

    Parameters: pvAddress   address
                zLen        length
                iProt       prot
                iFlags      flags
                iFile       file descriptor
                zOffset     offset
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    Address on success, MAP_FAILED on error

*/
void *fs_mmap
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvAddress, 
    size_t zLen, 
    int iProt, 
    int iFlags, 
    int iFile, 
    off_t zOffset,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvAddress, 
    size_t zLen, 
    int iProt, 
    int iFlags, 
    int iFile, 
    off_t zOffset
#endif
)
{

    void    *pvMappedAddress = NULL;


    /* Check the length */    
    if ( zLen < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() found a negative length, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() found a negative length.");
#endif
        return (MAP_FAILED);
    }

    /* Check the file descriptor */    
    if ( iFile < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() found a negative file descriptor, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() found a negative file descriptor.");
#endif
        return (MAP_FAILED);
    }

    /* Check the offset */    
    if ( zOffset < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() found a negative offset, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() found a negative offset.");
#endif
        return (MAP_FAILED);
    }


    /* Reset the error */
    errno = EOK;


    /* Map the file */
    pvMappedAddress = mmap(pvAddress, zLen, iProt, iFlags, iFile, zOffset);


    /* Mapping failed */
    if ( pvMappedAddress == MAP_FAILED ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mmap() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pvMappedAddress);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_munmap()

    Purpose:    Unmap a file from memory

    Parameters: pvAddress   address
                zLen        length
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_munmap
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvAddress, 
    size_t zLen, 
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvAddress, 
    size_t zLen
#endif
)
{

    int    iStatus = 0;


    /* Check the address */    
    if ( pvAddress < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munmap() found a NULL address, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munmap() found a NULL address.");
#endif
        return (-1);
    }

    /* Check the length */    
    if ( zLen < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munmap() found a negative length, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munmap() found a negative length.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Unmap the memory */
    iStatus = munmap(pvAddress, zLen);


    /* Unmapping failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munmap() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munmap() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_mlock()

    Purpose:    Lock an area of memory

    Parameters: pvAddress   address
                zLen        length
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_mlock
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvAddress, 
    size_t zLen, 
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvAddress, 
    size_t zLen
#endif
)
{

    int    iStatus = 0;


    /* Check the file descriptor */    
    if ( pvAddress < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlock() found a NULL address, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlock() found a NULL address.");
#endif
        return (-1);
    }

    /* Check the length */    
    if ( zLen < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlock() found a negative length, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlock() found a negative length.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Lock the memory */
    iStatus = mlock(pvAddress, zLen);


    /* Lock failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlock() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlock() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_munlock()

    Purpose:    Unlock an area of memory

    Parameters: pvAddress   address
                zLen        length
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_munlock
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    void *pvAddress, 
    size_t zLen, 
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    void *pvAddress, 
    size_t zLen
#endif
)
{

    int    iStatus = 0;


    /* Check the file descriptor */    
    if ( pvAddress < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlock() found a NULL address, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlock() found a NULL address.");
#endif
        return (-1);
    }

    /* Check the length */    
    if ( zLen < 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlock() found a negative length, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlock() found a negative length.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Unlock the memory */
    iStatus = munlock(pvAddress, zLen);


    /* Unlock failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlock() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlock() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_mlockall()

    Purpose:    Lock down memory

    Parameters: iFlags      flags
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_mlockall
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iFlags, 
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iFlags
#endif
)
{

    int    iStatus = 0;


    /* Reset the error */
    errno = EOK;


    /* Lock down memory */
    iStatus = mlockall(iFlags);


    /* Lock down failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlockall() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_mlockall() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_munlockall()

    Purpose:    Unlock memory

    Parameters: iFlags      flags
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_munlockall
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
#endif
)
{

    int    iStatus = 0;


    /* Reset the error */
    errno = EOK;


    /* Unlock memory */
    iStatus = munlockall();


    /* Unlock failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlockall() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_munlockall() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_shm_open()

    Purpose:    Open shared memory segment

    Parameters: pcName      name
                iFlags      flags
                zMode       mode
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    non-negative integer on success, -1 on error

*/
int fs_shm_open
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcName,
    int iFlags,
    mode_t zMode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcName,
    int iFlags,
    mode_t zMode
#endif
)
{

    int    iStatus = 0;


    /* Check the name */    
    if ( bUtlStringsIsStringNULL(pcName) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_open() found a NULL name, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_open() found a NULL name.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Open the shared memory segment */
    iStatus = shm_open(pcName, iFlags, zMode);


    /* Open failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

        /* Skip the error where the segment is not found */
        if ( errno != ENOENT ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_open() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                    iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_open() failed, errno: %d, strerror: '%s'.",
                    iError, pucBuffer);
#endif
        }

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_shm_unlink()

    Purpose:    Unlink shared memory segment

    Parameters: pcName      name
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_shm_unlink
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcName,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcName
#endif
)
{

    int    iStatus = 0;


    /* Check the name */    
    if ( bUtlStringsIsStringNULL(pcName) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_unlink() found a NULL name, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_unlink() found a NULL name.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Unlink the shared memory segment */
    iStatus = shm_unlink(pcName);


    /* Unlink failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

        /* Skip the error where the segment is not found */
        if ( errno != ENOENT ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_unlink() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                    iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
            iUtlLogError(UTL_LOG_CONTEXT, "fs_shm_unlink() failed, errno: %d, strerror: '%s'.",
                    iError, pucBuffer);
#endif
        }

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sem_create()

    Purpose:    Create a semaphore

    Parameters: pcName      name
                iFlags      flags
                zMode       mode
                iValue      value
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to a semaphore on success, SEM_FAILED on error

*/
sem_t *fs_sem_create
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcName,
    int iFlags,
    mode_t zMode,
    unsigned int iValue,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcName,
    int iFlags,
    mode_t zMode,
    unsigned int iValue
#endif
)
{

    sem_t   *psSemaphore = (sem_t *)SEM_FAILED;


    /* Check the name */    
    if ( bUtlStringsIsStringNULL(pcName) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_create() found a NULL name, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_create() found a NULL name.");
#endif
        return ((sem_t *)SEM_FAILED);
    }

#if defined(SEM_VALUE_MAX)
    /* Check the value */    
    if ( iValue > SEM_VALUE_MAX ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_create() found a value greater than: %d (SEM_VALUE_MAX), file: '%s', line: %lu.", SEM_VALUE_MAX, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_create() found a value greater than: %d (SEM_VALUE_MAX).", SEM_VALUE_MAX);
#endif
        return ((sem_t *)SEM_FAILED);
    }
#endif    /* defined(SEM_VALUE_MAX) */


    /* Reset the error */
    errno = EOK;


    /* Open (create) the semaphore */
    psSemaphore = sem_open(pcName, iFlags, zMode, iValue);


    /* Open (create) failed */
    if ( psSemaphore == (sem_t *)SEM_FAILED ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_create() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_create() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (psSemaphore);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sem_open()

    Purpose:    Open a semaphore

    Parameters: pcName      name
                iFlags      flags
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    pointer to a semaphore on success, SEM_FAILED on error

*/
sem_t *fs_sem_open
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcName,
    int iFlags,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcName,
    int iFlags
#endif
)
{

    sem_t   *psSemaphore = (sem_t *)SEM_FAILED;


    /* Check the name */    
    if ( bUtlStringsIsStringNULL(pcName) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_open() found a NULL name, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_open() found a NULL name.");
#endif
        return ((sem_t *)SEM_FAILED);
    }


    /* Reset the error */
    errno = EOK;


    /* Open the semaphore */
    psSemaphore = sem_open(pcName, iFlags);


    /* Open failed */
    if ( psSemaphore == (sem_t *)SEM_FAILED ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_open() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_open() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (psSemaphore);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sem_wait()

    Purpose:    Wait for a semaphore

    Parameters: psSemaphore     semaphore
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_sem_wait
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    sem_t *psSemaphore,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    sem_t *psSemaphore
#endif
)
{

    int    iStatus = 0;


    /* Check the semaphore */    
    if ( psSemaphore == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_wait() found a NULL semaphore, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_wait() found a NULL semaphore.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Wait for the semaphore */
    iStatus = sem_wait(psSemaphore);


    /* Wait failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_wait() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_wait() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sem_post()

    Purpose:    Post a semaphore

    Parameters: psSemaphore     semaphore
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_sem_post
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    sem_t *psSemaphore,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    sem_t *psSemaphore
#endif
)
{

    int    iStatus = 0;


    /* Check the semaphore */    
    if ( psSemaphore == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_post() found a NULL semaphore, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_post() found a NULL semaphore.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Post the semaphore */
    iStatus = sem_post(psSemaphore);


    /* Post failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_post() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_post() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sem_getvalue()

    Purpose:    Get the value of a semaphore

    Parameters: psSemaphore     semaphore
                piValue         value return pointer
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_sem_getvalue
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    sem_t *psSemaphore,
    int *piValue,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    sem_t *psSemaphore,
    int *piValue
#endif
)
{

    int    iStatus = 0;


    /* Check the semaphore */    
    if ( psSemaphore == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_getvalue() found a NULL semaphore, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_getvalue() found a NULL semaphore.");
#endif
        return (-1);
    }

    /* Check the return pointer for the value */    
    if ( piValue == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_getvalue() found a NULL value pointer, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_getvalue() found a NULL value pointer.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Get the value from the semaphore */
    iStatus = sem_getvalue(psSemaphore, piValue);


    /* Get value failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_getvalue() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_getvalue() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sem_close()

    Purpose:    Close a semaphore

    Parameters: psSemaphore     semaphore
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_sem_close
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    sem_t *psSemaphore,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    sem_t *psSemaphore
#endif
)
{

    int    iStatus = 0;


    /* Check the semaphore */    
    if ( psSemaphore == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_close() found a NULL semaphore, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_close() found a NULL semaphore.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Close the semaphore */
    iStatus = sem_close(psSemaphore);


    /* Close failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_close() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_close() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_sem_unlink()

    Purpose:    Unlink a semaphore

    Parameters: pcName      name
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_sem_unlink
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcName,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcName
#endif
)
{

    int    iStatus = 0;


    /* Check the name */    
    if ( bUtlStringsIsStringNULL(pcName) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_unlink() found a NULL name, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_unlink() found a NULL name.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Unlink the semaphore */
    iStatus = sem_unlink(pcName);


    /* Unlink failed */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_unlink() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_sem_unlink() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_regcomp()

    Purpose:    Compile a regular expression

    Parameters: prRegex     pointer a regex allocation
                pcRegex     pointer a regex string
                iFlags      flags
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_regcomp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    regex_t *prRegex,
    char *pcRegex,
    int iFlags,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    regex_t *prRegex,
    char *pcRegex,
    int iFlags
#endif
)
{

    int    iError = 0;


    /* Check the regex allocation */    
    if ( prRegex == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regcomp() found a NULL regex allocation, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regcomp() found a NULL regex allocation.");
#endif
        return (-1);
    }

    /* Check the regex string */    
    if ( bUtlStringsIsStringNULL(pcRegex) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regcomp() found a NULL regex string, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regcomp() found a NULL regex string.");
#endif
        return (-1);
    }


    /* Compile the regex */
    iError = regcomp(prRegex, pcRegex, iFlags);


    /* Compilation failed */
    if ( iError != 0 ) {

         unsigned char    pucErrorMessage[256] = {'\0'};

        /* Get the error string */
        regerror(iError, prRegex, pucErrorMessage, 256);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regcomp() failed, regex: '%s', error: %d, message: '%s', file: '%s', line: %lu.",
                pcRegex, iError, pucErrorMessage, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regcomp() failed, regex: '%s, error: %d, message: '%s'.",
                pcRegex, iError, pucErrorMessage);
#endif
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_regexec()

    Purpose:    Execute a regular expression

    Parameters: prRegex         pointer a regex allocation
                pcString        pointer a string
                zNMatch
                prmRMatch
                iFlags          flags
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, REG_NOMATCH on error

*/
int fs_regexec
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    regex_t *prRegex,
    char *pcString,
    size_t zNMatch, 
    regmatch_t prmRMatch[],
    int iFlags,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    regex_t *prRegex,
    char *pcString,
    size_t zNMatch, 
    regmatch_t prmRMatch[],
    int iFlags
#endif
)
{

    int    iError = 0;


    /* Check the regex allocation */    
    if ( prRegex == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regexec() found a NULL regex allocation, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regexec() found a NULL regex allocation.");
#endif
        return (-1);
    }

    /* Check the string */    
    if ( bUtlStringsIsStringNULL(pcString) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regexec() found a NULL string, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regexec() found a NULL string.");
#endif
        return (-1);
    }


    /* Execute the regex */
    iError = regexec(prRegex, pcString, zNMatch, prmRMatch, iFlags);


    /* Execution failed */
    if ( (iError != 0) && (iError != REG_NOMATCH) ) {

         unsigned char    pucErrorMessage[256] = {'\0'};

        /* Get the error string */
        regerror(iError, prRegex, pucErrorMessage, 256);


#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regexec() failed, string: '%s', error: %d, message: '%s', file: '%s', line: %lu.",
                pcString, iError, pucErrorMessage, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regexec() failed, pcString: '%s', error: %d, message: '%s'.",
                pcString, iError, pucErrorMessage);
#endif
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/

#if defined(TRE_REGEX_ENABLE)

/*

    Function:   fs_regwcomp()

    Purpose:    Compile a regular expression

    Parameters: prRegex     pointer a regex allocation
                pwcRegex    pointer a regex wide character string
                iFlags      flags
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_regwcomp
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    regex_t *prRegex,
    wchar_t *pwcRegex,
    int iFlags,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    regex_t *prRegex,
    wchar_t *pwcRegex,
    int iFlags
#endif
)
{

    int    iError = 0;


    /* Check the regex allocation */    
    if ( prRegex == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwcomp() found a NULL regex allocation, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwcomp() found a NULL regex allocation.");
#endif
        return (-1);
    }

    /* Check the regex string */    
    if ( bUtlStringsIsWideStringNULL(pwcRegex) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwcomp() found a NULL regex string, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwcomp() found a NULL regex string.");
#endif
        return (-1);
    }


    /* Compile the regex */
    iError = regwcomp(prRegex, pwcRegex, iFlags);


    /* Compilation failed */
    if ( iError != 0 ) {

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwcomp() failed, regex: '%s', error: %dfile: '%s', line: %lu.",
                pcRegex, iError, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwcomp() failed, regex: '%s, error: %d.",
                pcRegex, iError);
#endif
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_regwexec()

    Purpose:    Execute a regular expression

    Parameters: prRegex         pointer a regex allocation
                pwcString       pointer a string of wide characters
                zNMatch
                prmRMatch
                iFlags          flags
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, REG_NOMATCH on error

*/
int fs_regwexec
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    regex_t *prRegex,
    wchar_t *pwcString,
    size_t zNMatch, 
    regmatch_t prmRMatch[],
    int iFlags,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    regex_t *prRegex,
    wchar_t *pwcString,
    size_t zNMatch, 
    regmatch_t prmRMatch[],
    int iFlags
#endif
)
{

    int    iError = 0;


    /* Check the regex allocation */    
    if ( prRegex == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwexec() found a NULL regex allocation, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwexec() found a NULL regex allocation.");
#endif
        return (-1);
    }

    /* Check the string */    
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwexec() found a NULL string, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwexec() found a NULL string.");
#endif
        return (-1);
    }


    /* Execute the regex */
    iError = regwexec(prRegex, pwcString, zNMatch, prmRMatch, iFlags);


    /* Execution failed */
    if ( (iError != 0) && (iError != REG_NOMATCH) ) {

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwexec() failed, string: '%ls', error: %d, file: '%s', line: %lu.",
                pwcString, iError, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regwexec() failed, pcString: '%ls', error: %d.",
                pwcString, iError);
#endif
    }


    return (iError);

}

#endif    /* TRE_REGEX_ENABLE */

/*---------------------------------------------------------------------------*/


/*

    Function:   fs_regfree()

    Purpose:    Free a regular expression

    Parameters: prRegex     pointer a regex allocation
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    void

*/
void fs_regfree
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    regex_t *prRegex,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    regex_t *prRegex
#endif
)
{

    /* Check the regex allocation */    
    if ( prRegex == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regfree() found a NULL regex allocation, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_regfree() found a NULL regex allocation.");
#endif
        return;
    }


#if defined(TRE_REGEX_ENABLE)
    /* Free the regex */
    s_free(prRegex);
#else
    /* Free the regex */
    regfree(prRegex);
#endif    /* TRE_REGEX_ENABLE */


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_pthread_create()

    Purpose:    Create a thread

    Parameters: ptThread            thread
                pThreadAttributes   thread attributes
                pvFunction          function
                pvParameter         function parameter
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_pthread_create
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pthread_t *ptThread, 
    pthread_attr_t *pThreadAttributes, 
    void *(*pvFunction)(void *), 
    void *pvParameter,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pthread_t *ptThread, 
    pthread_attr_t *pThreadAttributes, 
    void *(*pvFunction)(void *), 
    void *pvParameter
#endif
)
{

    int    iStatus = 0;


    /* Check the thread */    
    if ( ptThread == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_create() found a NULL thread, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_create() found a NULL thread.");
#endif
        return (-1);
    }

    /* Check the function */    
    if ( pvFunction == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_create() found a NULL function, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_create() found a NULL function.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Create the thread */
    iStatus = pthread_create(ptThread, pThreadAttributes, pvFunction, pvParameter);


    /* Create failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_create() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_create() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_pthread_join()

    Purpose:    Join a thread

    Parameters: tThread             thread
                ppvReturnValue      return value
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_pthread_join
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pthread_t tThread, 
    void **ppvReturnValue, 
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pthread_t tThread,
    void **ppvReturnValue 
#endif
)
{

    int            iStatus = 0;
    pthread_t       tThreadLocal;

    
    /* Create a blank pthread structure to compare against */
    s_memset(&tThreadLocal, '\0', sizeof(pthread_t));
                
    /* Check for an empty thread structure */
    if ( s_memcmp(&tThread, &tThreadLocal, sizeof(pthread_t)) == 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_join() found a NULL thread, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
/*         iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_join() found a NULL thread."); */
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Join the thread */
    iStatus = pthread_join(tThread, ppvReturnValue);


    /* Join failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_join() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_join() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}



/*---------------------------------------------------------------------------*/


/*

    Function:   fs_pthread_detach()

    Purpose:    Detach a thread

    Parameters: tThread     thread
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_pthread_detach
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pthread_t tThread, 
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pthread_t tThread
#endif
)
{

    int         iStatus = 0;
    pthread_t   tThreadLocal;

    
    /* Create a blank pthread structure to compare against */
    s_memset(&tThreadLocal, '\0', sizeof(pthread_t));
                
    /* Check for an empty thread structure */
    if ( s_memcmp(&tThread, &tThreadLocal, sizeof(pthread_t)) == 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_detach() found a NULL thread, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_detach() found a NULL thread.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Detach the thread */
    iStatus = pthread_detach(tThread);


    /* Detach failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_detach() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_detach() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_pthread_cancel()

    Purpose:    Cancel a thread

    Parameters: tThread     thread
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_pthread_cancel
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pthread_t tThread, 
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pthread_t tThread
#endif
)
{

    int            iStatus = 0;
    pthread_t       tThreadLocal;

    
    /* Create a blank pthread structure to compare against */
    s_memset(&tThreadLocal, '\0', sizeof(pthread_t));
                
    /* Check for an empty thread structure */
    if ( s_memcmp(&tThread, &tThreadLocal, sizeof(pthread_t)) == 0 ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_cancel() found a NULL thread, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_cancel() found a NULL thread.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Cancel the thread */
    iStatus = pthread_cancel(tThread);


    /* Cancel failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_cancel() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_cancel() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_pthread_mutex_lock()

    Purpose:    Lock a mutex

    Parameters: ptmThreadMutex      thread mutex
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_pthread_mutex_lock
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pthread_mutex_t *ptmThreadMutex,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pthread_mutex_t *ptmThreadMutex
#endif
)
{

    int    iStatus = 0;


    /* Check the mutex */    
    if ( ptmThreadMutex == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_lock() found a NULL mutex, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_lock() found a NULL mutex.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Lock the mutex */
    iStatus = pthread_mutex_lock(ptmThreadMutex);


    /* Lock failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_lock() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_lock() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_pthread_mutex_unlock()

    Purpose:    Unlock a mutex

    Parameters: ptmThreadMutex      thread mutex
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    0 on success, non-0 on error

*/
int fs_pthread_mutex_unlock
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    pthread_mutex_t *ptmThreadMutex,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    pthread_mutex_t *ptmThreadMutex
#endif
)
{

    int    iStatus = 0;


    /* Check the mutex */    
    if ( ptmThreadMutex == NULL ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_unlock() found a NULL mutex, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_unlock() found a NULL mutex.");
#endif
        return (-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Unlock the mutex */
    iStatus = pthread_mutex_unlock(ptmThreadMutex);


    /* Unlock failed */
    if ( iStatus != 0 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_unlock() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_pthread_mutex_unlock() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_setlocale()

    Purpose:    Set the local

    Parameters: iCategory   category
                pcLocale    locale
                pcFile      __FILE__
                zLine       __LINE__

    Globals:    none

    Returns:    a pointer to a string on success, null on error

*/
char *fs_setlocale
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    int iCategory,
    char *pcLocale,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    int iCategory,
    char *pcLocale
#endif
)
{

    char    *pcString = NULL;


    /* Reset the error */
    errno = EOK;


    /* Set the local */
    pcString = setlocale(iCategory, pcLocale);


    /* Failed to set the locale */
    if ( pcString == NULL ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setlocale() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_setlocale() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (pcString);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   fs_iconv_open()

    Purpose:    Open a conversion descriptor

    Parameters: pcToCode        to code
                pcFromCode      from code
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    a conversion descriptor on success, -1 on error

*/
iconv_t fs_iconv_open
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    char *pcToCode,
    char *pcFromCode,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    char *pcToCode,
    char *pcFromCode
#endif
)
{

    iconv_t     zDescriptor = 0;


    /* Check the to code string */    
    if ( bUtlStringsIsStringNULL(pcToCode) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_open() found a NULL 'to code' string, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_open() found a NULL 'to code' string.");
#endif
        return ((iconv_t)-1);
    }

    /* Check the from code string */    
    if ( bUtlStringsIsStringNULL(pcToCode) == true ) {
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_open() found a NULL 'from code' string, file: '%s', line: %lu.", pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_open() found a NULL 'from code' string.");
#endif
        return ((iconv_t)-1);
    }


    /* Reset the error */
    errno = EOK;


    /* Open the conversion descriptor */
    zDescriptor = iconv_open(pcToCode, pcFromCode);


    /* Failed to open the conversion descriptor */
    if ( zDescriptor == (iconv_t)-1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_open() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_open() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (zDescriptor);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_iconv()

    Purpose:    Convert text

    Parameters: zDescriptor         conversion descriptor
                ppcSource           source text
                pzSourceLen         source text length
                ppcDestination      destination text
                pzDestinationLen    destination text length
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    number of characters converted on success, -1 on error

*/
size_t fs_iconv
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    iconv_t zDescriptor,
    char **ppcSource,
    size_t *pzSourceLen,
    char **ppcDestination,
    size_t *pzDestinationLen,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    iconv_t zDescriptor,
    char **ppcSource,
    size_t *pzSourceLen,
    char **ppcDestination,
    size_t *pzDestinationLen
#endif
)
{

    size_t      zStatus = 0;


    /* Reset the error */
    errno = EOK;


    /* Convert text */
    zStatus = iconv(zDescriptor, ppcSource, pzSourceLen, ppcDestination, pzDestinationLen);


    /* Failed to convert the text */
    if ( zStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
/*         iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv() failed, errno: %d, strerror: '%s'.", */
/*                 iError, pucBuffer); */
#endif

         errno = iError;
    }


    return (zStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_iconv_close()

    Purpose:    Close a conversion descriptor

    Parameters: zDescriptor     conversion descriptor
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int fs_iconv_close
(
#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
    iconv_t zDescriptor,
    char *pcFile,
    size_t zLine
#elif defined(UTL_CWRAPPERS_ENABLE)
    iconv_t zDescriptor
#endif
)
{

    int        iStatus = 0;


    /* Reset the error */
    errno = EOK;


    /* Close the conversion descriptor */
    iStatus = iconv_close(zDescriptor);


    /* Failed to close the conversion descriptor */
    if ( iStatus == -1 ) {

        int             iError = errno;
        unsigned char   pucBuffer[UTL_CWRAPPERS_ERROR_STRING_LEN + 1] = {'\0'};

        /* Get the error string */
        strerror_r(iError, pucBuffer, UTL_CWRAPPERS_ERROR_STRING_LEN);

#if defined(UTL_CWRAPPERS_ENABLE_DEBUG)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_close() failed, errno: %d, strerror: '%s', file: '%s', line: %lu.",
                iError, pucBuffer, pcFile, zLine);
#elif defined(UTL_CWRAPPERS_ENABLE)
        iUtlLogError(UTL_LOG_CONTEXT, "fs_iconv_close() failed, errno: %d, strerror: '%s'.",
                iError, pucBuffer);
#endif

         errno = iError;
    }


    return (iStatus);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


#endif /* defined(UTL_CWRAPPERS_ENABLE) || defined(UTL_CWRAPPERS_ENABLE_DEBUG) */


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
