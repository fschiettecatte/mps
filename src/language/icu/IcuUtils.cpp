//****************************************************************************
//       Copyright (C) 1993-2011, FS Consulting LLC. All rights reserved     *
//                                                                           *
// This notice is intended as a precaution against inadvertent publication   *
//  and does not constitute an admission or acknowledgement that publication *
//  has occurred or constitute a waiver of confidentiality.                  *
//                                                                           *
//  This software is the proprietary and confidential property               *
//  of FS Consulting LLC.                                                    *
//****************************************************************************


//
//
//  Module:     IcuUtils.cpp
//
//  Author:	    Francois Schiettecatte (FS Consulting LLC.)
//
//  Creation    Date:	2 April 2010
//
//  Purpose:	This module provides a centralized point where we can 
//				define wrapper functions for the functionalities provided 
//				by the MPS framework or other third-party libraries 
//				(e.g. logging, file system operations, etc.). Doing so 
//				limits the amount of the ICU code that is 
//				dependent on the MPS code and on third-party libraries.
//
//


//---------------------------------------------------------------------------


//
// Includes
//

#include "utils.h"

#include "IcuErr.h"
#include "IcuUtils.h"


//---------------------------------------------------------------------------


//
// Feature defines
//

// Enable debugging output
// #define ICU_NORMALIZER_DEBUG

// Context for logging
static char ICU_LOG_CONTEXT[] = "com.fsconsult.mps.src.language.icu.IcuUtils";


//---------------------------------------------------------------------------


//
//
//	Function:	iIcuUtlLogFatal
//
//	Purpose:	This function logs a fatal message
//
//	Parameters:	pcContext       Context
//				pcMessage		Message to log
//				...				args (optional)
//
// 	Globals:	none
//
//	Returns:	void
//
//
int iIcuUtlLogFatal
(
	const char *pcContext,
	const char *pcMessage,
	...
)
{

	va_list     ap;


	va_start(ap, pcMessage);
	iUtlLogFatal((unsigned char *)pcContext, (unsigned char *)pcMessage, ap);
	va_end(ap);


	return (ICU_NoError);

}


//---------------------------------------------------------------------------


//
//
//	Function:	iIcuUtlLogError
//
//	Purpose:	This function logs an error message
//
//	Parameters:	pcContext		Context
//				pcMessage		Message to log
//				...				args (optional)
//
// 	Globals:	none
//
//	Returns:	void
//
//
int iIcuUtlLogError
(
	const char *pcContext,
	const char *pcMessage,
	...
)
{

	va_list     ap;


	va_start(ap, pcMessage);
	iUtlLogError((unsigned char *)pcContext, (unsigned char *)pcMessage, ap);
	va_end(ap);


	return (ICU_NoError);

}


//---------------------------------------------------------------------------


//
//
//	Function:	iIcuUtlLogWarn
//
//	Purpose:	This function logs a warning message
//
//	Parameters:	pcContext		Context
//				pcMessage		Message to log
//				...				args (optional)
//
// 	Globals:	none
//
//	Returns:	void
//
//
int iIcuUtlLogWarn
(
	const char *pcContext,
	const char *pcMessage,
	...
)
{

	va_list     ap;


	va_start(ap, pcMessage);
	iUtlLogWarn((unsigned char *)pcContext, (unsigned char *)pcMessage, ap);
	va_end(ap);


	return (ICU_NoError);

}


//---------------------------------------------------------------------------


//
//
//	Function:	iIcuUtlLogInfo
//
//	Purpose:	This function logs an info message
//
//	Parameters:	pcContext		Context
//				pcMessage		Message to log
//				...				args (optional)
//
// 	Globals:	none
//
//	Returns:	void
//
//
int iIcuUtlLogInfo
(
	const char *pcContext,
	const char *pcMessage,
	...
)
{

	va_list     ap;


	va_start(ap, pcMessage);
	iUtlLogInfo((unsigned char *)pcContext, (unsigned char *)pcMessage, ap);
	va_end(ap);


	return (ICU_NoError);

}


//---------------------------------------------------------------------------


//
//
//	Function:	iIcuUtlLogDebug
//
//	Purpose:	This function logs a debug message
//
//	Parameters:	pcContext		Context
//				pcMessage		Message to log
//				...				args (optional)
//
// 	Globals:	none
//
//	Returns:	void
//
//
int iIcuUtlLogDebug
(
	const char *pcContext,
	const char *pcMessage,
	...
)
{

	va_list     ap;


	va_start(ap, pcMessage);
	iUtlLogDebug((unsigned char *)pcContext, (unsigned char *)pcMessage, ap);
	va_end(ap);


	return (ICU_NoError);

}


//---------------------------------------------------------------------------
