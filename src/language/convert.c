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

    Module:     convert.c

    Author:     Francois Schiettcatte (FS Consulting LLC.)

    Created:    8 January 2005

    Purpose:    This file contains various language support function

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.convert"


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Converter structure */
struct lngConverter {
    iconv_t         iConvDescriptor;                        /* Convertor descriptor */
    unsigned int    uiSourceCharacterSetID;                 /* Source character set */
    unsigned int    uiDestinationCharacterSetID;            /* Destination character set */
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConverterCreateByName()

    Purpose:    This function returns a language converter structure for 
                a particular source and destination character set name.

    Parameters: pucSourceCharacterSetName           Source character set name
                pucDestinationCharacterSetName      Destination character set name
                ppvLngConverter                     Return pointer for the converter structure

    Globals:    

    Returns:    An LNG error code

*/
int iLngConverterCreateByName
(
    unsigned char *pucSourceCharacterSetName,
    unsigned char *pucDestinationCharacterSetName,
    void **ppvLngConverter
)
{

    struct lngConverter     *plcLngConverter = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSourceCharacterSetName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSourceCharacterSetName' parameter passed to 'iLngConverterCreateByName'."); 
        return (LNG_LanguageInvalidCharacterSetName);
    }

    if ( bUtlStringsIsStringNULL(pucDestinationCharacterSetName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDestinationCharacterSetName' parameter passed to 'iLngConverterCreateByName'."); 
        return (LNG_LanguageInvalidCharacterSetName);
    }

    if ( ppvLngConverter == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngConverter' parameter passed to 'iLngConverterCreateByName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Allocate the converter structure */
    if ( (plcLngConverter = (struct lngConverter *)s_malloc(sizeof(struct lngConverter))) == NULL ) {
        return (LNG_MemError);
    }

    /* Open the convertor descriptor */
    if ( (plcLngConverter->iConvDescriptor = (iconv_t)s_iconv_open(pucDestinationCharacterSetName, pucSourceCharacterSetName)) == -1 ) {
        s_free(plcLngConverter);
        return (LNG_ConverterInvalidCharacterSetCombination);
    }    


    /* Set the structure fields */
    plcLngConverter->uiSourceCharacterSetID = (s_strcmp(LNG_CHARACTER_SET_WCHAR_NAME, pucSourceCharacterSetName) == 0) ?
            LNG_CHARACTER_SET_WCHAR_ID : LNG_CHARACTER_SET_ANY_ID;
    plcLngConverter->uiDestinationCharacterSetID = (s_strcmp(LNG_CHARACTER_SET_WCHAR_NAME, pucDestinationCharacterSetName) == 0) ?
            LNG_CHARACTER_SET_WCHAR_ID : LNG_CHARACTER_SET_ANY_ID;

    /* Set the return parameter */
    *ppvLngConverter = (void *)plcLngConverter;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConverterCreateByID()

    Purpose:    This function returns a language converter structure for 
                a particular source and destination character set ID.

    Parameters: uiSourceCharacterSetID          Source character set ID
                uiDestinationCharacterSetID     Destination character set ID
                ppvLngConverter                 Return pointer for the converter structure

    Globals:    

    Returns:    An LNG error code

*/
int iLngConverterCreateByID
(
    unsigned int uiSourceCharacterSetID,
    unsigned int uiDestinationCharacterSetID,
    void **ppvLngConverter
)
{

    int             iError = LNG_NoError;
    unsigned char   pucSourceCharacterSetName[LNG_CHARACTER_SET_NAME_LENGTH + 1] = {'\0'};
    unsigned char   pucDestimationCharacterSetName[LNG_CHARACTER_SET_NAME_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( ppvLngConverter == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngConverter' parameter passed to 'iLngConverterCreateByID'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the source character set name */
    if ( (iError = iLngGetCharacterSetNameFromID(uiSourceCharacterSetID, pucSourceCharacterSetName, LNG_CHARACTER_SET_NAME_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the character set name from the source character set ID: %u, lng error: %d.", uiSourceCharacterSetID, iError); 
        return (iError);
    }

    /* Get the destination character set name */
    if ( (iError = iLngGetCharacterSetNameFromID(uiDestinationCharacterSetID, pucDestimationCharacterSetName, LNG_CHARACTER_SET_NAME_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the character set name from the destination character set ID: %u, lng error: %d.", uiDestinationCharacterSetID, iError); 
        return (iError);
    }


    /* Pass the call onto create by names */
    return (iLngConverterCreateByName(pucSourceCharacterSetName, pucDestimationCharacterSetName, ppvLngConverter));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConverterConvertString()

    Purpose:    This function converts from one character set to another
                based on the passed language converter structure.

    Parameters: pvLngConverter                  Character set converter structure
                uiErrorHandling                 Error handling
                pucSourceString                 Source string
                uiSourceStringLength            Source string length
                ppucDestinationString           Destination string/return pointer
                puiDestinationStringLength      Destination string length/return pointer
                                                (capacity may be longer if destination string is allocated)

    Globals:    none

    Returns:    An LNG error code

*/
int iLngConverterConvertString
(
    void *pvLngConverter,
    unsigned int uiErrorHandling,
    unsigned char *pucSourceString, 
    unsigned int uiSourceStringLength, 
    unsigned char **ppucDestinationString, 
    unsigned int *puiDestinationStringLength
)
{

    int                     iError = LNG_NoError;
    struct lngConverter     *plcLngConverter = (struct lngConverter *)pvLngConverter;
    unsigned char           *pucDestinationString = NULL;
    size_t                  zDestinationStringLength = 0;
    unsigned int            uiDestinationStringCapacity = 0;
    unsigned char           *pucSourceStringPtr = NULL;
    size_t                  zSourceStringLength = 0;
    unsigned char           *pucDestinationStringPtr = NULL;
    size_t                  zStatus = 0;


    /* Check the parameters */
    if ( pvLngConverter == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngConverter' parameter passed to 'iLngConverterConvertString'."); 
        return (LNG_ConverterInvalidConverter);
    }

    if ( (uiErrorHandling != LNG_CONVERTER_SKIP_ON_ERROR) && (uiErrorHandling != LNG_CONVERTER_RETURN_ON_ERROR) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiErrorHandling' parameter passed to 'iLngConverterConvertString'."); 
        return (LNG_ConverterInvalidErrorHandling);
    }

    /* This is commented out because the source string can be specified as a
    ** wide character string, in which case the first byte may well be NULL
    */
/*     if ( bUtlStringsIsStringNULL(pucSourceString) == true ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSourceString' parameter passed to 'iLngConverterConvertString'.");  */
/*         return (LNG_ConverterInvalidSourceString); */
/*     } */

    if ( uiSourceStringLength == 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSourceStringLength' parameter passed to 'iLngConverterConvertString'."); 
        return (LNG_ConverterInvalidSourceStringLength);
    }

    if ( ppucDestinationString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucDestinationString' parameter passed to 'iLngConverterConvertString'."); 
        return (LNG_ConverterInvalidDestinationString);
    }

    if ( puiDestinationStringLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDestinationStringLength' parameter passed to 'iLngConverterConvertString'."); 
        return (LNG_ConverterInvalidDestinationStringLength);
    }


    /* See if the destination string needs to be allocated */
    if ( *ppucDestinationString == NULL ) {
            
        /* Work out the capacity of the destination string, add one for the terminating NULL byte */
        if ( plcLngConverter->uiDestinationCharacterSetID == LNG_CHARACTER_SET_WCHAR_ID ) {
            uiDestinationStringCapacity = (uiSourceStringLength  + 1) * sizeof(wchar_t);
        }
        else if ( plcLngConverter->uiSourceCharacterSetID == LNG_CHARACTER_SET_WCHAR_ID ) {
            uiDestinationStringCapacity = (uiSourceStringLength + 1) * 2;
        }
        else {
            uiDestinationStringCapacity = (uiSourceStringLength + 1) * LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER;
        }
        
        /* Allocate the destination string */
        if ( (pucDestinationString = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * uiDestinationStringCapacity))) == NULL ) {
            iError = LNG_MemError;
            goto bailFromiLngConverterConvertString;
        }

        
        /* Set up the destination pointers/variables */
        pucDestinationStringPtr = pucDestinationString;
    }
    else {

        /* Set up the destination pointers/variables */
        pucDestinationString = *ppucDestinationString;
        uiDestinationStringCapacity = *puiDestinationStringLength;

        pucDestinationStringPtr = pucDestinationString;
    }

    /* Set up the destination string length, leave room for the NULL */
    zDestinationStringLength = uiDestinationStringCapacity - ((plcLngConverter->uiDestinationCharacterSetID == LNG_CHARACTER_SET_WCHAR_ID) ? sizeof(wchar_t) : 1);



    /* Set up the source pointers/variables */
    pucSourceStringPtr = pucSourceString;
    zSourceStringLength = uiSourceStringLength;
    

/* printf("pucSourceStringPtr: '%s'\n", pucSourceStringPtr); */
/* printf("zDestinationStringLength: %u\n", zDestinationStringLength); */


    /* Loop forever, we control the loop from within */
    while ( true ) {
    
        /* Do the conversion */
        zStatus = s_iconv(plcLngConverter->iConvDescriptor, (char **)&pucSourceStringPtr, &zSourceStringLength, (char **)&pucDestinationStringPtr, &zDestinationStringLength);

        /* Conversion was not successful */
        if ( zStatus == - 1 ) {
        
            /* Skip the error, if we can skip errors and is the error can be skipped */
            if ( (uiErrorHandling == LNG_CONVERTER_SKIP_ON_ERROR) && ((errno == EILSEQ) || (errno == EINVAL)) ) {
                pucSourceStringPtr++;
                zSourceStringLength--;
            }
            /* Cant skip errors or the error was not recoverable */
            else {
                iError = LNG_ConverterConversionFailed;
                goto bailFromiLngConverterConvertString;
            }
        }

        /* Conversion was successful if there are no more characters left to convert */
        else if ( zSourceStringLength == 0 ) {
            iError = LNG_NoError;
            break;
        }

        /* Conversion was not successful */
        else {
            iError = LNG_ConverterConversionFailed;
            goto bailFromiLngConverterConvertString;
        }

    }



    /* Bail label */
    bailFromiLngConverterConvertString:

    /* Handle the error */
    if ( iError == LNG_NoError ) {

        /* Set the destination string length and NULL terminate the destination string,
        ** we do this regardless of whether the string was allocated or not
        */
        if ( plcLngConverter->uiDestinationCharacterSetID == LNG_CHARACTER_SET_WCHAR_ID ) {
            wchar_t    *pwcDestinationString = (wchar_t *)pucDestinationString;
            *puiDestinationStringLength = (uiDestinationStringCapacity - zDestinationStringLength - sizeof(wchar_t));
            pwcDestinationString[(*puiDestinationStringLength) / sizeof(wchar_t)] = L'\0';
        }
        else {
            pucDestinationStringPtr = *ppucDestinationString;
            *puiDestinationStringLength = (uiDestinationStringCapacity - zDestinationStringLength - sizeof(unsigned char));
            pucDestinationString[*puiDestinationStringLength] = '\0';
        }

        /* String was allocated */
        if ( *ppucDestinationString == NULL ) {
            /* Set the return pointer */
            *ppucDestinationString = pucDestinationString;
        }
    }
    else {

        /* String was allocated */
        if ( *ppucDestinationString == NULL ) {

            /* Free any allocated resources */
            s_free(pucDestinationString);
        
            /* Clear the return pointers if the string was allocated */
            *ppucDestinationString = NULL;
            *puiDestinationStringLength = 0;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConverterFree()

    Purpose:    This function frees the language converter structure.

    Parameters: pvLngConverter      Character set converter structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngConverterFree
(
    void *pvLngConverter
)
{

    struct lngConverter     *plcLngConverter = (struct lngConverter *)pvLngConverter;


    /* Check the parameters */
    if ( pvLngConverter == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvLngConverter' parameter passed to 'iLngConverterFree'."); 
        return (LNG_ConverterInvalidConverter);
    }


    /* Close the convertor descriptor */
    s_iconv_close(plcLngConverter->iConvDescriptor);

    /* Free the convertor structure */
    free(plcLngConverter);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConvertWideStringToUtf8_d()

    Purpose:    This is a wrapper function to convert a string from wide characters to utf8,
                allocating ppucDestinationString in the process

    Parameters: pwcSourceString             Source string
                uiSourceStringLength        Source string (optional, set to 0 if unknown)
                ppucDestinationString       Destination string return pointer

    Globals:    none

    Returns:    An LNG error code

*/
int iLngConvertWideStringToUtf8_d
(
    wchar_t *pwcSourceString,
    unsigned int uiSourceStringLength,
    unsigned char **ppucDestinationString
)
{

    int             iError = LNG_NoError;
    void            *pvLngConverter = NULL;
    unsigned char   *pucDestinationString = NULL; 
    unsigned int    uiDestinationStringLength = 0;


    /* Check the parameters */
    if ( bUtlStringsIsWideStringNULL(pwcSourceString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcSourceString' parameter passed to 'iLngConvertWideStringToUtf8_d'."); 
        return (LNG_ConverterInvalidSourceString);
    }

/*     if ( uiSourceStringLength == 0 ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiSourceStringLength' parameter passed to 'iLngConvertWideStringToUtf8_d'.");  */
/*         return (LNG_ConverterInvalidSourceStringLength); */
/*     } */

    if ( ppucDestinationString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucDestinationString' parameter passed to 'iLngConvertWideStringToUtf8_d'."); 
        return (LNG_ConverterInvalidDestinationString);
    }


    /* Create a character converter */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_WCHAR_NAME, LNG_CHARACTER_SET_UTF_8_NAME, &pvLngConverter)) != LNG_NoError ) {
        return (iError);
    }
    
    /* Set the source string length */
    if ( uiSourceStringLength == 0 ) {
        uiSourceStringLength = s_wcslen(pwcSourceString);
    }

    /* Convert the string */
    iError = iLngConverterConvertString(pvLngConverter, LNG_CONVERTER_RETURN_ON_ERROR, (unsigned char *)pwcSourceString, uiSourceStringLength * sizeof(wchar_t), 
            &pucDestinationString, &uiDestinationStringLength);

    /* Close the convertor descriptor */
    iLngConverterFree(pvLngConverter);

    /* Set the return pointer */
    *ppucDestinationString = pucDestinationString;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConvertWideStringToUtf8_s()

    Purpose:    This is a wrapper function to convert a string from wide characters to utf8,
                pucDestinationString is already allocated and uiDestinationStringLength
                is its length

    Parameters: pwcSourceString             Source string
                uiSourceStringLength        Source string (optional, set to 0 if unknown)
                pucDestinationString        Destination string
                uiDestinationStringLength   Destination string length

    Globals:    none

    Returns:    An LNG error code

*/
int iLngConvertWideStringToUtf8_s
(
    wchar_t *pwcSourceString, 
    unsigned int uiSourceStringLength,
    unsigned char *pucDestinationString, 
    unsigned int uiDestinationStringLength
)
{

    int             iError = LNG_NoError;
    void            *pvLngConverter = NULL;
    unsigned char   *pucLocalDestinationString = pucDestinationString;
    unsigned int    uiLocalDestinationStringLength = uiDestinationStringLength;


    /* Check the parameters */
    if ( bUtlStringsIsWideStringNULL(pwcSourceString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcSourceString' parameter passed to 'iLngConvertWideStringToUtf8_s'."); 
        return (LNG_ConverterInvalidSourceString);
    }

/*     if ( uiSourceStringLength == 0 ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiSourceStringLength' parameter passed to 'iLngConvertWideStringToUtf8_s'.");  */
/*         return (LNG_ConverterInvalidSourceStringLength); */
/*     } */

    if ( pucDestinationString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDestinationString' parameter passed to 'iLngConvertWideStringToUtf8_s'."); 
        return (LNG_ConverterInvalidDestinationString);
    }

    if ( uiDestinationStringLength == 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiDestinationStringLength' parameter passed to 'iLngConvertWideStringToUtf8_s'."); 
        return (LNG_ConverterInvalidDestinationStringLength);
    }


    /* Create a character converter */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_WCHAR_NAME, LNG_CHARACTER_SET_UTF_8_NAME, &pvLngConverter)) != LNG_NoError ) {
        return (iError);
    }    

    /* Set the source string length */
    if ( uiSourceStringLength == 0 ) {
        uiSourceStringLength = s_wcslen(pwcSourceString);
    }

    /* Convert the string */
    iError = iLngConverterConvertString(pvLngConverter, LNG_CONVERTER_RETURN_ON_ERROR, (unsigned char *)pwcSourceString, uiSourceStringLength * sizeof(wchar_t), 
            &pucLocalDestinationString, &uiLocalDestinationStringLength);

    /* Close the convertor descriptor */
    iLngConverterFree(pvLngConverter);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConvertUtf8ToWideString_d()

    Purpose:    This is a wrapper function to convert a string from utf8 to wide characters,
                allocating ppwcDestinationString in the process

    Parameters: pucSourceString         Source string
                uiSourceStringLength    Source string (optional, set to 0 if unknown)
                ppwcDestinationString   Destination string return pointer

    Globals:    none

    Returns:    An LNG error code

*/
int iLngConvertUtf8ToWideString_d
(
    unsigned char *pucSourceString,
    unsigned int uiSourceStringLength,
    wchar_t **ppwcDestinationString
)
{

    int             iError = LNG_NoError;
    void            *pvLngConverter = NULL;
    wchar_t         *pwcDestinationString = NULL;
    unsigned int    uiDestinationStringLength = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSourceString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSourceString' parameter passed to 'iLngConvertUtf8ToWideString_d'."); 
        return (LNG_ConverterInvalidSourceString);
    }

/*     if ( uiSourceStringLength == 0 ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiSourceStringLength' parameter passed to 'iLngConvertUtf8ToWideString_d'.");  */
/*         return (LNG_ConverterInvalidSourceStringLength); */
/*     } */

    if ( ppwcDestinationString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcDestinationString' parameter passed to 'iLngConvertUtf8ToWideString_d'."); 
        return (LNG_ConverterInvalidDestinationString);
    }


    /* Create a character converter */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_UTF_8_NAME, LNG_CHARACTER_SET_WCHAR_NAME, &pvLngConverter)) != LNG_NoError ) {
        return (iError);
    }    

    /* Set the source string length */
    if ( uiSourceStringLength == 0 ) {
        uiSourceStringLength = s_strlen(pucSourceString);
    }

    /* Convert the string */
    iError = iLngConverterConvertString(pvLngConverter, LNG_CONVERTER_RETURN_ON_ERROR, pucSourceString, uiSourceStringLength, 
            (unsigned char **)&pwcDestinationString, &uiDestinationStringLength);

    /* Close the convertor descriptor */
    iLngConverterFree(pvLngConverter);

    /* Set the return pointer */
    *ppwcDestinationString = pwcDestinationString;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngConvertUtf8ToWideString_s()

    Purpose:    This is a wrapper function to convert a string from utf8 to wide characters,
                pwcDestinationString is already allocated and uiDestinationStringLength
                is its length

    Parameters: pucSourceString             Source string
                uiSourceStringLength        Source string (optional, set to 0 if unknown)
                pwcDestinationString        Destination string
                uiDestinationStringLength   Destination string length

    Globals:    none

    Returns:    An LNG error code

*/
int iLngConvertUtf8ToWideString_s
(
    unsigned char *pucSourceString, 
    unsigned int uiSourceStringLength,
    wchar_t *pwcDestinationString, 
    unsigned int uiDestinationStringLength
)
{

    int             iError = LNG_NoError;
    void            *pvLngConverter = NULL;
    wchar_t         *pwcLocalDestinationString = pwcDestinationString; 
    unsigned int    uiLocalDestinationStringLength = uiDestinationStringLength * sizeof(wchar_t);


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSourceString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSourceString' parameter passed to 'iLngConvertUtf8ToWideString_s'."); 
        return (LNG_ConverterInvalidSourceString);
    }

/*     if ( uiSourceStringLength == 0 ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiSourceStringLength' parameter passed to 'iLngConvertUtf8ToWideString_s'.");  */
/*         return (LNG_ConverterInvalidSourceStringLength); */
/*     } */

    if ( pwcDestinationString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pwcDestinationString' parameter passed to 'iLngConvertUtf8ToWideString_s'."); 
        return (LNG_ConverterInvalidDestinationString);
    }

    if ( uiDestinationStringLength == 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiDestinationStringLength' parameter passed to 'iLngConvertUtf8ToWideString_s'."); 
        return (LNG_ConverterInvalidDestinationStringLength);
    }


    /* Create a character converter */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_UTF_8_NAME, LNG_CHARACTER_SET_WCHAR_NAME, &pvLngConverter)) != LNG_NoError ) {
        return (iError);
    }    

    /* Set the source string length */
    if ( uiSourceStringLength == 0 ) {
        uiSourceStringLength = s_strlen(pucSourceString);
    }

    /* Convert the string */
    iError = iLngConverterConvertString(pvLngConverter, LNG_CONVERTER_RETURN_ON_ERROR, pucSourceString, uiSourceStringLength, 
            (unsigned char **)&pwcLocalDestinationString, &uiLocalDestinationStringLength);

    /* Close the convertor descriptor */
    iLngConverterFree(pvLngConverter);


    return (iError);

}


/*---------------------------------------------------------------------------*/
