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

    Module:     srvr_http.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    15 May 2006

    Purpose:    This module implements the 'http' protocol handler.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRVR_HTTP_H)
#define SRVR_HTTP_H


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
#define SRVR_HTTP_PROTOCOL_NAME             (unsigned char *)"HTTP"


/* Protocol header byte */
#define SRVR_HTTP_PROTOCOL_HEADER_BYTE      (unsigned char)'G'          /* HTTP, 'G' for 'GET' method */


/*---------------------------------------------------------------------------*/


/*
** Public function prototype
*/

int iSrvrProtocolHandlerHttp (struct srvrServerSession *psssSrvrServerSession);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRVR_HTTP_H) */


/*---------------------------------------------------------------------------*/



