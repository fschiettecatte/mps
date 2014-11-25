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

    Module:     srvr_http.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    15 May 2006

    Purpose:    This module implements the 'http' protocol handler.

                /SearchIndex?
                    index               - index names
                    language            - language code
                    search              - search text
                    positiveFeedback    - positive feedback text
                    negativeFeedback    - negative feedback text
                    start               - start index
                    limit               - limit (0 means no limit)
                    report              - search report, 'none', 'raw' or 'formatted', defaults to 'formatted'
                    format              - format, 'rss', 'xml', 'mxg', 'json', 'ruby' or 'python', defaults to 'rss'
                    extensions          - extensions, comma delimited, 'mps', 'opensearch', for 'rss' format only
        
                /RetrieveDocument?
                    index               - index name
                    documentKey         - document key
                    itemName            - item name
                    mimeType            - mime type 
                    format              - format, 'rss', 'xml', 'json', 'ruby', 'python' or 'raw', defaults to 'rss'
        
                /ServerInfo
                    format              - format, 'xml', 'json', 'ruby' or 'python', defaults to 'xml'
            
                /ServerIndexInfo
                    format              - format, 'xml', 'json', 'ruby' or 'python', defaults to 'xml'
            
                /IndexInfo?
                    index               - index name
                    format              - format, 'xml', 'json', 'ruby' or 'python', defaults to 'xml'
            
                /IndexFieldInfo?
                    index               - index name
                    format              - format, 'xml', 'json', 'ruby' or 'python', defaults to 'xml'
            
                /IndexTermInfo?
                    index               - index name
                    termMatch           - term match, 'regular', 'stop', 'wildcard', 'soundex', 'metaphone', 'phonix', 'typo', 'regex', defaults to 'regular'  
                    termCase            - term case, 'sensitive' or 'insensitive', defaults to 'sensitive'
                    term                - term (optional)
                    field               - field name (optional)
                    format              - format, 'xml', 'json', 'ruby' or 'python', defaults to 'xml'
            
                /DocumentInfo?
                    index               - index name
                    documentKey         - document key
                    format              - format, 'xml', 'json', 'ruby' or 'python', defaults to 'xml'
        
            
                Running a search:
                    wget "http://localhost:9000/SearchIndex?index=jfif&language=en&search=animals&start=0&limit=10&report=formatted" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=jfif&language=en&search=animals&start=0&limit=10&report=formatted&format=rss" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=jfif&language=en&search=animals&start=0&limit=10&report=formatted&format=rss&extensions=mps" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=jfif&language=en&search=animals&start=0&limit=10&report=formatted&format=rss&extensions=opensearch" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=jfif&language=en&search=animals&start=0&limit=10&report=formatted&format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=jfif&language=en&search=animals&start=0&limit=10&report=formatted&format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=hci-bib&language=en&search=animals&start=0&limit=10&report=formatted&format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=hci-bib&language=en&search=animals&start=0&limit=10&report=formatted&format=ruby" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/SearchIndex?index=hci-bib&language=en&search=animals&start=0&limit=10&report=formatted&format=python" --output-document=index.xml && more index.xml
        
                Getting a document:
                    wget "http://localhost:9000/RetrieveDocument?index=jfif&documentKey=1&itemName=document&mimeType=text/plain" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/RetrieveDocument?index=jfif&documentKey=1&itemName=document&mimeType=text/plain&format=rss" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/RetrieveDocument?index=jfif&documentKey=1&itemName=document&mimeType=text/plain&format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/RetrieveDocument?index=jfif&documentKey=1&itemName=document&mimeType=text/plain&format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/RetrieveDocument?index=jfif&documentKey=1&itemName=document&mimeType=text/plain&format=ruby" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/RetrieveDocument?index=jfif&documentKey=1&itemName=document&mimeType=text/plain&format=python" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/RetrieveDocument?index=jfif&documentKey=1&itemName=document&mimeType=text/plain&format=raw" --output-document=index.xml && more index.xml
                
                Getting information:
                    wget "http://localhost:9000/ServerInfo?format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/ServerInfo?format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/ServerIndexInfo?format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/ServerIndexInfo?format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/IndexInfo?index=jfif&format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/IndexInfo?index=jfif&format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/IndexFieldInfo?index=jfif&format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/IndexFieldInfo?index=jfif&format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/IndexTermInfo?index=jfif&format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/IndexTermInfo?index=jfif&format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/DocumentInfo?index=jfif&documentKey=1&format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/DocumentInfo?index=hci-bib&documentKey=1&format=xml" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/DocumentInfo?index=jfif&documentKey=1&format=json" --output-document=index.xml && more index.xml
                    wget "http://localhost:9000/DocumentInfo?index=hci-bib&documentKey=1&format=json" --output-document=index.xml && more index.xml
        
*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "rep.h"

#include "spi.h"

#include "server.h"

#include "srvr_http.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.server.srvr_http"


/* Enable the logging of http errors */
#define SRVR_HTTP_ENABLE_HTTP_ERROR_LOGGING

/* Enable the logging of spi errors */
#define SRVR_HTTP_ENABLE_SPI_ERROR_LOGGING


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* HTTP methods */
#define SRVR_HTTP_METHOD_GET                        (unsigned char *)"GET"

/* HTTP content types */
#define SRVR_HTTP_CONTENT_TYPE_TEXT_XML             (unsigned char *)"text/xml; charset=utf-8"
#define SRVR_HTTP_CONTENT_TYPE_TEXT_RSS             (unsigned char *)"application/rss+xml; charset=utf-8"
#define SRVR_HTTP_CONTENT_TYPE_TEXT_ATOM            (unsigned char *)"application/atom+xml; charset=utf-8"
#define SRVR_HTTP_CONTENT_TYPE_TEXT_JSON            (unsigned char *)"text/x-json; charset=utf-8"
#define SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY            (unsigned char *)"text/x-ruby; charset=utf-8"
#define SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON          (unsigned char *)"text/x-python; charset=utf-8"

/* HTTP status */
#define SRVR_HTTP_STATUS_OK                         (200)
#define SRVR_HTTP_STATUS_BAD_REQUEST                (400)
#define SRVR_HTTP_STATUS_METHOD_NOT_ALLOWED         (405)
#define SRVR_HTTP_STATUS_REQUEST_TIMEOUT            (408)
#define SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR      (500)
#define SRVR_HTTP_STATUS_NOT_IMPLEMENTED            (501)
#define SRVR_HTTP_STATUS_SERVICE_UNAVAILABLE        (503)



/* Paths */
#define SRVR_HTTP_PATH_SEARCH_INDEX                 (unsigned char *)"SearchIndex"
#define SRVR_HTTP_PATH_RETRIEVE_DOCUMENT            (unsigned char *)"RetrieveDocument"
#define SRVR_HTTP_PATH_SERVER_INFO                  (unsigned char *)"ServerInfo"
#define SRVR_HTTP_PATH_SERVER_INDEX_INFO            (unsigned char *)"ServerIndexInfo"
#define SRVR_HTTP_PATH_INDEX_INFO                   (unsigned char *)"IndexInfo"
#define SRVR_HTTP_PATH_INDEX_FIELD_INFO             (unsigned char *)"IndexFieldInfo"
#define SRVR_HTTP_PATH_INDEX_TERM_INFO              (unsigned char *)"IndexTermInfo"
#define SRVR_HTTP_PATH_DOCUMENT_INFO                (unsigned char *)"DocumentInfo"


/* Parameter names */
#define SRVR_HTTP_PARAMETER_INDEX                   (unsigned char *)"index"
#define SRVR_HTTP_PARAMETER_LANGUAGE                (unsigned char *)"language"
#define SRVR_HTTP_PARAMETER_ENCODING                (unsigned char *)"encoding"
#define SRVR_HTTP_PARAMETER_SEARCH                  (unsigned char *)"search"
#define SRVR_HTTP_PARAMETER_POSITIVE_FEEDBACK       (unsigned char *)"positiveFeedback"
#define SRVR_HTTP_PARAMETER_NEGATIVE_FEEDBACK       (unsigned char *)"negativeFeedback"
#define SRVR_HTTP_PARAMETER_START                   (unsigned char *)"start"
#define SRVR_HTTP_PARAMETER_LIMIT                   (unsigned char *)"limit"
#define SRVR_HTTP_PARAMETER_REPORT                  (unsigned char *)"report"


#define SRVR_HTTP_PARAMETER_DOCUMENT_KEY            (unsigned char *)"documentKey"
#define SRVR_HTTP_PARAMETER_ITEM_NAME               (unsigned char *)"itemName"
#define SRVR_HTTP_PARAMETER_MIME_TYPE               (unsigned char *)"mimeType"


#define SRVR_HTTP_PARAMETER_TERM_MATCH              (unsigned char *)"termMatch"
#define SRVR_HTTP_PARAMETER_TERM_CASE               (unsigned char *)"termCase"
#define SRVR_HTTP_PARAMETER_TERM                    (unsigned char *)"term"
#define SRVR_HTTP_PARAMETER_FIELD_NAME              (unsigned char *)"field"

#define SRVR_HTTP_OPTION_TERM_MATCH_REGULAR         (unsigned char *)"regular"
#define SRVR_HTTP_OPTION_TERM_MATCH_STOP            (unsigned char *)"stop"
#define SRVR_HTTP_OPTION_TERM_MATCH_WILDCARD        (unsigned char *)"wildcard"
#define SRVR_HTTP_OPTION_TERM_MATCH_SOUNDEX         (unsigned char *)"soundex"
#define SRVR_HTTP_OPTION_TERM_MATCH_METAPHONE       (unsigned char *)"metaphone"
#define SRVR_HTTP_OPTION_TERM_MATCH_PHONIX          (unsigned char *)"phonix"
#define SRVR_HTTP_OPTION_TERM_MATCH_TYPO            (unsigned char *)"typo"
#define SRVR_HTTP_OPTION_TERM_MATCH_REGEX           (unsigned char *)"regex"

#define SRVR_HTTP_OPTION_TERM_CASE_SENSITIVE        (unsigned char *)"sensitive"
#define SRVR_HTTP_OPTION_TERM_CASE_INSENSITIVE      (unsigned char *)"insensitive"


#define SRVR_HTTP_PARAMETER_FORMAT                  (unsigned char *)"format"

#define SRVR_HTTP_OPTION_RSS                        (unsigned char *)"rss"
#define SRVR_HTTP_OPTION_XML                        (unsigned char *)"xml"
#define SRVR_HTTP_OPTION_MXG                        (unsigned char *)"mxg"
#define SRVR_HTTP_OPTION_JSON                       (unsigned char *)"json"
#define SRVR_HTTP_OPTION_RUBY                       (unsigned char *)"ruby"
#define SRVR_HTTP_OPTION_PYTHON                     (unsigned char *)"python"
#define SRVR_HTTP_OPTION_RAW                        (unsigned char *)"raw"

#define SRVR_HTTP_FORMAT_UNKNOWN                    (0)
#define SRVR_HTTP_FORMAT_RSS                        (1)
#define SRVR_HTTP_FORMAT_XML                        (2)
#define SRVR_HTTP_FORMAT_MXG                        (3)         /* Not yet supported */
#define SRVR_HTTP_FORMAT_JSON                       (4)
#define SRVR_HTTP_FORMAT_RUBY                       (5)
#define SRVR_HTTP_FORMAT_PYTHON                     (6)
#define SRVR_HTTP_FORMAT_RAW                        (7)

#define SRVR_HTTP_FORMAT_VALID(n)                   (((n) >= SRVR_HTTP_FORMAT_RSS) && \
                                                            ((n) <= SRVR_HTTP_FORMAT_RAW))


#define SRVR_HTTP_PARAMETER_EXTENSIONS              (unsigned char *)"extensions"

#define SRVR_HTTP_OPTION_MPS                        (unsigned char *)"mps"
#define SRVR_HTTP_OPTION_OPENSEARCH                 (unsigned char *)"opensearch"

#define SRVR_HTTP_EXTENSION_NONE                    (0)
#define SRVR_HTTP_EXTENSION_MPS                     (1 << 0)        /* Applies to 'rss' format only */
#define SRVR_HTTP_EXTENSION_OPENSEARCH              (1 << 1)        /* Applies to 'rss' format only */

#define bSrvrHttpExtensionMps(f)                    (((f) & SRVR_HTTP_EXTENSION_MPS) > 0 ? true : false)
#define bSrvrHttpExtensionOpenSearch(f)             (((f) & SRVR_HTTP_EXTENSION_OPENSEARCH) > 0 ? true : false)

#define vSrvrHttpExtensionMpsOn(f)                  ((f) |= SRVR_HTTP_EXTENSION_MPS)
#define vSrvrHttpExtensionMpsOff(f)                 ((f) &= ~SRVR_HTTP_EXTENSION_MPS)
#define vSrvrHttpExtensionOpenSearchOn(f)           ((f) |= SRVR_HTTP_EXTENSION_OPENSEARCH)
#define vSrvrHttpExtensionOpenSearchOff(f)          ((f) &= ~SRVR_HTTP_EXTENSION_OPENSEARCH)



#define SRVR_HTTP_OPTION_NONE                       (unsigned char *)"none"
#define SRVR_HTTP_OPTION_RAW                        (unsigned char *)"raw"
#define SRVR_HTTP_OPTION_FORMATTED                  (unsigned char *)"formatted"

#define SRVR_HTTP_REPORT_UNKNOWN                    (0)
#define SRVR_HTTP_REPORT_NONE                       (1)
#define SRVR_HTTP_REPORT_RAW                        (2)
#define SRVR_HTTP_REPORT_FORMATTED                  (3)

#define SRVR_HTTP_REPORT_VALID(n)                   (((n) >= SRVR_HTTP_REPORT_NONE) && \
                                                            ((n) <= SRVR_HTTP_REPORT_FORMATTED))



#define SRVR_HTTP_ERROR_DEFAULT                     (-1)

#define SRVR_HTTP_MESSAGE_LOAD_EXCEEDED             (unsigned char *)"Rejecting the client request because the server load is too high"
#define SRVR_HTTP_MESSAGE_INVALID_REQUEST           (unsigned char *)"Invalid request"
#define SRVR_HTTP_MESSAGE_INVALID_PROTOCOL          (unsigned char *)"Invalid protocol"
#define SRVR_HTTP_MESSAGE_INVALID_METHOD            (unsigned char *)"Invalid method"
#define SRVR_HTTP_MESSAGE_INVALID_PATH              (unsigned char *)"Invalid path"
#define SRVR_HTTP_MESSAGE_INVALID_QUERY             (unsigned char *)"Invalid query"
#define SRVR_HTTP_MESSAGE_MISSING_INDEX_NAMES       (unsigned char *)"Missing index names variable"
#define SRVR_HTTP_MESSAGE_MISSING_INDEX_NAME        (unsigned char *)"Missing index name variable"
#define SRVR_HTTP_MESSAGE_MISSING_DOCUMENT_KEY      (unsigned char *)"Missing document key variable"
#define SRVR_HTTP_MESSAGE_MISSING_ITEM_NAME         (unsigned char *)"Missing item name variable"
#define SRVR_HTTP_MESSAGE_MISSING_MIME_TYPE         (unsigned char *)"Missing mime type variable"


/* Quotes and separators */
#define SRVR_HTTP_QUOTE_JSON                        (unsigned char *)"\""
#define SRVR_HTTP_QUOTE_RUBY                        (unsigned char *)"'"
#define SRVR_HTTP_QUOTE_PYTHON                      (unsigned char *)"'"

#define SRVR_HTTP_SEPARATOR_JSON                    (unsigned char *)":"
#define SRVR_HTTP_SEPARATOR_RUBY                    (unsigned char *)"=>"
#define SRVR_HTTP_SEPARATOR_PYTHON                  (unsigned char *)":"



/* Set the maximum length of long strings */
#define SRVR_HTTP_LONG_STRING_LENGTH                (5120)

/* Set the maximum length of short strings */
#define SRVR_HTTP_SHORT_STRING_LENGTH               (256)

/* Sets the default number of documents to retrieve */
#define SRVR_HTTP_DOCUMENTS_TO_RETRIEVE_DEFAULT     (50)


/* Index name separators */
#define SRVR_HTTP_INDEX_NAME_SEPARATORS             (unsigned char *)", "



/* Set the maximum and minimum opensearch relevance score */
#define SRVR_HTTP_OPENSEARCH_RELEVANCE_SCORE_MAX    (1.0)
#define SRVR_HTTP_OPENSEARCH_RELEVANCE_SCORE_MIN    (0.0)




/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrvrHttpHandleSearchIndex (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);

static int iSrvrHttpHandleRetrieveDocument (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);

static int iSrvrHttpHandleServerInfo (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);

static int iSrvrHttpHandleServerIndexInfo (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);

static int iSrvrHttpHandleIndexInfo (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);

static int iSrvrHttpHandleIndexFieldInfo (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);

static int iSrvrHttpHandleIndexTermInfo (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);

static int iSrvrHttpHandleDocumentInfo (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat);



static int iSrvrHttpCheckLoadForRejection (struct srvrServerSession *psssSrvrServerSession, 
        double dLoadMaximum, boolean *pbRejected);



static int iSrvrHttpSendHttpHeader (struct srvrServerSession *psssSrvrServerSession, 
        unsigned int uiHttpStatus, unsigned char *pucContentType, unsigned int uiContentLength);

static int iSrvrHttpSendRssHeader (struct srvrServerSession *psssSrvrServerSession,
        unsigned char *pucPath, unsigned char *pucQuery);

static int iSrvrHttpSendRssFooter (struct srvrServerSession *psssSrvrServerSession);

static int iSrvrHttpSendXmlHeader (struct srvrServerSession *psssSrvrServerSession,
        unsigned char *pucPath, unsigned char *pucQuery);

static int iSrvrHttpSendXmlFooter (struct srvrServerSession *psssSrvrServerSession);

static int iSrvrHttpSendOnHeader (struct srvrServerSession *psssSrvrServerSession,
        unsigned char *pucPath, unsigned char *pucQuery);

static int iSrvrHttpSendOnFooter (struct srvrServerSession *psssSrvrServerSession);



static int iSrvrHttpSendSearchResponseRss (struct srvrServerSession *psssSrvrServerSession,
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        unsigned char *pucSearchText, unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText,
        unsigned int uiStartIndex, unsigned int uiEndIndex,
        struct spiSearchResponse *pssrSpiSearchResponse, unsigned int uiSearchReport, 
        unsigned char **ppucSearchReportList, unsigned int uiSearchReportListLength);

static int iSrvrHttpSendSearchResponseXml (struct srvrServerSession *psssSrvrServerSession,
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        unsigned char *pucSearchText, unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText,
        unsigned int uiStartIndex, unsigned int uiEndIndex,
        struct spiSearchResponse *pssrSpiSearchResponse, unsigned int uiSearchReport, 
        unsigned char **ppucSearchReportList, unsigned int uiSearchReportListLength);

static int iSrvrHttpSendSearchResponseOn (struct srvrServerSession *psssSrvrServerSession,
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        unsigned char *pucSearchText, unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText,
        unsigned int uiStartIndex, unsigned int uiEndIndex,
        struct spiSearchResponse *pssrSpiSearchResponse, unsigned int uiSearchReport, 
        unsigned char **ppucSearchReportList, unsigned int uiSearchReportListLength);


static int iSrvrHttpSendDocumentRss (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        unsigned char *pucTitle, unsigned char *pucDocumentKey, unsigned char *pucItemName, 
        unsigned char *pucMimeType, void *pvData, unsigned int uiDataLength);

static int iSrvrHttpSendDocumentXml (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        unsigned char *pucTitle, unsigned char *pucDocumentKey, unsigned char *pucItemName, 
        unsigned char *pucMimeType, void *pvData, unsigned int uiDataLength);

static int iSrvrHttpSendDocumentOn (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        unsigned char *pucTitle, unsigned char *pucDocumentKey, unsigned char *pucItemName, 
        unsigned char *pucMimeType, void *pvData, unsigned int uiDataLength);

static int iSrvrHttpSendDocumentRaw (struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        unsigned char *pucTitle, unsigned char *pucDocumentKey, unsigned char *pucItemName, 
        unsigned char *pucMimeType, void *pvData, unsigned int uiDataLength);


static int iSrvrHttpSendServerInfoXml(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiServerInfo *pssiSpiServerInfo);

static int iSrvrHttpSendServerInfoOn(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiServerInfo *pssiSpiServerInfo);

static int iSrvrHttpSendServerIndexInfoXml(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiServerIndexInfo *pssiiSpiServerIndexInfos, unsigned int uiSpiServerIndexInfosLength);

static int iSrvrHttpSendServerIndexInfoOn(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiServerIndexInfo *pssiiSpiServerIndexInfos, unsigned int uiSpiServerIndexInfosLength);

static int iSrvrHttpSendIndexInfoXml(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiIndexInfo *psiiSpiIndexInfo);

static int iSrvrHttpSendIndexInfoOn(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiIndexInfo *psiiSpiIndexInfo);

static int iSrvrHttpSendIndexFieldInfoXml(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiFieldInfo *psfiSpiFieldInfos, unsigned int uiSpiFieldInfosLength);

static int iSrvrHttpSendIndexFieldInfoOn(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiFieldInfo *psfiSpiFieldInfos, unsigned int uiSpiFieldInfosLength);

static int iSrvrHttpSendIndexTermInfoXml(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiTermInfo *pstiSpiTermInfos, unsigned int uiSpiTermInfosLength);

static int iSrvrHttpSendIndexTermInfoOn(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiTermInfo *pstiSpiTermInfos, unsigned int uiSpiTermInfosLength);

static int iSrvrHttpSendDocumentInfoXml(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiDocumentInfo *psdiSpiDocumentInfo);

static int iSrvrHttpSendDocumentInfoOn(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucPath, unsigned char *pucQuery, unsigned int uiFormat,
        struct spiDocumentInfo *psdiSpiDocumentInfo);


static int iSrvrHttpSendError (struct srvrServerSession *psssSrvrServerSession,
        unsigned int uiHttpStatus, int iError, unsigned char *pucErrorMessage);

static int iSrvrHttpHandleSpiError (struct srvrServerSession *psssSrvrServerSession,
        int iError, unsigned char *pucIndexName);

static int iSrvrHttpSendf(struct srvrServerSession *psssSrvrServerSession, 
        unsigned char *pucFormat, ...);

static int iSrvrHttpSendEncodeXml(struct srvrServerSession *psssSrvrServerSession, 
        void *pvData, unsigned int uiDataLength);

static int iSrvrHttpSendEncodeOn(struct srvrServerSession *psssSrvrServerSession, 
        void *pvData, unsigned int uiDataLength);



static int iSrvrHttpGetQueryVariableValue (unsigned char *pucQuery, 
        unsigned char *pucVariableName, unsigned char **ppucVariableValue);


static int iSrvrHttpDecodeUrlString (unsigned char *pucString);

static unsigned char *pucSrvrHttpEncodeUrlString (unsigned char *pucString, 
        unsigned char *pucEncodedString, unsigned int uiEncodedStringLength);


static int iSrvrHttpDecodeXmlString (unsigned char *pucString);

static unsigned char *pucSrvrHttpEncodeXmlString (unsigned char *pucString, 
        unsigned char *pucEncodedString, unsigned int uiEncodedStringLength);


