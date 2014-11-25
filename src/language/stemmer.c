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

    Module:     stemmer.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This file contains various stemming function

                Note for more information about the porter stemmer, see:

                Information Retrieval, Data Structures & Algorithms by
                W. Frakes and R. Baeza-Yates, Editors, 
                Published by Prentice Hall, 1992
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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.stemmer"


/*---------------------------------------------------------------------------*/

/*
** Structures
*/

/* Language stemmer function structure */
struct lngStemmerFunction {
    unsigned int    uiStemmerID;                    /* Stemmer ID */
    unsigned int    uiLanguageID;                   /* Language ID */
    int             (*iLngStemmerFunction)();       /* Stemmer function pointer */
};


/* Language stemmer structure */
struct lngStemmer {
    unsigned int    uiStemmerID;                    /* Stemmer ID */
    unsigned int    uiLanguageID;                   /* Language ID */
    int             (*iLngStemmerFunction)();       /* Stemmer function pointer (denormalization) */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLngStemmerNone_un (struct lngStemmer *plsLngStemmer, void *pvTerm, unsigned int uiTermLength);


static int iLngStemmerPlural_en (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_fr (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_es (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_pt (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_gl (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_it (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_de (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_nl (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_sv (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_no (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_da (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_bg (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);
static int iLngStemmerPlural_un (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);

static boolean bLngStemmerPluralIsVowel (wchar_t wcCharacter);


static int iLngStemmerPorter_en (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);


static int iLngStemmerLovins_en (struct lngStemmer *plsLngStemmer, wchar_t *pwcTerm, unsigned int uiTermLength);



/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Stemmer functions */
static struct lngStemmerFunction plsfLngStemmerFunctionListGlobal[] = 
{
    /* This catches all cases where no stemmer is selected */
    {   LNG_STEMMER_NONE_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngStemmerNone_un,
    },
    
    /* Plural stemmer - English */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_EN_ID,
        iLngStemmerPlural_en,
    },

    /* Plural stemmer - French */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_FR_ID,
        iLngStemmerPlural_fr,
    },

    /* Plural stemmer - Spanish */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_ES_ID,
        iLngStemmerPlural_es,
    },

    /* Plural stemmer - Portuguese */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_PT_ID,
        iLngStemmerPlural_pt,
    },

    /* Plural stemmer - Galician */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_GL_ID,
        iLngStemmerPlural_gl,
    },

    /* Plural stemmer - Italian */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_IT_ID,
        iLngStemmerPlural_it,
    },

    /* Plural stemmer - German */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_DE_ID,
        iLngStemmerPlural_de,
    },

    /* Plural stemmer - Dutch */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_NL_ID,
        iLngStemmerPlural_nl,
    },

    /* Plural stemmer - Swedish */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_SV_ID,
        iLngStemmerPlural_sv,
    },

    /* Plural stemmer - Norwegian */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_NO_ID,
        iLngStemmerPlural_no,
    },

    /* Plural stemmer - Danish */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_DA_ID,
        iLngStemmerPlural_da,
    },

    /* Plural stemmer - Bulgarian */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_BG_ID,
        iLngStemmerPlural_bg,
    },

    /* Plural stemmer - Generic */
    {   LNG_STEMMER_PLURAL_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngStemmerPlural_un,
    },

    /* Porter stemmer */
    {   LNG_STEMMER_PORTER_ID,
        LNG_LANGUAGE_EN_ID,
        iLngStemmerPorter_en,
    },

    /* Lovins stemmer */
    {   LNG_STEMMER_LOVINS_ID,
        LNG_LANGUAGE_EN_ID,
        iLngStemmerLovins_en,
    },

    /* This catches cases where we have no stemmer defined for the 
    ** stemmer/language/character set combination so that we don't 
    ** just error out
    */
    {   LNG_STEMMER_ANY_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngStemmerNone_un,
    },

    /* Terminator */
    {   0,
        0,
        NULL,
    },
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerCreateByName()

    Purpose:    This function returns a language stemmer structure for a particular stemmer name
                and language code combination.

    Parameters: pucStemmerName      Stemmer name
                pucLanguageCode     Language code
                ppvLngStemmer       Return pointer for the language stemmer structure

    Globals:    plsfLngStemmerFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngStemmerCreateByName
(
    unsigned char *pucStemmerName,
    unsigned char *pucLanguageCode,
    void **ppvLngStemmer
)
{

    int             iError = LNG_NoError;
    unsigned int    uiStemmerID = 0;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStemmerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStemmerName' parameter passed to 'iLngStemmerCreateByName'."); 
        return (LNG_LanguageInvalidStemmerName);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngStemmerCreateByName'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngStemmer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngStemmer' parameter passed to 'iLngStemmerCreateByName'."); 
        return (LNG_ReturnParameterError);
    }
    

    /* Get the stemmer ID */
    if ( (iError = iLngGetStemmerIDFromName(pucStemmerName, &uiStemmerID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer ID from the stemmer name: '%s', lng error: %d.", pucStemmerName, iError); 
        return (iError);
    }

    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngStemmerCreateByID(uiStemmerID, uiLanguageID, ppvLngStemmer));


}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerCreateByID()

    Purpose:    This function returns a language stemmer structure for a particular stemmer name 
                and language ID combination.

