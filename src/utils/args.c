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

    Module:     args.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 2005

    Purpose:    This file contains functions to peek at and get the C args.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.args"


/*---------------------------------------------------------------------------*/


/*

    Function:   pucUtlArgsPeekNextArg()

    Purpose:    Returns the next argument without popping it.
                Returns NULL when it is out of arguments. 

    Parameters: piArgc      pointer to the argument index
                pppcArgv    pointer to the argument list

    Globals:    none

    Returns:    A pointer to the next argument, NULL on error or if there are no more.

*/
unsigned char *pucUtlArgsPeekNextArg
(
    int *piArgc,
    char ***pppcArgv
)
{

    /* Check the parameters */
    if ( piArgc == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piArgc' parameter passed to 'pucUtlArgsPeekNextArg'."); 
        return (NULL);
    }

    if ( pppcArgv == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pppcArgv' parameter passed to 'pucUtlArgsPeekNextArg'."); 
        return (NULL);
    }


    /* Return a pointer to the parameter */
    if ( (*piArgc) > 0 ) {
        return ((unsigned char *)**pppcArgv);
    }


    return (NULL);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucUtlArgsGetNextArg()

    Purpose:    Returns the next argument. Returns NULL when it is out of arguments
                This side effects both piArgc and pppcArgv, piArgc always contains the number
                of arguments left. The first returned is the command name. 

    Parameters: piArgc      pointer to the argument index
                pppcArgv    pointer to the argument list

    Globals:    none

    Returns:    A pointer to the next argument, NULL on error or if there are no more.

*/
unsigned char *pucUtlArgsGetNextArg
(
    int *piArgc,
    char ***pppcArgv
)
{

    /* Check the parameters */
    if ( piArgc == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piArgc' parameter passed to 'pucUtlArgsGetNextArg'."); 
        return (NULL);
    }

    if ( pppcArgv == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pppcArgv' parameter passed to 'pucUtlArgsGetNextArg'."); 
        return (NULL);
    }


    /* Return a pointer to the parameter, and decrement piArgc, and increment pppcArgv */
    if ( (*piArgc)-- > 0 ) {
        return ((unsigned char *)*((*pppcArgv)++));
    }


    return (NULL);

}


/*---------------------------------------------------------------------------*/
