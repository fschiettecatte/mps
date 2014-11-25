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

    Module:     date.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 April 2004

    Purpose:    This is the header file for date.c. This file contains a
                lot of useful date functions.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_DATE_H)
#define UTL_DATE_H


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
** Macros
*/

/* Macro to get the diff between two timeval structures */
#define UTL_DATE_DIFF_TIMEVAL(tvStartTimeVal, tvEndTimeVal, tvDiffTimeVal) \
{   \
    tvDiffTimeVal.tv_sec = tvEndTimeVal.tv_sec - tvStartTimeVal.tv_sec; \
    tvDiffTimeVal.tv_usec = tvEndTimeVal.tv_usec - tvStartTimeVal.tv_usec; \
    if ( tvDiffTimeVal.tv_usec < 0 ) {  \
        tvDiffTimeVal.tv_sec--; \
        tvDiffTimeVal.tv_usec += 1000000; \
    }   \
}


/* Macro in add two time val structures together */
#define UTL_DATE_ADD_TIMEVAL(tvTimeVal1, tvTimeVal2, tvDestTimeVal) \
{   \
    tvDestTimeVal.tv_sec = tvTimeVal1.tv_sec + tvTimeVal2.tv_sec; \
    tvDestTimeVal.tv_usec = tvTimeVal1.tv_usec + tvTimeVal2.tv_usec; \
    if ( tvDestTimeVal.tv_usec >= 1000000 ) {   \
        tvDestTimeVal.tv_sec += 1; \
        tvDestTimeVal.tv_usec -= 1000000; \
    }   \
}


/* Macro to convert a timeval structure to seconds */
#define UTL_DATE_TIMEVAL_TO_SECONDS(tvTimeVal, dDouble) \
{   \
    dDouble = (double)tvTimeVal.tv_sec + ((double)tvTimeVal.tv_usec / 1000000); \
}


/* Macro to convert a timeval structure to milliseconds */
#define UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvTimeVal, dDouble) \
{   \
    dDouble = ((double)tvTimeVal.tv_sec * 1000) + ((double)tvTimeVal.tv_usec / 1000); \
}


/* Macro to convert a timeval structure to microseconds */
#define UTL_DATE_TIMEVAL_TO_MICROSECONDS(tvTimeVal, dDouble) \
{   \
    dDouble = ((double)tvTimeVal.tv_sec * 1000000) + (double)tvTimeVal.tv_usec; \
}


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

/* Functions to get an ansi date from a date string */
int iUtlDateParseDateToAnsiDate (unsigned char *pucDate, unsigned long *pulAnsiDate);
int iUtlDateParseWideDateToAnsiDate (wchar_t *pwcDate, unsigned long *pulAnsiDate);


/* Functions to validate/normalize ansi dates */
int iUtlDateValidateAnsiDate (unsigned char *pucAnsiDate, unsigned long *pulAnsiDate);
int iUtlDateValidateWideAnsiDate (wchar_t *pwcAnsiDate, unsigned long *pulAnsiDate);


/* Function to get a web date from an ansi date */
int iUtlDateGetWebDateFromAnsiDate (unsigned long ulAnsiDate, unsigned char *pucDate, unsigned int uiDateLength);

/* Function to get a web date from a time */
int iUtlDateGetWebDateFromTime (time_t tTime, unsigned char *pucDate, unsigned int uiDateLength);


/* Function to get a zulu date from an ansi date */
int iUtlDateGetZuluDateFromAnsiDate (unsigned long ulAnsiDate, unsigned char *pucDate, unsigned int uiDateLength);

/* Function to get a zulu date from a time */
int iUtlDateGetZuluDateFromTime (time_t tTime, unsigned char *pucDate, unsigned int uiDateLength);


/* Function to get the time from an ansi date */
int iUtlDateGetTimeFromAnsiDate (unsigned long ulAnsiDate, time_t *ptTime);

/* Function to get the ansi date from a time */
int iUtlDateGetAnsiDateFromTime (time_t tTime, unsigned long *pulAnsiDate);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_DATE_H) */


/*---------------------------------------------------------------------------*/
