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

    Module:     mpsparser.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This is where the main() is implemented for the parser.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"
#include "lng.h"

#include "parser.h"
#include "functions.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.parsers.mpsparser"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* File name extension separators */
#define PRS_FILENAME_EXTENSION_SEPARATORS       (unsigned char *)","


/* Default language code */
#define PRS_LANGUAGE_CODE_DEFAULT               LNG_LANGUAGE_EN_CODE

/* Default language ID */
#define PRS_LANGUAGE_ID_DEFAULT                 LNG_LANGUAGE_EN_ID

/* Default tokenizer name */
#define PRS_TOKENIZER_NAME_DEFAULT              LNG_TOKENIZER_FSCLT_2_NAME


/* String length */
#define PRS_STRING_LENGTH                       (1024)



/* Default index name */
#define PRS_INDEX_NAME_DEFAULT                  (unsigned char *)"index"


/* Term length minimum */
#define PRS_TERM_LENGTH_MINIMUM                 (1)

/* Term length maximum */
#define PRS_TERM_LENGTH_MAXIMUM                 (1024)

/* Term length minimum default */
#define PRS_TERM_LENGTH_MINIMUM_DEFAULT         (1)

/* Term length maximum default, hey we can manage 'Antidisestablishmentarianism', cool!! */
#define PRS_TERM_LENGTH_MAXIMUM_DEFAULT         (255)


/* Default locale name */
#define PRS_LOCALE_NAME_DEFAULT                 LNG_LOCALE_EN_US_UTF_8_NAME


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Parser formats */
static struct prsFormat ppfPrsFormatListGlobal[] = 
{
    {   (unsigned char *)"text",
        (unsigned char *)"One document per file, first line is title, all subsequent lines are text",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        NULL,
        NULL,
        bPrsTextFieldInformationFunction,
        vPrsTextDocumentLineFunction,
        vPrsTextDocumentInformationFunction
    },
    {   (unsigned char *)"para",
        (unsigned char *)"One document per paragraph, first line is title, all subsequent lines are text",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        NULL,
        bPrsParaDocumentEndFunction,
        bPrsParaFieldInformationFunction,
        vPrsParaDocumentLineFunction,
        vPrsParaDocumentInformationFunction
    },
    {   (unsigned char *)"line",
        (unsigned char *)"One document per file, first line is title, all subsequent lines are terms",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        NULL,
        NULL,
        bPrsLineFieldInformationFunction,
        vPrsLineDocumentLineFunction,
        vPrsLineDocumentInformationFunction
    },
    {   (unsigned char *)"mbox",
        (unsigned char *)"Mbox/mail/mail digest format",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        bPrsMboxDocumentStartFunction,
        NULL,
        bPrsMboxFieldInformationFunction,
        vPrsMboxDocumentLineFunction,
        vPrsMboxDocumentInformationFunction
    },
    {   (unsigned char *)"refer",
        (unsigned char *)"Refer format",
        (unsigned char *)"document",
        (unsigned char *)"text/refer",
        NULL,
        bPrsReferDocumentEndFunction,
        bPrsReferFieldInformationFunction,
        vPrsReferDocumentLineFunction,
        vPrsReferDocumentInformationFunction
    },
    {   (unsigned char *)"mps-text",
        (unsigned char *)"MPS text format",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        NULL,
        bPrsMpsTextDocumentEndFunction,
        bPrsMpsFieldInformationFunction,
        vPrsMpsTextDocumentLineFunction,
        vPrsMpsDocumentInformationFunction
    },
    {   (unsigned char *)"mps-xml",
        (unsigned char *)"MPS xml format",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        bPrsMpsXmlDocumentStartFunction,
        bPrsMpsXmlDocumentEndFunction,
        bPrsMpsFieldInformationFunction,
        vPrsMpsXmlDocumentLineFunction,
        vPrsMpsDocumentInformationFunction
    },
    {   (unsigned char *)"poplar",
        (unsigned char *)"Poplar format",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        NULL,
        bPrsPoplarDocumentEndFunction,
        bPrsPoplarFieldInformationFunction,
        vPrsPoplarDocumentLineFunction,
        vPrsPoplarDocumentInformationFunction
    },
    {   (unsigned char *)"medline",
        (unsigned char *)"Medline format",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        NULL,
        bPrsMedlineDocumentEndFunction,
        bPrsMedlineFieldInformationFunction,
        vPrsMedlineDocumentLineFunction,
        vPrsMedlineDocumentInformationFunction
    },
    {   (unsigned char *)"omim",
        (unsigned char *)"Omim format",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        bPrsOmimDocumentStartFunction,
        NULL,
        bPrsOmimFieldInformationFunction,
        vPrsOmimDocumentLineFunction,
        vPrsOmimDocumentInformationFunction,
    },
    {   (unsigned char *)"trec",
        (unsigned char *)"Trec format",
        (unsigned char *)"document",
        (unsigned char *)"text/plain",
        bPrsTrecDocumentStartFunction,
        bPrsTrecDocumentEndFunction,
        NULL,
        vPrsTrecDocumentLineFunction,
        vPrsTrecDocumentInformationFunction
    },
    {   NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);


static boolean bPrsIsFileParseable (struct prsSelector *ppsPrsSelector, unsigned char *pucFilePath);

static int iPrsParsePath (struct prsSelector *ppsPrsSelector, struct prsParser *pppPrsParser,
        struct prsFormat *ppfPrsFormat, unsigned char *pucPath, FILE *pfIndexerFile, float *pfDataLength);


/*---------------------------------------------------------------------------*/


/*

    Function:   main()

    Purpose:    This function is the main one.

    Parameters: argc,argv

    Globals:    none

    Returns:    int

*/
int main
(
    int argc,
    char *argv[]
)
{

    int                     iError = UTL_NoError;
    unsigned char           *pucCommandPath = NULL;
    unsigned char           *pucNextArgument = NULL;

    struct prsFormat        *ppfPrsFormatPtr = NULL;
    struct prsSelector      psPrsSelector;    
    struct prsParser        ppPrsParser;    

    unsigned int            uiI = 0;

    unsigned char           pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char           *pucIndexName = PRS_INDEX_NAME_DEFAULT;
    unsigned char           *pucIndexDescription = NULL;

    boolean                 bIndexStreamHeader = true;
    boolean                 bIndexStreamBody = true;
    boolean                 bIndexStreamFooter = true;

    FILE                    *pfIndexerFile = stdout;

    unsigned char           pucScanfFormatAssociation[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           *pucFileNameIncludes = NULL;
    unsigned char           *pucFileNameExcludes = NULL;

    unsigned char           *pucTokenizerName = PRS_TOKENIZER_NAME_DEFAULT;
    unsigned char           *pucLanguageCode = PRS_LANGUAGE_CODE_DEFAULT;

    boolean                 bNormalizeUnicode = false;
    
    time_t                  tStartTime = (time_t)0;
    unsigned int            uiTimeTaken = 0;
    unsigned int            uiHours = 0;
    unsigned int            uiMinutes = 0;
    unsigned int            uiSeconds = 0;

    float                   fDataLength = 0;
    float                   fTotalDataLength = 0;
    unsigned char           pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};

    unsigned char           *pucLocaleName = PRS_LOCALE_NAME_DEFAULT;

    unsigned char           *pucPath = NULL;

    unsigned char           *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int            uiLogLevel = UTL_LOG_LEVEL_INFO;


    /* Set up the selector structure */
    psPrsSelector.bTraverseDirectories = false;
    psPrsSelector.ppucFileNameIncludeList = NULL;
    psPrsSelector.ppucFileNameExcludeList = NULL;


    /* Set up the parser structure */
    ppPrsParser.uiLanguageID = PRS_LANGUAGE_ID_DEFAULT;
    ppPrsParser.pvLngTokenizer = NULL;
    ppPrsParser.pvLngConverter = NULL;
    ppPrsParser.pvLngUnicodeNormalizer = NULL;
    ppPrsParser.uiTermLengthMinimum = PRS_TERM_LENGTH_MINIMUM_DEFAULT;
    ppPrsParser.uiTermLengthMaximum = PRS_TERM_LENGTH_MAXIMUM_DEFAULT;
    ppPrsParser.pucItemName = NULL;
    ppPrsParser.pucMimeType = NULL;
    ppPrsParser.ppaPrsAssociation = NULL;
    ppPrsParser.uiPrsAssociationLength = 0;
    ppPrsParser.uiDocumentKey = 0;    /* Do not produce keys */
    ppPrsParser.uiDocumentCountMax = 0;
    ppPrsParser.uiDocumentCount = 0;
    ppPrsParser.bStoreFilePaths = true;
    ppPrsParser.bCleanUnicode = false;
    ppPrsParser.bSkipUnicodeErrors = false;
    ppPrsParser.bSuppressMessages = false;


    /* Prepare the scan strings */
    snprintf(pucScanfFormatAssociation, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]:%%%d[^=]=%%%dc", UTL_FILE_PATH_MAX + 1, UTL_FILE_PATH_MAX + 1, UTL_FILE_PATH_MAX + 1);
        

    /* Initialize the log */
    if ( (iError = iUtlLogInit()) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the log, utl error: %d", iError);
    }


    /* Get the command path */
    pucCommandPath = pucUtlArgsGetNextArg(&argc, &argv);

    /* Put up an error message if there were no arguments defined and bail */
    if ( (pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv)) == NULL ) {
        vVersion();
        vUsage(pucCommandPath);
        s_exit(EXIT_SUCCESS);
    }

