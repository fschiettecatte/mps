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

    Module:     functions.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This file contains a the various functions for indexing 
                different types of documents.

                Add up to six functions for each parser to this file:
                
                    boolean bPrsFooDocumentStartFunction (wchar_t *pwcLine);
                    
                    boolean bPrsFooDocumentEndFunction (wchar_t *pwcLine);
                    
                    boolean bPrsFooFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
                            unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription,
                            boolean *pbUnfieldedSearch);
                    
                    void vPrsFooDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
                            unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine);
                    
                    void vPrsFooDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
                            wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
                            unsigned long *pulDocumentAnsiDate);

                Then add the prototypes to functions.h, and the function names to the structure in mpsparser.c

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"
#include "lng.h"

#include "spi.h"

#include "parser.h"
#include "functions.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.parsers.functions"


/* Enable this to filter a number of TREC tags */
/* #define PRS_ENABLE_TREC_TAG_FILTERING */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define PRS_MAX_YEAR_LENGTH             (4)

#define PRS_SHORT_STRING_LENGTH         (512)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Generic field name/description structure */
struct prsFieldNameDescription {
    unsigned int    uiFieldID;                  /* Field ID */
    wchar_t         *pwcFieldName;              /* Field name */
    unsigned char   *pucFieldType;              /* Field type */
    unsigned char   *pucFieldOptions;           /* Field options */
    wchar_t         *pwcFieldDescription;       /* Field description */
    boolean         bUnfieldedSearch;           /* Unfielded search field */
};


/*---------------------------------------------------------------------------*/


/* 
** ======================
** ===  Text Format  ====
** ======================
*/


/* 
** Text format:
**
** One document per file, first line is title, all subsequent lines are text.
**

title text
body text
...

**
*/


/* Field IDs */
#define PRS_TEXT_FIELD_INVALID_ID           (0)
#define PRS_TEXT_FIELD_TITLE_ID             (1)
#define PRS_TEXT_FIELD_TEXT_ID              (2)


/* Field descriptions */
static struct prsFieldNameDescription pfndTextFieldNameDescriptionGlobal[] = 
{
    {PRS_TEXT_FIELD_TITLE_ID,       L"title",   NULL,   NULL,   L"Title",   false},
    {PRS_TEXT_FIELD_TEXT_ID,        L"text",    NULL,   NULL,   L"Text",    false},
    {PRS_TEXT_FIELD_INVALID_ID,     NULL,       NULL,   NULL,   NULL,       false},
};


/* Globals */
static wchar_t pwcTextTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH +1] = {L'\0'};


/*

    Function:   bPrsTextFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsTextFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsTextFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsTextFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsTextFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsTextFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsTextFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndTextFieldNameDescriptionGlobal) / sizeof(struct prsFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pfndTextFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pfndTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pfndTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pfndTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pfndTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pfndTextFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsTextDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 
                
                Here we pick out the first line as the document title.

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcTextTitleGlobal

    Returns:    void

*/
void vPrsTextDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsTextDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsTextDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsTextDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsTextDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsTextDocumentLineFunction'."); 
    }
        
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsTextDocumentLineFunction'."); 
    }
    

    /* Set the document title and the field ID */
    if ( bUtlStringsIsWideStringNULL(pwcTextTitleGlobal) == true ) {
        s_wcsnncpy(pwcTextTitleGlobal, pwcLine, SPI_TITLE_MAXIMUM_LENGTH + 1);
        *puiFieldID = PRS_TEXT_FIELD_TITLE_ID;
    }
    else {
        /* Set the default field ID to the body */
        *puiFieldID = PRS_TEXT_FIELD_TEXT_ID;
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsTextDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcTextTitleGlobal

    Returns:    void

*/
void vPrsTextDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsTextDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsTextDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsTextDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsTextDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsTextDocumentInformationFunction'."); 
    }


    /* Set the document title */
    s_wcsnncpy(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcTextTitleGlobal) == false) ? pwcTextTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH + 1);

    /* Clear the globals */
    pwcTextTitleGlobal[0] = L'\0';


    return;

}


/*---------------------------------------------------------------------------*/


/* 
** ==========================
** ===  Paragraph Format  ===
** ==========================
*/


/* 
** Paragraph format:
** 
** One document per paragraph, first line is title, all subsequent lines are text.
**

title text
body text
...

title text
body text
...

**
*/


/* Field IDs */
#define PRS_PARA_FIELD_INVALID_ID           (0)
#define PRS_PARA_FIELD_TITLE_ID             (1)
#define PRS_PARA_FIELD_TEXT_ID              (2)


/* Field descriptions */
static struct prsFieldNameDescription pfndParaFieldNameDescriptionGlobal[] = 
{
    {PRS_PARA_FIELD_TITLE_ID,       L"title",   NULL,   NULL,   L"Title",   false},
    {PRS_PARA_FIELD_TEXT_ID,        L"text",    NULL,   NULL,   L"Text",    false},
    {PRS_PARA_FIELD_INVALID_ID,     NULL,       NULL,   NULL,   NULL,       false},
};


/* Flags */
#define PRS_PARA_UNDEFINED          (0)
#define PRS_PARA_NEW_DOCUMENT       (1)
#define PRS_PARA_FIRST_LINE         (2)


/* Globals */
static wchar_t pwcParaTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH +1] = {L'\0'};
static unsigned int uiParaTitleFlagGlobal = PRS_PARA_FIRST_LINE;


/*

    Function:   bPrsParaFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsParaFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsParaFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsParaFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsParaFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsParaFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsParaFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndParaFieldNameDescriptionGlobal) / sizeof(struct prsFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pfndParaFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pfndParaFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pfndParaFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pfndParaFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pfndParaFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pfndParaFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsParaDocumentEndFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the end of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsParaDocumentEndFunction
(
    wchar_t *pwcLine
)
{

    wchar_t     *pwcLinePtr = pwcLine;

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsParaDocumentEndFunction'."); 
    }


    /* Loop through all the characters in this line checking to see
    ** if any of them are printable, if there are, we have a real line
    */
    for ( pwcLinePtr = pwcLine; *pwcLinePtr != L'\0'; pwcLinePtr++ ) {
        /* Is this character printable? */
        if ( iswgraph(*pwcLinePtr) != false ) {
            return (false);
        }
    }

    /* Set the flag to indicate a new DOCUMENT */
    uiParaTitleFlagGlobal = PRS_PARA_NEW_DOCUMENT;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsParaDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 
                
    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcParaTitleGlobal, uiParaTitleFlagGlobal

    Returns:    void

*/
void vPrsParaDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsParaDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsParaDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsParaDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsParaDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsParaDocumentLineFunction'."); 
    }
    
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsParaDocumentLineFunction'."); 
    }
    
    
    /* Undefined area, this is the most common */
    if ( uiParaTitleFlagGlobal == PRS_PARA_UNDEFINED ) {
        *puiFieldID = PRS_PARA_FIELD_TEXT_ID;
    }

    /* This is the first line of a new paragraph so we copy it for our document title */
    else if ( uiParaTitleFlagGlobal == PRS_PARA_FIRST_LINE ) {
        *puiFieldID = PRS_PARA_FIELD_TITLE_ID;
        s_wcsnncpy(pwcParaTitleGlobal, pwcLine, SPI_TITLE_MAXIMUM_LENGTH + 1);
        uiParaTitleFlagGlobal = PRS_PARA_UNDEFINED;
    }

    /* This is a new paragraph so we need to get the next line for our document title */
    else if ( uiParaTitleFlagGlobal == PRS_PARA_NEW_DOCUMENT ) {
        uiParaTitleFlagGlobal = PRS_PARA_FIRST_LINE;
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsParaDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcParaTitleGlobal

    Returns:    void

*/
void vPrsParaDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsParaDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsParaDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsParaDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsParaDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsParaDocumentInformationFunction'."); 
    }


    /* Create the document title */
    s_wcsnncpy(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcParaTitleGlobal) == false) ? pwcParaTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH + 1);

    /* Clear the globals */
    pwcParaTitleGlobal[0] = L'\0';


    return;

}


/*---------------------------------------------------------------------------*/


/* 
** ======================
** ===  Line Format  ====
** ======================
*/


/* 
** Line format:
**
** One document per file, first line is title, all subsequent lines are terms.
**

title text
term
...

**
*/


/* Field IDs */
#define PRS_LINE_FIELD_INVALID_ID       (0)
#define PRS_LINE_FIELD_TITLE_ID         (1)
#define PRS_LINE_FIELD_TEXT_ID          (2)


/* Field descriptions */
static struct prsFieldNameDescription pfndLineFieldNameDescriptionGlobal[] = 
{
    {PRS_LINE_FIELD_TITLE_ID,       L"title",   NULL,   NULL,   L"Title",   false},
    {PRS_LINE_FIELD_TEXT_ID,        L"text",    NULL,   NULL,   L"Text",    false},
    {PRS_LINE_FIELD_INVALID_ID,     NULL,       NULL,   NULL,   NULL,       false},
};


/* Globals */
static wchar_t pwcLineTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH +1] = {L'\0'};


/*

    Function:   bPrsLineFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsLineFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsLineFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsLineFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsLineFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsLineFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsLineFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndLineFieldNameDescriptionGlobal) / sizeof(struct prsFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pfndLineFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pfndLineFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pfndLineFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pfndLineFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pfndLineFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pfndLineFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsLineDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 
                
                Here we pick out the first line as the document title.

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcLineTitleGlobal

    Returns:    void

*/
void vPrsLineDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsLineDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsLineDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsLineDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsLineDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsLineDocumentLineFunction'."); 
    }
        
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsLineDocumentLineFunction'."); 
    }
    

    /* Empty title means we need to set it */
    if ( bUtlStringsIsWideStringNULL(pwcLineTitleGlobal) == true ) {
        
        /* Set the title */
        s_wcsnncpy(pwcLineTitleGlobal, pwcLine, SPI_TITLE_MAXIMUM_LENGTH + 1);

        /* Set the field ID to the title */
        *puiFieldID = PRS_LINE_FIELD_TITLE_ID;

        /* Each line is a term */
        *pbIndexLine = true;
        *pbParseLine = false;
        *pbTermPositions = true;
    }
    else {

        /* Set the field ID to the text */
        *puiFieldID = PRS_LINE_FIELD_TEXT_ID;

        /* Each line is a term */
        *pbIndexLine = true;
        *pbParseLine = false;
        *pbTermPositions = true;
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsLineDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcLineTitleGlobal

    Returns:    void

*/
void vPrsLineDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsLineDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsLineDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsLineDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsLineDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsLineDocumentInformationFunction'."); 
    }


    /* Set the document title */
    s_wcsnncpy(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcLineTitleGlobal) == false) ? pwcLineTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH + 1);

    /* Clear the globals */
    pwcLineTitleGlobal[0] = L'\0';


    return;

}


/*---------------------------------------------------------------------------*/


/* 
** =====================
** ===  Mbox format  ===
** =====================
*/


/* Mbox/mail/mail digest format 
** TBD:
**
** - Has not been tested in a while
** - Add more header fields
** - Suppress all unknown headers
**
*/


/* Field IDs */
#define PRS_MBOX_FIELD_INVALID_ID       (0)
#define PRS_MBOX_FIELD_SUBJECT_ID       (1)
#define PRS_MBOX_FIELD_FROM_ID          (2)
#define PRS_MBOX_FIELD_TO_ID            (3)
#define PRS_MBOX_FIELD_DATE_ID          (4)
#define PRS_MBOX_FIELD_BODY_ID          (5)


/* Field descriptions */
static struct prsFieldNameDescription pfndMboxFieldNameDescriptionGlobal[] = 
{
    {PRS_MBOX_FIELD_SUBJECT_ID,     L"subject",     NULL,   NULL,   L"Subject",     false},
    {PRS_MBOX_FIELD_FROM_ID,        L"from",        NULL,   NULL,   L"From",        false},
    {PRS_MBOX_FIELD_TO_ID,          L"to",          NULL,   NULL,   L"To",          false},
    {PRS_MBOX_FIELD_DATE_ID,        L"date",        NULL,   NULL,   L"Date",        false},
    {PRS_MBOX_FIELD_BODY_ID,        L"body",        NULL,   NULL,   L"Body",        false},
    {PRS_MBOX_FIELD_INVALID_ID,     NULL,           NULL,   NULL,   NULL,           false},
};


