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

    Module:     strbuf.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    8 November 2004

    Purpose:    This file contains the string buffer functions.
    
                Functions to concatenate strings, this set of functions is more cumbersome
                than the iUtlStringsAppendString() function in strings.c, but they are a lot faster 
                when we need to append lots of strings together, or when we working with 
                long strings.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.strbuf"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* String buffer allocation length */
#define UTL_STRING_BUFFER_ALLOCATION_LENGTH     (5120)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* String buffer structure */
struct utlStringBuffer {
    void        *pvString;
    size_t      zStringLength;
    size_t      zStringCapacity;
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringBufferCreate()

    Purpose:    Create a new string buffer. 

                This function will set up the new string buffer structure and 
                return a pointer to that structure. This pointer needs to be passed
                back as part of every string buffer function.

    Parameters: ppvUtlStringBuffer      return pointer for the newly created string buffer pointer

    Globals:    none

    Returns:    UTL error code

*/

int iUtlStringBufferCreate
(
    void **ppvUtlStringBuffer
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlStringBufferCreate"); */


    /* Check the parameters */
    if ( ppvUtlStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlStringBuffer' parameter passed to 'iUtlStringBufferCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate the string buffer pointer */
    if ( (pusbUtlStringbuffer = (struct utlStringBuffer *)s_malloc((size_t)(sizeof(struct utlStringBuffer)))) == NULL ) {
        return (UTL_MemError);
    }


    /* Initialize the structure */
    pusbUtlStringbuffer->pvString = NULL;
    pusbUtlStringbuffer->zStringLength = 0;
    pusbUtlStringbuffer->zStringCapacity = 0;


    /* Set the return pointer */
    *ppvUtlStringBuffer = (void *)pusbUtlStringbuffer;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringBufferCreateWithString()

    Purpose:    Create a new string buffer, initializing it with the passed string. 

                This function will set up the new string buffer structure and 
                return a pointer to that structure. This pointer needs to be passed
                back as part of every string buffer function.

    Parameters: pucString               string to create the string buffer with
                ppvUtlStringBuffer      return pointer for the newly created string buffer

    Globals:    none

    Returns:    UTL error code

*/

int iUtlStringBufferCreateWithString
(
    unsigned char *pucString,
    void **ppvUtlStringBuffer
)
{

    void        *pvUtlStringBuffer = NULL;
    int         iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlStringBufferCreateWithString"); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucString' parameter passed to 'iUtlStringBufferCreateWithString'."); 
        return (UTL_StringBufferInvalidString);
    }

    if ( ppvUtlStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlStringBuffer' parameter passed to 'iUtlStringBufferCreateWithString'."); 
        return (UTL_ReturnParameterError);
    }

    
    /* Create the string buffer */
    if ( (iError = iUtlStringBufferCreate(&pvUtlStringBuffer)) != UTL_NoError ) {
        return (iError);
    }


    /* Append the string to the string buffer */
    if ( (iError = iUtlStringBufferAppend(pvUtlStringBuffer, pucString)) != UTL_NoError ) {
        return (iError);
    }


    /* Set the return pointer */
    *ppvUtlStringBuffer = (void *)pvUtlStringBuffer;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringBufferFree()

    Purpose:    Free the string buffer.

                This function will release all the memory resources currently
                held by this string buffer. After this the string buffer pointer 
                will be invalid.

    Parameters: pvUtlStringBuffer   string buffer pointer to release
                bFreeString         set to true to free the string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringBufferFree
(
    void *pvUtlStringBuffer,
    boolean bFreeString
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlStringBufferFree"); */


    /* Check the parameters */
    if ( pvUtlStringBuffer == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlStringBuffer' parameter passed to 'iUtlStringBufferFree'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }


    /* Free the string */
    if ( bFreeString == true ) {
        s_free(pusbUtlStringbuffer->pvString);
    }

    /* Free the string buffer pointer */
    s_free(pusbUtlStringbuffer);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringBufferGetLength()

    Purpose:    Get the string buffer pointer string length.

                This function will return the length of the string being held
                by the string buffer.

    Parameters: pvUtlStringBuffer   string buffer pointer
                pzStringLength      return pointer for the string length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringBufferGetLength
(
    void *pvUtlStringBuffer,
    size_t *pzStringLength
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlStringBufferGetLength"); */


    /* Check the parameters */
    if ( pvUtlStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlStringBuffer' parameter passed to 'iUtlStringBufferGetLength'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }

    if ( pzStringLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzStringLength' parameter passed to 'iUtlStringBufferGetLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the return pointer */
    *pzStringLength = pusbUtlStringbuffer->zStringLength;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringBufferGetString()

    Purpose:    Get the string buffer pointer string.

                This function will return a pointer to the string being held
                by the string buffer.

    Parameters: pvUtlStringBuffer   string buffer pointer
                ppucString          return pointer for the string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringBufferGetString
(
    void *pvUtlStringBuffer,
    unsigned char **ppucString
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlStringBufferGetLength"); */


    /* Check the parameters */
    if ( pvUtlStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlStringBuffer' parameter passed to 'iUtlStringBufferGetString'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }

    if ( ppucString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucString' parameter passed to 'iUtlStringBufferGetString'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the return pointer */
    *ppucString = (unsigned char *)pusbUtlStringbuffer->pvString;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringBufferClear()

    Purpose:    Clear the string buffer pointer.

                This function will clears the string buffer pointer.

    Parameters: pvUtlStringBuffer   string buffer pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringBufferClear
(
    void *pvUtlStringBuffer
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlStringBufferClear"); */


    /* Check the parameters */
    if ( pvUtlStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlStringBuffer' parameter passed to 'iUtlStringBufferClear'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }


    /* Free the string */
    s_free(pusbUtlStringbuffer->pvString);

    
    /* Initialize the structure */
    pusbUtlStringbuffer->pvString = NULL;
    pusbUtlStringbuffer->zStringLength = 0;
    pusbUtlStringbuffer->zStringCapacity = 0;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlStringBufferAppend()

    Purpose:    Get the string buffer pointer string.

                This function will append a string to the string buffer.

    Parameters: pvUtlStringBuffer   string buffer pointer
                pucString           the string to append (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlStringBufferAppend
(
    void *pvUtlStringBuffer,
    unsigned char *pucString
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlStringBufferAppend"); */


    /* Check the parameters */
    if ( pvUtlStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlStringBuffer' parameter passed to 'iUtlStringBufferAppend'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }

    
    /* Dont append NULL strings */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (UTL_NoError);
    }


    /* Allocate the destination string if needed, otherwise we append to it */
    if ( pusbUtlStringbuffer->pvString == NULL ) {
        
        if ( (pusbUtlStringbuffer->pvString = (void *)s_strdup(pucString)) == NULL ) {
            return (UTL_MemError);
        }
        
        pusbUtlStringbuffer->zStringLength = s_strlen(pucString);
        pusbUtlStringbuffer->zStringCapacity = pusbUtlStringbuffer->zStringLength + 1;
    }
    else {

        size_t    zStringLength = s_strlen(pucString);
 
        if ( (pusbUtlStringbuffer->zStringLength + zStringLength) >= pusbUtlStringbuffer->zStringCapacity ) {
    
            pusbUtlStringbuffer->zStringCapacity += UTL_MACROS_MAX(UTL_STRING_BUFFER_ALLOCATION_LENGTH, ((pusbUtlStringbuffer->zStringLength + zStringLength) - pusbUtlStringbuffer->zStringCapacity) + 1);
        
            if ( (pusbUtlStringbuffer->pvString = (void *)s_realloc(pusbUtlStringbuffer->pvString, (pusbUtlStringbuffer->zStringCapacity * sizeof(unsigned char )))) == NULL ) {
                return (UTL_MemError);
            }
        }

        s_strcat((unsigned char *)pusbUtlStringbuffer->pvString + pusbUtlStringbuffer->zStringLength, pucString);
        
        pusbUtlStringbuffer->zStringLength += zStringLength;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlWideStringBufferCreate()

    Purpose:    Create a new wide string buffer. 

                This function will set up the new wide string buffer structure and 
                return a pointer to that structure. This pointer needs to be passed
                back as part of every string buffer function.

    Parameters: ppvUtlWideStringBuffer      return pointer for the newly created wide string buffer pointer

    Globals:    none

    Returns:    UTL error code

*/

int iUtlWideStringBufferCreate
(
    void **ppvUtlWideStringBuffer
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlWideStringBufferCreate"); */


    /* Check the parameters */
    if ( ppvUtlWideStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlWideStringBuffer' parameter passed to 'iUtlWideStringBufferCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate the wide string buffer pointer */
    if ( (pusbUtlStringbuffer = (struct utlStringBuffer *)s_malloc((size_t)(sizeof(struct utlStringBuffer)))) == NULL ) {
        return (UTL_MemError);
    }


    /* Initialize the structure */
    pusbUtlStringbuffer->pvString = NULL;
    pusbUtlStringbuffer->zStringLength = 0;
    pusbUtlStringbuffer->zStringCapacity = 0;


    /* Set the return pointer */
    *ppvUtlWideStringBuffer = (void *)pusbUtlStringbuffer;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlWideStringBufferCreateWithString()

    Purpose:    Create a new wide string buffer, initializing it with the passed string. 

                This function will set up the new wide string buffer structure and 
                return a pointer to that structure. This pointer needs to be passed
                back as part of every wide string buffer function.

    Parameters: ppvUtlWideStringBuffer      return pointer for the newly created wide string buffer
                pwcString                   wide string to create the wide string buffer with

    Globals:    none

    Returns:    UTL error code

*/

int iUtlWideStringBufferCreateWithString
(
    wchar_t *pwcString,
    void **ppvUtlWideStringBuffer
)
{

    void        *pvUtlWideStringBuffer = NULL;
    int         iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlWideStringBufferCreateWithString"); */

    
    /* Check the parameters */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iUtlWideStringBufferCreateWithString'."); 
        return (UTL_StringBufferInvalidString);
    }

    if ( ppvUtlWideStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlWideStringBuffer' parameter passed to 'iUtlWideStringBufferCreateWithString'."); 
        return (UTL_ReturnParameterError);
    }


    /* Create the string buffer */
    if ( (iError = iUtlWideStringBufferCreate(&pvUtlWideStringBuffer)) != UTL_NoError ) {
        return (iError);
    }


    /* Append the string to the string buffer */
    if ( (iError = iUtlWideStringBufferAppend(pvUtlWideStringBuffer, pwcString)) != UTL_NoError ) {
        return (iError);
    }


    /* Set the return pointer */
    *ppvUtlWideStringBuffer = (void *)pvUtlWideStringBuffer;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlWideStringBufferFree()

    Purpose:    Free the wide string buffer.

                This function will release all the memory resources currently
                held by this wide string buffer. After this the wide string buffer pointer 
                will be invalid.

    Parameters: pvUtlWideStringBuffer       wide string buffer pointer to release
                bFreeString                 set to true to free the wide string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlWideStringBufferFree
(
    void *pvUtlWideStringBuffer,
    boolean bFreeString
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlWideStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlWideStringBufferFree"); */


    /* Check the parameters */
    if ( pvUtlWideStringBuffer == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlWideStringBuffer' parameter passed to 'iUtlWideStringBufferFree'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }


    /* Free the wide string */
    if ( bFreeString == true ) {
        s_free(pusbUtlStringbuffer->pvString);
    }

    /* Free the string buffer pointer */
    s_free(pusbUtlStringbuffer);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlWideStringBufferGetLength()

    Purpose:    Get the wide string buffer pointer string length.

                This function will return the length of the wide string being held
                by the wide string buffer.

    Parameters: pvUtlWideStringBuffer   wide string buffer pointer
                pzStringLength          return pointer for the wide string length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlWideStringBufferGetLength
(
    void *pvUtlWideStringBuffer,
    size_t *pzStringLength
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlWideStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlWideStringBufferGetLength"); */


    /* Check the parameters */
    if ( pvUtlWideStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlWideStringBuffer' parameter passed to 'iUtlWideStringBufferGetLength'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }

    if ( pzStringLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzStringLength' parameter passed to 'iUtlWideStringBufferGetLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the return pointer */
    *pzStringLength = pusbUtlStringbuffer->zStringLength;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlWideStringBufferGetString()

    Purpose:    Get the wide string buffer pointer string.

                This function will return a pointer to the wide string being held
                by the wide string buffer.

    Parameters: pvUtlWideStringBuffer   wide string buffer pointer
                ppwcString              return pointer for the wide string

    Globals:    none

    Returns:    UTL error code

*/
int iUtlWideStringBufferGetString
(
    void *pvUtlWideStringBuffer,
    wchar_t **ppwcString
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlWideStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlWideStringBufferGetLength"); */


    /* Check the parameters */
    if ( pvUtlWideStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlWideStringBuffer' parameter passed to 'iUtlWideStringBufferGetString'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }

    if ( ppwcString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcString' parameter passed to 'iUtlWideStringBufferGetLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the return pointer */
    *ppwcString = (wchar_t *)pusbUtlStringbuffer->pvString;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlWideStringBufferClear()

    Purpose:    Clear the wide string buffer pointer.

                This function will clears the wide string buffer pointer.

    Parameters: pvUtlWideStringBuffer   wide string buffer pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlWideStringBufferClear
(
    void *pvUtlWideStringBuffer
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlWideStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlWideStringBufferClear"); */


    /* Check the parameters */
    if ( pvUtlWideStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlWideStringBuffer' parameter passed to 'iUtlWideStringBufferGetString'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }


    /* Free the wide string */
    s_free(pusbUtlStringbuffer->pvString);

    
    /* Initialize the structure */
    pusbUtlStringbuffer->pvString = NULL;
    pusbUtlStringbuffer->zStringLength = 0;
    pusbUtlStringbuffer->zStringCapacity = 0;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlWideStringBufferAppend()

    Purpose:    Get the wide string buffer pointer string.

                This function will append a wide string to the wide string buffer.

    Parameters: pvUtlWideStringBuffer   wide string buffer pointer
                pwcString               the wide string to append (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlWideStringBufferAppend
(
    void *pvUtlWideStringBuffer,
    wchar_t *pwcString
)
{

    struct utlStringBuffer      *pusbUtlStringbuffer = (struct utlStringBuffer *)pvUtlWideStringBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlWideStringBufferAppend"); */


    /* Check the pointer */
    if ( pvUtlWideStringBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlWideStringBuffer' parameter passed to 'iUtlWideStringBufferAppend'."); 
        return (UTL_StringBufferInvalidStringBuffer);
    }


    /* Dont append NULL strings */
    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        return (UTL_NoError);
    }


    /* Allocate the destination string if needed, otherwise we append to it */
    if ( pusbUtlStringbuffer->pvString == NULL ) {
        
        if ( (pusbUtlStringbuffer->pvString = (void *)s_wcsdup(pwcString)) == NULL ) {
            return (UTL_MemError);
        }
        
        pusbUtlStringbuffer->zStringLength = s_wcslen(pwcString);
        pusbUtlStringbuffer->zStringCapacity = pusbUtlStringbuffer->zStringLength + 1;
    }
    else {

        size_t      zStringLength = s_wcslen(pwcString);
 
        if ( (pusbUtlStringbuffer->zStringLength + zStringLength) >= pusbUtlStringbuffer->zStringCapacity ) {
    
            pusbUtlStringbuffer->zStringCapacity += UTL_MACROS_MAX(UTL_STRING_BUFFER_ALLOCATION_LENGTH, ((pusbUtlStringbuffer->zStringLength + zStringLength) - pusbUtlStringbuffer->zStringCapacity) + 1);
        
            if ( (pusbUtlStringbuffer->pvString = (void *)s_realloc(pusbUtlStringbuffer->pvString, (pusbUtlStringbuffer->zStringCapacity * sizeof(wchar_t)))) == NULL ) {
                return (UTL_MemError);
            }
        }

        s_wcscat((wchar_t *)pusbUtlStringbuffer->pvString + pusbUtlStringbuffer->zStringLength, pwcString);
        
        pusbUtlStringbuffer->zStringLength += zStringLength;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


