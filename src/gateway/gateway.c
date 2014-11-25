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

    Module:     gateway.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 Dec 1995

    Purpose:    This serves as a gateway for a client. It allows the creation of logical index 
                which are composed of multiple physical index.

                A logical index is defined in the configuration file using a index location 
                parameter, for example for a index named 'foo', the definition would be as follows:

                    index-location:foo=lwps://sun.fsconsult.com/foo1,lwps://sun.fsconsult.com/foo2

                In this instance the logical index name is 'foo', and the physical index
                are called 'foo1' and 'foo2', and are located on sun.fsconsult.com, and are
                accessed using the LWPS protocol. The location is defined as a URL for familiarity.


                Another instance of a logical index is a fully defined URL, for example if 
                one wanted to define a index 'foo', located on the 'sun.fsconsult.com' host
                and accessed using the 'lwps' protocol, the index name would be as follows:

                    lwps://sun.fsconsult.com/foo

                In this instance the server is really acting as a gateway, this would only really
                be used when one needed to convert from one protocol to another.

                The other server characteristics are defined in the configuration file.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "spi.h"

#include "lwps.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.gateway.gateway"


/* Enable the expensive computation of the total results estimate when doing partial searches */
#define GTWY_ENABLE_EXPENSIVE_TOTAL_RESULTS_ESTIMATE



/*---------------------------------------------------------------------------*/


/* The gateway configuration file must be located in the gateway configuration 
** directory it if is to be used.
**
** It contains symbols which allows the gateway adminstrator to configure
** various aspects of the gateway such as:
**
**  - version
**      - version
**
**  - gateway information
**      - gateway description
**      - gateway administrator name
**      - gateway administrator email
**      - gateway ranking algorithm
**      - gateway connection timeout
**      - gateway search timeout
**      - gateway retrieval timeout
**      - gateway information timeout
**      - gateway mirror affinity
**      - gateway information cache timeout
**      - gateway allow search overrides
**
**  - protocol configuration
**      - gateway network protocol
**      - gateway send init
**
**  - index configuration
**      - index hosts (autodiscovery)/ (autodiscovery, specific index)
**      - index (autodiscovery)
**      - index location (fixed location, specific index)
**      - index sort orders
**      - index maximum segments searched
**      - index minimum segments searched
**      - index connection policy
**      - index connection error
**      - index search error
**      - index retrieval error
**
**  - index information
**      - index description
**      - index language code
**      - index tokenizer name
**      - index stemmer name
**      - index stop list name
**
**  - index field information
**      - field names
**      - field description
**      - field type
**
**  - index document information
**      - document item names
**      - document mime types
*/

#define GTWY_CONFIG_FILE_NAME                                       (unsigned char *)"gateway.cf"

#define GTWY_CONFIG_VERSION                                         (unsigned char *)"version"

#define GTWY_CONFIG_GATEWAY_DESCRIPTION                             (unsigned char *)"gateway-description"
#define GTWY_CONFIG_GATEWAY_ADMIN_NAME                              (unsigned char *)"gateway-admin-name"
#define GTWY_CONFIG_GATEWAY_ADMIN_EMAIL                             (unsigned char *)"gateway-admin-email"
#define GTWY_CONFIG_GATEWAY_RANKING_ALGORITHM                       (unsigned char *)"gateway-ranking-algorithm"
#define GTWY_CONFIG_GATEWAY_CONNECTION_TIMEOUT                      (unsigned char *)"gateway-connection-timeout"
#define GTWY_CONFIG_GATEWAY_SEARCH_TIMEOUT                          (unsigned char *)"gateway-search-timeout"
#define GTWY_CONFIG_GATEWAY_RETRIEVAL_TIMEOUT                       (unsigned char *)"gateway-retrieval-timeout"
#define GTWY_CONFIG_GATEWAY_INFORMATION_TIMEOUT                     (unsigned char *)"gateway-information-timeout"
#define GTWY_CONFIG_GATEWAY_MIRROR_AFFINITY                         (unsigned char *)"gateway-mirror-affinity"
#define GTWY_CONFIG_GATEWAY_INFORMATION_CACHE_TIMEOUT               (unsigned char *)"gateway-information-cache-timeout"
#define GTWY_CONFIG_GATEWAY_ALLOW_SEARCH_OVERRIDES                  (unsigned char *)"gateway-allow-search-overrides"   /* yes/no */

#define GTWY_CONFIG_GATEWAY_NETWORK_PROTOCOL                        (unsigned char *)"gateway-network-protocol"         /* tcp/udp */
#define GTWY_CONFIG_GATEWAY_SEND_INIT                               (unsigned char *)"gateway-send-init"                /* yes/no */

#define GTWY_CONFIG_INDEX_HOSTS                                     (unsigned char *)"index-hosts"
#define GTWY_CONFIG_INDEX                                           (unsigned char *)"index"
#define GTWY_CONFIG_INDEX_LOCATION                                  (unsigned char *)"index-location"
#define GTWY_CONFIG_INDEX_SORT_ORDERS                               (unsigned char *)"index-sort-orders"
#define GTWY_CONFIG_INDEX_MAXIMUM_SEGMENTS_SEARCHED                 (unsigned char *)"index-maximum-segments-searched"
#define GTWY_CONFIG_INDEX_MINIMUM_SEGMENTS_SEARCHED                 (unsigned char *)"index-minimum-segments-searched"
#define GTWY_CONFIG_INDEX_CONNECTION_POLICY                         (unsigned char *)"index-connection-policy"          /* strict/lazy */
#define GTWY_CONFIG_INDEX_CONNECTION_ERROR                          (unsigned char *)"index-connection-error"           /* ignore/fail */
#define GTWY_CONFIG_INDEX_SEARCH_ERROR                              (unsigned char *)"index-search-error"               /* ignore/fail */
#define GTWY_CONFIG_INDEX_RETRIEVAL_ERROR                           (unsigned char *)"index-retrieval-error"            /* ignore/fail */

#define GTWY_CONFIG_INDEX_DESCRIPTION                               (unsigned char *)"index-description"
#define GTWY_CONFIG_INDEX_LANGUAGE_NAME                             (unsigned char *)"index-language-name"
#define GTWY_CONFIG_INDEX_TOKENIZER_NAME                            (unsigned char *)"index-tokenizer-name"
#define GTWY_CONFIG_INDEX_STEMMER_NAME                              (unsigned char *)"index-stemmer-name"
#define GTWY_CONFIG_INDEX_STOP_LIST_NAME                            (unsigned char *)"index-stop-list-name"

#define GTWY_CONFIG_INDEX_FIELD_NAMES                               (unsigned char *)"index-field-name"
#define GTWY_CONFIG_INDEX_FIELD_DESCRIPTION                         (unsigned char *)"index-field-description"
#define GTWY_CONFIG_INDEX_FIELD_TYPE                                (unsigned char *)"index-field-type"


/* Field type defines - mirrors ./src/spi/spi.h */
#define GTWY_INDEX_FIELD_TYPE_UNKNOWN                               (unsigned char *)"unknown"
#define GTWY_INDEX_FIELD_TYPE_TEXT                                  (unsigned char *)"text"
#define GTWY_INDEX_FIELD_TYPE_NUMBER                                (unsigned char *)"number"
#define GTWY_INDEX_FIELD_TYPE_DATE                                  (unsigned char *)"date"


/*---------------------------------------------------------------------------*/


/* Connection timeout default, 10 milliseconds (expressed in milliseconds) */
#define GTWY_GATEWAY_CONNECTION_TIMEOUT_DEFAULT                     (10)

/* Search timeout default, 60 seconds (expressed in milliseconds) */
#define GTWY_GATEWAY_SEARCH_TIMEOUT_DEFAULT                         (60000)

/* Retrieval timeout default, 5 seconds (expressed in milliseconds) */
#define GTWY_GATEWAY_RETRIEVAL_TIMEOUT_DEFAULT                      (5000)

/* Information timeout default, 5 seconds (expressed in milliseconds) */
#define GTWY_GATEWAY_INFORMATION_TIMEOUT_DEFAULT                    (5000)


/* Mirror affinity default, -1 (positive number if defined) */
#define GTWY_GATEWAY_MIRROR_AFFINITY_DEFAULT                        (-1)


/* Information cache timeout default, 10 minutes (expressed in seconds) */
#define GTWY_GATEWAY_INFORMATION_CACHE_TIMEOUT_DEFAULT              (600)


/*---------------------------------------------------------------------------*/


/* Gateway index states */
#define GTWY_INDEX_CONNECTION_STATE_INVALID                         (0)
#define GTWY_INDEX_CONNECTION_STATE_DISCONNECTED                    (1)
#define GTWY_INDEX_CONNECTION_STATE_CONNECTED                       (2)

#define GTWY_INDEX_CONNECTION_STATE_VALID(n)                        (((n) >= GTWY_INDEX_CONNECTION_STATE_DISCONNECTED) && \
                                                                            ((n) <= GTWY_INDEX_CONNECTION_STATE_CONNECTED))


/* Gateway mirror states */
#define GTWY_MIRROR_CONNECTION_STATE_INVALID                        (0)
#define GTWY_MIRROR_CONNECTION_STATE_DISCONNECTED                   (1)
#define GTWY_MIRROR_CONNECTION_STATE_CONNECTED                      (2)
#define GTWY_MIRROR_CONNECTION_STATE_PERMANENT_ERROR                (3)
#define GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR                (4)

#define GTWY_MIRROR_CONNECTION_STATE_VALID(n)                       (((n) >= GTWY_MIRROR_CONNECTION_STATE_DISCONNECTED) && \
                                                                            ((n) <= GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR))


/*---------------------------------------------------------------------------*/


/* Yes/No strings */
#define GTWY_YES                                                    (unsigned char *)"yes"
#define GTWY_NO                                                     (unsigned char *)"no"


/* Protocol names */
#define GTWY_PROTOCOL_UDP_NAME                                      UTL_NET_PROTOCOL_UDP_NAME
#define GTWY_PROTOCOL_TCP_NAME                                      UTL_NET_PROTOCOL_TCP_NAME

/* Protocol IDs */
#define GTWY_PROTOCOL_UDP_ID                                        UTL_NET_PROTOCOL_UDP_ID
#define GTWY_PROTOCOL_TCP_ID                                        UTL_NET_PROTOCOL_TCP_ID


/* Index origin */
#define GTWY_INDEX_ORIGIN_UNKNOWN_ID                                (0)
#define GTWY_INDEX_ORIGIN_FROM_URL_ID                               (1)
#define GTWY_INDEX_ORIGIN_FROM_CONFIG_ID                            (2)


/* Host separator */
#define GTWY_HOST_SEPARATORS                                        (unsigned char *)", "


/* Index location separators */
#define GTWY_INDEX_LOCATION_MIRROR_SEPARATORS                       (unsigned char *)"|"
#define GTWY_INDEX_LOCATION_SEGMENT_SEPARATORS                      (unsigned char *)", "


/* Index sort orders separators */
#define GTWY_INDEX_SORT_ORDERS_SEPARATORS                           (unsigned char *)", "


/* Index segment regex start and end */
#define GTWY_INDEX_SEGMENT_REGEX_START                              (unsigned char *)"["
#define GTWY_INDEX_SEGMENT_REGEX_END                                (unsigned char *)"]"



/* Field name separators */
#define GTWY_FIELD_NAME_SEPARATORS                                  (unsigned char *)", "


/* Index sort */
#define GTWY_INDEX_SORT_ORDER_INVALID_ID                            (0)
#define GTWY_INDEX_SORT_ORDER_ASC_ID                                (1)
#define GTWY_INDEX_SORT_ORDER_DESC_ID                               (2)
#define GTWY_INDEX_SORT_NONE_ID                                     (3)

#define GTWY_INDEX_SORT_ORDER_ASC                                   (unsigned char *)"asc"
#define GTWY_INDEX_SORT_ORDER_DESC                                  (unsigned char *)"desc"
#define GTWY_INDEX_SORT_ORDER_ABR_ASC                               (unsigned char *)"a"
#define GTWY_INDEX_SORT_ORDER_ABR_DESC                              (unsigned char *)"d"

#define GTWY_INDEX_SORT_FIELD_NAME_RELEVANCE                        (unsigned char *)"relevance"
#define GTWY_INDEX_SORT_FIELD_NAME_RANK                             (unsigned char *)"rank"
#define GTWY_INDEX_SORT_FIELD_NAME_DATE                             (unsigned char *)"date"
#define GTWY_INDEX_SORT_FIELD_NAME_ABR_RELEVANCE                    (unsigned char *)"r"
#define GTWY_INDEX_SORT_FIELD_NAME_ABR_RANK                         (unsigned char *)"rk"
#define GTWY_INDEX_SORT_FIELD_NAME_ABR_DATE                         (unsigned char *)"d"

#define GTWY_MODIFIER_SORT_STRING                                   (unsigned char *)"{sort:"
#define GTWY_MODIFIER_SORT_NONE_STRING                              (unsigned char *)"{sort:none}"
#define GTWY_MODIFIER_SORT_ABR_STRING                               (unsigned char *)"{s:"
#define GTWY_MODIFIER_SORT_NONE_ABR_STRING                          (unsigned char *)"{s:n}"


/* Early search completion modifier */
#define GTWY_MODIFIER_GTWY_EARLY_COMPLETION_ENABLE_STRING           (unsigned char *)"{gtwy_early_completion:enable}"
#define GTWY_MODIFIER_GTWY_EARLY_COMPLETION_DISABLE_STRING          (unsigned char *)"{gtwy_early_completion:disable}"
#define GTWY_MODIFIER_GTWY_EARLY_COMPLETION_ENABLE_ABR_STRING       (unsigned char *)"{gec:e}"
#define GTWY_MODIFIER_GTWY_EARLY_COMPLETION_DISABLE_ABR_STRING      (unsigned char *)"{gec:d}"

/* Timeout modifiers */
#define GTWY_MODIFIER_GTWY_CONNECTION_TIMEOUT_STRING                (unsigned char *)"{gtwy_connection_timeout:"
#define GTWY_MODIFIER_GTWY_SEARCH_TIMEOUT_STRING                    (unsigned char *)"{gtwy_search_timeout:"
#define GTWY_MODIFIER_GTWY_RETRIEVAL_TIMEOUT_STRING                 (unsigned char *)"{gtwy_retrieval_timeout:"
#define GTWY_MODIFIER_GTWY_INFORMATION_TIMEOUT_STRING               (unsigned char *)"{gtwy_information_timeout:"
#define GTWY_MODIFIER_GTWY_CONNECTION_TIMEOUT_ABR_STRING            (unsigned char *)"{gct:"
#define GTWY_MODIFIER_GTWY_SEARCH_TIMEOUT_ABR_STRING                (unsigned char *)"{gst:"
#define GTWY_MODIFIER_GTWY_RETRIEVAL_TIMEOUT_ABR_STRING             (unsigned char *)"{grt:"
#define GTWY_MODIFIER_GTWY_INFORMATION_TIMEOUT_ABR_STRING           (unsigned char *)"{git:"

/* Segments searched modifiers */
#define GTWY_MODIFIER_GTWY_MAXIMUM_SEGMENTS_SEARCHED_STRING         (unsigned char *)"{gtwy_segments_searched_maximum:"
#define GTWY_MODIFIER_GTWY_MINIMUM_SEGMENTS_SEARCHED_STRING         (unsigned char *)"{gtwy_segments_searched_minimum:"
#define GTWY_MODIFIER_GTWY_MAXIMUM_SEGMENTS_SEARCHED_ABR_STRING     (unsigned char *)"{gssmx:"
#define GTWY_MODIFIER_GTWY_MINIMUM_SEGMENTS_SEARCHED_ABR_STRING     (unsigned char *)"{gssmn:"

/* Mirror affinity modifier */
#define GTWY_MODIFIER_GTWY_MIRROR_AFFINITY_STRING                    (unsigned char *)"{gtwy_mirror_affinity:"
#define GTWY_MODIFIER_GTWY_MIRROR_AFFINITY_ABR_STRING                (unsigned char *)"{gma:"

/* Modifier separator character */
#define GTWY_MODIFIER_SEPARATOR_CHARACTER                            (unsigned char)':'


/* Index connection policy */
#define GTWY_INDEX_CONNECTION_POLICY_STRICT                         (unsigned char *)"strict"
#define GTWY_INDEX_CONNECTION_POLICY_LAZY                           (unsigned char *)"lazy"

/* Index error handling */
#define GTWY_INDEX_ERROR_IGNORE                                     (unsigned char *)"ignore"
#define GTWY_INDEX_ERROR_FAIL                                       (unsigned char *)"fail"


/* Index maximum segments searched default */
#define GTWY_INDEX_MAXIMUM_SEGMENTS_SEARCHED_DEFAULT                (0)

/* Index minimum segments searched default */
#define GTWY_INDEX_MINIMUM_SEGMENTS_SEARCHED_DEFAULT                (0)


/* Index flags */
#define GTWY_NO_BIT_MASK                                            (0)
#define GTWY_IGNORE_CONNECTION_ERROR_BIT_MASK                       (1 << 0)
#define GTWY_IGNORE_SEARCH_ERROR_BIT_MASK                           (1 << 1)
#define GTWY_IGNORE_RETRIEVAL_ERROR_BIT_MASK                        (1 << 2)
#define GTWY_LAZY_CONNECTION_POLICY_BIT_MASK                        (1 << 3)

/* Index flags support macros (to set and extract bitmap flags) */
#define bGtwyIgnoreConnectionError(f)                               (((f) & GTWY_IGNORE_CONNECTION_ERROR_BIT_MASK) > 0 ? true : false)
#define bGtwyIgnoreSearchError(f)                                   (((f) & GTWY_IGNORE_SEARCH_ERROR_BIT_MASK) > 0 ? true : false)
#define bGtwyIgnoreRetrievalError(f)                                (((f) & GTWY_IGNORE_RETRIEVAL_ERROR_BIT_MASK) > 0 ? true : false)
#define bGtwyLazyConnectionPolicy(f)                                (((f) & GTWY_LAZY_CONNECTION_POLICY_BIT_MASK) > 0 ? true : false)

#define bGtwyFailConnectionError(f)                                 (!bGtwyIgnoreConnectionError(f))
#define bGtwyFailSearchError(f)                                     (!bGtwyIgnoreSearchError(f))
#define bGtwyFailRetrievalError(f)                                  (!bGtwyIgnoreRetrievalError(f))
#define bGtwyStrictConnectionPolicy(f)                              (!bGtwyLazyConnectionPolicy(f))

#define vGtwySetIgnoreConnectionError(f)                            ((f) |= GTWY_IGNORE_CONNECTION_ERROR_BIT_MASK)
#define vGtwySetIgnoreSearchError(f)                                ((f) |= GTWY_IGNORE_SEARCH_ERROR_BIT_MASK)
#define vGtwySetIgnoreRetrievalError(f)                             ((f) |= GTWY_IGNORE_RETRIEVAL_ERROR_BIT_MASK)
#define vGtwySetLazyConnectionPolicy(f)                             ((f) |= GTWY_LAZY_CONNECTION_POLICY_BIT_MASK)

#define vGtwySetFailConnectionError(f)                              ((f) &= ~GTWY_IGNORE_CONNECTION_ERROR_BIT_MASK)
#define vGtwySetFailSearchError(f)                                  ((f) &= ~GTWY_IGNORE_SEARCH_ERROR_BIT_MASK)
#define vGtwySetFailRetrievalError(f)                               ((f) &= ~GTWY_IGNORE_RETRIEVAL_ERROR_BIT_MASK)
#define vGtwySetStrictConnectionPolicy(f)                           ((f) &= ~GTWY_LAZY_CONNECTION_POLICY_BIT_MASK)


/* Clear the flags, setting connection polizy to strict, connection error to fail, 
** search error to fail and retrieval error to fail 
*/
#define vGtwyClearFlags(f)                                          (f &= ~GTWY_NO_BIT_MASK)


/*---------------------------------------------------------------------------*/


/* Mirror priority default */
#define GTWY_MIRROR_PRIORITY_DEFAULT                                (1)


/*---------------------------------------------------------------------------*/


/* Gateway information cache types */
#define GTWY_INFO_CACHE_TYPE_INVALID                                (0)
#define GTWY_INFO_CACHE_TYPE_EMPTY                                  (1)
#define GTWY_INFO_CACHE_TYPE_SERVER_INFO                            (2)
#define GTWY_INFO_CACHE_TYPE_SERVER_INDEX_INFO                      (3)
#define GTWY_INFO_CACHE_TYPE_INDEX_INFO                             (4)
#define GTWY_INFO_CACHE_TYPE_FIELD_INFO                             (5)
#define GTWY_INFO_CACHE_TYPE_TERM_INFO                              (6)
#define GTWY_INFO_CACHE_TYPE_DOCUMENT_INFO                          (7)

#define GTWY_INFO_CACHE_TYPE_VALID(n)                               (((n) >= GTWY_INFO_CACHE_TYPE_EMPTY) && \
                                                                            ((n) <= GTWY_INFO_CACHE_TYPE_DOCUMENT_INFO))


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Gateway mirror structure */
struct gtwyMirror {
    unsigned char               pucCanonicalIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1];   /* Canonical index name */
    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1];            /* Index name */
    unsigned char               pucHostName[MAXHOSTNAMELEN + 1];                            /* Host name */
    int                         iPort;                                                      /* Port */
    void                        *pvUtlNet;                                                  /* Net structure */
    void                        *pvLwps;                                                    /* LWPS structure */
    unsigned int                uiCurrentState;                                             /* Current LWPS protocol state */
    unsigned int                uiPriority;                                                 /* Priority */
};


/* Gateway segment structure */
struct gtwySegment {
    struct gtwyMirror           *pgmGtwyMirrors;                                            /* Gateway mirror array */
    unsigned int                uiGtwyMirrorsLength;                                        /* Gateway mirror array length */
};


/* Gateway Index Sort structure */
struct gtwyIndexSort {
    unsigned char               pucSortFieldName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1];        /* Sort field name */
    unsigned int                uiSortOrderID;                                              /* Sort order ID */
};    


/* Gateway Index structure */
struct gtwyIndex {
    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1];            /* Index name */
    unsigned int                uiIndexProtocol;                                            /* Index protocol (currently unused) */
    unsigned int                uiIndexOrigin;                                              /* Index origin (url/configuration) */
    unsigned int                uiFlags;                                                    /* Flags */
    unsigned int                uiCurrentState;                                             /* Current state */
    time_t                      tLastAccessTime;                                            /* time_t of the last access, open, search, retrieval */

    struct gtwySegment          *pgsGtwySegments;                                           /* Gateway segments array */
    unsigned int                uiGtwySegmentsLength;                                       /* Gateway segments array length */

    struct gtwyIndexSort        *psdsGtwyIndexSorts;                                        /* Gateway index sorts array */
    unsigned int                uiGtwyIndexSortsLength;                                     /* Gateway index sorts array length */

    unsigned int                uiConnectionTimeOut;                                        /* Connection timeout (milliseconds) (setable from search) */
    unsigned int                uiSearchTimeOut;                                            /* Search timeout (milliseconds) (setable from search) */
    unsigned int                uiRetrievalTimeOut;                                         /* Retrieval timeout (milliseconds) (setable from search) */
    unsigned int                uiInformationTimeOut;                                       /* Information timeout (milliseconds) (setable from search) */

    int                         iMirrorAffinity;                                            /* Mirror affinity (setable from search) */

    unsigned int                uiMaximumSegmentsSearched;                                  /* Maximum segments searched (setable from search) */
    unsigned int                uiMinimumSegmentsSearched;                                  /* Minimum segments searched (setable from search) */
};


/* Gateway cache structure */
struct gtwyInfoCache {
    unsigned char               pucCanonicalIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1];   /* Canonical index name */
    unsigned int                uiInfoCacheType;                                            /* Cache type */
    time_t                      tInfoCacheTimeOut;                                          /* Cache timeout, set to 0 for no timeout */
    void                        *pvInfoCacheEntry;                                          /* Cache entry */
    unsigned int                uiInfoCacheEntryLength;                                     /* Cache entry length */
};


/* Gateway structure which gets added to the client pointer of the SPI Server Handle */
struct gtwyGateway {
    unsigned char               pucConfigurationFilePath[UTL_FILE_PATH_MAX + 1];            /* Gateway configuration file path */
    time_t                      tConfigurationFileLastStatusChange;                         /* Gateway configuration file last status change time */

    void                        *pvUtlGtwyConfig;                                           /* Gateway configuration structure */

    unsigned int                uiConnectionTimeOut;                                        /* Connection timeout (milliseconds) */
    unsigned int                uiSearchTimeOut;                                            /* Search timeout (milliseconds) */
    unsigned int                uiRetrievalTimeOut;                                         /* Retrieval timeout (milliseconds) */
    unsigned int                uiInformationTimeOut;                                       /* Information timeout (milliseconds) */
    int                         iMirrorAffinity;                                            /* Mirror affinity */
    unsigned int                uiInformationCacheTimeOut;                                  /* Information cache timeout (seconds) */
    boolean                     bAllowSearchOverrides;                                      /* Allow search overrides (yes/no) */

    unsigned int                uiLwpsNetworkProtocolID;                                    /* Connection protocol (tcp/udp) */
    boolean                     bLwpsSendInit;                                              /* Send init (yes/no) */

    struct gtwyIndex            **ppgdGtwyIndexList;                                        /* Gateway index list (cache) */
    unsigned int                uiGtwyIndexListLength;                                      /* Gateway index list length */

    void                        *pvUtlGatewayIndexLocationsTrie;                            /* Gateway index location trie for auto-discovered index */

    struct gtwyInfoCache        *pgicGtwyInfoCache;                                         /* Gateway info cache */
    unsigned int                uiGtwyInfoCacheLength;                                      /* Gateway info cache length */
};


/* Gateway open gateway segment structure */
struct gtwyOpenGatewaySegment {
    struct gtwyGateway          *pggGtwyGateway;                                            /* Gateway structure */
    struct gtwyIndex            *pgdGtwyIndex;                                              /* Gateway index structure */
    struct gtwySegment          *pgsGtwySegment;                                            /* Segment structure */
};


/* Gateway search gateway index structure */
struct gtwySearchGatewayIndex {
    struct gtwyGateway          *pggGtwyGateway;                                            /* Gateway structure */
    struct gtwyIndex            *pgdGtwyIndex;                                              /* Gateway index structure */
    unsigned char               *pucLanguageCode;                                           /* Language code */
    unsigned char               *pucSearchText;                                             /* Search text */
    unsigned char               *pucPositiveFeedbackText;                                   /* Positive feedback text */
    unsigned char               *pucNegativeFeedbackText;                                   /* Negative feedback text */
    unsigned int                uiStartIndex;                                               /* Start index */ 
    unsigned int                uiEndIndex;                                                 /* End index */
    struct spiSearchResponse    *pssrSpiSearchResponse;                                     /* SPI search response (returned) */
    unsigned char               *pucSortFieldName;                                          /* Sort field name */
    unsigned int                uiSortOrderID;                                              /* Sort order ID */
};


/* Gateway search gateway segment structure */
struct gtwySearchGatewaySegment {
    struct gtwyGateway          *pggGtwyGateway;                                            /* Gateway structure */
    struct gtwyIndex            *pgdGtwyIndex;                                              /* Gateway index structure */
    struct gtwySegment          *pgsGtwySegment;                                            /* Segment structure */
    unsigned int                uiSearchTimeOut;                                            /* Search timeout (milliseconds)  */
    unsigned char               *pucLanguageCode;                                           /* Language code */
    unsigned char               *pucSearchText;                                             /* Search text */
    unsigned char               *pucPositiveFeedbackText;                                   /* Positive feedback text */
    unsigned char               *pucNegativeFeedbackText;                                   /* Negative feedback text */
    unsigned int                uiStartIndex;                                               /* Start index */
    unsigned int                uiEndIndex;                                                 /* End index */
    struct spiSearchResponse    *pssrSpiSearchResponse;                                     /* SPI search response (returned) */
};


/* Gateway Search Options structure */
struct gtwySearchOptions {
    unsigned char               pucSortFieldName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1];        /* Sort field name */
    unsigned int                uiSortOrderID;                                              /* Sort order */

    unsigned int                uiConnectionTimeOut;                                        /* Connection timeout (milliseconds) */
    unsigned int                uiSearchTimeOut;                                            /* Search timeout (milliseconds) */
    unsigned int                uiRetrievalTimeOut;                                         /* Retrieval timeout (milliseconds) */
    unsigned int                uiInformationTimeOut;                                       /* Information timeout (milliseconds) */

    int                         iMirrorAffinity;                                            /* Mirror affinity */

    unsigned int                uiMaximumSegmentsSearched;                                  /* Maximum segments searched */
    unsigned int                uiMinimumSegmentsSearched;                                  /* Minimum segments searched */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iGtwyCheckGatewayConfiguration (struct spiSession *pssSpiSession);


static int iGtwyAutoDiscoverIndices (struct gtwyGateway *pggGtwyGateway);

static int iGtwyAutoDiscoverIndicesPrintCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);

static int iGtwyAutoDiscoverIndicesRegexMatchCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);

static int iGtwyAutoDiscoverIndicesScanHosts (struct gtwyGateway *pggGtwyGateway,
        unsigned char *pucIndexHosts, void **ppvIndexLocationsTrie);



static int iGtwyInitializeGatewayIndex (struct gtwyGateway *pggGtwyGateway, 
        unsigned char *pucIndexName, struct gtwyIndex **ppgdGtwyIndex);

static int iGtwyDuplicateGatewayIndex (struct gtwyGateway *pggGtwyGateway, 
        struct gtwyIndex *pgdGtwyIndex, struct gtwyIndex **ppgdGtwyIndex);

static int iGtwyAddGatewayIndexToList (struct gtwyGateway *pggGtwyGateway, 
        struct gtwyIndex *pgdGtwyIndex);

static int iGtwyGetGatewayIndexFromList (struct gtwyGateway *pggGtwyGateway, 
        unsigned char *pucIndexName, struct gtwyIndex **ppgdGtwyIndex);

static int iGtwyResetSearchOverridesOnGatewayIndex (struct gtwyGateway *pggGtwyGateway, 
        struct gtwyIndex *pgdGtwyIndex);

static int iGtwyResetTemporaryErrorsOnGatewayIndex (struct gtwyGateway *pggGtwyGateway, 
            struct gtwyIndex *pgdGtwyIndex);




static int iGtwyOpenGatewayIndex (struct gtwyGateway *pggGtwyGateway, 
        struct gtwyIndex *pgdGtwyIndex);

static int iGtwyCloseGatewayIndex (struct gtwyGateway *pggGtwyGateway,
        struct gtwyIndex *pgdGtwyIndex);

static int iGtwySearchGatewayIndex(struct gtwySearchGatewayIndex *pgsgdGtwySearchGatewayIndex);

static int iGtwyRetrieveDocumentFromGatewayIndex (struct gtwyGateway *pggGtwyGateway, struct gtwyIndex *pgdGtwyIndex,
        unsigned char *pucDocumentKey, unsigned char *pucItemName,  unsigned char *pucMimeType, unsigned int uiChunkType, 
        unsigned int uiChunkStart, unsigned int uiChunkEnd, void **ppvData, unsigned int *puiDataLength);

static int iGtwyFreeGatewayIndex (struct gtwyGateway *pggGtwyGateway,
        struct gtwyIndex *pgdGtwyIndex);



static int iGtwyOpenGatewaySegment (struct gtwyOpenGatewaySegment *pgogsGtwyOpenGatewaySegment);

static int iGtwyCloseGatewaySegment (struct gtwyGateway *pggGtwyGateway, 
        struct gtwyIndex *pgdGtwyIndex, struct gtwySegment *pgsGtwySegment);

static int iGtwySearchGatewaySegment(struct gtwySearchGatewaySegment *pgsgsGtwySearchGatewaySegment);

static int iGtwyRetrieveDocumentFromGatewaySegment (struct gtwyGateway *pggGtwyGateway, struct gtwyIndex *pgdGtwyIndex, 
        struct gtwySegment *pgsGtwySegment, struct gtwyMirror *pgmGtwyMirror, unsigned char *pucDocumentKey, 
        unsigned char *pucItemName, unsigned char *pucMimeType, unsigned int uiChunkType, unsigned int uiChunkStart, 
        unsigned int uiChunkEnd, void **ppvData, unsigned int *puiDataLength);



static int iGtwyOpenGatewayMirror (struct gtwyGateway *pggGtwyGateway, struct gtwyIndex *pgdGtwyIndex, 
        struct gtwyMirror *pgmGtwyMirror);

static int iGtwyCloseGatewayMirror (struct gtwyGateway *pggGtwyGateway, struct gtwyIndex *pgdGtwyIndex, 
        struct gtwyMirror *pgmGtwyMirror);



static int iGtwyAddToInfoCache (struct gtwyGateway *pggGtwyGateway, unsigned char *pucCanonicalIndexName,
        unsigned int uiInfoCacheType, void *pvInfoCacheEntry, unsigned int uiInfoCacheEntryLength);

static int iGtwyGetFromInfoCache (struct gtwyGateway *pggGtwyGateway, unsigned char *pucCanonicalIndexName,
        unsigned int uiInfoCacheType, void **ppvInfoCacheEntry, unsigned int *puiInfoCacheEntryLength);

static int iGtwyFreeInfoCache (struct gtwyGateway *pggGtwyGateway);

static int iGtwyFreeInfoCacheEntry (struct gtwyGateway *pggGtwyGateway, struct gtwyInfoCache *pgicGtwyInfoCache);


static int iGtwyGetSearchOptionsFromSearchText (struct gtwyGateway *pggGtwyGateway, 
        unsigned char *pucSearchText, struct gtwySearchOptions **ppgsoGtwySearchOptions);


/*---------------------------------------------------------------------------*/


/* 
** ==============================
** ===  Server SPI Functions  ===
** ==============================
*/


