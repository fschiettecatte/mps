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

    Module:     utils_err.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    5 June 1995

    Purpose:    This header file defines all the error codes which 
                can be returned by the utility functions.

                These error codes are organized in such a way that 
                anything other that a 0 is an error.

                Also they have been divided into error ranges as 
                follows:


                generic             -1 through    -99 
                alloc             -100 through   -199
                config            -200 through   -299
                date              -300 through   -399
                dict              -400 through   -499 
                file              -500 through   -599
                hash              -600 through   -699
                load              -700 through   -799
                log               -800 through   -899
                net               -900 through   -999
                sha1             -1000 through  -1099
                signals          -1100 through  -1199
                socket           -1200 through  -1299
                string buffer    -1300 through  -1399
                strings          -1400 through  -1499
                table            -1500 through  -1599
                data             -1600 through  -1699
                trie             -1700 through  -1799
                version          -1800 through  -1899


                Error code -1 is a generic error.

                Error codes -2 through -99 are reserved for future.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_UTILS_ERR_H)
#define UTL_UTILS_ERR_H


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

/* Generic */
#define UTL_NoError                                     (0)
#define UTL_MemError                                    (-1)
#define UTL_ParameterError                              (-2)
#define UTL_ReturnParameterError                        (-3)
#define UTL_MiscError                                   (-4)


/* Alloc */
#define UTL_AllocInvalidAllocationSize                  (-100)
#define UTL_AllocInvalidAllocator                       (-101)
#define UTL_AllocInvalidSize                            (-102)


/* Config */
#define UTL_ConfigInvalidFilePath                       (-200)
#define UTL_ConfigInvalidFileStatus                     (-201)
#define UTL_ConfigCreateFailed                          (-202)
#define UTL_ConfigInvalidConfig                         (-203)
#define UTL_ConfigInvalidConfigKey                      (-204)
#define UTL_ConfigInvalidConfigSubKey                   (-205)
#define UTL_ConfigInvalidConfigValue                    (-206)
#define UTL_ConfigInvalidMode                           (-207)
#define UTL_ConfigWriteFailed                           (-208)
#define UTL_ConfigOpenFailed                            (-209)
#define UTL_ConfigSeekFailed                            (-210)
#define UTL_ConfigSymbolNotFound                        (-211)


/* Date */
#define UTL_DateInvalidDate                             (-300)
#define UTL_DateInvalidTime                             (-301)
#define UTL_DateInvalidTimeT                            (-302)


/* Dict */
#define UTL_DictInvalidFilePath                         (-400)
#define UTL_DictInvalidKeyLength                        (-401)
#define UTL_DictCreateFailed                            (-402)
#define UTL_DictInvalidDict                             (-403)
#define UTL_DictInvalidKey                              (-404)
#define UTL_DictInvalidEntry                            (-405)
#define UTL_DictInvalidMode                             (-406)
#define UTL_DictWriteFailed                             (-407)
#define UTL_DictOpenFailed                              (-408)
#define UTL_DictMappingFailed                           (-409)
#define UTL_DictInvalidCallBackFunction                 (-407)
#define UTL_DictKeyNotFound                             (-408)


/* File */
#define UTL_FileInvalidFile                             (-500)
#define UTL_FileInvalidFileDescriptor                   (-501)
#define UTL_FileInvalidPath                             (-502)
#define UTL_FileStatFailed                              (-503)
#define UTL_FileUTimeFailed                             (-504)
#define UTL_FileOpenFailed                              (-505)
#define UTL_FileSeekFailed                              (-506)
#define UTL_FilePathIsNotName                           (-507)
#define UTL_FilePathIsNotPath                           (-508)
#define UTL_FilePathIsNotAbsolute                       (-509)
#define UTL_FilePathIsNotTilde                          (-510)

#define UTL_FileDirectoryCreateFailed                   (-520)

#define UTL_FileInvalidDirectoryPath                    (-530)
#define UTL_FileOpenDirectoryFailed                     (-531)
#define UTL_FileInvalidDirectoryEntryList               (-532)

#define UTL_FileInvalidOffset                           (-540)
#define UTL_FileInvalidSize                             (-541)
#define UTL_FileInvalidMapping                          (-542)


/* Hash */
#define UTL_HashInvalidHash                             (-600)
#define UTL_HashInvalidKey                              (-601)
#define UTL_HashInvalidCallBackFunction                 (-602)
#define UTL_HashKeyNotFound                             (-603)
#define UTL_HashAddKeyFailed                            (-604)


/* Load */
#define UTL_LoadFailedGetAverages                       (-700)
#define UTL_LoadUnsupportedGetAverages                  (-701)


/* Log */
#define UTL_LogInvalidLogFilePath                       (-800)
#define UTL_LogFileWriteDenied                          (-801)
#define UTL_LogDirectoryWriteDenied                     (-802)
#define UTL_LogInvalidLogOwnerName                      (-803)
#define UTL_LogFileNotRoot                              (-804)
#define UTL_LogInvalidUser                              (-805)
#define UTL_LogFailedToSetOwnership                     (-806)


/* Net */
#define UTL_NetInvalidNet                               (-900)
#define UTL_NetInvalidHostName                          (-901)
#define UTL_NetInvalidPort                              (-902)
#define UTL_NetInvalidProtocol                          (-903)
#define UTL_NetInvalidTimeOut                           (-904)
#define UTL_NetInvalidNetProtocol                       (-905)
#define UTL_NetInvalidBuffer                            (-906)
#define UTL_NetInvalidBufferLength                      (-907)

#define UTL_NetDuplicateProtocol                        (-910)
#define UTL_NetNotAServerService                        (-911)

