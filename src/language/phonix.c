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

    Module:     phonix.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 April 1998

    Purpose:    This implements the phonix algorithm as defined by Gadd. 

                T.N. Gadd: PHONIX --- The Algorithm, Program 24/4, 1990, 
                p.363-366.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.phonix"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define LNG_PHONIX_KEY              L"Z0000000"         /* Default phonix key */


#define LNG_PHONIX_NON              (1)                 /* No condition */
#define LNG_PHONIX_VOC              (2)                 /* Vowel needed */ 
#define LNG_PHONIX_CON              (3)                 /* Consonant needed */

#define LNG_PHONIX_START            (1)                 /* Condition refers to beginning of term */
#define LNG_PHONIX_MIDDLE           (2)                 /* Condition refers to middle of term */
#define LNG_PHONIX_END              (3)                 /* Condition refers to end position of term */
#define LNG_PHONIX_ALL              (4)                 /* Condition refers to whole term */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Language phonix function structure */
struct lngPhonixFunction {
    unsigned int    ulPhonixID;                         /* Phonix ID */
    unsigned int    uiLanguageID;                       /* Language ID */
    int             (*iLngPhonixFunction)();            /* Phonix function pointer */
};


/* Language phonix structure */
struct lngPhonix {
    unsigned int    uiPhonixID;                         /* Phonix ID */
    unsigned int    uiLanguageID;                       /* Language ID */
    int             (*iLngPhonixFunction)();            /* Phonix function pointer (denormalization) */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLngPhonixStandardKey_un (struct lngPhonix *plpLngPhonix, wchar_t *pwcTerm, 
        wchar_t *pwcPhonixKey, unsigned int uiPhonixKeyLen);

static int iLngPhonixStrDel (wchar_t *pwcDelPos, unsigned int uiDelSize);

static int iLngPhonixStrIns (wchar_t *pwcInsPos, wchar_t *pwcInsStr);

static boolean bLngPhonixIsVowel (wchar_t wcChar);

static int iLngPhonixCode (wchar_t *pwcTerm, wchar_t *pwcPhonixKey);

static int iLngPhonixReplace1 (unsigned int uiWhere, wchar_t *pwcTerm,
        wchar_t *pwcOldString, wchar_t *pwcNewString, 
        unsigned int uiCondPre, unsigned int uiCondPost);

static int iLngPhonixReplace2 (unsigned int uiWhere, wchar_t *pwcTerm, 
        wchar_t *pwcOldString, wchar_t *pwcNewString);


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Phonix functions */
static struct lngPhonixFunction plpfLngPhonixFunctionListGlobal[] = 
{
    /* Standard Phonix - euro languages */
    {   LNG_PHONIX_STANDARD_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngPhonixStandardKey_un,
    },

    /* Terminator */
    {   0,
        0,
        NULL,
    },
};


/* Phonix cross-check list */
static wchar_t *ppwcLngPhonixCrossCheckListGlobal[] = {
    L"aehiorsuy",       /* A */
    L"b",               /* B */
    L"ct",              /* C */
    L"djr",             /* D */
    L"aeiotuxy",        /* E */
    L"fp",              /* F */
    L"deg",             /* G */
    L"h",               /* H */
    L"aeilouy",         /* I */
    L"j",               /* J */
    L"cgjkq",           /* K */
    L"l",               /* L */
    L"m",               /* M */
    L"kgmnp",           /* N */
    L"aeiouy",          /* O */
    L"p",               /* P */
    L"q",               /* Q */
    L"hrw",             /* R */
    L"cepsz",           /* S */
    L"bdgptz",          /* T */
    L"aeiouy",          /* U */
    L"vw",              /* V */
    L"w",               /* W */
    L"x",               /* X */
    L"aeijouy",         /* Y */
    L"wz",              /* Z */
    NULL};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixCreateByName()

    Purpose:    This function returns a language phonix structure for a particular phonix name 
                and language code combination.