static int iSrvrHttpDecodeOnString (unsigned char *pucString);

static unsigned char *pucSrvrHttpEncodeOnString (unsigned char *pucString, 
        unsigned char *pucEncodedString, unsigned int uiEncodedStringLength);



static int iSrvrHttpOpenIndex (struct spiSession *pssSpiSession, 
        unsigned char *pucIndexName, void **ppvIndex);

static int iSrvrHttpCloseIndex (struct spiSession *pssSpiSession, void *pvIndex);

static int iSrvrHttpSearchIndex (struct spiSession *pssSpiSession, unsigned char *pucIndexNames, 
        unsigned char *pucLanguageCode, unsigned char *pucSearchText, 
        unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText, 
        unsigned int uiStartIndex, unsigned int uiEndIndex, 
        struct spiSearchResponse **ppssrSpiSearchResponse);

static int iSrvrHttpRetrieveDocument (struct spiSession *pssSpiSession, unsigned char *pucIndexName, 
        unsigned char *pucDocumentKey, unsigned char *pucItemName, unsigned char *pucMimeType, 
        void **ppvData, unsigned int *puiDataLength);

static int iSrvrHttpGetDocumentInfo (struct spiSession *pssSpiSession, unsigned char *pucIndexName,    
        unsigned char *pucDocumentKey, struct spiDocumentInfo **ppsdiSpiDocumentInfo);



/*---------------------------------------------------------------------------*/


/* 
** ============================================
** ===  HTTP Protocol Decoding and Routing  ===
** ============================================
*/


/*

    Function:   iSrvrProtocolHandlerHttp()

    Purpose:    Serve an HTTP request, this function reads in the request,
                interprets it and sends the reply to the client.

                When this function is called the client has been accepted and 
                the message has been received.

    Parameters: psssSrvrServerSession   server session structure

    Globals:    none

    Returns:    SPI error code

*/
int iSrvrProtocolHandlerHttp
(
    struct srvrServerSession *psssSrvrServerSession
)
{

    int             iError = SPI_NoError;
    unsigned int    uiBufferLength = 0;
    unsigned char   *pucBuffer = NULL;
    unsigned char   pucScanfFormat1[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucScanfFormat2[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   *pucPath = NULL;
    unsigned char   *pucQuery = NULL;
    unsigned char   *pucFormat = NULL;
    unsigned int    uiFormat = SRVR_HTTP_FORMAT_UNKNOWN;


    /* Check the parameters */
    if ( psssSrvrServerSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psssSrvrServerSession' parameter passed to 'iSrvrProtocolHandlerHttp'."); 
        return (SPI_ParameterError);
    }


    /* Get the amount of data currently in the buffer */
    if ( (iError = iUtlNetGetReceiveBufferDataLength(psssSrvrServerSession->pvUtlNet, &uiBufferLength)) != UTL_NoError ) {
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_REQUEST);
        iError = SPI_MiscError;
        goto bailFromiSrvrProtocolHandlerHttp;
    }
    
    
    /* Allocate space for the buffer, the path and the query */
    if ( (pucBuffer = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * (uiBufferLength + 1)))) == NULL ) {
        iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MemError, NULL);
        iError = SPI_MemError;
        goto bailFromiSrvrProtocolHandlerHttp;
    }
    
    if ( (pucPath = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * (uiBufferLength + 1)))) == NULL ) {
        iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MemError, NULL);
        iError = SPI_MemError;
        goto bailFromiSrvrProtocolHandlerHttp;
    }
    
    if ( (pucQuery = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * (uiBufferLength + 1)))) == NULL ) {
        iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MemError, NULL);
        iError = SPI_MemError;
        goto bailFromiSrvrProtocolHandlerHttp;
    }
    
        
    /* Read the request, the client has been accepted and the message has been received */
    if ( (iError = iUtlNetRead(psssSrvrServerSession->pvUtlNet, pucBuffer, uiBufferLength)) != UTL_NoError ) {

        /* Timeout waiting to receive data */
        if ( iError == UTL_NetTimeOut ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "The socket timed out while reading data.");
        }
        /* Report bad errors */
        else if ( iError != UTL_NetSocketClosed ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive a message from the client, utl error: %d.", iError);
        }

        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_REQUEST_TIMEOUT, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_REQUEST);
        iError = SPI_MiscError;
        goto bailFromiSrvrProtocolHandlerHttp;
    }
    
    /* Null terminate the buffer */
    pucBuffer[uiBufferLength] = '\0';

/* printf("pucBuffer: [%s]\n", pucBuffer); */


    /* Check the method and the space after it */
    if ( (s_strncmp(pucBuffer, SRVR_HTTP_METHOD_GET, s_strlen(SRVR_HTTP_METHOD_GET)) != 0) || (*(pucBuffer + s_strlen(SRVR_HTTP_METHOD_GET)) != ' ') ) {
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_METHOD_NOT_ALLOWED, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_METHOD);
        iError = SPI_MiscError;
        goto bailFromiSrvrProtocolHandlerHttp;
    }
    
    
    /* Create the scan formats */
    snprintf(pucScanfFormat1, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "/%%%d[^ ] ", uiBufferLength);
    snprintf(pucScanfFormat2, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "/%%%d[^?]?%%%d[^ ] ", uiBufferLength, uiBufferLength);

    /* Parse out the url */
    if ( sscanf(pucBuffer + s_strlen(SRVR_HTTP_METHOD_GET) + 1, pucScanfFormat2, pucPath, pucQuery) == 2 ) {
/* printf("pucPath: [%s], pucQuery: [%s]\n", pucPath, pucQuery); */
    }
    else if ( sscanf(pucBuffer + s_strlen(SRVR_HTTP_METHOD_GET) + 1, pucScanfFormat1, pucPath) == 1 ) {
/* printf("pucPath: [%s]\n", pucPath); */
    }
    else {
/* printf("pucBuffer: [%s]\n", pucBuffer); */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_PATH);
        iError = SPI_MiscError;
        goto bailFromiSrvrProtocolHandlerHttp;
    }


    /* Free the path and the query here if there is nothing in them */
    if ( bUtlStringsIsStringNULL(pucPath) == true ) {
        s_free(pucPath);
        pucPath = NULL;
    }

    if ( bUtlStringsIsStringNULL(pucQuery) == true ) {
        s_free(pucQuery);
        pucQuery = NULL;
    }


    /* Get the format variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_FORMAT, &pucFormat)) != SPI_NoError ) {
        /* Failed to get the format variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrProtocolHandlerHttp;
    }
/* printf("pucFormat: '%s'\n", pucFormat); */
    
    /* Set the format, default to 'unknown' */
    if ( bUtlStringsIsStringNULL(pucFormat) == true ) {
        uiFormat = SRVR_HTTP_FORMAT_UNKNOWN;
    }
    else if ( s_strcasecmp(pucFormat, SRVR_HTTP_OPTION_RSS) == 0 ) {
        uiFormat = SRVR_HTTP_FORMAT_RSS;
    }
    else if ( s_strcasecmp(pucFormat, SRVR_HTTP_OPTION_XML) == 0 ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( s_strcasecmp(pucFormat, SRVR_HTTP_OPTION_MXG) == 0 ) {
        uiFormat = SRVR_HTTP_FORMAT_MXG;
    }
    else if ( s_strcasecmp(pucFormat, SRVR_HTTP_OPTION_JSON) == 0 ) {
        uiFormat = SRVR_HTTP_FORMAT_JSON;
    }
    else if ( s_strcasecmp(pucFormat, SRVR_HTTP_OPTION_RUBY) == 0 ) {
        uiFormat = SRVR_HTTP_FORMAT_RUBY;
    }
    else if ( s_strcasecmp(pucFormat, SRVR_HTTP_OPTION_PYTHON) == 0 ) {
        uiFormat = SRVR_HTTP_FORMAT_PYTHON;
    }
    else if ( s_strcasecmp(pucFormat, SRVR_HTTP_OPTION_RAW) == 0 ) {
        uiFormat = SRVR_HTTP_FORMAT_RAW;
    }
    else {
        uiFormat = SRVR_HTTP_FORMAT_UNKNOWN;
    }


    /*  Route each method */
    if ( s_strcmp(pucPath, SRVR_HTTP_PATH_SEARCH_INDEX) == 0 ) {
        /* Search index */
        iSrvrHttpHandleSearchIndex(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else if ( s_strcmp(pucPath, SRVR_HTTP_PATH_RETRIEVE_DOCUMENT) == 0 ) {
        /* Retrieve document */
        iSrvrHttpHandleRetrieveDocument(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else if ( s_strcmp(pucPath, SRVR_HTTP_PATH_SERVER_INFO) == 0 ) {
        /* Get the server information */
        iSrvrHttpHandleServerInfo(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else if ( s_strcmp(pucPath, SRVR_HTTP_PATH_SERVER_INDEX_INFO) == 0 ) {
        /* Get the server index information */
        iSrvrHttpHandleServerIndexInfo(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else if ( s_strcmp(pucPath, SRVR_HTTP_PATH_INDEX_INFO) == 0 ) {
        /* Get the index information */
        iSrvrHttpHandleIndexInfo(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else if ( s_strcmp(pucPath, SRVR_HTTP_PATH_INDEX_FIELD_INFO) == 0 ) {
        /* Get the index field information */
        iSrvrHttpHandleIndexFieldInfo(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else if ( s_strcmp(pucPath, SRVR_HTTP_PATH_INDEX_TERM_INFO) == 0 ) {
        /* Get the term information */
        iSrvrHttpHandleIndexTermInfo(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else if ( s_strcmp(pucPath, SRVR_HTTP_PATH_DOCUMENT_INFO) == 0 ) {
        /* Get the document information */
        iSrvrHttpHandleDocumentInfo(psssSrvrServerSession, pucPath, pucQuery, uiFormat);
    }
    else {
        /* Invalid method */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_PATH);
    }



    /* Bail label */
    bailFromiSrvrProtocolHandlerHttp:
    
    
    /* Free data */
    s_free(pucBuffer);
    s_free(pucPath);
    s_free(pucQuery);
    s_free(pucFormat);


    /* And send the message */
    if ( (iError = iUtlNetSend(psssSrvrServerSession->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send a message to the client, utl error: %d.", iError);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* 
** =========================================
** ===  HTTP protocol request functions  ===
** =========================================
*/


/*

    Function:   iSrvrHttpHandleSearchIndex()

    Purpose:    Get and send the index search

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleSearchIndex
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                         iError = SPI_NoError;

    unsigned char               *pucIndexNames = NULL;
    unsigned char               *pucLanguageCode = NULL;
    unsigned char               *pucSearchText = NULL;
    unsigned char               *pucPositiveFeedbackText = NULL;
    unsigned char               *pucNegativeFeedbackText = NULL;
    unsigned char               *pucStartIndex = NULL;
    unsigned char               *pucLimit = NULL;
    unsigned char               *pucSearchReport = NULL;

    unsigned char               pucHostName[MAXHOSTNAMELEN + 1] = {'\0'};
    int                         iPort = -1;

    unsigned int                uiStartIndex = 0;
    unsigned int                uiEndIndex = SRVR_HTTP_DOCUMENTS_TO_RETRIEVE_DEFAULT - 1;

    unsigned char               **ppucSearchReportList = NULL;
    unsigned int                uiSearchReportListLength = 0;

    unsigned int                uiSearchReport = SRVR_HTTP_REPORT_NONE;

    boolean                     bRejected = false;

    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    unsigned int                uiI = 0;



    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_RSS) || (uiFormat == SRVR_HTTP_FORMAT_XML) ||
            (uiFormat == SRVR_HTTP_FORMAT_MXG) || (uiFormat == SRVR_HTTP_FORMAT_JSON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RAW)) ) {
    
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Get the index names variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_INDEX, &pucIndexNames)) != SPI_NoError ) {
        /* Failed to get the index names variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
    
    /* Check the index names */
    if ( bUtlStringsIsStringNULL(pucIndexNames) == true ) {
        /* Failed to get the index names variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_INDEX_NAMES);
        iError = SPI_SearchIndexFailed;
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
/* printf("pucIndexNames: '%s'\n", pucIndexNames); */


    /* Get the language code variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_LANGUAGE, &pucLanguageCode)) != SPI_NoError ) {
        /* Failed to get the language code variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
/* printf("pucLanguageCode: '%s'\n", pucLanguageCode); */


    /* Get the search text variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_SEARCH, &pucSearchText)) != SPI_NoError ) {
        /* Failed to get the search text variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
/* printf("pucSearchText: '%s'\n", pucSearchText); */

    
    /* Get the positive feedback text variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_POSITIVE_FEEDBACK, &pucPositiveFeedbackText)) != SPI_NoError ) {
        /* Failed to get the positive feedback text variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
/* printf("pucPositiveFeedbackText: '%s'\n", pucPositiveFeedbackText); */

    
    /* Get the negative feedback text variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_NEGATIVE_FEEDBACK, &pucNegativeFeedbackText)) != SPI_NoError ) {
        /* Failed to get the negative feedback text variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
/* printf("pucNegativeFeedbackText: '%s'\n", pucNegativeFeedbackText); */


    /* Get the start index variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_START, &pucStartIndex)) != SPI_NoError ) {
        /* Failed to get the start index variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
    else if ( bUtlStringsIsStringNULL(pucStartIndex) == false ) {    

        /* Set the start index */
        uiStartIndex = s_strtol(pucStartIndex, NULL, 10);
/* printf("pucStartIndex: '%s'\n", pucStartIndex); */
    }


    /* Get the limit variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_LIMIT, &pucLimit)) != SPI_NoError ) {
        /* Failed to get the limit variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
    else if ( bUtlStringsIsStringNULL(pucLimit) == false ) {

        /* Convert the limit, 0 means no limit */
        unsigned int    uiLimit = s_strtol(pucLimit, NULL, 10);
        
        /* Set the end index, 0 means no end */
        if ( uiLimit > 0 ) {
            uiEndIndex = (uiStartIndex + uiLimit) - 1;
        }
        else {
            uiEndIndex = 0;
        }
/* printf("pucLimit: %s, uiEndIndex: %u\n", pucLimit, uiEndIndex); */
    }



    /* Get the search report variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_REPORT, &pucSearchReport)) != SPI_NoError ) {
        /* Failed to get the search report variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
    
    /* Set the search report format, default to 'formatted' */
    if ( bUtlStringsIsStringNULL(pucSearchReport) == true ) {
        uiSearchReport = SRVR_HTTP_REPORT_FORMATTED;
    }
    else if ( s_strcasecmp(pucSearchReport, SRVR_HTTP_OPTION_NONE) == 0 ) {
        uiSearchReport = SRVR_HTTP_REPORT_NONE;
    }
    else if ( s_strcasecmp(pucSearchReport, SRVR_HTTP_OPTION_RAW) == 0 ) {
        uiSearchReport = SRVR_HTTP_REPORT_RAW;
    }
    else if ( s_strcasecmp(pucSearchReport, SRVR_HTTP_OPTION_FORMATTED) == 0 ) {
        uiSearchReport = SRVR_HTTP_REPORT_FORMATTED;
    }
    else {
        uiSearchReport = SRVR_HTTP_REPORT_FORMATTED;
    }


    /* Check the current load and reject if the search load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dSearchLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleSearchIndex;
    }

    /* Bail if the search request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleSearchIndex;
    }


    /* Get the host name for this connection */
    iError = iUtlNetGetConnectedHostName(psssSrvrServerSession->pvUtlNet, pucHostName, MAXHOSTNAMELEN + 1);
    
    /* Failed to get the host name from the connection, so we get our host name */
    if ( (iError != UTL_NoError) || (bUtlStringsIsStringNULL(pucHostName) == true) ) {
        iUtlNetGetHostName(pucHostName, MAXHOSTNAMELEN + 1);
    }
    
    /* Failed to get the host name, so we get our host address */
    if ( (iError != UTL_NoError) || (bUtlStringsIsStringNULL(pucHostName) == true) ) {
        iUtlNetGetHostAddress(pucHostName, MAXHOSTNAMELEN + 1);
    }

    /* Get the port for this connection */
    iError = iUtlNetGetConnectedPort(psssSrvrServerSession->pvUtlNet, &iPort);


    /* Run the search */
    if ( (iError = iSrvrHttpSearchIndex(psssSrvrServerSession->pssSpiSession, pucIndexNames, pucLanguageCode, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
            uiStartIndex, uiEndIndex, &pssrSpiSearchResponse)) != SPI_NoError ) {
        
        /* Failed to run the search, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexNames);
        goto bailFromiSrvrHttpHandleSearchIndex;
    }
    else {    

        /* We need to extract the search report first if are to append it to the search results, we do this
        ** first so that we can handle any errors up front before we have sent the search results
        */
        if ( uiSearchReport != SRVR_HTTP_REPORT_NONE ) {
        
            /* Cycle through the search results and send them, they are already in order */
            for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) {

                /* Check that this document is indeed the search report */
                if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
    
                    void            *pvData = NULL;
                    unsigned int    uiDataLength = 0;

                    /* Get the document from the search result structure if it is there */
                    if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pvData != NULL ) {
    
                        /* Duplicate the data */
                        if ( (pvData = s_memdup(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pvData, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->uiDataLength)) == NULL ) {
                            iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MemError, pucIndexNames);
                            iError = SPI_MemError;
                            goto bailFromiSrvrHttpHandleSearchIndex;
                        }
                        
                        /* Hand over the data length */
                        uiDataLength = pssrSpiSearchResultsPtr->psdiSpiDocumentItems->uiDataLength;
                    }
                    /* Otherwise get the document from the server */
                    else {
                
                        if ( (iError = iSrvrHttpRetrieveDocument(psssSrvrServerSession->pssSpiSession, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey,
                                pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, 
                                &pvData, &uiDataLength)) != SPI_NoError ) {
                            /* Failed to run the search, error out */
                            iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexNames);
                            goto bailFromiSrvrHttpHandleSearchIndex;
                        }
                    }
                        
                    /* Add the search report document to the search report array */
                    if ( pvData != NULL ) {
                
                        /* Extend the array */
                        if ( (ppucSearchReportList = (unsigned char **)s_realloc(ppucSearchReportList, (size_t)(sizeof(unsigned char *) * (uiSearchReportListLength + 2)))) == NULL ) {
                            iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MemError, pucIndexNames);
                            iError = SPI_MemError;
                            goto bailFromiSrvrHttpHandleSearchIndex;
                        }
                        
                        /* NULL terminate the data */
                        if ( (iError = iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to NULL terminate data, utl error: %d.", iError);
                            iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MemError, pucIndexNames);
                            iError = SPI_MemError;
                            goto bailFromiSrvrHttpHandleSearchIndex;
                        }

                        /* Set the pointer and NULL terminate the array */
                        ppucSearchReportList[uiSearchReportListLength] = (unsigned char *)pvData;
                        ppucSearchReportList[++uiSearchReportListLength] = NULL;
                    }
                }
            }
        }


        
        /* Write the search response */
        if ( uiFormat == SRVR_HTTP_FORMAT_RSS ) {

            iError = iSrvrHttpSendSearchResponseRss(psssSrvrServerSession, pucPath, pucQuery, uiFormat, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
                uiStartIndex, uiEndIndex, pssrSpiSearchResponse, uiSearchReport, ppucSearchReportList, uiSearchReportListLength);
        }

        else if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {

            iError = iSrvrHttpSendSearchResponseXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
                uiStartIndex, uiEndIndex, pssrSpiSearchResponse, uiSearchReport, ppucSearchReportList, uiSearchReportListLength);
        }
/*
        else if ( uiFormat == SRVR_HTTP_FORMAT_MXG ) {

            iError = iSrvrHttpSendSearchResultsMxg(psssSrvrServerSession, pucPath, pucQuery, uiFormat, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
                uiStartIndex, uiEndIndex, pssrSpiSearchResponse, uiSearchReport, ppucSearchReportList, uiSearchReportListLength);
        }
*/
        else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {

            iError = iSrvrHttpSendSearchResponseOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
                uiStartIndex, uiEndIndex, pssrSpiSearchResponse, uiSearchReport, ppucSearchReportList, uiSearchReportListLength);
        }

        
        /* Failed to output the search results, error out */
        if ( iError != SPI_NoError ) {
            iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexNames);
            goto bailFromiSrvrHttpHandleSearchIndex;
        }

    }
    
    
    
    /* Bail label */
    bailFromiSrvrHttpHandleSearchIndex:
    
    /* Free the search response */
    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;

    /* Free query data */
    s_free(pucIndexNames);
    s_free(pucLanguageCode);
    s_free(pucSearchText);
    s_free(pucPositiveFeedbackText);
    s_free(pucNegativeFeedbackText);
    s_free(pucStartIndex);
    s_free(pucLimit);
    s_free(pucSearchReport);
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleRetrieveDocument()

    Purpose:    Get and send the a index document

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleRetrieveDocument
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                         iError = SPI_NoError;

    unsigned char               *pucIndexName = NULL;
    unsigned char               *pucDocumentKey = NULL;
    unsigned char               *pucItemName = NULL;
    unsigned char               *pucMimeType = NULL;

    boolean                     bRejected = false;

    struct spiDocumentInfo      *psdiSpiDocumentInfo = NULL;

    unsigned char               *pucTitlePtr = NULL;
    void                        *pvData = NULL;
    unsigned int                uiDataLength = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_RSS) || (uiFormat == SRVR_HTTP_FORMAT_XML) ||
            (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || 
            (uiFormat == SRVR_HTTP_FORMAT_PYTHON) || (uiFormat == SRVR_HTTP_FORMAT_RAW)) ) {

        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Get the index name */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_INDEX, &pucIndexName)) != SPI_NoError ) {
        /* Failed to get the index name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }
    
    /* Check the index name */
    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        /* Failed to get the index name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_INDEX_NAME);
        iError = SPI_RetrieveDocumentFailed;
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }
/* printf("pucIndexName: '%s'\n", pucIndexName); */


    /* Get the document key */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_DOCUMENT_KEY, &pucDocumentKey)) != SPI_NoError ) {
        /* Could not get the document key variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }

    /* Check the document key */
    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        /* Could not get the document key variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_DOCUMENT_KEY);
        iError = SPI_RetrieveDocumentFailed;
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }
/* printf("pucDocumentKey: '%s'\n", pucDocumentKey); */


    /* Get the item name */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_ITEM_NAME, &pucItemName)) != SPI_NoError ) {
        /* Could not get the item name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }

    /* Check the item name */
    if ( bUtlStringsIsStringNULL(pucItemName) == true ) {
        /* Could not get the item name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_ITEM_NAME);
        iError = SPI_RetrieveDocumentFailed;
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }
/* printf("pucItemName: '%s'\n", pucItemName); */


    /* Get the mime type */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_MIME_TYPE, &pucMimeType)) != SPI_NoError ) {
        /* Could not get the mime type variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }

    /* Check the mime type */
    if ( bUtlStringsIsStringNULL(pucMimeType) == true ) {
        /* Could not get the mime type variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_MIME_TYPE);
        iError = SPI_RetrieveDocumentFailed;
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }
/* printf("pucMimeType: '%s'\n", pucMimeType); */


    /* Check the current load and reject if the retrieval load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dRetrievalLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }

    /* Bail if the retrieval request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }




    /* Retrieve the document */
    if ( (iError = iSrvrHttpRetrieveDocument(psssSrvrServerSession->pssSpiSession, pucIndexName, pucDocumentKey, pucItemName, pucMimeType, &pvData, &uiDataLength)) != SPI_NoError ) {
        /* Failed to get the document, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        goto bailFromiSrvrHttpHandleRetrieveDocument;
    }
    else {

        /* Format search report */
        if ( (s_strcmp(pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) && (s_strcmp(pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
            
            unsigned char   *pucSearchReportFormatted = NULL;
            unsigned char   *ppucSearchReportList[2] = {NULL, NULL};
            
            /* NULL terminate the data */
            if ( (iError = iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to NULL terminate data, utl error: %d.", iError);
                iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MemError, pucIndexName);
                iError = SPI_MemError;
                goto bailFromiSrvrHttpHandleRetrieveDocument;
            }

            /* Set the array */
            ppucSearchReportList[0] = (unsigned char *)pvData;

            /* Merge and format the search reports */
            if ( (iError = iRepMergeAndFormatSearchReports(ppucSearchReportList, &pucSearchReportFormatted)) != REP_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to merge and format the search reports, rep error: %d.", iError);
                iSrvrHttpHandleSpiError(psssSrvrServerSession, SPI_MiscError, pucIndexName);
                iError = SPI_MiscError;
                goto bailFromiSrvrHttpHandleRetrieveDocument;
            }
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "\n[%s]\n\n\n", pucSearchReportFormatted); */

            /* Free the data */
            s_free(pvData);

            /* Set the title pointer */
            pucTitlePtr = (unsigned char *)"Search Report";

            /* Set the data and data length pointers */
            pvData = (void *)pucSearchReportFormatted;
            uiDataLength = s_strlen(pucSearchReportFormatted);
        }
        else {

            /* Get the document info for the document title */
            if ( (iError = iSrvrHttpGetDocumentInfo(psssSrvrServerSession->pssSpiSession, pucIndexName, pucDocumentKey, &psdiSpiDocumentInfo)) == SPI_NoError ) {
                pucTitlePtr = psdiSpiDocumentInfo->pucTitle;
            }
            else {
                pucTitlePtr = (unsigned char *)"(Document title was unavailable)";
            }
        }


        /* Write the document */
        if ( uiFormat == SRVR_HTTP_FORMAT_RSS ) {
            iSrvrHttpSendDocumentRss(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pucTitlePtr, pucDocumentKey, pucItemName, pucMimeType, pvData, uiDataLength);
        }
        else if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {
            iSrvrHttpSendDocumentXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pucTitlePtr, pucDocumentKey, pucItemName, pucMimeType, pvData, uiDataLength);
        }
        else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {
            iSrvrHttpSendDocumentOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pucTitlePtr, pucDocumentKey, pucItemName, pucMimeType, pvData, uiDataLength);
        }
        else if ( uiFormat == SRVR_HTTP_FORMAT_RAW ) {
            iSrvrHttpSendDocumentRaw(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pucTitlePtr, pucDocumentKey, pucItemName, pucMimeType, pvData, uiDataLength);
        }


        /* Free the document information */
        iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
        psdiSpiDocumentInfo = NULL;

        /* Free the data */
        s_free(pvData);
    }
    
    
    
    /* Bail label */
    bailFromiSrvrHttpHandleRetrieveDocument:

    /* Free data */
    s_free(pucIndexName);
    s_free(pucDocumentKey);
    s_free(pucItemName);
    s_free(pucMimeType);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleServerInfo()

    Purpose:    Get and send the server information

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleServerInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                     iError = SPI_NoError;
    boolean                 bRejected = false;
    struct spiServerInfo    *pssiSpiServerInfo = NULL;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_XML) || (uiFormat == SRVR_HTTP_FORMAT_JSON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON)) ) {
    
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleServerInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleServerInfo;
    }


    /* Get the server information */
    if ( (iError = iSpiGetServerInfo(psssSrvrServerSession->pssSpiSession, &pssiSpiServerInfo)) != SPI_NoError ) {
        /* Failed to get the server information, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, NULL);
        goto bailFromiSrvrHttpHandleServerInfo;
    }
    else {

        /* Write the server info */
        if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {
            iSrvrHttpSendServerInfoXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pssiSpiServerInfo);
        }
        else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {
            iSrvrHttpSendServerInfoOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pssiSpiServerInfo);
        }

        /* Free the server information structure */
        iSpiFreeServerInfo(pssiSpiServerInfo);
        pssiSpiServerInfo = NULL;
    }

    

    /* Bail label */
    bailFromiSrvrHttpHandleServerInfo:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleServerIndexInfo()

    Purpose:    Get and send the server index information

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleServerIndexInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                         iError = SPI_NoError;
    boolean                     bRejected = false;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
    unsigned int                uiSpiServerIndexInfosLength = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_XML) || (uiFormat == SRVR_HTTP_FORMAT_JSON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON)) ) {
    
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleServerIndexInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleServerIndexInfo;
    }


    /* Get the server index information */
    if ( (iError = iSpiGetServerIndexInfo(psssSrvrServerSession->pssSpiSession, &pssiiSpiServerIndexInfos, &uiSpiServerIndexInfosLength)) != SPI_NoError ) {
        /* Failed to get the server index information, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, NULL);
        goto bailFromiSrvrHttpHandleServerIndexInfo;
    }
    else {

        /* Write the server index info */
        if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {
            iSrvrHttpSendServerIndexInfoXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
        }
        else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {
            iSrvrHttpSendServerIndexInfoOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
        }

        /* Free the server index information structure */
        iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
        pssiiSpiServerIndexInfos = NULL;
    }
    
    

    /* Bail label */
    bailFromiSrvrHttpHandleServerIndexInfo:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleIndexInfo()

    Purpose:    Get and send the index information

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleIndexInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                     iError = SPI_NoError;
    unsigned char           *pucIndexName = NULL;
    boolean                 bRejected = false;
    void                    *pvIndex = NULL;
    struct spiIndexInfo     *psiiSpiIndexInfo = NULL;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_XML) || (uiFormat == SRVR_HTTP_FORMAT_JSON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON)) ) {
    
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Get the index name */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_INDEX, &pucIndexName)) != SPI_NoError ) {
        /* Failed to get the index name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleIndexInfo;
    }
    
    /* Check the index name */
    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        /* Failed to get the index names variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_INDEX_NAME);
        iError = SPI_GetIndexInfoFailed;
        goto bailFromiSrvrHttpHandleIndexInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleIndexInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleIndexInfo;
    }


    /* Open the index */
    if ( (iError = iSrvrHttpOpenIndex(psssSrvrServerSession->pssSpiSession, pucIndexName, &pvIndex)) != SPI_NoError ) {
        /* Failed to open the index, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        goto bailFromiSrvrHttpHandleIndexInfo;
    }
    else {

        /* Get the index information */
        if ( (iError = iSpiGetIndexInfo(psssSrvrServerSession->pssSpiSession, pvIndex, &psiiSpiIndexInfo)) != SPI_NoError ) {
            /* Failed to get the index information, error out */
            iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        }
        else {

            /* Write the index info */
            if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {
                iSrvrHttpSendIndexInfoXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                        psiiSpiIndexInfo);
            }
            else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {
                iSrvrHttpSendIndexInfoOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                        psiiSpiIndexInfo);
            }

            /* Free the index information structure */
            iSpiFreeIndexInfo(psiiSpiIndexInfo);
            psiiSpiIndexInfo = NULL;
        }

        /* Close the index */
        iSrvrHttpCloseIndex(psssSrvrServerSession->pssSpiSession, pvIndex);
        pvIndex = NULL;
    }



    /* Bail label */
    bailFromiSrvrHttpHandleIndexInfo:


    /* Free data */
    s_free(pucIndexName);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleIndexFieldInfo()

    Purpose:    Get and send the index field information

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleIndexFieldInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                     iError = SPI_NoError;
    unsigned char           *pucIndexName = NULL;
    boolean                 bRejected = false;
    void                    *pvIndex = NULL;
    struct spiFieldInfo     *psfiSpiFieldInfos = NULL;
    unsigned int            uiSpiFieldInfosLength = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_XML) || (uiFormat == SRVR_HTTP_FORMAT_JSON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON)) ) {
    
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Get the index name */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_INDEX, &pucIndexName)) != SPI_NoError ) {
        /* Failed to get the index name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleIndexFieldInfo;
    }
    
    /* Check the index name */
    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        /* Failed to get the index names variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_INDEX_NAME);
        iError = SPI_GetIndexFieldInfoFailed;
        goto bailFromiSrvrHttpHandleIndexFieldInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleIndexFieldInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleIndexFieldInfo;
    }


    /* Open the index */
    if ( (iError = iSrvrHttpOpenIndex(psssSrvrServerSession->pssSpiSession, pucIndexName, &pvIndex)) != SPI_NoError ) {
        /* Failed to open the index, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        goto bailFromiSrvrHttpHandleIndexFieldInfo;
    }
    else {

        /* Get the index field information */
        if ( (iError = iSpiGetIndexFieldInfo(psssSrvrServerSession->pssSpiSession, pvIndex, &psfiSpiFieldInfos, &uiSpiFieldInfosLength)) != SPI_NoError ) {
            /* Failed to get the index field information, error out */
            iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        }
        else {

            /* Write the index field info */
            if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {
                iSrvrHttpSendIndexFieldInfoXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                        psfiSpiFieldInfos, uiSpiFieldInfosLength);
            }
            else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {
                iSrvrHttpSendIndexFieldInfoOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                        psfiSpiFieldInfos, uiSpiFieldInfosLength);
            }

            /* Free the index field information structure */
            iSpiFreeIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength);
            psfiSpiFieldInfos = NULL;
        }

        /* Close the index */
        iSrvrHttpCloseIndex(psssSrvrServerSession->pssSpiSession, pvIndex);
        pvIndex = NULL;
    }


    
    /* Bail label */
    bailFromiSrvrHttpHandleIndexFieldInfo:


    /* Free data */
    s_free(pucIndexName);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleIndexTermInfo()

    Purpose:    Get and send the term information

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleIndexTermInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                     iError = SPI_NoError;
    unsigned char           *pucIndexName = NULL;
    unsigned char           *pucTermMatch = NULL;
    unsigned int            uiTermMatch = SPI_TERM_MATCH_UNKNOWN;
    unsigned char           *pucTermCase = NULL;
    unsigned int            uiTermCase = SPI_TERM_CASE_UNKNOWN;
    unsigned char           *pucTerm = NULL;
    unsigned char           *pucFieldName = NULL;
    boolean                 bRejected = false;
    void                    *pvIndex = NULL;
    struct spiTermInfo      *pstiSpiTermInfos = NULL;
    unsigned int            uiSpiTermInfosLength = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_XML) || (uiFormat == SRVR_HTTP_FORMAT_JSON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON)) ) {
    
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Get the index name */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_INDEX, &pucIndexName)) != SPI_NoError ) {
        /* Failed to get the index name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }
    
    /* Check the index name */
    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        /* Failed to get the index names variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_INDEX_NAME);
        iError = SPI_GetIndexFieldInfoFailed;
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }


    /* Get the term match variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_TERM_MATCH, &pucTermMatch)) != SPI_NoError ) {
        /* Failed to get the term match variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }
/* printf("pucTermMatch: '%s'\n", pucTermMatch); */
    
    /* Set the term match, default to 'regular' */
    if ( bUtlStringsIsStringNULL(pucTermMatch) == true ) {
        uiTermMatch = SPI_TERM_MATCH_REGULAR;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_REGULAR) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_REGULAR;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_STOP) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_STOP;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_WILDCARD) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_WILDCARD;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_SOUNDEX) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_SOUNDEX;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_METAPHONE) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_METAPHONE;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_PHONIX) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_PHONIX;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_TYPO) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_TYPO;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_MATCH_REGEX) == 0 ) {
        uiTermMatch = SPI_TERM_MATCH_REGEX;
    }
    else {
        uiTermMatch = SPI_TERM_MATCH_REGULAR;
    }


    /* Get the term case variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_TERM_CASE, &pucTermCase)) != SPI_NoError ) {
        /* Failed to get the term case variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }
/* printf("pucTermCase: '%s'\n", pucTermCase); */
    
    /* Set the term case, default to 'sensitive' */
    if ( bUtlStringsIsStringNULL(pucTermMatch) == true ) {
        uiTermCase = SPI_TERM_CASE_SENSITIVE;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_CASE_SENSITIVE) == 0 ) {
        uiTermCase = SPI_TERM_CASE_SENSITIVE;
    }
    else if ( s_strcasecmp(pucTermMatch, SRVR_HTTP_OPTION_TERM_CASE_INSENSITIVE) == 0 ) {
        uiTermCase = SPI_TERM_CASE_INSENSITIVE;
    }
    else {
        uiTermCase = SPI_TERM_CASE_SENSITIVE;
    }


    /* Get the term variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_TERM_CASE, &pucTerm)) != SPI_NoError ) {
        /* Failed to get the term variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }
/* printf("pucTerm: '%s'\n", pucTerm); */
    

    /* Get the field name variable */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_TERM_CASE, &pucFieldName)) != SPI_NoError ) {
        /* Failed to get the field name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }
/* printf("pucFieldName: '%s'\n", pucFieldName); */
    

    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }


    /* Open the index */
    if ( (iError = iSrvrHttpOpenIndex(psssSrvrServerSession->pssSpiSession, pucIndexName, &pvIndex)) != SPI_NoError ) {
        /* Failed to open the index, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        goto bailFromiSrvrHttpHandleIndexTermInfo;
    }
    else {

        /* Get the term information */
        if ( (iError = iSpiGetIndexTermInfo(psssSrvrServerSession->pssSpiSession, pvIndex, uiTermMatch, uiTermCase, pucTerm, 
                pucFieldName, &pstiSpiTermInfos, &uiSpiTermInfosLength)) != SPI_NoError ) {
            /* Failed to get the term information, error out */
            iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        }
        else {

            /* Write the index term info */
            if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {
                iSrvrHttpSendIndexTermInfoXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                        pstiSpiTermInfos, uiSpiTermInfosLength);
            }
            else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {
                iSrvrHttpSendIndexTermInfoOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                        pstiSpiTermInfos, uiSpiTermInfosLength);
            }

            /* Free the term information structure */
            iSpiFreeTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength);
            pstiSpiTermInfos = NULL;
        }

        /* Close the index */
        iSrvrHttpCloseIndex(psssSrvrServerSession->pssSpiSession, pvIndex);
        pvIndex = NULL;
    }
    
    
    
    /* Bail label */
    bailFromiSrvrHttpHandleIndexTermInfo:

    /* Free data */
    s_free(pucIndexName);
    s_free(pucTermMatch);
    s_free(pucTermCase);
    s_free(pucTerm);
    s_free(pucFieldName);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleDocumentInfo()

    Purpose:    Get and send the document information

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpHandleDocumentInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery, 
    unsigned int uiFormat
)
{

    int                         iError = SPI_NoError;
    unsigned char               *pucIndexName = NULL;
    unsigned char               *pucDocumentKey = NULL;
    boolean                     bRejected = false;
    struct spiDocumentInfo      *psdiSpiDocumentInfo = NULL;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(SRVR_HTTP_FORMAT_VALID(uiFormat) || (uiFormat == SRVR_HTTP_FORMAT_UNKNOWN));


    /* Set the format, default to 'xml' */
    if ( uiFormat == SRVR_HTTP_FORMAT_UNKNOWN ) {
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }
    else if ( !((uiFormat == SRVR_HTTP_FORMAT_XML) || (uiFormat == SRVR_HTTP_FORMAT_JSON) ||
            (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON)) ) {
    
        uiFormat = SRVR_HTTP_FORMAT_XML;
    }


    /* Get the index name */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_INDEX, &pucIndexName)) != SPI_NoError ) {
        /* Failed to get the index name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleDocumentInfo;
    }
    
    /* Check the index name */
    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        /* Failed to get the index name variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_INDEX_NAME);
        iError = SPI_GetDocumentInfoFailed;
        goto bailFromiSrvrHttpHandleDocumentInfo;
    }
/* printf("pucIndexName: '%s'\n", pucIndexName); */


    /* Get the document key */
    if ( (iError = iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_DOCUMENT_KEY, &pucDocumentKey)) != SPI_NoError ) {
        /* Could not get the document key variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_INVALID_QUERY);
        goto bailFromiSrvrHttpHandleDocumentInfo;
    }

    /* Check the document key */
    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        /* Could not get the document key variable, return an error */
        iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_BAD_REQUEST, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_MISSING_DOCUMENT_KEY);
        iError = SPI_GetDocumentInfoFailed;
        goto bailFromiSrvrHttpHandleDocumentInfo;
    }
