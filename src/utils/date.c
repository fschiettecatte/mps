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

    Module:     date.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 April 2004

    Purpose:    This file contains a lot of useful date functions.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.date"


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateParseDateToAnsiDate()

    Purpose:    This functions tries to parse a date out of the line that 
                is passed and returns it as an ansi date. For example 
                February 20th, 1994 is represented as 19940220 (YYYYMMDD). 

                Formats recognised:
                    yyyy-mm-ddThh:mm:ssZ            (zulu date)
                    yyyy-mm-dd hh:mm:ss             (zulu date without markers)
                    ddd, dd mmm yyyy hh:mm:ss       (web date)
                    dd mmm yyyy
                    mmm yyyy
                    yyyy mmm dd
                    yyyy mmm
                    mm/dd/yyyy
                    dd/mm/yyyy
                    mm/yyyy
                    dd-mm-yyyy
                    yyyy-mm-dd
                    yyyy-mm
                    yyyy season
                    yyyy

    Parameters: pucDate         a string containing the date
                pulAnsiDate     a return pointer for the ansi date

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateParseDateToAnsiDate
(
    unsigned char *pucDate,
    unsigned long *pulAnsiDate
)
{
    
    struct tm       tmTime;
    unsigned char   *pucDatePtr = NULL;
    unsigned char   *pucParsePtr = NULL;
    
    boolean         bYear = false;
    boolean         bMonth = false;
    boolean         bDay = false;
    boolean         bTime = false;

    unsigned char   pucAnsiDate[15] = {'\0'};


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucDate) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDate' parameter passed to 'iUtlDateParseDateToAnsiDate'."); 
        return (UTL_DateInvalidDate);
    }
    
    if ( pulAnsiDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulAnsiDate' parameter passed to 'iUtlDateParseDateToAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get a pointer to the first alphanumetic */
    for ( pucDatePtr = pucDate; (*pucDatePtr != L'\0') && (iswalnum(*pucDatePtr) == 0); pucDatePtr++ );
    if ( *pucDatePtr == L'\0' ) {
        return (UTL_DateInvalidDate);
    }


    /* Clear out the tm structure */
    memset((void *)&tmTime, 0, sizeof(struct tm));

    
    /* Zulu date - "2001-11-12T18:31:01Z" */
    if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y-%m-%dT%H:%M:%SZ", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = bTime = true;
    }

    /* Zulu date without markers - "2001-11-12 18:31:01" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y-%m-%d %H:%M:%S", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = bTime = true;
    }

    /* Web date without timezone - "Fri, 20 May 2009 17:00:00" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%a, %d %h %Y %H:%M:%S", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = bTime = true;
    }
        
    /* dd mmm yyyy - "20 May 2009" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%d %h %Y", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = true;
    }
        
    /* mmm yyyy - "May 2009" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%h %Y", &tmTime)) != NULL ) {
        bYear = bMonth = true;
    }
        
    /* yyyy mmm dd - "2009 May 20" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y %h %d", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = true;
    }
        
    /* yyyy mmm - "2009 May" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y %h", &tmTime)) != NULL ) {
        bYear = bMonth = true;
    }
        
    /* mm/dd/yyyy - "05/20/2009" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%m/%d/%Y", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = true;
    }
        
    /* dd/mm/yyyy - "20/05/2009" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%d/%m/%Y", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = true;
    }
        
    /* mm/yyyy - "05/2009" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%m/%Y", &tmTime)) != NULL ) {
        bYear = bMonth = true;
    }
        
    /* dd-mm-yyyy - "20-05-2009" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%d/%m/%Y", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = true;
    }
        
    /* yyyy-mm-dd - "2009-05-20" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y-%m-%d", &tmTime)) != NULL ) {
        bYear = bMonth = bDay = true;
    }
        
    /* yyyy-mm - "2009-05" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y-%m", &tmTime)) != NULL ) {
        bYear = bMonth = true;
    }
        
    /* yyyy season - "2009 Spring" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y Spring", &tmTime)) != NULL ) {
        bYear = bMonth = true;
        tmTime.tm_mon = 2;
    }
        
    /* yyyy season - "2009 Summer" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y Summer", &tmTime)) != NULL ) {
        bYear = bMonth = true;
        tmTime.tm_mon = 5;
    }
        
    /* yyyy season - "2009 Fall" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y Fall", &tmTime)) != NULL ) {
        bYear = bMonth = true;
        tmTime.tm_mon = 8;
    }
        
    /* yyyy season - "2009 Autumn" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y Autumn", &tmTime)) != NULL ) {
        bYear = bMonth = true;
        tmTime.tm_mon = 8;
    }
        
    /* yyyy season - "2009 Winter" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y Winter", &tmTime)) != NULL ) {
        bYear = bMonth = true;
        tmTime.tm_mon = 11;
    }
        
    /* yyyy - "2009" */
    else if ( (pucParsePtr = s_strptime(pucDatePtr, "%Y", &tmTime)) != NULL ) {
        bYear = true;
    }
    
    /* Failed to parse the date */
    else {
        return (UTL_DateInvalidDate);
    }
        

    /* Create the ansi date string */
    sprintf(pucAnsiDate, "%04d%02d%02d%02d%02d%02d", 
            tmTime.tm_year + 1900, 
            (bMonth == true) ? tmTime.tm_mon + 1 : 0, 
            (bDay == true) ? tmTime.tm_mday : 0, 
            (bTime == true) ? tmTime.tm_hour : 0, 
            (bTime == true) ? tmTime.tm_min : 0, 
            (bTime == true) ? tmTime.tm_sec : 0);

    /* Convert the ansi date to a number and set the return pointer */
    *pulAnsiDate = s_strtol(pucAnsiDate, NULL, 10);
    
