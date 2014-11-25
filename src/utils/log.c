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

    Module:     log.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 July 1995

    Purpose:    This file contains the log function.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Externals
*/
extern int    errno;


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.log"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Print the context */
/* #define UTL_LOG_ENABLE_CONTEXT_PRINTING */

/* Print the file name */
/* #define UTL_LOG_ENABLE_FILE_NAME_PRINTING */

/* Print the line number */
/* #define UTL_LOG_ENABLE_LINE_NUMBER_PRINTING */

/* Print the function name */
/* #define UTL_LOG_ENABLE_FUNCTION_NAME_PRINTING */


/* Print the system error with ERROR in addition to FATAL */
/* #define UTL_LOG_ENABLE_SYSTEM_ERROR_PRINTING_ON_ERROR */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Defaults */
#define UTL_LOG_FILE_PATH_DEFAULT           UTL_LOG_FILE_STDERR
#define UTL_LOG_LEVEL_DEFAULT               UTL_LOG_LEVEL_INFO
#define UTL_LOG_TYPE_DEFAULT                UTL_LOG_TYPE_PROCESS


/* Log level strings */
#define UTL_LOG_FATAL_LABEL                 (unsigned char *)"FATAL"
#define UTL_LOG_ERROR_LABEL                 (unsigned char *)"ERROR"
#define UTL_LOG_WARN_LABEL                  (unsigned char *)"WARN"
#define UTL_LOG_INFO_LABEL                  (unsigned char *)"INFO"
#define UTL_LOG_DEBUG_LABEL                 (unsigned char *)"DEBUG"


/* Log level to log label mapping, map array index to string */
static unsigned char *ppucLogLevelLabelsGlobal[] = {
    NULL,
    UTL_LOG_FATAL_LABEL,
    UTL_LOG_ERROR_LABEL,
    UTL_LOG_WARN_LABEL,
    UTL_LOG_INFO_LABEL,
    UTL_LOG_DEBUG_LABEL
};


/* Error string length for strerror_r() */
#define UTL_LOG_ERROR_STRING_LENGTH        (128)


/* Log contexts to list on DEBUG logging */
static unsigned char *ppucDebugLogContextsGlobal[] = {
/*     (unsigned char *)"com.fsconsult.mps.src.search.parser", */
/*     (unsigned char *)"com.fsconsult.mps.src.search.search", */
/*     (unsigned char *)"com.fsconsult.mps.src.search.termdict", */
/*     (unsigned char *)"com.fsconsult.mps.src.search.posting", */
/*     (unsigned char *)"com.fsconsult.mps.src.gateway.gateway", */
/*     (unsigned char *)"com.fsconsult.mps.src.utils.net", */
/*     (unsigned char *)"com.fsconsult.mps.src.utils.socket", */
    NULL
};



/*---------------------------------------------------------------------------*/


/*
** Globals
*/

static unsigned char    pucLogFilePathGlobal[UTL_FILE_PATH_MAX + 1] = {'\0'};

static unsigned int     uiLogLevelGlobal = UTL_LOG_LEVEL_DEFAULT;
static unsigned int     uiLogTypeGlobal = UTL_LOG_TYPE_DEFAULT;
static unsigned int     uiLogLineGlobal = 1;

static int              iHoursOffsetFromGmtGlobal = -1;

static boolean          bLogInitializedGlobal = false;


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLogInit()

    Purpose:    Initialize the log, setting default parameters

    Parameters: 
    
    Globals:    pucLogFilePathGlobal, uiLogLevelGlobal, 
                uiLogTypeGlobal, uiLogLineGlobal, bLogInitializedGlobal

    Returns:    UTL error code

