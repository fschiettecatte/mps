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

    Module:     srch_err.h

    Author:     Francois Schiettecatte     (FS Consulting LLC.)

    Created:    13 September 1995

    Purpose:    This header file defines all the error codes which 
                can be returned by search engine functions.

                The numbers are set up to complement the SPI error 
                codes.

                Error codes below 1000 map search engine to SPI error 
                codes and can be returned as such.

                Error codes above and including 1000 are not mapped
                to SPI error code and are to be used as proper error
                codes.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_ERR_H)
#define SRCH_ERR_H

/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Mapped error codes */
#define SRCH_NoError                                                (SPI_NoError)
#define SRCH_MemError                                               (SPI_MemError)
#define SRCH_ParameterError                                         (SPI_ParameterError)
#define SRCH_ReturnParameterError                                   (SPI_ReturnParameterError)
#define SRCH_MiscError                                              (SPI_MiscError)
                                
#define SRCH_ExceededLoadMaximum                                    (SPI_ExceededLoadMaximum)
                            
#define SRCH_InvalidSession                                         (SPI_InvalidSession)
#define SRCH_InvalidIndexDirectory                                  (SPI_InvalidIndexDirectory)
#define SRCH_InvalidConfigurationDirectory                          (SPI_InvalidConfigurationDirectory)
#define SRCH_InvalidTemporaryDirectory                              (SPI_InvalidTemporaryDirectory)
#define SRCH_InitializeServerFailed                                 (SPI_InitializeServerFailed)
#define SRCH_ShutdownServerFailed                                   (SPI_ShutdownServerFailed)
                            
#define SRCH_InvalidIndex                                           (SPI_InvalidIndex)
#define SRCH_InvalidIndexName                                       (SPI_InvalidIndexName)
#define SRCH_OpenIndexFailed                                        (SPI_OpenIndexFailed)
#define SRCH_CloseIndexFailed                                       (SPI_CloseIndexFailed)
                            
#define SRCH_InvalidLanguageCode                                    (SPI_InvalidLanguageCode)
#define SRCH_InvalidSearchText                                      (SPI_InvalidSearchText)
#define SRCH_InvalidPositiveFeedbackText                            (SPI_InvalidPositiveFeedbackText)
#define SRCH_InvalidNegativeFeedbackText                            (SPI_InvalidNegativeFeedbackText)
#define SRCH_InvalidSearchResultsRange                              (SPI_InvalidSearchResultsRange)
#define SRCH_SearchIndexFailed                                      (SPI_SearchIndexFailed)
                            
#define SRCH_InvalidDocumentKey                                     (SPI_InvalidDocumentKey)
#define SRCH_InvalidItemName                                        (SPI_InvalidItemName)
#define SRCH_InvalidMimeType                                        (SPI_InvalidMimeType)
#define SRCH_InvalidChunkType                                       (SPI_InvalidChunkType)
#define SRCH_InvalidChunkRange                                      (SPI_InvalidChunkRange)
#define SRCH_RetrieveDocumentFailed                                 (SPI_RetrieveDocumentFailed)
                            
#define SRCH_GetServerInfoFailed                                    (SPI_GetServerInfoFailed)
                            
#define SRCH_GetServerIndexInfoFailed                               (SPI_GetServerIndexInfoFailed)
#define SRCH_ServerHasNoIndices                                     (SPI_ServerHasNoIndices)
                            
#define SRCH_GetIndexInfoFailed                                     (SPI_GetIndexInfoFailed)
                            
#define SRCH_GetIndexFieldInfoFailed                                (SPI_GetIndexFieldInfoFailed)
#define SRCH_IndexHasNoSearchFields                                 (SPI_IndexHasNoSearchFields)
                            
