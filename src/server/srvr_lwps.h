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

    Module:     srvr_lwps.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 December 1995

    Purpose:    This module implements the 'lwps' protocol handler.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRVR_LWPS_H)
#define SRVR_LWPS_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "spi.h"

#include "server.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Public defines
*/

/* Protocol Name */
#define SRVR_LWPS_PROTOCOL_NAME             (unsigned char *)"LWPS"


/* Protocol header byte */
#define SRVR_LWPS_PROTOCOL_HEADER_BYTE      (unsigned char)'L'          /* LWPS */


/*---------------------------------------------------------------------------*/


/*
** Public function prototype
*/

int iSrvrProtocolHandlerLwps (struct srvrServerSession *psssSrvrServerSession);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRVR_LWPS_H) */


/*---------------------------------------------------------------------------*/



