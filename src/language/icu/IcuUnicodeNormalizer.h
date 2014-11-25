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
//  Purpose:    This is the header file for IcuWrappers.cpp
//
//


//---------------------------------------------------------------------------


#if !defined(LNG_ICU_NORMALIZER_H)
#define LNG_ICU_NORMALIZER_H


//---------------------------------------------------------------------------


//
// This module provides the Unicode normalization filter that is used by MPS 
// 
// Please check http://www.unicode.org/reports/tr15/ for more on Unicode Normalization.
//
// Here we use the ICU lib to provide the basic Unicode Normalization functionality
// as well as the Transform functionality, which allows us to go beyond simple 
// Unicode Normalization.
//
// Unless otherwise noted, all methods should be pretty self-explanatory.
//


//
// Configuration file format:
//
// The configuration file used by this module contains the specification of 
// the Unicode transformation that will be applied when calling 
// normalize().
//
// The format of this file is the one used by the ICU library itself and 
// which is described at http://icu.sourceforge.net/userguide/TransformRule.html
//


//---------------------------------------------------------------------------

//
// Includes
//

#include <string>
using namespace std;

#include <unicode/unistr.h>
#include <unicode/translit.h>


//---------------------------------------------------------------------------


//
// Class declaration
//

class IcuUnicodeNormalizer {

    public:

        // Factory method:
        //
        // Input parameter a configuration path, file or directory
        //
        // Output is new IcuUnicodeNormalizer instance or NULL if 
        // normalizer cannot be instantiated
        //
        static IcuUnicodeNormalizer *create(const string sConfigurationPath);


        // Default constructor:
        //
        IcuUnicodeNormalizer();


        // Destructor:
        //
        virtual ~IcuUnicodeNormalizer();


        // Applies normalization:
        //
        // --> Standard Unicode Normalization + Character Mapping and/or transliteration
        //
        // Input parameter is string to normalize
        //
        // Output is normalized string according to the transformation 
        // specified in config file
        //
        string normalize(const string sInput);


        // Same as above but with UnicodeString input
        // and it normalizes in place 
        //
        void normalize(UnicodeString &usInput);

        
    private:

        // Initializes the ICU transliterator object with
        // transformation specified by sConfigContent
        //
        bool init(const string sConfigContent);


        // Pointer to ICU Transliterator instance
        //
        Transliterator *pICUTransliterator;

};


//---------------------------------------------------------------------------


#endif    // !defined(LNG_ICU_NORMALIZER_H)


//---------------------------------------------------------------------------
