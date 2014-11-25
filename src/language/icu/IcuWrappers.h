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
//  Module:     IcuWrappers.h
//
//  Author:     Francois Schiettecatte (FS Consulting LLC.)
//
//  Created:    15 March 2005
//
//  Purpose:    This is the header file for IcuWrappers.c
//
//


//---------------------------------------------------------------------------


#if !defined(LNG_ICU_WRAPPERS_C_H)
#define LNG_ICU_WRAPPERS_C_H


//---------------------------------------------------------------------------


//
// Includes
//

#include "IcuErr.h"


//---------------------------------------------------------------------------


// 
// C++ wrapper
//

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)


//---------------------------------------------------------------------------


//
// Public function prototypes
//


//
// Normalizer functions
//
int iIcuNormalizerCreate(unsigned char *pucConfigurationPath, void **ppvIcuNormalizer);

int iIcuNormalizerNormalizeString(void *pvIcuNormalizer, 
        unsigned char *pucString, unsigned int uiStringLength, 
        unsigned char **ppucNormalizedString, unsigned int *puiNormalizedStringLength);

int iIcuNormalizerFree(void *pvIcuNormalizer);


//---------------------------------------------------------------------------


// 
// C++ wrapper
//

#if defined(__cplusplus)
}
#endif    // defined(__cplusplus)


//---------------------------------------------------------------------------


#endif    // !defined(LNG_ICU_WRAPPERS_C_H)


//---------------------------------------------------------------------------
