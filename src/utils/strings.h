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

    Module:     strings.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 April 2004

    Purpose:    This is the header file for strings.c. This file contains a
                lot of useful strings functions.

*/


#if !defined(UTL_STRINGS_H)
#define UTL_STRINGS_H


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
** Feature defines
*/

/* We automatically turn on strings debugging if DEBUG is set */
#if defined(DEBUG)
#define DEBUG_STRINGS
#endif    /* defined(DEBUG) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Buffer size for the scanf(), sscanf() and fscanf() format string length */
#define    UTL_STRING_SCANF_FORMAT_LENGTH               (512)


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

/* Check for NULL or empty string */
#define bUtlStringsIsStringNULL(s)                      (!((s != NULL) && (s[0] != '\0')))

/* Get a NULL string, returns NULL if the string pointer is NULL or the string is empty */
#define pucUtlStringsGetNULLString(s)                   ((bUtlStringsIsStringNULL(s) == false) ? ((unsigned char *)s) : (unsigned char *)NULL)

/* Get a safe string, returns "" if the string pointer is NULL */
#define pucUtlStringsGetSafeString(s)                   ((s != NULL) ? ((unsigned char *)s) : (unsigned char *)"")

/* Get a printable string, returns "(null)" if the string pointer is NULL or the string is empty */
#define pucUtlStringsGetPrintableString(s)              ((bUtlStringsIsStringNULL(s) == false) ? ((unsigned char *)s) : (unsigned char *)"(null)")

/* Get a string with default, returns (d) if the string pointer is NULL or the string is empty */
#define pucUtlStringsGetDefaultString(s, d)             ((bUtlStringsIsStringNULL(s) == false) ? (s) : (d))



/* Check for NULL or empty wide string */
#define bUtlStringsIsWideStringNULL(s)                 (!((s != NULL) && (s[0] != L'\0')))

/* Get a NULL wide string, returns NULL if the wide string pointer is NULL or the string is empty */
#define pwcUtlStringsGetNULLWideString(s)               ((bUtlStringsIsWStringNULL(s) == false) ? (s) : NULL)

/* Get a safe wide string, returns "" if the wide string pointer is NULL */
#define pwcUtlStringsGetSafeWideString(s)               ((s != NULL) ? (s) : L"")

/* Get a printable wide string, returns "(null)" if the wide string pointer is NULL or the wide string is empty */
#define pwcUtlStringsGetPrintableWideString(s)          ((bUtlStringsIsWideStringNULL(s) == false) ? (s) : L"(null)")

/* Get a wide string with default, returns (d) if the wide string pointer is NULL or the wide string is empty */
#define pwcUtlStringsGetDefaultWideString(s, d)         ((bUtlStringsIsWideStringNULL(s) == false) ? (s) : (d))


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

/* Enhanced string handling functions - dont use those, use the macros below */
#if defined(DEBUG_STRINGS)
unsigned char   *fs_strnncat (unsigned char *pucDestination, unsigned char *pucSource, size_t zMaxToAdd, size_t zMaxTotal, char *pcFile, size_t zLine);
unsigned char   *fs_strnncpy (unsigned char *pucDestination, unsigned char *pucSource, size_t zDestinationLength, char *pcFile, size_t zLine);
int             fs_substrcmp (unsigned char *pucString1, unsigned char *pucString2, char *pcFile, size_t zLine);
unsigned char   *fs_strcasestr (unsigned char *pucString1, unsigned char *pucString2, char *pcFile, size_t zLine);
#else
unsigned char   *fs_strnncat (unsigned char *pucDestination, unsigned char *pucSource, size_t zMaxToAdd, size_t zMaxTotal);
unsigned char   *fs_strnncpy (unsigned char *pucDestination, unsigned char *pucSource, size_t zDestinationLength);
int             fs_substrcmp (unsigned char *pucString1, unsigned char *pucString2);
unsigned char   *fs_strcasestr (unsigned char *pucString1, unsigned char *pucString2);
#endif    /* defined(DEBUG_STRINGS) */

/* Enhanced string handling macros - use these */
#if defined(DEBUG_STRINGS)
#define s_strnncat(s1, s2, n, m)                    fs_strnncat((s1), (s2), (n), (m), (__FILE__), (__LINE__))
#define s_strnncpy(s1, s2, n)                       fs_strnncpy((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_substrcmp(s1, s2)                         fs_substrcmp((s1), (s2), (__FILE__), (__LINE__))
#define s_strcasestr(s1, s2)                        fs_strcasestr((s1), (s2), (__FILE__), (__LINE__))
#else
#define s_strnncat(s1, s2, n, m)                    fs_strnncat((s1), (s2), (n), (m))
#define s_strnncpy(s1, s2, n)                       fs_strnncpy((s1), (s2), (n))
#define s_substrcmp(s1, s2)                         fs_substrcmp((s1), (s2))
#define s_strcasestr(s1, s2)                        fs_strcasestr((s1), (s2))
#endif    /* defined(DEBUG_STRINGS) */



