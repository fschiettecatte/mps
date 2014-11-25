/*****************************************************************************
*        Copyright (C) 2010-2011, FS Consulting LLC. All rights reserved     *
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

    Created:    5 May 2010

    Purpose:    This is a simple app to test the parser

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.misc.parser"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* File name extension separators */
#define PRS_FILENAME_EXTENSION_SEPARATORS       (unsigned char *)", "


/* Default language ID */
#define PRS_LANGUAGE_ID_DEFAULT                 LNG_LANGUAGE_EN_ID

/* Default language code */
#define PRS_LANGUAGE_CODE_DEFAULT               LNG_LANGUAGE_EN_CODE

/* Default tokenizer ID */
#define PRS_TOKENIZER_ID_DEFAULT                LNG_TOKENIZER_FSCLT_2_ID

/* Default tokenizer name */
#define PRS_TOKENIZER_NAME_DEFAULT              LNG_TOKENIZER_FSCLT_2_NAME


/* String length */
#define PRS_STRING_LENGTH                       (5120)


/* Actions */
#define PRS_ACTION_NORMALIZE                    (1)
#define PRS_ACTION_TOKENIZE                     (2)
#define PRS_ACTION_SEARCH                       (3)


/* Default locale name */
#define PRS_LOCALE_NAME_DEFAULT                 LNG_LOCALE_EN_US_UTF_8_NAME


/* Replacement character for bad unicode */
#define PRS_UNICODE_REPLACEMENT_CHARACTER       (unsigned char)' '


/* Spacer string for output */
#define PRS_SPACER_STRING                       (unsigned char *)"  "


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Parser structure */
struct prsParser {

    unsigned int    uiLanguageID;                       /* Language ID */
    unsigned int    uiTokenizerID;                      /* Tokenizer ID */

    void            *pvLngTokenizer;                    /* Tokenizer structure */
    void            *pvLngConverter;                    /* Converter structure */
    void            *pvLngUnicodeNormalizer;            /* Unicode normalizer structure */
    void            *pvSrchParser;                      /* Search parser structure */

    boolean         bCleanUnicode;                      /* Clean unicode */
    boolean         bSkipUnicodeErrors;                 /* Skip unicode errors */

    unsigned int    uiAction;                           /* Action */
    
    unsigned char   *pucSpacerString;                   /* Spacer string */
    
    boolean         bOutputOriginalText;                /* Output the original text along with the parsed text */
    FILE            *pfOutputFile;                      /* Output file */

};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);

