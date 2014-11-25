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

    Module:     unicode.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    19 November 2005

    Purpose:    This file contains various unicode functions

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "lng.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.unicode"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Enable ICU for normalization only if ICU is gobally enabled */
#if defined(MPS_ENABLE_ICU)

/* To enable normalization with ICU */
#define LNG_UNICODE_NORMALIZATION_ENABLE_ICU

#endif    /* defined(MPS_ENABLE_ICU) */


/*---------------------------------------------------------------------------*/


/*
** Define specific includes
*/

/* ICU specific includes */
#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

#include "IcuWrappers.h"

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/


/* Unicode normalizer structure */
struct lngUnicodeNomalizer {

/* ICU normalizer */
#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

    void            *pvIcuNormalizer;

    void            *pvLngConverterUTF8ToWChar;
    void            *pvLngConverterWCharToUTF8;

    unsigned char   *pucBuffer;
    unsigned int    uiBufferLength;
    unsigned int    uiStringLength;

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */

};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

/* ICU normalizer */
#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

static int iLngUnicodeNormalizeString_icu (struct lngUnicodeNomalizer *plunLngUnicodeNomalizer, 
        unsigned char *pucString, unsigned int uiStringLength,
        unsigned char **ppucNormalizedString, unsigned int *puiNormalizedStringLength);

static int iLngUnicodeNormalizeWideString_icu (struct lngUnicodeNomalizer *plunLngUnicodeNomalizer, 
        wchar_t *pwcString, unsigned int uiStringLength,
        wchar_t **ppwcNormalizedString, unsigned int *puiNormalizedStringLength);

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeNormalizerCreate()

    Purpose:    This function created a unicode normalizer structure.

    Parameters: pucConfigurationDirectoryPath   Configuration directory path (optional)
                ppvLngUnicodeNormalizer         Return pointer for the unicode normalizer structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngUnicodeNormalizerCreate
(
    unsigned char *pucConfigurationDirectoryPath,
    void **ppvLngUnicodeNormalizer
)
{

    int                             iError = LNG_NoError;
    struct lngUnicodeNomalizer      *plunLngUnicodeNomalizer = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucConfigurationDirectoryPath' parameter passed to 'iLngUnicodeNormalizerCreate'."); 
        return (LNG_UnicodeInvalidConfigurationDirectoryPath);
    }

    if ( ppvLngUnicodeNormalizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngUnicodeNormalizer' parameter passed to 'iLngUnicodeNormalizerCreate'."); 
        return (LNG_ReturnParameterError);
    }

    
    /* Preset the error - this is the catch-all error if there is no normalizers defined */
    iError = LNG_UnicodeNormalizationUnsupported;