*/
int iUtlLogInit
(

)
{

    time_t          tTime = s_time(NULL);
    unsigned int    uiDaylightSavingsTimeHoursOffset = 0;
    struct tm       tmTime;


    /* Set the globals to their default values */
    s_strnncpy(pucLogFilePathGlobal, UTL_LOG_FILE_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    uiLogLevelGlobal = UTL_LOG_LEVEL_INFO;
    uiLogTypeGlobal = UTL_LOG_TYPE_PROCESS;
    uiLogLineGlobal = 1;


    /* Get the local time */
    s_localtime_r(&tTime, &tmTime);
    
    /* Set the daylight savings time offset in hours */
    uiDaylightSavingsTimeHoursOffset = (tmTime.tm_isdst == 1) ? 1 : 0;
    
    /* Get the Coordinated Universal Time (UTC) */
    s_gmtime_r(&tTime, &tmTime);

    /* Get the global offset from GMT in hours */
    iHoursOffsetFromGmtGlobal = (s_difftime(tTime, s_mktime(&tmTime)) / 3600) + uiDaylightSavingsTimeHoursOffset;

    
    /* Logging has been initialized */
    bLogInitializedGlobal = true;

    
    return (UTL_NoError);

}



/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLogSetFilePath()

    Purpose:    Set the log file path

    Parameters: pucLogFilePath      log file path

    Globals:    pucLogFilePathGlobal, bLogInitializedGlobal

    Returns:    UTL error code

*/
int iUtlLogSetFilePath
(
    unsigned char *pucLogFilePath
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucLogFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLogFilePath' parameter passed to 'iUtlLogSetFilePath'."); 
        return (UTL_LogInvalidLogFilePath);
    }

    
    /* Make sure the log is initialized */
    if ( bLogInitializedGlobal == false ) {
        iUtlLogInit();
    }


    /* Test to see if the log file is writable if it is a file path */
    if ( (s_strcmp(pucLogFilePath, UTL_LOG_FILE_STDOUT) != 0) && (s_strcmp(pucLogFilePath, UTL_LOG_FILE_STDERR) != 0) ) {

        /* Check that we can write to the log file if it is already there, otherwise create the file */
        if ( (bUtlFilePathExists(pucLogFilePath) == true) && (bUtlFileIsFile(pucLogFilePath) == true) ) {

            /* Check that we can write to the log file */
            if ( bUtlFilePathWrite(pucLogFilePath) == false ) {
                return (UTL_LogFileWriteDenied);
            }
        }
        else {
        
            FILE    *pfLogStream = NULL;

            /* Create the file */
            if ( (pfLogStream = s_fopen(pucLogFilePath, "a")) == NULL ) {
                return (UTL_LogFileWriteDenied);
            }

            /* Close the file */
            s_fclose(pfLogStream);
        }
    }

    /* Copy the log file path to the global */
    s_strnncpy(pucLogFilePathGlobal, pucLogFilePath, UTL_FILE_PATH_MAX + 1);

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLogSetOwnerName()

    Purpose:    Set the log owner name

    Parameters: pucLogFileOwnerName     log file owner name

    Globals:    bLogInitializedGlobal

    Returns:    UTL error code

*/
int iUtlLogSetOwnerName
(
    unsigned char *pucLogFileOwnerName
)
{


    struct passwd   pwPasswd;
    unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1];
    struct passwd   *ppwPasswdPtr = NULL;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucLogFileOwnerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLogFileOwnerName' parameter passed to 'iUtlLogSetOwnerName'."); 
        return (UTL_LogInvalidLogOwnerName);
    }

    
    /* Make sure the log is initialized */
    if ( bLogInitializedGlobal == false ) {
        iUtlLogInit();
    }


    /* Dont bother if we are not logging to a file */ 
    if ( (s_strcmp(pucLogFilePathGlobal, UTL_LOG_FILE_STDOUT) != 0) && (s_strcmp(pucLogFilePathGlobal, UTL_LOG_FILE_STDERR) != 0) ) {

        /* Check the user ID, cant modify the ownership if we are not root */
        if ( s_getuid() != 0 ) {
            return (UTL_LogFileNotRoot);
        }

        /* Get the password entry for this user */
        if ( s_getpwnam_r(pucLogFileOwnerName, &pwPasswd, pucBuffer, UTL_FILE_PATH_MAX + 1, &ppwPasswdPtr) == -1 ) {
            return (UTL_LogInvalidUser);
        }

        /* Check the password entry for this user */
        if ( ppwPasswdPtr == NULL ) {
            return (UTL_LogInvalidUser);
        }

        /* Modify the user and group ownership of the log file */
        if ( s_chown(pucLogFilePathGlobal, ppwPasswdPtr->pw_uid, ppwPasswdPtr->pw_gid) < 0 ) {
            return (UTL_LogFailedToSetOwnership);
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLogSetLevel()

    Purpose:    Set the log level

    Parameters: uiLogLevel      log level

    Globals:    uiLogLevelGlobal, bLogInitializedGlobal

    Returns:    UTL error code

*/
int iUtlLogSetLevel
(
    unsigned int uiLogLevel
)
{

    /* Make sure the log is initialized */
    if ( bLogInitializedGlobal == false ) {
        iUtlLogInit();
    }


    /* Check that the log level makes sense, default to INFO */
    if ( UTL_LOG_LEVEL_VALID(uiLogLevel) == true ) {

        /* Set the log level */
        uiLogLevelGlobal = uiLogLevel;
    }
    else {

        /* Set the default log level */
        uiLogLevelGlobal = UTL_LOG_LEVEL_DEFAULT;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLogSetType()

    Purpose:    Set the log type

    Parameters: uiLogType       log type

    Globals:    uiLogTypeGlobal, bLogInitializedGlobal

    Returns:    UTL error code

*/
int iUtlLogSetType
(
    unsigned int uiLogType
)
{
    
    /* Make sure the log is initialized */
    if ( bLogInitializedGlobal == false ) {
        iUtlLogInit();
    }


    /* Check that the log type makes sense, default to PROCESS */
    if ( UTL_LOG_TYPE_VALID(uiLogType) == true ) {

        /* Set the log type */
        uiLogTypeGlobal = uiLogType;
    }
    else {

        /* Set the default log type */
        uiLogTypeGlobal = UTL_LOG_TYPE_DEFAULT;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLogSetLine()

    Purpose:    Set the log line

    Parameters: uiLogLine       log line

    Globals:    uiLogLineGlobal, bLogInitializedGlobal

    Returns:    UTL error code

*/
int iUtlLogSetLine
(
    unsigned int uiLogLine
)
{
    
    /* Make sure the log is initialized */
    if ( bLogInitializedGlobal == false ) {
        iUtlLogInit();
    }


    /* Set the log line */
    uiLogLineGlobal = uiLogLine;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlLogIsFatal()

    Purpose:    Return true if the FATAL level is currently active

    Parameters: pucContext      Context string

    Globals:    uiLogLevelGlobal

    Returns:    true or false

*/
boolean bUtlLogIsFatal
(
    unsigned char *pucContext
)
{

    /* Return true if the FATAL level is currently active */
    return ((uiLogLevelGlobal >= UTL_LOG_LEVEL_FATAL) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlLogIsError()

    Purpose:    Return true if the ERROR level is currently active

    Parameters: pucContext      Context string

    Globals:    uiLogLevelGlobal

    Returns:    true or false

*/
boolean bUtlLogIsError
(
    unsigned char *pucContext
)
{

    /* Return true if the ERROR level is currently active */
    return ((uiLogLevelGlobal >= UTL_LOG_LEVEL_ERROR) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlLogIsWarn()

    Purpose:    Return true if the WARN level is currently active

    Parameters: pucContext      Context string

    Globals:    uiLogLevelGlobal

    Returns:    true or false

*/
boolean bUtlLogIsWarn
(
    unsigned char *pucContext
)
{

    /* Return true if the WARN level is currently active */
    return ((uiLogLevelGlobal >= UTL_LOG_LEVEL_WARN) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlLogIsInfo()

    Purpose:    Return true if the INFO level is currently active

    Parameters: pucContext      Context string

    Globals:    uiLogLevelGlobal

    Returns:    true or false

*/
boolean bUtlLogIsInfo
(
    unsigned char *pucContext
)
{

    /* Return true if the INFO level is currently active */
    return ((uiLogLevelGlobal >= UTL_LOG_LEVEL_INFO) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bUtlLogIsDebug()

    Purpose:    Return true if the DEBUG level is currently active

    Parameters: pucContext      Context string

    Globals:    uiLogLevelGlobal

    Returns:    true or false

*/
boolean bUtlLogIsDebug
(
    unsigned char *pucContext
)
{

    unsigned int    uiI = 0;


    /* Return false if the DEBUG level is not currently active */
    if ( uiLogLevelGlobal < UTL_LOG_LEVEL_DEBUG ) {
        return (false);
    }


    /* Return false if no context was specified */
    if ( bUtlStringsIsStringNULL(pucContext) == true ) {
        return (false);
    }

    
    /* Return true if we matched on a context we are currently tracking */
    for ( uiI = 0; ppucDebugLogContextsGlobal[uiI] != NULL; uiI++ ) {
        if ( s_strcmp(pucContext, ppucDebugLogContextsGlobal[uiI]) == 0 ) {
            return (true);
        }
    }


    /* Return false because there was no match on any context */
    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlLogMessage()

    Purpose:    Log the message
                
    Parameters: uiLogLevel      If uiLogLevel >= uiLogLevelGlobal, output this
                pucContext      Context string
                pucFile         File name
                uiLine          Line number
                pucFunction     Function name
                vExit           Exit flag
                pucMessage      Message
                ...             args (optional)

    Globals:    pucLogFilePathGlobal, uiLogLevelGlobal, 
                        uiLogLineGlobal, uiLogTypeGlobal,
                        ppucLogLevelLabelsGlobal

    Returns:    UTL error code

*/
int iUtlLogMessage
(
    unsigned int uiLogLevel,
    unsigned char *pucContext,
    unsigned char *pucFile,
    unsigned int uiLine,
    unsigned char *pucFunction,
    boolean bExit,
    unsigned char *pucMessage,
    ...
)
{

    va_list         ap;
    FILE            *pfLogStream = NULL;
    unsigned char   pucDateString[UTL_FILE_PATH_MAX + 1];

    /* Save the system error number in case it gets side-effected before we use it, we restore it later */
    int                iError = errno;


    /* Make sure the log is initialized */
    if ( bLogInitializedGlobal == false ) {
        iUtlLogInit();
    }


    /* Only log if the priority level is high enough */
    if ( uiLogLevel > uiLogLevelGlobal ) {
        return (UTL_NoError);
    }


    /* Check that we are currently tracking the passed context if this is a DEBUG log entry */
    if ( uiLogLevel == UTL_LOG_LEVEL_DEBUG ) {

        unsigned int    uiI = 0;
        boolean         bMatch = false;

        /* Return if no context was specified */
        if ( bUtlStringsIsStringNULL(pucContext) == true ) {
            return (UTL_NoError);
        }
    
        /* See if we have a match on a context we are currently tracking */
        for ( uiI = 0, bMatch = false; ppucDebugLogContextsGlobal[uiI] != NULL; uiI++ ) {
            if ( s_strcmp(pucContext, ppucDebugLogContextsGlobal[uiI]) == 0 ) {
                bMatch = true;
                break;
            }
        }
        
        /* No match so we return */
        if ( bMatch == false ) {
            return (UTL_NoError);
        }
    }


    /* Create the date/time string */
    {
        time_t      tTime = s_time(NULL);
        struct tm   tmTime;

        /* Get the local time */
        s_localtime_r(&tTime, &tmTime);
        
        /* Create the date string */
        s_strftime(pucDateString, UTL_FILE_PATH_MAX, "%d/%b/%Y:%H:%M:%S", &tmTime);
    }        


    /* Set the log stream */
    if ( s_strcmp(pucLogFilePathGlobal, UTL_LOG_FILE_STDOUT) == 0 ) {
        /* Set to 'stdout' */
        pfLogStream = stdout;
    }
    else if ( s_strcmp(pucLogFilePathGlobal, UTL_LOG_FILE_STDERR) == 0 ) {
        /* Set to 'stderr' */
        pfLogStream = stderr;
    }
    else {
        /* Open the log file, switch to stderr if we fail and let the user know, note that we don't call the s_ functions to prevent circular errors */
        if ( (pfLogStream = fopen(pucLogFilePathGlobal, "a")) == NULL ) {

            /* Set the log stream to stderr */
            pfLogStream = stderr;

            /* Add extra stuff if we are doing a full log */
            if ( (uiLogTypeGlobal == UTL_LOG_TYPE_PROCESS) || (uiLogTypeGlobal == UTL_LOG_TYPE_FULL) ) {
                fprintf(pfLogStream, "%d: ", (int)getpid());
            }
            if ( (uiLogTypeGlobal == UTL_LOG_TYPE_THREAD) || (uiLogTypeGlobal == UTL_LOG_TYPE_FULL) ) {
                fprintf(pfLogStream, "%lu: ", (unsigned long)s_pthread_self());
            }
            if ( (uiLogTypeGlobal == UTL_LOG_TYPE_PROCESS) || (uiLogTypeGlobal == UTL_LOG_TYPE_THREAD) || (uiLogTypeGlobal == UTL_LOG_TYPE_FULL) ) {
                fprintf(pfLogStream, "%u: %s %s%02d00: %s: ", uiLogLineGlobal++, pucDateString, ((iHoursOffsetFromGmtGlobal > 0) ? "+" : (iHoursOffsetFromGmtGlobal < 0) ? "-" : ""), 
                        (int)abs(iHoursOffsetFromGmtGlobal), ppucLogLevelLabelsGlobal[UTL_LOG_LEVEL_WARN]);
            }
            
            /* Let the user know */
            fprintf(pfLogStream, "Failed to open: '%s' for logging, now logging to '%s'.\n", pucLogFilePathGlobal, UTL_LOG_FILE_PATH_DEFAULT);
            
            /* Set the log file path global to the default */
            strcpy(pucLogFilePathGlobal, UTL_LOG_FILE_PATH_DEFAULT);
        }
    }


    /* Add extra stuff if we are doing a full log */
    if ( (uiLogTypeGlobal == UTL_LOG_TYPE_PROCESS) || (uiLogTypeGlobal == UTL_LOG_TYPE_FULL) ) {
        fprintf(pfLogStream, "%d: ", (int)getpid());
    }
    if ( (uiLogTypeGlobal == UTL_LOG_TYPE_THREAD) || (uiLogTypeGlobal == UTL_LOG_TYPE_FULL) ) {
        fprintf(pfLogStream, "%lu: ", (unsigned long)s_pthread_self());
    }
    if ( (uiLogTypeGlobal == UTL_LOG_TYPE_PROCESS) || (uiLogTypeGlobal == UTL_LOG_TYPE_THREAD) || (uiLogTypeGlobal == UTL_LOG_TYPE_FULL) ) {
        fprintf(pfLogStream, "%u: %s %s%02d00: %s: ", uiLogLineGlobal++, pucDateString, ((iHoursOffsetFromGmtGlobal > 0) ? "+" : (iHoursOffsetFromGmtGlobal < 0) ? "-" : ""), 
                (int)abs(iHoursOffsetFromGmtGlobal), ppucLogLevelLabelsGlobal[uiLogLevel]);
    }


    /* Context and location information */
#if defined(UTL_LOG_ENABLE_CONTEXT_PRINTING)
    fprintf(pfLogStream, "%s: ", pucContext);
#endif    /* defined(UTL_LOG_ENABLE_CONTEXT_PRINTING) */

#if defined(UTL_LOG_ENABLE_FILE_NAME_PRINTING)
    fprintf(pfLogStream, "%s: ", pucFile);
#endif    /* defined(UTL_LOG_ENABLE_FILE_NAME_PRINTING) */

#if defined(UTL_LOG_ENABLE_LINE_NUMBER_PRINTING)
    fprintf(pfLogStream, "%u: ", uiLine);
#endif    /* defined(UTL_LOG_ENABLE_LINE_NUMBER_PRINTING) */

#if defined(UTL_LOG_ENABLE_FUNCTION_NAME_PRINTING)
    fprintf(pfLogStream, "%s: ", pucFunction);
#endif    /* defined(UTL_LOG_ENABLE_FUNCTION_NAME_PRINTING) */


    /* Print the message with its parameters */
    if ( bUtlStringsIsStringNULL(pucMessage) == false ) {
        va_start(ap, pucMessage);
        vfprintf(pfLogStream, pucMessage, ap);
        va_end(ap);
    }

    
    /* Print the system error message if there is one */
    if ( (iError != EOK ) && ((uiLogLevel == UTL_LOG_LEVEL_FATAL) 
#if defined(UTL_LOG_ENABLE_SYSTEM_ERROR_PRINTING_ON_ERROR)
        || (uiLogLevel == UTL_LOG_LEVEL_ERROR) 
#endif    /* defined(UTL_LOG_ENABLE_SYSTEM_ERROR_PRINTING_ON_ERROR) */
    ) ) {

        unsigned char   *pucErrorStringPtr = NULL;
        unsigned char   pucBuffer[UTL_LOG_ERROR_STRING_LENGTH + 1] = {'\0'};

        /* Get the error string */
        pucErrorStringPtr = strerror_r(iError, pucBuffer, UTL_LOG_ERROR_STRING_LENGTH);

        /* Log the error string */
        fprintf(pfLogStream, ", System error: '%s'.", pucErrorStringPtr);
    }

    
    /* New line */
    fprintf(pfLogStream, "\n");


    /* Flush/close the log stream as needed, note that we don't call the s_ functions to prevent circular errors */
    fflush(pfLogStream);
    if ( (s_strcmp(pucLogFilePathGlobal, UTL_LOG_FILE_STDOUT) != 0) && (s_strcmp(pucLogFilePathGlobal, UTL_LOG_FILE_STDERR) != 0) ) {
        fclose(pfLogStream);
    }


    /* Exit if requested */
    if ( bExit == true ) {

/* Abort if DEBUG is switched on so that we get a core dump */
#if defined(DEBUG)
        s_abort();
#else
        s_exit(EXIT_FAILURE);
#endif    /* defined(DEBUG) */

    }


    /* Restore the system error number */
    errno = iError;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/