    Parameters: pucPhonixName       Phonix name
                pucLanguageCode     Language code
                ppvLngPhonix        Return pointer for the language phonix structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngPhonixCreateByName
(
    unsigned char *pucPhonixName,
    unsigned char *pucLanguageCode,
    void **ppvLngPhonix
)
{

    int             iError = LNG_NoError;
    unsigned int    uiPhonixID = 0;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPhonixName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPhonixName' parameter passed to 'iLngPhonixCreateByName'."); 
        return (LNG_LanguageInvalidPhonixName);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngPhonixCreateByName'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngPhonix == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngPhonix' parameter passed to 'iLngPhonixCreateByName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the phonix ID */
    if ( (iError = iLngGetPhonixIDFromName(pucPhonixName, &uiPhonixID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the phonix ID from the phonix name: '%s', lng error: %d.", pucPhonixName, iError); 
        return (iError);
    }

    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngPhonixCreateByID(uiPhonixID, uiLanguageID, ppvLngPhonix));
    
}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixCreateByID()

    Purpose:    This function returns a language phonix structure for a particular phonix ID 
                and language ID combination.

    Parameters: uiPhonixID      Phonix name
                uiLanguageID    Language ID
                ppvLngPhonix    Return pointer for the language phonix structure

    Globals:    plpfLngPhonixFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngPhonixCreateByID
(
    unsigned int uiPhonixID,
    unsigned int uiLanguageID,
    void **ppvLngPhonix
)
{

    int                         iError = LNG_NoError;
    struct lngPhonixFunction    *plpfLngPhonixFunctionPtr = plpfLngPhonixFunctionListGlobal;
    struct lngPhonix            *plpLngPhonix = NULL;


    /* Check the parameters */
    if ( uiPhonixID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiPhonixID' parameter passed to 'iLngPhonixCreateByID'."); 
        return (LNG_LanguageInvalidPhonixID);
    }
    
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngPhonixCreateByID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( ppvLngPhonix == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngPhonix' parameter passed to 'iLngPhonixCreateByID'."); 
        return (LNG_ReturnParameterError);
    }


    /* Canonicalize the language ID */
    if ( (iError = iLngGetCanonicalLanguageID(uiLanguageID, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiLanguageID, iError); 
        return (iError);
    }


    /* Loop over the phonix function structures to see which one we want */
    for ( plpfLngPhonixFunctionPtr = plpfLngPhonixFunctionListGlobal; plpfLngPhonixFunctionPtr->iLngPhonixFunction != NULL; plpfLngPhonixFunctionPtr++ ) {

        /* Select this phonix function if there is a match */
        if ( ((plpfLngPhonixFunctionPtr->ulPhonixID == uiPhonixID) || (plpfLngPhonixFunctionPtr->ulPhonixID == LNG_PHONIX_ANY_ID)) &&
                ((plpfLngPhonixFunctionPtr->uiLanguageID == uiLanguageID) || (plpfLngPhonixFunctionPtr->uiLanguageID == LNG_LANGUAGE_ANY_ID)) ) {
            
            /* Allocate the language phonix structure */
            if ( (plpLngPhonix = (struct lngPhonix *)s_malloc((size_t)(sizeof(struct lngPhonix)))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Set the language phonix structure */
            plpLngPhonix->uiPhonixID = uiPhonixID;
            plpLngPhonix->uiLanguageID = uiLanguageID;
            plpLngPhonix->iLngPhonixFunction = plpfLngPhonixFunctionPtr->iLngPhonixFunction;

            /* Set the return pointer */
            *ppvLngPhonix = (void *)plpLngPhonix;

            return (LNG_NoError);
        }
    }
    

    /* There is no phonix function available for this phonix name and language code combination */
    return (LNG_PhonixUnavailablePhonix);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixFree()

    Purpose:    This function frees the language phonix structure.

    Parameters: pvLngPhonix     Language phonix structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngPhonixFree
(
    void *pvLngPhonix
)
{

    struct lngPhonix    *plpLngPhonix = (struct lngPhonix *)pvLngPhonix;


    /* Check the parameters */
    if ( pvLngPhonix == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvLngPhonix' parameter passed to 'iLngPhonixFree'."); 
        return (LNG_PhonixInvalidPhonix);
    }


    /* Free the language phonix structure */
    s_free(plpLngPhonix);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixGetPhonixCharacterList()

    Purpose:    This is the router function for the phonix

    Parameters: pvLngPhonix                 Language phonix structure
                pwcTerm                     Pointer to the term for which we want the phonix
                ppwcPhonixCharacterList     Return pointer for the character list to scan for this phonix key

    Globals:    plpfLngPhonixFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngPhonixGetPhonixCharacterList
(
    void *pvLngPhonix,
    wchar_t *pwcTerm,
    wchar_t **ppwcPhonixCharacterList
)
{

    int                 iError = LNG_NoError;
/*     struct lngPhonix    *plpLngPhonix = (struct lngPhonix *)pvLngPhonix; */
    wint_t              wiCrossCheckIndex = 0;
        

    /* Check the parameters */
    if ( pvLngPhonix == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngPhonix' parameter passed to 'iLngPhonixGetPhonixCharacterList'."); 
        return (LNG_PhonixInvalidPhonix);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngPhonixGetPhonixCharacterList'."); 
        return (LNG_PhonixInvalidTerm);
    }

    if ( ppwcPhonixCharacterList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcPhonixCharacterList' parameter passed to 'iLngPhonixGetPhonixCharacterList'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the index */
    wiCrossCheckIndex = (wint_t)(towlower(pwcTerm[0]) - L'a');

    /* Copy over the cross-check list */
    if ( (wiCrossCheckIndex <= 25) && (ppwcLngPhonixCrossCheckListGlobal[wiCrossCheckIndex] != NULL) ) {
        
        unsigned int    uiCharacterListLength = 0;
        wchar_t         *pwcPhonixCharacterList = NULL;
        wchar_t         wcCharacter = L'\0';
        wchar_t         pwcCharacter[2] = {L'\0'};
        wchar_t         pwcPhonixKey[LNG_PHONIX_KEY_LENGTH + 1] = {L'\0'};


        /* Get the length of the character list, make room for one more character and the terminating NULL */
        uiCharacterListLength = s_wcslen(ppwcLngPhonixCrossCheckListGlobal[wiCrossCheckIndex]) + 2;
        
        /* Allocate space for the character list */
        if ( (pwcPhonixCharacterList = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * uiCharacterListLength))) == NULL ) {
            return (LNG_MemError);
        }
        
        /* Copy over the cross-check list */
        s_wcscpy(pwcPhonixCharacterList, ppwcLngPhonixCrossCheckListGlobal[wiCrossCheckIndex]);
        
        /* Get the phonix key, we need its first character*/
        if ( (iError = iLngPhonixGetPhonixKey(pvLngPhonix, pwcTerm, pwcPhonixKey, LNG_PHONIX_KEY_LENGTH + 1)) != LNG_NoError ) {
            s_free(pwcPhonixCharacterList);
            return (iError);
        }

        /* Add the first character of the encoded term if it is not on the list */
        if ( pwcPhonixKey[0] != L'$' ) {
            
            wcCharacter = towlower(pwcPhonixKey[0]);
            
            if ( s_wcschr(pwcPhonixCharacterList, wcCharacter) == NULL ) {
                pwcCharacter[0] = wcCharacter;
                pwcCharacter[1] = '\0';
                s_wcsnncat(pwcPhonixCharacterList, pwcCharacter, uiCharacterListLength, uiCharacterListLength + 1);
            }
        }

        /* Set the return pointer */
        *ppwcPhonixCharacterList = pwcPhonixCharacterList;
    }
    /* We dont have a cross-check list for this character, so we create a character list for that character only */
    else {
    
        wchar_t     pwcPhonixCharacterList[2] = {L'\0'};
        wchar_t     *pwcPhonixCharacterListPtr = NULL;

        /* Create the character list */
        pwcPhonixCharacterList[0] = pwcTerm[0];
        pwcPhonixCharacterList[1] = L'\0';
    
        /* Duplicate the character list */
        if ( (pwcPhonixCharacterListPtr = s_wcsdup(pwcPhonixCharacterList)) == NULL ) {
            return (LNG_MemError);
        }

        /* Set the return pointer */
        *ppwcPhonixCharacterList = pwcPhonixCharacterListPtr;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixGetPhonixKey()

    Purpose:    This is the router function for the phonix

    Parameters: pvLngPhonix         Language phonix structure
                pwcTerm             Pointer to the term for which we want the phonix key
                pwcPhonixKey        Return pointer for the phonix key
                uiPhonixKeyLen      Length of the return pointer for the phonix key

    Globals:    plpfLngPhonixFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngPhonixGetPhonixKey
(
    void *pvLngPhonix,
    wchar_t *pwcTerm,
    wchar_t *pwcPhonixKey,
    unsigned int uiPhonixKeyLen
)
{

    int                 iError = LNG_NoError;
    struct lngPhonix    *plpLngPhonix = (struct lngPhonix *)pvLngPhonix;


    /* Check the parameters */
    if ( plpLngPhonix == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'plpLngPhonix' parameter passed to 'iLngPhonixGetPhonixKey'."); 
        return (LNG_PhonixInvalidPhonix);
    }

    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcTerm' parameter passed to 'iLngPhonixGetPhonixKey'."); 
        return (LNG_PhonixInvalidTerm);
    }

    if ( pwcPhonixKey == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pwcPhonixKey' parameter passed to 'iLngPhonixGetPhonixKey'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiPhonixKeyLen <= LNG_PHONIX_KEY_LENGTH ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiPhonixKeyLen' parameter passed to 'iLngPhonixGetPhonixKey'."); 
        return (LNG_ParameterError);
    }


    /* Check that the phonix function is valid */
    if ( plpLngPhonix->iLngPhonixFunction == NULL ) {
        return (LNG_PhonixInvalidPhonix);
    }

    /* Call the phonix function */
    if ( (iError = plpLngPhonix->iLngPhonixFunction(plpLngPhonix, pwcTerm, pwcPhonixKey, uiPhonixKeyLen)) != LNG_NoError ) {
        return (LNG_PhonixPhonixingFailed);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixStandardKey_un()

    Purpose:    Calculates the phonix code for the string pwcTerm.

    Parameters: plpLngPhonix        Language phonix structure
                pwcTerm             Pointer to the term for which we want the phonix key
                pwcPhonixKey        Return pointer for the phonix key
                uiPhonixKeyLen      Length of the return pointer for the phonix key

    Globals:    none

    Returns:     LNG error code

*/
static int iLngPhonixStandardKey_un
(
    struct lngPhonix *plpLngPhonix,
    wchar_t *pwcTerm,
    wchar_t *pwcPhonixKey,
    unsigned int uiPhonixKeyLen
)
{

    wchar_t     *pwcNewTerm = NULL;


    ASSERT(plpLngPhonix != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(pwcPhonixKey != NULL);
    ASSERT(uiPhonixKeyLen > LNG_PHONIX_KEY_LENGTH);


    /* Initialize the return variable */
    pwcPhonixKey[0] = L'\0';


    /* Make a copy of the term because we side effect it, also allocate extra space because 
    ** the phonix key for short terms can be longer than the term itself
    */
    if ( (pwcNewTerm = (wchar_t *)s_malloc((size_t)(s_wcslen(pwcTerm) + 100))) == NULL ) {
        return (LNG_MemError);
    }
    s_wcscpy(pwcNewTerm, pwcTerm);

    /* Strip the term of accents */
    pwcLngCaseStripAccentsFromWideString(pwcNewTerm);

    /* Convert the term to upper case */
    pwcLngCaseConvertWideStringToUpperCase(pwcNewTerm);
    


    /* Replace letter groups according to Gadd's definition */
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"DG",      L"G");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CO",      L"KO");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CA",      L"KA");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CU",      L"KU");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CY",      L"SI");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CI",      L"SI");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CE",      L"SE");
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"CL",      L"KL",      LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CK",      L"K");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"GC",      L"K");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"JC",      L"K");
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"CHR",     L"KR",      LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"CR",      L"KR",      LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"WR",      L"R");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"NC",      L"NK");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"CT",      L"KT");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"PH",      L"F");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"AA",      L"AR");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"SCH",     L"SH");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"BTL",     L"TL");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"GHT",     L"T");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"AUGH",    L"ARF");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"LJ",      L"LD",      LNG_PHONIX_VOC, LNG_PHONIX_VOC);
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"LOUGH",   L"LOW");
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"Q",       L"KW");
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"KN",      L"N");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"GN",      L"N");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"GHN",     L"N");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"GNE",     L"N");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"GHNE",    L"NE");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"GNES",    L"NS");
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"GN",      L"N");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"GN",      L"N",       LNG_PHONIX_NON, LNG_PHONIX_CON);
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"GN",      L"N",       LNG_PHONIX_NON, LNG_PHONIX_NON); /* LNG_PHONIX_NON, LNG_PHONIX_CON */
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"PS",      L"S");
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"PT",      L"T");
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"CZ",      L"C");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"WZ",      L"Z",       LNG_PHONIX_VOC,    LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"CZ",      L"CH");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"LZ",      L"LSH");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"RZ",      L"RSH");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"Z",       L"S",       LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"ZZ",      L"TS");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"Z",       L"TS",      LNG_PHONIX_CON, LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"HROUG",   L"REW");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"OUGH",    L"OF");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"Q",       L"KW",      LNG_PHONIX_VOC, LNG_PHONIX_VOC);
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"J",       L"Y",       LNG_PHONIX_VOC, LNG_PHONIX_VOC);
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"YJ",      L"Y",       LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"GH",      L"G");
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"E",       L"GH",      LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"CY",      L"S");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"NX",      L"NKS");
    iLngPhonixReplace2(LNG_PHONIX_START,        pwcNewTerm,     L"PF",      L"F");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"DT",      L"T");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"TL",      L"TIL");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"DL",      L"DIL");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"YTH",     L"ITH");
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"TJ",      L"CH",      LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"TSJ",     L"CH",      LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"TS",      L"T",       LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"TCH",     L"CH");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"WSK",     L"VSKIE",   LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"WSK",     L"VSKIE",   LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"MN",      L"N",       LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace1(LNG_PHONIX_START,        pwcNewTerm,     L"PN",      L"N",       LNG_PHONIX_NON, LNG_PHONIX_VOC);
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"STL",     L"SL",      LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"STL",     L"SL",      LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"TNT",     L"ENT");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"EAUX",    L"OH");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"EXCI",    L"ECS");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"X",       L"ECS");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"NED",     L"ND");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"JR",      L"DR");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"EE",      L"EA");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"ZS",      L"S");
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"R",       L"AH",      LNG_PHONIX_VOC, LNG_PHONIX_CON);
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"R",       L"AH",      LNG_PHONIX_VOC, LNG_PHONIX_NON); /* LNG_PHONIX_VOC, LNG_PHONIX_CON */
    iLngPhonixReplace1(LNG_PHONIX_MIDDLE,       pwcNewTerm,     L"HR",      L"AH",      LNG_PHONIX_VOC, LNG_PHONIX_CON);
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"HR",      L"AH",      LNG_PHONIX_VOC, LNG_PHONIX_NON); /* LNG_PHONIX_VOC, LNG_PHONIX_CON */
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"HR",      L"AH",      LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"RE",      L"AR");
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"R",       L"AH",      LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"LLE",     L"LE");
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"LE",      L"ILE",     LNG_PHONIX_CON, LNG_PHONIX_NON);
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"LES",     L"ILES",    LNG_PHONIX_CON, LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"E",       L"");
    iLngPhonixReplace2(LNG_PHONIX_END,          pwcNewTerm,     L"ES",      L"S");
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"SS",      L"AS",      LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace1(LNG_PHONIX_END,          pwcNewTerm,     L"MB",      L"M",       LNG_PHONIX_VOC, LNG_PHONIX_NON);
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"MPTS",    L"MPS");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"MPS",     L"MS");
    iLngPhonixReplace2(LNG_PHONIX_ALL,          pwcNewTerm,     L"MPT",     L"MT");


    /* Calculate the phonix for the term */
    iLngPhonixCode(pwcNewTerm, pwcPhonixKey);


    /* Free the new term */
    s_free(pwcNewTerm);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "'%ls' --> '%ls' --> '%ls'", pwcTerm, pwcNewTerm, pwcPhonixKey);   */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixCode()

    Purpose:    This calculates a eight-letter phonix code for the string 
                pwcTerm and returns this code in the string pwcPhonixKey.

    Parameters: pwcTerm         The term to create a phonix key for
                pwcPhonixKey    The return parameter for the phonix key

    Globals:    none

    Returns:    LNG error code