/* ICU normalizer */
#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

    /* Allocate the unicode normalizer structure */
    if ( (plunLngUnicodeNomalizer = (struct lngUnicodeNomalizer *)s_malloc((size_t)(sizeof(struct lngUnicodeNomalizer)))) == NULL ) {
        iError = LNG_MemError;
        goto bailFromiLngUnicodeNormalizerCreate;
    }

    /* Clear the unicode normalizer structure ICU specific fields */
    plunLngUnicodeNomalizer->pucBuffer = NULL;
    plunLngUnicodeNomalizer->uiBufferLength = 0;
    plunLngUnicodeNomalizer->uiStringLength = 0;


    /* Create underlying Unicode Normalizer */
    if ( (iError = iIcuNormalizerCreate(pucConfigurationDirectoryPath, &plunLngUnicodeNomalizer->pvIcuNormalizer)) != ICU_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the ICU unicode normalizer, icu error: %d.", iError);
        iError = LNG_UnicodeFailedNormalizerCreate;
        goto bailFromiLngUnicodeNormalizerCreate;
    }
    
    /* Create a character set converter to convert from utf-8 to wide characters */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_UTF_8_NAME, LNG_CHARACTER_SET_WCHAR_NAME, &plunLngUnicodeNomalizer->pvLngConverterUTF8ToWChar)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from utf-8 to wide characters, lng error: %d.", iError);
        goto bailFromiLngUnicodeNormalizerCreate;
    }

    /* Create a character set converter to convert from wide characters to utf-8 */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_WCHAR_NAME, LNG_CHARACTER_SET_UTF_8_NAME, &plunLngUnicodeNomalizer->pvLngConverterWCharToUTF8)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from wide characters to utf-8, lng error: %d.", iError);
        goto bailFromiLngUnicodeNormalizerCreate;
    }

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */



    /* Bail label */
    bailFromiLngUnicodeNormalizerCreate:
    
    
    /* Handle the error */
    if ( iError == LNG_NoError ) {

        /* Set the return pointer */
        *ppvLngUnicodeNormalizer = (void *)plunLngUnicodeNomalizer;
    }
    else {

        /* Free the normalizer */
        iLngUnicodeNormalizerFree(plunLngUnicodeNomalizer);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeNormalizerFree()

    Purpose:    This function frees a unicode normalizer structure.

    Parameters: pvLngUnicodeNormalizer      Unicode normalizer structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngUnicodeNormalizerFree
(
    void *pvLngUnicodeNormalizer
)
{
    struct lngUnicodeNomalizer      *plunLngUnicodeNomalizer = (struct lngUnicodeNomalizer *)pvLngUnicodeNormalizer;


    /* Check the unicode normalizer structure */
    if ( plunLngUnicodeNomalizer == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvLngUnicodeNormalizer' parameter passed to 'iLngUnicodeNormalizerFree'."); 
        return (LNG_UnicodeInvalidNormalizer);
    }
    

/* ICU normalizer */
#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

    /* Free the character set converters */
    iLngConverterFree(plunLngUnicodeNomalizer->pvLngConverterUTF8ToWChar);
    plunLngUnicodeNomalizer->pvLngConverterUTF8ToWChar = NULL;
    
    iLngConverterFree(plunLngUnicodeNomalizer->pvLngConverterWCharToUTF8);
    plunLngUnicodeNomalizer->pvLngConverterWCharToUTF8 = NULL;

    /* Free the buffer */
    s_free(plunLngUnicodeNomalizer->pucBuffer);

    /* Free the ICU normalizer */
    iIcuNormalizerFree(plunLngUnicodeNomalizer->pvIcuNormalizer);
    plunLngUnicodeNomalizer->pvIcuNormalizer = NULL;

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */


    /* Free the unicode normalizer structure */
    s_free(plunLngUnicodeNomalizer);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeNormalizeString()

    Purpose:    This function normalizes a unicode string.

    Parameters: pvLngUnicodeNormalizer      Unicode normalizer structure
                pucString                   String to normalize
                uiStringLength              String length
                ppucNormalizedString        Destination string/return pointer for the normalized string
                puiNormalizedStringLength   Destination string length/return pointer
                                            (set to capacity if destination string is allocated)

    Globals:    none

    Returns:    An LNG error code

*/
int iLngUnicodeNormalizeString
(
    void *pvLngUnicodeNormalizer,
    unsigned char *pucString, 
    unsigned int uiStringLength,
    unsigned char **ppucNormalizedString, 
    unsigned int *puiNormalizedStringLength
)
{

    struct lngUnicodeNomalizer      *plunLngUnicodeNomalizer = (struct lngUnicodeNomalizer *)pvLngUnicodeNormalizer;


    /* Check the unicode normalizer structure */
    if ( plunLngUnicodeNomalizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngUnicodeNormalizer' parameter passed to 'iLngUnicodeNormalizeString'."); 
        return (LNG_UnicodeInvalidNormalizer);
    }

    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iLngUnicodeNormalizeString'."); 
        return (LNG_UnicodeInvalidString);
    }

    if ( uiStringLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStringLength' parameter passed to 'iLngUnicodeNormalizeString'."); 
        return (LNG_UnicodeInvalidStringLength);
    }

    if ( ppucNormalizedString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucNormalizedString' parameter passed to 'iLngUnicodeNormalizeString'."); 
        return (LNG_UnicodeInvalidNormalizedString);
    }

    if ( (*ppucNormalizedString != NULL) && (*puiNormalizedStringLength <= 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'puiNormalizedStringLength' parameter passed to 'iLngUnicodeNormalizeString'."); 
        return (LNG_UnicodeInvalidNormalizedStringLength);
    }


/* ICU normalizer */
#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

    return (iLngUnicodeNormalizeString_icu(plunLngUnicodeNomalizer, pucString, uiStringLength, ppucNormalizedString, puiNormalizedStringLength));

#else    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */

    /* Normalization is not supported */
    return (LNG_UnicodeUnsupportedNormalizationMode);

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */


}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeNormalizeWideString()

    Purpose:    This function normalizes a unicode string.

    Parameters: pvLngUnicodeNormalizer      Unicode normalizer structure
                pwcString                   String to normalize
                uiStringLength              String length
                ppwcNormalizedString        Destination string/return pointer for the normalized string
                puiNormalizedStringLength   Destination string length/return pointer
                                            (set to capacity if destination string is allocated)

    Globals:    none

    Returns:    An LNG error code

*/
int iLngUnicodeNormalizeWideString
(
    void *pvLngUnicodeNormalizer,
    wchar_t *pwcString, 
    unsigned int uiStringLength,
    wchar_t **ppwcNormalizedString, 
    unsigned int *puiNormalizedStringLength
)
{

    struct lngUnicodeNomalizer      *plunLngUnicodeNomalizer = (struct lngUnicodeNomalizer *)pvLngUnicodeNormalizer;


    /* Check the unicode normalizer structure */
    if ( plunLngUnicodeNomalizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngUnicodeNormalizer' parameter passed to 'iLngUnicodeNormalizeWideString'."); 
        return (LNG_UnicodeInvalidNormalizer);
    }

    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iLngUnicodeNormalizeWideString'."); 
        return (LNG_UnicodeInvalidString);
    }

    if ( uiStringLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStringLength' parameter passed to 'iLngUnicodeNormalizeWideString'."); 
        return (LNG_UnicodeInvalidStringLength);
    }

    if ( ppwcNormalizedString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcNormalizedString' parameter passed to 'iLngUnicodeNormalizeWideString'."); 
        return (LNG_UnicodeInvalidNormalizedString);
    }

    if ( (*ppwcNormalizedString != NULL) && (*puiNormalizedStringLength <= 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'puiNormalizedStringLength' parameter passed to 'iLngUnicodeNormalizeWideString'."); 
        return (LNG_UnicodeInvalidNormalizedStringLength);
    }


/* ICU normalizer */
#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

    return (iLngUnicodeNormalizeWideString_icu(plunLngUnicodeNomalizer, pwcString, uiStringLength, ppwcNormalizedString, puiNormalizedStringLength));

#else    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */

    /* Normalization is not supported */
    return (LNG_UnicodeUnsupportedNormalizationMode);

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#if defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU)

/*

    Function:   iLngUnicodeNormalizeString_icu()

    Purpose:    This function normalizes a unicode string.

    Parameters: plunLngUnicodeNomalizer     Unicode normalizer structure
                pucString                   String to normalize
                uiStringLength              String length
                ppucNormalizedString        Destination string/return pointer for the normalized string
                puiNormalizedStringLength   Destination string length/return pointer
                                            (set to capacity if destination string is allocated)

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngUnicodeNormalizeString_icu
(
    struct lngUnicodeNomalizer *plunLngUnicodeNomalizer, 
    unsigned char *pucString, 
    unsigned int uiStringLength,
    unsigned char **ppucNormalizedString, 
    unsigned int *puiNormalizedStringLength
)
{

    int     iError = LNG_NoError;


    /* Asserts */
    ASSERT(plunLngUnicodeNomalizer != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucString) == false);
    ASSERT(uiStringLength > 0);
    ASSERT(pucString != NULL);
    ASSERT((*ppucNormalizedString == NULL) || ((*ppucNormalizedString != NULL) && (*puiNormalizedStringLength > 0)));
    

    /* Normalize, let the normalizer allocate the return string for us */
    if ( (iError = iIcuNormalizerNormalizeString(plunLngUnicodeNomalizer->pvIcuNormalizer, pucString, uiStringLength, ppucNormalizedString, puiNormalizedStringLength)) != ICU_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to normalize a string with the ICU unicode normalizer, icu error: %d.", iError);
        iError = LNG_UnicodeFailedNormalizeString;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeNormalizeWideString_icu()

    Purpose:    This function normalizes a unicode string.

    Parameters: plunLngUnicodeNomalizer     Unicode normalizer structure
                pwcString                   String to normalize
                uiStringLength              String length
                ppwcNormalizedString        Destination string/return pointer for the normalized string
                puiNormalizedStringLength   Destination string length/return pointer
                                            (set to capacity if destination string is allocated)

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngUnicodeNormalizeWideString_icu
(
    struct lngUnicodeNomalizer *plunLngUnicodeNomalizer, 
    wchar_t *pwcString, 
    unsigned int uiStringLength,
    wchar_t **ppwcNormalizedString, 
    unsigned int *puiNormalizedStringLength
)
{

    int             iError = LNG_NoError;
    unsigned char   *pucNormalizedString = NULL;
    unsigned int    uiNormalizedStringLength = 0;


    /* Asserts */
    ASSERT(plunLngUnicodeNomalizer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcString) == false);
    ASSERT(uiStringLength > 0);
    ASSERT(ppwcNormalizedString != NULL);
    ASSERT((*ppwcNormalizedString == NULL) || ((*ppwcNormalizedString != NULL) && (*puiNormalizedStringLength > 0)));
    

    /* Extend the buffer if needed */
    if ( ((uiStringLength + 1) * LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER) > plunLngUnicodeNomalizer->uiBufferLength ) {
        
        unsigned char   *pucBufferPtr = NULL;
        
        /* Set the buffer length */
        plunLngUnicodeNomalizer->uiBufferLength = (uiStringLength + 1) * LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER;
    
        /* Extend the buffer */
        if ( (pucBufferPtr = (unsigned char *)s_realloc(plunLngUnicodeNomalizer->pucBuffer, (size_t)(sizeof(unsigned char) * plunLngUnicodeNomalizer->uiBufferLength))) == NULL ) {
            return (LNG_MemError);
        }

        /* Hand over the pointer */
        plunLngUnicodeNomalizer->pucBuffer = pucBufferPtr; 
    }


    /* Reset the buffer */
    plunLngUnicodeNomalizer->pucBuffer[0] = '\0';


    /* Convert the string from wide characters to utf-8 */
    if ( uiStringLength > 0 ) {
        
        /* Use a separate variable for the buffer length */
        plunLngUnicodeNomalizer->uiStringLength = plunLngUnicodeNomalizer->uiBufferLength;
        
        /* Convert the text */
        if ( (iError = iLngConverterConvertString(plunLngUnicodeNomalizer->pvLngConverterWCharToUTF8, LNG_CONVERTER_RETURN_ON_ERROR, (unsigned char *)pwcString, uiStringLength * sizeof(wchar_t), 
                (unsigned char **)&plunLngUnicodeNomalizer->pucBuffer, &plunLngUnicodeNomalizer->uiStringLength)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert text from wide characters to utf-8, lng error: %d.", iError);
            goto bailFromiLngUnicodeNormalizeWideString_icu;
        }
    }


    /* Normalize, let the normalizer allocate the return string for us */
    if ( (iError = iIcuNormalizerNormalizeString(plunLngUnicodeNomalizer->pvIcuNormalizer, plunLngUnicodeNomalizer->pucBuffer, plunLngUnicodeNomalizer->uiStringLength, 
            &pucNormalizedString, &uiNormalizedStringLength)) != ICU_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to normalize a string with the ICU unicode normalizer, icu error: %d.", iError);
        iError = LNG_UnicodeFailedNormalizeString;
        goto bailFromiLngUnicodeNormalizeWideString_icu;
    }


    /* Convert the string from utf-8 to wide characters */
    if ( uiStringLength > 0 ) {
        
        /* Use a separate variables for the passed string pointer and passed string pointer length */
        wchar_t         *pwcPassedNormalizedString = NULL;
        unsigned int    uiPassedNormalizedStringByteLength = 0;

        /* Set the pointers, if pointers were passed, note that iLngConverterConvertString() needs actual byte lengths */
        if ( *ppwcNormalizedString != NULL ) {
            pwcPassedNormalizedString = *ppwcNormalizedString;
            uiPassedNormalizedStringByteLength = *puiNormalizedStringLength * sizeof(wchar_t);
        }

        /* Convert the text */
        if ( (iError = iLngConverterConvertString(plunLngUnicodeNomalizer->pvLngConverterUTF8ToWChar, LNG_CONVERTER_RETURN_ON_ERROR, pucNormalizedString, uiNormalizedStringLength, 
                (unsigned char **)&pwcPassedNormalizedString, &uiPassedNormalizedStringByteLength)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the text from utf-8 to wide characters, lng error: %d.", iError);
            goto bailFromiLngUnicodeNormalizeWideString_icu;
        }

        /* Set the return pointers, if pointers were not passed, note that we need to adjust the returned lengths */
        if ( *ppwcNormalizedString == NULL ) {
            *ppwcNormalizedString = pwcPassedNormalizedString;
            *puiNormalizedStringLength = uiPassedNormalizedStringByteLength / sizeof(wchar_t);
        }
        else {
            *puiNormalizedStringLength = uiPassedNormalizedStringByteLength / sizeof(wchar_t);
        }
    }



    /* Bail label */
    bailFromiLngUnicodeNormalizeWideString_icu:
    
    /* Free the normalized string */
    s_free(pucNormalizedString);


    return (iError);

}

#endif    /* defined(LNG_UNICODE_NORMALIZATION_ENABLE_ICU) */


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeGetCharacterLengthFromUtf8String()

    Purpose:    Return the wide character length of a UTF-8 string

    Parameters: pucString   String

    Globals:    none

    Returns:    The length of the string or -1 on error 

*/
int iLngUnicodeGetCharacterLengthFromUtf8String
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = NULL;
    unsigned int    uiStringLength = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iLngUnicodeGetCharacterLengthFromUtf8String'."); 
        return (LNG_UnicodeInvalidString);
    }


    /* Loop over each byte in the string */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) {
    
        /* Single byte encoding - check for 0xxx xxxx */
        if ( (*pucStringPtr & 0x80) == 0x00 ) {
            uiStringLength++;
        }
        
        /* Multibyte encoding - check for 11xx xxxx */
        else if ( (*pucStringPtr & 0xC0) == 0xC0 ) {
            uiStringLength++;
        }
    }
  
 
    return (uiStringLength);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeTruncateUtf8String()

    Purpose:    Truncate a UTF-8 string on a wide character boundary, so the 
                maximum length of the resulting string will be uiMaxStringLength
                or less depending on where the boundary fell

    Parameters: pucString           String
                uiMaxStringLength   Maximum string length

    Globals:    none

    Returns:    UTL error code

*/
int iLngUnicodeTruncateUtf8String
(
    unsigned char *pucString,
    unsigned int uiMaxStringLength
)
{

    unsigned char   *pucStringPtr = NULL;
    unsigned char   *pucStringBoundaryPtr = NULL;
    unsigned int    uiStringLength = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iLngUnicodeTruncateUtf8String'."); 
        return (LNG_UnicodeInvalidString);
    }
    
    
    /* Loop over each byte in the string */
    for ( pucStringPtr = pucString, uiStringLength = 0, pucStringBoundaryPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++, uiStringLength++ ) {
    
        if ( uiStringLength >= uiMaxStringLength ) {
            *pucStringBoundaryPtr = '\0';
            break;
        }

        /* Single byte encoding - check for 0xxx xxxx */
        if ( (*pucStringPtr & 0x80) == 0x00 ) {
            pucStringBoundaryPtr = pucStringPtr;
        }
        
        /* Multibyte encoding - check for 11xx xxxx */
        else if ( (*pucStringPtr & 0xC0) == 0xC0 ) {
            pucStringBoundaryPtr = pucStringPtr;
        }        
    }
 

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeValidateUtf8String()

    Purpose:    Verify a UTF-8 string.

    Parameters: pucString   String

    Globals:    none

    Returns:    UTL error code

*/
int iLngUnicodeValidateUtf8String
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iLngUnicodeValidateUtf8String'."); 
        return (LNG_UnicodeInvalidString);
    }
    
    
    /* Loop over each byte in the string */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; ) {

        unsigned int    uiByteCount = 0;

        /* 0xxxxxxx - single byte character */
        if ( *pucStringPtr <= 127 ) {
            uiByteCount = 1;
        }
        
        /* 110xxxxx - 2 byte header */
        else if ( (*pucStringPtr >= 192) && (*pucStringPtr <= 223) ) {
            uiByteCount = 2;
        }

        /* 1110xxxx - 3 byte header */
        else if ( (*pucStringPtr >= 224) && (*pucStringPtr <= 239) ) {
            uiByteCount = 3;
        }
    
        /* 11110xxx - 4 byte header */
        else if ( (*pucStringPtr >= 240) && (*pucStringPtr <= 247) ) {
            uiByteCount = 4;
        }
    
        /* 111110xx - 5 byte header */
        else if ( (*pucStringPtr >= 240) && (*pucStringPtr <= 251) ) {
            uiByteCount = 5;
        }
    
        /* 1111110x - 6 byte header */
        else if ( (*pucStringPtr >= 252) && (*pucStringPtr <= 253) ) {
            uiByteCount = 6;
        }
        
        /* Invalid byte - return NULL */
        else {
            return (LNG_UnicodeInvalidUtf8String);
        }
        
        
        /* Skip to the next byte if this is a single byte character */
        if ( uiByteCount == 1 ) {
            pucStringPtr++;
        }

        /* Otherwise loop over the trailer character bytes as this is a multibyte character */
        else {
            
            unsigned int    uiI = 0;
            
            /* Loop over the trailer character bytes */
            for ( uiI = 1, pucStringPtr++; uiI < uiByteCount; uiI++ ) {
            
                /* 10xxxxxx - character byte */
                if ( (*pucStringPtr >= 128) && (*pucStringPtr <= 191) ) {
                    pucStringPtr++;
                }
            
                /* Invalid byte - return NULL */
                else {
                    return (LNG_UnicodeInvalidUtf8String);
                }
            }
        }
    }
 

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngUnicodeCleanUtf8String()

    Purpose:    Clean a UTF-8 string.

    Parameters: pucString   String

    Globals:    none

    Returns:    UTL error code

