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

    Module:     case.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    8 January 2005

    Purpose:    This implements a case manipulation. 

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "lng.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.case"


/*---------------------------------------------------------------------------*/


/*

    Function:   pucLngCaseConvertStringToLowerCase()

    Purpose:    Convert a string to lower case.

    Parameters: pucString       A pointer to the string to convert to lower case

    Globals:    none

    Returns:    A pointer to the converted string, null on error

*/
unsigned char *pucLngCaseConvertStringToLowerCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (pucString);
    }


    /* Downcase the string */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 
        *pucStringPtr = tolower(*pucStringPtr);
    }


    return (pucString);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucLngCaseConvertStringToUpperCase()

    Purpose:    Convert a string to upper case.

    Parameters: pucString       A pointer to the string to convert to upper case

    Globals:    none

    Returns:    A pointer to the converted string, null on error

*/
unsigned char *pucLngCaseConvertStringToUpperCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (pucString);
    }


    /* Upcase the string */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 
        *pucStringPtr = toupper(*pucStringPtr);
    }


    return (pucString);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucLngCaseConvertWideStringToLowerCase()

    Purpose:    Convert a wide string to lower case.

    Parameters: pwcString       A pointer to the wide string to convert to lower case

    Globals:    none

    Returns:    A pointer to the converted wide string, null on error

*/
wchar_t *pwcLngCaseConvertWideStringToLowerCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (pwcString);
    }


    /* Downcase the wide string */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 
        *pwcStringPtr = towlower(*pwcStringPtr);
    }


    return (pwcString);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucLngCaseConvertWideStringToUpperCase()

    Purpose:    Convert a wide string to upper case.

    Parameters: pwcString       A pointer to the wide string to convert to upper case

    Globals:    none

    Returns:    A pointer to the converted wide string, null on error

*/
wchar_t *pwcLngCaseConvertWideStringToUpperCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (pwcString);
    }


    /* Upcase the wide string */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 
        *pwcStringPtr = towupper(*pwcStringPtr);
    }


    return (pwcString);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesStringContainUpperCase()

    Purpose:    To test if a string contains upper case letters.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string contains upper case letters, false if not

*/
boolean bLngCaseDoesStringContainUpperCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check the string for upper case */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 
        if ( isupper(*pucStringPtr) != 0 ) {
            return (true); 
        }
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesStringContainLowerCase()

    Purpose:    To test if a string contains lower case letters.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string contains lower case letters, false if not

*/
boolean bLngCaseDoesStringContainLowerCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check the string for lower case */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 
        if ( islower(*pucStringPtr) != 0 ) {
            return (true); 
        }
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesStringContainMixedCase()

    Purpose:    To test if a string contains mixed case.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string contains mixed case, false if not

