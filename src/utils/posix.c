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

    Module:     posix.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 January 1994

    Purpose:    We put a number of things in this file, mainly to provide
                a uniform posix environment for the software to be built in,
                yes, believe it or not, some, so called, posix systems are not 
                really completely posix compliant (hear that Redmond!):

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.posix"


/*---------------------------------------------------------------------------*/


#if defined(UTL_POSIX_ENABLE_WCSDUP)

/*

    Function:   wcsdup()

    Purpose:    Return a copy of pwcStr.

    Parameters: pwcStr      The wide string to duplicate

    Globals:    none

    Returns:    A pointer to the duplicated wide string
*/
wchar_t *wcsdup
(
    wchar_t *pwcStr
)
{

    wchar_t     *pwcStrCopy = NULL;


    /* Dont crash on stupid errors */
    if ( pwcStr == NULL ) { 
        return (NULL);
    }


    /* Duplicate the string */
    if ( (pwcStrCopy = (wchar_t *)malloc((size_t)(sizeof(wchar_t) * (wcslen(pwcStr) + 1)))) != NULL ) {
        wcscpy(pwcStrCopy, pwcStr);
    }


    return (pwcStrCopy);

}

#endif    /* defined(UTL_POSIX_ENABLE_WCSDUP) */


/*---------------------------------------------------------------------------*/


#if defined(UTL_POSIX_ENABLE_WCSCASECMP)

/*

    Function:   wcscasecmp()

    Purpose:    Compares the wide strings in a caseless fashion, up to n
                character

    Parameters: pwcStr1     First string
                pwcStr2     Second string

    Globals:    none

    Returns:    0 if they are the same, positive integer if pwcPtr1 > pwcPtr2,
                negative integer if pwcPtr2 > pwcPtr1

*/
int wcscasecmp
(
    wchar_t *pwcStr1,
    wchar_t *pwcStr2
)
{

    wchar_t     *pwcPtr1 = NULL;
    wchar_t     *pwcPtr2 = NULL;
    int         iDiff = 0;


    for ( pwcPtr1 = pwcStr1, pwcPtr2 = pwcStr2; (*pwcPtr1 != L'\0') && (*pwcPtr2 != L'\0'); pwcPtr1++, pwcPtr2++ ) {
        if ( (iDiff = towlower(*pwcPtr1) - towlower(*pwcPtr2)) != 0) {
            return (iDiff);
        }
    }

    /* PucPtr1 was longer than pwcPtr2 */
    if ( *pwcPtr1 != L'\0' ) {
        return (1); 
    }

    /* PucPtr1 was shorter than pwcPtr2 */
    if ( *pwcPtr2 != L'\0' ) {
        return (-1);
    }

    /* Exact match */
    return (0);

}

#endif    /* defined(UTL_POSIX_ENABLE_WCSCASECMP) */


/*---------------------------------------------------------------------------*/


#if defined(UTL_POSIX_ENABLE_WCSNCASECMP)

/*

    Function:   wcsncasecmp()

    Purpose:    Compares the strings up until one of them ends.
                returns true if they are the same, false if not.

    Parameters: pwcStr1     First string
                pwcStr2     Second string
                zCount      Number of characters to compare

    Globals:    none

    Returns:    0 if they are the same, positive integer if pwcPtr1 > pwcPtr2,
                negative integer if pwcPtr2 > pwcPtr1

*/
int wcsncasecmp
(
    wchar_t *pwcStr1,
    wchar_t *pwcStr2,
    size_t zCount
)
{

    wchar_t     *pwcPtr1 = NULL;
    wchar_t     *pwcPtr2 = NULL;
    int         iDiff = 0;


    for ( pwcPtr1 = pwcStr1, pwcPtr2 = pwcStr2; /* Nothing */; pwcPtr1++, pwcPtr2++ ) {

        /* Match up to zCount characters */
        if (pwcPtr1 == (pwcStr1 + zCount) ) {
            return (0);
        }

        if ( (*pwcPtr1 == L'\0') || (*pwcPtr2 == L'\0') ) {
            return (*pwcPtr1 - *pwcPtr2);
        }

        if ( (iDiff = towlower(*pwcPtr1) - towlower(*pwcPtr2)) != 0 ) {
            return (iDiff);
          }
    }

}

#endif    /* defined(UTL_POSIX_ENABLE_WCSNCASECMP) */


/*---------------------------------------------------------------------------*/
