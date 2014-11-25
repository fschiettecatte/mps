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

    Module:     server.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    3 August 2006

    Purpose:    This is the header file for server.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRVR_SERVER_H)
#define SRVR_SERVER_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "lng.h"

#include "spi.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Server session structure */
struct srvrServerSession {

    struct spiSession   *pssSpiSession;                     /* SPI session structure */

    void                *pvUtlNet;                          /* UTL net structure */
    unsigned int        uiTimeOut;                          /* Timeout */

    unsigned int        uiSessionCount;                     /* Session count */
    boolean             bActiveSession;                     /* Active session flag */

    double              dLoadMaximum;                       /* Load maximum */
    double              dConnectionLoadMaximum;             /* Connection load maximum */
    double              dSearchLoadMaximum;                 /* Search load maximum */
    double              dRetrievalLoadMaximum;              /* Retrieval load maximum */
    double              dInformationLoadMaximum;            /* Information load maximum */

};


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRVR_SERVER_H) */


/*---------------------------------------------------------------------------*/
