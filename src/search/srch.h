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

    Module:     srch.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    3 September 2001

    Purpose:    This is the include file for the search engine

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_H)
#define SRCH_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/


/* Pull the in the SPI utilities, and the server SPI includes */
#include "utils.h"
#include "spi.h"


/* Pull in the language */
#include "lng.h"


/* Pull in the error codes */
#include "srch_err.h"


/* Pull in the search report include */
#include "rep.h"


/* Pull in the rest of the thundering herd, the order matter */

#include "index.h"
#include "search.h"
#include "shortrslt.h"
#include "weight.h"
#include "bitmap.h"
#include "posting.h"
#include "parser.h"

#include "cache.h"
#include "document.h"
#include "feedback.h"
#include "filepaths.h"
#include "filter.h"
#include "indexer.h"
#include "info.h"
#include "invert.h"
#include "keydict.h"
#include "language.h"
#include "report.h"
#include "retrieval.h"
#include "srchconf.h"
#include "stemmer.h"
#include "stoplist.h"
#include "termdict.h"
#include "termlen.h"
#include "termsrch.h"
#include "version.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_H) */


/*---------------------------------------------------------------------------*/
