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

    Module:     strings.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 April 2004

    Purpose:    This file contains a lot of useful strings functions.

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

/* This is here for reference purposes only, it must be enabled/disabled in strings.h */
/* #define DEBUG_STRINGS */


/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.strings"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* String length */
#define UTL_STRING_LENGTH                    (1024)


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strnncat()

    Purpose:    Like strncat, except the fourth argument limits the maximum total 
                length of the resulting string, 

    Parameters: pucDestination      The string to concatenate to 
                pucSource           The string to concatenate
                zMaxToAdd           Maximum number of characters to add
                zMaxTotal           Maximum final length of the string
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    A pointer to destination string
*/
unsigned char *fs_strnncat
(
#if defined(DEBUG_STRINGS)
    unsigned char *pucDestination,
    unsigned char *pucSource,
    size_t zMaxToAdd,
    size_t zMaxTotal,
    char *pcFile,
    size_t zLine
#else
    unsigned char *pucDestination,
    unsigned char *pucSource,
    size_t zMaxToAdd,
    size_t zMaxTotal
#endif    /* defined(DEBUG_STRINGS) */
)
{

    size_t      zDestinationLength = (size_t)0;
    size_t      zSourceLength = (size_t)0;


#if defined(DEBUG_STRINGS)
    if ( pucDestination == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strnncat found a NULL string pointer (pucDestination), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pucSource == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strnncat found a NULL string pointer (pucSource), file: '%s', line: %lu", pcFile, zLine);
    }

    if ( zMaxToAdd < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strnncat found a maxToAdd length of: %ld, file: '%s', line: %lu", zMaxToAdd, pcFile, zLine);
    }
    else if ( zMaxToAdd == 0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strnncat found a maxToAdd length of: %ld, file: '%s', line: %lu.", zMaxToAdd, pcFile, zLine);
    }

    if ( zMaxTotal < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strnncat found a zMaxTotal length of: %ld, file: '%s', line: %lu", zMaxTotal, pcFile, zLine);
    }
    else if ( zMaxTotal == 0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strnncat found a zMaxTotal length of: %ld, file: '%s', line: %lu.", zMaxTotal, pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    zDestinationLength = s_strlen(pucDestination);
    zSourceLength = s_strlen(pucSource);

    /* Use regular old strncat */
    if ( (zDestinationLength + zSourceLength) < zMaxTotal ) {
        s_strncat(pucDestination, pucSource, zMaxToAdd);
    }
    else {
        /* Else concatenate and truncate if there is room */
        size_t    zTruncateTo = (size_t)(zMaxTotal - zDestinationLength - 1);

        if ( zTruncateTo > 0 ) {
            s_strncat(pucDestination, pucSource, zTruncateTo);
        }
    }


    return (pucDestination);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strnncpy()

    Purpose:    Like strncpy except that it garantees that the destination 
                string ends with a NULL at position zDestinationLength

    Parameters: pucDestination          The string to copy to 
                pucSource               The string to copy
                zDestinationLength      Max length of pucDestination
                pcFile                  __FILE__
                zLine                   __LINE__

    Globals:    none

    Returns:    A pointer to destination string
*/
unsigned char *fs_strnncpy
(
#if defined(DEBUG_STRINGS)
    unsigned char *pucDestination,
    unsigned char *pucSource,
    size_t zDestinationLength,
    char *pcFile,
    size_t zLine
#else
    unsigned char *pucDestination,
    unsigned char *pucSource,
    size_t zDestinationLength
#endif    /* defined(DEBUG_STRINGS) */
)
{

#if defined(DEBUG_STRINGS)
    if ( pucDestination == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strnncpy found a NULL string pointer (pucDestination), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pucSource == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strnncpy found a NULL string pointer (pucSource), file: '%s', line: %lu", pcFile, zLine);
    }

    if ( zDestinationLength < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strnncpy found a length of: %ld, file: '%s', line: %lu", zDestinationLength, pcFile, zLine);
    }
    else if ( zDestinationLength == 0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_strnncpy found a length of: %ld, file: '%s', line: %lu.", zDestinationLength, pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    if ( s_strlen(pucSource) >= zDestinationLength ) {
        pucDestination[zDestinationLength - 1] = '\0';
        s_strncpy(pucDestination, pucSource, zDestinationLength - 1);
    }
    else {
        s_strcpy(pucDestination, pucSource);
    }


    return (pucDestination);
    
}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_substrcmp()

    Purpose:    Compares the strings up until one of them ends,
                returns 0 if there are the same, -1 if they are not

    Parameters: pucString1      First string
                pucString2      Second string
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if there are the same, -1 if they are not
*/
int fs_substrcmp
(
#if defined(DEBUG_STRINGS)
    unsigned char *pucString1,
    unsigned char *pucString2,
    char *pcFile,
    size_t zLine
#else
    unsigned char *pucString1,
    unsigned char *pucString2
#endif    /* defined(DEBUG_STRINGS) */
)
{

    unsigned char   *pucString1Ptr = NULL;
    unsigned char   *pucString2Ptr = NULL;


#if defined(DEBUG_STRINGS)
    /* Check the strings */ 
    if ( pucString1 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_substrcmp found a NULL string pointer (pucString1), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pucString2 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_substrcmp found a NULL string pointer (pucString2), file: '%s', line: %lu", pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    for ( pucString1Ptr = pucString1, pucString2Ptr = pucString2; (*pucString1Ptr != '\0') && (*pucString2Ptr != '\0'); pucString1Ptr++, pucString2Ptr++ ) {
        if ( *pucString1Ptr != *pucString2Ptr ) {
            return (-1);
        }
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_strcasestr()

    Purpose:    Compares the strings up until one of them ends.
                returns true if they are the same, false if not.

    Parameters: pucString1      First string
                pucString2      Second string
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    A pointer to the first occurance of pucString2 in pucString1

*/
unsigned char *fs_strcasestr
(
#if defined(DEBUG_STRINGS)
    unsigned char *pucString1,
    unsigned char *pucString2,
    char *pcFile,
    size_t zLine
#else
    unsigned char *pucString1,
    unsigned char *pucString2
#endif    /* defined(DEBUG_STRINGS) */
)
{

    unsigned char   *pucString1Ptr = NULL;
    unsigned char   *pucString2Ptr = NULL;
    unsigned int    uiString1Length = 0;
    unsigned int    uiString2Length = 0;


#if defined(DEBUG_STRINGS)
    /* Check the parameters */ 
    if ( pucString1 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strcasestr found a NULL string pointer (pucString1), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pucString2 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_strcasestr found a NULL string pointer (pucString2), file: '%s', line: %lu", pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    if ( bUtlStringsIsStringNULL(pucString2) == true ) {
        return (pucString1);
    }

    for ( pucString1Ptr = pucString1, pucString2Ptr = pucString2, uiString1Length = s_strlen(pucString1), uiString2Length = s_strlen(pucString2); 
            (*pucString1Ptr != '\0') && (uiString2Length <= (uiString1Length - (pucString1Ptr - pucString1))); pucString1Ptr++, pucString2Ptr++ ) {
        
        if ( tolower(*pucString1Ptr) == tolower(*pucString2) ) {
            if ( s_strncasecmp(pucString1Ptr, pucString2, uiString2Length) == 0 ) {
                return (pucString1Ptr);
            }
          }
    }
  
  
    return (NULL);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsnncat()

    Purpose:    Like wcsncat, except the fourth argument limits the maximum total 
                length of the resulting string, 

    Parameters: pwcDestination      The string to concatenate to 
                pwcSource           The string to concatenate
                zMaxToAdd           Maximum number of characters to add
                zMaxTotal           Maximum final length of the string
                pcFile              __FILE__
                zLine               __LINE__

    Globals:    none

    Returns:    A pointer to destination string
*/
wchar_t *fs_wcsnncat
(
#if defined(DEBUG_STRINGS)
    wchar_t *pwcDestination,
    wchar_t *pwcSource,
    size_t zMaxToAdd,
    size_t zMaxTotal,
    char *pcFile,
    size_t zLine
#else
    wchar_t *pwcDestination,
    wchar_t *pwcSource,
    size_t zMaxToAdd,
    size_t zMaxTotal
#endif    /* defined(DEBUG_STRINGS) */
)
{

    size_t      zDestinationLength = (size_t)0;
    size_t      zSourceLength = (size_t)0;


#if defined(DEBUG_STRINGS)
    if ( pwcDestination == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsnncat found a NULL string pointer (pwcDestination), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pwcSource == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsnncat found a NULL string pointer (pwcSource), file: '%s', line: %lu", pcFile, zLine);
    }

    if ( zMaxToAdd < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsnncat found a maxToAdd length of: %ld, file: '%s', line: %lu", zMaxToAdd, pcFile, zLine);
    }
    else if ( zMaxToAdd == 0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsnncat found a maxToAdd length of: %ld, file: '%s', line: %lu.", zMaxToAdd, pcFile, zLine);
    }

    if ( zMaxTotal < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsnncat found a zMaxTotal length of: %ld, file: '%s', line: %lu", zMaxTotal, pcFile, zLine);
    }
    else if ( zMaxTotal == 0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsnncat found a zMaxTotal length of: %ld, file: '%s', line: %lu.", zMaxTotal, pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    zDestinationLength = s_wcslen(pwcDestination);
    zSourceLength = s_wcslen(pwcSource);

    /* Use regular old strncat */
    if ( (zDestinationLength + zSourceLength) < zMaxTotal ) {
        s_wcsncat(pwcDestination, pwcSource, zMaxToAdd);
    }
    else {
        /* Else concatenate and truncate if there is room */
        size_t    zTruncateTo = (size_t)(zMaxTotal - zDestinationLength - 1);

        if ( zTruncateTo > 0 ) {
            s_wcsncat(pwcDestination, pwcSource, zTruncateTo);
        }
    }


    return (pwcDestination);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcsnncpy()

    Purpose:    Like wcsncpy except that it garantees that the destination 
                string ends with a NULL at position zDestinationLength

    Parameters: pwcDestination          The string to copy to 
                pwcSource               The string to copy
                zDestinationLength      Max length of pwcDestination
                pcFile                  __FILE__
                zLine                   __LINE__

    Globals:    none

    Returns:    A pointer to destination string
*/
wchar_t *fs_wcsnncpy
(
#if defined(DEBUG_STRINGS)
    wchar_t *pwcDestination,
    wchar_t *pwcSource,
    size_t zDestinationLength,
    char *pcFile,
    size_t zLine
#else
    wchar_t *pwcDestination,
    wchar_t *pwcSource,
    size_t zDestinationLength
#endif    /* defined(DEBUG_STRINGS) */
)
{

#if defined(DEBUG_STRINGS)
    if ( pwcDestination == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsnncpy found a NULL string pointer (pwcDestination), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pwcSource == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsnncpy found a NULL string pointer (pwcSource), file: '%s', line: %lu", pcFile, zLine);
    }

    if ( zDestinationLength < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcsnncpy found a length of: %ld, file: '%s', line: %lu", zDestinationLength, pcFile, zLine);
    }
    else if ( zDestinationLength == 0 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "fs_wcsnncpy found a length of: %ld, file: '%s', line: %lu.", zDestinationLength, pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    if ( s_wcslen(pwcSource) >= zDestinationLength ) {
        pwcDestination[zDestinationLength - 1] = L'\0';
        s_wcsncpy(pwcDestination, pwcSource, zDestinationLength - 1);
    }
    else {
        s_wcscpy(pwcDestination, pwcSource);
    }


    return (pwcDestination);
    
}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_subwcscmp()

    Purpose:    Compares the strings up until one of them ends,
                returns 0 if there are the same, -1 if they are not

    Parameters: pwcString1      First string
                pwcString2      Second string
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    0 if there are the same, -1 if they are not
*/
int fs_subwcscmp
(
#if defined(DEBUG_STRINGS)
    wchar_t *pwcString1,
    wchar_t *pwcString2,
    char *pcFile,
    size_t zLine
#else
    wchar_t *pwcString1,
    wchar_t *pwcString2
#endif    /* defined(DEBUG_STRINGS) */
)
{

    wchar_t     *pwcString1Ptr = NULL;
    wchar_t     *pwcString2Ptr = NULL;


#if defined(DEBUG_STRINGS)
    /* Check the strings */ 
    if ( pwcString1 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_subwcscmp found a NULL string pointer (pwcString1), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pwcString2 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_subwcscmp found a NULL string pointer (pwcString2), file: '%s', line: %lu", pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    for ( pwcString1Ptr = pwcString1, pwcString2Ptr = pwcString2; (*pwcString1Ptr != L'\0') && (*pwcString2Ptr != L'\0'); pwcString1Ptr++, pwcString2Ptr++ ) {
        if ( *pwcString1Ptr != *pwcString2Ptr ) {
            return (-1);
        }
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   fs_wcscasestr()

    Purpose:    Returns a pointer to the first occurance of pwcString2 in pwcString1.

    Parameters: pwcString1      First string
                pwcString2      Second string
                pcFile          __FILE__
                zLine           __LINE__

    Globals:    none

    Returns:    A pointer to the first occurance of pwcString2 in pwcString1

*/
wchar_t *fs_wcscasestr
(
#if defined(DEBUG_STRINGS)
    wchar_t *pwcString1,
    wchar_t *pwcString2,
    char *pcFile,
    size_t zLine
#else
    wchar_t *pwcString1,
    wchar_t *pwcString2
#endif    /* defined(DEBUG_STRINGS) */
)
{

    wchar_t         *pwcString1Ptr = NULL;
    wchar_t         *pwcString2Ptr = NULL;
    unsigned int    uiString1Length = 0;
    unsigned int    uiString2Length = 0;


#if defined(DEBUG_STRINGS)
    /* Check the parameters */ 
    if ( pwcString1 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcscasestr found a NULL string pointer (pwcString1), file: '%s', line: %lu", pcFile, zLine);
    }
    if ( pwcString2 == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "fs_wcscasestr found a NULL string pointer (pwcString2), file: '%s', line: %lu", pcFile, zLine);
    }
#endif    /* defined(DEBUG_STRINGS) */


    if ( bUtlStringsIsWideStringNULL(pwcString2) == true ) {
        return (NULL);
    }

    for ( pwcString1Ptr = pwcString1, pwcString2Ptr = pwcString2, uiString1Length = s_wcslen(pwcString1), uiString2Length = s_wcslen(pwcString2); 
            (*pwcString1Ptr != L'\0') && (uiString2Length <= (uiString1Length - (pwcString1Ptr - pwcString1))); pwcString1Ptr++, pwcString2Ptr++ ) {
        
        if ( towlower(*pwcString1Ptr) == towlower(*pwcString2) ) {
            if ( s_wcsncasecmp(pwcString1Ptr, pwcString2, uiString2Length) == 0 ) {
                return (pwcString1Ptr);
            }
          }
    }
  
  
    return (NULL);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucUtlStringsFormatSignedNumber()

    Purpose:    Returns a pointer to a nicely formated integer, currently we
                only deal with US formats.

                We also assume that there is enough space in the return string

    Parameters: llNumber            number to format
                pucString           return pointer for the formated string
                uiStringLength      size of the return pointer

    Globals:    none

    Returns:    A pointer to a formated string, null on error

*/
unsigned char *pucUtlStringsFormatSignedNumber
(
    long lNumber,
    unsigned char *pucString,
    unsigned int uiStringLength
)
{

    unsigned char   pucNumberString[UTL_STRING_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( pucString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucString' parameter passed to 'pucUtlStringsFormatSignedNumber'."); 
        return (NULL);
    }

    if ( uiStringLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiStringLength' parameter passed to 'pucUtlStringsFormatSignedNumber'."); 
        return (NULL);
    }


    /* Print the integer */
    snprintf(pucNumberString, UTL_STRING_LENGTH + 1, "%ld", lNumber);


    /* Convert the integer into a pretty string */
    return (pucUtlStringsFormatNumberString(pucNumberString, pucString, uiStringLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucUtlStringsFormatUnsignedNumber()

    Purpose:    Returns a pointer to a nicely formated integer, currently we
                only deal with US formats.

                We also assume that there is enough space in the return string

    Parameters: ulNumber        number to format
                pucString           return pointer for the formated string
                uiStringLength      size of the return pointer

    Globals:    none

    Returns:    A pointer to a formated string, null on error

*/
unsigned char *pucUtlStringsFormatUnsignedNumber
(
    unsigned long ulNumber,
    unsigned char *pucString,
    unsigned int uiStringLength
)
{

    unsigned char   pucNumberString[UTL_STRING_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( pucString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucString' parameter passed to 'pucUtlStringsFormatUnsignedNumber'."); 
        return (NULL);
    }

    if ( uiStringLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiStringLength' parameter passed to 'pucUtlStringsFormatUnsignedNumber'."); 
        return (NULL);
    }


    /* Print the integer */
    snprintf(pucNumberString, UTL_STRING_LENGTH + 1, "%lu", ulNumber);


    /* Convert the integer into a pretty string */
    return (pucUtlStringsFormatNumberString(pucNumberString, pucString, uiStringLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucUtlStringsFormatNumberString()

    Purpose:    Returns a pointer to a nicely formated number, currently we
                only deal with US formats.

                We also assume that there is enough space in the return string

    Parameters: pucNumberString     number string to format
                pucString           return pointer for the formated string
                uiStringLength      size of the return pointer

    Globals:    none

    Returns:    A pointer to a formated string, null on error

*/
unsigned char *pucUtlStringsFormatNumberString
(
    unsigned char *pucNumberString,
    unsigned char *pucString,
    unsigned int uiStringLength
)
{

    unsigned char   *pucStringPtr = NULL;
    unsigned char   *pucNumberStringPtr = NULL;
    boolean         bFlag = false;


    /* Check the parameters */
    if ( pucNumberString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucNumberString' parameter passed to 'pucUtlStringsFormatNumberString'."); 
        return (NULL);
    }

    if ( pucString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucString' parameter passed to 'pucUtlStringsFormatNumberString'."); 
        return (NULL);
    }

    if ( uiStringLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiStringLength' parameter passed to 'pucUtlStringsFormatNumberString'."); 
        return (NULL);
    }


    /* Start off the pointers */
    pucNumberStringPtr = pucNumberString;
    pucStringPtr = pucString;


    /* Loop over all the characters */
    while ( *pucNumberStringPtr != '\0' ) {
        
        /* Do we want to put a comma down? */
        if ( ((s_strlen(pucNumberStringPtr) % 3) == 0) && (bFlag == true) ) {

            /* Check that we are not going to burst the buffer */
            if ( (pucStringPtr - pucString) == uiStringLength ) {
                *pucStringPtr = '\0';
                return (pucString);
            }
    
            *pucStringPtr = ',';
            pucStringPtr++;
        }
        
        /* Copy the character over */
        *pucStringPtr = *pucNumberStringPtr;

        /* Set the flag when we can start putting commas down */
        if ( (pucNumberStringPtr != pucNumberString) || (*pucNumberStringPtr != '-') ) {
            bFlag = true;
        }

        /* Check that we are not going to burst the buffer */
        if ( (pucStringPtr - pucString) == uiStringLength ) {
            *pucStringPtr = '\0';
            return (pucString);
        }

        pucStringPtr++;
        pucNumberStringPtr++;
    }

    /* Terminate the destination string */
    *pucStringPtr = '\0';


    return (pucString);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucUtlStringsFormatDoubleAsBytes()

    Purpose:    Returns a pointer to a byte formated number.

                We also assume that there is enough space in the return string

    Parameters: dNumber     number to format
                pucString   return pointer for the formated string

    Globals:    none

    Returns:    A pointer to a formated string, null on error

*/
unsigned char *pucUtlStringsFormatDoubleAsBytes
(
    double dNumber,
    unsigned char *pucString,
    unsigned int uiStringLength
)
{

    double      dI1 = 1024;
    double      dI2 = 1024 * dI1;
    double      dI3 = 1024 * dI2;
    double      dI4 = 1024 * dI3;
    double      dI5 = 1024 * dI4;


    /* Check the parameters */
    if ( pucString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucString' parameter passed to 'pucUtlStringsFormatDoubleAsBytes'."); 
        return (NULL);
    }

    if ( uiStringLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiStringLength' parameter passed to 'pucUtlStringsFormatDoubleAsBytes'."); 
        return (NULL);
    }


    if ( (dNumber > (dI5)) || (dNumber < -(dI5)) ) {
        snprintf(pucString, uiStringLength, "%.1eTB", (dNumber / (dI5)));
    }
    else if ( (dNumber > (dI4)) || (dNumber < -(dI4)) ) {
        snprintf(pucString, uiStringLength, "%.1fTB", (dNumber / (dI4)));
    }
    else if ( (dNumber > (dI3)) || (dNumber < -(dI3)) ) {
        snprintf(pucString, uiStringLength, "%.1fGB", (dNumber / (dI3)));
    }
    else if ( (dNumber > (dI2)) || (dNumber < -(dI2)) ) {
        snprintf(pucString, uiStringLength, "%.1fMB", (dNumber / (dI2)));
    }
    else if ( (dNumber > dI1) || (dNumber < -dI1) ) {
        snprintf(pucString, uiStringLength, "%.1fKB", (dNumber / dI1));
    }
    else {
        snprintf(pucString, uiStringLength, "%.0f bytes", dNumber);
    }


    return (pucString);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsTrimString()

    Purpose:    Trims a string

    Parameters: pucString   a pointer to the string to trim (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsTrimString 
(
    unsigned char *pucString
)
{

    unsigned char   *pucStartPtr = NULL;
    unsigned char   *pucEndPtr = NULL;


    /* Trim the string */
    if ( bUtlStringsIsStringNULL(pucString) == false ) {

        /* Scan for leading spaces */
        for ( pucStartPtr = pucString; *pucStartPtr != '\0'; pucStartPtr++ ) {
            if ( isspace(*pucStartPtr) == 0 ) {
                break;
            }
        }
    
    
        /* Delete trailing stuff */
        for ( pucEndPtr = pucStartPtr + (s_strlen(pucStartPtr) - 1); pucEndPtr >= pucStartPtr; pucEndPtr-- ) {
            if ( isspace(*pucEndPtr) == 0 ) {
                break;
            }
            *pucEndPtr = '\0';
        }
    
    
        /* Move up the string if we need to */
        if ( pucString != pucStartPtr ) {
    
            unsigned char   *pucReadPtr = NULL;
            unsigned char   *pucWritePtr = NULL;
    
            for ( pucWritePtr = pucString, pucReadPtr = pucStartPtr; *pucReadPtr != '\0'; pucWritePtr++, pucReadPtr++ ) {
                *pucWritePtr = *pucReadPtr;
            }
            
            *pucWritePtr = '\0';
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsTrimWideString()

    Purpose:    Trims a wide string

    Parameters: pwcString   a pointer to the wide string to trim (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsTrimWideString 
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStartPtr = NULL;
    wchar_t     *pwcEndPtr = NULL;


    /* Trim the string */
    if ( bUtlStringsIsWideStringNULL(pwcString) == false ) {
    
        /* Scan for leading spaces */
        for ( pwcStartPtr = pwcString; *pwcStartPtr != L'\0'; pwcStartPtr++ ) {
            if ( iswspace(*pwcStartPtr) == 0 ) {
                break;
            }
        }
    
    
        /* Delete trailing stuff */
        for ( pwcEndPtr = pwcStartPtr + (s_wcslen(pwcStartPtr) - 1); pwcEndPtr >= pwcStartPtr; pwcEndPtr-- ) {
            if ( iswspace(*pwcEndPtr) == 0 ) {
                break;
            }
            *pwcEndPtr = L'\0';
        }
    
    
        /* Move up the string if we need to */
        if ( pwcString != pwcStartPtr ) {
    
            wchar_t     *pwcReadPtr = NULL;
            wchar_t     *pwcWritePtr = NULL;
    
            for ( pwcWritePtr = pwcString, pwcReadPtr = pwcStartPtr; *pwcReadPtr != '\0'; pwcWritePtr++, pwcReadPtr++ ) {
                *pwcWritePtr = *pwcReadPtr;
            }
            
            *pwcWritePtr = L'\0';
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsRemoveCharacterFromString()

    Purpose:    Remove character from a string

    Parameters: pucString       a pointer to the string to process (optional)
                ucCharacter     the character to remove

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsRemoveCharacterFromString 
(
    unsigned char *pucString,
    unsigned char ucCharacter
)
{

    unsigned char   *pucReadPtr = NULL;
    unsigned char   *pucWritePtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iUtlStringsRemoveCharacterFromString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pucReadPtr = pucString, pucWritePtr = pucString; *pucReadPtr != '\0'; pucReadPtr++ ) {

        /* Copy the character if it is not in the hit list and advance the write pointer */
        if ( *pucReadPtr != ucCharacter ) {
            *pucWritePtr = *pucReadPtr;
            pucWritePtr++;
        }
    }
    
    /* Append a NULL to terminate the string */
    *pucWritePtr = '\0';


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsRemoveCharacterFromWideString()

    Purpose:    Remove characters from a wide string

    Parameters: pwcString       a pointer to the wide string to process (optional)
                wcCharacter     the character to remove

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsRemoveCharacterFromWideString 
(
    wchar_t *pwcString,
    wchar_t wcCharacter
)
{

    wchar_t     *pwcReadPtr = NULL;
    wchar_t     *pwcWritePtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iUtlStringsRemoveCharacterFromWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pwcReadPtr = pwcString, pwcWritePtr = pwcString; *pwcReadPtr != L'\0'; pwcReadPtr++ ) {

        /* Copy the character if it is not in the hit list and advance the write pointer */
        if ( *pwcReadPtr != wcCharacter ) {
            *pwcWritePtr = *pwcReadPtr;
            pwcWritePtr++;
        }
    }
    
    /* Append a NULL to terminate the string */
    *pwcWritePtr = L'\0';


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsRemoveCharactersFromString()

    Purpose:    Remove characters from a string

    Parameters: pucString       a pointer to the string to process (optional)
                pucCharacters   a pointer to the characters to remove (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsRemoveCharactersFromString 
(
    unsigned char *pucString,
    unsigned char *pucCharacters
)
{

    unsigned char   *pucReadPtr = NULL;
    unsigned char   *pucWritePtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iUtlStringsRemoveCharactersFromString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsStringNULL(pucCharacters) == true ) {
/*         iUtlLogWarn(UTL_LOG_CONTEXT, "Null or empty 'pucCharacters' parameter passed to 'iUtlStringsRemoveCharactersFromString'.");  */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pucReadPtr = pucString, pucWritePtr = pucString; *pucReadPtr != '\0'; pucReadPtr++ ) {

        /* Copy the character if it is not in the hit list and advance the write pointer */
        if ( s_strchr(pucCharacters, *pucReadPtr) == NULL ) {
            *pucWritePtr = *pucReadPtr;
            pucWritePtr++;
        }
    }
    
    /* Append a NULL to terminate the string */
    *pucWritePtr = '\0';


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsRemoveCharactersFromWideString()

    Purpose:    Remove characters from a wide string

    Parameters: pwcString       a pointer to the wide string to process (optional)
                pwcCharacters   a pointer to the characters to remove (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsRemoveCharactersFromWideString 
(
    wchar_t *pwcString,
    wchar_t *pwcCharacters
)
{

    wchar_t     *pwcReadPtr = NULL;
    wchar_t     *pwcWritePtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iUtlStringsRemoveCharactersFromWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsWideStringNULL(pwcCharacters) == true ) {
/*         iUtlLogWarn(UTL_LOG_CONTEXT, "Null or empty 'pwcCharacters' parameter passed to 'iUtlStringsRemoveCharactersFromWideString'.");  */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pwcReadPtr = pwcString, pwcWritePtr = pwcString; *pwcReadPtr != L'\0'; pwcReadPtr++ ) {

        /* Copy the character if it is not in the hit list and advance the write pointer */
        if ( s_wcschr(pwcCharacters, *pwcReadPtr) == NULL ) {
            *pwcWritePtr = *pwcReadPtr;
            pwcWritePtr++;
        }
    }
    
    /* Append a NULL to terminate the string */
    *pwcWritePtr = L'\0';


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsReplaceCharacterInString()

    Purpose:    Replaces characters in a string

    Parameters: pucString           a pointer to the string to process (optional)
                ucCharacter         the character to replace
                ucNewCharacter      character to replace with

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsReplaceCharacterInString 
(
    unsigned char *pucString,
    unsigned char ucCharacter,
    unsigned char ucNewCharacter
)
{

    unsigned char   *pucStringPtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iUtlStringsReplaceCharacterInString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) {

        /* Replace the character if it is on the hit list */
        if ( *pucStringPtr == ucCharacter ) {
            *pucStringPtr = ucNewCharacter;
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsReplaceCharacterInWideString()

    Purpose:    Replaces characters in a wide string

    Parameters: pwcString           a pointer to the wide string to process (optional)
                wcCharacter         the character to replace
                wcNewCharacter      character to replace with

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsReplaceCharacterInWideString 
(
    wchar_t *pwcString,
    wchar_t wcCharacter,
    wchar_t wcNewCharacter
)
{

    wchar_t     *pwcStringPtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iUtlStringsReplaceCharacterInWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) {

        /* Replace the character if it is on the hit list */
        if ( *pwcStringPtr == wcCharacter ) {
            *pwcStringPtr = wcNewCharacter;
        }
    }
    

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsReplaceCharactersInString()

    Purpose:    Replaces characters in a string

    Parameters: pucString           a pointer to the string to process (optional)
                pucCharacters       a pointer to the characters to replace (optional)
                ucNewCharacter      character to replace with

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsReplaceCharactersInString 
(
    unsigned char *pucString,
    unsigned char *pucCharacters,
    unsigned char ucNewCharacter
)
{

    unsigned char       *pucStringPtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iUtlStringsReplaceCharactersInString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsStringNULL(pucCharacters) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucCharacters' parameter passed to 'iUtlStringsReplaceCharactersInString'.");  */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) {

        /* Replace the character if it is on the hit list */
        if ( s_strchr(pucCharacters, *pucStringPtr) != NULL ) {
            *pucStringPtr = ucNewCharacter;
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsReplaceCharactersInWideString()

    Purpose:    Replaces characters in a wide string

    Parameters: pwcString           a pointer to the wide string to process (optional)
                pwcCharacters       a pointer to the characters to replace (optional)
                wcNewCharacter      character to replace with

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsReplaceCharactersInWideString 
(
    wchar_t *pwcString,
    wchar_t *pwcCharacters,
    wchar_t wcNewCharacter
)
{

    wchar_t     *pwcStringPtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iUtlStringsReplaceCharactersInWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsWideStringNULL(pwcCharacters) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcCharacters' parameter passed to 'iUtlStringsReplaceCharactersInWideString'.");  */
        return (UTL_NoError);
    }


    /* Loop over all the characters in the string */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) {

        /* Replace the character if it is on the hit list */
        if ( s_wcschr(pwcCharacters, *pwcStringPtr) != NULL ) {
            *pwcStringPtr = wcNewCharacter;
        }
    }
    

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsReplaceStringInString()

    Purpose:    Replaces a string in a string

    Parameters: pucSourceString             a pointer to the source string
                pucOriginalString           a pointer to the original string to replace
                pucReplacementString        a pointer to the replacement string to replace with
                pucDestinationString        a pointer to the destination string
                uiDestinationStringLength   the destination string length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsReplaceStringInString 
(
    unsigned char *pucSourceString, 
    unsigned char *pucOriginalString,
    unsigned char *pucReplacementString, 
    unsigned char *pucDestinationString, 
    unsigned int uiDestinationStringLength
)
{

    unsigned char   *pucSourceStringPtr = pucSourceString;
    unsigned char   *pucDestinationStringPtr = pucDestinationString;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSourceString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSourceString' parameter passed to 'iUtlStringsReplaceStringInString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsStringNULL(pucOriginalString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucOriginalString' parameter passed to 'iUtlStringsReplaceStringInString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsStringNULL(pucReplacementString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucReplacementString' parameter passed to 'iUtlStringsReplaceStringInString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( pucDestinationString == NULL ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDestinationString' parameter passed to 'iUtlStringsReplaceStringInString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Loop while we are in the source string */
    while ( *pucSourceStringPtr != '\0' ) {
    
        /* Check for a match */
        if ( s_strncmp(pucSourceStringPtr, pucOriginalString, s_strlen(pucOriginalString)) == 0 ) {
            /* Copy in the replacement string and increment the counters */
            s_strnncpy(pucDestinationStringPtr, pucReplacementString, uiDestinationStringLength);
            pucSourceStringPtr += s_strlen(pucOriginalString);
            pucDestinationStringPtr += s_strlen(pucReplacementString);
        }
        else {
            /* Copy the character and increment the counters */
            *pucDestinationStringPtr = *pucSourceStringPtr;
            pucSourceStringPtr++;
            pucDestinationStringPtr++;
        }
    }
    
    /* Terminate the destination string */
    *pucDestinationStringPtr = '\0';


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsReplaceStringInWideString()

    Purpose:    Replaces a string in a string

    Parameters: pwcSourceString             a pointer to the source string
                pwcOriginalString           a pointer to the original string to replace
                pwcReplacementString        a pointer to the replacement string to replace with
                pwcDestinationString        a pointer to the destination string
                uiDestinationStringLength   the destination string length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsReplaceStringInWideString 
(
    wchar_t *pwcSourceString,
    wchar_t *pwcOriginalString,
    wchar_t *pwcReplacementString, 
    wchar_t *pwcDestinationString,
    unsigned int uiDestinationStringLength
)
{

    wchar_t     *pwcSourceStringPtr = pwcSourceString;
    wchar_t     *pwcDestinationStringPtr = pwcDestinationString;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pwcSourceString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcSourceString' parameter passed to 'iUtlStringsReplaceStringInWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsStringNULL(pwcOriginalString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcOriginalString' parameter passed to 'iUtlStringsReplaceStringInWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( bUtlStringsIsStringNULL(pwcReplacementString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcReplacementString' parameter passed to 'iUtlStringsReplaceStringInWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }

    if ( pwcDestinationString == NULL ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null 'pwcDestinationString' parameter passed to 'iUtlStringsReplaceStringInWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Loop while we are in the source string */
    while ( *pwcSourceStringPtr != L'\0' ) {
    
        /* Check for a match */
        if ( s_wcsncmp(pwcSourceStringPtr, pwcOriginalString, s_wcslen(pwcOriginalString)) == 0 ) {
            /* Copy in the replacement string and increment the counters */
            s_wcsnncpy(pwcDestinationStringPtr, pwcReplacementString, uiDestinationStringLength);
            pwcSourceStringPtr += s_wcslen(pwcOriginalString);
            pwcDestinationStringPtr += s_wcslen(pwcReplacementString);
        }
        else {
            /* Copy the character and increment the counters */
            *pwcDestinationStringPtr = *pwcSourceStringPtr;
            pwcSourceStringPtr++;
            pwcDestinationStringPtr++;
        }
    }
    
    /* Terminate the destination string */
    *pwcDestinationStringPtr = L'\0';


    return (UTL_NoError);

}

/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsAppendString()

    Purpose:    Append a string to another, allocating space for the destination
                string

    Parameters: pucDestination      String to append to 
                pucSource           String to append (optional)
                ppucDestination     Return pointer for the destination string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsAppendString
(
    unsigned char *pucDestination,
    unsigned char *pucSource,
    unsigned char **ppucDestination
)
{
    

    /* Dont append NULL data */
    if ( pucSource == NULL ) {
        *ppucDestination = pucDestination;
        return (UTL_NoError);
    }


    /* Allocate the destination string if needed, otherwise we append to it */
    if ( pucDestination == NULL ) {
        
        /* Duplicate the source */
        if ( (*ppucDestination = (unsigned char *)s_strdup(pucSource)) == NULL ) {
            return (UTL_MemError);
        }
    }
    else {

        unsigned int    uiDestinationLength = s_strlen(pucDestination);    
        unsigned int    uiSourceLength = s_strlen(pucSource);    
        unsigned char   *pucLocalDestination = NULL;
 
        /* Reallocate, adding space for the source string */
        if ( (pucLocalDestination = (unsigned char *)s_realloc(pucDestination, (size_t)(uiDestinationLength + uiSourceLength + 1))) == NULL ) {
            return (UTL_MemError);
        }

        /* Copy the source to the destination */
        s_strcat(pucLocalDestination + uiDestinationLength, pucSource);

        /* Set the return pointer */
        *ppucDestination = pucLocalDestination;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsAppendData()

    Purpose:    Append data to a string, allocating space for the destination
                string

    Parameters: pucDestination      String to append to 
                pucSource           Data to append (optional)
                uiSourceLength      Length of data to append (optional)
                ppucDestination     Return pointer for the destination string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsAppendData
(
    unsigned char *pucDestination,
    void *pvSource,
    unsigned int uiSourceLength,
    unsigned char **ppucDestination
)
{
    

    /* Dont append NULL data */
    if ( (pvSource == NULL) || (uiSourceLength == 0) ) {
        *ppucDestination = pucDestination;
        return (UTL_NoError);
    }


    /* Allocate the destination string if needed, otherwise we append to it */
    if ( pucDestination == NULL ) {
        
        /* Allocate space for the source */    
        if ( (*ppucDestination = (unsigned char *)s_malloc((size_t)(uiSourceLength + 1))) == NULL ) {
            return (UTL_MemError);
        }
        
        /* Copy the source to the destination */
        s_memcpy(*ppucDestination, pvSource, uiSourceLength);
    
        /* NULL terminate the destination */
        *(*ppucDestination + uiSourceLength) = '\0';
    }
    else {

        size_t          uiDestinationLength = s_strlen(pucDestination);    
        unsigned char   *pucLocalDestination = NULL;

        /* Reallocate, adding space for the source string */
        if ( (pucLocalDestination = (unsigned char *)s_realloc(pucDestination, (size_t)(uiDestinationLength + uiSourceLength + 1))) == NULL ) {
            return (UTL_MemError);
        }

        /* Copy the source to the destination */
        s_memcpy(pucLocalDestination + uiDestinationLength, pvSource, uiSourceLength);

        /* NULL terminate the destination */
        *(pucLocalDestination + uiDestinationLength + 1) = '\0';
        
        /* Set the return pointer */
        *ppucDestination = pucLocalDestination;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsNullTerminateData()

    Purpose:    NULL terminate data, extending the source by one byte 
                and appending a NULL

    Parameters: pucSource           Data to NULL terminate
                uiSourceLength      Length of data to NULL terminate
                ppucDestination     Return pointer for the destination string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsNullTerminateData
(
    void *pvSource, 
    unsigned int uiSourceLength, 
    unsigned char **ppucDestination
)
{
    
    unsigned char   *pucDestination = NULL;


    /* Dont append NULL data */
    if ( (pvSource == NULL) || (uiSourceLength == 0) ) {
        return (UTL_NoError);
    }


    /* Reallocate, adding one byte for the terminating NULL */
    if ( (pucDestination = (unsigned char *)s_realloc(pvSource, (size_t)(uiSourceLength + 1))) == NULL ) {
        return (UTL_MemError);
    }

    /* NULL terminate the destination */
    *(pucDestination + uiSourceLength) = '\0';

    /* Set the return pointer */
    *ppucDestination = pucDestination;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlStringsIsStringUrl()

    Purpose:    Returns true if the string is a URL

    Parameters: pucString       String to test 

    Globals:    none

    Returns:    true if the string is a URL, false if not

*/
boolean bUtlStringsIsStringUrl
(
    unsigned char *pucString
)
{
    
    regex_t     rRegex;
    boolean     bStatus = false;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'bUtlStringsIsStringUrl'.");  */
        return (false);
    }


    /* Create the regex structure */
    if ( s_regcomp(&rRegex, "^[a-z]{1,10}://", REG_EXTENDED | REG_NOSUB) != 0 ) {
        return (false);
    }

    /* Run the match */
    bStatus = (s_regexec(&rRegex, pucString, (size_t)0, NULL, 0) == 0) ? true : false; 

    /* Free the regex */
    s_regfree(&rRegex);

    
    /* Return the status */ 
    return (bStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlStringsIsWideStringUrl()

    Purpose:    Returns true if the wide string is a URL

    Parameters: pucString       String to test 

    Globals:    none

    Returns:    true if the wide string is a URL, false if not

*/
boolean bUtlStringsIsWideStringUrl
(
    wchar_t *pwcString
)
{
    
    unsigned char   pucDestinationString[UTL_STRING_LENGTH + 1] = {'\0'};
#if defined (NOTUSED)
    unsigned int    uiSourceStringLength = 0;
#endif    /* defined(NOTUSED) */


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'bUtlStringsIsWideStringUrl'."); 
        return (false);
    }


    /* Convert the string - cheap option, expensive option below, note that we don't 
    ** need all the string to check that it is a URL, only the first 20 character 
    */    
    snprintf(pucDestinationString, 21, "%ls", pwcString);


#if defined (NOTUSED)
    /* Get the length of the string */
    uiSourceStringLength = s_wcslen(pwcString);

    /* We don't need all the string to check that it is a URL, only the first 20 character */
    uiSourceStringLength = UTL_MACROS_MIN(uiSourceStringLength, 20);

    /* Convert the string */    
    if ( (iError = iLngConvertWideStringToUtf8_s(pwcString, uiSourceStringLength, pucDestinationString, UTL_STRING_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a url from wide characters to utf-8, lng error: %d.", iError);
        return (false);
    }
#endif    /* defined(NOTUSED) */


    /* Check the string */ 
    return (bUtlStringsIsStringUrl(pucDestinationString));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsDecodeXmlString()

    Purpose:    Decode an XML encoded string in place

    Parameters: pucString       string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsDecodeXmlString
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringReadPtr = NULL;
    unsigned char   *pucStringWritePtr = NULL;
    

    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iUtlStringsDecodeXmlString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Decode the string */
    for ( pucStringReadPtr = pucString, pucStringWritePtr = pucString; *pucStringReadPtr != '\0'; pucStringWritePtr++ ) {
        
        /* Check for an escape on '&' */
        if ( *pucStringReadPtr == '&' ) {

            /* Convert "&amp;" to '&' */
            if ( s_strncmp(pucStringReadPtr, "&amp;", 5) == 0 ) {
                *pucStringWritePtr = '&';
                pucStringReadPtr += 5;
            }
            /* Convert "&gt;" to '>' */
            else if ( s_strncmp(pucStringReadPtr, "&gt;", 4) == 0 ) {
                *pucStringWritePtr = '>';
                pucStringReadPtr += 4;
            }
            /* Convert "&lt;" to '<' */
            else if ( s_strncmp(pucStringReadPtr, "&lt;", 4) == 0 ) {
                *pucStringWritePtr = '<';
                pucStringReadPtr += 4;
            }
            /* Convert "&apos;" to "'" */
            else if ( s_strncmp(pucStringReadPtr, "&apos;", 6) == 0 ) {
                *pucStringWritePtr = '\'';
                pucStringReadPtr += 6;
            }
            /* Convert "&quot;" to '"' */
            else if ( s_strncmp(pucStringReadPtr, "&quot;", 6) == 0 ) {
                *pucStringWritePtr = '"';
                pucStringReadPtr += 6;
            }
            /* Copy the character */
            else {
                *pucStringWritePtr = *pucStringReadPtr;
                pucStringReadPtr++;
            }
        }

        /* Copy the character */
        else {
            *pucStringWritePtr = *pucStringReadPtr;
            pucStringReadPtr++;
        }
    }

    /* NULL terminate the string */
    *pucStringWritePtr = '\0';
    

    /* Return */
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsEncodeXmlString()

    Purpose:    Encode a string to an XML string

    Parameters: pucString               string
                pucEncodedString        return pointer for the encoded string
                uiEncodedStringLength   length of the return pointer for the encoded string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsEncodeXmlString
(
    unsigned char *pucString,
    unsigned char *pucEncodedString,
    unsigned int uiEncodedStringLength
)
{

    unsigned char   *pucStringPtr = NULL;
    unsigned char   *pucEncodedStringPtr = NULL;
    

    /* Check the parameters */
    if ( pucEncodedString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucEncodedString' parameter passed to 'iUtlStringsEncodeXmlString'."); 
        return (UTL_StringsInvalidReturnParameter);
    }

    if ( uiEncodedStringLength <= 6 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEncodedStringLength' parameter passed to 'iUtlStringsEncodeXmlString'."); 
        return (UTL_StringsInvalidReturnParameter);
    }

    if ( bUtlStringsIsStringNULL(pucString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iUtlStringsEncodeXmlString'.");  */
/*         return (UTL_StringsInvalidString); */
        pucEncodedString[0] = '\0';
        return (UTL_NoError);
    }


    /* Encode the string, stop when we run out of string, or of space - note the '- 7' */
    for ( pucStringPtr = pucString, pucEncodedStringPtr = pucEncodedString; 
            (*pucStringPtr != '\0') && ((pucEncodedStringPtr - pucEncodedString) < (uiEncodedStringLength - 7)); 
            pucStringPtr++ ) {
        
        /* Convert '&' to "&amp;" */
        if ( *pucStringPtr == '&' ) {
            s_strcpy(pucEncodedStringPtr, "&amp;");
            pucEncodedStringPtr += 5;
        }
        /* Convert '>' to "&gt;" */
        else if ( *pucStringPtr == '>' ) {
            s_strcpy(pucEncodedStringPtr, "&gt;");
            pucEncodedStringPtr += 4;
        }
        /* Convert '<' to "&lt;" */
        else if ( *pucStringPtr == '<' ) {
            s_strcpy(pucEncodedStringPtr, "&lt;");
            pucEncodedStringPtr += 4;
        }
        /* Convert "'" to "&apos;" */
        else if ( *pucStringPtr == '\'' ) {
            s_strcpy(pucEncodedStringPtr, "&apos;");
            pucEncodedStringPtr += 6;
        }
        /* Convert '"' to "&quot;" */
        else if ( *pucStringPtr == '"' ) {
            s_strcpy(pucEncodedStringPtr, "&quot;");
            pucEncodedStringPtr += 6;
        }
        /* Copy the character */
        else {
            *pucEncodedStringPtr = *pucStringPtr;
            pucEncodedStringPtr++;
        }
    }
    
    /* NULL terminate the string */
    *pucEncodedStringPtr = '\0';


    /* Return */
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsDecodeXmlWideString()

    Purpose:    Decode an XML encoded wide string in place

    Parameters: pwcString       string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsDecodeXmlWideString
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringReadPtr = NULL;
    wchar_t *pwcStringWritePtr = NULL;
    

    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iUtlStringsDecodeXmlWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        return (UTL_NoError);
    }


    /* Decode the string */
    for ( pwcStringReadPtr = pwcString, pwcStringWritePtr = pwcString; *pwcStringReadPtr != L'\0'; pwcStringWritePtr++ ) {
        
        /* Check for an escape on '&' */
        if ( *pwcStringReadPtr == L'&' ) {

            /* Convert "&amp;" to '&' */
            if ( s_wcsncmp(pwcStringReadPtr, L"&amp;", 5) == 0 ) {
                *pwcStringWritePtr = L'&';
                pwcStringReadPtr += 5;
            }
            /* Convert "&gt;" to '>' */
            else if ( s_wcsncmp(pwcStringReadPtr, L"&gt;", 4) == 0 ) {
                *pwcStringWritePtr = L'>';
                pwcStringReadPtr += 4;
            }
            /* Convert "&lt;" to '<' */
            else if ( s_wcsncmp(pwcStringReadPtr, L"&lt;", 4) == 0 ) {
                *pwcStringWritePtr = L'<';
                pwcStringReadPtr += 4;
            }
            /* Convert "&apos;" to "'" */
            else if ( s_wcsncmp(pwcStringReadPtr, L"&apos;", 6) == 0 ) {
                *pwcStringWritePtr = L'\'';
                pwcStringReadPtr += 6;
            }
            /* Convert "&quot;" to '"' */
            else if ( s_wcsncmp(pwcStringReadPtr, L"&quot;", 6) == 0 ) {
                *pwcStringWritePtr = L'"';
                pwcStringReadPtr += 6;
            }
            /* Copy the character */
            else {
                *pwcStringWritePtr = *pwcStringReadPtr;
                pwcStringReadPtr++;
            }
        }

        /* Copy the character */
        else {
            *pwcStringWritePtr = *pwcStringReadPtr;
            pwcStringReadPtr++;
        }
    }

    /* NULL terminate the string */
    *pwcStringWritePtr = L'\0';
    

    /* Return */
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringsEncodeXmlWideString()

    Purpose:    Encode a string to an XML string

    Parameters: pwcString               string
                pucEncodedString        return pointer for the encoded string
                uiEncodedStringLength   length of the return pointer for the encoded string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringsEncodeXmlWideString
(
    wchar_t *pwcString,
    wchar_t *pwcEncodedString,
    unsigned int uiEncodedStringLength
)
{

    wchar_t     *pwcStringPtr = NULL;
    wchar_t     *pwcEncodedStringPtr = NULL;
    

    /* Check the parameters */
    if ( pwcEncodedString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pwcEncodedString' parameter passed to 'iUtlStringsEncodeXmlWideString'."); 
        return (UTL_StringsInvalidReturnParameter);
    }

    if ( uiEncodedStringLength <= 6 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEncodedStringLength' parameter passed to 'iUtlStringsEncodeXmlWideString'."); 
        return (UTL_StringsInvalidReturnParameter);
    }

    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iUtlStringsEncodeXmlWideString'.");  */
/*         return (UTL_StringsInvalidString); */
        pwcEncodedString[0] = L'\0';
        return (UTL_NoError);
    }


    /* Encode the string, stop when we run out of string, or of space - note the '- 7' */
    for ( pwcStringPtr = pwcString, pwcEncodedStringPtr = pwcEncodedString; 
            (*pwcStringPtr != L'\0') && ((pwcEncodedStringPtr - pwcEncodedString) < (uiEncodedStringLength - 7)); 
            pwcStringPtr++ ) {
        
        /* Convert '&' to "&amp;" */
        if ( *pwcStringPtr == L'&' ) {
            s_wcscpy(pwcEncodedStringPtr, L"&amp;");
            pwcEncodedStringPtr += 5;
        }
        /* Convert '>' to "&gt;" */
        else if ( *pwcStringPtr == '>' ) {
            s_wcscpy(pwcEncodedStringPtr, L"&gt;");
            pwcEncodedStringPtr += 4;
        }
        /* Convert '<' to "&lt;" */
        else if ( *pwcStringPtr == L'<' ) {
            s_wcscpy(pwcEncodedStringPtr, L"&lt;");
            pwcEncodedStringPtr += 4;
        }
        /* Convert "'" to "&apos;" */
        else if ( *pwcStringPtr == L'\'' ) {
            s_wcscpy(pwcEncodedStringPtr, L"&apos;");
            pwcEncodedStringPtr += 6;
        }
        /* Convert '"' to "&quot;" */
        else if ( *pwcStringPtr == L'"' ) {
            s_wcscpy(pwcEncodedStringPtr, L"&quot;");
            pwcEncodedStringPtr += 6;
        }
        /* Copy the character */
        else {
            *pwcEncodedStringPtr = *pwcStringPtr;
            pwcEncodedStringPtr++;
        }
    }
    
    /* NULL terminate the string */
    *pwcEncodedStringPtr = L'\0';


    /* Return */
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/

