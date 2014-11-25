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
//  Module:     IcuWrappers.cpp
//
//	Author:     Francois Schiettecatte (FS Consulting LLC.)
//
//	Created:    15 March 2005
//
//	Purpose:    This file provides the C wrapper functions.
//
//


//---------------------------------------------------------------------------


//
// Includes
//

#include <unicode/uchar.h>

#include "utils.h"

#include "IcuErr.h"
#include "IcuUtils.h"
#include "IcuWrappers.h"

#include "IcuUnicodeNormalizer.h"


//---------------------------------------------------------------------------


//
// Defines and constants
//

// Enable debugging output
// #define ICU_NORMALIZER_DEBUG

// Context for logging
static char ICU_LOG_CONTEXT[] = "com.fsconsult.mps.src.language.icu.IcuWrappers";


//---------------------------------------------------------------------------


//     
//
//	Function:	iIcuNormalizerCreate
//                                                                                        
//	Purpose:	C Wrapper for IcuUnicodeNormalizer::create
//        
//	Parameters:	pucConfigurationPath        Configuration path (directory/file)
//				ppvIcuNormalizer            Return pointer for the ICU normalizer
//
//	Returns:	ICU error code
//
//
int iIcuNormalizerCreate
(
	unsigned char *pucConfigurationPath, 
	void **ppvIcuNormalizer
)
{

	IcuUnicodeNormalizer    *pIcuUnicodeNormalizer = NULL;


#if defined(ICU_NORMALIZER_DEBUG)
printf("iIcuNormalizerCreate - pucConfigurationPath: [%s]\n", pucConfigurationPath);
#endif


	// Check the parameters
	if ( bUtlStringsIsStringNULL(pucConfigurationPath) == true ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null or empty 'pucConfigurationPath' parameter passed to 'iIcuNormalizerCreate'.");
		return (ICU_NormalizerInvalidConfigurationPath);
	}

	if ( ppvIcuNormalizer == NULL ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null 'ppvIcuNormalizer' parameter passed to 'iIcuNormalizerCreate'."); 
		return (ICU_ReturnParameterError);
	}


	// Create the ICU unicode normalizer - bail on error
	if ( (pIcuUnicodeNormalizer = IcuUnicodeNormalizer::create(string((char *)pucConfigurationPath))) == NULL ) {
		return (ICU_NormalizerFailedCreate);
	}
	
	
	// Set the return pointer 
	*ppvIcuNormalizer = (void *)pIcuUnicodeNormalizer;


	return (ICU_NoError);

}


//---------------------------------------------------------------------------


//
//
//	Function:	iIcuNormalizerNormalizeString
//
//	Purpose:	C wrapper for IcuUnicodeNormalizer::normalize
//
//	Parameters:	pvIcuNormalizer				ICU unicode normalizer structure 
//				pucString 					String to normalize
//				uiStringLength				String length
//				ppucNormalizedString		Destination string/return pointer for the normalized string
//				puiNormalizedStringLength   Destination string length/return pointer
//											(set to capacity if destination string is allocated)
//
//	Returns:	ICU error code
//
//
int iIcuNormalizerNormalizeString
(
	void *pvIcuNormalizer, 
	unsigned char *pucString, 
	unsigned int uiStringLength,
	unsigned char **ppucNormalizedString, 
	unsigned int *puiNormalizedStringLength
)
{

	IcuUnicodeNormalizer    *pIcuUnicodeNormalizer = NULL;
	string                  normalizedString;


#if defined(ICU_NORMALIZER_DEBUG)
printf("iIcuNormalizerNormalizeString - pucString: [%s], uiStringLength: [%u]\n", pucString, uiStringLength);
#endif


	// Check the parameters
	if ( (pIcuUnicodeNormalizer = static_cast<IcuUnicodeNormalizer *>(pvIcuNormalizer)) == NULL ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null 'pvIcuNormalizer' parameter passed to 'iIcuNormalizerNormalizeString'."); 
		return (ICU_NormalizerInvalidNormalizer);
	}

	if ( bUtlStringsIsStringNULL(pucString) == true ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iIcuNormalizerNormalizeString'."); 
		return (ICU_NormalizerInvalidString);
	}

	if ( uiStringLength <= 0 ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Invalid 'uiStringLength' parameter passed to 'iIcuNormalizerNormalizeString'."); 
		return (ICU_NormalizerInvalidStringLength);
	}

	if ( ppucNormalizedString == NULL ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Invalid 'ppucNormalizedString' parameter passed to 'iIcuNormalizerNormalizeString'."); 
		return (ICU_NormalizerInvalidNormalizedString);
	}

	if ( (*ppucNormalizedString != NULL) && (*puiNormalizedStringLength == 0) ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Invalid 'ppucNormalizedString' and 'puiNormalizedStringLength' parameters passed to 'iIcuNormalizerNormalizeString'."); 
		return (ICU_NormalizerInvalidNormalizedStringLength);
	}


	// Normalize the string
	normalizedString = pIcuUnicodeNormalizer->normalize(string((char *)pucString));


	// Allocate the return pointer if it is not allocated
	// otherwise we copy the string into the passed pointer
	if ( *ppucNormalizedString == NULL ) {

		// Duplicate the string to the return pointer
		if ( (*ppucNormalizedString = (unsigned char *)strdup(normalizedString.c_str())) == NULL ) {
			return (ICU_MemError);
		} 

		// Set the return pointer with the length of the normalized string
		*puiNormalizedStringLength = normalizedString.length();
	}
	else {
		
		// Check that there is enough space in the passed pointer
		if ( normalizedString.length() >= *puiNormalizedStringLength ) {
			return (ICU_NormalizerFailedNormalization);
		}

		// Copy the string into the passed pointer 
		strcpy((char *)*ppucNormalizedString, (char *)normalizedString.c_str());
	}


	return (ICU_NoError);

}


//---------------------------------------------------------------------------


//
//
//	Function:	iIcuNormalizerFree
//
//	Purpose:	C wrapper for IcuUnicodeNormalizer deallocation
//
//	Parameters:	pvIcuNormalizer			ICU unicode normalizer structure 
//
//	Returns: 	ICU error code                                                                                     
//
//
int iIcuNormalizerFree
(
	void *pvIcuNormalizer
) 
{

	IcuUnicodeNormalizer    *pIcuUnicodeNormalizer = NULL;


#if defined(ICU_NORMALIZER_DEBUG)
printf("iIcuNormalizerFree\n");
#endif


	// Check the parameters
	if ( (pIcuUnicodeNormalizer = static_cast<IcuUnicodeNormalizer *>(pvIcuNormalizer)) == NULL ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null 'pvIcuNormalizer' parameter passed to 'iIcuNormalizerFree'."); 
		return (ICU_NormalizerInvalidNormalizer);
	}


	// Delete the ICU unicode normalizer instance
	delete pIcuUnicodeNormalizer;
	

	return (ICU_NoError);

}


//---------------------------------------------------------------------------
