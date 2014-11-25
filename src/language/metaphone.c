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

    Module:     metaphone.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    10 October 1998

    Purpose:    This implements the metaphone algorithm. 


                Metaphone copied from C Gazette, June/July 1991, pp 56-57,
                author Gary A. Parker, with changes by Bernard Tiffany of the
                University of Michigan, and more changes by Tim Howes of the
                University of Michigan.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.metaphone"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Macros to access character coding array */
#define LNG_METAPHONE_IS_VOWEL(x)       ((x) != L'\0' && pwcCharacterCodingGlobal[(x) - L'A'] & 1)   /* AEIOU */
#define LNG_METAPHONE_IS_SAME(x)        ((x) != L'\0' && pwcCharacterCodingGlobal[(x) - L'A'] & 2)   /* FJLMNR */
#define LNG_METAPHONE_IS_VARSON(x)      ((x) != L'\0' && pwcCharacterCodingGlobal[(x) - L'A'] & 4)   /* CGPST */
#define LNG_METAPHONE_IS_FRONTV(x)      ((x) != L'\0' && pwcCharacterCodingGlobal[(x) - L'A'] & 8)   /* EIY */
#define LNG_METAPHONE_IS_NOGHF(x)       ((x) != L'\0' && pwcCharacterCodingGlobal[(x) - L'A'] & 16)  /* BDH */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Language metaphone function structure */
struct lngMetaphoneFunction {
    unsigned int    ulMetaphoneID;                      /* Metaphone ID */
    unsigned int    uiLanguageID;                       /* Language ID */
    int             (*iLngMetaphoneFunction)();         /* Metaphone function pointer */
};


/* Language metaphone structure */
struct lngMetaphone {
    unsigned int    uiMetaphoneID;                      /* Metaphone ID */
    unsigned int    uiLanguageID;                       /* Language ID */
    int             (*iLngMetaphoneFunction)();         /* Metaphone function pointer (denormalization) */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/
static int iLngMetaphoneGetKey_un (struct lngMetaphone *plmLngMetaphone, wchar_t *pwcTerm, 
        wchar_t *pwcMetaphoneKey, unsigned int uiMetaphoneKeyLen);


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Metaphone functions */
static struct lngMetaphoneFunction plmfLngMetaphoneFunctionListGlobal[] = 
{
    /* Standard Metaphone - euro languages */
    {   LNG_METAPHONE_STANDARD_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngMetaphoneGetKey_un,
    },

    /* Terminator */
    {   0,
        0,
        NULL,
    },
};


/* Character coding array */
static wchar_t pwcCharacterCodingGlobal[26] = {
        1, 16, 4, 16, 9, 2, 4, 16, 9, 2, 0, 2, 2, 2, 1, 4, 0, 2, 4, 4, 1, 0, 0, 0, 8, 0};
     /* A   B  C   D  E  F  G   H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z */


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngMetaphoneCreateByName()

    Purpose:    This function returns a language metaphone structure for a particular metaphone name 
                and language code combination.

    Parameters: pucMetaphoneName    Metaphone name
                pucLanguageCode     Language code
                ppvLngMetaphone     Return pointer for the language metaphone structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngMetaphoneCreateByName
(
    unsigned char *pucMetaphoneName,
    unsigned char *pucLanguageCode,
    void **ppvLngMetaphone
)
{

    int             iError = LNG_NoError;
    unsigned int    uiMetaphoneID = 0;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucMetaphoneName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucMetaphoneName' parameter passed to 'iLngMetaphoneCreateByName'."); 
        return (LNG_LanguageInvalidMetaphoneName);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngMetaphoneCreateByName'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngMetaphone == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngMetaphone' parameter passed to 'iLngMetaphoneCreateByName'."); 
        return (LNG_ReturnParameterError);
    }
    

