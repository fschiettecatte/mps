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

    Module:     mpsterms.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 April 2005

    Purpose:    

*/


/*---------------------------------------------------------------------------*/


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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.mpsterms"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Default min term count */
#define SRCH_TERMS_TERM_COUNT_MIN_DEFAULT           (10)

/* Default max term count - 0 mean no limit */
#define SRCH_TERMS_TERM_COUNT_MAX_DEFAULT           (0)

/* Default term percentage */
#define SRCH_TERMS_TERM_PERCENTAGE_DEFAULT          (20.0)

/* Default document coverage */
#define SRCH_TERMS_DOCUMENT_COVERAGE_DEFAULT        (2.5)

/* Default frequent term percentage */
#define SRCH_TERMS_FREQUENT_TERM_COVERAGE_DEFAULT   (0.0)

/* Default maximum errors */
#define SRCH_TERMS_ERRORS_MAX_DEFAULT               (20)


/* Default locale name */
#define SRCH_TERMS_LOCALE_NAME_DEFAULT              LNG_LOCALE_EN_US_UTF_8_NAME


/* Max term length */
#define SRCH_TERMS_MAX_TERM_LENGTH                  (1024)


/* Index name separators */
#define SRCH_TERMS_INDEX_NAME_SEPARATORS            (unsigned char *)", "


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Terms sort structure */
struct termsSort {
    unsigned char       *pucTerm;                   /* Term */
    unsigned int        uiTermCountInText;          /* Number of occurrences of this term in the text */
    unsigned int        uiTermCount;                /* Number of occurrences of this term */
    unsigned int        uiDocumentCount;            /* Number of documents in which this term occurs */
    float               fWeight;                    /* Term weight */
};    


/* Terms index list structure */
struct termsIndexList {
    struct srchIndex    *psiSrchIndex;              /* Index */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);


static int iSrchTermsExtractTerms (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath, 
        unsigned char *pucIndexNames, unsigned char *pucStopListName, unsigned char *pucStopListFilePath, 
        unsigned char *pucLanguageCode, boolean bLanguageFirstTerm, unsigned char *pucTextFilePath, 
        unsigned char *pucTermsFilePath, unsigned int uiMinTermCount, unsigned int uiMaxTermCount, 
        float fTermPercentage, float fDocumentCoverage, float fFrequentTermCoverage, 
        unsigned int uiMaxTokenizationErrors);

static int iSrchTermsAddStopTermsToTrie (void *pvLngStemmer, void *pvUtlStopTermsTrie,
        wchar_t **ppwcStopListTermList, unsigned int uiStopListTermListLength);

static int iSrchTermsExtractTermsFromText (struct termsIndexList *ptilTermsIndexList, unsigned int uiTermsIndexListLength, 
        void *pvLngTokenizer, void *pvLngStemmer, void *pvUtlStopTermsTrie, unsigned char *pucText, unsigned char *pucLanguageCode,
        FILE *pfOutputFile, unsigned int uiDocumentCount, unsigned int uiMinTermCount, unsigned int uiMaxTermCount, 
        float fTermPercentage, float fDocumentCoverage, float fFrequentTermCoverage, unsigned int uiMaxTokenizationErrors);

static int iSrchTermsParseTextToTrie (struct srchIndex *psiSrchIndex, void *pvLngTokenizer, void *pvUtlStopTermsTrie, 
        unsigned char *pucText, unsigned char *pucLanguageCode, unsigned int uiMaxTokenizationErrors, 
        void **ppvUtlTermsTrie, unsigned int *puiTotalTermCount, unsigned int *puiUniqueTermCount);

static int iSrchTermsAddTermToTrie (struct srchIndex *psiSrchIndex, void *pvUtlStopTermsTrie, void *pvUtlTermsTrie, 
        wchar_t *pwcTerm, unsigned int *puiTotalTermCount, unsigned int *puiUniqueTermCount);

static int iSrchTermsCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);

static int iSrchTermsListTerms (struct termsSort *ptsTermsSort, unsigned int uiTermsSortLength, FILE *pfOutputFile, 
        unsigned int uiDocumentCount, unsigned int uiMinTermCount, unsigned int uiMaxTermCount, 
        float fTermPercentage, float fDocumentCoverage, unsigned int *puiUsedTermCount);

