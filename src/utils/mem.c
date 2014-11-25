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

    Module:     mem.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    19 June 2007

    Purpose:    This file contain mem* style function.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.mem"


/*---------------------------------------------------------------------------*/

/*

    Function:   fs_memdup()

    Purpose:    Duplicate a fixed number of bytes 

    Parameters: pvSource        The data to duplicate 
                zSourceLength   The length of data to duplicate
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    A pointer to duplicated data
*/
void *fs_memdup
(
#if defined(DEBUG_MEM)
    void *pvSource, 
    size_t zSourceLength,
    char *pcFile,
    size_t zLine
#else
    void *pvSource, 
    size_t zSourceLength
#endif    /* defined(DEBUG_MEM) */
)
{

    void    *pvDestination = NULL;


#if defined(DEBUG_MEM)
    if ( pvSource == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_memdup found a NULL string pointer (pucDestination), file: '%s', line: %lu", pcFile, zLine);
    }

    if ( zSourceLength < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_memdup found a zSourceLength length of: %ld, file: '%s', line: %lu", zSourceLength, pcFile, zLine);
    }
#endif    /* defined(DEBUG_MEM) */


    /* Allocate space for the duplicate */
    if ( (pvDestination = (void *)s_malloc(zSourceLength)) == NULL ) {
        return (NULL);
    }

    /* Duplicate the data */
    if ( zSourceLength > 0 ) {
        s_memcpy(pvDestination, pvSource, zSourceLength);
    }


    return (pvDestination);

}


/*---------------------------------------------------------------------------*/