*/
static int iLngPhonixCode
(
    wchar_t *pwcTerm,
    wchar_t *pwcPhonixKey
)
{
    wchar_t         *pwcTermPtr = pwcTerm;
    wchar_t         wcLastLetter = L'\0';
    unsigned int    uiIndex = 0;

    int             piCodeList[] = {0, 1, 2, 3, 0, 7, 2, 0, 0, 2, 2, 4, 5, 5, 0, 1, 2, 6, 8, 3, 0, 7, 0, 8, 0, 8};
                                 /* A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z */


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(bUtlStringsIsWideStringNULL(pwcPhonixKey) == false);


    /* Set default phonix key */
    s_wcscpy(pwcPhonixKey, LNG_PHONIX_KEY);

    /* Keep first letter or replace it with '$'
    **
    ** NOTE: Gadd replaces vowels being the first letter of the
    ** term with a 'v'. Due to the implementation of the MPS Server all
    ** letters will be lowercased. Therefore '$' is used instead
    ** of 'v'.
    */
    pwcPhonixKey[0] = (bLngPhonixIsVowel(*pwcTermPtr) == true) ? L'$' : *pwcTermPtr;
    wcLastLetter = *pwcTermPtr;
    pwcTermPtr++;


    /* Scan rest of string */
    for ( uiIndex = 1; (uiIndex < LNG_PHONIX_KEY_LENGTH) && (*pwcTermPtr != L'\0'); pwcTermPtr++ ) {

        /* Use only letters */
        if ( iswalpha(*pwcTermPtr) != 0 ) {

            /* Ignore duplicate successive chars */
            if ( wcLastLetter != *pwcTermPtr ) {
                wcLastLetter = *pwcTermPtr;

                /* Ignore letters with code 0 except as separators */
                if ( piCodeList[towupper(*pwcTermPtr) - L'A'] != 0 ) {
                      pwcPhonixKey[uiIndex] = L'0' + (wint_t)piCodeList[towupper(*pwcTermPtr) - L'A'];
                    uiIndex++;
                }
            }
        }
    }


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixReplace1()

    Purpose:    This procedure replaces the letter group pwcOldString with the letter 
                group pwcNewString in the string pwcTerm, regarding the position of
                pwcOldString where (LNG_PHONIX_START, LNG_PHONIX_MIDDLE, LNG_PHONIX_END, LNG_PHONIX_ALL) 
                and the conditions uiCondPre and uiCondPost (LNG_PHONIX_NON, LNG_PHONIX_VOC, LNG_PHONIX_CON).

                iLngPhonixReplace1(LNG_PHONIX_START, "WAWA", "W", "V", LNG_PHONIX_NON, LNG_PHONIX_NON) 
                replaces only the first W with a V because of the condition LNG_PHONIX_START.

                iLngPhonixReplace1(LNG_PHONIX_START, "WAWA", "W", "V", LNG_PHONIX_NON, LNG_PHONIX_CON)
                replaces neither the first W with a V (because of the condition LNG_PHONIX_CON, 
                i.e. a consonant must follow the W) nor the second W (because of the condition 
                LNG_PHONIX_START).

    Parameters: uiWhere         replace pwcOldString only if it occurs at this position
                pwcTerm         string to work
                pwcOldString    old letter group to delete
                pwcNewString    new letter group to insert
                uiCondPre       condition referring to letter before pwcOldString
                uiCondPost      condition referring to letter after pwcOldString

    Globals:    none

    Returns:     LNG error code

*/
static int iLngPhonixReplace1
(
    unsigned int uiWhere,
    wchar_t *pwcTerm,
    wchar_t *pwcOldString,
    wchar_t *pwcNewString,
    unsigned int uiCondPre,
    unsigned int uiCondPost
)
{

    wchar_t     *pwcOldPosPtr = NULL;
    wchar_t     *pwcEndPosPtr = NULL;
    wchar_t     *pwcTermPtr = pwcTerm;

    /* Vowels before or after pwcOldString */
    wchar_t     wcLetterPre = '\0';     /* Letter before pwcOldString */
    wchar_t     wcLetterPost = '\0';    /* Letter after pwcOldString  */
    boolean     bOkayPre = false;       /* Pre-condition okay?  */
    boolean     bOkayPost = false;      /* Post-condition okay? */


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(bUtlStringsIsWideStringNULL(pwcOldString) == false);
    ASSERT(bUtlStringsIsWideStringNULL(pwcNewString) == false);


    do  { 

        /* Find pwcOldString in pwcTermPtr */
        pwcOldPosPtr = s_wcsstr(pwcTermPtr, pwcOldString);

        /* Find pwcEndPosPtr of pwcTerm */
        pwcEndPosPtr = &pwcTerm[s_wcslen(pwcTerm) - s_wcslen(pwcOldString)];

        /* Check conditions if pwcOldPosPtr != NULL */
        if ( pwcOldPosPtr != NULL ) {

            /* Vowel before pwcOldPosPtr */
            wcLetterPre = (pwcOldPosPtr == pwcTerm) ? L'\0' : *(pwcOldPosPtr - 1);

            /* Vowel after pwcOldPosPtr + s_wcslen(pwcOldString) */
            wcLetterPost = *(pwcOldPosPtr + s_wcslen(pwcOldString));

            /* Check conditions */
            switch ( uiCondPre ) {
                case LNG_PHONIX_NON:
                    bOkayPre = true;
                    break;

                case LNG_PHONIX_VOC: 
                    bOkayPre = wcLetterPre ? bLngPhonixIsVowel(wcLetterPre) : false;
                    break;

                case LNG_PHONIX_CON: 
                    bOkayPre = wcLetterPre ? !bLngPhonixIsVowel(wcLetterPre) : false;
                    break;

                default: 
                    bOkayPre = false;
                    break;
            }

            switch ( uiCondPost ) {
                case LNG_PHONIX_NON: 
                    bOkayPost = true;
                    break;

                case LNG_PHONIX_VOC: 
                    bOkayPost = wcLetterPost ? bLngPhonixIsVowel(wcLetterPost) : false;
                    break;

                case LNG_PHONIX_CON:
                    bOkayPost = wcLetterPost ? !bLngPhonixIsVowel(wcLetterPost) : false;
                    break;

                default: 
                    bOkayPost = false;
                    break;
            }
        }

        /* Replace pwcOldString with pwcNewString */
        if ( (pwcOldPosPtr != NULL) && (bOkayPre == true) && (bOkayPost == true) && 
                (((uiWhere == LNG_PHONIX_START) && (pwcOldPosPtr == pwcTerm)) || 
                ((uiWhere == LNG_PHONIX_MIDDLE) && (pwcOldPosPtr != pwcTerm) && (pwcOldPosPtr != pwcEndPosPtr)) ||
                ((uiWhere == LNG_PHONIX_END) && (pwcOldPosPtr == pwcEndPosPtr)) || 
                (uiWhere == LNG_PHONIX_ALL)) ) {

            /* Replace old letter group with new letter group */
            iLngPhonixStrDel(pwcOldPosPtr, s_wcslen(pwcOldString));
            iLngPhonixStrIns(pwcOldPosPtr, pwcNewString);

            /* Advance pwcTermPtr to the position of pwcOldString */
            pwcTermPtr = pwcOldPosPtr;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iLngPhonixReplace1 = '%ls' --> '%ls'", pwcOldString, pwcNewString);   */
        }
        else {
            /* Advance pwcTermPtr one char */
            pwcTermPtr++;
        }
    }  while ( pwcOldPosPtr != NULL );


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixReplace2()

    Purpose:    This procedure replaces the letter group pwcOldString with the letter 
                group pwcNewString in the string pwcTerm, regarding the position of pwcOldString
                where (LNG_PHONIX_START, LNG_PHONIX_MIDDLE, LNG_PHONIX_END, LNG_PHONIX_ALL).

                iLngPhonixReplace2(LNG_PHONIX_START, "WAWA", "W", "V") replaces only the first W 
                with a V because of the condition LNG_PHONIX_START.

    Parameters: uiWhere         replace pwcOldString only if it occurs at this position
                pwcTerm         string to work
                pwcOldString    old letter group to delete
                pwcNewString    new letter group to insert

    Globals:    none

    Returns:     LNG error code

*/
static int iLngPhonixReplace2
(
    unsigned int uiWhere,
    wchar_t *pwcTerm,
    wchar_t *pwcOldString,
    wchar_t *pwcNewString
)
{

    wchar_t     *pwcOldPosPtr = NULL;
    wchar_t     *pwcEndPosPtr = NULL;
    wchar_t     *pwcTermPtr = pwcTerm;


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(bUtlStringsIsWideStringNULL(pwcOldString) == false);
    ASSERT(bUtlStringsIsWideStringNULL(pwcNewString) == false);


    do { 

        /* Find pwcOldString in pwcTermPtr */
        pwcOldPosPtr = s_wcsstr(pwcTermPtr, pwcOldString);

        /* Find pwcEndPosPtr of pwcTerm */
        pwcEndPosPtr = &pwcTerm[s_wcslen(pwcTerm) - s_wcslen(pwcOldString)];

        /* Replace pwcOldString with pwcNewString */
        if ( (pwcOldPosPtr != NULL) && 
                (((uiWhere == LNG_PHONIX_START) && (pwcOldPosPtr == pwcTerm)) ||
                 ((uiWhere == LNG_PHONIX_MIDDLE) && (pwcOldPosPtr != pwcTerm) && (pwcOldPosPtr != pwcEndPosPtr)) ||
                ((uiWhere == LNG_PHONIX_END) && (pwcOldPosPtr == pwcEndPosPtr)) || 
                (uiWhere == LNG_PHONIX_ALL))) { 

            /* Replace old letter group with new letter group */
            iLngPhonixStrDel(pwcOldPosPtr, s_wcslen(pwcOldString));
            iLngPhonixStrIns(pwcOldPosPtr, pwcNewString);

            /* Advance pwcTermPtr to the position of pwcOldString */
            pwcTermPtr = pwcOldPosPtr;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iLngPhonixReplace2 = '%ls' --> '%ls'", pwcOldString, pwcNewString);   */
        }
        else {
            /* Advance pwcTermPtr one char */
            pwcTermPtr++;
        }
    }  while (pwcOldPosPtr != NULL);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixStrDel()

    Purpose:    This procedure deletes uiDelSize chars at position pucDelPos and 
                moves the remaining chars left to pucDelPos.

                If Del is pointing at the L of the string "DELETE" the call
                iLngPhonixStrDel(Del, 2) will return Del pointing at "TE". 

    Parameters: pucDelPos       pointer to first char to be deleted
                uiDelSize       number of chars to be deleted

    Globals:    none

    Returns:     void

*/
static int iLngPhonixStrDel
(
    wchar_t *pucDelPos,
    unsigned int uiDelSize
)
{

    wchar_t     *pucHelp = pucDelPos + uiDelSize;


    ASSERT(pucDelPos != NULL);


    /* Move chars left */
    while ( *pucHelp != L'\0' ) {
        *pucDelPos++ = *pucHelp++;
    }

    /* Move trailing \0 */
    *pucDelPos = *pucHelp;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngPhonixStrIns()

    Purpose:    iLngPhonixStrIns moves the chars at position pucInsPos right and
                copies the string pucInsStr into this free space.

                If Ins is pointing at the S of the string "INSERT" the call
                iLngPhonixStrIns(Ins, "NEW") will return Ins pointing at "NEWSERT". 

    Parameters: pucInsPos       pointer to insert position

    Globals:    none

    Returns:     new string to be inserted

*/
static int iLngPhonixStrIns
(
    wchar_t *pucInsPos,
    wchar_t *pucInsStr
)
{

    int             iI = 0;
    unsigned int    uiMoveSize = s_wcslen(pucInsStr);


    ASSERT(pucInsPos != NULL);
    ASSERT(pucInsStr != NULL);



    /* Move chars right */
    for ( iI = s_wcslen(pucInsPos) + 1; iI >= 0 ; iI-- ) {
        pucInsPos[iI + uiMoveSize] = pucInsPos[iI];
    }

    /* Copy pucInsStr to pucInsPos */
    while ( *pucInsStr != L'\0' ) {
        *pucInsPos++ = *pucInsStr++;
    }

    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngPhonixIsVowel()

    Purpose:    bLngPhonixIsVowel checks if c is an uppercase vowel or an uppercase Y. If c
                  is one of those chars bLngPhonixIsVowel will return true, else it will return
                a false.

    Parameters: ucChar      character to be examined

    Globals:    none

    Returns:     true if the character is a vowel, false if not

*/
static boolean bLngPhonixIsVowel
(
    wchar_t ucChar
)
{

    return ( ((ucChar == L'A') || (ucChar == L'E') || (ucChar == L'I') ||
            (ucChar == L'O') || (ucChar == L'U') || (ucChar == L'Y') ||
            (ucChar == 0304) || (ucChar == 0344) || (ucChar == 0334) ||
            (ucChar == 0366) || (ucChar == 0326) || (ucChar == 0374)) ? true : false);

}


/*---------------------------------------------------------------------------*/
