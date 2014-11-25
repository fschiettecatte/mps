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

    Module:     mpsindexer.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This is where the main() is implemented for the indexer.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.mpsindexer"



/* Enable this to check that the memory size allocated to the indexer can be allocated,
** this is a waste on operating systems which allocate memory without being sure that 
** it will indeed be available, such as linux.
*/
/* #define SRCH_INDEXER_ENABLE_MAX_MEMORY_SIZE_CHECK */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* The default amount of memory to use (megabytes) */
#define SRCH_INDEXER_MEMORY_SIZE_DEFAULT                    (512)

/* Default locale name */
#define SRCH_INDEXER_LOCALE_NAME_DEFAULT                    LNG_LOCALE_EN_US_UTF_8_NAME

/* The default stemmer name */
#define SRCH_INDEXER_STEMMER_NAME_DEFAULT                   LNG_STEMMER_PLURAL_NAME

/* The default stop list name */
#define SRCH_INDEXER_STOP_LIST_NAME_DEFAULT                 LNG_STOP_LIST_GOOGLE_MODIFIED_NAME


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);


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

    int                     iError = SRCH_NoError;
    unsigned char           *pucNextArgument = NULL;
    unsigned char           *pucCommandPath = NULL;

    unsigned char           pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char           pucIndexDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char           pucTemporaryDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char           pucIndexStreamFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};

    struct srchIndexer      siSrchIndexer;

    unsigned char           *pucLocaleName = SRCH_INDEXER_LOCALE_NAME_DEFAULT;

    unsigned char           *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int            uiLogLevel = UTL_LOG_LEVEL_INFO;



    /* Initialize the log */
    if ( (iError = iUtlLogInit()) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the log, utl error: %d", iError);
    }


    /* Set up the search indexer structure */
    siSrchIndexer.pucIndexDirectoryPath = NULL;    
    siSrchIndexer.pucConfigurationDirectoryPath = NULL;
    siSrchIndexer.pucTemporaryDirectoryPath = NULL;

    siSrchIndexer.pucIndexName = NULL;
    siSrchIndexer.pucIndexDescription = NULL;

    siSrchIndexer.uiTermLengthMinimum = SRCH_TERM_LENGTH_MINIMUM_DEFAULT;
    siSrchIndexer.uiTermLengthMaximum = SRCH_TERM_LENGTH_MAXIMUM_DEFAULT;
    siSrchIndexer.pucStemmerName = SRCH_INDEXER_STEMMER_NAME_DEFAULT;
    siSrchIndexer.pucLanguageCode = NULL;
    siSrchIndexer.pucTokenizerName = NULL;
    siSrchIndexer.pucStopListName = SRCH_INDEXER_STOP_LIST_NAME_DEFAULT;
    siSrchIndexer.pucStopListFilePath = NULL;

    siSrchIndexer.uiIndexerMemorySizeMaximum = SRCH_INDEXER_MEMORY_SIZE_DEFAULT;
    siSrchIndexer.bSuppressMessages = false;

    siSrchIndexer.pfFile = stdin;



    /* Get the command path */
    pucCommandPath = pucUtlArgsGetNextArg(&argc, &argv);

    /* Put up an error message if there were no arguments defined and bail */
    if ( (pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv)) == NULL ) {
        vVersion();
        vUsage(pucCommandPath);
        s_exit(EXIT_SUCCESS);
    }


    /* Process parameters */
    while ( pucNextArgument != NULL ) {

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

            /* Set the configuration directory path */
            siSrchIndexer.pucConfigurationDirectoryPath = pucConfigurationDirectoryPath;
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

            /* Set the index directory path */
            siSrchIndexer.pucIndexDirectoryPath = pucIndexDirectoryPath;
        }

        /* Check for temporary directory path */
        else if ( s_strncmp("--temporary-directory=", pucNextArgument, s_strlen("--temporary-directory=")) == 0 ) {

            /* Get the temporary temporary directory path */
            pucNextArgument += s_strlen("--temporary-directory=");

            /* Get the true temporary directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucTemporaryDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true temporary directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that text file path exists */
            if ( bUtlFilePathExists(pucTemporaryDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory path: '%s', does not exist", pucTemporaryDirectoryPath);
            }

            /* Check that the temporary directory path exists */
            if ( bUtlFileIsDirectory(pucTemporaryDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory path: '%s', is not a directory", pucTemporaryDirectoryPath);
            }

            /* Check that the temporary directory path can be written to */
            if ( (bUtlFilePathRead(pucTemporaryDirectoryPath) == false) || (bUtlFilePathWrite(pucTemporaryDirectoryPath) == false) || (bUtlFilePathExec(pucTemporaryDirectoryPath) == false) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory path: '%s', cannot be accessed", pucTemporaryDirectoryPath);
            }

            /* Set temporary directory path */
            siSrchIndexer.pucTemporaryDirectoryPath = pucTemporaryDirectoryPath;
        }

        /* Check for index */
        else if ( s_strncmp("--index=", pucNextArgument, s_strlen("--index=")) == 0 ) {

            /* Get the index name */
            pucNextArgument += s_strlen("--index=");

            /* Set the index name */
            siSrchIndexer.pucIndexName = pucNextArgument;
        }

        /* Check for description */
        else if ( s_strncmp("--description=", pucNextArgument, s_strlen("--description=")) == 0 ) {

            /* Get the index description */
            pucNextArgument += s_strlen("--description=");

            /* Set the index description */
            siSrchIndexer.pucIndexDescription = pucNextArgument;
        }

        /* Check for stop list */
        else if ( s_strncmp("--stoplist=", pucNextArgument, s_strlen("--stoplist=")) == 0 ) {

            /* Get the stop list name */
            pucNextArgument += s_strlen("--stoplist=");

            /* Check the stop list name */
            if ( (iError = iLngCheckStopListName(pucNextArgument)) != LNG_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid stop list name: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Set stop list name */
            siSrchIndexer.pucStopListName = pucNextArgument;
        } 

        /* Check for stop file */
        else if ( s_strncmp("--stopfile=", pucNextArgument, s_strlen("--stopfile=")) == 0 ) {

            /* Get the stop list file path */
            pucNextArgument += s_strlen("--stopfile=");

            /* Clean the stop list file path */
            if ( (iError = iUtlFileCleanPath(pucNextArgument)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to clean the stop list file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that stop list file path exists */
            if ( bUtlFilePathExists(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The stop list file path: '%s', does not exist", pucNextArgument);
            }

            /* Check that the stop list file path is a file */
            if ( bUtlFileIsFile(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The stop list file path: '%s', is not a file", pucNextArgument);
            }

            /* Check that the stop list file path can be accessed */
            if ( bUtlFilePathRead(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The stop list file path: '%s', cannot be accessed", pucNextArgument);
            }

            /* Set stop list file path */
            siSrchIndexer.pucStopListFilePath = pucNextArgument;
        } 

        /* Check for stemmer */
        else if ( s_strncmp("--stemmer=", pucNextArgument, s_strlen("--stemmer=")) == 0 ) {

            /* Get the stemmer name */
            pucNextArgument += s_strlen("--stemmer=");

            /* Check the stemmer name */
            if ( (iError = iLngCheckStemmerName(pucNextArgument)) != LNG_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid stemmer name: '%s', lng error: %d", pucNextArgument, iError);
            }

            /* Set the stemmer name */
            siSrchIndexer.pucStemmerName = pucNextArgument;
        }

        /* Check for minimum term length */
        else if ( s_strncmp("--minimum-term-length=", pucNextArgument, s_strlen("--minimum-term-length=")) == 0 ) {

            /* Get the minimum term length */
            pucNextArgument += s_strlen("--minimum-term-length=");

            /* Check the minimum term length */
            if ( s_strtol(pucNextArgument, NULL, 10) < SRCH_TERM_LENGTH_MINIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the minimum term length to be greater than or equal to: %d", SRCH_TERM_LENGTH_MINIMUM);
            }
            
            /* Set the minimum term length */
            siSrchIndexer.uiTermLengthMinimum = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for maximum term length */
        else if ( s_strncmp("--maximum-term-length=", pucNextArgument, s_strlen("--maximum-term-length=")) == 0 ) {

            /* Get the maximum term length */
            pucNextArgument += s_strlen("--maximum-term-length=");

            /* Check the maximum term length */
            if ( s_strtol(pucNextArgument, NULL, 10) < SRCH_TERM_LENGTH_MINIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum term length to be greater than or equal to: %d", SRCH_TERM_LENGTH_MINIMUM);
            }
            
            if ( s_strtol(pucNextArgument, NULL, 10) > SRCH_TERM_LENGTH_MAXIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum term length to be less than or equal to: %d", SRCH_TERM_LENGTH_MAXIMUM);
            }
            
            /* Set the maximum term length */
            siSrchIndexer.uiTermLengthMaximum = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for stream */
        else if ( s_strncmp("--stream=", pucNextArgument, s_strlen("--stream=")) == 0 ) {

            /* Get the structured index stream file path */
            pucNextArgument += s_strlen("--stream=");

            /* Get the true structured index stream file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucIndexStreamFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true structured index stream file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that structured index stream file path exists */
            if ( bUtlFilePathExists(pucIndexStreamFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The structured index stream file path: '%s', does not exist", pucIndexStreamFilePath);
            }

            /* Check that the structured index stream file path is a file */
            if ( bUtlFileIsFile(pucIndexStreamFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The structured index stream file path: '%s', is not a file", pucIndexStreamFilePath);
            }

            /* Check that the structured index stream file path can be accessed */
            if ( bUtlFilePathRead(pucIndexStreamFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The structured index stream file path: '%s', cannot be accessed", pucIndexStreamFilePath);
            }
        }

        /* Check for maximum memory */
        else if ( s_strncmp("--maximum-memory=", pucNextArgument, s_strlen("--maximum-memory=")) == 0 ) {

            /* Get the maximum memory */
            pucNextArgument += s_strlen("--maximum-memory=");

            /* Check the maximum memory */
            if ( s_strtol(pucNextArgument, NULL, 10) < SRCH_INDEXER_MEMORY_MINIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum amount of memory to use to be greater than or equal to: %d", SRCH_INDEXER_MEMORY_MINIMUM);
            }

            if ( s_strtol(pucNextArgument, NULL, 10) > SRCH_INDEXER_MEMORY_MAXIMUM ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum amount of memory to use to be less than or equal to: %d", SRCH_INDEXER_MEMORY_MAXIMUM);
            }

            /* Set the maximum memory */
            siSrchIndexer.uiIndexerMemorySizeMaximum = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for suppress */
        else if ( s_strcmp("--suppress", pucNextArgument) == 0 ) {

            /* Set suppress */
            siSrchIndexer.bSuppressMessages = true;
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



#if defined(SRCH_INDEXER_ENABLE_MAX_MEMORY_SIZE_CHECK)
    /* See if we can actually get the memory we have been allows to use to index this index,
    ** we do this so  that we are pretty sure that we going to have enough memory to run the index
    ** without running out mid-way
    */
    if ( (pucPtr = (unsigned char *)s_malloc((size_t)((siSrchIndexer.uiIndexerMemorySizeMaximum * (1024 * 1024)) * sizeof(unsigned char)))) == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to allocate: %u MB of memory, please reduce the allocation or free up memory", siSrchIndexer.uiIndexerMemorySizeMaximum / (1024 * 1024));
    }
    s_free(pucPtr);
#endif    /* defined(SRCH_INDEXER_ENABLE_MAX_MEMORY_SIZE_CHECK) */


    /* Check for index directory path */
    if ( bUtlStringsIsStringNULL(siSrchIndexer.pucIndexDirectoryPath) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "An index directory path is required");
    }

    /* Check for configuration directory path */
    if ( bUtlStringsIsStringNULL(siSrchIndexer.pucConfigurationDirectoryPath) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A configuration directory path is required");
    }

    /* Check for index name */
    if ( bUtlStringsIsStringNULL(siSrchIndexer.pucIndexName) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "An index name is required");
    }


    /* Set the index description from the index name, default to the index name if we cannot get the index name */
    if ( bUtlStringsIsStringNULL(siSrchIndexer.pucIndexDescription) == true ) {
        if ( iUtlFileGetPathBase(siSrchIndexer.pucIndexName, &siSrchIndexer.pucIndexDescription) != UTL_NoError ) {
            siSrchIndexer.pucIndexDescription = siSrchIndexer.pucIndexName;
        }
    }


    /* We use the stop list file path if both the stop list name and stop list file path are 
    ** set, to do that we just need to NULL the stop list name pointer, we do this silently
    ** because the stop list name is defaulted
    */
    if ( (bUtlStringsIsStringNULL(siSrchIndexer.pucStopListName) == false) && (bUtlStringsIsStringNULL(siSrchIndexer.pucStopListFilePath) == false) ) {
        siSrchIndexer.pucStopListName = NULL;
    }


    /* Check that the maximum term length is greater than the minimum term length */
    if ( siSrchIndexer.uiTermLengthMinimum > siSrchIndexer.uiTermLengthMaximum ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The maximum term length needs to be greater than the minimum term length");
    }


    /* Open the file we want to read the stream from (if specified) */        
    if ( bUtlStringsIsStringNULL(pucIndexStreamFilePath) == false ) { 
        if ( (siSrchIndexer.pfFile = s_fopen(pucIndexStreamFilePath, "r")) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the structured index stream file: '%s'", pucIndexStreamFilePath);
        }
    }
    


    /* Index the index from the indexer profile */
    if ( (iError = iSrchIndexerCreateIndexFromSearchIndexer(&siSrchIndexer)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create the index, srch error: %d", iError);
    }


    /* Close the search indexer structure file descriptor if it was opened from a file */
    if ( bUtlStringsIsStringNULL(pucIndexStreamFilePath) == false ) {
        s_fclose(siSrchIndexer.pfFile);
    }


    /* Free any allocated resources */
    if ( siSrchIndexer.pucLanguageCode != NULL ) {
        s_free(siSrchIndexer.pucLanguageCode);
    }

    if ( siSrchIndexer.pucTokenizerName != NULL ) {
        s_free(siSrchIndexer.pucTokenizerName);
    }


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
    printf("MPS Indexer, %s\n", UTL_VERSION_COPYRIGHT_STRING);


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    printf("%s\n", pucVersionString);


    /* Get the tokenizer features string */
    iLngTokenizerGetFeaturesString(pucTokenizerFeaturesString, LNG_TOKENIZER_FEATURES_STRING_LENGTH + 1);

    /* Tokenizer features message */
    printf("%s\n", pucTokenizerFeaturesString);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters that the indexer
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
    printf(" Indexing parameters:\n");

    printf("  --configuration-directory=name \n");
    printf("                  Configuration directory. \n");
    printf("  --index-directory=name \n");
    printf("                  Index directory.\n");
    printf("  --temporary-directory=name \n");
    printf("                  Temporary directory. \n");
    printf("  --index=name \n");
    printf("                  Index name, required. \n");
    printf("  --description=name \n");
    printf("                  Index description, defaults to the index name. \n");
    printf("  --stoplist=name \n");
    printf("                  Stop list to use, default: '%s' , stop lists available: '%s', \n", SRCH_INDEXER_STOP_LIST_NAME_DEFAULT, LNG_STOP_LIST_NONE_NAME);
    printf("                  '%s', '%s'.\n", LNG_STOP_LIST_GOOGLE_NAME, LNG_STOP_LIST_GOOGLE_MODIFIED_NAME);
    printf("  --stopfile=name \n");
    printf("                  Stop list term file, overriding the internal stop list, one term per line. \n");
    printf("  --stemmer=name  Stemmer to use, default: '%s', stemmers available: '%s', \n", SRCH_INDEXER_STEMMER_NAME_DEFAULT, LNG_STEMMER_NONE_NAME);
    printf("                  '%s', '%s', '%s'.\n", LNG_STEMMER_PLURAL_NAME, LNG_STEMMER_PORTER_NAME, LNG_STEMMER_LOVINS_NAME);
    printf("  --minimum-term-length=# \n");
    printf("                  Minimum length of a term, defaults to %d, minimum: %d. \n", SRCH_TERM_LENGTH_MINIMUM_DEFAULT, SRCH_TERM_LENGTH_MINIMUM);
    printf("  --maximum-term-length=# \n");
    printf("                  Maximum length of a term, defaults to %d, maximum: %d. \n", SRCH_TERM_LENGTH_MAXIMUM_DEFAULT, SRCH_TERM_LENGTH_MAXIMUM);
    printf("  --stream=name   Read the index stream from this file, defaults to 'stdin'. \n");
    printf("  --maximum-memory=# \n");
    printf("                  Number of megabytes to limit the indexer to, defaults to: %dMB, \n", SRCH_INDEXER_MEMORY_SIZE_DEFAULT);
    printf("                  minimum: %dMB, maximum: %dMB. \n", SRCH_INDEXER_MEMORY_MINIMUM, SRCH_INDEXER_MEMORY_MAXIMUM);
    printf("  --suppress      Suppress routine parser messages that may be sent as part of the stream. \n");

    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", SRCH_INDEXER_LOCALE_NAME_DEFAULT);
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

