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

    Module:     typo.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    13 September 1995

    Purpose:    This implements a typographical match. 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.typo"


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Language typo functions structure */
struct lngTypoFunction {
    unsigned int    uiTypoID;                       /* Typo ID */
    unsigned int    uiLanguageID;                   /* Language ID */
    int             (*iLngTypoFunction)();          /* Typo function pointer */
};


/* Language typo structure */
struct lngTypo {
    unsigned int    uiTypoID;                       /* Typo ID */
    unsigned int    uiLanguageID;                   /* Language ID */
    int             (*iLngTypoFunction)();          /* Typo function pointer (denomalization) */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLngTypoStandardMatch_un (struct lngTypo *pltLngTypo, wchar_t *pwcTerm,
        wchar_t *pwcCandidateTerm, int bCaseSensitive, unsigned int uiTypoMaxCount);


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Typo functions */
static struct lngTypoFunction pltLngTypoFunctionListGlobal[] = 
{
    /* Standard Typo */
    {   LNG_TYPO_STANDARD_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngTypoStandardMatch_un,
    },
    
    /* Terminator */
    {   0,
        0,
        NULL,
    },
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTypoCreateByName()

    Purpose:    This function returns a language typo structure for a particular typo name
                and language code combination.

    Parameters: pucTypoName         Typo name
                pucLanguageCode     Language code
                ppvLngTypo          Return pointer for the language typo structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngTypoCreateByName
(
    unsigned char *pucTypoName,
    unsigned char *pucLanguageCode,
    void **ppvLngTypo
)
{

    int             iError = LNG_NoError;
    unsigned int    uiTypoID = 0;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucTypoName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTypoName' parameter passed to 'iLngTypoCreateByName'."); 
        return (LNG_LanguageInvalidTypoName);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngTypoCreateByName'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngTypo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngTypo' parameter passed to 'iLngTypoCreateByName'."); 
        return (LNG_ReturnParameterError);
    }

    
    /* Get the typo ID */
    if ( (iError = iLngGetTypoIDFromName(pucTypoName, &uiTypoID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the typo ID from the typo name: '%s', lng error: %d.", pucTypoName, iError); 
        return (iError);
    }

    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngTypoCreateByID(uiTypoID, uiLanguageID, ppvLngTypo));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTypoCreateByID()

    Purpose:    This function returns a language typo structure for a particular typo ID 
                and language ID combination.

    Parameters: uiTypoID        Typo ID
                uiLanguageID    Language ID
                ppvLngTypo      Return pointer for the language typo structure

    Globals:    pltLngTypoFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngTypoCreateByID
(
    unsigned int uiTypoID,
    unsigned int uiLanguageID,
    void **ppvLngTypo
)
{

    int                         iError = LNG_NoError;
    struct lngTypoFunction      *ptfTypoFunctionPtr = pltLngTypoFunctionListGlobal;
    struct lngTypo              *pltLngTypo = NULL;


    /* Check the parameters */
    if ( uiTypoID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTypoID' parameter passed to 'iLngTypoCreateByName'."); 
        return (LNG_LanguageInvalidTypoID);
    }
    
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngTypoCreateByName'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( ppvLngTypo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngTypo' parameter passed to 'iLngTypoCreateByName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Canonicalize the language ID */
    if ( (iError = iLngGetCanonicalLanguageID(uiLanguageID, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiLanguageID, iError); 
        return (iError);
    }


    /* Loop over the typo function structures to see which one we want */
    for ( ptfTypoFunctionPtr = pltLngTypoFunctionListGlobal; ptfTypoFunctionPtr->iLngTypoFunction != NULL; ptfTypoFunctionPtr++ ) {

        /* Select this typo function if there is a match */
        if ( ((ptfTypoFunctionPtr->uiTypoID == uiTypoID) || (ptfTypoFunctionPtr->uiTypoID == LNG_TYPO_ANY_ID)) &&
                ((ptfTypoFunctionPtr->uiLanguageID == uiLanguageID) || (ptfTypoFunctionPtr->uiLanguageID == LNG_LANGUAGE_ANY_ID)) ) {
            
            /* Allocate the language typo structure */
            if ( (pltLngTypo = (struct lngTypo *)s_malloc((size_t)(sizeof(struct lngTypo)))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Set the language typo structure index */
            pltLngTypo->uiTypoID = uiTypoID;
            pltLngTypo->uiLanguageID = uiLanguageID;
            pltLngTypo->iLngTypoFunction = ptfTypoFunctionPtr->iLngTypoFunction;

            /* Set the return pointer */
            *ppvLngTypo = (void *)pltLngTypo;

            return (LNG_NoError);
        }
    }
    

    /* There is no typo function available for this typo name and language code combination */
    return (LNG_TypoUnavailableTypo);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTypoFree()

    Purpose:    This function frees the language typo structure.

    Parameters: pvLngTypo       Language typo structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngTypoFree
(
    void *pvLngTypo
)
{

    struct lngTypo  *pltLngTypo = (struct lngTypo *)pvLngTypo;


    /* Check the parameters */
    if ( pltLngTypo == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pltLngTypo' parameter passed to 'iLngTypoFree'."); 
        return (LNG_TypoInvalidTypo);
    }

    
    /* Free the language typo structure */
    s_free(pltLngTypo);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTypoGetTypoCharacterList()

    Purpose:    This is the router function for the typo

    Parameters: pvLngTypo                   Language typo structure
                pwcTerm                     Pointer to the term for which we want the typo
                ppwcTypoCharacterList       Return pointer for the character list to scan for this typo key

    Globals:    pltLngTypoFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngTypoGetTypoCharacterList
(
    void *pvLngTypo,
    wchar_t *pwcTerm,
    wchar_t **ppwcTypoCharacterList
)
{

    struct lngTypo      *pltLngTypo = (struct lngTypo *)pvLngTypo;
    wchar_t             pwcTypoCharacterList[2] = {L'\0'};
    wchar_t             *pwcTypoCharacterListPtr = NULL;


    /* Check the parameters */
    if ( pltLngTypo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pltLngTypo' parameter passed to 'iLngTypoGetTypoCharacterList'."); 
        return (LNG_TypoInvalidTypo);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngTypoGetTypoCharacterList'."); 
        return (LNG_TypoInvalidTerm);
    }

    if ( ppwcTypoCharacterList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcTypoCharacterList' parameter passed to 'iLngTypoGetTypoCharacterList'."); 
        return (LNG_ReturnParameterError);
    }

    
    /* Create the character list */
    pwcTypoCharacterList[0] = pwcTerm[0];
    pwcTypoCharacterList[1] = L'\0';

    /* Duplicate the character list */
    if ( (pwcTypoCharacterListPtr = s_wcsdup(pwcTypoCharacterList)) == NULL ) {
        return (LNG_MemError);
    }
    
    /* Set the return pointer */
    *ppwcTypoCharacterList = pwcTypoCharacterListPtr;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTypoGetTypoMatch()

    Purpose:    This is the router function for the typo

    Parameters: pvLngTypo               Language typo structure
                pwcTerm                 Pointer to the term for which we want the typo
                pwcCandidateTerm        Pointer to the candidate term
                bCaseSensitive          True is the match should be case sensitive
                uiTypoMaxCount          Maximum number of typographical errors    

    Globals:    pltLngTypoFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngTypoGetTypoMatch
(
    void *pvLngTypo,
    wchar_t *pwcTerm,
    wchar_t *pwcCandidateTerm,
    boolean bCaseSensitive, 
    unsigned int uiTypoMaxCount
)
{

    struct lngTypo      *pltLngTypo = (struct lngTypo *)pvLngTypo;
    int                 iError = LNG_NoError;


    /* Check the parameters */
    if ( pltLngTypo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pltLngTypo' parameter passed to 'iLngTypoGetTypoMatch'."); 
        return (LNG_TypoInvalidTypo);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngTypoGetTypoMatch'."); 
        return (LNG_TypoInvalidTerm);
    }

    if ( bUtlStringsIsWideStringNULL(pwcCandidateTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcCandidateTerm' parameter passed to 'iLngTypoGetTypoMatch'."); 
        return (LNG_TypoInvalidCandidateTerm);
    }

    if ( uiTypoMaxCount <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTypoMaxCount' parameter passed to 'iLngTypoGetTypoMatch'."); 
        return (LNG_TypoInvalidMaxCount);
    }


    /* Check that the typo function is valid */
    if ( pltLngTypo->iLngTypoFunction == NULL ) {
        return (LNG_TypoInvalidTypo);
    }

    /* Call the typo function, note that the boolean is cast to an int otherwise the compiler complains */
    iError = pltLngTypo->iLngTypoFunction(pltLngTypo, pwcTerm, pwcCandidateTerm, (int)bCaseSensitive, uiTypoMaxCount);
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTypoStandardMatch_un()

    Purpose:    Compare a candidate term with a real term to check for 
                typographical errors.

    Parameters: pltLngTypo          Language typo structure
                pwcTerm             term to check against
                pwcCandidateTerm    candidate term
                bCaseSensitive      true is the match should be case sensitive
                uiTypoMaxCount      maximum number of typographical errors    

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTypoStandardMatch_un
(
    struct lngTypo *pltLngTypo,
    wchar_t *pwcTerm,
    wchar_t *pwcCandidateTerm,
    int bCaseSensitive,
    unsigned int uiTypoMaxCount
)
{

    unsigned int    uiErrorCount = 0;
    unsigned int    uiTermIndex = 0;
    unsigned int    uiCandidateTermIndex = 0;
    wchar_t         wcTerm = L'\0';
    wchar_t         wcCandidateTerm = L'\0';


    ASSERT(pltLngTypo != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(bUtlStringsIsWideStringNULL(pwcCandidateTerm) == false);
    ASSERT((bCaseSensitive == true) || (bCaseSensitive == false));
    ASSERT(uiTypoMaxCount > 0);


    /* Check that the first letters match */
    if ( bCaseSensitive == false ) {
        if ( towlower(pwcTerm[0]) != towlower(pwcCandidateTerm[0]) ) {
            return (LNG_TypoFailedMatch);
        }
    }
    else {
        if ( pwcTerm[0] != pwcCandidateTerm[0] ) {
            return (LNG_TypoFailedMatch);
        }
    }


    /* Check for the case where the terms are the same */
    if ( bCaseSensitive == false ) {
        if ( s_wcscasecmp(pwcTerm, pwcCandidateTerm) == 0 ) {
            return (LNG_NoError);
        }
    }
    else {
        if ( s_wcscmp(pwcTerm, pwcCandidateTerm) == 0 ) {
            return (LNG_NoError);
        }
    }


    /* Check for the case where one term is a subset of the other */
    if ( bCaseSensitive == false ) {
        if ( (s_wcsncasecmp(pwcTerm, pwcCandidateTerm, s_wcslen(pwcTerm)) == 0) && 
                ((s_wcslen(pwcCandidateTerm) - s_wcslen(pwcTerm)) <= uiTypoMaxCount) ) {
            return (LNG_NoError);
        }
    }
    else {
        if ( (s_wcsncmp(pwcTerm, pwcCandidateTerm, s_wcslen(pwcTerm)) == 0) && 
                ((s_wcslen(pwcCandidateTerm) - s_wcslen(pwcTerm)) <= uiTypoMaxCount) ) {
            return (LNG_NoError);
        }
    }


    wcTerm = pwcTerm[++uiTermIndex];
    wcCandidateTerm = pwcCandidateTerm[++uiCandidateTermIndex];

    /* Loop over each letter */
    while ( (wcTerm != L'\0') && (wcCandidateTerm != L'\0') ) {

        
        /* Downcase the letters if this is not case sensitive */
        if ( bCaseSensitive == false ) {
            wcTerm = towlower(wcTerm);
            wcCandidateTerm = towlower(wcCandidateTerm);
        }


        /* If the letters dont match? */
        if ( wcTerm != wcCandidateTerm ) {

            if ( (pwcCandidateTerm[uiCandidateTermIndex + 1] == L'\0') && (pwcTerm[uiTermIndex + 1] == L'\0') ) {
                ;
            }
            /* Is this a juxaposition? */
            else if ( (wcTerm == pwcCandidateTerm[uiCandidateTermIndex + 1]) && (pwcTerm[uiTermIndex + 1] == wcCandidateTerm) ) {
                /* It is, skip over it */
                uiTermIndex++;
                uiCandidateTermIndex++;
            }

            /* Is this just the wrong letter? */
            else if ( pwcTerm[uiTermIndex + 1] == pwcCandidateTerm[uiCandidateTermIndex + 1] ) {
                /* It is, skip over it */
                uiTermIndex++;
                uiCandidateTermIndex++;
            }

            /* Is this an extra letter? */
            else if ( (wcTerm == pwcCandidateTerm[uiCandidateTermIndex + 1]) ) {
                /* It is, skip over it */
                uiCandidateTermIndex++;
            }

            /* Is this an extra letter? */
            else if ( pwcTerm[uiTermIndex + 1] == wcCandidateTerm ) {
                /* It is, skip over it */
                uiTermIndex++;
            }

            /* Is this an accentless letter? */
            else if ( wcLngCaseStripAccentFromWideCharacter(pwcTerm[uiTermIndex]) == wcLngCaseStripAccentFromWideCharacter(pwcCandidateTerm[uiCandidateTermIndex]) ) {
                /* It is, skip over it */
                uiTermIndex++;
                uiCandidateTermIndex++;
            }

            /* Letters are just plain wrong with no recovery */
            else {
                ;
            }

            /* Increment the error count */
            uiErrorCount++;

        }

        wcTerm = pwcTerm[++uiTermIndex];
        wcCandidateTerm = pwcCandidateTerm[++uiCandidateTermIndex];

    }

    /* How many letters are there left */
    if ( pwcCandidateTerm[uiCandidateTermIndex] != L'\0' ) {
        uiErrorCount += s_wcslen(&pwcCandidateTerm[uiCandidateTermIndex]);
    }
    else if ( pwcTerm[uiTermIndex] != L'\0' ) {
        uiErrorCount += s_wcslen(&pwcTerm[uiTermIndex]);
    }


    return (((uiErrorCount > 0) && (uiErrorCount <= uiTypoMaxCount)) ? LNG_NoError : LNG_TypoFailedMatch);

}


/*---------------------------------------------------------------------------*/