#define SRCH_InvalidTermMatch                                       (SPI_InvalidTermMatch)
#define SRCH_InvalidTermCase                                        (SPI_InvalidTermCase)
#define SRCH_InvalidTerm                                            (SPI_InvalidTerm)
#define SRCH_InvalidFieldName                                       (SPI_InvalidFieldName)
#define SRCH_GetIndexTermInfoFailed                                 (SPI_GetIndexTermInfoFailed)
#define SRCH_IndexHasNoTerms                                        (SPI_IndexHasNoTerms)
                            
#define SRCH_BadGetDocumentInfo                                     (SPI_GetDocumentInfoFailed)
                            
#define SRCH_BadGetIndexName                                        (SPI_GetIndexNameFailed)
                            
                            

/* Srch error codes */


/* Bitmap */
#define SRCH_BitmapInvalidBitmap                                    (-1000)
#define SRCH_BitmapInvalidName                                      (-1001)
                
                
/* Cache */                
#define SRCH_CacheInvalidSearch                                     (-1100)
#define SRCH_CacheInvalidConfig                                     (-1101)
#define SRCH_CacheInvalidCache                                      (-1102)
#define SRCH_CacheInvalidMode                                       (-1103)
#define SRCH_CacheInvalidType                                       (-1104)
#define SRCH_CacheInvalidParserTerm                                 (-1105)
#define SRCH_CacheInvalidWeightName                                 (-1106)
#define SRCH_CacheInvalidWeight                                     (-1107)
#define SRCH_CacheInvalidBitmapName                                 (-1108)
#define SRCH_CacheInvalidBitmap                                     (-1109)
#define SRCH_CacheCreateFailed                                      (-1110)
#define SRCH_CacheSaveFailed                                        (-1111)
#define SRCH_CacheGetFailed                                         (-1112)
#define SRCH_CacheLockFailed                                        (-1113)
#define SRCH_CacheUnlockFailed                                      (-1114)
#define SRCH_CacheSHA1Failed                                        (-1115)
#define SRCH_CacheGetFilePathFailed                                 (-1116)
#define SRCH_CacheCreateDirectoryFailed                             (-1117)
#define SRCH_CacheUnsupportedCache                                  (-1118)
                
                
/* Document */                
#define SRCH_DocumentInvalidDocumentID                              (-1200)
#define SRCH_DocumentInvalidDocumentTitle                           (-1201)
#define SRCH_DocumentInvalidDocumentKey                             (-1202)
#define SRCH_DocumentInvalidDocumentRank                            (-1203)
#define SRCH_DocumentInvalidDocumentTermCount                       (-1204)
#define SRCH_DocumentInvalidDocumentAnsiDate                        (-1205)
#define SRCH_DocumentInvalidDocumentLanguageID                      (-1206)
#define SRCH_DocumentInvalidDocumentObjectID                        (-1207)
#define SRCH_DocumentInvalidDocumentItems                           (-1208)
#define SRCH_DocumentInvalidDocumentItemsLength                     (-1209)
#define SRCH_DocumentInvalidItemID                                  (-1210)
#define SRCH_DocumentInvalidItemName                                (-1211)
#define SRCH_DocumentInvalidMimeType                                (-1212)
#define SRCH_DocumentInvalidOffsets                                 (-1213)
#define SRCH_DocumentInvalidData                                    (-1214)
#define SRCH_DocumentInvalidDataLength                              (-1215)
#define SRCH_DocumentInvalidURL                                     (-1216)
#define SRCH_DocumentInvalidFilePath                                (-1217)
                    
#define SRCH_DocumentGetNewDocumentIDFailed                         (-1220)
#define SRCH_DocumentCheckDocumentIDFailed                          (-1221)
                    
#define SRCH_DocumentGetDocumentDataEntryFailed                     (-1230)
#define SRCH_DocumentSaveDocumentDataEntryFailed                    (-1231)
                
#define SRCH_DocumentSaveDocumentTableEntryFailed                   (-1240)
#define SRCH_DocumentGetDocumentTableEntryFailed                    (-1241)