#define UTL_NetOpenClientSocketFailed                   (-920)
#define UTL_NetOpenServerSocketFailed                   (-921)
#define UTL_NetAcceptClientFailed                       (-922)
#define UTL_NetCloseSocketFailed                        (-923)

#define UTL_NetSocketInvalidDescriptor                  (-930)
#define UTL_NetSocketPollError                          (-931)
#define UTL_NetSocketError                              (-932)

#define UTL_NetSocketNotReadyToSend                     (-940)
#define UTL_NetReadyToSendFailed                        (-941)
#define UTL_NetSocketNotReadyToReceive                  (-942)
#define UTL_NetReadyToReceiveFailed                     (-943)

#define UTL_NetInsufficientData                         (-950)
#define UTL_NetInsufficientBufferSpace                  (-951)
#define UTL_NetWouldBlock                               (-952)
#define UTL_NetSendDataFailed                           (-953)
#define UTL_NetReceiveDataFailed                        (-954)
#define UTL_NetTimeOut                                  (-955)
#define UTL_NetSocketClosed                             (-956)

#define UTL_NetGetSocketBufferLengthFailed              (-960)

#define UTL_NetInvalidHostAddress                       (-970)


/* SHA1 */
#define UTL_SHA1InvalidSHA1                             (-1000)
#define UTL_SHA1InvalidBuffer                           (-1001)
#define UTL_SHA1InvalidDigestBuffer                     (-1002)
#define UTL_SHA1InvalidHexDigestBuffer                  (-1003)


/* Signals */
#define UTL_SignalActionInstallFailed                   (-1100)


/* Socket */
#define UTL_SocketInvalidHostName                       (-1200)
#define UTL_SocketInvalidHostAddress                    (-1201)
#define UTL_SocketInvalidPort                           (-1202)
#define UTL_SocketInvalidSocket                         (-1203)
#define UTL_SocketInvalidSocketAddress                  (-1204)
#define UTL_SocketInvalidBlocking                       (-1205)
#define UTL_SocketOpenServerFailed                      (-1206)
#define UTL_SocketAcceptClientFailed                    (-1207)
#define UTL_SocketOpenClientFailed                      (-1208)
#define UTL_SocketCloseFailed                           (-1209)

#define UTL_SocketSetSocketOptionFailed                 (-1220)
#define UTL_SocketGetSocketOptionFailed                 (-1221)
#define UTL_SocketSetSocketBlockingStateFailed          (-1222)

#define UTL_SocketInvalidTimeOut                        (-1230)
#define UTL_SocketNotReadyToReceive                     (-1231)
#define UTL_SocketNotReadyToSend                        (-1232)
#define UTL_SocketPollError                             (-1233)
#define UTL_SocketError                                 (-1234)
#define UTL_SocketInvalidDescriptor                     (-1235)

#define UTL_SocketInvalidData                           (-1240)
#define UTL_SocketWouldBlock                            (-1241)
#define UTL_SocketSendFailed                            (-1242)
#define UTL_SocketPartialSend                           (-1243)
#define UTL_SocketReceiveFailed                         (-1244)
#define UTL_SocketPartialReceive                        (-1245)
#define UTL_SocketClosed                                (-1246)

#define UTL_SocketGetClientNameFailed                   (-1250)
#define UTL_SocketGetClientAddressFailed                (-1251)


/* StringBuffer */
#define UTL_StringBufferInvalidStringBuffer             (-1300)
#define UTL_StringBufferInvalidString                   (-1301)


/* Strings */
#define UTL_StringsInvalidString                        (-1400)
#define UTL_StringsInvalidReturnParameter               (-1401)


/* Table */
#define UTL_TableInvalidMode                            (-1500)
#define UTL_TableInvalidFilePath                        (-1501)
#define UTL_TableInvalidTable                           (-1502)
#define UTL_TableInvalidEntryLength                     (-1503)
#define UTL_TableInvalidData                            (-1504)
#define UTL_TableCreateFailed                           (-1505)
#define UTL_TableWriteFailed                            (-1506)
#define UTL_TableOpenFailed                             (-1507)
#define UTL_TableReadFailed                             (-1508)
#define UTL_TableInvalidTableEntryID                    (-1509)
#define UTL_TableInvalidCallBackFunction                (-1510)
#define UTL_TableMappingFailed                          (-1511)


/* Data */
#define UTL_DataInvalidMode                             (-1600)
#define UTL_DataInvalidFilePath                         (-1601)
#define UTL_DataInvalidData                             (-1602)
#define UTL_DataInvalidDataLength                       (-1603)
#define UTL_DataCreateFailed                            (-1604)
#define UTL_DataWriteFailed                             (-1605)
#define UTL_DataOpenFailed                              (-1606)
#define UTL_DataReadFailed                              (-1607)
#define UTL_DataInvalidDataEntryID                      (-1608)
#define UTL_DataInvalidCallBackFunction                 (-1609)
#define UTL_DataMappingFailed                           (-1610)


/* Trie */
#define UTL_TrieInvalidTrie                             (-1700)
#define UTL_TrieInvalidKey                              (-1701)
#define UTL_TrieInvalidCallBackFunction                 (-1702)
#define UTL_TrieKeyNotFound                             (-1703)
#define UTL_TrieAddKeyFailed                            (-1704)
#define UTL_TrieInvalidKeyLength                        (-1705)


/* Version */
#define UTL_VersionInvalidFileDescriptor                (-1800)
#define UTL_VersionInvalidLogContext                    (-1801)


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_UTILS_ERR_H) */


/*---------------------------------------------------------------------------*/