/* Enhanced wide string handling functions - dont use those, use the macros below */
#if defined(DEBUG_STRINGS)
wchar_t         *fs_wcsnncat (wchar_t *pwcDestination, wchar_t *pwcSource, size_t zMaxToAdd, size_t zMaxTotal, char *pcFile, size_t zLine);
wchar_t         *fs_wcsnncpy (wchar_t *pwcDestination, wchar_t *pwcSource, size_t zDestinationLength, char *pcFile, size_t zLine);
int             fs_subwcscmp (wchar_t *pwcString1, wchar_t *pwcString2, char *pcFile, size_t zLine);
wchar_t         *fs_wcscasestr (wchar_t *pwcString1, wchar_t *pwcString2, char *pcFile, size_t zLine);
#else
wchar_t         *fs_wcsnncat (wchar_t *pwcDestination, wchar_t *pwcSource, size_t zMaxToAdd, size_t zMaxTotal);
wchar_t         *fs_wcsnncpy (wchar_t *pwcDestination, wchar_t *pwcSource, size_t zDestinationLength);
int             fs_subwcscmp (wchar_t *pwcString1, wchar_t *pwcString2);
wchar_t         *fs_wcscasestr (wchar_t *pwcString1, wchar_t *pwcString2);
#endif    /* defined(DEBUG_STRINGS) */

/* Enhanced string handling macros - use these */
#if defined(DEBUG_STRINGS)
#define s_wcsnncat(s1, s2, n, m)                    fs_wcsnncat((s1), (s2), (n), (m), (__FILE__), (__LINE__))
#define s_wcsnncpy(s1, s2, n)                       fs_wcsnncpy((s1), (s2), (n), (__FILE__), (__LINE__))
#define s_subwcscmp(s1, s2)                         fs_subwcscmp((s1), (s2), (__FILE__), (__LINE__))
#define s_wcscasestr(s1, s2)                        fs_wcscasestr((s1), (s2), (__FILE__), (__LINE__))
#else
#define s_wcsnncat(s1, s2, n, m)                    fs_wcsnncat((s1), (s2), (n), (m))
#define s_wcsnncpy(s1, s2, n)                       fs_wcsnncpy((s1), (s2), (n))
#define s_subwcscmp(s1, s2)                         fs_subwcscmp((s1), (s2))
#define s_wcscasestr(s1, s2)                        fs_wcscasestr((s1), (s2))
#endif    /* defined(DEBUG_STRINGS) */


/* Number formatting */
unsigned char *pucUtlStringsFormatUnsignedNumber (unsigned long ulNumber, unsigned char *pucString, unsigned int uiStringLength);
unsigned char *pucUtlStringsFormatSignedNumber (long lNumber, unsigned char *pucString, unsigned int uiStringLength);
unsigned char *pucUtlStringsFormatNumberString (unsigned char *pucNumber, unsigned char *pucString, unsigned int uiStringLength);
unsigned char *pucUtlStringsFormatDoubleAsBytes (double dNumber, unsigned char *pucString, unsigned int uiStringLength);


/* Functions to trim/clean strings */
int iUtlStringsTrimString (unsigned char *pucString);
int iUtlStringsTrimWideString (wchar_t *pwcString);

int iUtlStringsRemoveCharacterFromString (unsigned char *pucString, unsigned char ucCharacter);
int iUtlStringsRemoveCharacterFromWideString (wchar_t *pwcString, wchar_t wcCharacter);

int iUtlStringsRemoveCharactersFromString (unsigned char *pucString, unsigned char *pucCharacters);
int iUtlStringsRemoveCharactersFromWideString (wchar_t *pwcString, wchar_t *pwcCharacters);

int iUtlStringsReplaceCharacterInString (unsigned char *pucString, unsigned char ucCharacter, unsigned char ucNewCharacter);
int iUtlStringsReplaceCharacterInWideString (wchar_t *pwcString, wchar_t wcCharacters, wchar_t wcNewCharacter);

int iUtlStringsReplaceCharactersInString (unsigned char *pucString, unsigned char *pucCharacters, unsigned char ucCharacter);
int iUtlStringsReplaceCharactersInWideString (wchar_t *pwcString, wchar_t *pwcCharacters, wchar_t wcCharacter);

int iUtlStringsReplaceStringInString (unsigned char *pucSourceString, unsigned char *pucOriginalString, unsigned char *pucReplacementString, 
        unsigned char *pucDestinationString, unsigned int uiDestinationStringLength);
int iUtlStringsReplaceStringInWideString (wchar_t *pwcSourceString, wchar_t *pwcOriginalString, wchar_t *pwcReplacementString, 
        wchar_t *pwcDestinationString, unsigned int uiDestinationStringLength);


/* Functions to concatenate strings */
int iUtlStringsAppendString (unsigned char *pucDestination, unsigned char *pucSource, unsigned char **ppucDestination);
int iUtlStringsAppendData (unsigned char *pucDestination, void *pvSource, unsigned int uiSourceLength, 
        unsigned char **ppucDestination);


/* Functions to NULL terminate data */
int iUtlStringsNullTerminateData (void *pvSource, unsigned int uiSourceLength, 
        unsigned char **ppucDestination);


/* Functions to test whether a string is a URL */
boolean bUtlStringsIsStringUrl (unsigned char *pucString);
boolean bUtlStringsIsWideStringUrl (wchar_t *pwcString);


/* Functions decode and encode xml strings */
int iUtlStringsDecodeXmlString(unsigned char *pucString);
int iUtlStringsEncodeXmlString(unsigned char *pucString, unsigned char *pucEncodedString,
    unsigned int uiEncodedStringLength);

int iUtlStringsDecodeXmlWideString(wchar_t *pwcString);
int iUtlStringsEncodeXmlWideString(wchar_t *pwcString, wchar_t *pwcEncodeString,
    unsigned int uiEncodedStringLength);

/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_STRINGS_H) */


/*---------------------------------------------------------------------------*/