static void vParseFilePath (struct prsParser *pppPrsParser, unsigned char *pucFilePath);
static void vParseFile (struct prsParser *pppPrsParser, FILE *pfFile);
static void vParseString (struct prsParser *pppPrsParser, unsigned char *pucLine);


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

    int                 iError = UTL_NoError;
    unsigned char       *pucCommandPath = NULL;
    unsigned char       *pucNextArgument = NULL;

    struct prsParser    ppPrsParser;    

    unsigned char       pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};

    unsigned char       *pucLanguageCode = PRS_LANGUAGE_CODE_DEFAULT;
    unsigned char       *pucTokenizerName = PRS_TOKENIZER_NAME_DEFAULT;

    boolean             bNormalizeUnicode = false;

    unsigned char       pucLine[PRS_STRING_LENGTH + 1] = {'\0'};;
    unsigned char       *pucFilePath = NULL;

    unsigned char       *pucLocaleName = PRS_LOCALE_NAME_DEFAULT;

    unsigned char       *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int        uiLogLevel = UTL_LOG_LEVEL_INFO;




    /* Set up the parser structure */
    ppPrsParser.uiLanguageID = PRS_LANGUAGE_ID_DEFAULT;
    ppPrsParser.uiTokenizerID = PRS_TOKENIZER_ID_DEFAULT;
    ppPrsParser.pvLngTokenizer = NULL;
    ppPrsParser.pvLngConverter = NULL;
    ppPrsParser.pvLngUnicodeNormalizer = NULL;
    ppPrsParser.pvSrchParser = NULL;
    ppPrsParser.bCleanUnicode = false;
    ppPrsParser.bSkipUnicodeErrors = false;
    ppPrsParser.uiAction = PRS_ACTION_NORMALIZE;
    ppPrsParser.pucSpacerString = PRS_SPACER_STRING;
    ppPrsParser.bOutputOriginalText = false;
    ppPrsParser.pfOutputFile = stdout;


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

        /* Check for tokenize */
        else if ( s_strcmp("--tokenize", pucNextArgument) == 0 ) {

            /* Set the action to tokenize */
            ppPrsParser.uiAction = PRS_ACTION_TOKENIZE;
        }

        /* Check for search */
        else if ( s_strcmp("--search", pucNextArgument) == 0 ) {

            /* Set the action as search */
            ppPrsParser.uiAction = PRS_ACTION_SEARCH;
        }

        /* Check for text */
        else if ( s_strncmp("--text=", pucNextArgument, s_strlen("--text=")) == 0 ) {

            /* Get the text */
            pucNextArgument += s_strlen("--text=");

            /* Set the text */
            s_strncpy(pucLine, pucNextArgument, PRS_STRING_LENGTH + 1);
        }    

        /* Check for file name */
        else if ( s_strncmp("--file-name=", pucNextArgument, s_strlen("--file-name=")) == 0 ) {

            /* Get the file name */
            pucNextArgument += s_strlen("--file-name=");

            /* Set the file path */
            pucFilePath = pucNextArgument;
        }    

        /* Check for output original text */
        else if ( s_strcmp("--output-original-text", pucNextArgument) == 0 ) {

            /* Set the output original text */
            ppPrsParser.bOutputOriginalText = true;
        }

        /* Check for spacer string */
        else if ( s_strncmp("--spacer-string=", pucNextArgument, s_strlen("--spacer-string=")) == 0 ) {

            /* Get the spacer string */
            pucNextArgument += s_strlen("--spacer-string=");

            /* Set the spacer string */
            ppPrsParser.pucSpacerString = pucNextArgument;
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
            vVersion();
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid option: '%s', try '-?', '--help' or '--usage' for more information", pucNextArgument);
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


    /* Warn if the text and file path were both defined */
    if ( (bUtlStringsIsStringNULL(pucLine) == false) && (bUtlStringsIsStringNULL(pucFilePath) == false) ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The text and file path cannont both be defined.");
    }    


    /* Get the tokenizer ID from the tokenizer name */
    if ( (iError = iLngGetTokenizerIDFromName(pucTokenizerName, &ppPrsParser.uiTokenizerID)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the tokenizer ID from the tokenizer name: '%s', lng error: %d.", pucTokenizerName, iError); 
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
    if ( (iError = iLngTokenizerCreateByID(pucConfigurationDirectoryPath, ppPrsParser.uiTokenizerID, ppPrsParser.uiLanguageID, &ppPrsParser.pvLngTokenizer)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create the tokenizer, lng error: %d", iError);
    }


    /* Create the unicode normalizer */
    if ( (bNormalizeUnicode == true) || (ppPrsParser.uiAction == PRS_ACTION_NORMALIZE) ) {
        if ( (iError = iLngUnicodeNormalizerCreate(pucConfigurationDirectoryPath, &ppPrsParser.pvLngUnicodeNormalizer)) != LNG_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create a unicode normalizer, lng error: %d", iError);
        }
    }


    /* Create the search parser */
    if (  ppPrsParser.uiAction == PRS_ACTION_SEARCH ) {
    
        /* Search stucture */
        struct srchSearch ssSrchSearch;
        
        /* Copy the configuration directory path to the search structure */
        s_strnncpy(ssSrchSearch.pucConfigurationDirectoryPath, pucConfigurationDirectoryPath, UTL_FILE_PATH_MAX + 1);
    
        /* Create the search configuration file path into the search structure */
        if ( (iError = iUtlFileMergePaths(ssSrchSearch.pucConfigurationDirectoryPath, SRCH_SEARCH_CONFIG_FILE_NAME, ssSrchSearch.pucConfigurationFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create the search configuration file path, search configuration file name: '%s', search configuration directory path: '%s', utl error: %d.", 
                    SRCH_SEARCH_CONFIG_FILE_NAME, ssSrchSearch.pucConfigurationDirectoryPath, iError); 
        }

        /* Open the search configuration - search configuration file may or may not be there */
        if ( (iError = iUtlConfigOpen(ssSrchSearch.pucConfigurationFilePath, UTL_CONFIG_FILE_FLAG_OPTIONAL, &ssSrchSearch.pvUtlConfig)) != UTL_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create the search configuration, utl error: %d.", iError); 
        }
        
        /* Create the search parser */
        if ( (iError = iSrchParserCreate(&ssSrchSearch, &ppPrsParser.pvSrchParser)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the search parser, srch error: %d.", iError);
        }
    }



    /* Parse text */
    if ( bUtlStringsIsStringNULL(pucLine) == false ) {
        vParseString(&ppPrsParser, pucLine);
    }
    /* Parse file path */
    else if ( bUtlStringsIsStringNULL(pucFilePath) == false ) {
        vParseFilePath(&ppPrsParser, pucFilePath);
    }
    /* Parse from 'stdin' */
    else {
        vParseFile(&ppPrsParser, stdin);
    }


    
    /* Free the tokenizer */
    iLngTokenizerFree(ppPrsParser.pvLngTokenizer);
    ppPrsParser.pvLngTokenizer = NULL;
    
    /* Free the character set converter */
    iLngConverterFree(ppPrsParser.pvLngConverter);
    ppPrsParser.pvLngConverter = NULL;
    
    /* Free the unicode normalizer */
    iLngUnicodeNormalizerFree(ppPrsParser.pvLngUnicodeNormalizer);
    ppPrsParser.pvLngUnicodeNormalizer = NULL;


    /* Free the search parser */
    if ( ppPrsParser.pvSrchParser != NULL ) {
    
/*         if ( ssSrchSearch->pvUtlConfig != NULL ) { */
/*             iUtlConfigFree(ssSrchSearch->pvUtlConfig); */
/*             ppPrsParser.ssSrchSearch = NULL; */
/*         } */
    
        iSrchParserFree(ppPrsParser.pvSrchParser);
        ppPrsParser.pvSrchParser = NULL;
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
    fprintf(stderr, "Parser, %s\n", UTL_VERSION_COPYRIGHT_STRING);


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
    printf("\n");

    printf(" Parsing control parameters:\n");
    printf("  --language=code \n");
    printf("                  Language code, defaults to '%s'. \n", PRS_LANGUAGE_CODE_DEFAULT);
    printf("  --tokenizer=name \n");
    printf("                  Tokenizer name, defaults to: '%s', tokenizers available: '%s', \n", PRS_TOKENIZER_NAME_DEFAULT, LNG_TOKENIZER_FSCLT_1_NAME);
    printf("                  '%s'. \n", LNG_TOKENIZER_FSCLT_2_NAME);
    printf("  --normalize-unicode \n");
    printf("                  Normalize the unicode, default is to leave it as is, normalization mode is set by configuration file. \n");
    printf("  --clean-unicode  \n");
    printf("                  Clean the unicode replacing invalid characters with spaces. \n");
    printf("  --skip-unicode-errors \n");
    printf("                  Skip unicode errors as opposed to erroring out. \n");
    printf("  --tokenize|--search \n");
    printf("                  Tokenize the text or treat is as a search, default is to normalize only. \n");
    printf("\n");

    printf(" Text parameters: \n");
    printf("  --text=\"text\"   Text, optional, defaults to 'stdin'. \n");
    printf("  --file-name=name \n");
    printf("                  File name, optional, defaults to 'stdin'. \n");

    printf(" Output parameters: \n");
    printf("  --output-original-text \n");
    printf("                  Output the original text along with the parsed text, default is not to do so. \n");
    printf("  --spacer-string=string \n");
    printf("                  Spacer string output between each term, default: '%s'. \n", PRS_SPACER_STRING);

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", PRS_LOCALE_NAME_DEFAULT);
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

    Function:   vParseFilePath()

    Purpose:    Parse the file text

    Parameters: pppPrsParser    parser structure
                pucFilePath     file path

    Globals:    none

    Returns:    void

*/
static void vParseFilePath
(
    struct prsParser *pppPrsParser,
    unsigned char *pucFilePath
)
{
    
    FILE    *pfFile = NULL;


    ASSERT(pppPrsParser != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucFilePath) == false);


    /* Open the file */
    if ( (pfFile = s_fopen(pucFilePath, "r")) == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to access the structured index stream file: '%s'", pucFilePath);
    }

    /* Parse the file */
    vParseFile(pppPrsParser, pfFile);
    
    /* Close the file */
    s_fclose(pfFile);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vParseFile()

    Purpose:    Parse the file

    Parameters: pppPrsParser        parser structure
                pfFile              file

    Globals:    none

    Returns:    void

*/
static void vParseFile
(
    struct prsParser *pppPrsParser,
    FILE *pfFile
)
{
    
    unsigned char   pucLine[PRS_STRING_LENGTH + 1] = {'\0'};


    ASSERT(pppPrsParser != NULL);
    ASSERT(pfFile != NULL);


    /* Loop over the file descriptor reading each line and parsing the file */
    while ( s_fgets(pucLine, PRS_STRING_LENGTH, pfFile) != NULL ) {
            
        unsigned char    *pucLineEndPtr = NULL;

        /* Get the text length */
        unsigned int uiLineLength = s_strlen(pucLine);

        /* Erase the trailing new line - be platform sensitive, handle PC first, then Unix and Mac  */
        if ( (uiLineLength >= 2) && (pucLine[uiLineLength - 2] == '\r') ) {
            pucLineEndPtr = pucLine + (uiLineLength - 2);
        }
        else if ( (uiLineLength >= 1) && ((pucLine[uiLineLength - 1] == '\n') || (pucLine[uiLineLength - 1] == '\r')) ) {
            pucLineEndPtr = pucLine + (uiLineLength - 1);
        }

        /* Null terminate the text */
        if ( pucLineEndPtr != NULL ) {
            *pucLineEndPtr = '\0';
        }

        /* Parse the text */
        vParseString(pppPrsParser, pucLine);
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vParseString()

    Purpose:    Parse the string

    Parameters: pppPrsParser    parser structure
                pucString       string

    Globals:    none

    Returns:    void

*/
static void vParseString
(
    struct prsParser *pppPrsParser,
    unsigned char *pucString
)
{


    unsigned int    iError = LNG_NoError;
    unsigned char   *pucStringPtr = pucString;
    unsigned int    uiStringLength = s_strlen(pucString);
    unsigned char   *pucNormalizedString = NULL;
    wchar_t         *pwcWideString = NULL;
    unsigned int    uiWideTextLength = 0;
    unsigned int    uiWideTextByteLength = 0;
    boolean         bFirstTerm = true;


    ASSERT(pppPrsParser != NULL);


    /* Bail this if the text is empty */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        goto bailFromvParseString;
    }


    /* Output the text */
    if ( pppPrsParser->bOutputOriginalText == true ) {
        fprintf(pppPrsParser->pfOutputFile, "%s\n", pucString);
    }


    /* Clean the unicode if requested */
    if ( pppPrsParser->bCleanUnicode == true ) {
        
        /* Clean the unicode */
        if ( (iError = iLngUnicodeCleanUtf8String(pucStringPtr, PRS_UNICODE_REPLACEMENT_CHARACTER)) != LNG_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to clean the string, lng error: %d, string: '%s'", iError, pucStringPtr);
        }

        /* Set the text pointer */
        pucStringPtr = pucString;
    }


    /* Normalize the line if the unicode normalizer is enabled */
    if ( pppPrsParser->pvLngUnicodeNormalizer != NULL ) {
        
        /* Set the normalized text length */
        unsigned int uiNormalizedTextLength = s_strlen(pucStringPtr) * LNG_UNICODE_NORMALIZATION_EXPANSION_MULTIPLIER_MAX;

        /* Allocate the normalized text */
        if ( (pucNormalizedString = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * uiNormalizedTextLength))) == NULL ) {
            goto bailFromvParseString;
        }

        /* Normalize the text */
        if ( (iError = iLngUnicodeNormalizeString(pppPrsParser->pvLngUnicodeNormalizer, pucStringPtr, uiStringLength, &pucNormalizedString, &uiNormalizedTextLength)) != LNG_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to normalize the string, lng error: %d, string: '%s'", iError, pucStringPtr);
        }
        
        /* Set the text pointer */
        pucStringPtr = pucNormalizedString;
        uiStringLength = s_strlen(pucStringPtr);
    }


    /* Allocate the wide character text */
    if ( (pwcWideString = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (uiStringLength  + 1)))) == NULL ) {
        goto bailFromvParseString;
    }

    /* Set the wide text byte length */
    uiWideTextByteLength = (uiStringLength  + 1) * sizeof(wchar_t);


    /* Convert the line from utf-8 to wide character */
    if ( (iError = iLngConverterConvertString(pppPrsParser->pvLngConverter, ((pppPrsParser->bSkipUnicodeErrors == true) ? LNG_CONVERTER_SKIP_ON_ERROR : LNG_CONVERTER_RETURN_ON_ERROR), 
            pucStringPtr, uiStringLength, (unsigned char **)&pwcWideString, &uiWideTextByteLength)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to convert the string from utf-8 set to wide characters, lng error: %d, string: '%s'", iError, pucStringPtr);
    }
    
    /* Set the wide text length */
    uiWideTextLength = s_wcslen(pwcWideString);



    /* Tokenize the text */
    if ( pppPrsParser->uiAction == PRS_ACTION_TOKENIZE ) {

        /* Parse the line, note that we use the language ID if it is known, otherwise we fall back to the default language ID */
        if ( (iError = iLngTokenizerParseString(pppPrsParser->pvLngTokenizer, pppPrsParser->uiLanguageID, pwcWideString, uiWideTextLength)) != LNG_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to parse the string, lng error: %d, string: '%ls'", iError, pwcWideString);
        }
    
        /* Process tokens, control the loop from within */
        while ( true ) {
        
            wchar_t     *pwcTermStartPtr = NULL;
            wchar_t     *pwcTermEndPtr = NULL;
            boolean     bNormalized = true;
            wchar_t     wcTermEnd = L'\0';
            boolean     bFirstComponent = true;
    
            /* Get the next token */
            if ( (iError = iLngTokenizerGetToken(pppPrsParser->pvLngTokenizer, &pwcTermStartPtr, &pwcTermEndPtr)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a token from the string, lng error: %d, string: '%ls'", iError, pwcWideString);
            }
    
            /* Break if this was the last token */
            if ( pwcTermStartPtr == NULL ) {
                break;
            }

            /* Save the term end character and NULL terminate the string */ 
            wcTermEnd = *pwcTermEndPtr;
            *pwcTermEndPtr = L'\0';
        
            /* See if this term is normalized */
            if ( (iError = iLngTokenizerIsTokenNormalized(pppPrsParser->pvLngTokenizer, &bNormalized)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the normalized flag for term, lng error: %d, term: '%ls'", iError, pwcTermStartPtr);
            }
    
            /* Output a spacer */
            if ( bFirstTerm == false ) {
                fprintf(pppPrsParser->pfOutputFile, pppPrsParser->pucSpacerString);
            }
            bFirstTerm = false;

            /* Output the term */
            fprintf(pppPrsParser->pfOutputFile, "%ls%s", pwcTermStartPtr, (bNormalized == true) ? "{N}" : "");
    
            /* Restore the end character */
            *pwcTermEndPtr = wcTermEnd;

            /* Process components, control the loop from within */
            while ( true ) {
            
                wchar_t     *pwcComponentStartPtr = NULL;
                wchar_t     *pwcComponentEndPtr = NULL;
                wchar_t     wcComponentEnd = L'\0';
            
                /* Get the next component for this token */
                if ( (iError = iLngTokenizerGetComponent(pppPrsParser->pvLngTokenizer, &pwcComponentStartPtr, &pwcComponentEndPtr)) != LNG_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the string, lng error: %d, string: '%ls'", iError, pwcWideString);
                }
    
                /* Break if this was the last component */
                if ( pwcComponentStartPtr == NULL ) {
                    break;
                }
    
                /* Save the component end character and NULL terminate the string */ 
                wcComponentEnd = *pwcComponentEndPtr;
                *pwcComponentEndPtr = L'\0';
    
                /* Open up the component */
                if ( bFirstComponent == true ) {
                    fprintf(pppPrsParser->pfOutputFile, "(");
                    bFirstComponent = false;
                }
                /* Else output a spacer */
                else {
                    fprintf(pppPrsParser->pfOutputFile, pppPrsParser->pucSpacerString);
                }
            
                /* Output the component */
                fprintf(pppPrsParser->pfOutputFile, "%ls%s", pwcComponentStartPtr, (bNormalized == true) ? "{N}" : "");
                
                /* Restore the end character */
                *pwcComponentEndPtr = wcComponentEnd;
            }    
    
            /* Close off the component */
            if ( bFirstComponent == false ) {
                fprintf(pppPrsParser->pfOutputFile, ")");
            }
        }
    }

    /* Treat the text as a search */
    else if ( pppPrsParser->uiAction == PRS_ACTION_SEARCH ) {

        unsigned int    iSrchParserError = SRCH_NoError;
        wchar_t         *pwcSrchParserNormalizedSearchText = NULL;
        unsigned char   pucErrorString[PRS_STRING_LENGTH + 1] = {'\0'};

        /* Parse the line, note that we use the language ID if it is known, otherwise we fall back to the default language ID */
        iError = iSrchParserParse(pppPrsParser->pvSrchParser, pppPrsParser->uiLanguageID, pppPrsParser->uiTokenizerID, pwcWideString);

        /* Get the parser error if one occured and output it */
        if ( iSrchParserError != SRCH_NoError ) {

            /* Get the error text, default to an unknown error */
            if ( iSrchParserGetErrorText(iSrchParserError, pucErrorString, PRS_STRING_LENGTH + 1) != SRCH_NoError ) {
                s_strnncpy(pucErrorString, "Unknown parser error", PRS_STRING_LENGTH + 1);
            }
            
            fprintf(pppPrsParser->pfOutputFile, "Parser error: %d, error text: '%s'\n", iSrchParserError, pucErrorString);
        }


        /* Get the parser full normalized search text */
        if ( (iError = iSrchParserGetFullNormalizedSearchText(pppPrsParser->pvSrchParser, &pwcSrchParserNormalizedSearchText)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the parser full normalized search text, srch error: %d.", iError);
        }

        fprintf(pppPrsParser->pfOutputFile, "%ls", pwcSrchParserNormalizedSearchText);

    }

    /* Normalize */
    else if ( pppPrsParser->uiAction == PRS_ACTION_NORMALIZE ) {

        /* Output the text */
        fprintf(pppPrsParser->pfOutputFile, "%ls", pwcWideString);

    }


    /* Output new lines, two if the original text is output */
    fprintf(pppPrsParser->pfOutputFile, (pppPrsParser->bOutputOriginalText == true) ? "\n\n" : "\n");



    /* Bail label */
    bailFromvParseString:
    
    /* Free allocations */
    s_free(pucNormalizedString);
    s_free(pwcWideString);


    return;

}


/*---------------------------------------------------------------------------*/

