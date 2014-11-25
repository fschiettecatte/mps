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

    Module:     num.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    6 February 2006

    Purpose:    Number support functions

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.num"


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNumRoundNumber()

    Purpose:    Return a copy of pwcStr.

    Parameters: llNumber        the number to round
                uiThreshold     the number of digits under which we don't round
                uiRounding      the minimum number of digits to round off
                uiDigits        the maximum number of digits to keep

    Globals:    none

    Returns:    UTL error code
*/
int iUtlNumRoundNumber
(
    long llNumber,
    unsigned int uiThreshold,
    unsigned int uiRounding,
    unsigned int uiDigits,
    long *pllNumber
)
{

    unsigned char   pucNumber[255] = {'\0'};
    unsigned int    uiTextLength = 0;
    
    
    /* Check the parameters */
    if ( pllNumber == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pllNumber' parameter passed to 'iUtlNumRoundNumber'."); 
        return (UTL_ReturnParameterError);
    }


    /* Print the number to a string */
    snprintf(pucNumber, 255, "%ld", llNumber);
    

    /* Get the length of the string, note that we need to reduce that by one if the number is negative */
    uiTextLength = s_strlen(pucNumber) + ((llNumber < 0) ? -1 : 0);
    
    
    /* Round off if we reach the threshold */
    if ( uiTextLength >= uiThreshold ) {

        /* Adjust the length based on the number of digits we want to keep and what we want to round off */
        uiTextLength -= (uiTextLength > uiDigits) ? uiDigits : (uiTextLength - uiRounding);

        /* Round off the number, setting the return paramter from it */
        *pllNumber = ((float)llNumber / pow(10, uiTextLength)) * pow(10, uiTextLength);
    }



    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