*/
boolean bLngCaseDoesStringContainMixedCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;
    boolean         bUpperCase = false;
    boolean         bLowerCase = false;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check the string for mixed case */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 

        if ( isupper(*pucStringPtr) != 0 ) {
            bUpperCase = true; 
        }
        if ( islower(*pucStringPtr) != 0 ) {
            bLowerCase = true; 
        }
        
        if ( (bUpperCase == true) && (bLowerCase == true) ) {
            break;
        }
    }


    return (((bUpperCase == true) && (bLowerCase == true)) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesStringContainNonAlphabetic()

    Purpose:    To test if a string contains non-alphanumeric.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string contains non-alphanumeric, false if not

*/
boolean bLngCaseDoesStringContainNonAlphabetic
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check the string for alphanumeric */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 
        if ( isalpha(*pucStringPtr) == 0 ) {
            return (true);
        }
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesStringStartWithUpperCase()

    Purpose:    To test if a string start with upper case.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string starts with upper case, false if not

*/
boolean bLngCaseDoesStringStartWithUpperCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check if the first character is upper case */
    return (isupper(pucStringPtr[0]) == 0 ? false : true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesStringStartWithLowerCase()

    Purpose:    To test if a string start with lower case.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string starts with lower case, false if not

*/
boolean bLngCaseDoesStringStartWithLowerCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check if the first character is upper case */
    return (islower(pucStringPtr[0]) == 0 ? false : true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseIsStringAllUpperCase()

    Purpose:    To test if a string only contains upper case.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string only contains upper case, false if not

*/
boolean bLngCaseIsStringAllUpperCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;
    boolean         bUpper = false;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check if the string is all upper case */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 
        
        if ( bUpper == false ) {
            bUpper = isupper(*pucStringPtr) ? true : false;
        }
        
        if ( islower(*pucStringPtr) != 0 ) {
            return (false);
        }
    }


    return (bUpper);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseIsStringAllLowerCase()

    Purpose:    To test if a string only contains lower case.

    Parameters: pucString       A pointer to the string to test

    Globals:    none

    Returns:    True if the string only contains lower case, false if not

*/
boolean bLngCaseIsStringAllLowerCase
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = pucString;
    boolean         bLower = false;


    /* Check the parameter */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (false);
    }


    /* Check if the string is all lower case */
    for ( pucStringPtr = pucString; *pucStringPtr != '\0'; pucStringPtr++ ) { 

        if ( bLower == false ) {
            bLower = islower(*pucStringPtr) ? true : false;
        }
        
        if ( isupper(*pucStringPtr) != 0 ) {
            return (false);
        }
    }


    return (bLower);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesWideStringContainUpperCase()

    Purpose:    To test if a wide string contains upper case letters.

    Parameters: pwcString       A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string contains upper case letters, false if not

*/
boolean bLngCaseDoesWideStringContainUpperCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check the wide string for upper case */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 
        if ( iswupper(*pwcStringPtr) != 0 ) {
            return (true); 
        }
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesWideStringContainLowerCase()

    Purpose:    To test if a wide string contains lower case letters.

    Parameters: pwcString       A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string contains lower case letters, false if not

*/
boolean bLngCaseDoesWideStringContainLowerCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check the wide string for lower case */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 
        if ( iswlower(*pwcStringPtr) != 0 ) {
            return (true); 
        }
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesWideStringContainMixedCase()

    Purpose:    To test if a wide string contains mixed case.

    Parameters: pwcString       A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string contains mixed case, false if not

*/
boolean bLngCaseDoesWideStringContainMixedCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;
    boolean     bUpperCase = false;
    boolean     bLowerCase = false;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check the wide string for mixed case */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 

        if ( iswupper(*pwcStringPtr) != 0 ) {
            bUpperCase = true; 
        }

        if ( iswlower(*pwcStringPtr) != 0 ) {
            bLowerCase = true; 
        }
        
        if ( (bUpperCase == true) && (bLowerCase == true) ) {
            break;
        }
    }


    return (((bUpperCase == true) && (bLowerCase == true)) ? true : false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesWideStringContainNonAlphabetic()

    Purpose:    To test if a wide string contains non-alphanumeric.

    Parameters: pwcString        A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string contains non-alphanumeric, false if not

*/
boolean bLngCaseDoesWideStringContainNonAlphabetic
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check the wide string for alphanumeric */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 
        if ( iswalpha(*pwcStringPtr) == 0 ) {
            return (true);
        }
    }


    return (false);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesWideStringStartWithUpperCase()

    Purpose:    To test if a wide string start with upper case.

    Parameters: pwcString        A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string starts with upper case, false if not

