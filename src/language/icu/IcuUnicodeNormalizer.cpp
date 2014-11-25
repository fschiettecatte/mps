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
//  Author:     Francois Schiettecatte (FS Consulting LLC.)
//
//  Created:    15 March 2005
//
//	Purpose:    This file provides the source code for a generic 
//              ICU-based Unicode normalizer/transliterator
//
//


//---------------------------------------------------------------------------


//
// Includes
//

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/unistr.h>
#include <unicode/unorm.h>
#include <unicode/normlzr.h>
#include <unicode/translit.h>
#include <unicode/utrans.h>

#include "IcuErr.h"
#include "IcuUtils.h"
#include "IcuUnicodeNormalizer.h"


//---------------------------------------------------------------------------

//
// Defines and constants
//

// Enable debugging output
// #define ICU_NORMALIZER_DEBUG

// Context for logging
static char ICU_LOG_CONTEXT[] = "com.fsconsult.mps.src.language.icu.IcuUnicodeNormalizer";


// ICU normalizer configuration file default, used if the passed
// configuration path is a directory path rather than a file path
#define LNG_ICU_NORMALIZATION_CONFIG_FILE_NAME_DEFAULT      (char *)"icu-normalizer.cf"


//---------------------------------------------------------------------------

//
//
//	Function:	IcuUnicodeNormalizer::create
//
//	Purpose:	Factory method (static) to instantiate an ICU
//				Unicode Normalizer given a set of normalization rules
//
//	Parameters:	sConfigurationPath      Configuration path
//
//	Returns:	A pointer to the newly created IcuUnicodeNormalizer 
//				instance or NULL if an error occurred (e.g. invalid configuration)
//
//
IcuUnicodeNormalizer *IcuUnicodeNormalizer::create 
(
	const string sConfigurationPath
)
{
	
	struct stat     psStatBuffer;
	ifstream        ifsConfigurationFile;


#if defined(ICU_NORMALIZER_DEBUG)
printf("IcuUnicodeNormalizer::create - sConfigurationPath: [%s]\n", sConfigurationPath.c_str());
#endif


	// Check the parameters
	if ( sConfigurationPath.c_str() == NULL ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null or empty 'sConfigurationPath' parameter passed to 'IcuUnicodeNormalizer::create'."); 
		return (NULL);
	}


	// Stat the path to see if it is a file or a directory
	if ( stat(sConfigurationPath.c_str(), &psStatBuffer) == -1 ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Invalid configuration path: '%s'.", sConfigurationPath.c_str()); 
		return (NULL);
	}


	// Check if this is a file
	if ( S_ISREG(psStatBuffer.st_mode) != false ) {
		
		// Open the stream
		ifsConfigurationFile.open(sConfigurationPath.c_str());
	}

	// Check if this is a directory
	else if ( S_ISDIR(psStatBuffer.st_mode) != false ) {
		
		// Create the configuration file path by appending the default configuration file name, 
		// assume unix file naming conventions
		string sConfigConfigurationFilePath = sConfigurationPath + "/" + string(LNG_ICU_NORMALIZATION_CONFIG_FILE_NAME_DEFAULT);
		
		// Open the stream
		ifsConfigurationFile.open(sConfigConfigurationFilePath.c_str());
	}

	
	// Check that we managed to open the configuration file
	if ( !ifsConfigurationFile ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Failed to open configuration file: '%s'.", sConfigurationPath.c_str()); 
		return (NULL);
	}
	
	

	// Create a new IcuUnicodeNormalizer instance
	IcuUnicodeNormalizer *pIcuUnicodeNormalizer = new IcuUnicodeNormalizer();

	
	// Read the rules from the configuration file into sRules
	string sRules = "";
	while ( !ifsConfigurationFile.eof() ) {
		string sLine;
		getline(ifsConfigurationFile, sLine);
		sRules += sLine + "\n";
	}
	ifsConfigurationFile.close();


	// Create a transliterator based on the config file
	if ( ! pIcuUnicodeNormalizer->init(sRules) ) {
		// Initialization failed, return NULL
		delete pIcuUnicodeNormalizer;
		return (NULL);
	}


	// Initialization was successful, return pIcuUnicodeNormalizer
	return (pIcuUnicodeNormalizer);

}


//---------------------------------------------------------------------------


//
//
//	Function:	IcuUnicodeNormalizer::IcuUnicodeNormalizer()
//
//	Purpose:	Default constructor for the IcuUnicodeNormalizer class
//
//	Parameters:	none
//
//	Returns:	N/A
//
//
IcuUnicodeNormalizer::IcuUnicodeNormalizer():pICUTransliterator
(
	NULL
) 
{

#if defined(ICU_NORMALIZER_DEBUG)
printf("IcuUnicodeNormalizer():pICUTransliterator\n");
#endif


	// Nothing to do

}


