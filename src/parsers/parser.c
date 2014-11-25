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

    Module:     parser.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This implements the guts of the parser. It will parse every file
                presented and put out a parser stream out on stdout. Errors and
                debugging output go to stderr.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.parsers.parser"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Replacement character for bad unicode */
#define PRS_UNICODE_REPLACEMENT_CHARACTER       (unsigned char)' '


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iPrsParseTextLine (struct prsParser *pppPrsParser, struct prsFormat *ppfPrsFormat,
    FILE *pfIndexerFile, wchar_t *pwcLineStartPtr, wchar_t *pwcLineEndPtr, unsigned int uiLanguageID, 
    unsigned int uiFieldID, unsigned int *puiTermPosition);

static int iPrsSendTermToIndexer (struct prsParser *pppPrsParser, struct prsFormat *ppfPrsFormat, 
    FILE *pfIndexerFile, wchar_t *pwcTermStartPtr, wchar_t *pwcTermEndPtr, 
    unsigned int uiTermPosition, unsigned int uiFieldID);

static int iPrsFinishDocument (struct prsParser *pppPrsParser, struct prsFormat *ppfPrsFormat,
    FILE *pfIndexerFile, unsigned char *pucFilePath, off_t zStartOffset, off_t zEndOffset,
    unsigned int uiDocumentTermCount);


/*---------------------------------------------------------------------------*/


