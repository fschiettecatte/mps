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

    Module:     load.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 2005

    Purpose:    This file contains a functions to get the current machine load.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.load"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Load average - Solaris  */
#if defined(__sun__)
#include <sys/loadavg.h>

#define UTL_LOAD_GETLOADAVG_AVAILABLE

#define UTL_LOAD_LOADAVG_NSTATS             LOADAVG_NSTATS
#define UTL_LOAD_LOADAVG_1MIN               LOADAVG_1MIN
#define UTL_LOAD_LOADAVG_5MIN               LOADAVG_5MIN
#define UTL_LOAD_LOADAVG_15MIN              LOADAVG_15MIN

#endif    /* defined(__sun__) */


/* Load average - Linux */
#if defined(linux)

#define UTL_LOAD_GETLOADAVG_AVAILABLE

#define UTL_LOAD_LOADAVG_NSTATS             (3)
#define UTL_LOAD_LOADAVG_1MIN               (0)
#define UTL_LOAD_LOADAVG_5MIN               (1)
#define UTL_LOAD_LOADAVG_15MIN              (2)

#endif    /* defined(linux) */


/* Load average - MacOS X */
#if defined(__APPLE__) && defined(__MACH__)

#define UTL_LOAD_GETLOADAVG_AVAILABLE

#define UTL_LOAD_LOADAVG_NSTATS             (3)
#define UTL_LOAD_LOADAVG_1MIN               (0)
#define UTL_LOAD_LOADAVG_5MIN               (1)
#define UTL_LOAD_LOADAVG_15MIN              (2)

#endif    /* defined(__APPLE__) && defined(__MACH__) */


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLoadGetAverages()

    Purpose:    Return the load averages for the last 1, 5 and 15 minutes.
                The return pointer will be set to -1 if the value was not
                available.

    Parameters: pd1Minute       return pointer for the load average for the last 1 minute (optional)
                pd5Minute       return pointer for the load average for the last 5 minutes (optional)
                pd15Minute      return pointer for the load average for the last 15 minutes (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlLoadGetAverages
(
    double *pd1Minute,
    double *pd5Minute,
    double *pd15Minute
)
{

#if defined(UTL_LOAD_GETLOADAVG_AVAILABLE)

    /* Array to hold the load average values */
    double      pdLoadAvg[UTL_LOAD_LOADAVG_NSTATS] = {0, 0, 0};
    
        
    /* Get the load average, request all the stats, 
    ** this returns the number of samples or -1 on error
    */
    int iSamples = getloadavg(pdLoadAvg, UTL_LOAD_LOADAVG_NSTATS);
    

    /* Check for an error */
    if ( iSamples == - 1 ) {
        return (UTL_LoadFailedGetAverages);
    }

    
    /* Preset the return parameters */
    if ( pd1Minute != NULL ) {
        *pd1Minute = -1;
    }

    if ( pd5Minute != NULL ) {
        *pd5Minute = -1;
    }

    if ( pd15Minute != NULL ) {
        *pd15Minute = -1;
    }


    /* Get the 1 minute load average if available */
    if ( (pd1Minute != NULL) && (iSamples > UTL_LOAD_LOADAVG_1MIN) && (iSamples <= UTL_LOAD_LOADAVG_NSTATS) ) {
        *pd1Minute = pdLoadAvg[UTL_LOAD_LOADAVG_1MIN];
    }
    
    /* Get the 5 minute load average if available */
    if ( (pd5Minute != NULL) && (iSamples > UTL_LOAD_LOADAVG_5MIN) && (iSamples <= UTL_LOAD_LOADAVG_NSTATS) ) {
        *pd5Minute = pdLoadAvg[UTL_LOAD_LOADAVG_5MIN];
    }
    
    /* Get the 15 minute load average if available */
    if ( (pd15Minute != NULL) && (iSamples > UTL_LOAD_LOADAVG_15MIN) && (iSamples <= UTL_LOAD_LOADAVG_NSTATS) ) {
        *pd15Minute = pdLoadAvg[UTL_LOAD_LOADAVG_15MIN];
    }
    
    /* Success */
    return (UTL_NoError);

#else/* defined(UTL_LOAD_GETLOADAVG_AVAILABLE) */

    /* Failure since it is not supported */
    return (UTL_LoadUnsupportedGetAverages);

#endif    /* defined(UTL_LOAD_GETLOADAVG_AVAILABLE) */

    
}


/*---------------------------------------------------------------------------*/
