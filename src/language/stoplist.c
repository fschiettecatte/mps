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

    Module:     stoplist.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This implements the stop list. We can either use the
                built in list, or use an external file for the list.
            
                The modified stop lists have had single characters,
                single digits and all apostrophied and plural variations
                of each word in the original list added.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.stoplist"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define LNG_STOPLIST_TERM_LENGTH_MAX        (1024)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/


/* Language stoplist function structure */
struct lngStopListFunction {
    unsigned int    uiStopListID;               /* StopList ID */
    unsigned int    uiLanguageID;               /* Language ID */
    wchar_t         **ppwcStopListTermList;     /* StopList term list */
};


/* Language stoplist structure */
struct lngStopList {
    unsigned int    uiStopListID;               /* StopList ID - ANY means from file */
    unsigned int    uiLanguageID;               /* Language ID */
    wchar_t         **ppwcStopListTermList;     /* StopList term list */
    unsigned int    uiStopListTermListLength;   /* StopList term list length */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLngCompareStopTerms (wchar_t **ppwcTerm1, wchar_t **ppwcTerm2);


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* No stop term list defined */ 
static wchar_t *ppwcStopListNoneGlobal[] = {
    NULL};


/* Google stop list */
static wchar_t *ppwcStopListGoogleGlobal_en[] = {
    L"a",
    L"about",
    L"an",
    L"are",
    L"as",
    L"at",
    L"be",
    L"by",
    L"com",
    L"edu",
    L"for",
    L"from",
    L"how",
    L"i",
    L"in",
    L"is",
    L"it",
    L"of",
    L"on",
    L"that",
    L"the",
    L"this",
    L"to",
    L"was",
    L"what",
    L"when",
    L"where",
    L"which",
    L"who",
    L"why",
    L"will",
    L"with",
    NULL};


/* Google stop list, with modifications, filled out single character words and word variations */
static wchar_t *ppwcStopListGoogleModifiedGlobal_en[] = {
    L"0",
    L"1",
    L"2",
    L"3",
    L"4",
    L"5",
    L"6",
    L"7",
    L"8",
    L"9",
    L"a",
    L"about",
    L"an",
    L"are",
    L"as",
    L"at",
    L"b",
    L"be",
    L"by",
    L"c",
    L"com",
    L"d",
    L"e",
    L"edu",
    L"f",
    L"for",
    L"for's",
    L"from",
    L"g",
    L"h",
    L"how",
    L"how's",
    L"how'd",
    L"i",
    L"in",
    L"is",
    L"it",
    L"it'd",
    L"it'll",
    L"it's",
    L"its",
    L"j",
    L"k",
    L"l",
    L"m",
    L"n",
    L"o",
    L"of",
    L"on",
    L"p",
    L"q",
    L"r",
    L"s",
    L"t",
    L"that",
    L"that'll",
    L"that's",
    L"thats",
    L"the",
    L"this",
    L"this'd",
    L"this'll",
    L"to",
    L"u",
    L"v",
    L"w",
    L"was",
    L"what",
    L"what'd",
    L"what'll",
    L"what're",
    L"what's",
    L"when",
    L"when's",
    L"where",
    L"where'd",
    L"where'll",
    L"where're",
    L"where's",
    L"where've",
    L"which",
    L"who",
    L"who'd",
    L"who'll",
    L"who're",
    L"who's",
    L"who've",
    L"why",
    L"why'd",
    L"will",
    L"will's",
    L"wills",
    L"with",
    L"x",
    L"y",
    L"z",
    NULL};


/* StopList functions */
static struct lngStopListFunction plslfLngStopListFunctionListGlobal[] = 
{

    /* None */
    {   LNG_STOP_LIST_NONE_ID,
        LNG_LANGUAGE_EN_ID,
        ppwcStopListNoneGlobal,
    },

    /* Google */
    {   LNG_STOP_LIST_GOOGLE_ID,
        LNG_LANGUAGE_EN_ID,
        ppwcStopListGoogleGlobal_en,
    },

    /* Google modified */
    {   LNG_STOP_LIST_GOOGLE_MODIFIED_ID,
        LNG_LANGUAGE_EN_ID,
        ppwcStopListGoogleModifiedGlobal_en,
    },

    /* Terminator */
    {    0,
        0,
        NULL,
    },
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStopListCreateByName()

    Purpose:    This function returns a language stop list structure for a particular stop list name
                and language code combination.