/*

    Function:   iPrsParseTextFile()

    Purpose:    Adds term to the index for a given file.

    Parameters: pppPrsParser    structure containing the various parser options 
                                we use to parse the data
                ppfPrsFormat    file profile pointing to the various file options 
                                we use to parse the data
                pucFilePath     file path to be parsed
                pfInputFile     data stream to be parsed
                pfIndexerFile   structured index stream output file
                pfDataLength    return pointer for the data length parsed

    Globals:    none

    Returns:    0 on success, -1 on error

*/
int iPrsParseTextFile
(
    struct prsParser *pppPrsParser,
    struct prsFormat *ppfPrsFormat,
    unsigned char *pucFilePath,
    FILE *pfInputFile,
    FILE *pfIndexerFile,
    float *pfDataLength
)
{

    int             iError = UTL_NoError;
    int             iStatus = 0;
    FILE            *pfLocalFile = NULL;
    unsigned char   *pucLine = NULL;
    unsigned char   *pucNormalizedLine = NULL;
    wchar_t         *pwcLine = NULL;
    unsigned int    uiLineLength = 0;
    unsigned int    ulLineCapacity = 0;
    unsigned int    uiNormalizedLineCapacity = 0;
    wchar_t         *pwcParseLinePtr = NULL;
    off_t           zCurrentOffset = 0;
    off_t           zStartOffset = 0;
    off_t           zEndOffset = 0;
    unsigned int    uiTermPosition = 0;
    unsigned char   pucTrueFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};

    unsigned int    uiLanguageID = LNG_LANGUAGE_ANY_ID;
    unsigned int    uiFieldID = 0;
    boolean         bIndexLine = false;
    boolean         bParseLine = false;
    boolean         bTermPositions = false;

    boolean         bIndexing = false;
    boolean         bSetStartCharacter = true;
    boolean         bFinishDocument = false;

    float           fLineLength = 0;
    float           fDataLength = 0;


    /* Check the parameters */
    if ( pppPrsParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pppPrsParser' parameter passed to 'iPrsParseTextFile'."); 
        return (-1);
    }

    if ( ppfPrsFormat == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppfPrsFormat' parameter passed to 'iPrsParseTextFile'."); 
        return (-1);
    }

    if ( pfIndexerFile == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pfIndexerFile' parameter passed to 'iPrsParseTextFile'."); 
        return (-1);
    }

    if ( (bUtlStringsIsStringNULL(pucFilePath) == true) && (pfInputFile == NULL) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucFilePath' &'pfInputFile' parameters passed to 'iPrsParseTextFile'."); 
        return (-1);
    }

    if ( (bUtlStringsIsStringNULL(pucFilePath) == false) && (pfInputFile != NULL) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucFilePath' &'pfInputFile' parameters passed to 'iPrsParseTextFile'."); 
        return (-1);
    }

    if ( pfDataLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pfDataLength' parameter passed to 'iPrsParseTextFile'."); 
        return (-1);
    }


    /* Preset the data length return pointer */
    *pfDataLength = 0;
        

    /* Open the file path if it was provided */
    if ( bUtlStringsIsStringNULL(pucFilePath) == false ) {

        /* Get the real file path of this file */
        if ( (iError = iUtlFileGetTruePath(pucFilePath, pucTrueFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the true path of the file path: '%s', utl error: %d.", pucFilePath, iError);
            iStatus = -1;
            goto bailFromlPrsParseTextFile;
        }
    
        /* Open the file to index */
        if ( (pfLocalFile = s_fopen(pucTrueFilePath, "r")) == NULL ) {
            /* Send a message 
            ** M    message
            */
            if ( fprintf(pfIndexerFile, "M File: '%s' could not be opened.\n", pucTrueFilePath) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
            }
            iStatus = -1;
            goto bailFromlPrsParseTextFile;
        }

        /* Send a message to the indexer
        ** M    (Message to be displayed by the indexer)
        */
        if ( pppPrsParser->bSuppressMessages == false ) {
            if ( fprintf(pfIndexerFile, "M Parsing file: '%s'.\n", pucTrueFilePath) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
            }
        }
    }
    /* Otherwise read from the input file */ 
    else if ( pfInputFile != NULL ) {
        pfLocalFile = pfInputFile;
    }
    /* Otherwise croak */
    else {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find data either from a file path or an input file stream");
    }


    
    /* Initialize for the first document */
    zCurrentOffset = 0;
    zStartOffset = 0;
    zEndOffset = 0;
    uiTermPosition = 1;

    bIndexing = false;
    bSetStartCharacter = true;
    bFinishDocument = false;


    /* Loop around reading every line in the file */
    while ( true ) {
    
        boolean         bEoF = false;
        unsigned char   *pucLineEndPtr = NULL;


        /* Reset the line to an empty string, note that pucLine is dynamically
        ** allocated so we make sure that it is allocated before doing so 
        */
        if ( pucLine != NULL ) {
            pucLine[0] = '\0';
        }

        /* Reset the line length */
        uiLineLength = 0;

        /* This ugly little loop reads stuff from the file we are indexing making sure that
        ** it reads an entire actual line (ie not limited by the fact that fgets() can only 
        ** read fixed length buffers) and erases any end of line characters
        */
        do {

            /* Allocate/reallocate lines if needed */
            if ( (pucLine == NULL) || (ulLineCapacity == 0) || ((uiLineLength + BUFSIZ) > ulLineCapacity) ) {

                unsigned char   *pucLinePtr = NULL;


                /* Extend the line */
                if ( (pucLinePtr = (unsigned char *)s_realloc(pucLine, (size_t)(sizeof(unsigned char) * (ulLineCapacity + BUFSIZ)))) == NULL ) {
                    iStatus = -1;
                    goto bailFromlPrsParseTextFile;
                }

                /* Hand over the pointer */
                pucLine = pucLinePtr;

                /* Clear the new allocation */
                s_memset(pucLine + ulLineCapacity, 0, BUFSIZ);
                
                /* Set the line capacity */
                ulLineCapacity += BUFSIZ;

                
                /* Extend the normalized line if normalization is supported */
                if ( pppPrsParser->pvLngUnicodeNormalizer != NULL ) {
                
                    unsigned char    *pucNormalizedLinePtr = NULL;
                    
                    /* Set the normalized line capacity */
                    uiNormalizedLineCapacity = ulLineCapacity * LNG_UNICODE_NORMALIZATION_EXPANSION_MULTIPLIER_MAX;

                    /* Extend the normalized line */
                    if ( (pucNormalizedLinePtr = (unsigned char *)s_realloc(pucNormalizedLine, (size_t)(sizeof(unsigned char) * uiNormalizedLineCapacity))) == NULL ) {
                        iStatus = -1;
                        goto bailFromlPrsParseTextFile;
                    }
    
                    /* Hand over the pointer */
                    pucNormalizedLine = pucNormalizedLinePtr;
                }

                
                /* Extend the wide character line */
                {
                    wchar_t     *pwcLinePtr = NULL;
                
                    /* Extend the wide character line */
                    if ( (pwcLinePtr = (wchar_t *)s_realloc(pwcLine, (size_t)(sizeof(wchar_t) * ulLineCapacity))) == NULL ) {
                        iStatus = -1;
                        goto bailFromlPrsParseTextFile;
                    }
    
                    /* Hand over the pointer */
                    pwcLine = pwcLinePtr;
                }
            }


            /* Read the next line chunk, appending to the current line and setting the end of file flag */
            bEoF = (s_fgets(pucLine + uiLineLength, ulLineCapacity - uiLineLength, pfLocalFile) == NULL) ? true : false;

            /* Get the new line length here for optimization */
            uiLineLength = s_strlen(pucLine);

            /* Erase the trailing new line - be platform sensitive, handle PC first, then Unix and Mac  */
            if ( (uiLineLength >= 2) && (pucLine[uiLineLength - 2] == '\r') ) {
                pucLineEndPtr = pucLine + (uiLineLength - 2);
                uiLineLength -= 2;
            }
            else if ( (uiLineLength >= 1) && ((pucLine[uiLineLength - 1] == '\n') || (pucLine[uiLineLength - 1] == '\r')) ) {
                pucLineEndPtr = pucLine + (uiLineLength - 1);
                uiLineLength -= 1;
            }
            else if ( (uiLineLength >= 1) && (bEoF == true) ) {
                pucLineEndPtr = pucLine + uiLineLength;
            }
            else {
                pucLineEndPtr = NULL;
            }

            if ( pucLineEndPtr != NULL ) {
                *pucLineEndPtr = '\0';
            }

        } while ( (pucLineEndPtr == NULL) && (bEoF == false) );


        /* Increment the line length */
        fLineLength++;

        /* Increment the data length */
        fDataLength += uiLineLength;
        

        /* Set the current character */
        zCurrentOffset += uiLineLength;


        /* Check to see if we have reached the end of the file, if we have we finish the document and bail */
        if ( (bEoF == true) && (pucLineEndPtr == NULL) ) {
            
            /* Finish the document if we are indexing */
            if ( bIndexing == true ) {
                
                /* Finish the document */
                if ( iPrsFinishDocument(pppPrsParser, ppfPrsFormat, pfIndexerFile, pucTrueFilePath, zStartOffset, zCurrentOffset - 1, uiTermPosition - 1) != 0 ) {
                    iStatus = -1;
                    goto bailFromlPrsParseTextFile;
                }
            }
            
            break;
        }


        /* Reset the wide character line */
        if ( pwcLine != NULL ) {
            pwcLine[0] = L'\0';
        }
        
        /* Set the parse line pointer */
        pwcParseLinePtr = pwcLine;

        /* Convert the line from its native character set to wide characters */
        if ( bUtlStringsIsStringNULL(pucLine) == false ) {
            
            /* Set the line pointer */
            unsigned char   *pucLinePtr = pucLine;
            
            /* Use a separate variable for the line length */
            unsigned int    uiLineAllocatedByteLength = ulLineCapacity * sizeof(wchar_t);


            /* Clean the unicode if requested */
            if ( pppPrsParser->bCleanUnicode == true ) {
                
                /* Clean the unicode */
                if ( (iError = iLngUnicodeCleanUtf8String(pucLinePtr, PRS_UNICODE_REPLACEMENT_CHARACTER)) != LNG_NoError ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to clean the string, lng error: %d, string: '%s'", iError, pucLinePtr);
                }
            }


            /* Normalize the line if the unicode normalizer is enabled */
            if ( pppPrsParser->pvLngUnicodeNormalizer != NULL ) {
                
                /* Normalize the line */
                if ( (iError = iLngUnicodeNormalizeString(pppPrsParser->pvLngUnicodeNormalizer, pucLinePtr, uiLineLength, &pucNormalizedLine, &uiNormalizedLineCapacity)) != LNG_NoError ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to normalize the string, lng error: %d, string: '%s'", iError, pucLinePtr);
                }
                
                /* Set the line pointer */
                pucLinePtr = pucNormalizedLine;
                uiLineLength = s_strlen(pucLinePtr);
            }


            /* Convert the line from utf-8 to wide character, note the conversion error handling */
            if ( (iError = iLngConverterConvertString(pppPrsParser->pvLngConverter, ((pppPrsParser->bSkipUnicodeErrors == true) ? LNG_CONVERTER_SKIP_ON_ERROR : LNG_CONVERTER_RETURN_ON_ERROR), 
                    pucLinePtr, uiLineLength, (unsigned char **)&pwcLine, &uiLineAllocatedByteLength)) != LNG_NoError ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to convert the string from utf-8 set to wide characters, lng error: %d, string: '%s'", iError, pucLinePtr);
            }

            /* Set the parse line pointer */
            pwcParseLinePtr = pwcLine;
        }


        /* Check for a document start if a start function was supplied */
        if ( (ppfPrsFormat->bPrsDocumentStartFunction != NULL) && (ppfPrsFormat->bPrsDocumentStartFunction(pwcParseLinePtr) == true) ) {
            
            /* We need to finish the current document if we are currently indexing */
            if ( bIndexing == true ) {

                /* Set the end character if it has not been set, this occurs if we have 
                ** both a start function and an end function, and we detect a new document
                ** start without having first detected a document end
                */
                if ( zEndOffset <= zStartOffset ) {
                    zEndOffset = (zCurrentOffset - uiLineLength) - 1;
                }
                
                /* Finish the document, we do this here because we dont want to process the current line, it belongs to the new document */
                if ( iPrsFinishDocument(pppPrsParser, ppfPrsFormat, pfIndexerFile, pucTrueFilePath, zStartOffset, zEndOffset, uiTermPosition - 1) != 0 ) {
                    iStatus = -1;
                    goto bailFromlPrsParseTextFile;
                }

                /* Check to see if we have reached the document count limit and bail if we have */
                if ( (pppPrsParser->uiDocumentCountMax > 0) && (pppPrsParser->uiDocumentCount >= pppPrsParser->uiDocumentCountMax) ) {
                    break;
                }

                /* Initialize for the next document */
                uiTermPosition = 1;
            }
            else {
                /* Turn on indexing */
                bIndexing = true;
            }
            
            /* Set the start character */
            zStartOffset = zCurrentOffset - uiLineLength;
            bSetStartCharacter = false;
        }

        /* We are always indexing if there is no start function, because if we
        ** reach the end of a document, we are automatically indexing the next one
        */
        else if ( ppfPrsFormat->bPrsDocumentStartFunction == NULL ) {
            
            /* Make sure indexing is turned on */
            bIndexing = true;
            
            /* Set the start character if needed */
            if ( bSetStartCharacter == true ) {
                zStartOffset = zCurrentOffset - uiLineLength;
                bSetStartCharacter = false;
            }
        }

        
        /* Check for a document end if an end function was supplied */ 
        if ( (ppfPrsFormat->bPrsDocumentEndFunction != NULL) && (ppfPrsFormat->bPrsDocumentEndFunction(pwcParseLinePtr) == true) && (fLineLength > 1) ) {
            
            /* Finish the document if we run into a document end */ 
            if ( bIndexing == true ) {
            
                /* Request that we set a new start character, finish the document, and turn off indexing */
                bSetStartCharacter = true;
                bFinishDocument = true;
                bIndexing = false;
                
                /* Set the end character */
                zEndOffset = zCurrentOffset - 1;
            }
        }
        
        /* No end function */
        else if ( ppfPrsFormat->bPrsDocumentEndFunction == NULL ) {
            /* Set the end character */
            zEndOffset = zCurrentOffset - 1;
        }



        /* Just loop to read the next line if we are not indexing and we dont need to finish a document */
        if ( (bIndexing == false) && (bFinishDocument == false) ) {
            continue;
        }



        /* Clear the parameters */
        uiLanguageID = LNG_LANGUAGE_ANY_ID;
        uiFieldID = 0;
        bIndexLine = true;
        bParseLine = true;
        bTermPositions = true;

        /* Process the document line */
        if ( ppfPrsFormat->vPrsDocumentLineFunction != NULL ) {
            ppfPrsFormat->vPrsDocumentLineFunction(pwcParseLinePtr, &uiLanguageID, &uiFieldID, &bIndexLine, &bParseLine, &bTermPositions);
        }


        /* Index the contents of the line */
        if ( bIndexLine == true ) {

            /* Parse the line */
            if ( bParseLine == true ) {

                /* Parse the text line */
                if ( iPrsParseTextLine(pppPrsParser, ppfPrsFormat, pfIndexerFile, pwcParseLinePtr, pwcParseLinePtr + s_wcslen(pwcParseLinePtr), 
                        uiLanguageID, uiFieldID, (bTermPositions == true) ? &uiTermPosition : NULL) != 0 ) {
                    if ( fprintf(pfIndexerFile, "M Failed to add the document terms.\n") < 0 ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
                    }
                }
            }
            /* Get a term from the line and add */
            else {

                wchar_t     *pwcLinePtr = NULL;


                /* Find the start of the term */
                for ( pwcLinePtr = pwcParseLinePtr; *pwcLinePtr != L'\0'; pwcLinePtr++ ) { 
                    if ( iswalnum(*pwcLinePtr) != 0 ) {
                        break;
                    }
                }


                /* Is the term long enough to add? */
                if ( s_wcslen(pwcLinePtr) >= pppPrsParser->uiTermLengthMinimum ) {
    
                    /* Send the language line if the language is known */
                    if ( uiLanguageID != LNG_LANGUAGE_ANY_ID ) {

                        unsigned char   pucLanguageCode[LNG_LANGUAGE_CODE_LENGTH + 1] = {'\0'};
                        
                        if ( iLngGetLanguageCodeFromID(uiLanguageID, pucLanguageCode, LNG_LANGUAGE_CODE_LENGTH + 1) == LNG_NoError ) {
                
                            /* Send the language
                            ** L language{A}
                            */
                            if ( fprintf(pfIndexerFile, "L %s\n", pucLanguageCode) < 0 ) {
                                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a language line to the indexer");
                            }
                        }
                    }

                    /* Send the term to the indexer */
                    if ( iPrsSendTermToIndexer(pppPrsParser, ppfPrsFormat, pfIndexerFile, pwcLinePtr, pwcLinePtr + s_wcslen(pwcLinePtr), 
                            uiFieldID, (bTermPositions == true) ? uiTermPosition : 0) != 0 ) {
                        if ( fprintf(pfIndexerFile, "M Failed to add a document term.\n") < 0 ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
                        }
                    }
                    
                    /* Increment the term position */
                    if ( bTermPositions == true ) {
                        uiTermPosition++;
                    }
                }
            }
        }

    
        /* Finish the document if we were requested to do so */    
        if ( bFinishDocument == true ) {

            /* Finish the current document */
            if ( iPrsFinishDocument(pppPrsParser, ppfPrsFormat, pfIndexerFile, pucTrueFilePath, zStartOffset, zEndOffset, uiTermPosition - 1) != 0 ) {
                if ( fprintf(pfIndexerFile, "M Failed to finish adding a document.\n") < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
                }
            }
    
            /* Check to see if we have reached the document count limit */
            if ( (pppPrsParser->uiDocumentCountMax > 0) && (pppPrsParser->uiDocumentCount >= pppPrsParser->uiDocumentCountMax) ) {
                break;
            }
            
            /* Initialize for the next document */
            uiTermPosition = 1;
    
            /* Turn off the flag requesting that we finish the document */
            bFinishDocument = false;
        }
        

    }    /* while (true) */



    /* Bail label */
    bailFromlPrsParseTextFile:
    

    /* Close the file descriptor if we opened it ourselves */
    if ( bUtlStringsIsStringNULL(pucFilePath) == false ) {
        s_fclose(pfLocalFile);
    }

    /* Free the line pointers */
    s_free(pucLine);
    s_free(pucNormalizedLine);
    s_free(pwcLine);

    
    /* Increment the data length return pointer */
    *pfDataLength += fDataLength;


    return (iStatus);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iPrsParseTextLine()

    Purpose:    Adds the terms in the line parameter to the index. Note that
                this function effectively destroys the line as it parses it,
                so if you want to save it for a later date, make a copy.

    Parameters: pppPrsParser        structure containing the various parser options 
                                    we use to parse the data
                ppfPrsFormat        file profile pointing to the various file options 
                                    we use to parse the data
                pfIndexerFile       file descriptor to which we send the structured index stream
                pwcLineStartPtr     pointer to the line start
                pwcLineEndPtr       pointer to the line end
                uiLanguageID        language ID
                uiFieldID           field ID
                puiTermPosition     two-way pointer for the term position (set to NULL for no term position)

    Globals:    none

    Returns:    0 on success, -1 on error

*/
static int iPrsParseTextLine
(
    struct prsParser *pppPrsParser,
    struct prsFormat *ppfPrsFormat,
    FILE *pfIndexerFile,
    wchar_t *pwcLineStartPtr,
    wchar_t *pwcLineEndPtr,
    unsigned int uiLanguageID,
    unsigned int uiFieldID,
    unsigned int *puiTermPosition
)
{

    int     iError = UTL_NoError;

    
    ASSERT(pppPrsParser != NULL);
    ASSERT(ppfPrsFormat != NULL);
    ASSERT(pfIndexerFile != NULL);
    ASSERT(pwcLineStartPtr != NULL);
    ASSERT(pwcLineEndPtr != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT(uiFieldID >= 0);


    /* Skip parsing the text line if it is 0 bytes long */
    if ( pwcLineStartPtr == pwcLineEndPtr ) {
        return (0);
    }


    /* Parse the line, note that we use the language ID if it is known, otherwise we fall back to the default language ID */
    if ( (iError = iLngTokenizerParseString(pppPrsParser->pvLngTokenizer, (uiLanguageID != LNG_LANGUAGE_ANY_ID) ? uiLanguageID : pppPrsParser->uiLanguageID, 
            pwcLineStartPtr, pwcLineEndPtr - pwcLineStartPtr)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse the string, lng error: %d, string: '%lls'.", iError, pwcLineStartPtr);
        return (-1);
    }


    /* Send the language line if the language is known */
    if ( uiLanguageID != LNG_LANGUAGE_ANY_ID ) {

        unsigned char    pucLanguageCode[LNG_LANGUAGE_CODE_LENGTH + 1] = {'\0'};

        if ( iLngGetLanguageCodeFromID(uiLanguageID, pucLanguageCode, LNG_LANGUAGE_CODE_LENGTH + 1) == LNG_NoError ) {

            /* Send the language
            ** L language{A}
            */
            if ( fprintf(pfIndexerFile, "L %s\n", pucLanguageCode) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a language line to the indexer");
            }
        }
    }


    /* Process tokens, control the loop from within */
    while ( true ) {
    
        wchar_t     *pwcTermStartPtr = NULL;
        wchar_t     *pwcTermEndPtr = NULL;

        /* Get the next token */
        if ( (iError = iLngTokenizerGetToken(pppPrsParser->pvLngTokenizer, &pwcTermStartPtr, &pwcTermEndPtr)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a token from the string, lng error: %d, string: '%lls'.", iError, pwcLineStartPtr);
            return (-1);
        }

        /* Break if this was the last token */
        if ( pwcTermStartPtr == NULL ) {
            break;
        }

        /* Is the term long enough to add? */
        if ( (pwcTermEndPtr - pwcTermStartPtr) >= pppPrsParser->uiTermLengthMinimum ) {

            /* Send the term to the indexer */
            if ( iPrsSendTermToIndexer(pppPrsParser, ppfPrsFormat, pfIndexerFile, pwcTermStartPtr, pwcTermEndPtr, uiFieldID,
                    (puiTermPosition != NULL) ? *puiTermPosition : 0) != 0 ) {
                return (-1);
            }
        }

        /* Process components, control the loop from within */
        while ( true ) {
        
            wchar_t     *pwcComponentStartPtr = NULL;
            wchar_t     *pwcComponentEndPtr = NULL;
        
            /* Get the next component for this token */
            if ( (iError = iLngTokenizerGetComponent(pppPrsParser->pvLngTokenizer, &pwcComponentStartPtr, &pwcComponentEndPtr)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the string line, lng error: %d, string: '%lls'.", iError, pwcLineStartPtr);
                return (-1);
            }

            /* Break if this was the last component */
            if ( pwcComponentStartPtr == NULL ) {
                break;
            }

            /* Is the component long enough to add? */
            if ( (pwcComponentEndPtr - pwcComponentStartPtr) >= pppPrsParser->uiTermLengthMinimum ) {

                /* Send the component to the indexer */
                if ( iPrsSendTermToIndexer(pppPrsParser, ppfPrsFormat, pfIndexerFile, pwcComponentStartPtr, pwcComponentEndPtr, uiFieldID,
                        (puiTermPosition != NULL) ? *puiTermPosition : 0) != 0 ) {
                    return (-1);
                }
            }
        }    

        /* Increment the term position */
        if ( puiTermPosition != NULL ) {
            (*puiTermPosition)++;
        }
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iPrsSendTermToIndexer()

    Purpose:    Sends a term to the indexer.

    Parameters: pppPrsParser        structure containing the various parser options 
                                    we use to parse the data
                ppfPrsFormat        file profile pointing to the various file options 
                                    we use to parse the data
                pfIndexerFile       file descriptor to which we send the structured index stream
                pwcTermStartPtr     pointer to the term start
                pwcTermEndPtr       pointer to the term end
                uiFieldID           field ID
                uiTermPosition      term position

    Globals:    none

    Returns:    0 on success, -1 on error

*/
static int iPrsSendTermToIndexer
(
    struct prsParser *pppPrsParser,
    struct prsFormat *ppfPrsFormat,
    FILE *pfIndexerFile,
    wchar_t *pwcTermStartPtr,
    wchar_t *pwcTermEndPtr,
    unsigned int uiFieldID,
    unsigned int uiTermPosition
)
{

    unsigned int    uiTermLength = pwcTermEndPtr - pwcTermStartPtr;
    wchar_t         wcTermEnd = L'\0';
    

    ASSERT(pppPrsParser != NULL);
    ASSERT(ppfPrsFormat != NULL);
    ASSERT(pfIndexerFile != NULL);
    ASSERT(pwcTermStartPtr != NULL);
    ASSERT(pwcTermEndPtr != NULL);
    ASSERT(uiFieldID >= 0);


    /* Truncate the term if it is too long */
    if ( (pppPrsParser->uiTermLengthMaximum > 0) && (uiTermLength > pppPrsParser->uiTermLengthMaximum) ) {
        pwcTermEndPtr = pwcTermStartPtr + pppPrsParser->uiTermLengthMaximum;
    }

    /* Save the term end character and NULL terminate the string */ 
    wcTermEnd = *pwcTermEndPtr;
    *pwcTermEndPtr = L'\0';

    /* Send the term
    ** T term [term_position{N}] field_id{N}
    */
    if ( uiTermPosition > 0 ) {
        if ( fprintf(pfIndexerFile, "T %ls %u %u\n", pwcTermStartPtr, uiTermPosition, uiFieldID) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a term line to the indexer");
        }
    }
    else {
        if ( fprintf(pfIndexerFile, "T %ls %u\n", pwcTermStartPtr, uiFieldID) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a term line to the indexer");
        }
    }
    
    /* Restore the end character */
    *pwcTermEndPtr = wcTermEnd;


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iPrsFinishDocument()

    Purpose:    Finish the document.

    Parameters: pppPrsParser            structure containing the various parser options 
                                        we use to parse the data
                ppfPrsFormat            file profile pointing to the various file options 
                                        we use to parse the data
                pfIndexerFile           file descriptor to which we send the structured index stream
                pucFilePath             file path of the file currently being parsed (optional)
                zStartOffset            start index in the file
                zEndOffset              end index in the file
                uiDocumentTermCount     document term count

    Globals:    none

    Returns:    0 on success, -1 on error

*/
static int iPrsFinishDocument
(
    struct prsParser *pppPrsParser,
    struct prsFormat *ppfPrsFormat,
    FILE *pfIndexerFile,
    unsigned char *pucFilePath,
    off_t zStartOffset,
    off_t zEndOffset,
    unsigned int uiDocumentTermCount
)
{

    int             iError = UTL_NoError;
    wchar_t         pwcDocumentTitle[SPI_TITLE_MAXIMUM_LENGTH + 1] = {L'\0'};
    wchar_t         pwcDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {L'\0'};
    wchar_t         pwcDocumentUrl[SPI_URL_MAXIMUM_LENGTH + 1] = {L'\0'};
    unsigned int    uiDocumentRank = 0;
    unsigned long   ulDocumentAnsiDate = 0;


    ASSERT(pppPrsParser != NULL);
    ASSERT(ppfPrsFormat != NULL);
    ASSERT(pfIndexerFile != NULL);
    ASSERT(zStartOffset >= 0);
    ASSERT(zEndOffset >= 0);
    ASSERT(zEndOffset >= zStartOffset);
    ASSERT(uiDocumentTermCount >= 0);


    /* Get the document information */
    if ( ppfPrsFormat->vPrsDocumentInformationFunction != NULL ) {
        ppfPrsFormat->vPrsDocumentInformationFunction(pucFilePath, pwcDocumentTitle, pwcDocumentKey, pwcDocumentUrl, &uiDocumentRank, &ulDocumentAnsiDate);
    }


    /* Clean the title and document key from extraneous characters that may have crept in */
    iUtlStringsTrimWideString(pwcDocumentTitle);
    iUtlStringsTrimWideString(pwcDocumentKey);


    /* Use the base name of the file path as the title if we have no title */                
    if ( (bUtlStringsIsWideStringNULL(pwcDocumentTitle) == true) && (bUtlStringsIsStringNULL(pucFilePath) == false) ) {

        unsigned char   *pucFilePathBasePtr = NULL;

        swprintf(pwcDocumentTitle, SPI_TITLE_MAXIMUM_LENGTH + 1, L"%s", 
                (iUtlFileGetPathBase(pucFilePath, &pucFilePathBasePtr) == UTL_NoError) ? pucFilePathBasePtr : pucFilePath);
    }


    /* Send the document title
    ** H    title{A} (Title)
    */
    if ( fprintf(pfIndexerFile, "H %ls\n", pwcDocumentTitle) < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a title line to the indexer");
    }


    /* Send the date
    ** D   date{D} (Document ansi date - optional)
    */
    if ( ulDocumentAnsiDate > 0 ) {
        if ( fprintf(pfIndexerFile, "D %lu\n", ulDocumentAnsiDate) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a date line to the indexer");
        }
    }
    /* There is no date or date/time defined, so we get them from the file we are parsing */
    else if ( bUtlStringsIsStringNULL(pucFilePath) == false ) {

        time_t          tTime = (time_t)0;
        struct tm       tmTime;
        unsigned char   pucDateString[UTL_FILE_PATH_MAX + 1] = {'\0'};

        /* Get the date and time from the file */
        if ( iUtlFileGetPathModificationTimeT(pucFilePath, &tTime) == UTL_NoError ) {

            s_localtime_r(&tTime, &tmTime);

            s_strftime(pucDateString, UTL_FILE_PATH_MAX, "%Y%m%d%H%M%S", &tmTime);
            if ( fprintf(pfIndexerFile, "D %s\n", pucDateString) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a date line to the indexer");
            }
        }
    }


    /* Send the rank
    ** R   rank{N}] (Document rank - optional)
    */
    if ( uiDocumentRank > 0 ) {
        if ( fprintf(pfIndexerFile, "R %u\n", uiDocumentRank) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a rank line to the indexer");
        }
    }


    /* Send the document key if needed
    ** K   key{A} (document key - optional)
    */
    if ( bUtlStringsIsWideStringNULL(pwcDocumentKey) == false ) {
        if ( fprintf(pfIndexerFile, "K %ls\n", pwcDocumentKey) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document key line to the indexer");
        }
    }
    else {

        if ( pppPrsParser->uiDocumentKey > 0 ) {
            if ( fprintf(pfIndexerFile, "K %u\n", pppPrsParser->uiDocumentKey) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document key line to the indexer");
            }
        }
    }


    /* Send the term count
    ** C   count{N} (term count - optional)
    */
    if ( fprintf(pfIndexerFile, "C %u\n", uiDocumentTermCount) < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a term count line to the indexer");
    }


    /* Send the item information
    ** I    itemName{A} mimeType{A} length{N} [filepath{A} start{N} end{N}] [URL{A}] (Document types and source files)
    */
    if ( pppPrsParser->ppaPrsAssociation == NULL ) {

        if ( fprintf(pfIndexerFile, "I %s %s %ld", (pppPrsParser->pucItemName != NULL) ? pppPrsParser->pucItemName : ppfPrsFormat->pucItemName, 
                (pppPrsParser->pucMimeType != NULL) ? pppPrsParser->pucMimeType : ppfPrsFormat->pucMimeType, (long)(zEndOffset - zStartOffset)) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document type line to the indexer");
        }

        if ( (pppPrsParser->bStoreFilePaths == true) && (bUtlStringsIsStringNULL(pucFilePath) == false) ) {
            if ( fprintf(pfIndexerFile, " %s %ld %ld", pucFilePath, (long)zStartOffset, (long)(zEndOffset - 1)) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document type line to the indexer");
            }
        }

        if ( bUtlStringsIsWideStringNULL(pwcDocumentUrl) == false ) {
            if ( fprintf(pfIndexerFile, " %ls", pwcDocumentUrl) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document type line to the indexer");
            }
        }

        if ( fprintf(pfIndexerFile, "\n") < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document type line to the indexer");
        }
    }
    else if ( bUtlStringsIsStringNULL(pucFilePath) == false ) {

        unsigned int    uiI = 0;

        for ( uiI = 0; uiI < pppPrsParser->uiPrsAssociationLength; uiI++ ) {

            unsigned char   pucSavedFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
            unsigned char   *pucSavedFilePathPtr = NULL;

            s_strnncpy(pucSavedFilePath, pucFilePath, UTL_FILE_PATH_MAX + 1);

            /* Strip the current extension, but not the period (add a period if it is not there) */
            if ( (pucSavedFilePathPtr = (unsigned char *)s_strrchr(pucSavedFilePath, '.')) != NULL ) {
                *pucSavedFilePathPtr = '\0';
            }

            /* Append the extension to the file path */
            s_strnncat(pucSavedFilePath, (pppPrsParser->ppaPrsAssociation + uiI)->pucExtension, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);


            /* Check to see if the file is there */
            if ( bUtlFilePathRead(pucSavedFilePath) == true ) {
                
                off_t   zFileLength = 0;

                /* Get the file length */
                if ( (iError = iUtlFileGetFilePathLength(pucSavedFilePath, &zFileLength)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get length of the file: '%s', utl error: %d.", pucSavedFilePath, iError);
                    return (-1);
                }

                if ( fprintf(pfIndexerFile, "I %s %s", (pppPrsParser->ppaPrsAssociation + uiI)->pucItemName, (pppPrsParser->ppaPrsAssociation + uiI)->pucMimeType) < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document type line to the indexer");
                }


                if ( (pppPrsParser->bStoreFilePaths == true) && (bUtlStringsIsStringNULL(pucFilePath) == false) ) {
                    if ( fprintf(pfIndexerFile, " %ld %s 0", (long)zFileLength, pucSavedFilePath) < 0 ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document type line to the indexer");
                    }
                }

                if ( fprintf(pfIndexerFile, " %ld\n", (long)(zFileLength - 1)) < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a document type line to the indexer");
                }
            }
        }
    }


    /* Send end of document marker
    ** E    (End of document marker)
    */
    if ( fprintf(pfIndexerFile, "E\n") < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a end of document line to the indexer");
    }
    
                
    /* Increment the number of documents */
    pppPrsParser->uiDocumentCount++;

    
    /* Check to see if we have reached the document count limit and generate a message if we have */
    if ( (pppPrsParser->uiDocumentCountMax > 0) && (pppPrsParser->uiDocumentCount >= pppPrsParser->uiDocumentCountMax) ) {

        if ( bUtlStringsIsWideStringNULL(pwcDocumentKey) == false ) {
            
            iUtlLogInfo(UTL_LOG_CONTEXT, "Reached maximum number of documents to parse (%u), last document key: '%ls'.", pppPrsParser->uiDocumentCountMax, pwcDocumentKey);
            
            if ( fprintf(pfIndexerFile, "M Reached maximum number of documents to parse (%u), last document key: '%ls'\n", pppPrsParser->uiDocumentCountMax, pwcDocumentKey) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
            }
        }
        else {
            if ( pppPrsParser->uiDocumentKey > 0 ) {

                iUtlLogInfo(UTL_LOG_CONTEXT, "Reached maximum number of documents to parse (%u), last document key: '%u'.", pppPrsParser->uiDocumentCountMax, pppPrsParser->uiDocumentKey);

                if ( fprintf(pfIndexerFile, "M Reached maximum number of documents to parse (%u), last document key: '%u'\n", pppPrsParser->uiDocumentCountMax, pppPrsParser->uiDocumentKey) < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
                }
            }
            else {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Reached maximum number of documents to parse (%u).", pppPrsParser->uiDocumentCountMax);

                if ( fprintf(pfIndexerFile, "M Reached maximum number of documents to parse (%u)\n", pppPrsParser->uiDocumentCountMax) < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
                }
            }
        }
    }
    
    
    /* Increment the document key */
    if ( pppPrsParser->uiDocumentKey > 0 ) {
        pppPrsParser->uiDocumentKey++;
    }


    return (0);

}


/*---------------------------------------------------------------------------*/
