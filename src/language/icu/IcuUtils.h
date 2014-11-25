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
//  Module:     IcuUtils.h
//
//  Author:     Francois Schiettecatte (FS Consulting LLC.)
//
//  Created:    2 April 2010
//
//  Purpose:    This file is the header file for IcuUtils.cpp
//
//


//
//---------------------------------------------------------------------------
//


#if !defined(ICU_UTILS_H)
#define ICU_UTILS_H


//---------------------------------------------------------------------------


//
// Public function prototypes
//

int iIcuUtlLogFatal(const char *pcContext, const char *pcMessage, ...);

int iIcuUtlLogError(const char *pcContext, const char *pcMessage, ...);

int iIcuUtlLogWarn(const char *pcContext, const char *pcMessage, ...);

int iIcuUtlLogInfo(const char *pcContext, const char *pcMessage, ...);

int iIcuUtlLogDebug(const char *pcContext, const char *pcMessage, ...);



//---------------------------------------------------------------------------


#endif    // !defined(ICU_UTILS_H)
