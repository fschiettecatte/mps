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

    Module:     soundex.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 April 1994

    Purpose:    This implements the soundex algorithm as defined by Gadd. 

                T.N. Gadd: 'Fishing fore Werds': Phonetic Retrieval of written 
                text in Information Retrieval Systems, Program 22/3, 1988, 
                p.222-237.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.soundex"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define LNG_SOUNDEX_STANDARD_KEY_LEN            (4)                 /* Length of the standard soundex key */
#define LNG_SOUNDEX_DEFAULT_STANDARD_KEY        L"Z000"             /* Default standard soundex key */

#define LNG_SOUNDEX_ALTERNATIVE_KEY_LEN         (6)                 /* Length of the alternative soundex key */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Language soundex functions structure */
struct lngSoundexFunction {
    unsigned int    uiSoundexID;                    /* Soundex ID */
    unsigned int    uiLanguageID;                   /* Language ID */
    int             (*iLngSoundexFunction)();       /* Soundex function pointer */
};


/* Language soundex structure */
struct lngSoundex {
    unsigned int    uiSoundexID;                    /* Soundex ID */
    unsigned int    uiLanguageID;                   /* Language ID */
    int             (*iLngSoundexFunction)();       /* Soundex function pointer (denomalization) */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLngSoundexStandardKey_un (struct lngSoundex *plsLngSoundex, wchar_t *pwcTerm, 
        wchar_t *pwcSoundexKey, unsigned int uiSoundexKeyLen);

static int iLngSoundexAlternativeKey_un (struct lngSoundex *plsLngSoundex, wchar_t *pwcTerm, 
        wchar_t *pwcSoundexKey, unsigned int uiSoundexKeyLen);


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Soundex functions */
static struct lngSoundexFunction plsfLngSoundexFunctionListGlobal[] = 
{
    /* Standard Soundex - euro languages */
    {   LNG_SOUNDEX_STANDARD_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngSoundexStandardKey_un,
    },
    
    /* Alternative Soundex - euro languages */
    {   LNG_SOUNDEX_ALTERNATIVE_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngSoundexAlternativeKey_un,
    },
    
    /* Terminator */
    {   0,
        0,
        NULL,
    },
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngSoundexCreateByName()

    Purpose:    This function returns a language soundex structure for a particular soundex name 
                and language code combination.

    Parameters: pucSoundexName      Soundex name
                pucLanguageCode     Language code
                ppvLngSoundex       Return pointer for the language soundex structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngSoundexCreateByName
(
    unsigned char *pucSoundexName,
    unsigned char *pucLanguageCode,
    void **ppvLngSoundex
)
{

    int             iError = LNG_NoError;
    unsigned int    uiSoundexID = 0;
    unsigned int    uiLanguageID = 0;
    

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSoundexName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSoundexName' parameter passed to 'iLngSoundexCreateByName'."); 
        return (LNG_LanguageInvalidSoundexName);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngSoundexCreateByName'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngSoundex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngSoundex' parameter passed to 'iLngSoundexCreateByName'."); 
        return (LNG_ReturnParameterError);
    }
    