    Parameters: uiStemmerID     Stemmer ID
                uiLanguageID    Language ID
                ppvLngStemmer   Return pointer for the language stemmer structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngStemmerCreateByID
(
    unsigned int uiStemmerID,
    unsigned int uiLanguageID,
    void **ppvLngStemmer
)
{

    int                         iError = LNG_NoError;
    struct lngStemmerFunction   *plsfLngStemmerFunctionPtr = plsfLngStemmerFunctionListGlobal;
    struct lngStemmer           *plsLngStemmer = NULL;


    /* Check the parameters */
    if ( uiStemmerID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStemmerID' parameter passed to 'iLngStemmerCreateByID'."); 
        return (LNG_LanguageInvalidStemmerID);
    }
    
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngStemmerCreateByID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( ppvLngStemmer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngStemmer' parameter passed to 'iLngStemmerCreateByID'."); 
        return (LNG_ReturnParameterError);
    }
    

    /* Canonicalize the language ID */
    if ( (iError = iLngGetCanonicalLanguageID(uiLanguageID, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiLanguageID, iError); 
        return (iError);
    }


    /* Loop over the stemmer function structures to see which one we want */
    for ( plsfLngStemmerFunctionPtr = plsfLngStemmerFunctionListGlobal;    plsfLngStemmerFunctionPtr->iLngStemmerFunction != NULL; plsfLngStemmerFunctionPtr++ ) {

        /* Select this stemmer function if there is a match */
        if ( ((plsfLngStemmerFunctionPtr->uiStemmerID == uiStemmerID) || (plsfLngStemmerFunctionPtr->uiStemmerID == LNG_STEMMER_ANY_ID)) &&
                ((plsfLngStemmerFunctionPtr->uiLanguageID == uiLanguageID) || (plsfLngStemmerFunctionPtr->uiLanguageID == LNG_LANGUAGE_ANY_ID)) ) {
            
            /* Allocate the language stemmer structure */
            if ( (plsLngStemmer = (struct lngStemmer *)s_malloc((size_t)(sizeof(struct lngStemmer)))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Set the language stemmer structure */
            plsLngStemmer->uiStemmerID = uiStemmerID;
            plsLngStemmer->uiLanguageID = uiLanguageID;
            plsLngStemmer->iLngStemmerFunction = plsfLngStemmerFunctionPtr->iLngStemmerFunction;

            /* Set the return pointer */
            *ppvLngStemmer = (void *)plsLngStemmer;

            return (LNG_NoError);
        }
    }
    

    /* There is no stemmer function available for this stemmer name and language code combination */
    return (LNG_StemmerUnavailableStemmer);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerFree()

    Purpose:    This function frees the language stemmer structure.

    Parameters: pvLngStemmer    Language stemmer structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngStemmerFree
(
    void *pvLngStemmer
)
{

    struct lngStemmer   *plsLngStemmer = (struct lngStemmer *)pvLngStemmer;


    /* Check the parameters */
    if ( pvLngStemmer == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvLngStemmer' parameter passed to 'iLngStemmerFree'."); 
        return (LNG_StemmerInvalidStemmer);
    }


    /* Free the language stemmer structure */
    s_free(plsLngStemmer);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerStemTerm()

    Purpose:    This is the router function for the stemmer

    Parameters: pvLngStemmer    Language stemmer structure
                pwcTerm         Pointer to the term being stemmed
                uiTermLength    The term length if known (0 if unknown)

    Globals:    

    Returns:    An LNG error code

*/
int iLngStemmerStemTerm
(
    void *pvLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    struct lngStemmer   *plsLngStemmer = (struct lngStemmer *)pvLngStemmer;
    int                 iError = LNG_NoError;


    /* Check the parameters */
    if ( pvLngStemmer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngStemmer' parameter passed to 'iLngStemmerStemTerm'."); 
        return (LNG_StemmerInvalidStemmer);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngStemmerStemTerm'."); 
        return (LNG_StemmerInvalidTerm);
    }

    if ( uiTermLength < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermLength' parameter passed to 'iLngStemmerStemTerm'."); 
        return (LNG_ParameterError);
    }


    /* Check that the stemmer function is valid */
    if ( plsLngStemmer->iLngStemmerFunction == NULL ) {
        return (LNG_StemmerInvalidStemmer);
    }


    /* Get the term length */
    uiTermLength = (uiTermLength == 0) ? s_wcslen(pwcTerm) : uiTermLength;


    /* Call the stemmer function */
    if ( uiTermLength > 0 ) {
        if ( (iError = plsLngStemmer->iLngStemmerFunction(plsLngStemmer, pwcTerm, uiTermLength)) != LNG_NoError ) {
            return (LNG_StemmerStemmingFailed);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*
** No Stemmer
*/


/*

    Function:   iLngStemmerNone_un()

    Purpose:    Does not stem a term

    Parameters: plsLngStemmer   Language stemmer structure
                pvTerm          The term to stem
                uiTermLength     The term length if known

    Globals:    none

    Returns:    LNG error code

*/
static int iLngStemmerNone_un
(
    struct lngStemmer *plsLngStemmer,
    void *pvTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*
** Plural Stemmers
*/


/*

    Function:   iLngStemmerPlural_en()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer   Language stemmer structure
                pwcTerm         The term to stem
                uiTermLength    The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_en
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 2 characters long, it ends in 's', 
    ** and if the penultimate character is not punctuation like "Alyce's"
    */
    if ( (uiTermLength > 2) && (towlower(pwcTerm[uiTermLength - 1]) == 's') && (iswpunct(pwcTerm[uiTermLength - 2]) == 0) ) {

        switch ( towlower(pwcTerm[uiTermLength - 2]) ) {

            case L'e':    
                /* Still 2 possibilities - 'es', 'ies' */
                if ( (uiTermLength > 3) && (tolower(pwcTerm[uiTermLength - 3]) == L'i') && (towlower(pwcTerm[uiTermLength - 4]) != L'e') && 
                        (towlower(pwcTerm[uiTermLength - 4]) != L'a') ) {
                    /* Convert -'ies' to -'y' */
                    pwcTerm[uiTermLength - 3] = L'y';
                    pwcTerm[uiTermLength - 2] = L'\0';
                }

                /* Consider terms ending in  -'shes', -'ches', -'xes', -'sses' from Hodge's Harbrace College Handbook */
                else if ( (uiTermLength > 4) && ((((towlower(pwcTerm[uiTermLength - 3]) == L'h') && (towlower(pwcTerm[uiTermLength - 4]) == L's'))) ||  
                        ((towlower(pwcTerm[uiTermLength - 3]) == L'h') && (towlower(pwcTerm[uiTermLength - 4]) == L'c')) ||  
                        ((towlower(pwcTerm[uiTermLength - 3]) == L's') && (towlower(pwcTerm[uiTermLength - 4]) == L's')) ||  
                        ((towlower(pwcTerm[uiTermLength - 3]) == L'x')))) {
                    /* Convert -'es' to -'' */
                    pwcTerm[uiTermLength - 2] = L'\0';
                }

                /* Consider terms ending in -?'es' where ? is not a vowel */
                else if ( (uiTermLength > 3) && (towlower(pwcTerm[uiTermLength - 3]) != L'a') && (towlower(pwcTerm[uiTermLength - 3]) != L'e') && 
                        (towlower(pwcTerm[uiTermLength - 3]) != L'i') && (towlower(pwcTerm[uiTermLength - 3]) != L'o')) {
                    /* Convert -?'es' to -?'e' */
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
                break;

            
            /* Do not remove final 's' */
            case L'u':    
            case L's':
                break;

            
            /* Remove final 's' */
            default:     
                /* Convert -'s' => -'\0' */
                pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_fr()

    Purpose:    Stems a term down to its singular

                This French stemmer is provided for distribution without 
                restriction by permission of its developer:
                    Jacques Savoy
                    Université de Neuchâtel
                    Faculté de droit et de sciences économiques

                For information about its characteristics see:
                    Savoy, Jacques and Le Calvé, Anne:
                    "Rapport d'experience Amaryllis 1997",
                    Cahier de recherche en informatique CR-I-97-03.

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_fr
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 5 characters long */
    if ( uiTermLength > 5 ) {
        
        /* Handle -'x' */
        if ( towlower(pwcTerm[uiTermLength - 1]) == L'x' ) {
            
            /* Convert -'aux' to -'alx' */
            if ( (towlower(pwcTerm[uiTermLength - 2]) == L'u') && (towlower(pwcTerm[uiTermLength - 3]) == L'a') ) {
                pwcTerm[uiTermLength - 2] = L'l';
            }
            
            /* Convert -'x' to -'\0' */
            pwcTerm[uiTermLength - 1] = L'\0';
        }
        else {
    
            /* Convert -'s' to -'\0' */
            if  ( towlower(pwcTerm[uiTermLength - 1]) == L's' ) {
                pwcTerm[uiTermLength - 1] = L'\0';
            }
            
            /* Convert -'r?' to -'\0?' */
            if ( towlower(pwcTerm[uiTermLength - 2]) == L'r' ) {
                pwcTerm[uiTermLength - 2] = L'\0';
            }
    
            /* Convert -'e??' to -'\0??' */
            if ( towlower(pwcTerm[uiTermLength - 3]) == L'e' ) {
                pwcTerm[uiTermLength - 3] = L'\0';
            }
    
            /* Convert -'é???' to -'\0???' */
            if ( towlower(pwcTerm[uiTermLength - 4]) == 0xE9 ) {
                pwcTerm[uiTermLength - 4] = L'\0';
            }
    
            /* Convert -'**????' to -'*\0????' */
            if ( (towlower(pwcTerm[uiTermLength - 5]) == towlower(pwcTerm[uiTermLength - 5])) && (towlower(pwcTerm[uiTermLength - 6]) == towlower(pwcTerm[uiTermLength - 5])) ) {
                pwcTerm[uiTermLength - 5] = L'\0';
            }
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_es()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_es
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 4 characters long */
    if ( uiTermLength > 4 ) {
    
        switch ( wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 1])) ) {
        
            case L's':
            
                /* Convert -'eses' to -'es' */
                if ( (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'e') && 
                        (towlower(pwcTerm[uiTermLength - 3]) == L's') && 
                        (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 4])) == L'e') ) {
                    pwcTerm[uiTermLength - 2] = L'\0';
                }
                
                /* Convert -'ces' -> -'ez' */
                else if ( (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'e') && 
                        (towlower(pwcTerm[uiTermLength - 3]) == L'c') ) {
                    pwcTerm[uiTermLength - 3] = L'z';
                    pwcTerm[uiTermLength - 2] = L'\0';
                }
            
                /* Convert -'os', -'as' and -'es' to -'\0' */
                else if ( (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'o') || 
                        (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'a') || 
                        (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'e') ) {  
                    pwcTerm[uiTermLength - 2] = L'\0';
                }
                break;


            case L'o':
            case L'a':
            case L'e':
                /* Convert -'o', -'a' and -'e' to -'\0' */
                pwcTerm[uiTermLength - 1] = L'\0';  
                break;


            default:     
                break;
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_pt()

    Purpose:    Stems a term down to its singular

                This module implements a Portuguese stemming 
                algorithm proposed in the paper:

                    A Stemming Algorithm for the Portuguese Language 
                    by Moreira, V. and Huyck, C.
                    
                Only the plural portion of the stemmer is implemented
                as this is all we want, the code was ported from the 
                following perl module:
                
                    http://www.cpan.org/authors/id/X/XE/XERN/Lingua-PT-Stemmer-0.01.tar.gz

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_pt
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 2 characters long, it ends in 's' */
    if ( (uiTermLength > 2) && (towlower(pwcTerm[uiTermLength - 1]) == 's') ) {

        switch ( towlower(pwcTerm[uiTermLength - 2]) ) {
            
            case L'e':  
                
                /* Convert -'ões' and -'ães' to -'ão\0' */
                if ( ((uiTermLength > 5) && (towlower(pwcTerm[uiTermLength - 3]) == 0xF5)) ||
                        ((uiTermLength > 3) && (towlower(pwcTerm[uiTermLength - 3]) == 0xE3)) ) {
                    pwcTerm[uiTermLength - 3] = 0xE3;
                    pwcTerm[uiTermLength - 2] = L'o';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }

                /* Convert -'res' to -'r\0' */
                else if ( (uiTermLength > 5) && (towlower(pwcTerm[uiTermLength - 3]) == L'r') ) {
                    pwcTerm[uiTermLength - 3] = L'r';
                    pwcTerm[uiTermLength - 2] = L'\0';
                }

                /* Convert -'les' to -'l\0' */
                else if ( (uiTermLength > 4) && (towlower(pwcTerm[uiTermLength - 3]) == L'l') ) {
                    pwcTerm[uiTermLength - 3] = L'l';
                    pwcTerm[uiTermLength - 2] = L'\0';
                }
                
                /* Convert -'es' => -'e\0' */
                else {
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
                break;


            case L'i':  
                
                /* Convert -'éis' and -'eis' to -'el\0' */
                if ( (uiTermLength > 4) && ((towlower(pwcTerm[uiTermLength - 3]) == 0xE9) || (towlower(pwcTerm[uiTermLength - 3]) == L'e')) ) {
                    pwcTerm[uiTermLength - 3] = L'e';
                    pwcTerm[uiTermLength - 2] = L'l';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
                
                /* Convert -'óis' to -'ol\0' */
                else if ( (uiTermLength > 4) && (towlower(pwcTerm[uiTermLength - 3]) == 0xF3) ) {
                    pwcTerm[uiTermLength - 3] = L'o';
                    pwcTerm[uiTermLength - 2] = L'l';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
            
                /* Convert -'ais' to -'al\0' */
                else if ( (uiTermLength > 3) && (towlower(pwcTerm[uiTermLength - 3]) == L'a') ) {
                    pwcTerm[uiTermLength - 3] = L'a';
                    pwcTerm[uiTermLength - 2] = L'l';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
            
                /* Convert -'is' to -'il' */
                else if ( uiTermLength > 3 ) {
                    pwcTerm[uiTermLength - 2] = L'i';
                    pwcTerm[uiTermLength - 1] = L'l';
                }
                break;


            /* convert -'ns'  to 'm' */
            case L'n':
                /* convert -'ns'  => 'm\0' */
                pwcTerm[uiTermLength - 2] = L'm';
                pwcTerm[uiTermLength - 1] = L'\0';
                break;

                
            /* Remove final 's' */
            default:     
                /* Convert -'s' to -'\0' */
                pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_gl()

    Purpose:    Stems a term down to its singular

                Galician is an endangered language spoken in northwest 
                region of Spain. Galician is morphologically similar to 
                Portuguese but phonetics differs greatly. Due to the 
                morphological similarity between Portuguese and Galician, 
                Portuguese stemming algorithms can be adopted to stem 
                Galician texts.

                See http://bvg.udc.es/recursos_lingua/stemming.html
                for stemming rules.

                Only the plural portion of the stemmer is implemented
                as this is all we want, the code was ported from the 
                following perl module:
                
                    http://www.cpan.org/authors/id/X/XE/XERN/Lingua-PT-Stemmer-0.01.tar.gz

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_gl
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 2 characters long, it ends in 's' */
    if ( (uiTermLength > 2) && (towlower(pwcTerm[uiTermLength - 1]) == 's') ) {

        switch ( towlower(pwcTerm[uiTermLength - 2]) ) {
            
            case L'e':  
                
                /* Convert -'ões' to -'ãn\0' */
                if ( (uiTermLength > 5) && (towlower(pwcTerm[uiTermLength - 3]) == 0xF5) ) {
                    pwcTerm[uiTermLength - 3] = 0xE3;
                    pwcTerm[uiTermLength - 2] = L'n';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }

                /* Convert -'ães' to -'ão\0' */
                else if ( (uiTermLength > 3) && (towlower(pwcTerm[uiTermLength - 3]) == 0xE3) ) {
                    pwcTerm[uiTermLength - 3] = 0xE3;
                    pwcTerm[uiTermLength - 2] = L'o';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }

                /* Convert -'res' to -'r\0' */
                else if ( (uiTermLength > 5) && (towlower(pwcTerm[uiTermLength - 3]) == L'r') ) {
                    pwcTerm[uiTermLength - 3] = L'r';
                    pwcTerm[uiTermLength - 2] = L'\0';
                }

                /* Convert -'les' to -'l\0' */
                else if ( (uiTermLength > 4) && (towlower(pwcTerm[uiTermLength - 3]) == L'l') ) {
                    pwcTerm[uiTermLength - 3] = L'l';
                    pwcTerm[uiTermLength - 2] = L'\0';
                }
                
                /* Convert -'s' => -'\0' */
                else {
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
                break;


            case L'i':  
                
                /* Convert -'éis' and -'eis' to -'el\0' */
                if ( (uiTermLength > 4) && ((towlower(pwcTerm[uiTermLength - 3]) == 0xE9) || (towlower(pwcTerm[uiTermLength - 3]) == L'e')) ) {
                    pwcTerm[uiTermLength - 3] = L'e';
                    pwcTerm[uiTermLength - 2] = L'l';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
                
                /* Convert -'óis' and -'ois' to -'ol\0' */
                else if ( (uiTermLength > 4) && ((towlower(pwcTerm[uiTermLength - 3]) == 0xF3) || (towlower(pwcTerm[uiTermLength - 3]) == L'o')) ) {
                    pwcTerm[uiTermLength - 3] = L'o';
                    pwcTerm[uiTermLength - 2] = L'l';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
            
                /* Convert -'ais' to -'al\0' */
                else if ( (uiTermLength > 3) && (towlower(pwcTerm[uiTermLength - 3]) == L'a') ) {
                    pwcTerm[uiTermLength - 3] = L'a';
                    pwcTerm[uiTermLength - 2] = L'l';
                    pwcTerm[uiTermLength - 1] = L'\0';
                }
                break;


            case 0xED:    /* í */
                
                /* Convert -'ís' to -'il' */
                if ( uiTermLength > 3 ) {
                    pwcTerm[uiTermLength - 2] = L'i';
                    pwcTerm[uiTermLength - 1] = L'l';
                }
                break;
                
                
            /* convert -'ns'  to 'n' */
            case L'n':
                /* convert -'ns'  => 'n\0' */
                pwcTerm[uiTermLength - 2] = L'n';
                pwcTerm[uiTermLength - 1] = L'\0';
                break;

            
            /* Remove final 's' */
            default:     
                /* Convert -'s' to -'\0' */
                pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_it()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_it
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 5 characters long */
    if ( uiTermLength > 5 ) {
    
        switch ( wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 1])) ) {

            case L'e':
            case L'i':
            
                /* Convert -'ie', -'he', -'ii' and -'hi' to -'\0' */
                if ( (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'i') || (towlower(pwcTerm[uiTermLength - 2]) == L'h') ) {
                    pwcTerm[uiTermLength - 2] = L'\0';
                }
                
                /* Convert -'e' or -'i' to -'\0' */
                pwcTerm[uiTermLength - 1] = L'\0';
                break;


            case L'o':
            case L'a':
            
                /* Convert -'io' and -'ia' to -'\0' */
                if ( wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'i' ) {
                    pwcTerm[uiTermLength - 2] = L'\0';
                }
                
                /* Convert -'o' or -'a' to -'\0' */
                pwcTerm[uiTermLength - 1] = L'\0';
                break;
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_de()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_de
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 4 characters long */
    if ( uiTermLength > 4 ) {
    
        /* Convert -'nen' to -'\0' */
        if ( (uiTermLength > 6) && (towlower(pwcTerm[uiTermLength - 1]) == L'n') && (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'e') && 
                (towlower(pwcTerm[uiTermLength - 3]) == L'n') ) {  
            pwcTerm[uiTermLength - 3] = L'\0';
        }

        /* Convert -'en', -'es', -'er' to -'\0' */
        else if ( (uiTermLength > 5) && ((towlower(pwcTerm[uiTermLength - 1]) == L'n') || (towlower(pwcTerm[uiTermLength - 1]) == L's') || (towlower(pwcTerm[uiTermLength - 1]) == L'r')) &&
                (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'e') ) {
            pwcTerm[uiTermLength - 2] = L'\0';  
        }

        /* Convert -'se' to -'\0' */
        else if ( (uiTermLength > 5) && (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 1])) == L'e') && (towlower(pwcTerm[uiTermLength - 2]) == L's') ) {
            pwcTerm[uiTermLength - 2] = L'\0';  
        }

        /* Convert -'n', -'s', -'r' and -'e' to -'\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == L'n') || (towlower(pwcTerm[uiTermLength - 1]) == L's') || (towlower(pwcTerm[uiTermLength - 1]) == L'r') || 
                (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 1])) == L'e') ) {
            pwcTerm[uiTermLength - 1] = L'\0';  
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_nl()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_nl
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 4 characters long */
    if ( uiTermLength > 4 ) {
        
        /* Convert -'en' to -'\0' if preceeded by a consonant */
        if ( (towlower(pwcTerm[uiTermLength - 1]) == L'n') && (wcLngCaseStripAccentFromWideCharacter(towlower(pwcTerm[uiTermLength - 2])) == L'e') &&
                (bLngStemmerPluralIsVowel(pwcTerm[uiTermLength - 3]) == false) ) {
            pwcTerm[uiTermLength - 2] = L'\0';
        }
        
        /* Convert -'s' to -'\0' if preceeded by a consonant except for 'j' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == L's') && (towlower(pwcTerm[uiTermLength - 2]) != L'j') &&
                (bLngStemmerPluralIsVowel(pwcTerm[uiTermLength - 2]) == false) ) {
            pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_sv()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_sv
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 4 characters long */
    if ( uiTermLength > 4 ) {
        
        /* Convert -'s' to -'\0' if the preceeding character one of 'bcdfghjklmnoprtvy' */
        if ( (towlower(pwcTerm[uiTermLength - 1]) == 's') && 
                (s_wcschr(L"bcdfghjklmnoprtvy", towlower(pwcTerm[uiTermLength - 2])) != NULL) ) {
    
            /* Remove final 's' */
            pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_no()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_no
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 4 characters long */
    if ( uiTermLength > 4 ) {
        
        /* Convert -'s' to -'\0' if the preceeding character one of 'bcdfghjklmnoprtvyz' */
        if ( (towlower(pwcTerm[uiTermLength - 1]) == 's') && 
                (s_wcschr(L"bcdfghjklmnoprtvyz", towlower(pwcTerm[uiTermLength - 2])) != NULL) ) {

            /* Remove final 's' */
            pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_da()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_da
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 4 characters long */
    if ( uiTermLength > 4 ) {
        
        /* Convert -'s' to -'\0' if the preceeding character one of 'abcdfghjklmnoprtvyzå' */
        if ( (towlower(pwcTerm[uiTermLength - 1]) == 's') && 
                ((s_wcschr(L"abcdfghjklmnoprtvyz", towlower(pwcTerm[uiTermLength - 2])) != NULL) || (towlower(pwcTerm[uiTermLength - 2]) == 0xE5)) ) {

            /* Remove final 's' */
            pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_bg()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_bg
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Possible plural term if it is more than 4 characters long */
    if ( uiTermLength > 4 ) {
        
        /* Convert -'ове' to -'\0' */
        if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0435) && (towlower(pwcTerm[uiTermLength - 2]) == 0x0432) && (towlower(pwcTerm[uiTermLength - 3]) == 0x043E) ) {
            pwcTerm[uiTermLength - 3] = L'\0';
        }

        /* Convert -'еве' to -'й\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0435) && (towlower(pwcTerm[uiTermLength - 2]) == 0x0432) && (towlower(pwcTerm[uiTermLength - 3]) == 0x0435) ) {
            pwcTerm[uiTermLength - 3] = 0x0439;
            pwcTerm[uiTermLength - 2] = L'\0';
        }

        /* Convert -'ища' to -'\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0430) && (towlower(pwcTerm[uiTermLength - 2]) == 0x0449) && (towlower(pwcTerm[uiTermLength - 3]) == 0x0438) ) {
            pwcTerm[uiTermLength - 3] = L'\0';
        }

        /* Convert -'овци' to -'о\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0438) && (towlower(pwcTerm[uiTermLength - 2]) == 0x0446) && (towlower(pwcTerm[uiTermLength - 3]) == 0x0432) 
                 && (towlower(pwcTerm[uiTermLength - 4]) == 0x043E)) {
            pwcTerm[uiTermLength - 3] = L'\0';
        }

        /* Convert -'ци' to -'к\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0438) && (towlower(pwcTerm[uiTermLength - 2]) == 0x0446) ) {
            pwcTerm[uiTermLength - 2] = 0x043A;
            pwcTerm[uiTermLength - 1] = L'\0';
        }

        /* Convert -'зи' to -'г\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0438) && (towlower(pwcTerm[uiTermLength - 2]) == 0x0437) ) {
            pwcTerm[uiTermLength - 2] = 0x0433;
            pwcTerm[uiTermLength - 1] = L'\0';
        }

        /* Convert -'е..и' to -'я..\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0438) && (towlower(pwcTerm[uiTermLength - 4]) == 0x0435) ) {
            pwcTerm[uiTermLength - 4] = 0x044F;
            pwcTerm[uiTermLength - 1] = L'\0';
        }

        /* Convert -'та' to -'\0' */
        else if ( (towlower(pwcTerm[uiTermLength - 1]) == 0x0430) && (towlower(pwcTerm[uiTermLength - 2]) == 0x0442) ) {
            pwcTerm[uiTermLength - 2] = L'\0';
        }

        /* Convert -'и' to -'\0' */
        else if ( towlower(pwcTerm[uiTermLength - 1]) == 0x0438 ) {
            pwcTerm[uiTermLength - 1] = L'\0';
        }
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPlural_un()

    Purpose:    Stems a term down to its singular

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    A pointer to the stemmed term

*/
static int iLngStemmerPlural_un
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* Plural term if it is more than 2 characters long, it ends in 's', 
    ** and if the penultimate character is not punctuation like "Alyce's"
    */
    if ( (uiTermLength > 2) && (towlower(pwcTerm[uiTermLength - 1]) == 's') && (iswpunct(pwcTerm[uiTermLength - 2]) == 0) ) {

        /* Remove final 's' */
        pwcTerm[uiTermLength - 1] = L'\0';
    }


    return (LNG_NoError);         

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngStemmerPluralIsVowel()

    Purpose:    Returns true if the character is a vowel

    Parameters: wcChar      Character to test

    Globals:    none

    Returns:    true is the character is a vowel

*/
static boolean bLngStemmerPluralIsVowel
(
    wchar_t wcChar
)
{

    wcChar = wcLngCaseStripAccentFromWideCharacter(towlower(wcChar));

    return (((wcChar == L'a') || (wcChar == L'e') || (wcChar == L'i') || (wcChar == L'o') || (wcChar == L'u') /* || (wcChar == L'y') */) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*
** Porter Stemmer
*/


/*
** Structures
*/
struct ruleList {
    unsigned int    uiRuleID;
    wchar_t         *pwcOldEnd;
    wchar_t         *pwcNewEnd;
    int             iOldIndex;
    int             iNewIndex;
    int             iMinRootSize;
    boolean         (*bConditionFunction) ();
};



/*
** Private function prototypes
*/
static int iTermSize (wchar_t *pwcTerm);
static boolean bContainsVowel (wchar_t *pwcTerm);
static boolean bEndsWithCVC (wchar_t *pwcTerm, wchar_t **ppwcEnd);
static boolean bAddAnE (wchar_t *pwcTerm, wchar_t **ppwcEnd);
static boolean bRemoveAnE (wchar_t *pwcTerm, wchar_t **ppwcEnd);
static int iReplaceEnd (wchar_t *pwcTerm, wchar_t **ppwcEnd, struct ruleList *prlRule);



/*
** Defines
*/
#define IS_VOWEL(c) ( L'a' == (c) || L'e' == (c) || L'i' == (c) || L'o' == (c) || L'u' == (c) || \
        L'A' == (c) || L'E' == (c) || L'I' == (c) || L'O' == (c) || L'U' == (c) ) 
/* #define IS_VOWEL(c) (s_wcschr(L"aeiouAEIOU",(c)) != NULL)  */

#define LAMBDA L""




/*
** - Rule structure declaration
*/
static struct ruleList rlStep1aRules[] = 
{
    {101,   L"sses",        L"ss",      3,     1,    -1,    NULL},
    {102,   L"ies",         L"i",       2,     0,    -1,    NULL},
    {103,   L"ss",          L"ss",      1,     1,    -1,    NULL},
    {104,   L"s",           LAMBDA,     0,    -1,    -1,    NULL},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};



static struct ruleList rlStep1bRules[] = 
{
    {105,   L"eed",         L"ee",      2,     1,     0,    NULL},
    {106,   L"ed",          LAMBDA,     1,    -1,    -1,    bContainsVowel},
    {107,   L"ing",         LAMBDA,     2,    -1,    -1,    bContainsVowel},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};  



static struct ruleList rlStep1b1Rules[] = 
{
    {108,   L"at",          L"ate",     1,     2,    -1,    NULL},
    {109,   L"bl",          L"ble",     1,     2,    -1,    NULL},
    {110,   L"iz",          L"ize",     1,     2,    -1,    NULL},
    {111,   L"is",          L"ise",     1,     2,    -1,    NULL},
    {112,   L"bb",          L"b",       1,     0,    -1,    NULL},
    {113,   L"dd",          L"d",       1,     0,    -1,    NULL},
    {114,   L"ff",          L"f",       1,     0,    -1,    NULL},
    {115,   L"gg",          L"g",       1,     0,    -1,    NULL},
    {116,   L"mm",          L"m",       1,     0,    -1,    NULL},
    {117,   L"nn",          L"n",       1,     0,    -1,    NULL},
    {118,   L"pp",          L"p",       1,     0,    -1,    NULL},
    {119,   L"rr",          L"r",       1,     0,    -1,    NULL},
    {120,   L"tt",          L"t",       1,     0,    -1,    NULL},
    {121,   L"ww",          L"w",       1,     0,    -1,    NULL},
    {122,   L"xx",          L"x",       1,     0,    -1,    NULL},
    {123,   LAMBDA,         L"e",      -1,     0,    -1,    bAddAnE},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};


static struct ruleList rlStep1cRules[] = 
{
    {124,   L"y",           L"i",       0,     0,    -1,    bContainsVowel},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};


static struct ruleList rlStep2Rules[] = 
{
    {203,   L"ational",     L"ate",     6,     2,     0,    NULL},
    {204,   L"tional",      L"tion",    5,     3,     0,    NULL},
    {205,   L"enci",        L"ence",    3,     3,     0,    NULL},
    {206,   L"anci",        L"ance",    3,     3,     0,    NULL},
    {207,   L"izer",        L"ize",     3,     2,     0,    NULL},
    {208,   L"iser",        L"ise",     3,     2,     0,    NULL},
    {209,   L"abli",        L"able",    3,     3,     0,    NULL},
    {210,   L"alli",        L"al",      3,     1,     0,    NULL},
    {211,   L"entli",       L"ent",     4,     2,     0,    NULL},
    {212,   L"eli",         L"e",       2,     0,     0,    NULL},
    {213,   L"ousli",       L"ous",     4,     2,     0,    NULL},
    {214,   L"ization",     L"ize",     6,     2,     0,    NULL},
    {215,   L"isation",     L"ise",     6,     2,     0,    NULL},
    {216,   L"ation",       L"ate",     4,     2,     0,    NULL},
    {217,   L"ator",        L"ate",     3,     2,     0,    NULL},
    {218,   L"alism",       L"al",      4,     1,     0,    NULL},
    {219,   L"iveness",     L"ive",     6,     2,     0,    NULL},
    {220,   L"fulnes",      L"ful",     5,     2,     0,    NULL},
    {221,   L"ousness",     L"ous",     6,     2,     0,    NULL},
    {222,   L"aliti",       L"al",      4,     1,     0,    NULL},
    {223,   L"iviti",       L"ive",     4,     2,     0,    NULL},
    {224,   L"biliti",      L"ble",     5,     2,     0,    NULL},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};



static struct ruleList rlStep3Rules[] = 
{
    {301,   L"icate",       L"ic",      4,     1,     0,    NULL},
    {302,   L"ative",       LAMBDA,     4,    -1,     0,    NULL},
    {303,   L"alize",       L"al",      4,     1,     0,    NULL},
    {304,   L"alise",       L"al",      4,     1,     0,    NULL},
    {305,   L"iciti",       L"ic",      4,     1,     0,    NULL},
    {306,   L"ical",        L"ic",      4,     1,     0,    NULL},
    {308,   L"ful",         LAMBDA,     3,    -1,     0,    NULL},
    {309,   L"ness",        LAMBDA,     4,    -1,     0,    NULL},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};




static struct ruleList rlStep4Rules[] = 
{
    {401,   L"al",          LAMBDA,     1,    -1,     1,    NULL},
    {402,   L"ance",        LAMBDA,     3,    -1,     1,    NULL},
    {403,   L"ence",        LAMBDA,     3,    -1,     1,    NULL},
    {405,   L"er",          LAMBDA,     1,    -1,     1,    NULL},
    {406,   L"ic",          LAMBDA,     1,    -1,     1,    NULL},
    {407,   L"able",        LAMBDA,     3,    -1,     1,    NULL},
    {408,   L"ible",        LAMBDA,     3,    -1,     1,    NULL},
    {409,   L"ant",         LAMBDA,     2,    -1,     1,    NULL},
    {410,   L"ement",       LAMBDA,     4,    -1,     1,    NULL},
    {411,   L"ment",        LAMBDA,     3,    -1,     1,    NULL},
    {412,   L"ent",         LAMBDA,     2,    -1,     1,    NULL},
    {423,   L"sion",        L"s",       3,     0,     1,    NULL},
    {424,   L"tion",        L"t",       3,     0,     1,    NULL},
    {415,   L"ou",          LAMBDA,     1,    -1,     1,    NULL},
    {416,   L"ism",         LAMBDA,     2,    -1,     1,    NULL},
    {417,   L"ate",         LAMBDA,     2,    -1,     1,    NULL},
    {418,   L"iti",         LAMBDA,     2,    -1,     1,    NULL},
    {419,   L"ous",         LAMBDA,     2,    -1,     1,    NULL},
    {420,   L"ive",         LAMBDA,     2,    -1,     1,    NULL},
    {421,   L"ize",         LAMBDA,     2,    -1,     1,    NULL},
    {422,   L"ise",         LAMBDA,     2,    -1,     1,    NULL},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};



static struct ruleList rlStep5aRules[] = 
{
    {501,   L"e",           LAMBDA,     0,    -1,     0,    NULL},
    {502,   L"e",           LAMBDA,     0,    -1,    -1,    bRemoveAnE},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};


static struct ruleList rlStep5bRules[] = 
{
    {503,   L"ll",          L"l",       1,     0,     0,    NULL},
    {000,   NULL,           NULL,       0,     0,     0,    NULL}
};

/*---------------------------------------------------------------------------*/


/*

    Function:   iTermSize()

    Purpose:    Calculates the term size in adjusted syllables

    Parameters: pwcTerm     the term to check

    Globals:    none

    Returns:    The calculated size of the term

*/
static int iTermSize
(
    wchar_t *pwcTerm
)
{

    int     iResult = 0;
    int     iState = 0;


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);


    while ( *pwcTerm != L'\0' ) {
        switch ( iState ) {
            case 0:    iState = (IS_VOWEL (*pwcTerm)) ? 1 : 2;
                    break;
            case 1:    iState = (IS_VOWEL (*pwcTerm)) ? 1 : 2;
                    if ( iState == 2 ) {
                        iResult++;
                    }
                    break;
            case 2:    iState = (IS_VOWEL (*pwcTerm) || ((*pwcTerm == L'y') || (*pwcTerm == L'Y'))) ? 1 : 2;
                    break;
        }

        pwcTerm++;
    }

    return (iResult);
}


/*---------------------------------------------------------------------------*/


/*

    Function:   bContainsVowel()

    Purpose:    Checks to see if the term contains a vowel.

    Parameters: pwcTerm        the term to check

    Globals:    none

    Returns:    Returns true if the term contains a vowel, false otherwise

*/
static boolean bContainsVowel
(
    wchar_t *pwcTerm
)
{

    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);


    if ( *pwcTerm == L'\0' ) {
        return (false);
    }
    else {
        return (IS_VOWEL(*pwcTerm) || (s_wcspbrk(pwcTerm + 1, L"aeiouyAEIOUY") != NULL));
    }
}


/*---------------------------------------------------------------------------*/


/*

    Function:   bEndsWithCVC()

    Purpose:    Checks to see if the term ends with a consonant-vowel-consonant

    Parameters: pwcTerm     the term to check
                ppwcEnd     return pointer for the end

    Globals:    none

    Returns:    Returns true if the term ends with a consonant-vowel-consonant, 
                false otherwise

*/
static boolean bEndsWithCVC
(
    wchar_t *pwcTerm,
    wchar_t **ppwcEnd
)
{

    unsigned int    uiTermLength = 0;
    wchar_t         *pwcEnd = NULL;
    boolean         bStatus = false;


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(ppwcEnd != NULL);


    /* Check the ending */
    if ( (uiTermLength = s_wcslen(pwcTerm)) > 1 ) {
        pwcEnd = pwcTerm + uiTermLength - 1;
        bStatus = ((s_wcschr(L"aeiouwxyAEIOUWXY", *pwcEnd--) == NULL) && (s_wcschr(L"aeiouyAEIOUY", *pwcEnd--) != NULL) && (s_wcschr(L"aeiouAEIOU", *pwcEnd) == NULL));
        *ppwcEnd = pwcEnd;
    }

    return (bStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bAddAnE()

    Purpose:    Checks to see if the term meets conditions for adding an E

    Parameters: pwcTerm     the term to check
                ppwcEnd     return pointer for the end

    Globals:    none

    Returns:    Returns true if the term meets the conditions, false otherwise 

*/
static boolean bAddAnE
(
    wchar_t *pwcTerm,
    wchar_t **ppwcEnd
)
{

    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(ppwcEnd != NULL);


    return ((iTermSize(pwcTerm) == 1) && (bEndsWithCVC(pwcTerm, ppwcEnd) == true));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bRemoveAnE()

    Purpose:    Checks to see if the term meets conditions for removing an E

    Parameters: pwcTerm     the term to check
                ppwcEnd     return pointer for the end

    Globals:    none

    Returns:    Returns true if the term meets the conditions, false otherwise 

*/
static boolean bRemoveAnE
(
    wchar_t *pwcTerm,
    wchar_t **ppwcEnd
)
{

    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(ppwcEnd != NULL);


    return ((iTermSize(pwcTerm) == 1) && (bEndsWithCVC(pwcTerm, ppwcEnd) == false));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iReplaceEnd()

    Purpose:    Apply a set of rules to replace the suffix of a term

    Parameters: pwcTerm     the term to check
                ppwcEnd     return pointer for the end
                prlRule     rules

    Globals:    none

    Returns:    The ID of the rule which fired 

*/
static int iReplaceEnd
(
    wchar_t *pwcTerm,
    wchar_t **ppwcEnd,
    struct ruleList *prlRule
)
{

    wchar_t     *pwcEnding = NULL;
    wchar_t     wcChar = L'\0';


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(ppwcEnd != NULL);
    ASSERT(prlRule != NULL);


    /* Check all rules */
    while (prlRule->uiRuleID != 0) {

        pwcEnding = *ppwcEnd - prlRule->iOldIndex;

        /* Check that we dont undershoot */
        if ( pwcTerm <= pwcEnding ) {
            
            if ( s_wcscasecmp(pwcEnding, prlRule->pwcOldEnd) == 0 ) {
                
                wcChar = *pwcEnding;
                *pwcEnding = L'\0';

                if ( prlRule->iMinRootSize < iTermSize(pwcTerm) ) {
                    
                    if ( !prlRule->bConditionFunction || (*prlRule->bConditionFunction)(pwcTerm, ppwcEnd) ) {
                        
                        if ( bLngCaseIsWideStringAllUpperCase(pwcTerm) == true ) {
                            s_wcscat(pwcTerm, prlRule->pwcNewEnd);
                            pwcLngCaseConvertWideStringToUpperCase(pwcTerm);
                        }
                        else {
                            s_wcscat(pwcTerm, prlRule->pwcNewEnd);
                        }
                        
                        *ppwcEnd = pwcEnding + prlRule->iNewIndex;
                        break;
                    }
                }

                *pwcEnding = wcChar;
            }
        }

        prlRule++;
    }

    return (prlRule->uiRuleID);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerPorter_en()

    Purpose:    Stems a term using the porter algorithm, both US and UK english
                are supported.

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length

    Globals:    none

    Returns:    LNG error code

*/
int iLngStemmerPorter_en
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{
    unsigned int    uiRuleID = 0;
    wchar_t         *pwcEnd = NULL;


    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    for ( pwcEnd = pwcTerm; *pwcEnd != L'\0'; pwcEnd++ ) {
        if ( iswalpha(*pwcEnd) == 0 ) {
            return (LNG_NoError);
        }
    }
    pwcEnd--;


    iReplaceEnd(pwcTerm, &pwcEnd, rlStep1aRules);

    uiRuleID = iReplaceEnd(pwcTerm, &pwcEnd, rlStep1bRules);

    if ( (uiRuleID == 106) || (uiRuleID == 107) ) {
        iReplaceEnd(pwcTerm, &pwcEnd, rlStep1b1Rules);
    }

    iReplaceEnd(pwcTerm, &pwcEnd, rlStep1cRules);

    iReplaceEnd(pwcTerm, &pwcEnd, rlStep2Rules);


    iReplaceEnd(pwcTerm, &pwcEnd, rlStep3Rules);


    iReplaceEnd(pwcTerm, &pwcEnd, rlStep4Rules);


    iReplaceEnd(pwcTerm, &pwcEnd, rlStep5aRules);


    iReplaceEnd(pwcTerm, &pwcEnd, rlStep5bRules);

    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*
** Lovins Stemmer
*/


/*
** Defines
*/

#define MIN_STEM_LENGTH     (2)
#define MAX_ENDING_SIZE     (11)
#define PREDEFINED_SIZE     (MAX_ENDING_SIZE + MIN_STEM_LENGTH)



/*
** Private function prototypes
*/
static wchar_t *pwcRemoveEnding (wchar_t *pwcTerm, unsigned int uiTermLength);
static void vRecodeStem (wchar_t *pwcStemEnd);


static int aio (wchar_t wcChar);
static int s (wchar_t wcChar);
static int pt (wchar_t wcChar);
static int m (wchar_t wcChar);
static int n (wchar_t wcChar);


static int B (int iStemLength, wchar_t *pwcEnd);
static int C (int iStemLength, wchar_t *pwcEnd);
static int D (int iStemLength, wchar_t *pwcEnd);
static int E (int iStemLength, wchar_t *pwcEnd);
static int F (int iStemLength, wchar_t *pwcEnd);
static int G (int iStemLength, wchar_t *pwcEnd);
static int H (int iStemLength, wchar_t *pwcEnd);
static int I (int iStemLength, wchar_t *pwcEnd);
static int J (int iStemLength, wchar_t *pwcEnd);
static int K (int iStemLength, wchar_t *pwcEnd);
static int L (int iStemLength, wchar_t *pwcEnd);
static int M (int iStemLength, wchar_t *pwcEnd);
static int N (int iStemLength, wchar_t *pwcEnd);
static int O (int iStemLength, wchar_t *pwcEnd);
static int P (int iStemLength, wchar_t *pwcEnd);
static int Q (int iStemLength, wchar_t *pwcEnd);
static int R (int iStemLength, wchar_t *pwcEnd);
static int S (int iStemLength, wchar_t *pwcEnd);
static int T (int iStemLength, wchar_t *pwcEnd);
static int U (int iStemLength, wchar_t *pwcEnd);
static int V (int iStemLength, wchar_t *pwcEnd);
static int W (int iStemLength, wchar_t *pwcEnd);
static int X (int iStemLength, wchar_t *pwcEnd);
static int Y (int iStemLength, wchar_t *pwcEnd);
static int Z (int iStemLength, wchar_t *pwcEnd);
static int AA (int iStemLength, wchar_t *pwcEnd);
static int BB (int iStemLength, wchar_t *pwcEnd);
static int CC (int iStemLength, wchar_t *pwcEnd);




/*---------------------------------------------------------------------------*/


/*
** Structures
*/



/* Data structure for recoding and the method for recoding the stem */
struct RecodeRules {
    wchar_t     *pwcOldEnd;             /* Old ending */
    wchar_t     *pwcNewEnd;             /* New ending */
    int         iOldIndex;              /* Length of the old ending - 1 */
    int         (*cond)(wchar_t);       /* Condition rule */
    int         iEndGroup;              /* Signal the end of the group */
};


/* Stem terminations are divided into groups having the same last character */
static struct RecodeRules Rules[] =
{
    {L"uad",        L"uas",     2,      NULL,   0},
    {L"vad",        L"vas",     2,      NULL,   0},
    {L"cid",        L"cis",     2,      NULL,   0},
    {L"lid",        L"lis",     2,      NULL,   0},
    {L"erid",       L"eris",    3,      NULL,   0},
    {L"pand",       L"pans",    3,      NULL,   0},
    {L"end",        L"ens",     2,      s,      0},
    {L"end",        L"ens",     2,      m,      0},
    {L"ond",        L"ons",     2,      NULL,   0},
    {L"lud",        L"lus",     2,      NULL,   0},
    {L"rud",        L"rus",     2,      NULL,   1},

    {L"ul",         L"l",       1,      aio,    1},

    {L"istr",       L"ister",   3,      NULL,   0},
    {L"metr",       L"meter",   3,      NULL,   0},
    {L"her",        L"hes",     2,      pt,     1},

    {L"urs",        L"ur",      2,      NULL,   1},

    {L"uct",        L"uc",      2,      NULL,   0},
    {L"umpt",       L"um",      3,      NULL,   0},
    {L"rpt",        L"rb",      2,      NULL,   0},
    {L"mit",        L"mis",     2,      NULL,   0},
    {L"ert",        L"ers",     2,      NULL,   0},
    {L"et",         L"es",      1,      n,      0},
    {L"yt",         L"ys",      1,      NULL,   1},

    {L"iev",        L"ief",     2,      NULL,   0},
    {L"olv",        L"olut",    2,      NULL,   1},

    {L"bex",        L"bic",     2,      NULL,   0},
    {L"dex",        L"dic",     2,      NULL,   0},
    {L"pex",        L"pic",     2,      NULL,   0},
    {L"tex",        L"tic",     2,      NULL,   0},
    {L"ax",         L"ac",      1,      NULL,   0},
    {L"ex",         L"ec",      1,      NULL,   0},
    {L"ix",         L"ic",      1,      NULL,   0},
    {L"lux",        L"luc",     2,      NULL,   1},

    {L"yz",         L"ys",      1,      NULL,   1},
};



struct LastCharNode {
    wchar_t                 wcLastChar;                 /* The last character */
    struct LastCharNode     *plcnLastCharNodeLeft;      /* Used in balanced */
    struct LastCharNode     *plcnLastCharNodeRight;     /* Binary tree */
    struct RecodeRules      *prrRecodeRule;             /* Location of approriate group */
};


/*
**       s
**     /   \
**    l     x
**   / \   / \
**  d   r t   z
**       \
**        v
*/

static struct LastCharNode pr[] = {
    {L'd',      NULL,       NULL,       Rules},
    {L'l',      pr,         pr + 2,     Rules + 11},
    {L'r',      NULL,       NULL,       Rules + 12},
    {L's',      pr + 1,     pr + 6,     Rules + 15},
    {L't',      NULL,       pr + 5,     Rules + 16},
    {L'v',      NULL,       NULL,       Rules + 23},
    {L'x',      pr + 4,     pr + 7,     Rules + 25},
    {L'z',      NULL,       NULL,       Rules + 33},
};



/* List of endings and the method to remove the ending */

/* Data structures for list of endings */
struct EndingList {
    wchar_t     *pwcEnding;                         /* Old ending */
    int         (*cond)(int, wchar_t *);            /* Conditional rule */
    int         iLeftIndex;                         /* Used to find the siblings */
    int         iRightIndex;                        /* In balanced binary tree */
};


/* The original list of endings is re-organised into groups having
** the same length and the same first character.
*/
static struct EndingList List[] = {
    {L"a",              NULL,       0,      0},

    {L"ae",             NULL,       0,        0},
    {L"al",             BB,        -1,        2},
    {L"ar",             X,          0,        0},
    {L"as",             B,         -1,        0},

    {L"acy",            NULL,       0,        1},
    {L"age",            B,          0,        0},
    {L"aic",            NULL,      -2,        1},
    {L"als",            BB,         0,        0},
    {L"ant",            B,         -2,        2},
    {L"ars",            O,          0,        0},
    {L"ary",            F,         -1,        2},
    {L"ata",            NULL,       0,        0},
    {L"ate",            NULL,      -1,        0},

    {L"able",           NULL,       0,        1},
    {L"ably",           NULL,       0,        0},
    {L"ages",           B,         -2,        2},
    {L"ally",           B,          0,        0},
    {L"ance",           B,         -1,        1},
    {L"ancy",           B,          0,        0},
    {L"ants",           B,         -4,        4},
    {L"aric",           NULL,       0,        0},
    {L"arly",           K,         -1,        1},
    {L"ated",           I,          0,        0},
    {L"ates",           NULL,      -2,        2},
    {L"atic",           B,          0,        0},
    {L"ator",           NULL,      -1,        0},

    {L"acies",          NULL,       0,        0},
    {L"acity",          NULL,      -1,        1},
    {L"aging",          B,          0,        0},
    {L"aical",          NULL,      -2,        2},
    {L"alist",          NULL,       0,        0},
    {L"alism",          B,         -1,        0},
    {L"ality",          NULL,      -3,        3},
    {L"alize",          NULL,       0,        1},
    {L"allic",          BB,         0,        0},
    {L"anced",          B,         -2,        2},
    {L"ances",          B,          0,        0},
    {L"antic",          C,         -1,        0},
    {L"arial",          NULL,      -6,        6},
    {L"aries",          NULL,       0,        1},
    {L"arily",          NULL,       0,        0},
    {L"arity",          B,         -2,        2},
    {L"arize",          NULL,       0,        0},
    {L"aroid",          NULL,      -1,        0},
    {L"ately",          NULL,      -3,        3},
    {L"ating",          I,          0,        1},
    {L"ation",          B,          0,        0},
    {L"ative",          NULL,      -2,        2},
    {L"ators",          NULL,       0,        0},
    {L"atory",          NULL,      -1,        1},
    {L"ature",          E,          0,        0},

    {L"aceous",         NULL,       0,        1},
    {L"acious",         B,          0,        0},
    {L"action",         G,         -2,        2},
    {L"alness",         NULL,       0,        0},
    {L"ancial",         NULL,      -1,        1},
    {L"ancies",         NULL,       0,        0},
    {L"ancing",         B,         -4,        4},
    {L"ariser",         NULL,       0,        0},
    {L"arized",         NULL,      -1,        1},
    {L"arizer",         NULL,       0,        0},
    {L"atable",         NULL,      -2,        2},
    {L"ations",         B,          0,        0},
    {L"atives",         NULL,      -1,        0},

    {L"ability",        NULL,       0,        1},
    {L"aically",        NULL,       0,        0},
    {L"alistic",        B,         -2,        2},
    {L"alities",        NULL,       0,        0},
    {L"ariness",        E,         -1,        0},
    {L"aristic",        NULL,      -3,        3},
    {L"arizing",        NULL,       0,        1},
    {L"ateness",        NULL,       0,        0},
    {L"atingly",        NULL,      -2,        2},
    {L"ational",        B,          0,        0},
    {L"atively",        NULL,      -1,        1},
    {L"ativism",        NULL,       0,        0},

    {L"ableness",       NULL,       0,        1},
    {L"arizable",       NULL,       0,        0},

    {L"allically",      C,          0,        0},
    {L"antaneous",      NULL,      -1,        1},
    {L"antiality",      NULL,       0,        0},
    {L"arisation",      NULL,      -2,        2},
    {L"arization",      NULL,       0,        0},
    {L"ationally",      B,         -1,        1},
    {L"ativeness",      NULL,       0,        0},

    {L"antialness",     NULL,       0,        0},
    {L"arisations",     NULL,      -1,        1},
    {L"arizations",     NULL,       0,        0},

    {L"alistically",    B,          0,        1},
    {L"arizability",    NULL,       0,        0},

    {L"e",              NULL,       0,        0},

    {L"ed",             E,          0,        0},
    {L"en",             F,         -1,        1},
    {L"es",             E,          0,        0},

    {L"eal",            Y,          0,        0},
    {L"ear",            Y,         -1,        1},
    {L"ely",            E,          0,        0},
    {L"ene",            E,         -2,        2},
    {L"ent",            C,          0,        0},
    {L"ery",            E,         -1,        1},
    {L"ese",            NULL,       0,        0},

    {L"ealy",           Y,          0,        1},
    {L"edly",           E,          0,        0},
    {L"eful",           NULL,      -2,        1},
    {L"eity",           NULL,       0,        0},
    {L"ence",           NULL,      -2,        2},
    {L"ency",           NULL,       0,        0},
    {L"ened",           E,         -1,        2},
    {L"enly",           E,          0,        0},
    {L"eous",           NULL,      -1,        0},

    {L"early",          Y,          0,        1},
    {L"ehood",          NULL,       0,        0},
    {L"eless",          NULL,      -2,        2},
    {L"elity",          NULL,       0,        0},
    {L"ement",          NULL,      -1,        0},
    {L"enced",          NULL,      -3,        3},
    {L"ences",          NULL,       0,        1},
    {L"eness",          E,          0,        0},
    {L"ening",          E,         -2,        2},
    {L"ental",          NULL,       0,        0},
    {L"ented",          C,         -1,        1},
    {L"ently",          NULL,       0,        0},

    {L"eature",         Z,          0,        0},
    {L"efully",         NULL,      -1,        1},
    {L"encies",         NULL,       0,        0},
    {L"encing",         NULL,      -2,        2},
    {L"ential",         NULL,       0,        0},
    {L"enting",         C,         -1,        1},
    {L"entist",         NULL,       0,        1},
    {L"eously",         NULL,       0,        0},

    {L"elihood",        E,          0,        1},
    {L"encible",        NULL,       0,        0},
    {L"entally",        NULL,      -2,        2},
    {L"entials",        NULL,       0,        0},
    {L"entiate",        NULL,      -1,        1},
    {L"entness",        NULL,       0,        0},

    {L"entation",       NULL,       0,        0},
    {L"entially",       NULL,      -1,        1},
    {L"eousness",       NULL,       0,        0},

    {L"eableness",      E,          0,        1},
    {L"entations",      NULL,       0,        0},
    {L"entiality",      NULL,      -2,        2},
    {L"entialize",      NULL,       0,        0},
    {L"entiation",      NULL,      -1,        0},

    {L"entialness",     NULL,       0,        0},

    {L"ful",            NULL,       0,        0},

    {L"fully",          NULL,       0,        0},

    {L"fulness",        NULL,       0,        0},

    {L"hood",           NULL,       0,        0},

    {L"i",              NULL,       0,        0},

    {L"ia",             NULL,       0,        0},
    {L"ic",             NULL,      -1,        1},
    {L"is",             NULL,       0,        0},

    {L"ial",            NULL,       0,        0},
    {L"ian",            NULL,      -1,        1},
    {L"ics",            NULL,       0,        1},
    {L"ide",            L,          0,        0},
    {L"ied",            NULL,      -3,        3},
    {L"ier",            NULL,       0,        0},
    {L"ies",            P,         -1,        0},
    {L"ily",            NULL,      -1,        1},
    {L"ine",            M,          0,        0},
    {L"ing",            N,         -5,        5},
    {L"ion",            Q,          0,        0},
    {L"ish",            C,         -1,        1},
    {L"ism",            B,          0,        1},
    {L"ist",            NULL,       0,        0},
    {L"ite",            AA,        -3,        3},
    {L"ity",            NULL,       0,        0},
    {L"ium",            NULL,      -1,        0},
    {L"ive",            NULL,      -1,        1},
    {L"ize",            F,          0,        0},

    {L"ials",           NULL,       0,        0},
    {L"ians",           NULL,      -1,        0},
    {L"ible",           NULL,      -1,        1},
    {L"ibly",           NULL,       0,        0},
    {L"ical",           NULL,      -2,        2},
    {L"ides",           L,          0,        0},
    {L"iers",           NULL,      -1,        1},
    {L"iful",           NULL,       0,        0},
    {L"ines",           M,         -4,        4},
    {L"ings",           N,          0,        0},
    {L"ions",           B,         -1,        1},
    {L"ious",           NULL,       0,        0},
    {L"isms",           B,         -2,        2},
    {L"ists",           NULL,       0,        0},
    {L"itic",           H,         -1,        1},
    {L"ized",           F,          0,        1},
    {L"izer",           F,          0,        0},

    {L"ially",          NULL,       0,        0},
    {L"icant",          NULL,      -1,        1},
    {L"ician",          NULL,       0,        0},
    {L"icide",          NULL,      -2,        2},
    {L"icism",          NULL,       0,        0},
    {L"icist",          NULL,      -1,        0},
    {L"icity",          NULL,      -3,        3},
    {L"idine",          I,          0,        1},
    {L"iedly",          NULL,       0,        0},
    {L"ihood",          NULL,      -2,        2},
    {L"inate",          NULL,       0,        0},
    {L"iness",          NULL,      -1,        0},
    {L"ingly",          B,         -6,        6},
    {L"inism",          J,          0,        1},
    {L"inity",          CC,         0,        0},
    {L"ional",          NULL,      -2,        2},
    {L"ioned",          NULL,       0,        0},
    {L"ished",          NULL,      -1,        0},
    {L"istic",          NULL,      -3,        3},
    {L"ities",          NULL,       0,        1},
    {L"itous",          NULL,       0,        0},
    {L"ively",          NULL,      -2,        2},
    {L"ivity",          NULL,       0,        0},
    {L"izers",          F,         -1,        1},
    {L"izing",          F,          0,        0},

    {L"ialist",         NULL,       0,        0},
    {L"iality",         NULL,      -1,        1},
    {L"ialize",         NULL,       0,        0},
    {L"ically",         NULL,      -2,        2},
    {L"icance",         NULL,       0,        0},
    {L"icians",         NULL,      -1,        1},
    {L"icists",         NULL,       0,        0},
    {L"ifully",         NULL,      -4,        4},
    {L"ionals",         NULL,       0,        0},
    {L"ionate",         D,         -1,        1},
    {L"ioning",         NULL,       0,        0},
    {L"ionist",         NULL,      -2,        2},
    {L"iously",         NULL,       0,        0},
    {L"istics",         NULL,      -1,        1},
    {L"izable",         E,          0,        0},

    {L"ibility",        NULL,       0,        0},
    {L"icalism",        NULL,      -1,        1},
    {L"icalist",        NULL,       0,        1},
    {L"icality",        NULL,       0,        0},
    {L"icalize",        NULL,      -3,        3},
    {L"ication",        G,          0,        0},
    {L"icianry",        NULL,      -1,        0},
    {L"ination",        NULL,      -1,        1},
    {L"ingness",        NULL,       0,        0},
    {L"ionally",        NULL,      -5,        5},
    {L"isation",        NULL,       0,        0},
    {L"ishness",        NULL,      -1,        1},
    {L"istical",        NULL,       0,        1},
    {L"iteness",        NULL,       0,        0},
    {L"iveness",        NULL,      -3,        3},
    {L"ivistic",        NULL,       0,        0},
    {L"ivities",        NULL,      -1,        0},
    {L"ization",        F,         -1,        1},
    {L"izement",        NULL,       0,        0},

    {L"ibleness",       NULL,       0,        0},
    {L"icalness",       NULL,      -1,        1},
    {L"ionalism",       NULL,       0,        0},
    {L"ionality",       NULL,      -2,        2},
    {L"ionalize",       NULL,       0,        0},
    {L"iousness",       NULL,      -1,        1},
    {L"izations",       NULL,       0,        0},

    {L"ionalness",      NULL,       0,        1},
    {L"istically",      NULL,       0,        0},
    {L"itousness",      NULL,      -2,        2},
    {L"izability",      NULL,       0,        0},
    {L"izational",      NULL,      -1,        0},

    {L"izationally",    B,          0,        0},

    {L"ly",             B,          0,        0},

    {L"less",           NULL,       0,        1},
    {L"lily",           NULL,       0,        0},

    {L"lessly",         NULL,       0,        0},

    {L"lessness",       NULL,       0,        0},

    {L"ness",           NULL,       0,        0},

    {L"nesses",         NULL,       0,        0},

    {L"o",              NULL,       0,        0},

    {L"on",             S,          0,        1},
    {L"or",             T,          0,        0},

    {L"oid",            NULL,       0,        0},
    {L"one",            R,         -1,        1},
    {L"ous",            NULL,       0,        0},

    {L"ogen",           NULL,       0,        0},

    {L"oidal",          NULL,       0,        0},
    {L"oides",          NULL,      -1,        2},
    {L"otide",          NULL,       0,        0},
    {L"ously",          NULL,      -1,        0},

    {L"oidism",         NULL,       0,        0},

    {L"oidally",        NULL,       0,        1},
    {L"ousness",        NULL,       0,        0},

    {L"s",              W,          0,        0},

    {L"s'",             NULL,       0,        0},

    {L"um",             U,          0,        1},
    {L"us",             V,          0,        0},

    {L"ward",           NULL,       0,        1},
    {L"wise",           NULL,       0,        0},

    {L"y",              B,          0,        0},

    {L"yl",             R,          0,        0},

    {L"ying",           B,          0,        1},
    {L"yish",           NULL,       0,        0},

    {L"'s",             NULL,       0,        0},
};





struct FirstCharNode {
    wchar_t                 wcFirstChar;                /* First character    */
    struct FirstCharNode    *pfcnFirstCharNodeLeft;     /* Used in balanced */
    struct FirstCharNode    *pfcnFirstCharNodeRight;    /* Binary tree */
    struct EndingList       *ptr[11];                   /* The approriate location */
};


static struct FirstCharNode First[] =
{
    {L'\'', NULL, NULL,
    {NULL,
    List + 293, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL}},

    {L'a', First, NULL,
    {List,
    List + 2, List + 9, List + 20, List + 39, List + 58,
    List + 70, List + 77, List + 82, List + 87, List + 89}},

    {L'e', First + 1, First + 4,
    {List + 91,
    List + 93, List + 98, List + 106, List + 116, List + 126,
    List + 133, List + 138, List + 142, List + 145, NULL}},

    {L'f', NULL, NULL,
    {NULL,
    NULL, List + 146, NULL, List + 147, NULL,
    List + 148, NULL, NULL, NULL, NULL}},

    {L'h', First + 3, First + 5,
    {NULL,
    NULL, NULL, List + 149, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL}},

    {L'i', NULL, NULL,
    {List + 150,
    List + 152, List + 163, List + 181, List + 202, List + 222,
    List + 239, List + 252, List + 258, NULL, List + 261}},

    {L'l', First + 2, First + 10,
    {NULL,
    List + 262, NULL, List + 263, NULL, List + 265,
    NULL, List + 266, NULL, NULL, NULL}},

    {L'n', NULL, NULL,
    {NULL,
    NULL, NULL, List + 267, NULL, List + 268,
    NULL, NULL, NULL, NULL, NULL}},

    {L'o', First + 7, First + 9,
    {List + 269,
    List + 270, List + 273, List + 275, List + 277, List + 280,
    List + 281, NULL, NULL, NULL, NULL}},

    {L's', NULL, NULL,
    {List + 283,
    List + 284, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL}},

    {L'u', First + 8, First + 12,
    {NULL,
    List + 285, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL}},

    {L'w', NULL, NULL,
    {NULL,
    NULL, NULL, List + 287, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL}},

    {L'y', First + 11, NULL,
    {List + 289,
    List + 290, NULL, List + 291, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL}},
};





/*---------------------------------------------------------------------------*/

/*

    Function:   ()

    Purpose:    Transformational Rules used in Recoding Stem Terminations

    Parameters: wcChar      the second last character of the old ending
                            (after removing one of double consonants) 

    Globals:    none

    Returns:    true or false

*/

/* Rule number 9 */
static int aio
(
    wchar_t wcChar
)
{
    return ((wcChar != L'a') && (wcChar != L'A') && (wcChar != L'i') && (wcChar != L'I') && (wcChar != L'o') && (wcChar != L'O'));
}

/* Rule Number 24 */
static int s
(
    wchar_t wcChar
)
{
    return ((wcChar != L's') && (wcChar != L'S'));
}

/* Rule number 28 */
static int pt
(
    wchar_t wcChar
)
{
    return ((wcChar != L'p') && (wcChar != L'P') && (wcChar != L't') && (wcChar != L'T'));
}

/* Rule number 30 */
static int m
(
    wchar_t wcChar
)
{
    return ((wcChar != L'm') && (wcChar != L'M'));
}

/* Rule number 32 */
static int n
(
    wchar_t wcChar
)
{
    return ((wcChar != L'n') && (wcChar != L'N'));
}


/*---------------------------------------------------------------------------*/


/*

    Function:   ()

    Purpose:    Context-sensitive rules associated with certain endings.
                The notations of the rules are the same as in Lovins' paper
                except that rule A is replaced by a NULL in the data structure.

    Parameters: iStemLength     possible stem length
                pwcEnd      pointer points to the end of the possible stem

    Globals:    none

    Returns:    true or false

*/

static int B
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return (iStemLength >= 3);
}

static int C
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return (iStemLength >= 4);
}

static int D
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return (iStemLength >= 5);
}

static int E
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return ((*pwcEnd != L'e') && (*pwcEnd != L'E'));
}

static int F
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return ((iStemLength >= 3) && (*pwcEnd != L'e') && (*pwcEnd != L'E'));
}

static int G
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
  return ((iStemLength >= 3) && (*pwcEnd != L'f') && (*pwcEnd != L'F'));
}

static int H
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar1 = L'\0';
    wchar_t wcChar2 = L'\0';

    wcChar1 = *pwcEnd;
    wcChar2 = *(pwcEnd - 1);

    return ((wcChar1 == L't') || (wcChar1 == L'T') || 
            (((wcChar1 == L'l') || (wcChar1 == L'L')) && ((wcChar2 == L'l') || (wcChar2 == L'L'))));
}

static int I
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((wcChar != L'o') && (wcChar != L'O') && (wcChar != L'e') && (wcChar != L'E'));
}

static int J
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((wcChar != L'a') && (wcChar != L'A') && (wcChar != L'e') && (wcChar != L'E'));
}

static int K
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((iStemLength >= 3) && ((wcChar == L'l') || (wcChar == L'L') || (wcChar == L'i') || (wcChar == L'I') || 
            (((wcChar == L'e') || (wcChar == L'E')) && ((*(pwcEnd - 2) == L'u') || (*(pwcEnd - 2) == L'U')))));
}

static int L
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar1 = L'\0';
    wchar_t wcChar2 = L'\0';

    wcChar1 = *pwcEnd;
    wcChar2 = *(pwcEnd - 1);

    if ( (wcChar1 == L's') || (wcChar1 == L'S') ) {
        return ((wcChar2 == L'o') || (wcChar2 == L'O'));
    }
    else {
        return ((wcChar1 != L'u') && (wcChar1 != L'U') && (wcChar1 != L'x') && (wcChar1 != L'X'));
    }
}

static int M
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((wcChar != L'a') && (wcChar != L'A') && (wcChar != L'c') && (wcChar != L'C') && 
            (wcChar != L'e') && (wcChar != L'E') && (wcChar != L'm') && (wcChar != L'M') );
}

static int N
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    if ( iStemLength >= 3 ) {
        if ( (*(pwcEnd - 2) == L's') || (*(pwcEnd - 2) == L'S') ) {
            return (iStemLength >= 4);
        }
        else {
            return (1);
        }
    }
    else {
        return (0);
    }
}

static int O
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((wcChar == L'l') || (wcChar == L'L') || (wcChar == L'i') || (wcChar == L'I'));
}

static int    P
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return ((*pwcEnd != L'c') && (*pwcEnd != L'C'));
}

static int Q
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((iStemLength >= 3) && (wcChar != L'l') && (wcChar != L'L') && (wcChar != L'n') && (wcChar != L'N'));
}

static int R
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((wcChar == L'n') || (wcChar == L'N') || (wcChar == L'r') || (wcChar == L'R'));
}

static int S
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar1 = L'\0';
    wchar_t wcChar2 = L'\0';

    wcChar1 = *pwcEnd;
    wcChar2 = *(pwcEnd - 1);

    if ( (wcChar1 == L't') || (wcChar1 == L'T') ) {
        return ((wcChar2 != L't') && (wcChar2 != L'T'));
    }
    else {
        return (((wcChar1 == L'r') || (wcChar1 == L'R')) && ((wcChar2 == L'd') || (wcChar2 == L'D')));
    }
}

static int T
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar1 = L'\0';
    wchar_t wcChar2 = L'\0';

    wcChar1 = *pwcEnd;
    wcChar2 = *(pwcEnd - 1);

    if ( (wcChar1 == L't') || (wcChar1 == L'T') ) {
        return ((wcChar2 != L'o') && (wcChar2 != L'O'));
    }
    else {
        return ((wcChar1 != L's') && (wcChar1 != L'S'));
    }
}

static int U
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((wcChar == L'l') || (wcChar == L'L') || (wcChar == L'm') || (wcChar == L'M') || 
            (wcChar == L'n') || (wcChar == L'N') || (wcChar == L'r') || (wcChar == L'R'));
}

static int V
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return ((*pwcEnd == L'c') || (*pwcEnd == L'C'));
}

static int W
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    return ((wcChar != L's') && (wcChar != L'S') && (wcChar != L'u') && (wcChar != L'U'));
}

static int X
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar = L'\0';

    wcChar = *pwcEnd;

    if ( (wcChar == L'e') || (wcChar == L'E') ) {
        return ((iStemLength >= 3) && ((*(pwcEnd - 2) == L'u') || (*(pwcEnd - 2) == L'U')));
    }
    else {
        return ((wcChar == L'l') || (wcChar == L'L') || (wcChar == L'i') || (wcChar == L'I'));
    }
}

static int Y
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return (((*pwcEnd == L'n') || (*pwcEnd == L'N')) && ((*(pwcEnd - 1) == L'i') || (*(pwcEnd - 1) == L'I')));
}

static int Z
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return ((*pwcEnd != L'f') || (*pwcEnd != L'F'));
}

static int AA
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    wchar_t wcChar1 = L'\0';
    wchar_t wcChar2 = L'\0';

    wcChar1 = *pwcEnd;
    wcChar2 = *(pwcEnd - 1);

    if ( (wcChar1 == L'h') || (wcChar1 == L'H') ) {
        return ((wcChar2 == L'p') || (wcChar2 == L'P') || (wcChar2 == L't') || (wcChar2 == L'T'));
    }
    else if ( (wcChar1 == L'r') || (wcChar1 == L'R') ) {
        return ((wcChar2 == L'e') || (wcChar2 == L'E') || (wcChar2 == L'o') || (wcChar2 == L'O'));
    }
    else if ( (wcChar1 == L's') || (wcChar1 == L'S') ) {
        return ((wcChar2 == L'e') || (wcChar2 == L'E'));
    }
    else {
        return ((wcChar1 == L'd') || (wcChar1 == L'D') || (wcChar1 == L'f') || (wcChar1 == L'F') || 
                (wcChar1 == L'l') || (wcChar1 == L'L') || (wcChar1 == 't') || (wcChar1 == L'T'));
    }
}

static int BB
(
    int iStemLength,
    wchar_t *pwcEnd
)
{

    if ( iStemLength >= 3 ) {
        if ( iStemLength >= 4 ) {
            return (s_wcsncasecmp(pwcEnd - 3, L"ryst", 4) != 0);
        }
        else {
            return (s_wcsncasecmp(pwcEnd - 2, L"met", 3) != 0);
        }
    }
    else {
        return (0);
    }
}

static int CC
(
    int iStemLength,
    wchar_t *pwcEnd
)
{
    return ((*pwcEnd == L'l') || (*pwcEnd == L'L'));
}


/*---------------------------------------------------------------------------*/


/*

    Function:   vRecodeStem()

    Purpose:    Recode the stem according to the transformation rules..

    Parameters: pwcStemEnd      stem end

    Globals:    none

    Returns:    void

*/
static void vRecodeStem
(
    wchar_t *pwcStemEnd
)
{

    static struct LastCharNode  *plchLastCharNodeRoot = pr + 3;
    struct LastCharNode         *plchLastCharNode = plchLastCharNodeRoot;
    struct RecodeRules          *prrRecodeRule = NULL;                      /* Points to the transformation list */
    wchar_t                     *pwcEnding = NULL;                          /* Points to start of possible ending */
    wchar_t                     wcChar1 = L'\0';                            /* Last character of the old ending     */
    wchar_t                     wcChar2 = L'\0';


    ASSERT(pwcStemEnd != NULL);


    pwcEnding = pwcStemEnd - 1;
    wcChar1 = *pwcStemEnd - ( *pwcStemEnd >= L'a' ) ? L'a' : L'A';


    /* Check for double consonant */
    if ( *pwcEnding == wcChar1 ) {
        if ( s_wcschr(L"bdglmnprstBDGLMNPRST", wcChar1) != NULL ) {
            *pwcStemEnd = L'\0';
            pwcStemEnd = pwcEnding;
        }
    }


    /* Check for the last character */
    do {
        wcChar2 = plchLastCharNode->wcLastChar;

        if ( wcChar1 == wcChar2 ) {
            break;
        }
        else if ( wcChar1 > wcChar2 ) {
            plchLastCharNode = plchLastCharNode->plcnLastCharNodeRight;
        }
        else {
            plchLastCharNode = plchLastCharNode->plcnLastCharNodeLeft;
        }

    } while ( plchLastCharNode != NULL );


    /* Check for the rest of suffix list */
    if ( plchLastCharNode != NULL ) {

        prrRecodeRule = plchLastCharNode->prrRecodeRule;

        for (;;) {

            pwcEnding = pwcStemEnd - prrRecodeRule->iOldIndex;

            if ( s_wcscmp(pwcEnding, prrRecodeRule->pwcOldEnd) == 0 ) {
                if ( !prrRecodeRule->cond || (*prrRecodeRule->cond) (*(pwcEnding - 1)) ) {
                    s_wcscpy(pwcEnding, prrRecodeRule->pwcNewEnd);
                    break;
                }
            }

            if ( prrRecodeRule->iEndGroup ) {
                break;
            }

            prrRecodeRule++;
        }
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pwcRemoveEnding()

    Purpose:    Look for a match in the suffix list.
                Return the pointer to the end of the new stem if there is a match.
                Otherwise, return the pointer to the end of the original term.

                Method:
                    * Search for the first character in the suffix list.
                    * If there is match, search for the rest of the suffix list.

    Parameters: pwcTerm         term
                uiTermLength    term length

    Globals:    none

    Returns:    pointer to the stem end

*/
static wchar_t *pwcRemoveEnding
(
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    static struct FirstCharNode     *pfcnFirstCharNodeRoot = First + 6;     /* The root of binary tree is 'l' */
    struct FirstCharNode            *pfcnFirstCharNode = NULL;              /* Points to the first character of the possible suffix */
    struct EndingList               *pelEndingList = NULL;                  /* Points to the List of Endings */
    wchar_t                         *pucSuffixStart = NULL;                 /* Points to start of possible suffix */
    wchar_t                         *pwcStemEnd = NULL;                     /* Points to the end of possible stem */
    wchar_t                         wcChar1 = L'\0';
    wchar_t                         wcChar2 = L'\0';
    int                             iStemLength = 0;                        /* Possible stem length*/
    int                             iIndex = 0;                             /* Index from the end to the start of possible suffix */
    boolean                         bNotDone = true;
    int                             iCompare = 0;


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    pwcStemEnd = pwcTerm + uiTermLength - 1;


    /* Find the start of suffix      */
    if ( uiTermLength > PREDEFINED_SIZE ) {
        pucSuffixStart = pwcTerm + uiTermLength - MAX_ENDING_SIZE;
    }
    else {
        pucSuffixStart = pwcTerm + MIN_STEM_LENGTH;
    }

    wcChar1 = *pucSuffixStart - ( *pucSuffixStart >= L'a' ) ? L'a' : L'A';


    /* Search for the first character */
    do {
        pfcnFirstCharNode = pfcnFirstCharNodeRoot;

        do {
            wcChar2 = pfcnFirstCharNode->wcFirstChar;

            if ( wcChar1 == wcChar2 ) {
                break;
            }
            else if ( wcChar1 > wcChar2 ) {
                pfcnFirstCharNode = pfcnFirstCharNode->pfcnFirstCharNodeRight;
            }
            else {
                pfcnFirstCharNode = pfcnFirstCharNode->pfcnFirstCharNodeLeft;
            }

        } while ( pfcnFirstCharNode != NULL );


        /* Search for the rest */
        if ( pfcnFirstCharNode != NULL ) {

            iIndex = pwcStemEnd - pucSuffixStart;

            if ( (pelEndingList = pfcnFirstCharNode->ptr[iIndex]) != NULL ) {

                /* No need to compare the first char  */
                for (;;) {
                    iCompare = s_wcscasecmp(pucSuffixStart + 1, pelEndingList->pwcEnding + 1);

                    if ( iCompare > 0 ) {
                        if ( pelEndingList->iRightIndex ) {
                            pelEndingList += pelEndingList->iRightIndex;
                        }
                        else {
                            break;
                        }
                    }
                    else if ( iCompare < 0 ) {
                        if ( pelEndingList->iLeftIndex ) {
                            pelEndingList += pelEndingList->iLeftIndex;
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        iStemLength = pucSuffixStart - pwcTerm;

                        if ( !pelEndingList->cond || (*pelEndingList->cond) (iStemLength, pucSuffixStart - 1) ) {
                            *pucSuffixStart = '\0';
                            pwcStemEnd = pucSuffixStart - 1;
                            bNotDone = false;
                        }
                        break;
                    }
                }
            }
        }

        pucSuffixStart++;

    } while ( bNotDone && ((wcChar1 = (*pucSuffixStart - ( *pucSuffixStart >= 'a' ) ? 'a' : 'A')) != '\0') );


    return (pwcStemEnd);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStemmerLovins_en()

    Purpose:    Implemetation of the Lovins' stemming algorithm described in
                J.B. Lovins, "Development of a Stemming Algorithm",
                Mechanical Translation and Computational Linguistics, Vol 11,1968.

    Parameters: plsLngStemmer       Language stemmer structure
                pwcTerm             The term to stem
                uiTermLength        The term length if known

    Globals:    none

    Returns:    LNG error code

*/
static int iLngStemmerLovins_en
(
    struct lngStemmer *plsLngStemmer,
    wchar_t *pwcTerm,
    unsigned int uiTermLength
)
{

    wchar_t     *pwcStemEnd = NULL;        /* Points to last character of stem portion */


    ASSERT(plsLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(uiTermLength > 0);


    /* The term must be at least MIN_STEM_LENGTH characters long. */
    if ( uiTermLength <= MIN_STEM_LENGTH ) {
        return(LNG_NoError);
    }


    /* Remove the ending */
    pwcStemEnd = pwcRemoveEnding(pwcTerm, uiTermLength);


    /* And recode it */
    vRecodeStem(pwcStemEnd);

    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


