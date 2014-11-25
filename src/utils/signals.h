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

    Module:     signals.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    5 July 1998

    Purpose:    This is the header file for signals.c. This file contains all
                the public definitions for accessing the signals functions.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_SIGNALS_H)
#define UTL_SIGNALS_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

void vUtlSignalsHangUpHandler (int iSignal);
void vUtlSignalsNonFatalHandler (int iSignal);
void vUtlSignalsFatalHandler (int iSignal);
void vUtlSignalsChildHandler (int iSignal);


int iUtlSignalsInstallHangUpHandler (void (*vUtlSignalHandlerFunction)());
int iUtlSignalsInstallNonFatalHandler (void (*vUtlSignalHandlerFunction)());
int iUtlSignalsInstallFatalHandler (void (*vUtlSignalHandlerFunction)());
int iUtlSignalsInstallChildHandler (void (*vUtlSignalHandlerFunction)());


int iUtlSignalsIgnoreHandler (int iSignal, struct sigaction **ppsaSigAction);
int iUtlSignalsRestoreHandler (int iSignal, struct sigaction *psaSigAction);


void vUtlSignalsPassToChildProcesses (int iSignal);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_SIGNALS_H) */


/*---------------------------------------------------------------------------*/