/* Globals */
static wchar_t pwcMboxSubjectGlobal[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcMboxFromGlobal[PRS_SHORT_STRING_LENGTH + 1] = {L'\0'};
static wchar_t pwcMboxToGlobal[PRS_SHORT_STRING_LENGTH + 1] = {L'\0'};
static wchar_t pwcMboxDateGlobal[PRS_SHORT_STRING_LENGTH + 1] = {L'\0'};
static unsigned long ulMboxAnsiDateGlobal = 0;


/*

    Function:   bPrsMboxDocumentStartFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the start of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsMboxDocumentStartFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsMboxDocumentStartFunction'"); 
    }


    /* Handle mbox and eudora mailbox format */
    if ( (s_wcsncmp(pwcLine, L"From ", /* s_wcslen(L"From ") */ 5) == 0) || (s_wcsncmp(pwcLine, L"From ???@???", /* s_wcslen(L"From ???@???") */ 12) == 0) ) {
        return (true);
    }

    /* Handle mail digest format */
    if ( s_wcsncmp(pwcLine, L"-----------------------------", /* s_wcslen(L"-----------------------------") */ 29) == 0 ) {
        return (true);
    }

    /* Handle netnews format */
    if ( (s_wcsncmp(pwcLine, L"From ", /* s_wcslen(L"From ") */ 5) == 0) || (s_wcsncmp(pwcLine, L"Path: ", /* s_wcslen(L"Path: ") */ 6) == 0) || 
            (s_wcsncmp(pwcLine, L"Article ", /* s_wcslen(L"Article ") */ 8) == 0) || (s_wcsncmp(pwcLine, L"Article: ", /* s_wcslen(L"Article: ") */ 9) == 0) ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsMboxFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with                
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsMboxFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsMboxFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsMboxFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsMboxFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsMboxFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsMboxFieldInformationFunction'."); 
    }

    
    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndMboxFieldNameDescriptionGlobal) / sizeof(struct prsFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pfndMboxFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pfndMboxFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pfndMboxFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pfndMboxFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pfndMboxFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pfndMboxFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsMboxDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcMboxSubjectGlobal, pwcMboxFromGlobal, pwcMboxToGlobal, 
                        pwcMboxDateGlobal, ulMboxAnsiDateGlobal

    Returns:    void

*/
void vPrsMboxDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    wchar_t     *pwcLinePtr = pwcLine;


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsMboxDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsMboxDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsMboxDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsMboxDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsMboxDocumentLineFunction'."); 
    }
    
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsMboxDocumentLineFunction'."); 
    }


    /* Field defaults to Body */
    *puiFieldID = PRS_MBOX_FIELD_BODY_ID;
    
    /* Check for a Subject */
    if ( (s_wcsncmp(pwcLine, L"Subject: ", /* s_wcslen(L"Subject: ") */ 9) == 0) && (bUtlStringsIsWideStringNULL(pwcMboxSubjectGlobal) == true) ) {
        s_wcsnncpy(pwcMboxSubjectGlobal, pwcLine + /* s_wcslen(L"Subject: ") */ 8, SPI_TITLE_MAXIMUM_LENGTH + 1);
        *puiFieldID = PRS_MBOX_FIELD_SUBJECT_ID;
    }
    
    /* Check for a From */
    else if ( (s_wcsncmp(pwcLine, L"From: ", /* s_wcslen(L"From: ") */ 6) == 0) && (bUtlStringsIsWideStringNULL(pwcMboxFromGlobal) == true) ) {
        
        /* Handle 'From: Francois Schiettecatte <francois@fsconsult.com>' */
        if ( (pwcLinePtr = s_wcschr(pwcLine + /* s_wcslen(L"From: ") */ 6, L'<')) != NULL ) {
            s_wcsnncpy(pwcMboxFromGlobal, pwcLine + /* s_wcslen(L"From: ") */ 6, PRS_SHORT_STRING_LENGTH + 1);
            pwcMboxFromGlobal[pwcLinePtr - (pwcLine + /* s_wcslen(L"From: ") */ 6) - 1] = L'\0';
        }
        /* Handle 'From: francois@fsconsult.com (Francois Schiettecatte)' */
        else if ( (pwcLinePtr = s_wcschr(pwcLine + /* s_wcslen(L"From: ") */ 6, L'(')) != NULL ) {

            s_wcsnncpy(pwcMboxFromGlobal, pwcLinePtr + 1, PRS_SHORT_STRING_LENGTH + 1);

            if ( (pwcLinePtr = s_wcschr(pwcMboxFromGlobal, L')')) != NULL ) {
                *(pwcLinePtr) = L'\0';
            }
        }
        /* Handle everything else */
        else {
            s_wcsnncpy(pwcMboxFromGlobal, pwcLine + /* s_wcslen(L"From: ") */ 6, PRS_SHORT_STRING_LENGTH + 1);
        }

        *puiFieldID = PRS_MBOX_FIELD_FROM_ID;
    }
    
    /* Check for To */
    else if ( (s_wcsncmp(pwcLine, L"To: ", /* s_wcslen(L"To: ") */ 4) == 0) && (bUtlStringsIsWideStringNULL(pwcMboxToGlobal) == true) ) {
        
        /* Handle 'To: Francois Schiettecatte <francois@fsconsult.com>' */
        if ( (pwcLinePtr = s_wcschr(pwcLine + /* s_wcslen(L"To: ") */ 4, L'<')) != NULL ) {
            s_wcsnncpy(pwcMboxToGlobal, pwcLine + /* s_wcslen(L"To: ") */ 4, PRS_SHORT_STRING_LENGTH + 1);
            pwcMboxToGlobal[pwcLinePtr - (pwcLine + /* s_wcslen(L"To: ") */ 4) - 1] = L'\0';
        }
        /* Handle 'To: francois@fsconsult.com (Francois Schiettecatte)' */
        else if ( (pwcLinePtr = s_wcschr(pwcLine + /* s_wcslen(L"To: ") */ 4, L'(')) != NULL ) {

            s_wcsnncpy(pwcMboxToGlobal, pwcLinePtr + 1, PRS_SHORT_STRING_LENGTH + 1);

            if ( (pwcLinePtr = s_wcschr(pwcMboxToGlobal, L')')) != NULL ) {
                *(pwcLinePtr) = L'\0';
            }
        }
        /* Handle everything else */
        else {
            s_wcsnncpy(pwcMboxToGlobal, pwcLine + /* s_wcslen(L"To: ") */ 4, PRS_SHORT_STRING_LENGTH + 1);
        }
        
        *puiFieldID = PRS_MBOX_FIELD_TO_ID;
    }
    
    /* Check for Date */
    else if ( (s_wcsncmp(pwcLine, L"Date: ", /* s_wcslen(L"Date: ") */ 6) == 0) && (bUtlStringsIsWideStringNULL(pwcMboxDateGlobal) == true) ) {

        s_wcsnncpy(pwcMboxDateGlobal, pwcLine + /* s_wcslen(L"Date: ") */ 6, PRS_SHORT_STRING_LENGTH + 1);
        
        /* Parse out the date, set to 0 on error */
        if ( iUtlDateParseWideDateToAnsiDate(pwcLine + 6, &ulMboxAnsiDateGlobal) != UTL_NoError ) {
            ulMboxAnsiDateGlobal = 0;
        }
        
        *puiFieldID = PRS_MBOX_FIELD_DATE_ID;
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsMboxDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcMboxSubjectGlobal, pwcMboxFromGlobal, pwcMboxToGlobal, 
                        pwcMboxDateGlobal, ulMboxAnsiDateGlobal

    Returns:    void

*/
void vPrsMboxDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsMboxDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsMboxDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsMboxDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsMboxDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsMboxDocumentInformationFunction'."); 
    }


    /* Add the "From:" */
    if ( bUtlStringsIsWideStringNULL(pwcMboxFromGlobal) == false ) {

        if ( bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false ) {
            s_wcsnncat(pwcDocumentTitle, L", ", SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }

        iUtlStringsTrimWideString(pwcMboxFromGlobal);
        s_wcsnncat(pwcDocumentTitle, pwcMboxFromGlobal, SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }
    

    /* Add the "To:" */
    if ( bUtlStringsIsWideStringNULL(pwcMboxToGlobal) == false ) {

        if ( bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false ) {
            s_wcsnncat(pwcDocumentTitle, L", ", SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }

        iUtlStringsTrimWideString(pwcMboxToGlobal);
        s_wcsnncat(pwcDocumentTitle, pwcMboxToGlobal, SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }


    /* Add the "Date:" */
    if ( bUtlStringsIsWideStringNULL(pwcMboxDateGlobal) == false ) {

        if ( bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false ) {
            s_wcsnncat(pwcDocumentTitle, L", ", SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }

        iUtlStringsTrimWideString(pwcMboxDateGlobal);
        s_wcsnncat(pwcDocumentTitle, pwcMboxDateGlobal, SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }
    

    /* Add the "Subject:" */
    if ( bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false ) {
        s_wcsnncat(pwcDocumentTitle, L", ", SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }
    
    iUtlStringsTrimWideString(pwcMboxSubjectGlobal);
    s_wcsnncat(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcMboxSubjectGlobal) == false) ? pwcMboxSubjectGlobal : L"(no subject)"), SPI_TITLE_MAXIMUM_LENGTH + 1, SPI_TITLE_MAXIMUM_LENGTH + 1);


    /* Set the date and time return parameters */
    *pulDocumentAnsiDate = ulMboxAnsiDateGlobal;


    /* Clear the globals */
    pwcMboxFromGlobal[0] = L'\0';
    pwcMboxToGlobal[0] = L'\0';
    pwcMboxDateGlobal[0] = L'\0';
    pwcMboxSubjectGlobal[0] = L'\0';
    ulMboxAnsiDateGlobal = 0;


    return;

}


/*---------------------------------------------------------------------------*/


/* 
** ========================
** ====  Refer Format  ====
** ========================
*/


/* Refer citation format:
**
** See this link for format description:
**
**    http://www.hcibib.org/refer.html
** 
** Documents are separated by a blank line
**

%T title text
%A author text
%D date text
...

%T title text
%A author text
%D date text
...

** 
*/

/* Maximum author name length */
#define PRS_HCI_BIB_AUTHOR_LENGTH_MAXIMUM       (60)


/* Field IDs */
#define PRS_HCIBIB_FIELD_INVALID_ID             (0)
#define PRS_HCIBIB_FIELD_AUTHOR_ID              (1)     /* %A */
#define PRS_HCIBIB_FIELD_BOOK_ID                (2)     /* %B */
#define PRS_HCIBIB_FIELD_CITY_ID                (3)     /* %C */
#define PRS_HCIBIB_FIELD_DATE_ID                (4)     /* %D */
#define PRS_HCIBIB_FIELD_EDITOR_ID              (5)     /* %E */
#define PRS_HCIBIB_FIELD_FOOTNOTE_ID            (6)     /* %F */
#define PRS_HCIBIB_FIELD_GOVT_ID                (7)     /* %G */
#define PRS_HCIBIB_FIELD_COMMENTARY_ID          (8)     /* %H */
#define PRS_HCIBIB_FIELD_PUBLISHER_ID           (9)     /* %I */
#define PRS_HCIBIB_FIELD_JOURNAL_ID             (10)    /* %J */
#define PRS_HCIBIB_FIELD_KEYWORD_ID             (11)    /* %K */
#define PRS_HCIBIB_FIELD_LABEL_ID               (12)    /* %L */
#define PRS_HCIBIB_FIELD_BELL_ID                (13)    /* %M */
#define PRS_HCIBIB_FIELD_ISSUE_ID               (14)    /* %N */
#define PRS_HCIBIB_FIELD_OTHER_ID               (15)    /* %O */
#define PRS_HCIBIB_FIELD_PAGE_ID                (16)    /* %P */
#define PRS_HCIBIB_FIELD_CORPORATE_ID           (17)    /* %Q */
#define PRS_HCIBIB_FIELD_REPORT_ID              (18)    /* %R */
#define PRS_HCIBIB_FIELD_SERIES_ID              (19)    /* %S */
#define PRS_HCIBIB_FIELD_TITLE_ID               (20)    /* %T */
#define PRS_HCIBIB_FIELD_ANNOTATION_ID          (21)    /* %U */
#define PRS_HCIBIB_FIELD_VOLUME_ID              (22)    /* %V */
#define PRS_HCIBIB_FIELD_WEBLINK_ID             (23)    /* %W */
#define PRS_HCIBIB_FIELD_ABSTRACT_ID            (24)    /* %X */
#define PRS_HCIBIB_FIELD_TOC_ID                 (25)    /* %Y */
#define PRS_HCIBIB_FIELD_REFERENCE_ID           (26)    /* %Z */
#define PRS_HCIBIB_FIELD_PRICE_ID               (27)    /* %$ */
#define PRS_HCIBIB_FIELD_COPYRIGHT_ID           (28)    /* %* */
#define PRS_HCIBIB_FIELD_PARTS_ID               (29)    /* %^ */


/* Field descriptions */
struct prsFieldNameDescription pfndReferFieldNameDescriptionGlobal[] = 
{
    {PRS_HCIBIB_FIELD_AUTHOR_ID,        L"author",      NULL,   NULL,   L"Author's name",                           true},
    {PRS_HCIBIB_FIELD_BOOK_ID,          L"book",        NULL,   NULL,   L"Book containing article referenced",      false},
    {PRS_HCIBIB_FIELD_CITY_ID,          L"city",        NULL,   NULL,   L"City (place of publication)",             false},
    {PRS_HCIBIB_FIELD_DATE_ID,          L"date",        NULL,   NULL,   L"Date of publication",                     false},
    {PRS_HCIBIB_FIELD_EDITOR_ID,        L"editor",      NULL,   NULL,   L"Editors",                                 false},
    {PRS_HCIBIB_FIELD_FOOTNOTE_ID,      L"footnote",    NULL,   NULL,   L"Footnote number or label (computed)",     false},
    {PRS_HCIBIB_FIELD_GOVT_ID,          L"govt",        NULL,   NULL,   L"Government order number",                 false},
    {PRS_HCIBIB_FIELD_COMMENTARY_ID,    L"commentary",  NULL,   NULL,   L"Title commentary, printed before ref.",   false},
    {PRS_HCIBIB_FIELD_PUBLISHER_ID,     L"publisher",   NULL,   NULL,   L"Issuer (publisher)",                      false},
    {PRS_HCIBIB_FIELD_JOURNAL_ID,       L"journal",     NULL,   NULL,   L"Journal containing article",              false},
    {PRS_HCIBIB_FIELD_KEYWORD_ID,       L"keyword",     NULL,   NULL,   L"Keywords to help search for references",  false},
    {PRS_HCIBIB_FIELD_LABEL_ID,         L"label",       NULL,   NULL,   L"Label field used by -k option of refer",  false},
    {PRS_HCIBIB_FIELD_BELL_ID,          L"bell",        NULL,   NULL,   L"Bell Labs Memorandum (undefined)",        false},
    {PRS_HCIBIB_FIELD_ISSUE_ID,         L"issue",       NULL,   NULL,   L"Number of issue within volume",           false},
    {PRS_HCIBIB_FIELD_OTHER_ID,         L"other",       NULL,   NULL,   L"Other commentary, printed after ref.",    false},
    {PRS_HCIBIB_FIELD_PAGE_ID,          L"page",        NULL,   NULL,   L"Page number(s)",                          false},
    {PRS_HCIBIB_FIELD_CORPORATE_ID,     L"corporate",   NULL,   NULL,   L"Corporate, Foreign Author",               false},
    {PRS_HCIBIB_FIELD_REPORT_ID,        L"report",      NULL,   NULL,   L"Report, paper, thesis (unpublished)",     false},
    {PRS_HCIBIB_FIELD_SERIES_ID,        L"series",      NULL,   NULL,   L"Series title",                            false},
    {PRS_HCIBIB_FIELD_TITLE_ID,         L"title",       NULL,   NULL,   L"Title of article or book",                true},
    {PRS_HCIBIB_FIELD_ANNOTATION_ID,    L"annotation",  NULL,   NULL,   L"User annotations, stored separately",     false},
    {PRS_HCIBIB_FIELD_VOLUME_ID,        L"volume",      NULL,   NULL,   L"Volume number",                           false},
    {PRS_HCIBIB_FIELD_WEBLINK_ID,       L"weblink",     NULL,   NULL,   L"Weblink",                                 false},            
    {PRS_HCIBIB_FIELD_ABSTRACT_ID,      L"abstract",    NULL,   NULL,   L"Abstract",                                true},
    {PRS_HCIBIB_FIELD_TOC_ID,           L"toc",         NULL,   NULL,   L"Table of Contents",                       false},
    {PRS_HCIBIB_FIELD_REFERENCE_ID,     L"reference",   NULL,   NULL,   L"References",                              false},
    {PRS_HCIBIB_FIELD_PRICE_ID,         L"price",       NULL,   NULL,   L"Purchase Price",                          false},
    {PRS_HCIBIB_FIELD_COPYRIGHT_ID,     L"copyright",   NULL,   NULL,   L"Copyright Notice",                        false},
    {PRS_HCIBIB_FIELD_PARTS_ID,         L"parts",       NULL,   NULL,   L"Contained Parts",                         false},
    {PRS_HCIBIB_FIELD_INVALID_ID,       NULL,           NULL,   NULL,   NULL,                                       false},
};


/* Globals */
static wchar_t pwcReferTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcReferUrlGlobal[SPI_URL_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcReferDateGlobal[PRS_MAX_YEAR_LENGTH + 1] = {L'\0'};
static wchar_t pwcReferAuthorGlobal[PRS_HCI_BIB_AUTHOR_LENGTH_MAXIMUM + 1] = {L'\0'};
static unsigned int uiReferFieldIDGlobal = PRS_HCIBIB_FIELD_INVALID_ID;
static unsigned long ulReferAnsiDateGlobal = 0;
static boolean bReferTitleDefined = false;
static boolean bReferBookDefined = false;


/*

    Function:   bPrsReferDocumentEndFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the end of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsReferDocumentEndFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsReferDocumentEndFunction'."); 
    }


    if ( (pwcLine[0] == L'\0') || (pwcLine[1] == L'\0') ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsReferFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsReferFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsReferFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsReferFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsReferFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsReferFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsReferFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndReferFieldNameDescriptionGlobal) / sizeof(struct prsFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pfndReferFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pfndReferFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pfndReferFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pfndReferFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pfndReferFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pfndReferFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsReferDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 

                Here we pick out the '%T ', the '%B ', the '%D ', the '%A ',
                and the '%E ' fields and place them in the appropriate globals.
                Note that the '%T ' and '%B ' fields are alternate fields, as
                are the '%A ' and the '%E ' fields.

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcReferTitleGlobal, pwcReferUrlGlobal, pwcReferDateGlobal, 
                        pwcReferAuthorGlobal, uiReferFieldIDGlobal, ulReferAnsiDateGlobal

    Returns:    void

*/
void vPrsReferDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    wchar_t     *pwcDataPtr = pwcLine + 3;
        


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsReferDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsReferDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsReferDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsReferDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsReferDocumentLineFunction'."); 
    }
        
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsReferDocumentLineFunction'."); 
    }

    
    /* Update the field ID global if this is a new field, as indicated by the field header '%' */
    if ( pwcLine[0] == L'%' ) {

        if ( (pwcLine[1] != L'\0') && (pwcLine[1] >= L'A') && (pwcLine[1] <= L'Z') ) {
            uiReferFieldIDGlobal = (unsigned int)(pwcLine[1] - '@');
        }
        else if ( pwcLine[1] == L'$' ) {
            uiReferFieldIDGlobal = PRS_HCIBIB_FIELD_PRICE_ID;
        }
        else if ( pwcLine[1] == L'*' ) {
            uiReferFieldIDGlobal = PRS_HCIBIB_FIELD_COPYRIGHT_ID;
        }
        else if ( pwcLine[1] == L'^' ) {
            uiReferFieldIDGlobal = PRS_HCIBIB_FIELD_PARTS_ID;
        }
    }


    /* Grab fields we need */
    switch ( uiReferFieldIDGlobal ) {
    
        /* Grab the document title */
        case PRS_HCIBIB_FIELD_TITLE_ID:
        
            /* Toss out the 'B' title is it has been defined, we prefer the 'T' title */
            if ( bReferBookDefined == true ) {
                pwcReferTitleGlobal[0] = L'\0';
            }
            
            if ( bUtlStringsIsWideStringNULL(pwcReferTitleGlobal) == true ) {
                s_wcsnncpy(pwcReferTitleGlobal, pwcDataPtr, SPI_TITLE_MAXIMUM_LENGTH + 1);
                bReferTitleDefined = true;
            }
            else {
                s_wcsnncat(pwcReferTitleGlobal, L" ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
                s_wcsnncat(pwcReferTitleGlobal, pwcLine, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
            }

            break;
        
        
        /* Grab the document title */
        case PRS_HCIBIB_FIELD_BOOK_ID:
        
            /* Use the 'B' title only if the 'T' title was not defined */
            if ( bReferTitleDefined == false ) {
                if ( bUtlStringsIsWideStringNULL(pwcReferTitleGlobal) == true ) {
                    s_wcsnncpy(pwcReferTitleGlobal, pwcDataPtr, SPI_TITLE_MAXIMUM_LENGTH + 1);
                    bReferBookDefined = true;
                }
                else {
                    s_wcsnncat(pwcReferTitleGlobal, L" ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
                    s_wcsnncat(pwcReferTitleGlobal, pwcDataPtr, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
                }
            }

            break;
        
        
        /* Grab the date */
        case PRS_HCIBIB_FIELD_DATE_ID:
        
            /* Parse the date, set to 0 on error */
            if ( iUtlDateParseWideDateToAnsiDate(pwcDataPtr, &ulReferAnsiDateGlobal) != UTL_NoError ) {
                ulReferAnsiDateGlobal = 0;
            }

            s_wcsnncpy(pwcReferDateGlobal, pwcDataPtr, PRS_MAX_YEAR_LENGTH + 1);
    
            break;
    
    
        /* Grab the author */
        case PRS_HCIBIB_FIELD_AUTHOR_ID:

            if ( bUtlStringsIsWideStringNULL(pwcReferAuthorGlobal) == true ) {
                s_wcsnncpy(pwcReferAuthorGlobal, pwcDataPtr, PRS_HCI_BIB_AUTHOR_LENGTH_MAXIMUM + 1);
            }
            
            break;
            

        /* Don't parse the identifier */
        case PRS_HCIBIB_FIELD_BELL_ID:

            /* Don't parse the identifier */
            *pbParseLine = false;
            break;
            

        /* Truncate the weblink if it is a URL */
        case PRS_HCIBIB_FIELD_WEBLINK_ID:

            /* Check for a URL */
            if ( (s_wcsncmp(pwcDataPtr, L"http://", 7) == 0) || (s_wcsncmp(pwcDataPtr, L"http://", 7) == 0) ) {
    
                wchar_t     *pwcPtr = NULL;
                
                /* Replace all spaces with a '+' sign */
                while ( (pwcPtr = s_wcschr(pwcDataPtr, L' ')) != NULL ) {
                    *pwcPtr = L'+';
                }
    
                /* Copy the url */
                s_wcsnncpy(pwcReferUrlGlobal, pwcDataPtr, SPI_URL_MAXIMUM_LENGTH + 1);
    
                /* And don't parse it */
                *pbParseLine = false;
            }
            
            break;

    }


#if defined(NOTUSED)
    /* Normalize the author name, format for author names is:
    **
    **    FirstName MiddleNames/MiddleInitials LastNames
    **
    ** We want to turn this into:
    **
    **    LastName_Initials
    */
    if ( (uiReferFieldIDGlobal == PRS_HCIBIB_FIELD_AUTHOR_ID) || (uiReferFieldIDGlobal == PRS_HCIBIB_FIELD_EDITOR_ID) ) { 
        
        wchar_t     *pwcAuthorNamePtr = NULL;
        wchar_t     pwcAuthorName[PRS_HCI_BIB_AUTHOR_LENGTH_MAXIMUM + 1] = {L'\0'};
        wchar_t     *pwcReadPtr = NULL;
        wchar_t     *pwcWritePtr = NULL;
        boolean     bNewInitialFlag = true;

        /* Get a pointer to the space before the start of the author's last name, note 
        ** that s_wcsrchr() will return NULL if there is only one word in the author name
        */
        if ( (pwcAuthorNamePtr = s_wcsrchr(pwcDataPtr, L' ')) != NULL) {
            
            /* Point to the author name past the space */
            pwcAuthorNamePtr++;

            /* Copy the author name to a temporary buffer, get the write pointer to 
            ** that temporary buffer and append an underscore at the end of the name,
            ** making sure we update the write pointer
            */
            s_wcsnncpy(pwcAuthorName, pwcAuthorNamePtr, PRS_HCI_BIB_AUTHOR_LENGTH_MAXIMUM +1);
            pwcWritePtr = pwcAuthorName + s_wcslen(pwcAuthorName);
            *pwcWritePtr++ = L'_';
            
            /* Loop over the first name and middle names/initials, extracting the initials only
            ** and tacking them on to the end of the name, making sure we add an underscore 
            */
            for ( pwcReadPtr = pwcDataPtr, bNewInitialFlag = true; pwcReadPtr < pwcAuthorNamePtr; pwcReadPtr++  ) {
                
                /* Copy the first initial over, we control that through the flag */
                if ( (iswalpha(*pwcReadPtr) != 0) && (bNewInitialFlag == true) ) {
                    *pwcWritePtr++ = (*pwcReadPtr >= L'a') ? (*pwcReadPtr - 32) : *pwcReadPtr;
                    bNewInitialFlag = false;
                }
                else if ( isalpha(*pwcReadPtr) == 0 ) {
                    bNewInitialFlag = true;
                }
            }
            
            /* NULL terminate the author name and copy it back to the line */
            *pwcWritePtr = L'\0';
            s_wcsnncpy(pwcDataPtr, pwcAuthorName, PRS_HCI_BIB_AUTHOR_LENGTH_MAXIMUM +1);
        }

        *pbParseLine = false;
    }
#endif    /* defined(NOTUSED) */


    /* Filter out field header and field tag */
    if ( pwcLine[0] == L'%'  ) {
        pwcLine[0] = L' ';
        pwcLine[1] = L' ';
    }


    /* Set the field ID return pointer */
    *puiFieldID = uiReferFieldIDGlobal;


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsReferDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcReferTitleGlobal, pwcReferUrlGlobal, pwcReferDateGlobal, 
                        pwcReferAuthorGlobal, uiReferFieldIDGlobal, ulReferAnsiDateGlobal

    Returns:    void

*/
void vPrsReferDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsReferDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsReferDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsReferDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsReferDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsReferDocumentInformationFunction'."); 
    }


    /* Add the author */
    if ( bUtlStringsIsWideStringNULL(pwcReferAuthorGlobal) == false ) {
        s_wcsnncat(pwcDocumentTitle, pwcReferAuthorGlobal, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }

    /* Add the date */
    if ( bUtlStringsIsWideStringNULL(pwcReferDateGlobal) == false ) {
        s_wcsnncat(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false) ? L" (" : L"("), SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        s_wcsnncat(pwcDocumentTitle, pwcReferDateGlobal, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        s_wcsnncat(pwcDocumentTitle, L") ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }
    else {
        if ( bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false ) {
            s_wcsnncat(pwcDocumentTitle, L"(no date)", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }
    }

    /* Set the document title */
    s_wcsnncat(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcReferTitleGlobal) == false) ? 
            pwcReferTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);

    /* Set the url */
    if ( bUtlStringsIsWideStringNULL(pwcReferUrlGlobal) == false ) {
        s_wcsnncpy(pwcDocumentUrl, pwcReferUrlGlobal, SPI_URL_MAXIMUM_LENGTH + 1);
    }

    /* Set the date */
    *pulDocumentAnsiDate = ulReferAnsiDateGlobal;



    /* Clear the globals */
    pwcReferTitleGlobal[0] = L'\0';
    pwcReferUrlGlobal[0] = L'\0';
    pwcReferDateGlobal[0] = L'\0';
    pwcReferAuthorGlobal[0] = L'\0';
    uiReferFieldIDGlobal = PRS_HCIBIB_FIELD_INVALID_ID;
    ulReferAnsiDateGlobal = 0;
    bReferTitleDefined = false;
    bReferBookDefined = false;

    
    return;

}


/*---------------------------------------------------------------------------*/


/* 
** ===============================
** ====  MPS Text/Xml Format  ====
** ===============================
*/


/* Field IDs */
#define PRS_MPS_FIELD_INVALID_ID        (0)
#define PRS_MPS_FIELD_TITLE_ID          (1)
#define PRS_MPS_FIELD_TEXT_ID           (2)
#define PRS_MPS_FIELD_URL_ID            (3)


/* Field descriptions */
struct prsFieldNameDescription pfndMpsTextFieldNameDescriptionGlobal[] = 
{
    {PRS_MPS_FIELD_TITLE_ID,    L"title",   (unsigned char*)"CHAR",     NULL,                                                        L"Title",  false},
    {PRS_MPS_FIELD_TEXT_ID,     L"text",    (unsigned char*)"CHAR",     NULL,                                                        L"Text",   false},
    {PRS_MPS_FIELD_URL_ID,      L"url",     (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",   L"Url",    false},
    {PRS_MPS_FIELD_INVALID_ID,  NULL,       NULL,                       NULL,                                                        NULL,      false},
};


/* Globals */
static wchar_t pwcMpsTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcMpsKeyGlobal[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcMpsUrlGlobal[SPI_URL_MAXIMUM_LENGTH + 1] = {L'\0'};
static unsigned int uiMpsLanguageIDGlobal = LNG_LANGUAGE_ANY_ID;
static unsigned int uiMpsRankGlobal = 0;
static unsigned long ulMpsAnsiDateGlobal = 0;


/*

    Function:   bPrsMpsFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsMpsFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsMpsFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsMpsFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsMpsFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsMpsFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsMpsFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndMpsTextFieldNameDescriptionGlobal) / sizeof(struct prsFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pfndMpsTextFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pfndMpsTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pfndMpsTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pfndMpsTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pfndMpsTextFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pfndMpsTextFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsMpsDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcMpsTitleGlobal, pwcMpsKeyGlobal, pwcMpsUrlGlobal, 
                        uiMpsRankGlobal, ulMpsAnsiDateGlobal

    Returns:    void

*/
void vPrsMpsDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsMpsDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsMpsDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsMpsDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsMpsDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsMpsDocumentInformationFunction'."); 
    }


    /* Set the document title */
    s_wcsnncpy(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcMpsTitleGlobal) == false) ? 
            pwcMpsTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH + 1);

    /* Set the document key */
    if ( bUtlStringsIsWideStringNULL(pwcMpsKeyGlobal) == false ) {
        s_wcsnncpy(pwcDocumentKey, pwcMpsKeyGlobal, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
    }

    /* Set the url */
    if ( bUtlStringsIsWideStringNULL(pwcMpsUrlGlobal) == false ) {
        s_wcsnncpy(pwcMpsUrlGlobal, pwcMpsUrlGlobal, SPI_URL_MAXIMUM_LENGTH + 1);
    }

    /* Set the rank */
    *puiDocumentRank = uiMpsRankGlobal;

    /* Set the date */
    *pulDocumentAnsiDate = ulMpsAnsiDateGlobal;



    /* Clear the globals */
    pwcMpsTitleGlobal[0] = L'\0';
    pwcMpsKeyGlobal[0] = L'\0';
    pwcMpsUrlGlobal[0] = L'\0';
    uiMpsLanguageIDGlobal = LNG_LANGUAGE_ANY_ID;
    uiMpsRankGlobal = 0;
    ulMpsAnsiDateGlobal = 0;

    
    return;

}


/*---------------------------------------------------------------------------*/


/* 
** ===========================
** ====  MPS Text Format  ====
** ===========================
*/


/* MPS text format:
**
** Documents are separated by a blank line.
**

%K key
%L language
%R rank
%D date (ansi format)
%U url
%T title
%X text

%K key
%L language
%R rank
%D date (ansi format)
%U url
%T title
%X text

** 
*/

/*

    Function:   bPrsMpsTextDocumentEndFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the end of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsMpsTextDocumentEndFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsMpsTextDocumentEndFunction'."); 
    }


    if ( (pwcLine[0] == L'\0') || (pwcLine[1] == L'\0') ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsMpsTextDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcMpsTitleGlobal, pwcMpsKeyGlobal, pwcMpsUrlGlobal, 
                        uiMpsRankGlobal, ulMpsAnsiDateGlobal

    Returns:    void

*/
void vPrsMpsTextDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    wchar_t     *pwcDataPtr = pwcLine + 3;
    wchar_t     wcFieldTag = L'\0';


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsMpsTextDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsMpsTextDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsMpsTextDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsMpsTextDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsMpsTextDocumentLineFunction'."); 
    }
            
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsMpsTextDocumentLineFunction'."); 
    }



    /* Check that this a potentially valid field tag */
    if ( (pwcLine[0] != L'%') || (pwcLine[2] != L' ') ) {
    
        /* Don't index */
        *pbIndexLine = false;
        
        return; 
    }


    /* Set the field tag */
    wcFieldTag = pwcLine[1];


    /* Process the field */
    switch ( wcFieldTag ) {
    
        /* Key */
        case L'K':

            /* Save the key */
            s_wcsnncpy(pwcMpsKeyGlobal, pwcDataPtr, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
    
            /* Don't index the key */
            *pbIndexLine = false;
        
            break;


        /* Language */
        case L'L':
            
            {
                unsigned char    pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};
                unsigned int    uiDataLength = 0;
        
                /* Get the length of the string */
                uiDataLength = s_wcslen(pwcDataPtr);
        
                /* We don't need all the string to check that it is a language, only the first SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH character */
                uiDataLength = UTL_MACROS_MIN(uiDataLength, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1);
        
                /* Convert the string */    
                if ( iLngConvertWideStringToUtf8_s(pwcDataPtr, uiDataLength, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1)== LNG_NoError ) {
        
                    /* Replace some characters */
                    iUtlStringsReplaceCharacterInString(pucLanguageCode, '-', '_');
        
                    /* Get the language ID, default to LNG_LANGUAGE_ANY_ID on error */
                    if ( iLngGetLanguageIDFromCode(pucLanguageCode, &uiMpsLanguageIDGlobal) != LNG_NoError ) {
                        uiMpsLanguageIDGlobal = LNG_LANGUAGE_ANY_ID;
                    }
                }
        
                /* Don't index or parse the language */
                *pbIndexLine = false;
                *pbParseLine = false;
                *pbTermPositions = false;
            }
        
            break;


        /* Rank */
        case L'R':
            
            {
                /* Get the rank */
                int iMpsRank = (int)s_wcstol(pwcDataPtr, NULL, 10);
    
                /* Save the rank, min of 0 */
                uiMpsRankGlobal = UTL_MACROS_MAX(iMpsRank, 0);
                
                /* Don't index the rank */
                *pbIndexLine = false;
            }
    
            break;


        /* Date & Time */
        case L'D':
            
            {
                /* Get the date */
                long lAnsiDate = s_wcstol(pwcDataPtr, NULL, 10);
    
                /* Save the date, min of 0 */
                ulMpsAnsiDateGlobal = UTL_MACROS_MAX(lAnsiDate, 0);
    
                /* Don't index the date & time */
                *pbIndexLine = false;
            }
            
            break;


        /* Title */
        case L'T':
    
            /* Save the document title */
            s_wcsnncpy(pwcMpsTitleGlobal, pwcDataPtr, SPI_TITLE_MAXIMUM_LENGTH + 1);
    
            /* Set the language for the document title */
            *puiLanguageID = uiMpsLanguageIDGlobal;
            
            /* Set the field ID */
            *puiFieldID = PRS_MPS_FIELD_TITLE_ID;
    
            break;


        /* Text */
        case L'X':

            /* Set the language for the text */
            *puiLanguageID = uiMpsLanguageIDGlobal;
    
            /* Set the field ID */
            *puiFieldID = PRS_MPS_FIELD_TEXT_ID;
    
            break;


        /* Url */
        case L'U':

            {
                wchar_t     *pwcPtr = NULL;
                
                /* Replace all spaces with a '+' sign */
                while ( (pwcPtr = s_wcschr(pwcDataPtr, L' ')) != NULL ) {
                    *pwcPtr = L'+';
                }
    
                /* Copy the url */
                s_wcsnncpy(pwcMpsUrlGlobal, pwcDataPtr, SPI_URL_MAXIMUM_LENGTH + 1);

                /* Set the field ID */
                *puiFieldID = PRS_MPS_FIELD_URL_ID;
        
                /* Don't index the url */
/*                 *pbIndexLine = false; */

                /* Don't parse the url, it is a single term */
                *pbParseLine = false;
        
                /* Don't send term positions for the url */
                *pbTermPositions = false;
            }

            break;


        default:

/*             iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid field tag in line: '%ls'.", pwcLine);  */
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid field tag in line: '%ls'.", pwcLine); 
    }

    

    /* Filter out field tags */
    if ( pwcLine[0] == L'%'  ) {
        pwcLine[0] = L' ';
        pwcLine[1] = L' ';
    }


    return;

}


/*---------------------------------------------------------------------------*/

/* 
** ==========================
** ====  MPS XML Format  ====
** ==========================
*/


/* MPS XML format:
**
** Documents are separated by a blank line.
**

<document>
<key>...</key>
<language>...</language>
<rank>...</rank>
<date>...</date>
<url>...</url>
<title>...</title>
<text>...</text>
</document>

<document>
<key>...</key>
<language>...</language>
<rank>...</rank>
<date>...</date>
<url>...</url>
<title>...</title>
<text>
.
.
.
</text>
</document>

** 
*/


static unsigned int uiMpsXmlFieldIDGlobal = PRS_MPS_FIELD_INVALID_ID;


/*

    Function:   bPrsMpsXmlDocumentStartFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the start of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsMpsXmlDocumentStartFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsMpsXmlDocumentStartFunction'."); 
    }


    if ( (pwcLine[0] == L'<') && (s_wcsncmp(pwcLine, L"<document>", /* s_wcslen(L"<document>") */ 10) == 0) ) {
        return (true);
    }

    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsMpsXmlDocumentEndFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the end of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsMpsXmlDocumentEndFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsMpsXmlDocumentEndFunction'."); 
    }


    if ( (pwcLine[0] == L'<') && (s_wcsncmp(pwcLine, L"</document>", /* s_wcslen(L"</document>") */ 11) == 0) ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsMpsXmlDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcMpsTitleGlobal, pwcMpsKeyGlobal, pwcMpsUrlGlobal, 
                uiMpsRankGlobal, ulMpsAnsiDateGlobal

    Returns:    void

*/
void vPrsMpsXmlDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsMpsXmlDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsMpsXmlDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsMpsXmlDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsMpsXmlDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsMpsXmlDocumentLineFunction'."); 
    }
            
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsMpsXmlDocumentLineFunction'."); 
    }


    /* New tag */
    if ( pwcLine[0] == L'<' ) {

        wchar_t     *pwcDataPtr = NULL;


        /* Don't index the line if there is a closing tag - </..> */
        if ( pwcLine[1] == L'/' ) {
    
            /* Don't index */
            *pbIndexLine = false;
            
            return; 
        }
        

        /* Truncate the line if there is a closing tag on the line - '<..>..</..>' */
        if ( (pwcDataPtr = s_wcschr(pwcLine + 1, L'<')) != NULL ) {
            *pwcDataPtr = L'\0';
        }
    

        /* Reset the field ID global */
        uiMpsXmlFieldIDGlobal = PRS_MPS_FIELD_INVALID_ID;
        

        /* Key */
        if ( s_wcsncmp(pwcLine, L"<key>", /* s_wcslen(L"<key>") */ 5) == 0 ) {
        
            /* Set the data pointer - s_wcslen(L"<key>") */
            pwcDataPtr = pwcLine + 5;
        
            /* Save the key */
            s_wcsnncpy(pwcMpsKeyGlobal, pwcDataPtr, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
            
            /* Don't index the key */
            *pbIndexLine = false;
            
        }
        
        /* Language */
        else if ( s_wcsncmp(pwcLine, L"<language>", /* s_wcslen(L"<language>") */ 10) == 0 ) {
            
            unsigned char   pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned int    uiDataLength = 0;
    
            /* Set the data pointer - s_wcslen(L"<language>") */
            pwcDataPtr = pwcLine + 10;

            /* Get the length of the string */
            uiDataLength = s_wcslen(pwcDataPtr);
    
            /* We don't need all the string to check that it is a language, only the first SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH character */
            uiDataLength = UTL_MACROS_MIN(uiDataLength, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1);
    
            /* Convert the string */    
            if ( iLngConvertWideStringToUtf8_s(pwcDataPtr, uiDataLength, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1)== LNG_NoError ) {
    
                /* Replace some characters */
                iUtlStringsReplaceCharacterInString(pucLanguageCode, '-', '_');
    
                /* Get the language ID, default to LNG_LANGUAGE_ANY_ID on error */
                if ( iLngGetLanguageIDFromCode(pucLanguageCode, &uiMpsLanguageIDGlobal) != LNG_NoError ) {
                    uiMpsLanguageIDGlobal = LNG_LANGUAGE_ANY_ID;
                }
            }
    
            /* Don't index or parse the language */
            *pbIndexLine = false;
            *pbParseLine = false;
            *pbTermPositions = false;
        }
        
        /* Rank */
        else if ( s_wcsncmp(pwcLine, L"<rank>", /* s_wcslen(L"<rank>") */ 6) == 0 ) {

            int     iMpsRank = 0;

            /* Set the data pointer - s_wcslen(L"<rank>") */
            pwcDataPtr = pwcLine + 6;

            /* Get the rank */
            iMpsRank = (int)s_wcstol(pwcDataPtr, NULL, 10);

            /* Save the rank, min of 0 */
            uiMpsRankGlobal = UTL_MACROS_MAX(iMpsRank, 0);

            /* Don't index the rank */
            *pbIndexLine = false;
        }
        
        /* Date */
        else if ( s_wcsncmp(pwcLine, L"<date>", /* s_wcslen(L"<date>") */ 6) == 0 ) {

            long    lMpsDate = 0;

            /* Set the data pointer - s_wcslen(L"<date>") */
            pwcDataPtr = pwcLine + 6;

            /* Get the date */
            lMpsDate = s_wcstol(pwcDataPtr, NULL, 10);

            /* Save the date, min of 0 */
            ulMpsAnsiDateGlobal = UTL_MACROS_MAX(lMpsDate, 0);
    
            /* Don't index the date & time */
            *pbIndexLine = false;
        }
        
        /* Title */
        else if ( s_wcsncmp(pwcLine, L"<title>", /* s_wcslen(L"<title>") */ 7) == 0 ) {

            /* Set the data pointer - s_wcslen(L"<title>") */
            pwcDataPtr = pwcLine + 7;
            
            /* Decode the XML in place */
            iUtlStringsDecodeXmlWideString(pwcDataPtr);

            /* Save the document title */
            s_wcsnncpy(pwcMpsTitleGlobal, pwcDataPtr, SPI_TITLE_MAXIMUM_LENGTH + 1);
    
            /* Set the language for the document title */
            *puiLanguageID = uiMpsLanguageIDGlobal;
            
            /* Set the field ID */
            *puiFieldID = PRS_MPS_FIELD_TITLE_ID;

            /* Erase the tag - <title> - brute force */
            pwcLine[0] = L' ';    
            pwcLine[1] = L' ';    
            pwcLine[2] = L' ';    
            pwcLine[3] = L' ';    
            pwcLine[4] = L' ';    
            pwcLine[5] = L' ';    
            pwcLine[6] = L' ';    
        }
        
        /* Text */
        else if ( s_wcsncmp(pwcLine, L"<text>", /* s_wcslen(L"<text>") */ 6) == 0 ) {

            /* Set the data pointer - s_wcslen(L"<text>") */
            pwcDataPtr = pwcLine + 6;
        
            /* Decode the XML in place */
            iUtlStringsDecodeXmlWideString(pwcDataPtr);

            /* Set the language for the text */
            *puiLanguageID = uiMpsLanguageIDGlobal;
    
            /* Set the field ID */
            *puiFieldID = PRS_MPS_FIELD_TEXT_ID;

            /* Set the field ID global */
            uiMpsXmlFieldIDGlobal = PRS_MPS_FIELD_TEXT_ID;
            
            /* Erase the tag - <text> - brute force */
            pwcLine[0] = L' ';    
            pwcLine[1] = L' ';    
            pwcLine[2] = L' ';    
            pwcLine[3] = L' ';    
            pwcLine[4] = L' ';    
            pwcLine[5] = L' ';    
        }
        
        /* Url */
        else if ( s_wcsncmp(pwcLine, L"<url>", /* s_wcslen(L"<url>") */ 5) == 0 ) {

            wchar_t     *pwcPtr = NULL;
                
            /* Set the data pointer - s_wcslen(L"<url>") */
            pwcDataPtr = pwcLine + 5;

            /* Decode the XML in place */
            iUtlStringsDecodeXmlWideString(pwcDataPtr);

            /* Replace all spaces with a '+' sign */
            while ( (pwcPtr = s_wcschr(pwcDataPtr, L' ')) != NULL ) {
                *pwcPtr = L'+';
            }

            /* Copy the url */
            s_wcsnncpy(pwcMpsUrlGlobal, pwcDataPtr, SPI_URL_MAXIMUM_LENGTH + 1);

            /* Set the field ID */
            *puiFieldID = PRS_MPS_FIELD_URL_ID;
    
            /* Don't index the url */
/*             *pbIndexLine = false; */

            /* Don't parse the url, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the url */
            *pbTermPositions = false;

            /* Erase the tag - <url> - brute force */
            pwcLine[0] = L' ';    
            pwcLine[1] = L' ';    
            pwcLine[2] = L' ';    
            pwcLine[3] = L' ';    
            pwcLine[4] = L' ';    
        }
        
        /* Document */
        else if ( (s_wcsncmp(pwcLine, L"<document>", /* s_wcslen(L"<document>") */ 10) == 0) ||
                (s_wcsncmp(pwcLine, L"</document>", /* s_wcslen(L"</document>") */ 11) == 0) ) {
        
            /* Don't index */
            *pbIndexLine = false;
        
        }

        /* Invalid tag */
        else {
/*             iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid field tag in line: '%ls'.", pwcLine);  */
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid field tag in line: '%ls'.", pwcLine); 
        }
    }
    
    /* Keep indexing the text field */
    else if ( uiMpsXmlFieldIDGlobal == PRS_MPS_FIELD_TEXT_ID ) {
        
        /* Decode the XML in place */
        iUtlStringsDecodeXmlWideString(pwcLine);

        /* Set the language for the text */
/*         *puiLanguageID = uiMpsLanguageIDGlobal; */

        /* Set the field ID */
        *puiFieldID = PRS_MPS_FIELD_TEXT_ID;
    }


    return;

}