static int iSrchTermsCompareByWeightDesc (struct termsSort *ptsTermsSort1, struct termsSort *ptsTermsSort2);


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
    unsigned char   pucTermsFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int    uiMinTermCount = SRCH_TERMS_TERM_COUNT_MIN_DEFAULT;
    unsigned int    uiMaxTermCount = SRCH_TERMS_TERM_COUNT_MAX_DEFAULT;
    float           fTermPercentage = SRCH_TERMS_TERM_PERCENTAGE_DEFAULT;
    float           fDocumentCoverage = SRCH_TERMS_DOCUMENT_COVERAGE_DEFAULT;
    float           fFrequentTermCoverage = SRCH_TERMS_FREQUENT_TERM_COVERAGE_DEFAULT;
    unsigned int    uiMaxTokenizationErrors = SRCH_TERMS_ERRORS_MAX_DEFAULT;

    unsigned char   *pucLocaleName = SRCH_TERMS_LOCALE_NAME_DEFAULT;

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
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucConfigurationDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true index directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that index directory path exists */
            if ( bUtlFilePathExists(pucConfigurationDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', does not exist", pucConfigurationDirectoryPath);
            }

            /* Check that the index directory path is a directory */
            if ( bUtlFileIsDirectory(pucConfigurationDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', is not a directory", pucConfigurationDirectoryPath);
            }

            /* Check that the index directory path can be accessed */
            if ( (bUtlFilePathRead(pucConfigurationDirectoryPath) == false) || (bUtlFilePathExec(pucConfigurationDirectoryPath) == false) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', cannot be accessed", pucConfigurationDirectoryPath);
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

        /* Check for text file */
        else if ( s_strncmp("--text=", pucNextArgument, s_strlen("--text=")) == 0 ) {

            /* Get the text file */
            pucNextArgument += s_strlen("--text=");

            /* Get the true text file path */
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

        /* Check for terms file */
        else if ( s_strncmp("--terms=", pucNextArgument, s_strlen("--terms=")) == 0 ) {

            /* Get the terms file */
            pucNextArgument += s_strlen("--terms=");

            /* Get the true terms file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucTermsFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true terms file path: '%s', utl error: %d", pucNextArgument, iError);
            }
            
            /* Check that terms file path can be written to if it exists */
            if ( bUtlFilePathExists(pucTermsFilePath) == true ) {
                if ( bUtlFilePathWrite(pucTermsFilePath) == false ) {
                    vVersion();
                    iUtlLogPanic(UTL_LOG_CONTEXT, "The terms file path: '%s', cannot be accessed", pucTermsFilePath);
                }
            }
        }

        /* Check for minimum terms */
        else if ( s_strncmp("--minimum-terms=", pucNextArgument, s_strlen("--minimum-terms=")) == 0 ) {

            /* Get the minimum phrases */
            pucNextArgument += s_strlen("--minimum-terms=");

            /* Check the minimum terms */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid minimum number of terms: '%s'", pucNextArgument);
            }

            /* Set the minimum terms */
            uiMinTermCount = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for maximum terms */
        else if ( s_strncmp("--maximum-terms=", pucNextArgument, s_strlen("--maximum-terms=")) == 0 ) {

            /* Get the maximum phrases */
            pucNextArgument += s_strlen("--maximum-terms=");

            /* Check the maximum terms */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid maximum number of terms: '%s'", pucNextArgument);
            }

            /* Set the maximum terms */
            uiMaxTermCount = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for term percentage */
        else if ( s_strncmp("--term-percentage=", pucNextArgument, s_strlen("--term-percentage=")) == 0 ) {

            /* Get the term percentage */
            pucNextArgument += s_strlen("--term-percentage=");

            /* Set the term percentage */
            fTermPercentage = s_strtof(pucNextArgument, NULL);
            
            /* Check the term percentage */
            if ( (fTermPercentage <= 0) || (fTermPercentage > 100) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid term percentage: '%s'", pucNextArgument);
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


    /* Check the min term count */
    if ( uiMinTermCount <= 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The minimum number of terms cannot be zero or less, minimum number of terms specified: %u", uiMinTermCount);
    }

    /* Check the max term count - 0 mean no limit */
    if ( uiMaxTermCount < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The maximum number of terms cannot be less than 0, maximum number of terms specified: %u", uiMaxTermCount);
    }

    /* Check that the max term count is more than the min term count */
    if ( (uiMaxTermCount > 0) && (uiMinTermCount > uiMaxTermCount) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The maximum number of terms cannot be less than the minimum number of terms, minimum number of terms specified: %u, maximum number of terms specified: %u", 
                uiMinTermCount, uiMaxTermCount);
    }

    /* Check for term percentage */
    if ( (fTermPercentage <= 0) || (fTermPercentage > 100) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The term percentage cannot be zero or less or more than 100, value specified: %9.4", fTermPercentage);
    }

    /* Check for document coverage */
    if ( (fDocumentCoverage <= 0) || (fDocumentCoverage > 100) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The document coverage cannot be zero or less or more than 100, value specified: %9.4", fDocumentCoverage);
    }

    /* Check for frequent term coverage */
    if ( fFrequentTermCoverage < 0 ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The frequent term coverage cannot less than 0, value specified: %9.4", fFrequentTermCoverage);
    }



    /* Extract the terms */
    iError = iSrchTermsExtractTerms(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexNames, pucStopListName, pucStopListFilePath, 
            pucLanguageCode, bLanguageFirstTerm, pucTextFilePath, pucTermsFilePath, uiMinTermCount, uiMaxTermCount, fTermPercentage, fDocumentCoverage, 
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
    fprintf(stderr, "MPS Terms, %s\n", UTL_VERSION_COPYRIGHT_STRING);


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
    printf("  --index=name[%sname[%s...]] \n", SRCH_TERMS_INDEX_NAME_SEPARATORS, SRCH_TERMS_INDEX_NAME_SEPARATORS);
    printf("                  Index names, '%s' delimited, required. \n", SRCH_TERMS_INDEX_NAME_SEPARATORS);
    printf("  --stoplist=name \n");
    printf("                  Stop list to use, defaults to index stop list , stop lists available: '%s', \n", LNG_STOP_LIST_NONE_NAME);
    printf("                  '%s', '%s'.\n", LNG_STOP_LIST_GOOGLE_NAME, LNG_STOP_LIST_GOOGLE_MODIFIED_NAME);
    printf("  --stopfile=name \n");
    printf("                  Stop list term file, overriding the internal stop list, one term per line. \n");
    printf("  --language=name \n");
    printf("                  Language code, defaults to index language. \n");
    printf("  --language-first-term \n");
    printf("                  First term in the text file will be used as the language. \n");
    printf("  --text=name     Text file name, defaults to 'stdin', one line per text file. \n");
    printf("  --terms=name    Terms file name, defaults to 'stdout', one line per term file. \n");
    printf("  --minimum-terms=# \n");
    printf("                  Minimum number of terms, defaults to: %d. \n", SRCH_TERMS_TERM_COUNT_MIN_DEFAULT);
    printf("  --maximum-terms=# \n");
    printf("                  Maximum number of terms, defaults to: %d, 0 means no limit. \n", SRCH_TERMS_TERM_COUNT_MAX_DEFAULT);
    printf("  --term-percentage=# \n");
    printf("                  Maximum number of terms as a percentage of the original text, \n");
    printf("                  defaults to: %3.1f%%, range: 1-100. \n", SRCH_TERMS_TERM_PERCENTAGE_DEFAULT);
    printf("  --document-coverage=# \n");
    printf("                  Document coverage threshold, defaults to: %3.1f%%, range: 1-100. \n", SRCH_TERMS_DOCUMENT_COVERAGE_DEFAULT);
    printf("  --frequent-term-coverage=# \n");
    printf("                  Frequent term coverage threshold, defaults to: %3.1f%%, \n", SRCH_TERMS_FREQUENT_TERM_COVERAGE_DEFAULT);
    printf("                  range: 0-.., 0 = means not applied. \n");
    printf("  --maximum-tokenization-errors=# \n");
    printf("                  Maximum number of tokenization errors per text file, defaults to: %d%%. \n", SRCH_TERMS_ERRORS_MAX_DEFAULT);
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", SRCH_TERMS_LOCALE_NAME_DEFAULT);
    printf("                  supported locales, locale chosen must support utf-8. \n");
    printf("\n");

    printf(" Logging parameters: \n");
    printf("  --log=name      Log output file name, defaults to 'stderr', console options: '%s', '%s'. \n", UTL_LOG_FILE_STDOUT, UTL_LOG_FILE_STDERR);
    printf("  --level=#       Log level, defaults to info, %d = debug, %d = info, %d = warn, %d = error, %d = fatal. \n",
            UTL_LOG_LEVEL_DEBUG, UTL_LOG_LEVEL_INFO, UTL_LOG_LEVEL_WARN, UTL_LOG_LEVEL_ERROR, UTL_LOG_LEVEL_FATAL);

    printf(" Help & version: \n");
    printf("  -?, --help, --usage \n");
    printf("                  Prints the usage and exits. \n");
    printf("  --version       Prints the version and exits. \n");
    printf("\n");


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermsExtractTerms()

    Purpose:    This function extracts the terms from the text.

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexNames                   index names
                pucStopListName                 stop list name (optional)
                pucStopListFilePath             stop list file path (optional)
                pucLanguageCode                 language code (optional)
                bLanguageFirstTerm              language first term flag
                pucTextFilePath                 text file path (optional)
                pucTermsFilePath                terms file path (optional)
                uiMinTermCount                  minimum term count
                uiMaxTermCount                  maximum term count
                fTermPercentage                 term percentage
                fDocumentCoverage               document coverage
                fFrequentTermCoverage           frequent term coverage
                uiMaxTokenizationErrors         maximum tokenization errors

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchTermsExtractTerms
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexNames,
    unsigned char *pucStopListName,
    unsigned char *pucStopListFilePath,
    unsigned char *pucLanguageCode,
    boolean bLanguageFirstTerm,
    unsigned char *pucTextFilePath,
    unsigned char *pucTermsFilePath,
    unsigned int uiMinTermCount,
    unsigned int uiMaxTermCount,
    float fTermPercentage,
    float fDocumentCoverage,
    float fFrequentTermCoverage,
    unsigned int uiMaxTokenizationErrors
)
{


    int                     iError = SRCH_NoError;
    unsigned char           *pucIndexNamesCopy = NULL;
    unsigned char           *pucIndexNamePtr = NULL;
    unsigned char           *pucIndexNameStrtokPtr = NULL;
    struct termsIndexList   *ptilTermsIndexList = NULL;
    struct termsIndexList   *ptdlTermsIndexListPtr = NULL;
    unsigned int            uiTermsIndexListLength = 0;
    unsigned int            uiDocumentCount = 0;
    void                    *pvLngTokenizer = NULL;
    void                    *pvLngStemmer = NULL;
    void                    *pvLngStopList = NULL;
    void                    *pvUtlStopTermsTrie = NULL;
    FILE                    *pfTextFile = NULL;
    FILE                    *pfOutputFile = NULL;

    unsigned char           *pucText = NULL;
    unsigned char           *pucTextPtr = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexNames) == false);
    ASSERT((bUtlStringsIsStringNULL(pucStopListName) == false) || (bUtlStringsIsStringNULL(pucStopListName) == true));
    ASSERT((bUtlStringsIsStringNULL(pucStopListFilePath) == false) || (bUtlStringsIsStringNULL(pucStopListFilePath) == true));
    ASSERT(!((bUtlStringsIsStringNULL(pucLanguageCode) == false) && (bLanguageFirstTerm == true)));
    ASSERT((bUtlStringsIsStringNULL(pucTextFilePath) == false) || (bUtlStringsIsStringNULL(pucTextFilePath) == true));
    ASSERT((bUtlStringsIsStringNULL(pucTermsFilePath) == false) || (bUtlStringsIsStringNULL(pucTermsFilePath) == true));
    ASSERT(uiMinTermCount > 0);
    ASSERT(uiMaxTermCount >= 0);
    ASSERT(!((uiMaxTermCount > 0) && (uiMinTermCount > uiMaxTermCount)));
    ASSERT((fTermPercentage > 0) && (fTermPercentage <= 100));
    ASSERT((fDocumentCoverage > 0) && (fDocumentCoverage <= 100));
    ASSERT(fFrequentTermCoverage >= 0);
    ASSERT(uiMaxTokenizationErrors >= 0);


    /* Check for terms file path */
    if ( bUtlStringsIsStringNULL(pucTermsFilePath) == false ) {

        /* Open the terms file */
        if ( (pfOutputFile = s_fopen(pucTermsFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the term file: '%s'.", pucTermsFilePath);
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
        }
    }
    else {
        pfOutputFile = stdout;
    }

    
    /* Duplicate the index names, we do this because s_strtok_r() destroys the buffer */
    if ( (pucIndexNamesCopy = (unsigned char *)s_strdup(pucIndexNames)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchTermsExtractTerms;
    }

    /* Loop parsing the index names */
    for ( pucIndexNamePtr = s_strtok_r(pucIndexNamesCopy, SRCH_TERMS_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr), uiTermsIndexListLength = 0; 
            pucIndexNamePtr != NULL; 
            pucIndexNamePtr = s_strtok_r(NULL, SRCH_TERMS_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr), uiTermsIndexListLength++ ) {

        /* Allocate space in the terms index list for this index */
        if ( (ptdlTermsIndexListPtr = (struct termsIndexList *)s_realloc(ptilTermsIndexList, (size_t)(sizeof(struct termsIndexList) * (uiTermsIndexListLength + 1)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchTermsExtractTerms;
        }

        /* Hand over the pointer */
        ptilTermsIndexList = ptdlTermsIndexListPtr;

        /* Dereference the terms index list pointer */
        ptdlTermsIndexListPtr = ptilTermsIndexList + uiTermsIndexListLength;

        /* Open the index */
        if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexNamePtr, SRCH_INDEX_INTENT_SEARCH, &ptdlTermsIndexListPtr->psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexNamePtr, iError);
            goto bailFromiSrchTermsExtractTerms;
        }
        
        /* Incremement the document count */
        uiDocumentCount += ptdlTermsIndexListPtr->psiSrchIndex->uiDocumentCount;
    }


    /* Create the tokenizer */
    if ( (iError = iLngTokenizerCreateByID(pucConfigurationDirectoryPath, ptilTermsIndexList->psiSrchIndex->uiTokenizerID, ptilTermsIndexList->psiSrchIndex->uiLanguageID, &pvLngTokenizer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a tokenizer, lng error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchTermsExtractTerms;
    }


    /* Create the stemmer */
    if ( (iError = iLngStemmerCreateByID(ptilTermsIndexList->psiSrchIndex->uiStemmerID, ptilTermsIndexList->psiSrchIndex->uiLanguageID, &pvLngStemmer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a stemmer, lng error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchTermsExtractTerms;
    }


    /* We are using an internal stop list if the stop list name is set */
    if ( pucStopListName != NULL ) {
    
        unsigned int    uiStopListID = LNG_STOP_LIST_ANY_ID;

        /* Get the stop list ID */
        if ( (iError = iLngGetStopListIDFromName(pucStopListName, &uiStopListID)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list ID from the stop list name: '%s', lng error: %d.", pucStopListName, iError); 
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
        }
    
        /* Get the stop list handle */
        if ( (iError = iLngStopListCreateByID(uiStopListID, ptilTermsIndexList->psiSrchIndex->uiLanguageID, &pvLngStopList)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the stop list, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
        }
    }
    /* We are using a stop list file if the stop list file path is set */
    else if ( pucStopListFilePath != NULL ) {

        /* Get the stop list handle */
        if ( (iError = iLngStopListCreateByIDFromFile(pucStopListFilePath, ptilTermsIndexList->psiSrchIndex->uiLanguageID, &pvLngStopList)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the stop list from the stop list file: '%s', lng error: %d.", pucStopListFilePath, iError); 
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
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
            goto bailFromiSrchTermsExtractTerms;
        }
    
        /* Create a trie for the stop terms */
        if ( (iError = iUtlTrieCreate(&pvUtlStopTermsTrie)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the stop terms, utl error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
        }

        /* Add the stop terms to the trie */
        if ( (iError = iSrchTermsAddStopTermsToTrie(pvLngStemmer, pvUtlStopTermsTrie, ppwcStopListTermList, uiStopListTermListLength)) != SRCH_NoError ) {
            goto bailFromiSrchTermsExtractTerms;
        }
    }



    /* Text file path */
    if ( bUtlStringsIsStringNULL(pucTextFilePath) == false ) {

        off_t   zTextFileLength = 0;

        /* Open the text file */
        if ( (pfTextFile = s_fopen(pucTextFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the text file: '%s'.", pucTextFilePath);
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
        }

        /* Get the text file length */
        if ( (iError = iUtlFileGetFileLength(pfTextFile, &zTextFileLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get length of the text file: '%s', utl error: %d.", pucTextFilePath, iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
        }
            
        /* Allocate space for the text */
        if ( (pucText = s_malloc(zTextFileLength + 1)) == NULL ) {
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
        }
            
        /* Read the text from the file */
        if ( s_fread(pucText, 1, zTextFileLength, pfTextFile) != zTextFileLength ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the text file.");
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTerms;
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


        /* Extract the terms */
        iError = iSrchTermsExtractTermsFromText(ptilTermsIndexList, uiTermsIndexListLength, pvLngTokenizer, pvLngStemmer, pvUtlStopTermsTrie, pucTextPtr, pucLanguageCode, 
                pfOutputFile, uiDocumentCount, uiMinTermCount, uiMaxTermCount, fTermPercentage, fDocumentCoverage, fFrequentTermCoverage, uiMaxTokenizationErrors);

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
                        goto bailFromiSrchTermsExtractTerms;
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
                        goto bailFromiSrchTermsExtractTerms;
                    }
                    
                    /* Check if we have hit the end of the file */
                    else if ( s_feof(stdin) != 0 ) {
                        
                        /* No text so we bail */
                        if ( s_strlen(pucText) == 0 ) {
                            iError = SRCH_NoError;
                            goto bailFromiSrchTermsExtractTerms;
                        }
                        /* Otherwise we break out of the loop and extract terms from the text */
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


            /* Extract the terms */
            iError = iSrchTermsExtractTermsFromText(ptilTermsIndexList, uiTermsIndexListLength, pvLngTokenizer, pvLngStemmer, pvUtlStopTermsTrie, pucTextPtr, pucLanguageCode, 
                    pfOutputFile, uiDocumentCount, uiMinTermCount, uiMaxTermCount, fTermPercentage, fDocumentCoverage, fFrequentTermCoverage, uiMaxTokenizationErrors);

        }    /* while ( true ) */
    }



    /* Bail label */
    bailFromiSrchTermsExtractTerms:


    /* Free the text */
    s_free(pucText);

    /* Close the text file */
    if ( bUtlStringsIsStringNULL(pucTextFilePath) == false ) {
        s_fclose(pfTextFile);
    }
    
    /* Close the terms file */
    if ( bUtlStringsIsStringNULL(pucTermsFilePath) == false ) {
        s_fclose(pfOutputFile);
    }
    
    /* Free the duplicate index names */
    s_free(pucIndexNamesCopy);

    /* Free the terms index list */
    if ( ptilTermsIndexList != NULL ) {
        
        unsigned int    uiI = 0;
        
        /* Loop over each index closing it */
        for ( uiI = 0, ptdlTermsIndexListPtr = ptilTermsIndexList; uiI < uiTermsIndexListLength; uiI++, ptdlTermsIndexListPtr++ ) {
            iSrchIndexClose(ptdlTermsIndexListPtr->psiSrchIndex);
        }
        
        /* Finally we free the terms index list */
        s_free(ptilTermsIndexList);
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

    Function:   iSrchTermsAddStopTermsToTrie()

    Purpose:    Add the stop terms to the trie.

    Parameters: pvLngStemmer                stemmer
                pvUtlStopTermsTrie          stop terms trie
                ppwcStopListTermList        stop term list
                uiStopListTermListLength    stop term list length

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchTermsAddStopTermsToTrie
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
        return (SRCH_NoError);
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
    if ( (iError = iSrchTermsAddStopTermsToTrie(pvLngStemmer, pvUtlStopTermsTrie, ppwcStopListTermList, uiMiddle)) != SRCH_NoError ) {
        return (iError);
    }


    /* Do the high end of the list */
    if ( (iError = iSrchTermsAddStopTermsToTrie(pvLngStemmer, pvUtlStopTermsTrie, ppwcStopListTermList + uiMiddle + 1, uiStopListTermListLength - uiMiddle - 1)) != SRCH_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermsExtractTermsFromText()

    Purpose:    This function extracts the terms from the text.

    Parameters: ptilTermsIndexList          terms index list structure
                uiTermsIndexListLength      terms index list length
                pvLngTokenizer              tokenizer
                pvLngStemmer                stemmer
                pvUtlStopTermsTrie          stop terms trie (optional)
                pucText                     text
                pucLanguageCode             language code
                pfOutputFile                output file
                uiDocumentCount             document count
                uiMinTermCount              minimum term count
                uiMaxTermCount              maximum term count
                fTermPercentage             term percentage
                fDocumentCoverage           document coverage
                fFrequentTermCoverage       frequent term coverage
                uiMaxTokenizationErrors     maximum tokenization errors

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchTermsExtractTermsFromText
(
    struct termsIndexList *ptilTermsIndexList, 
    unsigned int uiTermsIndexListLength,
    void *pvLngTokenizer, 
    void *pvLngStemmer,
    void *pvUtlStopTermsTrie,
    unsigned char *pucText,
    unsigned char *pucLanguageCode,
    FILE *pfOutputFile,
    unsigned int uiDocumentCount,
    unsigned int uiMinTermCount,
    unsigned int uiMaxTermCount,
    float fTermPercentage,
    float fDocumentCoverage,
    float fFrequentTermCoverage,
    unsigned int uiMaxTokenizationErrors
)
{
    int                     iError = SRCH_NoError;
    unsigned int            uiI = 0;
    void                    *pvUtlTermsTrie = NULL;
    struct termsSort        *ptsTermsSort = NULL;
    struct termsSort        *ptsTermsSortPtr = NULL;
    struct termsIndexList   *ptdlTermsIndexListPtr = NULL;
    unsigned int            uiTotalTermCount = 0;
    unsigned int            uiUniqueTermCount = 0;
    unsigned int            uiUsedTermCount = 0;
    unsigned int            uiTermsSortLength = 0;
    unsigned int            uiTermsSortIndex = 0;


    ASSERT(ptilTermsIndexList != NULL);
    ASSERT(uiTermsIndexListLength > 0);
    ASSERT(pvLngTokenizer != NULL);
    ASSERT(pvLngStemmer != NULL);
    ASSERT((pvUtlStopTermsTrie != NULL) || (pvUtlStopTermsTrie == NULL));
    ASSERT(bUtlStringsIsStringNULL(pucText) == false);
    ASSERT(bUtlStringsIsStringNULL(pucLanguageCode) == false);
    ASSERT(pfOutputFile != NULL);
    ASSERT(uiDocumentCount > 0);
    ASSERT(uiMinTermCount > 0);
    ASSERT(uiMaxTermCount >= 0);
    ASSERT(!((uiMaxTermCount > 0) && (uiMinTermCount > uiMaxTermCount)));
    ASSERT((fTermPercentage > 0) && (fTermPercentage <= 100));
    ASSERT((fDocumentCoverage > 0) && (fDocumentCoverage <= 100));
    ASSERT(fFrequentTermCoverage >= 0);
    ASSERT(uiMaxTokenizationErrors >= 0);



    /* Parse the text into the trie */
    if ( (iError = iSrchTermsParseTextToTrie(ptilTermsIndexList->psiSrchIndex, pvLngTokenizer, pvUtlStopTermsTrie, pucText, pucLanguageCode,
            uiMaxTokenizationErrors, &pvUtlTermsTrie, &uiTotalTermCount, &uiUniqueTermCount)) != SRCH_NoError ) {
        goto bailFromiSrchTermsExtractTermsFromText;
    }


    /* Extract the terms if there were any */
    if ( uiUniqueTermCount > 0 ) {

        /* Set the size of the terms sort array from the unique term count */
        uiTermsSortLength = uiUniqueTermCount;

        /* Allocate an terms sort structure array */
        if ( (ptsTermsSort = (struct termsSort *)s_malloc((size_t)(uiTermsSortLength * sizeof(struct termsSort)))) == NULL ) {
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsExtractTermsFromText;
        }
        
        /* Loop over each index in the terms index list */
        for ( uiI = 0, ptdlTermsIndexListPtr = ptilTermsIndexList; uiI < uiTermsIndexListLength; uiI++, ptdlTermsIndexListPtr++ ) {
    
            /* Dereference the index for convenience */
            struct srchIndex    *psiSrchIndex = ptdlTermsIndexListPtr->psiSrchIndex;

            /* Reset the terms sort index */
            uiTermsSortIndex = 0;

            /* Loop through each entry in the trie calling the call back function for each entry */
            if ( (iError = iUtlTrieLoop(pvUtlTermsTrie, NULL, (int (*)())iSrchTermsCallBackFunction, psiSrchIndex, pvLngStemmer, 
                    (double)fFrequentTermCoverage, ptsTermsSort, uiTermsSortLength, &uiTermsSortIndex)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the entries in the term trie, utl error: %d.", iError);
                iError = SRCH_MiscError;
                goto bailFromiSrchTermsExtractTermsFromText;
            }
        }


        /* List the terms from the terms sort structure array */
        if ( (iError = iSrchTermsListTerms(ptsTermsSort, uiTermsSortLength, pfOutputFile, uiDocumentCount, uiMinTermCount, uiMaxTermCount, 
                fTermPercentage, fDocumentCoverage, &uiUsedTermCount)) != SRCH_NoError ) {
            goto bailFromiSrchTermsExtractTermsFromText;
        }

        /* Log */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Terms, total: %u, unique: %u, used: %u.", uiTotalTermCount, uiUniqueTermCount, uiUsedTermCount); */
    }



    /* Bail label */
    bailFromiSrchTermsExtractTermsFromText:


    /* Write the new line and flush the output file handle */
    fprintf(pfOutputFile, "\n");
    s_fflush(pfOutputFile);


    /* Free the terms sort structure array */
    if ( ptsTermsSort != NULL ) {
    
        for ( uiI = 0, ptsTermsSortPtr = ptsTermsSort; uiI < uiTermsSortLength; uiI++, ptsTermsSortPtr++ ) {
            s_free(ptsTermsSortPtr->pucTerm);
        }

        s_free(ptsTermsSort);
    }

    /* Free the trie */
    iUtlTrieFree(pvUtlTermsTrie, false);
    pvUtlTermsTrie = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermsParseTextToTrie()

    Purpose:    This function prepares a trie from a terms string.

    Parameters: psiSrchIndex                index structure
                pvLngTokenizer              tokenizer
                pvUtlStopTermsTrie          stop terms trie (optional)
                pucText                     text
                pucLanguageCode             language code
                uiMaxTokenizationErrors     maximum tokenization errors
                ppvUtlTermsTrie             return pointer for the trie
                puiTotalTermCount           return pointer for the number of terms found    
                puiUniqueTermCount          return pointer for the number of unique terms found    

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchTermsParseTextToTrie
(
    struct srchIndex *psiSrchIndex,
    void *pvLngTokenizer,
    void *pvUtlStopTermsTrie,
    unsigned char *pucText,
    unsigned char *pucLanguageCode,
    unsigned int uiMaxTokenizationErrors,
    void **ppvUtlTermsTrie,
    unsigned int *puiTotalTermCount,
    unsigned int *puiUniqueTermCount
)
{

    int             iError = UTL_NoError;
    int             iTokenizationErrorCount = 0;
    void            *pvUtlTermsTrie = NULL;
    wchar_t         *pwcText = NULL;
    unsigned int    uiLanguageID = LNG_LANGUAGE_ANY_ID;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermsParseTextToTrie - [%s]", pucText); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(pvLngTokenizer != NULL);
    ASSERT((pvUtlStopTermsTrie != NULL) || (pvUtlStopTermsTrie == NULL));
    ASSERT(bUtlStringsIsStringNULL(pucText) == false);
    ASSERT(bUtlStringsIsStringNULL(pucLanguageCode) == false);
    ASSERT(uiMaxTokenizationErrors >= 0);
    ASSERT(ppvUtlTermsTrie != NULL);
    ASSERT(puiTotalTermCount != NULL);
    ASSERT(puiUniqueTermCount != NULL);


    /* Create a trie for the terms */
    if ( (iError = iUtlTrieCreate(&pvUtlTermsTrie)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the terms, utl error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchTermsParseTextToTermTrie;
    }


    /* Convert the text from utf-8 to wide characters, pwcText is allocated */
    if ( (iError = iLngConvertUtf8ToWideString_d(pucText, 0, &pwcText)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the text from utf-8 to wide characters, lng error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchTermsParseTextToTermTrie;
    }


    /* Initialize the return pointers */
    *puiTotalTermCount = 0;
    *puiUniqueTermCount = 0;



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
    if ( (iError = iLngTokenizerParseString(pvLngTokenizer, uiLanguageID, pwcText, s_wcslen(pwcText))) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse the text, lng error: %d.", iError);
        iError = SRCH_MiscError;
        goto bailFromiSrchTermsParseTextToTermTrie;
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
                iError = SRCH_MiscError;
                goto bailFromiSrchTermsParseTextToTermTrie;
            }

            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to get a token from the text, lng error: %d.", iError);

            iError = SRCH_NoError;
        }

        /* Break if this was the last token */
        if ( pwcTermStartPtr == NULL ) {
            break;
        }

/* Skip non terms */
/* if ( pwcTermStartPtr == pwcTermEndPtr ) { */
/*     printf("[%ls]\n", pwcText); */
/*     continue; */
/* } */

        /* Skip numbers */
        if ( iswdigit(*pwcTermStartPtr) != 0 ) {
            continue;
        }


        /* Save the term end character and NULL terminate the term */ 
        wcTermEnd = *pwcTermEndPtr;
        *pwcTermEndPtr = L'\0';


        /* Add the term to the trie */
        if ( (iError = iSrchTermsAddTermToTrie(psiSrchIndex, pvUtlStopTermsTrie, pvUtlTermsTrie, pwcTermStartPtr, puiTotalTermCount, puiUniqueTermCount)) != SRCH_NoError ) {
/*             goto bailFromiSrchTermsParseTextToTermTrie; */
        }
        iError = SRCH_NoError;

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
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the search text, lng error: %d.", iError);
                    iError = SRCH_MiscError;
                    goto bailFromiSrchTermsParseTextToTermTrie;
                }
    
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to get a component from the search text, lng error: %d.", iError);
                
                iError = SRCH_NoError;
            }
            
            /* Break if this was the last component */
            if ( pwcComponentStartPtr == NULL ) {
                break;
            }
            
/* Skip non components */
/* if ( pwcComponentStartPtr == pwcComponentEndPtr ) { */
/*     printf("[%ls]\n", pwcText); */
/*     continue; */
/* } */

            /* Save the component end character and NULL terminate the component */ 
            wcComponentEnd = *pwcComponentEndPtr;
            *pwcComponentEndPtr = L'\0';

            /* Add the component to the trie */
            if ( (iError = iSrchTermsAddTermToTrie(psiSrchIndex, pvUtlStopTermsTrie, pvUtlTermsTrie, pwcComponentStartPtr, puiTotalTermCount, puiUniqueTermCount)) != SRCH_NoError ) {
/*                 goto bailFromiSrchTermsParseTextToTermTrie; */
            }
            iError = SRCH_NoError;
        
            /* Restore the component end character */
            *pwcComponentEndPtr = wcComponentEnd;
        }
    }



    /* Bail label */
    bailFromiSrchTermsParseTextToTermTrie:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
    
        /* Set the return pointer */
        *ppvUtlTermsTrie = pvUtlTermsTrie;
    }
    else {
        /* Free the trie */
        iUtlTrieFree(pvUtlTermsTrie, false);
        pvUtlTermsTrie = NULL;
    }

    /* Free the text */
    s_free(pwcText);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermsAddTermToTrie()

    Purpose:    This function adds the term to the trie.

    Parameters: psiSrchIndex            index structure
                pvUtlStopTermsTrie      stop terms trie
                pvUtlTermsTrie          term trie
                pwcTerm                 term
                puiTotalTermCount       return pointer for the total term count    
                puiUniqueTermCount      return pointer for the unique term count

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchTermsAddTermToTrie
(
    struct srchIndex *psiSrchIndex,
    void *pvUtlStopTermsTrie,
    void *pvUtlTermsTrie,
    wchar_t *pwcTerm,
    unsigned int *puiTotalTermCount,
    unsigned int *puiUniqueTermCount
)
{

    int             iError = UTL_NoError;
    unsigned int    *puiData = NULL;

    unsigned char   pucTerm[SRCH_TERMS_MAX_TERM_LENGTH + 1] = {'\0'};
    unsigned int    ulTermLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermsAddTermToTrie - [%ls]", pwcTerm); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT((pvUtlStopTermsTrie != NULL) || (pvUtlStopTermsTrie == NULL));
    ASSERT(pvUtlTermsTrie != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(puiTotalTermCount != NULL);
    ASSERT(puiUniqueTermCount != NULL);


    /* Add the term in its original case if it is all upper case */
    if ( bLngCaseIsWideStringAllUpperCase(pwcTerm) == true ) {

        /* Convert the term from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(pwcTerm, 0, pucTerm, SRCH_TERMS_MAX_TERM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsAddTermToTrie;
        }

        /* Get the term length */
        ulTermLength = s_strlen(pucTerm);

        /* Is the term long enough to add, note the restriction of 1 */
        if ( (ulTermLength < psiSrchIndex->uiTermLengthMinimum) || (ulTermLength == 1) ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermsAddTermToTrie;
        }

        /* Truncate the term if it is too long */
        if ( ulTermLength > psiSrchIndex->uiTermLengthMaximum ) {
            pucTerm[psiSrchIndex->uiTermLengthMaximum] = '\0';
        }

        /* Reset the data pointer */
        puiData = NULL;
        
        /* Check if this term is in the stop list trie, this will set the data pointer if the term is there */
        if ( pvUtlStopTermsTrie != NULL ) {
            iUtlTrieLookup(pvUtlStopTermsTrie, pucTerm, (void ***)&puiData);
        }

        /* Add the term if it is not on the stop list */
        if ( (puiData == NULL) || ((*puiData) == 0) ) {

            /* Look up/store the term - skip it if it cant be looked-up/stored */
            if ( (iError = iUtlTrieAdd(pvUtlTermsTrie, pucTerm, (void ***)&puiData)) != UTL_NoError ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add an entry to the terms trie, term: '%s', utl error: %d.", pucTerm, iError);
                iError = SRCH_NoError;
                goto bailFromiSrchTermsAddTermToTrie;
            }
            else {
                /* Increment our counts */
                (*puiTotalTermCount)++;
                (*puiData) += 1;
                (*puiUniqueTermCount) += (*puiData == 1) ? 1 : 0;
            }
        }
    }

    /* Convert the term to lower case and add it */
    else {

        wchar_t     pwcTermCopy[SRCH_TERMS_MAX_TERM_LENGTH + 1] = {L'\0'};


        /* Copy the term, this is because we side-effect */
        s_wcsncpy(pwcTermCopy, pwcTerm, SRCH_TERMS_MAX_TERM_LENGTH + 1);

        /* Convert the term to lower case */
        pwcLngCaseConvertWideStringToLowerCase(pwcTermCopy);

        /* Convert the term from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(pwcTermCopy, 0, pucTerm, SRCH_TERMS_MAX_TERM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_MiscError;
            goto bailFromiSrchTermsAddTermToTrie;
        }

        /* Get the term length */
        ulTermLength = s_strlen(pucTerm);

        /* Is the term long enough to add, note the restriction of 1 */
        if ( (ulTermLength < psiSrchIndex->uiTermLengthMinimum) || (ulTermLength == 1) ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermsAddTermToTrie;
        }

        /* Truncate the term if it is too long */
        if ( ulTermLength > psiSrchIndex->uiTermLengthMaximum ) {
            pucTerm[psiSrchIndex->uiTermLengthMaximum] = '\0';
        }

        /* Reset the data pointer */
        puiData = NULL;
        
        /* Check if this term is in the stop list trie, this will set the data pointer if the term is there */
        if ( pvUtlStopTermsTrie != NULL ) {
            iUtlTrieLookup(pvUtlStopTermsTrie, pucTerm, (void ***)&puiData);
        }

        /* Add the term if it is not on the stop list */
        if ( (puiData == NULL) || ((*puiData) == 0) ) {

            /* Look up/store the term - skip it if it cant be looked-up/stored */
            if ( (iError = iUtlTrieAdd(pvUtlTermsTrie, pucTerm, (void ***)&puiData)) != UTL_NoError ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add an entry to the terms trie, term: '%s', utl error: %d.", pucTerm, iError);
                iError = SRCH_NoError;
                goto bailFromiSrchTermsAddTermToTrie;
            }
            else {
                /* Increment our counts */
                (*puiTotalTermCount)++;
                (*puiData) += 1;
                (*puiUniqueTermCount) += (*puiData == 1) ? 1 : 0;
            }
        }
    }



    /* Bail label */
    bailFromiSrchTermsAddTermToTrie:
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermsCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlTrieLoop()
                and gets called for every key in the trie. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlTrieLoop().

    Parameters: pucKey      key (term)
                pvData      data        
                ap      args (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
static int iSrchTermsCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list             ap_;
    int                 iError = 0;
    struct srchIndex    *psiSrchIndex = NULL;
    void                *pvLngStemmer = NULL;
    float               fFrequentTermCoverage = 0;
    struct termsSort    *ptsTermsSort = NULL;
    unsigned int        uiTermsSortLength = 0;
    struct termsSort    *ptsTermsSortPtr = NULL;
    unsigned int        *puiTermsSortIndex = NULL;
    wchar_t             pwcTerm[SRCH_TERMS_MAX_TERM_LENGTH + 1] = {L'\0'};
    unsigned char       pucTerm[SRCH_TERMS_MAX_TERM_LENGTH + 1] = {'\0'};
    unsigned int        uiTermType = 0;
    unsigned int        uiTermCount = 0;
    unsigned int        uiDocumentCount = 0;
    unsigned long       ulIndexBlockID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermsCallBackFunction - [%s][%u]", pucKey, (unsigned int)pvData); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    pvLngStemmer = (void *)va_arg(ap_, void *);
    fFrequentTermCoverage = (float)va_arg(ap_, double);
    ptsTermsSort = (struct termsSort *)va_arg(ap_, struct termsSort *);
    uiTermsSortLength = (unsigned int)va_arg(ap_, unsigned int);
    puiTermsSortIndex = (unsigned int *)va_arg(ap_, unsigned int *);
    va_end(ap_);


    ASSERT(psiSrchIndex != NULL);
    ASSERT(pvLngStemmer != NULL);
    ASSERT(fFrequentTermCoverage >= 0);
    ASSERT(ptsTermsSort != NULL);
    ASSERT(uiTermsSortLength >= 0);
    ASSERT(puiTermsSortIndex != NULL);
    ASSERT(*puiTermsSortIndex < uiTermsSortLength);


    /* Dereference the term sort structure */
    ptsTermsSortPtr = (ptsTermsSort + (*puiTermsSortIndex));

    /* Skip this term if the weight is already negative */
    if ( ptsTermsSortPtr->fWeight < -1 ) {
        return (0);
    }


    /* Convert the term from utf-8 to wide characters */
    if ( (iError = iLngConvertUtf8ToWideString_s(pucKey, 0, pwcTerm, SRCH_TERMS_MAX_TERM_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the term from utf-8 to wide characters, lng error: %d.", iError);
        return (-1);
    }

    /* Stem the term if it is in mixed case */
    if ( bLngCaseIsWideStringAllUpperCase(pwcTerm) == false ) {
        if ( (iError = iLngStemmerStemTerm(pvLngStemmer, pwcTerm, 0)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a term, lng error: %d.", iError);
            return (-1);
        }
    }

    /* Skip the term if it was stemmed out of existence */
    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        return (0);
    }
    
    /* Convert the term from wide characters to utf-8 */
    if ( (iError = iLngConvertWideStringToUtf8_s(pwcTerm, 0, pucTerm, SRCH_TERMS_MAX_TERM_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
        return (-1);
    }

    /* Make a copy of the key and the data, the data contains the term count in the text */
    if ( ptsTermsSortPtr->pucTerm == NULL ) {
        
        /* Duplicate the key */
        if ( (ptsTermsSortPtr->pucTerm = (unsigned char *)s_strdup(pucKey)) == NULL ) {
            return (-1);
        }
        
        /* This cast complains in 64 bit architectures */
        ptsTermsSortPtr->uiTermCountInText = (unsigned int)pvData;
    }


    /* Look up the term */
    if ( (iError = iSrchTermDictLookup(psiSrchIndex, pucTerm, NULL, 0, &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID)) == SRCH_NoError ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', uiTermCount: %u, uiDocumentCount: %u",  */
/*                 pucTerm, uiTermCount, uiDocumentCount); */

        /* Check if this term is frequent if the frequent term coverage has been set */
        if ( (fFrequentTermCoverage >  0) && (uiTermType == SPI_TERM_TYPE_REGULAR) ) {
                
            float    fTermCoverage = ((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100;
                
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', uiTermCount: %u, uiDocumentCount: %u, psiSrchIndex->uiDocumentCount: %u, fTermCoverage: %9.4f%%",  */
/*                     pucTerm, uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount, fTermCoverage); */
            
            /* Flag this terms as frequent if its coverage is greater than the set frequent term coverage */
            if ( fTermCoverage > fFrequentTermCoverage ) {
                uiTermType = SPI_TERM_TYPE_FREQUENT;
            }
        }
    
        /* If this term is a regular term, we work out its IDF weight */
        if ( uiTermType == SPI_TERM_TYPE_REGULAR ) {

            /* Set the term count and the document count */
            ptsTermsSortPtr->uiTermCount += uiTermCount;
            ptsTermsSortPtr->uiDocumentCount += uiDocumentCount;

            /* This works out the IDF of the term */
            ptsTermsSortPtr->fWeight += SRCH_SEARCH_IDF_FACTOR(uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount) * ptsTermsSortPtr->uiTermCountInText;

        }
        
        /* Set the term weight to -1 if this is not a regular term */
        else if ( uiTermType == SPI_TERM_TYPE_STOP ) {
            ptsTermsSortPtr->fWeight = -1;
        }
        else if ( uiTermType == SPI_TERM_TYPE_FREQUENT ) {
            ptsTermsSortPtr->fWeight = -1;
        }
        else {
            ptsTermsSortPtr->fWeight = -1;
        }
    }


    /* Increment the indent */
    (*puiTermsSortIndex)++;


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermsListTerms()

    Purpose:    This function sorts the search feedback sort structure array 
                into descending weight order and searches for the top N terms.

    Parameters: psiSrchIndex        index structure
                ptsTermsSort        terms sort structure array
                uiTermsSortLength   terms sort structure array length
                pfOutputFile        output file
                uiMinTermCount      min term count
                uiMaxTermCount      max term count
                fTermPercentage     term percentage
                fDocumentCoverage   document coverage
                puiUsedTermCount    used term count

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchTermsListTerms
(
    struct termsSort *ptsTermsSort, 
    unsigned int uiTermsSortLength, 
    FILE *pfOutputFile, 
    unsigned int uiDocumentCount, 
    unsigned int uiMinTermCount, 
    unsigned int uiMaxTermCount, 
    float fTermPercentage, 
    float fDocumentCoverage,
    unsigned int *puiUsedTermCount
)
{

    unsigned int        uiI = 0;
    struct termsSort    *ptsTermsSortPtr = NULL;
    unsigned int        uiCurrentTermCount = 0;
    unsigned int        uiLocalMaxTermCount = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermsListTerms"); */


    ASSERT(ptsTermsSort != NULL);
    ASSERT(uiTermsSortLength > 0);
    ASSERT(pfOutputFile != NULL);
    ASSERT(uiMinTermCount > 0);
    ASSERT(uiMaxTermCount >= 0);
    ASSERT(!((uiMaxTermCount > 0) && (uiMinTermCount > uiMaxTermCount)));
    ASSERT((fTermPercentage > 0) && (fTermPercentage <= 100));
    ASSERT((fDocumentCoverage > 0) && (fDocumentCoverage <= 100));


    /* Sort the relevance feedback terms in descending weight */
    s_qsort(ptsTermsSort, uiTermsSortLength, sizeof(struct termsSort), (int (*)(const void *, const void *))iSrchTermsCompareByWeightDesc);


    /* Calculate the maximum term count to use based on the min term count and the percentage */
    if ( uiMaxTermCount == 0 ) {
        uiLocalMaxTermCount = (float)uiTermsSortLength * (fTermPercentage / 100);
        uiLocalMaxTermCount = UTL_MACROS_MAX(uiLocalMaxTermCount, uiMinTermCount);
    }
    else {
        uiLocalMaxTermCount = uiMaxTermCount;
    }


/*     printf("\n\n");  */
/*     printf("uiDocumentCount     : %10u\n", uiDocumentCount);  */
/*     printf("uiMinTermCount      : %10u\n", uiMinTermCount);  */
/*     printf("uiMaxTermCount      : %10u\n", uiMaxTermCount);  */
/*     printf("fTermPercentage     :       %4.1f%%\n", fTermPercentage);  */
/*     printf("fDocumentCoverage   :       %4.1f%%\n", fDocumentCoverage);  */
/*     printf("uiLocalMaxTermCount : %10u\n", uiLocalMaxTermCount);  */
/*     printf("\n\n");  */
/*  */
/*     printf("Term                                Doc Term #      Term #      Document #         Weight        Document coverage\n");  */
/*     printf("---------------------------------------------------------------------------------------------------------------\n");  */


    /* Apply the first uiLocalMaxTermCount terms to the search */
    for ( uiI = 0, uiCurrentTermCount = 0, ptsTermsSortPtr = ptsTermsSort; (uiI < uiTermsSortLength) && (uiCurrentTermCount < uiLocalMaxTermCount); uiI++, ptsTermsSortPtr++ ) {

        /* Only apply terms which:
        **    - has a weight greater than 0
        **    - occurs in under #% of the documents (fDocumentCoverage)
        */
        if ( (ptsTermsSortPtr->fWeight > 0) && 
                ((float)ptsTermsSortPtr->uiTermCount / uiDocumentCount) <= (fDocumentCoverage / 100) ) {

/*             printf("%-30s  %10u      %10u      %10u      %9.4f      %9.4f %s\n", ptsTermsSortPtr->pucTerm, ptsTermsSortPtr->uiTermCountInText, ptsTermsSortPtr->uiTermCount,  */
/*                     ptsTermsSortPtr->uiDocumentCount, ptsTermsSortPtr->fWeight, ((float)ptsTermsSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100, */
/*                     ((float)ptsTermsSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) >= (fDocumentCoverage / 200) ? ">50% max cov" : ""); */

            /* Write the terms out */    
            fprintf(pfOutputFile, "%s ", ptsTermsSortPtr->pucTerm);

            /* Increment the number of terms we have applied */
            uiCurrentTermCount++;
        }
    }
    

    /* Set the return pointer */
    *puiUsedTermCount = uiCurrentTermCount;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermsCompareByWeightDesc()

    Purpose:    This functions takes a two term sort structures and compares their weight.
                This function is used by the qsort call in iSrchTermsListTerms().

    Parameters: ptsTermsSort1   pointer to a term sort structure
                ptsTermsSort2   pointer to a term sort structure

    Globals:    none

    Returns:    1 if ptsTermsSort2 > ptsTermsSort1, 
                -1 if ptsTermsSort1 > ptsTermsSort2, 
                and 0 if ptsTermsSort1 == ptsTermsSort2

*/
static int iSrchTermsCompareByWeightDesc
(
    struct termsSort *ptsTermsSort1,
    struct termsSort *ptsTermsSort2
)
{

    ASSERT(ptsTermsSort1 != NULL);
    ASSERT(ptsTermsSort2 != NULL);


    if ( ptsTermsSort1->fWeight < ptsTermsSort2->fWeight ) {
        return (1);
    }
    else if ( ptsTermsSort1->fWeight > ptsTermsSort2->fWeight ) {
        return (-1);
    }


    return (0);

}


/*---------------------------------------------------------------------------*/