/* Feedback */                
#define SRCH_FeedbackInvalidSearch                                  (-1300)
#define SRCH_FeedbackInvalidFeedbackText                            (-1301)
#define SRCH_FeedbackInvalidDocumentID                              (-1302)
#define SRCH_FeedbackInvalidLanguageID                              (-1303)
#define SRCH_FeedbackCreateTokenizerFailed                          (-1304)
#define SRCH_FeedbackCreateStemmerFailed                            (-1305)
#define SRCH_FeedbackCreateTrieFailed                               (-1306)
#define SRCH_FeedbackTokenizationFailed                             (-1307)
#define SRCH_FeedbackStemmingFailed                                 (-1308)
#define SRCH_FeedbackCharacterSetConvertionFailed                   (-1309)
#define SRCH_FeedbackFailed                                         (-1310)
                
                
/* Filepaths */                
#define SRCH_FilePathsInvalidIndexPath                              (-1400)
#define SRCH_FilePathsInvalidVersion                                (-1401)
#define SRCH_FilePathsFailed                                        (-1402)
                
                
/* Filter */                
#define SRCH_FilterInvalidSearch                                    (-1500)
#define SRCH_FilterInvalidLanguageID                                (-1501)
#define SRCH_FilterInvalidSearchParserFilters                       (-1502)
#define SRCH_FilterInvalidSearchParserFiltersLength                 (-1503)
#define SRCH_FilterInvalidDocumentID                                (-1504)
#define SRCH_FilterInvalidBitmapMergeType                           (-1505)
#define SRCH_FilterCreateStemmerFailed                              (-1506)
#define SRCH_FilterStemmingFailed                                   (-1507)
#define SRCH_FilterCharacterSetConvertionFailed                     (-1508)
#define SRCH_FilterInvalidFilterFileLocation                        (-1509)
#define SRCH_FilterInvalidFilterFile                                (-1510)
#define SRCH_FilterFailed                                           (-1511)
                
                
/* Index */                
#define SRCH_IndexInvalidIndexName                                  (-1600)
#define SRCH_IndexInvalidIndexPath                                  (-1601)
#define SRCH_IndexInvalidIntent                                     (-1602)
#define SRCH_IndexInvalidIndexDirectoryPath                         (-1603)
#define SRCH_IndexInvalidConfigurationDirectoryPath                 (-1604)
#define SRCH_IndexInvalidIndex                                      (-1605)
#define SRCH_IndexCreateFailed                                      (-1606)
#define SRCH_IndexIndexFailed                                       (-1607)
#define SRCH_IndexSearchFailed                                      (-1608)
#define SRCH_IndexConfigFailed                                      (-1609)
#define SRCH_IndexOpenFailed                                        (-1610)
#define SRCH_IndexCloseFailed                                       (-1611)
#define SRCH_IndexDeleteFailed                                      (-1612)
#define SRCH_IndexRenameFailed                                      (-1613)
#define SRCH_IndexPurgeFailed                                       (-1614)
#define SRCH_IndexLockFailed                                        (-1615)
#define SRCH_IndexUnlockFailed                                      (-1616)
                
                
/* Indexer */                
#define SRCH_IndexerInvalidIndexer                                  (-1700)
#define SRCH_IndexerInvalidInputFile                                (-1701)
#define SRCH_IndexerReadFailed                                      (-1702)
#define SRCH_IndexerVersionFailed                                   (-1703)
#define SRCH_IndexerInvalidVersion                                  (-1704)
#define SRCH_IndexerIndexNameFailed                                 (-1705)
#define SRCH_IndexerInvalidIndexName                                (-1706)
#define SRCH_IndexerInvalidIndexDescription                         (-1707)
#define SRCH_IndexerLanguageFailed                                  (-1708)
#define SRCH_IndexerInvalidLanguage                                 (-1709)
#define SRCH_IndexerInvalidLanguageCode                             (-1710)
#define SRCH_IndexerInvalidCharacterSetName                         (-1711)
#define SRCH_IndexerInvalidTokenizerName                            (-1712)
#define SRCH_IndexerInvalidTag                                      (-1713)
#define SRCH_IndexerInvalidFieldNameTag                             (-1714)
#define SRCH_IndexerInvalidUnfieldedSearchFieldNamesTag             (-1715)
#define SRCH_IndexerInvalidLanguageCodeTag                          (-1716)
#define SRCH_IndexerInvalidDocumentTermTag                          (-1717)
#define SRCH_IndexerInvalidDocumentDateTag                          (-1718)
#define SRCH_IndexerInvalidDocumentTitleTag                         (-1719)
#define SRCH_IndexerInvalidDocumentItemTag                          (-1720)
#define SRCH_IndexerInvalidDocumentKeyTag                           (-1721)
#define SRCH_IndexerInvalidDocumentRankTag                          (-1722)
#define SRCH_IndexerInvalidDocumentTermCountTag                     (-1723)
#define SRCH_IndexerInvalidDocumentDeleteTag                        (-1724)
#define SRCH_IndexerInvalidDocumentEndTag                           (-1725)
#define SRCH_IndexerInvalidStreamEndTag                             (-1726)
#define SRCH_IndexerInvalidWarningsExceeded                         (-1727)
#define SRCH_IndexerInvalidBackgroundIndexName                      (-1728)
#define SRCH_IndexerFailed                                          (-1729)
                    