/*---------------------------------------------------------------------------*/

/* 
** =========================
** ====  Poplar Format  ====
** =========================
*/


/* Poplar format:
**
** Documents start with <doc> and end with </doc>
** and are separated by a blank line.
**
**
**  Queue Format:
**
**  Feed:
**  <doc>
**      <field name="language"></field>
**      <field name="languageCalc"></field>
**      <field name="feedKey"></field>
**      <field name="websiteUrl"></field> - added to site field
**      <field name="websiteHost"></field>
**      <field name="websiteDomain"></field>
**      <field name="sourceUrl"></field> - added to site field
**      <field name="sourceHost"></field>
**      <field name="sourceDomain"></field>
**      <field name="category"></field>
**      <field name="authorName"></field>
**      <field name="rating"></field>
**      <field name="site"></field>
**      <field name="sourceType"></field>
**      <field name="date"></field>
**      <field name="rank"></field>
**      <field name="key"></field>
**      <field name="title"></field>
**      <field name="description"></field>
**  </doc>
**
**  Item:
**  <doc>
**      <field name="language"></field>
**      <field name="languageCalc"></field>
**      <field name="itemKey"></field>
**      <field name="feedKey"></field>
**      <field name="uri"></field>
**      <field name="link"></field>
**      <field name="signatureHash"></field>
**      <field name="category"></field>
**      <field name="authorName"></field>
**      <field name="rating"></field>
**      <field name="itemType"></field>
**      <field name="linkUrl"></field>
**      <field name="linkHost"></field>
**      <field name="linkDomain"></field>
**      <field name="date"></field>
**      <field name="rank"></field>
**      <field name="key"></field>
**
**  Including source adds:
**          <field name="sourceCategory"></field>
**          <field name="sourceLanguage"></field>
**          <field name="sourceLanguageCalc"></field>
**          <field name="websiteUrl"></field> - added to site field
**          <field name="websiteHost"></field>
**          <field name="websiteDomain"></field>
**          <field name="sourceUrl"></field> - added to site field
**          <field name="sourceHost"></field>
**          <field name="sourceDomain"></field>
**          <field name="site"></field>
**
**      <field name="title"></field>
**      <field name="content"></field> (in order of availability: website content text, content text, summary)
**  </doc>
**
**  Enclosure:
**  <doc>
**      <field name="language"></field>
**      <field name="enclosureKey"></field>
**      <field name="itemKey"></field>
**      <field name="feedKey"></field>
**      <field name="url"></field>
**      <field name="enclosureType"></field>
**      <field name="enclosureMimeType"></field>
**      <field name="rating"></field>
**      <field name="category"></field>
**      <field name="date"></field>
**      <field name="rank"></field>
**      <field name="key"></field>
**      <field name="title"></field>
**      <field name="description"></field>
**  </doc>
**
**  Channel:
**  <doc>
**      <field name="channelID"></field>
**      <field name="userKey"></field>
**      <field name="parentChanneID"></field>
**      <field name="category"></field>
**      <field name="rating"></field>
**      <field name="date"></field>
**      <field name="rank"></field>
**      <field name="key"></field>
**      <field name="title"></field>
**      <field name="description"></field>
**  </doc>
**
**  User:
**  <doc>
**      <field name="userKey"></field>
**      <field name="userName"></field>
**      <field name="emailAddress"></field>
**      <field name="name"></field>
**      <field name="mobilePhone"></field>
**      <field name="imAddress"></field>
**      <field name="date"></field>
**      <field name="address"></field>
**      <field name="address"></field>
**      <field name="cityName"></field>
**      <field name="stateName"></field>
**      <field name="postCode"></field>
**      <field name="countryCode"></field>
**      <field name="key"></field>
**  </doc>
**
**
**
*/