//---------------------------------------------------------------------------


//
//
//	Function:	IcuUnicodeNormalizer::~IcuUnicodeNormalizer()
//
//	Purpose:	Destructor for the IcuUnicodeNormalizer class
//
//	Parameters:	N/A
//
//	Returns:	N/A
//
//
IcuUnicodeNormalizer::~IcuUnicodeNormalizer
(
)
{

#if defined(ICU_NORMALIZER_DEBUG)
printf("IcuUnicodeNormalizer::~IcuUnicodeNormalizer\n");
#endif


	delete pICUTransliterator;

}


//---------------------------------------------------------------------------


//
//
//	Function:	IcuUnicodeNormalizer::init
//
//	Purpose:	Initializes the Unicode Normalizer instance with the 
//				rules contained in the string parameter. This method is 
//				private and should never be called directly.
//
//	Parameters:	sRules      std::string string containing the normalization rules,
//							normally the content of the configuration file. 
//
//	Returns:	True if the initialization is successful, false otherwise
//
//
bool IcuUnicodeNormalizer::init
(
	const string sRules
)
{

#if defined(ICU_NORMALIZER_DEBUG)
printf("IcuUnicodeNormalizer::init - sRules: [%s]\n", sRules.c_str());
#endif

	UErrorCode      status = U_ZERO_ERROR;
	UParseError     perror;


// 	ASSERT(sRules.c_str() != NULL);


	// Delete the current transliterator
	delete pICUTransliterator;


	// Create a new transliterator from the rules
	pICUTransliterator = Transliterator::createFromRules("MPSTransliterator", sRules.c_str(), UTRANS_FORWARD, perror, status);


	// Evaluate the status 
	if ( U_FAILURE(status) ) {
		// Initialization failed
		delete pICUTransliterator;
		pICUTransliterator = NULL;
		return (false);
	}


	// Initialization succeeded
	return (true);

}


//---------------------------------------------------------------------------


//
//
//	Function:	IcuUnicodeNormalizer::normalize
//
//	Purpose:	Normalizes a string according to the normalization 
//				rules of the IcuUnicodeNormalizer instance.
//
//	Parameters:	sString     String to normalize
//
//	Returns:	A new std::string string that is the normalized form of 
//				the input string according to normalization rules
//
//
string IcuUnicodeNormalizer::normalize
(
	const string sString
)
{

#if defined(ICU_NORMALIZER_DEBUG)
printf("IcuUnicodeNormalizer::normalize - sString: [%s]\n", sString.c_str());
#endif


	// Check the parameters
	if ( sString.c_str() == NULL ) {
		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null or empty 'sString' parameter passed to 'IcuUnicodeNormalizer::normalize'."); 
		return (NULL);
	}


	// Create unicode string from the passed string
	UnicodeString usNormalizedString(sString.c_str(), "UTF-8");


	// Normalize the unicode string, can we / should we 
	// add some error check on the following call ?
	normalize(usNormalizedString);


	// Get the length of the unicode string
	int iUnicodeStringLength = usNormalizedString.length();

	// First get the length of the unicode string as UTF-8
	int iUnicodeStringUtf8Length = usNormalizedString.extract(0, iUnicodeStringLength, NULL, "UTF-8");

	// Allocate the destination string
	char *pcNormalizedStringUtf8 = new char[iUnicodeStringUtf8Length + 1];

	// Extract the normalized string to the destination string
	usNormalizedString.extract(0, iUnicodeStringLength, pcNormalizedStringUtf8, "UTF-8");

	// Create a string from the destination string
	string sNormalizedString(pcNormalizedStringUtf8);

	// Deallocate the memory we have been allocating here
	delete[] pcNormalizedStringUtf8;


	return (sNormalizedString);

}


//---------------------------------------------------------------------------
   

//
//
//	Function:	 IcuUnicodeNormalizer::normalize
//
//	Purpose:	Normalizes a string according to the normalization 
//				rules of the IcuUnicodeNormalizer instance.
//
//	Parameters:	usInput     The UnicodeString string, content of 
//							which we want to normalize, passed by 
//							reference and normalized in place
//
//	Returns:	nothing
//
void IcuUnicodeNormalizer::normalize
(
	UnicodeString &usInput
)
{

#if defined(ICU_NORMALIZER_DEBUG)
printf("IcuUnicodeNormalizer::normalize - usInput: [%s]\n", usInput);
#endif

	// Check the parameters
// 	if ( usInput == NULL ) {
// 		iIcuUtlLogError(ICU_LOG_CONTEXT, "Null or empty 'usInput' parameter passed to 'IcuUnicodeNormalizer::normalize'."); 
// 		return;
// 	}


	pICUTransliterator->transliterate(usInput);

}


//---------------------------------------------------------------------------
