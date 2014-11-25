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

    Module:     mpsphrases.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 April 2005

    Purpose:    

*/


/*---------------------------------------------------------------------------


/*
** Includes
*/

#include "utils.h"

#include "lng.h"

#include "srch.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.mpsphrases"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Default minimum phrase count */
#define SRCH_PHRASES_PHRASE_COUNT_MIN_DEFAULT           (10)

/* Default maximum phrase count - 0 mean no limit */
#define SRCH_PHRASES_PHRASE_COUNT_MAX_DEFAULT           (0)

/* Default phrase percentage */
#define SRCH_PHRASES_PHRASE_PERCENTAGE_DEFAULT          (20.0)

/* Default document coverage */
#define SRCH_PHRASES_DOCUMENT_COVERAGE_DEFAULT          (1.0)

/* Default coverage */
#define SRCH_PHRASES_FREQUENT_TERM_COVERAGE_DEFAULT     (0.0)


/* Default maximum errors */
#define SRCH_TERMS_ERRORS_MAX_DEFAULT                   (20)


/* Default locale name */
#define SRCH_PHRASES_LOCALE_NAME_DEFAULT                LNG_LOCALE_EN_US_UTF_8_NAME


/* Max term length */
#define SRCH_PHRASES_MAX_TERM_LENGTH                    (1024)


/* Index name separators */
#define SRCH_PHRASES_INDEX_NAME_SEPARATORS              (unsigned char *)", "


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Phrases sort structure */
struct phrasesSort {
    unsigned char       *pucPhrase;                     /* Phrase */
    unsigned int        uiPhraseCount;                  /* Number of occurrences of this phrase in the text */
    unsigned int        uiDocumentCount;                /* Number of documents in which this phrase occurs */
    float               fWeight;                        /* Phrase weight */
};    


/* Phrases index list structure */
struct phrasesIndexList {
    struct srchIndex    *psiSrchIndex;                  /* Index */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);


static int iSrchPhrasesExtractPhrases (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath, 
        unsigned char *pucIndexNames, unsigned char *pucStopListName, unsigned char *pucStopListFilePath, 
        unsigned char *pucLanguageCode, boolean bLanguageFirstTerm, unsigned char *pucTextFilePath, unsigned char *pucPhrasesFilePath, 
        unsigned int uiMinPhraseCount, unsigned int uiMaxPhraseCount, float fPhrasePercentage, float fDocumentCoverage, 
        float fFrequentTermCoverage, unsigned int uiMaxTokenizationErrors);

static int iSrchPhrasesAddStopTermsToTrie (void *pvLngStemmer, void *pvUtlStopTermsTrie, 
        wchar_t **ppwcStopListTermList, unsigned int uiStopListTermListLength);

static int iSrchPhrasesExtractPhrasesFromText (struct phrasesIndexList *ppilPhrasesIndexList, unsigned int uiPhrasesIndexListLength, 
        void *pvLngTokenizer, void *pvLngStemmer, void *pvUtlStopTermsTrie, unsigned char *pucText, unsigned char *pucLanguageCode, 
        FILE *pfOutputFile, unsigned int uiDocumentCount, unsigned int uiMinPhraseCount, unsigned int uiMaxPhraseCount, 
        float fPhrasePercentage, float fDocumentCoverage, float fFrequentTermCoverage, unsigned int uiMaxTokenizationErrors);

static int iSrchPhrasesParseTextToTrie (struct srchIndex *psiSrchIndex, unsigned char *pucText,
        void **ppvUtlPhrasesTrie, unsigned int *puiTotalPhraseCount, unsigned int *puiUniquePhraseCount);

static int iSrchPhrasesCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);

static int iSrchPhrasesSetTermWeight (struct srchIndex *psiSrchIndex, float fFrequentTermCoverage, 
        void *pvLngStemmer, void *pvUtlStopTermsTrie, struct phrasesSort *ppsPhrasesSort, wchar_t *pwcTerm);

static int iSrchPhrasesListPhrases (struct phrasesSort *ppsPhrasesSort, unsigned int uiPhrasesSortLength, FILE *pfOutputFile, 
        unsigned int uiDocumentCount, unsigned int uiMinPhraseCount, unsigned int uiMaxPhraseCount, 
        float fPhrasePercentage, float fDocumentCoverage, unsigned int *puiUsedPhraseCount);

static int iSrchPhrasesCompareByWeightDesc (struct phrasesSort *pssPhrasesSort1, struct phrasesSort *pssPhrasesSort2);


/*---------------------------------------------------------------------------*/