/* Field IDs */
#define PRS_POPLAR_FIELD_INVALID_ID                     (0)
#define PRS_POPLAR_FIELD_TITLE_ID                       (1)     /* title */
#define PRS_POPLAR_FIELD_DESCRIPTION_ID                 (2)     /* description */
#define PRS_POPLAR_FIELD_CONTENT_ID                     (3)     /* content */
#define PRS_POPLAR_FIELD_URI_ID                         (4)     /* uri */
#define PRS_POPLAR_FIELD_LINK_ID                        (5)     /* link */
#define PRS_POPLAR_FIELD_URL_ID                         (6)     /* url */
#define PRS_POPLAR_FIELD_WEBSITE_URL_ID                 (7)     /* website_url */
#define PRS_POPLAR_FIELD_WEBSITE_HOST_ID                (8)     /* website_host */
#define PRS_POPLAR_FIELD_WEBSITE_DOMAIN_ID              (9)     /* website_domain */
#define PRS_POPLAR_FIELD_FEED_URL_ID                    (10)    /* source_url */
#define PRS_POPLAR_FIELD_FEED_HOST_ID                   (11)    /* source_host */
#define PRS_POPLAR_FIELD_FEED_DOMAIN_ID                 (12)    /* source_domain */
#define PRS_POPLAR_FIELD_LINK_URL_ID                    (13)    /* link_url */
#define PRS_POPLAR_FIELD_LINK_HOST_ID                   (14)    /* link_host */
#define PRS_POPLAR_FIELD_LINK_DOMAIN_ID                 (15)    /* link_domain */
#define PRS_POPLAR_FIELD_LANGUAGE_ID                    (16)    /* language */
#define PRS_POPLAR_FIELD_LANGUAGE_CALC_ID               (17)    /* language_calc */
#define PRS_POPLAR_FIELD_FEED_LANGUAGE_ID               (18)    /* source_language */
#define PRS_POPLAR_FIELD_FEED_LANGUAGE_CALC_ID          (19)    /* source_language_calc */
#define PRS_POPLAR_FIELD_CATEGORY_ID                    (20)    /* category */
#define PRS_POPLAR_FIELD_FEED_CATEGORY_ID               (21)    /* source_category */
#define PRS_POPLAR_FIELD_AUTHOR_NAME_ID                 (22)    /* author_name */
#define PRS_POPLAR_FIELD_RATING_ID                      (23)    /* rating */
#define PRS_POPLAR_FIELD_ENCLOSURE_TYPE_ID              (24)    /* enclosure_type */
#define PRS_POPLAR_FIELD_ENCLOSURE_MIME_TYPE_ID         (25)    /* enclosure_mime_type */
#define PRS_POPLAR_FIELD_SITE_ID                        (26)    /* field_site */
#define PRS_POPLAR_FIELD_PARENT_CHANNEL_ID_ID           (27)    /* parent_channel_id */
#define PRS_POPLAR_FIELD_USER_NAME_ID                   (28)    /* user_name */
#define PRS_POPLAR_FIELD_EMAIL_ADDRESS_ID               (29)    /* email_address */
#define PRS_POPLAR_FIELD_NAME_ID                        (30)    /* name */
#define PRS_POPLAR_FIELD_MOBILE_PHONE_ID                (31)    /* mobile_phone */
#define PRS_POPLAR_FIELD_IM_ADDRESS_ID                  (32)    /* im_address */
#define PRS_POPLAR_FIELD_ADDRESS_ID                     (33)    /* address */
#define PRS_POPLAR_FIELD_CITY_NAME_ID                   (34)    /* city_name */
#define PRS_POPLAR_FIELD_STATE_NAME_ID                  (35)    /* state_name */
#define PRS_POPLAR_FIELD_POST_CODE_ID                   (36)    /* post_code */
#define PRS_POPLAR_FIELD_COUNTRY_CODE_ID                (37)    /* country_code */
#define PRS_POPLAR_FIELD_FEED_KEY_ID                    (38)    /* source_key */
#define PRS_POPLAR_FIELD_ITEM_KEY_ID                    (39)    /* item_key */
#define PRS_POPLAR_FIELD_ENCLOSURE_KEY_ID               (40)    /* enclosure_key */
#define PRS_POPLAR_FIELD_USER_KEY_ID                    (41)    /* user_key */
#define PRS_POPLAR_FIELD_CHANNEL_ID_ID                  (42)    /* channel_id */
#define PRS_POPLAR_FIELD_SIGNATURE_HASH_ID              (43)    /* signature_hash */

#define PRS_POPLAR_FIELD_DATE_ID                        (44)    /* date */
#define PRS_POPLAR_FIELD_RANK_ID                        (45)    /* rank */    
#define PRS_POPLAR_FIELD_KEY_ID                         (46)    /* key */