/* printf("pucDocumentKey: '%s'\n", pucDocumentKey); */


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrHttpCheckLoadForRejection(psssSrvrServerSession, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != SPI_NoError ) {
        goto bailFromiSrvrHttpHandleDocumentInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        iError = SPI_ExceededLoadMaximum;
        goto bailFromiSrvrHttpHandleDocumentInfo;
    }


    /* Get the document information */
    if ( (iError = iSrvrHttpGetDocumentInfo(psssSrvrServerSession->pssSpiSession, pucIndexName, pucDocumentKey, &psdiSpiDocumentInfo)) != SPI_NoError ) {
        /* Failed to get the document information, error out */
        iSrvrHttpHandleSpiError(psssSrvrServerSession, iError, pucIndexName);
        goto bailFromiSrvrHttpHandleDocumentInfo;
    }
    else {

        /* Write the document info */
        if ( uiFormat == SRVR_HTTP_FORMAT_XML ) {
            iSrvrHttpSendDocumentInfoXml(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    psdiSpiDocumentInfo);
        }
        else if ( (uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON) ) {
            iSrvrHttpSendDocumentInfoOn(psssSrvrServerSession, pucPath, pucQuery, uiFormat, 
                    psdiSpiDocumentInfo);
        }

        /* Free the document information structure */
        iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
        psdiSpiDocumentInfo = NULL;
    }


    
    /* Bail label */
    bailFromiSrvrHttpHandleDocumentInfo:

    
    /* Free data */
    s_free(pucIndexName);
    s_free(pucDocumentKey);
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpCheckLoadForRejection()

    Purpose:    Checks whether the load maximum was reached and reject the
                request if so.

    Parameters: psssSrvrServerSession   server session structure
                dLoadMaximum            load maximum
                pbRejected              return pointer indicating whether
                                        the connection was rejected

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpCheckLoadForRejection
(
    struct srvrServerSession *psssSrvrServerSession,
    double dLoadMaximum,
    boolean *pbRejected
)
{
    
    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(pbRejected != NULL);


    /* Initially the connection is not rejected */
    *pbRejected = false;


    /* Check if the load maximum was exceeded */
    if ( dLoadMaximum > 0 ) {
        
        double  dCurrentLoad = -1;

        /* Get the current 1 minute load average and check that against the load maximum */
        if ( (iUtlLoadGetAverages(&dCurrentLoad, NULL, NULL) == UTL_NoError) && (dCurrentLoad > dLoadMaximum) ) {

            iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_SERVICE_UNAVAILABLE, SRVR_HTTP_ERROR_DEFAULT, SRVR_HTTP_MESSAGE_LOAD_EXCEEDED);
            iUtlLogWarn(UTL_LOG_CONTEXT, "Rejecting a client request because the server load is too high, current load: %.2f, maximum load: %.2f.", 
                            dCurrentLoad, dLoadMaximum);
            
            /* The connection was rejected */
            *pbRejected = true;
        }
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* 
** ==================================
** ===  Output support functions  ===
** ==================================
*/


/*

    Function:   iSrvrHttpSendHttpHeader()

    Purpose:    Send an http response header

    Parameters: spiServerHandle     spi session structure
                uiHttpStatus        http status
                pucContentType      content type (optional)
                uiContentLength     content length

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendHttpHeader
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned int uiHttpStatus,
    unsigned char *pucContentType,
    unsigned int uiContentLength
)
{

    unsigned char   pucDate[SRVR_HTTP_SHORT_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);


    /* Get the current date */
    iUtlDateGetWebDateFromTime(s_time(NULL), pucDate, SRVR_HTTP_SHORT_STRING_LENGTH + 1);


    /* Send the http header */
    iSrvrHttpSendf(psssSrvrServerSession, "HTTP/1.1 %d OK \n", uiHttpStatus);
    iSrvrHttpSendf(psssSrvrServerSession, "Date: %s \n", pucDate);
    iSrvrHttpSendf(psssSrvrServerSession, "Server: MPS Information Server, version: %u.%u.%u\n", UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH);
    iSrvrHttpSendf(psssSrvrServerSession, "Last-Modified: %s \n", pucDate);
    iSrvrHttpSendf(psssSrvrServerSession, "Connection: close \n");
    iSrvrHttpSendf(psssSrvrServerSession, "Content-Type: %s \n", (bUtlStringsIsStringNULL(pucContentType) == false) ? pucContentType : (unsigned char *)"text/xml; charset=utf-8");
    if ( uiContentLength > 0 ) {
        iSrvrHttpSendf(psssSrvrServerSession, "Content-Length: %u \n", uiContentLength);
    }
    iSrvrHttpSendf(psssSrvrServerSession, "\n");


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendRssHeader()

    Purpose:    Send an RSS response header

    Parameters: spiServerHandle     spi session structure
                pucPath             path
                pucQuery            query

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendRssHeader
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery
)
{

    int             iError = SPI_NoError;

    unsigned char   *pucExtensions = NULL;
    unsigned int    uiExtensions = SRVR_HTTP_EXTENSION_NONE;

    unsigned char   pucHostName[MAXHOSTNAMELEN + 1] = {'\0'};
    int             iPort = 0;

    unsigned char   pucDate[SRVR_HTTP_SHORT_STRING_LENGTH + 1] = {'\0'};

    unsigned char   pucString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);


    /* Get the extensions variable */
    iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_EXTENSIONS, &pucExtensions);
    
    /* Set the extensions */
    if ( bUtlStringsIsStringNULL(pucExtensions) == true ) {
        uiExtensions = SRVR_HTTP_EXTENSION_NONE;
    }
    else {
        if ( s_strcasestr(pucExtensions, SRVR_HTTP_OPTION_MPS) != NULL ) {
            vSrvrHttpExtensionMpsOn(uiExtensions);
        }
        if ( s_strcasestr(pucExtensions, SRVR_HTTP_OPTION_OPENSEARCH) != NULL ) {
            vSrvrHttpExtensionOpenSearchOn(uiExtensions);
        }
    }
    

    /* Get the host name for this connection */
    iError = iUtlNetGetConnectedHostName(psssSrvrServerSession->pvUtlNet, pucHostName, MAXHOSTNAMELEN + 1);
    
    /* Failed to get the host name from the connection, so we get our host name */
    if ( (iError != UTL_NoError) || (bUtlStringsIsStringNULL(pucHostName) == true) ) {
        iUtlNetGetHostName(pucHostName, MAXHOSTNAMELEN + 1);
    }
    
    /* Failed to get the host name, so we get our host address */
    if ( (iError != UTL_NoError) || (bUtlStringsIsStringNULL(pucHostName) == true) ) {
        iUtlNetGetHostAddress(pucHostName, MAXHOSTNAMELEN + 1);
    }

    /* Get the port for this connection */
    iError = iUtlNetGetConnectedPort(psssSrvrServerSession->pvUtlNet, &iPort);


    /* Get the current date */
    iUtlDateGetWebDateFromTime(s_time(NULL), pucDate, SRVR_HTTP_SHORT_STRING_LENGTH + 1);

    /* Send the xml header */
    iSrvrHttpSendf(psssSrvrServerSession, "<?xml version=\"1.0\" encoding=\"utf-8\"?> \n");


    /* Send the RSS header */
    iSrvrHttpSendf(psssSrvrServerSession, "<rss version=\"2.0\"");

    /* Send the MPS specific extensions */
    if ( bSrvrHttpExtensionMps(uiExtensions) == true ) {
        iSrvrHttpSendf(psssSrvrServerSession, " xmlns:mps=\"http://fsconsult.com/mps/ext/1.0\"");
    }

    /* Send the OpenSearch specific extensions */
    if ( bSrvrHttpExtensionOpenSearch(uiExtensions) == true ) {
        iSrvrHttpSendf(psssSrvrServerSession, " xmlns:opensearch=\"http://a9.com/-/spec/opensearch/1.1/\"");
        iSrvrHttpSendf(psssSrvrServerSession, " xmlns:relevance=\"http://a9.com/-/opensearch/extensions/relevance/1.0/\"");
    }
    
    /* Close off the RSS header */
    iSrvrHttpSendf(psssSrvrServerSession, ">\n");


    iSrvrHttpSendf(psssSrvrServerSession, "<channel>\n");
    iSrvrHttpSendf(psssSrvrServerSession, "<title>MPS Search on: </title>\n");
    
    if ( (bUtlStringsIsStringNULL(pucPath) == false) && (bUtlStringsIsStringNULL(pucQuery) == false) ) { 
        snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s", pucHostName, iPort, pucPath, pucQuery);
    }
    else if ( bUtlStringsIsStringNULL(pucPath) == false ) {
        snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s", pucHostName, iPort, pucPath);
    }
    else {
        snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/", pucHostName, iPort);
    }
    iSrvrHttpSendf(psssSrvrServerSession, "<link>%s</link>\n", pucSrvrHttpEncodeUrlString(pucString, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

    iSrvrHttpSendf(psssSrvrServerSession, "<description>MPS Search on: </description>\n");
    iSrvrHttpSendf(psssSrvrServerSession, "<pubDate>%s</pubDate>\n", pucSrvrHttpEncodeXmlString(pucDate, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<lastBuildDate>%s</lastBuildDate>\n", pucSrvrHttpEncodeXmlString(pucDate, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

    iSrvrHttpSendf(psssSrvrServerSession, "<generator>MPS Information Server, version: %u.%u.%u</generator>\n", UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH);

    iSrvrHttpSendf(psssSrvrServerSession, "<language>en</language>\n");



    /* Free allocations */
    s_free(pucExtensions);


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendRssFooter()

    Purpose:    Send an RSS footer

    Parameters: spiServerHandle     spi session structure

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendRssFooter
(
    struct srvrServerSession *psssSrvrServerSession
)
{

    ASSERT(psssSrvrServerSession != NULL);


    /* Send the footer */
    iSrvrHttpSendf(psssSrvrServerSession, "</channel>\n</rss>\n");


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendXmlHeader()

    Purpose:    Send an XML response header

    Parameters: spiServerHandle     spi session structure
                pucPath             path
                pucQuery            query

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendXmlHeader
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery
)
{


    ASSERT(psssSrvrServerSession != NULL);


    /* Send the xml header */
    iSrvrHttpSendf(psssSrvrServerSession, "<?xml version=\"1.0\" encoding=\"utf-8\"?> \n");

    /* Send MPS xml header */
    iSrvrHttpSendf(psssSrvrServerSession, "<mps version=\"1.0\" xmlns:mps=\"http://fsconsult.com/mps/ext/1.0\">\n");


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendXmlFooter()

    Purpose:    Send an XML footer

    Parameters: spiServerHandle     spi session structure

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendXmlFooter
(
    struct srvrServerSession *psssSrvrServerSession
)
{

    ASSERT(psssSrvrServerSession != NULL);


    /* Send the footer */
    iSrvrHttpSendf(psssSrvrServerSession, "</mps>\n");


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendOnHeader()

    Purpose:    Send an Object Notation response header

    Parameters: spiServerHandle     spi session structure
                pucPath             path
                pucQuery            query

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendOnHeader
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery
)
{


    ASSERT(psssSrvrServerSession != NULL);


    /* Send the header */
    iSrvrHttpSendf(psssSrvrServerSession, "{\n");

    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendOnFooter()

    Purpose:    Send an Object Notation footer, same footer for Ruby and Python

    Parameters: spiServerHandle     spi session structure

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendOnFooter
(
    struct srvrServerSession *psssSrvrServerSession
)
{

    ASSERT(psssSrvrServerSession != NULL);


    /* Send the footer */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendSearchResponseRss()

    Purpose:    Send the search response in RSS.

    Parameters: psssSrvrServerSession       server session structure
                pucPath                     path
                pucQuery                    query
                uiFormat                    format
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                pssrSpiSearchResponse       search response
                uiSearchReport              search report
                ppucSearchReportList        search report list
                uiSearchReportListLength    search report list length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendSearchResponseRss
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex, 
    unsigned int uiEndIndex,
    struct spiSearchResponse *pssrSpiSearchResponse,
    unsigned int uiSearchReport,
    unsigned char **ppucSearchReportList,
    unsigned int uiSearchReportListLength
)
{

    int                         iError = SPI_NoError;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;

    unsigned char               *pucExtensions = NULL;
    unsigned int                uiExtensions = SRVR_HTTP_EXTENSION_NONE;

    unsigned char               pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};

    unsigned char               pucHostName[MAXHOSTNAMELEN + 1] = {'\0'};
    int                         iPort = -1;

    unsigned char               pucDate[SRVR_HTTP_SHORT_STRING_LENGTH + 1] = {'\0'};

    unsigned char               *pucSearchReportFormatted = NULL;
    unsigned int                uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_RSS);
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(pssrSpiSearchResponse != NULL);
    ASSERT(((pssrSpiSearchResponse->pssrSpiSearchResults != NULL) && (pssrSpiSearchResponse->uiSpiSearchResultsLength >= 0)) || 
            ((pssrSpiSearchResponse->pssrSpiSearchResults == NULL) && (pssrSpiSearchResponse->uiSpiSearchResultsLength == 0)));
    ASSERT(pssrSpiSearchResponse->uiTotalResults >= 0);
    ASSERT(pssrSpiSearchResponse->uiStartIndex >= 0);
    ASSERT(pssrSpiSearchResponse->uiEndIndex >= 0);
    ASSERT((SPI_SORT_TYPE_VALID(pssrSpiSearchResponse->uiSortType) == true) || (pssrSpiSearchResponse->uiSortType == SPI_SORT_TYPE_UNKNOWN));
    ASSERT(pssrSpiSearchResponse->dMaxSortKey >= 0);
    ASSERT(pssrSpiSearchResponse->dSearchTime >= 0);
    ASSERT(SRVR_HTTP_REPORT_VALID(uiSearchReport) == true);
    ASSERT(((ppucSearchReportList != NULL) && (uiSearchReportListLength >= 0)) || ((ppucSearchReportList == NULL) && (uiSearchReportListLength == 0)));


    /* Get the extensions variable */
    iSrvrHttpGetQueryVariableValue(pucQuery, SRVR_HTTP_PARAMETER_EXTENSIONS, &pucExtensions);
    
    /* Set the extensions */
    if ( bUtlStringsIsStringNULL(pucExtensions) == true ) {
        uiExtensions = SRVR_HTTP_EXTENSION_NONE;
    }
    else {
        if ( s_strcasestr(pucExtensions, SRVR_HTTP_OPTION_MPS) != NULL ) {
            vSrvrHttpExtensionMpsOn(uiExtensions);
        }
        if ( s_strcasestr(pucExtensions, SRVR_HTTP_OPTION_OPENSEARCH) != NULL ) {
            vSrvrHttpExtensionOpenSearchOn(uiExtensions);
        }
    }


    /* Get the host name for this connection */
    iError = iUtlNetGetConnectedHostName(psssSrvrServerSession->pvUtlNet, pucHostName, MAXHOSTNAMELEN + 1);
    
    /* Failed to get the host name from the connection, so we get our host name */
    if ( (iError != UTL_NoError) || (bUtlStringsIsStringNULL(pucHostName) == true) ) {
        iUtlNetGetHostName(pucHostName, MAXHOSTNAMELEN + 1);
    }
    
    /* Failed to get the host name, so we get our host address */
    if ( (iError != UTL_NoError) || (bUtlStringsIsStringNULL(pucHostName) == true) ) {
        iUtlNetGetHostAddress(pucHostName, MAXHOSTNAMELEN + 1);
    }

    /* Get the port for this connection */
    iError = iUtlNetGetConnectedPort(psssSrvrServerSession->pvUtlNet, &iPort);



    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RSS, 0);

    /* Send the rss header */
    iSrvrHttpSendRssHeader(psssSrvrServerSession, pucPath, pucQuery);


    /* Send the MPS specific extensions */
    if ( bSrvrHttpExtensionMps(uiExtensions) == true ) {

        iSrvrHttpSendf(psssSrvrServerSession, "<mps:search>%s</mps:search>\n", pucSrvrHttpEncodeXmlString(pucSearchText, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        iSrvrHttpSendf(psssSrvrServerSession, "<mps:totalResults>%u</mps:totalResults>\n", pssrSpiSearchResponse->uiTotalResults);
        iSrvrHttpSendf(psssSrvrServerSession, "<mps:startIndex>%u</mps:startIndex>\n", pssrSpiSearchResponse->uiStartIndex);
        iSrvrHttpSendf(psssSrvrServerSession, "<mps:endIndex>%u</mps:endIndex>\n", pssrSpiSearchResponse->uiEndIndex);

        /* Send the sort type */
        iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortType>");

        switch ( pssrSpiSearchResponse->uiSortType ) {
        
            case SPI_SORT_TYPE_DOUBLE_ASC:
                iSrvrHttpSendf(psssSrvrServerSession, "double:asc");
                break;
            
            case SPI_SORT_TYPE_DOUBLE_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "double:desc");
                break;
            
            case SPI_SORT_TYPE_FLOAT_ASC:
                iSrvrHttpSendf(psssSrvrServerSession, "float:asc");
                break;
            
            case SPI_SORT_TYPE_FLOAT_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "float:desc");
                break;
            
            case SPI_SORT_TYPE_UINT_ASC:
                iSrvrHttpSendf(psssSrvrServerSession, "uint:asc");
                break;
            
            case SPI_SORT_TYPE_UINT_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "uint:desc");
                break;
            
            case SPI_SORT_TYPE_ULONG_ASC:
                iSrvrHttpSendf(psssSrvrServerSession, "ulong:asc");
                break;
            
            case SPI_SORT_TYPE_ULONG_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "ulong:desc");
                break;
            
            case SPI_SORT_TYPE_UCHAR_ASC:
                iSrvrHttpSendf(psssSrvrServerSession, "char:asc");
                break;
            
            case SPI_SORT_TYPE_UCHAR_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "char:desc");
                break;
            
            case SPI_SORT_TYPE_NO_SORT:
                iSrvrHttpSendf(psssSrvrServerSession, "nosort");
                break;
            
            case SPI_SORT_TYPE_UNKNOWN:
                iSrvrHttpSendf(psssSrvrServerSession, "unknown");
                break;
            
            default:
                iSrvrHttpSendf(psssSrvrServerSession, "invalid");
                break;
        }

        iSrvrHttpSendf(psssSrvrServerSession, "</mps:sortType>\n");

        iSrvrHttpSendf(psssSrvrServerSession, "<mps:maxSortKey>%.6f</mps:maxSortKey>\n", pssrSpiSearchResponse->dMaxSortKey);
        iSrvrHttpSendf(psssSrvrServerSession, "<mps:searchTime>%.0f</mps:searchTime>\n", pssrSpiSearchResponse->dSearchTime);
    }

    /* Send the OpenSearch specific extensions */
    if ( bSrvrHttpExtensionOpenSearch(uiExtensions) == true ) {

        iSrvrHttpSendf(psssSrvrServerSession, "<opensearch:totalResults>%d</opensearch:totalResults>\n", pssrSpiSearchResponse->uiTotalResults);
        iSrvrHttpSendf(psssSrvrServerSession, "<opensearch:startIndex>%d</opensearch:startIndex>\n", pssrSpiSearchResponse->uiStartIndex);
        iSrvrHttpSendf(psssSrvrServerSession, "<opensearch:itemsPerPage>%d</opensearch:itemsPerPage>\n", (uiEndIndex - uiStartIndex) + 1);
        iSrvrHttpSendf(psssSrvrServerSession, "<opensearch:query role=\"request\" searchTerms=\"%s\" />\n", 
                pucSrvrHttpEncodeXmlString(pucSearchText, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    }


    /* Cycle through the search results and send them, they are already in order */
    for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) {

        unsigned char    pucString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


        /* Skip the search report, it gets appended as an item */
        if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {

            continue;
        }


        /* Open the item */
        iSrvrHttpSendf(psssSrvrServerSession, "<item>\n");


        /* Title */
        iSrvrHttpSendf(psssSrvrServerSession, "<title>%s</title>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        

        /* Description, just grab the first document item/mime type */
        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {
            
            void            *pvData = NULL;
            unsigned int    uiDataLength = 0;
        
            if ( (iError = iSrvrHttpRetrieveDocument(psssSrvrServerSession->pssSpiSession, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, 
                    pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, 
                    &pvData, &uiDataLength)) == SPI_NoError ) {

                iSrvrHttpSendf(psssSrvrServerSession, "<description>");
        
                /* Send the data */
                if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, (unsigned char *)pvData, uiDataLength)) != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
                    iError = SPI_SearchIndexFailed;
                    goto bailFromiSrvrHttpSendSearchResultsRss;
                }

                iSrvrHttpSendf(psssSrvrServerSession, "</description>\n");
                
                /* Free the data */
                s_free(pvData);
            }
            
            /* Reset the error */
            iError = SPI_NoError;
        }


        /* Publication date */
        if ( pssrSpiSearchResultsPtr->ulAnsiDate != 0 ) {
            iUtlDateGetWebDateFromAnsiDate(pssrSpiSearchResultsPtr->ulAnsiDate, pucDate, SRVR_HTTP_SHORT_STRING_LENGTH + 1);
            iSrvrHttpSendf(psssSrvrServerSession, "<pubDate>%s</pubDate>\n", pucSrvrHttpEncodeXmlString(pucDate, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        }            
        

        /* Link */
        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {

            snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s=%s&%s=%s&%s=%s&%s=%s&%s=%s", 
                    pucHostName, iPort,
                    SRVR_HTTP_PATH_RETRIEVE_DOCUMENT, SRVR_HTTP_PARAMETER_INDEX, pssrSpiSearchResultsPtr->pucIndexName, SRVR_HTTP_PARAMETER_DOCUMENT_KEY, pssrSpiSearchResultsPtr->pucDocumentKey,
                    SRVR_HTTP_PARAMETER_ITEM_NAME, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SRVR_HTTP_PARAMETER_MIME_TYPE, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType,
                    SRVR_HTTP_PARAMETER_FORMAT, SRVR_HTTP_OPTION_RAW);

            iSrvrHttpSendf(psssSrvrServerSession, "<link>%s</link>\n", pucSrvrHttpEncodeUrlString(pucString, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        }
        else {

            snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s=%s&%s=%s&%s=%s", 
                    pucHostName, iPort,
                    SRVR_HTTP_PATH_RETRIEVE_DOCUMENT, SRVR_HTTP_PARAMETER_INDEX, pssrSpiSearchResultsPtr->pucIndexName, SRVR_HTTP_PARAMETER_DOCUMENT_KEY, pssrSpiSearchResultsPtr->pucDocumentKey,
                    SRVR_HTTP_PARAMETER_FORMAT, SRVR_HTTP_OPTION_RAW);

            iSrvrHttpSendf(psssSrvrServerSession, "<link>%s</link>\n", pucSrvrHttpEncodeUrlString(pucString, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        }

        
        /* Guid */
        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {

            snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s=%s&%s=%s&%s=%s&%s=%s&%s=%s", 
                    pucHostName, iPort,
                    SRVR_HTTP_PATH_RETRIEVE_DOCUMENT, SRVR_HTTP_PARAMETER_INDEX, pssrSpiSearchResultsPtr->pucIndexName, SRVR_HTTP_PARAMETER_DOCUMENT_KEY, pssrSpiSearchResultsPtr->pucDocumentKey,
                    SRVR_HTTP_PARAMETER_ITEM_NAME, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SRVR_HTTP_PARAMETER_MIME_TYPE, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType,
                    SRVR_HTTP_PARAMETER_FORMAT, SRVR_HTTP_OPTION_RAW);

            iSrvrHttpSendf(psssSrvrServerSession, "<guid isPermaLink=\"true\">%s</guid>\n", pucSrvrHttpEncodeUrlString(pucString, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        }
        else {

            snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s=%s&%s=%s&%s=%s", 
                    pucHostName, iPort,
                    SRVR_HTTP_PATH_RETRIEVE_DOCUMENT, SRVR_HTTP_PARAMETER_INDEX, pssrSpiSearchResultsPtr->pucIndexName, SRVR_HTTP_PARAMETER_DOCUMENT_KEY, pssrSpiSearchResultsPtr->pucDocumentKey,
                    SRVR_HTTP_PARAMETER_FORMAT, SRVR_HTTP_OPTION_RAW);

            iSrvrHttpSendf(psssSrvrServerSession, "<guid isPermaLink=\"true\">%s</guid>\n", pucSrvrHttpEncodeUrlString(pucString, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        }


        /* Enclosures */
        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {

            unsigned int                uiJ = 0;
            struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;

            /* Loop over each document item */
            for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {

                unsigned char   *pucUrlPtr = NULL;

                /* Use the url from the document items if present, otherwise contruct a retrieval url */
                if ( bUtlStringsIsStringNULL(psdiSpiDocumentItemsPtr->pucUrl) == false ) {
                    pucUrlPtr = psdiSpiDocumentItemsPtr->pucUrl;
                }
                else {
                    snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s=%s&%s=%s&%s=%s&%s=%s", 
                            pucHostName, iPort,
                            SRVR_HTTP_PATH_RETRIEVE_DOCUMENT, SRVR_HTTP_PARAMETER_INDEX, pssrSpiSearchResultsPtr->pucIndexName, SRVR_HTTP_PARAMETER_DOCUMENT_KEY, pssrSpiSearchResultsPtr->pucDocumentKey,
                            SRVR_HTTP_PARAMETER_ITEM_NAME, psdiSpiDocumentItemsPtr->pucItemName, SRVR_HTTP_PARAMETER_MIME_TYPE, psdiSpiDocumentItemsPtr->pucMimeType);
                    pucUrlPtr = pucString;
                }

                iSrvrHttpSendf(psssSrvrServerSession, "<enclosure url=\"%s\" type=\"%s\" length=\"%d\" />\n", pucSrvrHttpEncodeUrlString(pucUrlPtr, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), 
                        psdiSpiDocumentItemsPtr->pucMimeType, psdiSpiDocumentItemsPtr->uiLength);
            } 
        }


        /* Source */
/*         snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s", pucHostName, iPort, pucPath, pucQuery); */
/*         iSrvrHttpSendf(psssSrvrServerSession, "<source>%s</source>\n", pucSrvrHttpEncodeUrlString(pucString, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1)); */


        /* Send the MPS specific extensions */
        if ( bSrvrHttpExtensionMps(uiExtensions) == true ) {

            iSrvrHttpSendf(psssSrvrServerSession, "<mps:indexName>%s</mps:indexName>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucIndexName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
            iSrvrHttpSendf(psssSrvrServerSession, "<mps:documentKey>%s</mps:documentKey>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucDocumentKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
            iSrvrHttpSendf(psssSrvrServerSession, "<mps:languageCode>%s</mps:languageCode>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucLanguageCode, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
            iSrvrHttpSendf(psssSrvrServerSession, "<mps:rank>%u</mps:rank>\n", pssrSpiSearchResultsPtr->uiRank);
            iSrvrHttpSendf(psssSrvrServerSession, "<mps:termCount>%u</mps:termCount>\n", pssrSpiSearchResultsPtr->uiTermCount);

            /* Print the date and time */
            if ( pssrSpiSearchResultsPtr->ulAnsiDate != 0 ) {
                iSrvrHttpSendf(psssSrvrServerSession, "<mps:ansiDate>%lu</mps:ansiDate>\n", pssrSpiSearchResultsPtr->ulAnsiDate);
            }


            /* Print the sort key */
            switch ( pssrSpiSearchResponse->uiSortType ) {
    
                case SPI_SORT_TYPE_DOUBLE_ASC:
                case SPI_SORT_TYPE_DOUBLE_DESC:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>%.4f</mps:sortKey>\n", pssrSpiSearchResultsPtr->dSortKey);
                    break;
    
                case SPI_SORT_TYPE_FLOAT_ASC:
                case SPI_SORT_TYPE_FLOAT_DESC:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>%.4f</mps:sortKey>\n", pssrSpiSearchResultsPtr->fSortKey);
                    break;
                
                case SPI_SORT_TYPE_UINT_ASC:
                case SPI_SORT_TYPE_UINT_DESC:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>%u</mps:sortKey>\n", pssrSpiSearchResultsPtr->uiSortKey);
                    break;
                
                case SPI_SORT_TYPE_ULONG_ASC:
                case SPI_SORT_TYPE_ULONG_DESC:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>%lu</mps:sortKey>\n", pssrSpiSearchResultsPtr->ulSortKey);
                    break;
                
                case SPI_SORT_TYPE_UCHAR_ASC:
                case SPI_SORT_TYPE_UCHAR_DESC:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>%s</mps:sortKey>\n", 
                            pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucSortKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
                    break;
                
                case SPI_SORT_TYPE_NO_SORT:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>none</mps:sortKey>\n");
                    break;
    
                case SPI_SORT_TYPE_UNKNOWN:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>unknown</mps:sortKey>\n");
                    break;
                
                default:
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:sortKey>invalid</mps:sortKey>\n");
                    break;
            }

            /* Document items */
            if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {
    
                unsigned int                uiJ = 0;
                struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
    
                /* Loop over each document item */
                for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {
    
                    unsigned char   *pucUrlPtr = NULL;
    
                    /* Use the url from the document items if present, otherwise contruct a retrieval url */
                    if ( bUtlStringsIsStringNULL(psdiSpiDocumentItemsPtr->pucUrl) == false ) {
                        pucUrlPtr = psdiSpiDocumentItemsPtr->pucUrl;
                    }
                    else {
                        snprintf(pucString, SRVR_HTTP_LONG_STRING_LENGTH + 1, "http://%s:%d/%s?%s=%s&%s=%s&%s=%s&%s=%s", 
                                pucHostName, iPort, SRVR_HTTP_PATH_RETRIEVE_DOCUMENT, SRVR_HTTP_PARAMETER_INDEX, pssrSpiSearchResultsPtr->pucIndexName,
                                SRVR_HTTP_PARAMETER_DOCUMENT_KEY, pssrSpiSearchResultsPtr->pucDocumentKey,
                                SRVR_HTTP_PARAMETER_ITEM_NAME, psdiSpiDocumentItemsPtr->pucItemName, SRVR_HTTP_PARAMETER_MIME_TYPE, psdiSpiDocumentItemsPtr->pucMimeType);
                        
                        pucUrlPtr = pucString;
                    }
    
                    iSrvrHttpSendf(psssSrvrServerSession, "<mps:documentItem itemName=\"%s\" mimeType=\"%s\" length=\"%d\" url=\"%s\" />\n", 
                            psdiSpiDocumentItemsPtr->pucItemName, psdiSpiDocumentItemsPtr->pucMimeType, psdiSpiDocumentItemsPtr->uiLength,
                            pucSrvrHttpEncodeUrlString(pucString, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
                }
            }
        }

        /* Send the OpenSearch specific extensions */
        if ( bSrvrHttpExtensionOpenSearch(uiExtensions) == true ) {
        
            double  dNormalizedSortKey = 0.0;

            /* Print the sort key */
            switch ( pssrSpiSearchResponse->uiSortType ) {
    
                case SPI_SORT_TYPE_DOUBLE_ASC:
                case SPI_SORT_TYPE_DOUBLE_DESC:
                    dNormalizedSortKey = ((double)pssrSpiSearchResultsPtr->dSortKey / pssrSpiSearchResponse->dMaxSortKey);
                    break;
    
                case SPI_SORT_TYPE_FLOAT_ASC:
                case SPI_SORT_TYPE_FLOAT_DESC:
                    dNormalizedSortKey = ((double)pssrSpiSearchResultsPtr->fSortKey / pssrSpiSearchResponse->dMaxSortKey);
                    break;
                
                case SPI_SORT_TYPE_UINT_ASC:
                case SPI_SORT_TYPE_UINT_DESC:
                    dNormalizedSortKey = ((double)pssrSpiSearchResultsPtr->uiSortKey / pssrSpiSearchResponse->dMaxSortKey);
                    break;
                
                case SPI_SORT_TYPE_ULONG_ASC:
                case SPI_SORT_TYPE_ULONG_DESC:
                    dNormalizedSortKey = ((double)pssrSpiSearchResultsPtr->ulSortKey / pssrSpiSearchResponse->dMaxSortKey);
                    break;
                
                case SPI_SORT_TYPE_UCHAR_ASC:
                case SPI_SORT_TYPE_UCHAR_DESC:
                    dNormalizedSortKey = 0.0;
                    break;
                
                case SPI_SORT_TYPE_NO_SORT:
                    dNormalizedSortKey = 0.0;
                    break;
    
                case SPI_SORT_TYPE_UNKNOWN:
                    dNormalizedSortKey = 0.0;
                    break;
                
                default:
                    dNormalizedSortKey = 0.0;
                    break;
            }

            /* Adjust the normalized sort key if it was less than zero */
            if ( dNormalizedSortKey < 0.0 ) {
                dNormalizedSortKey = 0.0;
            }
            
            iSrvrHttpSendf(psssSrvrServerSession, "<relevance:score>%.4f</relevance:score>\n", dNormalizedSortKey);
        }


        /* Close the item */
        iSrvrHttpSendf(psssSrvrServerSession, "</item>\n");
    
    }


    /* Append the search report at the end of the search results */
    if ( uiSearchReport != SRVR_HTTP_REPORT_NONE ) {

        iSrvrHttpSendf(psssSrvrServerSession, "<item>\n");

        iSrvrHttpSendf(psssSrvrServerSession, "<title>Report for this search</title>\n");

        /* Get the current date */
        iUtlDateGetWebDateFromTime(s_time(NULL), pucDate, SRVR_HTTP_SHORT_STRING_LENGTH + 1);
        iSrvrHttpSendf(psssSrvrServerSession, "<pubDate>%s</pubDate>\n", pucSrvrHttpEncodeXmlString(pucDate, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

        iSrvrHttpSendf(psssSrvrServerSession, "<guid isPermaLink=\"false\">%s:%s</guid>\n", SPI_SEARCH_REPORT_ITEM_NAME, SPI_SEARCH_REPORT_MIME_TYPE);

        iSrvrHttpSendf(psssSrvrServerSession, "<description>");

        /* Process the search reports if there are any */
        if ( (uiSearchReportListLength > 0) && (ppucSearchReportList != NULL) ) {
            
            /* Raw report */
            if ( uiSearchReport == SRVR_HTTP_REPORT_RAW ) {
            
                /* Print the search reports */
                for ( uiI = 0; uiI < uiSearchReportListLength; uiI++ ) {

                    /* Send the data */
                    if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, ppucSearchReportList[uiI], s_strlen(ppucSearchReportList[uiI]))) != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiSrvrHttpSendSearchResultsRss;
                    }
                }
            }
            /* Formatted report */
            else if ( uiSearchReport == SRVR_HTTP_REPORT_FORMATTED ) {
            
                /* Merge and format the search reports, ignore errors at this point */
                if ( (iError = iRepMergeAndFormatSearchReports(ppucSearchReportList, &pucSearchReportFormatted)) != REP_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to merge and format the search reports, rep error: %d.", iError);
                    iError = SPI_NoError;
                }
                else {

                    /* Send the data */
                    if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, pucSearchReportFormatted, s_strlen(pucSearchReportFormatted))) != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiSrvrHttpSendSearchResultsRss;
                    }

                    /* Free the formatted search report */
                    s_free(pucSearchReportFormatted);
                }
            }

            /* Free the search reports */
            UTL_MACROS_FREE_NUMBERED_LIST(ppucSearchReportList, uiSearchReportListLength);
        }

        iSrvrHttpSendf(psssSrvrServerSession, "</description>\n");


        iSrvrHttpSendf(psssSrvrServerSession, "</item>\n");
    }


    /* Send the rss footer */
    iSrvrHttpSendRssFooter(psssSrvrServerSession);
    


    /* Bail label */
    bailFromiSrvrHttpSendSearchResultsRss:

    /* Free allocations */
    s_free(pucExtensions);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendSearchResponseXml()

    Purpose:    Send the search response in XML.

    Parameters: psssSrvrServerSession       server session structure
                pucPath                     path
                pucQuery                    query
                uiFormat                    format
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                pssrSpiSearchResponse       search response
                uiSearchReport              search report
                ppucSearchReportList        search report list
                uiSearchReportListLength    search report list length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendSearchResponseXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex, 
    unsigned int uiEndIndex,
    struct spiSearchResponse *pssrSpiSearchResponse,
    unsigned int uiSearchReport,
    unsigned char **ppucSearchReportList,
    unsigned int uiSearchReportListLength
)
{

    int                         iError = SPI_NoError;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;

    unsigned char               pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};

    unsigned char               *pucSearchReportFormatted = NULL;
    unsigned int                uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(pssrSpiSearchResponse != NULL);
    ASSERT(((pssrSpiSearchResponse->pssrSpiSearchResults != NULL) && (pssrSpiSearchResponse->uiSpiSearchResultsLength >= 0)) || 
            ((pssrSpiSearchResponse->pssrSpiSearchResults == NULL) && (pssrSpiSearchResponse->uiSpiSearchResultsLength == 0)));
    ASSERT(pssrSpiSearchResponse->uiTotalResults >= 0);
    ASSERT(pssrSpiSearchResponse->uiStartIndex >= 0);
    ASSERT(pssrSpiSearchResponse->uiEndIndex >= 0);
    ASSERT((SPI_SORT_TYPE_VALID(pssrSpiSearchResponse->uiSortType) == true) || (pssrSpiSearchResponse->uiSortType == SPI_SORT_TYPE_UNKNOWN));
    ASSERT(pssrSpiSearchResponse->dMaxSortKey >= 0);
    ASSERT(pssrSpiSearchResponse->dSearchTime >= 0);
    ASSERT(SRVR_HTTP_REPORT_VALID(uiSearchReport) == true);
    ASSERT(((ppucSearchReportList != NULL) && (uiSearchReportListLength >= 0)) || ((ppucSearchReportList == NULL) && (uiSearchReportListLength == 0)));


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the search response */
    iSrvrHttpSendf(psssSrvrServerSession, "<searchResponse>\n");

    iSrvrHttpSendf(psssSrvrServerSession, "<search>%s</search>\n", pucSrvrHttpEncodeXmlString(pucSearchText, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<totalResults>%u</totalResults>\n", pssrSpiSearchResponse->uiTotalResults);
    iSrvrHttpSendf(psssSrvrServerSession, "<startIndex>%u</startIndex>\n", pssrSpiSearchResponse->uiStartIndex);
    iSrvrHttpSendf(psssSrvrServerSession, "<endIndex>%u</endIndex>\n", pssrSpiSearchResponse->uiEndIndex);

    /* Send the sort type */
    iSrvrHttpSendf(psssSrvrServerSession, "<sortType>");

    switch ( pssrSpiSearchResponse->uiSortType ) {
    
        case SPI_SORT_TYPE_DOUBLE_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "double:asc");
            break;
        
        case SPI_SORT_TYPE_DOUBLE_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "double:desc");
            break;
        
        case SPI_SORT_TYPE_FLOAT_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "float:asc");
            break;
        
        case SPI_SORT_TYPE_FLOAT_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "float:desc");
            break;
        
        case SPI_SORT_TYPE_UINT_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "uint:asc");
            break;
        
        case SPI_SORT_TYPE_UINT_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "uint:desc");
            break;
        
        case SPI_SORT_TYPE_ULONG_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "ulong:asc");
            break;
        
        case SPI_SORT_TYPE_ULONG_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "ulong:desc");
            break;
        
        case SPI_SORT_TYPE_UCHAR_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "char:asc");
            break;
        
        case SPI_SORT_TYPE_UCHAR_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "char:desc");
            break;
        
        case SPI_SORT_TYPE_NO_SORT:
            iSrvrHttpSendf(psssSrvrServerSession, "nosort");
            break;
        
        case SPI_SORT_TYPE_UNKNOWN:
            iSrvrHttpSendf(psssSrvrServerSession, "unknown");
            break;
        
        default:
            iSrvrHttpSendf(psssSrvrServerSession, "invalid");
            break;
        
    }

    iSrvrHttpSendf(psssSrvrServerSession, "</sortType>\n");

    iSrvrHttpSendf(psssSrvrServerSession, "<maxSortKey>%.6f</maxSortKey>\n", pssrSpiSearchResponse->dMaxSortKey);
    iSrvrHttpSendf(psssSrvrServerSession, "<searchTime>%.0f</searchTime>\n", pssrSpiSearchResponse->dSearchTime);


    /* Open the search result list */
    iSrvrHttpSendf(psssSrvrServerSession, "<searchResultList>\n");

    /* Cycle through the search results and send them, they are already in order */
    for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) {

        /* Skip the search report, it gets appended as an item */
        if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
            continue;
        }


        /* Open the search result */
        iSrvrHttpSendf(psssSrvrServerSession, "<searchResult>\n");


        /* Print the index name */
        iSrvrHttpSendf(psssSrvrServerSession, "<indexName>%s</indexName>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucIndexName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

        /* Print the document key */
        iSrvrHttpSendf(psssSrvrServerSession, "<documentKey>%s</documentKey>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucDocumentKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

        /* Print the title */
        iSrvrHttpSendf(psssSrvrServerSession, "<title>%s</title>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        
        /* Print the sort key */
        switch ( pssrSpiSearchResponse->uiSortType ) {

            case SPI_SORT_TYPE_DOUBLE_ASC:
            case SPI_SORT_TYPE_DOUBLE_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>%.4f</sortKey>\n", pssrSpiSearchResultsPtr->dSortKey);
                break;

            case SPI_SORT_TYPE_FLOAT_ASC:
            case SPI_SORT_TYPE_FLOAT_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>%.4f</sortKey>\n", pssrSpiSearchResultsPtr->fSortKey);
                break;
            
            case SPI_SORT_TYPE_UINT_ASC:
            case SPI_SORT_TYPE_UINT_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>%u</sortKey>\n", pssrSpiSearchResultsPtr->uiSortKey);
                break;
            
            case SPI_SORT_TYPE_ULONG_ASC:
            case SPI_SORT_TYPE_ULONG_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>%lu</sortKey>\n", pssrSpiSearchResultsPtr->ulSortKey);
                break;
            
            case SPI_SORT_TYPE_UCHAR_ASC:
            case SPI_SORT_TYPE_UCHAR_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>%s</sortKey>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucSortKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
                break;
            
            case SPI_SORT_TYPE_NO_SORT:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>none</sortKey>\n");
                break;

            case SPI_SORT_TYPE_UNKNOWN:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>unknown</sortKey>\n");
                break;
            
            default:
                iSrvrHttpSendf(psssSrvrServerSession, "<sortKey>invalid</sortKey>\n");
                break;
        }

        /* Print the language code */
        iSrvrHttpSendf(psssSrvrServerSession, "<languageCode>%s</languageCode>\n", pucSrvrHttpEncodeXmlString(pssrSpiSearchResultsPtr->pucLanguageCode, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        
        /* Print the rank */
        iSrvrHttpSendf(psssSrvrServerSession, "<rank>%u</rank>\n", pssrSpiSearchResultsPtr->uiRank);
        
        /* Print the term count */
        iSrvrHttpSendf(psssSrvrServerSession, "<termCount>%u</termCount>\n", pssrSpiSearchResultsPtr->uiTermCount);

        /* Print the date and time */
        if ( pssrSpiSearchResultsPtr->ulAnsiDate != 0 ) {
            iSrvrHttpSendf(psssSrvrServerSession, "<ansiDate>%lu</ansiDate>\n", pssrSpiSearchResultsPtr->ulAnsiDate);
        }            



        /* Print the document items if any */
        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {
        
            unsigned int                uiJ = 0;
            struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
                    

            /* Loop over each document item */
            for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {

                void            *pvData = NULL;
                unsigned int    uiDataLength = 0;

                /* Open the document item */
                iSrvrHttpSendf(psssSrvrServerSession, "<documentItem>\n");

                /* Send the item name */
                iSrvrHttpSendf(psssSrvrServerSession, "<itemName>%s</itemName>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentItemsPtr->pucItemName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        
                /* Send the mime type */
                iSrvrHttpSendf(psssSrvrServerSession, "<mimeType>%s</mimeType>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentItemsPtr->pucMimeType, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
            
                /* Send the length */
                iSrvrHttpSendf(psssSrvrServerSession, "<length>%u</length>\n", psdiSpiDocumentItemsPtr->uiLength);
            
                /* Send the URL */
                if ( bUtlStringsIsStringNULL(psdiSpiDocumentItemsPtr->pucUrl) == false ) {
                    iSrvrHttpSendf(psssSrvrServerSession, "<url>%u</url>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentItemsPtr->pucUrl, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
                }
                    
                /* Send the data for the first document item */
                if ( uiJ == 0 ) {

                    /* Send the data */
                    if ( iSrvrHttpRetrieveDocument(psssSrvrServerSession->pssSpiSession, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, 
                            psdiSpiDocumentItemsPtr->pucItemName, psdiSpiDocumentItemsPtr->pucMimeType, &pvData, &uiDataLength) == SPI_NoError ) {
        
                        iSrvrHttpSendf(psssSrvrServerSession, "<data>");
                
                        /* Write the data */
                        if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, (unsigned char *)pvData, uiDataLength)) != SPI_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
                            iError = SPI_SearchIndexFailed;
                            goto bailFromiSrvrHttpSendSearchResultsXml;
                        }
        
                        iSrvrHttpSendf(psssSrvrServerSession, "</data>\n");
                        
                        /* Free the data */
                        s_free(pvData);
                    }
                }

                /* Close the document item */
                iSrvrHttpSendf(psssSrvrServerSession, "</documentItem>\n");
            } 
        }

        /* Close the search result */
        iSrvrHttpSendf(psssSrvrServerSession, "</searchResult>\n");
    }

    /* Close the search result list */
    iSrvrHttpSendf(psssSrvrServerSession, "</searchResultList>\n");


    /* Append the search report at the end of the search results */
    if ( uiSearchReport != SRVR_HTTP_REPORT_NONE ) {

        /* Open the search report */
        iSrvrHttpSendf(psssSrvrServerSession, "<searchReport>\n");

        iSrvrHttpSendf(psssSrvrServerSession, "<title>Report for this search</title>\n");

        /* Process the search reports if there are any */
        if ( (uiSearchReportListLength > 0) && (ppucSearchReportList != NULL) ) {
            
            /* Raw report */
            if ( uiSearchReport == SRVR_HTTP_REPORT_RAW ) {
            
                /* Print the search reports */
                for ( uiI = 0; uiI < uiSearchReportListLength; uiI++ ) {

                    iSrvrHttpSendf(psssSrvrServerSession, "<rawReport>");

                    /* Send the data */
                    if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, ppucSearchReportList[uiI], s_strlen(ppucSearchReportList[uiI]))) != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiSrvrHttpSendSearchResultsXml;
                    }

                    iSrvrHttpSendf(psssSrvrServerSession, "</rawReport>\n");

                }
            }
            /* Formatted report */
            else if ( uiSearchReport == SRVR_HTTP_REPORT_FORMATTED ) {
            
                /* Merge and format the search reports, ignore errors at this point */
                if ( (iError = iRepMergeAndFormatSearchReports(ppucSearchReportList, &pucSearchReportFormatted)) != REP_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to merge and format the search reports, rep error: %d.", iError);
                    iError = SPI_NoError;
                }
                else {

                    iSrvrHttpSendf(psssSrvrServerSession, "<formattedReport>");

                    /* Send the data */
                    if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, pucSearchReportFormatted, s_strlen(pucSearchReportFormatted))) != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiSrvrHttpSendSearchResultsXml;
                    }

                    iSrvrHttpSendf(psssSrvrServerSession, "</formattedReport>\n");
            
                    /* Free the formatted search report */
                    s_free(pucSearchReportFormatted);
                }
            }

            /* Free the search reports */
            UTL_MACROS_FREE_NUMBERED_LIST(ppucSearchReportList, uiSearchReportListLength);
        }

        /* Close the search report */
        iSrvrHttpSendf(psssSrvrServerSession, "</searchReport>\n");
    }


    /* Close the search response */
    iSrvrHttpSendf(psssSrvrServerSession, "</searchResponse>\n");

    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);
    


    /* Bail label */
    bailFromiSrvrHttpSendSearchResultsXml:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendSearchResponseOn()

    Purpose:    Send the search response in Object Notation.

    Parameters: psssSrvrServerSession       server session structure
                pucPath                     path
                pucQuery                    query
                uiFormat                    format
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                pssrSpiSearchResponse       search response
                uiSearchReport              search report
                ppucSearchReportList        search report list
                uiSearchReportListLength    search report list length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendSearchResponseOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex, 
    unsigned int uiEndIndex,
    struct spiSearchResponse *pssrSpiSearchResponse,
    unsigned int uiSearchReport,
    unsigned char **ppucSearchReportList,
    unsigned int uiSearchReportListLength
)
{

    int                         iError = SPI_NoError;
    unsigned char               *pucQuotePtr = NULL;
    unsigned char               *pucSeparatorPtr = NULL;
    unsigned char               pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    unsigned char               *pucSearchReportFormatted = NULL;
    unsigned int                uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(pssrSpiSearchResponse != NULL);
    ASSERT(((pssrSpiSearchResponse->pssrSpiSearchResults != NULL) && (pssrSpiSearchResponse->uiSpiSearchResultsLength >= 0)) || 
            ((pssrSpiSearchResponse->pssrSpiSearchResults == NULL) && (pssrSpiSearchResponse->uiSpiSearchResultsLength == 0)));
    ASSERT(pssrSpiSearchResponse->uiTotalResults >= 0);
    ASSERT(pssrSpiSearchResponse->uiStartIndex >= 0);
    ASSERT(pssrSpiSearchResponse->uiEndIndex >= 0);
    ASSERT((SPI_SORT_TYPE_VALID(pssrSpiSearchResponse->uiSortType) == true) || (pssrSpiSearchResponse->uiSortType == SPI_SORT_TYPE_UNKNOWN));
    ASSERT(pssrSpiSearchResponse->dMaxSortKey >= 0);
    ASSERT(pssrSpiSearchResponse->dSearchTime >= 0);
    ASSERT(SRVR_HTTP_REPORT_VALID(uiSearchReport) == true);
    ASSERT(((ppucSearchReportList != NULL) && (uiSearchReportListLength >= 0)) || ((ppucSearchReportList == NULL) && (uiSearchReportListLength == 0)));


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the search response */
    iSrvrHttpSendf(psssSrvrServerSession, "%ssearchResponse%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);


    iSrvrHttpSendf(psssSrvrServerSession, "%ssearch%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pucSearchText, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%stotalResults%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResponse->uiTotalResults);

    iSrvrHttpSendf(psssSrvrServerSession, "%sstartIndex%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResponse->uiStartIndex);

    iSrvrHttpSendf(psssSrvrServerSession, "%sendIndex%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResponse->uiEndIndex);

    /* Send the sort type */
    iSrvrHttpSendf(psssSrvrServerSession, "%ssortType%s %s %s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr);

    switch ( pssrSpiSearchResponse->uiSortType ) {
    
        case SPI_SORT_TYPE_DOUBLE_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "double:asc");
            break;
        
        case SPI_SORT_TYPE_DOUBLE_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "double:desc");
            break;
        
        case SPI_SORT_TYPE_FLOAT_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "float:asc");
            break;
        
        case SPI_SORT_TYPE_FLOAT_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "float:desc");
            break;
        
        case SPI_SORT_TYPE_UINT_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "uint:asc");
            break;
        
        case SPI_SORT_TYPE_UINT_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "uint:desc");
            break;
        
        case SPI_SORT_TYPE_ULONG_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "ulong:asc");
            break;
        
        case SPI_SORT_TYPE_ULONG_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "ulong:desc");
            break;
        
        case SPI_SORT_TYPE_UCHAR_ASC:
            iSrvrHttpSendf(psssSrvrServerSession, "char:asc");
            break;
        
        case SPI_SORT_TYPE_UCHAR_DESC:
            iSrvrHttpSendf(psssSrvrServerSession, "char:desc");
            break;
        
        case SPI_SORT_TYPE_NO_SORT:
            iSrvrHttpSendf(psssSrvrServerSession, "nosort");
            break;
        
        case SPI_SORT_TYPE_UNKNOWN:
            iSrvrHttpSendf(psssSrvrServerSession, "unknown");
            break;
        
        default:
            iSrvrHttpSendf(psssSrvrServerSession, "invalid");
            break;
        
    }

    iSrvrHttpSendf(psssSrvrServerSession, "%s,\n", pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%smaxSortKey%s %s %.6f,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResponse->dMaxSortKey);
    iSrvrHttpSendf(psssSrvrServerSession, "%ssearchTime%s %s %.0f,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResponse->dSearchTime);



    /* Open the search result list */
    iSrvrHttpSendf(psssSrvrServerSession, "%ssearchResultList%s %s [\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);


    /* Cycle through the search results and send them, they are already in order */
    for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) {

        /* Skip the search report, it gets appended as an item */
        if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
            continue;
        }

        
        if ( uiI > 0 ) {
            iSrvrHttpSendf(psssSrvrServerSession, ",\n");
        }

        /* Send the index name */
        iSrvrHttpSendf(psssSrvrServerSession, "{%sindexName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(pssrSpiSearchResultsPtr->pucIndexName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        /* Send the document key */
        iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentKey%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(pssrSpiSearchResultsPtr->pucDocumentKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        /* Send the title */
        iSrvrHttpSendf(psssSrvrServerSession, "%stitle%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(pssrSpiSearchResultsPtr->pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);
        
        /* Send the sort key */
        switch ( pssrSpiSearchResponse->uiSortType ) {

            case SPI_SORT_TYPE_DOUBLE_ASC:
            case SPI_SORT_TYPE_DOUBLE_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %.4f,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResultsPtr->dSortKey);
                break;

            case SPI_SORT_TYPE_FLOAT_ASC:
            case SPI_SORT_TYPE_FLOAT_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %.4f,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResultsPtr->fSortKey);
                break;
            
            case SPI_SORT_TYPE_UINT_ASC:
            case SPI_SORT_TYPE_UINT_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResultsPtr->uiSortKey);
                break;
            
            case SPI_SORT_TYPE_ULONG_ASC:
            case SPI_SORT_TYPE_ULONG_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResultsPtr->ulSortKey);
                break;
            
            case SPI_SORT_TYPE_UCHAR_ASC:
            case SPI_SORT_TYPE_UCHAR_DESC:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                        pucSrvrHttpEncodeOnString(pssrSpiSearchResultsPtr->pucSortKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);
                break;
            
            case SPI_SORT_TYPE_NO_SORT:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %snone%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, pucQuotePtr);
                break;

            case SPI_SORT_TYPE_UNKNOWN:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %sunknown%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, pucQuotePtr);
                break;
            
            default:
                iSrvrHttpSendf(psssSrvrServerSession, "%ssortKey%s %s %sinvalid%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, pucQuotePtr);
                break;
        }

        /* Send the language code */
        iSrvrHttpSendf(psssSrvrServerSession, "%slanguageName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(pssrSpiSearchResultsPtr->pucLanguageCode, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);
        
        /* Send the rank */
        iSrvrHttpSendf(psssSrvrServerSession, "%srank%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResultsPtr->uiRank);
        
        /* Send the term count */
        iSrvrHttpSendf(psssSrvrServerSession, "%stermCount%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResultsPtr->uiTermCount);

        /* Send the date and time */
        if ( pssrSpiSearchResultsPtr->ulAnsiDate != 0 ) {
            iSrvrHttpSendf(psssSrvrServerSession, "%sansiDate%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssrSpiSearchResultsPtr->ulAnsiDate);
        }            



        /* Send the document items if any */
        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {
        
            unsigned int                uiJ = 0;
            struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;

            /* Open the document item */
            iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentItem%s %s [\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

            /* Loop over each document item */
            for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {

                void            *pvData = NULL;
                unsigned int    uiDataLength = 0;
            
                /* Open the entry and send the item name */
                iSrvrHttpSendf(psssSrvrServerSession, "{%sitemName%s %s %s%s%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                        pucSrvrHttpEncodeOnString(psdiSpiDocumentItemsPtr->pucItemName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);
        
                /* Send the mime type */
                iSrvrHttpSendf(psssSrvrServerSession, ", %smimeType%s %s %s%s%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                        pucSrvrHttpEncodeOnString(psdiSpiDocumentItemsPtr->pucMimeType, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

                /* Send the length */
                iSrvrHttpSendf(psssSrvrServerSession, ", %slength%s%s %u", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, psdiSpiDocumentItemsPtr->uiLength);
            
                /* Send the URL */
                if ( bUtlStringsIsStringNULL(psdiSpiDocumentItemsPtr->pucUrl) == false ) {
                    iSrvrHttpSendf(psssSrvrServerSession, ", %surl%s%s %s%s%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                            pucSrvrHttpEncodeOnString(psdiSpiDocumentItemsPtr->pucUrl, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);
                }

                /* Send the data for the first document item */
                if ( uiJ == 0 ) {
    
                    if ( iSrvrHttpRetrieveDocument(psssSrvrServerSession->pssSpiSession, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, 
                            psdiSpiDocumentItemsPtr->pucItemName, psdiSpiDocumentItemsPtr->pucMimeType, &pvData, &uiDataLength) == SPI_NoError ) {
        
                        iSrvrHttpSendf(psssSrvrServerSession, ", %sdata%s%s %s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr);
                
                        /* Write the data */
                        if ( (iError = iSrvrHttpSendEncodeOn(psssSrvrServerSession, (unsigned char *)pvData, uiDataLength)) != SPI_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
                            iError = SPI_SearchIndexFailed;
                            goto bailFromiSrvrHttpSendSearchResultsJson;
                        }
        
                        iSrvrHttpSendf(psssSrvrServerSession, "%s", pucQuotePtr);
                        
                        /* Free the data */
                        s_free(pvData);
                    }
                }

                /* Close the entry */
                iSrvrHttpSendf(psssSrvrServerSession, "}%s", (uiJ < (pssrSpiSearchResultsPtr->uiDocumentItemsLength - 1)) ? ", " : "");
            } 

            /* Close the document item */
            iSrvrHttpSendf(psssSrvrServerSession, "\n]\n");

        }

        /* Close the search result */
        iSrvrHttpSendf(psssSrvrServerSession, "}");
    }

    /* Close the search result list */
    iSrvrHttpSendf(psssSrvrServerSession, "\n],\n");



    /* Append the search report at the end of the search results */
    if ( uiSearchReport != SRVR_HTTP_REPORT_NONE ) {

        /* Open the search report */
        iSrvrHttpSendf(psssSrvrServerSession, "%ssearchReport%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

        iSrvrHttpSendf(psssSrvrServerSession, "%stitle%s %s %sReport for this search%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, pucQuotePtr);

        /* Process the search reports if there are any */
        if ( (uiSearchReportListLength > 0) && (ppucSearchReportList != NULL) ) {
            
            /* Raw report */
            if ( uiSearchReport == SRVR_HTTP_REPORT_RAW ) {
            
                /* Open the raw reports */
                iSrvrHttpSendf(psssSrvrServerSession, "%srawReport%s %s [\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

                /* Send the search reports */
                for ( uiI = 0; uiI < uiSearchReportListLength; uiI++ ) {
                    iSrvrHttpSendf(psssSrvrServerSession, "{%srawReport%s %s %s%s%s}%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                            pucSrvrHttpEncodeOnString(ppucSearchReportList[uiI], pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr,
                            (uiI < (uiSearchReportListLength - 1)) ? ", " : "");
                }

                /* Close the raw reports */
                iSrvrHttpSendf(psssSrvrServerSession, " \n]\n");
            }
            /* Formatted report */
            else if ( uiSearchReport == SRVR_HTTP_REPORT_FORMATTED ) {
            
                /* Merge and format the search reports, ignore errors at this point */
                if ( (iError = iRepMergeAndFormatSearchReports(ppucSearchReportList, &pucSearchReportFormatted)) != REP_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to merge and format the search reports, rep error: %d.", iError);
                    iError = SPI_NoError;
                }
                else {
                    /* Print the search report */
                    iSrvrHttpSendf(psssSrvrServerSession, "%sformattedReport%s %s %s%s%s\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                            pucSrvrHttpEncodeOnString(pucSearchReportFormatted, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);
        
                    /* Free the formatted search report */
                    s_free(pucSearchReportFormatted);
                }
            }

            /* Free the search reports */
            UTL_MACROS_FREE_NUMBERED_LIST(ppucSearchReportList, uiSearchReportListLength);
        }

        /* Close the search report */
        iSrvrHttpSendf(psssSrvrServerSession, "}\n");
    }


    /* Close the search response */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);
    


    /* Bail label */
    bailFromiSrvrHttpSendSearchResultsJson:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendDocumentRss()

    Purpose:    Send the document in RSS.

    Parameters: psssSrvrServerSession       server session structure
                pucPath                     path
                pucQuery                    query
                uiFormat                    format
                pucTitle                    document title
                pucDocumentKey              document key
                pucItemName                 item name
                pucMimeType                 mime type
                pvData                      data
                uiDataLength                data length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendDocumentRss
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    unsigned char *pucTitle,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    void *pvData,
    unsigned int uiDataLength
)
{

    int             iError = SPI_NoError;
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_RSS);
    ASSERT(bUtlStringsIsStringNULL(pucTitle) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(((pvData != NULL) && (uiDataLength >= 0)) || ((pvData == NULL) && (uiDataLength == 0)));


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RSS, 0);

    /* Send the rss header */
    iSrvrHttpSendRssHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the item */
    iSrvrHttpSendf(psssSrvrServerSession, "<item>\n");

    /* Title */
    iSrvrHttpSendf(psssSrvrServerSession, "<title>%s</title>\n", pucSrvrHttpEncodeXmlString(pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

    /* Send the data */
    if ( (pvData != NULL) && (uiDataLength > 0) ) {

        /* Open the description */
        iSrvrHttpSendf(psssSrvrServerSession, "<description>");

        /* Send the data */
        if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, (unsigned char *)pvData, uiDataLength)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiSrvrHttpSendDocumentRss;
        }

        /* Close the description */
        iSrvrHttpSendf(psssSrvrServerSession, "</description>\n");
    }    

    /* Close the item */
    iSrvrHttpSendf(psssSrvrServerSession, "</item>\n");



    /* Bail label */
    bailFromiSrvrHttpSendDocumentRss:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendDocumentXml()

    Purpose:    Send the document in XML.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                pucTitle                document title
                pucDocumentKey          document key
                pucItemName             item name
                pucMimeType             mime type
                pvData                  data
                uiDataLength            data length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendDocumentXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    unsigned char *pucTitle,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    void *pvData,
    unsigned int uiDataLength
)
{

    int             iError = SPI_NoError;
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(bUtlStringsIsStringNULL(pucTitle) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(((pvData != NULL) && (uiDataLength >= 0)) || ((pvData == NULL) && (uiDataLength == 0)));


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the document */
    iSrvrHttpSendf(psssSrvrServerSession, "<document>\n");

    /* Send the document */
    iSrvrHttpSendf(psssSrvrServerSession, "<title>%s</title>\n", pucSrvrHttpEncodeXmlString(pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<documentKey>%s</documentKey>\n", pucSrvrHttpEncodeXmlString(pucDocumentKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<itemName>%s</itemName>\n", pucSrvrHttpEncodeXmlString(pucItemName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<mimeType>%s</mimeType>\n", pucSrvrHttpEncodeXmlString(pucMimeType, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

    if ( (pvData != NULL) && (uiDataLength > 0) ) {

        /* Open the data */
        iSrvrHttpSendf(psssSrvrServerSession, "<data>");
    
        /* Send the data */
        if ( (iError = iSrvrHttpSendEncodeXml(psssSrvrServerSession, (unsigned char *)pvData, uiDataLength)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiSrvrHttpSendDocumentXml;
        }

        /* Close the data */
        iSrvrHttpSendf(psssSrvrServerSession, "</data>\n");
    }

    /* Close the document */
    iSrvrHttpSendf(psssSrvrServerSession, "</document>\n");


    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);


    /* Bail label */
    bailFromiSrvrHttpSendDocumentXml:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendDocumentOn()

    Purpose:    Send the document in Object Notation.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                pucTitle                document title
                pucDocumentKey          document key
                pucItemName             item name
                pucMimeType             mime type
                pvData                  data
                uiDataLength            data length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendDocumentOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    unsigned char *pucTitle,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    void *pvData,
    unsigned int uiDataLength
)
{

    int             iError = SPI_NoError;
    unsigned char   *pucQuotePtr = NULL;
    unsigned char   *pucSeparatorPtr = NULL;
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(bUtlStringsIsStringNULL(pucTitle) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(((pvData != NULL) && (uiDataLength >= 0)) || ((pvData == NULL) && (uiDataLength == 0)));


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the document */
    iSrvrHttpSendf(psssSrvrServerSession, "%sdocument%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Send the document */
    iSrvrHttpSendf(psssSrvrServerSession, "%stitle%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentKey%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pucDocumentKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sitemName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pucItemName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%smimeType%s %s %s%s%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pucMimeType, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    if ( (pvData != NULL) && (uiDataLength > 0) ) {

        /* Data */
        iSrvrHttpSendf(psssSrvrServerSession, ",\n%sdata%s %s %s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr);

        /* Write the data */
        if ( (iError = iSrvrHttpSendEncodeOn(psssSrvrServerSession, (unsigned char *)pvData, uiDataLength)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, spi error: %d.", iError);
            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiSrvrHttpSendDocumentJson;
        }

        iSrvrHttpSendf(psssSrvrServerSession, "%s\n", pucQuotePtr);
    }
    else {
        iSrvrHttpSendf(psssSrvrServerSession, ",\n");
    }


    /* Close the document */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);



    /* Bail label */
    bailFromiSrvrHttpSendDocumentJson:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendDocumentRaw()

    Purpose:    Send the document in RAW.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                pucTitle                document title
                pucDocumentKey          document key
                pucItemName             item name
                pucMimeType             mime type
                pvData                  data
                uiDataLength            data length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendDocumentRaw
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    unsigned char *pucTitle,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    void *pvData,
    unsigned int uiDataLength
)
{

    int     iError = SPI_NoError;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_RAW);
    ASSERT(bUtlStringsIsStringNULL(pucTitle) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(((pvData != NULL) && (uiDataLength >= 0)) || ((pvData == NULL) && (uiDataLength == 0)));


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, pucMimeType, uiDataLength);

    /* Send the data */
    if ( (pvData != NULL) && (uiDataLength > 0) ) {

        /* Write the data */
        if ( (iError = iUtlNetWrite(psssSrvrServerSession->pvUtlNet, (unsigned char *)pvData, uiDataLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, utl error: %d.", iError);
            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiSrvrHttpSendDocumentRaw;
        }
    }


    /* Bail label */
    bailFromiSrvrHttpSendDocumentRaw:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendServerInfoXml()

    Purpose:    Send the server info in XML.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                pssiSpiServerInfo       spi server info

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendServerInfoXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiServerInfo *pssiSpiServerInfo
)
{

    int             iError = SPI_NoError;
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(pssiSpiServerInfo != NULL);


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the server info */
    iSrvrHttpSendf(psssSrvrServerSession, "<serverInfo>\n");

    /* Send the server info */
    iSrvrHttpSendf(psssSrvrServerSession, "<name>%s</name>\n", pucSrvrHttpEncodeXmlString(pssiSpiServerInfo->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<description>%s</description>\n", pucSrvrHttpEncodeXmlString(pssiSpiServerInfo->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<adminName>%s</adminName>\n", pucSrvrHttpEncodeXmlString(pssiSpiServerInfo->pucAdminName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<adminEmail>%s</adminEmail>\n", pucSrvrHttpEncodeXmlString(pssiSpiServerInfo->pucAdminEmail, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<indexCount>%u</indexCount>\n", pssiSpiServerInfo->uiIndexCount);
    iSrvrHttpSendf(psssSrvrServerSession, "<rankingAlgorithm>%s</rankingAlgorithm>\n", pucSrvrHttpEncodeXmlString(pssiSpiServerInfo->pucRankingAlgorithm, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<weightMinimum>%.4f</weightMinimum>\n", pssiSpiServerInfo->dWeightMinimum);
    iSrvrHttpSendf(psssSrvrServerSession, "<weightMaximum>%.4f</weightMaximum>\n", pssiSpiServerInfo->dWeightMaximum);

    /* Close the server info */
    iSrvrHttpSendf(psssSrvrServerSession, "</serverInfo>\n");

    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendServerInfoOn()

    Purpose:    Send the server info in Object Notation.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                pssiSpiServerInfo       spi server info

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendServerInfoOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiServerInfo *pssiSpiServerInfo
)
{

    int             iError = SPI_NoError;
    unsigned char   *pucQuotePtr = NULL;
    unsigned char   *pucSeparatorPtr = NULL;
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(pssiSpiServerInfo != NULL);


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the server info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sserverInfo%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Send the server info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sname%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pssiSpiServerInfo->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sdescription%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pssiSpiServerInfo->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sadminName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pssiSpiServerInfo->pucAdminName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sadminEmail%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(pssiSpiServerInfo->pucAdminEmail, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sindexCount%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssiSpiServerInfo->uiIndexCount);

    iSrvrHttpSendf(psssSrvrServerSession, "%srankingAlgorithm%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucUtlStringsGetSafeString(pssiSpiServerInfo->pucRankingAlgorithm), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sweightMinimum%s %s %.4f,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssiSpiServerInfo->dWeightMinimum);

    iSrvrHttpSendf(psssSrvrServerSession, "%sweightMaximum%s %s %.4f\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pssiSpiServerInfo->dWeightMaximum);

    /* Close the server info */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendServerIndexInfoXml()

    Purpose:    Send the server index info in XML.

    Parameters: psssSrvrServerSession           server session structure
                pucPath                         path
                pucQuery                        query
                uiFormat                        format
                pssiiSpiServerIndexInfos        spi server index infos
                uiSpiServerIndexInfosLength     spi server index infos length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendServerIndexInfoXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiServerIndexInfo *pssiiSpiServerIndexInfos, 
    unsigned int uiSpiServerIndexInfosLength
)
{

    int                         iError = SPI_NoError;
    unsigned char               pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;
    unsigned int                uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(((pssiiSpiServerIndexInfos != NULL) && (uiSpiServerIndexInfosLength >= 0)) || ((pssiiSpiServerIndexInfos == NULL) && (uiSpiServerIndexInfosLength == 0)));


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the server index info list */
    iSrvrHttpSendf(psssSrvrServerSession, "<serverIndexInfoList>\n");

    /* Send the server index info */
    for ( uiI = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos; uiI < uiSpiServerIndexInfosLength; uiI++, pssiiSpiServerIndexInfosPtr++ ) {
        iSrvrHttpSendf(psssSrvrServerSession, "<serverIndexInfo>\n");
        iSrvrHttpSendf(psssSrvrServerSession, "<name>%s</name>\n", pucSrvrHttpEncodeXmlString(pssiiSpiServerIndexInfosPtr->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        iSrvrHttpSendf(psssSrvrServerSession, "<description>%s</description>\n", pucSrvrHttpEncodeXmlString(pssiiSpiServerIndexInfosPtr->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        iSrvrHttpSendf(psssSrvrServerSession, "</serverIndexInfo>\n");
    }

    /* Close the server index info list */
    iSrvrHttpSendf(psssSrvrServerSession, "</serverIndexInfoList>\n");

    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendServerIndexInfoOn()

    Purpose:    Send the server index info in Object Notation.

    Parameters: psssSrvrServerSession           server session structure
                pucPath                         path
                pucQuery                        query
                uiFormat                        format
                pssiiSpiServerIndexInfos        spi server index infos
                uiSpiServerIndexInfosLength     spi server index infos length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendServerIndexInfoOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiServerIndexInfo *pssiiSpiServerIndexInfos, 
    unsigned int uiSpiServerIndexInfosLength
)
{

    int                         iError = SPI_NoError;
    unsigned char               *pucQuotePtr = NULL;
    unsigned char               *pucSeparatorPtr = NULL;
    unsigned char               pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;
    unsigned int                uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(((pssiiSpiServerIndexInfos != NULL) && (uiSpiServerIndexInfosLength >= 0)) || ((pssiiSpiServerIndexInfos == NULL) && (uiSpiServerIndexInfosLength == 0)));


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the server index info list */
    iSrvrHttpSendf(psssSrvrServerSession, "%sserverIndexInfoList%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Open the server index info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sserverIndexInfo%s %s [\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Send the server index info */
    for ( uiI = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos; uiI < uiSpiServerIndexInfosLength; uiI++, pssiiSpiServerIndexInfosPtr++ ) {

        iSrvrHttpSendf(psssSrvrServerSession, "{%sname%s %s %s%s%s, ", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(pssiiSpiServerIndexInfosPtr->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        iSrvrHttpSendf(psssSrvrServerSession, "%sdescription%s %s %s%s%s }%s\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(pssiiSpiServerIndexInfosPtr->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr, 
                (uiI < (uiSpiServerIndexInfosLength - 1)) ? ", " : "");
    }

    /* Close the server index info */
    iSrvrHttpSendf(psssSrvrServerSession, "]\n");

    /* Close the server index info list */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendIndexInfoXml()

    Purpose:    Send the index info in XML.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                psiiSpiIndexInfo        spi index info

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendIndexInfoXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiIndexInfo *psiiSpiIndexInfo
)
{

    int             iError = SPI_NoError;
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(psiiSpiIndexInfo != NULL);


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the index info */
    iSrvrHttpSendf(psssSrvrServerSession, "<indexInfo>\n");

    /* Send the index info */
    iSrvrHttpSendf(psssSrvrServerSession, "<name>%s</name>\n", pucSrvrHttpEncodeXmlString(psiiSpiIndexInfo->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<description>%s</description>\n", pucSrvrHttpEncodeXmlString(psiiSpiIndexInfo->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<languageCode>%s</languageCode>\n", pucSrvrHttpEncodeXmlString(psiiSpiIndexInfo->pucLanguageCode, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<tokenizerName>%s</tokenizerName>\n", pucSrvrHttpEncodeXmlString(psiiSpiIndexInfo->pucTokenizerName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<stemmerName>%s</stemmerName>\n", pucSrvrHttpEncodeXmlString(psiiSpiIndexInfo->pucStemmerName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<stoplistName>%s</stoplistName>\n", pucSrvrHttpEncodeXmlString(psiiSpiIndexInfo->pucStopListName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<documentCount>%u</documentCount>\n", psiiSpiIndexInfo->uiDocumentCount);
    iSrvrHttpSendf(psssSrvrServerSession, "<totalTermCount>%lu</totalTermCount>\n", psiiSpiIndexInfo->ulTotalTermCount);
    iSrvrHttpSendf(psssSrvrServerSession, "<uniqueTermCount>%lu</uniqueTermCount>\n", psiiSpiIndexInfo->ulUniqueTermCount);
    iSrvrHttpSendf(psssSrvrServerSession, "<totalStopTermCount>%lu</totalStopTermCount>\n", psiiSpiIndexInfo->ulTotalStopTermCount);
    iSrvrHttpSendf(psssSrvrServerSession, "<uniqueStopTermCount>%lu</uniqueStopTermCount>\n", psiiSpiIndexInfo->ulUniqueStopTermCount);
    iSrvrHttpSendf(psssSrvrServerSession, "<accessControl>%u</accessControl>\n", psiiSpiIndexInfo->uiAccessControl);
    iSrvrHttpSendf(psssSrvrServerSession, "<updateFrequency>%u</updateFrequency>\n", psiiSpiIndexInfo->uiUpdateFrequency);
    iSrvrHttpSendf(psssSrvrServerSession, "<lastUpdateAnsiDate>%lu</lastUpdateAnsiDate>\n", psiiSpiIndexInfo->ulLastUpdateAnsiDate);
    iSrvrHttpSendf(psssSrvrServerSession, "<caseSensitive>%u</caseSensitive>\n", psiiSpiIndexInfo->uiCaseSensitive);

    /* Close the index info */
    iSrvrHttpSendf(psssSrvrServerSession, "</indexInfo>\n");

    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendIndexInfoOn()

    Purpose:    Send the index info in Object Notation.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                psiiSpiIndexInfo        spi index info

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendIndexInfoOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiIndexInfo *psiiSpiIndexInfo
)
{

    int             iError = SPI_NoError;
    unsigned char   *pucQuotePtr = NULL;
    unsigned char   *pucSeparatorPtr = NULL;
    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(psiiSpiIndexInfo != NULL);


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the index info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sindexInfo%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Send the index info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sname%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr,
            pucSrvrHttpEncodeOnString(psiiSpiIndexInfo->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1),  pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sdescription%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psiiSpiIndexInfo->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%slanguageName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psiiSpiIndexInfo->pucLanguageCode, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%stokenizerName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psiiSpiIndexInfo->pucTokenizerName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sstemmerName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psiiSpiIndexInfo->pucStemmerName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sstoplistName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psiiSpiIndexInfo->pucStopListName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentCount%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->uiDocumentCount);

    iSrvrHttpSendf(psssSrvrServerSession, "%stotalTermCount%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->ulTotalTermCount);

    iSrvrHttpSendf(psssSrvrServerSession, "%suniqueTermCount%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->ulUniqueTermCount);

    iSrvrHttpSendf(psssSrvrServerSession, "%stotalStopTermCount%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->ulTotalStopTermCount);

    iSrvrHttpSendf(psssSrvrServerSession, "%suniqueStopTermCount%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->ulUniqueStopTermCount);

    iSrvrHttpSendf(psssSrvrServerSession, "%saccessControl%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->uiAccessControl);

    iSrvrHttpSendf(psssSrvrServerSession, "%supdateFrequency%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->uiUpdateFrequency);

    iSrvrHttpSendf(psssSrvrServerSession, "%slastUpdateAndiDate%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->ulLastUpdateAnsiDate);

    iSrvrHttpSendf(psssSrvrServerSession, "%scaseSensitive%s %s %u\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, 
            psiiSpiIndexInfo->uiCaseSensitive);

    /* Close the index info */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendIndexFieldInfoXml()

    Purpose:    Send the index field info in XML.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                psfiSpiFieldInfos       spi field infos
                uiSpiFieldInfosLength   spi field length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendIndexFieldInfoXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiFieldInfo *psfiSpiFieldInfos, 
    unsigned int uiSpiFieldInfosLength
)
{

    int                     iError = SPI_NoError;
    unsigned char           pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;
    unsigned int            uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(((psfiSpiFieldInfos != NULL) && (uiSpiFieldInfosLength >= 0)) || ((psfiSpiFieldInfos == NULL) && (uiSpiFieldInfosLength == 0)));


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the index field info list */
    iSrvrHttpSendf(psssSrvrServerSession, "<indexFieldInfoList>\n");

    /* Send the index field info */
    for ( uiI = 0, psfiSpiFieldInfosPtr = psfiSpiFieldInfos; uiI < uiSpiFieldInfosLength; uiI++, psfiSpiFieldInfosPtr++ ) {
        iSrvrHttpSendf(psssSrvrServerSession, "<indexFieldInfo>\n");
        iSrvrHttpSendf(psssSrvrServerSession, "<name>%s</name>\n", pucSrvrHttpEncodeXmlString(psfiSpiFieldInfosPtr->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        iSrvrHttpSendf(psssSrvrServerSession, "<description>%s</description>\n", pucSrvrHttpEncodeXmlString(psfiSpiFieldInfosPtr->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        iSrvrHttpSendf(psssSrvrServerSession, "<type>%u</type>\n", psfiSpiFieldInfosPtr->uiType);
        iSrvrHttpSendf(psssSrvrServerSession, "</indexFieldInfo>\n");
    }

    /* Close the index field info list */
    iSrvrHttpSendf(psssSrvrServerSession, "</indexFieldInfoList>\n");

    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendIndexFieldInfoOn()

    Purpose:    Send the index field info in Object Notation.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                psfiSpiFieldInfos       spi field infos
                uiSpiFieldInfosLength   spi field length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendIndexFieldInfoOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiFieldInfo *psfiSpiFieldInfos, 
    unsigned int uiSpiFieldInfosLength
)
{

    int                     iError = SPI_NoError;
    unsigned char           *pucQuotePtr = NULL;
    unsigned char           *pucSeparatorPtr = NULL;
    unsigned char           pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;
    unsigned int            uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(((psfiSpiFieldInfos != NULL) && (uiSpiFieldInfosLength >= 0)) || ((psfiSpiFieldInfos == NULL) && (uiSpiFieldInfosLength == 0)));


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the index field info list */
    iSrvrHttpSendf(psssSrvrServerSession, "%sindexFieldInfoList%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Open the index field info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sindexFieldInfo%s %s [\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Send the index field info */
    for ( uiI = 0, psfiSpiFieldInfosPtr = psfiSpiFieldInfos; uiI < uiSpiFieldInfosLength; uiI++, psfiSpiFieldInfosPtr++ ) {

        iSrvrHttpSendf(psssSrvrServerSession, "{%sname%s %s %s%s%s, ", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(psfiSpiFieldInfosPtr->pucName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        iSrvrHttpSendf(psssSrvrServerSession, "%sdescription%s %s %s%s%s, ", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(psfiSpiFieldInfosPtr->pucDescription, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        iSrvrHttpSendf(psssSrvrServerSession, "%stype%s %s %u }%s\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, psfiSpiFieldInfosPtr->uiType,
                (uiI < (uiSpiFieldInfosLength - 1)) ? ", " : "");
    }

    /* Close the index field info */
    iSrvrHttpSendf(psssSrvrServerSession, "]\n");

    /* Close the index field info list */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendIndexTermInfoXml()

    Purpose:    Send the index term info in XML.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                pstiSpiTermInfos        spi term infos
                uiSpiTermInfosLength    spi term length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendIndexTermInfoXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiTermInfo *pstiSpiTermInfos, 
    unsigned int uiSpiTermInfosLength
)
{

    int                     iError = SPI_NoError;
    unsigned char           pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiTermInfo      *pstiSpiTermInfosPtr = NULL;
    unsigned int            uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(((pstiSpiTermInfos != NULL) && (uiSpiTermInfosLength >= 0)) || ((pstiSpiTermInfos == NULL) && (uiSpiTermInfosLength == 0)));


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the index term info list */
    iSrvrHttpSendf(psssSrvrServerSession, "<indexTermInfoList>\n");

    /* Send the index term info */
    for ( uiI = 0, pstiSpiTermInfosPtr = pstiSpiTermInfos; uiI < uiSpiTermInfosLength; uiI++, pstiSpiTermInfosPtr++ ) {
        iSrvrHttpSendf(psssSrvrServerSession, "<indexTermInfo>\n");
        iSrvrHttpSendf(psssSrvrServerSession, "<term>%s</term>\n", pucSrvrHttpEncodeXmlString(pstiSpiTermInfosPtr->pucTerm, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        iSrvrHttpSendf(psssSrvrServerSession, "<type>%u</type>\n", pstiSpiTermInfosPtr->uiType);
        iSrvrHttpSendf(psssSrvrServerSession, "<count>%u</count>\n", pstiSpiTermInfosPtr->uiCount);
        iSrvrHttpSendf(psssSrvrServerSession, "<documentCount>%u</documentCount>\n", pstiSpiTermInfosPtr->uiDocumentCount);
        iSrvrHttpSendf(psssSrvrServerSession, "</indexTermInfo>\n");
    }

    /* Close the index term info list */
    iSrvrHttpSendf(psssSrvrServerSession, "</indexTermInfoList>\n");

    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendIndexTermInfoOn()

    Purpose:    Send the index term info in Object Notation.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                pstiSpiTermInfos        spi term infos
                uiSpiTermInfosLength    spi term length

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendIndexTermInfoOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiTermInfo *pstiSpiTermInfos, 
    unsigned int uiSpiTermInfosLength
)
{

    int                     iError = SPI_NoError;
    unsigned char           *pucQuotePtr = NULL;
    unsigned char           *pucSeparatorPtr = NULL;
    unsigned char           pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiTermInfo      *pstiSpiTermInfosPtr = NULL;
    unsigned int            uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(((pstiSpiTermInfos != NULL) && (uiSpiTermInfosLength >= 0)) || ((pstiSpiTermInfos == NULL) && (uiSpiTermInfosLength == 0)));


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the index term info list */
    iSrvrHttpSendf(psssSrvrServerSession, "%sindexTermInfoList%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Open the index term info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sindexTermInfo%s %s [\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Send the index term info */
    for ( uiI = 0, pstiSpiTermInfosPtr = pstiSpiTermInfos; uiI < uiSpiTermInfosLength; uiI++, pstiSpiTermInfosPtr++ ) {

        iSrvrHttpSendf(psssSrvrServerSession, "{%sterm%s %s %s%s%s, ", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(pstiSpiTermInfosPtr->pucTerm, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        iSrvrHttpSendf(psssSrvrServerSession, "%stype%s %s %u, ", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pstiSpiTermInfosPtr->uiType);

        iSrvrHttpSendf(psssSrvrServerSession, "%scount%s %s %u, ", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pstiSpiTermInfosPtr->uiCount);

        iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentCount%s %s %u }%s\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pstiSpiTermInfosPtr->uiDocumentCount,
                (uiI < (uiSpiTermInfosLength - 1)) ? ", " : "");
    }

    /* Close the index term info */
    iSrvrHttpSendf(psssSrvrServerSession, "]\n");

    /* Close the index term info list */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendDocumentInfoXml()

    Purpose:    Send the document info in XML.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                psdiSpiDocumentInfo     spi document info

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendDocumentInfoXml
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiDocumentInfo *psdiSpiDocumentInfo
)
{

    int                         iError = SPI_NoError;
    unsigned char               pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
    unsigned int                uiI = 0;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(uiFormat == SRVR_HTTP_FORMAT_XML);
    ASSERT(psdiSpiDocumentInfo != NULL);


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the xml header */
    iSrvrHttpSendXmlHeader(psssSrvrServerSession, pucPath, pucQuery);

    /* Open the document info */
    iSrvrHttpSendf(psssSrvrServerSession, "<documentInfo>\n");

    /* Send the document info */
    iSrvrHttpSendf(psssSrvrServerSession, "<indexName>%s</indexName>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentInfo->pucIndexName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<documentKey>%s</documentKey>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentInfo->pucDocumentKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<title>%s</title>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentInfo->pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<languageCode>%s</languageCode>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentInfo->pucLanguageCode, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
    iSrvrHttpSendf(psssSrvrServerSession, "<rank>%u</rank>\n", psdiSpiDocumentInfo->uiRank);
    iSrvrHttpSendf(psssSrvrServerSession, "<termCount>%u</termCount>\n", psdiSpiDocumentInfo->uiTermCount);
    iSrvrHttpSendf(psssSrvrServerSession, "<ansiDate>%lu</ansiDate>\n", psdiSpiDocumentInfo->ulAnsiDate);

    /* Loop through each item, */
    for ( uiI = 0, psdiSpiDocumentItemsPtr = psdiSpiDocumentInfo->psdiSpiDocumentItems; uiI < psdiSpiDocumentInfo->uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++ ) {

        /* Open the document item */
        iSrvrHttpSendf(psssSrvrServerSession, "<documentItem>\n");

        /* Send the item name */
        iSrvrHttpSendf(psssSrvrServerSession, "<itemName>%s</itemName>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentItemsPtr->pucItemName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

        iSrvrHttpSendf(psssSrvrServerSession, "<mimeType>%s</mimeType>\n", pucSrvrHttpEncodeXmlString(psdiSpiDocumentItemsPtr->pucMimeType, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        iSrvrHttpSendf(psssSrvrServerSession, "<length>%u</length>\n", psdiSpiDocumentItemsPtr->uiLength);
    
        if ( bUtlStringsIsStringNULL(psdiSpiDocumentItemsPtr->pucUrl) == false ) {
            iSrvrHttpSendf(psssSrvrServerSession, "<url>%s</url>\n", pucSrvrHttpEncodeUrlString(psdiSpiDocumentItemsPtr->pucUrl, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));
        }

        /* Close the document item */
        iSrvrHttpSendf(psssSrvrServerSession, "</documentItem>\n");
    } 

    /* Close the document info */
    iSrvrHttpSendf(psssSrvrServerSession, "</documentInfo>\n");

    /* Send the xml footer */
    iSrvrHttpSendXmlFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendDocumentInfoOn()

    Purpose:    Send the document info in Object Notation.

    Parameters: psssSrvrServerSession   server session structure
                pucPath                 path
                pucQuery                query
                uiFormat                format
                psdiSpiDocumentInfo     spi document info

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSendDocumentInfoOn
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucPath,
    unsigned char *pucQuery,
    unsigned int uiFormat,
    struct spiDocumentInfo *psdiSpiDocumentInfo
)
{

    int                         iError = SPI_NoError;
    unsigned char               *pucQuotePtr = NULL;
    unsigned char               *pucSeparatorPtr = NULL;
    unsigned char               pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
    unsigned int                uiI = 0;
    

    ASSERT(psssSrvrServerSession != NULL);
    ASSERT((uiFormat == SRVR_HTTP_FORMAT_JSON) || (uiFormat == SRVR_HTTP_FORMAT_RUBY) || (uiFormat == SRVR_HTTP_FORMAT_PYTHON));
    ASSERT(psdiSpiDocumentInfo != NULL);


    /* Set the quote and the separator */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_JSON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_JSON;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_RUBY;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_RUBY;
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        pucQuotePtr = SRVR_HTTP_QUOTE_PYTHON;
        pucSeparatorPtr = SRVR_HTTP_SEPARATOR_PYTHON;
    }


    /* Send the http header */
    if ( uiFormat == SRVR_HTTP_FORMAT_JSON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_JSON, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_RUBY ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_RUBY, 0);
    }
    else if ( uiFormat == SRVR_HTTP_FORMAT_PYTHON ) {
        iSrvrHttpSendHttpHeader(psssSrvrServerSession, SRVR_HTTP_STATUS_OK, SRVR_HTTP_CONTENT_TYPE_TEXT_PYTHON, 0);
    }

    /* Send the object notation header */
    iSrvrHttpSendOnHeader(psssSrvrServerSession, pucPath, pucQuery);


    /* Open the document info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentInfo%s %s {\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Send the document info */
    iSrvrHttpSendf(psssSrvrServerSession, "%sindexName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psdiSpiDocumentInfo->pucIndexName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentKey%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psdiSpiDocumentInfo->pucDocumentKey, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%stitle%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psdiSpiDocumentInfo->pucTitle, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%slanguageName%s %s %s%s%s,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
            pucSrvrHttpEncodeOnString(psdiSpiDocumentInfo->pucLanguageCode, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

    iSrvrHttpSendf(psssSrvrServerSession, "%srank%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, psdiSpiDocumentInfo->uiRank);

    iSrvrHttpSendf(psssSrvrServerSession, "%stermCount%s %s %u,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, psdiSpiDocumentInfo->uiTermCount);

    iSrvrHttpSendf(psssSrvrServerSession, "%sansiDate%s %s %lu,\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, psdiSpiDocumentInfo->ulAnsiDate);

    /* Open the document item */
    iSrvrHttpSendf(psssSrvrServerSession, "%sdocumentItem%s %s [\n", pucQuotePtr, pucQuotePtr, pucSeparatorPtr);

    /* Loop through each item, */
    for ( uiI = 0, psdiSpiDocumentItemsPtr = psdiSpiDocumentInfo->psdiSpiDocumentItems; uiI < psdiSpiDocumentInfo->uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++ ) {

        /* Open the entry and send the item name */
        iSrvrHttpSendf(psssSrvrServerSession, "{%sitemName%s %s %s%s%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(psdiSpiDocumentItemsPtr->pucItemName, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        /* Send the mime type */
        iSrvrHttpSendf(psssSrvrServerSession, ", %smimeType%s %s %s%s%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                pucSrvrHttpEncodeOnString(psdiSpiDocumentItemsPtr->pucMimeType, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);

        iSrvrHttpSendf(psssSrvrServerSession, ", %slength%s%s %u", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, psdiSpiDocumentItemsPtr->uiLength);
    
        if ( bUtlStringsIsStringNULL(psdiSpiDocumentItemsPtr->pucUrl) == false ) {
            iSrvrHttpSendf(psssSrvrServerSession, ", %surl%s%s %s%s%s", pucQuotePtr, pucQuotePtr, pucSeparatorPtr, pucQuotePtr, 
                    pucSrvrHttpEncodeOnString(psdiSpiDocumentItemsPtr->pucUrl, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1), pucQuotePtr);
        }

        /* Close the entry */
        iSrvrHttpSendf(psssSrvrServerSession, "}%s", (uiI < (psdiSpiDocumentInfo->uiDocumentItemsLength - 1)) ? ", " : "");
    } 

    /* Close the document item */
    iSrvrHttpSendf(psssSrvrServerSession, " \n]\n");

    /* Close the document info */
    iSrvrHttpSendf(psssSrvrServerSession, "}\n");

    /* Send the object notation footer */
    iSrvrHttpSendOnFooter(psssSrvrServerSession);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendError()

    Purpose:    Write an http response header along with an error message

    Parameters: spiServerHandle     spi session structure
                uiHttpStatus        http status
                iError              error
                pucErrorMessage     error message

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendError
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned int uiHttpStatus,
    int iError,
    unsigned char *pucErrorMessage
)
{

    unsigned char   pucEncodedString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucErrorMessage) == false);


    /* Send the http header */
    iSrvrHttpSendHttpHeader(psssSrvrServerSession, uiHttpStatus, SRVR_HTTP_CONTENT_TYPE_TEXT_XML, 0);

    /* Send the rss header */
    iSrvrHttpSendRssHeader(psssSrvrServerSession, NULL, NULL);

    /* Send the error message */
    iSrvrHttpSendf(psssSrvrServerSession, "<item>\n<title>Error code: %d, Error message: '%s'</title>\n</item>\n", iError, 
            pucSrvrHttpEncodeXmlString(pucErrorMessage, pucEncodedString, SRVR_HTTP_LONG_STRING_LENGTH + 1));

    /* Send the rss footer */
    iSrvrHttpSendRssFooter(psssSrvrServerSession);


#if defined(SRVR_HTTP_ENABLE_HTTP_ERROR_LOGGING)
    /* Log the error */
    iUtlLogError(UTL_LOG_CONTEXT, "Error code: %d, Error message: '%s'.", iError, pucErrorMessage);
#endif /* defined(SRVR_HTTP_ENABLE_HTTP_ERROR_LOGGING) */


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpHandleSpiError()

    Purpose:    Will list an error message corresponding to the
                error code passed and set the error string return
                pointer

    Parameters: psssSrvrServerSession   server session structure
                iError                  error code
                pucIndexName            index name (optional)

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpHandleSpiError
(
    struct srvrServerSession *psssSrvrServerSession,
    int iError,
    unsigned char *pucIndexName
)
{

    unsigned char   pucErrorString[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};


    ASSERT(psssSrvrServerSession != NULL);


    /* Check the error code */
    if ( iError == SPI_NoError ) {
        return (SPI_NoError);
    }


    /* Get the error text, default to an unknown error */
    if ( iSpiGetErrorText(iError, pucErrorString, SRVR_HTTP_LONG_STRING_LENGTH + 1) != SPI_NoError ) {
        s_strnncpy(pucErrorString, "Unknown error", SRVR_HTTP_LONG_STRING_LENGTH + 1);
    }

    /* Append the index name to the error string */
    if ( bUtlStringsIsStringNULL(pucIndexName) == false ) {
        s_strnncat(pucErrorString, ", index: '", SRVR_HTTP_LONG_STRING_LENGTH + 1, SRVR_HTTP_LONG_STRING_LENGTH + 1);
        s_strnncat(pucErrorString, pucIndexName, SRVR_HTTP_LONG_STRING_LENGTH + 1, SRVR_HTTP_LONG_STRING_LENGTH + 1);
        s_strnncat(pucErrorString, "'", SRVR_HTTP_LONG_STRING_LENGTH + 1, SRVR_HTTP_LONG_STRING_LENGTH + 1);
    }


    /* Send the error */
    iSrvrHttpSendError(psssSrvrServerSession, SRVR_HTTP_STATUS_INTERNAL_SERVER_ERROR, iError, pucErrorString);


#if defined(SRVR_HTTP_ENABLE_SPI_ERROR_LOGGING)
    /* Log the error */
    iUtlLogError(UTL_LOG_CONTEXT, "%s.", pucErrorString);
#endif /* defined(SRVR_HTTP_ENABLE_SPI_ERROR_LOGGING) */


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* 
** ================================
** ===  Data sending functions  ===
** ================================
*/


/*

    Function:   iSrvrHttpSendf()

    Purpose:    Send data using vsnprintf(), note that this is limited to
                strings shorter than SRVR_HTTP_LONG_STRING_LENGTH.

    Parameters: spiServerHandle     spi session structure
                pucFormat           format
                ...                 args (optional)

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendf
(
    struct srvrServerSession *psssSrvrServerSession,
    unsigned char *pucFormat,
    ...
)
{

    int             iError = SPI_NoError;
    va_list         ap;
    unsigned char   pucBuffer[SRVR_HTTP_LONG_STRING_LENGTH + 1];


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(psssSrvrServerSession->pvUtlNet != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucFormat) == false);


    /* Print the format/arguments into the buffer */
    va_start(ap, pucFormat);
    vsnprintf(pucBuffer, SRVR_HTTP_LONG_STRING_LENGTH, pucFormat, ap);
    va_end(ap);

    
    /* Write the data */
    if ( (iError = iUtlNetWrite(psssSrvrServerSession->pvUtlNet, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, utl error: %d.", iError);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendEncodeXml()

    Purpose:    Send data encoding it for XML

    Parameters: spiServerHandle     spi session structure
                pvData              data
                uiDataLength        data length

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendEncodeXml
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvData,
    unsigned int uiDataLength
)
{

    int             iError = SPI_NoError;
    unsigned char   *pucDataPtr = (unsigned char *)pvData;
    unsigned int    uiRemainingDataLength = uiDataLength;
    unsigned char   pucBuffer[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    unsigned int    uiExtentLength = SRVR_HTTP_LONG_STRING_LENGTH / 2;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(((pvData != NULL) && (uiDataLength >= 0)) || ((pvData == NULL) && (uiDataLength == 0)));


    /* Initialize the data pointer and the remaining data length */
    pucDataPtr = (unsigned char *)pvData;
    uiRemainingDataLength = uiDataLength;

    /* Loop processing extent lengths */
    while ( uiRemainingDataLength > 0 ) {
        
        /* Get a pointer to the end of the extent we are going to process */
        unsigned char    *pucDataEndPtr = pucDataPtr + UTL_MACROS_MIN(uiExtentLength, uiRemainingDataLength);

        /* Save the last character and NULL terminate the string */
        unsigned char     ucDataCharacter = *pucDataEndPtr;
        *pucDataEndPtr = '\0';

        /* Encode the string into an XML safe string */
        pucSrvrHttpEncodeXmlString(pucDataPtr, pucBuffer, SRVR_HTTP_LONG_STRING_LENGTH + 1);
                
        /* Write the data */
        if ( (iError = iUtlNetWrite(psssSrvrServerSession->pvUtlNet, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, utl error: %d.", iError);
            return (iError);
        }
        
        /* Restore the last character */ 
        *pucDataEndPtr = ucDataCharacter;

        /* Update the current data pointer */
        pucDataPtr += uiExtentLength;
        
        /* Decrement the remaining length, this will set uiRemainingDataLength to 0 if there is no more data to process */
        uiRemainingDataLength -= UTL_MACROS_MIN(uiExtentLength, uiRemainingDataLength);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSendEncodeOn()

    Purpose:    Send data encoding it for Object Notation

    Parameters: spiServerHandle     spi session structure
                pvData              data
                uiDataLength        data length

    Globals:    none

    Returns:    SPI error

*/
static int iSrvrHttpSendEncodeOn
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvData,
    unsigned int uiDataLength
)
{

    int             iError = SPI_NoError;
    unsigned char   *pucDataPtr = (unsigned char *)pvData;
    unsigned int    uiRemainingDataLength = uiDataLength;
    unsigned char   pucBuffer[SRVR_HTTP_LONG_STRING_LENGTH + 1] = {'\0'};
    unsigned int    uiExtentLength = SRVR_HTTP_LONG_STRING_LENGTH / 2;


    ASSERT(psssSrvrServerSession != NULL);
    ASSERT(((pvData != NULL) && (uiDataLength >= 0)) || ((pvData == NULL) && (uiDataLength == 0)));


    /* Initialize the data pointer and the remaining data length */
    pucDataPtr = (unsigned char *)pvData;
    uiRemainingDataLength = uiDataLength;

    /* Loop processing extent lengths */
    while ( uiRemainingDataLength > 0 ) {
        
        /* Get a pointer to the end of the extent we are going to process */
        unsigned char    *pucDataEndPtr = pucDataPtr + UTL_MACROS_MIN(uiExtentLength, uiRemainingDataLength);

        /* Save the last character and NULL terminate the string */
        unsigned char     ucDataCharacter = *pucDataEndPtr;
        *pucDataEndPtr = '\0';

        /* Encode the string into a JSON safe string */
        pucSrvrHttpEncodeOnString(pucDataPtr, pucBuffer, SRVR_HTTP_LONG_STRING_LENGTH + 1);
                
        /* Write the data */
        if ( (iError = iUtlNetWrite(psssSrvrServerSession->pvUtlNet, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write a message to the client, utl error: %d.", iError);
            return (iError);
        }
        
        /* Restore the last character */ 
        *pucDataEndPtr = ucDataCharacter;

        /* Update the current data pointer */
        pucDataPtr += uiExtentLength;
        
        /* Decrement the length, this will set uiRemainingDataLength to 0 if there is no more data to process */
        uiRemainingDataLength -= UTL_MACROS_MIN(uiExtentLength, uiRemainingDataLength);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ==================================
** ===  URL extraction functions  ===
** ==================================
*/


/*

    Function:   iSrvrHttpGetQueryVariableValue()

    Purpose:    Extract the variable value for a variable name from the query

    Parameters: pucQuery            query
                pucVariableName     variable name
                ppucVariableValue   return pointer for the variable value (allocated)

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpGetQueryVariableValue
(
    unsigned char *pucQuery,
    unsigned char *pucVariableName,
    unsigned char **ppucVariableValue
)
{

    unsigned char   pucVariableSignature[SRVR_HTTP_SHORT_STRING_LENGTH + 1] = {'\0'};
    unsigned char   *pucVariableNamePtr = NULL;
    unsigned char   *pucVariableValueStartPtr = NULL;
    unsigned char   *pucVariableValueEndPtr = NULL;
    unsigned char   ucCharacter = '\0';
    

    ASSERT(bUtlStringsIsStringNULL(pucVariableName) == false);
    ASSERT(ppucVariableValue != NULL);


    /* Catch empty queries here */
    if ( bUtlStringsIsStringNULL(pucQuery) == true ) {
        return (SPI_NoError);
    }


    /* Create the variable signature, ie 'in' becomes 'in=' */
    snprintf(pucVariableSignature, SRVR_HTTP_SHORT_STRING_LENGTH + 1, "%s=", pucVariableName);
    
    
    /* Look for the variable signature, erase the variable value and return if we did not find it */
    if ( (pucVariableNamePtr = s_strstr(pucQuery, pucVariableSignature)) == NULL ) {
        return (SPI_NoError);
    }
    
    /* Set the variable value start pointer */
    pucVariableValueStartPtr = pucVariableNamePtr + s_strlen(pucVariableSignature);


    /* Set the variable value end from the variable value start, we need to do this to prime the loop below */
    pucVariableValueEndPtr = pucVariableValueStartPtr;
    
    /* Loop looking for the end of the variable value - we control the loop from within */
    while ( true ) {

        /* Get the next '&', break out if we cant find it */
        if ( (pucVariableValueEndPtr = s_strchr(pucVariableValueEndPtr, '&')) == NULL ) {
            break;
        }
    
        /* Check if this is a hex value, break out if it isn't */
        if ( !((*pucVariableValueEndPtr == '&') && 
                (((*(pucVariableValueEndPtr + 1) >= '0') && (*(pucVariableValueEndPtr + 1) <= '9')) || 
                        ((*(pucVariableValueEndPtr + 1) >= 'A') && (*(pucVariableValueEndPtr + 1) <= 'F')) || ((*(pucVariableValueEndPtr + 1) >= 'a') && (*(pucVariableValueEndPtr + 1) <= 'f'))) &&
                (((*(pucVariableValueEndPtr + 2) >= '0') && (*(pucVariableValueEndPtr + 2) <= '9')) || 
                        ((*(pucVariableValueEndPtr + 2) >= 'A') && (*(pucVariableValueEndPtr + 2) <= 'F')) || ((*(pucVariableValueEndPtr + 2) >= 'a') && (*(pucVariableValueEndPtr + 2) <= 'f'))) &&
                (*(pucVariableValueEndPtr + 3) == ';')) ) {
            
            break;
        }
        
        /* Increment the variable value end to the next byte and repeat the loop */
        pucVariableValueEndPtr++;
    }


    /* Save the character if the variable value end is set */
    if ( pucVariableValueEndPtr != NULL ) {
        ucCharacter = *pucVariableValueEndPtr;
        *pucVariableValueEndPtr = '\0';
    }

    /* Duplicate the variable value */
    if ( (*ppucVariableValue = s_strdup(pucVariableValueStartPtr)) == NULL ) {
        return (SPI_MemError);
    }

    /* Restore the character if the variable value end is set */
    if ( pucVariableValueEndPtr != NULL ) {
        *pucVariableValueEndPtr = ucCharacter;
    }


    /* Decode the variable value */
    iSrvrHttpDecodeUrlString(*ppucVariableValue);
    

    /* Return */
    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* 
** =====================================
** ===  Encoding/decoding functions  ===
** =====================================
*/


/*

    Function:   iSrvrHttpDecodeUrlString()

    Purpose:    Decode an HTTP encoded string in place

    Parameters: pucString       string (optional)

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpDecodeUrlString
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringReadPtr = NULL;
    unsigned char   *pucStringWritePtr = NULL;
    

    /* Skip empty strings */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (SPI_NoError);
    }


    /* Decode the string */
    for ( pucStringReadPtr = pucString, pucStringWritePtr = pucString; *pucStringReadPtr != '\0'; pucStringWritePtr++ ) {
        
        /* Decode encoded byte */
        if ( *pucStringReadPtr == '%' ) {

            /* Save the end character for the encoded byte */
            unsigned char ucCharacter = *(pucStringReadPtr + 3);

            /* Null terminate the encoded byte, decode (base 16), and replace the end character */
            *(pucStringReadPtr + 3) = '\0';
            *pucStringWritePtr = (unsigned char)s_strtol(pucStringReadPtr + 1, NULL, 16);
            *(pucStringReadPtr + 3) = ucCharacter;

            pucStringReadPtr += 3;
        }

        /* Convert '+' to space */
        else if ( *pucStringReadPtr == '+' ) {
            *pucStringWritePtr = ' ';
            pucStringReadPtr++;
        }

        /* Check for an escape on '&' */
        else if ( *pucStringReadPtr == '&' ) {
    
            /* Convert "&amp;" to '&' */
            if ( s_strncmp(pucStringReadPtr, "&amp;", 5) == 0 ) {
                *pucStringWritePtr = '&';
                pucStringReadPtr += 5;
            }
            /* Convert "&gt;" to '>' */
            else if ( s_strncmp(pucStringReadPtr, "&gt;", 4) == 0 ) {
                *pucStringWritePtr = '>';
                pucStringReadPtr += 4;
            }
            /* Convert "&lt;" to '<' */
            else if ( s_strncmp(pucStringReadPtr, "&lt;", 4) == 0 ) {
                *pucStringWritePtr = '<';
                pucStringReadPtr += 4;
            }
            /* Convert "&apos;" to "'" */
            else if ( s_strncmp(pucStringReadPtr, "&apos;", 6) == 0 ) {
                *pucStringWritePtr = '\'';
                pucStringReadPtr += 6;
            }
            /* Convert "&quot;" to '"' */
            else if ( s_strncmp(pucStringReadPtr, "&quot;", 6) == 0 ) {
                *pucStringWritePtr = '"';
                pucStringReadPtr += 6;
            }
            /* Copy the byte */
            else {
                *pucStringWritePtr = *pucStringReadPtr;
                pucStringReadPtr++;
            }
        }

        /* Copy the byte */
        else {
            *pucStringWritePtr = *pucStringReadPtr;
            pucStringReadPtr++;
        }
    }

    /* NULL terminate the string */
    *pucStringWritePtr = '\0';
    

    /* Return */
    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucSrvrHttpEncodeUrlString()

    Purpose:    Encode a string to an HTTP string

    Parameters: pucString               string (optional)
                pucEncodedString        return pointer for the encoded string
                uiEncodedStringLength   length of the return pointer for the encoded string

    Globals:    none

    Returns:    SPI error code

*/
static unsigned char *pucSrvrHttpEncodeUrlString
(
    unsigned char *pucString,
    unsigned char *pucEncodedString,
    unsigned int uiEncodedStringLength
)
{

    unsigned char   *pucStringPtr = NULL;
    unsigned char   *pucEncodedStringPtr = NULL;
    

    ASSERT(pucEncodedString != NULL);
    ASSERT(uiEncodedStringLength > 6);


    /* Skip empty strings */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        pucEncodedString[0] = '\0';
        return (pucEncodedString);
    }


    /* Encode the string, stop when we run out of string, or of space - note the '- 7' */
    for ( pucStringPtr = pucString, pucEncodedStringPtr = pucEncodedString; 
            (*pucStringPtr != '\0') && ((pucEncodedStringPtr - pucEncodedString) < (uiEncodedStringLength  - 7)); 
            pucStringPtr++ ) {
        
        /* Convert '&' to "&amp;" */
        if ( *pucStringPtr == '&' ) {
            s_strcpy(pucEncodedStringPtr, "&amp;");
            pucEncodedStringPtr += 5;
        }
        /* Convert '>' to "&gt;" */
        else if ( *pucStringPtr == '>' ) {
            s_strcpy(pucEncodedStringPtr, "&gt;");
            pucEncodedStringPtr += 4;
        }
        /* Convert '<' to "&lt;" */
        else if ( *pucStringPtr == '<' ) {
            s_strcpy(pucEncodedStringPtr, "&lt;");
            pucEncodedStringPtr += 4;
        }
        /* Convert "'" to "&apos;" */
        else if ( *pucStringPtr == '<' ) {
            s_strcpy(pucEncodedStringPtr, "&apos;");
            pucEncodedStringPtr += 6;
        }
        /* Convert '"' to "&quot;" */
        else if ( *pucStringPtr == '<' ) {
            s_strcpy(pucEncodedStringPtr, "&quot;");
            pucEncodedStringPtr += 6;
        }
        /* Convert space to '+' */
        else if ( *pucStringPtr == ' ') {
            *pucEncodedStringPtr = '+';
            pucEncodedStringPtr++;
        }
        else if ( (*pucStringPtr < 32) || (*pucStringPtr > 125) ) {
            sprintf(pucEncodedStringPtr, "%X", *pucStringPtr);
            pucEncodedStringPtr += 3;
        }
        /* Copy the byte */
        else {
            *pucEncodedStringPtr = *pucStringPtr;
            pucEncodedStringPtr++;
        }
    }
    
    /* NULL terminate the string */
    *pucEncodedStringPtr = '\0';


    /* Return */
    return (pucEncodedString);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpDecodeXmlString()

    Purpose:    Decode an XML encoded string in place

    Parameters: pucString       string (optional)

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpDecodeXmlString
(
    unsigned char *pucString
)
{

    /* Check the string */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (SPI_NoError);
    }

    /* Decode the string */
    if ( iUtlStringsDecodeXmlString(pucString) == UTL_NoError ) {
        return (SPI_NoError);
    }


    /* Return - error */
    return (SPI_MiscError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucSrvrHttpEncodeXmlString()

    Purpose:    Encode a string to an XML string

    Parameters: pucString               string (optional)
                pucEncodedString        return pointer for the encoded string
                uiEncodedStringLength   length of the return pointer for the encoded string

    Globals:    none

    Returns:    pointer to the encoded string, null on error

*/
static unsigned char *pucSrvrHttpEncodeXmlString
(
    unsigned char *pucString,
    unsigned char *pucEncodedString,
    unsigned int uiEncodedStringLength
)
{

    ASSERT(pucEncodedString != NULL);
    ASSERT(uiEncodedStringLength > 6);


    /* Check the string */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (pucEncodedString);
    }

    /* Encode the string */
    if ( iUtlStringsEncodeXmlString(pucString, pucEncodedString, uiEncodedStringLength) == UTL_NoError ) {
        return (pucEncodedString);
    }


    /* Return - error */
    return (NULL);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpDecodeOnString()

    Purpose:    Decode an Object Notation encoded string in place

    Parameters  pucString       string (optional)

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpDecodeOnString
(
    unsigned char *pucString
)
{

    unsigned char   *pucStringReadPtr = NULL;
    unsigned char   *pucStringWritePtr = NULL;
    

    /* Skip empty strings */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        return (SPI_NoError);
    }


    /* Decode the string, note that we check the write pointer for the terminating NULL
    ** because it will skipped by the read pointer if it is escaped (as is allowed by
    ** the Object Notation spec)
    */
    for ( pucStringReadPtr = pucString, pucStringWritePtr = pucString; *pucStringWritePtr != '\0'; pucStringWritePtr++ ) {
        
        /* Skip over the escaped part of a character */
        if ( *pucStringReadPtr == '\\' ) {
            pucStringReadPtr++;
        }

        /* Copy the byte */
        *pucStringWritePtr = *pucStringReadPtr;
        pucStringReadPtr++;
    }
    
    /* NULL terminate the string - redundant */
    *pucStringWritePtr = '\0';


    /* Return */
    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucSrvrHttpEncodeOnString()

    Purpose:    Encode a string to an Object Notation string

    Parameters: pucString               string (optional)
                pucEncodedString        return pointer for the encoded string
                uiEncodedStringLength   length of the return pointer for the encoded string

    Globals:    none

    Returns:    SPI error code

*/
static unsigned char *pucSrvrHttpEncodeOnString
(
    unsigned char *pucString,
    unsigned char *pucEncodedString,
    unsigned int uiEncodedStringLength
)
{

    unsigned char   *pucStringPtr = NULL;
    unsigned char   *pucEncodedStringPtr = NULL;
    

    ASSERT(pucEncodedString != NULL);
    ASSERT(uiEncodedStringLength > 2);


    /* Skip empty strings */
    if ( bUtlStringsIsStringNULL(pucString) == true ) {
        pucEncodedString[0] = '\0';
        return (pucEncodedString);
    }


    /* Encode the string, stop when we run out of string, or of space - note the '- 3' */
    for ( pucStringPtr = pucString, pucEncodedStringPtr = pucEncodedString; 
            (*pucStringPtr != '\0') && ((pucEncodedStringPtr - pucEncodedString) < (uiEncodedStringLength - 3)); 
            pucStringPtr++ ) {
        
        /* Escape double quotes, single quotes, backslash and characters less than ascii 32 */
        if ( (*pucStringPtr == '"') || (*pucStringPtr == '\'') || (*pucStringPtr == '\\') || (*pucStringPtr < 32) ) {
            *pucEncodedStringPtr = '\\';
            pucEncodedStringPtr++;
        }

        /* Copy the byte */
        *pucEncodedStringPtr = *pucStringPtr;
        pucEncodedStringPtr++;
    }
    
    /* NULL terminate the string */
    *pucEncodedStringPtr = '\0';


    /* Return */
    return (pucEncodedString);

}


/*---------------------------------------------------------------------------*/


/* 
** ===========================================
** ===  SPI interaction support functions  ===
** ===========================================
*/


/*

    Function:   iSrvrHttpOpenIndex()

    Purpose:    Intermediate Layer function for opening a index, the call is 
                passed onto the SPI function which returns an error code.
                This code is processed and a Http error code is returned
                if needed. If the index was correctly opened, then the index
                object is returned via the return variable.

    Parameters: pssSpiSession   spi session structure
                pucIndexName    the name of the index to open
                ppvIndex        return pointer for the search index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpOpenIndex
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    void **ppvIndex
)
{

    int     iError = SPI_NoError;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(ppvIndex != NULL);


    /* Open the index */
    if ( (iError = iSpiOpenIndex(pssSpiSession, pucIndexName, ppvIndex)) != SPI_NoError ) {
        if ( *ppvIndex != NULL ) {
            iSrvrHttpCloseIndex(pssSpiSession, *ppvIndex);
            *ppvIndex = NULL;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpCloseIndex()

    Purpose:    Intermediate layer function for closing a index, the call is 
                passed onto the SPI function which returns an error code.

    Parameters: pvIndex         the search index structure to be close
                pssSpiSession   spi session structure

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpCloseIndex
(
    struct spiSession *pssSpiSession,
    void *pvIndex
)
{

    int     iError = SPI_NoError;


    ASSERT(pvIndex != NULL);
    ASSERT(pssSpiSession != NULL);


    /* Close the index  */
    iError = iSpiCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpSearchIndex()

    Purpose:    Run a search on the index and populate the results array with
                the result

    Parameters: pssSpiSession               spi session structure
                pucIndexNames               comma delimited list of index names
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                ppssrSpiSearchResponse      return pointer for the spi search response structure

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpSearchIndex
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexNames,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex, 
    unsigned int uiEndIndex,
    struct spiSearchResponse **ppssrSpiSearchResponse
)
{

    int             iError = SPI_NoError;
    void            *pvIndex = NULL;
    void            **ppvIndexList = NULL;
    unsigned int    uiI = 0;
    unsigned char   *pucIndexNamesCopy = NULL;
    unsigned char   *pucIndexNamePtr = NULL;
    unsigned char   *pucIndexNamesCopyStrtokPtr = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexNames) == false);
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(ppssrSpiSearchResponse != NULL);


    /* Make a copy of the index names, this is because s_strtok_r() destroys it */
    if ( (pucIndexNamesCopy = (unsigned char *)s_strdup(pucIndexNames)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromlSearchIndex;
    }

    /* Loop parsing the index names and opening them */
    for ( pucIndexNamePtr = s_strtok_r(pucIndexNamesCopy, SRVR_HTTP_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI = 0; 
            pucIndexNamePtr != NULL; 
            pucIndexNamePtr = s_strtok_r(NULL, SRVR_HTTP_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI++ ) {

        void **ppvIndexListPtr = NULL;

        /* Open the index */
        if ( (iError = iSrvrHttpOpenIndex(pssSpiSession, pucIndexNamePtr, &pvIndex)) != SPI_NoError ) {
            /* Failed to open this index, so we bail out (iSrvrHttpOpenIndex() reports its own errors) */
            goto bailFromlSearchIndex;
        }

        /* Extend the search index structure list */
        if ( (ppvIndexListPtr = (void **)s_realloc(ppvIndexList, (size_t)(sizeof(void *) * (uiI + 2)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromlSearchIndex;
        }

        /* Hand over the pointer */
        ppvIndexList = ppvIndexListPtr;

        /* Add the search index structure */
        ppvIndexList[uiI] = pvIndex;

        /* NULL terminate the index list */
        ppvIndexList[uiI + 1] = NULL;
    }

    /* Free the index names copy, we are done with it */
    s_free(pucIndexNamesCopy);


    /* Run the search */
    iError = iSpiSearchIndex(pssSpiSession, ppvIndexList, pucLanguageCode, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
            uiStartIndex, uiEndIndex, ppssrSpiSearchResponse);



    /* Bail label */
    bailFromlSearchIndex:


    /* Close the index and free the index list */
    if ( ppvIndexList != NULL ) {
        for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
            iSrvrHttpCloseIndex(pssSpiSession, ppvIndexList[uiI]);
        }
        s_free(ppvIndexList);
    }


    /* Return */
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpRetrieveDocument()

    Purpose:    Retrieve the document from the index. If the retrieval was
                successful, a pointer to a document will be returned. This 
                pointer will have to be freed by the calling function.

    Parameters: pssSpiSession       spi session structure
                pucIndexName        index name
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                ppvData             return pointer for the data retrieved
                puiDataLength       return pointer for the length of the data retrieved

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpRetrieveDocument
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int     iError = SPI_NoError;
    void    *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(ppvData != NULL);
    ASSERT(puiDataLength != NULL);


    /* Open the index */
    if ( (iError = iSrvrHttpOpenIndex(pssSpiSession, pucIndexName, &pvIndex)) != SPI_NoError ) {
        return (iError);
    }


    /* Retrieve the document */
    iError = iSpiRetrieveDocument(pssSpiSession, pvIndex, pucDocumentKey, pucItemName, pucMimeType, 
                SPI_CHUNK_TYPE_DOCUMENT, 0, 0, ppvData, puiDataLength);


    /* Close the index */
    iSrvrHttpCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrHttpGetDocumentInfo()

    Purpose:    Get the document info from the index.

    Parameters: pssSpiSession           spi session structure
                pucIndexName            index name
                pucDocumentKey          document key
                ppsdiSpiDocumentInfo    return pointer for the document info

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrHttpGetDocumentInfo
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    struct spiDocumentInfo **ppsdiSpiDocumentInfo
)
{

    int     iError = SPI_NoError;
    void    *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(ppsdiSpiDocumentInfo != NULL);


    /* Open the index */
    if ( (iError = iSrvrHttpOpenIndex(pssSpiSession, pucIndexName, &pvIndex)) != SPI_NoError ) {
        return (iError);
    }


    /* Get the document info */
    iError = iSpiGetDocumentInfo(pssSpiSession, pvIndex, pucDocumentKey, ppsdiSpiDocumentInfo);


    /* Close the index */
    iSrvrHttpCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/
