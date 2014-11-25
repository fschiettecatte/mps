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

    Module:     rand.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 2005

    Purpose:    This file contains functions for random number generation.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.rand"


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlRandSetSeed()

    Purpose:    This sets the random number generator seed

    Parameters: uiRandSeed      seed

    Globals:    none

    Returns:    UTL error code

*/
int iUtlRandSetSeed
(
    unsigned int uiRandSeed
)
{

    /* Set the seed */
    srand48(uiRandSeed);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlRandGetRand()

    Purpose:    This returns a random number in the range 0 to uiRandMax

    Parameters: uiRandMax   maximum number to return
                puiRand     return pointer for the random number

    Globals:    none

    Returns:    UTL error code

*/
int iUtlRandGetRand
(
    unsigned int uiRandMax,
    unsigned int *puiRand
)
{

    unsigned int    uiRand = 0;
    
    
    /* Check the parameters */
    if ( puiRand == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiRand' parameter passed to 'iUtlRandGetRand'."); 
        return (UTL_ReturnParameterError);
    }


    /* Note that we set the ceiling to uiRandMax + 1 because we need to
    ** hit it, if we set it to uiRandMax, we would only ever hit it if 
    ** drand48() returned 1, an unlikely occurrence, this also means 
    ** that we need to adjust for it later in case we do hit uiRandMax + 1
    */ 
    uiRand = ((double)(uiRandMax + 1) * drand48());
    
    /* And here we adjust in case we do hit uiRandMax + 1 */
    uiRand = UTL_MACROS_MIN(uiRand, uiRandMax);

    
    ASSERT((uiRand >= 0) || (uiRand <= uiRandMax));


    /* Set the return pointer */
    *puiRand = uiRand;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
