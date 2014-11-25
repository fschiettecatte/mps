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

    Module:     signals.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    6 July 1998

    Purpose:    Signal handling wrappers.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.signals"


/*---------------------------------------------------------------------------*/


/*

    Function:   vUtlSignalsHangUpHandler()

    Purpose:    To handle hang up signals. Currently we handle the 
                following signals:
                    SIGHUP

    Parameters: iSignal     signal

    Globals:    none

    Returns:    void

*/
void vUtlSignalsHangUpHandler
(
    int iSignal
)
{

    int     iStatus = 0;
    pid_t   zPid = 0;


    /* Pass on the signal to any kids */
    vUtlSignalsPassToChildProcesses(iSignal);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUtlSignalsNonFatalHandler()

    Purpose:    To handle non-fatal signals. Currently we handle the 
                following signals:

                    SIGINT
                    SIGKILL
                    SIGPIPE
                    SIGALRM
                    SIGTERM
                    SIGSTOP

    Parameters: iSignal     signal number

    Globals:    none

    Returns:    void

*/
void vUtlSignalsNonFatalHandler
(
    int iSignal
)
{

    int     iStatus = 0;
    pid_t   zPid = 0;


    /* Pass on the signal to any kids */
    vUtlSignalsPassToChildProcesses(iSignal);


    /*  Log the event */
    switch ( iSignal ) {
#if defined(SIGINT)
        case SIGINT:
            iUtlLogWarn(UTL_LOG_CONTEXT, "Received an interrupt from keyboard signal.");
            break;
#endif    /* defined(SIGINT) */

#if defined(SIGKILL)
        case SIGKILL:
            iUtlLogWarn(UTL_LOG_CONTEXT, "Received a kill signal.");
            break;
#endif    /* defined(SIGKILL) */

#if defined(SIGPIPE)
        case SIGPIPE:
            iUtlLogWarn(UTL_LOG_CONTEXT, "Received broken pipe signal.");
            break;
#endif    /* defined(SIGPIPE) */

#if defined(SIGALRM)
        case SIGALRM:
            iUtlLogWarn(UTL_LOG_CONTEXT, "Received an alarm signal.");
            break;
#endif    /* defined(SIGALRM) */

#if defined(SIGTERM)
        case SIGTERM:
            iUtlLogWarn(UTL_LOG_CONTEXT, "Received a termination signal.");
            break;
#endif    /* defined(SIGTERM) */

#if defined(SIGSTOP)
        case SIGSTOP:
            iUtlLogWarn(UTL_LOG_CONTEXT, "Received a stop signal.");
            break;
#endif    /* defined(SIGSTOP) */

        default:
            iUtlLogWarn(UTL_LOG_CONTEXT, "Received an unknown signal: %d.", iSignal);
    }


    /* Make sure we bail */
    s_exit(EXIT_SUCCESS);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUtlSignalsFatalHandler()

    Purpose:    To handle fatal signals. Currently we handle the 
                following signals:

                    SIGQUIT
                    SIGILL
                    SIGTRAP
                    SIGABRT
                    SIGEMT
                    SIGFPE
                    SIGBUS
                    SIGSEGV
                    SIGSYS
                    SIGXCPU
                    SIGXFSZ

    Parameters: iSignal     signal number

    Globals:    none

    Returns:    void

*/
void vUtlSignalsFatalHandler
(
    int iSignal
)
{

    int     iStatus = 0;
    pid_t   zPid = 0;


    /* Pass on the signal to any kids */
    vUtlSignalsPassToChildProcesses(iSignal);


    /* Log the event */
    switch ( iSignal ) {
#if defined(SIGQUIT)
        case SIGQUIT:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received a quit from keyboard signal");
#endif    /* defined(SIGQUIT) */

#if defined(SIGILL)
        case SIGILL:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received an illegal instruction signal");
#endif    /* defined(SIGILL) */

#if defined(SIGTRAP)
        case SIGTRAP:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received a trace/breakpoint trap signal");
#endif    /* defined(SIGTRAP) */

#if defined(SIGABRT)
        case SIGABRT:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received an abort signal");
#endif    /* defined(SIGABRT) */

#if defined(SIGEMT)
        case SIGEMT:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received an emulation trap signal");
#endif    /* defined(SIGEMT) */

#if defined(SIGFPE)
        case SIGFPE:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received a floating point exception signal");
#endif    /* defined(SIGFPE) */

#if defined(SIGBUS)
        case SIGBUS:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received a bus error signal");
#endif    /* defined(SIGBUS) */

#if defined(SIGSEGV)
        case SIGSEGV:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received an invalid memory reference signal");
#endif    /* defined(SIGSEGV) */

#if defined(SIGSYS)
        case SIGSYS:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received a bad system call signal");
#endif    /* defined(SIGSYS) */

#if defined(SIGXCPU)
        case SIGXCPU:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received a CPU time limit exceeded signal");
#endif    /* defined(SIGXCPU) */

#if defined(SIGXFSZ)
        case SIGXFSZ:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received a file size limit exceeded signal");
#endif    /* defined(SIGXFSZ) */

        default:
            iUtlLogPanic(UTL_LOG_CONTEXT, "Received an unknown signal: %d", iSignal);
    }


    /* Make sure we bail */
    s_exit(EXIT_FAILURE);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUtlSignalsChildHandler()

    Purpose:    To handle child signals.

    Parameters: iSignal     signal number

    Globals:    none

    Returns:    void

*/
void vUtlSignalsChildHandler
(
    int iSignal
)
{

    int     iStatus = 0;
    pid_t   zPid = 0;


    /* Give the kid a decent burial */
    while ( (zPid = s_waitpid(-1, &iStatus, WNOHANG)) > 0 ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, " caught child: %d.", (int)zPid); */
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSignalsInstallNonFatalHandler()

    Purpose:    To install handlers for non-fatal signals. Currently we handle 
                the following signals:

                    SIGINT
                    SIGKILL
                    SIGPIPE
                    SIGALRM
                    SIGTERM
                    SIGSTOP

    Parameters: vUtlSignalHandlerFunction   signal handler function 
                                            (can also be SIG_IGN or SIG_DFL)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSignalsInstallNonFatalHandler
(
    void (*vUtlSignalHandlerFunction)()
)
{

    int                 iError = UTL_NoError;
    struct sigaction    saSigAction;
    sigset_t            ssSigSet;


    /* Clear the structures */
    s_memset(&ssSigSet, 0, sizeof(sigset_t));
    s_memset(&saSigAction, 0, sizeof(struct sigaction));

    /* Set the structure fields */
    saSigAction.sa_handler = vUtlSignalHandlerFunction;
    saSigAction.sa_mask = ssSigSet;
    saSigAction.sa_flags = SA_RESTART;


#if defined(SIGINT)
    if ( s_sigaction(SIGINT, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGINT) */

#if defined(NOT_SUPPORTED_UNDER_POSIX)
    if ( s_sigaction(SIGKILL, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(NOT_SUPPORTED_UNDER_POSIX) */

#if defined(SIGPIPE)
    if ( s_sigaction(SIGPIPE, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGPIPE) */

#if defined(SIGALRM)
    if ( s_sigaction(SIGALRM, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGALRM) */

#if defined(SIGTERM)
    if ( s_sigaction(SIGTERM, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGTERM) */

#if defined(NOT_SUPPORTED_UNDER_POSIX)
    if ( s_sigaction(SIGSTOP, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(NOT_SUPPORTED_UNDER_POSIX) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSignalsInstallFatalHandler()

    Purpose:    To install handlers for fatal signals. Currently we handle 
                the following signals:

                    SIGQUIT
                    SIGILL
                    SIGTRAP
                    SIGABRT
                    SIGEMT
                    SIGFPE
                    SIGBUS
                    SIGSEGV
                    SIGSYS
                    SIGXCPU
                    SIGXFSZ

    Parameters: vUtlSignalHandlerFunction   signal handler function, can also be 
                                            SIG_IGN or SIG_DFL  (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSignalsInstallFatalHandler
(
    void (*vUtlSignalHandlerFunction)()
)
{

    int                 iError = UTL_NoError;
    struct sigaction    saSigAction;
    sigset_t            ssSigSet;


    /* Clear the structures */
    s_memset(&ssSigSet, 0, sizeof(sigset_t));
    s_memset(&saSigAction, 0, sizeof(struct sigaction));

    /* Set the structure fields */
    saSigAction.sa_handler = vUtlSignalHandlerFunction;
    saSigAction.sa_mask = ssSigSet;
    saSigAction.sa_flags = SA_RESTART;


#if defined(SIGQUIT)
    if ( s_sigaction(SIGQUIT, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGQUIT) */

#if defined(SIGILL)
    if ( s_sigaction(SIGILL, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGILL) */

#if defined(SIGTRAP)
    if ( s_sigaction(SIGTRAP, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGTRAP) */

#if defined(SIGABRT)
    /* Commented out since we need to core out when we get an abort signal */
/*     if ( s_sigaction(SIGABRT, &saSigAction, NULL) != 0 ) { */
/*         iError |= UTL_SignalActionInstallFailed; */
/*     } */
#endif    /* defined(SIGABRT) */

#if defined(SIGEMT)
    if ( s_sigaction(SIGEMT, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGEMT) */

#if defined(SIGFPE)
    if ( s_sigaction(SIGFPE, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGFPE) */

#if defined(SIGBUS)
    if ( s_sigaction(SIGBUS, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGBUS) */

#if defined(SIGSEGV)
    if ( s_sigaction(SIGSEGV, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGSEGV) */

#if defined(SIGSYS)
    if ( s_sigaction(SIGSYS, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGSYS) */

#if defined(SIGXCPU)
    if ( s_sigaction(SIGXCPU, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGXCPU) */

#if defined(SIGXFSZ)
    if ( s_sigaction(SIGXFSZ, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGXFSZ) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSignalsInstallChildHandler()

    Purpose:    To install a handler for children. Currently we handle the 
                following signals:
                    SIGCHLD

    Parameters: vUtlSignalHandlerFunction   signal handler function, can also be 
                                            SIG_IGN or SIG_DFL  (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSignalsInstallChildHandler
(
    void (*vUtlSignalHandlerFunction)()
)
{

    int                 iError = UTL_NoError;
    struct sigaction    saSigAction;
    sigset_t            ssSigSet;


    /* Clear the structures */
    s_memset(&ssSigSet, 0, sizeof(sigset_t));
    s_memset(&saSigAction, 0, sizeof(struct sigaction));

    /* Set the structure fields */
    saSigAction.sa_handler = vUtlSignalHandlerFunction;
    saSigAction.sa_mask = ssSigSet;
    saSigAction.sa_flags = SA_RESTART;


#if defined(SIGCHLD)
    if ( s_sigaction(SIGCHLD, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGCHLD) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSignalsInstallHangUpHandler()

    Purpose:    To install a handler for hang-ups. Currently we handle the 
                following signals:
                    SIGHUP

    Parameters: vUtlSignalHandlerFunction   signal handler function, can also be 
                                            SIG_IGN or SIG_DFL  (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSignalsInstallHangUpHandler
(
    void (*vUtlSignalHandlerFunction)()
)
{

    int                 iError = UTL_NoError;
    struct sigaction    saSigAction;
    sigset_t            ssSigSet;


    /* Clear the structures */
    s_memset(&ssSigSet, 0, sizeof(sigset_t));
    s_memset(&saSigAction, 0, sizeof(struct sigaction));

    /* Set the structure fields */
    saSigAction.sa_handler = vUtlSignalHandlerFunction;
    saSigAction.sa_mask = ssSigSet;
    saSigAction.sa_flags = SA_RESTART;


#if defined(SIGHUP)
    if ( s_sigaction(SIGHUP, &saSigAction, NULL) != 0 ) {
        iError |= UTL_SignalActionInstallFailed;
    }
#endif    /* defined(SIGHUP) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSignalsIgnoreHandler()

    Purpose:    To set the signal handler to 'ignore', and optionally 
                recover the current signal handler

    Parameters: iSignal         signal
                ppsaSigAction   return pointer for the current 
                                signal action (optional)
                                            

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSignalsIgnoreHandler
(
    int iSignal,
    struct sigaction **ppsaSigAction
)
{

    int                 iError = UTL_NoError;
    struct sigaction    *psaSigAction = NULL;
    struct sigaction    saSigAction;
    sigset_t            ssSigSet;


    /* Clear the structures */
    s_memset(&ssSigSet, 0, sizeof(sigset_t));
    s_memset(&saSigAction, 0, sizeof(struct sigaction));

    /* Set the structure fields */
    saSigAction.sa_handler = SIG_IGN;
    saSigAction.sa_mask = ssSigSet;
    saSigAction.sa_flags = SA_RESTART;


    /* Allocate space for the signal action structure if the return pointer is passed */
    if ( ppsaSigAction != NULL ) {
        if ( (psaSigAction = (struct sigaction *)s_malloc(sizeof(struct sigaction))) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiSignalsIgnoreHandler;
        }
    }

    /* Install the signal action */
    if ( s_sigaction(iSignal, &saSigAction, psaSigAction) != 0 ) {
        iError = UTL_SignalActionInstallFailed;
        goto bailFromiSignalsIgnoreHandler;
    }



    /* Bail label */
    bailFromiSignalsIgnoreHandler:
    
    
    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer if it was passed */
        if ( ppsaSigAction != NULL ) {
            *ppsaSigAction = psaSigAction; 
        }
    }
    else {
        
        /* Free allocations */
        s_free(psaSigAction);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSignalsRestoreHandler()

    Purpose:    To restore a signal handler

    Parameters: iSignal         signal
                psaSigAction    signal action (optional)
                                            

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSignalsRestoreHandler
(
    int iSignal,
    struct sigaction *psaSigAction
)
{

    int     iError = UTL_NoError;


    /* Install the signal action */
    if ( s_sigaction(iSignal, psaSigAction, NULL) != 0 ) {
        iError = UTL_SignalActionInstallFailed;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUtlSignalsPassToChildProcesses()

    Purpose:    To pass the signal to any child processes

    Parameters: iSignal     signal

    Globals:    none

    Returns:    void

*/
void vUtlSignalsPassToChildProcesses
(
    int iSignal
)
{

    int     iStatus = 0;
    pid_t   zPid = 0;


    /* Pass on the signal to any kids */
    if ( (zPid = s_waitpid(-1, &iStatus, WNOHANG)) == 0 ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "s_kill(0, %ld) [%ld][%ld][%ld].", iSignal, s_getpid(), s_getpgrp(), zPid); */
        s_kill(0, iSignal);
    }
/*     else { */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "zPid [%ld].", zPid); */
/*     } */


    return;

}


/*---------------------------------------------------------------------------*/