*/
int iLngUnicodeCleanUtf8String
(
    unsigned char *pucString,
    unsigned char ucReplacementByte
)
{

    unsigned char   *pucStringPtr = NULL;

/* iUtlLogInfo(UTL_LOG_CONTEXT, ">>>>>>>[%d] [%s].", s_strlen(pucString), pucString);  */

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iLngUnicodeCleanUtf8String'."); 
        return (LNG_UnicodeInvalidString);
    }
    

    /* Loop over each byte in the string */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; ) {

        unsigned int    uiByteCount = 1;

/* iUtlLogInfo(UTL_LOG_CONTEXT, "[%d][%c][%X][%d]", pucStringPtr - pucString, *pucStringPtr, *pucStringPtr, *pucStringPtr);  */

        /* 0xxxxxxx - 7F - single byte character, skip and continue */
        if ( *pucStringPtr <= 127 ) {
            pucStringPtr++;
            continue;
        }
        
        /* 11000000-11000001 - dead zone, replace and continue */
        else if ( (*pucStringPtr >= 192) && (*pucStringPtr <= 193) ) {
/* iUtlLogInfo(UTL_LOG_CONTEXT, "dead zone");  */
            *pucStringPtr = ucReplacementByte;
            pucStringPtr++;
            continue;
        }

        /* 110xxxxx - 2 byte header */
        else if ( (*pucStringPtr >= 194) && (*pucStringPtr <= 223) ) {
            uiByteCount = 2;
        }

        /* 1110xxxx - 3 byte header */
        else if ( (*pucStringPtr >= 224) && (*pucStringPtr <= 239) ) {
            uiByteCount = 3;
        }
    
        /* 11110xxx - 4 byte header */
        else if ( (*pucStringPtr >= 240) && (*pucStringPtr <= 247) ) {
            uiByteCount = 4;
        }
    
        /* 111110xx - 5 byte header */
        else if ( (*pucStringPtr >= 248) && (*pucStringPtr <= 251) ) {
            uiByteCount = 5;
        }

        /* 1111110x - 6 byte header */
        else if ( (*pucStringPtr >= 252) && (*pucStringPtr <= 253) ) {
            uiByteCount = 6;
        }
        
        /* Invalid byte - replace and continue */
        else {
/* iUtlLogInfo(UTL_LOG_CONTEXT, "one byte");  */
            *pucStringPtr = ucReplacementByte;
            pucStringPtr++;
            continue;
        }
        

        /* Loop over the trailer character bytes as this is a multibyte character */
        {
            unsigned int    uiI = 0;
            unsigned char    *pucPtr = pucStringPtr;
            
/* iUtlLogInfo(UTL_LOG_CONTEXT, "multi byte [%d][%c][%X][%d][%d]", pucStringPtr - pucString, *pucStringPtr, *pucStringPtr, *pucStringPtr, uiByteCount);  */

            /* Loop over the trailer character bytes */
            for ( uiI = 1, pucStringPtr++; uiI < uiByteCount; uiI++ ) {
            
/* iUtlLogInfo(UTL_LOG_CONTEXT, " valid byte [%d][%c][%X][%d].", pucStringPtr - pucString, *pucStringPtr, *pucStringPtr, *pucStringPtr);  */
                /* 101xxxxx - second character byte */
                if ( (uiI == 1) && (*pucStringPtr >= 160) && (*pucStringPtr <= 191) ) {
                    pucStringPtr++;
                }
            
                /* 10xxxxxx - all other character byte */
                else if ( (uiI > 1) && (*pucStringPtr >= 128) && (*pucStringPtr <= 191) ) {
                    pucStringPtr++;
                }
            
                /* Invalid character, so we replace all the bad bytes and break out */
                else {
/* iUtlLogInfo(UTL_LOG_CONTEXT, "invalid byte [%d][%d][%d].", uiByteCount, uiI, pucStringPtr - pucPtr);  */
                    while ( pucPtr < pucStringPtr ) {
/* iUtlLogInfo(UTL_LOG_CONTEXT, "=> [%c][%d].", *pucPtr, *pucPtr);  */
                        *pucPtr = ucReplacementByte;
                        pucPtr++;
                    }
                    break;
                }
            }
        }
    }


/* iUtlLogInfo(UTL_LOG_CONTEXT, "<<<<<<[%d] [%s].", s_strlen(pucString), pucString);  */
/*  */
/* for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { */
/*     iUtlLogInfo(UTL_LOG_CONTEXT, "[%d][%c][%X][%d].", pucStringPtr - pucString, *pucStringPtr, *pucStringPtr, *pucStringPtr);  */
/* } */


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