    /* Get the metaphone ID */
    if ( (iError = iLngGetMetaphoneIDFromName(pucMetaphoneName, &uiMetaphoneID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the metaphone ID from the metaphone name: '%s', lng error: %d.", pucMetaphoneName, iError); 
        return (iError);
    }

    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngMetaphoneCreateByID(uiMetaphoneID, uiLanguageID, ppvLngMetaphone));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngMetaphoneCreateByID()

    Purpose:    This function returns a language metaphone structure for a particular metaphone ID 
                and language ID combination.

    Parameters: uiMetaphoneID       Metaphone ID
                uiLanguageID        Language ID
                ppvLngMetaphone     Return pointer for the language metaphone structure

    Globals:    plmfLngMetaphoneFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngMetaphoneCreateByID
(
    unsigned int uiMetaphoneID,
    unsigned int uiLanguageID,
    void **ppvLngMetaphone
)
{

    int                             iError = LNG_NoError;
    struct lngMetaphoneFunction     *plmfLngMetaphoneFunctionPtr = plmfLngMetaphoneFunctionListGlobal;
    struct lngMetaphone             *plmLngMetaphone = NULL;


    /* Check the parameters */
    if ( uiMetaphoneID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiMetaphoneID' parameter passed to 'iLngMetaphoneCreateByID'."); 
        return (LNG_LanguageInvalidMetaphoneID);
    }
    
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngMetaphoneCreateByID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( ppvLngMetaphone == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngMetaphone' parameter passed to 'iLngMetaphoneCreateByID'."); 
        return (LNG_ReturnParameterError);
    }
    

    /* Canonicalize the language ID */
    if ( (iError = iLngGetCanonicalLanguageID(uiLanguageID, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiLanguageID, iError); 
        return (iError);
    }


    /* Loop over the metaphone function structures to see which one we want */
    for ( plmfLngMetaphoneFunctionPtr = plmfLngMetaphoneFunctionListGlobal; plmfLngMetaphoneFunctionPtr->iLngMetaphoneFunction != NULL; plmfLngMetaphoneFunctionPtr++ ) {

        /* Select this metaphone function if there is a match */
        if ( ((plmfLngMetaphoneFunctionPtr->ulMetaphoneID == uiMetaphoneID) || (plmfLngMetaphoneFunctionPtr->ulMetaphoneID == LNG_METAPHONE_ANY_ID)) &&
                ((plmfLngMetaphoneFunctionPtr->uiLanguageID == uiLanguageID) || (plmfLngMetaphoneFunctionPtr->uiLanguageID == LNG_LANGUAGE_ANY_ID)) ) {
            
            /* Allocate the language metaphone structure */
            if ( (plmLngMetaphone = (struct lngMetaphone *)s_malloc((size_t)(sizeof(struct lngMetaphone)))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Set the language metaphone structure */
            plmLngMetaphone->uiMetaphoneID = uiMetaphoneID;
            plmLngMetaphone->uiLanguageID = uiLanguageID;
            plmLngMetaphone->iLngMetaphoneFunction = plmfLngMetaphoneFunctionPtr->iLngMetaphoneFunction;

            /* Set the return pointer */
            *ppvLngMetaphone = (void *)plmLngMetaphone;

            return (LNG_NoError);
        }
    }
    

    /* There is no metaphone function available for this metaphone name and language code combination */
    return (LNG_MetaphoneUnavailableMetaphone);

}

/*---------------------------------------------------------------------------*/


/*

    Function:   iLngMetaphoneFree()

    Purpose:    This function frees the language metaphone structure.

    Parameters: pvLngMetaphone      Language metaphone structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngMetaphoneFree
(
    void *pvLngMetaphone
)
{

    struct lngMetaphone     *plmLngMetaphone = (struct lngMetaphone *)pvLngMetaphone;


    /* Check the parameters */
    if ( pvLngMetaphone == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvLngMetaphone' parameter passed to 'iLngMetaphoneFree'."); 
        return (LNG_MetaphoneInvalidMetaphone);
    }

    
    /* Free the language metaphone structure */
    s_free(plmLngMetaphone);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngMetaphoneGetMetaphoneCharacterList()

    Purpose:    This is the router function for the metaphone

    Parameters: pvLngMetaphone                  Language metaphone structure
                pwcTerm                         Pointer to the term for which we want the metaphone
                ppwcMetaphoneCharacterList      Return pointer for the character list to scan for this metaphone key

    Globals:    plmfLngMetaphoneFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngMetaphoneGetMetaphoneCharacterList
(
    void *pvLngMetaphone,
    wchar_t *pwcTerm,
    wchar_t **ppwcMetaphoneCharacterList
)
{

/*     struct lngMetaphone  *plmLngMetaphone = (struct lngMetaphone *)pvLngMetaphone; */
    wchar_t              pwcMetaphoneCharacterList[2] = {L'\0'};
    wchar_t             *pwcMetaphoneCharacterListPtr = NULL;


    /* Check the parameters */
    if ( pvLngMetaphone == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngMetaphone' parameter passed to 'iLngMetaphoneGetMetaphoneCharacterList'."); 
        return (LNG_MetaphoneInvalidMetaphone);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngMetaphoneGetMetaphoneCharacterList'."); 
        return (LNG_MetaphoneInvalidTerm);
    }

    if ( ppwcMetaphoneCharacterList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcMetaphoneCharacterList' parameter passed to 'iLngMetaphoneGetMetaphoneCharacterList'."); 
        return (LNG_ReturnParameterError);
    }


    /* Create the character list */
    pwcMetaphoneCharacterList[0] = pwcTerm[0];
    pwcMetaphoneCharacterList[1] = L'\0';

    /* Duplicate the character list */
    if ( (pwcMetaphoneCharacterListPtr = s_wcsdup(pwcMetaphoneCharacterList)) == NULL ) {
        return (LNG_MemError);
    }
    
    /* Set the return pointer */
    *ppwcMetaphoneCharacterList = pwcMetaphoneCharacterListPtr;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngMetaphoneGetMetaphoneKey()

    Purpose:    This is the router function for the metaphone

    Parameters: plmLngMetaphone     Language metaphone structure
                pwcTerm             Pointer to the term for which we want the metaphone key
                pwcMetaphoneKey     The return parameter for the metaphone key
                uiMetaphoneKeyLen   Length of the return pointer for the metaphone key

    Globals:    plmfLngMetaphoneFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngMetaphoneGetMetaphoneKey
(
    void *pvLngMetaphone,
    wchar_t *pwcTerm,
    wchar_t *pwcMetaphoneKey,
    unsigned int uiMetaphoneKeyLen
)
{

    int                     iError = LNG_NoError;
    struct lngMetaphone     *plmLngMetaphone = (struct lngMetaphone *)pvLngMetaphone;


    /* Check the parameters */
    if ( pvLngMetaphone == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngMetaphone' parameter passed to 'iLngMetaphoneGetMetaphoneKey'."); 
        return (LNG_MetaphoneInvalidMetaphone);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngMetaphoneGetMetaphoneKey'."); 
        return (LNG_MetaphoneInvalidTerm);
    }

    if ( pwcMetaphoneKey == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pwcMetaphoneKey' parameter passed to 'iLngMetaphoneGetMetaphoneKey'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiMetaphoneKeyLen <= LNG_METAPHONE_KEY_LENGTH ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiMetaphoneKeyLen' parameter passed to 'iLngMetaphoneGetMetaphoneKey'."); 
        return (LNG_ParameterError);
    }


    /* Check that the metaphone function is valid */
    if ( plmLngMetaphone->iLngMetaphoneFunction == NULL ) {
        return (LNG_MetaphoneInvalidMetaphone);
    }

    /* Call the metaphone function */
    if ( (iError = plmLngMetaphone->iLngMetaphoneFunction(plmLngMetaphone, pwcTerm, pwcMetaphoneKey, uiMetaphoneKeyLen)) != LNG_NoError ) {
        return (LNG_MetaphoneMetaphoningFailed);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngMetaphoneGetKey_un()

    Purpose:    To get a term's metaphone key.

    Parameters: plmLngMetaphone         Language metaphone structure
                pwcTerm                 Pointer to the term for which we want the metaphone key
                pwcMetaphoneKey         The return parameter for the metaphone key
                uiMetaphoneKeyLen       Length of the return pointer for the metaphone key

    Globals:    none

    Returns:    LNG error code

*/
static int iLngMetaphoneGetKey_un
(
    struct lngMetaphone *plmLngMetaphone,
    wchar_t *pwcTerm,
    wchar_t *pwcMetaphoneKey,
    unsigned int uiMetaphoneKeyLen
)
{


    wchar_t     *pwcTermPtr = pwcTerm;
    wchar_t     *pwcMetaphoneKeyPtr = pwcMetaphoneKey;
    wchar_t     *pwcBuffer = NULL;
    wchar_t     *pwcBufferPtr = NULL;


    ASSERT(plmLngMetaphone != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(pwcMetaphoneKey != NULL);
    ASSERT(uiMetaphoneKeyLen > LNG_METAPHONE_KEY_LENGTH);


    /* Init the key */
    pwcMetaphoneKey[0] = L'\0';


    /* Allocate the buffer */
    if ( (pwcBuffer = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (s_wcslen(pwcTerm) + 2)))) == NULL ) {
        return (LNG_MemError);
    }


    /* Copy term to internal buffer, dropping non-alphabetic characters, converting to upper case and stripping the accents */
    for ( pwcTermPtr = pwcTerm, pwcBufferPtr = pwcBuffer + 1; *pwcTermPtr != L'\0'; pwcTermPtr++ ) {
        if ( iswalpha(*pwcTermPtr) != 0 ) {
            *pwcBufferPtr = wcLngCaseStripAccentFromWideCharacter(towupper(*pwcTermPtr));
            pwcBufferPtr++;
        }
    }
    *pwcBufferPtr = L'\0';


    /* Set the pointer to the start of the buffer */
    pwcTermPtr = pwcBuffer + 1;


    /* Check for PN, KN, GN, AE, WR, WH, and X at start */
    switch ( *pwcTermPtr ) {

        case L'P':
        case L'K':
        case L'G':
            /* 'PN', 'KN', 'GN' becomes 'N' */
            if (*(pwcTermPtr + 1) == L'N') {
                pwcTermPtr++;
            }
            break;

        case L'A':
            /* 'AE' becomes 'E' */
            if (*(pwcTermPtr + 1) == L'E') {
                pwcTermPtr++;
            }
            break;

        case L'W':
            /* 'WR' becomes 'R', and 'WH' to 'H' */
            if (*(pwcTermPtr + 1) == L'R') {
                pwcTermPtr++;
            }
            else if (*(pwcTermPtr + 1) == L'H') {
                *(pwcTermPtr + 1) = *pwcTermPtr;
                pwcTermPtr++;
            }
            break;

        case L'X':
            /* 'X' becomes 'S' */
            *pwcTermPtr = L'S';
            break;
    }



    /* Now, loop step through string, stopping at end of string or when
    ** the computed 'metaphone' is LNG_METAPHONE_KEY_LENGTH characters long
    */
    for ( pwcMetaphoneKeyPtr = pwcMetaphoneKey; (s_wcslen(pwcMetaphoneKey) < LNG_METAPHONE_KEY_LENGTH) && (*pwcTermPtr != L'\0'); pwcTermPtr++ ) {

        /* Drop duplicates except for CC */
        if ( (*(pwcTermPtr - 1) == *pwcTermPtr) && (*pwcTermPtr != L'C') ) {
            continue;
        }

        /* Check for F J L M N R or first letter vowel */
        if ( LNG_METAPHONE_IS_SAME(*pwcTermPtr) || ((*(pwcTermPtr - 1) == L'\0') && (LNG_METAPHONE_IS_VOWEL(*pwcTermPtr))) ) {
            *pwcMetaphoneKeyPtr = *pwcTermPtr;
            *(++pwcMetaphoneKeyPtr) = L'\0';
        }
        else {

            switch ( *pwcTermPtr ) {

                case L'B':
                    /* B unless in -MB */
                    if ( *(pwcTermPtr + 1) || *(pwcTermPtr - 1) != L'M' ) {
                        *pwcMetaphoneKeyPtr = *pwcTermPtr;
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    break;


                case L'C':

                    /* X if in -CIA-, -CH- else S if in
                    ** -CI-, -CE-, -CY- else dropped if
                    ** in -SCI-, -SCE-, -SCY- else K
                    */
                    if ( *(pwcTermPtr - 1) != L'S' || !LNG_METAPHONE_IS_FRONTV(*(pwcTermPtr + 1)) ) {
                        if ( *(pwcTermPtr + 1) == L'I' && *(pwcTermPtr + 2) == L'A' ) {
                            *pwcMetaphoneKeyPtr = L'X';
                            pwcMetaphoneKeyPtr++;
                            *pwcMetaphoneKeyPtr = L'\0';
                        }
                        else if ( LNG_METAPHONE_IS_FRONTV(*(pwcTermPtr + 1)) ) {
                            *pwcMetaphoneKeyPtr = L'S';
                            pwcMetaphoneKeyPtr++;
                            *pwcMetaphoneKeyPtr = L'\0';
                        }
                        else if ( *(pwcTermPtr + 1) == L'H' ) {
                            *pwcMetaphoneKeyPtr = (((*(pwcTermPtr - 1) == L'\0' && !LNG_METAPHONE_IS_VOWEL(*(pwcTermPtr + 2))) || *(pwcTermPtr - 1) == L'S') ? L'K' : L'X');
                            pwcMetaphoneKeyPtr++;
                            *pwcMetaphoneKeyPtr = L'\0';
                        }
                        else {
                            *pwcMetaphoneKeyPtr = L'K';
                            pwcMetaphoneKeyPtr++;
                            *pwcMetaphoneKeyPtr = L'\0';
                        }
                    }
                    break;


                case L'D':

                    /* J if in DGE or DGI or DGY else T */
                    *pwcMetaphoneKeyPtr = ((*(pwcTermPtr + 1) == L'G' && LNG_METAPHONE_IS_FRONTV(*(pwcTermPtr + 2))) ? L'J' : L'T');
                    *(++pwcMetaphoneKeyPtr) = L'\0';
                    break;


                case L'G':

                    /* F if in -GH and not B--GH, D--GH,
                    ** -H--GH, -H---GH else dropped if
                    ** -GNED, -GN, -DGE-, -DGI-, -DGY-
                    ** else J if in -GE-, -GI-, -GY- and
                    ** not GG else K
                    */
                    if ( (*(pwcTermPtr + 1) != L'J' || LNG_METAPHONE_IS_VOWEL(*(pwcTermPtr + 2))) &&
                            (*(pwcTermPtr + 1) != L'N' || (*(pwcTermPtr + 1) &&  (*(pwcTermPtr + 2) != L'E' || *(pwcTermPtr + 3) != L'D'))) &&
                            (*(pwcTermPtr - 1) != 'D' || !LNG_METAPHONE_IS_FRONTV(*(pwcTermPtr + 1))) ) {
                        *pwcMetaphoneKeyPtr = (LNG_METAPHONE_IS_FRONTV(*(pwcTermPtr + 1)) && *(pwcTermPtr + 2) != L'G') ? L'G' : L'K';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    else if ( *(pwcTermPtr + 1) == L'H' && !LNG_METAPHONE_IS_NOGHF(*(pwcTermPtr - 3)) && *(pwcTermPtr - 4) != L'H' ) {
                        *pwcMetaphoneKeyPtr = L'F';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    break;


                case L'H':

                    /* H if before a vowel and not after
                    ** C, G, P, S, T else dropped
                    */
                    if ( !LNG_METAPHONE_IS_VARSON(*(pwcTermPtr - 1)) && (!LNG_METAPHONE_IS_VOWEL(*(pwcTermPtr - 1  )) || LNG_METAPHONE_IS_VOWEL(*(pwcTermPtr + 1))) ) {
                        *pwcMetaphoneKeyPtr = L'H';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    break;


                case L'K':

                    /* Dropped if after C else K */
                    if ( *(pwcTermPtr - 1) != L'C' ) {
                        *pwcMetaphoneKeyPtr = L'K';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    break;


                case L'P':

                    /* F if before H, else P */
                    *pwcMetaphoneKeyPtr = (*(pwcTermPtr + 1) == L'H' ? L'F' : L'P');
                    *(++pwcMetaphoneKeyPtr) = L'\0';
                    break;


                case L'Q':

                    /* K */
                    *pwcMetaphoneKeyPtr = L'K';
                    *(++pwcMetaphoneKeyPtr) = L'\0';
                    break;


                case L'S':

                    /* X in -SH-, -SIO- or -SIA- else S */
                    *pwcMetaphoneKeyPtr = ((*(pwcTermPtr + 1) == L'H' || (*(pwcTermPtr + 1) == L'I' && (*(pwcTermPtr + 2) == L'O' || *(pwcTermPtr + 2) == L'A'))) ? L'X' : L'S');
                    *(++pwcMetaphoneKeyPtr) = L'\0';
                    break;


                case L'T':

                    /* X in -TIA- or -TIO- else 0 (zero)
                    ** before H else dropped if in -TCH-
                    ** else T
                    */
                    if ( *(pwcTermPtr + 1) == L'I' && (*(pwcTermPtr + 2) == L'O' || *(pwcTermPtr + 2) == L'A') ) {
                        *pwcMetaphoneKeyPtr = L'X';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    else if ( *(pwcTermPtr + 1) == L'H' ) {
                        *pwcMetaphoneKeyPtr = L'0';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    else if ( *(pwcTermPtr + 1) != L'C' || *(pwcTermPtr + 2) != L'H' ) {
                        *pwcMetaphoneKeyPtr = L'T';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    break;


                case L'V':

                    /* F */
                    *pwcMetaphoneKeyPtr = L'F';
                    *(++pwcMetaphoneKeyPtr) = L'\0';
                    break;


                case L'W':

                    /* W after a vowel, else dropped */


                case L'Y':

                    /* Y unless followed by a vowel */
                    if ( LNG_METAPHONE_IS_VOWEL(*(pwcTermPtr + 1)) ) {
                        *pwcMetaphoneKeyPtr = *pwcTermPtr;
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    break;


                case L'X':

                    /* KS */
                    if ( *(pwcTermPtr - 1) == L'\0' ) {
                        *pwcMetaphoneKeyPtr = L'S';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    else {
                        *pwcMetaphoneKeyPtr = L'K';        /* Insert K, then S */
                        *(++pwcMetaphoneKeyPtr) = L'S';
                        *(++pwcMetaphoneKeyPtr) = L'\0';
                    }
                    break;


                case L'Z':

                    /* S */
                    *pwcMetaphoneKeyPtr = L'S';
                    *(++pwcMetaphoneKeyPtr) = L'\0';
                    break;
            }
        }
    }


    /* Free the buffer */
    s_free(pwcBuffer);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/