    /* Process the arguments */
    while ( pucNextArgument != NULL ) {

        /* Check for configuration directory */
        if ( s_strncmp("--configuration-directory=", pucNextArgument, s_strlen("--configuration-directory=")) == 0 ) {

            /* Get and check the user name */
            pucNextArgument += s_strlen("--configuration-directory=");

            /* Get the true configuration directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucConfigurationDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true configuration directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Clean the configuration directory path */
            if ( (iError = iUtlFileCleanPath(pucConfigurationDirectoryPath)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to clean the configuration directory path: '%s', utl error: %d", pucConfigurationDirectoryPath, iError);
            }

            /* Check that configuration directory path exists */
            if ( bUtlFilePathExists(pucConfigurationDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', does not exist", pucConfigurationDirectoryPath);
            }

            /* Check that the configuration directory path is a directory */
            if ( bUtlFileIsDirectory(pucConfigurationDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', is not a directory", pucConfigurationDirectoryPath);
            }

            /* Check that the configuration directory path can be accessed */
            if ( (bUtlFilePathRead(pucConfigurationDirectoryPath) == false) || (bUtlFilePathExec(pucConfigurationDirectoryPath) == false) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', cannot be accessed", pucConfigurationDirectoryPath);
            }
        }

        /* Check for index name */
        else if ( s_strncmp("--index=", pucNextArgument, s_strlen("--index=")) == 0 ) {

            /* Get the index name */
            pucNextArgument += s_strlen("--index=");

            /* Set the index name */
            pucIndexName = pucNextArgument;
        }

        /* Check for index description */
        else if ( s_strncmp("--description=", pucNextArgument, s_strlen("--description=")) == 0 ) {

            /* Get the index description */
            pucNextArgument += s_strlen("--description=");

            /* Set the index description */
            pucIndexDescription = pucNextArgument;
        }

        /* Check for index stream header */
        else if ( s_strcmp("--index-stream-header", pucNextArgument) == 0 ) {
            
            /* Set the flags accordingly */
            bIndexStreamHeader = true;
            bIndexStreamBody = false;
            bIndexStreamFooter = false;
        }

        /* Check for index stream body */
        else if ( s_strcmp("--index-stream-body", pucNextArgument) == 0 ) {
            
            /* Set the flags accordingly */
            bIndexStreamHeader = false;
            bIndexStreamBody = true;
            bIndexStreamFooter = false;
        }

        /* Check for index stream footer */
        else if ( s_strcmp("--index-stream-footer", pucNextArgument) == 0 ) {
            
            /* Set the flags accordingly */
            bIndexStreamHeader = false;
            bIndexStreamBody = false;
            bIndexStreamFooter = true;
        }

        /* Check for recurse */
        else if ( s_strcmp("--recurse", pucNextArgument) == 0 ) {

            /* Set the recurse flag */
            psPrsSelector.bTraverseDirectories = true;
        }

        /* Check for include extensions */
        else if ( s_strncmp("--include=", pucNextArgument, s_strlen("--include=")) == 0 ) {

            /* Get the include extensions */
            pucNextArgument += s_strlen("--include=");

            /* Set the include extensions */
            pucFileNameIncludes = pucNextArgument;
        }

        /* Check for exclude patterns */
        else if ( s_strncmp("--exclude=", pucNextArgument, s_strlen("--exclude=")) == 0 ) {

            /* Get the exclude patterns */
            pucNextArgument += s_strlen("--exclude=");

            /* Set the exclude patterns */
            pucFileNameExcludes = pucNextArgument;
        }

        /* Check for dont store file paths */
        else if ( s_strcmp("--dont-store-filepaths", pucNextArgument) == 0 ) {
            
            /* Set the store file paths flag */
            ppPrsParser.bStoreFilePaths = false;
        }

        /* Check for maximum documents */
        else if ( s_strncmp("--maximum-documents=", pucNextArgument, s_strlen("--maximum-documents=")) == 0 ) {

            /* Get the maximum documents */
            pucNextArgument += s_strlen("--maximum-documents=");

            /* Check the maximum number of documents */
            if ( s_strtol(pucNextArgument, NULL, 10) < 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum number of documents to submit to be greater than: 0");
            }
            
            /* Set the maximum number of documents */
            ppPrsParser.uiDocumentCountMax = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for autokey */
        else if ( s_strncmp("--autokey=", pucNextArgument, s_strlen("--autokey=")) == 0 ) {

            /* Get the key */
            pucNextArgument += s_strlen("--autokey=");

            /* Check the key */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the key to be greater than: 0");
            }

            /* Set the document key */
            ppPrsParser.uiDocumentKey = s_strtol(pucNextArgument, NULL, 10);

        }

        /* Check for autokey */
        else if ( s_strncmp("--autokey", pucNextArgument, s_strlen("--autokey")) == 0 ) {

            /* Set the document key */
            ppPrsParser.uiDocumentKey =  1;

        }

        /* Check for language code */
        else if ( s_strncmp("--language=", pucNextArgument, s_strlen("--language=")) == 0 ) {

            /* Get the language code */
            pucNextArgument += s_strlen("--language=");
            
            /* Check the language code */
            if ( (iError = iLngCheckLanguageCode(pucNextArgument)) != LNG_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid language code: '%s', lng error: %d", pucNextArgument, iError);
            }

            /* Set the language code */
            pucLanguageCode = pucNextArgument;
        }

        /* Check for tokenizer */
        else if ( s_strncmp("--tokenizer=", pucNextArgument, s_strlen("--tokenizer=")) == 0 ) {

            /* Get the tokenizer name */
            pucNextArgument += s_strlen("--tokenizer=");
            
            /* Check the tokenizer name */
            if ( (iError = iLngCheckTokenizerName(pucNextArgument)) != LNG_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid tokenizer name: '%s', lng error: %d", pucNextArgument, iError);
            }

            /* Set the tokenizer set name */
            pucTokenizerName = pucNextArgument;
        }

        /* Check for item name */
        else if ( s_strncmp("--item=", pucNextArgument, s_strlen("--item=")) == 0 ) {

            /* Get the item name */
            pucNextArgument += s_strlen("--item=");

            /* Set the item name */
            ppPrsParser.pucItemName = pucNextArgument;
        }

        /* Check for mime type */
        else if ( s_strncmp("--mime=", pucNextArgument, s_strlen("--mime=")) == 0 ) {

            /* Get the mime type */
            pucNextArgument += s_strlen("--mime=");

            /* Set the mime type */
            ppPrsParser.pucMimeType = pucNextArgument;
        }

        /* Check for association */
        else if ( s_strncmp("--associate=", pucNextArgument, s_strlen("--associate=")) == 0 ) {

            /* Get the association */
            pucNextArgument += s_strlen("--associate=");

            /* Extend the array by one entry */
            if ( (ppPrsParser.ppaPrsAssociation = (struct prsAssociation *)s_realloc(ppPrsParser.ppaPrsAssociation,
                    (size_t)(sizeof(struct prsAssociation) * (ppPrsParser.uiPrsAssociationLength + 1)))) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }

            /* Clear what was newly added */
            s_memset(ppPrsParser.ppaPrsAssociation + ppPrsParser.uiPrsAssociationLength, 0, sizeof(struct prsAssociation));

            /* Parse out the association entry */
            if ( sscanf(pucNextArgument, pucScanfFormatAssociation, (ppPrsParser.ppaPrsAssociation + ppPrsParser.uiPrsAssociationLength)->pucItemName,
                    (ppPrsParser.ppaPrsAssociation + ppPrsParser.uiPrsAssociationLength)->pucMimeType, 
                    (ppPrsParser.ppaPrsAssociation + ppPrsParser.uiPrsAssociationLength)->pucExtension) == 3 ) {
                ;
            }
            else {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to parse the association entry: '%s', something like 'document:text/plain=.txt' or 'item:image/gif=.gif' is required", pucNextArgument);
            }

            /* Increment the length */
            ppPrsParser.uiPrsAssociationLength++;
        }

        /* Check for minimum term length */
        else if ( s_strncmp("--minimum-term-length=", pucNextArgument, s_strlen("--minimum-term-length=")) == 0 ) {

            /* Get the minimum term length */
            pucNextArgument += s_strlen("--minimum-term-length=");

            /* Check the minimum term length */
            if ( s_strtol(pucNextArgument, NULL, 10) < PRS_TERM_LENGTH_MINIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the minimum term length to be greater than or equal to: %d", PRS_TERM_LENGTH_MINIMUM);
            }
            
            /* Set the minimum term length */
            ppPrsParser.uiTermLengthMinimum = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for maximum term length */
        else if ( s_strncmp("--maximum-term-length=", pucNextArgument, s_strlen("--maximum-term-length=")) == 0 ) {

            /* Get the maximum term length */
            pucNextArgument += s_strlen("--maximum-term-length=");

            /* Check the maximum term length */
            if ( s_strtol(pucNextArgument, NULL, 10) < PRS_TERM_LENGTH_MINIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum term length to be greater than or equal to: %d", PRS_TERM_LENGTH_MINIMUM);
            }
            
            if ( s_strtol(pucNextArgument, NULL, 10) > PRS_TERM_LENGTH_MAXIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum term length to be greater than or equal to: %d", PRS_TERM_LENGTH_MAXIMUM);
            }
            
            /* Set the maximum term length */
            ppPrsParser.uiTermLengthMaximum = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for normalize unicode */
        else if ( s_strcmp("--normalize-unicode", pucNextArgument) == 0 ) {

            /* Set normalize unicode */
            bNormalizeUnicode = true;
        }

        /* Check for clean unicode */
        else if ( s_strcmp("--clean-unicode", pucNextArgument) == 0 ) {

            /* Set the clean unicode */
            ppPrsParser.bCleanUnicode = true;
        }

        /* Check for skip unicode errors */
        else if ( s_strcmp("--skip-unicode-errors", pucNextArgument) == 0 ) {

            /* Set the skip unicode errors */
            ppPrsParser.bSkipUnicodeErrors = true;
        }

        /* Check for suppress */
        else if ( s_strcmp("--suppress", pucNextArgument) == 0 ) {

            /* Set suppress */
            ppPrsParser.bSuppressMessages = true;
        }

        /* Check for type */
        else if ( s_strncmp("--type=", pucNextArgument, s_strlen("--type=")) == 0 ) {

            /* Get the type */
            pucNextArgument += s_strlen("--type=");

            /* Loop through the file profile table and see if this is a document type we support */
            for ( ppfPrsFormatPtr = ppfPrsFormatListGlobal; ppfPrsFormatPtr->pucType != NULL; ppfPrsFormatPtr++ ) {
                if ( s_strcmp(ppfPrsFormatPtr->pucType, pucNextArgument) == 0 ) {
                    break;
                }
            }

            /* Check to see if we found a valid type */
            if ( ppfPrsFormatPtr->pucType == NULL ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to recognize the type: '%s'", pucNextArgument);
            }
        }

        /* Check for locale */
        else if ( s_strncmp("--locale=", pucNextArgument, s_strlen("--locale=")) == 0 ) {

            /* Get the locale */
            pucNextArgument += s_strlen("--locale=");

            /* Set the locale name */
            pucLocaleName = pucNextArgument;
        }    

        /* Check for log file */
        else if ( s_strncmp("--log=", pucNextArgument, s_strlen("--log=")) == 0 ) {

            /* Get the log file */
            pucNextArgument += s_strlen("--log=");

            /* Set the log file path */
            pucLogFilePath = pucNextArgument;
        }    

        /* Check for log level */
        else if ( s_strncmp("--level=", pucNextArgument, s_strlen("--level=")) == 0 ) {

            /* Get the log level */
            pucNextArgument += s_strlen("--level=");

            /* Check the log level */
            if ( (s_strtol(pucNextArgument, NULL, 10) < UTL_LOG_LEVEL_MINIMUM) || (s_strtol(pucNextArgument, NULL, 10) > UTL_LOG_LEVEL_MAXIMUM) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the log level to be greater than or equal to: %d, and less than or equal to: %d", UTL_LOG_LEVEL_MINIMUM, UTL_LOG_LEVEL_MAXIMUM);
            }
            
            /* Set the log level */
            uiLogLevel = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for help */
        else if ( (s_strcmp("-?", pucNextArgument) == 0) || (s_strcmp("--help", pucNextArgument) == 0) || (s_strcmp("--usage", pucNextArgument) == 0) ) {
            vVersion();
            vUsage(pucCommandPath);
            s_exit(EXIT_SUCCESS);
        }

        /* Check for version */
        else if ( (s_strcmp("--version", pucNextArgument) == 0) ) {
            vVersion();
            s_exit(EXIT_SUCCESS);
        }

        /* Check for invalid options */
        else if ( (s_strncmp("--", pucNextArgument, s_strlen("--")) == 0) || (s_strncmp("-", pucNextArgument, s_strlen("-")) == 0) ) {
            vVersion();
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid option: '%s', try '-?', '--help' or '--usage' for more information", pucNextArgument);
        }

        /* Everything else */
        else {
            /* Unknown option, could be a path, so we exit */
            pucPath = pucNextArgument;
            break;
        }

        /* Get the next argument */
        pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv);

    }



    /* Install signal handlers */
    if ( (iError = iUtlSignalsInstallHangUpHandler(SIG_IGN)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install hang-up signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallNonFatalHandler((void (*)())vUtlSignalsNonFatalHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install non-fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallFatalHandler((void (*)())vUtlSignalsFatalHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallChildHandler((void (*)())vUtlSignalsChildHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install child signal handler, utl error: %d", iError);
    }


    /* Set the log file path and the log level */
    if ( bUtlStringsIsStringNULL(pucLogFilePath) == false ) {
        if ( (iError = iUtlLogSetFilePath(pucLogFilePath)) != UTL_NoError ) {
            vVersion();
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log file path, utl error: %d", iError);
        }
    }

    if ( (iError = iUtlLogSetLevel(uiLogLevel)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log level, utl error: %d", iError);
    }


    /* Version message */
    vVersion();


    /* Check and set the locale, we require utf-8 compliance */
    if ( bLngLocationIsLocaleUTF8(LC_ALL, pucLocaleName) != true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The locale: '%s', does not appear to be utf-8 compliant", pucLocaleName);
    }
    if ( (iError = iLngLocationSetLocale(LC_ALL, pucLocaleName)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the locale to: '%s', lng error: %d", pucLocaleName, iError);
    }



    /* Warn if the configuration directory path was not defined */
    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "The configuration directory path was not defined.");
    }    


    /* Check if a valid type was set */
    if ( (ppfPrsFormatPtr == NULL) || (ppfPrsFormatPtr->pucType == NULL) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A type is required");
    }


    /* Check that the maximum term length is greater than the minimum term length */
    if ( ppPrsParser.uiTermLengthMinimum > ppPrsParser.uiTermLengthMaximum ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The maximum term length needs to be greater than the minimum term length");
    }


    /* Check that the index name was defined */
    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The index name  was not defined.");
    }    

    /* We cant use associations if there is no path */
    if ( (bUtlStringsIsStringNULL(pucPath) == true) && (ppPrsParser.ppaPrsAssociation != NULL) ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "File name extension associations will be ignored since no data path was provided.");
        s_free(ppPrsParser.ppaPrsAssociation);
    }


    /* Ignore file name includes if there is no path */
    if ( (bUtlStringsIsStringNULL(pucPath) == true) && (bUtlStringsIsStringNULL(pucFileNameIncludes) == false) ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Extension names will not be used since no data path was provided.");
    }
    /* Parse and load the include structure */
    else if ( bUtlStringsIsStringNULL(pucFileNameIncludes) == false ) {
        
        unsigned char   *pucFileNameIncludesCopy = NULL;
        unsigned char   *pucFileNameIncludePtr = NULL;
        unsigned char   *pucFileNameIncludesCopyStrtokPtr = NULL;  

        if ( (pucFileNameIncludesCopy = (unsigned char *)s_strdup(pucFileNameIncludes)) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }
        
        /* Loop parsing the file name includes */
        for ( pucFileNameIncludePtr = s_strtok_r(pucFileNameIncludesCopy, PRS_FILENAME_EXTENSION_SEPARATORS, (char **)&pucFileNameIncludesCopyStrtokPtr), uiI = 0;
                pucFileNameIncludePtr != NULL;
                pucFileNameIncludePtr = s_strtok_r(NULL, PRS_FILENAME_EXTENSION_SEPARATORS, (char **)&pucFileNameIncludesCopyStrtokPtr), uiI++ ) {
        
            /* Extend the file name includes list, and add the file name include */
            if ( (psPrsSelector.ppucFileNameIncludeList = (unsigned char **)s_realloc(psPrsSelector.ppucFileNameIncludeList,
                    (size_t)((uiI + 2) * sizeof(psPrsSelector.ppucFileNameIncludeList)))) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            } 
            if ( (psPrsSelector.ppucFileNameIncludeList[uiI] = (unsigned char *)s_strdup(pucFileNameIncludePtr)) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }
        
            /* NULL terminate the file name include list */
            psPrsSelector.ppucFileNameIncludeList[uiI + 1] = NULL;
        
        }
        
        s_free(pucFileNameIncludesCopy);
    }

    /* Ignore file name excludes if there is no path */
    if ( (bUtlStringsIsStringNULL(pucPath) == true) && (bUtlStringsIsStringNULL(pucFileNameExcludes) == false) ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Exclusion name patterns will not be used since no data path was provided.");
    }
    /* Parse and load the exclude structure */
    else if ( bUtlStringsIsStringNULL(pucFileNameExcludes) == false ) {
        
        unsigned char   *pucFileNameExcludesCopy = NULL;
        unsigned char   *pucFileNameExcludePtr = NULL;  
        unsigned char   *pucFileNameExcludesCopyStrtokPtr = NULL;
        
        if ( (pucFileNameExcludesCopy = (unsigned char *)s_strdup(pucFileNameExcludes)) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }

        /* Loop parsing the file name excludes */
        for ( pucFileNameExcludePtr = s_strtok_r(pucFileNameExcludesCopy, PRS_FILENAME_EXTENSION_SEPARATORS, (char **)&pucFileNameExcludesCopyStrtokPtr), uiI = 0;
                pucFileNameExcludePtr != NULL;
                pucFileNameExcludePtr = s_strtok_r(NULL, PRS_FILENAME_EXTENSION_SEPARATORS, (char **)&pucFileNameExcludesCopyStrtokPtr), uiI++ ) {
        
            /* Extend the file name excludes list, and add the file name exclude */
            if ( (psPrsSelector.ppucFileNameExcludeList = (unsigned char **)s_realloc(psPrsSelector.ppucFileNameExcludeList,
                    (size_t)((uiI + 2) * sizeof(psPrsSelector.ppucFileNameExcludeList)))) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            } 
            if ( (psPrsSelector.ppucFileNameExcludeList[uiI++] = (unsigned char *)s_strdup(pucFileNameExcludePtr)) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }

            /* NULL terminate the file name exclude list */
            psPrsSelector.ppucFileNameExcludeList[uiI + 1] = NULL;
        }
        
        s_free(pucFileNameExcludesCopy);
    }



    /* Get the language ID from the language code */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &ppPrsParser.uiLanguageID)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
    }


    /* Create a character set converter to convert from the utf-8 to wide characters */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_UTF_8_NAME, LNG_CHARACTER_SET_WCHAR_NAME, &ppPrsParser.pvLngConverter)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from %s to wide characters, lng error: %d.", LNG_CHARACTER_SET_UTF_8_NAME, iError);
    }


    /* Create the tokenizer */
    if ( (iError = iLngTokenizerCreateByName(pucConfigurationDirectoryPath, pucTokenizerName, pucLanguageCode, &ppPrsParser.pvLngTokenizer)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create the tokenizer, lng error: %d", iError);
    }



    /* Set up the unicode normalizer */
    if ( bNormalizeUnicode == true ) {

        /* Create the unicode normalizer, warn if normalization is not supported, but don't error out */
        iError = iLngUnicodeNormalizerCreate(pucConfigurationDirectoryPath, &ppPrsParser.pvLngUnicodeNormalizer);
    
        if ( iError == LNG_UnicodeNormalizationUnsupported ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Unicode normalization is not supported.");
        }
        else if ( iError != LNG_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create a unicode normalizer, lng error: %d", iError);
        }
    }


    /* Start the timing */
    tStartTime = s_time(NULL);


    /* Output the index stream header */
    if ( bIndexStreamHeader == true ) {

        /* Initialize the indexer by sending the version number, this is the first thing the indexer
        ** expects from us
        ** V    major_version{N} minor_version{N} (Version Number)
        */
        if ( fprintf(pfIndexerFile, "V %u %u\n", UTL_VERSION_MAJOR, UTL_VERSION_MINOR) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a version line to the indexer");
        }
    
    
        /* Send the index name and description
        ** N    index_name{A} index_description{A} (Index Name/Description)
        */
        if ( fprintf(pfIndexerFile, "N %s", pucIndexName) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send an index name/description line to the indexer");
        }
    
        if ( bUtlStringsIsStringNULL(pucIndexDescription) == false ) {
            if ( fprintf(pfIndexerFile, " %s", pucIndexDescription) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send an index name/description line to the indexer");
            }
        }
    
        if ( fprintf(pfIndexerFile, "\n") < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send an index name/description line to the indexer");
        }
    
    
        /* Send the language
        ** L language{A} tokenizer{A} (Language)
        */
        if ( fprintf(pfIndexerFile, "L %s %s\n", pucLanguageCode, pucTokenizerName) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a language line to the indexer");
        }
    
    
        /* Send the field names and description
        ** F    field_name{A} field_id{N} [field_description{A}] (Field names)
        **
        ** Send the unfielded search information
        ** S    field_name{A} [field_name{A} ...] (Field names)
        */
        if ( ppfPrsFormatPtr->bPrsFieldInformationFunction != NULL ) {
            
            unsigned int    uiFieldID = 1;
            wchar_t         *pwcFieldName = NULL;
            unsigned char   *pucFieldType = NULL;
            unsigned char   *pucFieldOptions = NULL;
            wchar_t         *pwcFieldDescription = NULL;
            boolean         bUnfieldedSearch = false;
    
            wchar_t         pwcUnfieldedSearchFieldNames[PRS_STRING_LENGTH + 1] = {L"\0"};
    
            /* Get it while it is forthcoming */
            while ( ppfPrsFormatPtr->bPrsFieldInformationFunction(uiFieldID, &pwcFieldName, &pucFieldType, 
                    &pucFieldOptions, &pwcFieldDescription, &bUnfieldedSearch) != false ) {
                
                /* Send the field names and description
                ** F    field_name{A} field_id{N} [field_description{A}] (Field names)
                */
    
                /* Send the field name and field ID */
                if ( fprintf(pfIndexerFile, "F %ls %u", pwcFieldName, uiFieldID) < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a field line to the indexer");
                }
    
                /* Send the field type */
                if ( bUtlStringsIsStringNULL(pucFieldType) == false ) {
                    if ( fprintf(pfIndexerFile, " %s", pucFieldType) < 0 ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a field line to the indexer");
                    }
                }
                else {
                    if ( fprintf(pfIndexerFile, " NONE") < 0 ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a field line to the indexer");
                    }
                }
    
                /* Send the field options */
                if ( bUtlStringsIsStringNULL(pucFieldOptions) == false ) {
                    if ( fprintf(pfIndexerFile, " %s", pucFieldOptions) < 0 ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a field line to the indexer");
                    }
                }
                else {
                    if ( fprintf(pfIndexerFile, " Defaults") < 0 ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a field line to the indexer");
                    }
                }
    
                /* Send the field description */
                if ( fprintf(pfIndexerFile, " %ls\n", pwcFieldDescription) < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a field line to the indexer");
                }
                
                /* Concatenate the unfielded search field names */
                if ( bUnfieldedSearch == true ) {
                    if ( bUtlStringsIsWideStringNULL(pwcUnfieldedSearchFieldNames) == false ) {
                        s_wcsnncat(pwcUnfieldedSearchFieldNames, L",", PRS_STRING_LENGTH, PRS_STRING_LENGTH + 1);
                    }
                    s_wcsnncat(pwcUnfieldedSearchFieldNames, pwcFieldName, PRS_STRING_LENGTH, PRS_STRING_LENGTH + 1);
                }
    
                /* Increment the field ID */
                uiFieldID++;
            }
    
            
            /* Send the unfielded search information
            ** S    field_name{A},[field_name{A},...] (Field names)
            */
    
            /* Send the unfielded search information */
            if ( bUtlStringsIsWideStringNULL(pwcUnfieldedSearchFieldNames) == false ) {
                if ( fprintf(pfIndexerFile, "S %ls\n", pwcUnfieldedSearchFieldNames) < 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send an unfielded search line to the indexer");
                }
            }
        }
    }
    


    /* Output the index stream body */
    if ( bIndexStreamBody == true ) {

        /* The first file path is already in pucPath */
        if ( pucPath != NULL ) {
    
            while ( pucPath != NULL ) {
        
                /* Clear the data length */
                fDataLength = 0;
        
                /* Parse the data from the path */
                if ( iPrsParsePath(&psPrsSelector, &ppPrsParser, ppfPrsFormatPtr, pucPath, pfIndexerFile, &fDataLength) != 0 ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to parse the data");
                }
                
                /* Increment the total length */
                fTotalDataLength += fDataLength;
                
                /* Check to see if there is a document count limit and if we have reached it */
                if ( (ppPrsParser.uiDocumentCountMax > 0) && (ppPrsParser.uiDocumentCount >= ppPrsParser.uiDocumentCountMax) ) {
                    /* Break out of the loop if we have reached it */
                    break;
                }
        
                /* Get the next path */
                pucPath = pucUtlArgsGetNextArg(&argc, &argv);
            }
        }
        else {
    
            /* Clear the data length */
            fTotalDataLength = 0;
        
            /* Parse the data from stdin */
            if ( iPrsParseTextFile(&ppPrsParser, ppfPrsFormatPtr, NULL, stdin, pfIndexerFile, &fTotalDataLength) != 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to parse the data");
            }
        }
    
    
        /* Get our time information */
        uiTimeTaken = s_difftime(s_time(NULL), tStartTime);
        uiHours = uiTimeTaken / (60 * 60);
        uiMinutes = (uiTimeTaken - (uiHours * 60 * 60)) / 60;
        uiSeconds = uiTimeTaken - ((uiTimeTaken / 60) * 60);
    
    
    
        /* Send a message
        ** M    message
        */
        if ( uiHours > 0 ) {
            if ( fprintf(pfIndexerFile, "M Parsing time for the data: %u hour%s, %u minute%s and %u second%s, %s of data.\n",
                    uiHours, (uiHours == 1) ? "" : "s", uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s", 
                    pucUtlStringsFormatDoubleAsBytes(fTotalDataLength, pucNumberString, UTL_FILE_PATH_MAX + 1) ) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
            }
        }
        else if ( uiMinutes > 0 ) {
            if ( fprintf(pfIndexerFile, "M Parsing time for the data: %u minute%s and %u second%s, %s of data.\n",
                    uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s", 
                    pucUtlStringsFormatDoubleAsBytes(fTotalDataLength, pucNumberString, UTL_FILE_PATH_MAX + 1) ) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
            }
        }
        else {
            if ( fprintf(pfIndexerFile, "M Parsing time for the data: %u second%s, %s of data.\n", uiSeconds, (uiSeconds == 1) ? "" : "s", 
                    pucUtlStringsFormatDoubleAsBytes(fTotalDataLength, pucNumberString, UTL_FILE_PATH_MAX + 1) ) < 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
            }
        }
    }



    /* Output the index stream footer */
    if ( bIndexStreamFooter == true ) {
    
        /* Tell the indexer that we have finished, this is the last thing the indexer will look at from us
        ** Z    (Stream End)
        */
        if ( fprintf(pfIndexerFile, "Z\n") < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a end of stream line to the indexer");
        }
    }
    
    

    /* Free the associations */
    if ( ppPrsParser.ppaPrsAssociation != NULL ) {
        s_free(ppPrsParser.ppaPrsAssociation);
    }

    /* Free the file name include list */
    UTL_MACROS_FREE_NULL_TERMINATED_LIST(psPrsSelector.ppucFileNameIncludeList);

    /* Free the file name exclude list */
    UTL_MACROS_FREE_NULL_TERMINATED_LIST(psPrsSelector.ppucFileNameExcludeList);
    
    /* Free the tokenizer */
    iLngTokenizerFree(ppPrsParser.pvLngTokenizer);
    ppPrsParser.pvLngTokenizer = NULL;
    
    /* Free the character set converter */
    iLngConverterFree(ppPrsParser.pvLngConverter);
    ppPrsParser.pvLngConverter = NULL;
    
    /* Free the unicode normalizer */
    iLngUnicodeNormalizerFree(ppPrsParser.pvLngUnicodeNormalizer);
    ppPrsParser.pvLngUnicodeNormalizer = NULL;


    return (EXIT_SUCCESS);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vVersion()

    Purpose:    This function list out the version message.

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vVersion
(

)
{

    unsigned char   pucVersionString[UTL_VERSION_STRING_LENGTH + 1] = {'\0'};
    unsigned char   pucTokenizerFeaturesString[LNG_TOKENIZER_FEATURES_STRING_LENGTH + 1] = {'\0'};

    
    /* Copyright message */
    fprintf(stderr, "MPS Parser, %s\n", UTL_VERSION_COPYRIGHT_STRING);


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    fprintf(stderr, "%s\n", pucVersionString);


    /* Get the tokenizer features string */
    iLngTokenizerGetFeaturesString(pucTokenizerFeaturesString, LNG_TOKENIZER_FEATURES_STRING_LENGTH + 1);

    /* Tokenizer features message */
    fprintf(stderr, "%s\n", pucTokenizerFeaturesString);

    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters supported by the MPS parser.

    Parameters: pucCommandPath      command path

    Globals:    none

    Returns:    void

*/
static void vUsage
(
    unsigned char *pucCommandPath
)
{

    unsigned char       *pucCommandNamePtr = NULL;
    struct prsFormat    *ppfPrsFormatPtr = ppfPrsFormatListGlobal;


    ASSERT(bUtlStringsIsStringNULL(pucCommandPath) == false);


    /* Get the command name */
    if ( iUtlFileGetPathBase(pucCommandPath, &pucCommandNamePtr) != UTL_NoError ) {
        pucCommandNamePtr = pucCommandPath;
    }


    /* Print out the usage */
    printf("\nUsage for: '%s'. \n", pucUtlStringsGetPrintableString(pucCommandNamePtr));
    printf("\n");

    printf(" General parameters: \n");
    
    printf("  --configuration-directory=name \n");
    printf("                  Configuration directory. \n");
    printf("  --index=name    Index name, defaults to: '%s'. \n", PRS_INDEX_NAME_DEFAULT);
    printf("  --description=description \n");
    printf("                  Index description, optional. \n");
    printf("\n");

    printf(" Index stream control parameters: \n");
    printf("  --index-stream-header \n");
    printf("                  Output the index stream header only. \n");
    printf("  --index-stream-body \n");
    printf("                  Output the index stream body only. \n");
    printf("  --index-stream-footer \n");
    printf("                  Output the index stream footer only. \n");
    printf("\n");

    printf(" File selection parameters: \n");
    printf("  --recurse       Recursively parse sub-directories. \n");
    printf("  --include=extension[,extension,...] \n");
    printf("                  Extension names of files to index. \n");
    printf("  --exclude=pattern[,pattern,...] \n");
    printf("                  Name patterns of files to exclude. \n");
    printf("\n");

    printf(" Index control parameters:\n");
    printf("  --language=code \n");
    printf("                  Language code, defaults to '%s'. \n", PRS_LANGUAGE_CODE_DEFAULT);
    printf("  --tokenizer=name \n");
    printf("                  Tokenizer name, defaults to: '%s', tokenizers available: '%s', '%s'. \n", PRS_TOKENIZER_NAME_DEFAULT, LNG_TOKENIZER_FSCLT_1_NAME, LNG_TOKENIZER_FSCLT_2_NAME);
    printf("  --item=name     Item name, defaults to -type item name. \n");
    printf("  --mime-type=name \n");
    printf("                  Mime type, defaults to the mime type set by --type. \n");
    printf("  --associate=item:mime/type=extension \n");
    printf("                  Associates a file name extention with an item name and mine type. \n");
    printf("  --minimum-term-length=# \n");
    printf("                  Minimum length of a term, defaults to %d, minimum: %d. \n", PRS_TERM_LENGTH_MINIMUM_DEFAULT, PRS_TERM_LENGTH_MINIMUM);
    printf("  --maximum-term-length=# \n");
    printf("                  Maximum length of a term, defaults to %d, minimum: %d. \n", PRS_TERM_LENGTH_MAXIMUM_DEFAULT, PRS_TERM_LENGTH_MAXIMUM);
    printf("  --normalize-unicode \n");
    printf("                  Normalize the unicode, default is to leave it as is, normalization mode is set by configuration file. \n");
    printf("  --clean-unicode  \n");
    printf("                  Clean the unicode replacing invalid characters with spaces. \n");
    printf("  --skip-unicode-errors \n");
    printf("                  Skip unicode errors as opposed to erroring out. \n");
    printf("  --dont-store-file-paths \n");
    printf("                  Don't store file paths as part of the document items. \n");
    printf("  --maximum-documents=# \n");
    printf("                  Maximum number of documents to be parsed, defaults to no limit. \n");
    printf("  --autokey[=#]   Auto-generate document keys, optionally start with supplied number. \n");
    printf("  --suppress      Suppress routine parser messages that may be sent as part of the stream. \n");
    printf("\n");

    printf(" File type parameters: \n");
    printf("  --type=type     File type, required. File types supported: \n");

    /* Loop through the format list printing out the entries */
    for ( ppfPrsFormatPtr = ppfPrsFormatListGlobal; ppfPrsFormatPtr->pucType != NULL; ppfPrsFormatPtr++ ) {
        printf("                  [%s] %s (%s:%s).\n", ppfPrsFormatPtr->pucType, ppfPrsFormatPtr->pucDescription, ppfPrsFormatPtr->pucItemName, ppfPrsFormatPtr->pucMimeType);
    }
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", PRS_LOCALE_NAME_DEFAULT);
    printf("                  supported locales, locale chosen must support utf-8. \n");
    printf("\n");

    printf(" Logging parameters: \n");
    printf("  --log=name      Log output file name, defaults to 'stderr', console options: '%s', '%s'. \n", UTL_LOG_FILE_STDOUT, UTL_LOG_FILE_STDERR);
    printf("  --level=#       Log level, defaults to info, %d = debug, %d = info, %d = warn, %d = error, %d = fatal. \n",
            UTL_LOG_LEVEL_DEBUG, UTL_LOG_LEVEL_INFO, UTL_LOG_LEVEL_WARN, UTL_LOG_LEVEL_ERROR, UTL_LOG_LEVEL_FATAL);
    printf("\n");

    printf(" File selection: \n");
    printf("  filename filename ...\n");
    printf("                  Read filenames from the command line, otherwise read file from 'stdin'. \n");
    printf("\n");

    printf(" Help & version: \n");
    printf("  -?, --help, --usage \n");
    printf("                  Prints the usage and exits. \n");
    printf("  --version       Prints the version and exits. \n");
    printf("\n");


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bPrsIsFileParseable()

    Purpose:    Returns true if this file can be parsed, false if not

    Parameters: ppsPrsSelector      selector structure
                pucFilePath         file path of the file to parse

    Globals:    none

    Returns:    true if this file can be parsed, false if not

*/
static boolean bPrsIsFileParseable
(
    struct prsSelector *ppsPrsSelector,
    unsigned char *pucFilePath
)
{

    int             iError = UTL_NoError;
    unsigned char   pucTrueFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int    uiI = 0;


    ASSERT(ppsPrsSelector != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucFilePath) == false);


    /* Get the true path of this file path */
    if ( (iError = iUtlFileGetTruePath(pucFilePath, pucTrueFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the true path of the file path: '%s', utl error: %d.", pucFilePath, iError);
        return (false);
    }


    /* If ppsPrsSelector->ppucFileNameIncludeList is set, we need to check
    ** the extensions of all the files to see if they match an allowable one
    */
    if ( ppsPrsSelector->ppucFileNameIncludeList != NULL ) {

        /* See if we have a match */
        for ( uiI = 0; ppsPrsSelector->ppucFileNameIncludeList[uiI] != NULL; uiI++ ) {
            if ( s_strcmp(pucTrueFilePath + (s_strlen(pucTrueFilePath) - s_strlen(ppsPrsSelector->ppucFileNameIncludeList[uiI])), ppsPrsSelector->ppucFileNameIncludeList[uiI]) == 0) {
                return (true);
            }
        }
        
        /* Failed to match the file */
        return (false);
    }


    /* If ppsPrsSelector->ppucFileNameExcludeList is set, we need to check
    ** for patterns in the file path to see if we need to exclude the file
    */
    if ( ppsPrsSelector->ppucFileNameExcludeList != NULL ) {

        /* See if we have a match */
        for ( uiI = 0; ppsPrsSelector->ppucFileNameExcludeList[uiI] != NULL; uiI++ ) {
            if ( s_strstr(pucTrueFilePath, ppsPrsSelector->ppucFileNameExcludeList[uiI]) != NULL) {
                return (false);
            }
        }
    }


    /* Its ok to index this file */
    return (true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iPrsParsePath()

    Purpose:    Recursively parse the files or directory in the path

    Parameters: ppsPrsSelector      selector structure
                pppPrsParser        parser structure
                ppfPrsFormat        format structure
                pucPath             file/directory path
                pfIndexerFile       structured index stream output file
                pfDataLength        return pointer for the cumulative data length parsed

    Globals:    none

    Returns:    0 on success, -1 on error

*/
static int iPrsParsePath
(
    struct prsSelector *ppsPrsSelector,
    struct prsParser *pppPrsParser,
    struct prsFormat *ppfPrsFormat,
    unsigned char *pucPath,
    FILE *pfIndexerFile,
    float *pfDataLength
)
{

    int             iError = UTL_NoError;
    unsigned char   **ppucDirectoryEntryList = NULL;
    unsigned char   **ppucDirectoryEntryListPtr = NULL;


    ASSERT(ppsPrsSelector != NULL);
    ASSERT(pppPrsParser != NULL);
    ASSERT(ppfPrsFormat != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucPath) == false);
    ASSERT(pfIndexerFile != NULL);
    ASSERT(pfDataLength != NULL);


    /* Process file */
    if ( bUtlFileIsFile(pucPath) == true ) {

        /* Do we want to index this file? */
        if ( bPrsIsFileParseable(ppsPrsSelector, pucPath) == true ) {

            /* Parse the file */
            if ( iPrsParseTextFile(pppPrsParser, ppfPrsFormat, pucPath, NULL, pfIndexerFile, pfDataLength) != 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse the file: '%s'.", pucPath);
                return (-1);
            }
        }
    }
    /* Process directory */
    else if ( bUtlFileIsDirectory(pucPath) == true ) {

        /* Traverse the directory if we are to do so */
        if ( ppsPrsSelector->bTraverseDirectories == true ) {

            /* Scan the data directory and pick up a list of all the directories/files */
            if ( (iError = iUtlFileScanDirectory(pucPath, NULL, NULL, &ppucDirectoryEntryList)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the contents of the directory path: '%s', utl error: %d.", pucPath, iError);
                return (-1);
            }
            
            /* Loop over all the directories/files we picked up in the scan */
            if ( ppucDirectoryEntryList != NULL ) {
    
                for ( ppucDirectoryEntryListPtr = ppucDirectoryEntryList; *ppucDirectoryEntryListPtr != NULL; ppucDirectoryEntryListPtr++ ) {
        
                    unsigned char   pucFinalPathName[UTL_FILE_PATH_MAX + 1] = {'\0'};
                    float           fDataLength = 0;
        
                    /* Create a fully qualified path */
                    if ( (iError = iUtlFileMergePaths(pucPath, *ppucDirectoryEntryListPtr, pucFinalPathName, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a fully qualified path, entry: '%s', path: '%s', utl error: %d.", 
                                *ppucDirectoryEntryListPtr, pucPath, iError);
                        continue;
                    }
    
                    /* Parse the path */
                    if ( iPrsParsePath(ppsPrsSelector, pppPrsParser, ppfPrsFormat, pucFinalPathName, pfIndexerFile, &fDataLength) != 0 ) {
                        return (-1);
                    }
                    
                    /* Increment the length */
                    *pfDataLength += fDataLength;
                    
                    /* Check to see if there is a document count limit and if we have reached it */
                    if ( (pppPrsParser->uiDocumentCountMax > 0) && (pppPrsParser->uiDocumentCount >= pppPrsParser->uiDocumentCountMax) ) {
                        /* Break out of the loop if we have reached it */
                        break;
                    }
                }
    
                /* Free the directory list */
                iUtlFileFreeDirectoryEntryList(ppucDirectoryEntryList);
                ppucDirectoryEntryList = NULL;
            }
        }
    }
    /* Bummer */
    else {
        /* Send a message
        ** M    message
        */
        if ( fprintf(pfIndexerFile, "M Failed to access file: '%s'.\n", pucPath) < 0 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to send a message line to the indexer");
        }
    }


    return (0);

}


/*---------------------------------------------------------------------------*/