    Parameters: pucStopListName     StopList name
                pucLanguageCode     Language code
                ppvLngStopList      Return pointer for the language stop list structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngStopListCreateByName
(
    unsigned char *pucStopListName,
    unsigned char *pucLanguageCode,
    void **ppvLngStopList
)
{

    int             iError = LNG_NoError;
    unsigned int    uiStopListID = 0;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStopListName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStopListName' parameter passed to 'iLngStopListCreateByName'."); 
        return (LNG_LanguageInvalidStopListName);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngStopListCreateByName'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngStopList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngStopList' parameter passed to 'iLngStopListCreateByName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the stop list ID */
    if ( (iError = iLngGetStopListIDFromName(pucStopListName, &uiStopListID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list ID from the stop list name: '%s', lng error: %d.", pucStopListName, iError); 
        return (iError);
    }

    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngStopListCreateByID(uiStopListID, uiLanguageID, ppvLngStopList));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStopListCreateByID()

    Purpose:    This function returns a language stop list structure for a particular stop list ID 
                and language ID combination.

    Parameters: uiStopListID        StopList ID
                uiLanguageID        Language ID
                ppvLngStopList      Return pointer for the language stop list structure

    Globals:    plslfLngStopListFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngStopListCreateByID
(
    unsigned int uiStopListID,
    unsigned int uiLanguageID,
    void **ppvLngStopList
)
{

    int                             iError = LNG_NoError;
    struct lngStopListFunction      *plslfLngStopListFunctionPtr = plslfLngStopListFunctionListGlobal;
    struct lngStopList              *plslLngStopList = NULL;


    /* Check the parameters */
    if ( uiStopListID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStopListID' parameter passed to 'iLngStopListCreateByID'."); 
        return (LNG_LanguageInvalidStopListID);
    }
    
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngStopListCreateByID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( ppvLngStopList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngStopList' parameter passed to 'iLngStopListCreateByID'."); 
        return (LNG_ReturnParameterError);
    }
    

    /* Canonicalize the language ID */
    if ( (iError = iLngGetCanonicalLanguageID(uiLanguageID, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiLanguageID, iError); 
        return (iError);
    }


    /* Loop over the stop list function structures to see which one we want */
    for ( plslfLngStopListFunctionPtr = plslfLngStopListFunctionListGlobal;    plslfLngStopListFunctionPtr->ppwcStopListTermList != NULL; plslfLngStopListFunctionPtr++ ) {

        /* Select this stop list function if there is a match */
        if ( ((plslfLngStopListFunctionPtr->uiStopListID == uiStopListID) || (plslfLngStopListFunctionPtr->uiStopListID == LNG_STOP_LIST_ANY_ID)) &&
                ((plslfLngStopListFunctionPtr->uiLanguageID == uiLanguageID) || (plslfLngStopListFunctionPtr->uiLanguageID == LNG_LANGUAGE_ANY_ID)) ) {
            
            /* Allocate the language stop list structure */
            if ( (plslLngStopList = (struct lngStopList *)s_malloc((size_t)(sizeof(struct lngStopList)))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Set the language stop list structure values */
            plslLngStopList->uiStopListID = uiStopListID;
            plslLngStopList->uiLanguageID = uiLanguageID;
            plslLngStopList->ppwcStopListTermList = plslfLngStopListFunctionPtr->ppwcStopListTermList;

            /* Get the length of the stop list, we computer this dynamically so that we dont have to do it manually everytime the list changes ;) */
            for ( plslLngStopList->uiStopListTermListLength = 0; plslLngStopList->ppwcStopListTermList[plslLngStopList->uiStopListTermListLength] != NULL; 
                    plslLngStopList->uiStopListTermListLength++ ) {
                /* */
            }

            /* Set the return pointer */
            *ppvLngStopList = (void *)plslLngStopList;

            return (LNG_NoError);
        }
    }
    

    /* There is no stop list function available for this stop list name and language code combination */
    return (LNG_StopListUnavailableStopList);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStopListCreateByNameFromFile()

    Purpose:    This function returns a language stop list structure for a particular file, 
                the language code is only there for convenience

    Parameters: pucStopListFilePath     StopList file path
                pucLanguageCode         Language code
                ppvLngStopList          Return pointer for the language stop list structure

    Globals:    plslfLngStopListFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngStopListCreateByNameFromFile
(
    unsigned char *pucStopListFilePath,
    unsigned char *pucLanguageCode, 
    void **ppvLngStopList
)
{


    int             iError = LNG_NoError;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStopListFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStopListFilePath' parameter passed to 'iLngStopListCreateByNameFromFile'."); 
        return (LNG_StopListInvalidStopListFilePath);
    }
    
    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngStopListCreateByNameFromFile'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngStopList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngStopList' parameter passed to 'iLngStopListCreateByNameFromFile'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngStopListCreateByIDFromFile(pucStopListFilePath, uiLanguageID, ppvLngStopList));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStopListCreateByIDFromFile()

    Purpose:    This function returns a language stop list structure for 
                a particular file, the language ID is only there for convenience

    Parameters: pucStopListFilePath     StopList file path
                uiLanguageID            Language ID
                ppvLngStopList          Return pointer for the language stop list structure

    Globals:    plslfLngStopListFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngStopListCreateByIDFromFile
(
    unsigned char *pucStopListFilePath,
    unsigned int uiLanguageID,
    void **ppvLngStopList
)
{

    int                     iError = LNG_NoError;
    wchar_t                 **ppwcStopListTermList = NULL;
    unsigned int            uiStopListTermListLength = 0;
    struct lngStopList      *plslLngStopList = NULL;
    FILE                    *pfStopListFile = NULL;
    unsigned char           pucTerm[(LNG_STOPLIST_TERM_LENGTH_MAX * LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER) + 1] = {'\0'};
    

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStopListFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStopListFilePath' parameter passed to 'iLngStopListCreateByIDFromFile'."); 
        return (LNG_StopListInvalidStopListFilePath);
    }
    
    if ( ppvLngStopList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngStopList' parameter passed to 'iLngStopListCreateByIDFromFile'."); 
        return (LNG_ReturnParameterError);
    }


    /* Open the passed stop list file */
    if ( (pfStopListFile = s_fopen(pucStopListFilePath, "r")) == NULL ) {
        return (LNG_StopListInvalidStopListFile);
    }

    /* Allocate the language stop list structure */
    if ( (plslLngStopList = (struct lngStopList *)s_malloc((size_t)(sizeof(struct lngStopList)))) == NULL ) {
        return (LNG_MemError);
    }
    
    /* Set the language stop list structure values */
    plslLngStopList->uiLanguageID = uiLanguageID;
    plslLngStopList->uiStopListID = LNG_STOP_LIST_ANY_ID;        /* This tells us that the stop list term list must be freed when we feel the stop list */
    plslLngStopList->uiStopListTermListLength = 0;
    plslLngStopList->ppwcStopListTermList = NULL;


    /* Loop through each stop term and add it to the list  */
    while ( s_fgets(pucTerm, (LNG_STOPLIST_TERM_LENGTH_MAX * LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER), pfStopListFile) != NULL ) {

        unsigned long   uiTermLength = 0;
        unsigned char   *pucTermEndPtr = NULL;
        wchar_t         *pwcTerm = NULL;
        wchar_t         **ppwcStopListTermListPtr = NULL;


        /* Check for a comment */
        if ( pucTerm[0] == '#' ) {
            continue;
        }

        /* Get the term length */
        uiTermLength = s_strlen(pucTerm);

        /* Erase the trailing new line - be platform sensitive, handle PC first, then Unix and Mac  */
        if ( (uiTermLength >= 2) && (pucTerm[uiTermLength - 2] == '\r') ) {
            uiTermLength -= 2;
            pucTermEndPtr = pucTerm + uiTermLength;
        }
        else if ( (uiTermLength >= 1) && ((pucTerm[uiTermLength - 1] == '\n') || (pucTerm[uiTermLength - 1] == '\r')) ) {
            uiTermLength -= 1;
            pucTermEndPtr = pucTerm + uiTermLength;
        }
        else {
            pucTermEndPtr = NULL;
        }

        /* Erase the trailing new line */
        if ( pucTermEndPtr != NULL ) {
            *pucTermEndPtr = '\0';
        }


        /* Trim the string */
        iUtlStringsTrimString(pucTerm);


        /* Check for an empty string */
        if ( bUtlStringsIsStringNULL(pucTerm) == true ) {
            continue;
        }
    
        /* Convert the stop term from utf-8 to wide characters - pwcTerm is allocated */
        if ( (iError = iLngConvertUtf8ToWideString_d(pucTerm, 0, &pwcTerm)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a stop terms from utf-8 to wide characters, lng error: %d.", iError);
            goto bailFromiLngStopListCreateByIDsFromFile;
        }

        /* Extend the list */
        if ( (ppwcStopListTermListPtr = (wchar_t **)s_realloc(ppwcStopListTermList, (size_t)((uiStopListTermListLength + 1) * sizeof(wchar_t *)))) == NULL ) {
            iError = LNG_MemError;
            goto bailFromiLngStopListCreateByIDsFromFile;
        }
        
        /* Hand over the list pointer */
        ppwcStopListTermList = ppwcStopListTermListPtr;
        
        /* And add the term */
        ppwcStopListTermList[uiStopListTermListLength++] = pwcTerm;
    }



    /* Bail label */
    bailFromiLngStopListCreateByIDsFromFile:

    /* Close the stop list file */
    s_fclose(pfStopListFile);

    /* Handle the error */
    if ( iError == LNG_NoError ) {
    
        /* Set the structure pointers */
        plslLngStopList->ppwcStopListTermList = ppwcStopListTermList;
        plslLngStopList->uiStopListTermListLength = uiStopListTermListLength;
    
        /* Sort the stop terms in accending order */
        s_qsort(plslLngStopList->ppwcStopListTermList, plslLngStopList->uiStopListTermListLength, sizeof(wchar_t *), (int (*)(const void *, const void *))iLngCompareStopTerms);
        
        /* Set the return pointer */
        *ppvLngStopList = plslLngStopList;
    }
    else {
        
        /* Free the stop term list */
        if ( ppwcStopListTermList != NULL ) {
            for ( uiStopListTermListLength = 0; uiStopListTermListLength < uiStopListTermListLength; uiStopListTermListLength++ ) {
                s_free(ppwcStopListTermList[uiStopListTermListLength]);
            }
            s_free(ppwcStopListTermList);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStopListFree()

    Purpose:    This function frees the language stop list structure.

    Parameters: pvLngStopList       StopList handle

    Globals:    none

    Returns:    An LNG error code

*/
int iLngStopListFree
(
    void *pvLngStopList
)
{

    struct lngStopList      *plslLngStopList = (struct lngStopList *)pvLngStopList;
    unsigned int            uiStopListTermListLength = 0;


    /* Check the parameters */
    if ( plslLngStopList == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'plslLngStopList' parameter passed to 'iLngStopListFree'."); 
        return (LNG_StopListInvalidStopList);
    }
    
    
    /* Free the term list */
    if ( plslLngStopList->uiStopListID == LNG_STOP_LIST_ANY_ID ) {
        if ( plslLngStopList->ppwcStopListTermList != NULL ) {
            for ( uiStopListTermListLength = 0; uiStopListTermListLength < plslLngStopList->uiStopListTermListLength; uiStopListTermListLength++ ) {
                s_free(plslLngStopList->ppwcStopListTermList[uiStopListTermListLength]);
            }
            s_free(plslLngStopList->ppwcStopListTermList);
        }
    }

    /* Free the language stop list structure */
    s_free(plslLngStopList);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngStopListGetTermList()

    Purpose:    This is the router function for the stop list

    Parameters: pvLngStopList               StopList handle
                pppwcStopListTermList       Return pointer for the stop list term list
                puiStopListTermListLength   Return pointer for the stop list term list length

    Globals:    plslfLngStopListFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngStopListGetTermList
(
    void *pvLngStopList,
    wchar_t ***pppwcStopListTermList,
    unsigned int *puiStopListTermListLength
)
{

    struct lngStopList      *plslLngStopList = (struct lngStopList *)pvLngStopList;


    /* Check the parameters */
    if ( pvLngStopList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngStopList' parameter passed to 'iLngStopListGetTermList'."); 
        return (LNG_StopListInvalidStopList);
    }

    if ( pppwcStopListTermList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pppwcStopListTermList' parameter passed to 'iLngStopListGetTermList'."); 
        return (LNG_ReturnParameterError);
    }

    if ( puiStopListTermListLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiStopListTermListLength' parameter passed to 'iLngStopListGetTermList'."); 
        return (LNG_ReturnParameterError);
    }


    /* Set the return pointers */
    *pppwcStopListTermList = plslLngStopList->ppwcStopListTermList;
    *puiStopListTermListLength = plslLngStopList->uiStopListTermListLength;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCompareStopTerms()

    Purpose:    This functions takes two stop terms and compares them.

    Parameters: ppwcTerm1       pointer to a term pointer
                ppwcTerm2       pointer to a term pointer

    Globals:    none

    Returns:    1 if pwcTerm2 > pwcTerm1,
                -1 if pwcTerm1 > pwcTerm2, 
                and 0 if pwcTerm1 == pwcTerm2

*/

static int iLngCompareStopTerms
(
    wchar_t **ppwcTerm1,
    wchar_t **ppwcTerm2
)
{

    /* Assertions */    
    ASSERT(ppwcTerm1 != NULL)
    ASSERT(ppwcTerm2 != NULL)
    ASSERT(*ppwcTerm1 != NULL)
    ASSERT(*ppwcTerm2 != NULL)


    /* Just compare the terms */
    return (s_wcscoll(*ppwcTerm1, *ppwcTerm2));

}


/*---------------------------------------------------------------------------*/