/*

    Function:   main()

    Purpose:    The main function

    Parameters: argc, argv

    Globals:    none

    Returns:    int

*/
int main
(
    int argc,
    char *argv[]
)
{

    int             iError = SPI_NoError;
    unsigned char   *pucCommandPath = NULL;
    unsigned char   *pucNextArgument = NULL;

    unsigned char   pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucIndexDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   *pucIndexNames = NULL;
    unsigned char   *pucStopListName = NULL;
    unsigned char   pucStopListFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   *pucLanguageCode = NULL;
    boolean         bLanguageFirstTerm = false;
    unsigned char   pucTextFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucPhrasesFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int    uiMinPhraseCount = SRCH_PHRASES_PHRASE_COUNT_MIN_DEFAULT;
    unsigned int    uiMaxPhraseCount = SRCH_PHRASES_PHRASE_COUNT_MAX_DEFAULT;
    float           fPhrasePercentage = SRCH_PHRASES_PHRASE_PERCENTAGE_DEFAULT;
    float           fDocumentCoverage = SRCH_PHRASES_DOCUMENT_COVERAGE_DEFAULT;
    float           fFrequentTermCoverage = SRCH_PHRASES_FREQUENT_TERM_COVERAGE_DEFAULT;
    unsigned int    uiMaxTokenizationErrors = SRCH_TERMS_ERRORS_MAX_DEFAULT;

    unsigned char   *pucLocaleName = SRCH_PHRASES_LOCALE_NAME_DEFAULT;

    unsigned char   *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int    uiLogLevel = UTL_LOG_LEVEL_INFO;



    /* Initialize the log */
    if ( (iError = iUtlLogInit()) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the log, utl error: %d", iError);
    }


    /* Get the command path */
    pucCommandPath = pucUtlArgsGetNextArg(&argc, &argv);

    /* Get the next argument, bail if none found */
    if ( (pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv)) == NULL ) {
        vVersion();
        vUsage(pucCommandPath);
        s_exit(EXIT_SUCCESS);
    }

    /* Process arguments */
    while( pucNextArgument != NULL ) {

        /* Check for configuration directory */
        if ( s_strncmp("--configuration-directory=", pucNextArgument, s_strlen("--configuration-directory=")) == 0 ) {

            /* Get the configuration directory path */
            pucNextArgument += s_strlen("--configuration-directory=");

            /* Get the true configuration directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucConfigurationDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true configuration directory path: '%s', utl error: %d", pucNextArgument, iError);
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

        /* Check for index directory */
        else if ( s_strncmp("--index-directory=", pucNextArgument, s_strlen("--index-directory=")) == 0 ) {

            /* Get the index directory path */
            pucNextArgument += s_strlen("--index-directory=");

            /* Get the true index directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucIndexDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true index directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that index directory path exists */
            if ( bUtlFilePathExists(pucIndexDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', does not exist", pucIndexDirectoryPath);
            }

            /* Check that the index directory path is a directory */
            if ( bUtlFileIsDirectory(pucIndexDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', is not a directory", pucIndexDirectoryPath);
            }

            /* Check that the index directory path can be accessed */
            if ( (bUtlFilePathRead(pucIndexDirectoryPath) == false) || (bUtlFilePathExec(pucIndexDirectoryPath) == false) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', cannot be accessed", pucIndexDirectoryPath);
            }
        }

        /* Check for index names */
        else if ( s_strncmp("--index=", pucNextArgument, s_strlen("--index=")) == 0 ) {

            /* Get the index names */
            pucNextArgument += s_strlen("--index=");

            /* Set the index names */
            pucIndexNames = pucNextArgument;
        }

        /* Check for stop list */
        else if ( s_strncmp("--stoplist=", pucNextArgument, s_strlen("--stoplist=")) == 0 ) {

            /* Get the stop list name */
            pucNextArgument += s_strlen("--stoplist=");

            /* Check the stop list name */
            if ( (iError = iLngCheckStopListName(pucNextArgument)) != LNG_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid the stop list name: '%s', lng error: %d", pucNextArgument, iError);
            }

            /* Set stop list name */
            pucStopListName = pucNextArgument;
        } 

        /* Check for stop file */
        else if ( s_strncmp("--stopfile=", pucNextArgument, s_strlen("--stopfile=")) == 0 ) {

            /* Get the stop list file path */
            pucNextArgument += s_strlen("--stopfile=");

            /* Get the true stop list file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucStopListFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true stop list file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that stop list file path exists */
            if ( bUtlFilePathExists(pucStopListFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The stop list file path: '%s', does not exist", pucStopListFilePath);
            }

            /* Check that the stop list file path is a file */
            if ( bUtlFileIsFile(pucStopListFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The stop list file path: '%s', is not a file", pucStopListFilePath);
            }

            /* Check that the stop list file path can be accessed */
            if ( bUtlFilePathRead(pucStopListFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The stop list file path: '%s', cannot be accessed", pucStopListFilePath);
            }
        } 

        /* Check for language */
        else if ( s_strncmp("--language=", pucNextArgument, s_strlen("--language=")) == 0 ) {

            /* Get the language code */
            pucNextArgument += s_strlen("--language=");

            /* Check the language code */
            if ( (iError = iLngCheckLanguageCode(pucNextArgument)) != LNG_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid language code: '%s', lng error: %d", pucNextArgument, iError);
            }

            /* Set language code */
            pucLanguageCode = pucNextArgument;
        } 

        /* Check for language first term */
        else if ( s_strcmp("--language-first-term", pucNextArgument) == 0 ) {

            /* Set language first term */
            bLanguageFirstTerm = true;
        } 

        /* Check for true text file */
        else if ( s_strncmp("--text=", pucNextArgument, s_strlen("--text=")) == 0 ) {

            /* Get the text file */
            pucNextArgument += s_strlen("--text=");

            /* Get the text file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucTextFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true text file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that text file path exists */
            if ( bUtlFilePathExists(pucTextFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The text file path: '%s', does not exist", pucTextFilePath);
            }

            /* Check that the text file path is a file */
            if ( bUtlFileIsFile(pucTextFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The text file path: '%s', is not a file", pucTextFilePath);
            }

            /* Check that the text file path can be accessed */
            if ( bUtlFilePathRead(pucTextFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The text file path: '%s', cannot be accessed", pucTextFilePath);
            }
        }

        /* Check for true phrases file */
        else if ( s_strncmp("--phrases=", pucNextArgument, s_strlen("--phrases=")) == 0 ) {

            /* Get the phrases file */
            pucNextArgument += s_strlen("--phrases=");

            /* Get the phrases file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucPhrasesFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true phrases file path: '%s', utl error: %d", pucNextArgument, iError);
            }
            
            /* Check that phrases file path can be written to if it exists */
            if ( bUtlFilePathExists(pucPhrasesFilePath) == true ) {
                if ( bUtlFilePathWrite(pucPhrasesFilePath) == false ) {
                    vVersion();
                    iUtlLogPanic(UTL_LOG_CONTEXT, "The phrases file path: '%s', cannot be accessed", pucPhrasesFilePath);
                }
            }
        }

        /* Check for minimum phrases */
        else if ( s_strncmp("--minimum-phrases=", pucNextArgument, s_strlen("--minimum-phrases=")) == 0 ) {

            /* Get the minimum phrases */
            pucNextArgument += s_strlen("--minimum-phrases=");

            /* Check the minimum phrases */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid minimum number of phrases: '%s'", pucNextArgument);
            }

            /* Set the minimum phrases count */
            uiMinPhraseCount = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for maximum phrases */
        else if ( s_strncmp("--maximum-phrases=", pucNextArgument, s_strlen("--maximum-phrases=")) == 0 ) {

            /* Get the maximum phrases */
            pucNextArgument += s_strlen("--maximum-phrases=");

            /* Check the maximum phrases */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid maximum number of phrases: '%s'", pucNextArgument);
            }

            /* Set the maximum phrases count */
            uiMaxPhraseCount = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for phrase percentage */
        else if ( s_strncmp("--phrase-percentage=", pucNextArgument, s_strlen("--phrase-percentage=")) == 0 ) {

            /* Get the phrase percentage */
            pucNextArgument += s_strlen("--phrase-percentage=");

            /* Set the phrase percentage */
            fPhrasePercentage = s_strtof(pucNextArgument, NULL);
            
            /* Check the phrase percentage */
            if ( (fPhrasePercentage <= 0) || (fPhrasePercentage > 100) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid phrase percentage: '%s'", pucNextArgument);
            }
        }

        /* Check for document overage */
        else if ( s_strncmp("--document-coverage=", pucNextArgument, s_strlen("--document-coverage=")) == 0 ) {

            /* Get the document coverage */
            pucNextArgument += s_strlen("--document-coverage=");
            
            /* Set the document coverage */
            fDocumentCoverage = s_strtof(pucNextArgument, NULL);
            
            /* Check the coverage */
            if ( (fDocumentCoverage <= 0) || (fDocumentCoverage > 100) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid coverage: '%s'", pucNextArgument);
            }
        }

        /* Check for frequent term coverage */
        else if ( s_strncmp("--frequent-term-coverage=", pucNextArgument, s_strlen("--frequent-term-coverage=")) == 0 ) {

            /* Get the frequent term coverage */
            pucNextArgument += s_strlen("--frequent-term-coverage=");

            /* Set the frequent term coverage */
            fFrequentTermCoverage = s_strtof(pucNextArgument, NULL);
            
            /* Check the frequent term coverage */
            if ( fFrequentTermCoverage < 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid frequent term coverage: '%s'", pucNextArgument);
            }
        }

        /* Check for maximum tokenization errors */
        else if ( s_strncmp("--maximum-tokenization-errors=", pucNextArgument, s_strlen("--maximum-tokenization-errors=")) == 0 ) {

            /* Get the maximum tokenization errors */
            pucNextArgument += s_strlen("--maximum-tokenization-errors=");

            /* Check the maximum tokenization errors */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid maximum number of tokenization errors: '%s'", pucNextArgument);
            }

            /* Set the maximum tokenization rrors */
            uiMaxTokenizationErrors = s_strtol(pucNextArgument, NULL, 10);
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

        /* Everything else */
        else {
            /* Cant recognize this argument */
            vVersion();
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid option: '%s', try '-?', '--help' or '--usage' for more information", pucNextArgument);
        }

        /* Get the next argument */
        pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv);
    }


    /* Install signal handlers */
    if ( (iError = iUtlSignalsInstallFatalHandler((void (*)())vUtlSignalsFatalHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallNonFatalHandler((void (*)())vUtlSignalsNonFatalHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install non-fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallHangUpHandler(SIG_IGN)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install hang-up signal handler, utl error: %d", iError);
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


    /* Check for index directory path */
    if ( bUtlStringsIsStringNULL(pucIndexDirectoryPath) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "An index directory path is required");
    }

    /* Check for configuration directory path */
    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A configuration directory path is required");
    }

    /* Check for index names */
    if ( bUtlStringsIsStringNULL(pucIndexNames) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "An index name(s) is required");
    }


    /* Check that a stop list name and stop list file name were not both specified */
    if ( (bUtlStringsIsStringNULL(pucStopListName) == false) && (bUtlStringsIsStringNULL(pucStopListFilePath) == false) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A stop list name and stop list file path cannot both be specified");
    }


    /* Check that a language code and language first term were not both specified */
    if ( (bUtlStringsIsStringNULL(pucLanguageCode) == false) && (bLanguageFirstTerm == true) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A language code and language first term cannot both be specified");
    }


    /* Check the minimum phrase count */
    if ( uiMinPhraseCount <= 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The minimum number of phrases cannot be zero or less, minimum number of phrases specified: %u", uiMinPhraseCount);
    }

    /* Check the maximum phrase count - 0 mean no limit */
    if ( uiMaxPhraseCount < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The maximum number of phrases cannot be less than 0, maximum number of phrases specified: %u", uiMaxPhraseCount);
    }

    /* Check that the maximum phrase count is more than the minimum phrase count */
    if ( (uiMaxPhraseCount > 0) && (uiMinPhraseCount > uiMaxPhraseCount) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The maximum number of phrases cannot be less than the minimum number of phrases, minimum number of phrases specified: %u, maximum number of phrases specified: %u", uiMinPhraseCount, uiMaxPhraseCount);
    }

    /* Check for phrase percentage */
    if ( (fPhrasePercentage <= 0) || (fPhrasePercentage > 100) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The phrase percentage cannot be zero or less or more than 100, value specified: %9.4f", fPhrasePercentage);
    }

    /* Check for document coverage */
    if ( (fDocumentCoverage <= 0) || (fDocumentCoverage > 100) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The document coverage cannot be zero or less or more than 100, value specified: %9.4f", fDocumentCoverage);
    }

    /* Check for frequent term coverage */
    if ( fFrequentTermCoverage < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The frequent term coverage cannot less than 0, value specified: %9.4", fFrequentTermCoverage);
    }


    /* Extract the phrases */
    iError = iSrchPhrasesExtractPhrases(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexNames, pucStopListName, pucStopListFilePath, 
            pucLanguageCode, bLanguageFirstTerm, pucTextFilePath, pucPhrasesFilePath, uiMinPhraseCount, uiMaxPhraseCount, fPhrasePercentage, fDocumentCoverage, 
            fFrequentTermCoverage, uiMaxTokenizationErrors);



    return ((iError == SRCH_NoError) ? EXIT_SUCCESS : EXIT_FAILURE);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vVersion()

    Purpose:    This function list out the version message.

    Parameters: none

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
    fprintf(stderr, "MPS Phrases, %s\n", UTL_VERSION_COPYRIGHT_STRING);


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

    Purpose:    This function list out all the parameters that irverify
                takes.

    Parameters: pucCommandPath  command path

    Globals:    none

    Returns:    void

*/
static void vUsage
(
    unsigned char *pucCommandPath
)
{

    unsigned char   *pucCommandNamePtr = NULL;


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
    printf("  --index-directory=name \n");
    printf("                  Index directory.\n");
    printf("  --index=name[%sname[%s...]] \n", SRCH_PHRASES_INDEX_NAME_SEPARATORS, SRCH_PHRASES_INDEX_NAME_SEPARATORS);
    printf("                  Index names, '%s' delimited, required. \n", SRCH_PHRASES_INDEX_NAME_SEPARATORS);
    printf("  --stoplist=name \n");
    printf("                  Stop list to use, defaults to index stop list , stop lists available: '%s', \n", LNG_STOP_LIST_NONE_NAME);
    printf("                  '%s', '%s'.\n", LNG_STOP_LIST_GOOGLE_NAME, LNG_STOP_LIST_GOOGLE_MODIFIED_NAME);
    printf("  --stopfile=name \n");
    printf("                  Stop list term file, overriding the internal stop list, one term per line. \n");
    printf("  --language=name \n");
    printf("                  Language code, defaults to index language. \n");
    printf("  --language-first-term \n");
    printf("                  First term in the text file will be used as the language. \n");
    printf("  --text=name     Text file name, defaults to 'stdin'. \n");
    printf("  --phrases=name  Phrases file name, defaults to 'stdout'. \n");
    printf("  --minimum-phrases=# \n");
    printf("                  Minimum number of phrases, defaults to: %d. \n", SRCH_PHRASES_PHRASE_COUNT_MIN_DEFAULT);
    printf("  --maximum-phrases=# \n");
    printf("                  Maximum number of phrases, defaults to: %d, 0 means no limit. \n", SRCH_PHRASES_PHRASE_COUNT_MAX_DEFAULT);
    printf("  --phrase-percentage=# \n");
    printf("                  Maximum number of phrases as a percentage of the original text, \n");
    printf("                  defaults to: %3.1f%%, range: 1-100. \n", SRCH_PHRASES_PHRASE_PERCENTAGE_DEFAULT);
    printf("  --document-coverage=# \n");
    printf("                  Document coverage threshold, defaults to: %3.1f%%, range: 1-100. \n", SRCH_PHRASES_DOCUMENT_COVERAGE_DEFAULT);
    printf("  --frequent-term-coverage=# \n");
    printf("                  Frequent term coverage threshold, defaults to: %3.1f%%, range: 0-.., 0 means not applied. \n", SRCH_PHRASES_FREQUENT_TERM_COVERAGE_DEFAULT);
    printf("  --maximum-tokenization-errors=# \n");
    printf("                  Maximum number of tokenization errors per text file, defaults to: %d%%. \n", SRCH_TERMS_ERRORS_MAX_DEFAULT);
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", SRCH_PHRASES_LOCALE_NAME_DEFAULT);
    printf("                  supported locales, locale chosen must support utf-8. \n");
    printf("\n");

    printf(" Logging parameters: \n");
    printf("  --log=name      Log output file name, defaults to 'stderr', console options: '%s', '%s'. \n", UTL_LOG_FILE_STDOUT, UTL_LOG_FILE_STDERR);
    printf("  --level=#       Log level, defaults to info, %d = debug, %d = info, %d = warn, %d = error, %d = fatal. \n",
            UTL_LOG_LEVEL_DEBUG, UTL_LOG_LEVEL_INFO, UTL_LOG_LEVEL_WARN, UTL_LOG_LEVEL_ERROR, UTL_LOG_LEVEL_FATAL);
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

    Function:   iSrchPhrasesExtractPhrases()

    Purpose:    This function extracts the phrases from the text.

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexNames                   index names
                pucStopListName                 stop list name (optional)
                pucStopListFilePath             stop list file path (optional)
                pucLanguageCode                 language code (optional)
                bLanguageFirstTerm              language first term flag
                pucTextFilePath                 text file path (optional)
                pucPhrasesFilePath              phrases file path (optional)
                uiMinPhraseCount                minimum phrase count
                uiMaxPhraseCount                maximum phrase count
                fPhrasePercentage               phrase percentage
                fDocumentCoverage               document coverage
                fFrequentTermCoverage           frequent term coverage
                uiMaxTokenizationErrors         maximum tokenization errors
                

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchPhrasesExtractPhrases
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexNames,
    unsigned char *pucStopListName,
    unsigned char *pucStopListFilePath,
    unsigned char *pucLanguageCode,
    boolean bLanguageFirstTerm,
    unsigned char *pucTextFilePath,
    unsigned char *pucPhrasesFilePath,
    unsigned int uiMinPhraseCount,
    unsigned int uiMaxPhraseCount,
    float fPhrasePercentage,
    float fDocumentCoverage,
    float fFrequentTermCoverage,
    unsigned int uiMaxTokenizationErrors
)
{


    int                         iError = SRCH_NoError;
    unsigned char               *pucIndexNamesCopy = NULL;
    unsigned char               *pucIndexNamePtr = NULL;
    unsigned char               *pucIndexNameStrtokPtr = NULL;
    struct phrasesIndexList     *ppilPhrasesIndexList = NULL;
    struct phrasesIndexList     *ppilPhrasesIndexListPtr = NULL;
    unsigned int                uiPhrasesIndexListLength = 0;
    unsigned int                uiDocumentCount = 0;
    void                        *pvLngTokenizer = NULL;
    void                        *pvLngStemmer = NULL;
    void                        *pvLngStopList = NULL;
    void                        *pvUtlStopTermsTrie = NULL;
    FILE                        *pfTextFile = NULL;
    FILE                        *pfOutputFile = NULL;

    unsigned char               *pucText = NULL;
    unsigned char               *pucTextPtr = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexNames) == false);
    ASSERT(!((bUtlStringsIsStringNULL(pucLanguageCode) == false) && (bLanguageFirstTerm == true)));
    ASSERT((bUtlStringsIsStringNULL(pucStopListName) == false) || (bUtlStringsIsStringNULL(pucStopListFilePath) == true));
    ASSERT((bUtlStringsIsStringNULL(pucStopListFilePath) == false) || (bUtlStringsIsStringNULL(pucStopListFilePath) == true));
    ASSERT((bUtlStringsIsStringNULL(pucLanguageCode) == false) || (bUtlStringsIsStringNULL(pucLanguageCode) == true));
    ASSERT((bLanguageFirstTerm == false) || (bLanguageFirstTerm == true));
    ASSERT((bUtlStringsIsStringNULL(pucTextFilePath) == false) || (bUtlStringsIsStringNULL(pucTextFilePath) == true));
    ASSERT(uiMinPhraseCount > 0);
    ASSERT(uiMaxPhraseCount >= 0);
    ASSERT(!((uiMaxPhraseCount > 0) && (uiMinPhraseCount > uiMaxPhraseCount)));
    ASSERT((fPhrasePercentage > 0) && (fPhrasePercentage <= 100));
    ASSERT((fDocumentCoverage > 0) && (fDocumentCoverage <= 100));
    ASSERT(fFrequentTermCoverage >= 0);
    ASSERT(uiMaxTokenizationErrors >= 0);


    /* Check for phrases file path */
    if ( bUtlStringsIsStringNULL(pucPhrasesFilePath) == false ) {

        /* Open the phrases file */
        if ( (pfOutputFile = s_fopen(pucPhrasesFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the phrase file: '%s'.", pucPhrasesFilePath);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
    }
    else {
        pfOutputFile = stdout;
    }


    /* Duplicate the index names, we do this because s_strtok_r() destroys the buffer */
    if ( (pucIndexNamesCopy = (unsigned char *)s_strdup(pucIndexNames)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchPhrasesExtractPhrases;
    }

    /* Loop parsing the index names */
    for ( pucIndexNamePtr = s_strtok_r(pucIndexNamesCopy, SRCH_PHRASES_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr), uiPhrasesIndexListLength = 0; 
            pucIndexNamePtr != NULL; 
            pucIndexNamePtr = s_strtok_r(NULL, SRCH_PHRASES_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr), uiPhrasesIndexListLength++ ) {

        /* Allocate space in the phrases index list for this index */
        if ( (ppilPhrasesIndexListPtr = (struct phrasesIndexList *)s_realloc(ppilPhrasesIndexList, (size_t)(sizeof(struct phrasesIndexList) * (uiPhrasesIndexListLength + 1)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }

        /* Hand over the pointer */
        ppilPhrasesIndexList = ppilPhrasesIndexListPtr;

        /* Dereference the phrases index list pointer */
        ppilPhrasesIndexListPtr = ppilPhrasesIndexList + uiPhrasesIndexListLength;

        /* Open the index */
        if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexNamePtr, SRCH_INDEX_INTENT_SEARCH, &ppilPhrasesIndexListPtr->psiSrchIndex)) != 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexNamePtr, iError);
            goto bailFromiSrchPhrasesExtractPhrases;
        }

        /* Incremement the document count */
        uiDocumentCount += ppilPhrasesIndexListPtr->psiSrchIndex->uiDocumentCount;
    }


    /* Create the tokenizer */
    if ( (iError = iLngTokenizerCreateByID(pucConfigurationDirectoryPath, ppilPhrasesIndexList->psiSrchIndex->uiTokenizerID, ppilPhrasesIndexList->psiSrchIndex->uiLanguageID, &pvLngTokenizer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a tokenizer, lng error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchPhrasesExtractPhrases;
    }

    /* Create the stemmer */
    if ( (iError = iLngStemmerCreateByID(ppilPhrasesIndexList->psiSrchIndex->uiStemmerID, ppilPhrasesIndexList->psiSrchIndex->uiLanguageID, &pvLngStemmer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a stemmer, lng error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchPhrasesExtractPhrases;
    }


    /* We are using an internal stop list if the stop list name is set */
    if ( pucStopListName != NULL ) {
    
        unsigned int    uiStopListID = LNG_STOP_LIST_ANY_ID;

        /* Get the stop list ID */
        if ( (iError = iLngGetStopListIDFromName(pucStopListName, &uiStopListID)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list ID from the stop list name: '%s', lng error: %d.", pucStopListName, iError); 
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
    
        /* Get the stop list handle */
        if ( (iError = iLngStopListCreateByID(uiStopListID, ppilPhrasesIndexList->psiSrchIndex->uiLanguageID, &pvLngStopList)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the stop list, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
    }
    /* We are using a stop list file if the stop list file path is set */
    else if ( pucStopListFilePath != NULL ) {

        /* Get the stop list handle */
        if ( (iError = iLngStopListCreateByIDFromFile(pucStopListFilePath, ppilPhrasesIndexList->psiSrchIndex->uiLanguageID, &pvLngStopList)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the stop list from the stop list file: '%s', lng error: %d.", pucStopListFilePath, iError); 
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
    }
    
    /* We have a stop list to process if the stop list pointer is set */
    if ( pvLngStopList != NULL ) {
    
        wchar_t         **ppwcStopListTermList = NULL;
        unsigned int    uiStopListTermListLength = 0;

        /* Get the stop list term list */
        if ( (iError = iLngStopListGetTermList(pvLngStopList, &ppwcStopListTermList, &uiStopListTermListLength)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop term list, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
    
        /* Create a trie for the stop terms */
        if ( (iError = iUtlTrieCreate(&pvUtlStopTermsTrie)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the stop terms, utl error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }

        /* Add the stop terms to the trie */
        if ( (iError = iSrchPhrasesAddStopTermsToTrie(pvLngStemmer, pvUtlStopTermsTrie, ppwcStopListTermList, uiStopListTermListLength)) != SRCH_NoError ) {
            goto bailFromiSrchPhrasesExtractPhrases;
        }
    }



    /* Text file path */
    if ( bUtlStringsIsStringNULL(pucTextFilePath) == false ) {

        off_t       zTextFileLength = 0;

        /* Open the text file */
        if ( (pfTextFile = s_fopen(pucTextFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the text file: '%s'.", pucTextFilePath);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }

        /* Get the text file length */
        if ( (iError = iUtlFileGetFileLength(pfTextFile, &zTextFileLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get length of the text file: '%s', utl error: %d.", pucTextFilePath, iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
            
        /* Allocate space for the text */
        if ( (pucText = s_malloc(zTextFileLength + 1)) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
            
        /* Read the text from the file */
        if ( s_fread(pucText, 1, zTextFileLength, pfTextFile) != zTextFileLength ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the text file.");
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesExtractPhrases;
        }
        
        /* NULL terminate the text */
        pucText[zTextFileLength] = '\0';


        /* Extract the first term as the language */
        if ( bLanguageFirstTerm == true ) {
        
            /* Set the language code */
            pucLanguageCode = pucText;
            
            /* Set the text pointer to point to the start of the text */
            pucTextPtr = s_strchr(pucText, ' ') + 1;
            
            /* Null out the space byte */            
            *(pucTextPtr - 1) = '\0';
        }
        else {
            pucTextPtr = pucText;
        }


        /* Extract the phrases */
        iError = iSrchPhrasesExtractPhrasesFromText(ppilPhrasesIndexList, uiPhrasesIndexListLength, pvLngTokenizer, pvLngStemmer, pvUtlStopTermsTrie, pucTextPtr, pucLanguageCode, 
                pfOutputFile, uiDocumentCount, uiMinPhraseCount, uiMaxPhraseCount, fPhrasePercentage, fDocumentCoverage, fFrequentTermCoverage, uiMaxTokenizationErrors);

    }
    /* Stdin */
    else {

        unsigned int    uiTextLength = 0;
        unsigned int    uiTextCapacity = 0;

        /* Loop forever */
        while ( true ) {
    
            /* Reset the text */
            if ( pucText != NULL ) {
                pucText[0] = '\0';
            }
            uiTextLength = 0;
    
            /* Read while there is data to be read, ie until we hit a new-line of some sort */
            while ( true ) {
    
                /* Allocate/reallocate the text if needed */
                if ( (pucText == NULL) || (uiTextCapacity == 0) || ((uiTextLength + BUFSIZ) > uiTextCapacity) ) {
    
                    /* Reallocate extending by BUFSIZ */
                    if ( (pucTextPtr = (unsigned char *)s_realloc(pucText, (size_t)(sizeof(unsigned char) * (uiTextCapacity + BUFSIZ)))) == NULL ) {
                        iError = SRCH_MemError;
                        goto bailFromiSrchPhrasesExtractPhrases;
                    }
    
                    /* Hand over the pointer */
                    pucText = pucTextPtr;
    
                    /* Clear the new allocation */
                    s_memset(pucText + uiTextCapacity, 0, BUFSIZ);
    
                    /* Set the new text length */
                    uiTextCapacity += BUFSIZ;
                }

    
                /* Read the next text chunk, appending to the current text */
                if ( s_fgets(pucText + uiTextLength, uiTextCapacity - uiTextLength, stdin) == NULL ) {
                    
                    /* Check if we got an error */
                    if ( s_ferror(stdin) != 0 ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the text.");
                        iError = SRCH_MiscError;
                        goto bailFromiSrchPhrasesExtractPhrases;
                    }
                    
                    /* Check if we have hit the end of the file */
                    else if ( s_feof(stdin) != 0 ) {
                        
                        /* No text so we bail */
                        if ( s_strlen(pucText) == 0 ) {
                            iError = SRCH_NoError;
                            goto bailFromiSrchPhrasesExtractPhrases;
                        }
                        /* Otherwise we break out of the loop and extract phrases from the text */
                        else {
                            break;
                        }
                    }
                }
    
    
                /* Get the new text length here for optimization */
                uiTextLength = s_strlen(pucText);

    
                /* Erase the trailing new line - be platform sensitive, handle PC first, then Unix and Mac  */
                if ( (uiTextLength >= 2) && (pucText[uiTextLength - 2] == '\r') ) {
                    uiTextLength -= 2;
                    pucTextPtr = pucText + uiTextLength;
                }
                else if ( (uiTextLength >= 1) && ((pucText[uiTextLength - 1] == '\n') || (pucText[uiTextLength - 1] == '\r')) ) {
                    uiTextLength -= 1;
                    pucTextPtr = pucText + uiTextLength;
                }
                else {
                    pucTextPtr = NULL;
                }
    
    
                /* See if we found a trailing new line */
                if ( pucTextPtr != NULL ) {
                    
                    /* Erase it */
                    *pucTextPtr = '\0';
            
                    /* We have read the line, so we break out */
                    break;
                }
            }    /* while ( true ) */


            /* Extract the first term as the language */
            if ( bLanguageFirstTerm == true ) {
            
                /* Set the language code */
                pucLanguageCode = pucText;
                
                /* Set the text pointer to point to the start of the text */
                pucTextPtr = s_strchr(pucText, ' ') + 1;
                
                /* Null out the space byte */            
                *(pucTextPtr - 1) = '\0';
            }
            else {
                pucTextPtr = pucText;
            }


            /* Extract the phrases */
            iError = iSrchPhrasesExtractPhrasesFromText(ppilPhrasesIndexList, uiPhrasesIndexListLength, pvLngTokenizer, pvLngStemmer, pvUtlStopTermsTrie, pucTextPtr, pucLanguageCode, 
                    pfOutputFile, uiDocumentCount, uiMinPhraseCount, uiMaxPhraseCount, fPhrasePercentage, fDocumentCoverage, fFrequentTermCoverage, uiMaxTokenizationErrors);

        }    /* while ( true ) */
    }



    /* Bail label */
    bailFromiSrchPhrasesExtractPhrases:


    /* Free the text */
    s_free(pucText);

    /* Close the text file */
    s_fclose(pfTextFile);
    
    /* Close the phrases file */
    s_fclose(pfOutputFile);
    
    /* Free the duplicate index names */
    s_free(pucIndexNamesCopy);

    /* Free the phrases index list */
    if ( ppilPhrasesIndexList != NULL ) {
        
        unsigned int    uiI = 0;
        
        /* Loop over each index closing it */
        for ( uiI = 0, ppilPhrasesIndexListPtr = ppilPhrasesIndexList; uiI < uiPhrasesIndexListLength; uiI++, ppilPhrasesIndexListPtr++ ) {
            iSrchIndexClose(ppilPhrasesIndexListPtr->psiSrchIndex);
        }
        
        /* Finally we free the terms index list */
        s_free(ppilPhrasesIndexList);
    }

    /* Free the tokenizer structure */
    iLngTokenizerFree(pvLngTokenizer);
    pvLngTokenizer = NULL;

    /* Free the stemmer */
    iLngStemmerFree(pvLngStemmer);
    pvLngStemmer = NULL;

    /* Free the stop list */
    iLngStopListFree(pvLngStopList);
    pvLngStopList = NULL;

    /* Free the stop terms trie */
    iUtlTrieFree(pvUtlStopTermsTrie, false);
    pvUtlStopTermsTrie = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPhrasesAddStopTermsToTrie()

    Purpose:    Add the stop terms to the trie.

    Parameters: pvLngStemmer                stemmer
                pvUtlStopTermsTrie          stop terms trie
                ppwcStopListTermList        stop term list
                uiStopListTermListLength    stop term list length

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchPhrasesAddStopTermsToTrie
(
    void *pvLngStemmer,
    void *pvUtlStopTermsTrie,
    wchar_t **ppwcStopListTermList,
    unsigned int uiStopListTermListLength
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiMiddle = 0;
    unsigned char   pucStopTerm[SRCH_TERM_LENGTH_MAXIMUM + 1] = {'\0'};
    wchar_t         pwcStopTerm[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};
    unsigned int    uiTermLength = SRCH_TERM_LENGTH_MAXIMUM + 1;
    unsigned int    *puiData = NULL;


     ASSERT(pvLngStemmer != NULL);
     ASSERT(pvUtlStopTermsTrie != NULL);
     ASSERT(ppwcStopListTermList != NULL);
     ASSERT(uiStopListTermListLength >= 0);
     


    /* Bail if we have hit the end of the list */
    if ( uiStopListTermListLength == 0 ) {
        return (0);
    }

    
    /* Add the term in the middle of the list */
    uiMiddle = uiStopListTermListLength / 2;

    /* Convert the stop term from wide characters to utf-8 */
    if ( (iError = iLngConvertWideStringToUtf8_s(ppwcStopListTermList[uiMiddle], 0, pucStopTerm, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a stop term from wide characters to utf-8, lng error: %d.", iError);
        return (SRCH_MiscError);
    }

    /* Look up/store the term - skip it if it cant be looked-up/stored */
    if ( (iError = iUtlTrieAdd(pvUtlStopTermsTrie, pucStopTerm, (void ***)&puiData)) == UTL_NoError ) {
        (*puiData) = 1;
    }
    else {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add an entry to the stop terms trie, stop term: '%s', utl error: %d.", pucStopTerm, iError);
    }



    /* Make a copy of the stop term and get its length */
    s_wcscpy(pwcStopTerm, ppwcStopListTermList[uiMiddle]);
    uiTermLength = s_wcslen(pwcStopTerm);

    /* Stem the term */
    if ( (iError = iLngStemmerStemTerm(pvLngStemmer, pwcStopTerm, uiTermLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a stop term, lng error: %d.", iError);
        return (SRCH_MiscError);
    }

    
    /* Add the stemmed stop term if it was not stemmed out of existance and if it was actually modified */
    if ( (bUtlStringsIsStringNULL(pwcStopTerm) == false) && (s_wcscmp(pwcStopTerm, ppwcStopListTermList[uiMiddle]) != 0) ) {
        
        /* Convert the stop term from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(pwcStopTerm, 0, pucStopTerm, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a stop term from wide characters to utf-8, lng error: %d.", iError);
            return (SRCH_MiscError);
        }
    
        /* Look up/store the term - skip it if it cant be looked-up/stored */
        if ( (iError = iUtlTrieAdd(pvUtlStopTermsTrie, pucStopTerm, (void ***)&puiData)) == UTL_NoError ) {
            (*puiData) = 1;
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add an entry to the stop terms trie, stop term: '%s', utl error: %d.", pucStopTerm, iError);
        }
    }


    /* Do the low end of the list */
    if ( (iError = iSrchPhrasesAddStopTermsToTrie(pvLngStemmer, pvUtlStopTermsTrie, ppwcStopListTermList, uiMiddle)) != SRCH_NoError ) {
        return (iError);
    }


    /* Do the high end of the list */
    if ( (iError = iSrchPhrasesAddStopTermsToTrie(pvLngStemmer, pvUtlStopTermsTrie, ppwcStopListTermList + uiMiddle + 1, uiStopListTermListLength - uiMiddle - 1)) != SRCH_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPhrasesExtractPhrasesFromText()

    Purpose:    This function extracts the phrases from the text.

    Parameters: ppilPhrasesIndexList        phrases index list structure
                uiPhrasesIndexListLength    phrases index list length
                pvLngTokenizer              tokenizer
                pvLngStemmer                stemmer
                pvUtlStopTermsTrie          stop terms trie (optional)
                pucText                     text
                pucLanguageCode             language code
                pfOutputFile                output file
                uiDocumentCount             document count
                uiMinPhraseCount            minimum phrase count
                uiMaxPhraseCount            maximum phrase count
                fPhrasePercentage           phrase percentage
                fDocumentCoverage           document coverage
                fFrequentTermCoverage       frequent term coverage
                uiMaxTokenizationErrors     maximum tokenization errors

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchPhrasesExtractPhrasesFromText
(
    struct phrasesIndexList *ppilPhrasesIndexList, 
    unsigned int uiPhrasesIndexListLength,
    void *pvLngTokenizer, 
    void *pvLngStemmer,
    void *pvUtlStopTermsTrie,
    unsigned char *pucText,
    unsigned char *pucLanguageCode,
    FILE *pfOutputFile,
    unsigned int uiDocumentCount,
    unsigned int uiMinPhraseCount,
    unsigned int uiMaxPhraseCount,
    float fPhrasePercentage,
    float fDocumentCoverage,
    float fFrequentTermCoverage,
    unsigned int uiMaxTokenizationErrors
)
{

    int                         iError = UTL_NoError;
    unsigned int                uiI = 0;
    void                        *pvUtlPhrasesTrie = NULL;
    struct phrasesSort          *ppsPhrasesSort = NULL;
    struct phrasesIndexList     *ppilPhrasesIndexListPtr = NULL;
    unsigned int                uiTotalPhraseCount = 0;
    unsigned int                uiUniquePhraseCount = 0;
    unsigned int                uiUsedTermCount = 0;
    unsigned int                uiPhrasesSortLength = 0;
    unsigned int                uiPhrasesSortIndex = 0;


    ASSERT(ppilPhrasesIndexList != NULL);
    ASSERT(uiPhrasesIndexListLength > 0);
    ASSERT(pvLngTokenizer != NULL);
    ASSERT(pvLngStemmer != NULL);
    ASSERT((pvUtlStopTermsTrie != NULL) || (pvUtlStopTermsTrie == NULL));
    ASSERT(bUtlStringsIsStringNULL(pucText) == false);
    ASSERT(bUtlStringsIsStringNULL(pucLanguageCode) == false);
    ASSERT(pfOutputFile != NULL);
    ASSERT(uiDocumentCount > 0);
    ASSERT(uiMinPhraseCount > 0);
    ASSERT(uiMaxPhraseCount >= 0);
    ASSERT(!((uiMaxPhraseCount > 0) && (uiMinPhraseCount > uiMaxPhraseCount)));
    ASSERT((fPhrasePercentage > 0) && (fPhrasePercentage <= 100));
    ASSERT((fDocumentCoverage > 0) && (fDocumentCoverage <= 100));
    ASSERT(fFrequentTermCoverage >= 0);
    ASSERT(uiMaxTokenizationErrors >= 0);



    /* Parse the text into the trie */
    if ( (iError = iSrchPhrasesParseTextToTrie(ppilPhrasesIndexList->psiSrchIndex, pucText, &pvUtlPhrasesTrie, &uiTotalPhraseCount, &uiUniquePhraseCount)) != SRCH_NoError ) {
        goto bailFromiSrchPhrasesExtractPhrasesFromText;
    }


    /* Extract the phrases if there were any */
    if ( uiUniquePhraseCount > 0 ) {

        /* Set the size of the phrases sort array from the unique phrase count */
        uiPhrasesSortLength = uiUniquePhraseCount;

        /* Allocate an phrases sort structure array */
        if ( (ppsPhrasesSort = (struct phrasesSort *)s_malloc((size_t)(uiPhrasesSortLength * sizeof(struct phrasesSort)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchPhrasesExtractPhrasesFromText;
        }

        /* Loop over each index in the phrases index list */
        for ( uiI = 0, ppilPhrasesIndexListPtr = ppilPhrasesIndexList; uiI < uiPhrasesIndexListLength; uiI++, ppilPhrasesIndexListPtr++ ) {
        
            /* Dereference the index for convenience */
            struct srchIndex    *psiSrchIndex = ppilPhrasesIndexListPtr->psiSrchIndex;

            /* Reset the terms sort index */
            uiPhrasesSortIndex = 0;

            /* Loop through each entry in the trie calling the call back function for each entry */
            if ( (iError = iUtlTrieLoop(pvUtlPhrasesTrie, NULL, (int (*)())iSrchPhrasesCallBackFunction, psiSrchIndex, pucLanguageCode, pvLngTokenizer, pvLngStemmer, 
                    pvUtlStopTermsTrie, (double)fFrequentTermCoverage, uiMaxTokenizationErrors, ppsPhrasesSort, uiPhrasesSortLength, &uiPhrasesSortIndex)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the entries in the phrases trie, utl error: %d.", iError);
                iError = SRCH_MiscError;
                goto bailFromiSrchPhrasesExtractPhrasesFromText;
            }
        }

        /* List the phrases from the phrases sort structure array */
        if ( (iError = iSrchPhrasesListPhrases(ppsPhrasesSort, uiPhrasesSortLength, pfOutputFile, uiDocumentCount, uiMinPhraseCount, uiMaxPhraseCount, 
                fPhrasePercentage, fDocumentCoverage, &uiUsedTermCount)) != SRCH_NoError ) {
            goto bailFromiSrchPhrasesExtractPhrasesFromText;
        }

        /* Log */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Phrases, total: %u, unique: %u, used: %u.", uiTotalPhraseCount, uiUniquePhraseCount, uiUsedTermCount); */
    }



    /* Bail label */
    bailFromiSrchPhrasesExtractPhrasesFromText:


    /* Write the new line and flush the phrase file handle */
    fprintf(pfOutputFile, "\n");
    s_fflush(pfOutputFile);


    /* Free the phrases sort structure array */
    if ( ppsPhrasesSort != NULL ) {
    
        struct phrasesSort  *ppsPhrasesSortPtr = NULL;

        for ( uiI = 0, ppsPhrasesSortPtr = ppsPhrasesSort; uiI < uiPhrasesSortLength; uiI++, ppsPhrasesSortPtr++ ) {
            s_free(ppsPhrasesSortPtr->pucPhrase);
        }

        s_free(ppsPhrasesSort);
    }

    /* Free the trie */
    iUtlTrieFree(pvUtlPhrasesTrie, false);
    pvUtlPhrasesTrie = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPhrasesParseTextToTrie()

    Purpose:    This function prepares a trie from a phrases string.

    Parameters: psiSrchIndex            index structure
                pucText                 text
                ppvUtlPhrasesTrie       return pointer for the trie
                puiTotalPhraseCount     return pointer for the total phrase count
                puiUniquePhraseCount    return pointer for the unique phrase count

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchPhrasesParseTextToTrie
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucText,
    void **ppvUtlPhrasesTrie,
    unsigned int *puiTotalPhraseCount,
    unsigned int *puiUniquePhraseCount
)
{


    int             iError = UTL_NoError;
    void            *pvUtlPhrasesTrie = NULL;
    unsigned char   *pucTextPtr = NULL;
    unsigned char   *pucTextStrtokPtr = NULL;
    unsigned int    *puiData = NULL;
    

    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucText) == false);
    ASSERT(ppvUtlPhrasesTrie != NULL);
    ASSERT(puiTotalPhraseCount != NULL);
    ASSERT(puiUniquePhraseCount != NULL);


    /* Create a trie for the phrases */
    if ( (iError = iUtlTrieCreate(&pvUtlPhrasesTrie)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the phrases, utl error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchPhrasesParseTextToPhrasesTrie;
    }


    /* Loop while we have phrases */
    for ( pucTextPtr = s_strtok_r(pucText, "\t", (char **)&pucTextStrtokPtr); 
            pucTextPtr != NULL;
            pucTextPtr = s_strtok_r(NULL, "\t", (char **)&pucTextStrtokPtr) ) {
        
        /* Look up/store the phrase - skip it if it cant be looked-up/stored */
        if ( (iError = iUtlTrieAdd(pvUtlPhrasesTrie, pucTextPtr, (void ***)&puiData)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add an entry to the phrases trie, phrase: '%s', utl error: %d.", pucTextPtr, iError);
            iError = SRCH_NoError;
        }
        else {
            /* Increment our counts */
            (*puiTotalPhraseCount)++;
            (*puiData) += 1;
            (*puiUniquePhraseCount) += (*puiData == 1) ? 1 : 0;
        }
    }



    /* Bail label */
    bailFromiSrchPhrasesParseTextToPhrasesTrie:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
    
        /* Set the return pointer */
        *ppvUtlPhrasesTrie = pvUtlPhrasesTrie;
    }
    else {
        /* Free the trie */
        iUtlTrieFree(pvUtlPhrasesTrie, false);
        pvUtlPhrasesTrie = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPhrasesCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlTrieLoop()
                and gets called for every key in the trie. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlTrieLoop().

    Parameters: pucKey      key (phrase)
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
static int iSrchPhrasesCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list                 ap_;
    int                     iError = SRCH_NoError;
    int                     iStatus = 0;
    struct srchIndex        *psiSrchIndex = NULL;
    unsigned char           *pucLanguageCode = NULL;
    void                    *pvLngTokenizer = NULL;
    void                    *pvLngStemmer = NULL;
    void                    *pvUtlStopTermsTrie = NULL;
    float                   fFrequentTermCoverage = 0;
    unsigned int            uiMaxTokenizationErrors = 0;
    struct phrasesSort      *ppsPhrasesSort = NULL;
    unsigned int            uiPhrasesSortLength = 0;
    struct phrasesSort      *ppsPhrasesSortPtr = NULL;
    unsigned int            *puiPhrasesSortIndex = NULL;
    unsigned int            iTokenizationErrorCount = 0;

    wchar_t                 *pwcText = NULL;
    
    unsigned int            uiLanguageID = LNG_LANGUAGE_ANY_ID;

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPhrasesCallBackFunction - [%s][%u]", pucKey, (unsigned int)pvData); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    pucLanguageCode = (unsigned char *)va_arg(ap_, unsigned char *);
    pvLngTokenizer = (void *)va_arg(ap_, void *);
    pvLngStemmer = (void *)va_arg(ap_, void *);
    pvUtlStopTermsTrie = (void *)va_arg(ap_, void *);
    fFrequentTermCoverage = (float)va_arg(ap_, double);
    uiMaxTokenizationErrors = (unsigned int)va_arg(ap_, unsigned int);
    ppsPhrasesSort = (struct phrasesSort *)va_arg(ap_, struct phrasesSort *);
    uiPhrasesSortLength = (unsigned int)va_arg(ap_, unsigned int);
    puiPhrasesSortIndex = (unsigned int *)va_arg(ap_, unsigned int *);
    va_end(ap_);


    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucLanguageCode) == false);
    ASSERT(pvLngTokenizer != NULL);
    ASSERT(pvLngStemmer != NULL);
    ASSERT((pvUtlStopTermsTrie != NULL) || (pvUtlStopTermsTrie == NULL));
    ASSERT(fFrequentTermCoverage >= 0);
    ASSERT(uiMaxTokenizationErrors >= 0);
    ASSERT(ppsPhrasesSort != NULL);
    ASSERT(puiPhrasesSortIndex != NULL);
    ASSERT(*puiPhrasesSortIndex < uiPhrasesSortLength);


    /* Dereference the phrases sort structure */
    ppsPhrasesSortPtr = (ppsPhrasesSort + (*puiPhrasesSortIndex));

    /* We return here if the weight for this phrase is already negative */
    if ( ppsPhrasesSort->fWeight < 0 ) {
        return (0);
    }


    /* Make a copy of the key and the data, the data contains the phrase count in the text */
    if ( ppsPhrasesSortPtr->pucPhrase == NULL ) {

        /* Make a copy of the key */
        if ( (ppsPhrasesSortPtr->pucPhrase = (unsigned char *)s_strdup(pucKey)) == NULL ) {
            return (-1);
        }

        /* This cast complains on 64 bit architectures */
        ppsPhrasesSortPtr->uiPhraseCount += (unsigned int)pvData;
    }


    /* Convert the text from utf-8 to wide characters, pwcText is allocated */
    if ( (iError = iLngConvertUtf8ToWideString_d(ppsPhrasesSortPtr->pucPhrase, 0, &pwcText)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the text from utf-8 to wide characters, lng error: %d.", iError);
        iStatus = -1;
        goto bailFromiSrchPhrasesCallBackFunction;
    }


    /* Set the language ID from the language code, default to the index language */
    if ( bUtlStringsIsStringNULL(pucLanguageCode) == false ) {
        if ( iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID) != LNG_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
            uiLanguageID = psiSrchIndex->uiLanguageID;
        }
    }
    else {
        uiLanguageID = psiSrchIndex->uiLanguageID;
    }


    /* Parse the text */
    if ( (iError = iLngTokenizerParseString(pvLngTokenizer, psiSrchIndex->uiLanguageID, pwcText, s_wcslen(pwcText))) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse the text, lng error: %d.", iError);
        iStatus = -1;
        goto bailFromiSrchPhrasesCallBackFunction;
    }


    /* Process tokens, control the loop from within */
    while ( true ) {

        wchar_t     *pwcTermStartPtr = NULL;
        wchar_t     *pwcTermEndPtr = NULL;
        wchar_t     wcTermEnd = L'\0';

        /* Get the next token in the text */
        if ( (iError = iLngTokenizerGetToken(pvLngTokenizer, &pwcTermStartPtr, &pwcTermEndPtr)) != LNG_NoError ) {
        
            /* Increment the tokenization error count and bail if we reached the maximum allowed */
            if ( (++iTokenizationErrorCount) > uiMaxTokenizationErrors ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a token from the text, lng error: %d.", iError);
                iStatus = -1;
                goto bailFromiSrchPhrasesCallBackFunction;
            }

            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to get a token from the text, lng error: %d.", iError);
        }

        /* Break if this was the last token */
        if ( pwcTermStartPtr == NULL ) {
            break;
        }
        

        /* Save the term end character and NULL terminate the term */ 
        wcTermEnd = *pwcTermEndPtr;
        *pwcTermEndPtr = L'\0';

        /* Set the term weight in the phrases sort structure */
        if ( (iStatus = iSrchPhrasesSetTermWeight(psiSrchIndex, fFrequentTermCoverage, pvLngStemmer, pvUtlStopTermsTrie, ppsPhrasesSortPtr, pwcTermStartPtr)) == -1 ) {
/*             iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the term weight, srch error: %d.", iError); */
/*             iStatus = -1; */
/*             goto bailFromiSrchPhrasesCallBackFunction; */
        }
        iStatus = 0;

        /* Restore the term end character */
        *pwcTermEndPtr = wcTermEnd;


        /* Process components, control the loop from within */
        while ( true ) {

            wchar_t     *pwcComponentStartPtr = NULL;
            wchar_t     *pwcComponentEndPtr = NULL;
            wchar_t     wcComponentEnd = L'\0';

            /* Get the next component for this token */
            if ( (iError = iLngTokenizerGetComponent(pvLngTokenizer, &pwcComponentStartPtr, &pwcComponentEndPtr)) != LNG_NoError ) {
                
                /* Increment the tokenization error count and bail if we reached the maximum allowed */
                if ( (++iTokenizationErrorCount) > uiMaxTokenizationErrors ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the text, lng error: %d.", iError);
                    iStatus = -1;
                    goto bailFromiSrchPhrasesCallBackFunction;
                }
    
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to get a component from the text, lng error: %d.", iError);
            }
            
            /* Break if this was the last component */
            if ( pwcComponentStartPtr == NULL ) {
                break;
            }
            
    
            /* Save the component end character and NULL terminate the component */ 
            wcComponentEnd = *pwcComponentEndPtr;
            *pwcComponentEndPtr = L'\0';

            /* Function to score the component */
            if ( (iStatus = iSrchPhrasesSetTermWeight(psiSrchIndex, fFrequentTermCoverage, pvLngStemmer, pvUtlStopTermsTrie, ppsPhrasesSortPtr, pwcComponentStartPtr)) == -1 ) {
/*                 iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the term weight, srch error: %d.", iError); */
/*                 iStatus = -1; */
/*                 goto bailFromiSrchPhrasesCallBackFunction; */
            }
            iStatus = 0;

            /* Restore the component end character */
            *pwcComponentEndPtr = wcComponentEnd;
        }
    }


    /* Increment the indent */
    (*puiPhrasesSortIndex)++;



    bailFromiSrchPhrasesCallBackFunction:


    return (iStatus);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPhrasesSetTermWeight()

    Purpose:    This function sets the term weight in the phrases sort structure.

    Parameters: psiSrchIndex            index structure
                fFrequentTermCoverage   frequent term coverage
                pvLngStemmer            stemmer
                pvUtlStopTermsTrie      stop terms trie (optional)
                ppsPhrasesSort          phrases sort structure
                pwcTerm                 term

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchPhrasesSetTermWeight
(
    struct srchIndex *psiSrchIndex,
    float fFrequentTermCoverage,
    void *pvLngStemmer, 
    void *pvUtlStopTermsTrie, 
    struct phrasesSort *ppsPhrasesSort, 
    wchar_t *pwcTerm
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiTermType = 0;
    unsigned int    uiTermCount = 0;
    unsigned int    uiDocumentCount = 0;
    unsigned long   ulIndexBlockID = 0;

    unsigned char   pucTerm[SRCH_PHRASES_MAX_TERM_LENGTH + 1] = {'\0'};
    unsigned int    ulTermLength = 0;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(fFrequentTermCoverage >= 0);
    ASSERT((pvUtlStopTermsTrie != NULL) || (pvUtlStopTermsTrie == NULL));
    ASSERT(pvLngStemmer != NULL);
    ASSERT(ppsPhrasesSort != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);


    /* We return here if the weight of this phrase is already negative */
    if ( ppsPhrasesSort->fWeight < 0 ) {
        return (SRCH_NoError);
    }


    /* Add the term in its original case state if it all in upper case */
    if ( bLngCaseIsWideStringAllUpperCase(pwcTerm) == true ) {

        /* Convert the term from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(pwcTerm, 0, pucTerm, SRCH_PHRASES_MAX_TERM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesRankTerm;
        }

        /* Get the term length */
        ulTermLength = s_strlen(pucTerm);

        /* Is the term long enough to deal with, note the restriction of 1 */
        if ( (ulTermLength < psiSrchIndex->uiTermLengthMinimum) || (ulTermLength == 1) ) {
            ppsPhrasesSort->fWeight = -1;
            iError = SRCH_NoError;
            goto bailFromiSrchPhrasesRankTerm;
        }

        /* Truncate the term if it is too long */
        if ( ulTermLength > psiSrchIndex->uiTermLengthMaximum ) {
            pucTerm[psiSrchIndex->uiTermLengthMaximum] = '\0';
        }

        /* Look up the term */
        if ( (iError = iSrchTermDictLookup(psiSrchIndex, pucTerm, NULL, 0, &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID)) == SRCH_NoError ) {
    
            /* Check if this term is frequent if the frequent term coverage has been set */
            if ( (fFrequentTermCoverage >  0) && (uiTermType == SPI_TERM_TYPE_REGULAR) ) {
                
                float   fTermCoverage = ((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100;
                
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', uiTermCount: %u, uiDocumentCount: %u, psiSrchIndex->uiDocumentCount: %u, fTermCoverage: %9.4f%%.",  */
/*                         pucTerm, uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount, fTermCoverage); */
            
                /* Flag this terms as frequent if its coverage is greater than the set frequent term coverage */
                if ( fTermCoverage > fFrequentTermCoverage ) {
                    uiTermType = SPI_TERM_TYPE_FREQUENT;
                }
            }

            /* If this term is a regular term, we work out its IDF weight */
            if ( uiTermType == SPI_TERM_TYPE_REGULAR ) {

                unsigned int    *puiData = NULL;
        
                /* Check if this term is in the stop list trie, this will set the data pointer if the term is there */
                if ( pvUtlStopTermsTrie != NULL ) {
                    iUtlTrieLookup(pvUtlStopTermsTrie, pucTerm, (void ***)&puiData);
                }

                /* Work out the term weight if it is not on the stop list, otherwise set it to -1 */
                if ( (puiData == NULL) || ((*puiData) == 0) ) {

                    /* Set the document count */
                    ppsPhrasesSort->uiDocumentCount = UTL_MACROS_MAX(ppsPhrasesSort->uiDocumentCount, uiDocumentCount);
    
                    /* This works out the IDF of the term */
                    ppsPhrasesSort->fWeight += SRCH_SEARCH_IDF_FACTOR(uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount) * ppsPhrasesSort->uiPhraseCount;
                }
                else {
                    ppsPhrasesSort->fWeight = -1;
                }
            }
            else if ( uiTermType == SPI_TERM_TYPE_STOP ) {
                ppsPhrasesSort->fWeight = -1;
            }
            else if ( uiTermType == SPI_TERM_TYPE_FREQUENT ) {
                ppsPhrasesSort->fWeight = -1;
            }
            else {
                ppsPhrasesSort->fWeight = -1;
            }
        }
    }

    /* Convert the term to lower case and add it */
    else {

        wchar_t     pwcTermCopy[SRCH_PHRASES_MAX_TERM_LENGTH + 1] = {L'\0'};


        /* Copy the term, this is because we side-effect */
        s_wcsncpy(pwcTermCopy, pwcTerm, SRCH_PHRASES_MAX_TERM_LENGTH + 1);

        /* Convert the term to lower case */
        pwcLngCaseConvertWideStringToLowerCase(pwcTermCopy);

        /* Stem the term */
        if ( (iError = iLngStemmerStemTerm(pvLngStemmer, pwcTermCopy, 0)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a term, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesRankTerm;
        }

        /* Skip the term if it was stemmed out of existence */
        if ( bUtlStringsIsWideStringNULL(pwcTermCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchPhrasesRankTerm;
        }
        
        /* Convert the term from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(pwcTermCopy, 0, pucTerm, SRCH_PHRASES_MAX_TERM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchPhrasesRankTerm;
        }

        /* Get the term length */
        ulTermLength = s_strlen(pucTerm);

        /* Is the term long enough to deal with, note the restriction of 1 */
        if ( (ulTermLength < psiSrchIndex->uiTermLengthMinimum) || (ulTermLength == 1) ) {
            ppsPhrasesSort->fWeight = -1;
            iError = SRCH_NoError;
            goto bailFromiSrchPhrasesRankTerm;
        }

        /* Truncate the term if it is too long */
        if ( ulTermLength > psiSrchIndex->uiTermLengthMaximum ) {
            pucTerm[psiSrchIndex->uiTermLengthMaximum] = '\0';
        }

        /* Look up the term */
        if ( (iError = iSrchTermDictLookup(psiSrchIndex, pucTerm, NULL, 0, &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID)) == SRCH_NoError ) {
    
            /* Check if this term is frequent if the frequent term coverage has been set */
            if ( (fFrequentTermCoverage >  0) && (uiTermType == SPI_TERM_TYPE_REGULAR) ) {
                
                float   fTermCoverage = ((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100;
                
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', uiTermCount: %u, uiDocumentCount: %u, psiSrchIndex->uiDocumentCount: %u, fTermCoverage: %9.4f%%.",  */
/*                         pucTerm, uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount, fTermCoverage); */
            
                /* Flag this terms as frequent if its coverage is greater than the set frequent term coverage */
                if ( fTermCoverage > fFrequentTermCoverage ) {
                    uiTermType = SPI_TERM_TYPE_FREQUENT;
                }
            }

            /* If this term is a regular term, we work out its IDF weight */
            if ( uiTermType == SPI_TERM_TYPE_REGULAR ) {
    
                unsigned int    *puiData = NULL;
        
                /* Check if this term is in the stop list trie, this will set the data pointer if the term is there */
                if ( pvUtlStopTermsTrie != NULL ) {
                    iUtlTrieLookup(pvUtlStopTermsTrie, pucTerm, (void ***)&puiData);
                }

                /* Work out the term weight if it is not on the stop list, otherwise set it to -1 */
                if ( (puiData == NULL) || ((*puiData) == 0) ) {

                    /* Set the document count */
                    ppsPhrasesSort->uiDocumentCount = UTL_MACROS_MAX(ppsPhrasesSort->uiDocumentCount, uiDocumentCount);
        
                    /* This works out the IDF of the term */
                    ppsPhrasesSort->fWeight += SRCH_SEARCH_IDF_FACTOR(uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount) * ppsPhrasesSort->uiPhraseCount;
                }
                else {
                    ppsPhrasesSort->fWeight = -1;
                }
            }
            else if ( uiTermType == SPI_TERM_TYPE_STOP ) {
                ppsPhrasesSort->fWeight = -1;
            }
            else if ( uiTermType == SPI_TERM_TYPE_FREQUENT ) {
                ppsPhrasesSort->fWeight = -1;
            }
            else {
                ppsPhrasesSort->fWeight = -1;
            }
        }
    }



    /* Bail label */
    bailFromiSrchPhrasesRankTerm:
    

    return (iError);

}

/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPhrasesListPhrases()

    Purpose:    This function sorts the search feedback sort structure array 
                into descending weight order and searches for the top N phrases.

    Parameters: ppsPhrasesSort          phrases sort structure array
                uiPhrasesSortLength     phrases sort structure array length
                pfOutputFile            output file
                uiDocumentCount         document count
                uiMinPhraseCount        minimum phrase count
                uiMaxPhraseCount        maximum phrase count
                fPhrasePercentage       phrase percentage
                fDocumentCoverage       document coverage
                puiUsedPhraseCount      used phrase count

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchPhrasesListPhrases
(
    struct phrasesSort *ppsPhrasesSort, 
    unsigned int uiPhrasesSortLength, 
    FILE *pfOutputFile, 
    unsigned int uiDocumentCount, 
    unsigned int uiMinPhraseCount, 
    unsigned int uiMaxPhraseCount, 
    float fPhrasePercentage, 
    float fDocumentCoverage,
    unsigned int *puiUsedPhraseCount
)
{

    unsigned int            uiI = 0;
    struct phrasesSort      *ppsPhrasesSortPtr = NULL;
    unsigned int            uiCurrentTermCount = 0;
    unsigned int            uiLocalMaxTermCount = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPhrasesListPhrases"); */


    ASSERT(ppsPhrasesSort != NULL);
    ASSERT(uiPhrasesSortLength > 0);
    ASSERT(pfOutputFile != NULL);
    ASSERT(uiDocumentCount > 0);
    ASSERT(uiMinPhraseCount > 0);
    ASSERT(uiMaxPhraseCount >= 0);
    ASSERT(!((uiMaxPhraseCount > 0) && (uiMinPhraseCount > uiMaxPhraseCount)));
    ASSERT((fPhrasePercentage > 0) && (fPhrasePercentage <= 100));
    ASSERT((fDocumentCoverage > 0) && (fDocumentCoverage <= 100));
    ASSERT(puiUsedPhraseCount != NULL);


    /* Sort the relevance feedback phrases in descending weight */
    s_qsort(ppsPhrasesSort, uiPhrasesSortLength, sizeof(struct phrasesSort), (int (*)(const void *, const void *))iSrchPhrasesCompareByWeightDesc);


    /* Calculate the maximum phrase count to use based on the minimum phrase count and the percentage */
    if ( uiMaxPhraseCount == 0 ) {
        uiLocalMaxTermCount = (uiPhrasesSortLength * (fPhrasePercentage / 100));
        uiLocalMaxTermCount = UTL_MACROS_MAX(uiLocalMaxTermCount, uiMinPhraseCount);
    }
    else {
        uiLocalMaxTermCount = uiMaxPhraseCount;
    }


/*     printf("\n\n");  */
/*     printf("uiDocumentCount      : %10u\n", uiDocumentCount);  */
/*     printf("uiMaxPhraseCount     : %10u\n", uiMaxPhraseCount);  */
/*     printf("uiMinPhraseCount     : %10u\n", uiMinPhraseCount);  */
/*     printf("fPhrasePercentage     :       %4.1f%%\n", fPhrasePercentage);  */
/*     printf("fDocumentCoverage    :       %4.1f%%\n", fDocumentCoverage);  */
/*     printf("uiLocalMaxTermCount  : %10u\n", uiLocalMaxTermCount);  */
/*     printf("\n\n");  */


/*     printf("Phrase                                    Document #        Weight       Document coverage\n");  */
/*     printf("--------------------------------------------------------------------------------------------\n");  */


    /* Apply the first uiLocalMaxTermCount phrases to the search */
    for ( uiI = 0, uiCurrentTermCount = 0, ppsPhrasesSortPtr = ppsPhrasesSort; (uiI < uiPhrasesSortLength) && (uiCurrentTermCount < uiLocalMaxTermCount); uiI++, ppsPhrasesSortPtr++ ) {

        /* Only apply phrases which:
        **    - has a weight greater than 0
        **    - occurs in under #% of the documents (fDocumentCoverage)
        */
        if ( (ppsPhrasesSortPtr->fWeight > 0) && 
                ((float)ppsPhrasesSortPtr->uiDocumentCount / uiDocumentCount) <= (fDocumentCoverage / 100) ) {

/*             printf("%-40s  %10u     %9.4f     %9.4f %s\n", ppsPhrasesSortPtr->pucPhrase, ppsPhrasesSortPtr->uiDocumentCount,  */
/*                     ppsPhrasesSortPtr->fWeight, ((float)ppsPhrasesSortPtr->uiDocumentCount / uiDocumentCount) * 100, */
/*                     ((float)ppsPhrasesSortPtr->uiDocumentCount / uiDocumentCount) >= (fDocumentCoverage / 200) ? ">50% maximum cov" : ""); */

            /* Write the phrases out */    
            fprintf(pfOutputFile, "%s\t", ppsPhrasesSortPtr->pucPhrase);

            /* Increment the number of phrases we have applied */
            uiCurrentTermCount++;
        }
    }
    

    /* Set the return pointer */
    *puiUsedPhraseCount = uiCurrentTermCount;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPhrasesCompareByWeightDesc()

    Purpose:    This functions takes a two phrase sort structures and compares their weight.
                This function is used by the qsort call in iSrchPhrasesListPhrases().

    Parameters: pssPhrasesSort1     pointer to an phrase sort structure
                pssPhrasesSort2     pointer to an phrase sort structure

    Globals:    none

    Returns:    1 if pssPhrasesSort2 > pssPhrasesSort1, 
                -1 if pssPhrasesSort1 > pssPhrasesSort2, 
                and 0 if pssPhrasesSort1 == pssPhrasesSort2

*/
static int iSrchPhrasesCompareByWeightDesc
(
    struct phrasesSort *pssPhrasesSort1,
    struct phrasesSort *pssPhrasesSort2
)
{

    ASSERT(pssPhrasesSort1 != NULL);
    ASSERT(pssPhrasesSort2 != NULL);


    if ( pssPhrasesSort1->fWeight < pssPhrasesSort2->fWeight ) {
        return (1);
    }
    else if ( pssPhrasesSort1->fWeight > pssPhrasesSort2->fWeight ) {
        return (-1);
    }
    
    
    return (0);

}


/*---------------------------------------------------------------------------*/

