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
//  Module:     IcuErr.h
//
//  Author:     Francois Schiettecatte (FS Consulting LLC.)
//
//  Created:    7 October 2006
//
//  Purpose:    This header file defines all the error codes which 
//              can be returned by the icu functions.
//
//              These error codes are organized in such a way that 
//              anything other that a 0 is an error.
//
//              Also they have been divided into error ranges as 
//              follows:
//
//
//                  generic         -1 through      -99 
//                  normalizer    -100 through     -199 
//
//
//                  Error code -1 is a generic error.
//
//                  Error codes -2 through -99 are reserved.
//
//


//---------------------------------------------------------------------------


#if !defined(ICU_ERR_H)
#define ICU_ERR_H


//---------------------------------------------------------------------------


//
// Defines
//

// Generic
#define ICU_NoError                                     (0)
#define ICU_MemError                                    (-1)
#define ICU_ParameterError                              (-2)
#define ICU_ReturnParameterError                        (-3)
#define ICU_MiscError                                   (-4)


// Normalizer
#define ICU_NormalizerInvalidConfigurationPath          (-100)
#define ICU_NormalizerFailedCreate                      (-101)
#define ICU_NormalizerInvalidNormalizer                 (-102)
#define ICU_NormalizerInvalidString                     (-103)
#define ICU_NormalizerInvalidStringLength               (-104)
#define ICU_NormalizerInvalidNormalizedString           (-105)
#define ICU_NormalizerInvalidNormalizedStringLength     (-106)
#define ICU_NormalizerFailedNormalization               (-107)


//---------------------------------------------------------------------------


#endif    // !defined(ICU_ERR_H)


//---------------------------------------------------------------------------