/* Field descriptions */
struct prsFieldNameDescription pfndPoplarFieldNameDescriptionGlobal[] = 
{
    {PRS_POPLAR_FIELD_TITLE_ID,                 L"title",                   (unsigned char*)"CHAR",     NULL,                                                       L"Title",                           true},
    {PRS_POPLAR_FIELD_DESCRIPTION_ID,           L"description",             (unsigned char*)"CHAR",     NULL,                                                       L"Description",                     true},
    {PRS_POPLAR_FIELD_CONTENT_ID,               L"content",                 (unsigned char*)"CHAR",     NULL,                                                       L"Content",                         true},
    {PRS_POPLAR_FIELD_URI_ID,                   L"uri",                     (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Uri",                             false},
    {PRS_POPLAR_FIELD_LINK_ID,                  L"link",                    (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Link",                            false},
    {PRS_POPLAR_FIELD_URL_ID,                   L"url",                     (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Url",                             false},
    {PRS_POPLAR_FIELD_WEBSITE_URL_ID,           L"website_url",             (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Website Url",                     false},
    {PRS_POPLAR_FIELD_WEBSITE_HOST_ID,          L"website_host",            (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Website Host",                    false},
    {PRS_POPLAR_FIELD_WEBSITE_DOMAIN_ID,        L"website_domain",          (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Website Domain",                  false},
    {PRS_POPLAR_FIELD_FEED_URL_ID,              L"feed_url",                (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Feed Url",                        false},
    {PRS_POPLAR_FIELD_FEED_HOST_ID,             L"feed_host",               (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Feed Host",                       false},
    {PRS_POPLAR_FIELD_FEED_DOMAIN_ID,           L"feed_domain",             (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Feed Domain",                     false},
    {PRS_POPLAR_FIELD_LINK_URL_ID,              L"link_url",                (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Link Url",                        false},
    {PRS_POPLAR_FIELD_LINK_HOST_ID,             L"link_host",               (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Link Host",                       false},
    {PRS_POPLAR_FIELD_LINK_DOMAIN_ID,           L"link_domain",             (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Link Domain",                     false},
    {PRS_POPLAR_FIELD_LANGUAGE_ID,              L"language",                (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Language (declared)",             false},
    {PRS_POPLAR_FIELD_LANGUAGE_CALC_ID,         L"language_calc",           (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Language (calculated)",           false},
    {PRS_POPLAR_FIELD_FEED_LANGUAGE_ID,         L"feed_language",           (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Feed Language (declared)",        false},
    {PRS_POPLAR_FIELD_FEED_LANGUAGE_CALC_ID,    L"feed_language_calc",      (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Feed Language (calculated)",      false},
    {PRS_POPLAR_FIELD_CATEGORY_ID,              L"category",                (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,TermPositionOff",              L"Category",                        false},
    {PRS_POPLAR_FIELD_FEED_CATEGORY_ID,         L"feed_category",           (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,TermPositionOff",              L"Feed Category",                   false},
    {PRS_POPLAR_FIELD_AUTHOR_NAME_ID,           L"author_name",             (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Author Name",                     false},
    {PRS_POPLAR_FIELD_RATING_ID,                L"rating",                  (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Rating",                          false},
    {PRS_POPLAR_FIELD_ENCLOSURE_TYPE_ID,        L"enclosure_type",          (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Enclosure Type",                  false},
    {PRS_POPLAR_FIELD_ENCLOSURE_MIME_TYPE_ID,   L"enclosure_mime_type",     (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Enclosure Mime Type",             false},
    {PRS_POPLAR_FIELD_SITE_ID,                  L"site",                    (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Site",                            false},
    {PRS_POPLAR_FIELD_PARENT_CHANNEL_ID_ID,     L"parent_channel_id",       (unsigned char*)"LONG",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Parent Channel ID",               false},
    {PRS_POPLAR_FIELD_USER_NAME_ID,             L"user_name",               (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"User name",                       false},
    {PRS_POPLAR_FIELD_EMAIL_ADDRESS_ID,         L"email_address",           (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Email Address",                   false},
    {PRS_POPLAR_FIELD_NAME_ID,                  L"name",                    (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Name",                            false},
    {PRS_POPLAR_FIELD_MOBILE_PHONE_ID,          L"mobile_phone",            (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Mobile Phone",                    false},
    {PRS_POPLAR_FIELD_IM_ADDRESS_ID,            L"im_address",              (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"IM Address",                      false},
    {PRS_POPLAR_FIELD_ADDRESS_ID,               L"address",                 (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Address",                         false},
    {PRS_POPLAR_FIELD_CITY_NAME_ID,             L"city_name",               (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"City Name",                       false},
    {PRS_POPLAR_FIELD_STATE_NAME_ID,            L"state_name",              (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"State Name",                      false},
    {PRS_POPLAR_FIELD_POST_CODE_ID,             L"post_code",               (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Post Code",                       false},
    {PRS_POPLAR_FIELD_COUNTRY_CODE_ID,          L"country",                 (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Country Code",                    false},
    {PRS_POPLAR_FIELD_FEED_KEY_ID,              L"feed_key",                (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Feed Key",                        false},
    {PRS_POPLAR_FIELD_ITEM_KEY_ID,              L"item_key",                (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Item Key",                        false},
    {PRS_POPLAR_FIELD_ENCLOSURE_KEY_ID,         L"enclosure_key",           (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Enclosure Key",                   false},
    {PRS_POPLAR_FIELD_USER_KEY_ID,              L"user_key",                (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"User Key",                        false},
    {PRS_POPLAR_FIELD_CHANNEL_ID_ID,            L"channel_id",              (unsigned char*)"LONG",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Channel ID",                      false},
    {PRS_POPLAR_FIELD_SIGNATURE_HASH_ID,        L"signature_hash",          (unsigned char*)"CHAR",     (unsigned char*)"StopTermOff,StemmingOff,TermPositionOff",  L"Signature Hash",                  false},
    {PRS_POPLAR_FIELD_DATE_ID,                  L"date",                    NULL,                       NULL,                                                       NULL,                               false},
    {PRS_POPLAR_FIELD_RANK_ID,                  L"rank",                    NULL,                       NULL,                                                       NULL,                               false},
    {PRS_POPLAR_FIELD_KEY_ID,                   L"key",                     NULL,                       NULL,                                                       NULL,                               false},
    {PRS_POPLAR_FIELD_INVALID_ID,               NULL,                       NULL,                       NULL,                                                       NULL,                               false},
};


/* Globals */
static wchar_t pwcPoplarTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcPoplarDocumentKeyGlobal[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {L'\0'};
static unsigned int uiPoplarLanguageIDGlobal = LNG_LANGUAGE_ANY_ID;
static unsigned int uiPoplarRankGlobal = 0;
static unsigned long ulPoplarAnsiDateGlobal = 0;

/*

    Function:   bPrsPoplarDocumentEndFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the end of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine     Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsPoplarDocumentEndFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsPoplarDocumentEndFunction'."); 
    }


    if ( (pwcLine[0] == L'\0') || (pwcLine[1] == L'\0') ) {
        return (true);
    }


//     if ( (pwcLine[0] == L'<') && (s_wcsncmp(pwcLine, L"</doc>", /* s_wcslen(L"</doc>") */ 6) == 0) ) {
//         return (true);
//     }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsPoplarFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsPoplarFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    struct prsFieldNameDescription  *pfndPoplarFieldNameDescriptionPtr = NULL;


    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsPoplarFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsPoplarFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsPoplarFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsPoplarFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsPoplarFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndPoplarFieldNameDescriptionGlobal) / sizeof(struct prsFieldNameDescription)) ) {
        return (false);
    }
    
    /* Get a pointer to the field name description entry */
    pfndPoplarFieldNameDescriptionPtr = pfndPoplarFieldNameDescriptionGlobal + (uiFieldID - 1);
    
    /* Check that we don't go beyond the last field ID we provide information for */
    if ( pfndPoplarFieldNameDescriptionPtr->uiFieldID > PRS_POPLAR_FIELD_SIGNATURE_HASH_ID ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT(pfndPoplarFieldNameDescriptionPtr->uiFieldID == uiFieldID);
    *ppwcFieldName = pfndPoplarFieldNameDescriptionPtr->pwcFieldName;
    *ppucFieldType = pfndPoplarFieldNameDescriptionPtr->pucFieldType;
    *ppucFieldOptions = pfndPoplarFieldNameDescriptionPtr->pucFieldOptions;
    *ppwcFieldDescription = pfndPoplarFieldNameDescriptionPtr->pwcFieldDescription;
    *pbUnfieldedSearch = pfndPoplarFieldNameDescriptionPtr->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsPoplarDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcPoplarTitleGlobal, pwcPoplarDocumentKeyGlobal, uiPoplarRankGlobal,
                ulPoplarAnsiDateGlobal

    Returns:    void

*/
void vPrsPoplarDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    int                             iError = UTL_NoError;
    wchar_t                         *pwcFieldNamePtr = NULL;
    wchar_t                         *pwcDataPtr = NULL;
    wchar_t                         *pwcPtr = NULL;
    struct prsFieldNameDescription  *pfndPoplarFieldNameDescriptionPtr = NULL;


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsPoplarDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsPoplarDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsPoplarDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsPoplarDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsPoplarDocumentLineFunction'."); 
    }
    
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsPoplarDocumentLineFunction'."); 
    }
    

    /* Check for tag start */
    pwcPtr = s_wcschr(pwcLine, L'<');
    
    /* Failed to find tag start */
    if ( pwcPtr == NULL ) {
        *pbIndexLine = false;
        return; 
    }

    /* Check that the tag is a field open tag */
    if ( s_wcsncmp(pwcPtr, L"<field name=\"", /* s_wcslen(L"<field name=\"") */ 13) != 0 ) {
        *pbIndexLine = false;
        return; 
    }
    
    /* Get a pointer to the 'name' attribute in the field open tag */
    pwcFieldNamePtr = pwcPtr + 13;   /* <field name=" */


    /* And get a pointer to the end of the field open tag */
    pwcPtr = s_wcschr(pwcFieldNamePtr, L'>');
    
    /* Failed to fine the end of the field open tag */
    if ( pwcPtr == NULL ) {
        *pbIndexLine = false;
        return; 
    }
    
    /* Get a pointer to the content */
    pwcDataPtr = pwcPtr + 1;
    
    
    /* Get a pointer to the end of the 'name' attribute and null terminate it */ 
    pwcPtr = s_wcschr(pwcFieldNamePtr, L'"');
    *pwcPtr = L'\0';


    /* Set the field name description pointer and the field ID */
    pfndPoplarFieldNameDescriptionPtr = pfndPoplarFieldNameDescriptionGlobal;
    *puiFieldID = PRS_POPLAR_FIELD_INVALID_ID;
    
    /* Loop over the field name descriptions */
    while ( pfndPoplarFieldNameDescriptionPtr->uiFieldID != PRS_POPLAR_FIELD_INVALID_ID ) {
    
        /* Compare field names, on match set the field ID and break */
        if ( s_wcscmp(pfndPoplarFieldNameDescriptionPtr->pwcFieldName, pwcFieldNamePtr) == 0 ) {
            *puiFieldID = pfndPoplarFieldNameDescriptionPtr->uiFieldID;
            break;
        }
        
        /* Increment to the next field name description */
        pfndPoplarFieldNameDescriptionPtr++;
    }


    /* We dont check the field ID here, this is handled in the switch() statement below */


    /* Erase the field open tag */
    for ( pwcPtr = pwcLine; pwcPtr < pwcDataPtr; pwcPtr++ ) {
        *pwcPtr = L' ';
    }
   
     /* Get a pointer to the end of the data and null terminate it */ 
    pwcPtr = s_wcschr(pwcDataPtr, L'<');
    *pwcPtr = L'\0';


    /* Skip if there is no data */
    if ( bUtlStringsIsWideStringNULL(pwcDataPtr) == true ) {
        *pbIndexLine = false;
        return; 
    }


    
    /* Decode any encoded xml in the data in place */
    if ( (iError = iUtlStringsDecodeXmlWideString(pwcDataPtr)) != UTL_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to decode xml, error: %d.", iError); 
    }



    /* Process the field */
    switch ( *puiFieldID ) {
    
        /* Title */
        case PRS_POPLAR_FIELD_TITLE_ID:
    
            /* Save the document title */
            s_wcsnncpy(pwcPoplarTitleGlobal, pwcDataPtr, SPI_TITLE_MAXIMUM_LENGTH + 1);
    
            /* Set the language for the document title */
            *puiLanguageID = uiPoplarLanguageIDGlobal;
        
            break;


        /* Description */
        case PRS_POPLAR_FIELD_DESCRIPTION_ID:
    
            /* Set the language for the descripton */
            *puiLanguageID = uiPoplarLanguageIDGlobal;
        
            break;


        /* Content */
        case PRS_POPLAR_FIELD_CONTENT_ID:
    
            /* Set the language for the stripped content */
            *puiLanguageID = uiPoplarLanguageIDGlobal;
        
            break;


        /* Uri */
        case PRS_POPLAR_FIELD_URI_ID:

            /* Don't parse the uri, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the uri */
            *pbTermPositions = false;
            
            break;


        /* Link */
        case PRS_POPLAR_FIELD_LINK_ID:

            /* Don't parse the link, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the link */
            *pbTermPositions = false;
            
            break;


        /* Url */
        case PRS_POPLAR_FIELD_URL_ID:
    
            /* Don't parse the url, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the url */
            *pbTermPositions = false;
            
            break;


        /* Website url */
        case PRS_POPLAR_FIELD_WEBSITE_URL_ID:
    
            /* Don't parse the website url, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the website url */
            *pbTermPositions = false;
            
            break;


        /* Website host */
        case PRS_POPLAR_FIELD_WEBSITE_HOST_ID:
    
            /* Don't parse the website host, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the website host */
            *pbTermPositions = false;
            
            break;


        /* Website domain */
        case PRS_POPLAR_FIELD_WEBSITE_DOMAIN_ID:
    
            /* Don't parse the website domain, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the website domain */
            *pbTermPositions = false;
        
            break;


        /* Feed url */
        case PRS_POPLAR_FIELD_FEED_URL_ID:
    
            /* Don't parse the feed url, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the feed url */
            *pbTermPositions = false;
            
            break;


        /* Feed host */
        case PRS_POPLAR_FIELD_FEED_HOST_ID:
    
            /* Don't parse the feed host, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the feed host */
            *pbTermPositions = false;
            
            break;


        /* Feed domain */
        case PRS_POPLAR_FIELD_FEED_DOMAIN_ID:
    
            /* Don't parse the feed domain, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the feed domain */
            *pbTermPositions = false;
        
            break;


        /* Link url */
        case PRS_POPLAR_FIELD_LINK_URL_ID:
    
            /* Don't parse the link url, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the link url */
            *pbTermPositions = false;
            
            break;


        /* Link host */
        case PRS_POPLAR_FIELD_LINK_HOST_ID:
    
            /* Don't parse the link host, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the link host */
            *pbTermPositions = false;
            
            break;


        /* Link domain  */
        case PRS_POPLAR_FIELD_LINK_DOMAIN_ID:
    
            /* Don't parse the link domain, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the link domain */
            *pbTermPositions = false;
        
            break;


        /* Language & language calc */
        case PRS_POPLAR_FIELD_LANGUAGE_ID:
        case PRS_POPLAR_FIELD_LANGUAGE_CALC_ID:
    
            /* Set the language if it has not already been set of if it is the calculated language */
            {        
                unsigned char   pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};
                unsigned int    uiDataLength = 0;
    
                /* Get the length of the string */
                uiDataLength = s_wcslen(pwcDataPtr);
        
                /* We don't need all the string to check that it is a language, only the first SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH character */
                uiDataLength = UTL_MACROS_MIN(uiDataLength, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1);
        
                /* Convert the string */    
                if ( iLngConvertWideStringToUtf8_s(pwcDataPtr, uiDataLength, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1) == LNG_NoError ) {
        
                    /* Replace some characters */
                    iUtlStringsReplaceCharacterInString(pucLanguageCode, '-', '_');
        
                    /* Get the language ID, default to LNG_LANGUAGE_ANY_ID on error */
                    if ( iLngGetLanguageIDFromCode(pucLanguageCode, &uiPoplarLanguageIDGlobal) != LNG_NoError ) {
                        uiPoplarLanguageIDGlobal = LNG_LANGUAGE_ANY_ID;
                    }
                }
            }
    
            /* Don't parse the language, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the language */
            *pbTermPositions = false;
            
            break;


        /* Feed language & feed language calc */
        case PRS_POPLAR_FIELD_FEED_LANGUAGE_ID:
        case PRS_POPLAR_FIELD_FEED_LANGUAGE_CALC_ID:
            
            /* Don't parse the feed language, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the feed language */
            *pbTermPositions = false;

            break;


        /* Category */
        case PRS_POPLAR_FIELD_CATEGORY_ID:

            /* Replace all the spaces with underscores */
            iUtlStringsReplaceCharacterInWideString(pwcDataPtr, ' ', '_');

            /* Don't parse the category, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the category */
            *pbTermPositions = false;
            
            break;


        /* Feed category */
        case PRS_POPLAR_FIELD_FEED_CATEGORY_ID:

            /* Replace all the spaces with underscores */
            iUtlStringsReplaceCharacterInWideString(pwcDataPtr, ' ', '_');

            /* Don't parse the feed category, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the feed category */
            *pbTermPositions = false;
            
            break;


        /* Author name */
        case PRS_POPLAR_FIELD_AUTHOR_NAME_ID:

            /* Set the field ID */
            *puiFieldID = PRS_POPLAR_FIELD_AUTHOR_NAME_ID;
    
            /* Don't send term positions for the author name */
            *pbTermPositions = false;
        
            break;


        /* Rating */
        case PRS_POPLAR_FIELD_RATING_ID:

            /* Don't parse the rating, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the rating */
            *pbTermPositions = false;
        
            break;


        /* Enclosure type */
        case PRS_POPLAR_FIELD_ENCLOSURE_TYPE_ID:

            /* Don't parse the enclosure type, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the enclosure type */
            *pbTermPositions = false;
        
            break;


        /* Enclosure mime type */
        case PRS_POPLAR_FIELD_ENCLOSURE_MIME_TYPE_ID:

            /* Don't parse the enclosure mime type, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the enclosure mime type */
            *pbTermPositions = false;
        
            break;


        /* Site */
        case PRS_POPLAR_FIELD_SITE_ID:

            /* Don't parse the site, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the site */
            *pbTermPositions = false;
        
            break;


        /* Parent channel ID */
        case PRS_POPLAR_FIELD_PARENT_CHANNEL_ID_ID:

            /* Don't parse the parent child ID, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the parent child ID */
            *pbTermPositions = false;
        
            break;


        /* User Name */
        case PRS_POPLAR_FIELD_USER_NAME_ID:

            /* Don't parse the user name, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the user name */
            *pbTermPositions = false;
        
            break;


        /* Email address */
        case PRS_POPLAR_FIELD_EMAIL_ADDRESS_ID:

            /* Don't parse the email address, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the email address */
            *pbTermPositions = false;
        
            break;


        /* Name */
        case PRS_POPLAR_FIELD_NAME_ID:

            /* Don't send term positions for the name */
            *pbTermPositions = false;
        
            break;


        /* Mobile phone */
        case PRS_POPLAR_FIELD_MOBILE_PHONE_ID:

            /* Don't parse the mobile phone, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the mobile phone */
            *pbTermPositions = false;
        
            break;


        /* IM address */
        case PRS_POPLAR_FIELD_IM_ADDRESS_ID:

            /* Don't parse the im address, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the im address */
            *pbTermPositions = false;
        
            break;


        /* Address */
        case PRS_POPLAR_FIELD_ADDRESS_ID:

            break;


        /* City name */
        case PRS_POPLAR_FIELD_CITY_NAME_ID:

            /* Don't parse the city name, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the city name */
            *pbTermPositions = false;
        
            break;


        /* State name */
        case PRS_POPLAR_FIELD_STATE_NAME_ID:

            /* Don't parse the state name, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the state name */
            *pbTermPositions = false;
        
            break;


        /* Post code */
        case PRS_POPLAR_FIELD_POST_CODE_ID:

            /* Don't parse the post code, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the post code */
            *pbTermPositions = false;
        
            break;


        /* Country code */
        case PRS_POPLAR_FIELD_COUNTRY_CODE_ID:

            /* Don't parse the country code, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the country code */
            *pbTermPositions = false;
        
            break;


        /* Feed key */
        case PRS_POPLAR_FIELD_FEED_KEY_ID:

            /* Don't parse the feed key, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the feed key */
            *pbTermPositions = false;
        
            break;


        /* Item key */
        case PRS_POPLAR_FIELD_ITEM_KEY_ID:

            /* Don't parse the item key, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the item key */
            *pbTermPositions = false;
        
            break;


        /* Enclosure key */
        case PRS_POPLAR_FIELD_ENCLOSURE_KEY_ID:

            /* Don't parse the enclosure key, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the enclosure key */
            *pbTermPositions = false;
        
            break;


        /* User key */
        case PRS_POPLAR_FIELD_USER_KEY_ID:

            /* Don't parse the user key, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the user key */
            *pbTermPositions = false;
        
            break;


        /* Channel ID */
        case PRS_POPLAR_FIELD_CHANNEL_ID_ID:

            /* Don't parse the channel ID, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the channel ID */
            *pbTermPositions = false;
        
            break;


        /* Signature hash */
        case PRS_POPLAR_FIELD_SIGNATURE_HASH_ID:

            /* Don't parse the signature hash, it is a single term */
            *pbParseLine = false;
    
            /* Don't send term positions for the signature hash */
            *pbTermPositions = false;
            
            break;


        /* Date */
        case PRS_POPLAR_FIELD_DATE_ID:

            /* Get the ansi date from the zulu date */
            iUtlDateParseWideDateToAnsiDate(pwcDataPtr, &ulPoplarAnsiDateGlobal);
    
            /* Don't index the date & time */
            *pbIndexLine = false;
        
            break;


        /* Rank */
        case PRS_POPLAR_FIELD_RANK_ID:

            {
                /* Get the rank */
                int iPoplarRank = (int)s_wcstol(pwcDataPtr, NULL, 10);
    
                /* Save the rank, min of 0 */
                uiPoplarRankGlobal = UTL_MACROS_MAX(iPoplarRank, 0);
        
                /* Don't index the rank */
                *pbIndexLine = false;
            }
        
            break;


        /* Document key */
        case PRS_POPLAR_FIELD_KEY_ID:

            /* Save the key */
            s_wcsnncpy(pwcPoplarDocumentKeyGlobal, pwcDataPtr, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
    
            /* Don't index the key */
            *pbIndexLine = false;
        
            break;


        default:

            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid field tag in line: '%ls'.", pwcLine); 

    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsPoplarDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcPoplarTitleGlobal, pwcPoplarDocumentKeyGlobal, uiPoplarRankGlobal,
                ulPoplarAnsiDateGlobal

    Returns:    void

*/
void vPrsPoplarDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsPoplarDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsPoplarDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsPoplarDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsPoplarDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsPoplarDocumentInformationFunction'."); 
    }



    /* Set the document title */
    s_wcsnncpy(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcPoplarTitleGlobal) == false) ? 
            pwcPoplarTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH + 1);

    /* Set the document key */
    if ( bUtlStringsIsWideStringNULL(pwcPoplarDocumentKeyGlobal) == false ) {
        s_wcsnncpy(pwcDocumentKey, pwcPoplarDocumentKeyGlobal, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
    }

    /* Set the rank */
    *puiDocumentRank = uiPoplarRankGlobal;

    /* Set the date */
    *pulDocumentAnsiDate = ulPoplarAnsiDateGlobal;

    /* Clear the globals */
    pwcPoplarTitleGlobal[0] = L'\0';
    pwcPoplarDocumentKeyGlobal[0] = L'\0';
    uiPoplarLanguageIDGlobal = LNG_LANGUAGE_ANY_ID;
    uiPoplarRankGlobal = 0;
    ulPoplarAnsiDateGlobal = 0;

    
    return;

}


/*---------------------------------------------------------------------------*/


/* 
** ==========================
** ====  Medline Format  ====
** ==========================
*/


/* Medline citation format:
** 
** Documents are separated by a blank line.
** 

CY  - UNITED STATES
PMID- 10970049
IP  - 1
AA  - AUTHOR
AB  - The influence of insulin-dependent diabetes mellitus on the oxidative
      stress caused by cadmium in the liver and kidney of laboratory rats has
      been studied. The results suggest that cadmium and alloxan diabetes
      independently promote lipid peroxidation in both liver and kidney.
      However, lipid peroxidation diminished in the diabetic rats fed
      cadmium. Administration of cadmium to normal and diabetic rats depleted
      glutathione in liver only. No significant change was observed in the
      activity of glutathione peroxidase in kidney, whereas administration of
      cadmium to diabetic rats stimulated catalase activity when compared to
      cadmium-fed rats. The actual mechanism of these effects still remains
      to be confirmed, but an antagonistic relationship between cytotoxic
      mechanisms of diabetes mellitus and cadmium is speculated upon. The
      insulin-dependent activity of a unique form of cytochrome 450j may be
      involved.
IS  - 0748-6642
AD  - Department of Zoology, Ch. Charan Singh University, Meerut, India.
PG  - 67-74
SB  - M
DA  - 20000831
TA  - Physiol Chem Phys Med NMR
TI  - Antioxidative enzymes in the liver and kidney of alloxan induced
      diabetic rats and their implications in cadmium toxicity [In Process
      Citation]
UI  - 20424466
RO  - O:099
VI  - 32
SO  - Physiol Chem Phys Med NMR 2000;32(1):67-74
AU  - Rana SV
AU  - Rastogi N
JC  - P7D
DP  - 2000
MHDA- 2000/09/02 11:00
LA  - Eng
EDAT- 2000/09/02 11:00

** 
*/


/* Medline loader format:
**
** Documents are separated with a '$$' symbol, which is also 
** present before the first record and after the last record.
**

$$
DA(1)=800228
CU(1)=90
IS(1)=0006-3002
TA(1)=Biochim Biophys Acta
LA(1)=Eng
CV(1)=NETHERLANDS
JC(1)=A0W
SB(1)=M
UI(1)=80065774
MH(1)=Anemia, Sickle Cell\1\I\*BL
MH(2)=Chemistry\1\I
MH(3)=Comparative Study\3\I
MH(4)=Cross-Linking Reagents\1\I
MH(5)=Electrophoresis, Polyacrylamide Gel\1\I
MH(6)=Erythrocyte Membrane\1\I\*AN/UL
MH(7)=Erythrocytes\1\M\*AN
MH(8)=Human\3\I
MH(9)=Membrane Proteins\1\I\*BL
TI(1)=Organizational differences in the membrane proteins of normal and irreversibly sickled erythrocytes.
PG(1)=1-8
DP(1)=1980
PT(1)=JOURNAL ARTICLE
AB(1)=Using two-dimensional gels, no unique membrane proteins were detected in irreversibly sickled cells. Membranes from irreversibly sickled cells were shown to cross-link much more readily with dithiobis(succinimidyl propionate) than normal erythrocyte membranes. Increased binding of band 4.5 protein and increased intra-chain disulfides were also demonstrated. These changes may correlate to enhanced cellular rigidity.
AU(1)=Rubin RW
AU(2)=Milikowski C
AU(3)=Wise GE
IP(1)=1
VI(1)=595
EM(1)=8004
$$

** 
*/


/* Maximum author name length */
#define PRS_MEDLINE_AUTHOR_LENGTH_MAXIMUM       (60)


/* Medline record format */
#define PRS_MEDLINE_FORMAT_UNKNOWN              (0)
#define PRS_MEDLINE_FORMAT_CITATION             (1)
#define PRS_MEDLINE_FORMAT_LOADER               (2)


/* Field IDs */
#define PRS_MEDLINE_FIELD_INVALID_ID            (0)
#define PRS_MEDLINE_FIELD_DA_ID                 (1)
#define PRS_MEDLINE_FIELD_CU_ID                 (2)
#define PRS_MEDLINE_FIELD_IS_ID                 (3)
#define PRS_MEDLINE_FIELD_TA_ID                 (4)
#define PRS_MEDLINE_FIELD_SO_ID                 (5)
#define PRS_MEDLINE_FIELD_LA_ID                 (6)
#define PRS_MEDLINE_FIELD_CV_ID                 (7)
#define PRS_MEDLINE_FIELD_JC_ID                 (8)
#define PRS_MEDLINE_FIELD_SB_ID                 (9)
#define PRS_MEDLINE_FIELD_UI_ID                 (10)
#define PRS_MEDLINE_FIELD_MH_ID                 (11)
#define PRS_MEDLINE_FIELD_TI_ID                 (12)
#define PRS_MEDLINE_FIELD_PG_ID                 (13)
#define PRS_MEDLINE_FIELD_DP_ID                 (14)
#define PRS_MEDLINE_FIELD_PT_ID                 (15)
#define PRS_MEDLINE_FIELD_AB_ID                 (16)
#define PRS_MEDLINE_FIELD_AU_ID                 (17)
#define PRS_MEDLINE_FIELD_IP_ID                 (18)
#define PRS_MEDLINE_FIELD_VI_ID                 (19)
#define PRS_MEDLINE_FIELD_EM_ID                 (20)
#define PRS_MEDLINE_FIELD_RO_ID                 (21)
#define PRS_MEDLINE_FIELD_LR_ID                 (22)
#define PRS_MEDLINE_FIELD_NI_ID                 (23)
#define PRS_MEDLINE_FIELD_RN_ID                 (24)
#define PRS_MEDLINE_FIELD_TT_ID                 (25)
#define PRS_MEDLINE_FIELD_LI_ID                 (26)
#define PRS_MEDLINE_FIELD_EA_ID                 (27)
#define PRS_MEDLINE_FIELD_PS_ID                 (28)
#define PRS_MEDLINE_FIELD_PY_ID                 (29)
#define PRS_MEDLINE_FIELD_SI_ID                 (30)
#define PRS_MEDLINE_FIELD_ID_ID                 (31)
#define PRS_MEDLINE_FIELD_CM_ID                 (32)
#define PRS_MEDLINE_FIELD_AD_ID                 (33)
#define PRS_MEDLINE_FIELD_RF_ID                 (34)
#define PRS_MEDLINE_FIELD_NP_ID                 (35)
#define PRS_MEDLINE_FIELD_GS_ID                 (36)
#define PRS_MEDLINE_FIELD_CA_ID                 (37)
#define PRS_MEDLINE_FIELD_MRI_ID                (38)
#define PRS_MEDLINE_FIELD_PMID_ID               (39)
#define PRS_MEDLINE_FIELD_MHDA_ID               (40)
#define PRS_MEDLINE_FIELD_EDAT_ID               (41)
#define PRS_MEDLINE_FIELD_URLS_ID               (42)
#define PRS_MEDLINE_FIELD_URLF_ID               (43)


/* Medline field name/description structure */
struct prsMedlineFieldNameDescription {
    unsigned int    uiFieldID;                  /* Field ID */
    wchar_t         *pwcFieldName;              /* Field name */
    unsigned int    uiFieldNameLength;          /* Field name length (denormalization) */
    unsigned char   *pucFieldType;              /* Field type */
    unsigned char   *pucFieldOptions;           /* Field options */
    wchar_t         *pwcFieldDescription;       /* Field description */
    boolean         bUnfieldedSearch;           /* Unfielded search field */
};


/* Field descriptions */
static struct prsMedlineFieldNameDescription pmfndMedlineFieldNameDescriptionGlobal[] = 
{
    {PRS_MEDLINE_FIELD_DA_ID,       L"DA",      2,  NULL,   NULL,   L"Date Added",                  false},
    {PRS_MEDLINE_FIELD_CU_ID,       L"CU",      2,  NULL,   NULL,   L"Class Update",                false},
    {PRS_MEDLINE_FIELD_IS_ID,       L"IS",      2,  NULL,   NULL,   L"ISSN",                        false},
    {PRS_MEDLINE_FIELD_TA_ID,       L"TA",      2,  NULL,   NULL,   L"Source",                      false},
    {PRS_MEDLINE_FIELD_SO_ID,       L"SO",      2,  NULL,   NULL,   L"Source (Full)",               false},
    {PRS_MEDLINE_FIELD_LA_ID,       L"LA",      2,  NULL,   NULL,   L"Language",                    false},
    {PRS_MEDLINE_FIELD_CV_ID,       L"CV",      2,  NULL,   NULL,   L"Country",                     false},
    {PRS_MEDLINE_FIELD_JC_ID,       L"JC",      2,  NULL,   NULL,   L"Journal Title Code",          false},
    {PRS_MEDLINE_FIELD_SB_ID,       L"SB",      2,  NULL,   NULL,   L"Journal Subset",              false},
    {PRS_MEDLINE_FIELD_UI_ID,       L"UI",      2,  NULL,   NULL,   L"Medline ID",                  false},
    {PRS_MEDLINE_FIELD_MH_ID,       L"MH",      2,  NULL,   NULL,   L"Mesh",                        false},
    {PRS_MEDLINE_FIELD_TI_ID,       L"TI",      2,  NULL,   NULL,   L"Title",                       true},
    {PRS_MEDLINE_FIELD_PG_ID,       L"PG",      2,  NULL,   NULL,   L"Pages",                       false},
    {PRS_MEDLINE_FIELD_DP_ID,       L"DP",      2,  NULL,   NULL,   L"Publication Date",            false},
    {PRS_MEDLINE_FIELD_PT_ID,       L"PT",      2,  NULL,   NULL,   L"Publication Type",            false},
    {PRS_MEDLINE_FIELD_AB_ID,       L"AB",      2,  NULL,   NULL,   L"Abstract",                    true},
    {PRS_MEDLINE_FIELD_AU_ID,       L"AU",      2,  NULL,   NULL,   L"Author",                      true},
    {PRS_MEDLINE_FIELD_IP_ID,       L"IP",      2,  NULL,   NULL,   L"Issue/Part/Sup.",             false},
    {PRS_MEDLINE_FIELD_VI_ID,       L"VI",      2,  NULL,   NULL,   L"Volume Issue",                false},
    {PRS_MEDLINE_FIELD_EM_ID,       L"EM",      2,  NULL,   NULL,   L"Entry Month",                 false},
    {PRS_MEDLINE_FIELD_RO_ID,       L"RO",      2,  NULL,   NULL,   L"Record Originator",           false},
    {PRS_MEDLINE_FIELD_LR_ID,       L"LR",      2,  NULL,   NULL,   L"Last Revision Date",          false},
    {PRS_MEDLINE_FIELD_NI_ID,       L"NI",      2,  NULL,   NULL,   L"No Author",                   false},
    {PRS_MEDLINE_FIELD_RN_ID,       L"RN",      2,  NULL,   NULL,   L"CAS Reg./EC No",              false},
    {PRS_MEDLINE_FIELD_TT_ID,       L"TT",      2,  NULL,   NULL,   L"Vernacular Title",            false},
    {PRS_MEDLINE_FIELD_LI_ID,       L"LI",      2,  NULL,   NULL,   L"List Indicator",              false},
    {PRS_MEDLINE_FIELD_EA_ID,       L"EA",      2,  NULL,   NULL,   L"English Abstract Available",  false},
    {PRS_MEDLINE_FIELD_PS_ID,       L"PS",      2,  NULL,   NULL,   L"Personal Name as Subject",    false},
    {PRS_MEDLINE_FIELD_PY_ID,       L"PY",      2,  NULL,   NULL,   L"Indexing Priority",           false},
    {PRS_MEDLINE_FIELD_SI_ID,       L"SI",      2,  NULL,   NULL,   L"Secondary Source ID",         false},
    {PRS_MEDLINE_FIELD_ID_ID,       L"ID",      2,  NULL,   NULL,   L"Ext. ID",                     false},
    {PRS_MEDLINE_FIELD_CM_ID,       L"CM",      2,  NULL,   NULL,   L"Comments",                    false},
    {PRS_MEDLINE_FIELD_AD_ID,       L"AD",      2,  NULL,   NULL,   L"Address",                     false},
    {PRS_MEDLINE_FIELD_RF_ID,       L"RF",      2,  NULL,   NULL,   L"# of References",             false},
    {PRS_MEDLINE_FIELD_NP_ID,       L"NP",      2,  NULL,   NULL,   L"Not for Publication",         false},
    {PRS_MEDLINE_FIELD_GS_ID,       L"GS",      2,  NULL,   NULL,   L"Gene Symbol",                 false},
    {PRS_MEDLINE_FIELD_CA_ID,       L"CA",      2,  NULL,   NULL,   L"Call Number",                 false},
    {PRS_MEDLINE_FIELD_MRI_ID,      L"MRI",     3,  NULL,   NULL,   L"Machine Readable Identifier", false},
    {PRS_MEDLINE_FIELD_PMID_ID,     L"PMID",    4,  NULL,   NULL,   L"PubMed Identifier",           false},
    {PRS_MEDLINE_FIELD_MHDA_ID,     L"MHDA",    4,  NULL,   NULL,   L"Date (??)",                   false},
    {PRS_MEDLINE_FIELD_EDAT_ID,     L"EDAT",    4,  NULL,   NULL,   L"Date (??)",                   false},
    {PRS_MEDLINE_FIELD_URLS_ID,     L"URLS",    4,  NULL,   NULL,   L"URL to Abstract",             false},
    {PRS_MEDLINE_FIELD_URLF_ID,     L"URLF",    4,  NULL,   NULL,   L"URL to Full Text",            false},
    {PRS_MEDLINE_FIELD_INVALID_ID,  NULL,       0,  NULL,   NULL,   NULL,                           false},
};


/* Globals */
static wchar_t pwcMedlineTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcMedlineDateGlobal[PRS_MAX_YEAR_LENGTH + 1] = {L'\0'};
static wchar_t pwcMedlineFirstAuthorGlobal[PRS_MEDLINE_AUTHOR_LENGTH_MAXIMUM + 1] = {L'\0'};
static wchar_t pwcMedlineIdentifierGlobal[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {L'\0'};
static unsigned int uiMedlineFieldIDGlobal = 0;
static unsigned long ulMedlineAnsiDateGlobal = 0;


/*

    Function:   bPrsMedlineDocumentEndFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the end of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsMedlineDocumentEndFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsMedlineDocumentEndFunction'."); 
    }


    if ( (pwcLine[0] == L'\0') || (pwcLine[1] == L'\0') || ((pwcLine[0] == L'$') && (s_wcsncmp(pwcLine, L"$$", /* s_wcslen(L"$$") */ 2) == 0)) ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsMedlineFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsMedlineFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsMedlineFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsMedlineFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsMedlineFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsMedlineFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsMedlineFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pmfndMedlineFieldNameDescriptionGlobal) / sizeof(struct prsMedlineFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pmfndMedlineFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pmfndMedlineFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pmfndMedlineFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pmfndMedlineFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pmfndMedlineFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pmfndMedlineFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsMedlineDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcMedlineTitleGlobal, pwcMedlineDateGlobal, pwcMedlineFirstAuthorGlobal,
                        pwcMedlineIdentifierGlobal, uiMedlineFieldIDGlobal, ulMedlineAnsiDateGlobal

    Returns:    void

*/
void vPrsMedlineDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    wchar_t         *pwcLinePtr = NULL;
    wchar_t         *pwcDataPtr = NULL;
    unsigned int    uiFormat = PRS_MEDLINE_FORMAT_UNKNOWN;


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsMedlineDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsMedlineDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsMedlineDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsMedlineDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsMedlineDocumentLineFunction'."); 
    }
    
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsMedlineDocumentLineFunction'."); 
    }
    
    
    /* Document terminator, no field */
    if ( (pwcLine[0] == L'\0') || (pwcLine[1] == L'\0') || ((pwcLine[0] == L'$') && (s_wcsncmp(pwcLine, L"$$", /* s_wcslen(L"$$") */ 2) == 0)) ) {
        return;
    }
    
    /* Continuation of the previous field in the citation format */
    else if ( (pwcLine[0] == L' ') && (s_wcsncmp(pwcLine, L"      ", /* s_wcslen(L"      ") */ 6) == 0) ) {
        /* Same field ID as previous line */
        uiFormat = PRS_MEDLINE_FORMAT_CITATION;
    }

    /* Potential new field ID */
    else {

        struct prsMedlineFieldNameDescription   *pmfndMedlineFieldNameDescriptionPtr = pmfndMedlineFieldNameDescriptionGlobal;

        /* Loop over each field */
        for ( pmfndMedlineFieldNameDescriptionPtr = pmfndMedlineFieldNameDescriptionGlobal, uiMedlineFieldIDGlobal = 0; 
                pmfndMedlineFieldNameDescriptionPtr->pwcFieldName != NULL; pmfndMedlineFieldNameDescriptionPtr++ ) {
            
            /* Is this a potential field? */
            if ( (pwcLine[0] == pmfndMedlineFieldNameDescriptionPtr->pwcFieldName[0]) && 
                    (s_wcsncmp(pwcLine, pmfndMedlineFieldNameDescriptionPtr->pwcFieldName, pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength) == 0) ) {
                
                /* Check for 'AU(' */
                if ( pwcLine[pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength] == L'(' ) {
                    uiMedlineFieldIDGlobal = pmfndMedlineFieldNameDescriptionPtr->uiFieldID;
                    uiFormat = PRS_MEDLINE_FORMAT_LOADER;
                    break;
                }
                /* Check for 'AU  - ' */
                else if ( (pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength == 2) &&
                         (pwcLine[pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength] == L' ') &&
                         (s_wcsncmp(pwcLine  + pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength, L"  - ", /* s_wcslen(L"  - ") */ 4) == 0) ) {
                    uiMedlineFieldIDGlobal = pmfndMedlineFieldNameDescriptionPtr->uiFieldID;
                    uiFormat = PRS_MEDLINE_FORMAT_CITATION;
                    break;
                }
                /* Check for 'MRI - ' */
                else if ( (pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength == 3) &&
                         (pwcLine[pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength] == L' ') &&
                         (s_wcsncmp(pwcLine  + pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength, L" - ", /* s_wcslen(L"  - ") */ 4) == 0) ) {
                    uiMedlineFieldIDGlobal = pmfndMedlineFieldNameDescriptionPtr->uiFieldID;
                    uiFormat = PRS_MEDLINE_FORMAT_CITATION;
                    break;
                }
                /* Check for 'URLS- ' */
                else if ( (pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength == 4) &&
                         (pwcLine[pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength] == L'-') &&
                        ( s_wcsncmp(pwcLine  + pmfndMedlineFieldNameDescriptionPtr->uiFieldNameLength, L"- ", /* s_wcslen(L"- ") */ 2) == 0) ) {
                    uiMedlineFieldIDGlobal = pmfndMedlineFieldNameDescriptionPtr->uiFieldID;
                    uiFormat = PRS_MEDLINE_FORMAT_CITATION;
                    break;
                }
            }
        }
    }

    /* Set the field name */
    *puiFieldID = uiMedlineFieldIDGlobal;

    /* We should have detected a style by now */
    if ( uiFormat == PRS_MEDLINE_FORMAT_UNKNOWN ) {
        return;
    }

    
    /* Get a pointer to the data */
    if ( (pwcDataPtr = (uiFormat == PRS_MEDLINE_FORMAT_LOADER) ? (s_wcschr(pwcLine, L'=') + 1) : (pwcLine + 6)) == NULL ) {
        return;
    }

    /* Get the document title */    
    if ( uiMedlineFieldIDGlobal == PRS_MEDLINE_FIELD_TI_ID ) {
        
        if ( bUtlStringsIsWideStringNULL(pwcMedlineTitleGlobal) == false ) {
            s_wcsnncat(pwcMedlineTitleGlobal, L" ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }
        s_wcsnncat(pwcMedlineTitleGlobal, pwcDataPtr, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }

    /* Get the Medline ID, but don't override the  */    
    else if ( (uiMedlineFieldIDGlobal == PRS_MEDLINE_FIELD_UI_ID) && (uiFormat == PRS_MEDLINE_FORMAT_LOADER) ) {
        s_wcsnncpy(pwcMedlineIdentifierGlobal, pwcDataPtr, PRS_SHORT_STRING_LENGTH + 1);
    }

    /* Get the PubMed ID */    
    else if ( (uiMedlineFieldIDGlobal == PRS_MEDLINE_FIELD_PMID_ID) && (uiFormat == PRS_MEDLINE_FORMAT_CITATION) ) {
        s_wcsnncpy(pwcMedlineIdentifierGlobal, pwcDataPtr, PRS_SHORT_STRING_LENGTH + 1);
    }

    /* Get the date of publication */    
    else if ( uiMedlineFieldIDGlobal == PRS_MEDLINE_FIELD_DP_ID ) {
        
        /* Grab the year for the document title */
        s_wcsnncpy(pwcMedlineDateGlobal, pwcDataPtr, PRS_MAX_YEAR_LENGTH + 1);
        
        /* Erase the second part of the date, eg '-May 4' in '1982 Apr 28-May 4' */
        if ( (pwcLinePtr = s_wcschr(pwcDataPtr, L'-')) != NULL ) {
            for ( ;(*pwcLinePtr != L'=') && (*pwcLinePtr != L'\0'); pwcLinePtr++ ) {
                *pwcLinePtr = L' ';
            }
        }

        /* Parse out the date, set to 0 on error */
        if ( iUtlDateParseWideDateToAnsiDate(pwcDataPtr, &ulMedlineAnsiDateGlobal) != UTL_NoError ) {
            ulMedlineAnsiDateGlobal = 0;
        }
    
    }

    /* Get the author */    
    else if ( uiMedlineFieldIDGlobal == PRS_MEDLINE_FIELD_AU_ID ) {
        
        if ( bUtlStringsIsWideStringNULL(pwcMedlineFirstAuthorGlobal) == true ) {
            s_wcsnncpy(pwcMedlineFirstAuthorGlobal, pwcDataPtr, PRS_MEDLINE_AUTHOR_LENGTH_MAXIMUM + 1);
        }
    
        /* Convert spaces to underscores */
        for ( pwcLinePtr = pwcDataPtr; (pwcLinePtr = s_wcschr(pwcLinePtr, L' ')) != NULL; ) {
            *pwcLinePtr = L'_';
        }
        
        /* Dont parse */
        *pbParseLine = false;
    }

    /* Get the MeSH heading */    
    else if ( uiMedlineFieldIDGlobal == PRS_MEDLINE_FIELD_MH_ID ) {
        
        if ( (pwcLinePtr = s_wcschr(pwcDataPtr, L'\\')) != NULL ) {
            for ( ; *pwcLinePtr != L'\0'; pwcLinePtr++ ) {
                *pwcLinePtr = L' ';
            }
        }
    }

    /* Get the source */    
    else if ( uiMedlineFieldIDGlobal == PRS_MEDLINE_FIELD_TA_ID ) {
        
        /* Convert spaces to underscores */
        for ( pwcLinePtr = pwcDataPtr; (pwcLinePtr = s_wcschr(pwcLinePtr, ' ')) != NULL; ) {
            *pwcLinePtr = L'_';
        }
        
        /* Dont parse */
        *pbParseLine = false;
    }


    /* Erase the field identifier */
    for ( pwcLinePtr = pwcLine; pwcLinePtr < pwcDataPtr; pwcLinePtr++ ) {
        *pwcLinePtr = L' ';
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsMedlineDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcMedlineTitleGlobal, pwcMedlineDateGlobal, pwcMedlineFirstAuthorGlobal,
                        pwcMedlineIdentifierGlobal, uiMedlineFieldIDGlobal, ulMedlineAnsiDateGlobal

    Returns:    void

*/
void vPrsMedlineDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsMedlineDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsMedlineDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsMedlineDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsMedlineDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsMedlineDocumentInformationFunction'."); 
    }

    
    /* Add the author */
    if ( bUtlStringsIsWideStringNULL(pwcMedlineFirstAuthorGlobal) == false ) {
        s_wcsnncat(pwcDocumentTitle, pwcMedlineFirstAuthorGlobal, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }

    /* Add the date */
    if ( bUtlStringsIsWideStringNULL(pwcMedlineDateGlobal) == false ) {
        s_wcsnncat(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false) ? L" (" : L"("), SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        s_wcsnncat(pwcDocumentTitle, pwcMedlineDateGlobal, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        s_wcsnncat(pwcDocumentTitle, L") ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
    }
    else {
        if ( bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false ) {
            s_wcsnncat(pwcDocumentTitle, L", ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }
    }

    /* Add the document title */
    s_wcsnncat(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcMedlineTitleGlobal) == false) ? 
            pwcMedlineTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);


    /* Set the document key */
    if ( bUtlStringsIsWideStringNULL(pwcMedlineIdentifierGlobal) == false ) {
        s_wcsnncpy(pwcDocumentKey, pwcMedlineIdentifierGlobal, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
    }
    

    /* Set the date */
    *pulDocumentAnsiDate = ulMedlineAnsiDateGlobal;


    /* Clear the globals */
    pwcMedlineFirstAuthorGlobal[0] = L'\0';
    pwcMedlineDateGlobal[0] = L'\0';
    pwcMedlineTitleGlobal[0] = L'\0';
    pwcMedlineIdentifierGlobal[0] = L'\0';
    uiMedlineFieldIDGlobal = 0;
    ulMedlineAnsiDateGlobal = 0;


    return;

}


/*---------------------------------------------------------------------------*/


/* 
** =======================
** ====  OMIM Format  ====
** =======================
*/


/* OMIM format:
** 
** Documents are separated with a blank line.
** 

*RECORD*
*FIELD* NO
100050
*FIELD* TI
100050 AARSKOG SYNDROME
*FIELD* TX
...


*RECORD*
*FIELD* NO
100070
*FIELD* TI
#100070 AORTIC ANEURYSM, ABDOMINAL
;;AAA;;
AAA1;;
ANEURYSM, ABDOMINAL AORTIC;;
ABDOMINAL AORTIC ANEURYSM
ARTERIOMEGALY, INCLUDED;;
ANEURYSMS, PERIPHERAL, INCLUDED
*FIELD* TX
...

** 
*/


/* Field IDs */
#define PRS_OMIM_FIELD_INVALID_ID       (0)
#define PRS_OMIM_FIELD_NO_ID            (1)
#define PRS_OMIM_FIELD_TI_ID            (2)
#define PRS_OMIM_FIELD_RF_ID            (3)
#define PRS_OMIM_FIELD_CS_ID            (4)
#define PRS_OMIM_FIELD_SA_ID            (5)
#define PRS_OMIM_FIELD_OLDNO_ID         (6)
#define PRS_OMIM_FIELD_ED_ID            (7)
#define PRS_OMIM_FIELD_MN_ID            (8)
#define PRS_OMIM_FIELD_TX_ID            (9)
#define PRS_OMIM_FIELD_CN_ID            (10)
#define PRS_OMIM_FIELD_CD_ID            (11)
#define PRS_OMIM_FIELD_AV_ID            (12)


/* Omim field name/description structure */
struct prsOmimFieldNameDescription {
    unsigned int    uiFieldID;              /* Field ID */
    wchar_t         *pwcFieldName;          /* Field name */
    unsigned int    uiFieldNameLength;      /* Field name length (denormalization) */
    unsigned char   *pucFieldType;          /* Field type */
    unsigned char   *pucFieldOptions;       /* Field options */
    wchar_t         *pwcFieldDescription;   /* Field description */
    boolean         bUnfieldedSearch;       /* Unfielded search field */
};


/* Field descriptions */
static struct prsOmimFieldNameDescription pfndOmimFieldNameDescriptionGlobal[] = 
{
    {PRS_OMIM_FIELD_NO_ID,          L"NO",      2,  NULL,   NULL,   L"MIM Number",          false},
    {PRS_OMIM_FIELD_TI_ID,          L"TI",      2,  NULL,   NULL,   L"Title",               true},
    {PRS_OMIM_FIELD_RF_ID,          L"RF",      2,  NULL,   NULL,   L"References",          false},
    {PRS_OMIM_FIELD_CS_ID,          L"CS",      2,  NULL,   NULL,   L"Clinical Symptoms",   true},
    {PRS_OMIM_FIELD_SA_ID,          L"SA",      2,  NULL,   NULL,   L"Other information",   false},
    {PRS_OMIM_FIELD_OLDNO_ID,       L"OLDNO",   5,  NULL,   NULL,   L"Old MIM Number",      false},
    {PRS_OMIM_FIELD_ED_ID,          L"ED",      2,  NULL,   NULL,   L"Editor",              false},
    {PRS_OMIM_FIELD_MN_ID,          L"MN",      2,  NULL,   NULL,   L"Mini-MIM",            true},
    {PRS_OMIM_FIELD_TX_ID,          L"TX",      2,  NULL,   NULL,   L"Text",                true},
    {PRS_OMIM_FIELD_CN_ID,          L"CN",      2,  NULL,   NULL,   L"Contributors",        false},
    {PRS_OMIM_FIELD_CD_ID,          L"CD",      2,  NULL,   NULL,   L"Creation Date",       false},
    {PRS_OMIM_FIELD_AV_ID,          L"AV",      2,  NULL,   NULL,   L"Allelic Variants",    false},
    {PRS_OMIM_FIELD_INVALID_ID,     NULL,       0,  NULL,   NULL,   NULL,                   false},
};


/* Globals */
static wchar_t pwcOmimTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcOmimNumberGlobal[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {L'\0'};
static boolean bOmimTitleFlagGlobal = false;
static boolean bOmimNumberFlagGlobal = false;
static boolean bOmimDateFlagGlobal = false;
static unsigned int uiOmimFieldIDGlobal = 0;
static unsigned long ulOmimAnsiDateGlobal = 0;


/*

    Function:   bPrsOmimDocumentStartFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the start of a document in a file while we are indexing a 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsOmimDocumentStartFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsOmimDocumentStartFunction'."); 
    }


    if ( (pwcLine[0] == L'*') && (s_wcsncmp(pwcLine, L"*RECORD*", /* s_wcslen(L"*RECORD*") */ 8) == 0) ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsOmimFieldInformationFunction()

    Purpose:    This function gets called when the parser wants to find out
                the name and description of the various field IDs for this
                type of document. This function will get called repeatedly with 
                incrementing field IDs starting at 1. The function should set
                the return pointers to the appropriate field name and field
                description for that field ID and return true. The function
                should return false if the field ID is invalid.

    Parameters: uiFieldID               Field ID for which we want to get the field name
                ppwcFieldName           Return pointer for the field name
                ppucFieldType           Return pointer for the field type
                ppucFieldOptions        Return pointer for the field options
                ppwcFieldDescription    Return pointer for the field description
                pbUnfieldedSearch       Return pointer indicating whether this field is to be included in unfielded searches

    Globals:    none

    Returns:    true if this is a valid field ID, false if not

*/
boolean bPrsOmimFieldInformationFunction
(
    unsigned int uiFieldID,
    wchar_t **ppwcFieldName,
    unsigned char **ppucFieldType, 
    unsigned char **ppucFieldOptions, 
    wchar_t **ppwcFieldDescription,
    boolean *pbUnfieldedSearch
)
{

    /* Check the parameters */
    if ( ppwcFieldName == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldName' parameter passed to 'bPrsOmimFieldInformationFunction'."); 
    }

    if ( ppucFieldType == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldType' parameter passed to 'bPrsOmimFieldInformationFunction'."); 
    }

    if ( ppucFieldOptions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppucFieldOptions' parameter passed to 'bPrsOmimFieldInformationFunction'."); 
    }

    if ( ppwcFieldDescription == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'ppwcFieldDescription' parameter passed to 'bPrsOmimFieldInformationFunction'."); 
    }

    if ( pbUnfieldedSearch == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbUnfieldedSearch' parameter passed to 'bPrsOmimFieldInformationFunction'."); 
    }


    /* Check that the field ID is valid */
    if ( uiFieldID >= (sizeof(pfndOmimFieldNameDescriptionGlobal) / sizeof(struct prsOmimFieldNameDescription)) ) {
        return (false);
    }
    
    /* Set the field name and description */
    ASSERT((pfndOmimFieldNameDescriptionGlobal + (uiFieldID - 1))->uiFieldID == uiFieldID);
    *ppwcFieldName = (pfndOmimFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldName;
    *ppucFieldType = (pfndOmimFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldType;
    *ppucFieldOptions = (pfndOmimFieldNameDescriptionGlobal + (uiFieldID - 1))->pucFieldOptions;
    *ppwcFieldDescription = (pfndOmimFieldNameDescriptionGlobal + (uiFieldID - 1))->pwcFieldDescription;
    *pbUnfieldedSearch = (pfndOmimFieldNameDescriptionGlobal + (uiFieldID - 1))->bUnfieldedSearch;


    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsOmimDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcOmimTitleGlobal, pwcOmimNumberGlobal, bOmimTitleFlagGlobal,
                        bOmimNumberFlagGlobal, bOmimDateFlagGlobal, uiOmimFieldIDGlobal,
                        ulOmimAnsiDateGlobal

    Returns:    void

*/
void vPrsOmimDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{

    wchar_t     *pwcLinePtr = NULL;


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsOmimDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsOmimDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsOmimDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsOmimDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsOmimDocumentLineFunction'."); 
    }
    
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsOmimDocumentLineFunction'."); 
    }
    

    /* Potential new field ID */
    if ( (pwcLine[0] == L'*') && (s_wcsncmp(pwcLine, L"*FIELD* ", /* s_wcslen(L"*FIELD* ") */ 8) == 0) ) {
        
        /* New field ID */
        struct prsOmimFieldNameDescription  *pofndOmimFieldNameDescriptionPtr = pfndOmimFieldNameDescriptionGlobal;

        /* Loop over each field */
        for ( pofndOmimFieldNameDescriptionPtr = pfndOmimFieldNameDescriptionGlobal, uiOmimFieldIDGlobal = 0; 
                pofndOmimFieldNameDescriptionPtr->pwcFieldName != NULL; pofndOmimFieldNameDescriptionPtr++ ) {
            
            /* Is this a potential field? */
            if ( (pwcLine[8] == pofndOmimFieldNameDescriptionPtr->pwcFieldName[0]) &&
                    (s_wcsncmp(pwcLine + /* s_wcslen(L"*FIELD* ") */ 8, pofndOmimFieldNameDescriptionPtr->pwcFieldName, pofndOmimFieldNameDescriptionPtr->uiFieldNameLength) == 0) ) {
                uiOmimFieldIDGlobal = pofndOmimFieldNameDescriptionPtr->uiFieldID;
                break;
            }
        }
    }

    /* Set the field ID */
    *puiFieldID = uiOmimFieldIDGlobal;



    /* If the document title flag is on, they we can collect titles */
    if ( bOmimTitleFlagGlobal == true ) {
        if ( uiOmimFieldIDGlobal != PRS_OMIM_FIELD_TI_ID ) {
            bOmimTitleFlagGlobal = false;
        }
        else {
            if ( bUtlStringsIsWideStringNULL(pwcOmimTitleGlobal) == false ) {
                s_wcsnncat(pwcOmimTitleGlobal, L" ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
            }
            s_wcsnncat(pwcOmimTitleGlobal, pwcLine, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }
    }
    else {
        /* Turn the flag on, this allows us to catch the next line */
        if ( uiOmimFieldIDGlobal == PRS_OMIM_FIELD_TI_ID ) {
            bOmimTitleFlagGlobal = true;
        }
    }


    /* Catch the document number */
    if ( bOmimNumberFlagGlobal == true ) {
        s_wcsnncpy(pwcOmimNumberGlobal, pwcLine, PRS_SHORT_STRING_LENGTH + 1);
        bOmimNumberFlagGlobal = false;
    }
    else {
        /* Turn the flag on, this allows us to catch the next line */
        if ( uiOmimFieldIDGlobal == PRS_OMIM_FIELD_NO_ID ) {
            bOmimNumberFlagGlobal = true;
        }
    }


    /* Catch the date, if it has not already been set */
    if ( (bOmimDateFlagGlobal == true) && (ulOmimAnsiDateGlobal == 0) ) {
        
        /* Get a pointer to the start of the date, default to the line if needed
        ** 'alopez: 06/03/1997'
        */
        if ( (pwcLinePtr = s_wcsstr(pwcLine, L": ")) != NULL ) {

            /* Parse the date, set to 0 on error */
            if ( iUtlDateParseWideDateToAnsiDate(pwcLinePtr + 2, &ulOmimAnsiDateGlobal) != UTL_NoError ) {
                ulOmimAnsiDateGlobal = 0;
            }

            bOmimDateFlagGlobal = false;
        }
    }
    else {
        /* Turn the flag on, this allows us to catch the next line */
        if ( uiOmimFieldIDGlobal == PRS_OMIM_FIELD_ED_ID ) {
            bOmimDateFlagGlobal = true;
        }
    }


    /* Filter out field titles, we dont really want to index them */
    if ( (pwcLine[0] == L'*') && ((s_wcsncmp(pwcLine, L"*FIELD*", /* s_wcslen(L"*FIELD*") */ 7) == 0) || (s_wcsncmp(pwcLine, L"*RECORD*", /* s_wcslen(L"*RECORD*") */ 8) == 0)) ) {

        pwcLinePtr = pwcLine;
        while ( *pwcLinePtr != L'\0' ) {
            *pwcLinePtr = L' ';
            pwcLinePtr++;
        }
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsOmimDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcOmimTitleGlobal, pwcOmimNumberGlobal, bOmimTitleFlagGlobal,
                        bOmimNumberFlagGlobal, bOmimDateFlagGlobal, uiOmimFieldIDGlobal,
                        ulOmimAnsiDateGlobal

    Returns:    void

*/
void vPrsOmimDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsOmimDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsOmimDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsOmimDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsOmimDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsOmimDocumentInformationFunction'."); 
    }


    /* Set the document title */
    s_wcsnncpy(pwcDocumentTitle, ((bUtlStringsIsWideStringNULL(pwcOmimTitleGlobal) == false) ? pwcOmimTitleGlobal : L"No Title"), SPI_TITLE_MAXIMUM_LENGTH + 1);

    /* Set the document key */
    if ( bUtlStringsIsWideStringNULL(pwcOmimNumberGlobal) == false ) {
        s_wcsnncpy(pwcDocumentKey, pwcOmimNumberGlobal, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
    }
    
    /* Set the date */
    *pulDocumentAnsiDate = ulOmimAnsiDateGlobal;


    /* Clear the globals */
    pwcOmimTitleGlobal[0] = L'\0';
    pwcOmimNumberGlobal[0] = L'\0';
    bOmimTitleFlagGlobal = false;
    bOmimNumberFlagGlobal = false;
    bOmimDateFlagGlobal = false;
    uiOmimFieldIDGlobal = 0;
    ulOmimAnsiDateGlobal = 0;


    return;

}


/*---------------------------------------------------------------------------*/



/* 
** =====================
** ===  TREC Format  ===
** =====================
*/

/* TREC format:  
** 
** Each doument is separated by a blank line, and 
** starts with a '<DOC>' and ends with a '</DOC>':
**

<DOC>
<HL>title text</HL>>
<DD>date</DD>
...
</DOC>

<DOC>
<HL>title text</HL>
<DD>date</DD>
...
</DOC>

**
*/

/* Filtered tag structure */
struct prsTrecFilteredTag {
    wchar_t             *pwcTag;        /* Tag */
    unsigned int        uiTagLen;       /* Tag length (optimization) */
};


/* Filtered tags */
static struct prsTrecFilteredTag ptftTrecFilteredTagGlobal[] = 
{
    {L"<IN>",           4},
    {L"<MS>",           4},
    {L"<DESCRIPT>",     10},
    {L"<SUBJECT>",      9},
    {L"<DOCHDR>",       8},
#if defined(PRS_ENABLE_TREC_TAG_FILTERING)
    {L"<DOCNO>",        7},
    {L"<FILEID>",       8},
    {L"<FIRST>",        7},
    {L"<SECOND>",       8},
    {L"<DATELINE>",     10},
    {L"<BYLINE>",       8},
    {L"<NOTE>",         6},
    {L"<UNK>",          5},
    {L"<DOCID>",        7},
    {L"<CENTER>",       8},
    {L"<DATE>",         6},
    {L"<FLD001>",       8},
    {L"<FLD002>",       8},
    {L"<FLD003>",       8},
    {L"<SO>",           4},
    {L"<PROFILE>",      9},
    {L"<PUB>",          5},
    {L"<PAGE>",         6},
    {L"<DD>",           4},
    {L"<G>",            3},
    {L"<GV>",           4},
    {L"<AN>",           4},
    {L"<RE>",           4},
    {L"<NS>",           4},
    {L"<DO>",           4},
    {L"<ST>",           4},
    {L"<JOURNAL>",      9},
    {L"<AUTHOR>",       8},
    {L"<WKU>",          5},
    {L"<SRC>",          5},
    {L"<APN>",          5},
    {L"<APT>",          5},
    {L"<ART>",          5},
    {L"<APD>",          5},
    {L"<ISD>",          5},
    {L"<ACCESS>",       8},
    {L"<CAPTION>",      9},
    {L"<SECTION>",      9},
    {L"<MEMO>",         6},
    {L"<COUNTRY>",      9},
    {L"<CITY>",         6},
    {L"<EDITION>",      9},
    {L"<CODE>",         6},
    {L"<NAME>",         6},
    {L"<PUBDATE>",      9},
    {L"<DAY>",          5},
    {L"<MONTH>",        7},
    {L"<PG.COL>",       8},
    {L"<PUBYEAR>",      9},
    {L"<REGION>",       8},
    {L"<STATE>",        7},
    {L"<WORD.CT>",      9},
    {L"<COPYRGHT>",     10},
    {L"<LIMLEN>",       8},
    {L"<LANGUAGE>",     10},
#endif    /*  defined(PRS_ENABLE_TREC_TAG_FILTERING) */
    {NULL,              0},
};


/* Title tags */
static wchar_t *pwcTrecTitleTagList[] =    
{
    L"<HEAD>",                  /* AP */
    L"<BYLINE>",                /* AP - alternative */
    L"<UNK>",                   /* AP - second alternative */
    L"<HL>",                    /* WSJ */ 
    L"<TITLE>",                 /* ZIFF */
    L"<TITLE>",                 /* SJM, FT, LA */
    L"<TTL>",                   /* Patents, CR */
    L"<TI>",                    /* FBIS */
    L"<title>",                 /* VLC1 */
    L"Subject: ", 
    /* L"<TEXT>",  */           /* if all else fails - DOE, FR */
    NULL
};


static wchar_t pwcTrecTitleGlobal[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
static wchar_t pwcTrecDocNoGlobal[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {L'\0'};
static boolean bTrecInTitleFlagGlobal = false;
static boolean bTrecInDocNoFlagGlobal = false;
static boolean bTrecEraseFlagGlobal = false;
static unsigned int uiTrecTitleTagGlobal = 0;


/*

    Function:   bPrsTrecDocumentStartFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the start of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsTrecDocumentStartFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsTrecDocumentStartFunction'."); 
    }


    if ( (pwcLine[0] == L'<') && (s_wcsncmp(pwcLine, L"<DOC>", /* s_wcslen(L"<DOC>") */ 5) == 0) ) {
        return (true);
    }

    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsTrecDocumentEndFunction()

    Purpose:    Checks every line that is passed to it to see if it is 
                the end of a document in a file while we are indexing that 
                document. Note that a file may contain multiple documents.

    Parameters: pwcLine        Line currently being processed

    Globals:    none

    Returns:    true if the line is an end of document, false if it is not

*/
boolean bPrsTrecDocumentEndFunction
(
    wchar_t *pwcLine
)
{

    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'bPrsTrecDocumentEndFunction'."); 
    }


    if ( (pwcLine[0] == L'<') && (s_wcsncmp(pwcLine, L"</DOC>", /* s_wcslen(L"</DOC>") */ 6) == 0) ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsTrecDocumentLineFunction()

    Purpose:    Checks every line that is passed to it to clean the text, extract
                whatever information is required, determine the field for this line
                and tell the parser whether to parse this line or not. 
                
                Here we pick out the first line as the document title.

    Parameters: pwcLine             Line currently being processed
                puiLanguageID       Return pointer for the language ID to be used for this line (defaults to LNG_LANGUAGE_ANY_ID)
                puiFieldID          Return pointer for the field ID to be used for this line (defaults to 0)
                pbIndexLine         Return pointer telling the parser whether to index this line or not (defaults to true)
                pbParseLine         Return pointer telling the parser whether to parse this line or not (defaults to true)
                pbTermPositions     Return pointer telling the parser whether to send term positions for this line or not (defaults to true)

    Globals:    pwcTrecTitleGlobal, pwcTrecDocNoGlobal, bTrecInTitleFlagGlobal,
                        bTrecInDocNoFlagGlobal, bTrecEraseFlagGlobal, uiTrecTitleTagGlobal

    Returns:    void

*/
void vPrsTrecDocumentLineFunction
(
    wchar_t *pwcLine,
    unsigned int *puiLanguageID,
    unsigned int *puiFieldID,
    boolean *pbIndexLine, 
    boolean *pbParseLine,
    boolean *pbTermPositions
)
{
    wchar_t     *pwcLinePtr = pwcLine;


    /* Check the parameters */
    if ( pwcLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcLine' parameter passed to 'vPrsTrecDocumentLineFunction'."); 
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'vPrsTrecDocumentLineFunction'."); 
    }

    if ( puiFieldID == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'vPrsTrecDocumentLineFunction'."); 
    }

    if ( pbIndexLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbIndexLine' parameter passed to 'vPrsTrecDocumentLineFunction'."); 
    }
    
    if ( pbParseLine == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbParseLine' parameter passed to 'vPrsTrecDocumentLineFunction'."); 
    }
            
    if ( pbTermPositions == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pbTermPositions' parameter passed to 'vPrsTrecDocumentLineFunction'."); 
    }


    /* Check for a document title */
    if ( ((bUtlStringsIsWideStringNULL(pwcTrecTitleGlobal) == true) && (bTrecInTitleFlagGlobal == false)) || (bTrecInTitleFlagGlobal == true) ) {

        /* Check for the document title */
        if ( (bUtlStringsIsWideStringNULL(pwcTrecTitleGlobal) == true) && (bTrecInTitleFlagGlobal == false) ) {
            for ( uiTrecTitleTagGlobal = 0; pwcTrecTitleTagList[uiTrecTitleTagGlobal] != NULL; uiTrecTitleTagGlobal++ ) {
                if ( (pwcLinePtr = s_wcsstr(pwcLine, pwcTrecTitleTagList[uiTrecTitleTagGlobal])) != NULL ) {
                    s_wcsnncpy(pwcTrecTitleGlobal, pwcLinePtr + s_wcslen(pwcTrecTitleTagList[uiTrecTitleTagGlobal]), SPI_TITLE_MAXIMUM_LENGTH + 1);
                    iUtlStringsTrimWideString(pwcTrecTitleGlobal);
                    bTrecInTitleFlagGlobal = true;
                    break;
                }
            }
        }

        if ( bTrecInTitleFlagGlobal == true ) {

            /* Concatenate and trim the document title */
            if ( pwcLinePtr != NULL ) {
                s_wcsnncpy(pwcTrecTitleGlobal, pwcLinePtr + s_wcslen(pwcTrecTitleTagList[uiTrecTitleTagGlobal]), SPI_TITLE_MAXIMUM_LENGTH + 1);
            }
            else {
                s_wcsnncat(pwcTrecTitleGlobal, L" ", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
                s_wcsnncat(pwcTrecTitleGlobal, pwcLine, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1);
            }
            iUtlStringsTrimWideString(pwcTrecTitleGlobal);

            /* Have we reached the end of the document title? */
            if ( (s_wcscmp(pwcTrecTitleTagList[uiTrecTitleTagGlobal], L"<TEXT>") == 0) && ((pwcLinePtr = s_wcsstr(pwcTrecTitleGlobal, L".")) != NULL) ) {
                *(pwcLinePtr + 1) = L'\0';
                bTrecInTitleFlagGlobal = false;
            }
            else if ( (pwcLinePtr = s_wcsstr(pwcTrecTitleGlobal, pwcTrecTitleTagList[uiTrecTitleTagGlobal] + 1)) != NULL ) {
                *(pwcLinePtr - 2) = L'\0';
                bTrecInTitleFlagGlobal = false;
            }
        }
    }



    /* Check for a document number */
    if ( ((bUtlStringsIsWideStringNULL(pwcTrecDocNoGlobal) == true) && (bTrecInDocNoFlagGlobal == false)) || (bTrecInDocNoFlagGlobal == true) ) {

        /* Check for document number */
        if ( (bUtlStringsIsWideStringNULL(pwcTrecDocNoGlobal) == true) && (bTrecInDocNoFlagGlobal == false) ) {
            if ( (pwcLine[0] == L'<') && (s_wcsncmp(pwcLine, L"<DOCNO>", /* s_wcslen(L"<DOCNO>") */ 7) == 0) ) {
                s_wcsnncpy(pwcTrecDocNoGlobal, pwcLine + /* s_wcslen(L"<DOCNO>") */ 7, PRS_SHORT_STRING_LENGTH + 1);
                iUtlStringsTrimWideString(pwcTrecDocNoGlobal);
                bTrecInDocNoFlagGlobal = true;
            }
        }

        /* Check for rest of the document number */
        else if ( bTrecInDocNoFlagGlobal == true ) {
            s_wcsnncat(pwcTrecDocNoGlobal, pwcLine, PRS_SHORT_STRING_LENGTH, PRS_SHORT_STRING_LENGTH + 1);
            iUtlStringsTrimWideString(pwcTrecDocNoGlobal);
        }

        /* Have we reached the end of the document number, if so we terminate it */
        if ( (pwcLinePtr = s_wcschr(pwcTrecDocNoGlobal, L'<')) != NULL ) {
            *pwcLinePtr = L'\0';
            bTrecInDocNoFlagGlobal = false;
        }
    }



    /* Is there a tag to erase in this line? */
    if ( (bTrecEraseFlagGlobal == false) && (pwcLine[0] == '<') ) {
    
        struct prsTrecFilteredTag   *ptftTrecFilteredTagPtr = NULL;
    
        for ( ptftTrecFilteredTagPtr = ptftTrecFilteredTagGlobal; ptftTrecFilteredTagPtr->pwcTag != NULL; ptftTrecFilteredTagPtr++ ) {
            if ( (pwcLine[0] == ptftTrecFilteredTagPtr->pwcTag[0]) && (s_wcsncmp(pwcLine, ptftTrecFilteredTagPtr->pwcTag, ptftTrecFilteredTagPtr->uiTagLen) == 0) ) {
                bTrecEraseFlagGlobal = true;
                break;
            }
        }
    }    


    /* We have a tag to erase */
    if ( bTrecEraseFlagGlobal == true ) {

        /* Is there an end tag on this line? */
        if ( s_wcsstr(pwcLine, L"</") != NULL ) {
            /* There is so we switch off the erase flag */
            bTrecEraseFlagGlobal = false;
        }
        else {
            /* Check the next line since the tag is not here */
            bTrecEraseFlagGlobal = true;
        }


        /* Erase the line */
        if ( (pwcLinePtr = pwcLine) != NULL ) {
            while ( *pwcLinePtr != L'\0' ) {
                *pwcLinePtr = L' ';
                pwcLinePtr++;
            }
        }
        return;
    }


    /* Erase any tags present on this line */
    if ( (pwcLinePtr = pwcLine) != NULL ) {
        while ( *pwcLinePtr != L'\0' ) {
            if ( *pwcLinePtr == L'<' ) {
                do {
                    *pwcLinePtr = L' ';
                    pwcLinePtr++;
                }
                while ( (*pwcLinePtr != L'>') && (*pwcLinePtr != L'\0') );
                if ( *pwcLinePtr != L'\0' ) {
                    *pwcLinePtr = L' ';
                }
            }
            pwcLinePtr++;
        }
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vPrsTrecDocumentInformationFunction()

    Purpose:    This gets called after the parser determines that a document end 
                has been encountered. 

    Parameters: pucFilePath             Path name of the file currently being parsed (optional)
                pwcDocumentTitle        Return pointer for the document title
                pwcDocumentKey          Return pointer for the document key
                pwcDocumentUrl          Return pointer for the document url
                puiDocumentRank         Return pointer for the rank
                pulDocumentAnsiDate     Return pointer for the ansi date

    Globals:    pwcTrecTitleGlobal, pwcTrecDocNoGlobal, bTrecInTitleFlagGlobal,
                        bTrecInDocNoFlagGlobal, bTrecEraseFlagGlobal, uiTrecTitleTagGlobal

    Returns:    void

*/
void vPrsTrecDocumentInformationFunction
(
    unsigned char *pucFilePath,
    wchar_t *pwcDocumentTitle,
    wchar_t *pwcDocumentKey,
    wchar_t *pwcDocumentUrl,
    unsigned int *puiDocumentRank,
    unsigned long *pulDocumentAnsiDate
)
{

    wchar_t     *pwcLinePtr = NULL;


    /* Check the parameters */
    if ( pwcDocumentTitle == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentTitle' parameter passed to 'vPrsTrecDocumentInformationFunction'."); 
    }

    if ( pwcDocumentKey == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentKey' parameter passed to 'vPrsTrecDocumentInformationFunction'."); 
    }

    if ( pwcDocumentUrl == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pwcDocumentUrl' parameter passed to 'vPrsTrecDocumentInformationFunction'."); 
    }

    if ( puiDocumentRank == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'puiDocumentRank' parameter passed to 'vPrsTrecDocumentInformationFunction'."); 
    }

    if ( pulDocumentAnsiDate == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Null 'pulDocumentAnsiDate' parameter passed to 'vPrsTrecDocumentInformationFunction'."); 
    }


    /* Trim the header */
    if ( (pwcLinePtr = pwcTrecTitleGlobal) != NULL ) {
        while ( *pwcLinePtr != L'\0' ) {
            if ( *pwcLinePtr == L'<' ) {
                do {
                    *pwcLinePtr = L' ';
                    pwcLinePtr++;
                }
                while ( (*pwcLinePtr != L'>') && (*pwcLinePtr != L'\0'));
                if ( *pwcLinePtr != L'\0' ) {
                    *pwcLinePtr = L' ';
                }
            }
            pwcLinePtr++;
        }
    }
    iUtlStringsTrimWideString(pwcTrecTitleGlobal);


    /* Trim the document number */
    iUtlStringsTrimWideString(pwcTrecDocNoGlobal);




    /* Create the header */
    s_wcsnncpy(pwcDocumentTitle, pwcTrecDocNoGlobal, SPI_TITLE_MAXIMUM_LENGTH + 1);
/*     s_wcsnncat(pwcDocumentTitle, (bUtlStringsIsWideStringNULL(pwcDocumentTitle) == false) ? L" - " : L"", SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1); */
/*     s_wcsnncat(pwcDocumentTitle, pwcTrecTitleGlobal, SPI_TITLE_MAXIMUM_LENGTH, SPI_TITLE_MAXIMUM_LENGTH + 1); */

    /* Write out the document key */
    if ( bUtlStringsIsWideStringNULL(pwcTrecDocNoGlobal) == false ) {
        s_wcsnncpy(pwcDocumentKey, pwcTrecDocNoGlobal, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
    }


    /* Clear the globals */
    pwcTrecTitleGlobal[0] = L'\0';
    pwcTrecDocNoGlobal[0] = L'\0';
    bTrecInTitleFlagGlobal = false;
    bTrecInDocNoFlagGlobal = false;
    bTrecEraseFlagGlobal = false;
    uiTrecTitleTagGlobal = 0;


    return;

}


/*---------------------------------------------------------------------------*/