/* if ( *pucParsePtr != '\0' ) { */
/*     fprintf(stderr, "[%s]   [%s]\n", pucDatePtr, pucAnsiDate); */
/* } */

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateParseWideDateToAnsiDate()

    Purpose:    This functions tries to parse a date out of the line that 
                is passed and returns it as an ansi date. For example 
                February 20th, 1994 is represented as 19940220 (YYYYMMDD). 

                Formats recognised:
                    yyyy-mm-ddThh:mm:ssZ            (zulu date)
                    yyyy-mm-dd hh:mm:ss             (zulu date without markers)
                    ddd, dd mmm yyyy hh:mm:ss       (web date)
                    dd mmm yyyy
                    mmm yyyy
                    yyyy mmm dd
                    yyyy mmm
                    mm/dd/yyyy
                    dd/mm/yyyy
                    mm/yyyy
                    dd-mm-yyyy
                    yyyy-mm-dd
                    yyyy-mm
                    yyyy season
                    yyyy

    Parameters: pucDate         a string containing the date
                pulAnsiDate     a return pointer for the ansi date

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateParseWideDateToAnsiDate
(
    wchar_t *pwcDate,
    unsigned long *pulAnsiDate
)
{

    int             iError = UTL_NoError;
    unsigned int    uiDateLength = 0;
    unsigned char   *pucDate = NULL;


    /* Check parameters */
    if ( bUtlStringsIsWideStringNULL(pwcDate) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcDate' parameter passed to 'iUtlDateParseWideDateToAnsiDate'."); 
        return (UTL_DateInvalidDate);
    }
    
    if ( pulAnsiDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulAnsiDate' parameter passed to 'iUtlDateParseWideDateToAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the length of the date */
    uiDateLength = s_wcslen(pwcDate) + 1;

    /* Allocate memory for the narrow character version of the date - potentially too short */
    if ( (pucDate = (unsigned char *)s_malloc((size_t)(uiDateLength * sizeof(unsigned char)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Create a narrow version of the date - potentially lossy */
    snprintf(pucDate, uiDateLength, "%ls", pwcDate);

    /* Parse the date */
    iError = iUtlDateParseDateToAnsiDate(pucDate, pulAnsiDate);

    /* Free the date */
    s_free(pucDate);
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateValidateAnsiDate()

    Purpose:    Returns a pointer to a normalized ansi date derived from
                an ansi date. An ansi date is usually in the formats:

                    YYYYMMDDHHMMSS
                    YYYYMMDDHHMM
                    YYYYMMDDHH
                    YYYYMMDD
                    YYYYMM
                    YYYY

    Parameters: pucAnsiDate     ansi date to validate
                pulAnsiDate     return pointer for the normalized ansi date

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateValidateAnsiDate
(
    unsigned char *pucAnsiDate,
    unsigned long *pulAnsiDate
)
{

    int         iError = UTL_NoError;
    wchar_t     pwcAnsiDate[15] = {L'\0'};


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pucAnsiDate) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucAnsiDate' parameter passed to 'iUtlDateValidateAnsiDate'."); 
        return (UTL_DateInvalidDate);
    }
    
    if ( pulAnsiDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulAnsiDate' parameter passed to 'iUtlDateValidateAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Create a wide version of the ansi date */
    swprintf(pwcAnsiDate, 15, L"%s", pucAnsiDate);

    /* Validate the wide ansi date */
    iError = iUtlDateValidateWideAnsiDate(pwcAnsiDate, pulAnsiDate);
    
    
    return (iError);


}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateValidateWideAnsiDate()

    Purpose:    Returns a pointer to a normalized ansi date derived from
                a wide ansi date. An ansi date is usually in the formats:

                    YYYYMMDDHHMMSS
                    YYYYMMDDHHMM
                    YYYYMMDDHH
                    YYYYMMDD
                    YYYYMM
                    YYYY

    Parameters: pwcAnsiDate     ansi date to validate
                pulAnsiDate     return pointer for the normalized ansi date

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateValidateWideAnsiDate
(
    wchar_t *pwcAnsiDate,
    unsigned long *pulAnsiDate
)
{

    int             iError = UTL_NoError;
    wchar_t         *pwcAnsiTimePtr = NULL;
    wchar_t         wcCharacter = L'\0';
    unsigned int    uiAnsiDate = 0;
    unsigned int    uiAnsiTime = 0;


    /* Check parameters */
    if ( bUtlStringsIsStringNULL(pwcAnsiDate) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcAnsiDate' parameter passed to 'iUtlDateValidateWideAnsiDate'."); 
        return (UTL_DateInvalidDate);
    }
    
    if ( pulAnsiDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulAnsiDate' parameter passed to 'iUtlDateValidateWideAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Dates longer than 8 characters are assumed to contain both a date and a time,
    ** so we null terminate the date portion of the date/time, process that, 
    ** restore the character and process the time
    */
    if ( s_wcslen(pwcAnsiDate) > 8 ) {
        wcCharacter = *(pwcAnsiDate + 8);
        *(pwcAnsiDate + 8) = L'\0';
    }

    
    /* Validate the date portion of the date/time */
    {
        wchar_t     pwcYear[5] = {L'\0'};
        wchar_t     pwcMonth[3] = {L'\0'};
        wchar_t     pwcDay[3] = {L'\0'};
        wchar_t     pwcBuffer[9] = {L'\0'};
        wchar_t     *pwcBufferPtr = NULL;
    
        /* Normalize the date to 8 characters */
        if ( s_wcslen(pwcAnsiDate) < 8 ) {
    
            /* Convert the value to a number */
            unsigned int uiDate = s_wcstol(pwcAnsiDate, NULL, 10);
    
            /* Only given year */
            if ( s_wcslen(pwcAnsiDate) <= 4 ) {
                swprintf(pwcBuffer, 9, L"%04u0000", uiDate);
            }
            /* Only given year and month */
            else if ( s_wcslen(pwcAnsiDate) <= 6 ) {
                swprintf(pwcBuffer, 9, L"%06u00", uiDate);
            }
            /* Pad out year, month and day */
            else if ( s_wcslen(pwcAnsiDate) < 8 ) {
                swprintf(pwcBuffer, 9, L"%08u", uiDate);
            }
            
            /* Set the buffer pointer */
            pwcBufferPtr = pwcBuffer;
        }
        else {
            /* Set the buffer pointer */
            pwcBufferPtr = pwcAnsiDate;
        }
    
        /* Check the validity of the year */
        s_wcsncpy(pwcYear, pwcBufferPtr, 4);
        if ( s_wcstol(pwcYear, NULL, 10) > 9999 ) {
            return (UTL_DateInvalidDate);
        }
        
        /* Check the validity of the month */
        s_wcsncpy(pwcMonth, pwcBufferPtr + 4, 2);
        if ( s_wcstol(pwcMonth, NULL, 10) > 12 ) {
            return (UTL_DateInvalidDate);
        }
                
        /* Check the validity of the day */
        s_wcsncpy(pwcDay, pwcBufferPtr + 6, 2);
        if ( s_wcstol(pwcDay, NULL, 10) > 31 ) {
            return (UTL_DateInvalidDate);
        }
    
        /* Set the ansi date */
        uiAnsiDate = s_wcstol(pwcBufferPtr, NULL, 10);
    
    }


    /* Restore the saved character and get a pointer to the time portion of the date/time */
    if ( wcCharacter != L'\0' ) {
        
        /* Restore the saved character */
        *(pwcAnsiDate + 8) = wcCharacter;

        /* Get a pointer to the time */
        pwcAnsiTimePtr = pwcAnsiDate + 8;
    }

    
    /* Validate the time portion of the date/time */
    if ( pwcAnsiTimePtr != NULL ) {
        
        wchar_t     pwcHours[3] = {L'\0'};
        wchar_t     pwcMinutes[3] = {L'\0'};
        wchar_t     pwcSeconds[3] = {L'\0'};
        wchar_t     pwcBuffer[7] = {L'\0'};
        wchar_t     *pwcBufferPtr = NULL;

        /* Normalize the time to 6 characters */
        if ( s_wcslen(pwcAnsiTimePtr) < 6 ) {
        
            /* Convert the value to a number */
            unsigned int uiTime = s_wcstol(pwcAnsiTimePtr, NULL, 10);
        
            /* Only given hours */
            if ( s_wcslen(pwcAnsiTimePtr) <= 2 ) {
                swprintf(pwcBuffer, 7, L"%02u0000", uiTime);
            }
            /* Only given hours and minutes */
            else if ( s_wcslen(pwcAnsiTimePtr) <= 4 ) {
                swprintf(pwcBuffer, 7, L"%04u00", uiTime);
            }
            /* Pad out hours, minutes and seconds */
            else if ( s_wcslen(pwcAnsiTimePtr) < 6 ) {
                swprintf(pwcBuffer, 7, L"%06u", uiTime);
            }
            
            /* Set the buffer pointer */
            pwcBufferPtr = pwcBuffer;
        }
        else {
            /* Set the buffer pointer */
            pwcBufferPtr = pwcAnsiTimePtr;
        }
        
        /* Check the validity of the hour */
        s_wcsncpy(pwcHours, pwcBufferPtr, 2);
        if ( s_wcstol(pwcHours, NULL, 10) > 23 ) {
            return (UTL_DateInvalidTime);
        }
        
        /* Check the validity of the minutes */
        s_wcsncpy(pwcMinutes, pwcBufferPtr + 2, 2);
        if ( s_wcstol(pwcMinutes, NULL, 10) > 59 ) {
            return (UTL_DateInvalidTime);
        }
                
        /* Check the validity of the seconds */
        s_wcsncpy(pwcSeconds, pwcBufferPtr + 4, 2);
        if ( s_wcstol(pwcSeconds, NULL, 10) > 59 ) {
            return (UTL_DateInvalidTime);
        }
        
        /* Set the ansi time */
        uiAnsiTime = s_wcstol(pwcBufferPtr, NULL, 10);
    }

    
    /* Set the return pointer with the validated date/time */ 
    *pulAnsiDate = ((unsigned long)uiAnsiDate * 1000000) + uiAnsiTime;


    return (iError);


}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateGetWebDateFromAnsiDate()

    Purpose:    Returns a pointer a pointer to a web date derived from
                an ansi date. An ansi date is in the format:

                    YYYYMMDDHHMMSS

    Parameters: ulAnsiDate      ansi date to format
                pucDate         return pointer for the web date
                uiDateLength    length of the return pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateGetWebDateFromAnsiDate
(
    unsigned long ulAnsiDate,
    unsigned char *pucDate,
    unsigned int uiDateLength
)
{

    int         iError = UTL_NoError;
    time_t      tTime = (time_t)0;


    /* Check parameters */
    if ( pucDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDate' parameter passed to 'iUtlDateGetWebDateFromAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiDateLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDateLength' parameter passed to 'iUtlDateGetWebDateFromAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the time for the ansi date */
    if ( (iError = iUtlDateGetTimeFromAnsiDate(ulAnsiDate, &tTime)) != UTL_NoError ) {
        return (iError);
    }
    
    /* Get the web date for the time */
    if ( (iError = iUtlDateGetWebDateFromTime(tTime, pucDate, uiDateLength)) != UTL_NoError ) {
        return (iError);
    }
    
    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateGetWebDateFromTime()

    Purpose:    Returns a web date the time, the current
                time is used if the passed time is <= 0.

    Parameters: tTime           time to convert
                pucDate         return pointer for the web date
                uiDateLength    length of the return pointer

    Globals:    none

    Returns:    Returns:    UTL error code

*/
int iUtlDateGetWebDateFromTime
(
    time_t tTime,
    unsigned char *pucDate,
    unsigned int uiDateLength
)
{

    time_t      tLocalTime = (time_t)0;
    struct tm   tmTime;


    /* Check parameters */
    if ( pucDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDate' parameter passed to 'iUtlDateGetWebDateFromTime'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiDateLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDateLength' parameter passed to 'iUtlDateGetWebDateFromTime'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the current time (if needed) and extract a time structure from it */
    tLocalTime = (tTime <= (time_t)0) ? s_time(NULL) : (time_t)tTime;
    if ( s_gmtime_r(&tLocalTime, &tmTime) == NULL ) {
        return (UTL_DateInvalidTimeT);
    }

    /* Create the web date - "Fri, 20 May 2009 17:00:00 GMT" */
    if ( s_strftime(pucDate, uiDateLength, "%a, %d %h %Y %H:%M:%S GMT", &tmTime) == 0 ) {
        return (UTL_DateInvalidTimeT);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateGetZuluDateFromAnsiDate()

    Purpose:    Returns a pointer a pointer to a zulu date derived from
                an ansi date. An ansi date is in the format:

                    YYYYMMDDHHMMSS

    Parameters: ulAnsiDate      ansi date to format
                pucDate         return pointer for the zulu date
                uiDateLength    length of the return pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateGetZuluDateFromAnsiDate
(
    unsigned long ulAnsiDate,
    unsigned char *pucDate,
    unsigned int uiDateLength
)
{

    int         iError = UTL_NoError;
    time_t      tTime = (time_t)0;


    /* Check parameters */
    if ( pucDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDate' parameter passed to 'iUtlDateGetZuluDateFromAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiDateLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDateLength' parameter passed to 'iUtlDateGetZuluDateFromAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }



    /* Get the time for the ansi date */
    if ( (iError = iUtlDateGetTimeFromAnsiDate(ulAnsiDate, &tTime)) != UTL_NoError ) {
        return (iError);
    }
    
    /* Get the zulu date for the time */
    if ( (iError = iUtlDateGetZuluDateFromTime(tTime, pucDate, uiDateLength)) != UTL_NoError ) {
        return (iError);
    }
    
    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/



/*

    Function:   iUtlDateGetZuluDateFromTime()

    Purpose:    Returns a zulu date the time, the current
                time is used if the passed time is <= 0.

    Parameters: tTime           time to convert
                pucDate         return pointer for the zulu date
                uiDateLength    length of the return pointer

    Globals:    none

    Returns:    Returns:    UTL error code

*/
int iUtlDateGetZuluDateFromTime
(
    time_t tTime,
    unsigned char *pucDate,
    unsigned int uiDateLength
)
{

    time_t      tLocalTime = (time_t)0;
    struct tm   tmTime;


    /* Check parameters */
    if ( pucDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDate' parameter passed to 'iUtlDateGetZuluDateFromTime'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiDateLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDateLength' parameter passed to 'iUtlDateGetZuluDateFromTime'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the current time (if needed) and extract a time structure from it */
    tLocalTime = (tTime <= (time_t)0) ? s_time(NULL) : (time_t)tTime;
    if ( s_gmtime_r(&tLocalTime, &tmTime) == NULL ) {
        return (UTL_DateInvalidTimeT);
    }

    /* Create the zulu date - "2008-01-20T14:48:44Z" */
    if ( s_strftime(pucDate, uiDateLength, "%Y-%m-%d-T%H:%M:%SZ", &tmTime) == 0 ) {
        return (UTL_DateInvalidTimeT);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateGetTimeFromAnsiDate()

    Purpose:    This function gets a time from an ansi date.

    Parameters: ulAnsiDate      ansi date
                ptTime          return pointer for the time_t

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateGetTimeFromAnsiDate
(
    unsigned long ulAnsiDate,
    time_t    *ptTime
)
{

    unsigned char   pucAnsiDate[15] = {'\0'};
    unsigned char   pucBuffer[5] = {'\0'};
    
    struct tm       tmTm;


    /* Check parameters */
    if ( ptTime == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ptTime' parameter passed to 'iUtlDateGetTimeFromAnsiDate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Clear the tm structure */    
    s_memset(&tmTm, 0, sizeof(struct tm));

    
    /* Print the date to a string */
    sprintf(pucAnsiDate, "%lu", ulAnsiDate);


    /* Get the year */
    sprintf(pucBuffer, "%c%c%c%c", pucAnsiDate[0], pucAnsiDate[1], pucAnsiDate[2], pucAnsiDate[3]);
    tmTm.tm_year = s_strtol(pucBuffer, NULL, 10) - 1900;

    /* Get the month */
    sprintf(pucBuffer, "%c%c", pucAnsiDate[4], pucAnsiDate[5]);
    tmTm.tm_mon = s_strtol(pucBuffer, NULL, 10);
    tmTm.tm_mon -= (tmTm.tm_mon > 0) ? 1 : 0;
    
    /* Get the day */
    sprintf(pucBuffer, "%c%c", pucAnsiDate[6], pucAnsiDate[7]);
    tmTm.tm_mday = s_strtol(pucBuffer, NULL, 10);
    tmTm.tm_mday -= (tmTm.tm_mday > 0) ? 1 : 0;

    /* Get the hour */
    sprintf(pucBuffer, "%c%c", pucAnsiDate[8], pucAnsiDate[9]);
    tmTm.tm_hour = s_strtol(pucBuffer, NULL, 10);
    
    /* Get the minutes */
    sprintf(pucBuffer, "%c%c", pucAnsiDate[10], pucAnsiDate[11]);
    tmTm.tm_min = s_strtol(pucBuffer, NULL, 10);
    
    /* Get the seconds */
    sprintf(pucBuffer, "%c%c", pucAnsiDate[12], pucAnsiDate[13]);
    tmTm.tm_sec = s_strtol(pucBuffer, NULL, 10);

    /* Make the time and return */
    if ( (*ptTime = s_mktime(&tmTm)) == -1 ) {
        return (UTL_DateInvalidDate);
    }

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDateGetAnsiDateFromTime()

    Purpose:    Returns the ansi date for tTime, or the current
                time if tTime <= 0.

    Parameters: tTime           time to convert
                pulAnsiDate     return pointer for the ansi date

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDateGetAnsiDateFromTime
(
    time_t tTime,
    unsigned long *pulAnsiDate
)
{

    time_t          tLocalTime = (time_t)0;
    struct tm       tmTime;
    unsigned char   pucAnsiDate[15] = {'\0'};


    /* Check parameters */
    if ( pulAnsiDate == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulAnsiDate' parameter passed to 'iUtlDateGetAnsiDateFromTime'."); 
        return (UTL_ReturnParameterError);
    }



    /* Get the current time (if needed) and extract a time structure from it */
    tLocalTime = (tTime <= (time_t)0) ? s_time(NULL) : (time_t)tTime;
    if ( s_localtime_r(&tLocalTime, &tmTime) == NULL ) {
        return (UTL_DateInvalidTimeT);
    }

    
    /* Create the ansi date string */
    sprintf(pucAnsiDate, "%04d%02d%02d%02d%02d%02d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);
    
    /* Conver the ansi date to a number and set the return pointer */
    *pulAnsiDate = s_strtol(pucAnsiDate, NULL, 10);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