/* Info */                        
#define SRCH_InfoInvalidVersion                                     (-1800)
#define SRCH_InfoInvalidLanguageID                                  (-1801)
#define SRCH_InfoInvalidCharacterSetID                              (-1802)
#define SRCH_InfoInvalidTokenizerID                                 (-1803)
#define SRCH_InfoInvalidStemmerID                                   (-1804)
#define SRCH_InfoInvalidStopListID                                  (-1805)
#define SRCH_InfoInvalidStopListTypeID                              (-1806)
#define SRCH_InfoInvalidFieldID                                     (-1807)
#define SRCH_InfoInvalidFieldName                                   (-1807)
#define SRCH_InfoInvalidFieldType                                   (-1808)
#define SRCH_InfoInvalidFieldOptions                                (-1809)
#define SRCH_InfoInvalidUnfieldedSearchFieldNames                   (-1810)
#define SRCH_InfoInvalidItemID                                      (-1811)
#define SRCH_InfoInvalidItemName                                    (-1812)
#define SRCH_InfoInvalidMimeTypeName                                (-1813)
#define SRCH_InfoInvalidIndexDescription                            (-1814)
#define SRCH_InfoInvalidLastUpdateTime                              (-1815)
#define SRCH_InfoInvalidInformationFile                             (-1816)
#define SRCH_InfoInvalidSymbolName                                  (-1817)
#define SRCH_InfoSymbolNotFound                                     (-1818)
#define SRCH_InfoSymbolSetFailed                                    (-1819)
#define SRCH_InfoFailed                                             (-1820)
                        
                        
/* Invert */                                                    
#define SRCH_InvertInvalidLanguageCode                              (-1900)
#define SRCH_InvertInvalidLanguageID                                (-1901)
#define SRCH_InvertInvalidCharacterSetName                          (-1902)
#define SRCH_InvertInvalidCharacterSetID                            (-1903)
#define SRCH_InvertInvalidTokenizerName                             (-1904)
#define SRCH_InvertInvalidTokenizerID                               (-1905)
#define SRCH_InvertInvalidStemmerName                               (-1906)
#define SRCH_InvertInvalidStemmerID                                 (-1907)
#define SRCH_InvertInvalidStopListName                              (-1908)
#define SRCH_InvertInvalidStopListID                                (-1909)
#define SRCH_InvertInvalidMemorySizeMax                             (-1910)
#define SRCH_InvertInvalidTermLengthMin                             (-1911)
#define SRCH_InvertInvalidTermLengthMax                             (-1912)
#define SRCH_InvertInvalidTermLengths                               (-1913)
#define SRCH_InvertInvalidDocumentID                                (-1914)
#define SRCH_InvertInvalidTerm                                      (-1915)
#define SRCH_InvertInvalidFieldID                                   (-1916)
#define SRCH_InvertInvalidFieldType                                 (-1917)
#define SRCH_InvertInvalidFieldOptions                              (-1918)
#define SRCH_InvertInvalidTermPosition                              (-1919)
#define SRCH_InvertCreateStemmerFailed                              (-1920)
#define SRCH_InvertTermTruncationFailed                             (-1921)
#define SRCH_InvertCharacterSetConvertionFailed                     (-1922)
#define SRCH_InvertIndexBlockAllocationFailed                       (-1923)
#define SRCH_InvertAddTermInitFailed                                (-1924)
#define SRCH_InvertStemmingFailed                                   (-1925)
#define SRCH_InvertAddTermFailed                                    (-1926)
#define SRCH_InvertIndexBlockFlushFailed                            (-1927)
#define SRCH_InvertIndexBlockWriteFailed                            (-1928)
#define SRCH_InvertIndexBlockReadFailed                             (-1929)
#define SRCH_InvertIndexBlockReadEOF                                (-1930)
#define SRCH_InvertIndexBlockFreeFailed                             (-1931)
#define SRCH_InvertMergeFailed                                      (-1932)
#define SRCH_InvertBlockObjectGetFailed                             (-1933)
#define SRCH_InvertBlockObjectStoreFailed                           (-1934)
#define SRCH_InvertBlockObjectUpdateFailed                          (-1935)
                            
                            
/* Keydict */                                                
#define SRCH_KeyDictInvalidDocumentKey                              (-2000)
#define SRCH_KeyDictDocumentKeyNotFound                             (-2001)
#define SRCH_KeyDictDocumentKeyLookupFailed                         (-2002)
#define SRCH_KeyDictCreateFailed                                    (-2003)
#define SRCH_KeyDictIndexBlockWriteFailed                           (-2004)
#define SRCH_KeyDictIndexBlockReadFailed                            (-2005)
#define SRCH_KeyDictIndexBlockReadEOF                               (-2006)
#define SRCH_KeyDictMergeFailed                                     (-2007)
                    
                    
/* Language */                    
#define SRCH_LanguageInvalidLanguageCode                            (-2100)
#define SRCH_LanguageInvalidCharacterSetName                        (-2101)
#define SRCH_LanguageInvalidTokenizerName                           (-2102)
#define SRCH_LanguageInvalidLanguageID                              (-2103)
#define SRCH_LanguageInvalidCharacterSetID                          (-2104)
#define SRCH_LanguageInvalidTokenizerID                             (-2105)
                    
                    
/* Parser */                    
#define SRCH_ParserInvalidSearch                                    (-2200)
#define SRCH_ParserInitFailed                                       (-2201)
#define SRCH_ParserInvalidParser                                    (-2202)
#define SRCH_ParserInvalidParserTerm                                (-2203)
#define SRCH_ParserInvalidRange                                     (-2204)
#define SRCH_ParserInvalidOperator                                  (-2205)
#define SRCH_ParserInvalidModifier                                  (-2206)
#define SRCH_ParserInvalidFunction                                  (-2207)
#define SRCH_ParserInvalidToken                                     (-2208)
#define SRCH_ParserInvalidBracket                                   (-2209)
#define SRCH_ParserInvalidQuote                                     (-2210)
#define SRCH_ParserInvalidWildCard                                  (-2211)
#define SRCH_ParserInvalidSyntax                                    (-2212)
#define SRCH_ParserInvalidOperatorDistance                          (-2213)
#define SRCH_ParserInvalidNotOperator                               (-2214)
#define SRCH_ParserInvalidModifierID                                (-2215)
#define SRCH_ParserInvalidModifierClassID                           (-2216)
#define SRCH_ParserInvalidSort                                      (-2217)
#define SRCH_ParserInvalidSortOrder                                 (-2218)
#define SRCH_ParserInvalidDate                                      (-2219)
#define SRCH_ParserInvalidTermWeight                                (-2220)
#define SRCH_ParserInvalidFeedbackTermWeight                        (-2221)
#define SRCH_ParserInvalidFrequentTermCoverageThreshold             (-2222)
#define SRCH_ParserInvalidMinimumTermCount                          (-2223)
#define SRCH_ParserInvalidFeedbackMaximumTermPercentage             (-2224)
#define SRCH_ParserInvalidFeedbackMaximumTermCoverageThreshold      (-2225)
#define SRCH_ParserInvalidConnectionTimeout                         (-2226)
#define SRCH_ParserInvalidSearchTimeout                             (-2227)
#define SRCH_ParserInvalidRetrievalTimeout                          (-2228)
#define SRCH_ParserInvalidInformationTimeout                        (-2229)
#define SRCH_ParserInvalidSegmentsSearchedMaximum                   (-2230)
#define SRCH_ParserInvalidSegmentsSearchedMinimum                   (-2231)
#define SRCH_ParserInvalidExclusionFilter                           (-2232)
#define SRCH_ParserInvalidInclusionFilter                           (-2233)
#define SRCH_ParserInvalidLanguage                                  (-2234)
#define SRCH_ParserRegexNoMatch                                     (-2235)
#define SRCH_ParserRegexBadRpt                                      (-2236)
#define SRCH_ParserRegexBadBr                                       (-2237)
#define SRCH_ParserRegexBadPat                                      (-2238)
#define SRCH_ParserRegexEBrace                                      (-2239)
#define SRCH_ParserRegexEBrack                                      (-2240)
#define SRCH_ParserRegexERange                                      (-2241)
#define SRCH_ParserRegexECType                                      (-2242)
#define SRCH_ParserRegexECollate                                    (-2243)
#define SRCH_ParserRegexEParen                                      (-2244)
#define SRCH_ParserRegexESubReg                                     (-2245)
#define SRCH_ParserRegexEEscape                                     (-2246)
#define SRCH_ParserRegexESpace                                      (-2247)
#define SRCH_ParserRegexEEnd                                        (-2248)
#define SRCH_ParserRegexESize                                       (-2249)
#define SRCH_ParserRegexENoSys                                      (-2250)
#define SRCH_ParserRegexFailed                                      (-2251)
#define SRCH_ParserCharacterSetConversionFailed                     (-2252)
#define SRCH_ParserTokenizationFailed                               (-2253)
#define SRCH_ParserCreateTokenizerFailed                            (-2254)
#define SRCH_ParserFailed                                           (-2255)
                
                
/* Posting */                
#define SRCH_PostingInvalidPostingsList                             (-2300)
#define SRCH_PostingInvalidPostings                                 (-2301)
#define SRCH_PostingInvalidSearchBooleanModifier                    (-2302)
#define SRCH_PostingInvalidTermDistance                             (-2303)
                
                
/* Report */                
#define SRCH_ReportInvalidSearch                                    (-2400)
#define SRCH_ReportInvalidConfig                                    (-2401)
#define SRCH_ReportInvalidReport                                    (-2402)
#define SRCH_ReportInvalidMode                                      (-2403)
#define SRCH_ReportInvalidType                                      (-2404)
#define SRCH_ReportCreateFailed                                     (-2405)
#define SRCH_ReportCreateReportFailed                               (-2406)
#define SRCH_ReportCloseReportFailed                                (-2407)
#define SRCH_ReportInvalidFormat                                    (-2408)
#define SRCH_ReportGetReportIDFailed                                (-2409)
#define SRCH_ReportGetReportOffsetFailed                            (-2410)
#define SRCH_ReportInvalidReportOffset                              (-2411)
#define SRCH_ReportGetReportSnippetFailed                           (-2412)
#define SRCH_ReportInvalidReportToken                               (-2413)
#define SRCH_ReportGetFilePathFailed                                (-2414)
#define SRCH_ReportGetReportTextFailed                              (-2415)
#define SRCH_ReportCreateDirectoryFailed                            (-2416)
                                
                                
/* Retrieval */                
#define SRCH_RetrievalInvalidSearch                                 (-2500)
                
                
/* Search */                
#define SRCH_SearchCharacterSetConvertionFailed                     (-2600)
#define SRCH_SearchCreateStemmerFailed                              (-2601)
#define SRCH_SearchInvalidStemmerID                                 (-2602)
#define SRCH_SearchStemmingFailed                                   (-2603)
                
                
/* ShortResult */                
#define SRCH_ShortResultInvalidShortResults                         (-2700)
#define SRCH_ShortResultInvalidSortOrder                            (-2701)
#define SRCH_ShortResultInvalidIndices                              (-2702)
                
                
/* Stemmer */                
#define SRCH_StemmerInvalidStemmerName                              (-2800)
#define SRCH_StemmerInvalidStemmerID                                (-2801)
                
                
/* StopList */                
#define SRCH_StopListInitFailed                                     (-2900)
#define SRCH_StopListInvalidStopListName                            (-2901)
#define SRCH_StopListInvalidStopListID                              (-2902)
#define SRCH_StopListInvalidTypeID                                  (-2903)
#define SRCH_StopListCreateFailed                                   (-2904)
                
                
/* TermDict */                
#define SRCH_TermDictInitFailed                                     (-3000)
#define SRCH_TermDictInvalidTerm                                    (-3001)
#define SRCH_TermDictInvalidTermType                                (-3002)
#define SRCH_TermDictInvalidTermCount                               (-3003)
#define SRCH_TermDictInvalidDocumentCount                           (-3004)
#define SRCH_TermDictInvalidIndexBlockID                            (-3005)
#define SRCH_TermDictInvalidFieldIDBitmap                           (-3006)
#define SRCH_TermDictAddFailed                                      (-3007)
#define SRCH_TermDictUpdateFailed                                   (-3008)
#define SRCH_TermDictFinishFailed                                   (-3009)
#define SRCH_TermDictTermLookupFailed                               (-3010)
#define SRCH_TermDictInvalidTermMatch                               (-3011)
#define SRCH_TermDictInvalidRangeID                                 (-3012)
#define SRCH_TermDictTermNotFound                                   (-3013)
#define SRCH_TermDictTermDoesNotOccur                               (-3014)
#define SRCH_TermDictTermBadRange                                   (-3015)
#define SRCH_TermDictTermBadWildCard                                (-3016)
#define SRCH_TermDictTermSoundexFailed                              (-3017)
#define SRCH_TermDictTermPhonixFailed                               (-3018)
#define SRCH_TermDictTermMetaphoneFailed                            (-3019)
#define SRCH_TermDictTermTypoFailed                                 (-3020)
#define SRCH_TermDictCharacterSetConvertionFailed                   (-3021)
                
                
/* TermLen */                
#define SRCH_TermLenInvalidMax                                      (-3100)
#define SRCH_TermLenInvalidMin                                      (-3101)
#define SRCH_TermLenInvalid                                         (-3102)
                

/* TermSearch */
#define SRCH_TermSearchInvalidSearch                                (-3200)
#define SRCH_TermSearchInvalidTerm                                  (-3201)
#define SRCH_TermSearchInvalidWeight                                (-3202)
#define SRCH_TermSearchInvalidFieldIDBitMap                         (-3203)
#define SRCH_TermSearchInvalidFrequentTermCoverageThreshold         (-3204)
#define SRCH_TermSearchInvalidDocumentID                            (-3205)
#define SRCH_TermSearchGetObjectFailed                              (-3206)


/* Version */
#define SRCH_Version                                                (-3400)


/* Weight */
#define SRCH_WeightInvalidWeight                                    (-3500)



/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_ERR_H) */


/*---------------------------------------------------------------------------*/