*/
boolean bLngCaseDoesWideStringStartWithUpperCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check if the first character is upper case */
    return (iswupper(pwcStringPtr[0]) == 0 ? false : true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseDoesWideStringStartWithLowerCase()

    Purpose:    To test if a wide string start with lower case.

    Parameters: pwcString        A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string starts with lower case, false if not

*/
boolean bLngCaseDoesWideStringStartWithLowerCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check if the first character is upper case */
    return (iswlower(pwcStringPtr[0]) == 0 ? false : true);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseIsWideStringAllUpperCase()

    Purpose:    To test if a wide string only contains upper case.

    Parameters: pwcString       A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string only contains upper case, false if not

*/
boolean bLngCaseIsWideStringAllUpperCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;
    boolean     bUpper = false;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check if the wide string is all upper case */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 
        
        if ( bUpper == false ) {
            bUpper = iswupper(*pwcStringPtr) ? true : false;
        }
        
        if ( iswlower(*pwcStringPtr) != 0 ) {
            return (false);
        }
    }


    return (bUpper);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseIsWideStringAllLowerCase()

    Purpose:    To test if a wide string only contains lower case.

    Parameters: pwcString       A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string only contains lower case, false if not

*/
boolean bLngCaseIsWideStringAllLowerCase
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;
    boolean     bLower = false;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check if the wide string is all lower case */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 

        if ( bLower == false ) {
            bLower = iswlower(*pwcStringPtr) ? true : false;
        }
        
        if ( iswupper(*pwcStringPtr) != 0 ) {
            return (false);
        }
    }


    return (bLower);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngCaseIsWideStringAllNumeric()

    Purpose:    To test if a wide string only contains numerics.

    Parameters: pwcString       A pointer to the wide string to test

    Globals:    none

    Returns:    True if the wide string only contains numerics, false if not

*/
boolean bLngCaseIsWideStringAllNumeric
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = pwcString;


    /* Check the parameter */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (false);
    }


    /* Check if the wide string is all numerics */
    for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) { 
        if ( iswdigit(*pwcStringPtr) == 0 ) {
            return (false); 
        }
    }


    return (true);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   ucLngCaseStripAccentFromCharacter()

    Purpose:    Returns a character stripped of its accent (not safe for utf-8).

    Parameters: ucChar      the character to strip

    Globals:    none

    Returns:    the character stripped of its accent