/*

    Function:   iSpiInitializeServer()

    Purpose:    This is called when the server is initialized.

                NOTE - This function may be called more than once

    Parameters: pssSpiSession       spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiInitializeServer
(
    struct spiSession *pssSpiSession
)
{

    int                 iError = SPI_NoError;
    struct gtwyGateway  *pggGtwyGateway = NULL;
    unsigned char       pucSymbolData[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiInitializeServer"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiInitializeServer'."); 
        return (SPI_InvalidSession);
    }


    /* Check for the configuration directory path */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucConfigurationDirectoryPath) == true ) {
        return (SPI_InvalidConfigurationDirectory);
    }

    /* Check that the configuration directory path can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucConfigurationDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucConfigurationDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucConfigurationDirectoryPath) == true)) ) {
        return (SPI_InvalidConfigurationDirectory);
    }


    /* Check for the temporary directory path */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucTemporaryDirectoryPath) == true ) {
        return (SPI_InvalidTemporaryDirectory);
    }

    /* Check that that temporary directory path can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucTemporaryDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucTemporaryDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucTemporaryDirectoryPath) == true)) ) {
        return (SPI_InvalidTemporaryDirectory);
    }


    /* Initialize the random seed */
    iUtlRandSetSeed(s_time(NULL));


    /* Shutdown the gateway if the gateway structure is already allocated, this is
    ** to handle situations where iSpiInitializeServer() is called more than once
    ** (note that the gateway structure is stored in the client pointer of the spi 
    ** session structure)
    */
    if ( pssSpiSession->pvClientPtr != NULL ) {
        iSpiShutdownServer(pssSpiSession);
        pssSpiSession = NULL;
    }

        
    /* Create a gateway structure */
    if ( (pggGtwyGateway = (struct gtwyGateway *)s_malloc((size_t)(sizeof(struct gtwyGateway)))) == NULL ) {
        return (SPI_MemError);
    }

    /* And place it in the client pointer of the spi session structure */
    pssSpiSession->pvClientPtr = (void *)pggGtwyGateway;


    /* Create the gateway configuration file path */
    if ( (iError = iUtlFileMergePaths(pssSpiSession->pucConfigurationDirectoryPath, GTWY_CONFIG_FILE_NAME, 
            pggGtwyGateway->pucConfigurationFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the gateway configuration file path, gateway configuration file name: '%s', gateway configuration directory path: '%s', utl error: %d.", 
                GTWY_CONFIG_FILE_NAME, pssSpiSession->pucConfigurationDirectoryPath, iError);
        iError = SPI_InitializeServerFailed;
        goto bailFromiGtwyInitializeServer;
    }

    /* Preset the last status change time of the gateway configuration file */
    pggGtwyGateway->tConfigurationFileLastStatusChange = -1;

    /* Open the gateway configuration - gateway configuration file may or may not be there */
    if ( (iError = iUtlConfigOpen(pggGtwyGateway->pucConfigurationFilePath, UTL_CONFIG_FILE_FLAG_OPTIONAL, (void **)&pggGtwyGateway->pvUtlGtwyConfig)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the gateway configuration, gateway configuration file: '%s', utl error: %d.", 
                pggGtwyGateway->pucConfigurationFilePath, iError); 
        iError = SPI_InitializeServerFailed;
        goto bailFromiGtwyInitializeServer;
    }


    /* Check the version if the gateway configuration file exists, and get its last status change time */
    if ( bUtlFilePathExists(pggGtwyGateway->pucConfigurationFilePath) == false ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to find the gateway configuration file: '%s'.", pggGtwyGateway->pucConfigurationFilePath); 
    }
    else if ( bUtlFileIsFile(pggGtwyGateway->pucConfigurationFilePath) == false ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to find the gateway configuration file: '%s'.", pggGtwyGateway->pucConfigurationFilePath); 
    }
    else if ( bUtlFilePathRead(pggGtwyGateway->pucConfigurationFilePath) == false ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to access the gateway configuration file: '%s'.", pggGtwyGateway->pucConfigurationFilePath); 
    }
    else {
        
        /* Get the version */
        if ( (iError = iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_VERSION, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the version number from the gateway configuration, symbol name: '%s', utl error: %d.", GTWY_CONFIG_VERSION, iError); 
            iError = SPI_InitializeServerFailed;
            goto bailFromiGtwyInitializeServer;
        }
        else {
            unsigned int    uiMajorVersion = 0;
            unsigned int    uiMinorVersion = 0;
            unsigned int    uiPatchVersion = 0;
            
            /* Scan for the version numbers, no need for a scan string here are these are just numbers */
            if ( sscanf(pucSymbolData, "%u.%u.%u", &uiMajorVersion, &uiMinorVersion, &uiPatchVersion) != 3 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid version number in the gateway configuration, expected: %u.%u.%u, symbol name: '%s', symbol: '%s'.", 
                        UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH, GTWY_CONFIG_VERSION, pucSymbolData); 
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyInitializeServer;
            }
    
            /* Check the version numbers we got against the internal version, note that we don't check the patch version */
            if ( !((uiMajorVersion == UTL_VERSION_MAJOR) && (uiMinorVersion <= UTL_VERSION_MINOR)) ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid version number in the search configuration, expected: %u.%u.%u or less, symbol name: '%s', symbol: '%s'.", 
                        UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH, GTWY_CONFIG_VERSION, pucSymbolData); 
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyInitializeServer;
            }
        }
        
        /* Get the last status change time of the gateway configuration file */
        if ( (iError = iUtlFileGetPathStatusChangeTimeT(pggGtwyGateway->pucConfigurationFilePath, &pggGtwyGateway->tConfigurationFileLastStatusChange)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the last status change time of the gateway configuration file: '%s', utl error: %d.", 
                    pggGtwyGateway->pucConfigurationFilePath, iError); 
            iError = SPI_InitializeServerFailed;
            goto bailFromiGtwyInitializeServer;
        }
    }



    /* Set the default gateway connection timeout */
    pggGtwyGateway->uiConnectionTimeOut = GTWY_GATEWAY_CONNECTION_TIMEOUT_DEFAULT;

    /* Get the gateway connection timeout */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_CONNECTION_TIMEOUT, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) <= 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway connection timeout found in the gateway configuration file, symbol name: '%d', symbol: '%s', using default: %d.", 
                    GTWY_CONFIG_GATEWAY_CONNECTION_TIMEOUT, pucSymbolData, GTWY_GATEWAY_CONNECTION_TIMEOUT_DEFAULT);
        }
        else {
            pggGtwyGateway->uiConnectionTimeOut = s_strtol(pucSymbolData, NULL, 10);
        }
    }


    /* Set the default gateway search timeout */
    pggGtwyGateway->uiSearchTimeOut = GTWY_GATEWAY_SEARCH_TIMEOUT_DEFAULT;

    /* Get the gateway search timeout */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_SEARCH_TIMEOUT, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) <= 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway search timeout found in the gateway configuration file, symbol name: '%d', symbol: '%s', using default: %d.", 
                    GTWY_CONFIG_GATEWAY_SEARCH_TIMEOUT, pucSymbolData, GTWY_GATEWAY_SEARCH_TIMEOUT_DEFAULT);
        }
        else {
            pggGtwyGateway->uiSearchTimeOut = s_strtol(pucSymbolData, NULL, 10);
        }
    }


    /* Set the default gateway retrieval timeout */
    pggGtwyGateway->uiRetrievalTimeOut = GTWY_GATEWAY_RETRIEVAL_TIMEOUT_DEFAULT;

    /* Get the gateway retrieval timeout */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_RETRIEVAL_TIMEOUT, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) <= 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway retrieval timeout found in the gateway configuration file, symbol name: '%d', symbol: '%s', using default: %d.", 
                    GTWY_CONFIG_GATEWAY_RETRIEVAL_TIMEOUT, pucSymbolData, GTWY_GATEWAY_RETRIEVAL_TIMEOUT_DEFAULT);
        }
        else {
            pggGtwyGateway->uiRetrievalTimeOut = s_strtol(pucSymbolData, NULL, 10);
        }
    }


    /* Set the default gateway information timeout */
    pggGtwyGateway->uiInformationTimeOut = GTWY_GATEWAY_INFORMATION_TIMEOUT_DEFAULT;

    /* Get the gateway information timeout */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_INFORMATION_TIMEOUT, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) <= 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway information timeout found in the gateway configuration file, symbol name: '%d', symbol: '%s', using default: %d.", 
                    GTWY_CONFIG_GATEWAY_INFORMATION_TIMEOUT, pucSymbolData, GTWY_GATEWAY_INFORMATION_TIMEOUT_DEFAULT);
        }
        else {
            pggGtwyGateway->uiInformationTimeOut = s_strtol(pucSymbolData, NULL, 10);
        }
    }


    /* Set the default gateway mirror affinity */
    pggGtwyGateway->iMirrorAffinity = GTWY_GATEWAY_MIRROR_AFFINITY_DEFAULT;

    /* Get the gateway mirror affinity */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_MIRROR_AFFINITY, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) < -1 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway mirror affinity found in the gateway configuration file, symbol name: '%d', symbol: '%s', using default: %d.", 
                    GTWY_CONFIG_GATEWAY_MIRROR_AFFINITY, pucSymbolData, GTWY_GATEWAY_MIRROR_AFFINITY_DEFAULT);
        }
        else {
            pggGtwyGateway->iMirrorAffinity = s_strtol(pucSymbolData, NULL, 10);
        }
    }


    /* Set the default gateway information cache timeout */
    pggGtwyGateway->uiInformationCacheTimeOut = GTWY_GATEWAY_INFORMATION_CACHE_TIMEOUT_DEFAULT;

    /* Get the gateway information cache timeout */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_INFORMATION_CACHE_TIMEOUT, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) < 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway information cache timeout found in the gateway configuration file, symbol name: '%d', symbol: '%s', using default: %d.", 
                    GTWY_CONFIG_GATEWAY_INFORMATION_CACHE_TIMEOUT, pucSymbolData, GTWY_GATEWAY_INFORMATION_CACHE_TIMEOUT_DEFAULT);
        }
        else {
            pggGtwyGateway->uiInformationCacheTimeOut = s_strtol(pucSymbolData, NULL, 10);
        }
    }


    /* Set the default gateway allow search override */
    pggGtwyGateway->bAllowSearchOverrides = false;

    /* Get the gateway allow search override */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_ALLOW_SEARCH_OVERRIDES, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strcasecmp(pucSymbolData, GTWY_YES) == 0 ) {
            pggGtwyGateway->bAllowSearchOverrides = true;
        }
        else if ( s_strcasecmp(pucSymbolData, GTWY_NO) == 0 ) {
            pggGtwyGateway->bAllowSearchOverrides = false;
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway allow search override found in the gateway configuration file, symbol name: '%d', symbol: '%s', using default: '%s'.", 
                    GTWY_CONFIG_GATEWAY_ALLOW_SEARCH_OVERRIDES, pucSymbolData, GTWY_NO);
        }
    }


    /* Set the default gateway lwps network protocol ID */
    pggGtwyGateway->uiLwpsNetworkProtocolID = GTWY_PROTOCOL_TCP_ID;

    /* Get the gateway lwps network protocol */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_NETWORK_PROTOCOL, LWPS_PROTOCOL_NAME, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strcasecmp(pucSymbolData, GTWY_PROTOCOL_UDP_NAME) == 0 ) {
            pggGtwyGateway->uiLwpsNetworkProtocolID = GTWY_PROTOCOL_UDP_ID;
        }
        else if ( s_strcasecmp(pucSymbolData, GTWY_PROTOCOL_TCP_NAME) == 0 ) {
            pggGtwyGateway->uiLwpsNetworkProtocolID = GTWY_PROTOCOL_TCP_ID;
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway lwps network protocol found in the gateway configuration file, config name: '%s', config appender: '%s', config value: '%s', using default: '%s'.", 
                    GTWY_CONFIG_GATEWAY_NETWORK_PROTOCOL, LWPS_PROTOCOL_NAME, pucSymbolData, GTWY_PROTOCOL_TCP_NAME);
        }
    }


    /* Set the default gateway lwps send init */
    pggGtwyGateway->bLwpsSendInit = false;

    /* Get the gateway lwps send init */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_SEND_INIT, LWPS_PROTOCOL_NAME, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strcasecmp(pucSymbolData, GTWY_YES) == 0 ) {
            pggGtwyGateway->bLwpsSendInit = true;
        }
        else if ( s_strcasecmp(pucSymbolData, GTWY_NO) == 0 ) {
            pggGtwyGateway->bLwpsSendInit = false;
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid gateway lwps send init found in the gateway configuration file, config name: '%s', config appender: '%s', config value: '%s', using default: '%s'.", 
                    GTWY_CONFIG_GATEWAY_SEND_INIT, LWPS_PROTOCOL_NAME, pucSymbolData, GTWY_NO);
        }
    }
    


    /* Auto-discover index */
    if ( (iError = iGtwyAutoDiscoverIndices(pggGtwyGateway)) != SPI_NoError ) {
        goto bailFromiGtwyInitializeServer;
    }


    /* Initialize the gateway index list */
    pggGtwyGateway->ppgdGtwyIndexList = NULL;
    pggGtwyGateway->uiGtwyIndexListLength = 0;



    /* Bail label */
    bailFromiGtwyInitializeServer:


    /* Check for errors */
    if ( iError != SPI_NoError ) {
        /* Shutdown the gateway if there was an error initializing it, ignoring errors */
        iSpiShutdownServer(pssSpiSession);
        pssSpiSession = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiShutdownServer()

    Purpose:    This is called when the server is being shut down.

                NOTE - This function may be called more than once

    Parameters: pssSpiSession       spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiShutdownServer
(
    struct spiSession *pssSpiSession
)
{

    struct gtwyGateway      *pggGtwyGateway = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiShutdownServer"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiShutdownServer'."); 
        return (SPI_InvalidSession);
    }


    /* Shut down the gateway if the gateway structure is allocated, this is
    ** to handle situations where iSpiShutdownServer() is called more than once
    ** (note that the gateway structure is stored in the client pointer of the spi 
    ** session structure)
    */
    if ( pssSpiSession->pvClientPtr != NULL ) {

        /* Dereference the gateway structure from the client pointer */
        pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr;

        /* Free the gateway index list */
        if ( (pggGtwyGateway->ppgdGtwyIndexList != NULL) && (pggGtwyGateway->uiGtwyIndexListLength > 0) ) {
        
            struct gtwyIndex        **ppgdGtwyIndexListPtr = NULL;
            unsigned int            uiI = 0;
            
            /* Loop over each entry in the gateway index list, freeing the gateway index */
            for ( uiI = 0, ppgdGtwyIndexListPtr = pggGtwyGateway->ppgdGtwyIndexList; uiI < pggGtwyGateway->uiGtwyIndexListLength; uiI++, ppgdGtwyIndexListPtr++ ) {
                iGtwyFreeGatewayIndex(pggGtwyGateway, *ppgdGtwyIndexListPtr);
            }
            
            /* Finally free the gateway index list */
            s_free(pggGtwyGateway->ppgdGtwyIndexList);
        }

        /* Close the gateway configuration */
        iUtlConfigClose(pggGtwyGateway->pvUtlGtwyConfig);
        pggGtwyGateway->pvUtlGtwyConfig = NULL;

        /* Free the gateway index locations trie */
        iUtlTrieFree(pggGtwyGateway->pvUtlGatewayIndexLocationsTrie, true);
        pggGtwyGateway->pvUtlGatewayIndexLocationsTrie = NULL;

        /* Free the cache */
        iGtwyFreeInfoCache(pggGtwyGateway);

        /* Free the gateway structure */
        s_free(pggGtwyGateway);

        /* Null out the client pointer of the spi session structure */
        pssSpiSession->pvClientPtr = NULL;
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =============================
** ===  Index SPI Functions  ===
** =============================
*/


/*

    Function:   iSpiOpenIndex()

    Purpose:    This function will be called to open a index before any
                operations are performed on it.

                This function must allocate the index structure returned. 
                Note that the index structure is opaque to the SPI framework 
                so you really can  get any size of pointer (even NULL, though 
                this may not be a good idea).

    Parameters: pssSpiSession   spi session structure
                pucIndexName    index name
                ppvIndex        return pointer for the index structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiOpenIndex
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    void **ppvIndex
)
{

    int                 iError = SPI_NoError;
    struct gtwyGateway  *pggGtwyGateway = NULL;
    struct gtwyIndex    *pgdGtwyIndex = NULL;
    struct gtwyIndex    *pgdGtwyIndexCopy = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiOpenIndex: '%s'", pucIndexName); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiOpenIndex'."); 
        return (SPI_InvalidSession);
    }

    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexName' parameter passed to 'iSpiOpenIndex'."); 
        return (SPI_InvalidIndexName);
    }

    if ( ppvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvIndex' parameter passed to 'iSpiOpenIndex'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Get the gateway index from the gateway index list */
    if ( (iError = iGtwyGetGatewayIndexFromList(pggGtwyGateway, pucIndexName, &pgdGtwyIndex)) != SPI_NoError ) {

        /* We failed to get the gateway index from the gateway index list, 
        ** so we need to initialize it and add it to the gateway index list 
        */
        
        /* Initialize the gateway index from the index name */
        if ( (iError = iGtwyInitializeGatewayIndex(pggGtwyGateway, pucIndexName, &pgdGtwyIndex)) != SPI_NoError ) {
            return (iError);
        }
        
        /* Add the gateway index to the gateway index list */
        if ( (iError = iGtwyAddGatewayIndexToList(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
            iGtwyFreeGatewayIndex(pggGtwyGateway, pgdGtwyIndex);
            pgdGtwyIndex = NULL;
            return (iError);
        }
    }


    /* We now need to duplicate the gateway index before we can use it */ 
    if ( (iError = iGtwyDuplicateGatewayIndex(pggGtwyGateway, pgdGtwyIndex, &pgdGtwyIndexCopy)) != SPI_NoError ) {
        return (iError);
    }
    
    /* Hand over the pointer */
    pgdGtwyIndex = pgdGtwyIndexCopy;


    /* Open the gateway index */
    if ( (iError = iGtwyOpenGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
    
        /* Failed to open the gateway index, so we close it (to clean up) */
        iSpiCloseIndex(pssSpiSession, (void *)pgdGtwyIndex);
        pgdGtwyIndex = NULL;

        return (iError);
    }

    /* Set the return pointer */
    *ppvIndex = (void *)pgdGtwyIndex;


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiCloseIndex()

    Purpose:    This function will be called after all operations performed
                on this index are done.

                This function must free the index structure passed, it will
                not be referenced anymore. Note that the index structure 
                is opaque to the SPI framework so you really can 
                get any size of pointer (even NULL, though this may not be a 
                good idea). The pointer gets allocated in iSpiOpenIndex.

    Parameters: pssSpiSession   spi session structure
                pvIndex         index structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiCloseIndex
(
    struct spiSession *pssSpiSession,
    void *pvIndex
)
{

    int                 iError = SPI_NoError;
    struct gtwyGateway  *pggGtwyGateway = NULL;
    struct gtwyIndex    *pgdGtwyIndex = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiCloseIndex"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiCloseIndex'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiCloseIndex'."); 
        return (SPI_InvalidIndex);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index pointer into a gateway index structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Close the gateway index */
    iGtwyCloseGatewayIndex(pggGtwyGateway, pgdGtwyIndex);
    pgdGtwyIndex = NULL;


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =================================
** ===  Searching SPI Functions  ===
** =================================
*/


/*

    Function:   iSpiSearchIndex()

    Purpose:    This function searches the index. 

    Parameters: pssSpiSession               spi session structure
                ppvIndexList                pointer to a NULL terminated list of index structures
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                ppssrSpiSearchResponse      return pointer for the spi search response structure


    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiSearchIndex
(
    struct spiSession *pssSpiSession,
    void **ppvIndexList,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex,
    unsigned int uiEndIndex,
    struct spiSearchResponse **ppssrSpiSearchResponse
)
{

    int                             iError = SPI_NoError;
    unsigned int                    uiIndexListCount = 0;
    struct gtwyGateway              *pggGtwyGateway = NULL;
    struct gtwyIndex                *pgdGtwyIndex = NULL;
    boolean                         bMultiGatewayIndexSearch = false;
    boolean                         bMultiIndexSearch = false;
    struct gtwySearchOptions        *pgsoGtwySearchOptions = NULL;
    unsigned int                    uiI = 0, uiJ = 0;
    struct spiSearchResult          *pssrSpiSearchResults = NULL;
    struct spiSearchResult          *pssrSpiSearchResultsPtr = NULL;
    unsigned int                    uiSpiSearchResultsLength = 0;
    unsigned int                    uiTotalResults = 0;
    unsigned int                    uiSortType = SPI_SORT_TYPE_UNKNOWN;
    double                          dMaxSortKey = 0;

    struct gtwySearchGatewayIndex   *pgsgdGtwySearchGatewayIndices = NULL;
    struct gtwySearchGatewayIndex   *pgsgdGtwySearchGatewayIndicesPtr = NULL;

    pthread_t                       *ptThreads = NULL;
    int                             iStatus = 0;
    void                            *pvStatus = NULL;

    /* To calculate the search time */
    struct timeval                  tvSearchStartTimeVal;
    double                          dSearchTime = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiSearchIndex"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSession);
    }

    if ( ppvIndexList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvIndexList' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppvIndexList[0] == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Empty 'ppvIndexList' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidIndex);
    }

    if ( uiStartIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex'  parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSearchResultsRange);
    }

    if ( uiEndIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndIndex'  parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSearchResultsRange);
    }

    if ( uiStartIndex > uiEndIndex ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex' and 'uiEndIndex' parameters passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSearchResultsRange);
    }

    if ( ppssrSpiSearchResponse == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSpiSearchResponse' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_ReturnParameterError);
    }

    
    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Count up the number of indices */
    for ( uiIndexListCount = 0; ppvIndexList[uiIndexListCount] != NULL; uiIndexListCount++ ) {
        ;
    }

    /* Set the multi-gateway index search flag, we set it if there is more than one filled entry in the index pointer list list */
    bMultiGatewayIndexSearch = (uiIndexListCount > 1) ? true : false;

    /* Set the multi-index search flag, we set it if there is more than one filled entry in the index pointer list list */
    bMultiIndexSearch = ((uiIndexListCount > 1) || (((struct gtwyIndex *)ppvIndexList[0])->uiGtwySegmentsLength > 1)) ? true : false;


    /* Allocate a search gateway index array, this is used to pass the search parameters
    ** and collects the return parameter for each index, the search of which is threaded
    */
    if ( (pgsgdGtwySearchGatewayIndices = (struct gtwySearchGatewayIndex *)s_malloc((size_t)(sizeof(struct gtwySearchGatewayIndex) * uiIndexListCount))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwySearchIndex;
    }

    /* Allocate the threads array to keep track of the threads */
    if ( bMultiGatewayIndexSearch == true ) {
        if ( (ptThreads = (pthread_t *)s_malloc((size_t)(sizeof(pthread_t) * uiIndexListCount))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwySearchIndex;
        }
    }



    /* Trim the search text, the positive feedback text and the negative feedback text */
    iUtlStringsTrimString(pucSearchText);
    iUtlStringsTrimString(pucPositiveFeedbackText);
    iUtlStringsTrimString(pucNegativeFeedbackText);


    /* Clean the search text, the positive feedback text and the negative feedback text, replacing new lines with a space */
    iUtlStringsReplaceCharactersInString(pucSearchText, "\n\r", ' ');
    iUtlStringsReplaceCharactersInString(pucPositiveFeedbackText, "\n\r", ' ');
    iUtlStringsReplaceCharactersInString(pucNegativeFeedbackText, "\n\r", ' ');


    /* Extract the search options from the search that we need to */
    if ( (iError = iGtwyGetSearchOptionsFromSearchText(pggGtwyGateway, pucSearchText, &pgsoGtwySearchOptions)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to extract the search options from the search text, spi error: %d.", iError);
        goto bailFromiGtwySearchIndex;
    }


    /* Set the search start time */
    s_gettimeofday(&tvSearchStartTimeVal, NULL);


    /* Loop through all the gateway index and kick off the search thread */
    for ( uiI = 0, pgsgdGtwySearchGatewayIndicesPtr = pgsgdGtwySearchGatewayIndices; ppvIndexList[uiI] != NULL; uiI++, pgsgdGtwySearchGatewayIndicesPtr++ ) {

        /* Dereference the gateway index for convenience */
        pgdGtwyIndex = (struct gtwyIndex *)ppvIndexList[uiI];

        /* Set the last access time */
        pgdGtwyIndex->tLastAccessTime = s_time(NULL);

        /* Reset the temporary errors */
        if ( (iError = iGtwyResetTemporaryErrorsOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
            goto bailFromiGtwySearchIndex;
        }

        /* Reset the overrides */
        if ( (iError = iGtwyResetSearchOverridesOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
            goto bailFromiGtwySearchIndex;
        }

        /* Set the search overrides if we are allowed to do so */
        if ( pggGtwyGateway->bAllowSearchOverrides == true ) {
            
            /* Set the connection timeout */
            if ( pgsoGtwySearchOptions->uiConnectionTimeOut > 0 ) {
                pgdGtwyIndex->uiConnectionTimeOut = pgsoGtwySearchOptions->uiConnectionTimeOut;
            }
    
            /* Set the search timeout */
            if ( pgsoGtwySearchOptions->uiSearchTimeOut > 0 ) {
                pgdGtwyIndex->uiSearchTimeOut = pgsoGtwySearchOptions->uiSearchTimeOut;
            }
    
            /* Set the retrieval timeout */
            if ( pgsoGtwySearchOptions->uiRetrievalTimeOut > 0 ) {
                pgdGtwyIndex->uiRetrievalTimeOut = pgsoGtwySearchOptions->uiRetrievalTimeOut;
            }
    
            /* Set the information timeout */
            if ( pgsoGtwySearchOptions->uiInformationTimeOut > 0 ) {
                pgdGtwyIndex->uiInformationTimeOut = pgsoGtwySearchOptions->uiInformationTimeOut;
            }

            /* Set the mirror affinity */
            if ( pgsoGtwySearchOptions->iMirrorAffinity >= -1 ) {
                pgdGtwyIndex->iMirrorAffinity = pgsoGtwySearchOptions->iMirrorAffinity;
            }

            /* Set the maximum segments searched */
            if ( pgsoGtwySearchOptions->uiMaximumSegmentsSearched > 0 ) {
                pgdGtwyIndex->uiMaximumSegmentsSearched = pgsoGtwySearchOptions->uiMaximumSegmentsSearched;
            }

            /* Set the minimum segments searched */
            if ( pgsoGtwySearchOptions->uiMinimumSegmentsSearched > 0 ) {
                pgdGtwyIndex->uiMinimumSegmentsSearched = pgsoGtwySearchOptions->uiMinimumSegmentsSearched;
            }

            /* Set the minimum segments searched to the maximum if it was larger than the maximum */
            if ( (pgdGtwyIndex->uiMaximumSegmentsSearched > 0) && (pgdGtwyIndex->uiMinimumSegmentsSearched > 0) && 
                    (pgdGtwyIndex->uiMaximumSegmentsSearched < pgdGtwyIndex->uiMinimumSegmentsSearched) ) {
                pgdGtwyIndex->uiMinimumSegmentsSearched = pgdGtwyIndex->uiMaximumSegmentsSearched;
            }
        }


        /* Populate the gate search gateway index structure */
        pgsgdGtwySearchGatewayIndicesPtr->pggGtwyGateway = pggGtwyGateway;
        pgsgdGtwySearchGatewayIndicesPtr->pgdGtwyIndex = pgdGtwyIndex;
        pgsgdGtwySearchGatewayIndicesPtr->pucLanguageCode = pucLanguageCode;
        pgsgdGtwySearchGatewayIndicesPtr->pucSearchText = pucSearchText;
        pgsgdGtwySearchGatewayIndicesPtr->pucPositiveFeedbackText = pucPositiveFeedbackText;
        pgsgdGtwySearchGatewayIndicesPtr->pucNegativeFeedbackText = pucNegativeFeedbackText;
        pgsgdGtwySearchGatewayIndicesPtr->uiStartIndex = ((bMultiIndexSearch == true) ? 0 : uiStartIndex);
        pgsgdGtwySearchGatewayIndicesPtr->uiEndIndex = uiEndIndex;
        pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse = NULL;

        pgsgdGtwySearchGatewayIndicesPtr->pucSortFieldName = pgsoGtwySearchOptions->pucSortFieldName;
        pgsgdGtwySearchGatewayIndicesPtr->uiSortOrderID = pgsoGtwySearchOptions->uiSortOrderID;


        /* Thread if there is more than one gateway index to search */
        if ( bMultiGatewayIndexSearch == true ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiSearchIndex - Thread: %u, Index: '%s'", uiI, pgdGtwyIndex->pucIndexName); */

            /* Kick off the search thread */
            if ( (iStatus = s_pthread_create(&ptThreads[uiI], NULL, (void *)iGtwySearchGatewayIndex, (void *)pgsgdGtwySearchGatewayIndicesPtr)) != 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a thread.");
                iError = SPI_SearchIndexFailed;
                goto bailFromiGtwySearchIndex;
            }

        }
        else {
        
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiSearchIndex - No Thread: %u, Index: '%s'", uiI, pgdGtwyIndex->pucIndexName); */

            /* Search the gateway index directly */
            if ( (iError = iGtwySearchGatewayIndex(pgsgdGtwySearchGatewayIndicesPtr)) != SPI_NoError ) {
                goto bailFromiGtwySearchIndex;
            }
        }
    }



    /* Loop through all the index and collect the search thread */
    for ( uiI = 0, pgsgdGtwySearchGatewayIndicesPtr = pgsgdGtwySearchGatewayIndices; ppvIndexList[uiI] != NULL; uiI++, pgsgdGtwySearchGatewayIndicesPtr++ ) {

        /* Dereference the gateway index for convenience */
        pgdGtwyIndex = (struct gtwyIndex *)ppvIndexList[uiI];

        /* Thread if there is more than one gateway index to search */
        if ( bMultiGatewayIndexSearch == true ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiSearchIndex - Joining thread: %u", uiI); */

            /* Join the thread */
            iStatus = s_pthread_join(ptThreads[uiI], &pvStatus);
            
            /* Erase the thread structure, we are done with it */
            s_memset(&ptThreads[uiI], '\0', sizeof(pthread_t));
    
            /* Handle the thread status */
            if ( iStatus != 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to join to a thread.");
                iError = SPI_MiscError;
                goto bailFromiGtwySearchIndex;
            }
    
            /* Check the status pointer, this contains the error code reported by the function we threaded */
            if ( (iError = (int)pvStatus) != SPI_NoError ) {
                /* An error occured so we hit the ol' silk */
                goto bailFromiGtwySearchIndex;
            }
        }


        /* The search is complete, we need to override the index name so we know which gateway index the documents came from */
        for ( uiJ = 0, pssrSpiSearchResultsPtr = pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->pssrSpiSearchResults; 
                uiJ < pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength; 
                uiJ++, pssrSpiSearchResultsPtr++ ) {
            
            s_free(pssrSpiSearchResultsPtr->pucIndexName);

            if ( (pssrSpiSearchResultsPtr->pucIndexName = (unsigned char *)s_strdup(pgdGtwyIndex->pucIndexName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwySearchIndex;
            }
        }
    }
    


    /* Loop through all the index checking the sort type, the goal 
    ** here is to check if all the search results have the same sort
    ** type, if not, we need to make sure we free any text sort keys
    */
    for ( uiI = 0, pgsgdGtwySearchGatewayIndicesPtr = pgsgdGtwySearchGatewayIndices; ppvIndexList[uiI] != NULL; uiI++, pgsgdGtwySearchGatewayIndicesPtr++ ) {

        /* Set the total results */
        uiTotalResults += pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiTotalResults;

        /* Set the sort type, set if this is the first iteration */
        if ( uiI == 0 ) {
            uiSortType = pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSortType;
        }
        else {
            /* All subsequent iterations, set the sort type to unknown if there is a mismatch */
            if ( uiSortType != pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSortType ) {
                uiSortType = SPI_SORT_TYPE_UNKNOWN;
            }
        }

        /* Set the maximum sort key */
        dMaxSortKey = UTL_MACROS_MAX(pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->dMaxSortKey, dMaxSortKey);
        
        /* Set the maximum search time */
        dSearchTime = UTL_MACROS_MAX(pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->dSearchTime, dSearchTime);
    }

    /* We need to free any char sort keys if the sort type is unknown */
    if ( uiSortType == SPI_SORT_TYPE_UNKNOWN ) {
        
        /* Loop over each index */
        for ( uiI = 0, pgsgdGtwySearchGatewayIndicesPtr = pgsgdGtwySearchGatewayIndices; ppvIndexList[uiI] != NULL; uiI++, pgsgdGtwySearchGatewayIndicesPtr++ ) {

            /* Skip if there are no spi search results */
            if ( pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength == 0 ) {
                continue;
            }

            /* Free the char sort keys in the spi search result if the sort key for this index is a char */
            if ( (pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSortType == SPI_SORT_TYPE_UCHAR_ASC) || 
                    (pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSortType == SPI_SORT_TYPE_UCHAR_DESC) ) {
                
                for ( pssrSpiSearchResultsPtr = pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->pssrSpiSearchResults; 
                        pssrSpiSearchResultsPtr < (pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->pssrSpiSearchResults + 
                        pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength); 
                        pssrSpiSearchResultsPtr++ ) {
                    
                    s_free(pssrSpiSearchResultsPtr->pucSortKey);
                }
            }
        }
    }



    /* Loop through all the index and accumulate the search results 
    ** retrieved from each index into the main search results 
    */
    for ( uiI = 0, uiSpiSearchResultsLength = 0, pgsgdGtwySearchGatewayIndicesPtr = pgsgdGtwySearchGatewayIndices; ppvIndexList[uiI] != NULL; uiI++, pgsgdGtwySearchGatewayIndicesPtr++ ) {

        /* Skip if there are no spi search results */
        if ( pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength == 0 ) {
            continue;
        }

        /* Check to see if we already have any search results we have already processed,
        ** if we do we need to add the new search results to the end of the current
        ** search results, otherwise we just pass over the pointers and save ourselves
        ** an allocation and a memory copy
        */
        if ( (pssrSpiSearchResults != NULL) && (uiSpiSearchResultsLength > 0) ) {
        
            if ( (pssrSpiSearchResultsPtr = (struct spiSearchResult *)s_realloc(pssrSpiSearchResults, (size_t)(sizeof(struct spiSearchResult) * 
                    (uiSpiSearchResultsLength + pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength)))) == NULL ) {

                /* An error occured so we hit the ol' silk */
                iError = SPI_MemError;

                goto bailFromiGtwySearchIndex;
            }

            /* Hand over the pointer */
            pssrSpiSearchResults = pssrSpiSearchResultsPtr;

            /* Increment the total number of search results retrieved so far */
            uiSpiSearchResultsLength += pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength;

            /* Move over the search results array we just got to the stored one */
            s_memcpy(pssrSpiSearchResults + (uiSpiSearchResultsLength - pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength), 
                    pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->pssrSpiSearchResults, 
                    pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength * sizeof(struct spiSearchResult));

            /* Free the search results, we dont need them any more, since we have stored them, note that we 
            ** dont free the document items since we only copied their pointers and not their data
            */
            s_free(pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->pssrSpiSearchResults);

        }
        else {
            /* Hand over the search results */
            pssrSpiSearchResults = pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->pssrSpiSearchResults;
            uiSpiSearchResultsLength = pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength;
        }

        /* Clear the values as we dont want to double-free if we get in trouble */
        pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->pssrSpiSearchResults = NULL;
        pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength = 0;
    }



    /* Resort the search results if needed */
    if ( (bMultiIndexSearch == true) && (uiSpiSearchResultsLength > 0) ) {
        
        if ( (iError = iSpiSortSearchResults(pssrSpiSearchResults, uiSpiSearchResultsLength, uiSortType)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to sort the search results, spi error: %d.", iError);
            goto bailFromiGtwySearchIndex;
        }
    }


    /* Splice the search results if needed */
    if ( (bMultiIndexSearch == true) && ((uiStartIndex > 0) || ((uiEndIndex > 0) && (uiSpiSearchResultsLength > (uiEndIndex + 1)))) ) {

        if ( (iError = iSpiSpliceSearchResults(&pssrSpiSearchResults, &uiSpiSearchResultsLength, uiStartIndex, uiEndIndex, uiSortType)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to splice the search results, spi error: %d.", iError);
            goto bailFromiGtwySearchIndex;
        }
    }



    /* Get the search time and optionally log it to the search report */
    {
        struct timeval    tvSearchEndTimeVal;
        struct timeval    tvSearchDiffTimeVal;

        /* Set the search end time */
        s_gettimeofday(&tvSearchEndTimeVal, NULL);

        /* Get the time taken for this search */
        UTL_DATE_DIFF_TIMEVAL(tvSearchStartTimeVal, tvSearchEndTimeVal, tvSearchDiffTimeVal);

        /* Turn it into milliseconds */
        UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvSearchDiffTimeVal, dSearchTime);
    }



    /* Bail label */
    bailFromiGtwySearchIndex:


    {
        unsigned char                pucIndexNames[SPI_INDEX_NAME_MAXIMUM_LENGTH * 3] = {'\0'};
        struct spiSearchResponse    *pssrSpiSearchResponse = NULL;

        /* Create a nice, comma delimited, index name from the index name list for the log */
        for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
            
            /* Dereference the gateway index for convenience */
            pgdGtwyIndex = (struct gtwyIndex *)ppvIndexList[uiI];
        
            /* Create a nice, comma delimited, index name from the index name list */
            if ( bUtlStringsIsStringNULL(pucIndexNames) == false ) {
                s_strnncat(pucIndexNames, ", ", (SPI_INDEX_NAME_MAXIMUM_LENGTH * 3) - 1, SPI_INDEX_NAME_MAXIMUM_LENGTH * 3);
            }
            s_strnncat(pucIndexNames, pgdGtwyIndex->pucIndexName, (SPI_INDEX_NAME_MAXIMUM_LENGTH * 3) - 1, SPI_INDEX_NAME_MAXIMUM_LENGTH * 3);
        }


        /* Allocate the search response - screwy way to do it because we can't bail at this point but we can let the error flow */
        if ( iError == SPI_NoError ) {
            if ( (pssrSpiSearchResponse = (struct spiSearchResponse *)s_malloc((size_t)(sizeof(struct spiSearchResponse)))) == NULL ) {
                iError = SPI_MemError;
            }
        }


        /* Handle the error */
        if ( iError == SPI_NoError ) {
    
            unsigned int uiSearchReportCount = 0;
        
            /* Count up the search reports */
            for ( pssrSpiSearchResultsPtr = pssrSpiSearchResults; pssrSpiSearchResultsPtr < (pssrSpiSearchResults + uiSpiSearchResultsLength); pssrSpiSearchResultsPtr++ ) {

                /* Increment the search report counter when we encounter one */
                if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {

                    uiSearchReportCount++;
                }
            }

            /* Log the appropriate log line */
            if ( uiSearchReportCount > 0 ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Search, %s: '%s', search: '%s', found: %u document%s and returned: %u (+%u search report%s), in: %.1f milliseconds.", 
                        (s_strchr(pucIndexNames, ',') != NULL) ? "indices" : "index", pucIndexNames, pucUtlStringsGetSafeString(pucSearchText),
                        uiTotalResults, (uiTotalResults == 1) ? "" : "s", uiSpiSearchResultsLength - uiSearchReportCount, 
                        uiSearchReportCount, (uiSearchReportCount == 1) ? "" : "s", dSearchTime);
            }
            else {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Search, %s: '%s', search: '%s', found: %u document%s and returned: %u, in: %.1f milliseconds.", 
                        (s_strchr(pucIndexNames, ',') != NULL) ? "indices" : "index", pucIndexNames, pucUtlStringsGetSafeString(pucSearchText),
                        uiTotalResults, (uiTotalResults == 1) ? "" : "s", uiSpiSearchResultsLength, dSearchTime);
            }

/*             iSpiPrintSpiSearchResults(pssrSpiSearchResults, uiSpiSearchResultsLength, uiSortType); */


            /* Check the search results */
            ASSERT(((pssrSpiSearchResults != NULL) && (uiSpiSearchResultsLength > 0)) || ((pssrSpiSearchResults == NULL) && (uiSpiSearchResultsLength == 0)));

            /* Set the search response values */
            pssrSpiSearchResponse->pssrSpiSearchResults = pssrSpiSearchResults;
            pssrSpiSearchResponse->uiSpiSearchResultsLength = uiSpiSearchResultsLength;
            pssrSpiSearchResponse->uiTotalResults = uiTotalResults;
            pssrSpiSearchResponse->uiStartIndex = (uiSpiSearchResultsLength > 0) ? uiStartIndex : 0;
            pssrSpiSearchResponse->uiEndIndex = ((uiSpiSearchResultsLength - uiSearchReportCount) > 0) ? ((uiStartIndex + (uiSpiSearchResultsLength - uiSearchReportCount)) - 1) : 0;
            pssrSpiSearchResponse->uiSortType = uiSortType;
            pssrSpiSearchResponse->dMaxSortKey = dMaxSortKey;
            pssrSpiSearchResponse->dSearchTime = dSearchTime;

            /* Set the return pointer */
            *ppssrSpiSearchResponse = pssrSpiSearchResponse;
        }
        else {
    
            iUtlLogInfo(UTL_LOG_CONTEXT, "Search, %s: '%s', search: '%s', spi error: %d.", (s_strchr(pucIndexNames, ',') != NULL) ? "index" : "index", 
                    pucIndexNames, pucUtlStringsGetSafeString(pucSearchText), iError);


            /* Collect all threads, ignore errors */
            if ( ptThreads != NULL ) {
                for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiSearchIndex - Joining thread: %u", uiI); */
                    s_pthread_join(ptThreads[uiI], NULL);
                }
            }
    
            /* Need to free spi search results if search gateway index is still allocated */        
            if ( pgsgdGtwySearchGatewayIndices != NULL ) {
    
                /* Loop through all the index and free the search results associated with each index */        
                for ( uiI = 0, pgsgdGtwySearchGatewayIndicesPtr = pgsgdGtwySearchGatewayIndices; ppvIndexList[uiI] != NULL; uiI++, pgsgdGtwySearchGatewayIndicesPtr++ ) {
        
                    /* Free the gateway index search response */
                    iSpiFreeSearchResponse(pgsgdGtwySearchGatewayIndicesPtr->pssrSpiSearchResponse);
                    pssrSpiSearchResponse = NULL;
                }
            }
    
            /* Free the search results */
            iSpiFreeSearchResults(pssrSpiSearchResults, uiSpiSearchResultsLength, uiSortType);
            pssrSpiSearchResults = NULL;
        }
    }


    /* Free allocations */
    s_free(ptThreads);
    s_free(pgsgdGtwySearchGatewayIndices);
    s_free(pgsoGtwySearchOptions);
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiRetrieveDocument()

    Purpose:    This function should return the text/data specified by the document
                ID from chunk = 'uiChunkStart' to chunk <= 'uiChunkEnd'.

                The chunk start and end are meaningless if a document chunk type is
                requested.

    Parameters: pssSpiSession       spi session structure
                pvIndex             index structure
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                uiChunkType         chunk type
                uiChunkStart        start of chunk
                uiChunkEnd          end of chunk
                ppvData             return pointer of data returned
                puiDataLength       return pointer for length of data returned

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiRetrieveDocument
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int                 iError = SPI_NoError;
    struct gtwyGateway  *pggGtwyGateway = NULL;
    struct gtwyIndex    *pgdGtwyIndex = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiRetrieveDocument [%s][%s][%s][%u][%u][%u]",  */
/*             pucDocumentKey, pucItemName, pucMimeType, uiChunkType, uiChunkStart, uiChunkEnd); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "NULL or empty 'pucDocumentKey' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidDocumentKey);
    }

    if ( bUtlStringsIsStringNULL(pucItemName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "NULL or empty 'pucItemName' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidItemName);
    }

    if ( bUtlStringsIsStringNULL(pucMimeType) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "NULL or empty 'pucMimeType' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidMimeType);
    }

    if ( SPI_CHUNK_TYPE_VALID(uiChunkType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkType' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkType);
    }

    if ( uiChunkStart < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkRange);
    }

    if ( uiChunkEnd < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkRange);
    }

    if ( uiChunkStart > uiChunkEnd ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' and 'uiChunkEnd' parameters passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkRange);
    }

    if ( ppvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvData' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiDataLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDataLength' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the gateway index pointer into a gateway index structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Set the last access time */
    pgdGtwyIndex->tLastAccessTime = s_time(NULL);


    /* Retrieve the document */
    switch ( uiChunkType ) {

        case SPI_CHUNK_TYPE_BYTE:
        case SPI_CHUNK_TYPE_DOCUMENT:

            /* Retrieve by byte, document */
            iError = iGtwyRetrieveDocumentFromGatewayIndex(pggGtwyGateway, pgdGtwyIndex, pucDocumentKey, pucItemName, pucMimeType, 
                        uiChunkType, uiChunkStart, uiChunkEnd, ppvData, puiDataLength);
            break;

        default:
            /* Retrieve by... nope its an error */
            iError = SPI_InvalidChunkType;
    }



    /* Bail label */
/*     bailFromiGtwyGetDocument: */


    /* Log the retrieval */
    {
        
        unsigned char   pucErrorMessage[UTL_FILE_PATH_MAX + 1] = {'\0'};

        /* Put together an error message if there was an error */
        if ( iError != SPI_NoError ) {
            snprintf(pucErrorMessage, UTL_FILE_PATH_MAX, ", spi error: %d", iError);
        }
        
        /* Log according to chunk type */
        switch ( uiChunkType ) {
    
            case SPI_CHUNK_TYPE_BYTE:
                /* Retrieve by bytes */
                iUtlLogDebug(UTL_LOG_CONTEXT, "Retrieving document, index: '%s', document key: '%s', byte range: %u %u, item: '%s', mime type: '%s'%s.", 
                        pgdGtwyIndex->pucIndexName, pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, pucErrorMessage);
                break;
    
            case SPI_CHUNK_TYPE_DOCUMENT:
                /* Retrieve by document */
                iUtlLogDebug(UTL_LOG_CONTEXT, "Retrieving document, index: '%s', document key: '%s', item: '%s', mime type: '%s'%s.", 
                        pgdGtwyIndex->pucIndexName, pucDocumentKey, pucItemName, pucMimeType, pucErrorMessage);
                break;
    
            default:
                /* Invalid chunk type */
                iUtlLogDebug(UTL_LOG_CONTEXT, "Retrieving document, index: '%s', document key: '%s', invalid chunk type, item: '%s', mime type: '%s'%s.", 
                        pgdGtwyIndex->pucIndexName, pucDocumentKey, pucItemName, pucMimeType, pucErrorMessage);
                break;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ==========================================
** ===  Server Information SPI Functions  ===
** ==========================================
*/


/*

    Function:   iSpiGetServerInfo()

    Purpose:    This function should allocate, populate a single spi server info
                structure. If an error is returned, the return pointer
                will be ignored.

    Parameters: pssSpiSession       spi session structure
                ppssiSpiServerInfo  return pointer for the spi server info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetServerInfo
(
    struct spiSession *pssSpiSession,
    struct spiServerInfo **ppssiSpiServerInfo
)
{

    int                     iError = SPI_NoError;
    struct gtwyGateway      *pggGtwyGateway = NULL;
    struct spiServerInfo    *pssiSpiServerInfo = NULL;
    unsigned char           pucHostName[MAXHOSTNAMELEN + 1] = {'\0'};
    unsigned char           pucSubKeys[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           *pucSubKeysStrtokPtr = NULL;
    unsigned char           pucSymbolData[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           *pucPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetServerInfo"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetServerInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( ppssiSpiServerInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiSpiServerInfo' parameter passed to 'iSpiGetServerInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }



    /* Allocate a spiServerInfo structure */
    if ( (pssiSpiServerInfo = (struct spiServerInfo *)s_malloc((size_t)(sizeof(struct spiServerInfo)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyGetServerInfo;
    }

    /* Get the host name */
    if ( iUtlNetGetHostName(pucHostName, MAXHOSTNAMELEN + 1) == UTL_NoError ) {
        if ( (pssiSpiServerInfo->pucName = (unsigned char *)s_strdup(pucHostName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyGetServerInfo;
        }
    }


    /* Get the gateway description */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_DESCRIPTION, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( (pssiSpiServerInfo->pucDescription = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyGetServerInfo;
        }
    }

    /* Get the gateway administrator name */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_ADMIN_NAME, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( (pssiSpiServerInfo->pucAdminName = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyGetServerInfo;
        }
    }

    /* Get the gateway administrator email */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_ADMIN_EMAIL, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( (pssiSpiServerInfo->pucAdminEmail = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyGetServerInfo;
        }
    }


    /* Preset the index count */
    pssiSpiServerInfo->uiIndexCount = SPI_SERVER_INDEX_COUNT_UNKNOWN;

    /* Get the list of index (autodiscovered) */
    if ( iUtlConfigGetSubKeys(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX, pucSubKeys, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        /* Loop counting the index names */
        for ( pucPtr = s_strtok_r(pucSubKeys, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSubKeysStrtokPtr);
                pucPtr != NULL;
                pucPtr = s_strtok_r(NULL, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSubKeysStrtokPtr) ) {

            /* Increment the number of index */
            pssiSpiServerInfo->uiIndexCount++;
        }
    }

    /* Get the list of index locations (fixed) */
    if ( iUtlConfigGetSubKeys(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_LOCATION, pucSubKeys, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        /* Loop counting the index names */
        for ( pucPtr = s_strtok_r(pucSubKeys, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSubKeysStrtokPtr);
                pucPtr != NULL;
                pucPtr = s_strtok_r(NULL, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSubKeysStrtokPtr) ) {

            /* Increment the number of index */
            pssiSpiServerInfo->uiIndexCount++;
        }
    }


    /* Set the ranking algorithm */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_GATEWAY_RANKING_ALGORITHM, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( (pssiSpiServerInfo->pucRankingAlgorithm = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyGetServerInfo;
        }
    }


    /* Set the min and max weight */
    pssiSpiServerInfo->dWeightMinimum = SPI_SERVER_WEIGHT_MINIMUM;
    pssiSpiServerInfo->dWeightMaximum = SPI_SERVER_WEIGHT_MAXIMUM;



    /* Bail label */
    bailFromiGtwyGetServerInfo:
    
    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppssiSpiServerInfo = pssiSpiServerInfo;
    }
    else {

        iSpiFreeServerInfo(pssiSpiServerInfo);
        pssiSpiServerInfo = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetServerIndexInfo()

    Purpose:    This function should allocate, populate and return a array of spi 
                server index info structures. The number of entries in the array 
                should be returned in puiSpiServerIndexInfosLength. If an error is returned, 
                the return pointers will be ignored.

    Parameters: pssSpiSession                   spi session structure
                ppssiiSpiServerIndexInfos       return pointer for an array of spi server index info structures
                puiSpiServerIndexInfosLength    return pointer for the number of entries
                                                in the spi server index info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetServerIndexInfo
(
    struct spiSession *pssSpiSession,
    struct spiServerIndexInfo **ppssiiSpiServerIndexInfos,
    unsigned int *puiSpiServerIndexInfosLength
)
{

    int                         iError = SPI_NoError;
    struct gtwyGateway          *pggGtwyGateway = NULL;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
    unsigned int                uiSpiServerIndexInfosLength = 0;
    unsigned char               pucSymbolData[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               *pucSymbolStrtokPtr = NULL;
    unsigned char               *pucPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetServerIndexInfo"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetServerIndexInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( ppssiiSpiServerIndexInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiiSpiServerIndexInfos' parameter passed to 'iSpiGetServerIndexInfo'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiSpiServerIndexInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiServerIndexInfosLength' parameter passed to 'iSpiGetServerIndexInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Get the list of indices */
    if ( iUtlConfigGetSubKeys(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        /* Loop adding the index names */
        for ( pucPtr = s_strtok_r(pucSymbolData, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSymbolStrtokPtr), uiSpiServerIndexInfosLength = 0;
                pucPtr != NULL;
                pucPtr = s_strtok_r(NULL, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSymbolStrtokPtr), uiSpiServerIndexInfosLength++ ) {

            struct spiServerIndexInfo    *pssiiSpiServerIndexInfosPtr = NULL;

            /* Add a new entry in our server index info pointer */
            if ( (pssiiSpiServerIndexInfosPtr = (struct spiServerIndexInfo *)s_realloc(pssiiSpiServerIndexInfos,
                    (size_t)(sizeof(struct spiServerIndexInfo) * (uiSpiServerIndexInfosLength + 1)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwyGetServerIndexInfo;
            }

            /* Hand over the pointer */
            pssiiSpiServerIndexInfos = pssiiSpiServerIndexInfosPtr;

            /* Derefence the server index info pointer for convenience */
            pssiiSpiServerIndexInfosPtr = (pssiiSpiServerIndexInfos + uiSpiServerIndexInfosLength);

            /* Set the index name */
            if ( (pssiiSpiServerIndexInfosPtr->pucName = (unsigned char *)s_strdup(pucPtr)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwyGetServerIndexInfo;
            }

            /* Set the index description */
            pssiiSpiServerIndexInfosPtr->pucDescription = NULL;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexInfo [%u][%s]", uiSpiServerIndexInfosLength - 1,  */
/*                     (pssiiSpiServerIndexInfos + (uiSpiServerIndexInfosLength - 1))->pucIndexName); */
        }
    }


    /* Get the list of index locations */
    if ( iUtlConfigGetSubKeys(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_LOCATION, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        /* Loop adding the index names - dont initialize uiSpiServerIndexInfosLength since we are adding to the server index info pointer */
        for ( pucPtr = s_strtok_r(pucSymbolData, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSymbolStrtokPtr)/* , uiSpiServerIndexInfosLength = 0 */;
                pucPtr != NULL;
                pucPtr = s_strtok_r(NULL, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSymbolStrtokPtr), uiSpiServerIndexInfosLength++ ) {

            struct spiServerIndexInfo    *pssiiSpiServerIndexInfosPtr = NULL;

            /* Add a new entry in our server index info pointer */
            if ( (pssiiSpiServerIndexInfosPtr = (struct spiServerIndexInfo *)s_realloc(pssiiSpiServerIndexInfos,
                    (size_t)(sizeof(struct spiServerIndexInfo) * (uiSpiServerIndexInfosLength + 1)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwyGetServerIndexInfo;
            }

            /* Hand over the pointer */
            pssiiSpiServerIndexInfos = pssiiSpiServerIndexInfosPtr;

            /* Derefence the server index info pointer for convenience */
            pssiiSpiServerIndexInfosPtr = (pssiiSpiServerIndexInfos + uiSpiServerIndexInfosLength);

            /* Set the index name */
            if ( (pssiiSpiServerIndexInfosPtr->pucName = (unsigned char *)s_strdup(pucPtr)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwyGetServerIndexInfo;
            }

            /* Set the index description */
            pssiiSpiServerIndexInfosPtr->pucDescription = NULL;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexInfo [%u][%s]", uiSpiServerIndexInfosLength - 1,  */
/*                     (pssiiSpiServerIndexInfos + (uiSpiServerIndexInfosLength - 1))->pucIndexName); */
        }
    }



    /* Bail label */
    bailFromiGtwyGetServerIndexInfo:
    
    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers */
        if ( pssiiSpiServerIndexInfos != NULL ) {
            *ppssiiSpiServerIndexInfos = pssiiSpiServerIndexInfos;
            *puiSpiServerIndexInfosLength = uiSpiServerIndexInfosLength;
        }
        else {
            iError = SPI_ServerHasNoIndices;
        }
    }
    else {

        iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
        pssiiSpiServerIndexInfos = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexInfo()

    Purpose:    This function should allocate and return a single spi index info structuren 
                populated with information pertinent to the index contained in the 
                index structure. If an error is returned, the return pointer will be ignored.

    Parameters: pssSpiSession       spi session structure
                pvIndex             index structure
                ppsiiSpiIndexInfo   return pointer for the spi index info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    struct spiIndexInfo **ppsiiSpiIndexInfo
)
{

    int                     iError = LWPS_NoError;
    struct gtwyGateway      *pggGtwyGateway = NULL;
    struct gtwyIndex        *pgdGtwyIndex = NULL;
    struct spiIndexInfo     *psiiSpiIndexInfo = NULL;
    unsigned char           pucSymbolData[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    struct gtwyMirror       *pgmGtwyMirrorsPtr = NULL;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;
    boolean                 bIndexInfoInCache = false;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexInfo"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppsiiSpiIndexInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsiiSpiIndexInfo' parameter passed to 'iSpiGetIndexInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index pointer into a gateway index structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Reset the temporary errors */
    if ( (iError = iGtwyResetTemporaryErrorsOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Reset the overrides */
    if ( (iError = iGtwyResetSearchOverridesOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Set the last access time */
    pgdGtwyIndex->tLastAccessTime = s_time(NULL);


    /* Where did this index come from? */
    switch ( pgdGtwyIndex->uiIndexOrigin ) {

        case GTWY_INDEX_ORIGIN_FROM_CONFIG_ID:

            /* We get the index information from the configuration file if there is more than one segment, 
            ** otherwise we get it directly from the mirror by dropping through the next case statement
            */
            if ( pgdGtwyIndex->uiGtwySegmentsLength > 1 ) {

                /* Allocate the return structure */
                if ( (psiiSpiIndexInfo = (struct spiIndexInfo *)s_malloc((size_t)(sizeof(struct spiIndexInfo)))) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexInfo;
                }

                /* And populate it */
                if ( (psiiSpiIndexInfo->pucName = (unsigned char *)s_strdup(pgdGtwyIndex->pucIndexName)) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiGtwyGetIndexInfo;
                }

                if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_DESCRIPTION, pgdGtwyIndex->pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
                    if ( (psiiSpiIndexInfo->pucDescription = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexInfo;
                    }
                }

                if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_LANGUAGE_NAME, pgdGtwyIndex->pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
                    if ( (psiiSpiIndexInfo->pucLanguageCode = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexInfo;
                    }
                }

                if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_TOKENIZER_NAME, pgdGtwyIndex->pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
                    if ( (psiiSpiIndexInfo->pucTokenizerName = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexInfo;
                    }
                }

                if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_STEMMER_NAME, pgdGtwyIndex->pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
                    if ( (psiiSpiIndexInfo->pucStemmerName = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexInfo;
                    }
                }

                if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_STOP_LIST_NAME, pgdGtwyIndex->pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
                    if ( (psiiSpiIndexInfo->pucStopListName = (unsigned char *)s_strdup(pucSymbolData)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexInfo;
                    }
                }

                psiiSpiIndexInfo->uiDocumentCount = SPI_INDEX_DOCUMENT_COUNT_UNKNOWN;
                psiiSpiIndexInfo->ulTotalTermCount = SPI_INDEX_TOTAL_TERM_COUNT_UNKNOWN;
                psiiSpiIndexInfo->ulUniqueTermCount = SPI_INDEX_UNIQUE_TERM_COUNT_UNKNOWN;
                psiiSpiIndexInfo->ulTotalStopTermCount = SPI_INDEX_TOTAL_TERM_COUNT_UNKNOWN;
                psiiSpiIndexInfo->ulUniqueStopTermCount = SPI_INDEX_UNIQUE_TERM_COUNT_UNKNOWN;
                psiiSpiIndexInfo->uiAccessControl = SPI_INDEX_ACCESS_UNKNOWN;
                psiiSpiIndexInfo->uiUpdateFrequency = SPI_INDEX_UPDATE_UNKNOWN;
                psiiSpiIndexInfo->ulLastUpdateAnsiDate = 0;
                psiiSpiIndexInfo->uiCaseSensitive = SPI_INDEX_CASE_UNKNOWN;

                break;
            }
            
            /* Fall through for index of type 'mirror' if there is only one lwps index */


        case GTWY_INDEX_ORIGIN_FROM_URL_ID:

            /* Dereference down to the first gateway mirror for convenience */
            pgmGtwyMirrorsPtr = pgdGtwyIndex->pgsGtwySegments->pgmGtwyMirrors;

            /* Get the index information from the cache, otherwise get it from the gateway mirror */
            if ( iGtwyGetFromInfoCache(pggGtwyGateway, pgmGtwyMirrorsPtr->pucCanonicalIndexName, GTWY_INFO_CACHE_TYPE_INDEX_INFO, 
                    (void **)&psiiSpiIndexInfo, NULL) == SPI_NoError ) {
                bIndexInfoInCache = true;
            }
            else {

                /* Set the timeout */
                if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorsPtr->pvUtlNet, pgdGtwyIndex->uiInformationTimeOut)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the net information timeout, index: '%s', utl error: %d.", 
                            pgmGtwyMirrorsPtr->pucCanonicalIndexName, iError);
                    iError = SPI_GetIndexInfoFailed;
                    goto bailFromiGtwyGetIndexInfo;
                }
    
                /* Get the index information */
                if ( (iError = iLwpsIndexInfoRequestHandle(pgmGtwyMirrorsPtr->pvLwps, pgmGtwyMirrorsPtr->pucIndexName, 
                        &psiiSpiIndexInfo, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index information request, index: '%s', lwps error: %d.", 
                            pgmGtwyMirrorsPtr->pucIndexName, iError);
                    iError = SPI_GetIndexInfoFailed;
                    goto bailFromiGtwyGetIndexInfo;
                }
    
                /* Check the error code and set the error from it */
                if ( iErrorCode != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index information, index: '%s', returned error: %d, error text: '%s'.", 
                            pgmGtwyMirrorsPtr->pucIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                    s_free(pucErrorString);
                    iError = iErrorCode;
                    goto bailFromiGtwyGetIndexInfo;
                }

                /* Add the index information to the cache */
                if ( iGtwyAddToInfoCache(pggGtwyGateway, pgmGtwyMirrorsPtr->pucCanonicalIndexName, GTWY_INFO_CACHE_TYPE_INDEX_INFO, 
                        (void *)psiiSpiIndexInfo, 0) == SPI_NoError ) {
                    bIndexInfoInCache = true;
                }
            }

            /* We need to duplicate the index information structure if it is in the cache */
            if ( bIndexInfoInCache == true ) {

                struct spiIndexInfo    *psdiSpiIndexInfoCopy = NULL;

                /* Duplicate the index information */
                if ( (iError = iSpiDuplicateIndexInfo(psiiSpiIndexInfo, &psdiSpiIndexInfoCopy)) != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to duplicate the index information, index: '%s', spi error: %d.", 
                            pgmGtwyMirrorsPtr->pucCanonicalIndexName, iError);
                    iError = SPI_GetIndexInfoFailed;
                    goto bailFromiGtwyGetIndexInfo;
                }
                
                /* Hand over the pointer */
                psiiSpiIndexInfo = psdiSpiIndexInfoCopy;
            }

            break;


        default:

            /* Unknown index origin */
            iError = SPI_GetIndexInfoFailed;
            goto bailFromiGtwyGetIndexInfo;
    }



    /* Bail label */
    bailFromiGtwyGetIndexInfo:
    
    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppsiiSpiIndexInfo = psiiSpiIndexInfo;
    }
    else {

        iSpiFreeIndexInfo(psiiSpiIndexInfo);
        psiiSpiIndexInfo = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexFieldInfo()

    Purpose:    This function should allocate and return an array of spi field info
                structures, populated with information pertinent to the index contained 
                in the index structure. The number of entries in the array should be returned 
                in puiSpiFieldInfosLength. If an error is returned, the return pointer
                will be ignored.

                Note that returning SPI_IndexHasNoSearchFields is not strictly
                an error, but the field list will also be ignored.

    Parameters: pssSpiSession           spi session structure
                pvIndex                 index structure
                ppsfiSpiFieldInfos      return pointer for an array of spi field info structures
                puiSpiFieldInfosLength  return pointer for the number of entries
                                        in the spi field info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexFieldInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    struct spiFieldInfo **ppsfiSpiFieldInfos,
    unsigned int *puiSpiFieldInfosLength
)
{

    int                     iError = LWPS_NoError;
    struct gtwyGateway      *pggGtwyGateway = NULL;
    struct gtwyIndex        *pgdGtwyIndex = NULL;
    struct spiFieldInfo     *psfiSpiFieldInfos = NULL;
    unsigned int            uiSpiFieldInfosLength = 0;
    unsigned char           *pucFieldName = NULL;
    unsigned char           pucFieldNames[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           *pucFieldNamesStrtokPtr = NULL;
    unsigned char           pucFieldDescription[SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           pucFieldType[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    struct gtwyMirror       *pgmGtwyMirrorsPtr = NULL;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;
    boolean                 bIndexFieldInfoInCache = false;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexFieldInfo"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppsfiSpiFieldInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsfiSpiFieldInfos' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiSpiFieldInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiFieldInfosLength' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index pointer into a gateway index structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Reset the temporary errors */
    if ( (iError = iGtwyResetTemporaryErrorsOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Reset the overrides */
    if ( (iError = iGtwyResetSearchOverridesOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Set the last access time */
    pgdGtwyIndex->tLastAccessTime = s_time(NULL);


    /* Where did this index come from? */
    switch ( pgdGtwyIndex->uiIndexOrigin ) {

        case GTWY_INDEX_ORIGIN_FROM_CONFIG_ID:

            /* We get the index information from the configuration file if there is more than one segment, 
            ** otherwise we get it directly from the mirror by dropping through the next case statement
            */
            if ( pgdGtwyIndex->uiGtwySegmentsLength > 1 ) {

                /* Get the field names for this index */
                if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_FIELD_NAMES, pgdGtwyIndex->pucIndexName, pucFieldNames, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) != UTL_NoError ) {
                    /* Could not find the symbol we were looking for, this index does not have any search fields */
                    iError = SPI_IndexHasNoSearchFields;
                    goto bailFromiGtwyGetIndexFieldInfo;
                }


                /* Loop while there are field name tokens to process */
                for ( pucFieldName = s_strtok_r(pucFieldNames, GTWY_FIELD_NAME_SEPARATORS, (char **)&pucFieldNamesStrtokPtr), uiSpiFieldInfosLength = 0;
                        pucFieldName != NULL;
                        pucFieldName = s_strtok_r(NULL, GTWY_FIELD_NAME_SEPARATORS, (char **)&pucFieldNamesStrtokPtr), uiSpiFieldInfosLength++ ) {

                    unsigned char           pucSubKey[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
                    struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;

                    /* Initialize the variables */
                    pucFieldDescription[0] = '\0';
                    pucFieldType[0] = '\0';
                    
                    /* Create the subkey */
                    snprintf(pucSubKey, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1, "%s:%s", pgdGtwyIndex->pucIndexName, pucFieldName);

                    /* Get the field description for this field */
                    iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_FIELD_DESCRIPTION, pucSubKey, pucFieldDescription, SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH + 1);

                    /* Get the field type for this field */
                    iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_FIELD_TYPE, pucSubKey, pucFieldType, SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH + 1);


                    /* Valid field name, add this field to the field name list */
                    if ( (psfiSpiFieldInfosPtr = (struct spiFieldInfo *)s_realloc(psfiSpiFieldInfos, 
                            (size_t)(sizeof(struct spiFieldInfo) * (uiSpiFieldInfosLength + 1)))) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexFieldInfo;
                    }


                    /* Hand over the pointer */
                    psfiSpiFieldInfos = psfiSpiFieldInfosPtr;

                    /* Dereference the field name pointer for convenience */
                    psfiSpiFieldInfosPtr = (psfiSpiFieldInfos + uiSpiFieldInfosLength);

                    /* Set the field name */
                    if ( (psfiSpiFieldInfosPtr->pucName = (unsigned char *)s_strdup(pucFieldName)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexFieldInfo;
                    }

                    /* Set the field description */
                    if ( (psfiSpiFieldInfosPtr->pucDescription = (unsigned char *)s_strdup(pucFieldDescription)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwyGetIndexFieldInfo;
                    }

                    /* Set the field type */
                    if ( s_strcasecmp(GTWY_INDEX_FIELD_TYPE_UNKNOWN, pucFieldType) == 0 ) {
                        psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_UNKNOWN;
                    }
                    else if ( s_strcasecmp(GTWY_INDEX_FIELD_TYPE_TEXT, pucFieldType) == 0 ) {
                        psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_TEXT;
                    }
                    else if ( s_strcasecmp(GTWY_INDEX_FIELD_TYPE_NUMBER, pucFieldType) == 0 ) {
                        psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_NUMERIC;
                    }
                    else if ( s_strcasecmp(GTWY_INDEX_FIELD_TYPE_DATE, pucFieldType) == 0 ) {
                        psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_DATE;
                    }
                    else {
                        psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_UNKNOWN;
                    }

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexFieldInfo [%u][%s][%s][%u]", uiSpiFieldInfosLength - 1,  */
/*                             (psfiSpiFieldInfos + (uiSpiFieldInfosLength - 1))->pucName, */
/*                             pucUtlStringsGetPrintableString((psfiSpiFieldInfos + (uiSpiFieldInfosLength - 1))->pucDescription), */
/*                             (psfiSpiFieldInfos + (uiSpiFieldInfosLength - 1))->uiType); */
                }

                break;
            }

            /* Fall through for index of type 'mirror' if there is only one lwps index */


        case GTWY_INDEX_ORIGIN_FROM_URL_ID:

            /* Dereference down to the first gateway mirror for convenience */
            pgmGtwyMirrorsPtr = pgdGtwyIndex->pgsGtwySegments->pgmGtwyMirrors;

            /* Get the field information from the cache, otherwise get it from the gateway mirror */
            if ( iGtwyGetFromInfoCache(pggGtwyGateway, pgmGtwyMirrorsPtr->pucCanonicalIndexName, GTWY_INFO_CACHE_TYPE_FIELD_INFO, 
                    (void **)&psfiSpiFieldInfos, &uiSpiFieldInfosLength) == SPI_NoError ) {
                bIndexFieldInfoInCache = true;
            }
            else {

                /* Set the information timeout */
                if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorsPtr->pvUtlNet, pgdGtwyIndex->uiInformationTimeOut)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the net information timeout, index: '%s', utl error: %d.", 
                            pgmGtwyMirrorsPtr->pucCanonicalIndexName, iError);
                    iError = SPI_GetIndexFieldInfoFailed;
                    goto bailFromiGtwyGetIndexFieldInfo;
                }
    
                /* Get the index field information */
                if ( (iError = iLwpsIndexFieldInfoRequestHandle(pgmGtwyMirrorsPtr->pvLwps, pgmGtwyMirrorsPtr->pucIndexName, 
                        &psfiSpiFieldInfos, &uiSpiFieldInfosLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index field information request, index: '%s', lwps error: %d.", 
                            pgmGtwyMirrorsPtr->pucIndexName, iError);
                    iError = SPI_GetIndexFieldInfoFailed;
                    goto bailFromiGtwyGetIndexFieldInfo;
                }
    
                /* Check the error code and set the error from it */
                if ( iErrorCode != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index field information, index: '%s', returned error: %d, error text: '%s'.", 
                            pgmGtwyMirrorsPtr->pucIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                    s_free(pucErrorString);
                    iError = iErrorCode;
                    goto bailFromiGtwyGetIndexFieldInfo;
                }

                /* Add the index field information to the cache */
                if ( iGtwyAddToInfoCache(pggGtwyGateway, pgmGtwyMirrorsPtr->pucCanonicalIndexName, GTWY_INFO_CACHE_TYPE_FIELD_INFO, 
                        (void *)psfiSpiFieldInfos, uiSpiFieldInfosLength) == SPI_NoError ) {
                    bIndexFieldInfoInCache = true;
                }
            }

            /* We need to duplicate the field info structure if it is in the cache */
            if ( bIndexFieldInfoInCache == true ) {

                struct spiFieldInfo    *psfiSpiFieldInfoCopy = NULL;

                /* Duplicate the field information */
                if ( (iError = iSpiDuplicateIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength, &psfiSpiFieldInfoCopy)) != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to duplicate the index field information, index: '%s', spi error: %d.", 
                            pgmGtwyMirrorsPtr->pucCanonicalIndexName, iError);
                    goto bailFromiGtwyGetIndexFieldInfo;
                }
                
                /* Hand over the pointer */
                psfiSpiFieldInfos = psfiSpiFieldInfoCopy;
            }

            break;


        default:
            iError = SPI_GetIndexFieldInfoFailed;
            goto bailFromiGtwyGetIndexFieldInfo;

    }



    /* Bail label */
    bailFromiGtwyGetIndexFieldInfo:
    
    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        if ( psfiSpiFieldInfos != NULL ) {
            *ppsfiSpiFieldInfos = psfiSpiFieldInfos;
            *puiSpiFieldInfosLength = uiSpiFieldInfosLength;
        }
        else {
            iError = SPI_IndexHasNoSearchFields;
        }
    }
    else {

        iSpiFreeIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength);
        psfiSpiFieldInfos = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexTermInfo()

    Purpose:    This function should allocate and return an array of spi term info
                structures, populated with information pertinent to the index contained 
                in the index structure. The number of entries in the array should be 
                returned in puiSpiTermInfosLength. If an error is returned, the return pointer
                will be ignored.

                The uiTermMatch specifies which term type we want to match
                on, pucTerm specifies a term to match on and pucFieldName 
                specifies the field name for which we get the terms.

                Note that returning SPI_IndexHasNoTerms is not strictly
                an error, but the term list will be ignored.

    Parameters: pssSpiSession           spi session structure
                pvIndex                 index structure
                uiTermMatch             term match type
                uiTermCase              term case to search for
                pucTerm                 term to match on (optional)
                pucFieldName            field name to match on (optional)
                ppstiSpiTermInfos       return pointer for an array of spi term info structures
                puiSpiTermInfosLength   return pointer for the number of entries
                                        in the spi term info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexTermInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned int uiTermMatch,
    unsigned int uiTermCase,
    unsigned char *pucTerm,
    unsigned char *pucFieldName,
    struct spiTermInfo **ppstiSpiTermInfos,
    unsigned int *puiSpiTermInfosLength
)
{

    int                 iError = LWPS_NoError;
    struct gtwyGateway  *pggGtwyGateway = NULL;
    struct gtwyIndex    *pgdGtwyIndex = NULL;
    struct gtwyMirror   *pgmGtwyMirrorsPtr = NULL;
    struct spiTermInfo  *pstiSpiTermInfos = NULL;
    int                 uiSpiTermInfosLength = 0;
    int                 iErrorCode = SPI_NoError;
    unsigned char       *pucErrorString = NULL;
    boolean             bIndexTermInfoInCache = false;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexTermInfo"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( SPI_TERM_MATCH_VALID(uiTermMatch) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermMatch' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_InvalidTermMatch);
    }

    if ( SPI_TERM_CASE_VALID(uiTermCase) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermCase' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_InvalidTermCase);
    }

    if ( ppstiSpiTermInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppstiSpiTermInfos' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiSpiTermInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiTermInfosLength' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index pointer into a gateway index structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Reset the temporary errors */
    if ( (iError = iGtwyResetTemporaryErrorsOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Reset the overrides */
    if ( (iError = iGtwyResetSearchOverridesOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Set the last access time */
    pgdGtwyIndex->tLastAccessTime = s_time(NULL);


    /* Where did this index come from? */
    switch ( pgdGtwyIndex->uiIndexOrigin ) {

        case GTWY_INDEX_ORIGIN_FROM_CONFIG_ID:

            /* We return an error if there is more than one segment, otherwise we get it directly from the mirror 
            ** by dropping through the next case statement
            */
            if ( pgdGtwyIndex->uiGtwySegmentsLength > 1 ) {
                iError = SPI_GetIndexTermInfoFailed;
                goto bailFromiGtwyGetIndexTermInfo;
            }
            
            /* Fall through for index of type 'mirror' if there is only one lwps index */


        case GTWY_INDEX_ORIGIN_FROM_URL_ID:

            /* Dereference down to the first gateway mirror for convenience */
            pgmGtwyMirrorsPtr = pgdGtwyIndex->pgsGtwySegments->pgmGtwyMirrors;

            /* Get the term information from the cache, otherwise get it from the  gateway mirror */
            if ( iGtwyGetFromInfoCache(pggGtwyGateway, pgmGtwyMirrorsPtr->pucCanonicalIndexName, GTWY_INFO_CACHE_TYPE_TERM_INFO, 
                    (void **)&pstiSpiTermInfos, &uiSpiTermInfosLength) == SPI_NoError ) {
                bIndexTermInfoInCache = true;
            }
            else {

                /* Set the information timeout */
                if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorsPtr->pvUtlNet, pgdGtwyIndex->uiInformationTimeOut)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the net information timeout, index: '%s', utl error: %d.", 
                            pgmGtwyMirrorsPtr->pucCanonicalIndexName, iError);
                    iError = SPI_GetIndexTermInfoFailed;
                    goto bailFromiGtwyGetIndexTermInfo;
                }
    
                /* Get the index term information */
                if ( (iError = iLwpsIndexTermInfoRequestHandle(pgmGtwyMirrorsPtr->pvLwps, pgmGtwyMirrorsPtr->pucIndexName, 
                        uiTermMatch, uiTermCase, pucTerm, pucFieldName, &pstiSpiTermInfos, &uiSpiTermInfosLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index term information request, index: '%s', lwps error: %d.", 
                            pgmGtwyMirrorsPtr->pucIndexName, iError);
                    iError = SPI_GetIndexTermInfoFailed;
                    goto bailFromiGtwyGetIndexTermInfo;
                }
    
                /* Check the error code and set the error from it */
                if ( iErrorCode != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the term information, index: '%s', returned error: %d, error text: '%s'.", 
                            pgmGtwyMirrorsPtr->pucIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                    s_free(pucErrorString);
                    iError = iErrorCode;
                    goto bailFromiGtwyGetIndexTermInfo;
                }

                /* Add the index term information to the cache */
                if ( iGtwyAddToInfoCache(pggGtwyGateway, pgmGtwyMirrorsPtr->pucCanonicalIndexName, GTWY_INFO_CACHE_TYPE_TERM_INFO, 
                        (void *)pstiSpiTermInfos, uiSpiTermInfosLength) == SPI_NoError ) {
                    bIndexTermInfoInCache = true;
                }
            }

            /* We need to duplicate the index term info structure if it is in the cache */
            if ( bIndexTermInfoInCache == true ) {

                struct spiTermInfo    *pstiSpiTermInfosCopy = NULL;

                /* Duplicate the term information */
                if ( (iError = iSpiDuplicateTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength, &pstiSpiTermInfosCopy)) != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to duplicate the term information, index: '%s', spi error: %d.", 
                            pgmGtwyMirrorsPtr->pucCanonicalIndexName, iError);
                    goto bailFromiGtwyGetIndexTermInfo;
                }
                
                /* Hand over the pointer */
                pstiSpiTermInfos = pstiSpiTermInfosCopy;
            }

            break;


        default:
            /* Unknown index origin */
            iError = SPI_GetIndexTermInfoFailed;
            goto bailFromiGtwyGetIndexTermInfo;
    }



    /* Bail label */
    bailFromiGtwyGetIndexTermInfo:
    
    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        if ( pstiSpiTermInfos != NULL ) {
            *ppstiSpiTermInfos = pstiSpiTermInfos;
            *puiSpiTermInfosLength = uiSpiTermInfosLength;
        }
        else {
            iError = SPI_IndexHasNoTerms;
        }
    }
    else {

        iSpiFreeTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength);
        pstiSpiTermInfos = NULL;
    }

    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetDocumentInfo()

    Purpose:    This function should allocate, populate and return a single spi document info
                structure. If an error is returned, the return pointer
                will be ignored.

    Parameters: pssSpiSession            spi session structure
                pvIndex                    index structure
                pucDocumentKey            document key
                ppsdiSpiDocumentInfo    return pointer for the spi document info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetDocumentInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned char *pucDocumentKey,
    struct spiDocumentInfo **ppsdiSpiDocumentInfo
)
{

    int                            iError = LWPS_NoError;
    struct gtwyGateway            *pggGtwyGateway = NULL;
    struct gtwyIndex            *pgdGtwyIndex = NULL;
    unsigned char                *pucDocumentKeyPtr = NULL;
    unsigned char                pucScanfFormatHost[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char                pucHostAddress[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char                pucHostName[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    int                         iPort = LWPS_PROTOCOL_PORT_DEFAULT;
    unsigned char                pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char                pucRemoteDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int                uiI = 0, uiJ = 0, uiK = 0;
    struct gtwySegment            *pgsGtwySegmentsPtr = NULL;
    struct gtwyMirror            *pgmGtwyMirrorsPtr = NULL;
    struct gtwyMirror            *pgmGtwyMirrorPtr = NULL;
    struct spiDocumentInfo         *psdiSpiDocumentInfo = NULL;
    int                            iErrorCode = SPI_NoError;
    unsigned char                *pucErrorString = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetDocumentInfo"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDocumentKey' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_InvalidDocumentKey);
    }

    if ( ppsdiSpiDocumentInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsdiSpiDocumentInfo' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index pointer into a gateway index structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Reset the temporary errors */
    if ( (iError = iGtwyResetTemporaryErrorsOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Reset the overrides */
    if ( (iError = iGtwyResetSearchOverridesOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Set the last access time */
    pgdGtwyIndex->tLastAccessTime = s_time(NULL);


    /* Where did this index come from? */
    switch ( pgdGtwyIndex->uiIndexOrigin ) {

        case GTWY_INDEX_ORIGIN_FROM_CONFIG_ID:

            /* Parse out the lwps index name and document key into its component parts */
            snprintf(pucScanfFormatHost, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%*[^:]://%%%d[^/]/%%%d[^/]/%%%ds", UTL_CONFIG_VALUE_MAXIMUM_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH);
            if ( sscanf(pucDocumentKey, pucScanfFormatHost, pucHostAddress, pucIndexName, pucRemoteDocumentKey) != 3 ) {
                iError = SPI_InvalidDocumentKey;
                goto bailFromiGtwyGetDocumentInfo;
            }
            
            /* Set the document key pointer to the remote document key */
            pucDocumentKeyPtr = pucRemoteDocumentKey;

            /* Parse out the host address into its component parts */
            snprintf(pucScanfFormatHost, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]:%%d", UTL_CONFIG_VALUE_MAXIMUM_LENGTH);
            if ( sscanf(pucHostAddress, pucScanfFormatHost, pucHostName, &iPort) < 1 ) {
                iError = SPI_InvalidDocumentKey;
                goto bailFromiGtwyGetDocumentInfo;
            }
    
            /* Loop over the gateway segments */
            for ( uiI = 0, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsGtwySegmentsPtr++ ) {
            
                /* Loop over the gateway mirrors */
                for ( uiJ = 0, pgmGtwyMirrorsPtr = pgsGtwySegmentsPtr->pgmGtwyMirrors; uiJ < pgsGtwySegmentsPtr->uiGtwyMirrorsLength; uiJ++, pgmGtwyMirrorsPtr++ ) {

                    /* Compare the host name, the index name and the host port if there is more than one segment,
                    ** otherwise just pick this segment if there is one segment
                    */
                    if ( (pgdGtwyIndex->uiGtwySegmentsLength == 1) || ((s_strcmp(pgmGtwyMirrorsPtr->pucHostName, pucHostName) == 0) && 
                            (s_strcmp(pgmGtwyMirrorsPtr->pucIndexName, pucIndexName) == 0) && (pgmGtwyMirrorsPtr->iPort == iPort)) ) {
                        
                        struct gtwyMirror    *pgmGtwyLocalMirrorsPtr = NULL;

                        /* We found the gateway mirror for this index, now we have to find the 
                        ** gateway mirror that is connected in this gateway segment
                        */
                        for ( uiK = 0, pgmGtwyLocalMirrorsPtr = pgsGtwySegmentsPtr->pgmGtwyMirrors; uiK < pgsGtwySegmentsPtr->uiGtwyMirrorsLength; uiK++, pgmGtwyLocalMirrorsPtr++ ) {
                    
                            /* Select the mirror that is connected, setting the mirror pointer and breaking out */
                            if ( pgmGtwyLocalMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_CONNECTED ) {
                                pgmGtwyMirrorPtr = pgmGtwyLocalMirrorsPtr;
                                break;
                            }
                        }
                    }
                }
            }
    
            break;


        case GTWY_INDEX_ORIGIN_FROM_URL_ID:

            /* Set the document key pointer from the passed document key */
            pucDocumentKeyPtr = pucDocumentKey;

            /* Dereference down to the first gateway mirror */
            pgmGtwyMirrorPtr = pgdGtwyIndex->pgsGtwySegments->pgmGtwyMirrors;

            break;


        default:
            /* Unknown index origin */
            iError = SPI_GetDocumentInfoFailed;
            goto bailFromiGtwyGetDocumentInfo;
    }


    /* Check that we found a gateway mirror and that the document key pointer is set */
    if ( (pgmGtwyMirrorPtr == NULL) || (pucDocumentKeyPtr == NULL) ) {
        iError = SPI_GetDocumentInfoFailed;
        goto bailFromiGtwyGetDocumentInfo;
    }



    /* Set the information timeout */
    if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorsPtr->pvUtlNet, pgdGtwyIndex->uiInformationTimeOut)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the net information timeout, index: '%s', utl error: %d.", 
                pgmGtwyMirrorPtr->pucCanonicalIndexName, iError);
        iError = SPI_GetDocumentInfoFailed;
        goto bailFromiGtwyGetDocumentInfo;
    }

    /* Get the document information */
    if ( (iError = iLwpsDocumentInfoRequestHandle(pgmGtwyMirrorPtr->pvLwps, pgmGtwyMirrorPtr->pucIndexName, 
            pucDocumentKeyPtr, &psdiSpiDocumentInfo, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps document information, index: '%s', lwps error: %d.", 
                pgmGtwyMirrorPtr->pucIndexName, iError);
        iError = SPI_GetDocumentInfoFailed;
        goto bailFromiGtwyGetDocumentInfo;
    }

    /* Check the error code and set the error from it */
    if ( iErrorCode != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information, index: '%s', returned error: %d, error text: '%s'.", 
                pgmGtwyMirrorPtr->pucIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
        s_free(pucErrorString);
        iError = iErrorCode;
        goto bailFromiGtwyGetDocumentInfo;
    }



    /* Bail label */
    bailFromiGtwyGetDocumentInfo:
    
    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppsdiSpiDocumentInfo = psdiSpiDocumentInfo;
    }
    else {

        iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
        psdiSpiDocumentInfo = NULL;
    }

    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexName()

    Purpose:    This function should allocate, populate and return the name of the 
                index for a index structure. If an error is returned, the
                return pointer will be ignored.

    Parameters: pssSpiSession   spi session structure
                pvIndex         index structure
                ppucIndexName   return pointer for the index name

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexName
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned char **ppucIndexName
)
{

    int                 iError = SPI_NoError;
    struct gtwyGateway  *pggGtwyGateway = NULL;
    struct gtwyIndex    *pgdGtwyIndex = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexName"); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexName'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexName'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppucIndexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucIndexName' parameter passed to 'iSpiGetIndexName'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the gateway configuration */
    if ( (iError = iGtwyCheckGatewayConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (iError);
    }

    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index pointer into a gateway index structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Reset the temporary errors */
    if ( (iError = iGtwyResetTemporaryErrorsOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Reset the overrides */
    if ( (iError = iGtwyResetSearchOverridesOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        return (SPI_InvalidIndex);
    }

    /* Set the last access time */
    pgdGtwyIndex->tLastAccessTime = s_time(NULL);


    /* Return an error if the index name is not specified */
    if ( bUtlStringsIsStringNULL(pgdGtwyIndex->pucIndexName) == true ) {
        return (SPI_InvalidIndexName);
    }

    /* Duplicate the index name since the SPI releases it when done and set
    ** the return pointer
    */
    if ( (*ppucIndexName = (unsigned char *)s_strdup(pgdGtwyIndex->pucIndexName)) == NULL ) {
        return (SPI_MemError);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* 
** =========================================
** ===  Gateway Configuration Functions  ===
** =========================================
*/

/*

    Function:   iGtwyCheckGatewayConfiguration()

    Purpose:    This function will reinitialize the gateway if the last status change time
                on the search configuration file changes.
                
                Note that the client pointer in the spi session structure will be updated
                if the server is reinitialized, so it will have to be dereferenced (again)
                after this function is called.

    Parameters: pssSpiSession   spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
static int iGtwyCheckGatewayConfiguration
(
    struct spiSession *pssSpiSession
)
{

    int                 iError = SPI_NoError;
    struct gtwyGateway  *pggGtwyGateway = NULL;
    time_t              tConfigurationFileLastStatusChange = (time_t)0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyCheckGatewayConfiguration."); */


    ASSERT(pssSpiSession != NULL);


    /* Dereference the gateway structure from the client pointer */
    if ( (pggGtwyGateway = (struct gtwyGateway *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Get the last status change time of the gateway configuration file */
    if ( (iError = iUtlFileGetPathStatusChangeTimeT(pggGtwyGateway->pucConfigurationFilePath, &tConfigurationFileLastStatusChange)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the last status change time of the gateway configuration file: '%s', utl error: %d.", 
                pggGtwyGateway->pucConfigurationFilePath, iError); 
        return (SPI_InitializeServerFailed);
    }

    /* Reinitialize the gateway if the last status change time has changed */
    if ( pggGtwyGateway->tConfigurationFileLastStatusChange != tConfigurationFileLastStatusChange ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Reinitializing the gateway because the gateway configuration file: '%s', has changed.", 
                pggGtwyGateway->pucConfigurationFilePath);
        iError = iSpiInitializeServer(pssSpiSession);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ==========================================
** ===  Gateway Auto Discovery Functions  ===
** ==========================================
*/


/*

    Function:   iGtwyAutoDiscoverIndices()

    Purpose:    This function auto-discovers index.

    Parameters: pggGtwyGateway  gateway structure

    Globals:    none

    Returns:    SPI Error Code

*/
static int iGtwyAutoDiscoverIndices
(
    struct gtwyGateway *pggGtwyGateway
)
{

    int             iError = SPI_NoError;
    unsigned char   pucIndices[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucIndexHosts[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlIndexLocationsTrie = NULL;
    void            *pvUtlHostsIndexLocationsTrie = NULL;
    void            *pvUtlIndexHostsIndexLocationsTrie = NULL;
    unsigned int    uiUniqueEntriesCount = 0;

    /* These are here so we are sure they get released in case of an error */
    regex_t         *prRegex = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAutoDiscoverIndices"); */


    ASSERT(pggGtwyGateway != NULL);


    /* Get the index hosts from the config file, and scan them */
    if ( iUtlConfigGetValue(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_HOSTS, pucIndexHosts, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        iUtlLogInfo(UTL_LOG_CONTEXT, "Scanning index hosts: '%s'.", pucIndexHosts);

        /* Scan the index hosts */
        if ( (iError = iGtwyAutoDiscoverIndicesScanHosts(pggGtwyGateway, pucIndexHosts, &pvUtlHostsIndexLocationsTrie)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to scan index hosts: '%s'.", pucIndexHosts);
            goto bailFromiGtwyAutoDiscoverIndices;
        }
    }


    /* Get the index */
    if ( iUtlConfigGetSubKeys(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX, pucIndices, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        unsigned char   *pucIndicesPtr = NULL;
        unsigned char   *pucIndicesStrtokPtr = NULL;


        /* We store the index urls in a trie, so we either reuse the 
        ** trie we got from scanning the index hosts or set a new one up
        */
        if ( pvUtlHostsIndexLocationsTrie != NULL ) {
            pvUtlIndexLocationsTrie = pvUtlHostsIndexLocationsTrie;
        }
        else {
            /* Create the index locations trie */
            if ( (iError = iUtlTrieCreate(&pvUtlIndexLocationsTrie)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the index locations, utl error: %d.", iError);
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyAutoDiscoverIndices;
            }
        }


        /* Loop over each index */
        for ( pucIndicesPtr = s_strtok_r(pucIndices, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucIndicesStrtokPtr);
                pucIndicesPtr != NULL;
                pucIndicesPtr = s_strtok_r(NULL, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucIndicesStrtokPtr) ) {
        
            unsigned char   pucIndexSegments[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned char   *pucIndexSegmentsStrtokPtr = NULL;
            unsigned char   *pucIndexSegmentPtr = NULL;

            void            *pvIndexUrlsTriePtr = NULL;

            void            *pvIndexLocationStringBuffer = NULL;
            unsigned char   *pucIndexLocationPtr = NULL;

            
            /* Get the index segments for this index */
            if ( (iError = iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX, pucIndicesPtr, pucIndexSegments, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index segments from the gateway configuration, config key: '%s', config value: '%s', utl error: %d.", GTWY_CONFIG_INDEX, pucIndicesPtr, iError); 
                iError = SPI_MiscError;
                goto bailFromiGtwyAutoDiscoverIndices;
            }

            
            /* Get the index hosts for this index and scan them */
            if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_HOSTS, pucIndicesPtr, pucIndexHosts, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

                iUtlLogInfo(UTL_LOG_CONTEXT, "Scanning index hosts: '%s', for index segments: '%s', for index: '%s'.", pucIndexHosts, pucIndexSegments, pucIndicesPtr);

                /* Scan the index hosts for this index */
                if ( (iError = iGtwyAutoDiscoverIndicesScanHosts(pggGtwyGateway, pucIndexHosts, &pvUtlIndexHostsIndexLocationsTrie)) != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to scan index hosts: '%s', for index segments: '%s', for index: '%s', spi error: %d.", 
                            pucIndexHosts, pucIndexSegments, pucIndicesPtr, iError);
                    goto bailFromiGtwyAutoDiscoverIndices;
                }
            }
        

            /* Check the index host index urls trie if it is specified */
            if ( pvUtlIndexHostsIndexLocationsTrie != NULL ) {
                pvIndexUrlsTriePtr = pvUtlIndexHostsIndexLocationsTrie;
            }
            /* Otherwise check the index urls trie if it is specified */
            else if ( pvUtlHostsIndexLocationsTrie != NULL ) {
                pvIndexUrlsTriePtr = pvUtlHostsIndexLocationsTrie;
            }
        
            /* Look up the index segment in the index urls trie to see if there are any urls for it */
            if ( pvIndexUrlsTriePtr != NULL ) {
    
                /* Allocate the index location string buffer */
                if ( (iError = iUtlStringBufferCreate(&pvIndexLocationStringBuffer)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index location string buffer, utl error: %d.", iError);
                    iError = SPI_MiscError;
                    goto bailFromiGtwyAutoDiscoverIndices;
                }    

                /* Loop over each index segment */
                for ( pucIndexSegmentPtr = s_strtok_r(pucIndexSegments, GTWY_INDEX_LOCATION_SEGMENT_SEPARATORS, (char **)&pucIndexSegmentsStrtokPtr);
                        pucIndexSegmentPtr != NULL;
                        pucIndexSegmentPtr = s_strtok_r(NULL, GTWY_INDEX_LOCATION_SEGMENT_SEPARATORS, (char **)&pucIndexSegmentsStrtokPtr) ) {
                    
                    /* See if this index segment name is a regex, if it is we need to expand it */
                    if ( (s_strncmp(pucIndexSegmentPtr, GTWY_INDEX_SEGMENT_REGEX_START, s_strlen(GTWY_INDEX_SEGMENT_REGEX_START)) == 0) &&
                            (s_strcmp(pucIndexSegmentPtr + s_strlen(pucIndexSegmentPtr) - s_strlen(GTWY_INDEX_SEGMENT_REGEX_END), GTWY_INDEX_SEGMENT_REGEX_END) == 0) ) {
    
                        unsigned char   pucRegex[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
                        int             iStatus = 0;

                        /* Extract the regex */
                        s_strnncpy(pucRegex, pucIndexSegmentPtr + s_strlen(GTWY_INDEX_SEGMENT_REGEX_START), s_strlen(pucIndexSegmentPtr) - s_strlen(GTWY_INDEX_SEGMENT_REGEX_END));
            
                        /* Allocate the buffer for the regex structure */
                        if ( (prRegex = (regex_t *)s_malloc((size_t)sizeof(regex_t))) == NULL ) {
                            iError = SPI_MemError;
                            goto bailFromiGtwyAutoDiscoverIndices;
                        }
            
                        /* Create the regex structure */
                        if ( (iStatus = s_regcomp(prRegex, pucRegex, REG_EXTENDED | REG_NOSUB)) != 0 ) {
                            iError = SPI_MiscError;
                            goto bailFromiGtwyAutoDiscoverIndices;
                        }
                    
                        /* Loop over the keys in the trie, matching them to the regex, appending the matching index urls to the index location string */
                        if ( (iError = iUtlTrieLoop(pvIndexUrlsTriePtr, NULL, (int (*)())iGtwyAutoDiscoverIndicesRegexMatchCallBackFunction, pvIndexLocationStringBuffer, prRegex)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the index urls trie, utl error: %d.", iError);
                            iError = SPI_MiscError;
                            goto bailFromiGtwyAutoDiscoverIndices;
                        }

                        /* Free the regex */
                        s_regfree(prRegex);
                        s_free(prRegex);
                    }
                    else {

                        unsigned char   **ppucIndexUrl = NULL;
            
                        /* Look up the index url for this index segment */
                        if ( iUtlTrieLookup(pvIndexUrlsTriePtr, pucIndexSegmentPtr, (void *)&ppucIndexUrl) == UTL_NoError ) {
                
                            unsigned char   *pucIndexUrl = (unsigned char *)*ppucIndexUrl;
                
                            /* Got a index url so we add it to the index location string */
                            if ( bUtlStringsIsStringNULL(pucIndexUrl) == false ) {
        
                                size_t  zIndexLocationStringLength = 0;
        
/*                                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAutoDiscoverIndices - pucIndexSegmentPtr: '%s' => pucIndexUrl: '%s'.",  */
/*                                         pucIndexSegmentPtr, pucIndexUrl); */

                                /* Get the length of the index location string */
                                iUtlStringBufferGetLength(pvIndexLocationStringBuffer, &zIndexLocationStringLength);
                                
                                /* Append the mirror separator to the data, if there is already data there */
                                if ( zIndexLocationStringLength > 0 ) {
                                    iUtlStringBufferAppend(pvIndexLocationStringBuffer, GTWY_INDEX_LOCATION_SEGMENT_SEPARATORS);
                                }
                                
                                /* Append the index url */
                                iUtlStringBufferAppend(pvIndexLocationStringBuffer, pucIndexUrl);
                            }
                        }
                    }
                }
                
    
                /* Get the string from the index location string buffer, setting the search copy */
                iUtlStringBufferGetString(pvIndexLocationStringBuffer, &pucIndexLocationPtr);
        
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAutoDiscoverIndices - pucIndicesPtr: '%s' => pucIndexLocationPtr: '%s'.",  */
/*                         pucIndicesPtr, pucIndexLocationPtr); */
    
                /* Add the index name if there was no index name to scan for or if the index names match */
                if ( bUtlStringsIsStringNULL(pucIndexLocationPtr) == false ) {
                
                    unsigned char   **ppucIndexLocationPtr = NULL;
                

                    /* Look up the index name to see if there is already a location for it */
                    if ( (iError = iUtlTrieLookup(pvUtlIndexLocationsTrie, pucIndicesPtr, (void *)&ppucIndexLocationPtr)) == UTL_NoError ) {
                        
                        iUtlLogError(UTL_LOG_CONTEXT, "Duplicate index: '%s', in the index locations trie, existing location: '%s', new location: '%s'.", 
                                pucIndicesPtr, (unsigned char *)*ppucIndexLocationPtr, pucIndexLocationPtr);
                        
                        /* Replace the index location in the trie */
                        s_free(*ppucIndexLocationPtr);
                        *ppucIndexLocationPtr = pucIndexLocationPtr;
                    }
                    else {
                            
                        /* Add the index location to the trie */
                        if ( (iError = iUtlTrieAdd(pvUtlIndexLocationsTrie, pucIndicesPtr, (void ***)&ppucIndexLocationPtr)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the index locations trie, index: '%s', index location: '%s', utl error: %d.", 
                                    pucIndicesPtr, pucIndexLocationPtr, iError);
                            iError = SPI_MiscError;
                            goto bailFromiGtwyAutoDiscoverIndices;
                        }
                        
                        /* Set the index location in the trie */
                        *ppucIndexLocationPtr = pucIndexLocationPtr;
                    }
                }
    
    
                /* Free the index location string buffer, note that we don't release 
                ** the string as it was place in the index locations trie 
                */
                iUtlStringBufferFree(pvIndexLocationStringBuffer, false);
                pvIndexLocationStringBuffer = NULL;
            }

            
            /* Free the index hosts trie, freeing the datum, 
            ** and NULLing out the pointer to prevent a double free
            */
            iUtlTrieFree(pvUtlIndexHostsIndexLocationsTrie, true);
            pvUtlIndexHostsIndexLocationsTrie = NULL;
        }


        /* We need to null out the hosts index urls trie of it is the same as the index urls trie 
        ** so that it does not get freed (the index urls trie was handed over as opposed to created)
        */
        if ( pvUtlIndexLocationsTrie == pvUtlHostsIndexLocationsTrie ) {
            pvUtlHostsIndexLocationsTrie = NULL;
        }
    }
    else {
        /* Hand over the hosts index urls trie to the index urls trie and NULL it so that
        ** it does not get freed 
        */
        pvUtlIndexLocationsTrie = pvUtlHostsIndexLocationsTrie;
        pvUtlHostsIndexLocationsTrie = NULL;
    }



    /* Report on autodiscovered index */
    if ( pvUtlIndexLocationsTrie != NULL ) {
        
        /* Get the number of unique entries in the trie */
        if ( (iError = iUtlTrieGetEntryCount(pvUtlIndexLocationsTrie, &uiUniqueEntriesCount)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the number of unique entries from the index locations trie, utl error: %d.", iError);
            iError = SPI_MiscError;
            goto bailFromiGtwyAutoDiscoverIndices;
        }
    
        /* Log the contents of the trie */
        if ( uiUniqueEntriesCount > 0 ) {
            if ( (iError = iUtlTrieLoop(pvUtlIndexLocationsTrie, NULL, (int (*)())iGtwyAutoDiscoverIndicesPrintCallBackFunction)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over then entries in the index locations trie, utl error: %d.", iError);
                iError = SPI_MiscError;
                goto bailFromiGtwyAutoDiscoverIndices;
            }
        }
        else {
            iUtlLogInfo(UTL_LOG_CONTEXT, "No index were autodiscovered.");
        }
    }


    /* Set the gateway index urls trie pointer */
    pggGtwyGateway->pvUtlGatewayIndexLocationsTrie = pvUtlIndexLocationsTrie;



    /* Bail label */
    bailFromiGtwyAutoDiscoverIndices:


    /* Free allocations */
    iUtlTrieFree(pvUtlHostsIndexLocationsTrie, true);
    pvUtlHostsIndexLocationsTrie = NULL;

    iUtlTrieFree(pvUtlIndexHostsIndexLocationsTrie, true);
    pvUtlIndexHostsIndexLocationsTrie = NULL;

    /* Free the regex */
    if ( prRegex != NULL ) {
        s_regfree(prRegex);
        s_free(prRegex);
    }

    /* Handle the error */
    if ( iError != SPI_NoError ) {

        /* Free the index urls trie if it was created */
        if ( pvUtlIndexLocationsTrie != pvUtlHostsIndexLocationsTrie ) {
            iUtlTrieFree(pvUtlIndexLocationsTrie, true);
            pvUtlIndexLocationsTrie = NULL;
        }
        
        /* Clear the gateway index urls trie pointer */
        pggGtwyGateway->pvUtlGatewayIndexLocationsTrie = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyAutoDiscoverIndicesRegexMatchCallBackFunction()

    Purpose:    Parameters: pucKey      key
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, non-0 on error
*/
static int iGtwyAutoDiscoverIndicesRegexMatchCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list     ap_;
    void        *pvIndexLocationStringBuffer = NULL;
    regex_t     *prRegex = NULL;
    int         iStatus = 0;
    size_t      zIndexLocationStringLength = 0;


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    pvIndexLocationStringBuffer = (void *)va_arg(ap_, void *);
    prRegex = (regex_t *)va_arg(ap_, regex_t *);
    va_end(ap_);


    ASSERT(pvIndexLocationStringBuffer != NULL);
    ASSERT(prRegex != NULL);


    /* Add the index url if the key (index name) matches the regex */
    if ( (iStatus = s_regexec(prRegex, pucKey, (size_t)0, NULL, 0)) == 0 ) {
    
        unsigned char   *pucIndexUrl = (unsigned char *)pvData;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAutoDiscoverIndicesRegexMatchCallBackFunction - pucKey: '%s' => pucIndexUrl: '%s'.", */
/*                 pucKey, pucIndexUrl); */

        /* Get the length of the index location string */
        iUtlStringBufferGetLength(pvIndexLocationStringBuffer, &zIndexLocationStringLength);
        
        /* Append the mirror separator to the data, if there is already data there */
        if ( zIndexLocationStringLength > 0 ) {
            iUtlStringBufferAppend(pvIndexLocationStringBuffer, GTWY_INDEX_LOCATION_SEGMENT_SEPARATORS);
        }
        
        /* Append the index url */
        iUtlStringBufferAppend(pvIndexLocationStringBuffer, pucIndexUrl);
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyAutoDiscoverIndicesPrintCallBackFunction()

    Purpose:    Parameters: pucKey      key
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, non-0 on error
*/
static int iGtwyAutoDiscoverIndicesPrintCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    iUtlLogInfo(UTL_LOG_CONTEXT, "Autodiscovered index: '%s', index location: '%s'.", pucKey, (unsigned char *)pvData);


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyAutoDiscoverIndicesScanHosts()

    Purpose:    This function auto-discovers index by scanning hosts.

    Parameters: pggGtwyGateway          gateway structure
                pucIndexHosts           comma delimited hosts in 'host/mask:port' format
                ppvIndexLocationsTrie   return pointer for the index locations trie

    Globals:    none

    Returns:    SPI Error Code

*/
static int iGtwyAutoDiscoverIndicesScanHosts
(
    struct gtwyGateway *pggGtwyGateway,
    unsigned char *pucIndexHosts,
    void **ppvIndexLocationsTrie
)
{

    int                 iError = SPI_NoError;

    void                *pvUtlIndexLocationsTrie = NULL;
    unsigned char       *pucIndexHostsCopy = NULL;
    unsigned char       *pucIndexHostPtr = NULL;
    unsigned char       *pucIndexHostsStrtokPtr = NULL;

    unsigned char       pucScanfFormatHostMaskPort[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char       pucScanfFormatHostMask[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char       pucScanfFormatHostPort[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char       pucScanfFormatHost[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};

#if defined(SIGPIPE)
/*     struct sigaction    *psaSigAction = NULL; */
#endif


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAutoDiscoverIndicesScanHosts [%s].", pucIndexHosts); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexHosts) == false);
    ASSERT(ppvIndexLocationsTrie != NULL);


    /* Prepare the sscanf formats, format being 'host/mask:port' or 'host/mask' or 'host:port' or 'host' */
    snprintf(pucScanfFormatHostMaskPort, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^/]/%%d:%%d", UTL_CONFIG_VALUE_MAXIMUM_LENGTH);
    snprintf(pucScanfFormatHostMask, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^/]/%%d", UTL_CONFIG_VALUE_MAXIMUM_LENGTH);
    snprintf(pucScanfFormatHostPort, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]:%%d", UTL_CONFIG_VALUE_MAXIMUM_LENGTH);
    snprintf(pucScanfFormatHost, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d", UTL_CONFIG_VALUE_MAXIMUM_LENGTH);


#if defined(SIGPIPE)
    /* We need to swap out the current SIGPIPE handler and ignore SIGPIPE signals as sometimes
    ** opening a socket in non-blocking mode to a non-existent host will tell us it is ready for 
    ** sending when it fact it is not, and a SIGPIPE signal will be generated, which we need to
    ** ignore. The current SIGPIPE handler will be restored at the end of this process.
    */
/*     if ( (iError = iUtlSignalsIgnoreHandler(SIGPIPE, &psaSigAction)) != UTL_NoError ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the SIGPIPE signal handler to 'ignore', utl error: %d.", iError); */
/*         iError = SPI_MiscError; */
/*         goto bailFromiGtwyAutoDiscoverIndicesScanHosts; */
/*     } */
#endif


    /* Create the index locations trie, this is used to store the index locations as they are built up */
    if ( (iError = iUtlTrieCreate(&pvUtlIndexLocationsTrie)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the index locations, utl error: %d.", iError);
        iError = SPI_MiscError;
        goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
    }


    /* Make a copy of the index hosts as s_strtok_r() destroys them */
    if ( (pucIndexHostsCopy = (unsigned char *)s_strdup(pucIndexHosts)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
    }

    /* Loop over each index host */
    for ( pucIndexHostPtr = s_strtok_r(pucIndexHostsCopy, GTWY_HOST_SEPARATORS, (char **)&pucIndexHostsStrtokPtr);
            pucIndexHostPtr != NULL;
            pucIndexHostPtr = s_strtok_r(NULL, GTWY_HOST_SEPARATORS, (char **)&pucIndexHostsStrtokPtr) ) {

        unsigned char   pucHostAddress[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
        int             iHostMaskBits = 0;
        int             iPort = 0;
        unsigned int    uiHostAddress = 0;
        unsigned int    uiHostRange = 4294967295U;
        unsigned int    uiHostMask = 4294967295U;
        unsigned int    uiI = 0;


        /* Parse out the index host into its component parts - 'host/mask:port' */
        if ( sscanf(pucIndexHostPtr, pucScanfFormatHostMaskPort, pucHostAddress, &iHostMaskBits, &iPort) == 3 ) {
            /* Complete host address */
            ;
        }
        /* 'host/mask:port' */
        else if ( sscanf(pucIndexHostPtr, pucScanfFormatHostMask, pucHostAddress, &iHostMaskBits) == 2 ) {
            /* Host address missing the port */
            iPort = LWPS_PROTOCOL_PORT_DEFAULT;
        }
        /* 'host:port' */
        else if ( sscanf(pucIndexHostPtr, pucScanfFormatHostPort, pucHostAddress, &iPort) == 2 ) {
            /* Host address missing the mask */
            iHostMaskBits = 32;
        }
        /* 'host' */
        else if ( sscanf(pucIndexHostPtr, pucScanfFormatHostPort, pucHostAddress) == 1 ) {
            /* Host address missing the mask and the port */
            iHostMaskBits = 32;
            iPort = LWPS_PROTOCOL_PORT_DEFAULT;
        }
        else {
            /* Invalid host address */
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid index host: '%s', expected a format of: 'host/mask:port' or 'host:port'.", pucIndexHostPtr);
            iError = SPI_MiscError;
            goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
        }
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pucIndexHostPtr: '%s', pucHostAddress: '%s', iHostMaskBits: %d, iPort: %d.",  */
/*                 pucIndexHostPtr, pucHostAddress, iHostMaskBits, iPort); */

        /* Check the host address */
        if ( bUtlStringsIsStringNULL(pucHostAddress) == true ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid host address: '%s', the address appears to be missing.", pucIndexHostPtr);
            iError = SPI_MiscError;
            goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
        }
        
        /* Check the host mask bits, should be in the range 0-32 */
        if ( (iHostMaskBits < 0) || (iHostMaskBits > 32) ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid host address: '%s', the mask should be in the range: 0-32.", pucIndexHostPtr);
            iError = SPI_MiscError;
            goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
        }
        
        /* Check the host port, should be in the range 0-... */
        if ( iPort < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid host address: '%s', the host port should be greater than: 0.", pucIndexHostPtr);
            iError = SPI_MiscError;
            goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
        }

        /* Set up the host range and mask */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iHostMaskBits: %d, uiHostRange: %u, uiHostMask: %u.", iHostMaskBits, uiHostRange, uiHostMask); */
        uiHostRange = (iHostMaskBits == 32) ? 0 : uiHostRange >> iHostMaskBits;
        uiHostMask <<= (32 - iHostMaskBits);
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iHostMaskBits: %d, uiHostRange: %u, uiHostMask: %u.", iHostMaskBits, uiHostRange, uiHostMask); */

        /* Get the numeric host address */
        if ( (iError = iUtlNetConvertStringHostAddress(pucHostAddress, &uiHostAddress)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get numeric host address for host: '%s', utl error: %d.", pucHostAddress, iError);
            iError = SPI_MiscError;
            goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
        }

        /* Clear out trailing bits that should not be in the numeric host address  */
        uiHostAddress &= uiHostMask;


        /* Loop over each host in the host range */
        for ( uiI = 0; uiI <= uiHostRange; uiI++ ) {
        
            unsigned int                uiByte = 0;
            unsigned int                uiFinalHostAddress = 0;
            void                        *pvUtlNet = NULL;
            void                        *pvLwps = NULL;
            int                         iErrorCode = SPI_NoError;
            unsigned char               *pucErrorString = NULL;
            struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
            struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;
            unsigned int                uiSpiServerIndexInfosLength = 0;
            unsigned int                uiJ = 0;

            
            /* Create the final host address, this is the address we check */
            uiFinalHostAddress = uiHostAddress + uiI;
            
            /* Skip .0, .1 and .255 hosts */
            uiByte = (uiFinalHostAddress & 0xFF);
            if ( (uiByte == 0) || (uiByte == 1) || (uiByte == 255) ) {
                iError = SPI_NoError;
                goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
            }

            /* Convert the final numeric host address to a host address string */
            if ( (iError = iUtlNetConvertNumericHostAddress(uiFinalHostAddress, pucHostAddress, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get host string address for host: %u, utl error: %d.", uiFinalHostAddress, iError);
                iError = SPI_MiscError;
                goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
            }
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucHostAddress: '%s', iPort: %d.", pucHostAddress, iPort); */
/* printf("pucHostAddress: '%s'.\n", pucHostAddress); */
        

            /* Open a connection to the host */
            if ( (iError = iUtlNetCreateClient(pggGtwyGateway->uiLwpsNetworkProtocolID, pucHostAddress, iPort, pggGtwyGateway->uiConnectionTimeOut, &pvUtlNet)) != UTL_NoError ) {
                iUtlLogDebug(UTL_LOG_CONTEXT, "Failed to open a net client connection, network protocol: %u, host: '%s', port: %d, utl error: %d", 
                        pggGtwyGateway->uiLwpsNetworkProtocolID, pucHostAddress, iPort, iError);
                iError = SPI_MiscError;
                goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
            }

            /* Create a LPWS handle */
            if ( (iError = iLwpsCreate(pvUtlNet, &pvLwps)) != LWPS_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create an lwps, utl error: %d.", iError);
                iError = SPI_MiscError;
                goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
            }


            /* Perform a LWPS init exchange before anything else happens */
            if ( pggGtwyGateway->bLwpsSendInit == true ) {

                /* Set the information timeout, an init is treated as information */
                if ( (iError = iUtlNetSetTimeOut(pvUtlNet, pggGtwyGateway->uiInformationTimeOut)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the net information timeout, utl error: %d.", iError);
                    iError = SPI_MiscError;
                    goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                }
    
                /* Send the init - note that we dont send a user name or a password */
                if (  (iError = iLwpsInitRequestHandle(pvLwps, NULL, NULL, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    if ( pggGtwyGateway->uiLwpsNetworkProtocolID == GTWY_PROTOCOL_TCP_ID ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps init request with: '%s', lwps error: %d.", pucHostAddress, iError);
                    }
                    iError = SPI_MiscError;
                    goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                }
            
                /* Check the error code */
                if ( iErrorCode != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to exchange lwps inits with: '%s', error code: %d, error text: '%s'.", 
                            pucHostAddress, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                    s_free(pucErrorString);
                    iError = iErrorCode;
                    goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                }
            }


            /* Set the information timeout */
            if ( (iError = iUtlNetSetTimeOut(pvUtlNet, pggGtwyGateway->uiInformationTimeOut)) != LWPS_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the net information timeout, utl error: %d.", iError);
                iError = SPI_MiscError;
                goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
            }

            /* Get the index list */
            if ( (iError = iLwpsServerIndexInfoRequestHandle(pvLwps, &pssiiSpiServerIndexInfos, &uiSpiServerIndexInfosLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                if ( pggGtwyGateway->uiLwpsNetworkProtocolID == GTWY_PROTOCOL_TCP_ID ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index information request with: '%s', lwps error: %d.", pucHostAddress, iError);
                }
                iError = SPI_MiscError;
                goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
            }
            else {
                /* Check the error code */
                if ( iErrorCode != SPI_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to retrieve server index information from: '%s', error code: %d, error text: '%s'.", 
                            pucHostAddress, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                    s_free(pucErrorString);
                    iError = iErrorCode;
                    goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                }
            }

/* printf("pucHostAddress: '%s', iPort: %d.\n", pucHostAddress, iPort); */

            /* Loop through the index list, adding each index to the trie (index/address:port tuple) */
            for ( uiJ = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos; uiJ < uiSpiServerIndexInfosLength; uiJ++, pssiiSpiServerIndexInfosPtr++ ) {
    
                unsigned char   **ppucIndexLocationPtr = NULL;
                unsigned char   *pucIndexLocation = NULL;


/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pssiiSpiServerIndexInfosPtr->pucIndexName: '%s'", pucUtlStringsGetPrintableString(pssiiSpiServerIndexInfosPtr->pucIndexName)); */

                /* Look up the index name to see if there already is some index location information for it */
                iError = iUtlTrieLookup(pvUtlIndexLocationsTrie, pssiiSpiServerIndexInfosPtr->pucName, (void *)&ppucIndexLocationPtr);
                
                /* We can only process if there was no error or if we did not find the key */
                if ( (iError == UTL_NoError) || (iError == UTL_TrieKeyNotFound) ) {
        
                    unsigned char   pucIndexLocationUrl[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
                    unsigned int    uiIndexLocationUrlLength = 0;
                    
                    /* Create the index location url */
                    snprintf(pucIndexLocationUrl, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1, "%s%s:%d/%s", LWPS_PROTOCOL_URL, pucHostAddress, iPort, pssiiSpiServerIndexInfosPtr->pucName);

                    /* Get the length of the index location url, note the space for the terminating NULL */
                    uiIndexLocationUrlLength = s_strlen(pucIndexLocationUrl);

                    /* The key was found, so we need to append the index url to the existing index location */
                    if ( iError == UTL_NoError ) {
                        
                        /* Dereference the index location for convenience */
                        pucIndexLocation = (unsigned char *)*ppucIndexLocationPtr;

                        /* Add the length of any data currently in the index location */
                        if ( bUtlStringsIsStringNULL(pucIndexLocation) == false ) {
                            uiIndexLocationUrlLength += s_strlen(pucIndexLocation);
                        }

                        /* Add the length of the mirror separator we are going to append */
                        uiIndexLocationUrlLength += s_strlen(GTWY_INDEX_LOCATION_MIRROR_SEPARATORS);

                        /* Reallocate a index location pointer, add space for the terminating NULL */
                        if ( (pucIndexLocation = (unsigned char *)s_realloc(pucIndexLocation, (size_t)(sizeof(unsigned char) * (uiIndexLocationUrlLength + 1)))) == NULL ) {
                            iError = SPI_MemError;
                            goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                        }
                    
                        /* Append the mirror separator to the index location */
                        s_strnncat(pucIndexLocation, GTWY_INDEX_LOCATION_MIRROR_SEPARATORS, uiIndexLocationUrlLength, uiIndexLocationUrlLength + 1);
                    }
                    /* The key was not found, so we need to allocate space for the index location */
                    else if ( iError == UTL_TrieKeyNotFound ) {
                        
                        /* Allocate a index location pointer, add space for the terminating NULL */
                        if ( (pucIndexLocation = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * (uiIndexLocationUrlLength + 1)))) == NULL ) {
                            iError = SPI_MemError;
                            goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                        }
                    }
                    else {
                        ASSERT(false);
                    }
                    
                    /* Append the index location url to the index location */
                    s_strnncat(pucIndexLocation, pucIndexLocationUrl, uiIndexLocationUrlLength, uiIndexLocationUrlLength + 1);
                }
                else {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index location for the index: '%s', from the index locations trie, utl error: %d.", 
                            pssiiSpiServerIndexInfosPtr->pucName, iError);
                    iError = SPI_MiscError;
                    goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                }


/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "'%s' => '%s'", pucUtlStringsGetPrintableString(pssiiSpiServerIndexInfosPtr->pucName), pucUtlStringsGetPrintableString(pucIndexLocation)); */

                /* Add/replace the index location in the trie */
                if ( (iError = iUtlTrieAdd(pvUtlIndexLocationsTrie, pssiiSpiServerIndexInfosPtr->pucName, (void ***)&ppucIndexLocationPtr)) == UTL_NoError ) {
                    *ppucIndexLocationPtr = pucIndexLocation;
                }
                else {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the index locations trie, index: '%s', index location: '%s', utl error: %d.", 
                            pssiiSpiServerIndexInfosPtr->pucName, ppucIndexLocationPtr, iError);
                    iError = SPI_MiscError;
                    goto bailFromiGtwyAutoDiscoverIndicesScanHostsScan;
                }
            }


            
            /* Bail label */
            bailFromiGtwyAutoDiscoverIndicesScanHostsScan:


            /* Free the error string */
            s_free(pucErrorString);

            /* Free the server index info */
            iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
            pssiiSpiServerIndexInfos = NULL;

            /* Free the lwps structure */
            iLwpsFree(pvLwps);
            pvLwps = NULL;
            
            /* Free the net structure */
            iUtlNetFree(pvUtlNet);
            pvUtlNet = NULL;
            
            /* Handle the error */
            if ( iError == SPI_MemError ) {
                goto bailFromiGtwyAutoDiscoverIndicesScanHosts;
            }
            
            /* Reset the error */
            iError = SPI_NoError;
        }
    }

    
    
    /* Bail label */
    bailFromiGtwyAutoDiscoverIndicesScanHosts:


#if defined(SIGPIPE)
    /* Restore the SIGPIPE handler */
/*     if ( (iError = iUtlSignalsRestoreHandler(SIGPIPE, psaSigAction)) != UTL_NoError ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Failed to restore the SIGPIPE signal handler: %d.", iError); */
/*         iError = SPI_InitializeServerFailed; */
/*     } */
    
    /* Free the action structure */
/*     s_free(psaSigAction); */
#endif

    /* Free the index hosts copy */
    s_free(pucIndexHostsCopy);

    /* Handle the error */
    if ( iError == SPI_NoError ) {
        
        /* Set the return pointer */
        *ppvIndexLocationsTrie = pvUtlIndexLocationsTrie;
    }
    else {
    
        /* Free the trie, freeing the datum */
        iUtlTrieFree(pvUtlIndexLocationsTrie, true);
        pvUtlIndexLocationsTrie = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ===========================================
** ===  Gateway Index Set Up Functions  ===
** ===========================================
*/


/*

    Function:   iGtwyInitializeGatewayIndex()

    Purpose:    Initialize a gateway index structure. This allocates space for the
                gateway index structure and populates the various fields.

    Parameters: pggGtwyGateway      gateway structure
                pucIndexName        name of the index to initialize
                ppgdGtwyIndex       return pointer for the gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyInitializeGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    unsigned char *pucIndexName,
    struct gtwyIndex **ppgdGtwyIndex
)
{

    unsigned int            iError = SPI_NoError;
    struct gtwyIndex        *pgdGtwyIndex = NULL;
    unsigned char           pucScanfFormatLocation[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucScanfFormatHost[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSymbolData[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           *pucIndexLocation = NULL;
    unsigned char           *pucIndexLocationPtr = NULL;
    unsigned char           *pucSegmentPtr = NULL;
    unsigned char           *pucSegmentStrtokPtr = NULL;

    unsigned int            uiIndexOrigin = GTWY_INDEX_ORIGIN_UNKNOWN_ID;

    struct gtwySegment      *pgsGtwySegments = NULL;
    struct gtwySegment      *pgsGtwySegmentsPtr = NULL;
    unsigned int            uiGtwySegmentsLength = 0;
    
    struct gtwyIndexSort    *psdsGtwyIndexSorts = NULL;
    struct gtwyIndexSort    *psdsGtwyIndexSortsPtr = NULL;
    unsigned int            uiGtwyIndexSortsLength = 0;

    unsigned int            uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyInitializeGatewayIndex [%s]", pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(ppgdGtwyIndex != NULL);


    /* A index name can be defined in one of three ways:
    **
    **    lwps://linux.fsconsult.com/index        - lwps index to be parsed from the URL
    **
    **    index                                - gateway index defined in the configuration file
    **
    **    index                                - gateway index defined in the index urls trie (autodiscovery)
    */

    /* Check if the index is defined with a URL or with a name */
    if ( s_strncasecmp(pucIndexName, LWPS_PROTOCOL_URL, s_strlen(LWPS_PROTOCOL_URL)) == 0 ) {
        
        /* Set the index location pointer */
        pucIndexLocationPtr = pucIndexName;

        /* This is a 'from url' index for information purposes */
        uiIndexOrigin = GTWY_INDEX_ORIGIN_FROM_URL_ID;
    }
    else {

        unsigned char    **ppucIndexLocationPtr = NULL;

        /* Look up the index location symbol in the configuration file */
        if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_LOCATION, pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

            /* We found the index location so we set the index location pointer */
            pucIndexLocationPtr = pucSymbolData;

            /* This is a 'from configuration' index for information purposes */
            uiIndexOrigin = GTWY_INDEX_ORIGIN_FROM_CONFIG_ID;
        }
        /* Look up the index location symbol in the gateway index location trie */
        else if ( iUtlTrieLookup(pggGtwyGateway->pvUtlGatewayIndexLocationsTrie, pucIndexName, (void *)&ppucIndexLocationPtr) == UTL_NoError ) {

            /* We found the index location so we set the index location pointer */
            pucIndexLocationPtr = (unsigned char *)*ppucIndexLocationPtr;

            /* This is a 'from configuration' index for information purposes */
            uiIndexOrigin = GTWY_INDEX_ORIGIN_FROM_CONFIG_ID;
        }
    }

    /* Check that the index location is specified */
    if ( pucIndexLocationPtr == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to find the index location, index: '%s'.", pucIndexName);
        iError = SPI_InitializeServerFailed;
        goto bailFromiGtwyInitializeGatewayIndex;
    }


    /* Duplicate the index location - it gets destroyed by s_strtok_r() */ 
    if ( (pucIndexLocation = (unsigned char *)s_strdup(pucIndexLocationPtr)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyInitializeGatewayIndex;
    }


    /* Create the scan formats, 'lwps://hostname:port/indexName', 'hostname:port' */
    snprintf(pucScanfFormatLocation, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]://%%%d[^/]/%%%ds", UTL_CONFIG_VALUE_MAXIMUM_LENGTH, UTL_CONFIG_VALUE_MAXIMUM_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH);
    snprintf(pucScanfFormatHost, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]:%%d", MAXHOSTNAMELEN);


    /* Loop while there are segment entries to process */
    for ( pucSegmentPtr = s_strtok_r(pucIndexLocation, GTWY_INDEX_LOCATION_SEGMENT_SEPARATORS, (char **)&pucSegmentStrtokPtr);
            pucSegmentPtr != NULL;
            pucSegmentPtr = s_strtok_r(NULL, GTWY_INDEX_LOCATION_SEGMENT_SEPARATORS, (char **)&pucSegmentStrtokPtr) ) {

        unsigned char   *pucMirrorPtr = NULL;
        unsigned char   *pucMirrorStrtokPtr = NULL;

        /* Add a new entry to the gateway segments array */
        if ( (pgsGtwySegmentsPtr = (struct gtwySegment *)s_realloc(pgsGtwySegments, (size_t)(sizeof(struct gtwySegment) * (uiGtwySegmentsLength + 1)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyInitializeGatewayIndex;
        }
    
        /* Hand over the pointer */
        pgsGtwySegments = pgsGtwySegmentsPtr;

        /* Dereference the gateway segments pointer for ease of use */
        pgsGtwySegmentsPtr = pgsGtwySegments + uiGtwySegmentsLength;

        /* Increment the number of gateway segments */
        uiGtwySegmentsLength++;

        /* Clear the new area */
        s_memset(pgsGtwySegmentsPtr, '\0', sizeof(struct gtwySegment));
        
        
        /* Loop while there are mirror entries to process */
        for ( pucMirrorPtr = s_strtok_r(pucSegmentPtr, GTWY_INDEX_LOCATION_MIRROR_SEPARATORS, (char **)&pucMirrorStrtokPtr);
                pucMirrorPtr != NULL;
                pucMirrorPtr = s_strtok_r(NULL, GTWY_INDEX_LOCATION_MIRROR_SEPARATORS, (char **)&pucMirrorStrtokPtr) ) {

            struct gtwyMirror   *pgmGtwyMirrorsPtr = NULL;
            unsigned char       pucProtocol[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned char       pucHostAddress[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    
            /* Add a new entry to the gateway mirrors list */
            if ( (pgmGtwyMirrorsPtr = (struct gtwyMirror *)s_realloc(pgsGtwySegmentsPtr->pgmGtwyMirrors, 
                    (size_t)(sizeof(struct gtwyMirror) * (pgsGtwySegmentsPtr->uiGtwyMirrorsLength + 1)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwyInitializeGatewayIndex;
            }
    
            /* Hand over the pointer */
            pgsGtwySegmentsPtr->pgmGtwyMirrors = pgmGtwyMirrorsPtr;
    
            /* Dereference the gateway mirrors pointer for ease of use */
            pgmGtwyMirrorsPtr = pgsGtwySegmentsPtr->pgmGtwyMirrors + pgsGtwySegmentsPtr->uiGtwyMirrorsLength;
    
            /* Increment the number of gateway mirrors */
            pgsGtwySegmentsPtr->uiGtwyMirrorsLength++;

            /* Clear the new area */
            s_memset(pgmGtwyMirrorsPtr, '\0', sizeof(struct gtwyMirror));
    
            /* Intitialize the variables */
            pgmGtwyMirrorsPtr->pucIndexName[0] = '\0';
            pgmGtwyMirrorsPtr->pucHostName[0] = '\0';
            pgmGtwyMirrorsPtr->iPort = LWPS_PROTOCOL_PORT_DEFAULT;
            pgmGtwyMirrorsPtr->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_DISCONNECTED;
            pgmGtwyMirrorsPtr->uiPriority = GTWY_MIRROR_PRIORITY_DEFAULT;
            
            /* Copy the mirror to the canonical index name */
            s_strnncpy(pgmGtwyMirrorsPtr->pucCanonicalIndexName, pucMirrorPtr, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);
    
            /* Parse out the canonical index name into its component parts */
            if ( sscanf(pgmGtwyMirrorsPtr->pucCanonicalIndexName, pucScanfFormatLocation,  pucProtocol, pucHostAddress, pgmGtwyMirrorsPtr->pucIndexName) != 3 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid index location: '%s', expected a format of: 'protocol://hostName:port/indexName', index: '%s'.",
                        pgmGtwyMirrorsPtr->pucCanonicalIndexName, pucIndexName);
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyInitializeGatewayIndex;
            }
    
            /* Parse out the host address into its component parts */
            if ( sscanf(pucHostAddress, pucScanfFormatHost, pgmGtwyMirrorsPtr->pucHostName, &pgmGtwyMirrorsPtr->iPort) < 1 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid host address: '%s', expected a format of: 'hostName:port', index: '%s'.",
                        pucHostAddress, pucIndexName);
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyInitializeGatewayIndex;
            }
    
            /* Check that all the required fields are defined */
            if ( (bUtlStringsIsStringNULL(pgmGtwyMirrorsPtr->pucIndexName) == true) || (bUtlStringsIsStringNULL(pgmGtwyMirrorsPtr->pucHostName) == true) ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid index location: '%s', expected a format of: 'protocol://hostName:port/indexName', index: '%s'.",
                        pgmGtwyMirrorsPtr->pucCanonicalIndexName, pucIndexName);
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyInitializeGatewayIndex;
            }
    
            /* Check the host port, should be in the range 0-... */
            if ( pgmGtwyMirrorsPtr->iPort < 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid index location: '%s', the host port should be greater than: 0, index: '%s'.",
                        pgmGtwyMirrorsPtr->pucCanonicalIndexName, pucIndexName);
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyInitializeGatewayIndex;
            }
        }
    }



    /* Get the index sort orders */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_SORT_ORDERS, pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        unsigned char   *pucSymbolPtr = NULL;
        unsigned char   pucScanfFormatSort[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
        unsigned char   pucScanfFormatSortAbr[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
        unsigned char   *pucIndexSortOrderPtr = NULL;
        unsigned char   *pucIndexSortOrdersStrtokPtr = NULL;
        
        /* Convert the symbol to lowercase - do it ourselves because we 
        ** dont want to link against language support libraries
        */
/*         pucLngCaseConvertStringToLowerCase(pucSymbolData); */
        for ( pucSymbolPtr = pucSymbolData; *pucSymbolPtr != '\0'; pucSymbolPtr++ ) { 
            *pucSymbolPtr = tolower(*pucSymbolPtr);
        }

        /* Create the scan formats, to scan for strings such as: '{sort:relevance:desc}' or '{sort:date:desc}' */
        snprintf(pucScanfFormatSort, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%s%%%d[^:]:%%%d[^}]}", GTWY_MODIFIER_SORT_STRING, 
                SPI_FIELD_NAME_MAXIMUM_LENGTH, SPI_FIELD_NAME_MAXIMUM_LENGTH);
        snprintf(pucScanfFormatSortAbr, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%s%%%d[^:]:%%%d[^}]}", GTWY_MODIFIER_SORT_ABR_STRING, 
                SPI_FIELD_NAME_MAXIMUM_LENGTH, SPI_FIELD_NAME_MAXIMUM_LENGTH);

        /* Loop adding the index sort orders */
        for ( pucIndexSortOrderPtr = s_strtok_r(pucSymbolData, GTWY_INDEX_SORT_ORDERS_SEPARATORS, (char **)&pucIndexSortOrdersStrtokPtr);
                pucIndexSortOrderPtr != NULL;
                pucIndexSortOrderPtr = s_strtok_r(NULL, GTWY_INDEX_SORT_ORDERS_SEPARATORS, (char **)&pucIndexSortOrdersStrtokPtr) ) {

            unsigned char   pucSortFieldName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned char   pucSortOrderName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned int    uiSortOrderID = GTWY_INDEX_SORT_ORDER_INVALID_ID;

            /* Scan the index sort order */
            if ( sscanf(pucIndexSortOrderPtr, pucScanfFormatSort, pucSortFieldName, pucSortOrderName) == 2 ) {
                ;
            }
            else if ( sscanf(pucIndexSortOrderPtr, pucScanfFormatSortAbr, pucSortFieldName, pucSortOrderName) == 2 ) {
                ;
            }
            else {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse the sort order symbol: '%s', index: '%s'.", pucIndexSortOrderPtr, pucIndexName);
                iError = SPI_InitializeServerFailed;
                goto bailFromiGtwyInitializeGatewayIndex;
            }
            
            /* Normalize the field name if it is abbreviated */
            if ( s_strcasecmp(pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_ABR_RELEVANCE) == 0 ) {
                s_strnncpy(pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_RELEVANCE, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
            }
            else if ( s_strcasecmp(pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_ABR_RANK) == 0 ) {
                s_strnncpy(pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_RANK, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
            }
            else if ( s_strcasecmp(pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_ABR_DATE) == 0 ) {
                s_strnncpy(pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_DATE, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
            }

            /* Get the sort order ID */
            if ( (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_ASC) == 0) || (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_ABR_ASC) == 0) ) {
                uiSortOrderID = GTWY_INDEX_SORT_ORDER_ASC_ID;
            }
            else if ( (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_DESC) == 0) || (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_ABR_DESC) == 0) ) {
                uiSortOrderID = GTWY_INDEX_SORT_ORDER_DESC_ID;
            }
            
            /* Add an entry to the gateway index sorts array if this is a valid sort field name and sort order ID */
            if ( (bUtlStringsIsStringNULL(pucSortFieldName) == false) && (uiSortOrderID != GTWY_INDEX_SORT_ORDER_INVALID_ID) ) {

                /* Extend the gateway index sorts array */
                if ( (psdsGtwyIndexSortsPtr = (struct gtwyIndexSort *)s_realloc(psdsGtwyIndexSorts, (size_t)(sizeof(struct gtwyIndexSort) * (uiGtwyIndexSortsLength + 1)))) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiGtwyInitializeGatewayIndex;
                }
                
                /* Hand over the pointer */
                psdsGtwyIndexSorts = psdsGtwyIndexSortsPtr;
                
                /* Dereference the entry in the array */
                psdsGtwyIndexSortsPtr = psdsGtwyIndexSorts + uiGtwyIndexSortsLength;

                /* Copy the sort field name */
                s_strnncpy(psdsGtwyIndexSortsPtr->pucSortFieldName, pucSortFieldName, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
                
                /* Set the sort order ID */
                psdsGtwyIndexSortsPtr->uiSortOrderID = uiSortOrderID;

                /* Increment the number of entries */
                uiGtwyIndexSortsLength++;
            }
            else {
                /* Invalid sort order, so we warn the user */
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid index sort order symbol: '%s', index: '%s'.", pucIndexSortOrderPtr, pucIndexName);
            }
        }
    }



    /* Create the index gateway structure */
    if ( (pgdGtwyIndex = (struct gtwyIndex *)s_malloc((size_t)(sizeof(struct gtwyIndex)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyInitializeGatewayIndex;
    }

    /* Store the name of the gateway index */
    s_strnncpy(pgdGtwyIndex->pucIndexName, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);

    /* Set the protocol (currently unused) */
    pgdGtwyIndex->uiIndexProtocol = 0;

    /* Set the origin */
    pgdGtwyIndex->uiIndexOrigin = uiIndexOrigin;


    /* Clear the flags */
    vGtwyClearFlags(pgdGtwyIndex->uiFlags);

    /* Set the connection policy flag */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_CONNECTION_POLICY, pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_CONNECTION_POLICY_LAZY, s_strlen(GTWY_INDEX_CONNECTION_POLICY_LAZY)) == 0 ) {
            vGtwySetLazyConnectionPolicy(pgdGtwyIndex->uiFlags);
        }
        else if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_CONNECTION_POLICY_STRICT, s_strlen(GTWY_INDEX_CONNECTION_POLICY_STRICT)) == 0 ) {
            vGtwySetStrictConnectionPolicy(pgdGtwyIndex->uiFlags);
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid index connection policy found in the gateway configuration file, config key: '%s', config appender: '%s', config value: '%s', index: '%s', using default: '%s'.", 
                    GTWY_CONFIG_INDEX_CONNECTION_POLICY, pucIndexName, pucSymbolData, pucIndexName, GTWY_INDEX_CONNECTION_POLICY_STRICT);
            vGtwySetStrictConnectionPolicy(pgdGtwyIndex->uiFlags);
        }
    }

    /* Set the connection error flag */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_CONNECTION_ERROR, pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_ERROR_IGNORE, s_strlen(GTWY_INDEX_ERROR_IGNORE)) == 0 ) {
            vGtwySetIgnoreConnectionError(pgdGtwyIndex->uiFlags);
        }
        else if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_ERROR_FAIL, s_strlen(GTWY_INDEX_ERROR_FAIL)) == 0 ) {
            vGtwySetFailConnectionError(pgdGtwyIndex->uiFlags);
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid connection error policy found in the gateway configuration file, config key: '%s', config appender: '%s', config value: '%s', index: '%s', using default: '%s'.", 
                    GTWY_CONFIG_INDEX_CONNECTION_ERROR, pucIndexName, pucSymbolData, pucIndexName, GTWY_INDEX_ERROR_FAIL);
            vGtwySetFailConnectionError(pgdGtwyIndex->uiFlags);
        }
    }

    /* Set the search error flag */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_SEARCH_ERROR, pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_ERROR_IGNORE, s_strlen(GTWY_INDEX_ERROR_IGNORE)) == 0 ) {
            vGtwySetIgnoreSearchError(pgdGtwyIndex->uiFlags);
        }
        else if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_ERROR_FAIL, s_strlen(GTWY_INDEX_ERROR_FAIL)) == 0 ) {
            vGtwySetFailSearchError(pgdGtwyIndex->uiFlags);
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search error policy found in the gateway configuration file, config key: '%s', config appender: '%s', config value: '%s', index: '%s', using default: '%s'.", 
                    GTWY_CONFIG_INDEX_SEARCH_ERROR, pucIndexName, pucSymbolData, pucIndexName, GTWY_INDEX_ERROR_FAIL);
            vGtwySetFailSearchError(pgdGtwyIndex->uiFlags);
        }
    }

    /* Set the retrieval error flag */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_RETRIEVAL_ERROR, pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_ERROR_IGNORE, s_strlen(GTWY_INDEX_ERROR_IGNORE)) == 0 ) {
            vGtwySetIgnoreRetrievalError(pgdGtwyIndex->uiFlags);
        }
        else if ( s_strncasecmp(pucSymbolData, GTWY_INDEX_ERROR_FAIL, s_strlen(GTWY_INDEX_ERROR_FAIL)) == 0 ) {
            vGtwySetFailRetrievalError(pgdGtwyIndex->uiFlags);
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid retrieval error policy found in the gateway configuration file, config key: '%s', config appender: '%s', config value: '%s', index: '%s', using default: '%s'.", 
                    GTWY_CONFIG_INDEX_RETRIEVAL_ERROR, pucIndexName, pucSymbolData, pucIndexName, GTWY_INDEX_ERROR_FAIL);
            vGtwySetFailRetrievalError(pgdGtwyIndex->uiFlags);
        }
    }

    /* Set the connection state */
    pgdGtwyIndex->uiCurrentState = GTWY_INDEX_CONNECTION_STATE_DISCONNECTED;

    /* Set the last access time */
    pgdGtwyIndex->tLastAccessTime = (time_t)0;

    /* Set gateway segments array pointer for this gateway index */
    pgdGtwyIndex->pgsGtwySegments = pgsGtwySegments;

    /* Set the length of the gateway segments array for this gateway index */
    pgdGtwyIndex->uiGtwySegmentsLength = uiGtwySegmentsLength;

    /* Set gateway index sorts array pointer for this gateway index */
    pgdGtwyIndex->psdsGtwyIndexSorts = psdsGtwyIndexSorts;

    /* Set the length of the gateway index sorts array for this gateway index */
    pgdGtwyIndex->uiGtwyIndexSortsLength = uiGtwyIndexSortsLength;

    
    /* Reset the overrides */
    if ( (iError = iGtwyResetSearchOverridesOnGatewayIndex(pggGtwyGateway, pgdGtwyIndex)) != SPI_NoError ) {
        goto bailFromiGtwyInitializeGatewayIndex;
    }



    /* Bail label */
    bailFromiGtwyInitializeGatewayIndex:


    /* Free allocated resources */
    s_free(pucIndexLocation);

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppgdGtwyIndex = pgdGtwyIndex;
    }
    else {

        /* Free allocated resources */
        s_free(pgdGtwyIndex);

        if ( pgsGtwySegments != NULL ) {
            for ( uiI = 0, pgsGtwySegmentsPtr = pgsGtwySegments; uiI < uiGtwySegmentsLength; uiI++, pgsGtwySegmentsPtr++ ) {
                s_free(pgsGtwySegmentsPtr->pgmGtwyMirrors);
            }
            s_free(pgsGtwySegments);
        }

        s_free(psdsGtwyIndexSorts);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyDuplicateGatewayIndex()

    Purpose:    Make a duplicate of the gateway index.

    Parameters: pggGtwyGateway      gateway structure
                pgdGtwyIndex        gateway index structure to duplicate
                ppgdGtwyIndex       return pointer for the duplicate gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyDuplicateGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex,
    struct gtwyIndex **ppgdGtwyIndex
)
{


    unsigned int            iError = SPI_NoError;
    struct gtwyIndex        *pgdGtwyIndexCopy = NULL;

    struct gtwySegment      *pgsGtwySegmentsPtr = NULL;
    struct gtwySegment      *pgsGtwySegmentsCopy = NULL;
    struct gtwySegment      *pgsGtwySegmentsCopyPtr = NULL;
    
    struct gtwyIndexSort    *psdsGtwyIndexSortsPtr = NULL;
    struct gtwyIndexSort    *psdsGtwyIndexSortsCopy = NULL;
    struct gtwyIndexSort    *psdsGtwyIndexSortsCopyPtr = NULL;

    unsigned int            uiI = 0;
    unsigned int            uiJ = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyDuplicateGatewayIndex [%s]", pgdGtwyIndex->pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);
    ASSERT(ppgdGtwyIndex != NULL);


    /* Create the gateway segments array copy */
    if ( (pgsGtwySegmentsCopy = (struct gtwySegment *)s_malloc((size_t)(sizeof(struct gtwySegment) * pgdGtwyIndex->uiGtwySegmentsLength))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyDuplicateGatewayIndex;
    }


    /* Populate the gateway segments array copy */
    for ( uiI = 0, pgsGtwySegmentsCopyPtr = pgsGtwySegmentsCopy, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments; 
            uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsGtwySegmentsCopyPtr++, pgsGtwySegmentsPtr++ ) {

        struct gtwyMirror   *pgmGtwyMirrorsPtr = NULL;
        struct gtwyMirror   *pgmGtwyMirrorsCopy = NULL;
        struct gtwyMirror   *pgmGtwyMirrorsCopyPtr = NULL;


        /* Create the gateway mirrors array copy */
        if ( (pgmGtwyMirrorsCopy = (struct gtwyMirror *)s_malloc((size_t)(sizeof(struct gtwyMirror) * pgsGtwySegmentsPtr->uiGtwyMirrorsLength))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyDuplicateGatewayIndex;
        }

        /* Set gateway mirrors array pointer and length for the gateway segments copy */
        pgsGtwySegmentsCopyPtr->pgmGtwyMirrors = pgmGtwyMirrorsCopy;
        pgsGtwySegmentsCopyPtr->uiGtwyMirrorsLength = pgsGtwySegmentsPtr->uiGtwyMirrorsLength;


        /* Populate the gateway mirrors array copy */
        for ( uiJ = 0, pgmGtwyMirrorsCopyPtr = pgmGtwyMirrorsCopy, pgmGtwyMirrorsPtr = pgsGtwySegmentsPtr->pgmGtwyMirrors; 
                uiJ < pgsGtwySegmentsPtr->uiGtwyMirrorsLength; uiJ++, pgmGtwyMirrorsCopyPtr++, pgmGtwyMirrorsPtr++) {

            s_strnncpy(pgmGtwyMirrorsCopyPtr->pucCanonicalIndexName, pgmGtwyMirrorsPtr->pucCanonicalIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);
            s_strnncpy(pgmGtwyMirrorsCopyPtr->pucIndexName, pgmGtwyMirrorsPtr->pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);
            s_strnncpy(pgmGtwyMirrorsCopyPtr->pucHostName, pgmGtwyMirrorsPtr->pucHostName, MAXHOSTNAMELEN + 1);
            pgmGtwyMirrorsCopyPtr->iPort = pgmGtwyMirrorsPtr->iPort;
            pgmGtwyMirrorsCopyPtr->uiCurrentState = pgmGtwyMirrorsPtr->uiCurrentState;
            pgmGtwyMirrorsCopyPtr->uiPriority = pgmGtwyMirrorsPtr->uiPriority;
        }
    }


    /* Create the gateway index sorts array copy */
    if ( (psdsGtwyIndexSortsCopy = (struct gtwyIndexSort *)s_malloc((size_t)(sizeof(struct gtwyIndexSort) * pgdGtwyIndex->uiGtwyIndexSortsLength))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyDuplicateGatewayIndex;
    }
    
    /* Populate the gateway index sorts array copy */
    for ( uiI = 0, psdsGtwyIndexSortsCopyPtr = psdsGtwyIndexSortsCopy, psdsGtwyIndexSortsPtr = pgdGtwyIndex->psdsGtwyIndexSorts; 
            uiI < pgdGtwyIndex->uiGtwyIndexSortsLength; uiI++, psdsGtwyIndexSortsCopyPtr++, psdsGtwyIndexSortsPtr++ ) {
    
        s_strnncpy(psdsGtwyIndexSortsCopyPtr->pucSortFieldName, psdsGtwyIndexSortsPtr->pucSortFieldName, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
        psdsGtwyIndexSortsCopyPtr->uiSortOrderID = psdsGtwyIndexSortsPtr->uiSortOrderID;
    }


    /* Create the gateway index copy */
    if ( (pgdGtwyIndexCopy = (struct gtwyIndex *)s_malloc((size_t)(sizeof(struct gtwyIndex)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyDuplicateGatewayIndex;
    }
    
    /* Set gateway segments array pointer and length for the gateway index copy */
    pgdGtwyIndexCopy->pgsGtwySegments = pgsGtwySegmentsCopy;
    pgdGtwyIndexCopy->uiGtwySegmentsLength = pgdGtwyIndex->uiGtwySegmentsLength;
    
    /* Set gateway index sorts array pointer and length for the gateway index copy */
    pgdGtwyIndexCopy->psdsGtwyIndexSorts = psdsGtwyIndexSortsCopy;
    pgdGtwyIndexCopy->uiGtwyIndexSortsLength = pgdGtwyIndex->uiGtwyIndexSortsLength;


    /* Populate the gateway index copy */
    s_strnncpy(pgdGtwyIndexCopy->pucIndexName, pgdGtwyIndex->pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);
    pgdGtwyIndexCopy->uiIndexProtocol = pgdGtwyIndex->uiIndexProtocol;
    pgdGtwyIndexCopy->uiIndexOrigin = pgdGtwyIndex->uiIndexOrigin;
    pgdGtwyIndexCopy->uiFlags = pgdGtwyIndex->uiFlags;
    pgdGtwyIndexCopy->uiCurrentState = pgdGtwyIndex->uiCurrentState;
    pgdGtwyIndexCopy->tLastAccessTime = pgdGtwyIndex->tLastAccessTime;
    pgdGtwyIndexCopy->uiConnectionTimeOut = pgdGtwyIndex->uiConnectionTimeOut;
    pgdGtwyIndexCopy->uiSearchTimeOut = pgdGtwyIndex->uiSearchTimeOut;
    pgdGtwyIndexCopy->uiRetrievalTimeOut = pgdGtwyIndex->uiRetrievalTimeOut;
    pgdGtwyIndexCopy->uiInformationTimeOut = pgdGtwyIndex->uiInformationTimeOut;
    pgdGtwyIndexCopy->iMirrorAffinity = pgdGtwyIndex->iMirrorAffinity;
    pgdGtwyIndexCopy->uiMaximumSegmentsSearched = pgdGtwyIndex->uiMaximumSegmentsSearched;
    pgdGtwyIndexCopy->uiMinimumSegmentsSearched = pgdGtwyIndex->uiMinimumSegmentsSearched;



    /* Bail label */
    bailFromiGtwyDuplicateGatewayIndex:


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppgdGtwyIndex = pgdGtwyIndexCopy;    
    }
    else {

        /* Free allocated resources */
        if ( pgdGtwyIndexCopy != NULL ) {
            s_free(pgdGtwyIndex);
        }

        if ( pgsGtwySegmentsCopy != NULL ) {
            for ( uiI = 0, pgsGtwySegmentsCopyPtr = pgsGtwySegmentsCopy; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsGtwySegmentsCopyPtr++ ) {
                s_free(pgsGtwySegmentsCopyPtr->pgmGtwyMirrors);
            }
            s_free(pgsGtwySegmentsCopy);
        }

        s_free(psdsGtwyIndexSortsCopy);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyAddGatewayIndexToList()

    Purpose:    Add the passed gateway index to the gateway index list.

    Parameters: pggGtwyGateway      gateway structure
                pgdGtwyIndex        gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyAddGatewayIndexToList
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex
)
{


    int                 iError = SPI_NoError;
    struct gtwyIndex    *pgdGtwyIndexLocal = NULL;
    struct gtwyIndex    **ppgdGtwyIndexListPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAddGatewayIndexToList [%s]", pgdGtwyIndex->pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);


    /* First check to see if the index is already in the gateway index list, no need to add it if it is already there */
    if ( iGtwyGetGatewayIndexFromList(pggGtwyGateway, pgdGtwyIndex->pucIndexName, &pgdGtwyIndexLocal) == SPI_NoError ) {
        return (SPI_NoError);
    }

    
    /* Extend the gateway index list by one entry */
    if ( (ppgdGtwyIndexListPtr = (struct gtwyIndex **)s_realloc(pggGtwyGateway->ppgdGtwyIndexList, 
            (size_t)((pggGtwyGateway->uiGtwyIndexListLength + 1) * sizeof(struct gtwyIndex **)))) == NULL ) {
        
        /* Bail with a memory error */
        iError = SPI_MemError;
        goto bailFromiGtwyAddGatewayIndexToList;
    }
    
    /* Hand over the gateway index list pointer */
    pggGtwyGateway->ppgdGtwyIndexList = ppgdGtwyIndexListPtr;
    
    /* Set the gateway index in the gateway index list */
    pggGtwyGateway->ppgdGtwyIndexList[pggGtwyGateway->uiGtwyIndexListLength] = pgdGtwyIndex;

    /* And extend the gateway index list by one */
    pggGtwyGateway->uiGtwyIndexListLength++;


    
    /* Bail label */
    bailFromiGtwyAddGatewayIndexToList:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyGetGatewayIndexFromList()

    Purpose:    Get the gateway index from the gateway index list.

    Parameters: pggGtwyGateway  gateway structure
                ppgdGtwyIndex   return pointer for the gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyGetGatewayIndexFromList
(
    struct gtwyGateway *pggGtwyGateway,
    unsigned char *pucIndexName,
    struct gtwyIndex **ppgdGtwyIndex
)
{

    unsigned int    uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyGetGatewayIndexFromList [%s]", pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pucIndexName != NULL);
    ASSERT(ppgdGtwyIndex != NULL);


    /* We check the gateway index list to see if the gateway index is there */
    if ( (pggGtwyGateway->ppgdGtwyIndexList != NULL) && (pggGtwyGateway->uiGtwyIndexListLength > 0) ) {
    
        struct gtwyIndex    **ppgdGtwyIndexListPtr = NULL;
        
        /* Loop over each entry in the gateway index list */
        for ( uiI = 0, ppgdGtwyIndexListPtr = pggGtwyGateway->ppgdGtwyIndexList; uiI < pggGtwyGateway->uiGtwyIndexListLength; uiI++, ppgdGtwyIndexListPtr++ ) {
            
            /* Dereference the gateway index for convenience */
            struct gtwyIndex    *pgdGtwyIndex = *ppgdGtwyIndexListPtr;
            
            /* See if this is the index we are looking for */
            if ( s_strcmp(pgdGtwyIndex->pucIndexName, pucIndexName) == 0 ) {
                
                /* It is, so we set the return pointer and return */
                *ppgdGtwyIndex = pgdGtwyIndex;
                
                return (SPI_NoError);
            }
        }
    }


    return (SPI_MiscError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyResetSearchOverridesOnGatewayIndex()

    Purpose:    Reset the field in the gateway index that can be overridden by the search

    Parameters: pggGtwyGateway  gateway structure
                pgdGtwyIndex    gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyResetSearchOverridesOnGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex
)
{

    unsigned char   pucSymbolData[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyResetSearchOverridesOnGatewayIndex [%s]", pgdGtwyIndex->pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);


    /* Set the index timeouts from the gateway timeouts */
    pgdGtwyIndex->uiConnectionTimeOut = pggGtwyGateway->uiConnectionTimeOut;
    pgdGtwyIndex->uiSearchTimeOut = pggGtwyGateway->uiSearchTimeOut;
    pgdGtwyIndex->uiRetrievalTimeOut = pggGtwyGateway->uiRetrievalTimeOut;
    pgdGtwyIndex->uiInformationTimeOut = pggGtwyGateway->uiInformationTimeOut;
    
    /* Set the index mirror affinity from the gateway mirror affinity */
    pgdGtwyIndex->iMirrorAffinity = pggGtwyGateway->iMirrorAffinity;


    /* Set the default maximum segments searched */
    pgdGtwyIndex->uiMaximumSegmentsSearched = GTWY_INDEX_MAXIMUM_SEGMENTS_SEARCHED_DEFAULT;

    /* Get the maximum segments searched */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_MAXIMUM_SEGMENTS_SEARCHED, pgdGtwyIndex->pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) < 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid index maximum segments searched found in the gateway configuration file, config key: '%s', config appender: '%s', config value: '%s', using default: %d.", 
                    GTWY_CONFIG_INDEX_MAXIMUM_SEGMENTS_SEARCHED, pgdGtwyIndex->pucIndexName, pucSymbolData, GTWY_INDEX_MAXIMUM_SEGMENTS_SEARCHED_DEFAULT);
        }
        else {
            pgdGtwyIndex->uiMaximumSegmentsSearched = s_strtol(pucSymbolData, NULL, 10);
        }
    }

    /* Adjust the maximum segments searched, can't be greater than the number of segments */
    if ( pgdGtwyIndex->uiMaximumSegmentsSearched > pgdGtwyIndex->uiGtwySegmentsLength ) {
        pgdGtwyIndex->uiMaximumSegmentsSearched = pgdGtwyIndex->uiGtwySegmentsLength;
    }

    
    /* Set the default minimum segments searched */
    pgdGtwyIndex->uiMinimumSegmentsSearched = GTWY_INDEX_MINIMUM_SEGMENTS_SEARCHED_DEFAULT;

    /* Get the minimum segments searched */
    if ( iUtlConfigGetValue1(pggGtwyGateway->pvUtlGtwyConfig, GTWY_CONFIG_INDEX_MINIMUM_SEGMENTS_SEARCHED, pgdGtwyIndex->pucIndexName, pucSymbolData, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        if ( s_strtol(pucSymbolData, NULL, 10) < 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid index minimum segments searched found in the gateway configuration file, config key: '%s', config appender: '%s', config value: '%s', using default: %d.", 
                    GTWY_CONFIG_INDEX_MINIMUM_SEGMENTS_SEARCHED, pgdGtwyIndex->pucIndexName, pucSymbolData, GTWY_INDEX_MINIMUM_SEGMENTS_SEARCHED_DEFAULT);
        }
        else {
            pgdGtwyIndex->uiMinimumSegmentsSearched = s_strtol(pucSymbolData, NULL, 10);
        }
    }

    /* Adjust the minimum segments searched, can't be greater than the number of segments */
    if ( pgdGtwyIndex->uiMinimumSegmentsSearched > pgdGtwyIndex->uiGtwySegmentsLength ) {
        pgdGtwyIndex->uiMinimumSegmentsSearched = pgdGtwyIndex->uiGtwySegmentsLength;
    }


    /* Set the minimum segments searched to the maximum if it was larger than the maximum */
    if ( (pgdGtwyIndex->uiMaximumSegmentsSearched > 0) && (pgdGtwyIndex->uiMaximumSegmentsSearched > 0) && 
            (pgdGtwyIndex->uiMaximumSegmentsSearched < pgdGtwyIndex->uiMinimumSegmentsSearched) ) {
        pgdGtwyIndex->uiMinimumSegmentsSearched = pgdGtwyIndex->uiMaximumSegmentsSearched;
    }



    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyResetTemporaryErrorsOnGatewayIndex()

    Purpose:    This function clears all the temporary error on a gateway index.

    Parameters: pggGtwyGateway  gateway structure
                pgdGtwyIndex    gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyResetTemporaryErrorsOnGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex
)
{

    int                 iError = SPI_NoError;
    unsigned int        uiI = 0;
    unsigned int        uiJ = 0;
    struct gtwySegment  *pgsGtwySegmentsPtr = NULL;
    struct gtwyMirror   *pgmGtwyMirrorsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyResetTemporaryErrorsOnGatewayIndex [%s]", pgdGtwyIndex->pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);


    /* Check to see if the gateway index is connected */
    if ( pgdGtwyIndex->uiCurrentState != GTWY_INDEX_CONNECTION_STATE_CONNECTED ) {
        return (SPI_InvalidIndex);
    }


    /* Loop over the gateway segments */
    for ( uiI = 0, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsGtwySegmentsPtr++ ) {

        /* Loop over the gateway mirrors */
        for ( uiJ = 0, pgmGtwyMirrorsPtr = pgsGtwySegmentsPtr->pgmGtwyMirrors; uiJ < pgsGtwySegmentsPtr->uiGtwyMirrorsLength; uiJ++, pgmGtwyMirrorsPtr++ ) {

            /* Skip non-connected gateway mirrors */
            if ( pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR ) {
                pgmGtwyMirrorsPtr->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_DISCONNECTED;
            }
        }
    }

    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ====================================
** ===  Gateway Index Functions  ===
** ====================================
*/


/*

    Function:   iGtwyOpenGatewayIndex()

    Purpose:    Open a gateway index.

    Parameters: pggGtwyGateway  gateway structure
                pgdGtwyIndex    gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyOpenGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex
)
{

    int                             iError = SPI_NoError;
    unsigned int                    uiI = 0;
    struct gtwyOpenGatewaySegment   *pgogsGtwyOpenGatewaySegments = NULL;
    struct gtwyOpenGatewaySegment   *pgogsGtwyOpenGatewaySegmentsPtr = NULL;
    struct gtwySegment              *pgsGtwySegmentsPtr = NULL;

    pthread_t                       *ptThreads = NULL;
    int                             iStatus = 0;
    void                            *pvStatus = NULL;
    

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewayIndex [%s]", pgdGtwyIndex->pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);


    /* Check to see if the gateway index is already connected */
    if ( pgdGtwyIndex->uiCurrentState == GTWY_INDEX_CONNECTION_STATE_CONNECTED ) {
        return (SPI_NoError);
    }


    /* Return here if a lazy connection policy in in place, the connections will occur when needed */
    if ( bGtwyLazyConnectionPolicy(pgdGtwyIndex->uiFlags) == true ) {
        pgdGtwyIndex->uiCurrentState = GTWY_INDEX_CONNECTION_STATE_CONNECTED;
        return (SPI_NoError);
    }


    /* Allocate space for the open gateway segment array, this is used to pass the parameters */
    if ( (pgogsGtwyOpenGatewaySegments = (struct gtwyOpenGatewaySegment *)s_malloc((size_t)(sizeof(struct gtwyOpenGatewaySegment) * pgdGtwyIndex->uiGtwySegmentsLength))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyOpenGatewayIndex;
    }
    
    /* Populate the open gateway segment array, loop over the gateway segments */
    for ( uiI = 0, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments, pgogsGtwyOpenGatewaySegmentsPtr = pgogsGtwyOpenGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; 
            uiI++, pgsGtwySegmentsPtr++, pgogsGtwyOpenGatewaySegmentsPtr++ ) {

        pgogsGtwyOpenGatewaySegmentsPtr->pggGtwyGateway = pggGtwyGateway;
        pgogsGtwyOpenGatewaySegmentsPtr->pgdGtwyIndex = pgdGtwyIndex;
        pgogsGtwyOpenGatewaySegmentsPtr->pgsGtwySegment = pgsGtwySegmentsPtr;
    }


    /* Thread if there is more than one gateway segment */
    if ( pgdGtwyIndex->uiGtwySegmentsLength > 1 ) {

        /* Allocate the threads array to keep track of the threads */
        if ( (ptThreads = (pthread_t *)s_malloc((size_t)(sizeof(pthread_t) * pgdGtwyIndex->uiGtwySegmentsLength))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiGtwyOpenGatewayIndex;
        }

        /* Loop over the open gateway segment array, kicking off the connection threads */
        for ( uiI = 0, pgogsGtwyOpenGatewaySegmentsPtr = pgogsGtwyOpenGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgogsGtwyOpenGatewaySegmentsPtr++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewayIndex - Thread: %u, Index: '%s'", uiI, pgogsGtwyOpenGatewaySegmentsPtr->pgdGtwyIndex->pucIndexName); */

            /* Kick off the connection thread */
            if ( (iStatus = s_pthread_create(&ptThreads[uiI], NULL, (void *)iGtwyOpenGatewaySegment, (void *)pgogsGtwyOpenGatewaySegmentsPtr)) != 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a thread.");
                iError = SPI_OpenIndexFailed;
                goto bailFromiGtwyOpenGatewayIndex;
            }
        }
        
        /* Loop over all the open gateway segment array, collecting the connection threads */
        for ( uiI = 0, pgogsGtwyOpenGatewaySegmentsPtr = pgogsGtwyOpenGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgogsGtwyOpenGatewaySegmentsPtr++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewayIndex - Joining thread: %u", uiI); */

            /* Join the thread */
            iStatus = s_pthread_join(ptThreads[uiI], &pvStatus);
            
            /* Erase the thread structure, we are done with it */
            s_memset(&ptThreads[uiI], '\0', sizeof(pthread_t));
    
            /* Handle the thread status */
            if ( iStatus != 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to join to a thread.");
                iError = SPI_MiscError;
                goto bailFromiGtwyOpenGatewayIndex;
            }
        
            /* Check the status pointer, this contains the error code reported by the function we threaded */
            if ( (iError = (int)pvStatus) != SPI_NoError) {
                
                /* Bail if we need to fail on connection errors */
                if ( bGtwyFailConnectionError(pgdGtwyIndex->uiFlags) == true ) {
                    iError = SPI_OpenIndexFailed;
                    goto bailFromiGtwyOpenGatewayIndex;
                }
                
                iUtlLogWarn(UTL_LOG_CONTEXT, "Ignoring connection error, proceeding with connection...");
            
                /* Reset the error for safety */
                iError = SPI_NoError;
            }
        }

        /* Free the threads structures array */
        s_free(ptThreads);
    }
    else {
        
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewayIndex - No Thread: %u, Index: '%s'", 0, pgogsGtwyOpenGatewaySegments->pgdGtwyIndex->pucIndexName); */

        /* Open the gateway segment */
        if ( (iError = iGtwyOpenGatewaySegment(pgogsGtwyOpenGatewaySegments)) != SPI_NoError ) {
                
            /* Bail if we need to fail on connection errors */
            if ( bGtwyFailConnectionError(pgdGtwyIndex->uiFlags) == true ) {
                iError = SPI_OpenIndexFailed;
                goto bailFromiGtwyOpenGatewayIndex;
            }
            
            iUtlLogWarn(UTL_LOG_CONTEXT, "Ignoring connection error, proceeding with connection...");
        
            /* Reset the error for safety */
            iError = SPI_NoError;
        }
    }



    /* Bail label */
    bailFromiGtwyOpenGatewayIndex:


    /* Free allocated resources */
    s_free(ptThreads);
    s_free(pgogsGtwyOpenGatewaySegments);
    
    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the gateway index state */    
        pgdGtwyIndex->uiCurrentState = GTWY_INDEX_CONNECTION_STATE_CONNECTED;
    }
    else {
    
        /* Collect all threads, ignore errors */
        if ( ptThreads != NULL ) {
            for ( uiI = 0; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++ ) {
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewayIndex - Joining thread: %u", uiI); */
                s_pthread_join(ptThreads[uiI], NULL);
            }
        }

        /* Set the gateway index state */    
        pgdGtwyIndex->uiCurrentState = GTWY_INDEX_CONNECTION_STATE_DISCONNECTED;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyCloseGatewayIndex()

    Purpose:    Close the gateway index.

    Parameters: pggGtwyGateway  gateway structure
                pgdGtwyIndex    gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyCloseGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex
)
{

    unsigned int        uiI = 0;
    struct gtwySegment  *pgsGtwySegmentsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyCloseGatewayIndex [%s]", pgdGtwyIndex->pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);


    /* Loop over the gateway segments */
    for ( uiI = 0, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsGtwySegmentsPtr++ ) {

        /* Close the gateway segment - ignore the error */
        iGtwyCloseGatewaySegment(pggGtwyGateway, pgdGtwyIndex, pgsGtwySegmentsPtr);
    }

    /* Free the gateway index */
    iGtwyFreeGatewayIndex(pggGtwyGateway, pgdGtwyIndex);
    pgdGtwyIndex = NULL;


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwySearchGatewayIndex()

    Purpose:    This function searches the gateway index.

    Parameters: pgsgdGtwySearchGatewayIndex search gateway index structure

    Globals:    none

    Returns:    SPI Error Code

*/
static int iGtwySearchGatewayIndex
(
    struct gtwySearchGatewayIndex *pgsgdGtwySearchGatewayIndex
)
{

    int                                 iError = SPI_NoError;
    unsigned int                        uiI = 0, uiJ = 0;
    struct gtwyIndex                    *pgdGtwyIndex = NULL;
    struct gtwySearchGatewaySegment     *pgsgsGtwySearchGatewaySegments = NULL;
    struct gtwySearchGatewaySegment     *pgsgsGtwySearchGatewaySegmentsPtr = NULL;
    struct gtwySegment                  *pgsGtwySegmentsPtr = NULL;
    struct gtwyMirror                   *pgmGtwyMirrorsPtr = NULL;
    struct gtwyMirror                   *pgmGtwyMirrorPtr = NULL;
    struct spiSearchResult              *pssrSpiSearchResultsPtr = NULL;
    unsigned char                       *pucDocumentKey = NULL;

    boolean                             bEarlyCompletion = false;
    boolean                             bReverseOrder = false;

    pthread_t                           *ptThreads = NULL;
    int                                 iStatus = 0;
    void                                *pvStatus = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewayIndex"); */


    ASSERT(pgsgdGtwySearchGatewayIndex != NULL)

    
    /* Dereference the gateyway index for convenience */
    pgdGtwyIndex = pgsgdGtwySearchGatewayIndex->pgdGtwyIndex;


    /* Check to see if the gateway index is connected */
    if ( pgdGtwyIndex->uiCurrentState != GTWY_INDEX_CONNECTION_STATE_CONNECTED ) {
        return (SPI_InvalidIndex);
    }


/*     if ( bUtlLogIsDebug(UTL_LOG_CONTEXT) == true ) { */
/*  */
/*         if ( pgdGtwyIndex->psdsGtwyIndexSorts != NULL ) { */
/*             struct gtwyIndexSort    *psdsGtwyIndexSortsPtr = NULL; */
/*             for ( uiI = 0, psdsGtwyIndexSortsPtr = pgdGtwyIndex->psdsGtwyIndexSorts; uiI < pgdGtwyIndex->uiGtwyIndexSortsLength; uiI++, psdsGtwyIndexSortsPtr++ ) { */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "psdsGtwyIndexSortsPtr->pucSortFieldName: '%s', psdsGtwyIndexSortsPtr->uiSortOrderID: %u",  */
/*                         pucUtlStringsGetPrintableString(psdsGtwyIndexSortsPtr->pucSortFieldName), psdsGtwyIndexSortsPtr->uiSortOrderID); */
/*             } */
/*         } */
/*  */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pgdGtwyIndex->uiMaximumSegmentsSearched: %u, pgdGtwyIndex->uiMinimumSegmentsSearched: %u, pgdGtwyIndex->uiGtwySegmentsLength: %u",  */
/*                 pgdGtwyIndex->uiMaximumSegmentsSearched, pgdGtwyIndex->uiMinimumSegmentsSearched, pgdGtwyIndex->uiGtwySegmentsLength); */
/*     } */


    /* Preset the early completion flag */
    bEarlyCompletion = false;

    /* Four conditions need to be met to complete the search early:
    **
    **    - the gateway index sorts need to be specified
    **    - the minimum number of segments searched must be less than the number of segments
    **    - a sort field name was specified in the search (defaults to relevance if not specified)
    **    - a sort order was specified in the search (defaults to descending if not specified)
    */
    if ( (pgdGtwyIndex->psdsGtwyIndexSorts != NULL) && 
            ((pgdGtwyIndex->uiMinimumSegmentsSearched > 0) && (pgdGtwyIndex->uiMinimumSegmentsSearched < pgdGtwyIndex->uiGtwySegmentsLength)) &&
            (bUtlStringsIsStringNULL(pgsgdGtwySearchGatewayIndex->pucSortFieldName) == false) &&
            (pgsgdGtwySearchGatewayIndex->uiSortOrderID != GTWY_INDEX_SORT_ORDER_INVALID_ID) ) {

        /* Check for early completion if the sort order is ascending or descending */
        if ( (pgsgdGtwySearchGatewayIndex->uiSortOrderID == GTWY_INDEX_SORT_ORDER_ASC_ID) || 
                (pgsgdGtwySearchGatewayIndex->uiSortOrderID == GTWY_INDEX_SORT_ORDER_DESC_ID) ) {
            
            struct gtwyIndexSort    *psdsGtwyIndexSortsPtr = NULL;

            /* Loop over all the entries in the gateway index sorts array to see if the 
            ** index is sorted in the same order as the sort order specifed in the search
            */
            for ( uiI = 0, psdsGtwyIndexSortsPtr = pgdGtwyIndex->psdsGtwyIndexSorts; uiI < pgdGtwyIndex->uiGtwyIndexSortsLength;uiI++, psdsGtwyIndexSortsPtr++ ) {

                /* Set the early completion flag and break out if the sort field names match */
                if ( s_strcasecmp(psdsGtwyIndexSortsPtr->pucSortFieldName, pgsgdGtwySearchGatewayIndex->pucSortFieldName) == 0 ) {
                    bEarlyCompletion = true;
                    break;
                }
            }

            /* Set the reverse order flag based on whether the sort orders match or not */
            if ( bEarlyCompletion == true ) {
                bReverseOrder = (psdsGtwyIndexSortsPtr->uiSortOrderID != pgsgdGtwySearchGatewayIndex->uiSortOrderID) ? true : false;
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "bEarlyCompletion = true, pgsgdGtwySearchGatewayIndex->pucSortFieldName = '%s', bReverseOrder = %s",  */
/*                         pgsgdGtwySearchGatewayIndex->pucSortFieldName, (bReverseOrder == true) ? "true" : "false"); */
            }
        }
        /* Check for early completion if the sort order is no sort order */
        else if ( pgsgdGtwySearchGatewayIndex->uiSortOrderID == GTWY_INDEX_SORT_NONE_ID ) {
            /* Set the early completion flag, in this case we just need to search 
            ** the segments one after the other until we have enough search results
            */
            bEarlyCompletion = true;
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pgsgdGtwySearchGatewayIndex->uiSortOrderID == GTWY_INDEX_SORT_NONE_ID -> bEarlyCompletion = true"); */
        }
    }



    /* Allocate a search gateway segment array, this is used to pass the search parameters
    ** and collects the return parameter for each gateway segment, the search of which may be threaded
    */
    if ( (pgsgsGtwySearchGatewaySegments = (struct gtwySearchGatewaySegment *)s_malloc((size_t)(sizeof(struct gtwySearchGatewaySegment) * pgdGtwyIndex->uiGtwySegmentsLength))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwySearchGatewayIndex;
    }


    /* Populate the search gateway segment array, loop over the gateway segments */
    for ( uiI = 0, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments, pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; 
            uiI++, pgsGtwySegmentsPtr++, pgsgsGtwySearchGatewaySegmentsPtr++ ) {

        pgsgsGtwySearchGatewaySegmentsPtr->pggGtwyGateway = pgsgdGtwySearchGatewayIndex->pggGtwyGateway;
        pgsgsGtwySearchGatewaySegmentsPtr->pgdGtwyIndex = pgdGtwyIndex;
        pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment = pgsGtwySegmentsPtr;
        pgsgsGtwySearchGatewaySegmentsPtr->uiSearchTimeOut = 0;
        pgsgsGtwySearchGatewaySegmentsPtr->pucLanguageCode = pgsgdGtwySearchGatewayIndex->pucLanguageCode;
        pgsgsGtwySearchGatewaySegmentsPtr->pucSearchText = pgsgdGtwySearchGatewayIndex->pucSearchText;
        pgsgsGtwySearchGatewaySegmentsPtr->pucPositiveFeedbackText = pgsgdGtwySearchGatewayIndex->pucPositiveFeedbackText;
        pgsgsGtwySearchGatewaySegmentsPtr->pucNegativeFeedbackText = pgsgdGtwySearchGatewayIndex->pucNegativeFeedbackText;
        pgsgsGtwySearchGatewaySegmentsPtr->uiStartIndex = pgsgdGtwySearchGatewayIndex->uiStartIndex;
        pgsgsGtwySearchGatewaySegmentsPtr->uiEndIndex = pgsgdGtwySearchGatewayIndex->uiEndIndex;
        pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse = NULL;
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "bEarlyCompletion = %s, bReverseOrder = %s", (bEarlyCompletion == true) ? "true" : "false", (bReverseOrder == true) ? "true" : "false"); */


    /* Process for early completion */
    if ( bEarlyCompletion == true ) {
        
        int             iGtwySearchGatewaySegmentsIndex = 0;
        unsigned int    uiSpiSearchResultsLength = 0;
        
        struct timeval  tvSearchStartTimeVal;

        struct timeval  tvSearchCurrentTimeVal;
        struct timeval  tvSearchDiffTimeVal;
        double          dSearchTime = 0;
        unsigned int    uiSearchTimeElapsed = 0;
        unsigned int    uiSearchTimeOut = 0;
        
        boolean         bSearchTimeOut = false;

        unsigned int    uiSegmentsSearched = 0;


        /* Set the search timeout */
        uiSearchTimeOut = (pgdGtwyIndex->uiSearchTimeOut != 0) ? pgdGtwyIndex->uiSearchTimeOut : pgsgdGtwySearchGatewayIndex->pggGtwyGateway->uiSearchTimeOut;

        /* Get the search start time */
        s_gettimeofday(&tvSearchStartTimeVal, NULL);


        /* Set the search gateway segments index */
        iGtwySearchGatewaySegmentsIndex = (bReverseOrder == true) ? (pgdGtwyIndex->uiGtwySegmentsLength - 1) : 0;


        /* We first search the required number of gateway segments in parallel, if specified ( > 0 ) */
        if ( pgdGtwyIndex->uiMinimumSegmentsSearched > 0 ) {
        
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Pre-searching"); */
        
            /* Allocate the threads array to keep track of the threads (we use pgdGtwyIndex->uiGtwySegmentsLength rather than
            ** pgdGtwyIndex->uiMinimumSegmentsSearched because we expect to find pgdGtwyIndex->uiGtwySegmentsLength threads in
            ** the error handling in the bail section of this function
            */
            if ( (ptThreads = (pthread_t *)s_malloc((size_t)(sizeof(pthread_t) * pgdGtwyIndex->uiGtwySegmentsLength))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwySearchGatewayIndex;
            }


            /* Kick off the search threads, loop over the search gateway segments */
            for ( uiI = 0, iGtwySearchGatewaySegmentsIndex = ((bReverseOrder == true) ? (pgdGtwyIndex->uiGtwySegmentsLength - 1) : 0); 
                    uiI < pgdGtwyIndex->uiMinimumSegmentsSearched; uiI++, iGtwySearchGatewaySegmentsIndex += (bReverseOrder == true) ? -1 : 1 ) {
                    
                /* Dereference the search gateway segments for convenience */
                pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments + iGtwySearchGatewaySegmentsIndex;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewayIndex - Thread: %u, iGtwySearchGatewaySegmentsIndex: %d", uiI, iGtwySearchGatewaySegmentsIndex); */
        
                /* Kick off the search thread */
                if ( (iStatus = s_pthread_create(&ptThreads[uiI], NULL, (void *)iGtwySearchGatewaySegment, (void *)pgsgsGtwySearchGatewaySegmentsPtr)) != 0 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a thread.");
                    iError = SPI_SearchIndexFailed;
                    goto bailFromiGtwySearchGatewayIndex;
                }
            }
        
        
            /* Collect the search threads, loop over the search gateway segments */
            for ( uiI = 0, iGtwySearchGatewaySegmentsIndex = ((bReverseOrder == true) ? (pgdGtwyIndex->uiGtwySegmentsLength - 1) : 0); 
                    uiI < pgdGtwyIndex->uiMinimumSegmentsSearched; uiI++, iGtwySearchGatewaySegmentsIndex += (bReverseOrder == true) ? -1 : 1 ) {
        
                /* Dereference the search gateway segments for convenience */
                pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments + iGtwySearchGatewaySegmentsIndex;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewayIndex - Joining thread: %u, iGtwySearchGatewaySegmentsIndex: %d", uiI, iGtwySearchGatewaySegmentsIndex); */
        
                /* Join the thread */
                iStatus = s_pthread_join(ptThreads[uiI], &pvStatus);
                
                /* Erase the thread structure, we are done with it */
                s_memset(&ptThreads[uiI], '\0', sizeof(pthread_t));
        
                /* Handle the thread status */
                if ( iStatus != 0 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to join to a thread.");
                    iError = SPI_MiscError;
                    goto bailFromiGtwySearchGatewayIndex;
                }
        
                /* Check the status pointer, this contains the error code reported by the function we threaded */
                if ( (iError = (int)pvStatus) != SPI_NoError ) {
                    
                    /* Bail if we need to fail on search errors */
                    if ( bGtwyFailSearchError(pgdGtwyIndex->uiFlags) == true ) {
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiGtwySearchGatewayIndex;
                    }
                
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Ignoring search error, proceeding with search...");
            
                    /* Reset the error for safety */
                    iError = SPI_NoError;
                }
                else {
                    /* Increment the segments searched */
                    uiSegmentsSearched++;
                }

    
                /* Increment the spi search results length */
                uiSpiSearchResultsLength += pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength;
                
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength: %u, uiSpiSearchResultsLength: %u",  */
/*                         pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength, uiSpiSearchResultsLength); */

                /* Increment the total results */
                pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults += pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiTotalResults;
                
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiTotalResults: %u, cummulated uiTotalResults: %u",  */
/*                         pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiTotalResults, pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults); */
            }
        
            /* Adjust the gateway segment index */
            iGtwySearchGatewaySegmentsIndex = (bReverseOrder == true) ? pgdGtwyIndex->uiGtwySegmentsLength - (pgdGtwyIndex->uiMinimumSegmentsSearched + 1) : pgdGtwyIndex->uiMinimumSegmentsSearched;


/*             iUtlLogDebug(UTL_LOG_CONTEXT, "End of pre-searching - iGtwySearchGatewaySegmentsIndex: %d", iGtwySearchGatewaySegmentsIndex); */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiSpiSearchResultsLength: %u, pgsgdGtwySearchGatewayIndex->uiEndIndex: %u",  */
/*                     uiSpiSearchResultsLength, pgsgdGtwySearchGatewayIndex->uiEndIndex); */


            /* Set the search current time */
            s_gettimeofday(&tvSearchCurrentTimeVal, NULL);
            
            /* Get the diff search time since we started searching (the first search) */
            UTL_DATE_DIFF_TIMEVAL(tvSearchStartTimeVal, tvSearchCurrentTimeVal, tvSearchDiffTimeVal);
            
            /* Turn the diff search time into milliseconds */
            UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvSearchDiffTimeVal, dSearchTime);
            
            /* Turn the float into an unsigned int */
            uiSearchTimeElapsed = (unsigned int)dSearchTime;

            /* Timeout if the search elapsed time is longer than the search timeout, 
            ** otherwise decrement the search timeout which is then used for the next search 
            */
            if ( uiSearchTimeElapsed >= uiSearchTimeOut ) {
    
                /* Skip over errors if we can ignore search errors */
                if ( bGtwyIgnoreSearchError(pgdGtwyIndex->uiFlags) == true ) {
                    /* Setting the timeout flag will cause the search to stop early */
                    bSearchTimeOut = true;
    
/*                     iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to search the index: '%s', timed out, executing a partial search.", pgdGtwyIndex->pucIndexName); */
                }
                else {
/*                     iUtlLogError(UTL_LOG_CONTEXT, "Failed to search the index: '%s', timed out.", pgdGtwyIndex->pucIndexName); */
                    iError = SPI_SearchIndexFailed;
                    goto bailFromiGtwySearchGatewayIndex;
                }
            }
            else {
                uiSearchTimeOut = (uiSearchTimeElapsed > uiSearchTimeOut) ? 0 : uiSearchTimeOut - uiSearchTimeElapsed;
            }


/*             iUtlLogDebug(UTL_LOG_CONTEXT, "bSearchTimeOut: '%s', uiSearchTimeOut: %u", (bSearchTimeOut == true) ? "true" : "false", uiSearchTimeOut); */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Segments searched: %d", (bReverseOrder == true) ? pgdGtwyIndex->uiGtwySegmentsLength - (iGtwySearchGatewaySegmentsIndex + 1) : iGtwySearchGatewaySegmentsIndex); */
        }



        /* Carry on searching if we have not timed out, and if we still need results, 
        ** and if we have not reached the maximum number of segments we can search (if it is set)
        */
        if ( (bSearchTimeOut == false) && (uiSpiSearchResultsLength <= pgsgdGtwySearchGatewayIndex->uiEndIndex) &&
                ((pgdGtwyIndex->uiMaximumSegmentsSearched == 0) || 
                (((bReverseOrder == true) ? pgdGtwyIndex->uiGtwySegmentsLength - (iGtwySearchGatewaySegmentsIndex + 1) : iGtwySearchGatewaySegmentsIndex) < pgdGtwyIndex->uiMaximumSegmentsSearched)) ) {
    
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Regular searching - iGtwySearchGatewaySegmentsIndex: %d", iGtwySearchGatewaySegmentsIndex); */
    
            /* Loop while we are still within the list */
            while ( (iGtwySearchGatewaySegmentsIndex >= 0) && (iGtwySearchGatewaySegmentsIndex < pgdGtwyIndex->uiGtwySegmentsLength) ) {
    
                /* Dereference the search gateway segments for convenience */
                pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments + iGtwySearchGatewaySegmentsIndex;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewaySegmentsIndex: %d", iGtwySearchGatewaySegmentsIndex); */
    
                /* Set the search timeout for this particular search, overrides the search timeout for this index */
                pgsgsGtwySearchGatewaySegmentsPtr->uiSearchTimeOut = uiSearchTimeOut;
    
    
                /* Run the search */
                if ( (iError = iGtwySearchGatewaySegment(pgsgsGtwySearchGatewaySegmentsPtr)) != SPI_NoError ) {
                    
                    /* Bail if we need to fail on search errors */
                    if ( bGtwyFailSearchError(pgdGtwyIndex->uiFlags) == true ) {
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiGtwySearchGatewayIndex;
                    }
                    
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Ignoring search error, proceeding with search...");
    
                    /* Reset the error for safety */
                    iError = SPI_NoError;
                }
                else {
                    /* Increment the segments searched */
                    uiSegmentsSearched++;
                }

    
                /* Set the search current time */
                s_gettimeofday(&tvSearchCurrentTimeVal, NULL);
                
                /* Get the diff search time since we started searching (the first search) */
                UTL_DATE_DIFF_TIMEVAL(tvSearchStartTimeVal, tvSearchCurrentTimeVal, tvSearchDiffTimeVal);
                
                /* Turn the diff search time into milliseconds */
                UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvSearchDiffTimeVal, dSearchTime);
                
                /* Turn the float into an unsigned int */
                uiSearchTimeElapsed = (unsigned int)dSearchTime;
                
                /* Timeout if the search elapsed time is longer than the search timeout, 
                ** otherwise decrement the search timeout which is then used for the next search 
                */
                if ( uiSearchTimeElapsed >= uiSearchTimeOut ) {

                    /* Skip over errors if we can ignore search errors */
                    if ( bGtwyIgnoreSearchError(pgdGtwyIndex->uiFlags) == true ) {
/*                         iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to search the index: '%s', timed out, executing a partial search.", pgdGtwyIndex->pucIndexName); */

                        /* Setting the timeout flag will cause the search to stop early */
                        bSearchTimeOut = true;
                    }
                    else {
/*                         iUtlLogError(UTL_LOG_CONTEXT, "Failed to search the index: '%s', timed out.", pgdGtwyIndex->pucIndexName); */
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiGtwySearchGatewayIndex;
                    }
                }
                else {
                    uiSearchTimeOut = (uiSearchTimeElapsed > uiSearchTimeOut) ? 0 : uiSearchTimeOut - uiSearchTimeElapsed;
                }
    
            
                /* Increment the spi search results length */
                uiSpiSearchResultsLength += pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pgsgsGtwySearchGatewaySegmentsPtr->uiSpiSearchResultsLength: %u, uiSpiSearchResultsLength: %u",  */
/*                         pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength, uiSpiSearchResultsLength); */
    
                /* Increment the total results */
                pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults += pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiTotalResults;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiTotalResults: %u, cummulated uiTotalResults: %u",  */
/*                         pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiTotalResults, pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults); */
    
    
                /* We have an opportunity to stop the search early if we have timed out, or we have gathered enough results, 
                ** or we have searched the maximum number of segments we can search
                */
                if ( (bSearchTimeOut == true) || (uiSpiSearchResultsLength > pgsgdGtwySearchGatewayIndex->uiEndIndex) ||
                        ((pgdGtwyIndex->uiMaximumSegmentsSearched > 0) && 
                        (((bReverseOrder == true) ? pgdGtwyIndex->uiGtwySegmentsLength - iGtwySearchGatewaySegmentsIndex : iGtwySearchGatewaySegmentsIndex + 1) >= pgdGtwyIndex->uiMaximumSegmentsSearched)) ) {
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Cutting out after searching: %d segments", (bReverseOrder == true) ? pgdGtwyIndex->uiGtwySegmentsLength - iGtwySearchGatewaySegmentsIndex : iGtwySearchGatewaySegmentsIndex + 1); */
                    break;
                }
    
                /* Increment/decrement the index into the search gateway segments */
                iGtwySearchGatewaySegmentsIndex += (bReverseOrder == true) ? -1 : 1;
    
            }    /* while ( (iGtwySearchGatewaySegmentsIndex >= 0) && (iGtwySearchGatewaySegmentsIndex < pgdGtwyIndex->uiGtwySegmentsLength) ) */
    
        }
    


            
        /* Estimate the total results if this was a partial search */
        if ( ((bReverseOrder == true) && (iGtwySearchGatewaySegmentsIndex > 0)) || ((bReverseOrder == false) && (iGtwySearchGatewaySegmentsIndex < (pgdGtwyIndex->uiGtwySegmentsLength - 1))) ) {

            unsigned long   ulDocumentPartialCount = 0;
            unsigned long   ulDocumentTotalCount = 0;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Estimating - iGtwySearchGatewaySegmentsIndex: %d", iGtwySearchGatewaySegmentsIndex); */

#if defined(GTWY_ENABLE_EXPENSIVE_TOTAL_RESULTS_ESTIMATE)
            /* Add up the number of documents in the index we have searched, loop over the gateway segments */
            for ( uiI = 0, pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsgsGtwySearchGatewaySegmentsPtr++ ) {
                
                struct spiIndexInfo     *psiiSpiIndexInfo = NULL;
                boolean                 bIndexInfoInCache = false;


                /* Loop over the gateway mirrors in the gateway segment looking for one in the cache */
                for ( uiJ = 0, psiiSpiIndexInfo = NULL, pgmGtwyMirrorsPtr = pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment->pgmGtwyMirrors; 
                        uiJ < pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment->uiGtwyMirrorsLength; uiJ++, pgmGtwyMirrorsPtr++ ) {
                
                    /* Get the index information from the cache, break out of the loop if we find it */
                    if ( iGtwyGetFromInfoCache(pgsgdGtwySearchGatewayIndex->pggGtwyGateway, pgmGtwyMirrorsPtr->pucCanonicalIndexName, 
                            GTWY_INFO_CACHE_TYPE_INDEX_INFO, (void **)&psiiSpiIndexInfo, NULL) == SPI_NoError ) {
                        bIndexInfoInCache = true;
                        break;
                    }
                }

                
                /* Failed to find the index information in the cache, so we need to get it */
                if ( psiiSpiIndexInfo == NULL ) {

                    unsigned char   *pucErrorString = NULL;
                    int             iErrorCode = SPI_NoError;

                    /* Make sure the gateway segment is open */
                    {
                        struct gtwyOpenGatewaySegment   gogsGtwyOpenGatewaySegment;

                        gogsGtwyOpenGatewaySegment.pggGtwyGateway = pgsgsGtwySearchGatewaySegmentsPtr->pggGtwyGateway;
                        gogsGtwyOpenGatewaySegment.pgdGtwyIndex = pgdGtwyIndex;
                        gogsGtwyOpenGatewaySegment.pgsGtwySegment = pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment;
                        
                        /* Open the gateway segment, ignore errors and continue */
                        if ( (iError = iGtwyOpenGatewaySegment(&gogsGtwyOpenGatewaySegment)) != SPI_NoError ) {
                            continue;
                        }
                    }

                    /* Loop over the gateway mirrors in the gateway segment looking for the connected one */
                    for ( uiJ = 0, pgmGtwyMirrorPtr = NULL, pgmGtwyMirrorsPtr = pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment->pgmGtwyMirrors; 
                            uiJ < pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment->uiGtwyMirrorsLength; uiJ++, pgmGtwyMirrorsPtr++ ) {
                    
                        /* Grab the first one that is connected, set the gateway mirror pointer and break out */
                        if ( pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_CONNECTED ) {
                            pgmGtwyMirrorPtr = pgmGtwyMirrorsPtr;
                            break;
                        }
                    }
                    
                    /* Skip the gateway segments we are not connected to */
                    if ( pgmGtwyMirrorPtr == NULL ) {
                        continue;
                    }

        
                    /* Set the information timeout */
                    if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorsPtr->pvUtlNet, pgdGtwyIndex->uiInformationTimeOut)) != LWPS_NoError ) {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to set the net information timeout for the index: '%s', utl error: %d.", 
                                pgmGtwyMirrorPtr->pucCanonicalIndexName, iError);
                    }

                    /* Get the index information */
                    if ( (iError = iLwpsIndexInfoRequestHandle(pgmGtwyMirrorPtr->pvLwps, pgmGtwyMirrorPtr->pucIndexName, &psiiSpiIndexInfo, 
                            &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to handle an lwps index information request with: '%s', lwps error: %d.", pgmGtwyMirrorPtr->pucIndexName, iError);

                        /* Skip the gateway segments for which we failed to get information */
                        continue;
                    }

                    /* Check the error code */
                    if (iErrorCode != SPI_NoError ) {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to get the index information from: '%s', returned error: %d, error text: '%s'.", 
                                pgmGtwyMirrorPtr->pucIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);

                        /* Skip the gateway segments for which we failed to get information */
                        continue;
                    }
                    
                    /* Add the index information to the cache */
                    if ( iGtwyAddToInfoCache(pgsgdGtwySearchGatewayIndex->pggGtwyGateway, pgmGtwyMirrorPtr->pucCanonicalIndexName, 
                            GTWY_INFO_CACHE_TYPE_INDEX_INFO, (void *)psiiSpiIndexInfo, 0) == SPI_NoError ) {
                        bIndexInfoInCache = true;
                    }
                }


                /* Increment the partial count for the lwps index we have searched */
                if ( ((bReverseOrder == false) && (uiI <= iGtwySearchGatewaySegmentsIndex)) || ((bReverseOrder == true) && (uiI >= iGtwySearchGatewaySegmentsIndex)) ) {
                    ulDocumentPartialCount += psiiSpiIndexInfo->uiDocumentCount;
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "ulDocumentPartialCount += psiiSpiIndexInfo->uiDocumentCount: %u.", psiiSpiIndexInfo->uiDocumentCount); */
                }

                /* Increment the total count */
                ulDocumentTotalCount += psiiSpiIndexInfo->uiDocumentCount;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "ulDocumentTotalCount += psiiSpiIndexInfo->uiDocumentCount: %u.", psiiSpiIndexInfo->uiDocumentCount); */

                /* Note that we do not free the index information because it was added to the cache */
                if ( bIndexInfoInCache == false ) {
                    iSpiFreeIndexInfo(psiiSpiIndexInfo);
                    psiiSpiIndexInfo = NULL;
                }

            }
#endif    /* defined(GTWY_ENABLE_EXPENSIVE_TOTAL_RESULTS_ESTIMATE) */


/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Original pgsgdGtwySearchGatewayIndex->uiTotalResults: %u, ulDocumentPartialCount; %lu, ulDocumentTotalCount: %lu, pgdGtwyIndex->uiGtwySegmentsLength: %u, uiSegmentsSearched: %u", */
/*                     pgsgdGtwySearchGatewayIndex->uiTotalResults, ulDocumentPartialCount, ulDocumentTotalCount, pgdGtwyIndex->uiGtwySegmentsLength, uiSegmentsSearched); */

            /* Guestimated the total number of search results based on the counts, use the total and
            ** partial counts if are more than 0, otherwise use the number of index we searched
            */
            if ( (ulDocumentTotalCount > 0) && (ulDocumentPartialCount > 0) ) {
                pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults = 
                        ((float)pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults * ((float)ulDocumentTotalCount / ulDocumentPartialCount));
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "Guestimated (i) pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults: %u",  */
/*                         pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults); */
            }
            /* Adjust that guestimate based on the ratio of gateway segments to those that were searched */
            else if ( uiSegmentsSearched != pgdGtwyIndex->uiGtwySegmentsLength ) {
                pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults = 
                        ((float)pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults * ((float)pgdGtwyIndex->uiGtwySegmentsLength / uiSegmentsSearched));
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "Guestimated (ii) pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults: %u",  */
/*                         pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults); */
            }
        }
    }
    
    /* Process for no early completion */
    else {
    
        /* Thread if there is more than one segment */
        if ( pgdGtwyIndex->uiGtwySegmentsLength > 1 ) {

            /* Allocate the threads array to keep track of the threads */
            if ( (ptThreads = (pthread_t *)s_malloc((size_t)(sizeof(pthread_t) * pgdGtwyIndex->uiGtwySegmentsLength))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwySearchGatewayIndex;
            }


            /* Kick off the search threads, loop over the search gateway segments */
            for ( uiI = 0, pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsgsGtwySearchGatewaySegmentsPtr++ ) {
            
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewayIndex - Thread: %u", uiI); */

                /* Kick off the search thread */
                if ( (iStatus = s_pthread_create(&ptThreads[uiI], NULL, (void *)iGtwySearchGatewaySegment, (void *)pgsgsGtwySearchGatewaySegmentsPtr)) != 0 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a thread.");
                    iError = SPI_SearchIndexFailed;
                    goto bailFromiGtwySearchGatewayIndex;
                }
            }
    
    
            /* Collect the search threads, loop over the search gateway segments */
            for ( uiI = 0; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++ ) {
            
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewayIndex - Joining thread: %u", uiI); */

                /* Join the thread */
                iStatus = s_pthread_join(ptThreads[uiI], &pvStatus);
                
                /* Erase the thread structure, we are done with it */
                s_memset(&ptThreads[uiI], '\0', sizeof(pthread_t));
        
                /* Handle the thread status */
                if ( iStatus != 0 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to join to a thread.");
                    iError = SPI_MiscError;
                    goto bailFromiGtwySearchGatewayIndex;
                }
        
                /* Check the status pointer, this contains the error code reported by the function we threaded */
                if ( (iError = (int)pvStatus) != SPI_NoError ) {
                    
                    /* Bail if we need to fail on search errors */
                    if ( bGtwyFailSearchError(pgdGtwyIndex->uiFlags) == true ) {
                        iError = SPI_SearchIndexFailed;
                        goto bailFromiGtwySearchGatewayIndex;
                    }
                
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Ignoring search error, proceeding with search...");
            
                    /* Reset the error for safety */
                    iError = SPI_NoError;
                }
            }
        }
        else {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "No Thread"); */

            /* Run the search directly */
            if ( (iError = iGtwySearchGatewaySegment(pgsgsGtwySearchGatewaySegments)) != SPI_NoError ) {
                
                /* Bail if we need to fail on search errors */
                if ( bGtwyFailSearchError(pgdGtwyIndex->uiFlags) == true ) {
                    iError = SPI_SearchIndexFailed;
                    goto bailFromiGtwySearchGatewayIndex;
                }
                
                iUtlLogWarn(UTL_LOG_CONTEXT, "Ignoring search error, proceeding with search...");
            
                /* Reset the error for safety */
                iError = SPI_NoError;
            }
        }
    }


    /* Process the search results, loop over the gateway search gateway mirrors */
    for ( uiI = 0, pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsgsGtwySearchGatewaySegmentsPtr++ ) {
    
        /* Get a pointer to the currently connected mirror */
        for ( uiJ = 0, pgmGtwyMirrorPtr = NULL, pgmGtwyMirrorsPtr = pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment->pgmGtwyMirrors; 
                uiJ < pgsgsGtwySearchGatewaySegmentsPtr->pgsGtwySegment->uiGtwyMirrorsLength; uiJ++, pgmGtwyMirrorsPtr++ ) {
        
            /* Grab the first one that is connected, set the gateway mirror pointer and break out */
            if ( pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_CONNECTED ) {
                pgmGtwyMirrorPtr = pgmGtwyMirrorsPtr;
                break;
            }
        }
        
        if ( pgmGtwyMirrorPtr == NULL ) {
            continue;
        }

            
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "merge - pgmGtwyMirrorPtr->pucCanonicalIndexName [%s]", pgmGtwyMirrorPtr->pucCanonicalIndexName); */


        /* We might need to side-effect the document IDs     */
        switch ( pgdGtwyIndex->uiIndexOrigin ) {

            case GTWY_INDEX_ORIGIN_FROM_CONFIG_ID:

                /* We side effect the document key to store the gateway mirror location information, so that we know which
                ** gateway mirror this document came from and can route future requests to the appropriate gateway mirror.
                */
                for ( uiJ = 0, pssrSpiSearchResultsPtr = pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->pssrSpiSearchResults; 
                        uiJ < pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength; 
                        uiJ++, pssrSpiSearchResultsPtr++) {
                    
                    /* Get the new document key length, make space for the '/' and the terminating NULL */
                    unsigned int uiDocumentKeyLength = s_strlen(pgmGtwyMirrorPtr->pucCanonicalIndexName) + s_strlen(pssrSpiSearchResultsPtr->pucDocumentKey) + 2;
                    
                    /* Allocate space for the new document key */
                    if ( (pucDocumentKey = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * uiDocumentKeyLength))) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiGtwySearchGatewayIndex;
                    }
                    
                    /* Create the new document key */
                    snprintf(pucDocumentKey, uiDocumentKeyLength, "%s/%s", pgmGtwyMirrorPtr->pucCanonicalIndexName, pssrSpiSearchResultsPtr->pucDocumentKey);
                    
                    /* Free the old document key and swap in the new document key */
                    s_free(pssrSpiSearchResultsPtr->pucDocumentKey);
                    pssrSpiSearchResultsPtr->pucDocumentKey = pucDocumentKey;
                }
                    

                break;

            case GTWY_INDEX_ORIGIN_FROM_URL_ID:
                break;

            default:
                break;
        }





        /* Process results if we got any */
        if ( pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength > 0 ) {
        
            /* Make sure we have enough space in our stored search results array for the one we just got */
            if ( (pssrSpiSearchResultsPtr = (struct spiSearchResult *)s_realloc(pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->pssrSpiSearchResults, (size_t)(sizeof(struct spiSearchResult) * 
                    (pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiSpiSearchResultsLength + pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiGtwySearchGatewayIndex;
            }

            /* Hand over the pointer */
            pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->pssrSpiSearchResults = pssrSpiSearchResultsPtr;

            /* Copy over the search results array we just got to the stored one */
            s_memcpy(pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->pssrSpiSearchResults + pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiSpiSearchResultsLength, 
                    pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->pssrSpiSearchResults,
                    pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength * sizeof(struct spiSearchResult));

            /* Increment the total number of documents retrieved so far */
            pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiSpiSearchResultsLength += pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength;

            /* Free the longs results, we dont need them any more, since we have stored them, note that we 
            ** dont free the document items since we only copied their pointers and not their data
            */
            s_free(pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->pssrSpiSearchResults);

            /* Clear the variables */
            pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->pssrSpiSearchResults = NULL;
            pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSpiSearchResultsLength = 0;
            
            /* Set the max sort key in the index structure */
            pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->dMaxSortKey = 
                    UTL_MACROS_MAX(pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->dMaxSortKey, pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->dMaxSortKey);
            
            /* Set the sort type in the index structure ?? */
            pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiSortType = pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiSortType;
            
            /* Set the total results in the index structure, only if this was not an early completion */
            if ( bEarlyCompletion == false ) {
                pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse->uiTotalResults += pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse->uiTotalResults;
            }
        }
    }



    /* Bail label */
    bailFromiGtwySearchGatewayIndex:


    /* Handle the error */
    if ( iError != SPI_NoError ) {
        
        /* Collect all threads, ignore errors */
        if ( ptThreads != NULL ) {
            for ( uiI = 0; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++ ) {
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewayIndex - Joining thread: %u", uiI); */
                s_pthread_join(ptThreads[uiI], NULL);
            }
        }

        /* Need to free spi search results if they are still allocated */        
        if ( pgsgsGtwySearchGatewaySegments != NULL ) {
            
            for ( uiI = 0, pgsgsGtwySearchGatewaySegmentsPtr = pgsgsGtwySearchGatewaySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsgsGtwySearchGatewaySegmentsPtr++ ) {
        
                /* Free the gateway search gateway segment search response */
                iSpiFreeSearchResponse(pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse);
                pgsgsGtwySearchGatewaySegmentsPtr->pssrSpiSearchResponse = NULL;
            }
        }
        
        /* Free the gateway index search response */
        iSpiFreeSearchResponse(pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse);
        pgsgdGtwySearchGatewayIndex->pssrSpiSearchResponse = NULL;
    }

    /* Free allocated resources */
    s_free(pgsgsGtwySearchGatewaySegments);
    s_free(ptThreads);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyRetrieveDocumentFromGatewayIndex()

    Purpose:    This function gets a document from a gateway index.

    Parameters: pggGtwyGateway      gateway structure
                pgdGtwyIndex        gateway index structure
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                uiChunkType         document chunk type
                uiChunkStart        start chunk
                uiChunkEnd          end chunk
                ppvData             return pointer of data returned
                puiDataLength       return pointer for length of data returned

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyRetrieveDocumentFromGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int                 iError = SPI_NoError;
    unsigned char       pucScanfFormatHost[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char       pucHostAddress[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char       pucHostName[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    int                 iPort = LWPS_PROTOCOL_PORT_DEFAULT;
    unsigned char       pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char       pucRemoteDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char       *pucDocumentKeyPtr = NULL;
    struct gtwySegment  *pgsGtwySegmentsPtr = NULL;
    struct gtwySegment  *pgsGtwySegmentPtr = NULL;
    struct gtwyMirror   *pgmGtwyMirrorsPtr = NULL;
    struct gtwyMirror   *pgmGtwyMirrorPtr = NULL;
    unsigned int        uiI = 0, uiJ = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyRetrieveDocumentFromGatewayIndex [%s][%s][%s][%u][%u][%u]",  */
/*             pucDocumentKey, pucItemName, pucMimeType, uiChunkType, uiChunkStart, uiChunkEnd); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(SPI_CHUNK_TYPE_VALID(uiChunkType) == true);
    ASSERT(uiChunkStart >= 0);
    ASSERT(uiChunkEnd >= 0);
    ASSERT(uiChunkEnd >= uiChunkStart);
    ASSERT(ppvData != NULL);
    ASSERT(puiDataLength != NULL);


    /* Check to see if the gateway index is connected */
    if ( pgdGtwyIndex->uiCurrentState != GTWY_INDEX_CONNECTION_STATE_CONNECTED ) {
        return (SPI_InvalidIndex);
    }


    /* Find out which lwps index this document came from */
    switch ( pgdGtwyIndex->uiIndexOrigin ) {

        case GTWY_INDEX_ORIGIN_FROM_CONFIG_ID:

            /* Parse out the lwps index name and document key into its component parts */
            snprintf(pucScanfFormatHost, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%*[^:]://%%%d[^/]/%%%d[^/]/%%%ds", UTL_CONFIG_VALUE_MAXIMUM_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH);
            if ( sscanf(pucDocumentKey, pucScanfFormatHost, pucHostAddress, pucIndexName, pucRemoteDocumentKey) != 3 ) {
                return (SPI_InvalidDocumentKey);
            }

            /* Set the document key pointer to the remote document key */
            pucDocumentKeyPtr = pucRemoteDocumentKey;

            /* Parse out the host address into its component parts */
            snprintf(pucScanfFormatHost, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]:%%d", UTL_CONFIG_VALUE_MAXIMUM_LENGTH);
            if ( sscanf(pucHostAddress, pucScanfFormatHost, pucHostName, &iPort) < 1 ) {
                return (SPI_InvalidDocumentKey);
            }
            
            /* Loop over the gateway segments, looking for the gateway segment which this document belongs to */
            for ( uiI = 0, pgsGtwySegmentPtr = NULL, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsGtwySegmentsPtr++ ) {
            
                /* Loop over the gateway mirrors, looking for the gateway mirror which this document belongs to */
                for ( uiJ = 0, pgmGtwyMirrorsPtr = pgsGtwySegmentsPtr->pgmGtwyMirrors; uiJ < pgsGtwySegmentsPtr->uiGtwyMirrorsLength; uiJ++, pgmGtwyMirrorsPtr++ ) {

                    /* Compare the host name, the index name and the host port */
                    if ( (s_strcmp(pgmGtwyMirrorsPtr->pucHostName, pucHostName) == 0) && (s_strcmp(pgmGtwyMirrorsPtr->pucIndexName, pucIndexName) == 0) && 
                            (pgmGtwyMirrorsPtr->iPort == iPort) ) {

                        /* We found the gateway mirror this document came from, so we set the gateway segment pointer */
                        pgsGtwySegmentPtr = pgsGtwySegmentsPtr;

                        /* And we set the gateway mirror pointer if this is a search report as we need to pick it up from this specific gateway mirror */
                        if ( (s_strcmp(pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) && (s_strcmp(pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
                            pgmGtwyMirrorPtr = pgmGtwyMirrorsPtr;
                        }

                        break;
                    }
                }
            }

            break;


        case GTWY_INDEX_ORIGIN_FROM_URL_ID:

            /* Set the document key pointer from the passed document key */
            pucDocumentKeyPtr = pucDocumentKey;

            /* Pick the first gateway segment (there is only one) */
            pgsGtwySegmentPtr = pgdGtwyIndex->pgsGtwySegments;

            break;


        default:
            return (SPI_InvalidDocumentKey);

    }


    /* Check that we found a gateway segment and that the document key pointer is set */
    if ( (pgsGtwySegmentPtr == NULL) || (pucDocumentKeyPtr == NULL) ) {
        return (SPI_RetrieveDocumentFailed);
    }


    /* Retrieve the document */
    if ( (iError = iGtwyRetrieveDocumentFromGatewaySegment(pggGtwyGateway, pgdGtwyIndex, pgsGtwySegmentPtr, pgmGtwyMirrorPtr, 
            pucDocumentKeyPtr, pucItemName, pucMimeType, uiChunkType, uiChunkStart, uiChunkEnd, ppvData, puiDataLength)) != SPI_NoError ) {
        
        /* Bail if we need to fail on retrieval errors */
        if ( bGtwyFailRetrievalError(pgdGtwyIndex->uiFlags) == true ) {
            return (iError);
        }

        /* Reset the error */
        iError = SPI_NoError;
    }
    

    return (iError);
    
}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyFreeGatewayIndex()

    Purpose:    Free the gateway index.

    Parameters: pggGtwyGateway  gateway structure
                pgdGtwyIndex    gateway index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyFreeGatewayIndex
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex
)
{

    unsigned int        uiI = 0;
    struct gtwySegment  *pgsGtwySegmentsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyFreeGatewayIndex [%s]", pgdGtwyIndex->pucIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);


    /* Free the segments/mirror arrays */
    if ( pgdGtwyIndex->pgsGtwySegments != NULL ) {
        for ( uiI = 0, pgsGtwySegmentsPtr = pgdGtwyIndex->pgsGtwySegments; uiI < pgdGtwyIndex->uiGtwySegmentsLength; uiI++, pgsGtwySegmentsPtr++ ) {
            s_free(pgsGtwySegmentsPtr->pgmGtwyMirrors);
        }
        s_free(pgdGtwyIndex->pgsGtwySegments);
    }
    
    /* Free the sorts array */
    s_free(pgdGtwyIndex->psdsGtwyIndexSorts);

    /* Free the gateway index */
    s_free(pgdGtwyIndex);


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/

/* 
** ===================================
** ===  Gateway Segment Functions  ===
** ===================================
*/


/*

    Function:   iGtwyOpenGatewaySegment()

    Purpose:    This opens the connection to the gateway segment.

    Parameters: pgogsGtwyOpenGatewaySegment open gateway segment structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyOpenGatewaySegment
(
    struct gtwyOpenGatewaySegment *pgogsGtwyOpenGatewaySegment
)
{

    int                     iError = SPI_NoError;
    struct gtwyGateway      *pggGtwyGateway = NULL;
    struct gtwyIndex        *pgdGtwyIndex = NULL;
    struct gtwySegment      *pgsGtwySegment = NULL;
    unsigned int            uiI = 0;
    unsigned int            uiJ = 0;
    unsigned int            *puilAvailability = NULL;
    unsigned int            uiAvailabilityLength = 0;
    struct gtwyMirror       *pgmGtwyMirrorsPtr = NULL;
    boolean                 bLogConnectionSuccess = false;
    boolean                 bMirrorAffinityTested = false;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewaySegment."); */


    ASSERT(pgogsGtwyOpenGatewaySegment != NULL);


    /* Dereference for convenience */
    pggGtwyGateway = pgogsGtwyOpenGatewaySegment->pggGtwyGateway;
    pgdGtwyIndex = pgogsGtwyOpenGatewaySegment->pgdGtwyIndex;
    pgsGtwySegment = pgogsGtwyOpenGatewaySegment->pgsGtwySegment;


    /* Create the availability array length from the priorities */
    for ( uiI = 0, uiAvailabilityLength = 0, pgmGtwyMirrorsPtr = pgsGtwySegment->pgmGtwyMirrors; uiI < pgsGtwySegment->uiGtwyMirrorsLength; uiI++, pgmGtwyMirrorsPtr++ ) {
        uiAvailabilityLength += pgmGtwyMirrorsPtr->uiPriority;
    }
    
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewaySegment - uiAvailabilityLength: %u.", uiAvailabilityLength); */


    /* Create the availability array, this is an array of all the available gateway mirrors 
    ** represented as offsets in the gateway mirrors array, so:
    **
    **    0 2 3
    **
    ** would tell that gateway mirrors 0, 2, and 3 were available. If a gateway mirror has a 
    ** higher priority, then it will be listed multiple times, so if gateway mirror 3 has a
    ** priority of 2, the above list would look like:
    **
    ** 0 2 3 3
    **
    */
    if ( (puilAvailability = s_malloc(sizeof(unsigned int) * uiAvailabilityLength)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyOpenGatewaySegment;
    }


    /* Loop forever, we control the loop from within */
    while ( true ) {

        unsigned int    uiAvailabilityIndex = 0;

        /* Loop through all the gateway mirrors, checking if any are connected and adding available ones to availability list */
        for ( uiI = 0, uiAvailabilityLength = 0, pgmGtwyMirrorsPtr = pgsGtwySegment->pgmGtwyMirrors; uiI < pgsGtwySegment->uiGtwyMirrorsLength; uiI++, pgmGtwyMirrorsPtr++ ) {
    
            /* Bail here if we found a gateway mirror that is already connected, note that this is a success */
            if ( pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_CONNECTED ) {
                iError = SPI_NoError;
                goto bailFromiGtwyOpenGatewaySegment;
            }
    
            /* Skip if the gateway mirror has a temporary or permanent error */
            if ( (pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR) || 
                    (pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_PERMANENT_ERROR) ) {
                continue;
            }

            /* Add this gateway mirror to the availability list and increment the availability list length */
            for ( uiJ = 0; uiJ < pgmGtwyMirrorsPtr->uiPriority; uiJ++ ) {
                puilAvailability[uiAvailabilityLength++] = uiI;
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewaySegment - uiAvailabilityLength: %u, uiI: %u.", uiAvailabilityLength, uiI); */
            }
        }


        /* Break here if there are no gateway mirrors left to connect to, 
        ** ie when the availability array length is 0 
        */
        if ( uiAvailabilityLength == 0 ) {
            iError = SPI_OpenIndexFailed;
            goto bailFromiGtwyOpenGatewaySegment;
        }

        
        /* If mirror affinity has been tested and it is set, we give priority to that gateway mirror */
        if ( (bMirrorAffinityTested == false) && (pgdGtwyIndex->iMirrorAffinity >= 0) && (pgdGtwyIndex->iMirrorAffinity <= pgsGtwySegment->uiGtwyMirrorsLength) &&
                ((pgsGtwySegment->pgmGtwyMirrors + pgdGtwyIndex->iMirrorAffinity)->uiCurrentState != GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR) && 
                ((pgsGtwySegment->pgmGtwyMirrors + pgdGtwyIndex->iMirrorAffinity)->uiCurrentState != GTWY_MIRROR_CONNECTION_STATE_PERMANENT_ERROR) ) {

/*            iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewaySegment - pgdGtwyIndex->iMirrorAffinity: %d.", pgdGtwyIndex->iMirrorAffinity); */

            /* Dereference the gateway mirror based on the affinity */
            pgmGtwyMirrorsPtr = pgsGtwySegment->pgmGtwyMirrors + pgdGtwyIndex->iMirrorAffinity;
        }
        else {
            /* Pick a gateway mirror to connect to at random from the availability list */
            iUtlRandGetRand(uiAvailabilityLength - 1, &uiAvailabilityIndex);

/*            iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewaySegment - uiAvailabilityIndex: %u.", uiAvailabilityIndex); */

            /* Dereference the gateway mirror from the availability list */
            pgmGtwyMirrorsPtr = pgsGtwySegment->pgmGtwyMirrors + puilAvailability[uiAvailabilityIndex];
        }

        /* The mirror affinity has been tested or has been found lacking, and we dont want to test it again */
        bMirrorAffinityTested = true;


/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewaySegment - pgmGtwyMirrorsPtr->pucCanonicalIndexName: '%s'.", pgmGtwyMirrorsPtr->pucCanonicalIndexName); */
        
        /* Open a connection to the gateway mirror, failure to connect will set the gateway mirror state to 'terminated' */
        if ( (iError = iGtwyOpenGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirrorsPtr)) == SPI_NoError ) {

            /* Log if we need to */
            if ( bLogConnectionSuccess == true ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Successfully connected to the mirror: '%s'.", pgmGtwyMirrorsPtr->pucCanonicalIndexName);
            }

            /* Bail out */
            iError = SPI_NoError;
            goto bailFromiGtwyOpenGatewaySegment;
        }
            
        iUtlLog((bGtwyIgnoreConnectionError(pgdGtwyIndex->uiFlags) == true) || (pgsGtwySegment->uiGtwyMirrorsLength > 1) ? 
                UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, "Failed to connect to the mirror: '%s', gtwy error: %d.", pgmGtwyMirrorsPtr->pucCanonicalIndexName, iError);
        
        /* Break here if this was the only gateway mirror we could connect to */
        if ( uiAvailabilityLength == 1 ) {
            iError = SPI_OpenIndexFailed;
            goto bailFromiGtwyOpenGatewaySegment;
        }

        /* Set the flag telling us that we need to log a successful connection since we had to skip one */
        bLogConnectionSuccess = true;
    }



    /* Bail label */
    bailFromiGtwyOpenGatewaySegment:


    /* Free the availability list */
    s_free(puilAvailability);

    /* Handle the error */
    if ( iError != SPI_NoError ) {

        /* Log the error */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Failed to connect to one of the segments of index: '%s', spi error: %d", pgdGtwyIndex->pucIndexName, iError); */
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyCloseGatewaySegment()

    Purpose:    Close the gateway segment.

    Parameters: pgsGtwySegment  gateway segment

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyCloseGatewaySegment
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex,
    struct gtwySegment *pgsGtwySegment
)
{

    unsigned int        uiI = 0;
    struct gtwyMirror   *pgmGtwyMirrorsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyCloseGatewaySegment"); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);
    ASSERT(pgsGtwySegment != NULL);


    /* Loop over the gateway mirrors */
    for ( uiI = 0, pgmGtwyMirrorsPtr = pgsGtwySegment->pgmGtwyMirrors; uiI < pgsGtwySegment->uiGtwyMirrorsLength; uiI++, pgmGtwyMirrorsPtr++ ) {

        /* Close the gateway mirror - ignore the error */
        iGtwyCloseGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirrorsPtr);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwySearchGatewaySegment()

    Purpose:    This function searches the gateway segment.

    Parameters: pgsgsGtwySearchGatewaySegment   gateway search gateway segment structure

    Globals:    none

    Returns:    SPI Error Code

*/
static int iGtwySearchGatewaySegment
(
    struct gtwySearchGatewaySegment *pgsgsGtwySearchGatewaySegment
)
{

    int                     iError = LWPS_NoError;
    struct gtwyGateway      *pggGtwyGateway = NULL;
    struct gtwyIndex        *pgdGtwyIndex = NULL;
    struct gtwySegment      *pgsGtwySegment = NULL;
    struct gtwyMirror       *pgmGtwyMirrorPtr = NULL;
    unsigned int            uiSearchTimeOut = 0;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;
    

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwySearchGatewaySegment"); */


    ASSERT(pgsgsGtwySearchGatewaySegment != NULL)


    /* Dereference for convenience */
    pggGtwyGateway = pgsgsGtwySearchGatewaySegment->pggGtwyGateway;
    pgdGtwyIndex = pgsgsGtwySearchGatewaySegment->pgdGtwyIndex;
    pgsGtwySegment = pgsgsGtwySearchGatewaySegment->pgsGtwySegment;


    /* Check to see if the gateway index is connected */
    if ( pgdGtwyIndex->uiCurrentState != GTWY_INDEX_CONNECTION_STATE_CONNECTED ) {
        return (SPI_InvalidIndex);
    }


    /* Select the search timeout we are going to use */
    if ( pgsgsGtwySearchGatewaySegment->uiSearchTimeOut > 0 ) {
        uiSearchTimeOut = pgsgsGtwySearchGatewaySegment->uiSearchTimeOut;
    }
    else if ( pgdGtwyIndex->uiSearchTimeOut > 0 ) {
        uiSearchTimeOut = pgdGtwyIndex->uiSearchTimeOut;
    }
    else {
        uiSearchTimeOut = pgsgsGtwySearchGatewaySegment->pggGtwyGateway->uiSearchTimeOut;
    }
    
    
    /* Loop forever, we control the loop from within, checking all the available gateway mirrors in this gateway segment */
    while ( true ) {

        struct gtwyOpenGatewaySegment   gogsGtwyOpenGatewaySegment;
        unsigned int                    uiI = 0;
        struct gtwyMirror               *pgmGtwyMirrorsPtr = NULL;
        unsigned char                   *ppucIndexNameList[2] = {NULL, NULL};
        
        /* Set the structure fields */
        gogsGtwyOpenGatewaySegment.pggGtwyGateway = pggGtwyGateway;
        gogsGtwyOpenGatewaySegment.pgdGtwyIndex = pgdGtwyIndex;
        gogsGtwyOpenGatewaySegment.pgsGtwySegment = pgsGtwySegment;
            
        /* Open the gateway segment, bail on error since there is no point searching if we are not connected */
        if ( (iError = iGtwyOpenGatewaySegment(&gogsGtwyOpenGatewaySegment)) != SPI_NoError ) {
            iError = SPI_SearchIndexFailed;
            goto bailFromiGtwySearchGatewaySegment;
        }
    
        /* Loop over the gateway mirrors in the gateway segment looking for the connected one */
        for ( uiI = 0, pgmGtwyMirrorPtr = NULL, pgmGtwyMirrorsPtr = pgsGtwySegment->pgmGtwyMirrors; uiI < pgsGtwySegment->uiGtwyMirrorsLength; uiI++, pgmGtwyMirrorsPtr++ ) {
        
            /* Grab the first one that is connected, set the gateway mirror pointer and break out */
            if ( pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_CONNECTED ) {
                pgmGtwyMirrorPtr = pgmGtwyMirrorsPtr;
                break;
            }
        }
    
        /* Check the gateway mirror pointer, this will be set to NULL if we could 
        ** not find a gateway mirror to run the search on
        */
        if ( pgmGtwyMirrorPtr == NULL ) {
            iError = SPI_SearchIndexFailed;
            goto bailFromiGtwySearchGatewaySegment;
        }

    
        /* Set the search timeout */
        if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorPtr->pvUtlNet, uiSearchTimeOut)) != LWPS_NoError ) {
            
            iUtlLog((bGtwyIgnoreSearchError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                    "Failed to set the net search timeout, index: '%s', utl error: %d.", pgmGtwyMirrorPtr->pucCanonicalIndexName, iError);
            
            iError = SPI_SearchIndexFailed;
            goto bailFromiGtwySearchGatewaySegment;
        }


        /* Attach the index name to the index name list */
        ppucIndexNameList[0] = pgmGtwyMirrorPtr->pucIndexName;
        ppucIndexNameList[1] = NULL;
    

        /* Handle the search */
        iError = iLwpsSearchRequestHandle(pgmGtwyMirrorPtr->pvLwps, ppucIndexNameList, pgsgsGtwySearchGatewaySegment->pucLanguageCode, 
                pgsgsGtwySearchGatewaySegment->pucSearchText, pgsgsGtwySearchGatewaySegment->pucPositiveFeedbackText, 
                pgsgsGtwySearchGatewaySegment->pucNegativeFeedbackText, pgsgsGtwySearchGatewaySegment->uiStartIndex, 
                pgsgsGtwySearchGatewaySegment->uiEndIndex, &pgsgsGtwySearchGatewaySegment->pssrSpiSearchResponse, 
                &iErrorCode, &pucErrorString);

        /* Break out here is there was no error, ie we got the search results */
        if ( (iError == LWPS_NoError) && (iErrorCode == SPI_NoError) ) {
            iError = SPI_NoError;
            goto bailFromiGtwySearchGatewaySegment;
        }


        /* We only warn because we will loop to try another mirror */
/*         if ( iError != LWPS_NoError ) { */
/*             iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, index: '%s', lwps error: %d.",  */
/*                     pgmGtwyMirrorPtr->pucCanonicalIndexName, iError); */
/*         } */
/*         else if ( iErrorCode != SPI_NoError ) { */
/*             iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, index: '%s', error code: %d, error text: '%s'.",  */
/*                     pgmGtwyMirrorPtr->pucCanonicalIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString)); */
/*         } */


        /* Close the gateway mirror */
        iGtwyCloseGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirrorPtr);
        
        /* Set the current state to temporary error as closing sets it to disconnected */
        pgmGtwyMirrorPtr->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR;
            
        /* Clear the gateway mirror pointer */
        pgmGtwyMirrorPtr = NULL;


        /* Break out if there was a timeout because we will have exceeded 
        ** the amount of time we were willing to wait for the search
        */
        if ( iError == LWPS_TimeOut ) {
            iError = SPI_SearchIndexFailed;
            goto bailFromiGtwySearchGatewaySegment;
        }
    }



    /* Bail label */
    bailFromiGtwySearchGatewaySegment:
    

    /* Handle the error */
    if ( (iError != SPI_NoError) || (iErrorCode != SPI_NoError) ) {

        /* Log the error */
        iUtlLog((bGtwyIgnoreConnectionError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                "Failed to run the search, index: '%s'.", (pgmGtwyMirrorPtr != NULL) ? 
                pgmGtwyMirrorPtr->pucIndexName : pgsGtwySegment->pgmGtwyMirrors->pucIndexName);

        /* Close the gateway mirror if it is connected */
        if ( pgmGtwyMirrorPtr != NULL ) {

            /* Close the gateway mirror */
            iGtwyCloseGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirrorPtr);
        
            /* Set the current state to temporary error as closing sets it to disconnected */
            pgmGtwyMirrorPtr->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR;
        }
    }

    /* Free any allocated strings */
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyRetrieveDocumentFromGatewaySegment()

    Purpose:    This function should return the text/data specified by the document
                ID from chunk = 'lStartChunk' to chunk <= 'lEndChunk'.

                An empty document item name indicates that the default document
                item should be retrieved.

                An empty document mime type indicates that the default document
                type should be retrieved.

                Using line chunks is not suitable for binary data such as images, maps, 
                photos, etc.

                The chunk start and end are meaningless if a document chunk type is
                requested.

    Parameters: pggGtwyGateway      gateway structure
                pgdGtwyIndex        gateway index structure
                pgsGtwySegment      gateway segment structure
                pgmGtwyMirror       gateway mirror structure (set if document is to be retrieved from a specific mirror)
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                uiChunkType         document chunk type
                uiChunkStart        start chunk
                uiChunkEnd          end chunk
                ppvData             return pointer of data returned
                puiDataLength       return pointer for length of data returned

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyRetrieveDocumentFromGatewaySegment
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex,
    struct gtwySegment *pgsGtwySegment,
    struct gtwyMirror *pgmGtwyMirror,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int                 iError = LWPS_NoError;
    unsigned int        uiRetrievalTimeOut = 0;
    struct gtwyMirror   *pgmGtwyMirrorPtr = NULL;
    int                 iErrorCode = SPI_NoError;
    unsigned char       *pucErrorString = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyRetrieveDocumentFromGatewaySegment [%s][%s][%s][%u][%u][%u]",  */
/*             pucDocumentKey, pucItemName, pucMimeType, uiChunkType, uiChunkStart, uiChunkEnd); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);
    ASSERT(pgsGtwySegment != NULL);
    ASSERT((pgmGtwyMirror != NULL) || (pgmGtwyMirror == NULL));
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(SPI_CHUNK_TYPE_VALID(uiChunkType) == true);
    ASSERT(uiChunkStart >= 0);
    ASSERT(uiChunkEnd >= 0);
    ASSERT(uiChunkEnd >= uiChunkStart);
    ASSERT(ppvData != NULL);
    ASSERT(puiDataLength != NULL);


    /* Check to see if the gateway index is connected */
    if ( pgdGtwyIndex->uiCurrentState != GTWY_INDEX_CONNECTION_STATE_CONNECTED ) {
        return (SPI_InvalidIndex);
    }


    /* Select the retrieval timeout we are going to use */
    if ( pgdGtwyIndex->uiRetrievalTimeOut > 0 ) { 
        uiRetrievalTimeOut = pgdGtwyIndex->uiRetrievalTimeOut;
    }
    else {
        uiRetrievalTimeOut = pggGtwyGateway->uiRetrievalTimeOut;
    }


    /* We need to get the document from a specific gateway mirror if the gateway mirror 
    ** was set, otherwise we get the document from the first gateway mirror which responds
    */ 
    if ( pgmGtwyMirror != NULL ) {

        /* Open a connection to the gateway mirror */
        if ( (iError = iGtwyOpenGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirror)) != SPI_NoError ) {
            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiGtwyGetDocumentFromGatewaySegment;
        }


        /* Set the gateway mirror pointer, we use this pointer to close this 
        ** mirror if we fail to retrieve the document from it 
        */
        pgmGtwyMirrorPtr = pgmGtwyMirror;


        /* Set the retrieval timeout */
        if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorPtr->pvUtlNet, uiRetrievalTimeOut)) != LWPS_NoError ) {

            iUtlLog((bGtwyIgnoreRetrievalError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                    "Failed to set the net information timeout, index: '%s', utl error: %d.", pgmGtwyMirrorPtr->pucCanonicalIndexName, iError);

            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiGtwyGetDocumentFromGatewaySegment;
        }

    
        /* Handle the retrieval request */
        if ( (iError = iLwpsRetrievalRequestHandle(pgmGtwyMirrorPtr->pvLwps, pgmGtwyMirrorPtr->pucIndexName, pucDocumentKey, pucItemName, 
                pucMimeType, uiChunkType, uiChunkStart, uiChunkEnd, ppvData, puiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

            iUtlLog((bGtwyIgnoreRetrievalError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                    "Failed to handle an lwps retrieval request, index: '%s', lwps error code: %d.", 
                    pgmGtwyMirrorPtr->pucCanonicalIndexName, iError);

            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiGtwyGetDocumentFromGatewaySegment;
        }


        /* Check the returned error */
        if ( iErrorCode != SPI_NoError ) {

            iUtlLog((bGtwyIgnoreRetrievalError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                    "Failed to handle an lwps retrieval request, index: '%s', error code: %d, error text: '%s'.", 
                    pgmGtwyMirrorPtr->pucCanonicalIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));

            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiGtwyGetDocumentFromGatewaySegment;
        }
    }
    else {

        /* Loop forever, we control the loop from within, checking all the available gateway mirrors in this gateway segment */
        while ( true ) {

            struct gtwyOpenGatewaySegment   gogsGtwyOpenGatewaySegment;
            unsigned int                    uiI = 0;
            struct gtwyMirror               *pgmGtwyMirrorsPtr = NULL;


            /* Set the structure fields */
            gogsGtwyOpenGatewaySegment.pggGtwyGateway = pggGtwyGateway;
            gogsGtwyOpenGatewaySegment.pgdGtwyIndex = pgdGtwyIndex;
            gogsGtwyOpenGatewaySegment.pgsGtwySegment = pgsGtwySegment;
    
            /* Make sure the gateway segment is open, bail on error since there is no point retrieving if we are not connected */
            if ( (iError = iGtwyOpenGatewaySegment(&gogsGtwyOpenGatewaySegment)) != SPI_NoError ) {
                iError = SPI_RetrieveDocumentFailed;
                goto bailFromiGtwyGetDocumentFromGatewaySegment;
            }
        

            /* Loop over the gateway mirrors in the gateway segment looking for the connected one */
            for ( uiI = 0, pgmGtwyMirrorPtr = NULL, pgmGtwyMirrorsPtr = pgsGtwySegment->pgmGtwyMirrors; uiI < pgsGtwySegment->uiGtwyMirrorsLength; uiI++, pgmGtwyMirrorsPtr++ ) {
            
                /* Grab the first one that is connected, set the gateway mirror pointer and break out */
                if ( pgmGtwyMirrorsPtr->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_CONNECTED ) {
                    pgmGtwyMirrorPtr = pgmGtwyMirrorsPtr;
                    break;
                }
            }
        

            /* Check the gateway mirror pointer, this will be set to NULL if we could 
            ** not find a gateway mirror to retrieve the document from 
            */
            if ( pgmGtwyMirrorPtr == NULL ) {
                iError = SPI_RetrieveDocumentFailed;
                goto bailFromiGtwyGetDocumentFromGatewaySegment;
            }
        

            /* Set the retrieval timeout */
            if ( (iError = iUtlNetSetTimeOut(pgmGtwyMirrorPtr->pvUtlNet, uiRetrievalTimeOut)) != LWPS_NoError ) {

                iUtlLog((bGtwyIgnoreRetrievalError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                        "Failed to set the net retrieval timeout, index: '%s', utl error: %d.", pgmGtwyMirrorPtr->pucCanonicalIndexName, iError);

                iError = SPI_RetrieveDocumentFailed;
                goto bailFromiGtwyGetDocumentFromGatewaySegment;
            }


            /* Handle the retrieval request */
            iError = iLwpsRetrievalRequestHandle(pgmGtwyMirrorPtr->pvLwps, pgmGtwyMirrorPtr->pucIndexName, pucDocumentKey, pucItemName, pucMimeType, 
                    uiChunkType, uiChunkStart, uiChunkEnd, ppvData, puiDataLength, &iErrorCode, &pucErrorString);

            /* Break out here is there was no error, ie we retrieved the document */
            if ( (iError == LWPS_NoError) && (iErrorCode == SPI_NoError) ) {
                iError = SPI_NoError;
                goto bailFromiGtwyGetDocumentFromGatewaySegment;
            }
    

            /* We only warn because we will loop to try another mirror */
            if ( iError != LWPS_NoError ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, index: '%s', lwps error: %d.", 
                        pgmGtwyMirrorPtr->pucCanonicalIndexName, iError);
            }
            else if ( iErrorCode != SPI_NoError ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, index: '%s', error code: %d, error text: '%s'.", 
                        pgmGtwyMirrorPtr->pucCanonicalIndexName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
            }


            /* Close the gateway mirror */
            iGtwyCloseGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirrorPtr);
            
            /* Set the current state to temporary error as closing sets it to disconnected */
            pgmGtwyMirrorPtr->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR;
            
            /* Clear the gateway mirror pointer */
            pgmGtwyMirrorPtr = NULL;

    
            /* Break out if there was a timeout because we will have exceeded 
            ** the amount of time we were willing to wait for the retrieval
            */
            if ( iError == LWPS_TimeOut ) {
                iError = SPI_RetrieveDocumentFailed;
                goto bailFromiGtwyGetDocumentFromGatewaySegment;
            }
        }
    }



    /* Bail label */
    bailFromiGtwyGetDocumentFromGatewaySegment:


    /* Handle the error */
    if ( (iError != SPI_NoError) || (iErrorCode != SPI_NoError) ) {

        /* Log the error */
        iUtlLog((bGtwyIgnoreRetrievalError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                "Failed to retrieve the document, index: '%s'.", (pgmGtwyMirrorPtr != NULL) ?
                        pgmGtwyMirrorPtr->pucIndexName : pgsGtwySegment->pgmGtwyMirrors->pucIndexName);

        /* Close the gateway mirror if it is connected */
        if ( pgmGtwyMirrorPtr != NULL ) {

            /* Close the gateway mirror */
            iGtwyCloseGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirrorPtr);
            
            /* Set the current state to temporary error as closing sets it to disconnected */
            pgmGtwyMirrorPtr->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR;
        }
    }

    /* Free any allocated strings */
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ==================================
** ===  Gateway Mirror Functions  ===
** ==================================
*/


/*

    Function:   iGtwyOpenGatewayMirror()

    Purpose:    This opens the connection to the gateway mirror.

    Parameters: pgdGtwyIndex    gateway index structure
                pgmGtwyMirror   gateway mirror structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyOpenGatewayMirror
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex,
    struct gtwyMirror *pgmGtwyMirror
)
{

    int             iError = SPI_NoError;
    unsigned int    uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_INVALID;
    int             iErrorCode = SPI_NoError;
    unsigned char   *pucErrorString = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyOpenGatewayMirror [%s]", pgmGtwyMirror->pucCanonicalIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);
    ASSERT(pgmGtwyMirror != NULL);


    /* Skip if the gateway mirror is already connected */
    if ( pgmGtwyMirror->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_CONNECTED ) {
        return (SPI_NoError);
    }


    /* Skip if the gateway mirror has a temporary error or a permanent error */
    if ( (pgmGtwyMirror->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR) || 
            (pgmGtwyMirror->uiCurrentState == GTWY_MIRROR_CONNECTION_STATE_PERMANENT_ERROR) ) {
        return (SPI_OpenIndexFailed);
    }
    

    /* Open a connection to the host */
    if ( (iError = iUtlNetCreateClient(pggGtwyGateway->uiLwpsNetworkProtocolID, pgmGtwyMirror->pucHostName, pgmGtwyMirror->iPort, 
            pgdGtwyIndex->uiConnectionTimeOut, &pgmGtwyMirror->pvUtlNet)) != UTL_NoError ) {

        iUtlLog((bGtwyIgnoreConnectionError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                "Failed to open a net client connection, network protocol: %u, host: '%s', port: %d, utl error: %d", 
                pggGtwyGateway->uiLwpsNetworkProtocolID, pgmGtwyMirror->pucHostName, pgmGtwyMirror->iPort, iError);

        uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_PERMANENT_ERROR;
        iError = SPI_OpenIndexFailed;
        goto bailFromiGtwyOpenGatewayMirror;
    }


    /* Create a LPWS handle */
    if ( (iError = iLwpsCreate(pgmGtwyMirror->pvUtlNet, &pgmGtwyMirror->pvLwps)) != LWPS_NoError ) {

        iUtlLog((bGtwyIgnoreConnectionError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                "Failed to create an lwps, host: '%s', lwps error: %d.", pgmGtwyMirror->pucHostName, iError);

        uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR;
        iError = SPI_OpenIndexFailed;
        goto bailFromiGtwyOpenGatewayMirror;
    }


    /* Perform a LWPS init exchange before anything else happens - note that this makes no sense if we are using UDP rather than TCP */
    if ( pggGtwyGateway->bLwpsSendInit == true ) {

        /* Send the init - note that we dont send a user name or a password */
        if (  (iError = iLwpsInitRequestHandle(pgmGtwyMirror->pvLwps, NULL, NULL, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

            iUtlLog((bGtwyIgnoreConnectionError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                    "Failed to handle an lwps init request, host : '%s', lwps error: %d.", pgmGtwyMirror->pucHostName, iError);

            uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR;
            iError = SPI_OpenIndexFailed;
            goto bailFromiGtwyOpenGatewayMirror;
        }
    
        /* Check the returned error */
        if ( iErrorCode != SPI_NoError ) {

            iUtlLog((bGtwyIgnoreConnectionError(pgdGtwyIndex->uiFlags) == true) ? UTL_LOG_LEVEL_WARN : UTL_LOG_LEVEL_ERROR, UTL_LOG_CONTEXT, 
                    "Failed to handle an lwps init request, host: '%s', error code: %d, error text: '%s'.", 
                    pgmGtwyMirror->pucHostName, iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));

            s_free(pucErrorString);
            uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_TEMPORARY_ERROR;
            iError = iErrorCode;
            goto bailFromiGtwyOpenGatewayMirror;
        }
    }



    /* Bail label */
    bailFromiGtwyOpenGatewayMirror:


    /* Handle the error */
    if ( (iError != SPI_NoError) || (iErrorCode != SPI_NoError) ) {
        
        /* Close the gateway mirror */
        iGtwyCloseGatewayMirror(pggGtwyGateway, pgdGtwyIndex, pgmGtwyMirror);
        
        /* Override the current state as closing sets it to disconnected unless there is a permanent error */
        if ( uiCurrentState != GTWY_MIRROR_CONNECTION_STATE_INVALID ) {
            pgmGtwyMirror->uiCurrentState = uiCurrentState;
        }

        /* Hand over the error code if error is set to SPI_NoError */
        if ( iError == SPI_NoError ) {
            iError = iErrorCode;
        }
    }
    else {
        /* Set the current state to connected */
        pgmGtwyMirror->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_CONNECTED;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyCloseGatewayMirror()

    Purpose:    Close the connection to the gateway mirror.

    Parameters: pggGtwyGateway  gateway structure
                pgdGtwyIndex    gateway index structure
                pgmGtwyMirror   gateway mirror structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyCloseGatewayMirror
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyIndex *pgdGtwyIndex,
    struct gtwyMirror *pgmGtwyMirror
)
{

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyCloseGatewayMirror [%s]", pgmGtwyMirror->pucCanonicalIndexName); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgdGtwyIndex != NULL);
    ASSERT(pgmGtwyMirror != NULL);


    /* Free the lwps structure */
    iLwpsFree(pgmGtwyMirror->pvLwps);
    pgmGtwyMirror->pvLwps = NULL;

    /* Free the net structure */
    iUtlNetFree(pgmGtwyMirror->pvUtlNet);
    pgmGtwyMirror->pvUtlNet = NULL;

    /* Set the state to disconnected if we are not in a state of permanent error */
    if ( pgmGtwyMirror->uiCurrentState != GTWY_MIRROR_CONNECTION_STATE_PERMANENT_ERROR ) {
        pgmGtwyMirror->uiCurrentState = GTWY_MIRROR_CONNECTION_STATE_DISCONNECTED;
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** ==============================
** ===  Info Cache Functions  ===
** ==============================
*/


/*

    Function:   iGtwyAddToInfoCache()

    Purpose:    Add an info entry to the info cache.

    Parameters: pggGtwyGateway          gateway structure
                pucCanonicalIndexName   canonical index name
                uiInfoCacheType         info cache type
                pvInfoCacheEntry        info cache entry
                uiInfoCacheEntryLength  info cache entry length

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyAddToInfoCache 
(
    struct gtwyGateway *pggGtwyGateway,
    unsigned char *pucCanonicalIndexName,
    unsigned int uiInfoCacheType, 
    void *pvInfoCacheEntry, 
    unsigned int uiInfoCacheEntryLength
)
{

    struct gtwyInfoCache    *pgicGtwyInfoCachePtr = NULL;
    unsigned int            uiI = 0;
    boolean                 bFoundFreeEntry = false;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAddToInfoCache '%s', %d", pucCanonicalIndexName, uiInfoCacheType); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucCanonicalIndexName) == false);
    ASSERT((GTWY_INFO_CACHE_TYPE_VALID(uiInfoCacheType) == true) && (uiInfoCacheType != GTWY_INFO_CACHE_TYPE_EMPTY));
    ASSERT(pvInfoCacheEntry != NULL);
    ASSERT(uiInfoCacheEntryLength >= 0);


    /* See if the entry is already in the info cache */
    if ( iGtwyGetFromInfoCache(pggGtwyGateway, pucCanonicalIndexName, uiInfoCacheType, NULL, NULL) == SPI_NoError ) {
        return (SPI_NoError);
    }
    

    /* Loop over the info cache looking for an empty entry */
    for ( pgicGtwyInfoCachePtr = pggGtwyGateway->pgicGtwyInfoCache, uiI = 0; uiI < pggGtwyGateway->uiGtwyInfoCacheLength; pgicGtwyInfoCachePtr++, uiI++ ) {
        
        /* Check the entry, break here if it is empty */
        if ( (pgicGtwyInfoCachePtr->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_INVALID) || (pgicGtwyInfoCachePtr->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_EMPTY) ) {
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAddToInfoCache - free entry: %d", uiI); */
            bFoundFreeEntry = true;
            break;
        }
    }


    /* Allocate space in the info cache if a free entry was not found */
    if ( bFoundFreeEntry == false ) {
        
        /* Extend the cache by one entry */
        if ( (pgicGtwyInfoCachePtr = (struct gtwyInfoCache *)s_realloc(pggGtwyGateway->pgicGtwyInfoCache, 
                (size_t)(sizeof(struct gtwyInfoCache) * (pggGtwyGateway->uiGtwyInfoCacheLength + 1)))) == NULL ) {
            return (SPI_MemError);
        }
    
        /* Hand over the pointer and increment the info cache length */
        pggGtwyGateway->pgicGtwyInfoCache = pgicGtwyInfoCachePtr;
        pggGtwyGateway->uiGtwyInfoCacheLength++;
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAddToInfoCache - added entry: %d", pggGtwyGateway->uiGtwyInfoCacheLength); */
    
        /* Dereference the pointer for convenience */    
        pgicGtwyInfoCachePtr = pggGtwyGateway->pgicGtwyInfoCache + (pggGtwyGateway->uiGtwyInfoCacheLength - 1);
    }
    
    
    /* Copy the data and pointers into the info cache */
    s_strnncpy(pgicGtwyInfoCachePtr->pucCanonicalIndexName, pucCanonicalIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);
    pgicGtwyInfoCachePtr->uiInfoCacheType = uiInfoCacheType;
    pgicGtwyInfoCachePtr->tInfoCacheTimeOut = s_time(NULL) + pggGtwyGateway->uiInformationCacheTimeOut;
    pgicGtwyInfoCachePtr->pvInfoCacheEntry = pvInfoCacheEntry;
    pgicGtwyInfoCachePtr->uiInfoCacheEntryLength = uiInfoCacheEntryLength;

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyAddToInfoCache - added to info cache: '%s', type: %d, cache length: %d",  */
/*             pucCanonicalIndexName, uiInfoCacheType, pggGtwyGateway->uiGtwyInfoCacheLength); */


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyGetFromInfoCache()

    Purpose:    Get an info entry from the info cache.

    Parameters: pggGtwyGateway              gateway structure
                pucCanonicalIndexName       canonical index name
                uiInfoCacheType             info cache type
                ppvInfoCacheEntry           return pointer for the info cache entry
                puiInfoCacheEntryLength     return pointer for the info cache entry length

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyGetFromInfoCache 
(
    struct gtwyGateway *pggGtwyGateway,
    unsigned char *pucCanonicalIndexName,
    unsigned int uiInfoCacheType, 
    void **ppvInfoCacheEntry, 
    unsigned int *puiInfoCacheEntryLength
)
{

    time_t                  tCurrentTime = (time_t)0;
    struct gtwyInfoCache    *pgicGtwyInfoCachePtr = NULL;
    unsigned int            uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyGetFromInfoCache '%s', %d", pucCanonicalIndexName, uiInfoCacheType); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucCanonicalIndexName) == false);
    ASSERT((GTWY_INFO_CACHE_TYPE_VALID(uiInfoCacheType) == true) && (uiInfoCacheType != GTWY_INFO_CACHE_TYPE_EMPTY));
    ASSERT( ((ppvInfoCacheEntry == NULL) && (puiInfoCacheEntryLength == NULL)) ||
            ((ppvInfoCacheEntry != NULL) && (puiInfoCacheEntryLength == NULL)) ||
            ((ppvInfoCacheEntry != NULL) && (puiInfoCacheEntryLength != NULL)) );


    /* Check that the info cache exists */
    if ( (pggGtwyGateway->pgicGtwyInfoCache == NULL) || (pggGtwyGateway->uiGtwyInfoCacheLength == 0) ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyGetFromInfoCache - empty info cache"); */
        return (SPI_MiscError);
    }

    
    /* Get the current time, do it here so we don't call time() over and over again */
    tCurrentTime = s_time(NULL); 
    
    
    /* Loop over the info cache looking for the entry */
    for ( pgicGtwyInfoCachePtr = pggGtwyGateway->pgicGtwyInfoCache, uiI = 0; uiI < pggGtwyGateway->uiGtwyInfoCacheLength; pgicGtwyInfoCachePtr++, uiI++ ) {
        
        /* Check the entry, check the info cache type first to save time */
        if ( (pgicGtwyInfoCachePtr->uiInfoCacheType != GTWY_INFO_CACHE_TYPE_INVALID) && (pgicGtwyInfoCachePtr->uiInfoCacheType != GTWY_INFO_CACHE_TYPE_EMPTY) && 
                (pgicGtwyInfoCachePtr->uiInfoCacheType == uiInfoCacheType) && (s_strcmp(pgicGtwyInfoCachePtr->pucCanonicalIndexName, pucCanonicalIndexName) == 0) ) {
            
            /* Free the info cache entry if there is a timeout set and if it has been reached, and return an error */
            if ( (pgicGtwyInfoCachePtr->tInfoCacheTimeOut > (time_t)0) && (pgicGtwyInfoCachePtr->tInfoCacheTimeOut <= tCurrentTime) ) {
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyGetFromInfoCache - info cache timeout: '%s', %d", pucCanonicalIndexName, uiInfoCacheType); */
                iGtwyFreeInfoCacheEntry(pggGtwyGateway, pgicGtwyInfoCachePtr);
                return (SPI_MiscError);
            }

            /* Set the return pointers and return */
            if ( ppvInfoCacheEntry != NULL ) {
                *ppvInfoCacheEntry = pgicGtwyInfoCachePtr->pvInfoCacheEntry;
            }
            if ( puiInfoCacheEntryLength != NULL ) {
                *puiInfoCacheEntryLength = pgicGtwyInfoCachePtr->uiInfoCacheEntryLength;
            }
        
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyGetFromInfoCache - got from info cache: '%s', %d", pucCanonicalIndexName, uiInfoCacheType); */

            return (SPI_NoError);
        }
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyGetFromInfoCache - not in info cache"); */


    return (SPI_MiscError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyFreeInfoCache()

    Purpose:    Free the info cache.

    Parameters: pggGtwyGateway  gateway structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyFreeInfoCache 
(
    struct gtwyGateway *pggGtwyGateway
)
{

    struct gtwyInfoCache    *pgicGtwyInfoCachePtr = NULL;
    unsigned int            uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyFreeInfoCache"); */


    ASSERT(pggGtwyGateway != NULL);


    /* Free the info cache if it exists */
    if ( pggGtwyGateway->pgicGtwyInfoCache != NULL ) {

        /* Loop over the info cache freeing the entries */
        for ( pgicGtwyInfoCachePtr = pggGtwyGateway->pgicGtwyInfoCache, uiI = 0; uiI < pggGtwyGateway->uiGtwyInfoCacheLength; pgicGtwyInfoCachePtr++, uiI++ ) {
            iGtwyFreeInfoCacheEntry(pggGtwyGateway, pgicGtwyInfoCachePtr);
        }

        /* Free the info cache */
        s_free(pggGtwyGateway->pgicGtwyInfoCache);

        /* Clear the pointers */
        pggGtwyGateway->pgicGtwyInfoCache = NULL;
        pggGtwyGateway->uiGtwyInfoCacheLength = 0;
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyFreeInfoCacheEntry()

    Purpose:    Free the info cache entry.

    Parameters: pggGtwyGateway  gateway structure

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyFreeInfoCacheEntry 
(
    struct gtwyGateway *pggGtwyGateway,
    struct gtwyInfoCache *pgicGtwyInfoCache
)
{

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iGtwyFreeInfoCacheEntry '%s', %d", pgicGtwyInfoCache->pucCanonicalIndexName, pgicGtwyInfoCache->uiInfoCacheType); */


    ASSERT(pggGtwyGateway != NULL);
    ASSERT(pgicGtwyInfoCache != NULL);


    /* Free the info cache entry */
    if ( pgicGtwyInfoCache->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_SERVER_INFO ) {
        iSpiFreeServerInfo((struct spiServerInfo *)pgicGtwyInfoCache->pvInfoCacheEntry);
    }
    else if ( pgicGtwyInfoCache->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_SERVER_INDEX_INFO ) {
        iSpiFreeServerIndexInfo((struct spiServerIndexInfo *)pgicGtwyInfoCache->pvInfoCacheEntry, pgicGtwyInfoCache->uiInfoCacheEntryLength);
    }
    else if ( pgicGtwyInfoCache->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_INDEX_INFO ) {
        iSpiFreeIndexInfo((struct spiIndexInfo *)pgicGtwyInfoCache->pvInfoCacheEntry);
    }
    else if ( pgicGtwyInfoCache->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_FIELD_INFO ) {
        iSpiFreeIndexFieldInfo((struct spiFieldInfo *)pgicGtwyInfoCache->pvInfoCacheEntry, pgicGtwyInfoCache->uiInfoCacheEntryLength);
    }
    else if ( pgicGtwyInfoCache->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_TERM_INFO ) {
        iSpiFreeTermInfo((struct spiTermInfo *)pgicGtwyInfoCache->pvInfoCacheEntry, pgicGtwyInfoCache->uiInfoCacheEntryLength);
    }
    else if ( pgicGtwyInfoCache->uiInfoCacheType == GTWY_INFO_CACHE_TYPE_DOCUMENT_INFO ) {
        iSpiFreeDocumentInfo((struct spiDocumentInfo *)pgicGtwyInfoCache->pvInfoCacheEntry);
    }
    else {
        ;
    }

    /* Clear the pointers */
    pgicGtwyInfoCache->pucCanonicalIndexName[0] = '\0';
    pgicGtwyInfoCache->uiInfoCacheType = GTWY_INFO_CACHE_TYPE_EMPTY;
    pgicGtwyInfoCache->tInfoCacheTimeOut = (time_t)0;
    pgicGtwyInfoCache->pvInfoCacheEntry = NULL;
    pgicGtwyInfoCache->uiInfoCacheEntryLength = 0;


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iGtwyGetSearchOptionsFromSearchText()

    Purpose:    Get the search options from the search text

    Parameters: pggGtwyGateway          gateway structure
                pucSearchText           search text
                ppgsoGtwySearchOptions  return pointer for the search options

    Globals:    none

    Returns:    SPI error code

*/
static int iGtwyGetSearchOptionsFromSearchText 
(
    struct gtwyGateway *pggGtwyGateway,
    unsigned char *pucSearchText,
    struct gtwySearchOptions **ppgsoGtwySearchOptions
)
{

    int                         iError = SPI_NoError;
    unsigned char               *pucSearchTextPtr = NULL;
    struct gtwySearchOptions    *pgsoGtwySearchOptions = NULL;
    
    
    ASSERT(pggGtwyGateway != NULL);
    ASSERT(ppgsoGtwySearchOptions != NULL);


    /* Allocate space for the sort field name */
    if ( (pgsoGtwySearchOptions = (struct gtwySearchOptions *)s_malloc(sizeof(struct gtwySearchOptions))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiGtwyGetSearchOptionsFromSearchText;
    }


    /* Set the defaults */
    s_strnncpy(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_RELEVANCE, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
    pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_ORDER_DESC_ID;

    pgsoGtwySearchOptions->uiConnectionTimeOut = 0;
    pgsoGtwySearchOptions->uiSearchTimeOut = 0;
    pgsoGtwySearchOptions->uiRetrievalTimeOut = 0;
    pgsoGtwySearchOptions->uiInformationTimeOut = 0;

    pgsoGtwySearchOptions->iMirrorAffinity = 0;

    pgsoGtwySearchOptions->uiMaximumSegmentsSearched = 0;
    pgsoGtwySearchOptions->uiMinimumSegmentsSearched = 0;


    /* Bail here if there was no search */
    if ( bUtlStringsIsStringNULL(pucSearchText) == true ) {
        iError = SPI_NoError;
        goto bailFromiGtwyGetSearchOptionsFromSearchText;
    }
    

    /* Process the {gtwy_early_completion:disable} modifier if we find it */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_EARLY_COMPLETION_DISABLE_STRING)) != NULL) || 
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_EARLY_COMPLETION_DISABLE_ABR_STRING)) != NULL) ) {

        pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_ORDER_INVALID_ID;
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "{gtwy_early_completion:disable} - pucSearchTextPtr: '%s', pgsoGtwySearchOptions->uiSortOrderID: %u.", pucSearchTextPtr, pgsoGtwySearchOptions->uiSortOrderID); */
    }

    /* Process the {sort:none} modifier if we find it */
    else if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_SORT_NONE_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_SORT_NONE_ABR_STRING)) != NULL) ) {

        pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_NONE_ID;
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "{nosort} - pucSearchTextPtr: '%s', uiSortOrderID: %u.", pucSearchTextPtr, uiSortOrderID); */
    }
    
    /* Process the {sort:...} modifier if we find it, look for the preamble */
    else if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_SORT_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_SORT_ABR_STRING)) != NULL) ) {
    
        unsigned char    pucScanfFormatHost[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
        unsigned char    pucSortOrderName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {'\0'};

        /* Create the scan format, to scan for strings sych as: '{sort:relevance:desc}' or '{sort:date:desc}' */
        snprintf(pucScanfFormatHost, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "{%%*[^:]:%%%d[^:]:%%%d[^}]}", SPI_FIELD_NAME_MAXIMUM_LENGTH, SPI_FIELD_NAME_MAXIMUM_LENGTH);
        
        /* Scan the sort clause */
        if ( sscanf(pucSearchTextPtr, pucScanfFormatHost, pgsoGtwySearchOptions->pucSortFieldName, pucSortOrderName) == 2 ) {

            /* Normalize the field name if it is abbreviated */
            if ( s_strcasecmp(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_ABR_RELEVANCE) == 0 ) {
                s_strnncpy(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_RELEVANCE, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
            }
            else if ( s_strcasecmp(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_ABR_RANK) == 0 ) {
                s_strnncpy(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_RANK, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
            }
            else if ( s_strcasecmp(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_ABR_DATE) == 0 ) {
                s_strnncpy(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_DATE, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
            }

            /* Get the sort order ID */
            if ( (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_ASC) == 0) || (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_ABR_ASC) == 0) ) {
                pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_ORDER_ASC_ID;
            }
            else if ( (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_DESC) == 0) || (s_strcasecmp(pucSortOrderName, GTWY_INDEX_SORT_ORDER_ABR_DESC) == 0) ) {
                pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_ORDER_DESC_ID;
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid sort order symbol: '%s', in sort clause, search : '%s'.", pucSortOrderName, pucSearchTextPtr);
                pgsoGtwySearchOptions->pucSortFieldName[0] = '\0';
                pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_ORDER_INVALID_ID;
            }
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid sort clause, search: '%s'.", pucSearchTextPtr);
            pgsoGtwySearchOptions->pucSortFieldName[0] = '\0';
            pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_ORDER_INVALID_ID;
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "{sort:...} - pgsoGtwySearchOptions->pucSortFieldName: '%s', pucSortOrderName: '%s', pgsoGtwySearchOptions->uiSortOrderID: %u.",  */
/*                 pgsoGtwySearchOptions->pucSortFieldName, pucSortOrderName, pgsoGtwySearchOptions->uiSortOrderID); */
    }
    
    /* Otherwise set defaults, sort by relevance in descending order */
    else {
        s_strnncpy(pgsoGtwySearchOptions->pucSortFieldName, GTWY_INDEX_SORT_FIELD_NAME_RELEVANCE, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
        pgsoGtwySearchOptions->uiSortOrderID = GTWY_INDEX_SORT_ORDER_DESC_ID;
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pucSortFieldName: '%s', uiSortOrderID: %u.", pucSortFieldName, uiSortOrderID); */
    }
    
    
    /* Process the {gtwy_connection_timeout:...} modifier */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_CONNECTION_TIMEOUT_STRING)) != NULL) || 
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_CONNECTION_TIMEOUT_ABR_STRING)) != NULL) ) {

        /* Set the connection timeout */
        if ( (pgsoGtwySearchOptions->uiConnectionTimeOut = s_strtol(s_strchr(pucSearchTextPtr, GTWY_MODIFIER_SEPARATOR_CHARACTER) + 1, NULL, 10)) <= 0 ) {
            pgsoGtwySearchOptions->uiConnectionTimeOut = 0;
        }
    }
    
    /* Process the {gtwy_search_timeout:...} modifier */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_SEARCH_TIMEOUT_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_SEARCH_TIMEOUT_ABR_STRING)) != NULL) ) {

        /* Set the search timeout */
        if ( (pgsoGtwySearchOptions->uiSearchTimeOut = s_strtol(s_strchr(pucSearchTextPtr, GTWY_MODIFIER_SEPARATOR_CHARACTER) + 1, NULL, 10)) <= 0 ) {
            pgsoGtwySearchOptions->uiSearchTimeOut = 0;
        }
    }

    /* Process the {gtwy_retrieval_timeout:...} modifier */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_RETRIEVAL_TIMEOUT_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_RETRIEVAL_TIMEOUT_ABR_STRING)) != NULL) ) {

        /* Check the retrieval timeout */
        if ( (pgsoGtwySearchOptions->uiRetrievalTimeOut = s_strtol(s_strchr(pucSearchTextPtr, GTWY_MODIFIER_SEPARATOR_CHARACTER) + 1, NULL, 10)) <= 0 ) {
            pgsoGtwySearchOptions->uiRetrievalTimeOut = 0;
        }
    }

    /* Process the {gtwy_information_timeout:...} modifier */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_INFORMATION_TIMEOUT_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_INFORMATION_TIMEOUT_ABR_STRING)) != NULL) ) {

        /* Check the information timeout */
        if ( (pgsoGtwySearchOptions->uiInformationTimeOut = s_strtol(s_strchr(pucSearchTextPtr, GTWY_MODIFIER_SEPARATOR_CHARACTER) + 1, NULL, 10)) <= 0 ) {
            pgsoGtwySearchOptions->uiInformationTimeOut = 0;
        }
    }


    /* Process the {gtwy_mirror_affinity:...} modifier */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_MIRROR_AFFINITY_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_MIRROR_AFFINITY_ABR_STRING)) != NULL) ) {

        /* Check the mirror affinity */
        if ( (pgsoGtwySearchOptions->iMirrorAffinity = s_strtol(s_strchr(pucSearchTextPtr, GTWY_MODIFIER_SEPARATOR_CHARACTER) + 1, NULL, 10)) < -1 ) {
            pgsoGtwySearchOptions->iMirrorAffinity = -1;
        }
    }


    /* Process the {gtwy_segments_searched_maximum:...} modifier */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_MAXIMUM_SEGMENTS_SEARCHED_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_MAXIMUM_SEGMENTS_SEARCHED_ABR_STRING)) != NULL) ) {

        /* Check the maximum segments searched */
        if ( (pgsoGtwySearchOptions->uiMaximumSegmentsSearched = s_strtol(s_strchr(pucSearchTextPtr, GTWY_MODIFIER_SEPARATOR_CHARACTER) + 1, NULL, 10)) <= 0 ) {
            pgsoGtwySearchOptions->uiMaximumSegmentsSearched = 0;
        }
    }

    /* Process the {gtwy_segments_searched_minimum:...} modifier */
    if ( ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_MINIMUM_SEGMENTS_SEARCHED_STRING)) != NULL) ||
            ((pucSearchTextPtr = s_strcasestr(pucSearchText, GTWY_MODIFIER_GTWY_MINIMUM_SEGMENTS_SEARCHED_ABR_STRING)) != NULL) ) {

        /* Check the minimum segments searched */
        if ( (pgsoGtwySearchOptions->uiMinimumSegmentsSearched = s_strtol(s_strchr(pucSearchTextPtr, GTWY_MODIFIER_SEPARATOR_CHARACTER) + 1, NULL, 10)) <= 0 ) {
            pgsoGtwySearchOptions->uiMinimumSegmentsSearched = 0;
        }
    }
    


    /* Bail label */
    bailFromiGtwyGetSearchOptionsFromSearchText:


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppgsoGtwySearchOptions = pgsoGtwySearchOptions;
    }
    else {

        /* Free allocations */
        s_free(pgsoGtwySearchOptions);
    }
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/