    /* Get the soundex ID */
    if ( (iError = iLngGetSoundexIDFromName(pucSoundexName, &uiSoundexID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the soundex ID from the soundex name: '%s', lng error: %d.", pucSoundexName, iError); 
        return (iError);
    }

    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngSoundexCreateByID(uiSoundexID, uiLanguageID, ppvLngSoundex));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngSoundexCreateByID()

    Purpose:    This function returns a language soundex structure for a particular soundex ID 
                and language ID combination.

    Parameters: uiSoundexID         Soundex ID
                uiLanguageID        Language ID
                ppvLngSoundex       Return pointer for the language soundex structure

    Globals:    plsfLngSoundexFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngSoundexCreateByID
(
    unsigned int uiSoundexID,
    unsigned int uiLanguageID,
    void **ppvLngSoundex
)
{

    int                         iError = LNG_NoError;
    struct lngSoundexFunction   *plsfLngSoundexFunctionPtr = plsfLngSoundexFunctionListGlobal;
    struct lngSoundex           *plsLngSoundex = NULL;


    /* Check the parameters */
    if ( uiSoundexID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSoundexID' parameter passed to 'iLngSoundexCreateByID'."); 
        return (LNG_LanguageInvalidSoundexID);
    }
    
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngSoundexCreateByID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( ppvLngSoundex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngSoundex' parameter passed to 'iLngSoundexCreateByID'."); 
        return (LNG_ReturnParameterError);
    }
    

    /* Canonicalize the language ID */
    if ( (iError = iLngGetCanonicalLanguageID(uiLanguageID, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiLanguageID, iError); 
        return (iError);
    }


    /* Loop over the soundex function structures to see which one we want */
    for ( plsfLngSoundexFunctionPtr = plsfLngSoundexFunctionListGlobal; plsfLngSoundexFunctionPtr->iLngSoundexFunction != NULL; plsfLngSoundexFunctionPtr++ ) {

        /* Select this soundex function if there is a match */
        if ( ((plsfLngSoundexFunctionPtr->uiSoundexID == uiSoundexID) || (plsfLngSoundexFunctionPtr->uiSoundexID == LNG_SOUNDEX_ANY_ID)) &&
                ((plsfLngSoundexFunctionPtr->uiLanguageID == uiLanguageID) || (plsfLngSoundexFunctionPtr->uiLanguageID == LNG_LANGUAGE_ANY_ID)) ) {
            
            /* Allocate the language soundex structure */
            if ( (plsLngSoundex = (struct lngSoundex *)s_malloc((size_t)(sizeof(struct lngSoundex)))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Set the language soundex structure */
            plsLngSoundex->uiSoundexID = uiSoundexID;
            plsLngSoundex->uiLanguageID = uiLanguageID;
            plsLngSoundex->iLngSoundexFunction = plsfLngSoundexFunctionPtr->iLngSoundexFunction;

            /* Set the return pointer */
            *ppvLngSoundex = (void *)plsLngSoundex;

            return (LNG_NoError);
        }
    }
    

    /* There is no soundex function available for this soundex name and language code combination */
    return (LNG_SoundexUnavailableSoundex);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngSoundexFree()

    Purpose:    This function frees the language soundex structure.

    Parameters: pvLngSoundex        Language soundex structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngSoundexFree
(
    void *pvLngSoundex
)
{

    struct lngSoundex *plsLngSoundex = (struct lngSoundex *)pvLngSoundex;


    /* Check the parameters */
    if ( plsLngSoundex == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'plsLngSoundex' parameter passed to 'iLngSoundexFree'."); 
        return (LNG_SoundexInvalidSoundex);
    }

    
    /* Free the language soundex structure */
    s_free(plsLngSoundex);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngSoundexGetSoundexCharacterList()

    Purpose:    This gets the character list for a term

    Parameters: pvLngSoundex                Language soundex structure
                pwcTerm                     Pointer to the term for which we want the soundex
                ppwcSoundexCharacterList    Return pointer for the character list to scan for this soundex key

    Globals:    plsfLngSoundexFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngSoundexGetSoundexCharacterList
(
    void *pvLngSoundex,
    wchar_t *pwcTerm,
    wchar_t **ppwcSoundexCharacterList
)
{

    struct lngSoundex   *plsLngSoundex = (struct lngSoundex *)pvLngSoundex;
    wchar_t             pwcSoundexCharacterList[2] = {L'\0'};
    wchar_t             *pwcSoundexCharacterListPtr = NULL;


    /* Check the parameters */
    if ( plsLngSoundex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngMetaphone' parameter passed to 'iLngSoundexGetSoundexCharacterList'."); 
        return (LNG_SoundexInvalidSoundex);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngSoundexGetSoundexCharacterList'."); 
        return (LNG_SoundexInvalidTerm);
    }

    if ( ppwcSoundexCharacterList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcMetaphoneCharacterList' parameter passed to 'iLngSoundexGetSoundexCharacterList'."); 
        return (LNG_ReturnParameterError);
    }

    
    /* Create the character list */
    pwcSoundexCharacterList[0] = tolower(pwcTerm[0]);
    pwcSoundexCharacterList[1] = L'\0';

    /* Duplicate the character list */
    if ( (pwcSoundexCharacterListPtr = s_wcsdup(pwcSoundexCharacterList)) == NULL ) {
        return (LNG_MemError);
    }
    
    /* Set the return pointer */
    *ppwcSoundexCharacterList = pwcSoundexCharacterListPtr;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngSoundexGetSoundexKey()

    Purpose:    This is the router function for the soundex

    Parameters: pvLngSoundex        Language soundex structure
                pwcTerm             Pointer to the term for which we want the soundex key
                pwcSoundexKey       Return pointer for the soundex key
                uiSoundexKeyLen     Length of the return pointer for the soundex key

    Globals:    plsfLngSoundexFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngSoundexGetSoundexKey
(
    void *pvLngSoundex,
    wchar_t *pwcTerm,
    wchar_t *pwcSoundexKey,
    unsigned int uiSoundexKeyLen
)
{

    int                 iError = LNG_NoError;
    struct lngSoundex   *plsLngSoundex = (struct lngSoundex *)pvLngSoundex;


    /* Check the parameters */
    if ( pvLngSoundex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngSoundex' parameter passed to 'iLngSoundexGetSoundexKey'."); 
        return (LNG_SoundexInvalidSoundex);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngSoundexGetSoundexKey'."); 
        return (LNG_SoundexInvalidTerm);
    }

    if ( pwcSoundexKey == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pwcSoundexKey' parameter passed to 'iLngSoundexGetSoundexKey'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiSoundexKeyLen <= LNG_SOUNDEX_KEY_LENGTH ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSoundexKeyLen' parameter passed to 'iLngSoundexGetSoundexKey'."); 
        return (LNG_ParameterError);
    }


    /* Check that the soundex function is valid */
    if ( plsLngSoundex->iLngSoundexFunction == NULL ) {
        return (LNG_SoundexInvalidSoundex);
    }

    /* Call the soundex function */
    if ( (iError = plsLngSoundex->iLngSoundexFunction(plsLngSoundex, pwcTerm, pwcSoundexKey, uiSoundexKeyLen)) != LNG_NoError ) {
        return (LNG_SoundexSoundexingFailed);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngSoundexStandardKey_un()

    Purpose:    To get a term's soundex key - euro languages

    Parameters: plsLngSoundex       Language soundex structure
                pwcTerm             Pointer to the term for which we want the soundex key
                pwcSoundexKey       Return pointer for the soundex key
                uiSoundexKeyLen     Length of the return pointer for the soundex key

    Globals:    none

    Returns:    LNG error code

*/
static int iLngSoundexStandardKey_un
(
    struct lngSoundex *plsLngSoundex,
    wchar_t *pwcTerm,
    wchar_t *pwcSoundexKey,
    unsigned int uiSoundexKeyLen
)
{

    wchar_t         wcLastChar = L'\0';
    wchar_t         *pwcTermPtr = NULL;
    unsigned int    uiIndex = 0;

    int             piCodeList[] = {0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 2, 4, 5, 5, 0, 1, 2, 6, 2, 3, 0, 1, 0, 2, 0, 2};
                                 /* A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z */


    ASSERT(plsLngSoundex != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(pwcSoundexKey != NULL);
    ASSERT(uiSoundexKeyLen > LNG_SOUNDEX_KEY_LENGTH);


    /* Set default key */
    s_wcsnncpy(pwcSoundexKey, LNG_SOUNDEX_DEFAULT_STANDARD_KEY, LNG_SOUNDEX_STANDARD_KEY_LEN + 1);

    /* Prime the key */
    pwcSoundexKey[0] = towupper(wcLngCaseStripAccentFromWideCharacter(pwcTerm[0]));
    wcLastChar = pwcSoundexKey[0];

    /* Scan rest of string */
    for ( pwcTermPtr = pwcTerm + 1, uiIndex = 1; (uiIndex < LNG_SOUNDEX_STANDARD_KEY_LEN) && (*pwcTermPtr != L'\0'); pwcTermPtr++ ) {

        wchar_t    wcChar = towupper(wcLngCaseStripAccentFromWideCharacter(*pwcTermPtr));

        /* Use only letters */
        if ( (iswalpha(wcChar) != 0) && (wcChar <= L'Z') ) {

            /* Ignore duplicate successive chars */
            if ( wcLastChar != wcChar ) {

                /* New last character */
                wcLastChar = wcChar;

                /* Ignore letters with code 0 */
                if ( (wcChar = piCodeList[wcChar - L'A']) != 0 ) {
                    pwcSoundexKey[uiIndex] = L'0' + wcChar;
                    uiIndex++;
                }
            }
        }
    }


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngSoundexAlternativeKey_un()

    Purpose:    To get a term's soundex key - euro languages

    Parameters: plsLngSoundex       Language soundex structure
                pwcTerm             Pointer to the term for which we want the soundex key
                pwcSoundexKey       Return pointer for the soundex key
                uiSoundexKeyLen     Length of the return pointer for the soundex key

    Globals:    none

    Returns:    void

*/
static int iLngSoundexAlternativeKey_un
(
    struct lngSoundex *plsLngSoundex,
    wchar_t *pwcTerm,
    wchar_t *pwcSoundexKey,
    unsigned int uiSoundexKeyLen
)
{

    wchar_t         *pwcTermPtr = NULL;
    unsigned int    uiIndex = 0;
    wchar_t         wcChar = L'\0';


    ASSERT(plsLngSoundex != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(pwcSoundexKey != NULL);
    ASSERT(uiSoundexKeyLen > LNG_SOUNDEX_KEY_LENGTH);


    /* Prime the key */
    pwcSoundexKey[0] = wcLngCaseStripAccentFromWideCharacter(pwcTerm[0]);
    pwcSoundexKey[1] = L'\0';


    /* Process characters until we have a key of at least LNG_SOUNDEX_ALTERNATIVE_KEY_LEN bytes,
    ** or we have reached the end of the term
    */
    for ( pwcTermPtr = pwcTerm + 1, uiIndex = 1; (uiIndex < LNG_SOUNDEX_ALTERNATIVE_KEY_LEN) && (*pwcTermPtr!= L'\0'); pwcTermPtr++ ) {

        /* Downcase the character and strip accents */
        wcChar = wcLngCaseStripAccentFromWideCharacter(towlower(*pwcTermPtr));


        /* And process */
        switch ( wcChar ) {

            case L'b':
            case L'p':
            case L'f':
            case L'v':
                s_wcsnncat(pwcSoundexKey, L"1", LNG_SOUNDEX_ALTERNATIVE_KEY_LEN, LNG_SOUNDEX_ALTERNATIVE_KEY_LEN + 1);
                uiIndex++;
                break;

            case L'c':
            case L's':
            case L'k':
            case L'g':
            case L'j':
            case L'q':
            case L'x':
            case L'z':
                s_wcsnncat(pwcSoundexKey, L"2", LNG_SOUNDEX_ALTERNATIVE_KEY_LEN, LNG_SOUNDEX_ALTERNATIVE_KEY_LEN + 1);
                uiIndex++;
                break;

            case L'd':
            case L't':
                s_wcsnncat(pwcSoundexKey, L"3", LNG_SOUNDEX_ALTERNATIVE_KEY_LEN, LNG_SOUNDEX_ALTERNATIVE_KEY_LEN + 1);
                uiIndex++;
                break;

            case L'l':
                s_wcsnncat(pwcSoundexKey, L"4", LNG_SOUNDEX_ALTERNATIVE_KEY_LEN, LNG_SOUNDEX_ALTERNATIVE_KEY_LEN + 1);
                uiIndex++;
                break;

            case L'm':
            case L'n':
                s_wcsnncat(pwcSoundexKey, L"5", LNG_SOUNDEX_ALTERNATIVE_KEY_LEN, LNG_SOUNDEX_ALTERNATIVE_KEY_LEN + 1);
                uiIndex++;
                break;

            case L'r':
                s_wcsnncat(pwcSoundexKey, L"6", LNG_SOUNDEX_ALTERNATIVE_KEY_LEN, LNG_SOUNDEX_ALTERNATIVE_KEY_LEN + 1);
                uiIndex++;
                break;

            case L'a':
            case L'e':
            case L'i':
            case L'o':
            case L'u':
            case L'y':
            case L'w':
            case L'h':
                break;
            
            default:
                break;
        }
    }


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/