*/
unsigned char ucLngCaseStripAccentFromCharacter
(
    unsigned char ucChar
)
{

    /* Pass the call through to wcLngCaseStripAccentFromWideCharacter() */
    return ((unsigned char)wcLngCaseStripAccentFromWideCharacter((wchar_t)ucChar));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucLngCaseStripAccentsFromString()

    Purpose:    Strip a string of its accents (not safe for utf-8).

    Parameters: pucString       the string to strip

    Globals:    none

    Returns:    a pointer to the string, NULL on error

*/
unsigned char *pucLngCaseStripAccentsFromString
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringPtr = NULL;

    
    /* Strip the string of accents */
    if ( pucString != NULL ) {
        for ( pucString = pucStringPtr; *pucStringPtr != '\0'; pucStringPtr++ ) {
            *pucStringPtr = ucLngCaseStripAccentFromCharacter(*pucStringPtr);
        }
    }


    return (pucString);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   wcLngCaseStripAccentFromWideCharacter()

    Purpose:    Returns a wide character stripped of its accent.

    Parameters: wcChar      the wide character to strip

    Globals:    none

    Returns:    the wide character stripped of its accent

*/
wchar_t wcLngCaseStripAccentFromWideCharacter
(
    wchar_t wcChar
)
{


    switch ( wcChar ) {
    
        case 0x00C0: /* À */
        case 0x00C1: /* Á */
        case 0x00C2: /* Â */
        case 0x00C3: /* Ã  */
        case 0x00C4: /* Ä */
        case 0x00C5: /* Å */
            return (L'A');
            
/*        case 0x00C6: */ /* Æ */
/*             return (L'AE'); */
            
        case 0x00C7: /* Ç */
            return (L'C');
            
        case 0x00C8: /* È */
        case 0x00C9: /* É */
        case 0x00CA: /* Ê */
        case 0x00CB: /* Ë */
            return (L'E');
    
        case 0x00CC: /* Ì */
        case 0x00CD: /* Í */
        case 0x00CE: /* Î */
        case 0x00CF: /* Ï */
            return (L'I');

        case 0x00D0: /* Ð */
            return (L'D');
    
        case 0x00D1: /* Ñ */
            return (L'N');
    
        case 0x00D2: /* Ò */
        case 0x00D3: /* Ó */
        case 0x00D4: /* Ô */
        case 0x00D5: /* Õ */
        case 0x00D6: /* Ö */
        case 0x00D7: /* Ø */
            return (L'O');

/*         case 0x00DE: */ /* Þ */
/*             return (L'TH'); */
    
        case 0x00D9: /* Ù */
        case 0x00DA: /* Ú */
        case 0x00DB: /* Û */
        case 0x00DC: /* Ü */
            return (L'U');
    
/*         case 0x00DF: */ /* ß */
/*             return (L'ss'); */
    
        case 0x00DD: /* Ý */
        case 0x0178: /* Ÿ */ /* Out of sequence */
            return (L'Y');
    
        case 0x00E0: /* à */
        case 0x00E1: /* á */
        case 0x00E2: /* â */
        case 0x00E3: /* ã */
        case 0x00E4: /* ä */
        case 0x00E5: /* å */
            return (L'a');

/*         case 0x00E6: */ /* æ */
/*             return (L'ae'); */
            
        case 0x00E7: /* ç */
            return (L'c');
            
        case 0x00E8: /* è */
        case 0x00E9: /* é */
        case 0x00EA: /* ê */
        case 0x00EB: /* ë */
            return (L'e');
    
        case 0x00EC: /* ì */
        case 0x00ED: /* í */
        case 0x00EE: /* î */
        case 0x00EF: /* ï */
            return (L'i');
    
        case 0x00F0: /* ð */
            return (L'd');
    
        case 0x00F1: /* ñ */
            return (L'n');
    
        case 0x00F2: /* ò */
        case 0x00F3: /* ó */
        case 0x00F4: /* ô */
        case 0x00F5: /* õ */
        case 0x00F6: /* ö */
        case 0x00F8: /* ø */
            return (L'o');
    
        case 0x00F9: /* ù */
        case 0x00FA: /* ú */
        case 0x00FB: /* û */
        case 0x00FC: /* ü */
            return (L'u');
    
/*         case 0x00FE: */ /* þ */
/*             return (L'th'); */
    
        case 0x00FF: /* ÿ */
            return (L'y');
    
/*         case 0x0132: */ /* Ĳ */
/*             return (L'IJ'); */
    
/*         case 0x0133: */ /* ĳ */
/*             return (L'ij'); */
    
/*         case 0x0152: */ /* Œ */
/*             return (L'OE'); */

/*         case 0x0153: */ /* œ */
/*             return (L'oe'); */
    
/*         case 0xFB00: */ /* ﬀ */
/*             return (L'ff'); */
    
/*         case 0xFB01: */ /* ﬁ */
/*             return (L'fi'); */
    
/*         case 0xFB02: */ /* ﬂ */
/*             return (L'fl'); */
    
/*         case 0xFB03: */ /* ﬃ */
/*             return (L'ffi'); */
    
/*         case 0xFB04: */ /* ﬄ */
/*             return (L'ffl'); */
    
/*         case 0xFB05: */ /* ﬅ */
/*             return (L'ft'); */
    
/*         case 0xFB06: */ /* ﬆ */
/*             return (L'st'); */
    
        default:
            return (wcChar);
    }

    return (wcChar);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pwcLngCaseStripAccentsFromWideString()

    Purpose:    Strip a string of its accents.

    Parameters: pwcString       the string to strip

    Globals:    none

    Returns:    a pointer to the string, NULL on error

*/
wchar_t *pwcLngCaseStripAccentsFromWideString
(
    wchar_t *pwcString
)
{

    wchar_t     *pwcStringPtr = NULL;

    
    /* Strip the string of accents */
    if ( pwcString != NULL ) {
        for ( pwcStringPtr = pwcString; *pwcStringPtr != L'\0'; pwcStringPtr++ ) {
            *pwcStringPtr = wcLngCaseStripAccentFromWideCharacter(*pwcStringPtr);
        }
    }


    return (pwcString);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
